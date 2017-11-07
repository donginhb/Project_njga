/******************************************************************************

                  版权所有 (C), 2001-2013, 金智视讯技术有限公司

 ******************************************************************************
  文 件 名   : registrar.c
  版 本 号   : 初稿
  作    者   : yanghaifeng
  生成日期   : 2013年4月1日
  最近修改   :
  功能描述   : SIP 注册服务
  函数列表   :
              build_contact
              ProcRegister
              register_response_add_contacts
              reg_url_compare
              reg_url_free
              reg_url_init
  修改历史   :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 创建文件

******************************************************************************/

/*----------------------------------------------*
 * 包含头文件                                   *
 *----------------------------------------------*/
#ifdef WIN32
#include <winsock2.h>
#else
#include <sys/socket.h>
#endif

#include <malloc.h>
#include <osipparser2/osip_port.h>
#include "gbltype.h"
#include "registrar.inc"

#include "gblfunc.inc"
#include "csdbg.inc"

#include "sipmsg.inc"
#include "timerproc.inc"
#include "callback.inc"

//added by chenyu 130522
#ifdef WIN32
#define vsnprintf _vsnprintf
#define snprintf  _snprintf
#endif

#ifdef MEMORY_LEAKS1
static int freesipuascptr = 0;
static int freesipuaccptr = 0;
#endif

/*----------------------------------------------*
 * 外部变量说明                                 *
 *----------------------------------------------*/
extern app_callback_t* g_AppCallback;

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
UAS_Reg_Info_MAP g_UasRegInfoMap;
UAC_Reg_Info_MAP g_UacRegInfoMap;
#ifdef MULTI_THR
osip_mutex_t* g_UasRegInfoMapLock = NULL;
osip_mutex_t* g_UacRegInfoMapLock = NULL;
#endif

/*----------------------------------------------*
 * 常量定义                                     *
 *----------------------------------------------*/
#define MAX_REG_UAS        3000  /*最大注册的用户数*/
#define MAX_REG_UAC        500   /*最大对外注册的用户数*/

/*----------------------------------------------*
 * 宏定义                                       *
 *----------------------------------------------*/

#if DECS("服务器端注册管理")
/*****************************************************************************
 函 数 名  : uas_reginfo_init
 功能描述  : 服务端注册信息结构初始化
 输入参数  : uas_reg_info_t** uas_reg_info
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年5月23日 星期四
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int uas_reginfo_init(uas_reg_info_t** uas_reg_info)
{
    *uas_reg_info = new uas_reg_info_t;

    if (*uas_reg_info == NULL)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "uas_reginfo_init() exit---: *uas_reg_info Smalloc Error \r\n");
        return -1;
    }

    (*uas_reg_info)->iUsed = 0;
    (*uas_reg_info)->register_id[0] = '\0';

    /* auth info */
    (*uas_reg_info)->register_account[0] = '\0';
    (*uas_reg_info)->register_password[0] = '\0';

    /* register routes (get from REGISTER )*/
    (*uas_reg_info)->login_time = 0;
    (*uas_reg_info)->last_active_time = 0;

    (*uas_reg_info)->authorization = NULL;

    (*uas_reg_info)->cseqnum = 0;
    (*uas_reg_info)->callid[0] = '\0';
    (*uas_reg_info)->expires = 0;
    (*uas_reg_info)->expires_count = 0;
    (*uas_reg_info)->q_param = 0.00;

    (*uas_reg_info)->from_host[0] = '\0';
    (*uas_reg_info)->from_port = 0;
    (*uas_reg_info)->contact_url = NULL;

    (*uas_reg_info)->serverid[0] = '\0';
    (*uas_reg_info)->serverip[0] = '\0';
    (*uas_reg_info)->serverport = 0;

    (*uas_reg_info)->tr = NULL;
    (*uas_reg_info)->sip = NULL;

#ifdef MEMORY_LEAKS1
    static int comptr = 0;
    comptr++;
    freesipuascptr++;

    fprintf(stdout, (char*)"\r\n<register.c> uas_reginfo_init() malloc: (address = 0x%lx) comptr: %d, existing element %d\r\n", (long unsigned int)*uas_reg_info, comptr, freesipuascptr);
    fflush(stdout);
#endif

    return 0;
}

/*****************************************************************************
 函 数 名  : uas_reginfo_free
 功能描述  : 服务端注册信息结构释放
 输入参数  : uas_reg_info_t* uas_reg_info
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年5月23日 星期四
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void uas_reginfo_free(uas_reg_info_t* uas_reg_info)
{
    if (NULL == uas_reg_info)
    {
        //SIP_DEBUG_TRACE(LOG_DEBUG, "uas_reginfo_free() exit---: Param Error \r\n");
        return;
    }

    uas_reg_info->iUsed = 0;

    memset(uas_reg_info->register_id, 0, 132);

    memset(uas_reg_info->register_account, 0, 132);
    memset(uas_reg_info->register_password, 0, 132);

    uas_reg_info->login_time = 0;
    uas_reg_info->last_active_time = 0;

    if (NULL != uas_reg_info->authorization)
    {
        osip_authorization_free(uas_reg_info->authorization);
        uas_reg_info->authorization = NULL;
    }

    memset(uas_reg_info->callid, 0, 132);

    uas_reg_info->expires = 0;
    uas_reg_info->expires_count = 0;
    uas_reg_info->cseqnum = 0;
    uas_reg_info->q_param = 0.00;

    memset(uas_reg_info->from_host, 0, 16);

    uas_reg_info->from_port = 0;

    if (NULL != uas_reg_info->contact_url)
    {
        osip_uri_free(uas_reg_info->contact_url);
        uas_reg_info->contact_url = NULL;
    }

    memset(uas_reg_info->serverid, 0, 132);
    memset(uas_reg_info->serverip, 0, 16);

    uas_reg_info->serverport = 0;

    uas_reg_info->tr = NULL;
    uas_reg_info->sip = NULL;

#ifdef MEMORY_LEAKS1
    static int comptr = 0;
    comptr++;
    freesipuascptr--;

    fprintf(stdout, (char*)"\r\n<register.c> uas_reginfo_free() free: (address = 0x%lx) comptr: %d, existing element %d\r\n", (long unsigned int)uas_reg_info, comptr, freesipuascptr);
    fflush(stdout);
#endif

    return;
}

/*****************************************************************************
 函 数 名  : uas_reginfo_list_init
 功能描述  : 服务端注册信息队列初始化
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
int uas_reginfo_list_init()
{
    int i = 0;
    int pos = 0;

    g_UasRegInfoMap.clear();

    for (pos = 0; pos < MAX_REG_UAS; pos++)
    {
        uas_reg_info_t* pUasRegInfo = NULL;

        i = uas_reginfo_init(&pUasRegInfo);

        if (i != 0)
        {
            SIP_DEBUG_TRACE(LOG_DEBUG, "uas_reginfo_list_init() exit---: UAS Register Info Init Error \r\n");
            return -1;
        }

        g_UasRegInfoMap[pos] = pUasRegInfo;
    }

#ifdef MULTI_THR
    g_UasRegInfoMapLock = (osip_mutex_t*)osip_mutex_init();

    if (NULL == g_UasRegInfoMapLock)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "uas_reginfo_list_init() exit---: UAS Register Info Map Lock Init Error \r\n");
        return -1;
    }

#endif
    return 0;
}

/*****************************************************************************
 函 数 名  : uas_reginfo_list_free
 功能描述  : 服务端注册信息队列释放
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
void uas_reginfo_list_free()
{
    UAS_Reg_Info_Iterator Itr;
    uas_reg_info_t* uas_reg_info = NULL;

    for (Itr = g_UasRegInfoMap.begin(); Itr != g_UasRegInfoMap.end(); Itr++)
    {
        uas_reg_info = Itr->second;

        if (NULL != uas_reg_info)
        {
            uas_reginfo_free(uas_reg_info);
            delete uas_reg_info;
            uas_reg_info = NULL;
        }
    }

    g_UasRegInfoMap.clear();

#ifdef MULTI_THR

    if (NULL != g_UasRegInfoMapLock)
    {
        osip_mutex_destroy((struct osip_mutex*)g_UasRegInfoMapLock);
        g_UasRegInfoMapLock = NULL;
    }

#endif
    return;
}

/*****************************************************************************
 函 数 名  : uas_reginfo_list_lock
 功能描述  : 服务端注册信息队列锁定
 输入参数  : 无
 输出参数  : int
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年5月23日 星期四
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int uas_reginfo_list_lock()
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_UasRegInfoMapLock == NULL)
    {
        //SIP_DEBUG_TRACE(LOG_DEBUG, "uas_reginfo_list_lock() exit---: Param Error \r\n");
        return -1;
    }

    iRet = osip_mutex_lock((struct osip_mutex*)g_UasRegInfoMapLock);

#endif
    return iRet;
}

/*****************************************************************************
 函 数 名  : uas_reginfo_list_unlock
 功能描述  : 服务端注册信息队列解锁
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
int uas_reginfo_list_unlock()
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_UasRegInfoMapLock == NULL)
    {
        //SIP_DEBUG_TRACE(LOG_DEBUG, "uas_reginfo_list_unlock() exit---: Param Error \r\n");
        return -1;
    }

    iRet = osip_mutex_unlock((struct osip_mutex*)g_UasRegInfoMapLock);

#endif
    return iRet;
}

/*****************************************************************************
 函 数 名  : debug_uas_reginfo_list_lock
 功能描述  : 服务端注册信息队列锁定
 输入参数  : 无
 输出参数  : int
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年5月23日 星期四
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int debug_uas_reginfo_list_lock(const char* file, int line, const char* func)
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_UasRegInfoMapLock == NULL)
    {
        //SIP_DEBUG_TRACE(LOG_DEBUG, "debug_uas_reginfo_list_lock() exit---: Param Error \r\n");
        return -1;
    }

#ifndef WIN32   //modified by chenyu 131024
    iRet = osip_debug_mutex_lock((struct osip_mutex*)g_UasRegInfoMapLock, file, line, func);
#else
    iRet = osip_mutex_lock((struct osip_mutex*)g_UasRegInfoMapLock);
#endif

#endif
    return iRet;
}

/*****************************************************************************
 函 数 名  : debug_uas_reginfo_list_unlock
 功能描述  : 服务端注册信息队列解锁
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
int debug_uas_reginfo_list_unlock(const char* file, int line, const char* func)
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_UasRegInfoMapLock == NULL)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "debug_uas_reginfo_list_unlock() exit---: Param Error \r\n");
        return -1;
    }

#ifndef WIN32   //modified by chenyu 131024
    iRet = osip_debug_mutex_unlock((struct osip_mutex*)g_UasRegInfoMapLock, file, line, func);
#else
    iRet = osip_mutex_unlock((struct osip_mutex*)g_UasRegInfoMapLock);
#endif

#endif
    return iRet;
}

/*****************************************************************************
 函 数 名  : uas_reginfo_find
 功能描述  : 服务端注册信息查找
 输入参数  : char* user
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年5月23日 星期四
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int uas_reginfo_find(char* user, char* orig_host, int orig_port)
{
    uas_reg_info_t* pUasRegInfo = NULL;
    UAS_Reg_Info_Iterator Itr;

    if (user == NULL || orig_host == NULL || orig_port <= 0)
    {
        //SIP_DEBUG_TRACE(LOG_DEBUG, "uas_reginfo_find() exit---: Param Error \r\n");
        return -1;
    }

    UAS_SMUTEX_LOCK();

    if (g_UasRegInfoMap.size() <= 0)
    {
        UAS_SMUTEX_UNLOCK();
        //SIP_DEBUG_TRACE(LOG_DEBUG, "uas_reginfo_find() exit---: UAS Register Info Map NULL \r\n");
        return -1;
    }

    for (Itr = g_UasRegInfoMap.begin(); Itr != g_UasRegInfoMap.end(); Itr++)
    {
        pUasRegInfo = Itr->second;

        if ((NULL == pUasRegInfo) || (0 == pUasRegInfo->iUsed) || (pUasRegInfo->register_id[0] == '\0'))
        {
            continue;
        }

        if ((sstrcmp(pUasRegInfo->register_id, user) == 0)
            && (sstrcmp(pUasRegInfo->from_host, orig_host) == 0)
            && (pUasRegInfo->from_port == orig_port))
        {
            UAS_SMUTEX_UNLOCK();
            return Itr->first;
        }
    }

    UAS_SMUTEX_UNLOCK();
    return -1;
}

/*****************************************************************************
 函 数 名  : uas_reginfo_find_by_caller_host_and_port
 功能描述  : 根据ip和端口查找服务端注册信息
 输入参数  : char* orig_host
             int orig_port
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年3月9日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int uas_reginfo_find_by_caller_host_and_port(char* orig_host, int orig_port)
{
    uas_reg_info_t* pUasRegInfo = NULL;
    UAS_Reg_Info_Iterator Itr;

    if (orig_host == NULL || orig_port <= 0)
    {
        //SIP_DEBUG_TRACE(LOG_DEBUG, "uas_reginfo_find_by_caller_host_and_port() exit---: Param Error \r\n");
        return -1;
    }

    UAS_SMUTEX_LOCK();

    if (g_UasRegInfoMap.size() <= 0)
    {
        UAS_SMUTEX_UNLOCK();
        //SIP_DEBUG_TRACE(LOG_DEBUG, "uas_reginfo_find_by_caller_host_and_port() exit---: UAS Register Info Map NULL \r\n");
        return -1;
    }

    for (Itr = g_UasRegInfoMap.begin(); Itr != g_UasRegInfoMap.end(); Itr++)
    {
        pUasRegInfo = Itr->second;

        if ((NULL == pUasRegInfo) || (0 == pUasRegInfo->iUsed) || (pUasRegInfo->register_id[0] == '\0'))
        {
            continue;
        }

        if ((sstrcmp(pUasRegInfo->from_host, orig_host) == 0)
            && (pUasRegInfo->from_port == orig_port))
        {
            UAS_SMUTEX_UNLOCK();
            return Itr->first;
        }
    }

    UAS_SMUTEX_UNLOCK();
    return -1;
}

/*****************************************************************************
 函 数 名  : uas_reginfo_find_by_register_id
 功能描述  : 根据注册ID查找服务端注册信息
 输入参数  : char* register_id
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年3月9日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int uas_reginfo_find_by_register_id(char* register_id)
{
    uas_reg_info_t* pUasRegInfo = NULL;
    UAS_Reg_Info_Iterator Itr;

    if (register_id == NULL)
    {
        //SIP_DEBUG_TRACE(LOG_DEBUG, "uas_reginfo_find_by_register_id() exit---: Param Error \r\n");
        return -1;
    }

    UAS_SMUTEX_LOCK();

    if (g_UasRegInfoMap.size() <= 0)
    {
        UAS_SMUTEX_UNLOCK();
        //SIP_DEBUG_TRACE(LOG_DEBUG, "uas_reginfo_find() exit---: UAS Register Info Map NULL \r\n");
        return -1;
    }

    for (Itr = g_UasRegInfoMap.begin(); Itr != g_UasRegInfoMap.end(); Itr++)
    {
        pUasRegInfo = Itr->second;

        if ((NULL == pUasRegInfo) || (0 == pUasRegInfo->iUsed) || (pUasRegInfo->register_id[0] == '\0'))
        {
            continue;
        }

        if (sstrcmp(pUasRegInfo->register_id, register_id) == 0)
        {
            UAS_SMUTEX_UNLOCK();
            return Itr->first;
        }
    }

    UAS_SMUTEX_UNLOCK();
    return -1;
}

/*****************************************************************************
 函 数 名  : uas_reginfo_add
 功能描述  : 服务端注册信息添加到队列
 输入参数  : char* user
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年5月23日 星期四
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int uas_reginfo_add(char* user, char* from_host, int from_port)
{
    uas_reg_info_t* pUasRegInfo = NULL;
    UAS_Reg_Info_Iterator Itr;

    if (user == NULL)
    {
        //SIP_DEBUG_TRACE(LOG_DEBUG, "uas_reginfo_add() exit---: Param Error \r\n");
        return -1;
    }

    UAS_SMUTEX_LOCK();

    for (Itr = g_UasRegInfoMap.begin(); Itr != g_UasRegInfoMap.end(); Itr++)
    {
        pUasRegInfo = Itr->second;

        if (0 == pUasRegInfo->iUsed)
        {
            /* 找到空闲的位置 */
            pUasRegInfo->iUsed = 1;

            osip_strncpy(pUasRegInfo->register_id, user, 128);

            if (NULL != from_host)
            {
                osip_strncpy(pUasRegInfo->from_host, from_host, 16);
            }

            pUasRegInfo->from_port = from_port;

            UAS_SMUTEX_UNLOCK();
            //SIP_DEBUG_TRACE(LOG_INFO, "uas_reginfo_add() exit---: pos=%d \r\n", Itr->first);
            return Itr->first;
        }
    }

    UAS_SMUTEX_UNLOCK();
    return -1;
}

/*****************************************************************************
 函 数 名  : uas_reginfo_remove
 功能描述  : 服务端注册信息从队列中移除
 输入参数  : int pos
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年8月22日 星期四
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void uas_reginfo_remove(int pos)
{
    uas_reg_info_t* pUasRegInfo = NULL;

    if (pos < 0 || pos >= (int)g_UasRegInfoMap.size())
    {
        //SIP_DEBUG_TRACE(LOG_DEBUG, "uas_reginfo_remove() exit---: UAS Register Info Index Error \r\n");
        return;
    }

    UAS_SMUTEX_LOCK();

    pUasRegInfo = g_UasRegInfoMap[pos];

    if (0 == pUasRegInfo->iUsed)
    {
        UAS_SMUTEX_UNLOCK();
        //SIP_DEBUG_TRACE(LOG_DEBUG, "uas_reginfo_remove() exit---: UAS Register Info UnUsed:index=%d \r\n", pos);
        return;
    }

    uas_reginfo_free(pUasRegInfo);
    UAS_SMUTEX_UNLOCK();
    return;
}

/*****************************************************************************
 函 数 名  : uas_reginfo_get
 功能描述  : 服务端注册信息获取
 输入参数  : int index
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年5月23日 星期四
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
uas_reg_info_t* uas_reginfo_get(int index)
{
    uas_reg_info_t* pUasRegInfo = NULL;

    if (!is_valid_uas_reg_info_index(index))
    {
        //SIP_DEBUG_TRACE(LOG_DEBUG, "uas_reginfo_get() exit---: UAS Register Info Index Error \r\n");
        return NULL;
    }

    UAS_SMUTEX_LOCK();

    pUasRegInfo = g_UasRegInfoMap[index];

    if (0 == pUasRegInfo->iUsed)
    {
        UAS_SMUTEX_UNLOCK();
        //SIP_DEBUG_TRACE(LOG_DEBUG, "uas_reginfo_get() exit---: Uas Register Info UnUsed:index=%d \r\n", index);
        return NULL;
    }

    UAS_SMUTEX_UNLOCK();
    return pUasRegInfo;
}

/*****************************************************************************
 函 数 名  : uas_reginfo_get2
 功能描述  : 服务端注册信息获取
 输入参数  : int index
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年3月9日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
uas_reg_info_t* uas_reginfo_get2(int index)
{
    uas_reg_info_t* pUasRegInfo = NULL;

    if (!is_valid_uas_reg_info_index(index))
    {
        //SIP_DEBUG_TRACE(LOG_DEBUG, "uas_reginfo_get2() exit---: UAS Register Info Index Error \r\n");
        return NULL;
    }

    pUasRegInfo = g_UasRegInfoMap[index];

    if (0 == pUasRegInfo->iUsed)
    {
        //SIP_DEBUG_TRACE(LOG_DEBUG, "uas_reginfo_get() exit---: Uas Register Info UnUsed:index=%d \r\n", index);
        return NULL;
    }

    return pUasRegInfo;
}

/*****************************************************************************
 函 数 名  : BuildTargetUrl
 功能描述  : 构建目的的Url信息
 输入参数  : char* callee
                            char* remote_ip
                            int remote_port
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年5月23日 星期四
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
osip_uri_t* BuildTargetUrl(char* callee, char* remote_ip, int remote_port)
{
    int iRet = 0;
    char tmp[128] = {0};
    osip_uri_t* url = NULL;

    iRet = osip_uri_init(&url);

    if (iRet != 0)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "BuildTargetUrl() exit---: URL Init Error \r\n");
        return NULL;
    }

    snprintf(tmp, 128, "sip:%s@%s:%i", callee, remote_ip, remote_port);

    iRet = osip_uri_parse(url, tmp);

    if (iRet != 0)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "BuildTargetUrl() exit---: URL Parse Error \r\n");
        return NULL;
    }

    return url;
}

/*****************************************************************************
 函 数 名  : is_valid_uas_reg_info_index
 功能描述  : 是否是合法的服务端注册信息
 输入参数  : int index
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年5月23日 星期四
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int is_valid_uas_reg_info_index(int index)
{
    if (index < 0 || index >= (int)g_UasRegInfoMap.size())
    {
        return 0;
    }

    return 1;
}

/*****************************************************************************
 函 数 名  : build_contact
 功能描述  : 构建contact结构体
 输入参数  : osip_uri_t* contact_url
                            double q_param
                            int expires
                            char* dest
                            int dmax
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int build_contact(osip_uri_t* contact_url, double q_param, int expires, char* dest, int dmax)
{
    int i = -1;
    char* contactStr = NULL;

    if (contact_url == NULL)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "build_contact() exit---: Param Error \r\n");
        return -1;
    }

    i = osip_uri_to_str(contact_url, &contactStr);

    if (0 != i)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "build_contact() exit---: osip_uri_to_str Error \r\n");
        return -1;
    }

    snprintf(dest, dmax, "%s;q=%.2f;expires=%i", contactStr, q_param, expires);

    osip_free(contactStr);
    contactStr = NULL;
    return 0;
}

/*****************************************************************************
 函 数 名  : register_response_add_contacts
 功能描述  : 注册回应消息中添加Contacts
 输入参数  : sip_t *response
                            uas_reg_info_t* pUasRegInfo
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int  register_response_add_contacts(osip_message_t* response, uas_reg_info_t* pUasRegInfo)
{
    char contact[512] = {0};

    if ((response == NULL) || (pUasRegInfo == NULL) || (0 == pUasRegInfo->iUsed))
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "register_response_add_contacts() exit---: Param Error \r\n");
        return -1;
    }

    if ((pUasRegInfo->callid[0] == '\0') || (pUasRegInfo->expires == 0))
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "register_response_add_contacts() exit---: Uas Reg Info Error \r\n");
        return -1;
    }

    if (!build_contact(pUasRegInfo->contact_url, pUasRegInfo->q_param, pUasRegInfo->expires, contact, 511))
    {
        osip_message_set_contact(response, contact);
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : register_msg_proc
 功能描述  : 服务端的注册消息处理
 输入参数  : transaction_t *tr
             sip_t *reg
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int register_msg_proc(osip_transaction_t* tr, osip_message_t* reg)
{
    int pos = -1, i = 0, size = 0, pp = -1;
    int expires = 0;
    int cseq_num = 0;
    double q_param;
    osip_via_t* topvia = NULL;
    osip_uri_t* req_uri = NULL;
    osip_from_t* from = NULL;
    osip_to_t* to = NULL;
    osip_contact_t* contact = NULL;
    osip_header_t*  header = NULL;
    osip_header_t*  expires_header = NULL;
    osip_generic_param_t* gen_param = NULL;
    osip_message_t* response = NULL;
    osip_authorization_t* authorization = NULL;
    char* callid = NULL;
    char register_id[128 + 4] = {0};
    char register_account[128 + 4] = {0};
    char login_ip[16] = {0};
    char* tmp = NULL;
    char* orig_host = NULL;
    int orig_port = 0;
    uas_reg_info_t* pUasRegInfo = NULL;
    time_t now = time(NULL);
    int isNewUser = 0;
    int csexp = DEFAULT_REG_CLEARATE;
    osip_header_t* linktype_header = NULL;
    int link_type = 0;

    if (tr == NULL || reg == NULL)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "register_msg_proc() exit---: Param Error \r\n");
        return -1;
    }

    from = osip_message_get_from(reg);
    to = osip_message_get_to(reg);
    req_uri = osip_message_get_uri(reg);

    if (from == NULL || to == NULL || req_uri == NULL)
    {
        cs_generating_answer(tr, NULL, 400, (char*)"Register Message Error");
        SIP_DEBUG_TRACE(LOG_ERROR, "register_msg_proc() exit---: Register Message Error \r\n");
        return -1;
    }

    if (NULL == from->url || NULL == from->url->username)
    {
        cs_generating_answer(tr, NULL, 400, (char*)"Register Message From URL Error");
        SIP_DEBUG_TRACE(LOG_ERROR, "register_msg_proc() exit---: Register Message From URL Error \r\n");
        return -1;
    }

    /* step1: support necessary extensions (MUST process Require header as 8.2.2) */
    osip_message_get_require(reg, 0, &header);

    if (header != NULL) /*now we not support any extensions */
    {
        i = cs_generating_response_default(tr->orig_request, 420, NULL, &response);

        if (i != 0)
        {
            SIP_DEBUG_TRACE(LOG_ERROR, "register_msg_proc() exit---: CS Generating Default Response Error \r\n");
            return -1;
        }

        osip_message_set_unsupported(response, header->hvalue);
        i = ul_sendmessage(tr, response);

        if (i != 0)
        {
            SIP_DEBUG_TRACE(LOG_ERROR, "register_msg_proc() ul_sendmessage Error \r\n");
        }

        SIP_DEBUG_TRACE(LOG_ERROR, "register_msg_proc() exit---: Not Support Require Head \r\n");
        return 0;
    }

    /* setp2: check whether the request contains the Contact header.
              if not goto setp8  */
    size = osip_list_size(&reg->contacts);

    if (size > 1)
    {
        cs_generating_answer(tr, NULL, 400, (char*)"Register Contact Size Error");
        SIP_DEBUG_TRACE(LOG_ERROR, "register_msg_proc() exit---: Register Contact Size Error \r\n");
        return -1;
    }

    i = osip_message_get_contact(reg, 0, &contact);

    if (NULL == contact || NULL == contact->url)
    {
        cs_generating_answer(tr, NULL, 400, (char*)"Register Contact Error");
        SIP_DEBUG_TRACE(LOG_ERROR, "register_msg_proc() exit---: Register Contact Error \r\n");
        return -1;
    }

    /* setp3: get authorization*/
    pos = 0;
    authorization = NULL;

    while (!osip_list_eol(&reg->authorizations, pos))
    {
        osip_message_get_authorization(reg, pos, &authorization);

        if (authorization != NULL && authorization->realm != NULL)
        {
            break;
        }

        pos++;
        authorization = NULL;
    }

    /* setp4: process each contact address in te Contact header field in turn */
    /* 获取经过路由的IP地址 */
    osip_message_get_via(reg, 0, &topvia);
    osip_via_param_get_byname(topvia, (char*)"maddr", &gen_param);

    if (gen_param != NULL)
    {
        orig_host = gen_param->gvalue;
    }
    else
    {
        orig_host = NULL;
    }

    if (NULL == orig_host)
    {
        osip_via_param_get_byname(topvia, (char*)"received", &gen_param);

        if (gen_param != NULL)
        {
            orig_host = gen_param->gvalue;
        }
        else
        {
            orig_host = NULL;
        }

        /* 如果经过路由的IP地址为空，获取原始的IP地址 */
        if (NULL == orig_host)
        {
            if ((contact != NULL) && (contact->url != NULL))
            {
                orig_host = contact->url->host;
            }
            else
            {
                SIP_DEBUG_TRACE(LOG_WARN, "register_msg_proc() Get contact error ");
            }

            if (orig_host == NULL)
            {
                cs_generating_answer(tr, NULL, 400, (char*)"Register Message Contact Host Error");
                SIP_DEBUG_TRACE(LOG_ERROR, "register_msg_proc() exit---: Contact Host Error \r\n");
                return -1;
            }
        }
    }

    /* 获取经过路由的端口号 */
    tmp = NULL;
    osip_via_param_get_byname(topvia, (char*)"rport", &gen_param);

    if (gen_param != NULL)
    {
        tmp = gen_param->gvalue;
    }

    if (tmp != NULL)
    {
        orig_port = osip_atoi(tmp);
    }

    /* 如果经过路由的端口号为空，获取原始的端口号 */
    if (tmp == NULL)
    {
        if ((contact != NULL) && (contact->url != NULL))
        {
            tmp = contact->url->port;
        }

        if (tmp == NULL)
        {
            orig_port = 5060;
        }
        else
        {
            orig_port = osip_atoi(tmp);
        }
    }

    SIP_DEBUG_TRACE(LOG_TRACE, "register_msg_proc() register_id=%s, from_host=%s, from_port=%d \r\n", reg->from->url->username, orig_host, orig_port);

    /* step5: find the user*/
    pp = uas_reginfo_find(reg->from->url->username, orig_host, orig_port);//注册信息是否存在
    //SIP_DEBUG_TRACE(LOG_INFO, "register_msg_proc() uas_reginfo_find:pp=%d \r\n", pp);

    if (pp < 0)
    {
        pp = uas_reginfo_add(reg->from->url->username, orig_host, orig_port);
        //SIP_DEBUG_TRACE(LOG_INFO, "register_msg_proc() uas_reginfo_add:pp=%d \r\n", pp);

        if (pp < 0)
        {
            i = cs_generating_answer(tr, NULL, 503, (char*)"UAS Register Info Add Error");
            SIP_DEBUG_TRACE(LOG_ERROR, "register_msg_proc() exit---: UAS Register Info Add Error \r\n");
            return -1;
        }

        isNewUser = 1;
    }

    pUasRegInfo = uas_reginfo_get(pp);

    if (NULL == pUasRegInfo)
    {
        i = cs_generating_answer(tr, NULL, 503, (char*)"UAS Register Info Get Error");
        SIP_DEBUG_TRACE(LOG_ERROR, "register_msg_proc() exit---: UAS Register Info Get Error \r\n");
        return -1;
    }

    if ('\0' == pUasRegInfo->register_account[0])
    {
        if ((contact != NULL) && (NULL != contact->url) && (contact->url->username != NULL))
        {
            osip_strncpy(pUasRegInfo->register_account, contact->url->username, 128);
        }
        else if (pUasRegInfo->register_id[0] != '\0')
        {
            osip_strncpy(pUasRegInfo->register_account, pUasRegInfo->register_id, 128);
        }
    }

    /* 获取cseq */
    tmp = osip_message_get_cseq(reg)->number;

    if (tmp == NULL)
    {
        cs_generating_answer(tr, NULL, 400, (char*)"Register Message Cseq Error");
        SIP_DEBUG_TRACE(LOG_ERROR, "register_msg_proc() exit---: Register Message Get Cseq Error \r\n");
        return -1;
    }

    cseq_num = osip_atoi(tmp);

    /* 获取callid */
    osip_call_id_to_str(osip_message_get_call_id(reg), &callid);

    if (callid == NULL)
    {
        cs_generating_answer(tr, NULL, 400, (char*)"Register Message CallID Error");
        SIP_DEBUG_TRACE(LOG_ERROR, "register_msg_proc() exit---: Callid Error \r\n");
        return -1;
    }

    /* 获取注册超时时间头域 */
    osip_message_get_expires(reg, 0, &expires_header);

    if (expires_header == NULL || expires_header->hvalue == NULL)
    {
        cs_generating_answer(tr, NULL, 400, (char*)"Register Expires Error");
        SIP_DEBUG_TRACE(LOG_ERROR, "register_msg_proc() exit---: Register Expires Error \r\n");
        return 0;
    }

    osip_contact_param_get_byname(contact, (char*)"expires", &gen_param);

    if (gen_param == NULL)
    {
        osip_contact_param_get_byname(contact, (char*)"Expires", &gen_param);

        if (gen_param == NULL)
        {
            osip_contact_param_get_byname(contact, (char*)"EXPIRES", &gen_param);
        }
    }

    if (gen_param != NULL && gen_param->gvalue != NULL)
    {
        expires = osip_atoi(gen_param->gvalue);
    }
    else if (expires_header != NULL && expires_header->hvalue != NULL)
    {
        expires = osip_atoi(expires_header->hvalue);
    }
    else
    {
        expires = csexp;
    }

    /*if (expires != 0)
    {
        expires = (expires >= csexp) ? expires : csexp;
    }*/

    /* 获取contact 参数 */
    if (NULL == pUasRegInfo->contact_url && NULL != contact->url)
    {
        osip_uri_clone(contact->url, &(pUasRegInfo->contact_url));
    }

    if ('\0' == pUasRegInfo->callid[0] && NULL != callid)
    {
        osip_strncpy(pUasRegInfo->callid, callid, 128);
    }
    else if ('\0' != pUasRegInfo->callid[0] && NULL != callid)
    {
        if (0 != sstrcmp(pUasRegInfo->callid, callid))
        {
            memset(pUasRegInfo->callid, 0, 132);
            osip_strncpy(pUasRegInfo->callid, callid, 128);
        }
    }

    if ('\0' == pUasRegInfo->serverid[0] && NULL != reg->req_uri->username)
    {
        osip_strncpy(pUasRegInfo->serverid, reg->req_uri->username, 128);
    }
    else if ('\0' != pUasRegInfo->serverid[0] && NULL != reg->req_uri->username)
    {
        if (0 != sstrcmp(pUasRegInfo->serverid, reg->req_uri->username))
        {
            memset(pUasRegInfo->serverid, 0, 132);
            osip_strncpy(pUasRegInfo->serverid, reg->req_uri->username, 128);
        }
    }

    if ('\0' == pUasRegInfo->serverip[0] && NULL != reg->req_uri->host)
    {
        osip_strncpy(pUasRegInfo->serverip, reg->req_uri->host, 16);
    }
    else if ('\0' != pUasRegInfo->serverip[0] && NULL != reg->req_uri->host)
    {
        if (0 != sstrcmp(pUasRegInfo->serverip, reg->req_uri->host))
        {
            memset(pUasRegInfo->serverip, 0, 16);
            osip_strncpy(pUasRegInfo->serverip, reg->req_uri->host, 16);
        }
    }

    if (NULL != reg->req_uri->port)
    {
        pUasRegInfo->serverport = osip_atoi(reg->req_uri->port);
    }
    else
    {
        pUasRegInfo->serverport = 5060;
    }

    if (NULL == pUasRegInfo->authorization && authorization != NULL)
    {
        osip_authorization_clone(authorization, &pUasRegInfo->authorization);
    }
    else if (NULL != pUasRegInfo->authorization && authorization != NULL)
    {
        osip_authorization_free(pUasRegInfo->authorization);
        pUasRegInfo->authorization = NULL;

        osip_authorization_clone(authorization, &pUasRegInfo->authorization);
    }
    else if (NULL != pUasRegInfo->authorization && authorization == NULL)
    {
        osip_authorization_free(pUasRegInfo->authorization);
        pUasRegInfo->authorization = NULL;
    }

    osip_contact_param_get_byname(contact, (char*)"q", &gen_param);

    if (gen_param != NULL && gen_param->gvalue != NULL)
    {
        q_param = satod(gen_param->gvalue);
    }
    else
    {
        q_param = 0;
    }

    pUasRegInfo->cseqnum = cseq_num;
    pUasRegInfo->expires = expires;
    pUasRegInfo->expires_count = expires;
    pUasRegInfo->q_param = q_param;

    pUasRegInfo->tr = tr;
    pUasRegInfo->sip = reg;
    pUasRegInfo->last_active_time = now;

    if (1 == isNewUser && pUasRegInfo->expires > 0)
    {
        cs_timer_use(IN_REG_EXPIRE, pp, NULL);
        isNewUser = 0;
    }

    osip_message_header_get_byname(reg, (const char*)"Link-Type", 0, &linktype_header);

    if (NULL != linktype_header && NULL != linktype_header->hvalue)
    {
        if (0 == sstrcmp(linktype_header->hvalue, (char*)"Peering"))
        {
            link_type = 1;
        }
    }

    /* 调用钩子函数 */
    if (NULL != g_AppCallback && NULL != g_AppCallback->uas_register_received_cb)
    {
        osip_strncpy(register_id, pUasRegInfo->register_id, 128);
        osip_strncpy(register_account, pUasRegInfo->register_account, 128);
        osip_strncpy(login_ip, pUasRegInfo->from_host, 16);

        g_AppCallback->uas_register_received_cb(pUasRegInfo->serverid, register_id, login_ip, orig_port, register_account, pp, pUasRegInfo->expires, link_type);
    }

    if (NULL != callid)
    {
        osip_free(callid);
        callid = NULL;
    }

    return 0;
}
#endif

#if DECS("客户端注册管理")
/*****************************************************************************
 函 数 名  : uac_reginfo_init
 功能描述  : 客户端注册信息结构初始化
 输入参数  : uac_reg_info_t** uac_reg_info
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年5月23日 星期四
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int uac_reginfo_init(uac_reg_info_t** uac_reg_info)
{
    *uac_reg_info = (uac_reg_info_t*)osip_malloc(sizeof(uac_reg_info_t));

    if (*uac_reg_info == NULL)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "uac_reginfo_init() exit---: *uac_reg_info Smalloc Error \r\n");
        return -1;
    }

    (*uac_reg_info)->iUsed = 0;
    (*uac_reg_info)->register_id[0] = '\0';
    (*uac_reg_info)->proxy_id[0] = '\0';

    (*uac_reg_info)->proxyip[0] = '\0';
    (*uac_reg_info)->proxyport = 0;

    (*uac_reg_info)->register_account[0] = '\0';
    (*uac_reg_info)->register_password[0] = '\0';

    (*uac_reg_info)->register_callid_number[0] = '\0';
    (*uac_reg_info)->register_cseq_number = 0;

    (*uac_reg_info)->link_type = 0;

    (*uac_reg_info)->isReg = 0;
    (*uac_reg_info)->isReging = 0;
    (*uac_reg_info)->expires = 0;
    (*uac_reg_info)->min_expires = 0;

    (*uac_reg_info)->localip[0] = '\0';
    (*uac_reg_info)->localport = 0;

#ifdef MEMORY_LEAKS1
    static int comptr = 0;
    comptr++;
    freesipuaccptr++;

    fprintf(stdout, (char*)"\r\n<register.c> uac_reginfo_init() malloc: (address = 0x%lx) comptr: %d, existing element %d\r\n", (long unsigned int)*uac_reg_info, comptr, freesipuaccptr);
    fflush(stdout);
#endif

    return 0;
}

/*****************************************************************************
 函 数 名  : uac_reginfo_free
 功能描述  : 客户端注册信息结构释放
 输入参数  : uac_reg_info_t* uac_reg_info
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年5月23日 星期四
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void uac_reginfo_free(uac_reg_info_t* uac_reg_info)
{
    if (uac_reg_info == NULL)
    {
        //SIP_DEBUG_TRACE(LOG_DEBUG, "uac_reginfo_free() exit---: Param Error \r\n");
        return;
    }

    uac_reg_info->iUsed = 0;

    memset(uac_reg_info->register_id, 0, 132);

    memset(uac_reg_info->proxy_id, 0, 132);
    memset(uac_reg_info->proxyip, 0, 16);
    uac_reg_info->proxyport = 0;

    memset(uac_reg_info->register_account, 0, 132);
    memset(uac_reg_info->register_password, 0, 132);

    memset(uac_reg_info->register_callid_number, 0, 132);

    uac_reg_info->register_cseq_number = 0;
    uac_reg_info->link_type = 0;

    uac_reg_info->isReg = 0;
    uac_reg_info->isReging = 0;
    uac_reg_info->expires = 0;
    uac_reg_info->min_expires = 0;

    memset(uac_reg_info->localip, 0, 16);

    uac_reg_info->localport = 0;

#ifdef MEMORY_LEAKS1
    static int comptr = 0;
    comptr++;
    freesipuaccptr--;

    fprintf(stdout, (char*)"\r\n<register.c> uac_reginfo_free() free: (address = 0x%lx) comptr: %d, existing element %d\r\n", (long unsigned int)uac_reg_info, comptr, freesipuaccptr);
    fflush(stdout);
#endif

    return;
}

/*****************************************************************************
 函 数 名  : uac_reginfo_list_init
 功能描述  : 客户端注册信息队列初始化
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
int uac_reginfo_list_init()
{
    int i = 0;
    int pos = 0;

    g_UacRegInfoMap.clear();

    for (pos = 0; pos < MAX_REG_UAC; pos++)
    {
        uac_reg_info_t* pUacRegInfo = NULL;

        i = uac_reginfo_init(&pUacRegInfo);

        if (i != 0)
        {
            SIP_DEBUG_TRACE(LOG_DEBUG, "uac_reginfo_list_init() exit---: UAC Register Info Init Error \r\n");
            return -1;
        }

        g_UacRegInfoMap[pos] = pUacRegInfo;
    }

#ifdef MULTI_THR
    g_UacRegInfoMapLock = (osip_mutex_t*)osip_mutex_init();

    if (NULL == g_UacRegInfoMapLock)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "uac_reginfo_list_init() exit---: UAC Register Info Map Lock Init Error \r\n");
        return -1;
    }

#endif
    return 0;
}

/*****************************************************************************
 函 数 名  : uac_reginfo_list_free
 功能描述  : 客户端注册信息队列释放
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
void uac_reginfo_list_free()
{
    UAC_Reg_Info_Iterator Itr;
    uac_reg_info_t* uac_reg_info = NULL;

    for (Itr = g_UacRegInfoMap.begin(); Itr != g_UacRegInfoMap.end(); Itr++)
    {
        uac_reg_info = Itr->second;

        if (NULL != uac_reg_info)
        {
            uac_reginfo_free(uac_reg_info);
            osip_free(uac_reg_info);
            uac_reg_info = NULL;
        }
    }

    g_UacRegInfoMap.clear();

#ifdef MULTI_THR

    if (NULL != g_UacRegInfoMapLock)
    {
        osip_mutex_destroy((struct osip_mutex*)g_UacRegInfoMapLock);
        g_UacRegInfoMapLock = NULL;
    }

#endif
    return;
}

/*****************************************************************************
 函 数 名  : uac_reginfo_list_lock
 功能描述  : 客户端注册信息队列锁定
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
int uac_reginfo_list_lock()
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_UacRegInfoMapLock == NULL)
    {
        //SIP_DEBUG_TRACE(LOG_DEBUG, "uac_reginfo_list_lock() exit---: Param Error \r\n");
        return -1;
    }

    iRet = osip_mutex_lock((struct osip_mutex*)g_UacRegInfoMapLock);

#endif
    return iRet;
}

/*****************************************************************************
 函 数 名  : uac_reginfo_list_unlock
 功能描述  : 客户端注册信息队列解锁
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
int uac_reginfo_list_unlock()
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_UacRegInfoMapLock == NULL)
    {
        //SIP_DEBUG_TRACE(LOG_DEBUG, "uac_reginfo_list_unlock() exit---: Param Error \r\n");
        return -1;
    }

    iRet = osip_mutex_unlock((struct osip_mutex*)g_UacRegInfoMapLock);

#endif
    return iRet;
}

/*****************************************************************************
 函 数 名  : debug_uac_reginfo_list_lock
 功能描述  : 客户端注册信息队列锁定
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
int debug_uac_reginfo_list_lock(const char* file, int line, const char* func)
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_UacRegInfoMapLock == NULL)
    {
        //SIP_DEBUG_TRACE(LOG_DEBUG, "debug_uac_reginfo_list_lock() exit---: Param Error \r\n");
        return -1;
    }

#ifndef WIN32 //modified by chenyu 131024
    iRet = osip_debug_mutex_lock((struct osip_mutex*)g_UacRegInfoMapLock, file, line, func);
#else
    iRet = osip_mutex_lock((struct osip_mutex*)g_UacRegInfoMapLock);
#endif

#endif
    return iRet;
}

/*****************************************************************************
 函 数 名  : debug_uac_reginfo_list_unlock
 功能描述  : 客户端注册信息队列解锁
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
int debug_uac_reginfo_list_unlock(const char* file, int line, const char* func)
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_UacRegInfoMapLock == NULL)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "debug_uac_reginfo_list_unlock() exit---: Param Error \r\n");
        return -1;
    }

#ifndef WIN32   //modified by chenyu 131024
    iRet = osip_debug_mutex_unlock((struct osip_mutex*)g_UacRegInfoMapLock, file, line, func);
#else
    iRet = osip_mutex_unlock((struct osip_mutex*)g_UacRegInfoMapLock);
#endif

#endif
    return iRet;
}

/*****************************************************************************
 函 数 名  : uac_reginfo_find
 功能描述  : 客户端注册信息查找
 输入参数  : char* call_id
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年5月23日 星期四
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int uac_reginfo_find(char* call_id)
{
    uac_reg_info_t* pUacRegInfo = NULL;
    UAC_Reg_Info_Iterator Itr;

    if (call_id == NULL)
    {
        //SIP_DEBUG_TRACE(LOG_DEBUG, "uac_reginfo_find() exit---: Param Error \r\n");
        return -1;
    }

    if (g_UacRegInfoMap.size() <= 0)
    {
        //SIP_DEBUG_TRACE(LOG_DEBUG, "uac_reginfo_find() exit---: UAC Register Info Map NULL \r\n");
        return -1;
    }

    UAC_SMUTEX_LOCK();

    for (Itr = g_UacRegInfoMap.begin(); Itr != g_UacRegInfoMap.end(); Itr++)
    {
        pUacRegInfo = Itr->second;

        if ((NULL == pUacRegInfo) || (0 == pUacRegInfo->iUsed) || (pUacRegInfo->register_id[0] == '\0'))
        {
            continue;
        }

        if (sstrcmp(pUacRegInfo->register_callid_number, call_id) == 0)
        {
            UAC_SMUTEX_UNLOCK();
            return Itr->first;
        }
    }

    UAC_SMUTEX_UNLOCK();
    return -1;
}


/*****************************************************************************
 函 数 名  : uac_reginfo_find_by_server_and_local_info
 功能描述  : 根据注册的服务器和本地信息查找注册信息
 输入参数  : char* service_id
             char* server_ip
             int server_port
             char* local_id
             char* local_ip
             int local_port
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年5月23日 星期四
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int uac_reginfo_find_by_server_and_local_info(char* service_id, char* server_ip, int server_port, char* local_id, char* local_ip, int local_port)
{
    uac_reg_info_t* pUacRegInfo = NULL;
    UAC_Reg_Info_Iterator Itr;

    if ((NULL == service_id) || (service_id[0] == '\0')
        || (NULL == server_ip) || (server_ip[0] == '\0') || (server_port <= 0)
        || (NULL == local_id) || (local_id[0] == '\0')
        || (NULL == local_ip) || (local_ip[0] == '\0') || (local_port <= 0))
    {
        //SIP_DEBUG_TRACE(LOG_DEBUG, "uac_reginfo_find_by_server_and_local_info() exit---: Param Error \r\n");
        return -1;
    }

    if (g_UacRegInfoMap.size() <= 0)
    {
        //SIP_DEBUG_TRACE(LOG_DEBUG, "uac_reginfo_find_by_server_and_local_info() exit---: UAC Register Info Map NULL \r\n");
        return -1;
    }

    UAC_SMUTEX_LOCK();

    for (Itr = g_UacRegInfoMap.begin(); Itr != g_UacRegInfoMap.end(); Itr++)
    {
        pUacRegInfo = Itr->second;

        if ((NULL == pUacRegInfo) || (0 == pUacRegInfo->iUsed) || (pUacRegInfo->register_id[0] == '\0'))
        {
            continue;
        }

        if ((sstrcmp(pUacRegInfo->proxy_id, service_id) == 0)
            && (sstrcmp(pUacRegInfo->proxyip, server_ip) == 0)
            && (pUacRegInfo->proxyport == server_port)
            && (sstrcmp(pUacRegInfo->register_id, local_id) == 0)
            && (sstrcmp(pUacRegInfo->localip, local_ip) == 0)
            && (pUacRegInfo->localport == local_port))
        {
            UAC_SMUTEX_UNLOCK();
            return Itr->first;
        }
    }

    UAC_SMUTEX_UNLOCK();
    return -1;
}

/*****************************************************************************
 函 数 名  : uac_reginfo_find_by_serverinfo
 功能描述  : 根据注册的服务器信息查找客户端注册信息
 输入参数  : char* register_id
                            char* ip
                            int port
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年5月23日 星期四
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int uac_reginfo_find_by_serverinfo(char* server_id, char* server_ip, int server_port)
{
    uac_reg_info_t* pUacRegInfo = NULL;
    UAC_Reg_Info_Iterator Itr;

    if (server_id == NULL || server_ip == NULL || server_port <= 0)
    {
        //SIP_DEBUG_TRACE(LOG_DEBUG, "uac_reginfo_find_by_serverinfo() exit---: Param Error \r\n");
        return -1;
    }

    if (g_UacRegInfoMap.size() <= 0)
    {
        //SIP_DEBUG_TRACE(LOG_DEBUG, "uac_reginfo_find_by_serverinfo() exit---: UAC Register Info Map NULL \r\n");
        return -1;
    }

    UAC_SMUTEX_LOCK();

    for (Itr = g_UacRegInfoMap.begin(); Itr != g_UacRegInfoMap.end(); Itr++)
    {
        pUacRegInfo = Itr->second;

        if ((NULL == pUacRegInfo) || (0 == pUacRegInfo->iUsed) || (pUacRegInfo->proxy_id[0] == '\0'))
        {
            continue;
        }

        if ((sstrcmp(pUacRegInfo->proxy_id, server_id) == 0)
            && (sstrcmp(pUacRegInfo->proxyip, server_ip) == 0)
            && (pUacRegInfo->proxyport == server_port))
        {
            UAC_SMUTEX_UNLOCK();
            return Itr->first;
        }
    }

    UAC_SMUTEX_UNLOCK();
    return -1;
}

/*****************************************************************************
 函 数 名  : uac_reginfo_find_by_server_host_and_port
 功能描述  : 根据IP和端口查找UAC信息
 输入参数  : char* server_ip
             int server_port
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年2月10日 星期二
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int uac_reginfo_find_by_server_host_and_port(char* server_ip, int server_port)
{
    uac_reg_info_t* pUacRegInfo = NULL;
    UAC_Reg_Info_Iterator Itr;

    if (server_ip == NULL || server_port <= 0)
    {
        //SIP_DEBUG_TRACE(LOG_DEBUG, "uac_reginfo_find_by_server_host_and_port() exit---: Param Error \r\n");
        return -1;
    }

    if (g_UacRegInfoMap.size() <= 0)
    {
        //SIP_DEBUG_TRACE(LOG_DEBUG, "uac_reginfo_find_by_server_host_and_port() exit---: UAC Register Info Map NULL \r\n");
        return -1;
    }

    UAC_SMUTEX_LOCK();

    for (Itr = g_UacRegInfoMap.begin(); Itr != g_UacRegInfoMap.end(); Itr++)
    {
        pUacRegInfo = Itr->second;

        if ((NULL == pUacRegInfo) || (0 == pUacRegInfo->iUsed) || (pUacRegInfo->proxy_id[0] == '\0'))
        {
            continue;
        }

        if ((sstrcmp(pUacRegInfo->proxyip, server_ip) == 0)
            && (pUacRegInfo->proxyport == server_port))
        {
            UAC_SMUTEX_UNLOCK();
            return Itr->first;
        }
    }

    UAC_SMUTEX_UNLOCK();
    return -1;
}

/*****************************************************************************
 函 数 名  : uac_reginfo_add
 功能描述  : 客户端注册信息添加
 输入参数  : char* user
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年5月23日 星期四
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int uac_reginfo_add(char* user)
{
    uac_reg_info_t* pUacRegInfo = NULL;
    UAC_Reg_Info_Iterator Itr;

    if (user == NULL)
    {
        //SIP_DEBUG_TRACE(LOG_DEBUG, "uac_reginfo_add() exit---: Param Error \r\n");
        return -1;
    }

    UAC_SMUTEX_LOCK();

    for (Itr = g_UacRegInfoMap.begin(); Itr != g_UacRegInfoMap.end(); Itr++)
    {
        pUacRegInfo = Itr->second;

        if (0 == pUacRegInfo->iUsed)
        {
            /* 找到空闲的位置 */
            pUacRegInfo->iUsed = 1;

            osip_strncpy(pUacRegInfo->register_id, user, 128);
            //SIP_DEBUG_TRACE(LOG_INFO, "uac_reginfo_add() exit---: pos=%d \r\n", Itr->first);
            UAC_SMUTEX_UNLOCK();
            return Itr->first;
        }
    }

    UAC_SMUTEX_UNLOCK();
    return -1;
}

/*****************************************************************************
 函 数 名  : uac_reginfo_remove
 功能描述  : 客户端注册信息移除
 输入参数  : int pos
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年5月23日 星期四
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void uac_reginfo_remove(int pos)
{
    uac_reg_info_t* pUacRegInfo = NULL;

    UAC_SMUTEX_LOCK();

    if (!is_valid_uac_reg_info_index(pos))
    {
        //SIP_DEBUG_TRACE(LOG_DEBUG, "uac_reginfo_remove() exit---: UAC Register Info Index Error \r\n");
        UAC_SMUTEX_UNLOCK();
        return;
    }

    pUacRegInfo = g_UacRegInfoMap[pos];

    if (0 == pUacRegInfo->iUsed)
    {
        //SIP_DEBUG_TRACE(LOG_ERROR, "uac_reginfo_remove() exit---: UAC Register Info UnUsed:index=%d \r\n", pos);
        UAC_SMUTEX_UNLOCK();
        return;
    }

    uac_reginfo_free(pUacRegInfo);
    UAC_SMUTEX_UNLOCK();
    return;
}

/*****************************************************************************
 函 数 名  : uac_reginfo_get
 功能描述  : 客户端注册信息获取
 输入参数  : int index
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年5月23日 星期四
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
uac_reg_info_t* uac_reginfo_get(int index)
{
    uac_reg_info_t* pUacRegInfo = NULL;

    if (!is_valid_uac_reg_info_index(index))
    {
        //SIP_DEBUG_TRACE(LOG_DEBUG, "uac_reginfo_get() exit---: UAC Register Info Index Error \r\n");
        return NULL;
    }

    UAC_SMUTEX_LOCK();

    pUacRegInfo = g_UacRegInfoMap[index];

    if (0 == pUacRegInfo->iUsed)
    {
        UAC_SMUTEX_UNLOCK();
        //SIP_DEBUG_TRACE(LOG_ERROR, "uac_reginfo_get() exit---: UAC Register Info UnUsed:index=%d \r\n", index);
        return NULL;
    }

    UAC_SMUTEX_UNLOCK();
    return pUacRegInfo;
}

uac_reg_info_t* uac_reginfo_get2(int index)
{
    uac_reg_info_t* pUacRegInfo = NULL;

    if (!is_valid_uac_reg_info_index(index))
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "uac_reginfo_get2() exit---: UAC Register Info Index Error \r\n");
        return NULL;
    }

    pUacRegInfo = g_UacRegInfoMap[index];

    if (0 == pUacRegInfo->iUsed)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "uac_reginfo_get2() exit---: UAC Register Info UnUsed:index=%d \r\n", index);
        return NULL;
    }

    return pUacRegInfo;
}

/*****************************************************************************
 函 数 名  : is_valid_uac_reg_info_index
 功能描述  : 是否是合法的客户端注册信息
 输入参数  : int index
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年5月23日 星期四
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int is_valid_uac_reg_info_index(int index)
{
    if (index < 0 || index >= (int)g_UacRegInfoMap.size())
    {
        return 0;
    }

    return 1;
}
#endif

#if DECS("对外接口")

/*****************************************************************************
 函 数 名  : SIP_UASUpdateRegisterExpires
 功能描述  : 服务端更新注册超时时间
 输入参数  : int reg_info_index
                            int reg_routes_index
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年5月23日 星期四
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int SIP_UASUpdateRegisterExpires(int reg_info_index)
{
    uas_reg_info_t* pUasRegInfo = NULL;

#if 0

    SIP_DEBUG_TRACE(LOG_INFO,  "SIP_UASUpdateRegisterExpires() \
    \r\n In Para: \
    \r\n reg_info_index=%d \
    \r\n reg_routes_index=%d \
    \r\n ", reg_info_index, reg_routes_index);

#endif

    if (!is_valid_uas_reg_info_index(reg_info_index))
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "SIP_UASUpdateRegisterExpires() exit---: UAS Register Info Index Error \r\n");
        return -1;
    }

    pUasRegInfo = uas_reginfo_get(reg_info_index);

    if (NULL == pUasRegInfo)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "SIP_UASUpdateRegisterExpires() exit---: Get UAS Register Info Error \r\n");
        return -1;
    }

    pUasRegInfo->expires_count = pUasRegInfo->expires;

    return 0;
}

/*****************************************************************************
 函 数 名  : SIP_UASRemoveRegisterInfo
 功能描述  : 服务端移除注册信息
 输入参数  : int reg_info_index
                            int reg_routes_index
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年6月25日 星期四
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int SIP_UASRemoveRegisterInfo(int reg_info_index)
{
    uas_reg_info_t* pUasRegInfo = NULL;

#if 0

    SIP_DEBUG_TRACE(LOG_INFO,  "SIP_UASRemoveRegisterInfo() \
    \r\n In Para: \
    \r\n reg_info_index=%d \
    \r\n reg_routes_index=%d \
    \r\n ", reg_info_index, reg_routes_index);

#endif

    if (!is_valid_uas_reg_info_index(reg_info_index))
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "SIP_UASRemoveRegisterInfo() exit---: UAS Register Info Index Error \r\n");
        return -1;
    }

    pUasRegInfo = uas_reginfo_get(reg_info_index);

    if (NULL == pUasRegInfo)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "SIP_UASRemoveRegisterInfo() exit---: Get UAS Register Info Error \r\n");
        return -1;
    }

    cs_timer_remove(IN_REG_EXPIRE, reg_info_index, NULL);
    uas_reginfo_remove(reg_info_index);

    return 0;
}

/*****************************************************************************
 函 数 名  : SIP_UASGetRegisterAuthorization
 功能描述  : 服务端端获取注册认证信息
 输入参数  : int reg_info_index
                            int reg_routes_index
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年5月23日 星期四
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
osip_authorization_t* SIP_UASGetRegisterAuthorization(int reg_info_index)
{
    uas_reg_info_t* pUasRegInfo = NULL;

#if 0

    SIP_DEBUG_TRACE(LOG_INFO,  "SIP_UASGetRegisterAuthorization() \
    \r\n In Para: \
    \r\n reg_info_index=%d \
    \r\n reg_routes_index=%d \
    \r\n ", reg_info_index, reg_routes_index);

#endif

    if (!is_valid_uas_reg_info_index(reg_info_index))
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "SIP_UASGetRegisterAuthorization() exit---: UAS Register Info Index Error \r\n");
        return NULL;
    }

    pUasRegInfo = uas_reginfo_get(reg_info_index);

    if (NULL == pUasRegInfo)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "SIP_UASGetRegisterAuthorization() exit---: Get UAS Register Info Error \r\n");
        return NULL;
    }

    return pUasRegInfo->authorization;
}

/*****************************************************************************
 函 数 名  : SIP_UASAnswerToRegister
 功能描述  : 服务端发送响应给注册消息
 输入参数  : int reg_info_index
                            int reg_routes_index
                            int code
                            char* reasonphrase
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年5月23日 星期四
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int SIP_UASAnswerToRegister(int reg_info_index, int code, char* reasonphrase)
{
    int i = 0;
    uas_reg_info_t* pUasRegInfo = NULL;
    osip_message_t* response = NULL;
    char strExpires[32] = {0};

#if 1
    SIP_DEBUG_TRACE(LOG_INFO,  "SIP_UASAnswerToRegister() \
    \r\n In Para: \
    \r\n reg_info_index=%d \
    \r\n code=%d \
    \r\n ", reg_info_index, code);
#endif

    if (!is_valid_uas_reg_info_index(reg_info_index))
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "SIP_UASAnswerToRegister() exit---: UAS Register Info Index Error \r\n");
        return -1;
    }

    pUasRegInfo = uas_reginfo_get(reg_info_index);

    if (NULL == pUasRegInfo)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "SIP_UASAnswerToRegister() exit---: Get UAS Register Info Error \r\n");
        return -1;
    }

    if (401 == code)
    {
        if (NULL != pUasRegInfo->tr && NULL != pUasRegInfo->tr->orig_request)
        {
            i = cs_generating_response_default(pUasRegInfo->tr->orig_request, 401, NULL, &response);

            cs_response_add_www_authenticate(response, pUasRegInfo->serverip);
            i = ul_sendmessage(pUasRegInfo->tr, response);

            if (i != 0)
            {
                SIP_DEBUG_TRACE(LOG_ERROR, "register_msg_proc() ul_sendmessage Error \r\n");
            }
        }
    }
    else if (200 == code)
    {
        if (NULL != pUasRegInfo->tr && NULL != pUasRegInfo->tr->orig_request)
        {
            i = cs_generating_response_default(pUasRegInfo->tr->orig_request, 200, NULL, &response);

            if (pUasRegInfo->expires > 0)
            {
                register_response_add_contacts(response, pUasRegInfo);
                snprintf(strExpires, 32, "%d", pUasRegInfo->expires);
                osip_message_set_expires(response, strExpires);
                msg_set_data_header(response);
            }

            i = ul_sendmessage(pUasRegInfo->tr, response);

            if (i != 0)
            {
                SIP_DEBUG_TRACE(LOG_ERROR, "register_msg_proc() ul_sendmessage Error \r\n");
            }
        }
    }
    else
    {
        if (NULL != pUasRegInfo->tr && NULL != pUasRegInfo->tr->orig_request)
        {
            i = cs_generating_answer(pUasRegInfo->tr, pUasRegInfo->tr->orig_request, code, reasonphrase);
        }
    }

    if (pUasRegInfo->expires <= 0)
    {
        cs_timer_remove(IN_REG_EXPIRE, reg_info_index, NULL);
        uas_reginfo_remove(reg_info_index);
    }

    if ((code != 401) && (code >= 400))
    {
        cs_timer_remove(IN_REG_EXPIRE, reg_info_index, NULL);
        uas_reginfo_remove(reg_info_index);
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : SIP_UASAnswerToRegister4Auth
 功能描述  : 用户认证的注册回复消息
 输入参数  : int reg_info_index
             char* realm
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年7月14日 星期一
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int SIP_UASAnswerToRegister4Auth(int reg_info_index,  char* realm)
{
    int i = 0;
    uas_reg_info_t* pUasRegInfo = NULL;
    osip_message_t* response = NULL;

#if 0
    SIP_DEBUG_TRACE(LOG_INFO,  "SIP_UASAnswerToRegister4Auth() \
    \r\n In Para: \
    \r\n reg_info_index=%d \
    \r\n realm=%s \
    \r\n ", reg_info_index, realm);

#endif

    if (NULL == realm || realm[0] == '\0')
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "SIP_UASAnswerToRegister4Auth() exit---: Param Error \r\n");
        return -1;
    }

    if (!is_valid_uas_reg_info_index(reg_info_index))
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "SIP_UASAnswerToRegister4Auth() exit---: UAS Register Info Index Error \r\n");
        return -1;
    }

    pUasRegInfo = uas_reginfo_get(reg_info_index);

    if (NULL == pUasRegInfo)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "SIP_UASAnswerToRegister4Auth() exit---: Get UAS Register Info Error \r\n");
        return -1;
    }

    if (NULL != pUasRegInfo->tr && NULL != pUasRegInfo->tr->orig_request)
    {
        i = cs_generating_response_default(pUasRegInfo->tr->orig_request, 401, NULL, &response);

        cs_response_add_www_authenticate(response, realm);
        i = ul_sendmessage(pUasRegInfo->tr, response);

        if (i != 0)
        {
            SIP_DEBUG_TRACE(LOG_ERROR, "SIP_UASAnswerToRegister4Auth() ul_sendmessage Error \r\n");
        }
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : SIP_UASAnswerToRegister4RegExpire
 功能描述  : 用户注册刷新间隔时间太小的回复消息
 输入参数  : int reg_info_index
             int iMinRegExpire
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年7月15日 星期二
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int SIP_UASAnswerToRegister4RegExpire(int reg_info_index,  int iMinRegExpire)
{
    int i = 0;
    uas_reg_info_t* pUasRegInfo = NULL;
    osip_message_t* response = NULL;
    char strMinRegExpires[16] = {0};

#if 0
    SIP_DEBUG_TRACE(LOG_INFO,  "SIP_UASAnswerToRegister4Auth() \
    \r\n In Para: \
    \r\n reg_info_index=%d \
    \r\n MinRegExpire=%s \
    \r\n ", reg_info_index, iMinRegExpire);

#endif

    if (iMinRegExpire <= 0)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "SIP_UASAnswerToRegister4RegExpire() exit---: Param Error \r\n");
        return -1;
    }

    if (!is_valid_uas_reg_info_index(reg_info_index))
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "SIP_UASAnswerToRegister4RegExpire() exit---: UAS Register Info Index Error \r\n");
        return -1;
    }

    pUasRegInfo = uas_reginfo_get(reg_info_index);

    if (NULL == pUasRegInfo)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "SIP_UASAnswerToRegister4RegExpire() exit---: Get UAS Register Info Error \r\n");
        return -1;
    }

    if (NULL != pUasRegInfo->tr && NULL != pUasRegInfo->tr->orig_request)
    {
        i = cs_generating_response_default(pUasRegInfo->tr->orig_request, 423, NULL, &response);

        snprintf(strMinRegExpires, 16, "%d", iMinRegExpire);
        msg_setmin_expires(response, strMinRegExpires);

        i = ul_sendmessage(pUasRegInfo->tr, response);

        if (i != 0)
        {
            SIP_DEBUG_TRACE(LOG_ERROR, "SIP_UASAnswerToRegister4RegExpire() ul_sendmessage Error \r\n");
        }
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : SIP_ShowUASRegisterInfo
 功能描述  : 显示服务端的注册信息
 输入参数  : int sock
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年7月30日 星期二
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void SIP_ShowUASRegisterInfo(int sock)
{
    char strLine[] = "\r---------------------------------------------------------------------------------------------------------------\r\n";
    char strHead[] = "\rUAS Index   Register ID          Register Account           Login IP        Login Port Expires    Expires Count\r\n";
    uas_reg_info_t* pUasRegInfo = NULL;
    UAS_Reg_Info_Iterator UAS_RegInfo_Itr;

    char rbuf[128] = {0};

    if (sock > 0)
    {
        send(sock, strLine, strlen(strLine), 0);
        send(sock, strHead, strlen(strHead), 0);
    }

    UAS_SMUTEX_LOCK();

    if (g_UasRegInfoMap.size() <= 0)
    {
        UAS_SMUTEX_UNLOCK();
        return;
    }

    for (UAS_RegInfo_Itr = g_UasRegInfoMap.begin(); UAS_RegInfo_Itr != g_UasRegInfoMap.end(); UAS_RegInfo_Itr++)
    {
        pUasRegInfo = UAS_RegInfo_Itr->second;

        if ((NULL == pUasRegInfo) || (0 == pUasRegInfo->iUsed) || (pUasRegInfo->register_id[0] == '\0'))
        {
            continue;
        }

        if (pUasRegInfo->callid[0] == '\0')
        {
            continue;
        }

        snprintf(rbuf, 128, "\r%-11d %-20s %-26s %-15s %-10d %-10d %-13d\r\n", UAS_RegInfo_Itr->first, pUasRegInfo->register_id, pUasRegInfo->register_account, pUasRegInfo->from_host, pUasRegInfo->from_port, pUasRegInfo->expires, pUasRegInfo->expires_count);

        if (sock > 0)
        {
            send(sock, rbuf, strlen(rbuf), 0);
        }
    }

    if (sock > 0)
    {
        send(sock, strLine, strlen(strLine), 0);
    }

    UAS_SMUTEX_UNLOCK();

    return;
}

/*****************************************************************************
 函 数 名  : SIP_ShowUACRegisterInfo
 功能描述  : 显示客户端的注册信息
 输入参数  : int sock
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年2月10日 星期二
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
void SIP_ShowUACRegisterInfo(int sock)
{
    char strLine[] = "\r-------------------------------------------------------------------------------------------------------------------------------------------\r\n";
    char strHead[] = "\rUAC Index Local ID             Local IP        Local Port Proxy ID             Proxy IP        Proxy Port Expires LinkType isReged isReging\r\n";
    uac_reg_info_t* pUacRegInfo = NULL;
    UAC_Reg_Info_Iterator UAC_RegInfo_Itr;

    char rbuf[256] = {0};

    if (sock > 0)
    {
        send(sock, strLine, strlen(strLine), 0);
        send(sock, strHead, strlen(strHead), 0);
    }

    UAC_SMUTEX_LOCK();

    if (g_UacRegInfoMap.size() <= 0)
    {
        UAC_SMUTEX_UNLOCK();
        return;
    }

    for (UAC_RegInfo_Itr = g_UacRegInfoMap.begin(); UAC_RegInfo_Itr != g_UacRegInfoMap.end(); UAC_RegInfo_Itr++)
    {
        pUacRegInfo = UAC_RegInfo_Itr->second;

        if ((NULL == pUacRegInfo) || (0 == pUacRegInfo->iUsed) || (pUacRegInfo->register_id[0] == '\0'))
        {
            continue;
        }

        snprintf(rbuf, 256, "\r%-9d %-20s %-15s %-10d %-20s %-15s %-10d %-7d %-8d %-7d %-8d\r\n", UAC_RegInfo_Itr->first, pUacRegInfo->register_id, pUacRegInfo->localip, pUacRegInfo->localport, pUacRegInfo->proxy_id, pUacRegInfo->proxyip, pUacRegInfo->proxyport, pUacRegInfo->expires, pUacRegInfo->link_type, pUacRegInfo->isReg, pUacRegInfo->isReging);

        if (sock > 0)
        {
            send(sock, rbuf, strlen(rbuf), 0);
        }
    }

    if (sock > 0)
    {
        send(sock, strLine, strlen(strLine), 0);
    }

    UAC_SMUTEX_UNLOCK();

    return;
}

/*****************************************************************************
 函 数 名  : SIP_ReleaseAllUASRegisterInfo
 功能描述  : 释放所有服务器端注册信息
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年7月30日 星期二
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void SIP_ReleaseAllUASRegisterInfo()
{
    uas_reg_info_t* pUasRegInfo = NULL;
    UAS_Reg_Info_Iterator UAS_RegInfo_Itr;

    UAS_SMUTEX_LOCK();

    if (g_UasRegInfoMap.size() <= 0)
    {
        UAS_SMUTEX_UNLOCK();
        return;
    }

    for (UAS_RegInfo_Itr = g_UasRegInfoMap.begin(); UAS_RegInfo_Itr != g_UasRegInfoMap.end(); UAS_RegInfo_Itr++)
    {
        pUasRegInfo = UAS_RegInfo_Itr->second;

        if (NULL == pUasRegInfo)
        {
            continue;
        }

        uas_reginfo_free(pUasRegInfo);
        delete pUasRegInfo;
        pUasRegInfo = NULL;
    }

    UAS_SMUTEX_UNLOCK();
    return;
}

/*****************************************************************************
 函 数 名  : SIP_ReleaseAllUACRegisterInfo
 功能描述  : 释放客户端所有注册信息
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年7月30日 星期二
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void SIP_ReleaseAllUACRegisterInfo()
{
    uac_reg_info_t* pUacRegInfo = NULL;
    UAC_Reg_Info_Iterator Itr;

    UAC_SMUTEX_LOCK();

    if (g_UacRegInfoMap.size() <= 0)
    {
        UAC_SMUTEX_UNLOCK();
        return;
    }

    for (Itr = g_UacRegInfoMap.begin(); Itr != g_UacRegInfoMap.end(); Itr++)
    {
        pUacRegInfo = Itr->second;

        if ((NULL == pUacRegInfo) || (0 == pUacRegInfo->iUsed) || (pUacRegInfo->register_id[0] == '\0'))
        {
            continue;
        }

        uac_reginfo_free(pUacRegInfo);
    }

    UAC_SMUTEX_UNLOCK();
    return;
}

/*****************************************************************************
 函 数 名  : SIP_GetUASServerIP
 功能描述  : 获取UAS的注册服务器IP地址
 输入参数  : char* register_id
             char* login_ip
             int login_port
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年7月19日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
char* SIP_GetUASServerIP(char* register_id, char* login_ip, int login_port)
{
    int pos = -1;
    uas_reg_info_t* pUasRegInfo = NULL;

    if (NULL == register_id || NULL == login_ip || login_port <= 0)
    {
        return NULL;
    }

    pos = uas_reginfo_find(register_id, login_ip, login_port);

    if (pos < 0)
    {
        return NULL;
    }

    pUasRegInfo = uas_reginfo_get(pos);

    if (NULL == pUasRegInfo)
    {
        return NULL;
    }

    return pUasRegInfo->serverip;
}

/*****************************************************************************
 函 数 名  : SIP_GetUASServerPort
 功能描述  : 获取UAS的注册服务器端口
 输入参数  : char* register_id
             char* login_ip
             int login_port
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年7月19日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int SIP_GetUASServerPort(char* register_id, char* login_ip, int login_port)
{
    int pos = -1;
    uas_reg_info_t* pUasRegInfo = NULL;

    if (NULL == register_id || NULL == login_ip || login_port <= 0)
    {
        return -1;
    }

    pos = uas_reginfo_find(register_id, login_ip, login_port);

    if (pos < 0)
    {
        return -1;
    }

    pUasRegInfo = uas_reginfo_get(pos);

    if (NULL == pUasRegInfo)
    {
        return -1;
    }

    return pUasRegInfo->serverport;
}

/*****************************************************************************
 函 数 名  : SIP_GetUASRegExpires
 功能描述  : 获取UAS的超时时间
 输入参数  : char* register_id
             char* login_ip
             int login_port
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年7月19日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int SIP_GetUASRegExpires(char* register_id, char* login_ip, int login_port)
{
    int pos = -1;
    uas_reg_info_t* pUasRegInfo = NULL;

    if (NULL == register_id || NULL == login_ip || login_port <= 0)
    {
        return -1;
    }

    pos = uas_reginfo_find(register_id, login_ip, login_port);

    if (pos < 0)
    {
        return -1;
    }

    pUasRegInfo = uas_reginfo_get(pos);

    if (NULL == pUasRegInfo)
    {
        return -1;
    }

    return pUasRegInfo->expires;
}

/*****************************************************************************
 函 数 名  : SIP_GetUASCallID
 功能描述  : 获取UAS的Callid
 输入参数  : char* register_id
             char* login_ip
             int login_port
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年7月19日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
char* SIP_GetUASCallID(char* register_id, char* login_ip, int login_port)
{
    int pos = -1;
    uas_reg_info_t* pUasRegInfo = NULL;

    if (NULL == register_id || NULL == login_ip || login_port <= 0)
    {
        return NULL;
    }

    pos = uas_reginfo_find(register_id, login_ip, login_port);

    if (pos < 0)
    {
        return NULL;
    }

    pUasRegInfo = uas_reginfo_get(pos);

    if (NULL == pUasRegInfo)
    {
        return NULL;
    }

    return pUasRegInfo->callid;
}
#endif
