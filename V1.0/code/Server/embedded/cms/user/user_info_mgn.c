
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
#include <errno.h>
#endif

#include "common/gbldef.inc"
#include "common/gblfunc_proc.inc"
#include "common/gblconfig_proc.inc"
#include "common/log_proc.inc"

#include "user/user_srv_proc.inc"
#include "user/user_reg_proc.inc"

#include "device/device_info_mgn.inc"

/*----------------------------------------------*
 * 外部变量说明                                 *
 *----------------------------------------------*/
//extern int g_IsPay;                       /* 是否付费，默认1 */

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
User_Info_MAP g_UserInfoMap;              /* 用户信息队列 */
#ifdef MULTI_THR
osip_mutex_t* g_UserInfoMapLock = NULL;
#endif

/*----------------------------------------------*
 * 常量定义                                     *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 宏定义                                       *
 *----------------------------------------------*/

#if DECS("用户信息队列")
/*****************************************************************************
 函 数 名  : user_info_init
 功能描述  : 用户信息结构初始化
 输入参数  : user_info_t ** user_info
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月11日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int user_info_init(user_info_t** user_info)
{
    *user_info = new user_info_t;

    if (*user_info == NULL)
    {
        DEBUG_TRACE(MODULE_USER, LOG_DEBUG, "user_info_init() exit---: *user_info Smalloc Error \r\n");
        return -1;
    }

    (*user_info)->user_index = 0;
    (*user_info)->user_id[0] = '\0';
    (*user_info)->user_name[0] = '\0';
    (*user_info)->user_level = 0;
    (*user_info)->access_method = 0;
    (*user_info)->tcp_sock = -1;
    (*user_info)->tcp_keep_alive_sock = -1;
    (*user_info)->login_ip[0] = '\0';
    (*user_info)->login_port = 0;
    (*user_info)->reg_status = 0;
    (*user_info)->reg_info_index = -1;
    (*user_info)->auth_count = 0;
    (*user_info)->alarm_info_send_flag = 0;
    (*user_info)->tvwall_status_send_flag = 0;
    (*user_info)->strRegServerEthName[0] = '\0';
    (*user_info)->strRegServerIP[0] = '\0';
    (*user_info)->iRegServerPort = 5060;
    (*user_info)->strCallID[0] = '\0';
    (*user_info)->strRegGapIP[0] = '\0';
    return 0;
}

/*****************************************************************************
 函 数 名  : user_info_free
 功能描述  : 用户信息结构释放
 输入参数  : user_info_t * user_info
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月11日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void user_info_free(user_info_t* user_info)
{
    if (user_info == NULL)
    {
        //DEBUG_TRACE(MODULE_USER, LOG_DEBUG, "user_info_free() exit---: Param Error \r\n");
        return;
    }

    user_info->user_index = 0;
    memset(user_info->user_id, 0, MAX_ID_LEN + 4);
    memset(user_info->user_name, 0, MAX_32CHAR_STRING_LEN + 4);
    user_info->user_level = 0;
    user_info->access_method = 0;
    user_info->tcp_sock = -1;
    user_info->tcp_keep_alive_sock = -1;
    memset(user_info->login_ip, 0, MAX_IP_LEN);
    user_info->login_port = 0;
    user_info->reg_status = 0;
    user_info->reg_info_index = -1;
    user_info->auth_count = 0;
    user_info->alarm_info_send_flag = 0;
    user_info->tvwall_status_send_flag = 0;
    memset(user_info->strRegServerEthName, 0, MAX_IP_LEN);
    memset(user_info->strRegServerIP, 0, MAX_IP_LEN);
    user_info->iRegServerPort = 5060;
    memset(user_info->strCallID, 0, MAX_128CHAR_STRING_LEN + 4);
    memset(user_info->strRegGapIP, 0, MAX_IP_LEN);
    return;
}

/*****************************************************************************
 函 数 名  : user_info_list_init
 功能描述  : 用户信息队列初始化
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
int user_info_list_init()
{
    g_UserInfoMap.clear();

#ifdef MULTI_THR
    g_UserInfoMapLock = (osip_mutex_t*)osip_mutex_init();

    if (NULL == g_UserInfoMapLock)
    {
        DEBUG_TRACE(MODULE_USER, LOG_DEBUG, "user_info_list_init() exit---: User Info Map Lock Init Error \r\n");
        return -1;
    }

#endif

    return 0;
}

/*****************************************************************************
 函 数 名  : user_info_list_free
 功能描述  : 用户信息队列释放
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
void user_info_list_free()
{
    User_Info_Iterator Itr;
    user_info_t* pUserInfo = NULL;

    for (Itr = g_UserInfoMap.begin(); Itr != g_UserInfoMap.end(); Itr++)
    {
        pUserInfo = Itr->second;

        if (NULL != pUserInfo)
        {
            user_info_free(pUserInfo);
            delete pUserInfo;
            pUserInfo = NULL;
        }
    }

    g_UserInfoMap.clear();

#ifdef MULTI_THR

    if (NULL != g_UserInfoMapLock)
    {
        osip_mutex_destroy((struct osip_mutex*)g_UserInfoMapLock);
        g_UserInfoMapLock = NULL;
    }

#endif
    return;
}

/*****************************************************************************
 函 数 名  : user_info_list_lock
 功能描述  : 用户信息队列锁定
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
int user_info_list_lock()
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_UserInfoMapLock == NULL)
    {
        //DEBUG_TRACE(MODULE_USER, LOG_DEBUG, "user_info_list_lock() exit---: Param Error \r\n");
        return -1;
    }

    iRet = osip_mutex_lock((struct osip_mutex*)g_UserInfoMapLock);

#endif
    return iRet;
}

/*****************************************************************************
 函 数 名  : user_info_list_unlock
 功能描述  : 用户信息队列解锁
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
int user_info_list_unlock()
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_UserInfoMapLock == NULL)
    {
        //DEBUG_TRACE(MODULE_USER, LOG_DEBUG, "user_info_list_unlock() exit---: Param Error \r\n");
        return -1;
    }

    iRet = osip_mutex_unlock((struct osip_mutex*)g_UserInfoMapLock);

#endif
    return iRet;
}

/*****************************************************************************
 函 数 名  : debug_user_info_list_lock
 功能描述  : 用户信息队列锁定
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
int debug_user_info_list_lock(const char* file, int line, const char* func)
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_UserInfoMapLock == NULL)
    {
        DEBUG_TRACE(MODULE_USER, LOG_DEBUG, "debug_user_info_list_lock() exit---: Param Error \r\n");
        return -1;
    }

    iRet = osip_debug_mutex_lock((struct osip_mutex*)g_UserInfoMapLock, file, line, func);

#endif
    return iRet;
}

/*****************************************************************************
 函 数 名  : debug_user_info_list_unlock
 功能描述  : 用户信息队列解锁
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
int debug_user_info_list_unlock(const char* file, int line, const char* func)
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_UserInfoMapLock == NULL)
    {
        DEBUG_TRACE(MODULE_USER, LOG_DEBUG, "debug_user_info_list_unlock() exit---: Param Error \r\n");
        return -1;
    }

    iRet = osip_debug_mutex_unlock((struct osip_mutex*)g_UserInfoMapLock, file, line, func);

#endif
    return iRet;
}

/*****************************************************************************
 函 数 名  : user_info_add
 功能描述  : 添加用户信息到队列中
 输入参数  : char* user_id
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月11日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
//int user_info_add(char* user_id)
//{
//    user_info_t* pUserInfo = NULL;
//    int i = 0;

//    if (g_UserInfoList == NULL || user_id == NULL)
//    {
//        return -1;
//    }

//    i = user_info_init(&pUserInfo);

//    if (i != 0)
//    {
//        return -1;
//    }

//    pUserInfo->user_id = sgetcopy(user_id);

//    user_info_list_lock();
//    i = list_add(g_UserInfoList->pUserInfoList, pUserInfo, -1); /* add to list tail */

//    if (i == -1)
//    {
//        user_info_list_unlock();
//        user_info_free(pUserInfo);
//        sfree(pUserInfo);
//        return -1;
//    }

//    user_info_list_unlock();
//    return i;
//}

int user_info_add(user_info_t* user_info)
{
    int pos = -1;
    User_Info_Iterator Itr;

    if (user_info == NULL)
    {
        //DEBUG_TRACE(MODULE_USER, LOG_DEBUG, "user_info_add() exit---: Param Error \r\n");
        return -1;
    }

    USER_INFO_SMUTEX_LOCK();

    for (Itr = g_UserInfoMap.begin(); Itr != g_UserInfoMap.end(); Itr++)
    {
        pos = Itr->first;
    }

    pos = pos + 1;
    g_UserInfoMap[pos] = user_info;
    USER_INFO_SMUTEX_UNLOCK();

    //DEBUG_TRACE(MODULE_USER, LOG_TRACE, "user_info_add(): pos=%d \r\n", pos);
    return pos;
}

/*****************************************************************************
 函 数 名  : user_info_remove
 功能描述  : 从队列中移除用户信息
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
int user_info_remove(char* user_id, char* user_ip, int user_port)
{
    user_info_t* pUserInfo = NULL;
    User_Info_Iterator Itr;

    if (NULL == user_id || NULL == user_ip || user_port <= 0)
    {
        //DEBUG_TRACE(MODULE_USER, LOG_DEBUG, "user_info_remove() exit---: Param Error \r\n");
        return -1;
    }

    USER_INFO_SMUTEX_LOCK();

    if (g_UserInfoMap.size() <= 0)
    {
        USER_INFO_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_USER, LOG_DEBUG, "user_info_find() exit---: User Info Map NULL \n");
        return -1;
    }

    for (Itr = g_UserInfoMap.begin(); Itr != g_UserInfoMap.end(); Itr++)
    {
        pUserInfo = Itr->second;

        if ((NULL == pUserInfo) || (pUserInfo->user_id[0] == '\0'))
        {
            continue;
        }

        if ((sstrcmp(pUserInfo->user_id, user_id) == 0)
            && (sstrcmp(pUserInfo->login_ip, user_ip) == 0)
            && pUserInfo->login_port == user_port)
        {
            g_UserInfoMap.erase(Itr);
            user_info_free(pUserInfo);
            delete pUserInfo;
            pUserInfo = NULL;
            USER_INFO_SMUTEX_UNLOCK();
            return 0;
        }
    }

    USER_INFO_SMUTEX_UNLOCK();
    return -1;
}

/*****************************************************************************
 函 数 名  : user_info_find
 功能描述  : 从队列中查找用户信息
 输入参数  : char* user_id
                            char* user_ip
                            int user_port
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月16日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int user_info_find(char* user_id, char* user_ip, int user_port)
{
    user_info_t* pUserInfo = NULL;
    User_Info_Iterator Itr;

    if (NULL == user_id || NULL == user_ip || user_port <= 0)
    {
        //DEBUG_TRACE(MODULE_USER, LOG_DEBUG, "user_info_find() exit---: Param Error \r\n");
        return -1;
    }

    USER_INFO_SMUTEX_LOCK();

    if (g_UserInfoMap.size() <= 0)
    {
        USER_INFO_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_USER, LOG_TRACE, "user_info_find() exit---: User Info Map NULL:user_id=%s,user_ip=%s,user_port=%d \n", user_id, user_ip, user_port);
        return -1;
    }

    for (Itr = g_UserInfoMap.begin(); Itr != g_UserInfoMap.end(); Itr++)
    {
        pUserInfo = Itr->second;

        if ((NULL == pUserInfo) || (pUserInfo->user_id[0] == '\0'))
        {
            continue;
        }

        //DEBUG_TRACE(MODULE_USER, LOG_TRACE, "user_info_find(): User Info Map:pos=%d, user_id=%s,user_ip=%s,user_port=%d \n", Itr->first, pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);

        if ((sstrcmp(pUserInfo->user_id, user_id) == 0)
            && (sstrcmp(pUserInfo->login_ip, user_ip) == 0)
            && pUserInfo->login_port == user_port)
        {
            USER_INFO_SMUTEX_UNLOCK();
            return Itr->first;
        }
    }

    USER_INFO_SMUTEX_UNLOCK();
    return -1;
}

/*****************************************************************************
 函 数 名  : user_info_get_by_user_index
 功能描述  : 根据用户索引查找用户
 输入参数  : int user_index
 输出参数  : 无
 返 回 值  :    user_info_t
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年12月25日
    作    者   : 用户路由信息清理
    修改内容   : 新生成函数

*****************************************************************************/
user_info_t* user_info_get_by_user_index(unsigned int user_index)
{
    user_info_t* pUserInfo = NULL;
    User_Info_Iterator Itr;

    if (user_index <= 0)
    {
        //DEBUG_TRACE(MODULE_USER, LOG_DEBUG, "user_info_find_by_user_index() exit---: Param Error \r\n");
        return NULL;
    }

    USER_INFO_SMUTEX_LOCK();

    if (g_UserInfoMap.size() <= 0)
    {
        USER_INFO_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_USER, LOG_TRACE, "user_info_find_by_user_index() exit---: User Info Map NULL \n");
        return NULL;
    }

    for (Itr = g_UserInfoMap.begin(); Itr != g_UserInfoMap.end(); Itr++)
    {
        pUserInfo = Itr->second;

        if ((NULL == pUserInfo) || (pUserInfo->user_id[0] == '\0'))
        {
            continue;
        }

        if (pUserInfo->user_index == user_index)
        {
            USER_INFO_SMUTEX_UNLOCK();
            return pUserInfo;
        }
    }

    USER_INFO_SMUTEX_UNLOCK();
    return NULL;
}

/*****************************************************************************
 函 数 名  : user_info_get_by_user_index_and_ip
 功能描述  : 通过用户索引，ip地址和端口号获取用户信息
 输入参数  : unsigned int user_index
             char* login_ip
             int login_port
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年11月16日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
user_info_t* user_info_get_by_user_index_and_ip(unsigned int user_index, char* login_ip, int login_port)
{
    user_info_t* pUserInfo = NULL;
    User_Info_Iterator Itr;

    if (user_index <= 0 || NULL == login_ip || login_port <= 0)
    {
        //DEBUG_TRACE(MODULE_USER, LOG_DEBUG, "user_info_get_by_user_index_and_ip() exit---: Param Error \r\n");
        return NULL;
    }

    USER_INFO_SMUTEX_LOCK();

    if (g_UserInfoMap.size() <= 0)
    {
        USER_INFO_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_USER, LOG_TRACE, "user_info_get_by_user_index_and_ip() exit---: User Info Map NULL \n");
        return NULL;
    }

    for (Itr = g_UserInfoMap.begin(); Itr != g_UserInfoMap.end(); Itr++)
    {
        pUserInfo = Itr->second;

        if ((NULL == pUserInfo) || (pUserInfo->user_id[0] == '\0'))
        {
            continue;
        }

        if (pUserInfo->user_index == user_index
            && (sstrcmp(pUserInfo->login_ip, login_ip) == 0)
            && pUserInfo->login_port == login_port)
        {
            USER_INFO_SMUTEX_UNLOCK();
            return pUserInfo;
        }
    }

    USER_INFO_SMUTEX_UNLOCK();
    return NULL;
}

/*****************************************************************************
 函 数 名  : user_info_get_by_user_ip_and_port
 功能描述  : 通过IP地址和端口号获取用户信息
 输入参数  : char* login_ip
             int login_port
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年11月16日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
user_info_t* user_info_get_by_user_ip_and_port(char* login_ip, int login_port)
{
    user_info_t* pUserInfo = NULL;
    User_Info_Iterator Itr;

    if (NULL == login_ip || login_port <= 0)
    {
        //DEBUG_TRACE(MODULE_USER, LOG_DEBUG, "user_info_get_by_user_ip_and_port() exit---: Param Error \r\n");
        return NULL;
    }

    USER_INFO_SMUTEX_LOCK();

    if (g_UserInfoMap.size() <= 0)
    {
        USER_INFO_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_USER, LOG_TRACE, "user_info_get_by_user_ip_and_port() exit---: User Info Map NULL \n");
        return NULL;
    }

    for (Itr = g_UserInfoMap.begin(); Itr != g_UserInfoMap.end(); Itr++)
    {
        pUserInfo = Itr->second;

        if ((NULL == pUserInfo) || (pUserInfo->user_id[0] == '\0'))
        {
            continue;
        }

        if ((sstrcmp(pUserInfo->login_ip, login_ip) == 0)
            && pUserInfo->login_port == login_port)
        {
            USER_INFO_SMUTEX_UNLOCK();
            return pUserInfo;
        }
    }

    USER_INFO_SMUTEX_UNLOCK();
    return NULL;
}

/*****************************************************************************
 函 数 名  : user_info_get_by_user_id
 功能描述  : 根据用户ID获取用户信息
 输入参数  : char* user_id
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年4月1日 星期二
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
user_info_t* user_info_get_by_user_id(char* user_id)
{
    user_info_t* pUserInfo = NULL;
    User_Info_Iterator Itr;

    if (NULL == user_id)
    {
        //DEBUG_TRACE(MODULE_USER, LOG_DEBUG, "user_info_get_by_user_id() exit---: Param Error \r\n");
        return NULL;
    }

    USER_INFO_SMUTEX_LOCK();

    if (g_UserInfoMap.size() <= 0)
    {
        USER_INFO_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_USER, LOG_TRACE, "user_info_get_by_user_id() exit---: User Info Map NULL \n");
        return NULL;
    }

    for (Itr = g_UserInfoMap.begin(); Itr != g_UserInfoMap.end(); Itr++)
    {
        pUserInfo = Itr->second;

        if ((NULL == pUserInfo) || (pUserInfo->user_id[0] == '\0'))
        {
            continue;
        }

        if (sstrcmp(pUserInfo->user_id, user_id) == 0)
        {
            USER_INFO_SMUTEX_UNLOCK();
            return pUserInfo;
        }
    }

    USER_INFO_SMUTEX_UNLOCK();
    return NULL;
}

/*****************************************************************************
 函 数 名  : user_info_get_by_user_info
 功能描述  : 通过用户ID、IP地址和端口号获取用户信息
 输入参数  : char* login_ip
             int login_port
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年11月16日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
user_info_t* user_info_get_by_user_info(char* user_id, char* login_ip, int login_port)
{
    user_info_t* pUserInfo = NULL;
    User_Info_Iterator Itr;

    if (NULL == login_ip || login_port <= 0)
    {
        //DEBUG_TRACE(MODULE_USER, LOG_DEBUG, "user_info_get_by_user_info() exit---: Param Error \r\n");
        return NULL;
    }

    USER_INFO_SMUTEX_LOCK();

    if (g_UserInfoMap.size() <= 0)
    {
        USER_INFO_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_USER, LOG_TRACE, "user_info_get_by_user_info() exit---: User Info Map NULL \n");
        return NULL;
    }

    for (Itr = g_UserInfoMap.begin(); Itr != g_UserInfoMap.end(); Itr++)
    {
        pUserInfo = Itr->second;

        if ((NULL == pUserInfo) || (pUserInfo->user_id[0] == '\0'))
        {
            continue;
        }

        if ((sstrcmp(pUserInfo->user_id, user_id) == 0)
            && (sstrcmp(pUserInfo->login_ip, login_ip) == 0)
            && pUserInfo->login_port == login_port)
        {
            USER_INFO_SMUTEX_UNLOCK();
            return pUserInfo;
        }
    }

    USER_INFO_SMUTEX_UNLOCK();
    return NULL;
}

/*****************************************************************************
 函 数 名  : user_info_get
 功能描述  : 获取用户信息
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
user_info_t* user_info_get(int pos)
{
    user_info_t* pUserInfo = NULL;

    if (pos < 0)
    {
        DEBUG_TRACE(MODULE_USER, LOG_DEBUG, "user_info_get() exit---: Param Error \r\n");
        return NULL;
    }

    pUserInfo = g_UserInfoMap[pos];

    if (NULL == pUserInfo)
    {
        DEBUG_TRACE(MODULE_USER, LOG_ERROR, "user_info_get() exit---: User Info NULL \r\n");
        return NULL;
    }

    return pUserInfo;
}

/*****************************************************************************
 函 数 名  : free_user_reg_info_by_tcp_socket
 功能描述  : 释放用户信息里面的TCP Socket链接
 输入参数  : int tcp_socket
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年1月5日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int free_user_reg_info_by_tcp_socket(int tcp_socket)
{
    int i = 0;
    user_info_t* pTmpUserInfo = NULL;
    user_info_t* pUserInfo = NULL;
    User_Info_Iterator Itr;
    int iTCPType = 0;

    if (tcp_socket < 0)
    {
        return -1;
    }

    USER_INFO_SMUTEX_LOCK();

    if (g_UserInfoMap.size() <= 0)
    {
        USER_INFO_SMUTEX_UNLOCK();
        return -1;
    }

    for (Itr = g_UserInfoMap.begin(); Itr != g_UserInfoMap.end(); Itr++)
    {
        pTmpUserInfo = Itr->second;

        if ((NULL == pTmpUserInfo) || (pTmpUserInfo->user_id[0] == '\0'))
        {
            continue;
        }

        if (pTmpUserInfo->tcp_sock >= 0)
        {
            if (pTmpUserInfo->tcp_sock == tcp_socket)
            {
                pTmpUserInfo->tcp_sock = -1;

                if (pTmpUserInfo->tcp_keep_alive_sock >= 0)
                {
                    close(pTmpUserInfo->tcp_keep_alive_sock);
                    pTmpUserInfo->tcp_keep_alive_sock = -1;
                }

                pUserInfo = pTmpUserInfo;
                iTCPType = 1;

                break;
            }
        }
        else if (pTmpUserInfo->tcp_keep_alive_sock >= 0)
        {
            if (pTmpUserInfo->tcp_keep_alive_sock == tcp_socket)
            {
                pTmpUserInfo->tcp_keep_alive_sock = -1;
                pUserInfo = pTmpUserInfo;
                iTCPType = 2;
                break;
            }
        }
    }

    USER_INFO_SMUTEX_UNLOCK();

    if (NULL != pUserInfo)
    {
        if (1 == iTCPType)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "用户TCP连接断开, 主动注销登录:用户ID=%s, IP地址=%s, 端口号=%d, TCP Socket=%d", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, tcp_socket);
        }
        else
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "用户保活TCP连接断开, 主动注销登录:用户ID=%s, IP地址=%s, 端口号=%d, TCP Socket=%d", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, tcp_socket);
        }

        /* 移除协议栈的注册信息 */
        SIP_UASRemoveRegisterInfo(pUserInfo->reg_info_index);

        i = user_reg_msg_add(pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, pUserInfo->user_name, 0, pUserInfo->reg_info_index);

        if (i != 0)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "free_user_reg_info_by_tcp_socket() user_reg_msg_add Error:user_id=%s, login_ip=%s, login_port=%d \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
        }
        else
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "free_user_reg_info_by_tcp_socket() user_reg_msg_add OK:user_id=%s, login_ip=%s, login_port=%d \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
        }
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : is_user_tcp_socket_in_use
 功能描述  : 用户的TCP Soceket 是否使用了
 输入参数  : int tcp_socket
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年1月5日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int is_user_tcp_socket_in_use(int tcp_socket)
{
    user_info_t* pTmpUserInfo = NULL;
    User_Info_Iterator Itr;

    if (tcp_socket < 0)
    {
        return 0;
    }

    USER_INFO_SMUTEX_LOCK();

    if (g_UserInfoMap.size() <= 0)
    {
        USER_INFO_SMUTEX_UNLOCK();
        return 0;
    }

    for (Itr = g_UserInfoMap.begin(); Itr != g_UserInfoMap.end(); Itr++)
    {
        pTmpUserInfo = Itr->second;

        if ((NULL == pTmpUserInfo) || (pTmpUserInfo->user_id[0] == '\0'))
        {
            continue;
        }

        if (pTmpUserInfo->tcp_sock >= 0)
        {
            if (pTmpUserInfo->tcp_sock == tcp_socket)
            {
                USER_INFO_SMUTEX_UNLOCK();
                return 1;
            }
        }

        if (pTmpUserInfo->tcp_keep_alive_sock >= 0)
        {
            if (pTmpUserInfo->tcp_keep_alive_sock == tcp_socket)
            {
                USER_INFO_SMUTEX_UNLOCK();
                return 1;
            }
        }

    }

    USER_INFO_SMUTEX_UNLOCK();
    return 0;
}
#endif

/*****************************************************************************
 函 数 名  : IsMyUser
 功能描述  : 是否是本级CMS的用户
 输入参数  : char* user_id
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年2月11日 星期二
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int IsMyUser(char* user_id)
{
    user_info_t* pUserInfo = NULL;
    User_Info_Iterator Itr;

    if (NULL == user_id)
    {
        //DEBUG_TRACE(MODULE_USER, LOG_DEBUG, "IsMyUser() exit---: Param Error \r\n");
        return 0;
    }

    USER_INFO_SMUTEX_LOCK();

    if (g_UserInfoMap.size() <= 0)
    {
        USER_INFO_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_USER, LOG_TRACE, "IsMyUser() exit---: User Info Map NULL \n");
        return 0;
    }

    for (Itr = g_UserInfoMap.begin(); Itr != g_UserInfoMap.end(); Itr++)
    {
        pUserInfo = Itr->second;

        if ((NULL == pUserInfo) || (pUserInfo->user_id[0] == '\0'))
        {
            continue;
        }

        if (sstrcmp(pUserInfo->user_id, user_id) == 0)
        {
            USER_INFO_SMUTEX_UNLOCK();
            return 1;
        }
    }

    USER_INFO_SMUTEX_UNLOCK();
    return 0;
}

/*****************************************************************************
 函 数 名  : IsUserRegInfoHasChange
 功能描述  : 用户注册信息是否有变化
 输入参数  : int pos
                            char* pcLoginIP
                            int iLoginPort
                            int iRegInfoIndex
 输出参数  : 无
 返 回 值  :int
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年6月20日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int IsUserRegInfoHasChange(user_info_t* pUserInfo, char* pcLoginIP, int iLoginPort, int iRegInfoIndex)
{
    if (NULL == pUserInfo)
    {
        DEBUG_TRACE(MODULE_USER, LOG_DEBUG, "IsUserRegInfoHasChange() exit---: Param Error \r\n");
        return 0;
    }

    /* 设备注册索引 */
    if (pUserInfo->reg_info_index >= 0 && iRegInfoIndex >= 0)
    {
        if (pUserInfo->reg_info_index != iRegInfoIndex)
        {
            DEBUG_TRACE(MODULE_USER, LOG_TRACE, "IsUserRegInfoHasChange() exit---: User Register Info Index Change \r\n");
            return 1;
        }
    }

    /* 设备登录端口 */
    if (pUserInfo->login_port > 0 && iLoginPort > 0)
    {
        if (pUserInfo->login_port != iLoginPort)
        {
            DEBUG_TRACE(MODULE_USER, LOG_TRACE, "IsUserRegInfoHasChange() exit---: User Register Info Port Change \r\n");
            return 1;
        }
    }

    /* 设备登录IP 地址 */
    if (NULL != pUserInfo->login_ip && NULL != pcLoginIP)
    {
        if (0 != sstrcmp(pUserInfo->login_ip, pcLoginIP))
        {
            DEBUG_TRACE(MODULE_USER, LOG_TRACE, "IsUserRegInfoHasChange() exit---: User Register Info IP Change \r\n");
            return 1;
        }
    }

    DEBUG_TRACE(MODULE_USER, LOG_TRACE, "IsUserRegInfoHasChange() exit---: User Register Info No Change \r\n");
    return 0;
}

/*****************************************************************************
 函 数 名  : SendMessageToOnlineUser
 功能描述  : 发送Message给所有在线的客户端用户
 输入参数  : char* msg
             int msg_len
             int is_alarm
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年5月27日 星期一
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int SendMessageToOnlineUser(char* msg, int msg_len, int is_alarm)
{
    int i = 0;
    int index = -1;
    int user_pos = -1;
    user_info_t* pUserInfo = NULL;
    User_Info_Iterator Itr;
    vector<int> UserPosVector;

    EV9000_TCP_Head stTCPHead;
    EV9000_TCP_Data stTCPData;

    USER_INFO_SMUTEX_LOCK();

    if (g_UserInfoMap.size() <= 0)
    {
        USER_INFO_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_USER, LOG_DEBUG, "SendMessageToOnlineUser() exit---: g_UserInfoMap Size Error \r\n");
        return 0;
    }

    UserPosVector.clear();

    for (Itr = g_UserInfoMap.begin(); Itr != g_UserInfoMap.end(); Itr++)
    {
        pUserInfo = Itr->second;

        if ((NULL == pUserInfo) || (pUserInfo->user_id[0] == '\0') || (pUserInfo->reg_info_index == -1))
        {
            continue;
        }

        if (pUserInfo->reg_status != 2)
        {
            continue;
        }

        /* 如果是告警消息，还需要查看用户的告警发送标识 */
        if (is_alarm)
        {
            if (pUserInfo->alarm_info_send_flag != 1)
            {
                continue;
            }
        }

        UserPosVector.push_back(Itr->first);
    }

    USER_INFO_SMUTEX_UNLOCK();

    if ((int)UserPosVector.size() == 0)
    {
        return 0;
    }

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "发送Message给在线的客户端用户: 在线用户数=%d", (int)UserPosVector.size());
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Send the Message to the client user online: online users = % d", (int)UserPosVector.size());

    if (UserPosVector.size() > 0)
    {
        for (index = 0; index < (int)UserPosVector.size(); index++)
        {
            user_pos = UserPosVector[index];

            pUserInfo = user_info_get(user_pos);

            if ((NULL == pUserInfo) || (pUserInfo->user_id[0] == '\0') || (pUserInfo->reg_info_index == -1))
            {
                continue;
            }

            if (pUserInfo->reg_status != 2)
            {
                continue;
            }

            if (is_alarm)
            {
                if (pUserInfo->alarm_info_send_flag != 1)
                {
                    continue;
                }
            }

            if (pUserInfo->tcp_sock >= 0)
            {
                /* 用结构体发送出去 */
                memset(&stTCPHead, 0, sizeof(EV9000_TCP_Head));
                stTCPHead.mark = '$';
                stTCPHead.length = htons(msg_len);

                memset(&stTCPData, 0, sizeof(EV9000_TCP_Data));
                memcpy(&stTCPData.stTCPHead, &stTCPHead, sizeof(EV9000_TCP_Head));
                memcpy(&stTCPData.stTCPBody, msg, msg_len);

                i = send(pUserInfo->tcp_sock, &stTCPData, sizeof(EV9000_TCP_Head) + msg_len, 0);

                if (i > 0)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "通过TCP通道发送Message消息到在线用户成功:在线用户ID=%s, IP地址=%s, 端口号=%d, 用户Socket=%d", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, pUserInfo->tcp_sock);
                }
                else
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "通过TCP通道发送Message消息到在线用户失败:在线用户ID=%s, IP地址=%s, 端口号=%d, 用户Socket=%d, i=%d", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, pUserInfo->tcp_sock, i);
                }
            }
            else
            {
                /* 发送响应消息 */
                i |= SIP_SendMessage(NULL, local_cms_id_get(), pUserInfo->user_id, pUserInfo->strRegServerIP, pUserInfo->iRegServerPort, pUserInfo->login_ip, pUserInfo->login_port, msg, msg_len);

                if (i != 0)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "发送Message消息到在线用户失败:在线用户ID=%s, IP地址=%s, 端口号=%d", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Send the Message to the client user online failure:Online user ID = % s, = % s IP address, port number = % d", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
                    DEBUG_TRACE(MODULE_USER, LOG_ERROR, "SendMessageToOnlineUser() SIP_SendMessage Error:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
                }
                else
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "发送Message消息到在线用户成功:在线用户ID=%s, IP地址=%s, 端口号=%d", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Send the Message to the client user online success:Online user ID = % s, = % s IP address, port number = % d", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
                    DEBUG_TRACE(MODULE_USER, LOG_TRACE, "SendMessageToOnlineUser() SIP_SendMessage OK:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
                }
            }
        }
    }

    UserPosVector.clear();

    return 1;
}

/*****************************************************************************
 函 数 名  : SendMessageToOnlineUser2
 功能描述  : 发送消息到在线用户，判断用户是否有该点位权限
 输入参数  : unsigned int device_index
             char* msg
             int msg_len
             int is_alarm
             DBOper* pDboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年8月15日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int SendMessageToOnlineUser2(unsigned int device_index, char* msg, int msg_len, int is_alarm, DBOper* pDboper)
{
    int i = 0;
    int index = -1;
    int user_pos = -1;
    user_info_t* pUserInfo = NULL;
    User_Info_Iterator Itr;
    vector<int> UserPosVector;

    EV9000_TCP_Head stTCPHead;
    EV9000_TCP_Data stTCPData;

    if (msg == NULL || NULL == pDboper)
    {
        return -1;
    }

    USER_INFO_SMUTEX_LOCK();

    if (g_UserInfoMap.size() <= 0)
    {
        USER_INFO_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_USER, LOG_DEBUG, "SendMessageToOnlineUser2() exit---: g_UserInfoMap Size Error \r\n");
        return 0;
    }

    UserPosVector.clear();

    for (Itr = g_UserInfoMap.begin(); Itr != g_UserInfoMap.end(); Itr++)
    {
        pUserInfo = Itr->second;

        if ((NULL == pUserInfo) || (pUserInfo->user_id[0] == '\0') || (pUserInfo->reg_info_index == -1))
        {
            continue;
        }

        if (pUserInfo->reg_status != 2)
        {
            continue;
        }

        /* 如果是告警消息，还需要查看用户的告警发送标识 */
        if (is_alarm)
        {
            if (pUserInfo->alarm_info_send_flag != 1)
            {
                continue;
            }
        }

        UserPosVector.push_back(Itr->first);
    }

    USER_INFO_SMUTEX_UNLOCK();

    if ((int)UserPosVector.size() == 0)
    {
        return 0;
    }

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "发送Message给在线的客户端用户: 在线用户数=%d", (int)UserPosVector.size());
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Send the Message to the client user online: online users = % d", (int)UserPosVector.size());

    if (UserPosVector.size() > 0)
    {
        for (index = 0; index < (int)UserPosVector.size(); index++)
        {
            user_pos = UserPosVector[index];

            pUserInfo = user_info_get(user_pos);

            if ((NULL == pUserInfo) || (pUserInfo->user_id[0] == '\0') || (pUserInfo->reg_info_index == -1))
            {
                continue;
            }

            if (pUserInfo->reg_status != 2)
            {
                continue;
            }

            if (is_alarm)
            {
                if (pUserInfo->alarm_info_send_flag != 1)
                {
                    continue;
                }
            }

            if (0 != sstrcmp(pUserInfo->user_name, (char*)"WiscomV") && 0 != sstrcmp(pUserInfo->user_name, (char*)"admin"))
            {
                /* 判断用户是否有该点位的权限 */
                if (!IsUserHasPermissionForDevice(device_index, pUserInfo, pDboper))
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "发送Message消息到在线用户失败:在线用户ID=%s, IP地址=%s, 端口号=%d, 点位索引=%u, 用户没有改点位的权限", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Send the Message to the client user online failure:Online user ID = % s, = % s IP address, port number = % d, point index = % u, users do not have permission to change point ", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
                    continue;
                }
            }

            if (pUserInfo->tcp_sock >= 0)
            {
                /* 用结构体发送出去 */
                memset(&stTCPHead, 0, sizeof(EV9000_TCP_Head));
                stTCPHead.mark = '$';
                stTCPHead.length = htons(msg_len);

                memset(&stTCPData, 0, sizeof(EV9000_TCP_Data));
                memcpy(&stTCPData.stTCPHead, &stTCPHead, sizeof(EV9000_TCP_Head));
                memcpy(&stTCPData.stTCPBody, msg, msg_len);

                i = send(pUserInfo->tcp_sock, &stTCPData, sizeof(EV9000_TCP_Head) + msg_len, 0);

                if (i > 0)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "通过TCP通道发送Message消息到在线用户成功:在线用户ID=%s, IP地址=%s, 端口号=%d, 用户Socket=%d", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, pUserInfo->tcp_sock);
                }
                else
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "通过TCP通道发送Message消息到在线用户失败:在线用户ID=%s, IP地址=%s, 端口号=%d, 用户Socket=%d, i=%d", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, pUserInfo->tcp_sock, i);
                }
            }
            else
            {
                /* 发送响应消息 */
                i |= SIP_SendMessage(NULL, local_cms_id_get(), pUserInfo->user_id, pUserInfo->strRegServerIP, pUserInfo->iRegServerPort, pUserInfo->login_ip, pUserInfo->login_port, msg, msg_len);

                if (i != 0)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "发送Message消息到在线用户失败:在线用户ID=%s, IP地址=%s, 端口号=%d", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Send the Message to the client user online failure:Online user ID = % s, = % s IP address, port number = % d", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
                    DEBUG_TRACE(MODULE_USER, LOG_ERROR, "SendMessageToOnlineUser2() SIP_SendMessage Error:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
                }
                else
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "发送Message消息到在线用户成功:在线用户ID=%s, IP地址=%s, 端口号=%d", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Send the Message to the client user online success:Online user ID = % s, = % s IP address, port number = % d", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
                    DEBUG_TRACE(MODULE_USER, LOG_TRACE, "SendMessageToOnlineUser2() SIP_SendMessage OK:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
                }
            }
        }
    }

    UserPosVector.clear();

    return 1;
}

/*****************************************************************************
 函 数 名  : SendMessageToOnlineUserForTVWallStatus
 功能描述  : 发送电视墙状态Message消息给所有在线的客户端用户
 输入参数  : char* msg
             int msg_len
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年1月6日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int SendMessageToOnlineUserForTVWallStatus(char* msg, int msg_len)
{
    int i = 0;
    int index = -1;
    int user_pos = -1;
    user_info_t* pUserInfo = NULL;
    User_Info_Iterator Itr;
    vector<int> UserPosVector;

    EV9000_TCP_Head stTCPHead;
    EV9000_TCP_Data stTCPData;

    USER_INFO_SMUTEX_LOCK();

    if (g_UserInfoMap.size() <= 0)
    {
        USER_INFO_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_USER, LOG_DEBUG, "SendMessageToOnlineUserForTVWallStatus() exit---: g_UserInfoMap Size Error \r\n");
        return 0;
    }

    UserPosVector.clear();

    for (Itr = g_UserInfoMap.begin(); Itr != g_UserInfoMap.end(); Itr++)
    {
        pUserInfo = Itr->second;

        if ((NULL == pUserInfo) || (pUserInfo->user_id[0] == '\0') || (pUserInfo->reg_info_index == -1))
        {
            continue;
        }

        if (pUserInfo->reg_status != 2)
        {
            continue;
        }

        if (pUserInfo->tvwall_status_send_flag != 1)
        {
            continue;
        }

        UserPosVector.push_back(Itr->first);
    }

    USER_INFO_SMUTEX_UNLOCK();

    if ((int)UserPosVector.size() == 0)
    {
        return 0;
    }

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "发送电视墙状态Message消息给在线的客户端用户: 在线用户数=%d", (int)UserPosVector.size());
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Send the Message to the client user online: online users = % d", (int)UserPosVector.size());

    if (UserPosVector.size() > 0)
    {
        for (index = 0; index < (int)UserPosVector.size(); index++)
        {
            user_pos = UserPosVector[index];

            pUserInfo = user_info_get(user_pos);

            if ((NULL == pUserInfo) || (pUserInfo->user_id[0] == '\0') || (pUserInfo->reg_info_index == -1))
            {
                continue;
            }

            if (pUserInfo->reg_status != 2)
            {
                continue;
            }

            if (pUserInfo->tvwall_status_send_flag != 1)
            {
                continue;
            }

            if (pUserInfo->tcp_sock >= 0)
            {
                /* 用结构体发送出去 */
                memset(&stTCPHead, 0, sizeof(EV9000_TCP_Head));
                stTCPHead.mark = '$';
                stTCPHead.length = htons(msg_len);

                memset(&stTCPData, 0, sizeof(EV9000_TCP_Data));
                memcpy(&stTCPData.stTCPHead, &stTCPHead, sizeof(EV9000_TCP_Head));
                memcpy(&stTCPData.stTCPBody, msg, msg_len);

                i = send(pUserInfo->tcp_sock, &stTCPData, sizeof(EV9000_TCP_Head) + msg_len, 0);

                if (i > 0)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "通过TCP通道发送电视墙状态Message消息到在线用户成功:在线用户ID=%s, IP地址=%s, 端口号=%d, 用户Socket=%d", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, pUserInfo->tcp_sock);
                }
                else
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "通过TCP通道发送电视墙状态Message消息到在线用户失败:在线用户ID=%s, IP地址=%s, 端口号=%d, 用户Socket=%d, i=%d", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, pUserInfo->tcp_sock, i);
                }
            }
            else
            {
                /* 发送响应消息 */
                i |= SIP_SendMessage(NULL, local_cms_id_get(), pUserInfo->user_id, pUserInfo->strRegServerIP, pUserInfo->iRegServerPort, pUserInfo->login_ip, pUserInfo->login_port, msg, msg_len);

                if (i != 0)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "发送电视墙状态Message消息到在线用户失败:在线用户ID=%s, IP地址=%s, 端口号=%d", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Send the Message to the client user online failure:Online user ID = % s, = % s IP address, port number = % d", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
                    DEBUG_TRACE(MODULE_USER, LOG_ERROR, "SendMessageToOnlineUserForTVWallStatus() SIP_SendMessage Error:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
                }
                else
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "发送电视墙状态Message消息到在线用户成功:在线用户ID=%s, IP地址=%s, 端口号=%d", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Send the Message to the client user online success:Online user ID = % s, = % s IP address, port number = % d", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
                    DEBUG_TRACE(MODULE_USER, LOG_TRACE, "SendMessageToOnlineUserForTVWallStatus() SIP_SendMessage OK:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
                }
            }
        }
    }

    UserPosVector.clear();

    return 1;
}

/*****************************************************************************
 函 数 名  : SendMessageToExceptOnlineUser
 功能描述  : 发送Message给所有在线的除了特定的用户意外的客户端用户
 输入参数  : user_info_t* pExceptUserInfo
             char* caller_id
             char* msg
             int msg_len
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年10月31日 星期五
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int SendMessageToExceptOnlineUser(user_info_t* pExceptUserInfo, char* msg, int msg_len)
{
    int i = 0;
    int index = -1;
    int user_pos = -1;
    user_info_t* pUserInfo = NULL;
    User_Info_Iterator Itr;
    vector<int> UserPosVector;

    EV9000_TCP_Head stTCPHead;
    EV9000_TCP_Data stTCPData;

    if (NULL == pExceptUserInfo)
    {
        //DEBUG_TRACE(MODULE_USER, LOG_DEBUG, "SendMessageToExceptOnlineUser() exit---: Param Error \r\n");
        return -1;
    }

    USER_INFO_SMUTEX_LOCK();

    if (g_UserInfoMap.size() <= 0)
    {
        USER_INFO_SMUTEX_UNLOCK();
        return 0;
    }

    UserPosVector.clear();

    for (Itr = g_UserInfoMap.begin(); Itr != g_UserInfoMap.end(); Itr++)
    {
        pUserInfo = Itr->second;

        if ((NULL == pUserInfo) || (pUserInfo->user_id[0] == '\0') || (pUserInfo->reg_info_index == -1))
        {
            continue;
        }

        if (pUserInfo->reg_status != 2)
        {
            continue;
        }

        if (0 == sstrcmp(pExceptUserInfo->user_id, pUserInfo->user_id)
            && 0 == sstrcmp(pExceptUserInfo->login_ip, pUserInfo->login_ip)
            && pExceptUserInfo->login_port == pUserInfo->login_port)
        {
            continue;
        }

        UserPosVector.push_back(Itr->first);
    }

    USER_INFO_SMUTEX_UNLOCK();

    if ((int)UserPosVector.size() == 0)
    {
        return 0;
    }

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "发送Message给在线的客户端用户: 在线用户数=%d", (int)UserPosVector.size());
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Send the Message to the client user online: online users = % d", (int)UserPosVector.size());

    if (UserPosVector.size() > 0)
    {
        for (index = 0; index < (int)UserPosVector.size(); index++)
        {
            user_pos = UserPosVector[index];

            pUserInfo = user_info_get(user_pos);

            if ((NULL == pUserInfo) || (pUserInfo->user_id[0] == '\0') || (pUserInfo->reg_info_index == -1))
            {
                continue;
            }

            if (pUserInfo->reg_status != 2)
            {
                continue;
            }

            if (pUserInfo->tcp_sock >= 0)
            {
                /* 用结构体发送出去 */
                memset(&stTCPHead, 0, sizeof(EV9000_TCP_Head));
                stTCPHead.mark = '$';
                stTCPHead.length = htons(msg_len);

                memset(&stTCPData, 0, sizeof(EV9000_TCP_Data));
                memcpy(&stTCPData.stTCPHead, &stTCPHead, sizeof(EV9000_TCP_Head));
                memcpy(&stTCPData.stTCPBody, msg, msg_len);

                i = send(pUserInfo->tcp_sock, &stTCPData, sizeof(EV9000_TCP_Head) + msg_len, 0);

                if (i > 0)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "通过TCP通道发送Message消息到在线用户成功:在线用户ID=%s, IP地址=%s, 端口号=%d, 用户Socket=%d", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, pUserInfo->tcp_sock);
                }
                else
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "通过TCP通道发送Message消息到在线用户失败:在线用户ID=%s, IP地址=%s, 端口号=%d, 用户Socket=%d, i=%d", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, pUserInfo->tcp_sock, i);
                }
            }
            else
            {
                /* 发送响应消息 */
                i |= SIP_SendMessage(NULL, local_cms_id_get(), pUserInfo->user_id, pUserInfo->strRegServerIP, pUserInfo->iRegServerPort, pUserInfo->login_ip, pUserInfo->login_port, msg, msg_len);

                if (i != 0)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "发送Message消息到在线用户失败:在线用户ID=%s, IP地址=%s, 端口号=%d", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Send Message to the online user failure: online user ID = % s, IP address = % s, port = % d", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
                    DEBUG_TRACE(MODULE_USER, LOG_ERROR, "SendMessageToExceptOnlineUser() SIP_SendMessage Error:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
                }
                else
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "发送Message消息到在线用户成功:在线用户ID=%s, IP地址=%s, 端口号=%d", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Send message to online user success: online user ID = % s, IP address = % s, port = % d", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
                    DEBUG_TRACE(MODULE_USER, LOG_TRACE, "SendMessageToExceptOnlineUser() SIP_SendMessage OK:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
                }
            }
        }
    }

    UserPosVector.clear();

    return 1;
}

/*****************************************************************************
 函 数 名  : SendMessageToOnlineUserByUserID
 功能描述  : 根据用户ID 发送Message给所有在线的客户端用户
 输入参数  : char* user_id
             char* msg
             int msg_len
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年5月27日 星期一
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int SendMessageToOnlineUserByUserID(char* user_id, char* msg, int msg_len)
{
    int i = 0;
    int index = -1;
    int user_pos = -1;
    user_info_t* pUserInfo = NULL;
    User_Info_Iterator Itr;
    vector<int> UserPosVector;

    EV9000_TCP_Head stTCPHead;
    EV9000_TCP_Data stTCPData;

    USER_INFO_SMUTEX_LOCK();

    if (g_UserInfoMap.size() <= 0 || NULL == user_id)
    {
        USER_INFO_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_USER, LOG_DEBUG, "SendMessageToOnlineUserByUserID() exit---: Param Error \r\n");
        return 1;
    }

    UserPosVector.clear();

    for (Itr = g_UserInfoMap.begin(); Itr != g_UserInfoMap.end(); Itr++)
    {
        pUserInfo = Itr->second;

        if ((NULL == pUserInfo) || (pUserInfo->user_id[0] == '\0') || (pUserInfo->reg_info_index == -1))
        {
            continue;
        }

        if (pUserInfo->reg_status != 2)
        {
            continue;
        }

        if (0 != sstrcmp(pUserInfo->user_id, user_id))
        {
            continue;
        }

        UserPosVector.push_back(Itr->first);
    }

    USER_INFO_SMUTEX_UNLOCK();

    if ((int)UserPosVector.size() == 0)
    {
        return 0;
    }

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "根据用户ID发送Message给所有在线的客户端用户: 在线用户数=%d", (int)UserPosVector.size());
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "According to the user ID to send the Message to all online client users: online users = % d", (int)UserPosVector.size());

    if (UserPosVector.size() > 0)
    {
        for (index = 0; index < (int)UserPosVector.size(); index++)
        {
            user_pos = UserPosVector[index];

            pUserInfo = user_info_get(user_pos);

            if ((NULL == pUserInfo) || (pUserInfo->user_id[0] == '\0') || (pUserInfo->reg_info_index == -1))
            {
                continue;
            }

            if (pUserInfo->reg_status != 2)
            {
                continue;
            }

            if (pUserInfo->tcp_sock >= 0)
            {
                /* 用结构体发送出去 */
                memset(&stTCPHead, 0, sizeof(EV9000_TCP_Head));
                stTCPHead.mark = '$';
                stTCPHead.length = htons(msg_len);

                memset(&stTCPData, 0, sizeof(EV9000_TCP_Data));
                memcpy(&stTCPData.stTCPHead, &stTCPHead, sizeof(EV9000_TCP_Head));
                memcpy(&stTCPData.stTCPBody, msg, msg_len);

                i = send(pUserInfo->tcp_sock, &stTCPData, sizeof(EV9000_TCP_Head) + msg_len, 0);

                if (i > 0)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "通过TCP通道发送Message消息到在线用户成功:在线用户ID=%s, IP地址=%s, 端口号=%d, 用户Socket=%d", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, pUserInfo->tcp_sock);
                }
                else
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "通过TCP通道发送Message消息到在线用户失败:在线用户ID=%s, IP地址=%s, 端口号=%d, 用户Socket=%d, i=%d", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, pUserInfo->tcp_sock, i);
                }
            }
            else
            {
                /* 发送响应消息 */
                i |= SIP_SendMessage(NULL, local_cms_id_get(), pUserInfo->user_id, pUserInfo->strRegServerIP, pUserInfo->iRegServerPort, pUserInfo->login_ip, pUserInfo->login_port, msg, msg_len);

                if (i != 0)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "发送Message消息到在线用户失败:在线用户ID=%s, IP地址=%s, 端口号=%d", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Send Message Message to the online user failure: online user ID = % s, IP address = % s, port = % d", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
                    DEBUG_TRACE(MODULE_USER, LOG_ERROR, "SendMessageToOnlineUserByUserID() SIP_SendMessage Error:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
                }
                else
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "发送Message消息到在线用户成功:在线用户ID=%s, IP地址=%s, 端口号=%d", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Send Message Message to online success: online user ID = % s, IP address = % s, port = % d", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
                    DEBUG_TRACE(MODULE_USER, LOG_TRACE, "SendMessageToOnlineUserByUserID() SIP_SendMessage OK:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
                }
            }
        }
    }

    UserPosVector.clear();

    return 1;
}

/*****************************************************************************
 函 数 名  : SendMessageToOnlineUserByUserIDForTVWallStatus
 功能描述  : 根据用户ID 发送电视墙状态Message消息给所有在线的客户端用户
 输入参数  : char* user_id
             char* msg
             int msg_len
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年1月6日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int SendMessageToOnlineUserByUserIDForTVWallStatus(char* user_id, char* msg, int msg_len)
{
    int i = 0;
    int index = -1;
    int user_pos = -1;
    user_info_t* pUserInfo = NULL;
    User_Info_Iterator Itr;
    vector<int> UserPosVector;

    EV9000_TCP_Head stTCPHead;
    EV9000_TCP_Data stTCPData;

    USER_INFO_SMUTEX_LOCK();

    if (g_UserInfoMap.size() <= 0 || NULL == user_id)
    {
        USER_INFO_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_USER, LOG_DEBUG, "SendMessageToOnlineUserByUserIDForTVWallStatus() exit---: Param Error \r\n");
        return 1;
    }

    UserPosVector.clear();

    for (Itr = g_UserInfoMap.begin(); Itr != g_UserInfoMap.end(); Itr++)
    {
        pUserInfo = Itr->second;

        if ((NULL == pUserInfo) || (pUserInfo->user_id[0] == '\0') || (pUserInfo->reg_info_index == -1))
        {
            continue;
        }

        if (pUserInfo->reg_status != 2)
        {
            continue;
        }

        if (0 != sstrcmp(pUserInfo->user_id, user_id))
        {
            continue;
        }

        if (pUserInfo->tvwall_status_send_flag != 1)
        {
            continue;
        }

        UserPosVector.push_back(Itr->first);
    }

    USER_INFO_SMUTEX_UNLOCK();

    if ((int)UserPosVector.size() == 0)
    {
        return 0;
    }

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "根据用户ID发送电视墙状态Message消息给所有在线的客户端用户: 在线用户数=%d", (int)UserPosVector.size());
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "According to the user ID to send the Message to all online client users: online users = % d", (int)UserPosVector.size());

    if (UserPosVector.size() > 0)
    {
        for (index = 0; index < (int)UserPosVector.size(); index++)
        {
            user_pos = UserPosVector[index];

            pUserInfo = user_info_get(user_pos);

            if ((NULL == pUserInfo) || (pUserInfo->user_id[0] == '\0') || (pUserInfo->reg_info_index == -1))
            {
                continue;
            }

            if (pUserInfo->reg_status != 2)
            {
                continue;
            }

            if (0 != sstrcmp(pUserInfo->user_id, user_id))
            {
                continue;
            }

            if (pUserInfo->tvwall_status_send_flag != 1)
            {
                continue;
            }

            if (pUserInfo->tcp_sock >= 0)
            {
                /* 用结构体发送出去 */
                memset(&stTCPHead, 0, sizeof(EV9000_TCP_Head));
                stTCPHead.mark = '$';
                stTCPHead.length = htons(msg_len);

                memset(&stTCPData, 0, sizeof(EV9000_TCP_Data));
                memcpy(&stTCPData.stTCPHead, &stTCPHead, sizeof(EV9000_TCP_Head));
                memcpy(&stTCPData.stTCPBody, msg, msg_len);

                i = send(pUserInfo->tcp_sock, &stTCPData, sizeof(EV9000_TCP_Head) + msg_len, 0);

                if (i > 0)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "通过TCP通道发送电视墙状态Message消息到在线用户成功:在线用户ID=%s, IP地址=%s, 端口号=%d, 用户Socket=%d", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, pUserInfo->tcp_sock);
                }
                else
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "通过TCP通道发送电视墙状态Message消息到在线用户失败:在线用户ID=%s, IP地址=%s, 端口号=%d, 用户Socket=%d, i=%d", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, pUserInfo->tcp_sock, i);
                }
            }
            else
            {
                /* 发送响应消息 */
                i |= SIP_SendMessage(NULL, local_cms_id_get(), pUserInfo->user_id, pUserInfo->strRegServerIP, pUserInfo->iRegServerPort, pUserInfo->login_ip, pUserInfo->login_port, msg, msg_len);

                if (i != 0)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "发送电视墙状态Message消息到在线用户失败:在线用户ID=%s, IP地址=%s, 端口号=%d", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Send Message Message to the online user failure: online user ID = % s, IP address = % s, port = % d", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
                    DEBUG_TRACE(MODULE_USER, LOG_ERROR, "SendMessageToOnlineUserByUserIDForTVWallStatus() SIP_SendMessage Error:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
                }
                else
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "发送电视墙状态Message消息到在线用户成功:在线用户ID=%s, IP地址=%s, 端口号=%d", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Send Message Message to online success: online user ID = % s, IP address = % s, port = % d", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
                    DEBUG_TRACE(MODULE_USER, LOG_TRACE, "SendMessageToOnlineUserByUserIDForTVWallStatus() SIP_SendMessage OK:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
                }
            }
        }
    }

    UserPosVector.clear();

    return 1;
}

/*****************************************************************************
 函 数 名  : SendMessageToOnlineUserByUserIndex
 功能描述  : 根据用户索引发送Message给所有在线的客户端用户
 输入参数  : unsigned int user_index
             char* msg
             int msg_len
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年1月5日 星期一
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int SendMessageToOnlineUserByUserIndex(unsigned int user_index, char* msg, int msg_len)
{
    int i = 0;
    int index = -1;
    int user_pos = -1;
    user_info_t* pUserInfo = NULL;
    User_Info_Iterator Itr;
    vector<int> UserPosVector;

    EV9000_TCP_Head stTCPHead;
    EV9000_TCP_Data stTCPData;

    USER_INFO_SMUTEX_LOCK();

    if (g_UserInfoMap.size() <= 0 || user_index <= 0)
    {
        USER_INFO_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_USER, LOG_DEBUG, "SendMessageToOnlineUserByUserIndex() exit---: Param Error \r\n");
        return 1;
    }

    UserPosVector.clear();

    for (Itr = g_UserInfoMap.begin(); Itr != g_UserInfoMap.end(); Itr++)
    {
        pUserInfo = Itr->second;

        if ((NULL == pUserInfo) || (pUserInfo->user_id[0] == '\0') || (pUserInfo->reg_info_index == -1))
        {
            continue;
        }

        if (pUserInfo->reg_status != 2)
        {
            continue;
        }

        if (pUserInfo->user_index != user_index)
        {
            continue;
        }

        UserPosVector.push_back(Itr->first);
    }

    USER_INFO_SMUTEX_UNLOCK();

    if ((int)UserPosVector.size() == 0)
    {
        return 0;
    }

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "根据用户索引发送Message给所有在线的客户端用户: 在线用户数=%d", (int)UserPosVector.size());
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "According to the index of the user to send a Message to all online client users: online users = % d", (int)UserPosVector.size());

    if (UserPosVector.size() > 0)
    {
        for (index = 0; index < (int)UserPosVector.size(); index++)
        {
            user_pos = UserPosVector[index];

            pUserInfo = user_info_get(user_pos);

            if ((NULL == pUserInfo) || (pUserInfo->user_id[0] == '\0') || (pUserInfo->reg_info_index == -1))
            {
                continue;
            }

            if (pUserInfo->reg_status != 2)
            {
                continue;
            }

            if (pUserInfo->tcp_sock >= 0)
            {
                /* 用结构体发送出去 */
                memset(&stTCPHead, 0, sizeof(EV9000_TCP_Head));
                stTCPHead.mark = '$';
                stTCPHead.length = htons(msg_len);

                memset(&stTCPData, 0, sizeof(EV9000_TCP_Data));
                memcpy(&stTCPData.stTCPHead, &stTCPHead, sizeof(EV9000_TCP_Head));
                memcpy(&stTCPData.stTCPBody, msg, msg_len);

                i = send(pUserInfo->tcp_sock, &stTCPData, sizeof(EV9000_TCP_Head) + msg_len, 0);

                if (i > 0)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "通过TCP通道发送Message消息到在线用户成功:在线用户ID=%s, IP地址=%s, 端口号=%d, 用户Socket=%d", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, pUserInfo->tcp_sock);
                }
                else
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "通过TCP通道发送Message消息到在线用户失败:在线用户ID=%s, IP地址=%s, 端口号=%d, 用户Socket=%d, i=%d", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, pUserInfo->tcp_sock, i);
                }
            }
            else
            {
                /* 发送响应消息 */
                i |= SIP_SendMessage(NULL, local_cms_id_get(), pUserInfo->user_id, pUserInfo->strRegServerIP, pUserInfo->iRegServerPort, pUserInfo->login_ip, pUserInfo->login_port, msg, msg_len);

                if (i != 0)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "发送Message消息到在线用户失败:在线用户ID=%s, IP地址=%s, 端口号=%d", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Send Message message to the online user failure: online user ID = % s, IP address = % s, port = % d", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
                    DEBUG_TRACE(MODULE_USER, LOG_ERROR, "SendMessageToOnlineUserByUserIndex() SIP_SendMessage Error:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
                }
                else
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "发送Message消息到在线用户成功:在线用户ID=%s, IP地址=%s, 端口号=%d", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Send Message to online success: online user ID = % s, IP address = % s, port = % d ", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
                    DEBUG_TRACE(MODULE_USER, LOG_TRACE, "SendMessageToOnlineUserByUserIndex() SIP_SendMessage OK:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
                }
            }
        }
    }

    UserPosVector.clear();

    return 1;
}

/*****************************************************************************
 函 数 名  : SendAlarmMsgMailToUserByUserIndex
 功能描述  : 发送报警消息邮件给用户
 输入参数  : unsigned int user_index
             alarm_msg_t* pAlarmMsg
             DBOper* pDBoper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年12月16日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int SendAlarmMsgMailToUserByUserIndex(unsigned int user_index, alarm_msg_t* pAlarmMsg, DBOper* pDBoper)
{
    char strUserIndex[64] = {0};
    string strSQL = "";
    int record_count = 0;
    string strResved2 = "";
    string strMailCmd = "";
    string strMailContent = "";
    time_t file_name_now = time(NULL);
    struct tm file_name_tp = {0};
    char tmp_mail_file_time[20] = {0}; /* 邮件内容临时文件时间 */
    char tmp_mail_content_file_name[64] = {0}; /* 邮件内容临时文件名称 */

    FILE* mail_file = NULL;
    string strRMFileCmd = "";

    if (user_index <= 0 || NULL == pAlarmMsg || NULL == pDBoper)
    {
        DEBUG_TRACE(MODULE_USER, LOG_DEBUG, "SendAlarmMsgMailToUserByUserIndex() exit---: Param Error \r\n");
        return -1;
    }

    memset(strUserIndex, 0, 64);
    snprintf(strUserIndex, 64 , "%u", user_index);

    strSQL.clear();
    strSQL = "select * from UserConfig WHERE ID = ";
    strSQL += strUserIndex;

    record_count = pDBoper->DB_Select(strSQL.c_str(), 1);

    if (record_count < 0)
    {
        SystemLog(EV9000_CMS_SEND_NOTIFY_ERROR, EV9000_LOG_LEVEL_ERROR, "告警触发发送邮件给用户失败:用户ID=%u, 原因=%s", user_index, (char*)"查询数据库失败失败");
        EnSystemLog(EV9000_CMS_SEND_NOTIFY_ERROR, EV9000_LOG_LEVEL_ERROR, "Alarm trigger user send mail failed:User ID=%u, reason=%s", user_index, (char*)"Failed to query the database");
        DEBUG_TRACE(MODULE_USER, LOG_ERROR, "SendAlarmMsgMailToUserByUserIndex() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
        DEBUG_TRACE(MODULE_USER, LOG_ERROR, "SendAlarmMsgMailToUserByUserIndex() ErrorMsg=%s\r\n", pDBoper->GetLastDbErrorMsg());
        return -1;
    }
    else if (record_count == 0)
    {
        SystemLog(EV9000_CMS_SEND_NOTIFY_ERROR, EV9000_LOG_LEVEL_WARNING, "告警触发发送邮件给用户失败:用户ID=%u, 原因=%s", user_index, (char*)"未查到数据库中的用户信息");
        EnSystemLog(EV9000_CMS_SEND_NOTIFY_ERROR, EV9000_LOG_LEVEL_WARNING, "Alarm trigger user send mail failed:User ID=%u, reason=%s", user_index, (char*)"Not found user information from the database .");
        DEBUG_TRACE(MODULE_USER, LOG_WARN, "SendAlarmMsgMailToUserByUserIndex() exit---: No Record Count \r\n");
        return -1;
    }

    /* 备注2 */
    strResved2.clear();
    pDBoper->GetFieldValue("Resved2", strResved2);

    if (strResved2.empty())
    {
        SystemLog(EV9000_CMS_SEND_NOTIFY_ERROR, EV9000_LOG_LEVEL_ERROR, "告警触发发送邮件给用户失败:用户ID=%u, 原因=%s", user_index, (char*)"用户没有配置邮件地址");
        EnSystemLog(EV9000_CMS_SEND_NOTIFY_ERROR, EV9000_LOG_LEVEL_ERROR, "Alarm trigger user send mail failed:User ID=%u, reason=%s", user_index, (char*)"Not found user mail information");
        DEBUG_TRACE(MODULE_USER, LOG_WARN, "SendAlarmMsgMailToUserByUserIndex() exit---: User Not Config Emaill Address \r\n");
        return -1;
    }

    /* 移除之前的临时文件 */
    //system((char*)"rm -rf /data/log/mail_*");

    /* 生成邮件内容临时文件 */
    localtime_r(&file_name_now, &file_name_tp);
    memset(tmp_mail_file_time, 0, 20);
    strftime(tmp_mail_file_time, 20, "%Y_%m_%d_%H_%M_%S", &file_name_tp);

    memset(tmp_mail_content_file_name, 0, 64);
    snprintf(tmp_mail_content_file_name, 64, "%s%s_%s", LOGFILE_DIR, (char*)"mail", tmp_mail_file_time);

    /* 打开临时文件，并生成邮件内容 */
    mail_file = fopen(tmp_mail_content_file_name, "w+");

    if (NULL == mail_file)
    {
        SystemLog(EV9000_CMS_SEND_NOTIFY_ERROR, EV9000_LOG_LEVEL_ERROR, "告警触发发送邮件给用户失败:用户ID=%u, 原因=%s, 临时文件=%s", user_index, (char*)"打开邮件临时文件失败", tmp_mail_content_file_name);
        EnSystemLog(EV9000_CMS_SEND_NOTIFY_ERROR, EV9000_LOG_LEVEL_ERROR, "Alarm trigger user send mail failed:User ID=%u, reason=%s, tmp file=%s", user_index, (char*)"Open Send Mail Tmp File Error", tmp_mail_content_file_name);
        DEBUG_TRACE(MODULE_USER, LOG_WARN, "SendAlarmMsgMailToUserByUserIndex() exit---: Open Send Mail Tmp File Error \r\n");
        return -1;
    }

    if (1 == g_Language)
    {
        /* 邮件主题 */
        strMailContent = (char*)"subject:【Send By Wiscom iEV900 CMS System】System alarm notify mail";
        strMailContent += (char*)"\r\n";

        /* 邮件目的地址 */
        strMailContent += (char*)"to:";
        strMailContent += strResved2;
        strMailContent += (char*)"\r\n";

        /* 邮件内容 */
        strMailContent += (char*)"\r\n";
        strMailContent += (char*)"The following is the specific content of the alarm:";
        strMailContent += (char*)"\r\n";

        strMailContent += (char*)"-Alarm device ID:";
        strMailContent += pAlarmMsg->strDeviceID;
        strMailContent += (char*)"\r\n";

        strMailContent += (char*)"-Alarm time:";
        strMailContent += pAlarmMsg->strAlarmStartTime;
        strMailContent += (char*)"\r\n";

        strMailContent += (char*)"-Alarm priority:";
        strMailContent += pAlarmMsg->strPriority;
        strMailContent += (char*)"\r\n";

        strMailContent += (char*)"-Alarm method:";
        strMailContent += pAlarmMsg->strMethod;
        strMailContent += (char*)"\r\n";

        strMailContent += (char*)"-Alarm deseription:";
        strMailContent += pAlarmMsg->strDeseription;
        strMailContent += (char*)"\r\n\r\n";

        strMailContent += (char*)"--------------------------------------\r\n";
        strMailContent += (char*)"This mail automatically sent by Wiscom iEV900 CMS System, please do not reply!";
        strMailContent += (char*)"\r\n";

        DEBUG_TRACE(MODULE_USER, LOG_INFO, "SendAlarmMsgMailToUserByUserIndex() Send Mail=%s \r\n", (char*)strMailContent.c_str());

        fputs((char*)strMailContent.c_str(), mail_file);
    }
    else
    {
        /* 邮件主题 */
        strMailContent = (char*)"subject:【发自Wiscom iEV9000 CMS 系统】系统告警通知邮件";
        strMailContent += (char*)"\r\n";

        /* 邮件目的地址 */
        strMailContent += (char*)"to:";
        strMailContent += strResved2;
        strMailContent += (char*)"\r\n";

        /* 邮件内容 */
        strMailContent += (char*)"\r\n";
        strMailContent += (char*)"以下是报警的具体内容:";
        strMailContent += (char*)"\r\n";

        strMailContent += (char*)"-报警设备ID:";
        strMailContent += pAlarmMsg->strDeviceID;
        strMailContent += (char*)"\r\n";

        strMailContent += (char*)"-报警时间:";
        strMailContent += pAlarmMsg->strAlarmStartTime;
        strMailContent += (char*)"\r\n";

        strMailContent += (char*)"-报警级别:";
        strMailContent += pAlarmMsg->strPriority;
        strMailContent += (char*)"\r\n";

        strMailContent += (char*)"-报警方式:";
        strMailContent += pAlarmMsg->strMethod;
        strMailContent += (char*)"\r\n";

        strMailContent += (char*)"-报警描述:";
        strMailContent += pAlarmMsg->strDeseription;
        strMailContent += (char*)"\r\n\r\n";

        strMailContent += (char*)"--------------------------------------\r\n";
        strMailContent += (char*)"以上邮件由Wiscom iEV9000 CMS系统自动发出，请勿回复!";
        strMailContent += (char*)"\r\n";

        DEBUG_TRACE(MODULE_USER, LOG_INFO, "SendAlarmMsgMailToUserByUserIndex() Send Mail=%s \r\n", (char*)strMailContent.c_str());

        fputs((char*)strMailContent.c_str(), mail_file);
    }

    fclose(mail_file);
    mail_file = NULL;

    /*调用系统接口发送邮件 */
    strMailCmd = (char*)"exim4 -v ";
    strMailCmd += strResved2;
    strMailCmd += (char*)" < ";
    strMailCmd += tmp_mail_content_file_name;

    DEBUG_TRACE(MODULE_USER, LOG_INFO, "SendAlarmMsgMailToUserByUserIndex() MailCmd=%s \r\n", (char*)strMailCmd.c_str());
    system((char*)strMailCmd.c_str());

    /* 移除临时文件 */
    strRMFileCmd = (char*)"rm -rf ";
    strRMFileCmd += tmp_mail_content_file_name;
    system((char*)strRMFileCmd.c_str());

    return 0;
}

/*****************************************************************************
 函 数 名  : SendFaultMsgMailToUserByUserIndex
 功能描述  : 发送故障报警消息邮件给用户
 输入参数  : unsigned int user_index
             fault_msg_t* pFaultMsg
             DBOper* pDBoper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年12月16日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int SendFaultMsgMailToUserByUserIndex(unsigned int user_index, fault_msg_t* pFaultMsg, DBOper* pDBoper)
{
    char strUserIndex[64] = {0};
    string strSQL = "";
    int record_count = 0;
    string strResved2 = "";
    string strMailCmd = "";
    string strMailContent = "";

    char strFaultType[64] = {0};
    time_t utc_time;
    struct tm local_time = { 0 };
    char str_date[12] = {0};
    char str_time[12] = {0};
    char strTime[32] = {0};

    time_t file_name_now = time(NULL);
    struct tm file_name_tp = {0};
    char tmp_mail_file_time[20] = {0}; /* 邮件内容临时文件时间 */
    char tmp_mail_content_file_name[64] = {0}; /* 邮件内容临时文件名称 */

    FILE* mail_file = NULL;

    string strRMFileCmd = "";

    if (user_index <= 0 || NULL == pFaultMsg || NULL == pDBoper)
    {
        DEBUG_TRACE(MODULE_USER, LOG_DEBUG, "SendFaultMsgMailToUserByUserIndex() exit---: Param Error \r\n");
        return -1;
    }

    memset(strUserIndex, 0, 64);
    snprintf(strUserIndex, 64 , "%u", user_index);

    strSQL.clear();
    strSQL = "select * from UserConfig WHERE ID = ";
    strSQL += strUserIndex;

    record_count = pDBoper->DB_Select(strSQL.c_str(), 1);

    if (record_count < 0)
    {
        SystemLog(EV9000_CMS_SEND_NOTIFY_ERROR, EV9000_LOG_LEVEL_ERROR, "故障告警触发发送邮件给用户失败:用户ID=%u, 原因=%s", user_index, (char*)"查询数据库失败失败");
        EnSystemLog(EV9000_CMS_SEND_NOTIFY_ERROR, EV9000_LOG_LEVEL_ERROR, "Fault alarm trigger user send mail failed:User ID=%u, reason=%s", user_index, (char*)"Failed to query the database");
        DEBUG_TRACE(MODULE_USER, LOG_ERROR, "SendFaultMsgMailToUserByUserIndex() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
        DEBUG_TRACE(MODULE_USER, LOG_ERROR, "SendFaultMsgMailToUserByUserIndex() ErrorMsg=%s\r\n", pDBoper->GetLastDbErrorMsg());
        return -1;
    }
    else if (record_count == 0)
    {
        SystemLog(EV9000_CMS_SEND_NOTIFY_ERROR, EV9000_LOG_LEVEL_WARNING, "故障告警触发发送邮件给用户失败:用户ID=%u, 原因=%s", user_index, (char*)"未查到数据库中的用户信息");
        EnSystemLog(EV9000_CMS_SEND_NOTIFY_ERROR, EV9000_LOG_LEVEL_WARNING, "Fault alarm trigger user send mail failed:User ID=%u, reason=%s", user_index, (char*)"Not found user information from the database .");
        DEBUG_TRACE(MODULE_USER, LOG_WARN, "SendFaultMsgMailToUserByUserIndex() exit---: No Record Count \r\n");
        return -1;
    }

    /* 备注2 */
    strResved2.clear();
    pDBoper->GetFieldValue("Resved2", strResved2);

    if (strResved2.empty())
    {
        SystemLog(EV9000_CMS_SEND_NOTIFY_ERROR, EV9000_LOG_LEVEL_ERROR, "故障告警触发发送邮件给用户失败:用户ID=%u, 原因=%s", user_index, (char*)"用户没有配置邮件地址");
        EnSystemLog(EV9000_CMS_SEND_NOTIFY_ERROR, EV9000_LOG_LEVEL_ERROR, "Fault alarm trigger user send mail failed:User ID=%u, reason=%s", user_index, (char*)"Not found user mail information");
        DEBUG_TRACE(MODULE_USER, LOG_WARN, "SendFaultMsgMailToUserByUserIndex() exit---: User Not Config Emaill Address \r\n");
        return -1;
    }

    /* 故障类型 */
    snprintf(strFaultType, 64, "%u", pFaultMsg->uFaultType);

    /* 故障时间 */
    utc_time = (time_t)pFaultMsg->uLogTime;
    localtime_r(&utc_time, &local_time);
    strftime(str_date, sizeof(str_date), "%Y-%m-%d", &local_time);
    strftime(str_time, sizeof(str_time), "%H:%M:%S", &local_time);
    snprintf(strTime, 32, "%sT%s", str_date, str_time);

    /* 移除之前的临时文件 */
    //system((char*)"rm -rf /data/log/mail_*");

    /* 生成邮件内容临时文件 */
    localtime_r(&file_name_now, &file_name_tp);
    memset(tmp_mail_file_time, 0, 20);
    strftime(tmp_mail_file_time, 20, "%Y_%m_%d_%H_%M_%S", &file_name_tp);

    memset(tmp_mail_content_file_name, 0, 64);
    snprintf(tmp_mail_content_file_name, 64, "%s%s_%s", LOGFILE_DIR, (char*)"mail", tmp_mail_file_time);

    /* 打开临时文件，并生成邮件内容 */
    mail_file = fopen(tmp_mail_content_file_name, "w+");

    if (NULL == mail_file)
    {
        SystemLog(EV9000_CMS_SEND_NOTIFY_ERROR, EV9000_LOG_LEVEL_ERROR, "故障告警触发发送邮件给用户失败:用户ID=%u, 原因=%s, 临时文件=%s", user_index, (char*)"打开邮件临时文件失败", tmp_mail_content_file_name);
        EnSystemLog(EV9000_CMS_SEND_NOTIFY_ERROR, EV9000_LOG_LEVEL_ERROR, "Fault alarm trigger user send mail failed:User ID=%u, reason=%s, tmp file=%s", user_index, (char*)"Open Send Mail Tmp File Error", tmp_mail_content_file_name);
        DEBUG_TRACE(MODULE_USER, LOG_WARN, "SendFaultMsgMailToUserByUserIndex() exit---: Open Send Mail Tmp File Error \r\n");
        return -1;
    }

    if (1 == g_Language)
    {
        /* 邮件主题 */
        strMailContent = (char*)"subject:【Send By Wiscom iEV900 CMS System】System fault alarm notify mail";
        strMailContent += (char*)"\r\n";

        /* 邮件目的地址 */
        strMailContent += (char*)"to:";
        strMailContent += strResved2;
        strMailContent += (char*)"\r\n";

        /* 邮件内容 */
        strMailContent += (char*)"\r\n";
        strMailContent += (char*)"The following is the specific content of the fault alarm:";
        strMailContent += (char*)"\r\n";

        strMailContent += (char*)"-Fault alarm device ID:";
        strMailContent += pFaultMsg->strLogicDeviceID;
        strMailContent += (char*)"\r\n";

        strMailContent += (char*)"-Fault alarm time:";
        strMailContent += strTime;
        strMailContent += (char*)"\r\n";

        strMailContent += (char*)"-Fault alarm Type:";
        strMailContent += strFaultType;
        strMailContent += (char*)"\r\n";

        strMailContent += (char*)"-Fault alarm priority:";
        strMailContent += pFaultMsg->strPriority;
        strMailContent += (char*)"\r\n";

        strMailContent += (char*)"-Fault alarm method:";
        strMailContent += pFaultMsg->strMethod;
        strMailContent += (char*)"\r\n\r\n";

        strMailContent += (char*)"--------------------------------------\r\n";
        strMailContent += (char*)"This mail automatically sent by Wiscom iEV900 CMS System, please do not reply!";
        strMailContent += (char*)"\r\n";

        DEBUG_TRACE(MODULE_USER, LOG_INFO, "SendFaultMsgMailToUserByUserIndex() Send Mail=%s \r\n", (char*)strMailContent.c_str());

        fputs((char*)strMailContent.c_str(), mail_file);
    }
    else
    {
        /* 邮件主题 */
        strMailContent = (char*)"subject:【发自Wiscom iEV9000 CMS 系统】系统故障告警通知邮件";
        strMailContent += (char*)"\r\n";

        /* 邮件目的地址 */
        strMailContent += (char*)"to:";
        strMailContent += strResved2;
        strMailContent += (char*)"\r\n";

        /* 邮件内容 */
        strMailContent += (char*)"\r\n";
        strMailContent += (char*)"以下是故障报警的具体内容:";
        strMailContent += (char*)"\r\n";

        strMailContent += (char*)"-故障报警设备ID:";
        strMailContent += pFaultMsg->strLogicDeviceID;
        strMailContent += (char*)"\r\n";

        strMailContent += (char*)"-故障报警时间:";
        strMailContent += strTime;
        strMailContent += (char*)"\r\n";

        strMailContent += (char*)"-故障报警类型:";
        strMailContent += strFaultType;
        strMailContent += (char*)"\r\n";

        strMailContent += (char*)"-故障报警级别:";
        strMailContent += pFaultMsg->strPriority;
        strMailContent += (char*)"\r\n";

        strMailContent += (char*)"-故障报警方式:";
        strMailContent += pFaultMsg->strMethod;
        strMailContent += (char*)"\r\n";

        strMailContent += (char*)"-故障报警描述:";
        strMailContent += pFaultMsg->strDeseription;
        strMailContent += (char*)"\r\n\r\n";

        strMailContent += (char*)"--------------------------------------\r\n";
        strMailContent += (char*)"以上邮件由Wiscom iEV9000 CMS系统自动发出，请勿回复!";
        strMailContent += (char*)"\r\n";

        DEBUG_TRACE(MODULE_USER, LOG_INFO, "SendFaultMsgMailToUserByUserIndex() Send Mail=%s \r\n", (char*)strMailContent.c_str());

        fputs((char*)strMailContent.c_str(), mail_file);
    }

    fclose(mail_file);
    mail_file = NULL;

    /*调用系统接口发送邮件 */
    strMailCmd = (char*)"exim4 -v ";
    strMailCmd += strResved2;
    strMailCmd += (char*)" < ";
    strMailCmd += tmp_mail_content_file_name;

    DEBUG_TRACE(MODULE_USER, LOG_INFO, "SendFaultMsgMailToUserByUserIndex() MailCmd=%s \r\n", (char*)strMailCmd.c_str());
    system((char*)strMailCmd.c_str());

    /* 移除临时文件 */
    strRMFileCmd = (char*)"rm -rf ";
    strRMFileCmd += tmp_mail_content_file_name;
    system((char*)strRMFileCmd.c_str());

    return 0;
}

/*****************************************************************************
 函 数 名  : SendDeviceStatusToAllClientUser
 功能描述  : 发送设备状态给所有客户端用户,不判断该用户是否有该点位权限
 输入参数  : char* device_id
             int status
             DBOper* pDboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年8月15日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int SendDeviceStatusToAllClientUser(char* device_id, int status, DBOper* pDboper)
{
    int i = 0;
    CPacket outPacket;
    DOMElement* AccNode = NULL;
    string strStatus = "";

    if (NULL == device_id)
    {
        return -1;
    }

    if (0 == status)
    {
        strStatus = "OFF";
    }
    else if (1 == status)
    {
        strStatus = "ON";
    }
    else if (2 == status)
    {
        strStatus = "NOVIDEO";
    }
    else if (4 == status)
    {
        strStatus = "INTELLIGENT";
    }
    else if (5 == status)
    {
        strStatus = "CLOSE";
    }
    else if (6 == status)
    {
        strStatus = "APART";
    }

    /* 回复响应,组建消息 */
    outPacket.SetRootTag("Notify");
    AccNode = outPacket.CreateElement((char*)"CmdType");
    outPacket.SetElementValue(AccNode, (char*)"DeviceStatus");

    AccNode = outPacket.CreateElement((char*)"SN");
    outPacket.SetElementValue(AccNode, (char*)"1");

    AccNode = outPacket.CreateElement((char*)"DeviceID");

    if (NULL != device_id)
    {
        outPacket.SetElementValue(AccNode, device_id);
    }
    else
    {
        outPacket.SetElementValue(AccNode, (char*)"");
    }

    AccNode = outPacket.CreateElement((char*)"Status");
    outPacket.SetElementValue(AccNode, (char*)strStatus.c_str());

    i = SendMessageToOnlineUser((char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length(), 0);

    if (i > 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "发送点位状态变化消息给用户成功:点位ID=%s, 状态=%s", device_id, (char*)strStatus.c_str());
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "SendDeviceStatusToAllClientUser() SendMessageToOnlineUser OK:device ID=%s, state=%s", device_id, (char*)strStatus.c_str());

    }
    else if (i < 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "发送点位状态变化消息给用户失败:点位ID=%s, 状态=%s", device_id, (char*)strStatus.c_str());
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "SendDeviceStatusToAllClientUser() SendMessageToOnlineUser Error:device ID=%s, state=%s", device_id, (char*)strStatus.c_str());
    }

    return i;
}

/*****************************************************************************
 函 数 名  : SendRCUDeviceStatusToAllClientUser
 功能描述  : 发送RCU设备状态给所有客户端用户,判断该用户是否有该点位权限
 输入参数  : char* device_id
             int status
             int AlarmPriority
             int guard_type
             char* Value
             char* Unit
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年10月12日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int SendRCUDeviceStatusToAllClientUser(char* device_id, int status, int AlarmPriority, int guard_type, char* Value, char* Unit)
{
    int i = 0;
    CPacket outPacket;
    DOMElement* AccNode = NULL;
    char strAlarmPriority[32] = {0};
    char strGuard[32] = {0};
    char strStatus[32] = {0};

    if (NULL == device_id)
    {
        return -1;
    }

    /* 组建消息 */
    outPacket.SetRootTag("Notify");
    AccNode = outPacket.CreateElement((char*)"CmdType");
    outPacket.SetElementValue(AccNode, (char*)"RCUDeviceStatus");

    AccNode = outPacket.CreateElement((char*)"SN");
    outPacket.SetElementValue(AccNode, (char*)"1");

    AccNode = outPacket.CreateElement((char*)"DeviceID");
    outPacket.SetElementValue(AccNode, device_id);
    /*RCU点位状态*/
    AccNode = outPacket.CreateElement((char*)"Status");
    snprintf(strStatus, 32, "%u", status);
    outPacket.SetElementValue(AccNode, strStatus);

    /*RCU上报的报警级别*/
    AccNode = outPacket.CreateElement((char*)"AlarmPriority");
    snprintf(strAlarmPriority, 32, "%u", AlarmPriority);
    outPacket.SetElementValue(AccNode, strAlarmPriority);

    /*RCU上报的Guard */
    AccNode = outPacket.CreateElement((char*)"Guard");
    snprintf(strGuard, 32, "%u", guard_type);
    outPacket.SetElementValue(AccNode, strGuard);

    /* RCU上报的Value */
    AccNode = outPacket.CreateElement((char*)"Value");

    if (NULL != Value && '0' != Value[0])
    {
        outPacket.SetElementValue(AccNode, Value);
    }
    else
    {
        outPacket.SetElementValue(AccNode, (char*)"0");
    }

    /* RCU上报的Unit */
    AccNode = outPacket.CreateElement((char*)"Unit");
    outPacket.SetElementValue(AccNode, Unit);

    i = SendMessageToOnlineUser((char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length(), 0);

    if (i > 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "发送RCU点位状态变化消息给用户成功:点位ID=%s", device_id);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "SendRCUDeviceStatusToAllClientUser() SendMessageToOnlineUser OK:device ID=%s", device_id);

    }
    else if (i < 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "发送RCU点位状态变化消息给用户失败:点位ID=%s", device_id);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "SendRCUDeviceStatusToAllClientUser() SendMessageToOnlineUser Error:device ID=%s", device_id);
    }

    return i;
}

/*****************************************************************************
 函 数 名  : SendAllDeviceStatusToClientUser
 功能描述  : 发送设备状态给客户端用户,不判断点位权限
 输入参数  : user_info_t* pUserInfo
             vector<string>& DeviceIDVector
             int status
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年2月22日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int SendAllDeviceStatusToClientUser(user_info_t* pUserInfo, vector<string>& DeviceIDVector, int status)
{
    int i = 0;
    int index = -1;
    DOMElement* AccNode = NULL;
    string strStatus = "";
    EV9000_TCP_Head stTCPHead;
    EV9000_TCP_Data stTCPData;
    char* msg = NULL;
    int msg_len = 0;

    GBLogicDevice_info_t* pGBLogicDeviceInfo = NULL;

    if (NULL == pUserInfo)
    {
        return -1;
    }

    if (pUserInfo->reg_status != 2)
    {
        return -1;
    }

    if (0 == status)
    {
        strStatus = "OFF";
    }
    else if (1 == status)
    {
        strStatus = "ON";
    }
    else if (2 == status)
    {
        strStatus = "NOVIDEO";
    }
    else if (4 == status)
    {
        strStatus = "INTELLIGENT";
    }
    else if (5 == status)
    {
        strStatus = "CLOSE";
    }
    else if (6 == status)
    {
        strStatus = "APART";
    }
    else
    {
        return -1;
    }

    for (index = 0; index < (int)DeviceIDVector.size(); index++)
    {
        pGBLogicDeviceInfo = GBLogicDevice_info_find((char*)DeviceIDVector[index].c_str());

        if (NULL == pGBLogicDeviceInfo)
        {
            continue;
        }

        if (pUserInfo->reg_status != 2)
        {
            return -1;
        }

        /* 组建消息 */
        CPacket outPacket;
        outPacket.SetRootTag("Notify");
        AccNode = outPacket.CreateElement((char*)"CmdType");
        outPacket.SetElementValue(AccNode, (char*)"DeviceStatus");

        AccNode = outPacket.CreateElement((char*)"SN");
        outPacket.SetElementValue(AccNode, (char*)"1");

        AccNode = outPacket.CreateElement((char*)"DeviceID");
        outPacket.SetElementValue(AccNode, pGBLogicDeviceInfo->device_id);

        AccNode = outPacket.CreateElement((char*)"Status");
        outPacket.SetElementValue(AccNode, (char*)strStatus.c_str());

        if (pUserInfo->tcp_sock >= 0)
        {
            /* 用结构体发送出去 */
            memset(&stTCPHead, 0, sizeof(EV9000_TCP_Head));
            stTCPHead.mark = '$';
            stTCPHead.length = htons(outPacket.GetXml(NULL).length());

            memset(&stTCPData, 0, sizeof(EV9000_TCP_Data));
            memcpy(&stTCPData.stTCPHead, &stTCPHead, sizeof(EV9000_TCP_Head));
            msg = (char*)outPacket.GetXml(NULL).c_str();
            msg_len = outPacket.GetXml(NULL).length();
            memcpy(&stTCPData.stTCPBody, msg, msg_len);

            i = send(pUserInfo->tcp_sock, &stTCPData, sizeof(EV9000_TCP_Head) + msg_len, 0);

            if (i > 0)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "通过TCP通道发送点位状态Message消息到在线用户成功:在线用户ID=%s, IP地址=%s, 端口号=%d, 用户Socket=%d, 点位ID=%s, 点位状态=%d", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, pUserInfo->tcp_sock, pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->status);
            }
            else
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "通过TCP通道发送点位状态Message消息到在线用户失败:在线用户ID=%s, IP地址=%s, 端口号=%d, 用户Socket=%d, 点位ID=%s, 点位状态=%d", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, pUserInfo->tcp_sock, pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->status);
                return -1;
            }
        }
        else
        {
            /* 发送响应消息 */
            i = SIP_SendMessage(NULL, local_cms_id_get(), pUserInfo->user_id, pUserInfo->strRegServerIP, pUserInfo->iRegServerPort, pUserInfo->login_ip, pUserInfo->login_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

            if (i != 0)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "发送点位状态Message消息到在线用户失败:在线用户ID=%s, IP地址=%s, 端口号=%d, 点位ID=%s, 点位状态=%d", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->status);
            }
            else
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "发送点位状态Message消息到在线用户成功:在线用户ID=%s, IP地址=%s, 端口号=%d, 点位ID=%s, 点位状态=%d", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->status);
            }
        }
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : SendAllDeviceStatusToAllClientUser
 功能描述  : 发送设备状态给所有客户端用户,不判断点位权限
 输入参数  : vector<string>& DeviceIDVector
             int status
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年2月22日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int SendAllDeviceStatusToAllClientUser(vector<string>& DeviceIDVector, int status)
{
    int i = 0;
    int index = -1;
    int user_pos = -1;
    user_info_t* pUserInfo = NULL;
    User_Info_Iterator Itr;
    vector<int> UserPosVector;

    USER_INFO_SMUTEX_LOCK();

    if (g_UserInfoMap.size() <= 0)
    {
        USER_INFO_SMUTEX_UNLOCK();
        return 0;
    }

    UserPosVector.clear();

    for (Itr = g_UserInfoMap.begin(); Itr != g_UserInfoMap.end(); Itr++)
    {
        pUserInfo = Itr->second;

        if ((NULL == pUserInfo) || (pUserInfo->user_id[0] == '\0') || (pUserInfo->reg_info_index == -1))
        {
            continue;
        }

        if (pUserInfo->reg_status != 2)
        {
            continue;
        }

        UserPosVector.push_back(Itr->first);
    }

    USER_INFO_SMUTEX_UNLOCK();

    if ((int)UserPosVector.size() == 0)
    {
        return 0;
    }

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "发送设备状态变化消息给在线的客户端用户: 在线用户数=%d", (int)UserPosVector.size());
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Send the Message to the client user online: online users = % d", (int)UserPosVector.size());

    if (UserPosVector.size() > 0)
    {
        for (index = 0; index < (int)UserPosVector.size(); index++)
        {
            user_pos = UserPosVector[index];

            pUserInfo = user_info_get(user_pos);

            if ((NULL == pUserInfo) || (pUserInfo->user_id[0] == '\0') || (pUserInfo->reg_info_index == -1))
            {
                continue;
            }

            if (pUserInfo->reg_status != 2)
            {
                continue;
            }

            i = SendAllDeviceStatusToClientUser(pUserInfo, DeviceIDVector, status);
            DEBUG_TRACE(MODULE_USER, LOG_INFO, "SendAllDeviceStatusToAllClientUser() SendAllDeviceStatusToClientUser:i=%d \r\n", i);
        }
    }

    UserPosVector.clear();

    return 1;
}

/*****************************************************************************
 函 数 名  : UserGetGBDeviceListAndSendCataLogToClient
 功能描述  : 用户获取设备信息并将其发送给客户端
 输入参数  : user_info_t* pUserInfo
             char* strDeviceID
             char* strSN
             DBOper* pUser_Srv_dboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年6月25日 星期二
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int UserGetGBDeviceListAndSendCataLogToClient(user_info_t* pUserInfo, char* strDeviceID, char* strSN, DBOper* pUser_Srv_dboper)
{
    int i = 0;
    //int iRet = 0;
    int index = 0;
    int record_count = 0; /* 记录数 */
    int send_count = 0;   /* 发送的次数 */
    int query_count = 0;  /* 查询数据统计 */
    DOMElement* ListAccNode = NULL;

    string strSQL = "";
    vector<string> DeviceIDVector;

    if ((NULL == pUserInfo) || (NULL == strDeviceID) || (NULL == strSN) || (NULL == pUser_Srv_dboper))
    {
        DEBUG_TRACE(MODULE_USER, LOG_DEBUG, "UserGetGBDeviceListAndSendCataLogToClient() exit---: Param Error \r\n");
        return -1;
    }

    DEBUG_TRACE(MODULE_USER, LOG_TRACE, "UserGetGBDeviceListAndSendCataLogToClient() Enter---: user_id=%s, user_ip=%s, user_port=%d \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "用户获取设逻辑备信息:用户ID=%s, IP地址=%s, 端口号=%d", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Users get a logic for information: ID = % s, IP address = % s, port = % d", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);

    /* 用户的设备列表可能包括两个地方
       1、用户所在组的设备权限列表
       2、用户本身的设备权限列表
       所以，发送设备权限列表的时候，需要查找这两部分的并集
     */
    DeviceIDVector.clear();

    /* 1、先根据用户Index 查找用户所在的用户组，再根据用户组查找其权限 */
    i = FindUserGroupDevicePermConfig(pUserInfo->user_index, pUser_Srv_dboper, DeviceIDVector);

    /* 2、根据用户Index 查询用户设备权限表 */
    i = FindUserDevicePermConfig(pUserInfo->user_index, pUser_Srv_dboper, DeviceIDVector);

    /* 3、获取所有的电视墙点位信息 */
    //i = FindDECDeviceConfig(pUser_Srv_dboper, DeviceIndexVector);

    /* 4、获取容器中的设备个数 */
    record_count = DeviceIDVector.size();

    DEBUG_TRACE(MODULE_USER, LOG_INFO, "UserGetGBDeviceListAndSendCataLogToClient() record_count=%d \r\n", record_count);

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

        if (NULL != strSN)
        {
            outPacket.SetElementValue(AccNode, strSN);
        }
        else
        {
            outPacket.SetElementValue(AccNode, (char*)"");
        }

        AccNode = outPacket.CreateElement((char*)"DeviceID");

        if (NULL != strDeviceID)
        {
            outPacket.SetElementValue(AccNode, strDeviceID);
        }
        else
        {
            outPacket.SetElementValue(AccNode, (char*)"");
        }

        AccNode = outPacket.CreateElement((char*)"Result");
        outPacket.SetElementValue(AccNode, (char*)"OK");

        AccNode = outPacket.CreateElement((char*)"SumNum");
        outPacket.SetElementValue(AccNode, (char*)"0");

        ListAccNode = outPacket.CreateElement((char*)"DeviceList");
        outPacket.SetElementAttr(ListAccNode, (char*)"Num", (char*)"0");

        i = SIP_SendMessage(NULL, local_cms_id_get(), pUserInfo->user_id, pUserInfo->strRegServerIP, pUserInfo->iRegServerPort, pUserInfo->login_ip, pUserInfo->login_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

        if (i != 0)
        {
            DEBUG_TRACE(MODULE_USER, LOG_ERROR, "UserGetGBDeviceListAndSendCataLogToClient() SIP_SendMessage Error:user_id=%s, user_ip=%s, user_port=%d \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
        }
        else
        {
            DEBUG_TRACE(MODULE_USER, LOG_TRACE, "UserGetGBDeviceListAndSendCataLogToClient() SIP_SendMessage OK:user_id=%s, user_ip=%s, user_port=%d \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
        }

        SystemLog(EV9000_CMS_GET_CATALOG_ERROR, EV9000_LOG_LEVEL_WARNING, "用户获取逻辑设备信息失败:用户ID=%s, IP地址=%s, 端口号=%d, 原因=%s", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, (char*)"未查询到数据库记录");
        EnSystemLog(EV9000_CMS_GET_CATALOG_ERROR, EV9000_LOG_LEVEL_WARNING, "User get logical device information failed:User ID=%s, User IP=%s, Port=%d, reason=%s", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, (char*)"No data has been queried from the database.");
        DEBUG_TRACE(MODULE_USER, LOG_WARN, "UserGetGBDeviceListAndSendCataLogToClient() exit---: No Record Count \r\n");
        return i;
    }

#if 0

    if (!g_IsPay) /* 没有付费，点位发送三分之一 */
    {
        record_count = record_count / 3;
        DEBUG_TRACE(MODULE_USER, LOG_INFO, "UserGetGBDeviceListAndSendCataLogToClient() g_IsPay=%d, Sort Record Count=%d \r\n", g_IsPay, record_count);
    }

#endif

    /* 7、循环查找排序容器，读取用户的设备信息，加入xml中 */
    CPacket* pOutPacket = NULL;

    for (index = 0; index < record_count; index++)
    {
        //DEBUG_TRACE(MODULE_USER, LOG_INFO, "UserGetGBDeviceListAndSendCataLogToClient() DeviceIndex=%u \r\n", device_index);

        /* 如果记录数大于20，则要分次发送 */
        query_count++;

        /* 创建XML头部 */
        i = CreateGBLogicDeviceCatalogResponseXMLHead(&pOutPacket, query_count, record_count, strSN, strDeviceID, &ListAccNode);

        /* 加入Item 值 */
        i = AddLogicDeviceInfoToXMLItem(pOutPacket, ListAccNode, (char*)DeviceIDVector[index].c_str(), pUser_Srv_dboper);

        if ((query_count % MAX_USER_CATALOG_COUT_SEND == 0) || (query_count == record_count))
        {
            if (NULL != pOutPacket)
            {
                send_count++;
                /* 发送出去 */
                i |= SIP_SendMessage(NULL, local_cms_id_get(), pUserInfo->user_id, pUserInfo->strRegServerIP, pUserInfo->iRegServerPort, pUserInfo->login_ip, pUserInfo->login_port, (char*)pOutPacket->GetXml(NULL).c_str(), pOutPacket->GetXml(NULL).length());

                if (i != 0)
                {
                    SystemLog(EV9000_CMS_GET_CATALOG_ERROR, EV9000_LOG_LEVEL_ERROR, "用户获取逻辑设备信息, 发送Message消息到用户失败:用户ID=%s, IP地址=%s, 端口号=%d", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
                    EnSystemLog(EV9000_CMS_GET_CATALOG_ERROR, EV9000_LOG_LEVEL_ERROR, "User access to logical device information, Message sending messages to the user failed: user ID = % s, IP address = % s, port = % d", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
                    DEBUG_TRACE(MODULE_USER, LOG_ERROR, "UserGetGBDeviceListAndSendCataLogToClient() SIP_SendMessage Error:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
                }
                else
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "用户获取逻辑设备信息, 发送Message消息到用户成功:用户ID=%s, IP地址=%s, 端口号=%d", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "User access to logical device information, Message sending messages to the user: the user ID = % s, IP address = % s, port = % d", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
                    DEBUG_TRACE(MODULE_USER, LOG_TRACE, "UserGetGBDeviceListAndSendCataLogToClient() SIP_SendMessage OK:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
                }

                delete pOutPacket;
                pOutPacket = NULL;
            }
        }
    }

    if (i == 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "用户获取逻辑设备信息成功:用户ID=%s, IP地址=%s, 端口号=%d,逻辑设备数目=%d", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, record_count);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Users access logical device information users successfully: user ID = % s, = % s IP address, port number = % d, logical device number = %d", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, record_count);

    }
    else
    {
        SystemLog(EV9000_CMS_GET_CATALOG_ERROR, EV9000_LOG_LEVEL_ERROR, "用户获取逻辑设备信息失败:用户ID=%s, IP地址=%s, 端口号=%d, 原因=%s", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, (char*)"发送SIP响应消息失败");
        EnSystemLog(EV9000_CMS_GET_CATALOG_ERROR, EV9000_LOG_LEVEL_ERROR, "Users failed to get logic device information: user ID = % s, IP address = % s, port = % d, reason = % s", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, (char*)"发送SIP响应消息失败");
    }

    DEBUG_TRACE(MODULE_USER, LOG_TRACE, "UserGetGBDeviceListAndSendCataLogToClient Exit--- \r\n");

    return 0;
}

/*****************************************************************************
 函 数 名  : UserGetGBDeviceListAndSendRCUCataLogToClientForTCP
 功能描述  : 用户通过TCP获取RCU设备信息并将其发送给客户端
 输入参数  : char* user_ip
             int user_port
             int user_sock
             unsigned int user_index
             char* strDeviceID
             char* strSN
             DBOper* pUser_Srv_dboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年10月12日 星期二
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int UserGetGBDeviceListAndSendRCUCataLogToClientForTCP(char* user_ip, int user_port, int user_sock, unsigned int user_index, char* strDeviceID, char* strSN, DBOper* pUser_Srv_dboper)
{
    int i = 0;
    int index = 0;
    int iRet = 0;
    int record_count = 0; /* 记录数 */
    //int iChannel = 0;

    EV9000_TCP_Head stTCPHead;
    GBLogicDevice_info_t* pGBLogicDeviceInfo = NULL;
    EV9000_GBLogicDeviceConfig stEV9000GBLogicDevice;

    vector<string> DeviceIDVector;

    if (NULL == user_ip || user_sock <= 0 || NULL == strDeviceID || NULL == strSN || NULL == pUser_Srv_dboper)
    {
        DEBUG_TRACE(MODULE_USER, LOG_DEBUG, "UserGetGBDeviceListAndSendRCUCataLogToClientForTCP() exit---: Param Error \r\n");
        return -1;
    }

    DEBUG_TRACE(MODULE_USER, LOG_TRACE, "UserGetGBDeviceListAndSendRCUCataLogToClientForTCP() Enter---: user_ip=%s, user_port=%d, user_sock=%d \r\n", user_ip, user_port, user_sock);
    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "用户通过TCP方式获取RCU逻辑设备信息:用户IP=%s, 端口号=%d, Socket=%d", user_ip, user_port, user_sock);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Users by way of TCP for logical device information: IP = % s, port = %d, Socket=%d", user_ip, user_port, user_sock);

    /* 用户的设备列表可能包括两个地方
       1、用户所在组的设备权限列表
       2、用户本身的设备权限列表
       所以，发送设备权限列表的时候，需要查找这两部分的并集
     */
    DeviceIDVector.clear();

    /* 1、先根据用户Index 查找用户所在的用户组，再根据用户组查找其权限 */
    i = FindRCUUserGroupDevicePermConfig(user_index, pUser_Srv_dboper, DeviceIDVector);

    /* 2、根据用户Index 查询用户设备权限表 */
    i = FindRCUUserDevicePermConfig(user_index, pUser_Srv_dboper, DeviceIDVector);

    /* 3、获取容器中的设备个数 */
    record_count = DeviceIDVector.size();

    DEBUG_TRACE(MODULE_USER, LOG_INFO, "UserGetGBDeviceListAndSendRCUCataLogToClientForTCP() record_count=%d \r\n", record_count);

    /* 4、如果记录数为0 */
    if (record_count == 0)
    {
        memset(&stTCPHead, 0, sizeof(EV9000_TCP_Head));
        stTCPHead.mark = '$';
        stTCPHead.length = 0;
        send(user_sock, &stTCPHead, sizeof(EV9000_TCP_Head), 0);

        SystemLog(EV9000_CMS_GET_CATALOG_ERROR, EV9000_LOG_LEVEL_WARNING, "用户通过TCP方式获取RCU逻辑设备信息失败:用户IP=%s, 端口号=%d, Socket=%d, 原因=%s", user_ip, user_port, user_sock, (char*)"未查询到数据库记录");
        EnSystemLog(EV9000_CMS_GET_CATALOG_ERROR, EV9000_LOG_LEVEL_WARNING, "User failed to get logic device information through TCP:User IP=%s, Port=%d, Socket=%d, reason=%s", user_ip, user_port, user_sock, (char*)"No data has been queried from the database.");
        DEBUG_TRACE(MODULE_USER, LOG_WARN, "UserGetGBDeviceListAndSendRCUCataLogToClientForTCP() exit---: No Record Count \r\n");
        return i;
    }

    /* 用结构体发送出去 */
    memset(&stTCPHead, 0, sizeof(EV9000_TCP_Head));
    stTCPHead.mark = '$';
    stTCPHead.length = htons(record_count);
    send(user_sock, &stTCPHead, sizeof(EV9000_TCP_Head), 0);

    for (index = 0; index < record_count; index++)
    {
        /* 根据Index 获取逻辑设备信息，可能是只配置了物理设备，还没有上线，数据库和内存中都没有的*/
        pGBLogicDeviceInfo = GBLogicDevice_info_find((char*)DeviceIDVector[index].c_str());
        memset(&stEV9000GBLogicDevice, 0, sizeof(EV9000_GBLogicDeviceConfig));

        if (NULL != pGBLogicDeviceInfo)
        {
            /* 设备索引 */
            stEV9000GBLogicDevice.nID = htonl(pGBLogicDeviceInfo->id);

            /* 设备统一编号 */
            osip_strncpy(stEV9000GBLogicDevice.strDeviceID, pGBLogicDeviceInfo->device_id, EV9000_IDCODE_LEN);

            /* 点位名称 */
            osip_strncpy(stEV9000GBLogicDevice.strDeviceName, pGBLogicDeviceInfo->device_name, EV9000_NORMAL_STRING_LEN);

            /* 是否可控 */
            if (1 == pGBLogicDeviceInfo->ctrl_enable)
            {
                stEV9000GBLogicDevice.nCtrlEnable = htonl(1);
            }
            else
            {
                stEV9000GBLogicDevice.nCtrlEnable = htonl(0);
            }

            /* 是否支持对讲 */
            stEV9000GBLogicDevice.nMicEnable = htonl(pGBLogicDeviceInfo->mic_enable);

            /* 帧率 */
            stEV9000GBLogicDevice.nFrameCount = htonl(pGBLogicDeviceInfo->frame_count);

            /* 码流数 */
            stEV9000GBLogicDevice.nStreamCount = htonl(pGBLogicDeviceInfo->stream_count);

            /* RCU Value */
            osip_strncpy(stEV9000GBLogicDevice.strValue, pGBLogicDeviceInfo->Value, EV9000_LONG_LONG_STRING_LEN);

            /* RCU Unit */
            osip_strncpy(stEV9000GBLogicDevice.strUnit, pGBLogicDeviceInfo->Unit, EV9000_SHORT_STRING_LEN);

            /* 报警级别*/
            stEV9000GBLogicDevice.nAlarmPriority = htonl(pGBLogicDeviceInfo->AlarmPriority);

            /* 设备端是否布防 */
            stEV9000GBLogicDevice.nGuard = htonl(pGBLogicDeviceInfo->guard_type);

            /* 点位状态 */
            if (1 == pGBLogicDeviceInfo->status)
            {

                stEV9000GBLogicDevice.nStatus |= EV9000_LOGICDEVICE_STATUS_ONLINE;
            }
            else
            {
                stEV9000GBLogicDevice.nStatus |= EV9000_LOGICDEVICE_STATUS_OFFLINE;
            }

            stEV9000GBLogicDevice.nStatus = htonl(stEV9000GBLogicDevice.nStatus);
#if 0

            /* 点位状态 */
            if (1 == pGBLogicDeviceInfo->status)
            {
                if (INTELLIGENT_STATUS_ON == pGBLogicDeviceInfo->intelligent_status)
                {
                    ExstEV9000GBLogicDevice.nStatus |= EV9000_LOGICDEVICE_STATUS_ONLINE;
                    ExstEV9000GBLogicDevice.nStatus |= EV9000_LOGICDEVICE_STATUS_INTEL;
                }
                else if (ALARM_STATUS_CLOSE == pGBLogicDeviceInfo->alarm_status)
                {
                    ExstEV9000GBLogicDevice.nStatus |= EV9000_LOGICDEVICE_STATUS_ONLINE;
                    ExstEV9000GBLogicDevice.nStatus |= EV9000_LOGICDEVICE_STATUS_ALARM;
                }
                else if (ALARM_STATUS_APART == pGBLogicDeviceInfo->alarm_status)
                {
                    ExstEV9000GBLogicDevice.nStatus |= EV9000_LOGICDEVICE_STATUS_ONLINE;
                }
                else
                {
                    ExstEV9000GBLogicDevice.nStatus |= EV9000_LOGICDEVICE_STATUS_ONLINE;
                }
            }
            else if (2 == pGBLogicDeviceInfo->status)
            {
                ExstEV9000GBLogicDevice.nStatus |= EV9000_LOGICDEVICE_STATUS_ONLINE;
                ExstEV9000GBLogicDevice.nStatus |= EV9000_LOGICDEVICE_STATUS_NOVIDEO;
            }
            else
            {
                ExstEV9000GBLogicDevice.nStatus |= EV9000_LOGICDEVICE_STATUS_OFFLINE;
            }

            ExstEV9000GBLogicDevice.nStatus = htonl(ExstEV9000GBLogicDevice.nStatus);
#endif
            /* 经度 */
            stEV9000GBLogicDevice.dLongitude = pGBLogicDeviceInfo->longitude;

            /* 纬度 */
            stEV9000GBLogicDevice.dLatitude = pGBLogicDeviceInfo->latitude;

            /* 所属图层 */
            if (pGBLogicDeviceInfo->map_layer[0] != '\0')
            {
                osip_strncpy(stEV9000GBLogicDevice.strResved2, pGBLogicDeviceInfo->map_layer, EV9000_SHORT_STRING_LEN);
            }

            /* 报警设备子类型 */
            stEV9000GBLogicDevice.nResved1 = htonl(pGBLogicDeviceInfo->alarm_device_sub_type);

            /* 所属的CMS ID */
            osip_strncpy(stEV9000GBLogicDevice.strCMSID, pGBLogicDeviceInfo->cms_id, EV9000_IDCODE_LEN);

            /* 上海地标对应的通道ID */
            //iChannel = shdb_get_channel_by_device_index(pGBLogicDeviceInfo->id, pUser_Srv_dboper);

            if (pGBLogicDeviceInfo->shdb_channel_id > 0)
            {
                stEV9000GBLogicDevice.nResved3 = htonl(pGBLogicDeviceInfo->shdb_channel_id);
            }

            /* 鹰眼相机对应的预案ID */
            if (pGBLogicDeviceInfo->strResved2[0] != '\0')
            {
                osip_strncpy(stEV9000GBLogicDevice.strResved4, pGBLogicDeviceInfo->strResved2, EV9000_SHORT_STRING_LEN);
            }

            iRet = send(user_sock, &stEV9000GBLogicDevice, sizeof(EV9000_GBLogicDeviceConfig), 0);

            if (iRet <= 0)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "用户通过TCP方式获取RCU逻辑设备信息, 发送结果给用户失败:用户IP=%s, 端口号=%d, Socket=%d, 发送次数=%d, 返回结果=%d, errno=%d", user_ip, user_port, user_sock, index + 1, iRet, errno);
            }
            else
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "用户通过TCP方式获取RCU逻辑设备信息, 发送结果给用户成功:用户IP=%s, 端口号=%d, Socket=%d, 发送次数=%d, 返回结果=%d", user_ip, user_port, user_sock, index + 1, iRet);
            }
        }
        else/* 可能是数据库新增加逻辑设备，加载到内存中 */
        {
            iRet = load_db_data_to_LogicDevice_info_list_by_device_id(pUser_Srv_dboper, (char*)DeviceIDVector[index].c_str());
            DEBUG_TRACE(MODULE_USER, LOG_INFO, "UserGetGBDeviceListAndSendRCUCataLogToClientForTCP() load_db_data_to_LogicDevice_info_list_by_device_id: DeviceID=%s, iRet=%d \r\n", (char*)DeviceIDVector[index].c_str(), iRet);

            if (iRet == 0)
            {
                pGBLogicDeviceInfo = GBLogicDevice_info_find((char*)DeviceIDVector[index].c_str());

                if (NULL != pGBLogicDeviceInfo && pGBLogicDeviceInfo->enable == 1)
                {
                    /* 设备索引 */
                    stEV9000GBLogicDevice.nID = htonl(pGBLogicDeviceInfo->id);

                    /* 设备统一编号 */
                    osip_strncpy(stEV9000GBLogicDevice.strDeviceID, pGBLogicDeviceInfo->device_id, EV9000_IDCODE_LEN);

                    /* 点位名称 */
                    osip_strncpy(stEV9000GBLogicDevice.strDeviceName, pGBLogicDeviceInfo->device_name, EV9000_NORMAL_STRING_LEN);

                    /* 是否可控 */
                    if (1 == pGBLogicDeviceInfo->ctrl_enable)
                    {
                        stEV9000GBLogicDevice.nCtrlEnable = htonl(1);
                    }
                    else
                    {
                        stEV9000GBLogicDevice.nCtrlEnable = htonl(0);
                    }

                    /* 是否支持对讲 */
                    stEV9000GBLogicDevice.nMicEnable = htonl(pGBLogicDeviceInfo->mic_enable);

                    /* 帧率 */
                    stEV9000GBLogicDevice.nFrameCount = htonl(pGBLogicDeviceInfo->frame_count);

                    /* 码流数 */
                    stEV9000GBLogicDevice.nStreamCount = htonl(pGBLogicDeviceInfo->stream_count);

                    /* RCU Value */
                    osip_strncpy(stEV9000GBLogicDevice.strValue, pGBLogicDeviceInfo->Value, EV9000_LONG_LONG_STRING_LEN);

                    /* RCU Unit */
                    osip_strncpy(stEV9000GBLogicDevice.strUnit, pGBLogicDeviceInfo->Unit, EV9000_SHORT_STRING_LEN);

                    /* 报警级别*/
                    stEV9000GBLogicDevice.nAlarmPriority = htonl(pGBLogicDeviceInfo->AlarmPriority);

                    /* 设备端是否布防 */
                    stEV9000GBLogicDevice.nGuard = htonl(pGBLogicDeviceInfo->guard_type);

                    /* 点位状态 */
                    if (1 == pGBLogicDeviceInfo->status)
                    {

                        stEV9000GBLogicDevice.nStatus |= EV9000_LOGICDEVICE_STATUS_ONLINE;
                    }
                    else
                    {
                        stEV9000GBLogicDevice.nStatus |= EV9000_LOGICDEVICE_STATUS_OFFLINE;
                    }

                    stEV9000GBLogicDevice.nStatus = htonl(stEV9000GBLogicDevice.nStatus);
#if 0

                    /* 点位状态 */
                    if (1 == pGBLogicDeviceInfo->status)
                    {
                        if (INTELLIGENT_STATUS_ON == pGBLogicDeviceInfo->intelligent_status)
                        {
                            ExstEV9000GBLogicDevice.nStatus |= EV9000_LOGICDEVICE_STATUS_ONLINE;
                            ExstEV9000GBLogicDevice.nStatus |= EV9000_LOGICDEVICE_STATUS_INTEL;
                        }
                        else if (ALARM_STATUS_CLOSE == pGBLogicDeviceInfo->alarm_status)
                        {
                            ExstEV9000GBLogicDevice.nStatus |= EV9000_LOGICDEVICE_STATUS_ONLINE;
                            ExstEV9000GBLogicDevice.nStatus |= EV9000_LOGICDEVICE_STATUS_ALARM;
                        }
                        else if (ALARM_STATUS_APART == pGBLogicDeviceInfo->alarm_status)
                        {
                            ExstEV9000GBLogicDevice.nStatus |= EV9000_LOGICDEVICE_STATUS_ONLINE;
                        }
                        else
                        {
                            ExstEV9000GBLogicDevice.nStatus |= EV9000_LOGICDEVICE_STATUS_ONLINE;
                        }
                    }
                    else if (2 == pGBLogicDeviceInfo->status)
                    {
                        ExstEV9000GBLogicDevice.nStatus |= EV9000_LOGICDEVICE_STATUS_ONLINE;
                        ExstEV9000GBLogicDevice.nStatus |= EV9000_LOGICDEVICE_STATUS_NOVIDEO;
                    }
                    else
                    {
                        ExstEV9000GBLogicDevice.nStatus |= EV9000_LOGICDEVICE_STATUS_OFFLINE;
                    }

                    ExstEV9000GBLogicDevice.nStatus = htonl(ExstEV9000GBLogicDevice.nStatus);
#endif
                    /* 经度 */
                    stEV9000GBLogicDevice.dLongitude = pGBLogicDeviceInfo->longitude;

                    /* 纬度 */
                    stEV9000GBLogicDevice.dLatitude = pGBLogicDeviceInfo->latitude;

                    /* 所属图层 */
                    if (pGBLogicDeviceInfo->map_layer[0] != '\0')
                    {
                        osip_strncpy(stEV9000GBLogicDevice.strResved2, pGBLogicDeviceInfo->map_layer, EV9000_SHORT_STRING_LEN);
                    }

                    /* 报警设备子类型 */
                    stEV9000GBLogicDevice.nResved1 = htonl(pGBLogicDeviceInfo->alarm_device_sub_type);

                    /* 所属的CMS ID */
                    osip_strncpy(stEV9000GBLogicDevice.strCMSID, pGBLogicDeviceInfo->cms_id, EV9000_IDCODE_LEN);

                    /* 上海地标对应的通道ID */
                    //iChannel = shdb_get_channel_by_device_index(pGBLogicDeviceInfo->id, pUser_Srv_dboper);

                    if (pGBLogicDeviceInfo->shdb_channel_id > 0)
                    {
                        stEV9000GBLogicDevice.nResved3 = htonl(pGBLogicDeviceInfo->shdb_channel_id);
                    }

                    /* 鹰眼相机对应的预案ID */
                    if (pGBLogicDeviceInfo->strResved2[0] != '\0')
                    {
                        osip_strncpy(stEV9000GBLogicDevice.strResved4, pGBLogicDeviceInfo->strResved2, EV9000_SHORT_STRING_LEN);
                    }

                    iRet = send(user_sock, &stEV9000GBLogicDevice, sizeof(EV9000_GBLogicDeviceConfig), 0);
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "用户通过TCP方式获取RCU逻辑设备信息, 发送结果给用户:用户IP=%s, 端口号=%d, Socket=%d, 发送次数=%d, 返回结果=%d", user_ip, user_port, user_sock, index + 1, iRet);
                }
            }
        }
    }

    if (i == 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "用户通过TCP方式获取RCU逻辑设备信息成功:用户IP=%s, 端口号=%d, Socket=%d, 逻辑设备数目=%d", user_ip, user_port, user_sock, record_count);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "User success to get RCU logic device information through TCP:User IP=%s, Port=%d, Socket=%d, The number of logical devices=%d", user_ip, user_port, user_sock, record_count);
    }
    else
    {
        SystemLog(EV9000_CMS_GET_CATALOG_ERROR, EV9000_LOG_LEVEL_ERROR, "用户通过TCP方式获取RCU逻辑设备信息失败:用户IP=%s, 端口号=%d, Socket=%d, 原因=%s", user_ip, user_port, user_sock, (char*)"发送消息失败");
        EnSystemLog(EV9000_CMS_GET_CATALOG_ERROR, EV9000_LOG_LEVEL_ERROR, "User failed to get RCU logic device information through TCP:User IP=%s, Port=%d, Socket=%d, reason=%s", user_ip, user_port, user_sock, (char*)"Send message failed.");
    }

    DEBUG_TRACE(MODULE_USER, LOG_TRACE, "UserGetGBDeviceListAndSendRCUCataLogToClientForTCP Exit--- \r\n");

    return 0;
}

/*****************************************************************************
 函 数 名  : UserGetGBDeviceListAndSendCataLogToClientForTCP
 功能描述  : 用户通过TCP获取设备信息并将其发送给客户端
 输入参数  : char* user_ip
             int user_port
             int user_sock
             unsigned int user_index
             char* strDeviceID
             char* strSN
             DBOper* pUser_Srv_dboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年6月25日 星期二
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int UserGetGBDeviceListAndSendCataLogToClientForTCP(char* user_ip, int user_port, int user_sock, unsigned int user_index, char* strDeviceID, char* strSN, DBOper* pUser_Srv_dboper)
{
    int i = 0;
    int index = 0;
    int iRet = 0;
    int record_count = 0; /* 记录数 */
    //int iChannel = 0;

    EV9000_TCP_Head stTCPHead;
    GBLogicDevice_info_t* pGBLogicDeviceInfo = NULL;
    EV9000_GBLogicDeviceConfigEx stEV9000GBLogicDevice;

    vector<string> DeviceIDVector;

    if (NULL == user_ip || user_sock <= 0 || NULL == strDeviceID || NULL == strSN || NULL == pUser_Srv_dboper)
    {
        DEBUG_TRACE(MODULE_USER, LOG_DEBUG, "UserGetGBDeviceListAndSendCataLogToClientForTCP() exit---: Param Error \r\n");
        return -1;
    }

    DEBUG_TRACE(MODULE_USER, LOG_TRACE, "UserGetGBDeviceListAndSendCataLogToClientForTCP() Enter---: user_ip=%s, user_port=%d, user_sock=%d \r\n", user_ip, user_port, user_sock);
    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "用户通过TCP方式获取逻辑设备信息:用户IP=%s, 端口号=%d, Socket=%d", user_ip, user_port, user_sock);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Users by way of TCP for logical device information: IP = % s, port = %d, Socket=%d", user_ip, user_port, user_sock);

    /* 用户的设备列表可能包括两个地方
       1、用户所在组的设备权限列表
       2、用户本身的设备权限列表
       所以，发送设备权限列表的时候，需要查找这两部分的并集
     */
    DeviceIDVector.clear();

    /* 1、先根据用户Index 查找用户所在的用户组，再根据用户组查找其权限 */
    i = FindUserGroupDevicePermConfig(user_index, pUser_Srv_dboper, DeviceIDVector);

    /* 2、根据用户Index 查询用户设备权限表 */
    i = FindUserDevicePermConfig(user_index, pUser_Srv_dboper, DeviceIDVector);

    /* 3、获取容器中的设备个数 */
    record_count = DeviceIDVector.size();

    DEBUG_TRACE(MODULE_USER, LOG_INFO, "UserGetGBDeviceListAndSendCataLogToClientForTCP() record_count=%d \r\n", record_count);

    /* 4、如果记录数为0 */
    if (record_count == 0)
    {
        memset(&stTCPHead, 0, sizeof(EV9000_TCP_Head));
        stTCPHead.mark = '$';
        stTCPHead.length = 0;
        send(user_sock, &stTCPHead, sizeof(EV9000_TCP_Head), 0);

        SystemLog(EV9000_CMS_GET_CATALOG_ERROR, EV9000_LOG_LEVEL_WARNING, "用户通过TCP方式获取逻辑设备信息失败:用户IP=%s, 端口号=%d, Socket=%d, 原因=%s", user_ip, user_port, user_sock, (char*)"未查询到数据库记录");
        EnSystemLog(EV9000_CMS_GET_CATALOG_ERROR, EV9000_LOG_LEVEL_WARNING, "User failed to get logic device information through TCP:User IP=%s, Port=%d, Socket=%d, reason=%s", user_ip, user_port, user_sock, (char*)"No data has been queried from the database.");
        DEBUG_TRACE(MODULE_USER, LOG_WARN, "UserGetGBDeviceListAndSendCataLogToClientForTCP() exit---: No Record Count \r\n");
        return i;
    }

    /* 用结构体发送出去 */
    memset(&stTCPHead, 0, sizeof(EV9000_TCP_Head));
    stTCPHead.mark = '$';
    stTCPHead.length = htons(record_count);
    send(user_sock, &stTCPHead, sizeof(EV9000_TCP_Head), 0);

    for (index = 0; index < record_count; index++)
    {
        /* 根据Index 获取逻辑设备信息，可能是只配置了物理设备，还没有上线，数据库和内存中都没有的*/
        pGBLogicDeviceInfo = GBLogicDevice_info_find((char*)DeviceIDVector[index].c_str());
        memset(&stEV9000GBLogicDevice, 0, sizeof(EV9000_GBLogicDeviceConfigEx));

        if (NULL != pGBLogicDeviceInfo)
        {
            /* 设备索引 */
            stEV9000GBLogicDevice.nID = htonl(pGBLogicDeviceInfo->id);

            /* 设备统一编号 */
            osip_strncpy(stEV9000GBLogicDevice.strDeviceID, pGBLogicDeviceInfo->device_id, EV9000_IDCODE_LEN);

            /* 点位名称 */
            osip_strncpy(stEV9000GBLogicDevice.strDeviceName, pGBLogicDeviceInfo->device_name, EV9000_NORMAL_STRING_LEN);

            /* 是否可控 */
            if (1 == pGBLogicDeviceInfo->ctrl_enable)
            {
                stEV9000GBLogicDevice.nCtrlEnable = htonl(1);
            }
            else
            {
                stEV9000GBLogicDevice.nCtrlEnable = htonl(0);
            }

            /* 是否支持对讲 */
            stEV9000GBLogicDevice.nMicEnable = htonl(pGBLogicDeviceInfo->mic_enable);

            /* 帧率 */
            stEV9000GBLogicDevice.nFrameCount = htonl(pGBLogicDeviceInfo->frame_count);

            /* 码流数 */
            stEV9000GBLogicDevice.nStreamCount = htonl(pGBLogicDeviceInfo->stream_count);

            /* 点位状态 */
            if (1 == pGBLogicDeviceInfo->status)
            {
                if (INTELLIGENT_STATUS_ON == pGBLogicDeviceInfo->intelligent_status)
                {
                    stEV9000GBLogicDevice.nStatus |= EV9000_LOGICDEVICE_STATUS_ONLINE;
                    stEV9000GBLogicDevice.nStatus |= EV9000_LOGICDEVICE_STATUS_INTEL;
                }
                else if (ALARM_STATUS_CLOSE == pGBLogicDeviceInfo->alarm_status)
                {
                    stEV9000GBLogicDevice.nStatus |= EV9000_LOGICDEVICE_STATUS_ONLINE;
                    stEV9000GBLogicDevice.nStatus |= EV9000_LOGICDEVICE_STATUS_ALARM;
                }
                else if (ALARM_STATUS_APART == pGBLogicDeviceInfo->alarm_status)
                {
                    stEV9000GBLogicDevice.nStatus |= EV9000_LOGICDEVICE_STATUS_ONLINE;
                }
                else
                {
                    stEV9000GBLogicDevice.nStatus |= EV9000_LOGICDEVICE_STATUS_ONLINE;
                }
            }
            else if (2 == pGBLogicDeviceInfo->status)
            {
                stEV9000GBLogicDevice.nStatus |= EV9000_LOGICDEVICE_STATUS_ONLINE;
                stEV9000GBLogicDevice.nStatus |= EV9000_LOGICDEVICE_STATUS_NOVIDEO;
            }
            else
            {
                stEV9000GBLogicDevice.nStatus |= EV9000_LOGICDEVICE_STATUS_OFFLINE;
            }

            stEV9000GBLogicDevice.nStatus = htonl(stEV9000GBLogicDevice.nStatus);

            /* 经度 */
            stEV9000GBLogicDevice.dLongitude = pGBLogicDeviceInfo->longitude;

            /* 纬度 */
            stEV9000GBLogicDevice.dLatitude = pGBLogicDeviceInfo->latitude;

            /* 所属图层 */
            osip_strncpy(stEV9000GBLogicDevice.strResved2, pGBLogicDeviceInfo->map_layer, EV9000_SHORT_STRING_LEN);

            /* 报警设备子类型 */
            stEV9000GBLogicDevice.nResved1 = htonl(pGBLogicDeviceInfo->alarm_device_sub_type);

            /* 所属的CMS ID */
            osip_strncpy(stEV9000GBLogicDevice.strCMSID, pGBLogicDeviceInfo->cms_id, EV9000_IDCODE_LEN);

            /* 上海地标对应的通道ID */
            //iChannel = shdb_get_channel_by_device_index(pGBLogicDeviceInfo->id, pUser_Srv_dboper);

            if (pGBLogicDeviceInfo->shdb_channel_id > 0)
            {
                stEV9000GBLogicDevice.nResved3 = htonl(pGBLogicDeviceInfo->shdb_channel_id);
            }

            /* 鹰眼相机对应的预案ID */
            if (pGBLogicDeviceInfo->strResved2[0] != '\0')
            {
                osip_strncpy(stEV9000GBLogicDevice.strResved4, pGBLogicDeviceInfo->strResved2, EV9000_SHORT_STRING_LEN);
            }

            iRet = send(user_sock, &stEV9000GBLogicDevice, sizeof(EV9000_GBLogicDeviceConfigEx), 0);

            if (iRet <= 0)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "用户通过TCP方式获取逻辑设备信息, 发送结果给用户失败:用户IP=%s, 端口号=%d, Socket=%d, 发送次数=%d, 返回结果=%d, errno=%d", user_ip, user_port, user_sock, index + 1, iRet, errno);
            }
            else
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "用户通过TCP方式获取逻辑设备信息, 发送结果给用户成功:用户IP=%s, 端口号=%d, Socket=%d, 发送次数=%d, 返回结果=%d", user_ip, user_port, user_sock, index + 1, iRet);
            }
        }
        else/* 可能是数据库新增加逻辑设备，加载到内存中 */
        {
            iRet = load_db_data_to_LogicDevice_info_list_by_device_id(pUser_Srv_dboper, (char*)DeviceIDVector[index].c_str());
            DEBUG_TRACE(MODULE_USER, LOG_INFO, "UserGetGBDeviceListAndSendCataLogToClientForTCP() load_db_data_to_LogicDevice_info_list_by_device_id: DeviceID=%s, iRet=%d \r\n", (char*)DeviceIDVector[index].c_str(), iRet);

            if (iRet == 0)
            {
                pGBLogicDeviceInfo = GBLogicDevice_info_find((char*)DeviceIDVector[index].c_str());

                if (NULL != pGBLogicDeviceInfo && pGBLogicDeviceInfo->enable == 1)
                {
                    /* 设备索引 */
                    stEV9000GBLogicDevice.nID = htonl(pGBLogicDeviceInfo->id);

                    /* 设备统一编号 */
                    osip_strncpy(stEV9000GBLogicDevice.strDeviceID, pGBLogicDeviceInfo->device_id, EV9000_IDCODE_LEN);

                    /* 点位名称 */
                    osip_strncpy(stEV9000GBLogicDevice.strDeviceName, pGBLogicDeviceInfo->device_name, EV9000_NORMAL_STRING_LEN);

                    /* 是否可控 */
                    if (1 == pGBLogicDeviceInfo->ctrl_enable)
                    {
                        stEV9000GBLogicDevice.nCtrlEnable = htonl(1);
                    }
                    else
                    {
                        stEV9000GBLogicDevice.nCtrlEnable = htonl(0);
                    }

                    /* 是否支持对讲 */
                    stEV9000GBLogicDevice.nMicEnable = htonl(pGBLogicDeviceInfo->mic_enable);

                    /* 帧率 */
                    stEV9000GBLogicDevice.nFrameCount = htonl(pGBLogicDeviceInfo->frame_count);

                    /* 码流数 */
                    stEV9000GBLogicDevice.nStreamCount = htonl(pGBLogicDeviceInfo->stream_count);

                    /* 点位状态 */
                    if (1 == pGBLogicDeviceInfo->status)
                    {
                        if (INTELLIGENT_STATUS_ON == pGBLogicDeviceInfo->intelligent_status)
                        {
                            stEV9000GBLogicDevice.nStatus |= EV9000_LOGICDEVICE_STATUS_ONLINE;
                            stEV9000GBLogicDevice.nStatus |= EV9000_LOGICDEVICE_STATUS_INTEL;
                        }
                        else if (ALARM_STATUS_CLOSE == pGBLogicDeviceInfo->alarm_status)
                        {
                            stEV9000GBLogicDevice.nStatus |= EV9000_LOGICDEVICE_STATUS_ONLINE;
                            stEV9000GBLogicDevice.nStatus |= EV9000_LOGICDEVICE_STATUS_ALARM;
                        }
                        else if (ALARM_STATUS_APART == pGBLogicDeviceInfo->alarm_status)
                        {
                            stEV9000GBLogicDevice.nStatus |= EV9000_LOGICDEVICE_STATUS_ONLINE;
                        }
                        else
                        {
                            stEV9000GBLogicDevice.nStatus |= EV9000_LOGICDEVICE_STATUS_ONLINE;
                        }
                    }
                    else if (2 == pGBLogicDeviceInfo->status)
                    {
                        stEV9000GBLogicDevice.nStatus |= EV9000_LOGICDEVICE_STATUS_ONLINE;
                        stEV9000GBLogicDevice.nStatus |= EV9000_LOGICDEVICE_STATUS_NOVIDEO;
                    }
                    else
                    {
                        stEV9000GBLogicDevice.nStatus |= EV9000_LOGICDEVICE_STATUS_OFFLINE;
                    }

                    stEV9000GBLogicDevice.nStatus = htonl(stEV9000GBLogicDevice.nStatus);

                    /* 经度 */
                    stEV9000GBLogicDevice.dLongitude = pGBLogicDeviceInfo->longitude;

                    /* 纬度 */
                    stEV9000GBLogicDevice.dLatitude = pGBLogicDeviceInfo->latitude;

                    /* 所属图层 */
                    osip_strncpy(stEV9000GBLogicDevice.strResved2, pGBLogicDeviceInfo->map_layer, EV9000_SHORT_STRING_LEN);

                    /* 报警设备子类型 */
                    stEV9000GBLogicDevice.nResved1 = htonl(pGBLogicDeviceInfo->alarm_device_sub_type);

                    /* 所属的CMS ID */
                    osip_strncpy(stEV9000GBLogicDevice.strCMSID, pGBLogicDeviceInfo->cms_id, EV9000_IDCODE_LEN);

                    /* 上海地标对应的通道ID */
                    //iChannel = shdb_get_channel_by_device_index(pGBLogicDeviceInfo->id, pUser_Srv_dboper);

                    if (pGBLogicDeviceInfo->shdb_channel_id > 0)
                    {
                        stEV9000GBLogicDevice.nResved3 = htonl(pGBLogicDeviceInfo->shdb_channel_id);
                    }

                    /* 鹰眼相机对应的预案ID */
                    if (pGBLogicDeviceInfo->strResved2[0] != '\0')
                    {
                        osip_strncpy(stEV9000GBLogicDevice.strResved4, pGBLogicDeviceInfo->strResved2, EV9000_SHORT_STRING_LEN);
                    }

                    iRet = send(user_sock, &stEV9000GBLogicDevice, sizeof(EV9000_GBLogicDeviceConfigEx), 0);

                    if (iRet <= 0)
                    {
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "用户通过TCP方式获取逻辑设备信息, 发送结果给用户失败:用户IP=%s, 端口号=%d, Socket=%d, 发送次数=%d, 返回结果=%d, errno=%d", user_ip, user_port, user_sock, index + 1, iRet, errno);
                    }
                    else
                    {
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "用户通过TCP方式获取逻辑设备信息, 发送结果给用户成功:用户IP=%s, 端口号=%d, Socket=%d, 发送次数=%d, 返回结果=%d", user_ip, user_port, user_sock, index + 1, iRet);
                    }
                }
            }
        }
    }

    if (i == 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "用户通过TCP方式获取逻辑设备信息成功:用户IP=%s, 端口号=%d, Socket=%d, 逻辑设备数目=%d", user_ip, user_port, user_sock, record_count);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "User success to get logic device information through TCP:User IP=%s, Port=%d, Socket=%d, The number of logical devices=%d", user_ip, user_port, user_sock, record_count);
    }
    else
    {
        SystemLog(EV9000_CMS_GET_CATALOG_ERROR, EV9000_LOG_LEVEL_ERROR, "用户通过TCP方式获取逻辑设备信息失败:用户IP=%s, 端口号=%d, Socket=%d, 原因=%s", user_ip, user_port, user_sock, (char*)"发送消息失败");
        EnSystemLog(EV9000_CMS_GET_CATALOG_ERROR, EV9000_LOG_LEVEL_ERROR, "User failed to get logic device information through TCP:User IP=%s, Port=%d, Socket=%d, reason=%s", user_ip, user_port, user_sock, (char*)"Send message failed.");
    }

    DEBUG_TRACE(MODULE_USER, LOG_TRACE, "UserGetGBDeviceListAndSendCataLogToClientForTCP Exit--- \r\n");

    return 0;
}

/*****************************************************************************
 函 数 名  : UserGetAllGBDeviceListAndSendCataLogToClient
 功能描述  : 用户获取所有设备信息并将其发送给客户端
 输入参数  : user_info_t* pUserInfo
             char* strDeviceID
             char* strSN
             DBOper* pUser_Srv_dboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年6月4日 星期三
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int UserGetAllGBDeviceListAndSendCataLogToClient(user_info_t* pUserInfo, char* strDeviceID, char* strSN, DBOper* pUser_Srv_dboper)
{
    int i = 0;
    //int iRet = 0;
    int index = 0;
    int record_count = 0; /* 记录数 */
    int send_count = 0;   /* 发送的次数 */
    int query_count = 0;  /* 查询数据统计 */
    DOMElement* ListAccNode = NULL;

    string strSQL = "";
    vector<string> DeviceIDVector;

    if ((NULL == pUserInfo) || (NULL == strDeviceID) || (NULL == strSN) || (NULL == pUser_Srv_dboper))
    {
        DEBUG_TRACE(MODULE_USER, LOG_DEBUG, "UserGetAllGBDeviceListAndSendCataLogToClient() exit---: Param Error \r\n");
        return -1;
    }

    DEBUG_TRACE(MODULE_USER, LOG_TRACE, "UserGetAllGBDeviceListAndSendCataLogToClient() Enter---: user_id=%s, user_ip=%s, user_port=%d \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "用户获取设逻辑备信息:用户ID=%s, IP地址=%s, 端口号=%d", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "User access to a logical device information: user ID = % s, = % s IP address, port number = %d", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);

    DeviceIDVector.clear();

    /* 1、添加所有的逻辑设备 */
    i = AddAllGBLogicDeviceIDToVectorForUser(DeviceIDVector, pUser_Srv_dboper);

    /* 2、获取所有的电视墙点位信息 */
    //i = FindDECDeviceConfig(pUser_Srv_dboper, DeviceIndexVector);

    /* 4、获取容器中的设备个数 */
    record_count = DeviceIDVector.size();

    DEBUG_TRACE(MODULE_USER, LOG_INFO, "UserGetAllGBDeviceListAndSendCataLogToClient() record_count=%d \r\n", record_count);

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

        if (NULL != strSN)
        {
            outPacket.SetElementValue(AccNode, strSN);
        }
        else
        {
            outPacket.SetElementValue(AccNode, (char*)"");
        }

        AccNode = outPacket.CreateElement((char*)"DeviceID");

        if (NULL != strDeviceID)
        {
            outPacket.SetElementValue(AccNode, strDeviceID);
        }
        else
        {
            outPacket.SetElementValue(AccNode, (char*)"");
        }

        AccNode = outPacket.CreateElement((char*)"Result");
        outPacket.SetElementValue(AccNode, (char*)"OK");

        AccNode = outPacket.CreateElement((char*)"SumNum");
        outPacket.SetElementValue(AccNode, (char*)"0");

        ListAccNode = outPacket.CreateElement((char*)"DeviceList");
        outPacket.SetElementAttr(ListAccNode, (char*)"Num", (char*)"0");

        i = SIP_SendMessage(NULL, local_cms_id_get(), pUserInfo->user_id, pUserInfo->strRegServerIP, pUserInfo->iRegServerPort, pUserInfo->login_ip, pUserInfo->login_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

        if (i != 0)
        {
            DEBUG_TRACE(MODULE_USER, LOG_ERROR, "UserGetAllGBDeviceListAndSendCataLogToClient() SIP_SendMessage Error:user_id=%s, user_ip=%s, user_port=%d \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
        }
        else
        {
            DEBUG_TRACE(MODULE_USER, LOG_TRACE, "UserGetAllGBDeviceListAndSendCataLogToClient() SIP_SendMessage OK:user_id=%s, user_ip=%s, user_port=%d \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
        }

        SystemLog(EV9000_CMS_GET_CATALOG_ERROR, EV9000_LOG_LEVEL_WARNING, "用户获取逻辑设备信息失败:用户ID=%s, IP地址=%s, 端口号=%d, 原因=%s", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, (char*)"未查询到数据库记录");
        EnSystemLog(EV9000_CMS_GET_CATALOG_ERROR, EV9000_LOG_LEVEL_WARNING, "User get logical device information failed:User ID=%s, User IP=%s, Port=%d, reason=%s", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, (char*)"No data has been queried from the database.");
        DEBUG_TRACE(MODULE_USER, LOG_WARN, "UserGetAllGBDeviceListAndSendCataLogToClient() exit---: No Record Count \r\n");
        return i;
    }

#if 0
    /* 根据点位名称排序获取所有点位index */
    iRet = AddAllGBLogicDeviceIDToVectorForUser(allDeviceIndexVector, pUser_Srv_dboper);
    DEBUG_TRACE(MODULE_USER, LOG_TRACE, "UserGetAllGBDeviceListAndSendCataLogToClient() AddAllGBLogicDeviceIDToVectorForUser:iRet=%d \r\n", iRet);

    /* 排序要发出去的逻辑设备index */
    sortDeviceIndexVector.clear();
    iRet = AddDeviceIndexToSortVector(allDeviceIndexVector, DeviceIndexVector, sortDeviceIndexVector);
    DEBUG_TRACE(MODULE_USER, LOG_TRACE, "UserGetAllGBDeviceListAndSendCataLogToClient() AddDeviceIndexToSortVector:iRet=%d \r\n", iRet);

    record_count = sortDeviceIndexVector.size();

    DEBUG_TRACE(MODULE_USER, LOG_INFO, "UserGetAllGBDeviceListAndSendCataLogToClient() Sort Record Count=%d \r\n", record_count);

    /* 6、如果记录数为0 */
    if (record_count == 0)
    {
        /* 回复响应,组建消息 */
        CPacket outPacket;
        DOMElement* AccNode = NULL;

        outPacket.SetRootTag("Response");

        AccNode = outPacket.CreateElement((char*)"CmdType");
        outPacket.SetElementValue(AccNode, (char*)"Catalog");

        AccNode = outPacket.CreateElement((char*)"SN");

        if (NULL != strSN)
        {
            outPacket.SetElementValue(AccNode, strSN);
        }
        else
        {
            outPacket.SetElementValue(AccNode, (char*)"");
        }

        AccNode = outPacket.CreateElement((char*)"DeviceID");

        if (NULL != strDeviceID)
        {
            outPacket.SetElementValue(AccNode, strDeviceID);
        }
        else
        {
            outPacket.SetElementValue(AccNode, (char*)"");
        }

        AccNode = outPacket.CreateElement((char*)"Result");
        outPacket.SetElementValue(AccNode, (char*)"OK");

        AccNode = outPacket.CreateElement((char*)"SumNum");
        outPacket.SetElementValue(AccNode, (char*)"0");

        ListAccNode = outPacket.CreateElement((char*)"DeviceList");
        outPacket.SetElementAttr(ListAccNode, (char*)"Num", (char*)"0");

        i = SIP_SendMessage(NULL, local_cms_id_get(), pUserInfo->user_id, pUserInfo->strRegServerIP, pUserInfo->iRegServerPort, pUserInfo->login_ip, pUserInfo->login_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

        if (i != 0)
        {
            DEBUG_TRACE(MODULE_USER, LOG_ERROR, "UserGetAllGBDeviceListAndSendCataLogToClient() SIP_SendMessage Error:user_id=%s, user_ip=%s, user_port=%d \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
        }
        else
        {
            DEBUG_TRACE(MODULE_USER, LOG_TRACE, "UserGetAllGBDeviceListAndSendCataLogToClient() SIP_SendMessage OK:user_id=%s, user_ip=%s, user_port=%d \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
        }

        SystemLog(EV9000_CMS_GET_CATALOG_ERROR, EV9000_LOG_LEVEL_ERROR, "用户获取逻辑设备信息失败:用户ID=%s, IP地址=%s, 端口号=%d, 原因=%s", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, (char*)"未查询到数据库记录");
        DEBUG_TRACE(MODULE_USER, LOG_ERROR, "UserGetAllGBDeviceListAndSendCataLogToClient() exit---: No Record Count \r\n");
        return i;
    }

#endif

#if 0

    if (!g_IsPay) /* 没有付费，点位发送三分之一 */
    {
        record_count = record_count / 3;
        DEBUG_TRACE(MODULE_USER, LOG_INFO, "UserGetAllGBDeviceListAndSendCataLogToClient() g_IsPay=%d, Sort Record Count=%d \r\n", g_IsPay, record_count);
    }

#endif

    /* 7、循环查找排序容器，读取用户的设备信息，加入xml中 */
    CPacket* pOutPacket = NULL;

    for (index = 0; index < record_count; index++)
    {
        //DEBUG_TRACE(MODULE_USER, LOG_INFO, "UserGetAllGBDeviceListAndSendCataLogToClient() DeviceIndex=%u \r\n", device_index);

        /* 如果记录数大于20，则要分次发送 */
        query_count++;

        /* 创建XML头部 */
        i = CreateGBLogicDeviceCatalogResponseXMLHead(&pOutPacket, query_count, record_count, strSN, strDeviceID, &ListAccNode);

        /* 加入Item 值 */
        i = AddLogicDeviceInfoToXMLItem(pOutPacket, ListAccNode, (char*)DeviceIDVector[index].c_str(), pUser_Srv_dboper);

        if ((query_count % MAX_USER_CATALOG_COUT_SEND == 0) || (query_count == record_count))
        {
            if (NULL != pOutPacket)
            {
                send_count++;
                /* 发送出去 */
                i |= SIP_SendMessage(NULL, local_cms_id_get(), pUserInfo->user_id, pUserInfo->strRegServerIP, pUserInfo->iRegServerPort, pUserInfo->login_ip, pUserInfo->login_port, (char*)pOutPacket->GetXml(NULL).c_str(), pOutPacket->GetXml(NULL).length());

                if (i != 0)
                {
                    SystemLog(EV9000_CMS_GET_CATALOG_ERROR, EV9000_LOG_LEVEL_ERROR, "用户获取逻辑设备信息, 发送Message消息到用户失败:用户ID=%s, IP地址=%s, 端口号=%d", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
                    EnSystemLog(EV9000_CMS_GET_CATALOG_ERROR, EV9000_LOG_LEVEL_ERROR, "User access to logical device information, Message sending messages to the user failed: user ID = % s, IP address = % s, port = %d", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
                    DEBUG_TRACE(MODULE_USER, LOG_ERROR, "UserGetAllGBDeviceListAndSendCataLogToClient() SIP_SendMessage Error:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
                }
                else
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "用户获取逻辑设备信息, 发送Message消息到用户成功:用户ID=%s, IP地址=%s, 端口号=%d", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "User access to logical device information, Message sending messages to the user: the user ID = % s, IP address = % s, port = % d", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
                    DEBUG_TRACE(MODULE_USER, LOG_TRACE, "UserGetAllGBDeviceListAndSendCataLogToClient() SIP_SendMessage OK:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
                }

                delete pOutPacket;
                pOutPacket = NULL;
            }
        }
    }

    if (i == 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "用户获取逻辑设备信息成功:用户ID=%s, IP地址=%s, 端口号=%d,逻辑设备数目=%d", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, record_count);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Users access Logical device information successfully: user ID = % s, = % s IP address, port number = % d, logical device number = %d", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, record_count);
    }
    else
    {
        SystemLog(EV9000_CMS_GET_CATALOG_ERROR, EV9000_LOG_LEVEL_ERROR, "用户获取逻辑设备信息失败:用户ID=%s, IP地址=%s, 端口号=%d, 原因=%s", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, (char*)"发送SIP响应消息失败");
        EnSystemLog(EV9000_CMS_GET_CATALOG_ERROR, EV9000_LOG_LEVEL_ERROR, "Users failed to get logic device information: user ID = % s, IP address = % s, port = % d, reason = %s", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, (char*)"发送SIP响应消息失败");
    }

    DEBUG_TRACE(MODULE_USER, LOG_TRACE, "UserGetAllGBDeviceListAndSendCataLogToClient Exit--- \r\n");

    return 0;
}

/*****************************************************************************
 函 数 名  : UserGetAllGBDeviceListAndSendRCUCataLogToClientForTCP
 功能描述  : 用户通过TCP方式获取所有设备信息并将其发送给客户端
 输入参数  : char* user_ip
             int user_port
             int user_sock
             char* strDeviceID
             char* strSN
             DBOper* pUser_Srv_dboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年10月12日 星期三
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int UserGetAllGBDeviceListAndSendRCUCataLogToClientForTCP(char* user_ip, int user_port, int user_sock, char* strDeviceID, char* strSN, DBOper* pUser_Srv_dboper)
{
    int i = 0;
    int index = 0;
    int iRet = 0;
    int record_count = 0; /* 记录数 */
    //int iChannel = 0;

    EV9000_TCP_Head stTCPHead;
    GBLogicDevice_info_t* pGBLogicDeviceInfo = NULL;
    EV9000_GBLogicDeviceConfig stEV9000GBLogicDevice;
    vector<string> DeviceIDVector;

    if (NULL == user_ip || user_sock <= 0 || NULL == strDeviceID || NULL == strSN || NULL == pUser_Srv_dboper)
    {
        DEBUG_TRACE(MODULE_USER, LOG_DEBUG, "UserGetAllGBDeviceListAndSendRCUCataLogToClientForTCP() exit---: Param Error \r\n");
        return -1;
    }

    DEBUG_TRACE(MODULE_USER, LOG_TRACE, "UserGetAllGBDeviceListAndSendRCUCataLogToClientForTCP() Enter---: user_ip=%s, user_port=%d, user_sock=%d \r\n", user_ip, user_port, user_sock);
    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "用户通过TCP方式获取RCU逻辑设备信息:用户IP=%s, 端口号=%d, Socket=%d", user_ip, user_port, user_sock);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Users by way of TCP for RCU logical device information: IP = % s, port = % d, Socket = %d", user_ip, user_port, user_sock);

    DeviceIDVector.clear();

    /* 1、添加所有的逻辑设备 */
    i = AddAllRCUGBLogicDeviceIDToVectorForUser(DeviceIDVector, pUser_Srv_dboper);

    /* 2、获取容器中的设备个数 */
    record_count = DeviceIDVector.size();

    DEBUG_TRACE(MODULE_USER, LOG_INFO, "UserGetAllGBDeviceListAndSendRCUCataLogToClientForTCP() record_count=%d \r\n", record_count);

    /* 3、如果记录数为0 */
    if (record_count == 0)
    {
        memset(&stTCPHead, 0, sizeof(EV9000_TCP_Head));
        stTCPHead.mark = '$';
        stTCPHead.length = 0;
        send(user_sock, &stTCPHead, sizeof(EV9000_TCP_Head), 0);

        SystemLog(EV9000_CMS_GET_CATALOG_ERROR, EV9000_LOG_LEVEL_WARNING, "用户通过TCP方式获取RCU逻辑设备信息失败:用户IP=%s, 端口号=%d, Socket=%d, 原因=%s", user_ip, user_port, user_sock, (char*)"未查询到数据库记录");
        EnSystemLog(EV9000_CMS_GET_CATALOG_ERROR, EV9000_LOG_LEVEL_WARNING, "User failed to get logic device information through TCP:User IP=%s, Port=%d, Socket=%d, reason=%s", user_ip, user_port, user_sock, (char*)"No data has been queried from the database.");
        DEBUG_TRACE(MODULE_USER, LOG_WARN, "UserGetAllGBDeviceListAndSendRCUCataLogToClientForTCP() exit---: No Record Count \r\n");
        return i;
    }

    /* 通过结构体发送出去 */
    memset(&stTCPHead, 0, sizeof(EV9000_TCP_Head));
    stTCPHead.mark = '$';
    stTCPHead.length = htons(record_count);
    send(user_sock, &stTCPHead, sizeof(EV9000_TCP_Head), 0);

    for (index = 0; index < record_count; index++)
    {
        /* 根据Index 获取逻辑设备信息，可能是只配置了物理设备，还没有上线，数据库和内存中都没有的*/
        pGBLogicDeviceInfo = GBLogicDevice_info_find((char*)DeviceIDVector[index].c_str());
        memset(&stEV9000GBLogicDevice, 0, sizeof(EV9000_GBLogicDeviceConfig));

        if (NULL != pGBLogicDeviceInfo)
        {
            /* 设备索引 */
            stEV9000GBLogicDevice.nID = htonl(pGBLogicDeviceInfo->id);

            /* 设备统一编号 */
            osip_strncpy(stEV9000GBLogicDevice.strDeviceID, pGBLogicDeviceInfo->device_id, EV9000_IDCODE_LEN);

            /* 点位名称 */
            osip_strncpy(stEV9000GBLogicDevice.strDeviceName, pGBLogicDeviceInfo->device_name, EV9000_NORMAL_STRING_LEN);

            /* 是否可控 */
            if (1 == pGBLogicDeviceInfo->ctrl_enable)
            {
                stEV9000GBLogicDevice.nCtrlEnable = htonl(1);
            }
            else
            {
                stEV9000GBLogicDevice.nCtrlEnable = htonl(0);
            }

            /* 是否支持对讲 */
            stEV9000GBLogicDevice.nMicEnable = htonl(pGBLogicDeviceInfo->mic_enable);

            /* 帧率 */
            stEV9000GBLogicDevice.nFrameCount = htonl(pGBLogicDeviceInfo->frame_count);

            /* 码流数 */
            stEV9000GBLogicDevice.nStreamCount = htonl(pGBLogicDeviceInfo->stream_count);

            /* RCU Value */
            osip_strncpy(stEV9000GBLogicDevice.strValue, pGBLogicDeviceInfo->Value, EV9000_LONG_LONG_STRING_LEN);

            /* RCU Unit */
            osip_strncpy(stEV9000GBLogicDevice.strUnit, pGBLogicDeviceInfo->Unit, EV9000_SHORT_STRING_LEN);

            /* 报警级别*/
            stEV9000GBLogicDevice.nAlarmPriority = htonl(pGBLogicDeviceInfo->AlarmPriority);

            /* 设备端是否布防 */
            stEV9000GBLogicDevice.nGuard = htonl(pGBLogicDeviceInfo->guard_type);

            /* 点位状态 */
            if (1 == pGBLogicDeviceInfo->status)
            {

                stEV9000GBLogicDevice.nStatus |= EV9000_LOGICDEVICE_STATUS_ONLINE;
            }
            else
            {
                stEV9000GBLogicDevice.nStatus |= EV9000_LOGICDEVICE_STATUS_OFFLINE;
            }

            stEV9000GBLogicDevice.nStatus = htonl(stEV9000GBLogicDevice.nStatus);
#if 0

            if (1 == pGBLogicDeviceInfo->status)
            {
                if (INTELLIGENT_STATUS_ON == pGBLogicDeviceInfo->intelligent_status)
                {
                    ExstEV9000GBLogicDevice.nStatus |= EV9000_LOGICDEVICE_STATUS_ONLINE;
                    ExstEV9000GBLogicDevice.nStatus |= EV9000_LOGICDEVICE_STATUS_INTEL;
                }
                else if (ALARM_STATUS_CLOSE == pGBLogicDeviceInfo->alarm_status)
                {
                    ExstEV9000GBLogicDevice.nStatus |= EV9000_LOGICDEVICE_STATUS_ONLINE;
                    ExstEV9000GBLogicDevice.nStatus |= EV9000_LOGICDEVICE_STATUS_ALARM;
                }
                else if (ALARM_STATUS_APART == pGBLogicDeviceInfo->alarm_status)
                {
                    ExstEV9000GBLogicDevice.nStatus |= EV9000_LOGICDEVICE_STATUS_ONLINE;
                }
                else
                {
                    ExstEV9000GBLogicDevice.nStatus |= EV9000_LOGICDEVICE_STATUS_ONLINE;
                }
            }
            else if (2 == pGBLogicDeviceInfo->status)
            {
                ExstEV9000GBLogicDevice.nStatus |= EV9000_LOGICDEVICE_STATUS_ONLINE;
                ExstEV9000GBLogicDevice.nStatus |= EV9000_LOGICDEVICE_STATUS_NOVIDEO;
            }
            else
            {
                ExstEV9000GBLogicDevice.nStatus |= EV9000_LOGICDEVICE_STATUS_OFFLINE;
            }

            ExstEV9000GBLogicDevice.nStatus = htonl(ExstEV9000GBLogicDevice.nStatus);
#endif
            /* 经度 */
            stEV9000GBLogicDevice.dLongitude = pGBLogicDeviceInfo->longitude;

            /* 纬度 */
            stEV9000GBLogicDevice.dLatitude = pGBLogicDeviceInfo->latitude;

            /* 所属图层 */
            if (pGBLogicDeviceInfo->map_layer[0] != '\0')
            {
                osip_strncpy(stEV9000GBLogicDevice.strResved2, pGBLogicDeviceInfo->map_layer, EV9000_SHORT_STRING_LEN);
            }

            /* 报警设备子类型 */
            stEV9000GBLogicDevice.nResved1 = htonl(pGBLogicDeviceInfo->alarm_device_sub_type);

            /* 所属的CMS ID */
            osip_strncpy(stEV9000GBLogicDevice.strCMSID, pGBLogicDeviceInfo->cms_id, EV9000_IDCODE_LEN);

            /* 上海地标对应的通道ID */
            //iChannel = shdb_get_channel_by_device_index(pGBLogicDeviceInfo->id, pUser_Srv_dboper);

            if (pGBLogicDeviceInfo->shdb_channel_id > 0)
            {
                stEV9000GBLogicDevice.nResved3 = htonl(pGBLogicDeviceInfo->shdb_channel_id);
            }

            /* 鹰眼相机对应的预案ID */
            if (pGBLogicDeviceInfo->strResved2[0] != '\0')
            {
                osip_strncpy(stEV9000GBLogicDevice.strResved4, pGBLogicDeviceInfo->strResved2, EV9000_SHORT_STRING_LEN);
            }

            iRet = send(user_sock, &stEV9000GBLogicDevice, sizeof(EV9000_GBLogicDeviceConfig), 0);

            if (iRet <= 0)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "用户通过TCP方式获取RCU逻辑设备信息, 发送结果给用户失败:用户IP=%s, 端口号=%d, Socket=%d, 发送次数=%d, 返回结果=%d, errno=%d", user_ip, user_port, user_sock, index + 1, iRet, errno);
            }
            else
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "用户通过TCP方式获取RCU逻辑设备信息, 发送结果给用户成功:用户IP=%s, 端口号=%d, Socket=%d, 发送次数=%d, 返回结果=%d", user_ip, user_port, user_sock, index + 1, iRet);
            }
        }
        else/* 可能是数据库新增加逻辑设备，加载到内存中 */
        {
            iRet = load_db_data_to_LogicDevice_info_list_by_device_id(pUser_Srv_dboper, (char*)DeviceIDVector[index].c_str());
            DEBUG_TRACE(MODULE_USER, LOG_INFO, "UserGetAllGBDeviceListAndSendRCUCataLogToClientForTCP() load_db_data_to_LogicDevice_info_list_by_device_id: DeviceID=%s, iRet=%d \r\n", (char*)DeviceIDVector[index].c_str(), iRet);

            if (iRet == 0)
            {
                pGBLogicDeviceInfo = GBLogicDevice_info_find((char*)DeviceIDVector[index].c_str());

                if (NULL != pGBLogicDeviceInfo && pGBLogicDeviceInfo->enable == 1)
                {
                    /* 设备索引 */
                    stEV9000GBLogicDevice.nID = htonl(pGBLogicDeviceInfo->id);

                    /* 设备统一编号 */
                    osip_strncpy(stEV9000GBLogicDevice.strDeviceID, pGBLogicDeviceInfo->device_id, EV9000_IDCODE_LEN);

                    /* 点位名称 */
                    osip_strncpy(stEV9000GBLogicDevice.strDeviceName, pGBLogicDeviceInfo->device_name, EV9000_NORMAL_STRING_LEN);

                    /* 是否可控 */
                    if (1 == pGBLogicDeviceInfo->ctrl_enable)
                    {
                        stEV9000GBLogicDevice.nCtrlEnable = htonl(1);
                    }
                    else
                    {
                        stEV9000GBLogicDevice.nCtrlEnable = htonl(0);
                    }

                    /* 是否支持对讲 */
                    stEV9000GBLogicDevice.nMicEnable = htonl(pGBLogicDeviceInfo->mic_enable);

                    /* 帧率 */
                    stEV9000GBLogicDevice.nFrameCount = htonl(pGBLogicDeviceInfo->frame_count);

                    /* 码流数 */
                    stEV9000GBLogicDevice.nStreamCount = htonl(pGBLogicDeviceInfo->stream_count);

                    /*RCU Value*/
                    osip_strncpy(stEV9000GBLogicDevice.strValue, pGBLogicDeviceInfo->Value, EV9000_LONG_LONG_STRING_LEN);

                    /*RCU Unit*/
                    osip_strncpy(stEV9000GBLogicDevice.strUnit, pGBLogicDeviceInfo->Unit, EV9000_SHORT_STRING_LEN);

                    /* 报警级别*/
                    stEV9000GBLogicDevice.nAlarmPriority = htonl(pGBLogicDeviceInfo->AlarmPriority);

                    /*设备端是否布防 */
                    stEV9000GBLogicDevice.nGuard = htonl(pGBLogicDeviceInfo->guard_type);

                    /* 点位状态 */
                    if (1 == pGBLogicDeviceInfo->status)
                    {

                        stEV9000GBLogicDevice.nStatus |= EV9000_LOGICDEVICE_STATUS_ONLINE;
                    }
                    else
                    {
                        stEV9000GBLogicDevice.nStatus |= EV9000_LOGICDEVICE_STATUS_OFFLINE;
                    }

                    stEV9000GBLogicDevice.nStatus = htonl(stEV9000GBLogicDevice.nStatus);
#if 0

                    if (1 == pGBLogicDeviceInfo->status)
                    {
                        if (INTELLIGENT_STATUS_ON == pGBLogicDeviceInfo->intelligent_status)
                        {
                            ExstEV9000GBLogicDevice.nStatus |= EV9000_LOGICDEVICE_STATUS_ONLINE;
                            ExstEV9000GBLogicDevice.nStatus |= EV9000_LOGICDEVICE_STATUS_INTEL;
                        }
                        else if (ALARM_STATUS_CLOSE == pGBLogicDeviceInfo->alarm_status)
                        {
                            ExstEV9000GBLogicDevice.nStatus |= EV9000_LOGICDEVICE_STATUS_ONLINE;
                            ExstEV9000GBLogicDevice.nStatus |= EV9000_LOGICDEVICE_STATUS_ALARM;
                        }
                        else if (ALARM_STATUS_APART == pGBLogicDeviceInfo->alarm_status)
                        {
                            ExstEV9000GBLogicDevice.nStatus |= EV9000_LOGICDEVICE_STATUS_ONLINE;
                        }
                        else
                        {
                            ExstEV9000GBLogicDevice.nStatus |= EV9000_LOGICDEVICE_STATUS_ONLINE;
                        }
                    }
                    else if (2 == pGBLogicDeviceInfo->status)
                    {
                        ExstEV9000GBLogicDevice.nStatus |= EV9000_LOGICDEVICE_STATUS_ONLINE;
                        ExstEV9000GBLogicDevice.nStatus |= EV9000_LOGICDEVICE_STATUS_NOVIDEO;
                    }
                    else
                    {
                        ExstEV9000GBLogicDevice.nStatus |= EV9000_LOGICDEVICE_STATUS_OFFLINE;
                    }

                    ExstEV9000GBLogicDevice.nStatus = htonl(ExstEV9000GBLogicDevice.nStatus);
#endif
                    /* 经度 */
                    stEV9000GBLogicDevice.dLongitude = pGBLogicDeviceInfo->longitude;

                    /* 纬度 */
                    stEV9000GBLogicDevice.dLatitude = pGBLogicDeviceInfo->latitude;

                    /* 所属图层 */
                    if (pGBLogicDeviceInfo->map_layer[0] != '\0')
                    {
                        osip_strncpy(stEV9000GBLogicDevice.strResved2, pGBLogicDeviceInfo->map_layer, EV9000_SHORT_STRING_LEN);
                    }

                    /* 报警设备子类型 */
                    stEV9000GBLogicDevice.nResved1 = htonl(pGBLogicDeviceInfo->alarm_device_sub_type);

                    /* 所属的CMS ID */
                    osip_strncpy(stEV9000GBLogicDevice.strCMSID, pGBLogicDeviceInfo->cms_id, EV9000_IDCODE_LEN);

                    /* 上海地标对应的通道ID */
                    //iChannel = shdb_get_channel_by_device_index(pGBLogicDeviceInfo->id, pUser_Srv_dboper);

                    if (pGBLogicDeviceInfo->shdb_channel_id > 0)
                    {
                        stEV9000GBLogicDevice.nResved3 = htonl(pGBLogicDeviceInfo->shdb_channel_id);
                    }

                    /* 鹰眼相机对应的预案ID */
                    if (pGBLogicDeviceInfo->strResved2[0] != '\0')
                    {
                        osip_strncpy(stEV9000GBLogicDevice.strResved4, pGBLogicDeviceInfo->strResved2, EV9000_SHORT_STRING_LEN);
                    }

                    iRet = send(user_sock, &stEV9000GBLogicDevice, sizeof(EV9000_GBLogicDeviceConfig), 0);
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "用户通过TCP方式获取RCU逻辑设备信息, 发送结果给用户:用户IP=%s, 端口号=%d, Socket=%d, 发送次数=%d, 返回结果=%d", user_ip, user_port, user_sock, index + 1, iRet);
                }
            }
        }
    }

    if (i == 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "用户通过TCP方式获取RCU逻辑设备信息成功:用户IP=%s, 端口号=%d, Socket=%d, 逻辑设备数目=%d", user_ip, user_port, user_sock, record_count);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "User success to get RCU logic device information through TCP:User IP=%s, Port=%d, Socket=%d, The number of logical devices=%d", user_ip, user_port, user_sock, record_count);
    }
    else
    {
        SystemLog(EV9000_CMS_GET_CATALOG_ERROR, EV9000_LOG_LEVEL_ERROR, "用户通过TCP方式获取RCU逻辑设备信息失败:用户IP=%s, 端口号=%d, Socket=%d, 原因=%s", user_ip, user_port, user_sock, (char*)"发送消息失败");
        EnSystemLog(EV9000_CMS_GET_CATALOG_ERROR, EV9000_LOG_LEVEL_ERROR, "User failed to get RCU logic device information through TCP:User IP=%s, Port=%d, Socket=%d, reason=%s", user_ip, user_port, user_sock, (char*)"Send message failed.");
    }

    DEBUG_TRACE(MODULE_USER, LOG_TRACE, "UserGetAllGBDeviceListAndSendRCUCataLogToClientForTCP Exit--- \r\n");

    return 0;
}

/*****************************************************************************
 函 数 名  : UserGetAllGBDeviceListAndSendCataLogToClientForTCP
 功能描述  : 用户通过TCP方式获取所有设备信息并将其发送给客户端
 输入参数  : char* user_ip
             int user_port
             int user_sock
             char* strDeviceID
             char* strSN
             DBOper* pUser_Srv_dboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年6月4日 星期三
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int UserGetAllGBDeviceListAndSendCataLogToClientForTCP(char* user_ip, int user_port, int user_sock, char* strDeviceID, char* strSN, DBOper* pUser_Srv_dboper)
{
    int i = 0;
    int index = 0;
    int iRet = 0;
    int record_count = 0; /* 记录数 */
    //int iChannel = 0;

    EV9000_TCP_Head stTCPHead;
    GBLogicDevice_info_t* pGBLogicDeviceInfo = NULL;
    EV9000_GBLogicDeviceConfigEx stEV9000GBLogicDevice;

    vector<string> DeviceIDVector;

    if (NULL == user_ip || user_sock <= 0 || NULL == strDeviceID || NULL == strSN || NULL == pUser_Srv_dboper)
    {
        DEBUG_TRACE(MODULE_USER, LOG_DEBUG, "UserGetAllGBDeviceListAndSendCataLogToClientForTCP() exit---: Param Error \r\n");
        return -1;
    }

    DEBUG_TRACE(MODULE_USER, LOG_TRACE, "UserGetAllGBDeviceListAndSendCataLogToClientForTCP() Enter---: user_ip=%s, user_port=%d, user_sock=%d \r\n", user_ip, user_port, user_sock);
    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "用户通过TCP方式获取逻辑设备信息:用户IP=%s, 端口号=%d, Socket=%d", user_ip, user_port, user_sock);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Users by way of TCP for logical device information: IP = % s, port = % d, Socket = %d", user_ip, user_port, user_sock);

    DeviceIDVector.clear();

    /* 1、添加所有的逻辑设备 */
    i = AddAllGBLogicDeviceIDToVectorForUser(DeviceIDVector, pUser_Srv_dboper);

    /* 2、获取容器中的设备个数 */
    record_count = DeviceIDVector.size();

    DEBUG_TRACE(MODULE_USER, LOG_INFO, "UserGetAllGBDeviceListAndSendCataLogToClientForTCP() record_count=%d \r\n", record_count);

    /* 3、如果记录数为0 */
    if (record_count == 0)
    {
        memset(&stTCPHead, 0, sizeof(EV9000_TCP_Head));
        stTCPHead.mark = '$';
        stTCPHead.length = 0;
        send(user_sock, &stTCPHead, sizeof(EV9000_TCP_Head), 0);

        SystemLog(EV9000_CMS_GET_CATALOG_ERROR, EV9000_LOG_LEVEL_WARNING, "用户通过TCP方式获取逻辑设备信息失败:用户IP=%s, 端口号=%d, Socket=%d, 原因=%s", user_ip, user_port, user_sock, (char*)"未查询到数据库记录");
        EnSystemLog(EV9000_CMS_GET_CATALOG_ERROR, EV9000_LOG_LEVEL_WARNING, "User failed to get logic device information through TCP:User IP=%s, Port=%d, Socket=%d, reason=%s", user_ip, user_port, user_sock, (char*)"No data has been queried from the database.");
        DEBUG_TRACE(MODULE_USER, LOG_WARN, "UserGetAllGBDeviceListAndSendCataLogToClientForTCP() exit---: No Record Count \r\n");
        return i;
    }

    /* 通过结构体发送出去 */
    memset(&stTCPHead, 0, sizeof(EV9000_TCP_Head));
    stTCPHead.mark = '$';
    stTCPHead.length = htons(record_count);
    send(user_sock, &stTCPHead, sizeof(EV9000_TCP_Head), 0);

    for (index = 0; index < record_count; index++)
    {
        /* 根据Index 获取逻辑设备信息，可能是只配置了物理设备，还没有上线，数据库和内存中都没有的*/
        pGBLogicDeviceInfo = GBLogicDevice_info_find((char*)DeviceIDVector[index].c_str());
        memset(&stEV9000GBLogicDevice, 0, sizeof(EV9000_GBLogicDeviceConfigEx));

        if (NULL != pGBLogicDeviceInfo)
        {
            /* 设备索引 */
            stEV9000GBLogicDevice.nID = htonl(pGBLogicDeviceInfo->id);

            /* 设备统一编号 */
            osip_strncpy(stEV9000GBLogicDevice.strDeviceID, pGBLogicDeviceInfo->device_id, EV9000_IDCODE_LEN);

            /* 点位名称 */
            osip_strncpy(stEV9000GBLogicDevice.strDeviceName, pGBLogicDeviceInfo->device_name, EV9000_NORMAL_STRING_LEN);

            /* 是否可控 */
            if (1 == pGBLogicDeviceInfo->ctrl_enable)
            {
                stEV9000GBLogicDevice.nCtrlEnable = htonl(1);
            }
            else
            {
                stEV9000GBLogicDevice.nCtrlEnable = htonl(0);
            }

            /* 是否支持对讲 */
            stEV9000GBLogicDevice.nMicEnable = htonl(pGBLogicDeviceInfo->mic_enable);

            /* 帧率 */
            stEV9000GBLogicDevice.nFrameCount = htonl(pGBLogicDeviceInfo->frame_count);

            /* 码流数 */
            stEV9000GBLogicDevice.nStreamCount = htonl(pGBLogicDeviceInfo->stream_count);

            /* 点位状态 */
            if (1 == pGBLogicDeviceInfo->status)
            {
                if (INTELLIGENT_STATUS_ON == pGBLogicDeviceInfo->intelligent_status)
                {
                    stEV9000GBLogicDevice.nStatus |= EV9000_LOGICDEVICE_STATUS_ONLINE;
                    stEV9000GBLogicDevice.nStatus |= EV9000_LOGICDEVICE_STATUS_INTEL;
                }
                else if (ALARM_STATUS_CLOSE == pGBLogicDeviceInfo->alarm_status)
                {
                    stEV9000GBLogicDevice.nStatus |= EV9000_LOGICDEVICE_STATUS_ONLINE;
                    stEV9000GBLogicDevice.nStatus |= EV9000_LOGICDEVICE_STATUS_ALARM;
                }
                else if (ALARM_STATUS_APART == pGBLogicDeviceInfo->alarm_status)
                {
                    stEV9000GBLogicDevice.nStatus |= EV9000_LOGICDEVICE_STATUS_ONLINE;
                }
                else
                {
                    stEV9000GBLogicDevice.nStatus |= EV9000_LOGICDEVICE_STATUS_ONLINE;
                }
            }
            else if (2 == pGBLogicDeviceInfo->status)
            {
                stEV9000GBLogicDevice.nStatus |= EV9000_LOGICDEVICE_STATUS_ONLINE;
                stEV9000GBLogicDevice.nStatus |= EV9000_LOGICDEVICE_STATUS_NOVIDEO;
            }
            else
            {
                stEV9000GBLogicDevice.nStatus |= EV9000_LOGICDEVICE_STATUS_OFFLINE;
            }

            stEV9000GBLogicDevice.nStatus = htonl(stEV9000GBLogicDevice.nStatus);

            /* 经度 */
            stEV9000GBLogicDevice.dLongitude = pGBLogicDeviceInfo->longitude;

            /* 纬度 */
            stEV9000GBLogicDevice.dLatitude = pGBLogicDeviceInfo->latitude;

            /* 所属图层 */
            osip_strncpy(stEV9000GBLogicDevice.strResved2, pGBLogicDeviceInfo->map_layer, EV9000_SHORT_STRING_LEN);

            /* 报警设备子类型 */
            stEV9000GBLogicDevice.nResved1 = htonl(pGBLogicDeviceInfo->alarm_device_sub_type);

            /* 所属的CMS ID */
            osip_strncpy(stEV9000GBLogicDevice.strCMSID, pGBLogicDeviceInfo->cms_id, EV9000_IDCODE_LEN);

            /* 上海地标对应的通道ID */
            //iChannel = shdb_get_channel_by_device_index(pGBLogicDeviceInfo->id, pUser_Srv_dboper);

            if (pGBLogicDeviceInfo->shdb_channel_id > 0)
            {
                stEV9000GBLogicDevice.nResved3 = htonl(pGBLogicDeviceInfo->shdb_channel_id);
            }

            /* 鹰眼相机对应的预案ID */
            if (pGBLogicDeviceInfo->strResved2[0] != '\0')
            {
                osip_strncpy(stEV9000GBLogicDevice.strResved4, pGBLogicDeviceInfo->strResved2, EV9000_SHORT_STRING_LEN);
            }

            iRet = send(user_sock, &stEV9000GBLogicDevice, sizeof(EV9000_GBLogicDeviceConfigEx), 0);

            if (iRet <= 0)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "用户通过TCP方式获取逻辑设备信息, 发送结果给用户失败:用户IP=%s, 端口号=%d, Socket=%d, 发送次数=%d, 返回结果=%d, errno=%d", user_ip, user_port, user_sock, index + 1, iRet, errno);
            }
            else
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "用户通过TCP方式获取逻辑设备信息, 发送结果给用户成功:用户IP=%s, 端口号=%d, Socket=%d, 发送次数=%d, 返回结果=%d", user_ip, user_port, user_sock, index + 1, iRet);
            }
        }
        else/* 可能是数据库新增加逻辑设备，加载到内存中 */
        {
            iRet = load_db_data_to_LogicDevice_info_list_by_device_id(pUser_Srv_dboper, (char*)DeviceIDVector[index].c_str());
            DEBUG_TRACE(MODULE_USER, LOG_INFO, "UserGetAllGBDeviceListAndSendCataLogToClientForTCP() load_db_data_to_LogicDevice_info_list_by_device_id: DeviceID=%s, iRet=%d \r\n", (char*)DeviceIDVector[index].c_str(), iRet);

            if (iRet == 0)
            {
                pGBLogicDeviceInfo = GBLogicDevice_info_find((char*)DeviceIDVector[index].c_str());

                if (NULL != pGBLogicDeviceInfo && pGBLogicDeviceInfo->enable == 1)
                {
                    /* 设备索引 */
                    stEV9000GBLogicDevice.nID = htonl(pGBLogicDeviceInfo->id);

                    /* 设备统一编号 */
                    osip_strncpy(stEV9000GBLogicDevice.strDeviceID, pGBLogicDeviceInfo->device_id, EV9000_IDCODE_LEN);

                    /* 点位名称 */
                    osip_strncpy(stEV9000GBLogicDevice.strDeviceName, pGBLogicDeviceInfo->device_name, EV9000_NORMAL_STRING_LEN);

                    /* 是否可控 */
                    if (1 == pGBLogicDeviceInfo->ctrl_enable)
                    {
                        stEV9000GBLogicDevice.nCtrlEnable = htonl(1);
                    }
                    else
                    {
                        stEV9000GBLogicDevice.nCtrlEnable = htonl(0);
                    }

                    /* 是否支持对讲 */
                    stEV9000GBLogicDevice.nMicEnable = htonl(pGBLogicDeviceInfo->mic_enable);

                    /* 帧率 */
                    stEV9000GBLogicDevice.nFrameCount = htonl(pGBLogicDeviceInfo->frame_count);

                    /* 码流数 */
                    stEV9000GBLogicDevice.nStreamCount = htonl(pGBLogicDeviceInfo->stream_count);

                    /* 点位状态 */
                    if (1 == pGBLogicDeviceInfo->status)
                    {
                        if (INTELLIGENT_STATUS_ON == pGBLogicDeviceInfo->intelligent_status)
                        {
                            stEV9000GBLogicDevice.nStatus |= EV9000_LOGICDEVICE_STATUS_ONLINE;
                            stEV9000GBLogicDevice.nStatus |= EV9000_LOGICDEVICE_STATUS_INTEL;
                        }
                        else if (ALARM_STATUS_CLOSE == pGBLogicDeviceInfo->alarm_status)
                        {
                            stEV9000GBLogicDevice.nStatus |= EV9000_LOGICDEVICE_STATUS_ONLINE;
                            stEV9000GBLogicDevice.nStatus |= EV9000_LOGICDEVICE_STATUS_ALARM;
                        }
                        else if (ALARM_STATUS_APART == pGBLogicDeviceInfo->alarm_status)
                        {
                            stEV9000GBLogicDevice.nStatus |= EV9000_LOGICDEVICE_STATUS_ONLINE;
                        }
                        else
                        {
                            stEV9000GBLogicDevice.nStatus |= EV9000_LOGICDEVICE_STATUS_ONLINE;
                        }
                    }
                    else if (2 == pGBLogicDeviceInfo->status)
                    {
                        stEV9000GBLogicDevice.nStatus |= EV9000_LOGICDEVICE_STATUS_ONLINE;
                        stEV9000GBLogicDevice.nStatus |= EV9000_LOGICDEVICE_STATUS_NOVIDEO;
                    }
                    else
                    {
                        stEV9000GBLogicDevice.nStatus |= EV9000_LOGICDEVICE_STATUS_OFFLINE;
                    }

                    stEV9000GBLogicDevice.nStatus = htonl(stEV9000GBLogicDevice.nStatus);

                    /* 经度 */
                    stEV9000GBLogicDevice.dLongitude = pGBLogicDeviceInfo->longitude;

                    /* 纬度 */
                    stEV9000GBLogicDevice.dLatitude = pGBLogicDeviceInfo->latitude;

                    /* 所属图层 */
                    osip_strncpy(stEV9000GBLogicDevice.strResved2, pGBLogicDeviceInfo->map_layer, EV9000_SHORT_STRING_LEN);

                    /* 报警设备子类型 */
                    stEV9000GBLogicDevice.nResved1 = htonl(pGBLogicDeviceInfo->alarm_device_sub_type);

                    /* 所属的CMS ID */
                    osip_strncpy(stEV9000GBLogicDevice.strCMSID, pGBLogicDeviceInfo->cms_id, EV9000_IDCODE_LEN);

                    /* 上海地标对应的通道ID */
                    //iChannel = shdb_get_channel_by_device_index(pGBLogicDeviceInfo->id, pUser_Srv_dboper);

                    if (pGBLogicDeviceInfo->shdb_channel_id > 0)
                    {
                        stEV9000GBLogicDevice.nResved3 = htonl(pGBLogicDeviceInfo->shdb_channel_id);
                    }

                    /* 鹰眼相机对应的预案ID */
                    if (pGBLogicDeviceInfo->strResved2[0] != '\0')
                    {
                        osip_strncpy(stEV9000GBLogicDevice.strResved4, pGBLogicDeviceInfo->strResved2, EV9000_SHORT_STRING_LEN);
                    }

                    iRet = send(user_sock, &stEV9000GBLogicDevice, sizeof(EV9000_GBLogicDeviceConfigEx), 0);

                    if (iRet <= 0)
                    {
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "用户通过TCP方式获取逻辑设备信息, 发送结果给用户失败:用户IP=%s, 端口号=%d, Socket=%d, 发送次数=%d, 返回结果=%d, errno=%d", user_ip, user_port, user_sock, index + 1, iRet, errno);
                    }
                    else
                    {
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "用户通过TCP方式获取逻辑设备信息, 发送结果给用户成功:用户IP=%s, 端口号=%d, Socket=%d, 发送次数=%d, 返回结果=%d", user_ip, user_port, user_sock, index + 1, iRet);
                    }
                }
            }
        }
    }

    if (i == 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "用户通过TCP方式获取逻辑设备信息成功:用户IP=%s, 端口号=%d, Socket=%d, 逻辑设备数目=%d", user_ip, user_port, user_sock, record_count);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "User success to get logic device information through TCP:User IP=%s, Port=%d, Socket=%d, The number of logical devices=%d", user_ip, user_port, user_sock, record_count);
    }
    else
    {
        SystemLog(EV9000_CMS_GET_CATALOG_ERROR, EV9000_LOG_LEVEL_ERROR, "用户通过TCP方式获取逻辑设备信息失败:用户IP=%s, 端口号=%d, Socket=%d, 原因=%s", user_ip, user_port, user_sock, (char*)"发送消息失败");
        EnSystemLog(EV9000_CMS_GET_CATALOG_ERROR, EV9000_LOG_LEVEL_ERROR, "User failed to get logic device information through TCP:User IP=%s, Port=%d, Socket=%d, reason=%s", user_ip, user_port, user_sock, (char*)"Send message failed.");
    }

    DEBUG_TRACE(MODULE_USER, LOG_TRACE, "UserGetAllGBDeviceListAndSendCataLogToClientForTCP Exit--- \r\n");

    return 0;
}

/*****************************************************************************
 函 数 名  : FindUserGroupDevicePermConfig
 功能描述  : 查找用户所在组拥有的设备列表，加入队列
 输入参数  : int user_index
             DBOper* pUser_Srv_dboper
             vector<string>& DeviceIDVector
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年7月26日 星期五
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int FindUserGroupDevicePermConfig(unsigned int user_index, DBOper* pUser_Srv_dboper, vector<string>& DeviceIDVector)
{
    //int i = 0;
    int record_count = 0;
    char tmp[16] = {0};
    string strSQL = "";
    int while_count = 0;

    if ((user_index <= 0) || (NULL == pUser_Srv_dboper))
    {
        return -1;
    }

    DEBUG_TRACE(MODULE_USER, LOG_TRACE, "FindUserGroupDevicePermConfig Enter--- \r\n");
    //DebugRunTrace("用户获取逻辑设备信息开始查询用户组权限表数据库:用户Index=%d\r\n", user_index);

    strSQL.clear();
    strSQL = "select GDC.DeviceID from UserMapGroupConfig as UMGC, UserGroupDevicePermConfig as UGDPC, GBLogicDeviceConfig as GDC WHERE UMGC.GroupID = UGDPC.GroupID and UGDPC.DeviceIndex = GDC.ID and GDC.Enable=1 and (GDC.DeviceType <180 or GDC.DeviceType >185) and UMGC.UserIndex = ";
    snprintf(tmp, 16, "%u", user_index);
    strSQL += tmp;

    record_count = pUser_Srv_dboper->DB_Select(strSQL.c_str(), 1);

    if (record_count < 0)
    {
        //DebugRunTrace("用户获取逻辑设备信息查询用户组权限表数据库失败:用户Index=%d, 原因=%s:%s\r\n", user_index, (char*)"查询数据库失败", pUser_Srv_dboper->GetLastDbErrorMsg());
        DEBUG_TRACE(MODULE_USER, LOG_ERROR, "FindUserGroupDevicePermConfig() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
        DEBUG_TRACE(MODULE_USER, LOG_ERROR, "FindUserGroupDevicePermConfig() ErrorMsg=%s\r\n", pUser_Srv_dboper->GetLastDbErrorMsg());
        return -1;
    }
    else if (record_count > 0)
    {
        //将DeviceIndex加入容器
        do
        {
            string tmp_svalue = "";

            while_count++;

            if (while_count % 10000 == 0)
            {
                DEBUG_TRACE(MODULE_USER, LOG_WARN, "FindUserGroupDevicePermConfig() While Count=%d \r\n", while_count);
            }

            tmp_svalue.clear();
            pUser_Srv_dboper->GetFieldValue(0, tmp_svalue);

            if (tmp_svalue.empty())
            {
                DEBUG_TRACE(MODULE_USER, LOG_ERROR, "FindUserGroupDevicePermConfig() DeviceIndex Error \r\n");
                continue;
            }

            //DEBUG_TRACE(MODULE_USER, LOG_INFO, "FindUserGroupDevicePermConfig() DeviceIndex=%u \r\n", tmp_uivalue);
            //DeviceIDVector.push_back(tmp_svalue);
            AddDeviceIndexToDeviceIDVector(DeviceIDVector, tmp_svalue);
        }
        while (pUser_Srv_dboper->MoveNext() >= 0);
    }

    //DebugRunTrace("用户获取逻辑设备信息查询用户组权限表数据库成功:用户Index=%d, 记录数=%d\r\n", user_index, record_count);
    DEBUG_TRACE(MODULE_USER, LOG_TRACE, "FindUserGroupDevicePermConfig Exit--- \r\n");

    return 0;
}
/*****************************************************************************
 函 数 名  : FindUserRCUGroupDevicePermConfig
 功能描述  : 查找用户所在组拥有的设备列表，加入队列
 输入参数  : int user_index
             DBOper* pUser_Srv_dboper
             vector<string>& DeviceIDVector
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年10月31日 星期一
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int FindRCUUserGroupDevicePermConfig(unsigned int user_index, DBOper* pUser_Srv_dboper, vector<string>& DeviceIDVector)
{
    //int i = 0;
    int record_count = 0;
    char tmp[16] = {0};
    string strSQL = "";
    int while_count = 0;

    if ((user_index <= 0) || (NULL == pUser_Srv_dboper))
    {
        return -1;
    }

    DEBUG_TRACE(MODULE_USER, LOG_TRACE, "FindRCUUserGroupDevicePermConfig Enter--- \r\n");
    //DebugRunTrace("用户获取逻辑设备信息开始查询用户组权限表数据库:用户Index=%d\r\n", user_index);

    strSQL.clear();
    strSQL = "select GDC.DeviceID from UserMapGroupConfig as UMGC, UserGroupDevicePermConfig as UGDPC, GBLogicDeviceConfig as GDC WHERE UMGC.GroupID = UGDPC.GroupID and UGDPC.DeviceIndex = GDC.ID and GDC.Enable=1 and GDC.DeviceType >=180 and GDC.DeviceType <=185 and UMGC.UserIndex = ";
    snprintf(tmp, 16, "%u", user_index);
    strSQL += tmp;

    record_count = pUser_Srv_dboper->DB_Select(strSQL.c_str(), 1);

    if (record_count < 0)
    {
        //DebugRunTrace("用户获取逻辑设备信息查询用户组权限表数据库失败:用户Index=%d, 原因=%s:%s\r\n", user_index, (char*)"查询数据库失败", pUser_Srv_dboper->GetLastDbErrorMsg());
        DEBUG_TRACE(MODULE_USER, LOG_ERROR, "FindRCUUserGroupDevicePermConfig() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
        DEBUG_TRACE(MODULE_USER, LOG_ERROR, "FindRCUUserGroupDevicePermConfig() ErrorMsg=%s\r\n", pUser_Srv_dboper->GetLastDbErrorMsg());
        return -1;
    }
    else if (record_count > 0)
    {
        //将DeviceIndex加入容器
        do
        {
            string tmp_svalue = "";

            while_count++;

            if (while_count % 10000 == 0)
            {
                DEBUG_TRACE(MODULE_USER, LOG_WARN, "FindRCUUserGroupDevicePermConfig() While Count=%d \r\n", while_count);
            }

            tmp_svalue.clear();
            pUser_Srv_dboper->GetFieldValue(0, tmp_svalue);

            if (tmp_svalue.empty())
            {
                DEBUG_TRACE(MODULE_USER, LOG_ERROR, "FindRCUUserGroupDevicePermConfig() DeviceIndex Error \r\n");
                continue;
            }

            //DEBUG_TRACE(MODULE_USER, LOG_INFO, "FindRCUUserGroupDevicePermConfig() DeviceIndex=%u \r\n", tmp_uivalue);
            //DeviceIDVector.push_back(tmp_svalue);
            AddDeviceIndexToDeviceIDVector(DeviceIDVector, tmp_svalue);
        }
        while (pUser_Srv_dboper->MoveNext() >= 0);
    }

    //DebugRunTrace("用户获取逻辑设备信息查询用户组权限表数据库成功:用户Index=%d, 记录数=%d\r\n", user_index, record_count);
    DEBUG_TRACE(MODULE_USER, LOG_TRACE, "FindRCUUserGroupDevicePermConfig Exit--- \r\n");

    return 0;
}

/*****************************************************************************
 函 数 名  : FindUserDevicePermConfig
 功能描述  : 查找用户的设备权限列表，加入队列
 输入参数  : int user_index
             DBOper* pUser_Srv_dboper
             vector<string>& DeviceIDVector
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年7月26日 星期五
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int FindUserDevicePermConfig(unsigned int user_index, DBOper* pUser_Srv_dboper, vector<string>& DeviceIDVector)
{
    //int i = 0;
    int record_count = 0;
    char tmp[16] = {0};
    string strSQL = "";
    int while_count = 0;

    if ((user_index <= 0) || (NULL == pUser_Srv_dboper))
    {
        DEBUG_TRACE(MODULE_USER, LOG_DEBUG, "FindUserDevicePermConfig() exit---: Param Error \r\n");
        return -1;
    }

    DEBUG_TRACE(MODULE_USER, LOG_TRACE, "FindUserDevicePermConfig Enter--- \r\n");
    //DebugRunTrace("用户获取逻辑设备信息开始查询逻辑设备权限表数据库:用户Index=%d\r\n", user_index);

    strSQL.clear();
    strSQL = "select GDC.DeviceID from UserDevicePermConfig as UDPC, GBLogicDeviceConfig as GDC WHERE UDPC.DeviceIndex = GDC.ID and GDC.Enable=1 and (GDC.DeviceType <180 or GDC.DeviceType >185) and UDPC.UserIndex = ";
    snprintf(tmp, 16, "%u", user_index);
    strSQL += tmp;

    record_count = pUser_Srv_dboper->DB_Select(strSQL.c_str(), 1);

    if (record_count < 0)
    {
        //DebugRunTrace("用户获取逻辑设备信息查询逻辑设备权限表数据库失败:用户Index=%d, 原因=%s:%s\r\n", user_index, (char*)"查询数据库失败", pUser_Srv_dboper->GetLastDbErrorMsg());
        DEBUG_TRACE(MODULE_USER, LOG_ERROR, "FindUserDevicePermConfig() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
        DEBUG_TRACE(MODULE_USER, LOG_ERROR, "FindUserDevicePermConfig() ErrorMsg=%s\r\n", pUser_Srv_dboper->GetLastDbErrorMsg());
        return -1;
    }
    else if (record_count > 0)
    {
        //将DeviceIndex加入容器
        do
        {
            string tmp_svalue = "";

            while_count++;

            if (while_count % 10000 == 0)
            {
                DEBUG_TRACE(MODULE_USER, LOG_WARN, "FindUserDevicePermConfig() While Count=%d \r\n", while_count);
            }

            tmp_svalue.clear();
            pUser_Srv_dboper->GetFieldValue(0, tmp_svalue);

            if (tmp_svalue.empty())
            {
                DEBUG_TRACE(MODULE_USER, LOG_ERROR, "FindUserDevicePermConfig() DeviceIndex Error \r\n");
                continue;
            }

            //DEBUG_TRACE(MODULE_USER, LOG_INFO, "FindUserDevicePermConfig() DeviceIndex=%u \r\n", tmp_uivalue);
            //DeviceIDVector.push_back(tmp_svalue);
            AddDeviceIndexToDeviceIDVector(DeviceIDVector, tmp_svalue);
        }
        while (pUser_Srv_dboper->MoveNext() >= 0);
    }

    //DebugRunTrace("用户获取逻辑设备信息查询逻辑设备权限表数据库成功:用户Index=%d,记录数=%d\r\n", user_index, record_count);
    DEBUG_TRACE(MODULE_USER, LOG_TRACE, "FindUserDevicePermConfig Exit--- \r\n");

    return 0;
}

/*****************************************************************************
 函 数 名  : FindRCUUserDevicePermConfig
 功能描述  : 查找用户的设备权限列表，加入队列
 输入参数  : int user_index
             DBOper* pUser_Srv_dboper
             vector<string>& DeviceIDVector
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年7月26日 星期五
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int FindRCUUserDevicePermConfig(unsigned int user_index, DBOper* pUser_Srv_dboper, vector<string>& DeviceIDVector)
{
    //int i = 0;
    int record_count = 0;
    char tmp[16] = {0};
    string strSQL = "";
    int while_count = 0;

    if ((user_index <= 0) || (NULL == pUser_Srv_dboper))
    {
        DEBUG_TRACE(MODULE_USER, LOG_DEBUG, "FindRCUUserDevicePermConfig() exit---: Param Error \r\n");
        return -1;
    }

    DEBUG_TRACE(MODULE_USER, LOG_TRACE, "FindRCUUserDevicePermConfig Enter--- \r\n");
    //DebugRunTrace("用户获取逻辑设备信息开始查询逻辑设备权限表数据库:用户Index=%d\r\n", user_index);

    strSQL.clear();
    strSQL = "select GDC.DeviceID from UserDevicePermConfig as UDPC, GBLogicDeviceConfig as GDC WHERE UDPC.DeviceIndex = GDC.ID and GDC.Enable=1 and GDC.DeviceType >=180 and GDC.DeviceType <=185 and UDPC.UserIndex = ";
    snprintf(tmp, 16, "%u", user_index);
    strSQL += tmp;

    record_count = pUser_Srv_dboper->DB_Select(strSQL.c_str(), 1);

    if (record_count < 0)
    {
        //DebugRunTrace("用户获取逻辑设备信息查询逻辑设备权限表数据库失败:用户Index=%d, 原因=%s:%s\r\n", user_index, (char*)"查询数据库失败", pUser_Srv_dboper->GetLastDbErrorMsg());
        DEBUG_TRACE(MODULE_USER, LOG_ERROR, "FindRCUUserDevicePermConfig() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
        DEBUG_TRACE(MODULE_USER, LOG_ERROR, "FindRCUUserDevicePermConfig() ErrorMsg=%s\r\n", pUser_Srv_dboper->GetLastDbErrorMsg());
        return -1;
    }
    else if (record_count > 0)
    {
        //将DeviceIndex加入容器
        do
        {
            string tmp_svalue = "";

            while_count++;

            if (while_count % 10000 == 0)
            {
                DEBUG_TRACE(MODULE_USER, LOG_WARN, "FindRCUUserDevicePermConfig() While Count=%d \r\n", while_count);
            }

            tmp_svalue.clear();
            pUser_Srv_dboper->GetFieldValue(0, tmp_svalue);

            if (tmp_svalue.empty())
            {
                DEBUG_TRACE(MODULE_USER, LOG_ERROR, "FindRCUUserDevicePermConfig() DeviceIndex Error \r\n");
                continue;
            }

            //DEBUG_TRACE(MODULE_USER, LOG_INFO, "FindRCUUserDevicePermConfig() DeviceIndex=%u \r\n", tmp_uivalue);
            //DeviceIDVector.push_back(tmp_svalue);
            AddDeviceIndexToDeviceIDVector(DeviceIDVector, tmp_svalue);
        }
        while (pUser_Srv_dboper->MoveNext() >= 0);
    }

    //DebugRunTrace("用户获取逻辑设备信息查询逻辑设备权限表数据库成功:用户Index=%d,记录数=%d\r\n", user_index, record_count);
    DEBUG_TRACE(MODULE_USER, LOG_TRACE, "FindRCUUserDevicePermConfig Exit--- \r\n");

    return 0;
}

/*****************************************************************************
 函 数 名  : AddDeviceIndexToSortVector
 功能描述  : 根据名称排序逻辑设备index
 输入参数  : vector<unsigned int>& allDeviceIndexVector
                            vector<unsigned int>& DeviceIndexVector
                            vector<unsigned int>& sortDeviceIndexVector
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年5月21日 星期三
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int AddDeviceIndexToSortVector(vector<unsigned int>& allDeviceIndexVector, vector<unsigned int>& DeviceIndexVector, vector<unsigned int>& sortDeviceIndexVector)
{
    int index = 0;
    unsigned int device_index = 0;

    for (index = 0; index < (int)allDeviceIndexVector.size(); index++) /* 循环查找排序的所有index容器 */
    {
        device_index = allDeviceIndexVector[index];

        vector<unsigned int>::iterator result = std::find(DeviceIndexVector.begin(), DeviceIndexVector.end(), device_index); /* 如果index 在要发出去的容器中 */

        if (result == DeviceIndexVector.end())  // 没有找到
        {
            continue;
        }

        /* 添加到排序容器*/
        sortDeviceIndexVector.push_back(device_index);
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : CreateGBLogicDeviceCatalogResponseXMLHead
 功能描述  : 创建用户获取逻辑设备列表回应消息XML头部
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
int CreateGBLogicDeviceCatalogResponseXMLHead(CPacket** pOutPacket, int query_count, int record_count, char* strSN, char* strDeviceID, DOMElement** ListAccNode)
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
            DEBUG_TRACE(MODULE_USER, LOG_DEBUG, "CreateGBLogicDeviceCatalogResponseXMLHead() exit---: Create XML Packet Error \r\n");
            return -1;
        }

        if (record_count <= MAX_USER_CATALOG_COUT_SEND)
        {
            snprintf(strRecordCount, 32, "%d", record_count);
        }
        else
        {
            snprintf(strRecordCount, 32, "%d", MAX_USER_CATALOG_COUT_SEND);
        }

        (*pOutPacket)->SetRootTag("Response");

        AccNode = (*pOutPacket)->CreateElement((char*)"CmdType");
        (*pOutPacket)->SetElementValue(AccNode, (char*)"Catalog");

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

        AccNode = (*pOutPacket)->CreateElement((char*)"Result");
        (*pOutPacket)->SetElementValue(AccNode, (char*)"OK");

        AccNode = (*pOutPacket)->CreateElement((char*)"SumNum");
        (*pOutPacket)->SetElementValue(AccNode, strSumNum);

        (*ListAccNode) = (*pOutPacket)->CreateElement((char*)"DeviceList");
        (*pOutPacket)->SetElementAttr((*ListAccNode), (char*)"Num", strRecordCount);
    }
    else if ((query_count % MAX_USER_CATALOG_COUT_SEND == 1) && (record_count - query_count >= MAX_USER_CATALOG_COUT_SEND))
    {
        *pOutPacket = new CPacket();

        if (NULL == *pOutPacket)
        {
            DEBUG_TRACE(MODULE_USER, LOG_DEBUG, "CreateGBLogicDeviceCatalogResponseXMLHead() exit---: Create XML Packet Error \r\n");
            return -1;
        }

        snprintf(strRecordCount, 32, "%d", MAX_USER_CATALOG_COUT_SEND);
        (*pOutPacket)->SetRootTag("Response");

        AccNode = (*pOutPacket)->CreateElement((char*)"CmdType");
        (*pOutPacket)->SetElementValue(AccNode, (char*)"Catalog");

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

        AccNode = (*pOutPacket)->CreateElement((char*)"Result");
        (*pOutPacket)->SetElementValue(AccNode, (char*)"OK");

        AccNode = (*pOutPacket)->CreateElement((char*)"SumNum");
        (*pOutPacket)->SetElementValue(AccNode, strSumNum);

        (*ListAccNode) = (*pOutPacket)->CreateElement((char*)"DeviceList");
        (*pOutPacket)->SetElementAttr((*ListAccNode), (char*)"Num", strRecordCount);
    }
    else if ((query_count % MAX_USER_CATALOG_COUT_SEND == 1) && (record_count - query_count < MAX_USER_CATALOG_COUT_SEND))
    {
        *pOutPacket = new CPacket();

        if (NULL == *pOutPacket)
        {
            DEBUG_TRACE(MODULE_USER, LOG_DEBUG, "CreateGBLogicDeviceCatalogResponseXMLHead() exit---: Create XML Packet Error \r\n");
            return -1;
        }

        snprintf(strRecordCount, 32, "%d", record_count - query_count + 1);
        (*pOutPacket)->SetRootTag("Response");

        AccNode = (*pOutPacket)->CreateElement((char*)"CmdType");
        (*pOutPacket)->SetElementValue(AccNode, (char*)"Catalog");

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

        AccNode = (*pOutPacket)->CreateElement((char*)"Result");
        (*pOutPacket)->SetElementValue(AccNode, (char*)"OK");

        AccNode = (*pOutPacket)->CreateElement((char*)"SumNum");
        (*pOutPacket)->SetElementValue(AccNode, strSumNum);

        (*ListAccNode) = (*pOutPacket)->CreateElement((char*)"DeviceList");
        (*pOutPacket)->SetElementAttr((*ListAccNode), (char*)"Num", strRecordCount);
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : AddLogicDeviceInfoToXMLItem
 功能描述  : 添加逻辑设备信息到XML的Item
 输入参数  : CPacket* pOutPacket
             DOMElement* ListAccNode
             char* device_id
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年7月27日 星期六
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int AddLogicDeviceInfoToXMLItem(CPacket* pOutPacket, DOMElement* ListAccNode, char* device_id, DBOper* pUser_Srv_dboper)
{
    GBLogicDevice_info_t* pGBLogicDeviceInfo = NULL;
    //GBLogicDevice_info_t* pDBGBLogicDeviceInfo = NULL;
    DOMElement* ItemAccNode = NULL;
    DOMElement* AccNode = NULL;
    DOMElement* ItemInfoNode = NULL;

    char strID[64] = {0};
    //char strParental[16] = {0};
    //char strSafetyWay[16] = {0};
    //char strRegisterWay[16] = {0};
    //char strCertifiable[16] = {0};
    //char strErrCode[16] = {0};
    //char strSecrecy[16] = {0};
    //char strPort[16] = {0};
    char strFrameCount[64] = {0};
    char strStreamCount[64] = {0};
    char strLongitude[64] = {0};
    char strLatitude[64] = {0};
    char strAlarmDeviceSubType[64] = {0};
    char strPTZType[16] = {0};
    int iRet = 0;

    if (NULL == pOutPacket || NULL == ListAccNode || NULL == device_id)
    {
        DEBUG_TRACE(MODULE_USER, LOG_DEBUG, "AddLogicDeviceInfoToXMLItem() exit---: Param Error \r\n");
        return -1;
    }

    /* 填写XML数据*/
    pOutPacket->SetCurrentElement(ListAccNode);
    ItemAccNode = pOutPacket->CreateElement((char*)"Item");
    pOutPacket->SetCurrentElement(ItemAccNode);

    /* 根据Index 获取逻辑设备信息，可能是只配置了物理设备，还没有上线，数据库和内存中都没有的*/
    pGBLogicDeviceInfo = GBLogicDevice_info_find(device_id);

    if (NULL != pGBLogicDeviceInfo && pGBLogicDeviceInfo->enable == 1)
    {
        if (pGBLogicDeviceInfo->enable == 1)
        {
            /* 设备索引 */
            AccNode = pOutPacket->CreateElement((char*)"ID");
            snprintf(strID, 64, "%u", pGBLogicDeviceInfo->id);
            pOutPacket->SetElementValue(AccNode, strID);

            /* 设备统一编号 */
            AccNode = pOutPacket->CreateElement((char*)"DeviceID");
            pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->device_id);

            /* 点位名称 */
            AccNode = pOutPacket->CreateElement((char*)"Name");
            pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->device_name);

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
            snprintf(strFrameCount, 64, "%u", pGBLogicDeviceInfo->frame_count);
            pOutPacket->SetElementValue(AccNode, strFrameCount);

            /* 码流数 */
            AccNode = pOutPacket->CreateElement((char*)"StreamCount");
            snprintf(strStreamCount, 64, "%u", pGBLogicDeviceInfo->stream_count);
            pOutPacket->SetElementValue(AccNode, strStreamCount);

#if 0
            /* 设备生产商 */
            AccNode = pOutPacket->CreateElement((char*)"Manufacturer");

            if (NULL != pGBLogicDeviceInfo->manufacturer)
            {
                pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->manufacturer);
            }
            else
            {
                pOutPacket->SetElementValue(AccNode, (char*)"");
            }

            /* 设备型号 */
            AccNode = pOutPacket->CreateElement((char*)"Model");

            if (NULL != pGBLogicDeviceInfo->model)
            {
                pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->model);
            }
            else
            {
                pOutPacket->SetElementValue(AccNode, (char*)"");
            }

            /* 设备归属 */
            AccNode = pOutPacket->CreateElement((char*)"Owner");

            if (NULL != pGBLogicDeviceInfo->owner)
            {
                pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->owner);
            }
            else
            {
                pOutPacket->SetElementValue(AccNode, (char*)"");
            }

            /* 行政区域 */
            AccNode = pOutPacket->CreateElement((char*)"CivilCode");

            if (NULL != pGBLogicDeviceInfo->civil_code)
            {
                pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->civil_code);
            }
            else
            {
                pOutPacket->SetElementValue(AccNode, (char*)"");
            }

            /* 警区 */
            AccNode = pOutPacket->CreateElement((char*)"Block");

            if (NULL != pGBLogicDeviceInfo->block)
            {
                pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->block);
            }
            else
            {
                pOutPacket->SetElementValue(AccNode, (char*)"");
            }

            /* 安装地址 */
            AccNode = pOutPacket->CreateElement((char*)"Address");

            if (NULL != pGBLogicDeviceInfo->address)
            {
                pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->address);
            }
            else
            {
                pOutPacket->SetElementValue(AccNode, (char*)"");
            }

            /* 是否有子设备 */
            AccNode = pOutPacket->CreateElement((char*)"Parental");
            snprintf(strParental, 16, "%d", pGBLogicDeviceInfo->parental);
            pOutPacket->SetElementValue(AccNode, strParental);

            /* 父设备/区域/系统ID */
            AccNode = pOutPacket->CreateElement((char*)"ParentID");

            if (NULL != pGBLogicDeviceInfo->parentID)
            {
                pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->parentID);
            }
            else
            {
                pOutPacket->SetElementValue(AccNode, (char*)"");
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

            if (NULL != pGBLogicDeviceInfo->cert_num)
            {
                pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->cert_num);
            }
            else
            {
                pOutPacket->SetElementValue(AccNode, (char*)"");
            }

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

            if (NULL != pGBLogicDeviceInfo->end_time)
            {
                pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->end_time);
            }
            else
            {
                pOutPacket->SetElementValue(AccNode, (char*)"");
            }

            /* 保密属性 */
            AccNode = pOutPacket->CreateElement((char*)"Secrecy");
            snprintf(strSecrecy, 16, "%d", pGBLogicDeviceInfo->secrecy);
            pOutPacket->SetElementValue(AccNode, strSecrecy);

            /* IP地址*/
            AccNode = pOutPacket->CreateElement((char*)"IPAddress");

            if (NULL != pGBLogicDeviceInfo->ip_address)
            {
                pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->ip_address);
            }
            else
            {
                pOutPacket->SetElementValue(AccNode, (char*)"");
            }

            /* 端口号 */
            AccNode = pOutPacket->CreateElement((char*)"Port");
            snprintf(strPort, 16, "%d", pGBLogicDeviceInfo->port);
            pOutPacket->SetElementValue(AccNode, strPort);

            /* 密码*/
            AccNode = pOutPacket->CreateElement((char*)"Password");

            if (NULL != pGBLogicDeviceInfo->password)
            {
                pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->password);
            }
            else
            {
                pOutPacket->SetElementValue(AccNode, (char*)"");
            }

#endif

            /* 点位状态 */
            AccNode = pOutPacket->CreateElement((char*)"Status");

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

            /* 经度 */
            AccNode = pOutPacket->CreateElement((char*)"Longitude");
            snprintf(strLongitude, 64, "%.16lf", pGBLogicDeviceInfo->longitude);
            pOutPacket->SetElementValue(AccNode, strLongitude);

            /* 纬度 */
            AccNode = pOutPacket->CreateElement((char*)"Latitude");
            snprintf(strLatitude, 64, "%.16lf", pGBLogicDeviceInfo->latitude);
            pOutPacket->SetElementValue(AccNode, strLatitude);

            /* 所属图层 */
            AccNode = pOutPacket->CreateElement((char*)"MapLayer");
            pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->map_layer);

            /* 报警设备子类型 */
            AccNode = pOutPacket->CreateElement((char*)"ChlType");
            snprintf(strAlarmDeviceSubType, 64, "%u", pGBLogicDeviceInfo->alarm_device_sub_type);
            pOutPacket->SetElementValue(AccNode, strAlarmDeviceSubType);

            /* 所属的CMS ID */
            AccNode = pOutPacket->CreateElement((char*)"CMSID");
            pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->cms_id);
        }

        //DEBUG_TRACE(MODULE_USER, LOG_DEBUG, "AddLogicDeviceInfoToXMLItem() DeviceID=%s, Longitude:=%s, Latitude=%s \r\n", pGBLogicDeviceInfo->device_id, strLongitude, strLatitude);
    }
    else/* 可能是数据库新增加逻辑设备，加载到内存中，再组XML 消息 */
    {
        iRet = load_db_data_to_LogicDevice_info_list_by_device_id(pUser_Srv_dboper, device_id);
        DEBUG_TRACE(MODULE_USER, LOG_INFO, "AddLogicDeviceInfoToXMLItem() load_db_data_to_LogicDevice_info_list_by_device_id: DeviceID=%s, iRet=%d \r\n", device_id, iRet);

        if (iRet == 0)
        {
            pGBLogicDeviceInfo = GBLogicDevice_info_find(device_id);

            if (NULL != pGBLogicDeviceInfo && pGBLogicDeviceInfo->enable == 1)
            {
                /* 设备索引 */
                AccNode = pOutPacket->CreateElement((char*)"ID");
                snprintf(strID, 64, "%u", pGBLogicDeviceInfo->id);
                pOutPacket->SetElementValue(AccNode, strID);

                /* 设备统一编号 */
                AccNode = pOutPacket->CreateElement((char*)"DeviceID");
                pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->device_id);

                /* 点位名称 */
                AccNode = pOutPacket->CreateElement((char*)"Name");
                pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->device_name);

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
                snprintf(strFrameCount, 64, "%u", pGBLogicDeviceInfo->frame_count);
                pOutPacket->SetElementValue(AccNode, strFrameCount);

                /* 码流数 */
                AccNode = pOutPacket->CreateElement((char*)"StreamCount");
                snprintf(strStreamCount, 64, "%u", pGBLogicDeviceInfo->stream_count);
                pOutPacket->SetElementValue(AccNode, strStreamCount);

#if 0
                /* 设备生产商 */
                AccNode = pOutPacket->CreateElement((char*)"Manufacturer");

                if (NULL != pGBLogicDeviceInfo->manufacturer)
                {
                    pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->manufacturer);
                }
                else
                {
                    pOutPacket->SetElementValue(AccNode, (char*)"");
                }

                /* 设备型号 */
                AccNode = pOutPacket->CreateElement((char*)"Model");

                if (NULL != pGBLogicDeviceInfo->model)
                {
                    pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->model);
                }
                else
                {
                    pOutPacket->SetElementValue(AccNode, (char*)"");
                }

                /* 设备归属 */
                AccNode = pOutPacket->CreateElement((char*)"Owner");

                if (NULL != pGBLogicDeviceInfo->owner)
                {
                    pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->owner);
                }
                else
                {
                    pOutPacket->SetElementValue(AccNode, (char*)"");
                }

                /* 行政区域 */
                AccNode = pOutPacket->CreateElement((char*)"CivilCode");

                if (NULL != pGBLogicDeviceInfo->civil_code)
                {
                    pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->civil_code);
                }
                else
                {
                    pOutPacket->SetElementValue(AccNode, (char*)"");
                }

                /* 警区 */
                AccNode = pOutPacket->CreateElement((char*)"Block");

                if (NULL != pGBLogicDeviceInfo->block)
                {
                    pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->block);
                }
                else
                {
                    pOutPacket->SetElementValue(AccNode, (char*)"");
                }

                /* 安装地址 */
                AccNode = pOutPacket->CreateElement((char*)"Address");

                if (NULL != pGBLogicDeviceInfo->address)
                {
                    pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->address);
                }
                else
                {
                    pOutPacket->SetElementValue(AccNode, (char*)"");
                }

                /* 是否有子设备 */
                AccNode = pOutPacket->CreateElement((char*)"Parental");
                snprintf(strParental, 16, "%d", pGBLogicDeviceInfo->parental);
                pOutPacket->SetElementValue(AccNode, strParental);

                /* 父设备/区域/系统ID */
                AccNode = pOutPacket->CreateElement((char*)"ParentID");

                if (NULL != pGBLogicDeviceInfo->parentID)
                {
                    pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->parentID);
                }
                else
                {
                    pOutPacket->SetElementValue(AccNode, (char*)"");
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

                if (NULL != pGBLogicDeviceInfo->cert_num)
                {
                    pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->cert_num);
                }
                else
                {
                    pOutPacket->SetElementValue(AccNode, (char*)"");
                }

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

                if (NULL != pGBLogicDeviceInfo->end_time)
                {
                    pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->end_time);
                }
                else
                {
                    pOutPacket->SetElementValue(AccNode, (char*)"");
                }

                /* 保密属性 */
                AccNode = pOutPacket->CreateElement((char*)"Secrecy");
                snprintf(strSecrecy, 16, "%d", pGBLogicDeviceInfo->secrecy);
                pOutPacket->SetElementValue(AccNode, strSecrecy);

                /* IP地址*/
                AccNode = pOutPacket->CreateElement((char*)"IPAddress");

                if (NULL != pGBLogicDeviceInfo->ip_address)
                {
                    pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->ip_address);
                }
                else
                {
                    pOutPacket->SetElementValue(AccNode, (char*)"");
                }

                /* 端口号 */
                AccNode = pOutPacket->CreateElement((char*)"Port");
                snprintf(strPort, 16, "%d", pGBLogicDeviceInfo->port);
                pOutPacket->SetElementValue(AccNode, strPort);

                /* 密码*/
                AccNode = pOutPacket->CreateElement((char*)"Password");

                if (NULL != pGBLogicDeviceInfo->password)
                {
                    pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->password);
                }
                else
                {
                    pOutPacket->SetElementValue(AccNode, (char*)"");
                }

#endif

                /* 点位状态 */
                AccNode = pOutPacket->CreateElement((char*)"Status");

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

                /* 经度 */
                AccNode = pOutPacket->CreateElement((char*)"Longitude");
                snprintf(strLongitude, 64, "%.16lf", pGBLogicDeviceInfo->longitude);
                pOutPacket->SetElementValue(AccNode, strLongitude);

                /* 纬度 */
                AccNode = pOutPacket->CreateElement((char*)"Latitude");
                snprintf(strLatitude, 64, "%.16lf", pGBLogicDeviceInfo->latitude);
                pOutPacket->SetElementValue(AccNode, strLatitude);

                /* 所属图层 */
                AccNode = pOutPacket->CreateElement((char*)"MapLayer");
                pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->map_layer);

                /* 报警设备子类型 */
                AccNode = pOutPacket->CreateElement((char*)"ChlType");
                snprintf(strAlarmDeviceSubType, 64, "%u", pGBLogicDeviceInfo->alarm_device_sub_type);
                pOutPacket->SetElementValue(AccNode, strAlarmDeviceSubType);

                /* 所属的CMS ID */
                AccNode = pOutPacket->CreateElement((char*)"CMSID");
                pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->cms_id);

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
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : AddDeviceIndexToDeviceIndexVector
 功能描述  : 添加逻辑设备索引到容器队列
 输入参数  : vector<int>& DeviceIndexVector
             unsigned int DeviceIndex
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年7月26日 星期五
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int AddDeviceIndexToDeviceIndexVector(vector<unsigned int>& DeviceIndexVector, unsigned int DeviceIndex)
{
    vector<unsigned int>::iterator result = std::find(DeviceIndexVector.begin(), DeviceIndexVector.end(), DeviceIndex);

    if (result == DeviceIndexVector.end())  //没找到
    {
        DeviceIndexVector.push_back(DeviceIndex);
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : AddDeviceIndexToDeviceIDVector
 功能描述  : 添加逻辑设备ID到容器队列
 输入参数  : vector<string>& DeviceIDVector
             string& DeviceID
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年9月12日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int AddDeviceIndexToDeviceIDVector(vector<string>& DeviceIDVector, string& DeviceID)
{
    vector<string>::iterator result = std::find(DeviceIDVector.begin(), DeviceIDVector.end(), DeviceID);

    if (result == DeviceIDVector.end())  //没找到
    {
        DeviceIDVector.push_back(DeviceID);
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : AddUserIndexToUserIndexVector
 功能描述  : 添加用户索引到容器队列
 输入参数  : vector<unsigned int>& UserIndexVector
             unsigned int UserIndex
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年1月5日 星期一
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int AddUserIndexToUserIndexVector(vector<unsigned int>& UserIndexVector, unsigned int UserIndex)
{
    vector<unsigned int>::iterator result = std::find(UserIndexVector.begin(), UserIndexVector.end(), UserIndex);

    if (result == UserIndexVector.end())  //没找到
    {
        UserIndexVector.push_back(UserIndex);
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : UpdateUserRegInfo2DB
 功能描述  : 更新用户状态到数据库
 输入参数  : int user_pos
                            DBOper* pUser_Srv_dboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年8月7日 星期三
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int UpdateUserRegInfo2DB(char* user_id, int reg_status, DBOper* pUser_Srv_dboper)
{
    int iRet = 0;

    string strSQL = "";
    char strStatus[16] = {0};

    //printf("\r\n UpdateUserRegInfo2DB() Enter--- \r\n");

    if (NULL == user_id)
    {
        DEBUG_TRACE(MODULE_USER, LOG_DEBUG, "UpdateUserRegInfo2DB() exit---: Param Error \r\n");
        return -1;
    }

    if (NULL == pUser_Srv_dboper)
    {
        DEBUG_TRACE(MODULE_USER, LOG_DEBUG, "UpdateUserRegInfo2DB() exit---: User Srv DB Oper Error \r\n");
        return -1;
    }

    /* 更新数据库 */
    strSQL.clear();
    strSQL = "UPDATE UserConfig SET Status = ";
    snprintf(strStatus, 16, "%d", reg_status);
    strSQL += strStatus;
    strSQL += " WHERE UserID like '";
    strSQL += user_id;
    strSQL += "'";

    iRet = pUser_Srv_dboper->DB_Update(strSQL.c_str(), 1);

    if (iRet < 0)
    {
        DEBUG_TRACE(MODULE_USER, LOG_ERROR, "UpdateUserRegInfo2DB() DB Oper Error: strSQL=%s, iRet=%d \r\n", strSQL.c_str(), iRet);
        DEBUG_TRACE(MODULE_USER, LOG_ERROR, "UpdateUserRegInfo2DB() ErrorMsg=%s\r\n", pUser_Srv_dboper->GetLastDbErrorMsg());
    }

    //printf("\r\n UpdateUserRegInfo2DB() Exit--- \r\n");

    return 0;
}

int UpdateUserRegInfo2DB2(string strUserID, DBOper* pUser_Srv_dboper)
{
    user_info_t* pUserInfo = NULL;

    if (strUserID.empty())
    {
        return -1;
    }

    if (NULL == pUser_Srv_dboper)
    {
        DEBUG_TRACE(MODULE_USER, LOG_DEBUG, "UpdateUserRegInfo2DB() exit---: User Srv DB Oper Error \r\n");
        return -1;
    }

    DEBUG_TRACE(MODULE_USER, LOG_TRACE, "UpdateUserRegInfo2DB2() Enter--- \r\n");

    int iRet = 0;
    int iRegStatus = 0;
    string strSQL = "";
    char strStatus[16] = {0};

    USER_INFO_SMUTEX_LOCK();

    User_Info_Iterator iter = g_UserInfoMap.begin();
    User_Info_Iterator tmp_iter;

    while (iter != g_UserInfoMap.end())
    {
        pUserInfo = iter->second;

        if ((NULL == pUserInfo) || (pUserInfo->user_id[0] == '\0'))
        {
            tmp_iter = iter;
            iter++;
            g_UserInfoMap.erase(tmp_iter);
            continue;
        }

        if ((strcmp((char*)strUserID.c_str(), pUserInfo->user_id) == 0) && pUserInfo->reg_status)
        {
            iRegStatus = 1;
            break;
        }

        iter++;
    }

    USER_INFO_SMUTEX_UNLOCK();

    DEBUG_TRACE(MODULE_USER, LOG_TRACE, "UpdateUserRegInfo2DB2() UPDATE UserConfig Begin--- \r\n");

    /* 更新数据库 */
    strSQL.clear();
    strSQL = "UPDATE UserConfig SET Status = ";
    snprintf(strStatus, 16, "%d", iRegStatus);
    strSQL += strStatus;
    strSQL += " WHERE UserID like '";
    strSQL += strUserID;
    strSQL += "'";

    iRet = pUser_Srv_dboper->DB_Update(strSQL.c_str(), 1);

    DEBUG_TRACE(MODULE_USER, LOG_TRACE, "UpdateUserRegInfo2DB2() UPDATE UserConfig End--- \r\n");

    if (iRet < 0)
    {
        DEBUG_TRACE(MODULE_USER, LOG_ERROR, "UpdateUserRegInfo2DB2() DB Oper Error:strSQL=%s, iRet=%d \r\n", strSQL.c_str(), iRet);
        DEBUG_TRACE(MODULE_USER, LOG_ERROR, "UpdateUserRegInfo2DB2() ErrorMsg=%s\r\n", pUser_Srv_dboper->GetLastDbErrorMsg());
    }

    DEBUG_TRACE(MODULE_USER, LOG_TRACE, "UpdateUserRegInfo2DB2() Exit--- \r\n");

    return iRet;
}

/*****************************************************************************
 函 数 名  : UpdateAllUserRegInfo2DB
 功能描述  : 更新所有用户的注册状态到数据库
 输入参数  : int reg_status
             DBOper* pUser_Srv_dboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年4月15日 星期三
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int UpdateAllUserRegInfo2DB(int reg_status, DBOper* pUser_Srv_dboper)
{
    int iRet = 0;

    string strSQL = "";
    char strStatus[16] = {0};

    //printf("\r\n UpdateAllUserRegInfo2DB() Enter--- \r\n");

    if (NULL == pUser_Srv_dboper)
    {
        DEBUG_TRACE(MODULE_USER, LOG_DEBUG, "UpdateAllUserRegInfo2DB() exit---: User Srv DB Oper Error \r\n");
        return -1;
    }

    /* 更新数据库 */
    strSQL.clear();
    strSQL = "UPDATE UserConfig SET Status = ";
    snprintf(strStatus, 16, "%d", reg_status);
    strSQL += strStatus;

    iRet = pUser_Srv_dboper->DB_Update(strSQL.c_str(), 1);

    if (iRet < 0)
    {
        DEBUG_TRACE(MODULE_USER, LOG_ERROR, "UpdateAllUserRegInfo2DB() DB Oper Error: strSQL=%s, iRet=%d \r\n", strSQL.c_str(), iRet);
        DEBUG_TRACE(MODULE_USER, LOG_ERROR, "UpdateAllUserRegInfo2DB() ErrorMsg=%s\r\n", pUser_Srv_dboper->GetLastDbErrorMsg());
    }

    //printf("\r\n UpdateAllUserRegInfo2DB() Exit--- \r\n");

    return 0;
}

/*****************************************************************************
 函 数 名  : AddUserRegInfo2DB
 功能描述  : 添加在线用户到数据库
 输入参数  : unsigned int uUserIndex
             char* strLoginIP
             int iLoginPort
             DBOper* pUser_Reg_dboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年10月21日 星期二
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int AddUserRegInfo2DB(unsigned int uUserIndex, char* strLoginIP, int iLoginPort, DBOper* pUser_Reg_dboper)
{
    int iRet = 0;
    string strQuerySQL = "";
    string strInsertSQL = "";
    char strUserIndex[64] = {0};
    char strLoginPort[64] = {0};
    int record_count = -1;

    if (NULL == strLoginIP || iLoginPort <= 0)
    {
        return -1;
    }

    if (NULL == pUser_Reg_dboper)
    {
        DEBUG_TRACE(MODULE_USER, LOG_DEBUG, "AddUserRegInfo2DB() exit---: User Reg DB Oper Error \r\n");
        return -1;
    }

    snprintf(strUserIndex, 64, "%u", uUserIndex);
    snprintf(strLoginPort, 64, "%d", iLoginPort);

    /* 获取数据库记录 */
    strQuerySQL.clear();
    strQuerySQL = "select * from OnLineUserConfig";
    strQuerySQL += " WHERE ID = ";
    strQuerySQL += strUserIndex;
    strQuerySQL += " AND LoginIP like '";
    strQuerySQL += strLoginIP;
    strQuerySQL += "' AND Port = ";
    strQuerySQL += strLoginPort;

    record_count = pUser_Reg_dboper->DB_Select(strQuerySQL.c_str(), 1);

    DEBUG_TRACE(MODULE_USER, LOG_TRACE, "AddUserRegInfo2DB() record_count=%d, strQuerySQL=%s \r\n", record_count, strQuerySQL.c_str());

    if (record_count < 0) /* 数据库错误 */
    {
        DEBUG_TRACE(MODULE_USER, LOG_ERROR, "AddUserRegInfo2DB() DB Oper Error:strSQL=%s, record_count=%d \r\n", strQuerySQL.c_str(), record_count);
        DEBUG_TRACE(MODULE_USER, LOG_ERROR, "AddUserRegInfo2DB() ErrorMsg=%s\r\n", pUser_Reg_dboper->GetLastDbErrorMsg());
        return -1;
    }
    else if (record_count == 0) /* 没有记录 */
    {
        strInsertSQL.clear();
        strInsertSQL = "insert into OnLineUserConfig (ID,LoginIP,Port) values (";

        strInsertSQL += strUserIndex;
        strInsertSQL += ",";

        strInsertSQL += "'";
        strInsertSQL += strLoginIP;
        strInsertSQL += "'";
        strInsertSQL += ",";

        strInsertSQL += strLoginPort;

        strInsertSQL += ")";

        iRet = pUser_Reg_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);

        if (iRet < 0)
        {
            DEBUG_TRACE(MODULE_USER, LOG_ERROR, "AddUserRegInfo2DB() DB Oper Error:strInsertSQL=%s, iRet=%d \r\n", strInsertSQL.c_str(), iRet);
            DEBUG_TRACE(MODULE_USER, LOG_ERROR, "AddUserRegInfo2DB() ErrorMsg=%s\r\n", pUser_Reg_dboper->GetLastDbErrorMsg());
            return -1;
        }
    }

    return iRet;
}

/*****************************************************************************
 函 数 名  : DeleteUserRegInfoFromDB
 功能描述  : 从数据库中删除在线用户
 输入参数  : unsigned int uUserIndex
             char* strLoginIP
             int iLoginPort
             DBOper* pUser_Reg_dboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年10月21日 星期二
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int DeleteUserRegInfoFromDB(unsigned int uUserIndex, char* strLoginIP, int iLoginPort, DBOper* pUser_Reg_dboper)
{
    int iRet = 0;
    string strDeleteSQL = "";
    char strUserIndex[64] = {0};
    char strLoginPort[64] = {0};
    //int record_count = -1;

    if (NULL == strLoginIP || iLoginPort <= 0 || NULL == pUser_Reg_dboper)
    {
        return -1;
    }

    if (NULL == pUser_Reg_dboper)
    {
        DEBUG_TRACE(MODULE_USER, LOG_DEBUG, "DeleteUserRegInfoFromDB() exit---: User Reg DB Oper Error \r\n");
        return -1;
    }

    snprintf(strUserIndex, 64, "%u", uUserIndex);
    snprintf(strLoginPort, 64, "%d", iLoginPort);

    strDeleteSQL.clear();
    strDeleteSQL = "delete from OnLineUserConfig";
    strDeleteSQL += " WHERE ID = ";
    strDeleteSQL += strUserIndex;
    strDeleteSQL += " AND LoginIP like '";
    strDeleteSQL += strLoginIP;
    strDeleteSQL += "' AND Port = ";
    strDeleteSQL += strLoginPort;

    iRet = pUser_Reg_dboper->DB_Delete(strDeleteSQL.c_str(), 1);

    if (iRet < 0)
    {
        DEBUG_TRACE(MODULE_USER, LOG_ERROR, "DeleteUserRegInfoFromDB() DB Oper Error:strDeleteSQL=%s, iRet=%d \r\n", strDeleteSQL.c_str(), iRet);
        DEBUG_TRACE(MODULE_USER, LOG_ERROR, "DeleteUserRegInfoFromDB() ErrorMsg=%s\r\n", pUser_Reg_dboper->GetLastDbErrorMsg());
        return -1;
    }

    return iRet;
}

/*****************************************************************************
 函 数 名  : DeleteAllUserRegInfoFromDB
 功能描述  : 从数据库中删除所有在线用户
 输入参数  : DBOper* pUser_Reg_dboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年4月15日 星期三
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int DeleteAllUserRegInfoFromDB(DBOper* pUser_Reg_dboper)
{
    int iRet = 0;
    string strDeleteSQL = "";

    if (NULL == pUser_Reg_dboper)
    {
        return -1;
    }

    if (NULL == pUser_Reg_dboper)
    {
        DEBUG_TRACE(MODULE_USER, LOG_DEBUG, "DeleteAllUserRegInfoFromDB() exit---: User Reg DB Oper Error \r\n");
        return -1;
    }

    strDeleteSQL.clear();
    strDeleteSQL = "delete from OnLineUserConfig";

    iRet = pUser_Reg_dboper->DB_Delete(strDeleteSQL.c_str(), 1);

    if (iRet < 0)
    {
        DEBUG_TRACE(MODULE_USER, LOG_ERROR, "DeleteAllUserRegInfoFromDB() DB Oper Error:strDeleteSQL=%s, iRet=%d \r\n", strDeleteSQL.c_str(), iRet);
        DEBUG_TRACE(MODULE_USER, LOG_ERROR, "DeleteAllUserRegInfoFromDB() ErrorMsg=%s\r\n", pUser_Reg_dboper->GetLastDbErrorMsg());
        return -1;
    }

    return iRet;
}

/*****************************************************************************
 函 数 名  : NotifyOnlineUserToAllClientUser
 功能描述  : 通知在线用户信息
 输入参数  : user_info_t* pUserInfo
             int event
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
int NotifyOnlineUserToAllClientUser(user_info_t* pUserInfo, int event, DBOper* pdboper)
{
    int i = 0;
    CPacket outPacket;
    DOMElement* AccNode = NULL;
    char strUserIndex[64] = {0};
    char strPort[16] = {0};
    string strSQL = "";
    int record_count = 0;
    string strUserName = "";
    string strRealName = "";
    string strTel = "";
    string strResved2 = "";

    if (NULL == pUserInfo || pUserInfo->user_index <= 0 || pUserInfo->login_ip[0] == '\0' || pUserInfo->login_port <= 0 || NULL == pdboper)
    {
        DEBUG_TRACE(MODULE_USER, LOG_ERROR, "NotifyOnlineUserToAllClientUser() exit---: Param Error \r\n");
        return -1;
    }

    snprintf(strUserIndex, 64 , "%u", pUserInfo->user_index);

    strSQL.clear();
    strSQL = "select * from UserConfig WHERE ID = ";
    strSQL += strUserIndex;

    record_count = pdboper->DB_Select(strSQL.c_str(), 1);

    if (record_count < 0)
    {
        DEBUG_TRACE(MODULE_USER, LOG_ERROR, "NotifyOnlineUserToAllClientUser() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
        DEBUG_TRACE(MODULE_USER, LOG_ERROR, "NotifyOnlineUserToAllClientUser() ErrorMsg=%s\r\n", pdboper->GetLastDbErrorMsg());
        return -1;
    }
    else if (record_count == 0)
    {
        DEBUG_TRACE(MODULE_USER, LOG_WARN, "NotifyOnlineUserToAllClientUser() exit---: No Record Count \r\n");
        return 0;
    }

    /* 用户名 */
    strUserName.clear();
    pdboper->GetFieldValue("UserName", strUserName);

    /* 真实名 */
    strRealName.clear();
    pdboper->GetFieldValue("RealName", strRealName);

    /* 联系方式 */
    strTel.clear();
    pdboper->GetFieldValue("Tel", strTel);

    /* 备注2 */
    strResved2.clear();
    pdboper->GetFieldValue("Resved2", strResved2);

    /* 回复响应,组建消息 */
    outPacket.SetRootTag("Notify");
    AccNode = outPacket.CreateElement((char*)"CmdType");
    outPacket.SetElementValue(AccNode, (char*)"OnLineUser");

    AccNode = outPacket.CreateElement((char*)"SN");
    outPacket.SetElementValue(AccNode, (char*)"124");

    AccNode = outPacket.CreateElement((char*)"ID");
    outPacket.SetElementValue(AccNode, strUserIndex);

    AccNode = outPacket.CreateElement((char*)"UserName");
    outPacket.SetElementValue(AccNode, (char*)strUserName.c_str());

    AccNode = outPacket.CreateElement((char*)"RealName");
    outPacket.SetElementValue(AccNode, (char*)strRealName.c_str());

    AccNode = outPacket.CreateElement((char*)"Tel");
    outPacket.SetElementValue(AccNode, (char*)strTel.c_str());

    AccNode = outPacket.CreateElement((char*)"Resved2");
    outPacket.SetElementValue(AccNode, (char*)strResved2.c_str());

    AccNode = outPacket.CreateElement((char*)"LoginIP");
    outPacket.SetElementValue(AccNode, pUserInfo->login_ip);

    AccNode = outPacket.CreateElement((char*)"Port");
    snprintf(strPort, 16 , "%d", pUserInfo->login_port);
    outPacket.SetElementValue(AccNode, strPort);

    AccNode = outPacket.CreateElement((char*)"Event");

    if (1 == event)
    {
        outPacket.SetElementValue(AccNode, (char*)"ADD");
    }
    else if (2 == event)
    {
        outPacket.SetElementValue(AccNode, (char*)"DEL");
    }

    i = SendMessageToExceptOnlineUser(pUserInfo, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

    return i;
}

/*****************************************************************************
 函 数 名  : IsUserHasPermissionForDevice
 功能描述  : 检查用户是否有该点位的权限
 输入参数  : unsigned int device_index
             user_info_t* pUserInfo
             DBOper* pDboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年8月15日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int IsUserHasPermissionForDevice(unsigned int device_index, user_info_t* pUserInfo, DBOper* pDboper)
{
    int record_count = 0;
    char tmp[16] = {0};
    string strSQL = "";

    if ((device_index <= 0) || (NULL == pUserInfo) || (NULL == pDboper))
    {
        return 0;
    }

    /* 1、先根据用户Index 查找用户所在的用户组，再根据用户组查找其权限 */
    strSQL.clear();
    strSQL = "select GDC.ID from UserMapGroupConfig as UMGC, UserGroupDevicePermConfig as UGDPC, GBLogicDeviceConfig as GDC WHERE UMGC.GroupID = UGDPC.GroupID and UGDPC.DeviceIndex = GDC.ID and GDC.Enable=1 and UMGC.UserIndex = ";
    snprintf(tmp, 16, "%u", pUserInfo->user_index);
    strSQL += tmp;
    strSQL += " AND GDC.ID = ";
    snprintf(tmp, 16, "%u", device_index);
    strSQL += tmp;

    record_count = pDboper->DB_Select(strSQL.c_str(), 1);

    if (record_count <= 0)
    {
        /* 没找到,再查找逻辑设备权限表 */
        /* 2、根据用户Index 查询用户设备权限表 */
        strSQL.clear();
        strSQL = "select GDC.ID from UserDevicePermConfig as UDPC, GBLogicDeviceConfig as GDC WHERE UDPC.DeviceIndex = GDC.ID and GDC.Enable=1 and UDPC.UserIndex = ";
        snprintf(tmp, 16, "%u", pUserInfo->user_index);
        strSQL += tmp;
        strSQL += " AND GDC.ID = ";
        snprintf(tmp, 16, "%u", device_index);
        strSQL += tmp;

        record_count = pDboper->DB_Select(strSQL.c_str(), 1);

        if (record_count <= 0)
        {
            return 0;
        }
        else if (record_count > 0)
        {
            return 1;
        }
    }
    else if (record_count > 0)
    {
        return 1;
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : ShowOnlineUser
 功能描述  : 显示在线用户
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
void ShowOnlineUser(int sock)
{
    char strLine[] = "\r-------------------------------------------------------------------------------------------------------------------------------------------------------------\r\n";
    char strHead[] = "\rUser ID              Login IP        Login Port  Status AlarmSendStatus TVWallSendStatus RegServerEthName RegServerIP     UASIndex TCPSock KeepAliveTCPSocket\r\n";

    user_info_t* pUserInfo = NULL;
    User_Info_Iterator UserInfoItr;

    char rbuf[256] = {0};

    if (sock > 0)
    {
        send(sock, strLine, strlen(strLine), 0);
        send(sock, strHead, strlen(strHead), 0);
    }

    USER_INFO_SMUTEX_LOCK();

    if (g_UserInfoMap.size() <= 0)
    {
        USER_INFO_SMUTEX_UNLOCK();
        return;
    }

    for (UserInfoItr = g_UserInfoMap.begin(); UserInfoItr != g_UserInfoMap.end(); UserInfoItr++)
    {
        pUserInfo = UserInfoItr->second;

        if ((NULL == pUserInfo) || (pUserInfo->user_id[0] == '\0') || (pUserInfo->reg_info_index < 0))
        {
            continue;
        }

        snprintf(rbuf, 256, "\r%-20s %-15s %-11d %-6d %-15d %-16d %-16s %-15s %-8d %-7d %-18d\r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, pUserInfo->reg_status, pUserInfo->alarm_info_send_flag, pUserInfo->tvwall_status_send_flag, pUserInfo->strRegServerEthName, pUserInfo->strRegServerIP, pUserInfo->reg_info_index, pUserInfo->tcp_sock, pUserInfo->tcp_keep_alive_sock);

        if (sock > 0)
        {
            send(sock, rbuf, strlen(rbuf), 0);
        }
    }

    if (sock > 0)
    {
        send(sock, strLine, strlen(strLine), 0);
    }

    USER_INFO_SMUTEX_UNLOCK();
    return;
}
