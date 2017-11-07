/******************************************************************************

                  版权所有 (C), 2001-2013, 金智视讯技术有限公司

 ******************************************************************************
  文 件 名   : sipua.c
  版 本 号   : 初稿
  作    者   : yanghaifeng
  生成日期   : 2013年4月2日
  最近修改   :
  功能描述   : UA会话信息维护操作
  函数列表   :
              add_dialog_subscription
              get_dialog_local_sdp
              get_dialog_remote_sdp
              get_dialog_sip_dialog
              get_dialog_subscription
              get_dialog_ui_state
              get_video_port
              is_valid_dialog_index
              scan_ua_dialogs
              sdp_accept_video_codec
              sdp_get_video_port
              sdp_local_config
              find_dialog_as_uac
              find_dialog_as_uas
              find_dialog_by_replaces
              find_subscription
              find_subscription_by_transaction
              set_dialog_sip_dialog
              set_dialog_ui_state
              sip_dialog_free
              sip_dialog_init_as_uac
              sip_dialog_init_as_uas
              sip_dialog_match_as_uac
              sip_dialog_match_as_uas
              sip_dialog_set_localcseq
              sip_dialog_set_localpresdp
              sip_dialog_set_localsdp
              sip_dialog_set_localtag
              sip_dialog_set_localuri
              sip_dialog_set_remotecontact
              sip_dialog_set_remotecseq
              sip_dialog_set_remotepresdp
              sip_dialog_set_remotesdp
              sip_dialog_set_remotetag
              sip_dialog_set_remoteuri
              sip_dialog_set_routeset
              sip_subscription_match
              tr_info_free
              tr_info_init
              uas_check8_2
              uas_check8_2_2
              uas_check8_2_3
              uas_check8_2_4
              uas_check_invite_within_dialog
              ua_dialog_free
              ua_dialog_add
              ua_dialog_init
              ua_dialog_list_free
              ua_dialog_list_init
              ua_dialog_list_lock
              ua_dialog_list_unlock
              ua_dialog_remove
              update_dialog_as_uac
              update_dialog_as_uas
              update_subscription_state
  修改历史   :
  1.日    期   : 2013年4月2日
    作    者   : yanghaifeng
    修改内容   : 创建文件

******************************************************************************/

/*----------------------------------------------*
 * 包含头文件                                   *
 *----------------------------------------------*/
#include <stdlib.h>
#ifdef WIN32
#include <winsock2.h>
#include <windows.h>
#include <sys/types.h>
#else
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#endif

#include "include/libsip.h"

#include "gbltype.h"
#include "sipua.inc"
#include "udp_tl.inc"
#include "csdbg.inc"
#include "callback.inc"

#include "sipmsg.inc"
#include "timerproc.inc"
#include "registrar.inc"

//added by chenyu 130522
#ifdef WIN32
#define vsnprintf _vsnprintf
#define snprintf  _snprintf
#endif

#ifdef MEMORY_LEAKS1
static int freesipuacptr = 0;
#endif

/*----------------------------------------------*
 * 外部变量说明                                 *
 *----------------------------------------------*/
extern osip_t* g_recv_cell;           /* 接收消息的SIP协议栈,非Message消息专用 */
extern osip_t* g_send_message_cell;   /* 发送消息的SIP协议栈,Message消息专用 */

extern UAS_Reg_Info_MAP g_UasRegInfoMap;
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
UA_Dialog_MAP g_UaDialogMap;
free_UA_Dialog_Queue g_FreeUaDialogQueue;
used_UA_Dialog_Queue g_UsedUaDialogQueue;

#ifdef MULTI_THR
osip_mutex_t* g_FreeUaDialogMapLock = NULL;
osip_mutex_t* g_UsedUaDialogMapLock = NULL;
#endif

/*----------------------------------------------*
 * 常量定义                                     *
 *----------------------------------------------*/
#define MAX_SIP_DIALOGS        15000  /*number of dialog that created by INVITE,SUBSCRIBE or REFER*/
#define MIN_SESSION_EXPIRE     300
#define MIN_SUB_EXPIRE  3600

/*----------------------------------------------*
 * 宏定义                                       *
 *----------------------------------------------*/

#if DECS("UA Dialog操作")
/*****************************************************************************
 函 数 名  : ua_dialog_init
 功能描述  : 初始化UA会话结构
 输入参数  : ua_dialog_t **ua_dialog
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月2日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int ua_dialog_init(ua_dialog_t** ua_dialog)
{
    *ua_dialog = (ua_dialog_t*)osip_malloc(sizeof(ua_dialog_t));

    if (NULL == (*ua_dialog))
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "ua_dialog_init() exit---: *info Smalloc Error \r\n");
        return -1;
    }

    (*ua_dialog)->iStatus = 0;

    (*ua_dialog)->strCallerID[0] = '\0';
    (*ua_dialog)->strCalleeID[0] = '\0';

    (*ua_dialog)->strUserName[0] = '\0';
    (*ua_dialog)->strPassword[0] = '\0';

    (*ua_dialog)->strRemoteIP[0] = '\0';
    (*ua_dialog)->iRemotePort = 0;

    (*ua_dialog)->strRemoteRTPIP[0] = '\0';
    (*ua_dialog)->iRemoteRTPPort = 0;
    (*ua_dialog)->pRemoteSDP = NULL;

    (*ua_dialog)->strLocalIP[0] = '\0';
    (*ua_dialog)->iLocalPort = 0;

    (*ua_dialog)->strLocalRTPIP[0] = '\0';
    (*ua_dialog)->iLocalRTPPort = 0;
    (*ua_dialog)->pLocalSDP = NULL;

    (*ua_dialog)->pServerTr = NULL;
    (*ua_dialog)->pServerSIP = NULL;
    (*ua_dialog)->pAuthorization = NULL;

    (*ua_dialog)->iStatusCode = 0;
    (*ua_dialog)->eUiState = (ui_state_t)0;

    (*ua_dialog)->iReInviteCnt = 0;
    (*ua_dialog)->iSessionExpires = 0;
    (*ua_dialog)->iSessionExpiresCount = 0;
    (*ua_dialog)->iUpdateSendCount = 0;

    (*ua_dialog)->pSipDialog = NULL;
    (*ua_dialog)->subscription = NULL;

    (*ua_dialog)->uSeqNnumber = 0;
    (*ua_dialog)->ulTimeStamp = 0;

#ifdef MEMORY_LEAKS1
    static int comptr = 0;
    comptr++;
    freesipuacptr++;

    fprintf(stdout, (char*)"\r\n<sipua.c> ua_dialog_init() malloc: (address = 0x%lx) comptr: %d, existing element %d\r\n", (long unsigned int)*ua_dialog, comptr, freesipuacptr);
    fflush(stdout);
#endif

    return 0;
}

/*****************************************************************************
 函 数 名  : ua_dialog_free
 功能描述  : 释放UA会话结构
 输入参数  : ua_dialog_t *ua_dialog
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月2日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void ua_dialog_free(ua_dialog_t* ua_dialog)
{
    if (NULL == ua_dialog)
    {
        //SIP_DEBUG_TRACE(LOG_DEBUG, "ua_dialog_free() exit---: Param Error \r\n");
        return;
    }

    if ((ua_dialog->pSipDialog != NULL)
        || (ua_dialog->eUiState == UI_STATE_IDLE && ua_dialog->pSipDialog != NULL))
    {
        sip_dialog_free(ua_dialog->pSipDialog);
        osip_free(ua_dialog->pSipDialog);
        ua_dialog->pSipDialog = NULL;
    }

    if (ua_dialog->subscription != NULL)
    {
        sip_subscription_free(ua_dialog->subscription);
        osip_free(ua_dialog->subscription);
        ua_dialog->subscription = NULL;
    }

    ua_dialog->iStatus = 0;

    memset(ua_dialog->strCallerID, 0, 132);
    memset(ua_dialog->strCalleeID, 0, 132);

    memset(ua_dialog->strUserName, 0, 132);
    memset(ua_dialog->strPassword, 0, 132);

    memset(ua_dialog->strRemoteIP, 0, 16);
    ua_dialog->iRemotePort = 0;

    memset(ua_dialog->strRemoteRTPIP, 0, 16);
    ua_dialog->iRemoteRTPPort = 0;

    if (NULL != ua_dialog->pRemoteSDP)
    {
        sdp_message_free(ua_dialog->pRemoteSDP);
        ua_dialog->pRemoteSDP = NULL;
    }

    memset(ua_dialog->strLocalIP, 0, 16);
    ua_dialog->iLocalPort = 0;

    memset(ua_dialog->strLocalRTPIP, 0, 16);
    ua_dialog->iLocalRTPPort = 0;

    if (NULL != ua_dialog->pLocalSDP)
    {
        sdp_message_free(ua_dialog->pLocalSDP);
        ua_dialog->pLocalSDP = NULL;
    }

    ua_dialog->pServerTr = NULL;
    ua_dialog->pServerSIP = NULL;

    if (NULL != ua_dialog->pAuthorization)
    {
        osip_authorization_free(ua_dialog->pAuthorization);
        ua_dialog->pAuthorization = NULL;
    }

    ua_dialog->iStatusCode = 0;
    ua_dialog->eUiState = (ui_state_t)0;

    ua_dialog->iReInviteCnt = 0;
    ua_dialog->iSessionExpires = 0;
    ua_dialog->iSessionExpiresCount = 0;
    ua_dialog->iUpdateSendCount = 0;

    ua_dialog->uSeqNnumber = 0;
    ua_dialog->ulTimeStamp = 0;

#ifdef MEMORY_LEAKS1
    static int comptr = 0;
    comptr++;
    freesipuacptr--;

    fprintf(stdout, (char*)"\r\n<sipua.c> ua_dialog_free() free: (address = 0x%lx) comptr: %d, existing element %d\r\n", (long unsigned int)ua_dialog, comptr, freesipuacptr);
    fflush(stdout);
#endif

    return;
}

/*****************************************************************************
 函 数 名  : ua_dialog_list_init
 功能描述  : 初始化UA会话列表
 输入参数  : ua_dialog_list_t **ua_dialog_list
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月2日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int  ua_dialog_list_init()
{
    int i = 0;
    int pos = 0;

    g_UaDialogMap.clear();
    g_FreeUaDialogQueue.clear();
    g_UsedUaDialogQueue.clear();

    for (pos = 0; pos < MAX_SIP_DIALOGS; pos++)
    {
        ua_dialog_t* pUaDialog = NULL;

        i = ua_dialog_init(&pUaDialog);

        if (i != 0)
        {
            SIP_DEBUG_TRACE(LOG_DEBUG, "ua_dialog_list_init() exit---: UA Dialog Init Error \r\n");
            return -1;
        }

        g_UaDialogMap[pos] = pUaDialog;

        g_FreeUaDialogQueue.push_back(pos);

    }

#ifdef MULTI_THR
    g_FreeUaDialogMapLock = (osip_mutex_t*)osip_mutex_init();

    if (NULL == g_FreeUaDialogMapLock)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "ua_dialog_list_init() exit---: Free UA Dialog Map Lock Init Error \r\n");
        return -1;
    }

    g_UsedUaDialogMapLock = (osip_mutex_t*)osip_mutex_init();

    if (NULL == g_UsedUaDialogMapLock)
    {
        osip_mutex_destroy((struct osip_mutex*)g_FreeUaDialogMapLock);
        g_FreeUaDialogMapLock = NULL;

        SIP_DEBUG_TRACE(LOG_DEBUG, "ua_dialog_list_init() exit---: Used UA Dialog Map Lock Init Error \r\n");
        return -1;
    }

#endif
    return 0;
}

/*****************************************************************************
 函 数 名  : ua_dialog_list_free
 功能描述  : 释放UA会话列表
 输入参数  : ua_dialog_list_t * ua_dialog_list
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月2日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void ua_dialog_list_free()
{
    int pos = 0;
    ua_dialog_t* ua_dialog = NULL;

    for (pos = 0; pos < MAX_SIP_DIALOGS; pos++)
    {
        ua_dialog = g_UaDialogMap[pos];

        if (NULL != ua_dialog)
        {
            ua_dialog_free(ua_dialog);
            osip_free(ua_dialog);
            ua_dialog = NULL;
        }
    }

    g_UaDialogMap.clear();
    g_FreeUaDialogQueue.clear();
    g_UsedUaDialogQueue.clear();

#ifdef MULTI_THR

    if (NULL != g_FreeUaDialogMapLock)
    {
        osip_mutex_destroy((struct osip_mutex*)g_FreeUaDialogMapLock);
        g_FreeUaDialogMapLock = NULL;
    }

    if (NULL != g_UsedUaDialogMapLock)
    {
        osip_mutex_destroy((struct osip_mutex*)g_UsedUaDialogMapLock);
        g_UsedUaDialogMapLock = NULL;
    }

#endif
    return;
}

/*****************************************************************************
 函 数 名  : free_ua_dialog_list_lock
 功能描述  : 锁定空闲UA会话列表
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月2日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int free_ua_dialog_list_lock()
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_FreeUaDialogMapLock == NULL)
    {
        //SIP_DEBUG_TRACE(LOG_DEBUG, "free_ua_dialog_list_lock() exit---: Param Error \r\n");
        return -1;
    }

#ifndef WIN32 //modified by chenyu 131024
    iRet = osip_mutex_lock((struct osip_mutex*)g_FreeUaDialogMapLock);
#else
    iRet = osip_mutex_lock((struct osip_mutex*)g_FreeUaDialogMapLock);
#endif

#endif
    return iRet;
}

/*****************************************************************************
 函 数 名  : free_ua_dialog_list_unlock
 功能描述  : 解锁空闲UA会话列表
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月2日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int free_ua_dialog_list_unlock()
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_FreeUaDialogMapLock == NULL)
    {
        //SIP_DEBUG_TRACE(LOG_DEBUG, "free_ua_dialog_list_unlock() exit---: Param Error \r\n");
        return -1;
    }

    iRet = osip_mutex_unlock((struct osip_mutex*)g_FreeUaDialogMapLock);

#endif
    return iRet;
}

/*****************************************************************************
 函 数 名  : debug_free_ua_dialog_list_lock
 功能描述  : 锁定空闲UA会话列表
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月2日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int debug_free_ua_dialog_list_lock(const char* file, int line, const char* func)
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_FreeUaDialogMapLock == NULL)
    {
        //SIP_DEBUG_TRACE(LOG_DEBUG, "debug_free_ua_dialog_list_lock() exit---: Param Error \r\n");
        return -1;
    }

#ifndef WIN32   //modified by chenyu 131024
    iRet = osip_debug_mutex_lock((struct osip_mutex*)g_FreeUaDialogMapLock, file, line, func);
#else
    iRet = osip_mutex_lock((struct osip_mutex*)g_FreeUaDialogMapLock);
#endif

#endif
    return iRet;
}

/*****************************************************************************
 函 数 名  : debug_free_ua_dialog_list_unlock
 功能描述  : 解锁空闲UA会话列表
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月2日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int debug_free_ua_dialog_list_unlock(const char* file, int line, const char* func)
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_FreeUaDialogMapLock == NULL)
    {
        //SIP_DEBUG_TRACE(LOG_DEBUG, "debug_free_ua_dialog_list_unlock() exit---: Param Error \r\n");
        return -1;
    }

#ifndef WIN32   //modified by chenyu 131024
    iRet = osip_debug_mutex_unlock((struct osip_mutex*)g_FreeUaDialogMapLock, file, line, func);
#else
    iRet = osip_mutex_unlock((struct osip_mutex*)g_FreeUaDialogMapLock);
#endif

#endif
    return iRet;
}

/*****************************************************************************
 函 数 名  : used_ua_dialog_list_lock
 功能描述  : 锁定已经使用UA会话列表
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月2日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int used_ua_dialog_list_lock()
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_UsedUaDialogMapLock == NULL)
    {
        //SIP_DEBUG_TRACE(LOG_DEBUG, "used_ua_dialog_list_lock() exit---: Param Error \r\n");
        return -1;
    }

#ifndef WIN32 //modified by chenyu 131024
    iRet = osip_mutex_lock((struct osip_mutex*)g_UsedUaDialogMapLock);
#else
    iRet = osip_mutex_lock((struct osip_mutex*)g_UsedUaDialogMapLock);
#endif

#endif
    return iRet;
}

/*****************************************************************************
 函 数 名  : used_ua_dialog_list_unlock
 功能描述  : 解锁已经使用UA会话列表
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月2日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int used_ua_dialog_list_unlock()
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_UsedUaDialogMapLock == NULL)
    {
        //SIP_DEBUG_TRACE(LOG_DEBUG, "used_ua_dialog_list_unlock() exit---: Param Error \r\n");
        return -1;
    }

    iRet = osip_mutex_unlock((struct osip_mutex*)g_UsedUaDialogMapLock);

#endif
    return iRet;
}

/*****************************************************************************
 函 数 名  : debug_used_ua_dialog_list_lock
 功能描述  : 锁定已经使用UA会话列表
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月2日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int debug_used_ua_dialog_list_lock(const char* file, int line, const char* func)
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_UsedUaDialogMapLock == NULL)
    {
        //SIP_DEBUG_TRACE(LOG_DEBUG, "debug_used_ua_dialog_list_lock() exit---: Param Error \r\n");
        return -1;
    }

#ifndef WIN32   //modified by chenyu 131024
    iRet = osip_debug_mutex_lock((struct osip_mutex*)g_UsedUaDialogMapLock, file, line, func);
#else
    iRet = osip_mutex_lock((struct osip_mutex*)g_UsedUaDialogMapLock);
#endif

#endif
    return iRet;
}

/*****************************************************************************
 函 数 名  : debug_used_ua_dialog_list_unlock
 功能描述  : 解锁已经使用UA会话列表
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月2日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int debug_used_ua_dialog_list_unlock(const char* file, int line, const char* func)
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_UsedUaDialogMapLock == NULL)
    {
        //SIP_DEBUG_TRACE(LOG_DEBUG, "debug_used_ua_dialog_list_unlock() exit---: Param Error \r\n");
        return -1;
    }

#ifndef WIN32   //modified by chenyu 131024
    iRet = osip_debug_mutex_unlock((struct osip_mutex*)g_UsedUaDialogMapLock, file, line, func);
#else
    iRet = osip_mutex_unlock((struct osip_mutex*)g_UsedUaDialogMapLock);
#endif

#endif
    return iRet;
}

/*****************************************************************************
 函 数 名  : ua_dialog_add
 功能描述  : 添加一路空闲的UA会话空间
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月2日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int ua_dialog_add()
{
    int count = 0;
    int index = -1;

    /* 先从空闲队列中获取一个，移除出队列 */
    FREE_UA_SMUTEX_LOCK();
    count = g_FreeUaDialogQueue.size();

    if (count == 0)
    {
        FREE_UA_SMUTEX_UNLOCK();
        return -1;
    }

    index = g_FreeUaDialogQueue.front();
    g_FreeUaDialogQueue.pop_front();

    FREE_UA_SMUTEX_UNLOCK();


    /* 插入使用的队列*/
    USED_UA_SMUTEX_LOCK();

    g_UsedUaDialogQueue.push_back(index);

    USED_UA_SMUTEX_UNLOCK();

    return index;
}

/*****************************************************************************
 函 数 名  : ua_dialog_remove
 功能描述  : 删除一路UA会话信息
 输入参数  : int pos
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月2日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void ua_dialog_remove(int pos)
{
    int i = -1;
    int index = -1;
    ua_dialog_t* pUaDialog = NULL;
    used_UA_Dialog_Iterator Itr;
    vector<int> UAIndexVector;

    UAIndexVector.clear();

    if (!is_valid_dialog_index(pos))
    {
        //SIP_DEBUG_TRACE(LOG_DEBUG, "ua_dialog_remove() exit---: Dialog Index Error \r\n");
        return;
    }

    /* 先查找使用的队列，移除出队列 */
    USED_UA_SMUTEX_LOCK();

    if (g_UsedUaDialogQueue.size() <= 0)
    {
        USED_UA_SMUTEX_UNLOCK();
        return;
    }

    for (Itr = g_UsedUaDialogQueue.begin(); Itr != g_UsedUaDialogQueue.end();)
    {
        index = *Itr;

        if (!is_valid_dialog_index(index))
        {
            Itr = g_UsedUaDialogQueue.erase(Itr);
            continue;
        }

        if (index == pos)
        {
            Itr = g_UsedUaDialogQueue.erase(Itr);

            pUaDialog = g_UaDialogMap[index];

            ua_dialog_free(pUaDialog);

            UAIndexVector.push_back(index);

            continue;
        }

        Itr++;
    }

    USED_UA_SMUTEX_UNLOCK();

    if (UAIndexVector.size() > 0)
    {
        /* 插入未使用的队列*/
        FREE_UA_SMUTEX_LOCK();

        for (i = 0; i < (int)UAIndexVector.size(); i++)
        {
            index = UAIndexVector[i];

            if (is_valid_dialog_index(index))
            {
                g_FreeUaDialogQueue.push_back(index);
            }
        }

        FREE_UA_SMUTEX_UNLOCK();

        UAIndexVector.clear();
    }

    return;
}

/*****************************************************************************
 函 数 名  : ua_dialog_get
 功能描述  : 获取ua dialog
 输入参数  : int pos
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年5月31日 星期五
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
ua_dialog_t* ua_dialog_get(int pos)
{
    int index = -1;
    ua_dialog_t* pUaDialog = NULL;
    used_UA_Dialog_Iterator Itr;

    if (!is_valid_dialog_index(pos))
    {
        //SIP_DEBUG_TRACE(LOG_DEBUG, "ua_dialog_get() exit---: Dialog Index Error \r\n");
        return NULL;
    }

    USED_UA_SMUTEX_LOCK();

    if (g_UsedUaDialogQueue.size() <= 0)
    {
        USED_UA_SMUTEX_UNLOCK();
        return NULL;
    }

    for (Itr = g_UsedUaDialogQueue.begin(); Itr != g_UsedUaDialogQueue.end();)
    {
        index = *Itr;

        if (!is_valid_dialog_index(index))
        {
            Itr = g_UsedUaDialogQueue.erase(Itr);
            continue;
        }

        if (index == pos)
        {
            pUaDialog = g_UaDialogMap[index];

            if (NULL != pUaDialog)
            {
                USED_UA_SMUTEX_UNLOCK();
                return pUaDialog;
            }
            else
            {
                USED_UA_SMUTEX_UNLOCK();
                return NULL;
            }
        }

        Itr++;
    }

    USED_UA_SMUTEX_UNLOCK();
    return NULL;
}

/*****************************************************************************
 函 数 名  : ua_dialog_get2
 功能描述  : 获取ua dialog
 输入参数  : int pos
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年5月31日 星期五
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
ua_dialog_t* ua_dialog_get2(int pos)
{
    int index = -1;
    ua_dialog_t* pUaDialog = NULL;
    used_UA_Dialog_Iterator Itr;

    if (!is_valid_dialog_index(pos))
    {
        //SIP_DEBUG_TRACE(LOG_DEBUG, "ua_dialog_get() exit---: Dialog Index Error \r\n");
        return NULL;
    }

    if (g_UsedUaDialogQueue.size() <= 0)
    {
        return NULL;
    }

    for (Itr = g_UsedUaDialogQueue.begin(); Itr != g_UsedUaDialogQueue.end();)
    {
        index = *Itr;

        if (!is_valid_dialog_index(index))
        {
            Itr = g_UsedUaDialogQueue.erase(Itr);
            continue;
        }

        if (index == pos)
        {
            pUaDialog = g_UaDialogMap[index];

            if (NULL != pUaDialog)
            {
                return pUaDialog;
            }
            else
            {
                return NULL;
            }
        }

        Itr++;
    }

    return NULL;
}

/*****************************************************************************
 函 数 名  : is_valid_dialog_index
 功能描述  : UA会话索引是否合法
 输入参数  : int index
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月2日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int is_valid_dialog_index(int index)
{
    if (index < 0 || index >= (int)g_UaDialogMap.size())
    {
        return 0;
    }

    return 1;
}

/*****************************************************************************
 函 数 名  : find_dialog_as_uac
 功能描述  : 作为UAC查找UA dialog
 输入参数  : sip_t *answer
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月2日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int find_dialog_as_uac(osip_message_t* answer)
{
    int index = -1;
    ua_dialog_t* pUaDialog = NULL;
    sip_dialog_t* pSipDlg = NULL;
    used_UA_Dialog_Iterator Itr;

    if (NULL == answer)
    {
        //SIP_DEBUG_TRACE(LOG_DEBUG, "find_dialog_as_uac() exit---: Param Error \r\n");
        return -1;
    }

    USED_UA_SMUTEX_LOCK();

    if (g_UsedUaDialogQueue.size() <= 0)
    {
        USED_UA_SMUTEX_UNLOCK();
        //SIP_DEBUG_TRACE(LOG_DEBUG, "find_dialog_as_uac() exit---: UA Dialog Map NULL \r\n");
        return -1;
    }

    for (Itr = g_UsedUaDialogQueue.begin(); Itr != g_UsedUaDialogQueue.end();)
    {
        index = *Itr;

        if (!is_valid_dialog_index(index))
        {
            Itr = g_UsedUaDialogQueue.erase(Itr);
            continue;
        }

        pUaDialog = g_UaDialogMap[index];

        if (NULL == pUaDialog)
        {
            Itr++;
            continue;
        }

        pSipDlg = pUaDialog->pSipDialog;

        if (NULL == pSipDlg)
        {
            Itr++;
            continue;
        }

        if (pSipDlg->state == DLG_TERMINATED)
        {
            Itr++;
            continue;
        }

        if (sip_dialog_match_as_uac(pSipDlg, answer) == 0)
        {
            USED_UA_SMUTEX_UNLOCK();
            return index;
        }

        Itr++;
    }

    USED_UA_SMUTEX_UNLOCK();
    return -1;
}

/*****************************************************************************
 函 数 名  : find_dialog_as_uas
 功能描述  : 作为UAS查找UA dialog
 输入参数  : sip_t *request
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月2日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int find_dialog_as_uas(osip_message_t* request)
{
    int index = -1;
    ua_dialog_t* pUaDialog = NULL;
    sip_dialog_t* pSipDlg = NULL;
    used_UA_Dialog_Iterator Itr;

    if (NULL == request)
    {
        //SIP_DEBUG_TRACE(LOG_DEBUG, "find_dialog_as_uas() exit---: Param Error \r\n");
        return -1;
    }

    USED_UA_SMUTEX_LOCK();

    if (g_UsedUaDialogQueue.size() <= 0)
    {
        USED_UA_SMUTEX_UNLOCK();
        //SIP_DEBUG_TRACE(LOG_DEBUG, "find_dialog_as_uas() exit---: UA Dialog Map NULL \r\n");
        return -1;
    }

    for (Itr = g_UsedUaDialogQueue.begin(); Itr != g_UsedUaDialogQueue.end();)
    {
        index = *Itr;

        if (!is_valid_dialog_index(index))
        {
            Itr = g_UsedUaDialogQueue.erase(Itr);
            continue;
        }

        pUaDialog = g_UaDialogMap[index];

        if (NULL == pUaDialog)
        {
            Itr++;
            continue;
        }

        pSipDlg = pUaDialog->pSipDialog;

        if (NULL == pSipDlg)
        {
            Itr++;
            continue;
        }

        if (pSipDlg->state == DLG_TERMINATED)
        {
            Itr++;
            continue;
        }

        if (sip_dialog_match_as_uas(pSipDlg, request) == 0)
        {
            USED_UA_SMUTEX_UNLOCK();
            return index;
        }

        Itr++;
    }

    USED_UA_SMUTEX_UNLOCK();
    return -1;
}

/*****************************************************************************
 函 数 名  : update_dialog_as_uac
 功能描述  :  作为UAC更新UA dialog
 输入参数  : int index
             transaction_t *tr
             sip_t *sip
             dialog_event_t event
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月2日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int update_dialog_as_uac(int index, osip_transaction_t* tr, osip_message_t* sip, dialog_event_t event)
{
    osip_uri_param_t* to_tag = NULL;
    osip_contact_t* contact = NULL;
    dialog_event_t evt = DLG_EVENT_NULL;
    sip_dialog_t* pSipDlg = NULL;

    if (!is_valid_dialog_index(index))
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "update_dialog_as_uac() exit---: Dialog Index Error \r\n");
        return -1;
    }

    pSipDlg = get_dialog_sip_dialog(index);

    if (NULL == pSipDlg)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "update_dialog_as_uac() exit---: Get SIP Dialog Error \r\n");
        return -1;
    }

    if (pSipDlg->state == DLG_TERMINATED) /* NOT need to update. wait for free */
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "update_dialog_as_uac() exit---: SIP Dialog State Error \r\n");
        return 0;
    }

    if (pSipDlg->state == DLG_CONFIRMED)
    {
        if (event == DLG_EVENT_1XXNOTAG
            || event == DLG_EVENT_1XXTAG
            || event == DLG_EVENT_2XX
            || event == DLG_EVENT_CANCELLED
            || event == DLG_EVENT_REJECTED
            || event == DLG_EVENT_ERROR
            || event == DLG_EVENT_TIMEOUT)
        {
            evt = DLG_EVENT_UPDATE;
        }
        else
        {
            evt = event;
        }
    }
    else
    {
        evt = event;
    }

    switch (evt)
    {
        case DLG_EVENT_1XXNOTAG:
            if (pSipDlg->state == DLG_TRYING)
            {
                pSipDlg->state = DLG_PROCEEDDING;
                sip_dialog_set_remoteuri(pSipDlg, sip->to);
            }

            break;

        case DLG_EVENT_1XXTAG:
            if (pSipDlg->state == DLG_TRYING
                || pSipDlg->state == DLG_PROCEEDDING)
            {
                pSipDlg->state = DLG_EARLY;

                if (NULL != sip->to)
                {
                    osip_to_get_tag(sip->to, &to_tag);

                    if (to_tag)
                    {
                        sip_dialog_set_remotetag(pSipDlg, to_tag->gvalue);   /* update remote tag */
                    }

                    sip_dialog_set_remoteuri(pSipDlg, sip->to);   /* update remote uri */
                }
            }

            break;

        case DLG_EVENT_2XX:
            if (pSipDlg->state == DLG_TRYING
                || pSipDlg->state == DLG_PROCEEDDING
                || pSipDlg->state == DLG_EARLY)
            {
                pSipDlg->state = DLG_CONFIRMED;

                if (NULL != sip->to)
                {
                    osip_to_get_tag(sip->to, &to_tag);

                    if (to_tag)
                    {
                        sip_dialog_set_remotetag(pSipDlg, to_tag->gvalue);   /* update remote tag */
                    }

                    sip_dialog_set_remoteuri(pSipDlg, sip->to);   /* update remote uri */
                }

                sip_dialog_set_routeset(pSipDlg, &sip->record_routes);    /* update route set */

                if (!osip_list_eol(&sip->contacts, 0))     /* set remote contact uri */
                {
                    contact = (osip_contact_t*)osip_list_get(&sip->contacts, 0);

                    if (0 == url_compare(contact->url, sip->to->url)) /* 如果不一样，可能前端是通过NAT上来的,前端的contact就是to里面的，防止后面ack和bye消息发到私网 */
                    {
                        sip_dialog_set_remotecontact(pSipDlg, contact);
                    }
                    else
                    {
                        sip_dialog_set_remotecontact(pSipDlg, (osip_contact_t*)sip->to);
                    }
                }

                if (pSipDlg->type2 != DLG_INVITE)
                {
                    pSipDlg->nict_tr = NULL;
                    break;
                }

                pSipDlg->ict_tr = NULL;
            }

            break;

        case DLG_EVENT_CANCELLED:
        case DLG_EVENT_REJECTED:
        case DLG_EVENT_ERROR:
        case DLG_EVENT_TIMEOUT:
            if (pSipDlg->type2 != DLG_INVITE)
            {
                pSipDlg->nict_tr = NULL;
                pSipDlg->state = DLG_TERMINATED;
                break;
            }

            pSipDlg->ict_tr = NULL;
            pSipDlg->state = DLG_TERMINATED;
            break;

        case DLG_EVENT_REPLACED:
            pSipDlg->state = DLG_TERMINATED;
            break;

        case DLG_EVENT_LOCALBYE:

            if (pSipDlg->type2 != DLG_INVITE)
            {
                pSipDlg->state = DLG_TERMINATED;
                break;
            }

            pSipDlg->state = DLG_TERMINATED;
            break;

        case DLG_EVENT_REMOTEBYE:

            if (pSipDlg->type2 != DLG_INVITE)
            {
                pSipDlg->state = DLG_TERMINATED;
                break;
            }

            pSipDlg->state = DLG_TERMINATED;
            break;

        case DLG_EVENT_NULL:
            break;

        case DLG_EVENT_UPDATE:
            if (pSipDlg->state != DLG_CONFIRMED)
            {
                SIP_DEBUG_TRACE(LOG_DEBUG, "update_dialog_as_uac() exit---: SIP Dialog State Error \r\n");
                return -1;
            }

            if (MSG_IS_REQUEST(sip))
            {
                if (MSG_IS_INVITE(sip))
                {
                    pSipDlg->ict_tr = tr;
                    pSipDlg->local_cseq = osip_atoi(sip->cseq->number);  /* update local cseq number */
                }
                else if (MSG_IS_ACK(sip))
                {
                }
                else if (MSG_IS_SUBSCRIBE(sip))
                {
                    pSipDlg->nict_tr = tr;
                    pSipDlg->local_cseq = osip_atoi(sip->cseq->number);   /* update local cseq number */

                }
                else if (MSG_IS_REFER(sip))
                {
                    pSipDlg->nict_tr = tr;
                    pSipDlg->local_cseq = osip_atoi(sip->cseq->number);   /* update local cseq number */
                }
            }
            else
            {
                if (MSG_IS_STATUS_1XX(sip))
                {
                    break;
                }

                if (MSG_IS_STATUS_2XX(sip))
                {
                    if (MSG_IS_RESPONSE_FOR(sip, "INVITE"))
                    {
                        pSipDlg->ict_tr = NULL;
                    }
                    else
                    {
                        pSipDlg->nict_tr = NULL;
                    }

                    if (!MSG_IS_RESPONSE_FOR(sip, "INVITE")
                        && !MSG_IS_RESPONSE_FOR(sip, "SUBSCRIBE")
                        && !MSG_IS_RESPONSE_FOR(sip, "REFER"))
                    {
                        break;
                    }

                    sip_dialog_set_routeset(pSipDlg, &sip->record_routes);   /* update route set */

                    if (!osip_list_eol(&sip->contacts, 0))     /* update remote contact uri */
                    {
                        contact = (osip_contact_t*)osip_list_get(&sip->contacts, 0);

                        if (0 == url_compare(contact->url, sip->to->url)) /* 如果不一样，可能前端是通过NAT上来的,前端的contact就是to里面的，防止后面ack和bye消息发到私网 */
                        {
                            sip_dialog_set_remotecontact(pSipDlg, contact);
                        }
                        else
                        {
                            sip_dialog_set_remotecontact(pSipDlg, (osip_contact_t*)sip->to);
                        }
                    }
                }
                else
                {
                    if (MSG_IS_RESPONSE_FOR(sip, "INVITE"))
                    {
                        pSipDlg->ict_tr = NULL;
                    }
                    else
                    {
                        pSipDlg->nict_tr = NULL;
                    }
                }
            }

            break;
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : update_dialog_as_uac2
 功能描述  :  作为UAC更新UA dialog
 输入参数  : sip_dialog_t* pSipDlg
             transaction_t *tr
             sip_t *sip
             dialog_event_t event
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月2日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int update_dialog_as_uac2(sip_dialog_t* pSipDlg, osip_transaction_t* tr, osip_message_t* sip, dialog_event_t event)
{
    osip_uri_param_t* to_tag = NULL;
    osip_contact_t* contact = NULL;
    dialog_event_t evt = DLG_EVENT_NULL;

    if (NULL == pSipDlg)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "update_dialog_as_uac2() exit---: SIP Dialog Error \r\n");
        return -1;
    }

    if (pSipDlg->state == DLG_TERMINATED) /* NOT need to update. wait for free */
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "update_dialog_as_uac2() exit---: SIP Dialog State Error \r\n");
        return 0;
    }

    if (pSipDlg->state == DLG_CONFIRMED)
    {
        if (event == DLG_EVENT_1XXNOTAG
            || event == DLG_EVENT_1XXTAG
            || event == DLG_EVENT_2XX
            || event == DLG_EVENT_CANCELLED
            || event == DLG_EVENT_REJECTED
            || event == DLG_EVENT_ERROR
            || event == DLG_EVENT_TIMEOUT)
        {
            evt = DLG_EVENT_UPDATE;
        }
        else
        {
            evt = event;
        }
    }
    else
    {
        evt = event;
    }

    switch (evt)
    {
        case DLG_EVENT_1XXNOTAG:
            if (pSipDlg->state == DLG_TRYING)
            {
                pSipDlg->state = DLG_PROCEEDDING;
                sip_dialog_set_remoteuri(pSipDlg, sip->to);
            }

            break;

        case DLG_EVENT_1XXTAG:
            if (pSipDlg->state == DLG_TRYING
                || pSipDlg->state == DLG_PROCEEDDING)
            {
                pSipDlg->state = DLG_EARLY;

                if (NULL != sip->to)
                {
                    osip_to_get_tag(sip->to, &to_tag);

                    if (to_tag)
                    {
                        sip_dialog_set_remotetag(pSipDlg, to_tag->gvalue);   /* update remote tag */
                    }

                    sip_dialog_set_remoteuri(pSipDlg, sip->to);   /* update remote uri */
                }
            }

            break;

        case DLG_EVENT_2XX:
            if (pSipDlg->state == DLG_TRYING
                || pSipDlg->state == DLG_PROCEEDDING
                || pSipDlg->state == DLG_EARLY)
            {
                pSipDlg->state = DLG_CONFIRMED;

                if (NULL != sip->to)
                {
                    osip_to_get_tag(sip->to, &to_tag);

                    if (to_tag)
                    {
                        sip_dialog_set_remotetag(pSipDlg, to_tag->gvalue);   /* update remote tag */
                    }

                    sip_dialog_set_remoteuri(pSipDlg, sip->to);   /* update remote uri */
                }

                sip_dialog_set_routeset(pSipDlg, &sip->record_routes);    /* update route set */

                if (!osip_list_eol(&sip->contacts, 0))     /* set remote contact uri */
                {
                    contact = (osip_contact_t*)osip_list_get(&sip->contacts, 0);

                    if (0 == url_compare(contact->url, sip->to->url)) /* 如果不一样，可能前端是通过NAT上来的,前端的contact就是to里面的，防止后面ack和bye消息发到私网 */
                    {
                        sip_dialog_set_remotecontact(pSipDlg, contact);
                    }
                    else
                    {
                        sip_dialog_set_remotecontact(pSipDlg, (osip_contact_t*)sip->to);
                    }
                }

                if (pSipDlg->type2 != DLG_INVITE)
                {
                    pSipDlg->nict_tr = NULL;
                    break;
                }

                pSipDlg->ict_tr = NULL;
            }

            break;

        case DLG_EVENT_CANCELLED:
        case DLG_EVENT_REJECTED:
        case DLG_EVENT_ERROR:
        case DLG_EVENT_TIMEOUT:
            if (pSipDlg->type2 != DLG_INVITE)
            {
                pSipDlg->nict_tr = NULL;
                pSipDlg->state = DLG_TERMINATED;
                break;
            }

            pSipDlg->ict_tr = NULL;
            pSipDlg->state = DLG_TERMINATED;
            break;

        case DLG_EVENT_REPLACED:
            pSipDlg->state = DLG_TERMINATED;
            break;

        case DLG_EVENT_LOCALBYE:

            if (pSipDlg->type2 != DLG_INVITE)
            {
                pSipDlg->state = DLG_TERMINATED;
                break;
            }

            pSipDlg->state = DLG_TERMINATED;
            break;

        case DLG_EVENT_REMOTEBYE:

            if (pSipDlg->type2 != DLG_INVITE)
            {
                pSipDlg->state = DLG_TERMINATED;
                break;
            }

            pSipDlg->state = DLG_TERMINATED;
            break;

        case DLG_EVENT_NULL:
            break;

        case DLG_EVENT_UPDATE:
            if (pSipDlg->state != DLG_CONFIRMED)
            {
                SIP_DEBUG_TRACE(LOG_DEBUG, "update_dialog_as_uac2() exit---: SIP Dialog State Error \r\n");
                return -1;
            }

            if (MSG_IS_REQUEST(sip))
            {
                if (MSG_IS_INVITE(sip))
                {
                    pSipDlg->ict_tr = tr;
                    pSipDlg->local_cseq = osip_atoi(sip->cseq->number);  /* update local cseq number */
                }
                else if (MSG_IS_ACK(sip))
                {
                }
                else if (MSG_IS_SUBSCRIBE(sip))
                {
                    pSipDlg->nict_tr = tr;
                    pSipDlg->local_cseq = osip_atoi(sip->cseq->number);   /* update local cseq number */

                }
                else if (MSG_IS_REFER(sip))
                {
                    pSipDlg->nict_tr = tr;
                    pSipDlg->local_cseq = osip_atoi(sip->cseq->number);   /* update local cseq number */
                }
            }
            else
            {
                if (MSG_IS_STATUS_1XX(sip))
                {
                    break;
                }

                if (MSG_IS_STATUS_2XX(sip))
                {
                    if (MSG_IS_RESPONSE_FOR(sip, "INVITE"))
                    {
                        pSipDlg->ict_tr = NULL;
                    }
                    else
                    {
                        pSipDlg->nict_tr = NULL;
                    }

                    if (!MSG_IS_RESPONSE_FOR(sip, "INVITE")
                        && !MSG_IS_RESPONSE_FOR(sip, "SUBSCRIBE")
                        && !MSG_IS_RESPONSE_FOR(sip, "REFER"))
                    {
                        break;
                    }

                    sip_dialog_set_routeset(pSipDlg, &sip->record_routes);   /* update route set */

                    if (!osip_list_eol(&sip->contacts, 0))     /* update remote contact uri */
                    {
                        contact = (osip_contact_t*)osip_list_get(&sip->contacts, 0);

                        if (0 == url_compare(contact->url, sip->to->url)) /* 如果不一样，可能前端是通过NAT上来的,前端的contact就是to里面的，防止后面ack和bye消息发到私网 */
                        {
                            sip_dialog_set_remotecontact(pSipDlg, contact);
                        }
                        else
                        {
                            sip_dialog_set_remotecontact(pSipDlg, (osip_contact_t*)sip->to);
                        }
                    }
                }
                else
                {
                    if (MSG_IS_RESPONSE_FOR(sip, "INVITE"))
                    {
                        pSipDlg->ict_tr = NULL;
                    }
                    else
                    {
                        pSipDlg->nict_tr = NULL;
                    }
                }
            }

            break;
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : update_dialog_as_uas
 功能描述  : 作为UAS更新UA dialog
 输入参数  : int index
             transaction_t *tr
             sip_t *sip
             dialog_event_t event
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月2日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int update_dialog_as_uas(int index, osip_transaction_t* tr, osip_message_t* sip, dialog_event_t event)
{
    osip_uri_param_t* to_tag = NULL;
    osip_contact_t* contact = NULL;
    dialog_event_t evt = DLG_EVENT_NULL;
    sip_dialog_t* pSipDlg = NULL;

    if (!is_valid_dialog_index(index))
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "update_dialog_as_uas() exit---: Dialog Index Error \r\n");
        return -1;
    }

    pSipDlg = get_dialog_sip_dialog(index);

    if (NULL == pSipDlg)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "update_dialog_as_uas() exit---: Get SIP Dialog Error \r\n");
        return -1;
    }

    if (pSipDlg->state == DLG_TERMINATED) /* NOT need to update*/
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "update_dialog_as_uas() exit---: SIP Dialog State Error \r\n");
        return 0;
    }

    if (pSipDlg->state == DLG_CONFIRMED)
    {
        if (event == DLG_EVENT_1XXNOTAG
            || event == DLG_EVENT_1XXTAG
            || event == DLG_EVENT_2XX
            || event == DLG_EVENT_CANCELLED
            || event == DLG_EVENT_REJECTED
            || event == DLG_EVENT_ERROR
            || event == DLG_EVENT_TIMEOUT)
        {
            evt = DLG_EVENT_UPDATE;
        }
        else
        {
            evt = event;
        }
    }
    else
    {
        evt = event;
    }

    switch (evt)
    {
        case DLG_EVENT_1XXNOTAG:
            if (pSipDlg->state == DLG_TRYING)
            {
                pSipDlg->state = DLG_PROCEEDDING;
                sip_dialog_set_localuri(pSipDlg, sip->to); /* update local uri */
            }

            break;

        case DLG_EVENT_1XXTAG:
            if (pSipDlg->state == DLG_TRYING
                || pSipDlg->state == DLG_PROCEEDDING)
            {
                pSipDlg->state = DLG_EARLY;

                if (NULL != sip->to)
                {
                    osip_to_get_tag(sip->to, &to_tag);

                    if (to_tag)
                    {
                        sip_dialog_set_localtag(pSipDlg, to_tag->gvalue);   /* update local tag */
                    }

                    sip_dialog_set_localuri(pSipDlg, sip->to); /* update local uri */
                }
            }

            break;

        case DLG_EVENT_2XX:
            if (pSipDlg->state == DLG_TRYING
                || pSipDlg->state == DLG_PROCEEDDING
                || pSipDlg->state == DLG_EARLY)
            {
                pSipDlg->state = DLG_CONFIRMED;

                if (NULL != sip->to)
                {
                    osip_to_get_tag(sip->to, &to_tag);

                    if (to_tag)
                    {
                        sip_dialog_set_localtag(pSipDlg, to_tag->gvalue);   /* update local tag */
                    }

                    sip_dialog_set_localuri(pSipDlg, sip->to);          /* update local uri */
                }

                sip_dialog_set_routeset(pSipDlg, &sip->record_routes);  /* update route set */

                if (pSipDlg->type2 != DLG_INVITE)
                {
                    pSipDlg->nist_tr = NULL;
                    break;
                }

                pSipDlg->ist_tr = NULL;
            }

            break;

        case DLG_EVENT_CANCELLED:
            pSipDlg->state = DLG_TERMINATED;

            if (pSipDlg->type2 != DLG_INVITE)
            {
                break;
            }

            break;

        case DLG_EVENT_REJECTED:
        case DLG_EVENT_ERROR:
        case DLG_EVENT_TIMEOUT:
            pSipDlg->state = DLG_TERMINATED;

            if (pSipDlg->type2 != DLG_INVITE)
            {
                break;
            }

            break;

        case DLG_EVENT_REPLACED:
            pSipDlg->state = DLG_TERMINATED;
            break;

        case DLG_EVENT_LOCALBYE:
            pSipDlg->state = DLG_TERMINATED;

            if (pSipDlg->type2 != DLG_INVITE)
            {
                break;
            }

            break;

        case DLG_EVENT_REMOTEBYE:
            pSipDlg->state = DLG_TERMINATED;

            if (pSipDlg->type2 != DLG_INVITE)
            {
                break;
            }

            break;

        case DLG_EVENT_NULL:
            break;

        case DLG_EVENT_UPDATE:
            if (pSipDlg->state != DLG_CONFIRMED)
            {
                SIP_DEBUG_TRACE(LOG_DEBUG, "update_dialog_as_uas() exit---: SIP Dialog State Error \r\n");
                return -1;
            }

            if (MSG_IS_REQUEST(sip))
            {
                if (MSG_IS_INVITE(sip))
                {
                    pSipDlg->ist_tr = tr;
                    pSipDlg->remote_cseq = osip_atoi(sip->cseq->number);  /* update remote cseq number */
                }
                else if (MSG_IS_SUBSCRIBE(sip))
                {
                    pSipDlg->nist_tr = tr;
                    pSipDlg->remote_cseq = osip_atoi(sip->cseq->number);  /* update remote cseq number */
                }
                else if (MSG_IS_REFER(sip))
                {
                    pSipDlg->nist_tr = tr;
                    pSipDlg->remote_cseq = osip_atoi(sip->cseq->number);  /* update remote cseq number */
                }
                else if (MSG_IS_OPTIONS(sip))
                {
                    pSipDlg->nist_tr = tr;
                    pSipDlg->remote_cseq = osip_atoi(sip->cseq->number);  /* update remote cseq number */
                }
                else if (MSG_IS_ACK(sip))
                {
                }
            }
            else
            {
                if (MSG_IS_STATUS_1XX(sip))
                {
                    break;
                }

                if (MSG_IS_STATUS_2XX(sip))
                {
                    if (MSG_IS_RESPONSE_FOR(sip, "INVITE"))
                    {
                        pSipDlg->ist_tr = NULL;
                    }
                    else
                    {
                        pSipDlg->nist_tr = NULL;
                    }

                    if (!MSG_IS_RESPONSE_FOR(sip, "INVITE")
                        && !MSG_IS_RESPONSE_FOR(sip, "SUBSCRIBE")
                        && !MSG_IS_RESPONSE_FOR(sip, "REFER"))
                    {
                        break;
                    }

                    sip_dialog_set_routeset(pSipDlg, &sip->record_routes);  /* update route set */

                    if (!osip_list_eol(&tr->orig_request->contacts, 0))      /* update remote contact uri */
                    {
                        contact = (osip_contact_t*)osip_list_get(&tr->orig_request->contacts, 0);

                        if (0 == url_compare(contact->url, tr->orig_request->to->url)) /* 如果不一样，可能前端是通过NAT上来的,前端的contact就是to里面的，防止后面ack和bye消息发到私网 */
                        {
                            sip_dialog_set_remotecontact(pSipDlg, contact);
                        }
                        else
                        {
                            sip_dialog_set_remotecontact(pSipDlg, (osip_contact_t*)tr->orig_request->to);
                        }
                    }
                }
                else
                {
                    if (MSG_IS_RESPONSE_FOR(sip, "INVITE"))
                    {
                        pSipDlg->ist_tr = NULL;
                    }
                    else
                    {
                        pSipDlg->nist_tr = NULL;
                    }
                }
            }

            break;
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : update_dialog_as_uas
 功能描述  : 作为UAS更新UA dialog
 输入参数  : sip_dialog_t* pSipDlg
             transaction_t *tr
             sip_t *sip
             dialog_event_t event
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月2日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int update_dialog_as_uas2(sip_dialog_t* pSipDlg, osip_transaction_t* tr, osip_message_t* sip, dialog_event_t event)
{
    osip_uri_param_t* to_tag = NULL;
    osip_contact_t* contact = NULL;
    dialog_event_t evt = DLG_EVENT_NULL;

    if (NULL == pSipDlg)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "update_dialog_as_uas2() exit---: SIP Dialog Error \r\n");
        return -1;
    }

    if (pSipDlg->state == DLG_TERMINATED) /* NOT need to update*/
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "update_dialog_as_uas2() exit---: SIP Dialog State Error \r\n");
        return 0;
    }

    if (pSipDlg->state == DLG_CONFIRMED)
    {
        if (event == DLG_EVENT_1XXNOTAG
            || event == DLG_EVENT_1XXTAG
            || event == DLG_EVENT_2XX
            || event == DLG_EVENT_CANCELLED
            || event == DLG_EVENT_REJECTED
            || event == DLG_EVENT_ERROR
            || event == DLG_EVENT_TIMEOUT)
        {
            evt = DLG_EVENT_UPDATE;
        }
        else
        {
            evt = event;
        }
    }
    else
    {
        evt = event;
    }

    switch (evt)
    {
        case DLG_EVENT_1XXNOTAG:
            if (pSipDlg->state == DLG_TRYING)
            {
                pSipDlg->state = DLG_PROCEEDDING;
                sip_dialog_set_localuri(pSipDlg, sip->to); /* update local uri */
            }

            break;

        case DLG_EVENT_1XXTAG:
            if (pSipDlg->state == DLG_TRYING
                || pSipDlg->state == DLG_PROCEEDDING)
            {
                pSipDlg->state = DLG_EARLY;

                if (NULL != sip->to)
                {
                    osip_to_get_tag(sip->to, &to_tag);

                    if (to_tag)
                    {
                        sip_dialog_set_localtag(pSipDlg, to_tag->gvalue);   /* update local tag */
                    }

                    sip_dialog_set_localuri(pSipDlg, sip->to); /* update local uri */
                }
            }

            break;

        case DLG_EVENT_2XX:
            if (pSipDlg->state == DLG_TRYING
                || pSipDlg->state == DLG_PROCEEDDING
                || pSipDlg->state == DLG_EARLY)
            {
                pSipDlg->state = DLG_CONFIRMED;

                if (NULL != sip->to)
                {
                    osip_to_get_tag(sip->to, &to_tag);

                    if (to_tag)
                    {
                        sip_dialog_set_localtag(pSipDlg, to_tag->gvalue);   /* update local tag */
                    }

                    sip_dialog_set_localuri(pSipDlg, sip->to);          /* update local uri */
                }

                sip_dialog_set_routeset(pSipDlg, &sip->record_routes);  /* update route set */

                if (pSipDlg->type2 != DLG_INVITE)
                {
                    pSipDlg->nist_tr = NULL;
                    break;
                }

                pSipDlg->ist_tr = NULL;
            }

            break;

        case DLG_EVENT_CANCELLED:
            pSipDlg->state = DLG_TERMINATED;

            if (pSipDlg->type2 != DLG_INVITE)
            {
                break;
            }

            break;

        case DLG_EVENT_REJECTED:
        case DLG_EVENT_ERROR:
        case DLG_EVENT_TIMEOUT:
            pSipDlg->state = DLG_TERMINATED;

            if (pSipDlg->type2 != DLG_INVITE)
            {
                break;
            }

            break;

        case DLG_EVENT_REPLACED:
            pSipDlg->state = DLG_TERMINATED;
            break;

        case DLG_EVENT_LOCALBYE:
            pSipDlg->state = DLG_TERMINATED;

            if (pSipDlg->type2 != DLG_INVITE)
            {
                break;
            }

            break;

        case DLG_EVENT_REMOTEBYE:
            pSipDlg->state = DLG_TERMINATED;

            if (pSipDlg->type2 != DLG_INVITE)
            {
                break;
            }

            break;

        case DLG_EVENT_NULL:
            break;

        case DLG_EVENT_UPDATE:
            if (pSipDlg->state != DLG_CONFIRMED)
            {
                SIP_DEBUG_TRACE(LOG_DEBUG, "update_dialog_as_uas() exit---: SIP Dialog State Error \r\n");
                return -1;
            }

            if (MSG_IS_REQUEST(sip))
            {
                if (MSG_IS_INVITE(sip))
                {
                    pSipDlg->ist_tr = tr;
                    pSipDlg->remote_cseq = osip_atoi(sip->cseq->number);  /* update remote cseq number */
                }
                else if (MSG_IS_SUBSCRIBE(sip))
                {
                    pSipDlg->nist_tr = tr;
                    pSipDlg->remote_cseq = osip_atoi(sip->cseq->number);  /* update remote cseq number */
                }
                else if (MSG_IS_REFER(sip))
                {
                    pSipDlg->nist_tr = tr;
                    pSipDlg->remote_cseq = osip_atoi(sip->cseq->number);  /* update remote cseq number */
                }
                else if (MSG_IS_OPTIONS(sip))
                {
                    pSipDlg->nist_tr = tr;
                    pSipDlg->remote_cseq = osip_atoi(sip->cseq->number);  /* update remote cseq number */
                }
                else if (MSG_IS_ACK(sip))
                {
                }
            }
            else
            {
                if (MSG_IS_STATUS_1XX(sip))
                {
                    break;
                }

                if (MSG_IS_STATUS_2XX(sip))
                {
                    if (MSG_IS_RESPONSE_FOR(sip, "INVITE"))
                    {
                        pSipDlg->ist_tr = NULL;
                    }
                    else
                    {
                        pSipDlg->nist_tr = NULL;
                    }

                    if (!MSG_IS_RESPONSE_FOR(sip, "INVITE")
                        && !MSG_IS_RESPONSE_FOR(sip, "SUBSCRIBE")
                        && !MSG_IS_RESPONSE_FOR(sip, "REFER"))
                    {
                        break;
                    }

                    sip_dialog_set_routeset(pSipDlg, &sip->record_routes);  /* update route set */

                    if (!osip_list_eol(&tr->orig_request->contacts, 0))      /* update remote contact uri */
                    {
                        contact = (osip_contact_t*)osip_list_get(&tr->orig_request->contacts, 0);

                        if (0 == url_compare(contact->url, tr->orig_request->to->url)) /* 如果不一样，可能前端是通过NAT上来的,前端的contact就是to里面的，防止后面ack和bye消息发到私网 */
                        {
                            sip_dialog_set_remotecontact(pSipDlg, contact);
                        }
                        else
                        {
                            sip_dialog_set_remotecontact(pSipDlg, (osip_contact_t*)tr->orig_request->to);
                        }
                    }
                }
                else
                {
                    if (MSG_IS_RESPONSE_FOR(sip, "INVITE"))
                    {
                        pSipDlg->ist_tr = NULL;
                    }
                    else
                    {
                        pSipDlg->nist_tr = NULL;
                    }
                }
            }

            break;
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : set_dialog_ui_state
 功能描述  : 设置UA dialog的UI状态
 输入参数  : int index
             ui_state_t state
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月2日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int set_dialog_ui_state(int index,  ui_state_t state)
{
    ua_dialog_t* pUaDialog = NULL;

    if (!is_valid_dialog_index(index))
    {
        //SIP_DEBUG_TRACE(LOG_DEBUG, "set_dialog_ui_state() exit---: Dialog Index Error \r\n");
        return -1;
    }


    USED_UA_SMUTEX_LOCK();

    pUaDialog = ua_dialog_get2(index);

    if (NULL == pUaDialog)
    {
        USED_UA_SMUTEX_UNLOCK();
        return -1;
    }

    pUaDialog->eUiState = state;
    USED_UA_SMUTEX_UNLOCK();
    return 0;
}

/*****************************************************************************
 函 数 名  : get_dialog_ui_state
 功能描述  : 获取UA dialog的UI状态
 输入参数  : int index
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月2日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
ui_state_t  get_dialog_ui_state(int index)
{
    ua_dialog_t* pUaDialog = NULL;

    if (!is_valid_dialog_index(index))
    {
        //SIP_DEBUG_TRACE(LOG_DEBUG, "get_dialog_ui_state() exit---: Dialog Index Error \r\n");
        return (ui_state_t)0;
    }

    USED_UA_SMUTEX_LOCK();

    pUaDialog = ua_dialog_get2(index);

    if (NULL == pUaDialog)
    {
        USED_UA_SMUTEX_UNLOCK();
        return (ui_state_t)0;
    }

    USED_UA_SMUTEX_UNLOCK();
    return pUaDialog->eUiState;
}

/*****************************************************************************
 函 数 名  : get_dialog_sip_dialog
 功能描述  : 获取UA dialog中的sip dialog
 输入参数  : int index
 输出参数  : 无
 返 回 值  : sip_dialog_t
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月2日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
sip_dialog_t* get_dialog_sip_dialog(int index)
{
    ua_dialog_t* pUaDialog = NULL;

    if (!is_valid_dialog_index(index))
    {
        //SIP_DEBUG_TRACE(LOG_DEBUG, "get_dialog_sip_dialog() exit---: Dialog Index Error \r\n");
        return NULL;
    }

    USED_UA_SMUTEX_LOCK();

    pUaDialog = ua_dialog_get2(index);

    if (NULL == pUaDialog)
    {
        USED_UA_SMUTEX_UNLOCK();
        return NULL;
    }

    USED_UA_SMUTEX_UNLOCK();
    return pUaDialog->pSipDialog;
}

/*****************************************************************************
 函 数 名  : get_dialog_sip_dialog
 功能描述  : 获取UA dialog中的sip dialog
 输入参数  : int index
 输出参数  : 无
 返 回 值  : sip_dialog_t
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月2日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
sip_dialog_t* get_dialog_sip_dialog2(int index)
{
    ua_dialog_t* pUaDialog = NULL;

    if (!is_valid_dialog_index(index))
    {
        //SIP_DEBUG_TRACE(LOG_DEBUG, "get_dialog_sip_dialog2() exit---: Dialog Index Error \r\n");
        return NULL;
    }

    pUaDialog = ua_dialog_get2(index);

    if (NULL == pUaDialog)
    {
        return NULL;
    }

    return pUaDialog->pSipDialog;
}

/*****************************************************************************
 函 数 名  : scan_ua_dialogs
 功能描述  : 循环扫描ua会话dialog
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年5月31日 星期五
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void scan_ua_dialogs()
{
    int i = 0;
    int index = -1;
    used_UA_Dialog_Iterator Itr;
    ua_dialog_t* pUaDialog = NULL;
    sip_subscription_t* head = NULL;
    vector<int> UAIndexVector;
    vector<int> SubUAIndexVector;
    time_t now;

    now = time(NULL);

    UAIndexVector.clear();
    SubUAIndexVector.clear();

    USED_UA_SMUTEX_LOCK();

    if (g_UsedUaDialogQueue.size() <= 0)
    {
        USED_UA_SMUTEX_UNLOCK();
        return;
    }

    for (Itr = g_UsedUaDialogQueue.begin(); Itr != g_UsedUaDialogQueue.end();)
    {
        index = *Itr;

        if (!is_valid_dialog_index(index))
        {
            Itr = g_UsedUaDialogQueue.erase(Itr);
            continue;
        }

        pUaDialog = g_UaDialogMap[index];

        if (NULL == pUaDialog)
        {
            Itr++;
            continue;
        }

        if (NULL != pUaDialog->subscription)
        {
            head = pUaDialog->subscription;

            if ((now - head->begin) >= head->duration)
            {
#if 0

                //超时发notify
                if (!SipNotify(i, node, "closed"))
                {
                    node->state = SUB_STATE_TERMINATED;
                }
                else
                {
                    node->state = SUB_STATE_CLEAR;
                }

#endif
                head->state = SUB_STATE_CLEAR;
            }

            if (head->state == SUB_STATE_CLEAR)
            {
                sip_subscription_free(head);
                osip_free(head);
                head = NULL;

                pUaDialog->subscription = NULL;

                SubUAIndexVector.push_back(index);
            }
        }

        if (pUaDialog->subscription == NULL) /* check is need to remove the dialog */
        {
            if (NULL != pUaDialog->pSipDialog) /* check is need to remove the dialog */
            {
                if (pUaDialog->pSipDialog->type2 != DLG_INVITE)
                {
                    pUaDialog->pSipDialog->state = DLG_TERMINATED;
                }
            }
        }

        if (NULL != pUaDialog->pSipDialog) /* check is need to remove the dialog */
        {
            if (pUaDialog->pSipDialog->state == DLG_TERMINATED)
            {
                Itr = g_UsedUaDialogQueue.erase(Itr);

                //ua_dialog_free(pUaDialog);
                UAIndexVector.push_back(index);
                //SIP_DEBUG_TRACE(LOG_DEBUG, "scan_ua_dialogs() ua_dialog_free: index=%d, g_UsedUaDialogQueue.size()=%d \r\n", index, g_UsedUaDialogQueue.size());

                continue;
            }
        }

        Itr++;
    }

    USED_UA_SMUTEX_UNLOCK();

    if (SubUAIndexVector.size() > 0)
    {
        for (i = 0; i < (int)SubUAIndexVector.size(); i++)
        {
            index = SubUAIndexVector[i];
            SIP_DEBUG_TRACE(LOG_INFO, "scan_ua_dialogs() SUBSCRIBE Dialog Time Out Free: index=%d \r\n", index);

            pUaDialog = ua_dialog_get(index);

            if (NULL != pUaDialog)
            {
                /* 调用钩子函数 */
                if (NULL != g_AppCallback && NULL != g_AppCallback->subscribe_within_dialog_received_cb)
                {
                    g_AppCallback->subscribe_within_dialog_received_cb(pUaDialog->strCallerID, pUaDialog->strRemoteIP, pUaDialog->iRemotePort, pUaDialog->strCalleeID, (char*)"timeout", index, 0, NULL, 0);
                }
            }
        }

        SubUAIndexVector.clear();
    }

    if (UAIndexVector.size() > 0)
    {
        for (i = 0; i < (int)UAIndexVector.size(); i++)
        {
            index = UAIndexVector[i];

            ua_dialog_remove(index);
            SIP_DEBUG_TRACE(LOG_INFO, "scan_ua_dialogs() ua_dialog_remove: index=%d \r\n", index);
        }

        UAIndexVector.clear();
    }

    return;
}

/*****************************************************************************
 函 数 名  : sip_get_all_used_sipua_index
 功能描述  : 获取所有的当前使用的sip会话句柄
 输入参数  : vector<int>& SIPUAIndexVector
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年11月21日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int SIP_GetAllUsedSIPUAIndex(vector<int>& SIPUAIndexVector)
{
    int index = -1;
    used_UA_Dialog_Iterator Itr;

    SIPUAIndexVector.clear();

    USED_UA_SMUTEX_LOCK();

    if (g_UsedUaDialogQueue.size() <= 0)
    {
        USED_UA_SMUTEX_UNLOCK();
        return -1;
    }

    for (Itr = g_UsedUaDialogQueue.begin(); Itr != g_UsedUaDialogQueue.end();)
    {
        index = *Itr;

        if (!is_valid_dialog_index(index))
        {
            Itr = g_UsedUaDialogQueue.erase(Itr);
            continue;
        }

        SIPUAIndexVector.push_back(index);

        Itr++;
    }

    USED_UA_SMUTEX_UNLOCK();

    return 0;
}

/*****************************************************************************
 函 数 名  : SIP_ReleaseUnUsedSIPUA
 功能描述  : 释放没有使用的SIPUA
 输入参数  : int index
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年11月21日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int SIP_ReleaseUnUsedSIPUA(int index)
{
    ua_dialog_t* pUaDialog = NULL;

    if (!is_valid_dialog_index(index))
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "SIP_ReleaseUnUsedSIPUA() exit---: Dialog Index Error \r\n");
        return -1;
    }

    pUaDialog = ua_dialog_get(index);

    if (NULL == pUaDialog)
    {
        return -1;
    }

    if (NULL == pUaDialog->pSipDialog)
    {
        return -1;
    }

    if (DLG_INVITE == pUaDialog->pSipDialog->type2)
    {
        if (UI_STATE_CALL_RCV == pUaDialog->eUiState)
        {
            SIP_AnswerToInvite(index, 400, NULL);
        }
        else
        {
            SIP_SendBye(index);
        }
    }
    else if (DLG_SUBSCRIBE == pUaDialog->pSipDialog->type2)
    {
        pUaDialog->subscription->state = SUB_STATE_CLEAR;
    }

    return 0;
}
#endif

#if DECS("subscription Dialog操作")
int sip_subscription_init(sip_subscription_t ** sub)
{
    *sub = (sip_subscription_t *)smalloc(sizeof(sip_subscription_t));

    if (NULL == *sub)
    {
        return -1;
    }

    (*sub)->state = SUB_STATE_PRE;
    (*sub)->remote_contact_uri = NULL;
    (*sub)->event_type = NULL;
    (*sub)->sub_state = NULL;
    (*sub)->id_param = NULL;
    (*sub)->begin = 0;
    (*sub)->duration = 0;
    return 0;
}

void sip_subscription_free(sip_subscription_t * sub)
{
    if (NULL == sub)
    {
        return;
    }

    sub->state = SUB_STATE_PRE;

    if (NULL != sub->remote_contact_uri)
    {
        osip_uri_free(sub->remote_contact_uri);
        osip_free(sub->remote_contact_uri);
        sub->remote_contact_uri = NULL;
    }

    if (NULL != sub->event_type)
    {
        osip_free(sub->event_type);
        sub->event_type = NULL;
    }

    if (NULL != sub->sub_state)
    {
        osip_free(sub->sub_state);
        sub->sub_state = NULL;
    }

    if (NULL != sub->id_param)
    {
        osip_free(sub->id_param);
        sub->id_param = NULL;
    }

    sub->begin = 0;
    sub->duration = 0;
    return;
}

int sip_subscription_match(sip_subscription_t * sub, char *event_type, char *id)
{
    if (sub == NULL || event_type == NULL)
    {
        return -1;
    }

    if (strcmp(sub->event_type, event_type) != 0)
    {
        return -1;
    }

    if (sub->id_param != NULL && id == NULL)
    {
        return -1;
    }

    if (sub->id_param == NULL && id != NULL)
    {
        return -1;
    }

    if (sub->id_param != NULL && id != NULL)
    {
        if (strcmp(sub->id_param, id) != 0)
        {
            return -1;
        }
    }

    return 0;
}

sip_subscription_t *GetDialogSubscription(int index)
{
    ua_dialog_t* pUaDialog = NULL;

    if (!is_valid_dialog_index(index))
    {
        //SIP_DEBUG_TRACE(LOG_DEBUG, "get_dialog_sip_dialog() exit---: Dialog Index Error \r\n");
        return NULL;
    }

    USED_UA_SMUTEX_LOCK();

    pUaDialog = ua_dialog_get2(index);

    if (NULL == pUaDialog)
    {
        USED_UA_SMUTEX_UNLOCK();
        return NULL;
    }

    USED_UA_SMUTEX_UNLOCK();
    return pUaDialog->subscription;
}

int AddDialogSubscription(int index, sip_subscription_t * sip_sub)
{
    ua_dialog_t* pUaDialog = NULL;

    if (!is_valid_dialog_index(index))
    {
        //SIP_DEBUG_TRACE(LOG_DEBUG, "get_dialog_sip_dialog() exit---: Dialog Index Error \r\n");
        return NULL;
    }

    USED_UA_SMUTEX_LOCK();

    pUaDialog = ua_dialog_get2(index);

    if (NULL == pUaDialog)
    {
        USED_UA_SMUTEX_UNLOCK();
        return NULL;
    }

    pUaDialog->subscription = sip_sub;

    USED_UA_SMUTEX_UNLOCK();

    return 0;
}

sip_subscription_t * SearchSubscription(int dialog_index, char *event_type, char *id_param)
{
    int i = 0;
    sip_subscription_t *sip_sub = NULL;

    sip_sub = GetDialogSubscription(dialog_index);

    if (sip_sub == NULL)
    {
        return NULL;
    }

    if (NULL == event_type)
    {
        return NULL;
    }

    i = sip_subscription_match(sip_sub, event_type, id_param);

    if (i == 0)
    {
        return sip_sub;
    }

    return NULL;
}

int UpdateSubscription(sip_subscription_t * sip_sub , subscription_state state)
{
    if (sip_sub == NULL)
    {
        return -1;
    }

    sip_sub->state = state;
    return 0;
}
#endif

#if DECS("sip Dialog操作")
/*****************************************************************************
 函 数 名  : sip_dialog_init_as_uac
 功能描述  : sip dialog初始化为UAC
 输入参数  : sip_dialog_t ** dialog
             transaction_t *tr
             sip_t *sip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月2日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int sip_dialog_init_as_uac(sip_dialog_t** dialog, osip_transaction_t* tr, osip_message_t* sip)
{
    int i = 0;
    osip_generic_param_t* tag = NULL;
    sip_dialog_t* dlg = NULL;

    *dialog = NULL;

    if (NULL == tr || NULL == sip)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_dialog_init_as_uac() exit---: Param Error \r\n");
        return -1;
    }

    dlg = (sip_dialog_t*)osip_malloc(sizeof(sip_dialog_t));

    if (NULL == dlg)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_dialog_init_as_uac() exit---: dlg Smalloc Error \r\n");
        return -1;
    }

    dlg->state = DLG_TRYING;
    dlg->call_id = NULL;
    dlg->local_tag = NULL;
    dlg->remote_tag = NULL;
    dlg->route_set = NULL;
    dlg->local_cseq = 0;
    dlg->remote_cseq = -1;
    dlg->local_uri = NULL;
    dlg->remote_uri = NULL;
    dlg->remote_contact_uri = NULL;
    //dlg->local_sdp = NULL;
    //dlg->remote_sdp = NULL;
    dlg->ict_tr = NULL;
    dlg->ist_tr = NULL;
    dlg->nict_tr = NULL;
    dlg->nist_tr = NULL;

    i = osip_call_id_to_str(sip->call_id, &dlg->call_id); /*set call id */

    if (NULL != sip->from)
    {
        i = osip_from_get_tag(sip->from,  &tag);           /*set local tag */

        if (0 == i)
        {
            dlg->local_tag = osip_getcopy(tag->gvalue);
        }
    }

    i = osip_from_clone(sip->from, &dlg->local_uri);   /* set local  uri */

    if (0 != i)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_dialog_init_as_uac() exit---: From Clone Error \r\n");
        goto error;
    }

    i = osip_to_clone(sip->to, &dlg->remote_uri);       /*  set remote uri */

    if (0 != i)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_dialog_init_as_uac() exit---: To Clone Error \r\n");
        goto error;
    }

    dlg->local_cseq = osip_atoi(sip->cseq->number);   /* set local cseq number*/

    /* dialog type and state*/
    dlg->type1 = DLG_CALLER;

    if (MSG_IS_INVITE(sip))
    {
        dlg->type2 = DLG_INVITE;
        dlg->ict_tr = tr;
    }
    else if (MSG_IS_SUBSCRIBE(sip))
    {
        dlg->type2 = DLG_SUBSCRIBE;
        dlg->nict_tr = tr;
    }
    else if (MSG_IS_REFER(sip))
    {
        dlg->type2 = DLG_REFER;
        /*check refer-to header */
        /*header_clone( )       */
        dlg->nict_tr = tr;
    }

    *dialog = dlg;
    return 0;
error:
    sip_dialog_free(dlg);
    osip_free(dlg);
    dlg = NULL;
    return -1;
}

/*****************************************************************************
 函 数 名  : sip_dialog_init_as_uas
 功能描述  : sip dialog初始化为UAS
 输入参数  : sip_dialog_t ** dialog
             transaction_t *tr
             sip_t *sip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月2日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int sip_dialog_init_as_uas(sip_dialog_t** dialog, osip_transaction_t* tr, osip_message_t* sip)
{
    int i = 0;
    osip_generic_param_t* tag = NULL;
    osip_contact_t* contact = NULL;
    sip_dialog_t* dlg = NULL;

    *dialog = NULL;

    if (NULL == tr || NULL == sip)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_dialog_init_as_uas() exit---: Param Error \r\n");
        return -1;
    }

    dlg = (sip_dialog_t*)osip_malloc(sizeof(sip_dialog_t));

    if (NULL == dlg)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_dialog_init_as_uas() exit---: dlg Smalloc Error \r\n");
        return -1;
    }

    dlg->state = DLG_TRYING;
    dlg->call_id = NULL;
    dlg->local_tag = NULL;
    dlg->remote_tag = NULL;
    dlg->route_set = NULL;
    dlg->local_cseq = 0;
    dlg->remote_cseq = -1;
    dlg->local_uri = NULL;
    dlg->remote_uri = NULL;
    dlg->remote_contact_uri = NULL;
    //dlg->local_sdp = NULL;
    //dlg->remote_sdp = NULL;
    dlg->ict_tr = NULL;
    dlg->ist_tr = NULL;
    dlg->nict_tr = NULL;
    dlg->nist_tr = NULL;

    i = osip_call_id_to_str(osip_message_get_call_id(sip), &dlg->call_id);  /* set callid */

    if (NULL != sip->from)
    {
        i = osip_from_get_tag(sip->from, &tag);   /* set remote tag */

        if (i == 0)
        {
            dlg->remote_tag = osip_getcopy(tag->gvalue);
        }
    }

    dlg->remote_cseq = osip_atoi(sip->cseq->number);   /* set remote cseq number*/

    i = osip_from_clone(sip->from, &dlg->remote_uri);   /* set remote uri */

    if (0 != i)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_dialog_init_as_uas() exit---: From Clone Error \r\n");
        goto error;
    }

    i = osip_to_clone(sip->to, &dlg->local_uri);        /*  set local uri */

    if (0 != i)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_dialog_init_as_uas() exit---: To Clone Error \r\n");
        goto error;
    }

    if (!osip_list_eol(&sip->contacts, 0))   /* set remote contact uri */
    {
        contact = (osip_contact_t*)osip_list_get(&sip->contacts, 0);
        i = osip_contact_clone(contact, &dlg->remote_contact_uri);

        if (0 != i)
        {
            SIP_DEBUG_TRACE(LOG_DEBUG, "sip_dialog_init_as_uas() exit---: Contact Clone Error \r\n");
            goto error;
        }
    }

    /* dialog type and state*/
    dlg->type1 = DLG_CALLEE;

    if (MSG_IS_INVITE(sip))
    {
        dlg->type2 = DLG_INVITE;
        dlg->ist_tr = tr;
    }
    else if (MSG_IS_SUBSCRIBE(sip))
    {
        dlg->type2 = DLG_SUBSCRIBE;
        dlg->nist_tr = tr;
    }
    else if (MSG_IS_REFER(sip))
    {
        dlg->type2 = DLG_REFER;
        /*check refer-to header */
        /*header_clone( )       */
        dlg->nist_tr = tr;
    }

    *dialog = dlg;
    return 0;
error:
    sip_dialog_free(dlg);
    osip_free(dlg);
    dlg = NULL;
    return -1;
}

/*****************************************************************************
 函 数 名  : sip_dialog_free
 功能描述  : sip dialog结构释放
 输入参数  : sip_dialog_t * dialog
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月2日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void sip_dialog_free(sip_dialog_t* dialog)
{
    if (NULL == dialog)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_dialog_free() exit---: Param Error \r\n");
        return;
    }

    if (NULL != dialog->call_id)
    {
        osip_free(dialog->call_id);
        dialog->call_id = NULL;
    }

    if (NULL != dialog->local_tag)
    {
        osip_free(dialog->local_tag);
        dialog->local_tag = NULL;
    }

    if (NULL != dialog->remote_tag)
    {
        osip_free(dialog->remote_tag);
        dialog->remote_tag = NULL;
    }

    if (NULL != dialog->route_set)
    {
        osip_list_special_free(dialog->route_set, (void (*)(void*))&osip_route_free);
        osip_free(dialog->route_set);
        dialog->route_set = NULL;
    }

    if (NULL != dialog->local_uri)
    {
        osip_from_free(dialog->local_uri);
        dialog->local_uri = NULL;
    }

    if (NULL != dialog->remote_uri)
    {
        osip_to_free(dialog->remote_uri);
        dialog->remote_uri = NULL;
    }

    if (NULL != dialog->remote_contact_uri)
    {
        osip_contact_free(dialog->remote_contact_uri);
        dialog->remote_contact_uri = NULL;
    }

    dialog->ict_tr = NULL;
    dialog->ist_tr = NULL;
    dialog->nict_tr = NULL;
    dialog->nist_tr = NULL;

    return;
}

/*****************************************************************************
 函 数 名  : sip_dialog_match_as_uac
 功能描述  : sip dialog作为uac匹配
 输入参数  : sip_dialog_t * dlg
             sip_t *answer
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月2日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int  sip_dialog_match_as_uac(sip_dialog_t* dlg, osip_message_t* answer)
{
    osip_generic_param_t* tag_param_local = NULL;
    osip_generic_param_t* tag_param_remote = NULL;
    char* tmp = NULL;
    int i = 0;

    if (NULL == answer)
    {
        return -1;
    }

    osip_call_id_to_str(answer->call_id, &tmp);

    if (0 != sstrcmp(dlg->call_id, tmp))
    {
        osip_free(tmp);
        tmp = NULL;
        //SIP_DEBUG_TRACE(LOG_DEBUG, "sip_dialog_match_as_uac() exit---: Call ID Not Equal \r\n");
        return -1;
    }

    osip_free(tmp);
    tmp = NULL;

    /* for INCOMING RESPONSE:
    To: remote_uri;remote_tag
    From: local_uri;local_tag           <- LOCAL TAG ALWAYS EXIST
    */

    if (NULL != answer->from)
    {
        i = osip_from_get_tag(answer->from, &tag_param_local);

        if (0 != i)
        {
            //SIP_DEBUG_TRACE(LOG_DEBUG, "sip_dialog_match_as_uac() exit---: Get From Tag Error \r\n");
            return -1;
        }
    }

    if (dlg->local_tag == NULL)
    {
        /* NOT POSSIBLE BECAUSE I MANAGE REMOTE_TAG AND I ALWAYS ADD IT! */
        //SIP_DEBUG_TRACE(LOG_DEBUG, "sip_dialog_match_as_uac() exit---: Dlg Local Tag Error \r\n");
        return -1;
    }

    if (0 != sstrcmp(tag_param_local->gvalue, dlg->local_tag))
    {
        //SIP_DEBUG_TRACE(LOG_DEBUG, "sip_dialog_match_as_uac() exit---: Local Tag Not Equal \r\n");
        return -1;
    }

    if (dlg->state != DLG_CONFIRMED)
    {
        if (0 == url_compare(dlg->local_uri->url, answer->from->url)
            && 0 == url_compare(dlg->remote_uri->url, answer->to->url))
        {
            //SIP_DEBUG_TRACE(LOG_DEBUG, "sip_dialog_match_as_uac() exit---: URL Equal \r\n");
            return 0;
        }

        //SIP_DEBUG_TRACE(LOG_DEBUG, "sip_dialog_match_as_uac() exit---: URL Not Equal \r\n");
        return -1;
    }

    if (NULL != answer->to)
    {
        i = osip_to_get_tag(answer->to, &tag_param_remote);

        if (0 != i && dlg->remote_tag != NULL)  /* no tag in response but tag in dialog */
        {
            //SIP_DEBUG_TRACE(LOG_DEBUG, "sip_dialog_match_as_uac() exit---: Get To Tag Error \r\n");
            return -1;                          /* impossible... */
        }
    }

    if (0 != i && dlg->remote_tag == NULL)  /* no tag in response AND no tag in dialog */
    {
        if (0 == osip_from_compare((osip_from_t*) dlg->local_uri, (osip_from_t*) answer->from)
            && 0 == osip_from_compare(dlg->remote_uri, answer->to))
        {
            //SIP_DEBUG_TRACE(LOG_DEBUG, "sip_dialog_match_as_uac() exit---: From URL Equal \r\n");
            return 0;
        }

        //SIP_DEBUG_TRACE(LOG_DEBUG, "sip_dialog_match_as_uac() exit---: From URL Not Equal \r\n");
        return -1;
    }

    if (i == 0 && dlg->remote_tag == NULL)
    {
        if (0 == osip_from_compare((osip_from_t*) dlg->local_uri, (osip_from_t*) answer->from)
            && 0 == osip_from_compare(dlg->remote_uri, answer->to))
        {
            //SIP_DEBUG_TRACE(LOG_DEBUG, "sip_dialog_match_as_uac() exit---: From URL Equal \r\n");
            return 0;
        }

        //SIP_DEBUG_TRACE(LOG_DEBUG, "sip_dialog_match_as_uac() exit---: From URL Not Equal \r\n");
        return -1;
    }

    /* we don't have to compare
    remote_uri with from
    && local_uri with to.    ----> we have both tag recognized, it's enough..
    */
    if (0 == sstrcmp(tag_param_remote->gvalue, dlg->remote_tag))
    {
        //SIP_DEBUG_TRACE(LOG_DEBUG, "sip_dialog_match_as_uac() exit---: Remote Tag Equal \r\n");
        return 0;
    }

    //SIP_DEBUG_TRACE(LOG_DEBUG, "sip_dialog_match_as_uac() exit---: No Equal \r\n");
    return -1;
}

/*****************************************************************************
 函 数 名  : sip_dialog_match_as_uas
 功能描述  : sip dialog作为uas匹配
 输入参数  : sip_dialog_t * dlg
             sip_t *request
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月2日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int  sip_dialog_match_as_uas(sip_dialog_t* dlg, osip_message_t* request)
{
    osip_generic_param_t* tag_param_remote = NULL;
    osip_generic_param_t* tag_param_local = NULL;
    int i = 0;
    char* tmp = NULL;

    if (NULL == request)
    {
        return -1;
    }

    osip_call_id_to_str(request->call_id, &tmp);

    if (0 != sstrcmp(dlg->call_id, tmp))
    {
        osip_free(tmp);
        tmp = NULL;
        //SIP_DEBUG_TRACE(LOG_DEBUG, "sip_dialog_match_as_uas() exit---: Call ID Not Equal \r\n");
        return -1;
    }

    osip_free(tmp);
    tmp = NULL;

    /* for INCOMING REQUEST:
    To: local_uri;local_tag           <- LOCAL TAG ALWAYS EXIST
    From: remote_uri;remote_tag
    */
    if (MSG_IS_CANCEL(request))
    {
        /* check the cseq number */
        if (NULL == dlg->ist_tr)
        {
            //SIP_DEBUG_TRACE(LOG_DEBUG, "sip_dialog_match_as_uas() exit---: Dlg IST Tr NULL \r\n");
            return -1;
        }

        tmp = osip_message_get_cseq(request)->number;
        i = osip_atoi(tmp);

        if (dlg->remote_cseq != i)
        {
            //SIP_DEBUG_TRACE(LOG_DEBUG, "sip_dialog_match_as_uas() exit---: Dlg Remote Cseq Not Equal \r\n");
            return -1;
        }
    }
    else
    {
        if (NULL == dlg->local_tag)
        {
            /* NOT POSSIBLE BECAUSE I MANAGE REMOTE_TAG AND I ALWAYS ADD IT! */
            //SIP_DEBUG_TRACE(LOG_DEBUG, "sip_dialog_match_as_uas() exit---: Dlg Local Tag NULL \r\n");
            return -1;
        }

        if (NULL != request->to)
        {
            osip_to_get_tag(request->to, &tag_param_local);

            if (NULL == tag_param_local)
            {
                //SIP_DEBUG_TRACE(LOG_DEBUG, "sip_dialog_match_as_uas() exit---: Get To Tag Error \r\n");
                return -1;
            }
        }

        if (0 != sstrcmp(tag_param_local->gvalue, dlg->local_tag))
        {
            //SIP_DEBUG_TRACE(LOG_DEBUG, "sip_dialog_match_as_uas() exit---: Local Tag Not Equal \r\n");
            return -1;
        }
    }

    if (NULL != request->from)
    {
        i = osip_from_get_tag(request->from, &tag_param_remote);

        if (0 != i && dlg->remote_tag != NULL)  /* no tag in request but tag in dialog */
        {
            //SIP_DEBUG_TRACE(LOG_DEBUG, "sip_dialog_match_as_uas() exit---: Get From Tag Error \r\n");
            return -1;          /* impossible... */
        }
    }

    if (0 != i && dlg->remote_tag == NULL)  /* no tag in request AND no tag in dialog */
    {
        if (0 == osip_from_compare((osip_from_t*) dlg->remote_uri, (osip_from_t*) request->from)
            && 0 == osip_from_compare(dlg->local_uri, request->to))
        {
            //SIP_DEBUG_TRACE(LOG_DEBUG, "sip_dialog_match_as_uas() exit---: From URL Equal \r\n");
            return 0;
        }

        //SIP_DEBUG_TRACE(LOG_DEBUG, "sip_dialog_match_as_uas() exit---: From URL Not Equal \r\n");
        return -1;
    }

    if (i == 0 && dlg->remote_tag == NULL)  /* tag in request AND no tag in dialog */
    {
        if (0 == osip_from_compare((osip_from_t*) dlg->remote_uri, (osip_from_t*) request->from)
            && 0 == osip_from_compare(dlg->local_uri, request->to))
        {
            //SIP_DEBUG_TRACE(LOG_DEBUG, "sip_dialog_match_as_uas() exit---: From URL Equal \r\n");
            return 0;
        }

        //SIP_DEBUG_TRACE(LOG_DEBUG, "sip_dialog_match_as_uas() exit---: From URL Not Equal \r\n");
        return -1;
    }

    /* we don't have to compare
    remote_uri with from
    && local_uri with to.    ----> we have both tag recognized, it's enough..
    */
    if (0 == sstrcmp(tag_param_remote->gvalue, dlg->remote_tag))
    {
        //SIP_DEBUG_TRACE(LOG_DEBUG, "sip_dialog_match_as_uas() exit---: Remote Tag Equal \r\n");
        return 0;
    }

    //SIP_DEBUG_TRACE(LOG_DEBUG, "sip_dialog_match_as_uas() exit---: Not Equal \r\n");
    return -1;
}

/*****************************************************************************
 函 数 名  : sip_dialog_set_routeset
 功能描述  : sip dialog 设置routeset
 输入参数  : sip_dialog_t * dialog
             list_t *rset
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月2日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int sip_dialog_set_routeset(sip_dialog_t* dialog, osip_list_t* rset)
{
    int i = 0, pos = -1;
    osip_record_route_t* rr = NULL;
    osip_record_route_t* rr2 = NULL;

    if (NULL == dialog)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_dialog_set_routeset() exit---: Param Error \r\n");
        return -1;
    }

    /*free old route set*/
    osip_list_special_free(dialog->route_set, (void (*)(void*))&osip_route_free);
    osip_free(dialog->route_set);
    dialog->route_set = NULL;

    pos = 0;

    while (!osip_list_eol(rset, pos))
    {
        if (0 == pos) /* first init list */
        {
            dialog->route_set = (osip_list_t*) osip_malloc(sizeof(osip_list_t));

            if (NULL == dialog->route_set)
            {
                SIP_DEBUG_TRACE(LOG_DEBUG, "sip_dialog_set_routeset() exit---: Route Set Smalloc Error \r\n");
                return -1;
            }

            osip_list_init(dialog->route_set);
        }

        rr = (osip_record_route_t*) osip_list_get(rset, pos);

        if (NULL == rr)
        {
            pos++;
            continue;
        }

        i = osip_record_route_clone(rr, &rr2);

        if (0 != i)
        {
            SIP_DEBUG_TRACE(LOG_DEBUG, "sip_dialog_set_routeset() exit---: Record Route Clone Error \r\n");
            return -1;
        }

        i = osip_list_add(dialog->route_set, rr2, -1);  /*add to list tail */

        if (i < 0)
        {
            osip_record_route_free(rr2);
            rr2 = NULL;
            SIP_DEBUG_TRACE(LOG_DEBUG, "sip_dialog_set_routeset() exit---: List Add Error \r\n");
            return -1;
        }

        pos++;
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : sip_dialog_set_localtag
 功能描述  : sip dialog 设置localtag
 输入参数  : sip_dialog_t * dialog
             char *tag
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月2日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int sip_dialog_set_localtag(sip_dialog_t* dialog, char* tag)
{
    if (NULL == dialog)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_dialog_set_localtag() exit---: Param Error \r\n");
        return -1;
    }

    /* free old tag */
    osip_free(dialog->local_tag);
    dialog->local_tag = NULL;
    dialog->local_tag = osip_getcopy(tag); /* set new tag */
    return 0;
}

/*****************************************************************************
 函 数 名  : sip_dialog_set_localuri
 功能描述  : sip dialog 设置localuri
 输入参数  : sip_dialog_t * dialog
             from_t *local
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月2日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int sip_dialog_set_localuri(sip_dialog_t* dialog, osip_from_t* local)
{
    int i = 0;
    osip_from_t* copy = NULL;

    if (NULL == dialog || NULL == local)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_dialog_set_localuri() exit---: Param Error \r\n");
        return -1;
    }

    i = osip_from_clone(local, &copy);

    if (0 != i)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_dialog_set_localuri() exit---: From Clone Error \r\n");
        return -1;
    }

    /* free old uri */
    osip_from_free(dialog->local_uri);
    dialog->local_uri = NULL;
    dialog->local_uri = copy; /* set new uri*/
    return 0;
}

/*****************************************************************************
 函 数 名  : sip_dialog_set_localcseq
 功能描述  : sip dialog 设置localcseq
 输入参数  : sip_dialog_t * dialog
             int cseq
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月2日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int sip_dialog_set_localcseq(sip_dialog_t* dialog, int cseq)
{
    if (NULL == dialog)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_dialog_set_localcseq() exit---: Param Error \r\n");
        return -1;
    }

    dialog->local_cseq = cseq;
    return 0;
}

/*****************************************************************************
 函 数 名  : sip_dialog_set_remotetag
 功能描述  : sip dialog 设置remotetag
 输入参数  : sip_dialog_t * dialog
             char *tag
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月2日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int sip_dialog_set_remotetag(sip_dialog_t* dialog, char* tag)
{
    if (NULL == dialog)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_dialog_set_remotetag() exit---: Param Error \r\n");
        return -1;
    }

    /* free old tag */
    osip_free(dialog->remote_tag);
    dialog->remote_tag = osip_getcopy(tag); /* set new tag */
    return 0;
}

/*****************************************************************************
 函 数 名  : sip_dialog_set_remoteuri
 功能描述  : sip dialog 设置remoteuri
 输入参数  : sip_dialog_t * dialog
             to_t *remote
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月2日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int sip_dialog_set_remoteuri(sip_dialog_t* dialog, osip_to_t* remote)
{
    int i = 0;
    osip_from_t* copy = NULL;

    if (NULL == dialog || NULL == remote)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_dialog_set_remoteuri() exit---: Param Error \r\n");
        return -1;
    }

    i = osip_to_clone(remote, &copy);

    if (0 != i)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_dialog_set_remoteuri() exit---: To Clone Error \r\n");
        return -1;
    }

    /* free old uri */
    osip_to_free(dialog->remote_uri);
    dialog->remote_uri = copy; /* set new uri*/
    return 0;
}

/*****************************************************************************
 函 数 名  : sip_dialog_set_remotecontact
 功能描述  : sip dialog 设置remotecontact
 输入参数  : sip_dialog_t * dialog
             contact_t *contact
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月2日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int sip_dialog_set_remotecontact(sip_dialog_t* dialog, osip_contact_t* contact)
{
    int i = 0;
    osip_contact_t* copy = NULL;

    if (NULL == dialog)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_dialog_set_remotecontact() exit---: Param Error \r\n");
        return -1;
    }

    i = osip_contact_clone(contact, &copy);

    if (0 != i)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_dialog_set_remotecontact() exit---: Contact Clone Error \r\n");
        return -1;
    }

    /* free old contact */
    osip_contact_free(dialog->remote_contact_uri);
    dialog->remote_contact_uri = copy; /* set new contact*/
    return 0;
}

/*****************************************************************************
 函 数 名  : sip_dialog_set_remotecseq
 功能描述  : sip dialog 设置remotecseq
 输入参数  : sip_dialog_t * dialog
             int cseq
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月2日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int sip_dialog_set_remotecseq(sip_dialog_t* dialog, int cseq)
{
    if (NULL == dialog)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_dialog_set_remotecseq() exit---: Param Error \r\n");
        return -1;
    }

    dialog->remote_cseq = cseq;
    return 0;
}
#endif

#if DECS("UAS检查操作")
/*****************************************************************************
 函 数 名  : uas_check8_2_2
 功能描述  : sip 8.2.2检查
 输入参数  : transaction_t *tr
             sip_t *sip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月2日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int uas_check8_2_2(osip_transaction_t* tr, osip_message_t* sip)
{
    osip_to_t* to = NULL;
    osip_uri_t* request_uri = NULL;
    char* scheme = NULL;
    osip_header_t* header = NULL;
    osip_message_t* response = NULL;
    //char *local_user = NULL;

    int i;

    /* 8.2.2: Header Inspection OK */
    /* 8.2.2.1: To and Request-URI */
    /*check to header */
    to = osip_message_get_to(sip);

    if (NULL == to)
    {
        /*RESPONSE 400 (bad request)*/
        sip_response_default(-1, tr, 400, NULL);
        SIP_DEBUG_TRACE(LOG_DEBUG, "uas_check8_2_2() exit---: Message Get To Error \r\n");
        return -1;
    }

    /*check Request-URI*/
    request_uri = osip_message_get_uri(sip);

    if (NULL == request_uri)
    {
        /*RESPONSE 400 (bad request)*/
        sip_response_default(-1, tr, 400, NULL);
        SIP_DEBUG_TRACE(LOG_DEBUG, "uas_check8_2_2() exit---: Message Get Request URL Error \r\n");
        return -2;
    }

    scheme = osip_uri_get_scheme(request_uri);

    if (0 != sstrcmp(scheme, "sip"))
        //&&(0 != strcmp(scheme, "sips")))
    {
        /*response 416 */
        sip_response_default(-1, tr, 416, NULL);
        SIP_DEBUG_TRACE(LOG_DEBUG, "uas_check8_2_2() exit---: Message Get Scheme Error \r\n");
        return -3;
    }

    /* 8.2.2.3 :Require*/
    i = osip_message_get_require(sip, 0, &header);

    if (i != -1)
    {
        if (header->hvalue != NULL)
        {
            i = generating_response_default(&response, NULL, 420, tr->orig_request, NULL);

            if (0 == i)
            {
                osip_message_set_unsupported(response, header->hvalue);
                i = ul_sendmessage(tr, response);

                if (i != 0)
                {
                    SIP_DEBUG_TRACE(LOG_ERROR, "uas_check8_2_2(): ul_sendmessage Error \r\n");
                }
            }

            SIP_DEBUG_TRACE(LOG_DEBUG, "uas_check8_2_2() exit---: Generating Default Response 420 \r\n");
            return -6;
        }
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : uas_check8_2_3
 功能描述  : sip 8.2.3检查
 输入参数  : transaction_t *tr
                            sip_t *sip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月2日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int uas_check8_2_3(osip_transaction_t* tr, osip_message_t* sip)
{
    osip_content_type_t* content_type = NULL;
    osip_header_t* content_language = NULL;
    osip_content_encoding_t* content_encoding = NULL;
    osip_message_t* response = NULL;
    //sipevent_t *sipevent = NULL;
    int i = 0;
    int pos = -1;

    /* 8.2.3:Content Processing*/
    /* Content-Type header*/
    content_type = osip_message_get_content_type(sip);

    if (content_type != NULL)
    {
        int ctpass = 0;

        if (content_type->type != NULL)
        {
            if (!strncmp(content_type->type, "application", 12)
                || !strncmp(content_type->type, "Application", 12)
                || !strncmp(content_type->type, "APPLICATION", 12))
            {
                if (content_type->subtype != NULL)
                {
                    if (!strncmp(content_type->subtype, "sdp", 4)
                        || !strncmp(content_type->subtype, "SDP", 4)
                        || !strncmp(content_type->subtype, "manscdp+xml", 12)
                        || !strncmp(content_type->subtype, "MANSCDP+xml", 12)
                        || !strncmp(content_type->subtype, "mansrtsp", 9)
                        || !strncmp(content_type->subtype, "MANSRTSP", 9)
                        || !strncmp(content_type->subtype, "rtsp", 5)
                        || !strncmp(content_type->subtype, "RTSP", 5))
                    {
                        ctpass = 1;
                    }
                }

            }
        }

        if (!ctpass)
        {
            /* response 415  and add Accept header*/
            i = generating_response_default(&response, NULL, 415, tr->orig_request, NULL);

            if (0 == i)
            {
                osip_message_set_accept(response, (char*)"Application/SDP");
                osip_message_set_accept(response, (char*)"APPLICATION/SDP");
                osip_message_set_accept(response, (char*)"application/sdp");
                osip_message_set_accept(response, (char*)"Application/MANSCDP+xml");
                osip_message_set_accept(response, (char*)"APPLICATION/MANSCDP+xml");
                osip_message_set_accept(response, (char*)"Application/MANSRTSP");
                osip_message_set_accept(response, (char*)"APPLICATION/MANSRTSP");
                osip_message_set_accept(response, (char*)"Application/RTSP");
                osip_message_set_accept(response, (char*)"APPLICATION/RTSP");
                i = ul_sendmessage(tr, response);

                if (i != 0)
                {
                    SIP_DEBUG_TRACE(LOG_ERROR, "uas_check8_2_3(): ul_sendmessage Error \r\n");
                }
            }

            SIP_DEBUG_TRACE(LOG_DEBUG, "uas_check8_2_3() exit---: Generating Default Response 415 \r\n");
            return -1;
        }
    }

    /* Content-Language header*/
    pos = 0;

    while (-1 != osip_message_get_content_language(sip, pos, &content_language))
    {
        /*if (content_language) not support */
        if (content_language->hvalue == NULL || strncmp(content_language->hvalue, "en", 3)) /* not do now */
        {
            /* response 415  and add Accept-Language header*/
            i = generating_response_default(&response, NULL, 415, tr->orig_request, NULL);

            if (0 == i)
            {
                osip_message_set_accept_language(response, (char*)"en");
                i = ul_sendmessage(tr, response);

                if (i != 0)
                {
                    SIP_DEBUG_TRACE(LOG_ERROR, "uas_check8_2_3(): ul_sendmessage Error \r\n");
                }
            }

            SIP_DEBUG_TRACE(LOG_DEBUG, "uas_check8_2_3() exit---: Generating Default Response 415 \r\n");
            return -2;
        }

        pos++;
    }

    /* Content-Encoding header*/
    pos = 0;

    while (-1 != osip_message_get_content_encoding(sip, pos, &content_encoding))
    {
        if (content_encoding != NULL && content_encoding->value != NULL) /* we do not support any content encoding now */
        {
            if (strncmp(content_encoding->value, "identity", 9))
            {
                /* response 415  and add Accept-Encoding header*/
                i = generating_response_default(&response, NULL, 415, tr->orig_request, NULL);

                if (0 == i)
                {
                    osip_message_set_accept_encoding(response, (char*)"identity");
                    i = ul_sendmessage(tr, response);

                    if (i != 0)
                    {
                        SIP_DEBUG_TRACE(LOG_ERROR, "uas_check8_2_3(): ul_sendmessage Error \r\n");
                    }
                }

                SIP_DEBUG_TRACE(LOG_DEBUG, "uas_check8_2_3() exit---: Generating Default Response 415 \r\n");
                return -3;
            }
        }

        pos++;
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : uas_check8_2_4
 功能描述  : sip 8.2.4检查
 输入参数  : transaction_t *tr
             sip_t *sip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月2日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int uas_check8_2_4(osip_transaction_t* tr, osip_message_t* sip)
{
    /*8.2.4 :Applying Extensions */
    /* not need now */
    return 0;
}

/*****************************************************************************
 函 数 名  : uas_check8_2
 功能描述  : uas check8.2
 输入参数  : transaction_t *tr
                            sip_t *sip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月2日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int uas_check8_2(osip_transaction_t* tr, osip_message_t* sip)
{
    int i = 0;

    if (NULL == tr || NULL == sip)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "uas_check8_2() exit---: Param Error \r\n");
        return -1;
    }

    /* 8.2.1: Method Inspection OK */

    i = uas_check8_2_2(tr, sip);

    if (0 != i)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "uas_check8_2() exit---: uas_check8_2_2 Error \r\n");
        return -2;
    }

    i = uas_check8_2_3(tr, sip);

    if (0 != i)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "uas_check8_2() exit---: uas_check8_2_3 Error \r\n");
        return -3;
    }

    i = uas_check8_2_4(tr, sip);

    if (0 != i)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "uas_check8_2() exit---: uas_check8_2_4 Error \r\n");
        return -4;
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : uas_check_invite_within_dialog
 功能描述  : UAS在会话内检查INVITE消息
 输入参数  : int index
                            transaction_t *tr
                            sip_t *sip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月2日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int uas_check_invite_within_dialog(int index, osip_transaction_t* tr, osip_message_t* sip)
{
    sip_dialog_t* pSipDlg = NULL;

    if (NULL == tr || NULL == sip)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "uas_check_invite_within_dialog() exit---: Param Error \r\n");
        return -1;
    }

    if (!is_valid_dialog_index(index))
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "uas_check_invite_within_dialog() exit---: Dialog Index Error \r\n");
        return -1;
    }

    pSipDlg = get_dialog_sip_dialog(index);

    if (NULL == pSipDlg)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "uas_check_invite_within_dialog() exit---: Get SIP Dialog Error \r\n");
        return -1;
    }

    /* check has the old INVITE(incoming or outgoing)  finished ?*/
    if (pSipDlg->ict_tr != NULL || pSipDlg->ist_tr != NULL)
    {
//        sip_response_default(index, tr, 500);
        sip_response_default(index, tr, 486, NULL);
        SIP_DEBUG_TRACE(LOG_DEBUG, "uas_check_invite_within_dialog() exit---: SIP Dialog Tr Error \r\n");
        return -1;
    }

    /* check cseq header */
    if ((sip->cseq == NULL)
        || (osip_atoi(sip->cseq->number) <= pSipDlg->remote_cseq))
    {
        sip_response_default(index, tr, 500, (char*)"Cseq Error");
        SIP_DEBUG_TRACE(LOG_DEBUG, "uas_check_invite_within_dialog() exit---: SIP Cseq Error \r\n");
        return -1;
    }

    return 0;
}
#endif

#if DECS("本地SDP信息组装")
/*****************************************************************************
 函 数 名  : sdp_local_config
 功能描述  : 本地SDP信息设置
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月2日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int sdp_local_config()
{
    if (sdp_config_init())
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "sdp_local_config() exit---: SDP Config Init Error \r\n");
        return -1;
    }

    sdp_config_set_o_username(osip_getcopy("ZBITCLOUD_SIPUA"));

    sdp_config_set_o_session_id(osip_getcopy("0"));
    sdp_config_set_o_session_version(osip_getcopy("0"));

    sdp_config_set_o_nettype(osip_getcopy("IN"));
    sdp_config_set_o_addrtype(osip_getcopy("IP4"));

    sdp_config_set_c_nettype(osip_getcopy("IN"));
    sdp_config_set_c_addrtype(osip_getcopy("IP4"));

    /* 视频编码 */
    sdp_config_add_support_for_video_codec(osip_getcopy("96"),
                                           NULL,
                                           osip_getcopy("RTP/AVP"),
                                           NULL, NULL, NULL,
                                           NULL, NULL,
                                           osip_getcopy("96 PS/90000"));

    sdp_config_add_support_for_video_codec(osip_getcopy("97"),
                                           NULL,
                                           osip_getcopy("RTP/AVP"),
                                           NULL, NULL, NULL,
                                           NULL, NULL,
                                           osip_getcopy("97 MPEG4/90000"));

    sdp_config_add_support_for_video_codec(osip_getcopy("98"),
                                           NULL,
                                           osip_getcopy("RTP/AVP"),
                                           NULL, NULL, NULL,
                                           NULL, NULL,
                                           osip_getcopy("98 H264/90000"));

    sdp_config_add_support_for_video_codec(osip_getcopy("99"),
                                           NULL,
                                           osip_getcopy("RTP/AVP"),
                                           NULL, NULL, NULL,
                                           NULL, NULL,
                                           osip_getcopy("99 SVAC/90000"));

    sdp_config_add_support_for_video_codec(osip_getcopy("500"),
                                           NULL,
                                           osip_getcopy("RTP/AVP"),
                                           NULL, NULL, NULL,
                                           NULL, NULL,
                                           osip_getcopy("500 HIK/90000"));

    sdp_config_add_support_for_video_codec(osip_getcopy("501"),
                                           NULL,
                                           osip_getcopy("RTP/AVP"),
                                           NULL, NULL, NULL,
                                           NULL, NULL,
                                           osip_getcopy("501 DAH/90000"));

    sdp_config_add_support_for_video_codec(osip_getcopy("502"),
                                           NULL,
                                           osip_getcopy("RTP/AVP"),
                                           NULL, NULL, NULL,
                                           NULL, NULL,
                                           osip_getcopy("502 NETPOSA/90000"));

    sdp_config_add_support_for_video_codec(osip_getcopy("503"),
                                           NULL,
                                           osip_getcopy("RTP/AVP"),
                                           NULL, NULL, NULL,
                                           NULL, NULL,
                                           osip_getcopy("503 WENAN/90000"));

    sdp_config_set_fcn_accept_video_codec(&sdp_accept_video_codec);

    /* 音频编码 */
    sdp_config_add_support_for_audio_codec(osip_getcopy("8"),
                                           NULL,
                                           osip_getcopy("RTP/AVP"),
                                           NULL, NULL, NULL,
                                           NULL, NULL,
                                           osip_getcopy("8 PCMA/8000"));

    sdp_config_add_support_for_audio_codec(osip_getcopy("20"),
                                           NULL,
                                           osip_getcopy("RTP/AVP"),
                                           NULL, NULL, NULL,
                                           NULL, NULL,
                                           osip_getcopy("20 SVACA/8000"));

    sdp_config_add_support_for_audio_codec(osip_getcopy("4"),
                                           NULL,
                                           osip_getcopy("RTP/AVP"),
                                           NULL, NULL, NULL,
                                           NULL, NULL,
                                           osip_getcopy("4 G723/8000"));

    sdp_config_add_support_for_audio_codec(osip_getcopy("18"),
                                           NULL,
                                           osip_getcopy("RTP/AVP"),
                                           NULL, NULL, NULL,
                                           NULL, NULL,
                                           osip_getcopy("18 G729/8000"));

    sdp_config_add_support_for_audio_codec(osip_getcopy("9"),
                                           NULL,
                                           osip_getcopy("RTP/AVP"),
                                           NULL, NULL, NULL,
                                           NULL, NULL,
                                           osip_getcopy("9 G722/8000"));

    sdp_config_set_fcn_accept_audio_codec(&sdp_accept_audio_codec);

    return 0;
}

int sdp_accept_video_codec(sdp_context_t* context, char* port, char* number_of_port, int video_qty, char* payload)
{
    if (0 != video_qty)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "sdp_accept_video_codec() exit---: Param Error \r\n");
        return -1;
    }

    if ((0 == strncmp(payload, "96", 2))
        || (0 == strncmp(payload, "97", 2))
        || (0 == strncmp(payload, "98", 2))
        || (0 == strncmp(payload, "99", 2))
        || (0 == strncmp(payload, "500", 3))
        || (0 == strncmp(payload, "501", 3)))
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "sdp_accept_video_codec() exit---: OK \r\n");
        return 0;
    }

    return -1;

}

int sdp_accept_audio_codec(sdp_context_t * context, char * port, char * number_of_port, int audio_qty, char *payload)
{

    if (0 != audio_qty)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "sdp_accept_audio_codec() exit---: Param Error \r\n");
        return -1;
    }

    if (0 == strncmp(payload, "4", 1)
        || 0 == strncmp(payload, "8", 1)
        || 0 == strncmp(payload, "9", 1)
        || 0 == strncmp(payload, "18", 1)
        || 0 == strncmp(payload, "20", 1))
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "sdp_accept_audio_codec() exit---: OK \r\n");
        return 0;
    }

    return -1;

}
#endif

#if DECS("会话操作")
/*****************************************************************************
 函 数 名  : do_accept_call
 功能描述  : 接收呼叫
 输入参数  : int index
             sdp_t* local_sdp
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年5月28日 星期二
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int do_accept_call(int index, sdp_message_t* local_sdp)
{
    int i = 0;
    ua_dialog_t* pUaDialog = NULL;

    if (NULL == local_sdp)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "do_accept_call() exit---: Param Error \r\n");
        return -1;
    }

    pUaDialog = ua_dialog_get(index);

    if (NULL == pUaDialog)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "do_accept_call() exit---: Get UA Dialog Error:dialog_index=%d \r\n", index);
        return EV9000_SIPSTACK_INVITE_GET_UA_ERROR;
    }

    SIP_DEBUG_TRACE(LOG_DEBUG, "do_accept_call() CallerID=%s, CalleeID=%s, LocalIP=%s, LocalPort=%d  \r\n", pUaDialog->strCallerID, pUaDialog->strCalleeID, pUaDialog->strLocalIP, pUaDialog->iLocalPort);

    i = sip_2xx_answer_to_invite(index, pUaDialog->strCalleeID, pUaDialog->strLocalIP, pUaDialog->iLocalPort, pUaDialog->iSessionExpires, local_sdp);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "do_accept_call() exit---: SIP 2xx Answer To Invite Error \r\n");
        return i;
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : sip_invite
 功能描述  : 发送INVITE消息
 输入参数  : sip_dialog_t** dialog
                            char *caller
                            char *callee
                            char *proxyip
                            int proxyport
                            char *localip
                            int localport
                            int rtpport
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月3日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int sip_invite(int ua_dialog_pos)
{
    char strSubject[128] = {0};
    osip_message_t* invite = NULL;
    sdp_message_t* sdp = NULL;
    osip_transaction_t* transaction = NULL;
    ua_dialog_t* pUaDialog = NULL;
    sip_dialog_t* pSipDlg = NULL;
    int i = 0;
    int iSType = 0;
    char* body = NULL;
    int socket = 0;
    char strSessionExpires[32] = {0};

    if (!is_valid_dialog_index(ua_dialog_pos))
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "sip_invite() exit---: Dialog Index Error \r\n");
        return EV9000_SIPSTACK_INVITE_GET_UA_ERROR;
    }

    pUaDialog = ua_dialog_get(ua_dialog_pos);

    if (pUaDialog == NULL)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "sip_invite() exit---: Get UA Dialog Error:dialog_index=%d \r\n", ua_dialog_pos);
        return EV9000_SIPSTACK_INVITE_GET_UA_ERROR;
    }

    sdp = pUaDialog->pLocalSDP;

    if (NULL == sdp)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "sip_invite() exit---: Get UA Dialog SDP Error \r\n");
        return EV9000_SIPSTACK_INVITE_GET_SDP_INFO_ERROR;
    }

    i = sdp_message_to_str(sdp, &body);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "sip_invite() exit---: SDP 2 Char Error \r\n");
        return EV9000_SIPSTACK_SDP_TO_STR_ERROR;
    }

    i = generating_invite(&invite, pUaDialog->strCallerID, pUaDialog->strCalleeID, body, pUaDialog->strRemoteIP, pUaDialog->iRemotePort, pUaDialog->strLocalIP, pUaDialog->iLocalPort);
    osip_free(body);
    body = NULL;

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "sip_invite() exit---: Generating Invite Error \r\n");
        return EV9000_SIPSTACK_INVITE_GENERA_ERROR;
    }

    /* 设置Subject 头域 */
    if (0 == strncmp(sdp->s_name, "Playback", 8))
    {
        iSType = 1;
    }
    else if (0 == strncmp(sdp->s_name, "Play", 4))
    {
        iSType = 0;
    }
    else if (0 == strncmp(sdp->s_name, "Download", 8))
    {
        iSType = 1;
    }

    snprintf(strSubject, 128, "%s:%d,%s:%d", pUaDialog->strCalleeID, iSType, pUaDialog->strCallerID, iSType);
    osip_message_set_subject(invite, strSubject);

    /* 如果支持会话刷新，增加相应头域 */
    osip_message_set_supported(invite, (char*)"timer");
    snprintf(strSessionExpires, 32, "%d", MIN_SESSION_EXPIRE);
    msg_set_session_expires(invite, strSessionExpires);
    pUaDialog->iSessionExpires = MIN_SESSION_EXPIRE;

    /* 获取socket */
    socket = get_socket_by_port(pUaDialog->iLocalPort);

    if (socket <= 0)
    {
        osip_message_free(invite);
        invite = NULL;
        SIP_DEBUG_TRACE(LOG_ERROR, "sip_invite() exit---: Get Socket Error \r\n");
        return EV9000_SIPSTACK_GET_SOCKET_ERROR;
    }

    i = osip_transaction_init(&transaction, ICT, g_recv_cell, invite);

    if (0 != i)
    {
        osip_message_free(invite);
        invite = NULL;
        SIP_DEBUG_TRACE(LOG_ERROR, "sip_invite() exit---: Transaction Init Error \r\n");
        return EV9000_SIPSTACK_TRANSACTION_INIT_ERROR;
    }

    /* 设置transaction的socket */
    osip_transaction_set_in_socket(transaction, socket);
    osip_transaction_set_out_socket(transaction, socket);

    sip_dialog_init_as_uac(&pSipDlg, transaction, invite);
    pUaDialog->pSipDialog = pSipDlg;
    set_dialog_ui_state(ua_dialog_pos, UI_STATE_CALL_SENT);

    ua_timer_use(UA_INVITE_TIMEOUT, ua_dialog_pos, transaction, NULL); /*add INVITE timeout timer*/

    i = ul_sendmessage(transaction, invite);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "sip_invite(): ul_sendmessage Error \r\n");
        return EV9000_SIPSTACK_SEND_MESSAGE_ERROR;
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : sip_forward_invite_within_dialog
 功能描述  : 转发会话内的invite消息
 输入参数  : int caller_dialog_pos
                            int callee_dialog_pos
                            char* remote_ip
                            int remote_port
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年5月28日 星期二
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int sip_forward_invite_within_dialog(int caller_dialog_pos, int callee_dialog_pos)
{
    char strSubject[128] = {0};
    osip_message_t* invite = NULL;
    sdp_message_t* sdp = NULL;
    int socket = 0;
    osip_transaction_t* transaction = NULL;
    ua_dialog_t* pCallerUaDialog = NULL;
    ua_dialog_t* pCalleeUaDialog = NULL;
    sip_dialog_t*  pCallerSipDlg = NULL;
    sip_dialog_t*  pCalleeSipDlg = NULL;
    int i = 0;
    int iSType = 0;
    char* body = NULL;
    //osip_uri_t* old_url = NULL;
    //osip_uri_t* requrl = NULL;
    char strSessionExpires[32] = {0};

    if (!is_valid_dialog_index(caller_dialog_pos) || !is_valid_dialog_index(callee_dialog_pos))
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_forward_invite_within_dialog() exit---: Dialog Index Error \r\n");
        return EV9000_SIPSTACK_INVITE_GET_UA_ERROR;
    }

    /* 主叫侧UA */
    pCallerUaDialog = ua_dialog_get(caller_dialog_pos);

    if (pCallerUaDialog == NULL)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_forward_invite_within_dialog() exit---: Get Caller UA Dialog Error:dialog_index=%d \r\n", caller_dialog_pos);
        return EV9000_SIPSTACK_INVITE_GET_UA_ERROR;
    }

    pCallerSipDlg = get_dialog_sip_dialog(caller_dialog_pos);

    if (pCallerSipDlg == NULL)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_forward_invite_within_dialog() exit---: Get Caller SIP Dialog Error:dialog_index=%d \r\n", caller_dialog_pos);
        return EV9000_SIPSTACK_INVITE_GET_SIPDLG_ERROR;
    }

    /* 被叫侧UA */
    pCalleeUaDialog = ua_dialog_get(callee_dialog_pos);

    if (pCalleeUaDialog == NULL)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_forward_invite_within_dialog() exit---: Get Callee UA Dialog Error:dialog_index=%d \r\n", callee_dialog_pos);
        return EV9000_SIPSTACK_INVITE_GET_UA_ERROR;
    }

    /* sdp */
    sdp = pCalleeUaDialog->pLocalSDP;

    if (NULL == sdp)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_forward_invite_within_dialog() exit---: Get UA Dialog SDP Error \r\n");
        return EV9000_SIPSTACK_INVITE_GET_SDP_INFO_ERROR;
    }

    i = sdp_message_to_str(sdp, &body);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_forward_invite_within_dialog() exit---: SDP 2 Char Error \r\n");
        return EV9000_SIPSTACK_SDP_TO_STR_ERROR;
    }

    /* 生成invite */
    //i = generating_forward_invite_within_dialog(&invite, pCallerSipDlg, body, pCallerUaDialog->pcCallerID, pCalleeUaDialog->pcLocalIP, pCalleeUaDialog->iLocalPort);
    i = generating_invite(&invite, pCalleeUaDialog->strCallerID, pCalleeUaDialog->strCalleeID, body, pCalleeUaDialog->strRemoteIP, pCalleeUaDialog->iRemotePort, pCalleeUaDialog->strLocalIP, pCalleeUaDialog->iLocalPort);

    osip_free(body);
    body = NULL;

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_forward_invite_within_dialog() exit---: Generating Forward Invite Within Dialog Error \r\n");
        return EV9000_SIPSTACK_INVITE_GENERA_ERROR;
    }

    /* 设置Subject 头域 */
    if (0 == strncmp(sdp->s_name, "Playback", 8))
    {
        iSType = 1;
    }
    else if (0 == strncmp(sdp->s_name, "Play", 4))
    {
        iSType = 0;
    }
    else if (0 == strncmp(sdp->s_name, "Download", 8))
    {
        iSType = 1;
    }

    snprintf(strSubject, 128, "%s:%d,%s:%d", pCalleeUaDialog->strCalleeID, iSType, pCalleeUaDialog->strCallerID, iSType);
    osip_message_set_subject(invite, strSubject);

    /* 如果支持会话刷新，增加相应头域 */
    osip_message_set_supported(invite, (char*)"timer");
    snprintf(strSessionExpires, 32, "%d", MIN_SESSION_EXPIRE);
    msg_set_session_expires(invite, strSessionExpires);
    pCalleeUaDialog->iSessionExpires = MIN_SESSION_EXPIRE;

#if 0
    requrl = BuildTargetUrl(pCalleeUaDialog->pcCalleeID, remote_ip, remote_port);

    if (NULL == requrl)
    {
        osip_message_free(invite);
        invite = NULL;
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_forward_invite_within_dialog() exit---: Build Request URL Error \r\n");
        return -1;
    }

    old_url = osip_message_get_uri(invite);
    osip_message_set_uri(invite, requrl);
    osip_uri_free(old_url);
    old_url = NULL;
#endif

    /* 获取socket */
    socket = get_socket_by_port(pCalleeUaDialog->iLocalPort);

    if (socket <= 0)
    {
        osip_message_free(invite);
        invite = NULL;
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_forward_invite_within_dialog() exit---: Get Socket Error \r\n");
        return EV9000_SIPSTACK_GET_SOCKET_ERROR;
    }

    i = osip_transaction_init(&transaction,
                              ICT,
                              g_recv_cell,
                              invite);

    if (0 != i)
    {
        osip_message_free(invite);
        invite = NULL;
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_forward_invite_within_dialog() exit---: Transaction Init Error \r\n");
        return EV9000_SIPSTACK_TRANSACTION_INIT_ERROR;
    }

    /* 设置transaction的socket */
    osip_transaction_set_in_socket(transaction, socket);
    osip_transaction_set_out_socket(transaction, socket);

    sip_dialog_init_as_uac(&pCalleeSipDlg, transaction, invite);
    pCalleeUaDialog->pSipDialog = pCalleeSipDlg;
    pCalleeUaDialog->eUiState = UI_STATE_CALL_SENT;

    ua_timer_use(UA_INVITE_TIMEOUT, callee_dialog_pos, transaction, NULL); /*add INVITE timeout timer*/

    i = ul_sendmessage(transaction, invite);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "sip_forward_invite_within_dialog(): ul_sendmessage Error \r\n");
        return EV9000_SIPSTACK_SEND_MESSAGE_ERROR;
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : sip_invite2
 功能描述  : 添加认证信息的INVITE消息发送
 输入参数  : sip_t *request
                            sip_t *resp
                            char *csAuthname
                            char *csAuthpassword
                            char *proxyip
                            int proxyport
                            char *localip
                            int localport
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月3日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int sip_invite2(osip_message_t* request, osip_message_t* resp, char* csAuthname, char* csAuthpassword, char* proxyip, int proxyport, char* localip, int localport)
{
    int i = 0;
    int socket = 0;
    osip_message_t* sinvite = NULL;
    osip_transaction_t* transaction = NULL;
    osip_www_authenticate_t*   www_authenticate = NULL;
    osip_proxy_authenticate_t* proxy_authenticate = NULL;
    //char *contact = NULL;
    //contact_t *old_contact=NULL;
    //char *username = NULL;
    int  register_cseq_number = 1;    /* always start registration with 1 */
    osip_via_t* via = NULL;

    if (request == NULL || resp == NULL)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_invite2() exit---: Param Error \r\n");
        return -1;
    }

    if (resp->status_code == 401)
    {
        if (! osip_list_eol(&resp->www_authenticates, 0))
        {
            www_authenticate = (osip_www_authenticate_t*)osip_list_get(&resp->www_authenticates, 0);
        }

        if (www_authenticate == NULL)
        {
            SIP_DEBUG_TRACE(LOG_DEBUG, "sip_invite2() exit---: Get WWW Authenticate Error \r\n");
            return -1;
        }
    }
    else if (resp->status_code == 407)
    {
        if (!osip_list_eol(&resp->proxy_authenticates, 0))
        {
            proxy_authenticate = (osip_proxy_authenticate_t*)osip_list_get(&resp->proxy_authenticates, 0);
        }

        if (proxy_authenticate == NULL)
        {
            SIP_DEBUG_TRACE(LOG_DEBUG, "sip_invite2() exit---: Get Proxy Authenticate Error \r\n");
            return -1;
        }
    }
    else
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_invite2() exit---: Status Code Error \r\n");
        return -1;
    }

    if (csAuthname == NULL)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_invite2() exit---: Auth Name Error \r\n");
        return -1;
    }

    i = generating_request_fromrequest(request, &sinvite, localip, localport);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_invite2() exit---: Generating Request From Request Error \r\n");
        return -1;
    }

    /* remove all old authorizations and proxy authorizations*/
    {
        int pos = 0;
        osip_authorization_t* authorization = NULL;
        osip_proxy_authorization_t* proxy_authorization = NULL;
        pos = 0;

        while (!osip_list_eol(&sinvite->authorizations, pos))
        {
            authorization = (osip_authorization_t*)osip_list_get(&sinvite->authorizations, pos);

            if (NULL == authorization)
            {
                pos++;
                continue;
            }

            osip_list_remove(&sinvite->authorizations, pos);
            osip_authorization_free(authorization);
            authorization = NULL;
            pos++;
        }

        pos = 0;

        while (!osip_list_eol(&sinvite->proxy_authorizations, pos))
        {
            proxy_authorization = (osip_proxy_authorization_t*)osip_list_get(&sinvite->proxy_authorizations, pos);

            if (NULL == proxy_authorization)
            {
                pos++;
                continue;
            }

            osip_list_remove(&sinvite->proxy_authorizations, pos);
            osip_authorization_free(proxy_authorization);
            proxy_authorization = NULL;
            pos++;
        }
    }

    if (www_authenticate != NULL)
    {
        i = request_add_authorization(sinvite, www_authenticate, csAuthname, csAuthpassword, proxyip, proxyport);

        if (i != 0)
        {
            osip_message_free(sinvite);
            sinvite = NULL;
            SIP_DEBUG_TRACE(LOG_DEBUG, "sip_invite2() exit---: Request Add Authorization Error \r\n");
            return -1;
        }
    }
    else
    {
        i = request_add_proxy_authorization(sinvite, proxy_authenticate, csAuthname, csAuthpassword);

        if (i != 0)
        {
            osip_message_free(sinvite);
            sinvite = NULL;
            SIP_DEBUG_TRACE(LOG_DEBUG, "sip_invite2() exit---: Request Add Proxy Authorization Error \r\n");
            return -1;
        }
    }

    int pos = 0;
    char* tmp = NULL;
    char* _via = NULL;
    osip_generic_param_t* url_param = NULL;
    osip_list_remove(&sinvite->vias, 0);

    if (!osip_list_eol(&resp->vias, 0))
    {
        via = (osip_via_t*)osip_list_get(&resp->vias, 0);

        osip_via_param_get_byname(via, (char*)"branch", &url_param);

        if (url_param && url_param->gvalue)
        {
            if ((_via = (char*)osip_malloc(sizeof(char) * 100)) != NULL)
            {
                snprintf(_via, sizeof(char) * 100, "%s/%s %s:%u;branch=%s", SIP_VERSION, "UDP",
                         localip,
                         localport,
                         url_param->gvalue);
                osip_message_set_via(sinvite, _via);
                osip_free(_via);
                _via = NULL;
            }
        }
    }

    pos = 1;

    while (!osip_list_eol(&resp->vias, pos))
    {
        via = (osip_via_t*)osip_list_get(&resp->vias, pos);

        if (NULL == via)
        {
            pos++;
            continue;
        }

        osip_via_to_str(via, &tmp);
        osip_message_set_via(sinvite, tmp);
        osip_free(tmp);
        tmp = NULL;
        pos++;
    }

    register_cseq_number = osip_atoi(sinvite->cseq->number);
    register_cseq_number++;

    /* 获取socket */
    socket = get_socket_by_port(localport);

    if (socket <= 0)
    {
        osip_message_free(sinvite);
        sinvite = NULL;
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_invite2() exit---: Get Socket Error \r\n");
        return -1;
    }

    i = osip_transaction_init(&transaction,
                              ICT,
                              g_recv_cell,
                              sinvite);

    if (0 != i)
    {
        osip_message_free(sinvite);
        sinvite = NULL;
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_invite2() exit---: Transaction Init Error \r\n");
        return -1;
    }

    /* 设置transaction的socket */
    osip_transaction_set_in_socket(transaction, socket);
    osip_transaction_set_out_socket(transaction, socket);

    i = ul_sendmessage(transaction, sinvite);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "sip_invite2(): ul_sendmessage Error \r\n");
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : sip_register
 功能描述  : 发送注册消息
 输入参数  : char* caller
                            char* username
                            char* localip
                            int localport
                            char* proxyid
                            char* proxyip
                            int proxyport
                            int expires
                            char* register_callid_number
                            int register_cseq_number
                            int link_type
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月3日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int sip_register(char* caller, char* username, char* proxyid, char* proxyip, int proxyport, char* localip, int localport, int expires, char* register_callid_number, int register_cseq_number, int link_type)
{
    osip_message_t* sregister = NULL;
    osip_transaction_t* tr = NULL;
    int i = 0;
    int socket = 0;

    if (NULL == caller || NULL == username || NULL == proxyid || NULL == proxyip || NULL == localip || NULL == register_callid_number)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "sip_register() exit---: Param Error \r\n");
        return -1;
    }

    i = generating_register(&sregister, caller, username, proxyid, proxyip, proxyport, localip, localport, expires, register_callid_number, register_cseq_number);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "sip_register() exit---: Generating Register Error \r\n");
        return EV9000_SIPSTACK_REGISTER_GENERA_ERROR;
    }

    if (1 == link_type)
    {
        osip_message_set_header(sregister, (char*)"Link-Type", (char*)"Peering");
    }

    /* 获取socket */
    socket = get_socket_by_port(localport);

    if (socket <= 0)
    {
        osip_message_free(sregister);
        sregister = NULL;
        SIP_DEBUG_TRACE(LOG_ERROR, "sip_register() exit---: Get Socket Error \r\n");
        return EV9000_SIPSTACK_GET_SOCKET_ERROR;
    }

    i = osip_transaction_init(&tr, NICT, g_recv_cell, sregister);

    if (0 != i)
    {
        osip_message_free(sregister);
        sregister = NULL;
        SIP_DEBUG_TRACE(LOG_ERROR, "sip_register() exit---: Transaction Init Error \r\n");
        return EV9000_SIPSTACK_TRANSACTION_INIT_ERROR;
    }

    /* 设置transaction的socket */
    osip_transaction_set_in_socket(tr, socket);
    osip_transaction_set_out_socket(tr, socket);

    i = ul_sendmessage(tr, sregister);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "sip_register(): ul_sendmessage Error \r\n");
        return EV9000_SIPSTACK_SEND_MESSAGE_ERROR;
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : sip_register2
 功能描述  : 发送带认证信息的注册消息
 输入参数  : sip_t *request
                            sip_t *resp
                            char* localip
                            int localport
                            char* proxyip
                            int proxyport
                            char* username
                            char* userpassword
                            int link_type
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月3日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int sip_register2(osip_message_t* request, osip_message_t* resp, char* proxyip, int proxyport, char* localip, int localport, char* username, char* userpassword, int link_type)
{
    int i = 0;
    int socket = 0;
    osip_message_t*         sregister = NULL;
    osip_transaction_t* transaction = NULL;
    osip_www_authenticate_t*   www_authenticate = NULL;
    osip_proxy_authenticate_t* proxy_authenticate = NULL;

    if (request == NULL || resp == NULL || NULL == username)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_register2() exit---: Param Error \r\n");
        return -1;
    }

    if (resp->status_code == 401)
    {
        if (! osip_list_eol(&resp->www_authenticates, 0))
        {
            www_authenticate = (osip_www_authenticate_t*)osip_list_get(&resp->www_authenticates, 0);
        }

        if (www_authenticate == NULL)
        {
            SIP_DEBUG_TRACE(LOG_DEBUG, "sip_register2() exit---: Get WWW Authenticate Error \r\n");
            return -1;
        }
    }
    else if (resp->status_code == 407)
    {
        if (!osip_list_eol(&resp->proxy_authenticates, 0))
        {
            proxy_authenticate = (osip_proxy_authenticate_t*)osip_list_get(&resp->proxy_authenticates, 0);
        }

        if (proxy_authenticate == NULL)
        {
            SIP_DEBUG_TRACE(LOG_DEBUG, "sip_register2() exit---: Get Proxy Authenticate Error \r\n");
            return -1;
        }
    }
    else
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_register2() exit---: Status Code Error \r\n");
        return -1;
    }

    i = generating_request_fromrequest(request, &sregister, localip, localport);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_register2() exit---: Generating Request From Request Error \r\n");
        return -1;
    }

    /* remove all old authorizations and proxy authorizations*/
    {
        int pos = -1;
        osip_authorization_t* authorization = NULL;
        osip_proxy_authorization_t* proxy_authorization = NULL;
        pos = 0;

        while (!osip_list_eol(&sregister->authorizations, pos))
        {
            authorization = (osip_authorization_t*)osip_list_get(&sregister->authorizations, pos);

            if (NULL == authorization)
            {
                pos++;
                continue;
            }

            osip_list_remove(&sregister->authorizations, pos);
            osip_authorization_free(authorization);
            authorization = NULL;
            pos++;
        }

        pos = 0;

        while (!osip_list_eol(&sregister->proxy_authorizations, pos))
        {
            proxy_authorization = (osip_proxy_authorization_t*)osip_list_get(&sregister->proxy_authorizations, pos);

            if (NULL == proxy_authorization)
            {
                pos++;
                continue;
            }

            osip_list_remove(&sregister->proxy_authorizations, pos);
            osip_authorization_free(proxy_authorization);
            proxy_authorization = NULL;
            pos++;
        }
    }

    if (www_authenticate != NULL)
    {
        i = request_add_authorization(sregister, www_authenticate, username, userpassword, proxyip, proxyport);

        if (i != 0)
        {
            osip_message_free(sregister);
            sregister = NULL;
            SIP_DEBUG_TRACE(LOG_DEBUG, "sip_register2() exit---: Request Add Authorization Error \r\n");
            return -1;
        }
    }
    else
    {
        i = request_add_proxy_authorization(sregister, proxy_authenticate, username, userpassword);

        if (i != 0)
        {
            osip_message_free(sregister);
            sregister = NULL;
            SIP_DEBUG_TRACE(LOG_DEBUG, "sip_register2() exit---: Request Add Proxy Authorization Error \r\n");
            return -1;
        }
    }

    /* 获取socket */
    socket = get_socket_by_port(localport);

    if (socket <= 0)
    {
        osip_message_free(sregister);
        sregister = NULL;
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_register2() exit---: Get Socket Error \r\n");
        return -1;
    }

    i = osip_transaction_init(&transaction,
                              NICT,
                              g_recv_cell,
                              sregister);

    if (0 != i)
    {
        osip_message_free(sregister);
        sregister = NULL;
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_register2() exit---: Transaction Init Error \r\n");
        return -1;
    }

    /* 设置transaction的socket */
    osip_transaction_set_in_socket(transaction, socket);
    osip_transaction_set_out_socket(transaction, socket);

    i = ul_sendmessage(transaction, sregister);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "sip_register2(): ul_sendmessage Error \r\n");
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : sip_message
 功能描述  : 发送message消息
 输入参数  : char *caller
                            char *callee
                            char* msg
                            int msg_len
                            char *proxyip
                            int proxyport
                            char *localip
                            int localport
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月3日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int sip_message(char* callid, char* caller, char* callee, char* msg, int msg_len, char* proxyip, int proxyport, char* localip, int localport)
{
    osip_message_t*         message = NULL;
    osip_transaction_t* transaction = NULL;
    int i = 0;
    int socket = 0;

    i = generating_message(&message, caller, callee, callid, msg, msg_len, proxyip, proxyport, localip, localport);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "sip_message() exit---: Generating Message Error \r\n");
        return EV9000_SIPSTACK_MESSAGE_GENERA_ERROR;
    }

    /* 获取socket */
    socket = get_socket_by_port(localport);

    if (socket <= 0)
    {
        osip_message_free(message);
        message = NULL;
        SIP_DEBUG_TRACE(LOG_ERROR, "sip_message() exit---: Get Socket Error \r\n");
        return EV9000_SIPSTACK_GET_SOCKET_ERROR;
    }

    i = osip_transaction_init(&transaction,
                              NICT,
                              g_send_message_cell,
                              message);

    if (0 != i)
    {
        osip_message_free(message);
        message = NULL;
        SIP_DEBUG_TRACE(LOG_ERROR, "sip_message() exit---: Transaction Init Error \r\n");
        return EV9000_SIPSTACK_TRANSACTION_INIT_ERROR;
    }

    /* 设置transaction的socket */
    osip_transaction_set_in_socket(transaction, socket);
    osip_transaction_set_out_socket(transaction, socket);

    i = ul_sendmessage(transaction, message);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "sip_message(): ul_sendmessage Error \r\n");
        return EV9000_SIPSTACK_SEND_MESSAGE_ERROR;
    }

    return i;
}

/*****************************************************************************
 函 数 名  : sip_message_for_tcp
 功能描述  : 基于TCP发送message消息
 输入参数  : char* callid
             char* caller
             char* callee
             char* msg
             int msg_len
             char* proxyip
             int proxyport
             char* localip
             int localport
             int tcp_socket
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年2月21日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int sip_message_for_tcp(char* callid, char* caller, char* callee, char* msg, int msg_len, char* proxyip, int proxyport, char* localip, int localport, int tcp_socket)
{
    osip_message_t* message = NULL;
    int i = 0;

    i = generating_message_for_tcp(&message, caller, callee, callid, msg, msg_len, proxyip, proxyport, localip, localport);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "sip_message_for_tcp() exit---: generating_message_for_tcp Error \r\n");
        return EV9000_SIPSTACK_MESSAGE_GENERA_ERROR;
    }

    i = tl_sendmessage_by_tcp(message, proxyip, proxyport, tcp_socket);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "sip_message_for_tcp(): tl_sendmessage_by_tcp Error \r\n");
        return EV9000_SIPSTACK_SEND_MESSAGE_ERROR;
    }

    return i;
}

/*****************************************************************************
 函 数 名  : sip_info
 功能描述  : 发送info消息
 输入参数  : char *caller
                            char *callee
                            char* body
                            int body_len
                            url_t *requrl
                            char *proxyip
                            int proxyport
                            char *localip
                            int localport
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月3日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int sip_info(char* callid, char* caller, char* callee, char* body, int body_len, osip_uri_t* requrl, char* proxyip, int proxyport, char* localip, int localport)
{
    osip_message_t*         info = NULL;
    osip_transaction_t* transaction = NULL;
    int i = 0;
    int socket = 0;
    osip_uri_t* old_url = NULL;

    i = generating_info(&info, caller, callee, callid, body, body_len, proxyip, proxyport, localip, localport);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_info() exit---: Generating Info Error \r\n");
        return -1;
    }

    old_url = osip_message_get_uri(info);
    osip_message_set_uri(info, requrl);
    osip_uri_free(old_url);
    old_url = NULL;

    /* 获取socket */
    socket = get_socket_by_port(localport);

    if (socket <= 0)
    {
        osip_message_free(info);
        info = NULL;
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_info() exit---: Get Socket Error \r\n");
        return -1;
    }

    i = osip_transaction_init(&transaction,
                              NICT,
                              g_recv_cell,
                              info);

    if (0 != i)
    {
        osip_message_free(info);
        info = NULL;
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_info() exit---: Transaction Init Error \r\n");
        return -1;
    }

    /* 设置transaction的socket */
    osip_transaction_set_in_socket(transaction, socket);
    osip_transaction_set_out_socket(transaction, socket);

    i = ul_sendmessage(transaction, info);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "sip_info(): ul_sendmessage Error \r\n");
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : sip_ack
 功能描述  : 发送ack消息
 输入参数  : int dialog_pos
                            char *localip
                            int localport
                            int iSessionExpires
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月3日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int sip_ack(int dialog_pos, char* localip, int localport, int iSessionExpires)
{
    int i = 0, sock = 0;
    sip_dialog_t* pSipDlg = NULL;
    osip_message_t* ack = NULL;

    if (!is_valid_dialog_index(dialog_pos))
    {
        //SIP_DEBUG_TRACE(LOG_DEBUG, "sip_ack() exit---: Dialog Index Error \r\n");
        return -1;
    }

    USED_UA_SMUTEX_LOCK();

    pSipDlg = get_dialog_sip_dialog2(dialog_pos);

    if (NULL == pSipDlg)
    {
        //SIP_DEBUG_TRACE(LOG_DEBUG, "sip_ack() exit---: Get SIP Dialog Error \r\n");
        USED_UA_SMUTEX_UNLOCK();
        return -1;
    }

    i = generating_ack_for_2xx(&ack, pSipDlg, localip, localport);

    if (i != 0)
    {
        //SIP_DEBUG_TRACE(LOG_DEBUG, "sip_ack() exit---: Generating Ack For 2xx Error \r\n");
        USED_UA_SMUTEX_UNLOCK();
        goto error1;
    }

    i = osip_message_set_content_length(ack, (char*)"0");

    if (i == -1)
    {
        //SIP_DEBUG_TRACE(LOG_DEBUG, "sip_ack() exit---: Message Set Content Length Error \r\n");
        USED_UA_SMUTEX_UNLOCK();
        goto error2;
    }

    update_dialog_as_uac2(pSipDlg, NULL, ack, DLG_EVENT_UPDATE);

    USED_UA_SMUTEX_UNLOCK();

    /* 获取socket */
    sock = get_socket_by_port(localport);

    if (sock <= 0)
    {
        //SIP_DEBUG_TRACE(LOG_DEBUG, "sip_ack() exit---: Get Socket Error \r\n");
        goto error2;
    }

    tl_sendmessage(ack, NULL, 0, sock);

    if (iSessionExpires > 0)
    {
        cs_timer_use(UA_SESSION_EXPIRE, dialog_pos, NULL);
        cs_timer_use(UAC_SEND_UPDATE, dialog_pos, NULL);
        //SIP_DEBUG_TRACE(LOG_DEBUG, "sip_ack() cs_timer_use:UAC_SEND_UPDATE:pos=%d \r\n", dialog_pos);
    }

    osip_message_free(ack);
    ack = NULL;
    return 0;
error2:
    osip_message_free(ack);
    ack = NULL;
error1:
    return -1;
}

/*****************************************************************************
 函 数 名  : sip_bye
 功能描述  : 发送bye消息
 输入参数  : int dialog_pos
                            char *localip
                            int localport
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月3日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int sip_bye(int dialog_pos, char* localip, int localport)
{
    osip_message_t*         bye = NULL;
    osip_transaction_t* transaction = NULL;
    sip_dialog_t* pSipDlg = NULL;
    int i = 0;
    int socket = 0;

    if (NULL == localip || localport <= 0)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_bye() exit---: Param Error \r\n");
        return -1;
    }

    if (!is_valid_dialog_index(dialog_pos))
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_bye() exit---: Dialog Index Error \r\n");
        return -1;
    }

    pSipDlg = get_dialog_sip_dialog(dialog_pos);

    if (NULL == pSipDlg)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_bye() exit---: Get SIP Dialog Error \r\n");
        return -1;
    }

    i = generating_bye(&bye, pSipDlg, localip, localport);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_bye() exit---: Generating Bye Error \r\n");
        return -1;
    }

    /* 获取socket */
    socket = get_socket_by_port(localport);

    if (socket <= 0)
    {
        osip_message_free(bye);
        bye = NULL;
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_bye() exit---: Get Socket Error \r\n");
        return -1;
    }

    i = osip_transaction_init(&transaction,
                              NICT,
                              g_recv_cell,
                              bye);

    if (0 != i)
    {
        osip_message_free(bye);
        bye = NULL;
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_bye() exit---: Transaction Init Error \r\n");
        return -1;
    }

    /* 设置transaction的socket */
    osip_transaction_set_in_socket(transaction, socket);
    osip_transaction_set_out_socket(transaction, socket);

    i = ul_sendmessage(transaction, bye);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "sip_bye(): ul_sendmessage Error \r\n");
    }

    /* at this point, the dialog is terminated! */
    /* removing it right now may be dangerous... An other transaction
    might be using this dialog??? Race conditions should be verified
    here... In the general case, we will receive a BYE from the remote
    UA... Will this BYE use (and delete) the dialog? */
    update_dialog_as_uac(dialog_pos, transaction, bye, DLG_EVENT_LOCALBYE);
    cs_timer_remove(UA_SESSION_EXPIRE, dialog_pos, NULL);
    cs_timer_remove(UAC_SEND_UPDATE, dialog_pos, NULL);

    return 0;
}

/*****************************************************************************
 函 数 名  : sip_bye2
 功能描述  : 发送带认证信息的bye消息
 输入参数  : sip_t *request
                            sip_t *resp
                            char* proxyip
                            int proxyport
                            char* localip
                            int localport
                            char* username
                            char* userpassword
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月3日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int sip_bye2(osip_message_t* request, osip_message_t* resp, char* proxyip, int proxyport, char* localip, int localport, char* username, char* userpassword)
{
    int i = 0;
    int socket = 0;
    osip_message_t*         sbye = NULL;
    osip_transaction_t* transaction = NULL;
    osip_www_authenticate_t*   www_authenticate = NULL;
    osip_proxy_authenticate_t* proxy_authenticate = NULL;

    if (request == NULL || resp == NULL || NULL == username)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_bye2() exit---: Param Error \r\n");
        return -1;
    }

    if (resp->status_code == 401)
    {
        if (! osip_list_eol(&resp->www_authenticates, 0))
        {
            www_authenticate = (osip_www_authenticate_t*)osip_list_get(&resp->www_authenticates, 0);
        }

        if (www_authenticate == NULL)
        {
            SIP_DEBUG_TRACE(LOG_DEBUG, "sip_bye2() exit---: Get WWW Authenticate Error \r\n");
            return -1;
        }
    }
    else if (resp->status_code == 407)
    {
        if (!osip_list_eol(&resp->proxy_authenticates, 0))
        {
            proxy_authenticate = (osip_proxy_authenticate_t*)osip_list_get(&resp->proxy_authenticates, 0);
        }

        if (proxy_authenticate == NULL)
        {
            SIP_DEBUG_TRACE(LOG_DEBUG, "sip_bye2() exit---: Get Proxy Authenticate Error \r\n");
            return -1;
        }
    }
    else
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_bye2() exit---: Status Code Error \r\n");
        return -1;
    }

    i = generating_request_fromrequest(request, &sbye, localip, localport);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_bye2() exit---: Generating Request From Request Error \r\n");
        return -1;
    }

    /* remove all old authorizations and proxy authorizations*/
    {
        int pos = 0;
        osip_authorization_t* authorization = NULL;
        osip_proxy_authorization_t* proxy_authorization = NULL;
        pos = 0;

        while (!osip_list_eol(&sbye->authorizations, pos))
        {
            authorization = (osip_authorization_t*)osip_list_get(&sbye->authorizations, pos);

            if (NULL == authorization)
            {
                pos++;
                continue;
            }

            osip_list_remove(&sbye->authorizations, pos);
            osip_authorization_free(authorization);
            authorization = NULL;
            pos++;
        }

        pos = 0;

        while (!osip_list_eol(&sbye->proxy_authorizations, pos))
        {
            proxy_authorization = (osip_proxy_authorization_t*)osip_list_get(&sbye->proxy_authorizations, pos);

            if (NULL == proxy_authorization)
            {
                pos++;
                continue;
            }

            osip_list_remove(&sbye->proxy_authorizations, pos);
            osip_authorization_free(proxy_authorization);
            proxy_authorization = NULL;
            pos++;
        }
    }

    if (www_authenticate != NULL)
    {
        i = request_add_authorization(sbye, www_authenticate, username, userpassword, proxyip, proxyport);

        if (i != 0)
        {
            osip_message_free(sbye);
            sbye = NULL;
            SIP_DEBUG_TRACE(LOG_DEBUG, "sip_bye2() exit---: Request Add Authorization Error \r\n");
            return -1;
        }
    }
    else
    {
        i = request_add_proxy_authorization(sbye, www_authenticate, username, userpassword);

        if (i != 0)
        {
            osip_message_free(sbye);
            sbye = NULL;
            SIP_DEBUG_TRACE(LOG_DEBUG, "sip_bye2() exit---: Request Add Proxy Authorization Error \r\n");
            return -1;
        }
    }

    /* 获取socket */
    socket = get_socket_by_port(localport);

    if (socket <= 0)
    {
        osip_message_free(sbye);
        sbye = NULL;
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_bye2() exit---: Get Socket Error \r\n");
        return -1;
    }

    if (osip_transaction_init(&transaction, NICT , g_recv_cell , sbye) != 0)
    {
        osip_message_free(sbye);
        sbye = NULL;
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_bye2() exit---: Transaction Init Error \r\n");
        return -1;
    }

    /* 设置transaction的socket */
    osip_transaction_set_in_socket(transaction, socket);
    osip_transaction_set_out_socket(transaction, socket);

    i = ul_sendmessage(transaction, sbye);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "sip_bye2(): ul_sendmessage Error \r\n");
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : sip_cancel
 功能描述  : 发送cancel消息
 输入参数  : int dialog_pos
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月3日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int sip_cancel(int dialog_pos)
{
    osip_message_t*         cancel = NULL;
    osip_transaction_t* transaction = NULL;
    sip_dialog_t* pSipDlg = NULL;
    ua_dialog_t* pUaDialog = NULL;
    int i = 0;
    int socket = 0;

    if (!is_valid_dialog_index(dialog_pos))
    {
        //SIP_DEBUG_TRACE(LOG_DEBUG, "sip_cancel() exit---: Dialog Index Error \r\n");
        return -1;
    }

    USED_UA_SMUTEX_LOCK();

    pUaDialog = ua_dialog_get2(dialog_pos);

    if (NULL == pUaDialog)
    {
        //SIP_DEBUG_TRACE(LOG_DEBUG, "sip_cancel() exit---: Get UA Dialog Error \r\n");
        USED_UA_SMUTEX_UNLOCK();
        return -1;
    }

    pSipDlg = pUaDialog->pSipDialog;

    if (NULL == pSipDlg)
    {
        //SIP_DEBUG_TRACE(LOG_DEBUG, "sip_cancel() exit---: Get SIP Dialog Error \r\n");
        USED_UA_SMUTEX_UNLOCK();
        return -1;
    }

    if (pSipDlg->state == DLG_TERMINATED)
    {
        //SIP_DEBUG_TRACE(LOG_DEBUG, "sip_cancel() exit---: SIP Dialog State Error \r\n");
        USED_UA_SMUTEX_UNLOCK();
        return -1;
    }

    if ((NULL == pSipDlg->ict_tr) || (NULL == pSipDlg->ict_tr->orig_request))
    {
        //SIP_DEBUG_TRACE(LOG_DEBUG, "sip_cancel() exit---: SIP Dialog ICT Tr Error \r\n");
        USED_UA_SMUTEX_UNLOCK();
        return -1;
    }

    i = generating_cancel(&cancel, pSipDlg->ict_tr->orig_request);

    if (i != 0)
    {
        //SIP_DEBUG_TRACE(LOG_DEBUG, "sip_cancel() exit---: Generating Cancel Error \r\n");
        USED_UA_SMUTEX_UNLOCK();
        return -1;
    }

    USED_UA_SMUTEX_UNLOCK();

    /* 获取socket */
    socket = get_socket_by_port(pUaDialog->iLocalPort);

    if (socket <= 0)
    {
        osip_message_free(cancel);
        cancel = NULL;
        //SIP_DEBUG_TRACE(LOG_DEBUG, "sip_cancel() exit---: Get Socket Error \r\n");
        return -1;
    }

    i = osip_transaction_init(&transaction,
                              NICT,
                              g_recv_cell,
                              cancel);

    if (0 != i)
    {
        osip_message_free(cancel);
        cancel = NULL;
        //SIP_DEBUG_TRACE(LOG_DEBUG, "sip_cancel() exit---: Transaction Init Error \r\n");
        return -1;
    }

    /* 设置transaction的socket */
    osip_transaction_set_in_socket(transaction, socket);
    osip_transaction_set_out_socket(transaction, socket);

    i = ul_sendmessage(transaction, cancel);

    if (i != 0)
    {
        //SIP_DEBUG_TRACE(LOG_ERROR, "sip_cancel(): ul_sendmessage Error \r\n");
    }

    update_dialog_as_uac(dialog_pos, transaction, cancel, DLG_EVENT_CANCELLED);
    return 0;
}

/*****************************************************************************
 函 数 名  : sip_update
 功能描述  : 发送update消息
 输入参数  : int dialog_pos
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年9月3日 星期二
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int sip_update(int dialog_pos)
{
    int i = 0;
    ua_dialog_t* pUaDialog = NULL;

    if (!is_valid_dialog_index(dialog_pos))
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_update() exit---: Dialog Index Error \r\n");
        return -1;
    }

    pUaDialog = ua_dialog_get(dialog_pos);

    if (NULL == pUaDialog)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_update() exit---: Get UA Dialog Error:dialog_index=%d \r\n", dialog_pos);
        return -1;
    }

    if (pUaDialog->eUiState != UI_STATE_CONNECTED)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_update() exit---: Dialog State Error:dialog_pos=%d, eUiState=%d \r\n", dialog_pos, pUaDialog->eUiState);
        return -1;
    }

    i = sip_update_within_dialog(dialog_pos, pUaDialog->strLocalIP, pUaDialog->iLocalPort, pUaDialog->iSessionExpires);

    return i;
}

/*****************************************************************************
 函 数 名  : sip_options
 功能描述  : 发送Options消息
 输入参数  : int dialog_pos
                            char *caller
                            char *callee
                            char *proxyip
                            int proxyport
                            char *localip
                            int localport
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月3日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int sip_options(int dialog_pos, char* caller, char* callee, char* proxyip, int proxyport, char* localip, int localport)
{
    osip_message_t*         options = NULL;
    osip_transaction_t* transaction = NULL;
    sip_dialog_t* pSipDlg = NULL;
    int i = 0;
    int socket = 0;
    char* body = NULL;

    if (callee == NULL)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_options() exit---: Param Error \r\n");
        return -1;
    }

    if (!is_valid_dialog_index(dialog_pos))
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_options() exit---: Dialog Index Error \r\n");
        return -1;
    }

    pSipDlg = get_dialog_sip_dialog(dialog_pos);

    if (NULL == pSipDlg)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_options() exit---: Get SIP Dialog Error \r\n");
        return -1;
    }

    if (pSipDlg != NULL)
    {
        i = generating_options_within_dialog(&options, pSipDlg, body, localip, localport);
    }
    else
    {
        i = generating_options(&options, callee, body, caller, proxyip, proxyport, localip, localport);
    }

    osip_free(body);
    body = NULL;

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_options() exit---: Generating Options Error \r\n");
        return -1;
    }

    /* 获取socket */
    socket = get_socket_by_port(localport);

    if (socket <= 0)
    {
        osip_message_free(options);
        options = NULL;
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_options() exit---: Get Socket Error \r\n");
        return -1;
    }

    i = osip_transaction_init(&transaction,
                              NICT,
                              g_recv_cell,
                              options);

    if (0 != i)
    {
        osip_message_free(options);
        options = NULL;
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_options() exit---: Transaction Init Error \r\n");
        return -1;
    }

    /* 设置transaction的socket */
    osip_transaction_set_in_socket(transaction, socket);
    osip_transaction_set_out_socket(transaction, socket);

    i = ul_sendmessage(transaction, options);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "sip_options(): ul_sendmessage Error \r\n");
    }

    if (pSipDlg)
    {
        update_dialog_as_uac(dialog_pos, transaction, options, DLG_EVENT_UPDATE);
    }

    return 0;
}

#if 0
/*****************************************************************************
 函 数 名  : sip_subscribe
 功能描述  : 发送Subscribe消息
 输入参数  : char* callid
             char* caller
             char* callee
             char* event
             int event_id
             int expires
             char* proxyip
             int proxyport
             char* localip
             int localport
             char* msg
             int msg_len
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月3日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int sip_subscribe(char* callid, char* caller, char* callee, char* event, int event_id, int expires, char* proxyip, int proxyport, char* localip, int localport, char* msg, int msg_len)
{
    osip_message_t* message = NULL;
    osip_transaction_t* transaction = NULL;
    int i = 0;
    int socket = 0;
    char pcEventParam[32] = {0};
    char pcExpiresParam[32] = {0};

    i = generating_subscribe(&message, caller, callee, callid, msg, msg_len, proxyip, proxyport, localip, localport);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "sip_subscribe() exit---: Generating Message Error \r\n");
        return EV9000_SIPSTACK_MESSAGE_GENERA_ERROR;
    }

    snprintf(pcEventParam, 32, "%s;id=%d", event, event_id);
    msg_setevent(message, pcEventParam);

    snprintf(pcExpiresParam, 32, "%d", expires);
    msg_set_expires(message, pcExpiresParam);

    /* 获取socket */
    socket = get_socket_by_port(localport);

    if (socket <= 0)
    {
        osip_message_free(message);
        message = NULL;
        SIP_DEBUG_TRACE(LOG_ERROR, "sip_subscribe() exit---: Get Socket Error \r\n");
        return EV9000_SIPSTACK_GET_SOCKET_ERROR;
    }

    i = osip_transaction_init(&transaction,
                              NICT,
                              g_recv_cell,
                              message);

    if (0 != i)
    {
        osip_message_free(message);
        message = NULL;
        SIP_DEBUG_TRACE(LOG_ERROR, "sip_subscribe() exit---: Transaction Init Error \r\n");
        return EV9000_SIPSTACK_TRANSACTION_INIT_ERROR;
    }

    /* 设置transaction的socket */
    osip_transaction_set_in_socket(transaction, socket);
    osip_transaction_set_out_socket(transaction, socket);

    i = ul_sendmessage(transaction, message);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "sip_subscribe(): ul_sendmessage Error \r\n");
        return EV9000_SIPSTACK_SEND_MESSAGE_ERROR;
    }

    return i;
}
#endif

/*****************************************************************************
 函 数 名  : sip_subscribe2
 功能描述  : 发送Subscribe消息
 输入参数  : int dialog_pos
             char* callid
             char* event
             int event_id
             int expires
             char* body
             int body_len
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月3日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int sip_subscribe(int dialog_pos, char* callid, char* event, int event_id, int expires, char* body, int body_len)
{
    int i = 0;
    osip_message_t* subscribe = NULL;
    osip_transaction_t* transaction = NULL;

    sip_dialog_t* dialog = NULL;
    sip_subscription_t* sip_sub = NULL;
    ua_dialog_t* pUaDialog = NULL;

    int socket = 0;
    char pcEventParam[32] = {0};
    char pcExpiresParam[32] = {0};
    char pcEventID[32] = {0};

    if ((NULL == callid) || (NULL == event))
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "sip_subscribe() exit---: Param Error \r\n");
        return EV9000_SIPSTACK_PARAM_ERROR;
    }

    if (!is_valid_dialog_index(dialog_pos))
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "sip_subscribe() exit---: Dialog Index Error \r\n");
        return -1;
    }

    pUaDialog = ua_dialog_get(dialog_pos);

    if (NULL == pUaDialog)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "sip_subscribe() exit---: Get UA Dialog Error:dialog_index=%d \r\n", dialog_pos);
        return -1;
    }

    i = generating_subscribe(&subscribe, pUaDialog->strCallerID, pUaDialog->strCalleeID, callid, body, body_len, pUaDialog->strRemoteIP, pUaDialog->iRemotePort, pUaDialog->strLocalIP, pUaDialog->iLocalPort);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "sip_subscribe() exit---: generating_subscribe Error:dialog_index=%d \r\n", dialog_pos);
        return -1;
    }

    snprintf(pcEventParam, 32, "%s;id=%d", event, event_id);
    msg_setevent(subscribe, pcEventParam);

    if (expires > 0)
    {
        snprintf(pcExpiresParam, 32, "%d", expires);
    }
    else
    {
        snprintf(pcExpiresParam, 32, "%d", MIN_SUB_EXPIRE);
    }

    msg_set_expires(subscribe, pcExpiresParam);

    /* 获取socket */
    socket = get_socket_by_port(pUaDialog->iLocalPort);

    if (socket <= 0)
    {
        osip_message_free(subscribe);
        subscribe = NULL;
        SIP_DEBUG_TRACE(LOG_ERROR, "sip_subscribe() exit---: Get Socket Error \r\n");
        return EV9000_SIPSTACK_GET_SOCKET_ERROR;
    }

    i = osip_transaction_init(&transaction,
                              NICT,
                              g_recv_cell,
                              subscribe);

    if (0 != i)
    {
        osip_message_free(subscribe);
        subscribe = NULL;
        SIP_DEBUG_TRACE(LOG_ERROR, "sip_subscribe() exit---: Transaction Init Error \r\n");
        return EV9000_SIPSTACK_TRANSACTION_INIT_ERROR;
    }

    /* 设置transaction的socket */
    osip_transaction_set_in_socket(transaction, socket);
    osip_transaction_set_out_socket(transaction, socket);


    sip_dialog_init_as_uac(&dialog, transaction, subscribe);
    pUaDialog->pSipDialog = dialog;

    i = sip_subscription_init(&sip_sub);

    if (i != 0)
    {
        osip_message_free(subscribe);
        subscribe = NULL;
        SIP_DEBUG_TRACE(LOG_ERROR, "sip_subscribe() exit---: sip_subscription_init Error \r\n");
        return EV9000_SIPSTACK_INVITE_GET_SIPDLG_ERROR;
    }

    sip_sub->state = SUB_STATE_PRE;
    sip_sub->event_type = osip_getcopy(event);

    snprintf(pcEventID, 32, "%d", event_id);
    sip_sub->id_param = osip_getcopy(pcEventID);

    sip_sub->begin = time(NULL);

    if (expires > 0)
    {
        sip_sub->duration = expires;
    }
    else
    {
        sip_sub->duration = MIN_SUB_EXPIRE;
    }

    osip_uri_clone(subscribe->to->url, &sip_sub->remote_contact_uri);

    AddDialogSubscription(dialog_pos, sip_sub);

    i = ul_sendmessage(transaction, subscribe);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "sip_subscribe(): ul_sendmessage Error \r\n");
        return EV9000_SIPSTACK_SEND_MESSAGE_ERROR;
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : sip_notify
 功能描述  : 发送Notify消息
 输入参数  : char* callid
             char* caller
             char* callee
             char* event
             int event_id
             int expires
             char* proxyip
             int proxyport
             char* localip
             int localport
             char* msg
             int msg_len
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月3日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int sip_notify(char* callid, char* caller, char* callee, char* event, int event_id, int expires, char* proxyip, int proxyport, char* localip, int localport, char* msg, int msg_len)
{
    osip_message_t* message = NULL;
    osip_transaction_t* transaction = NULL;
    int i = 0;
    int socket = 0;
    char pcEventParam[32] = {0};
    char pcExpiresParam[32] = {0};

    i = generating_notify(&message, caller, callee, callid, msg, msg_len, proxyip, proxyport, localip, localport);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "sip_notify() exit---: Generating Message Error \r\n");
        return EV9000_SIPSTACK_MESSAGE_GENERA_ERROR;
    }

    snprintf(pcEventParam, 32, "%s;id=%d", event, event_id);
    msg_setevent(message, pcEventParam);

    snprintf(pcExpiresParam, 32, "%d", expires);
    msg_set_expires(message, pcExpiresParam);
    msg_setsubscription_state(message, (char*)"active");

    /* 获取socket */
    socket = get_socket_by_port(localport);

    if (socket <= 0)
    {
        osip_message_free(message);
        message = NULL;
        SIP_DEBUG_TRACE(LOG_ERROR, "sip_notify() exit---: Get Socket Error \r\n");
        return EV9000_SIPSTACK_GET_SOCKET_ERROR;
    }

    i = osip_transaction_init(&transaction,
                              NICT,
                              g_recv_cell,
                              message);

    if (0 != i)
    {
        osip_message_free(message);
        message = NULL;
        SIP_DEBUG_TRACE(LOG_ERROR, "sip_notify() exit---: Transaction Init Error \r\n");
        return EV9000_SIPSTACK_TRANSACTION_INIT_ERROR;
    }

    /* 设置transaction的socket */
    osip_transaction_set_in_socket(transaction, socket);
    osip_transaction_set_out_socket(transaction, socket);

    i = ul_sendmessage(transaction, message);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "sip_notify(): ul_sendmessage Error \r\n");
        return EV9000_SIPSTACK_SEND_MESSAGE_ERROR;
    }

    return i;
}

/*****************************************************************************
 函 数 名  : sip_notify_for_tcp
 功能描述  : 基于TCP发送Notify消息
 输入参数  : char* callid
             char* caller
             char* callee
             char* event
             int event_id
             int expires
             char* proxyip
             int proxyport
             char* localip
             int localport
             char* msg
             int msg_len
             int tcp_socket
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年2月21日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int sip_notify_for_tcp(char* callid, char* caller, char* callee, char* event, int event_id, int expires, char* proxyip, int proxyport, char* localip, int localport, char* msg, int msg_len, int tcp_socket)
{
    osip_message_t* message = NULL;
    int i = 0;
    char pcEventParam[32] = {0};
    char pcExpiresParam[32] = {0};

    i = generating_notify_for_tcp(&message, caller, callee, callid, msg, msg_len, proxyip, proxyport, localip, localport);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "sip_notify_for_tcp() exit---: generating_notify_for_tcp Error \r\n");
        return EV9000_SIPSTACK_MESSAGE_GENERA_ERROR;
    }

    snprintf(pcEventParam, 32, "%s;id=%d", event, event_id);
    msg_setevent(message, pcEventParam);

    snprintf(pcExpiresParam, 32, "%d", expires);
    msg_set_expires(message, pcExpiresParam);
    msg_setsubscription_state(message, (char*)"active");

    i = tl_sendmessage_by_tcp(message, proxyip, proxyport, tcp_socket);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "sip_notify_for_tcp(): tl_sendmessage_by_tcp Error \r\n");
        return EV9000_SIPSTACK_SEND_MESSAGE_ERROR;
    }

    return i;
}

int sip_notify_within_dialog(int dialog_pos, char* body, int body_len)
{
    int i = 0;
    int socket = 0;
    osip_message_t*         notify = NULL;
    osip_transaction_t* transaction = NULL;
    sip_dialog_t* pSipDlg = NULL;
    sip_subscription_t* sip_sub = NULL;
    ua_dialog_t* pUaDialog = NULL;

    if (!is_valid_dialog_index(dialog_pos))
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "sip_notify_within_dialog() exit---: Dialog Index Error \r\n");
        return -1;
    }

    pUaDialog = ua_dialog_get(dialog_pos);

    if (NULL == pUaDialog)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "sip_notify_within_dialog() exit---: Get UA Dialog Error:dialog_index=%d \r\n", dialog_pos);
        return -1;
    }

    pSipDlg = get_dialog_sip_dialog(dialog_pos);

    if (pSipDlg == NULL)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "sip_notify_within_dialog() exit---: Get SIP Dialog Error \r\n");
        return -1;
    }

    sip_sub = GetDialogSubscription(dialog_pos);

    if (sip_sub == NULL)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "sip_notify_within_dialog() exit---: GetDialogSubscription Error \r\n");
        return -1;
    }

    i = generating_notify_within_dialog(&notify, pSipDlg, sip_sub, body, body_len, pUaDialog->strCalleeID, pUaDialog->strLocalIP, pUaDialog->iLocalPort);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "sip_notify_within_dialog() exit---: generating_notify_within_dialog Error \r\n");
        return -1;
    }

    /* 获取socket */
    socket = get_socket_by_port(pUaDialog->iLocalPort);

    if (socket <= 0)
    {
        osip_message_free(notify);
        notify = NULL;
        SIP_DEBUG_TRACE(LOG_ERROR, "sip_notify_within_dialog() exit---: Get Socket Error \r\n");
        return -1;
    }

    i = osip_transaction_init(&transaction,
                              NICT,
                              g_recv_cell,
                              notify);

    if (0 != i)
    {
        osip_message_free(notify);
        notify = NULL;
        SIP_DEBUG_TRACE(LOG_ERROR, "sip_notify_within_dialog() exit---: Transaction Init Error \r\n");
        return -1;
    }

    /* 设置transaction的socket */
    osip_transaction_set_in_socket(transaction, socket);
    osip_transaction_set_out_socket(transaction, socket);

    i = ul_sendmessage(transaction, notify);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "sip_notify_within_dialog(): ul_sendmessage Error \r\n");
    }

    update_dialog_as_uac(dialog_pos, transaction, notify, DLG_EVENT_UPDATE);

    return 0;
}


/*****************************************************************************
 函 数 名  : sip_update_within_dialog
 功能描述  : 发送会话内UPDATE 消息
 输入参数  : int dialog_pos
                            char *localip
                            int localport
                            int session_expires
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月3日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int sip_update_within_dialog(int dialog_pos, char* localip, int localport, int session_expires)
{
    int i = 0;
    int socket = 0;
    osip_message_t*         update = NULL;
    osip_transaction_t* transaction = NULL;
    sip_dialog_t* pSipDlg = NULL;
    char pcSessionExpiresParam[32] = {0};

    if (!is_valid_dialog_index(dialog_pos))
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_update_within_dialog() exit---: Dialog Index Error \r\n");
        return -1;
    }

    pSipDlg = get_dialog_sip_dialog(dialog_pos);

    if (pSipDlg == NULL)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_update_within_dialog() exit---: Get SIP Dialog Error \r\n");
        return -1;
    }

    i = generating_update_within_dialog(&update, pSipDlg, localip, localport);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_update_within_dialog() exit---: Generating Update Within Dialog Error \r\n");
        return -1;
    }

    /* 增加会话刷新参数 */
    if (session_expires > 0)
    {
        osip_message_set_supported(update, (char*)"timer");
        snprintf(pcSessionExpiresParam, 32, "%d;refresher=uac", session_expires);
        msg_set_session_expires(update, pcSessionExpiresParam);
    }

    /* 获取socket */
    socket = get_socket_by_port(localport);

    if (socket <= 0)
    {
        osip_message_free(update);
        update = NULL;
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_update_within_dialog() exit---: Get Socket Error \r\n");
        return -1;
    }

    i = osip_transaction_init(&transaction,
                              NICT,
                              g_recv_cell,
                              update);

    if (0 != i)
    {
        osip_message_free(update);
        update = NULL;
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_update_within_dialog() exit---: Transaction Init Error \r\n");
        return -1;
    }

    /* 设置transaction的socket */
    osip_transaction_set_in_socket(transaction, socket);
    osip_transaction_set_out_socket(transaction, socket);

    i = ul_sendmessage(transaction, update);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "sip_update_within_dialog(): ul_sendmessage Error \r\n");
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : sip_message_within_dialog
 功能描述  : 发送会话内Message消息
 输入参数  : int dialog_pos
                            char* msg
                            int msg_len
                            char *localip
                            int localport
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月3日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int sip_message_within_dialog(int dialog_pos, char* msg, int msg_len, char* localip, int localport)
{
    int i = 0;
    int socket = 0;
    osip_message_t* message = NULL;
    osip_transaction_t* transaction = NULL;
    sip_dialog_t* pSipDlg = NULL;

    if (!is_valid_dialog_index(dialog_pos))
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_message_within_dialog() exit---: Dialog Index Error \r\n");
        return -1;
    }

    pSipDlg = get_dialog_sip_dialog(dialog_pos);

    if (pSipDlg == NULL)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_message_within_dialog() exit---: Get SIP Dialog Error \r\n");
        return -1;
    }

    i = generating_message_within_dialog(&message, pSipDlg, msg, msg_len, localip, localport);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_message_within_dialog() exit---: Generating Message Within Dialog Error \r\n");
        return -1;
    }

    /* 获取socket */
    socket = get_socket_by_port(localport);

    if (socket <= 0)
    {
        osip_message_free(message);
        message = NULL;
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_message_within_dialog() exit---: Get Socket Error \r\n");
        return -1;
    }

    i = osip_transaction_init(&transaction,
                              NICT,
                              g_recv_cell,
                              message);

    if (0 != i)
    {
        osip_message_free(message);
        message = NULL;
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_message_within_dialog() exit---: Transaction Init Error \r\n");
        return -1;
    }

    /* 设置transaction的socket */
    osip_transaction_set_in_socket(transaction, socket);
    osip_transaction_set_out_socket(transaction, socket);

    i = ul_sendmessage(transaction, message);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "sip_message_within_dialog(): ul_sendmessage Error \r\n");
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : sip_info_within_dialog
 功能描述  : 发送会话内Info消息
 输入参数  : int dialog_pos
                            char* body
                            int body_len
                            char *localip
                            int localport
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月3日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int sip_info_within_dialog(int dialog_pos, char* body, int body_len, char* localip, int localport)
{
    int i = 0;
    int socket = 0;
    osip_message_t*         info = NULL;
    osip_transaction_t* transaction = NULL;
    sip_dialog_t* pSipDlg = NULL;

    if (!is_valid_dialog_index(dialog_pos))
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "sip_info_within_dialog() exit---: Dialog Index Error \r\n");
        return -1;
    }

    pSipDlg = get_dialog_sip_dialog(dialog_pos);

    if (pSipDlg == NULL)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "sip_info_within_dialog() exit---: Get SIP Dialog Error \r\n");
        return -1;
    }

    i = generating_info_within_dialog(&info, pSipDlg, body, body_len, localip, localport);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "sip_info_within_dialog() exit---: Generating Info Within Dialog Error \r\n");
        return -1;
    }

    /* 获取socket */
    socket = get_socket_by_port(localport);

    if (socket <= 0)
    {
        osip_message_free(info);
        info = NULL;
        SIP_DEBUG_TRACE(LOG_ERROR, "sip_info_within_dialog() exit---: Get Socket Error \r\n");
        return -1;
    }

    i = osip_transaction_init(&transaction,
                              NICT,
                              g_recv_cell,
                              info);

    if (0 != i)
    {
        osip_message_free(info);
        info = NULL;
        SIP_DEBUG_TRACE(LOG_ERROR, "sip_info_within_dialog() exit---: Transaction Init Error \r\n");
        return -1;
    }

    /* 设置transaction的socket */
    osip_transaction_set_in_socket(transaction, socket);
    osip_transaction_set_out_socket(transaction, socket);

    i = ul_sendmessage(transaction, info);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "sip_info_within_dialog(): ul_sendmessage Error \r\n");
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : sip_2xx_answer_to_invite
 功能描述  : 生成INVITE的2xx回应消息
 输入参数  : int dialog_pos
             char* caller
             char* localip
             int localport
             int session_expires
             sdp_t* local_sdp
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月7日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int sip_2xx_answer_to_invite(int dialog_pos, char* callee, char* localip, int localport, int session_expires, sdp_message_t* local_sdp)
{
    osip_message_t* response = NULL;
    osip_message_t* response_copy = NULL;
    int i = 0;
    char* size = NULL;
    char* body = NULL;
    sip_dialog_t*  pSipDlg = NULL;
    osip_transaction_t* tr = NULL;
    char pcSessionExpiresParam[32] = {0};

    if (!is_valid_dialog_index(dialog_pos))
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_2xx_answer_to_invite() exit---: Dialog Index Error \r\n");
        return EV9000_SIPSTACK_INVITE_GET_UA_ERROR;
    }

    pSipDlg = get_dialog_sip_dialog(dialog_pos);

    if (NULL == pSipDlg)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_2xx_answer_to_invite() exit---: Get SIP Dialog Error \r\n");
        return EV9000_SIPSTACK_INVITE_GET_UA_ERROR;
    }

    tr = pSipDlg->ist_tr;

    if (tr == NULL)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_2xx_answer_to_invite() exit---: SIP Dialog IST Tr Error \r\n");
        return EV9000_SIPSTACK_GET_TRANSACTION_ERROR;
    }

    sdp_message_to_str(local_sdp, &body);

    if (body == NULL)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_2xx_answer_to_invite() exit---: SDP 2 Char Error \r\n");
        return EV9000_SIPSTACK_SDP_TO_STR_ERROR;
    }

    i = generating_response_default(&response, pSipDlg, 200, tr->orig_request, NULL);

    if (i != 0)
    {
        osip_free(body); /* not used */
        body = NULL;
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_2xx_answer_to_invite() exit---: Generating Default Response Error \r\n");
        return EV9000_SIPSTACK_INVITE_GENERA_ERROR;
    }

    /* 如果支持会话刷新，则增加会话刷新参数 */
    if (session_expires > 0)
    {
        osip_message_set_require(response, (char*)"timer");
        osip_message_set_supported(response, (char*)"timer");
        snprintf(pcSessionExpiresParam, 32, "%d;refresher=uac", session_expires);
        msg_set_session_expires(response, pcSessionExpiresParam);
    }

    i = osip_message_set_body(response, body, strlen(body));

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_2xx_answer_to_invite() exit---: Message Set Body Error \r\n");
        goto error_1;
    }

    size = (char*) osip_malloc(8 * sizeof(char));

    if (NULL == size)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_2xx_answer_to_invite() exit---: Malloc Error \r\n");
        goto error_1;
    }

    snprintf(size, 8 * sizeof(char), "%d", (int)strlen(body));
    i = osip_message_set_content_length(response, size);
    osip_free(size);
    size = NULL;

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_2xx_answer_to_invite() exit---: Message Set Content Length Error \r\n");
        goto error_1;
    }

    i = osip_message_set_header(response, (char*)"content-type", (char*)"Application/SDP");

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_2xx_answer_to_invite() exit---: Message Set Content Type Header Error \r\n");
        goto error_1;
    }

    /* request that estabish a dialog: */
    /* 12.1.1 UAS Behavior */
    i = complete_answer_that_establish_a_dialog(response, tr->orig_request, callee, localip, localport);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_2xx_answer_to_invite() exit---: Complete Answer That Establish a Dialog Error \r\n");
        goto error_1;    /* ?? */
    }

    /* response should contains the allow and supported headers */
    osip_message_set_allow(response, (char*)"INVITE");
    osip_message_set_allow(response, (char*)"ACK");
    osip_message_set_allow(response, (char*)"OPTIONS");
    osip_message_set_allow(response, (char*)"CANCEL");
    osip_message_set_allow(response, (char*)"BYE");
    osip_message_set_allow(response, (char*)"REFER");

    osip_free(body);
    body = NULL;

    update_dialog_as_uas(dialog_pos, tr, response, DLG_EVENT_2XX);
    osip_message_clone(response, &response_copy);

    i = ul_sendmessage(tr, response);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "sip_2xx_answer_to_invite(): ul_sendmessage Error \r\n");
        return EV9000_SIPSTACK_SEND_MESSAGE_ERROR;
    }

    ua_timer_use(UA_ACK2XX_RETRANSMIT, dialog_pos, tr, response_copy);

    if (session_expires > 0)
    {
        cs_timer_use(UA_SESSION_EXPIRE, dialog_pos, NULL);
    }

    return 0;

error_1:
    osip_free(body);
    body = NULL;
    osip_message_free(response);
    response = NULL;
    return -1;
}

/*****************************************************************************
 函 数 名  : sip_3456xxxx_answer_to_invite
 功能描述  : 生成INVITE的3456xx回应消息
 输入参数  : int dialog_pos
                            int code
                            char* reasonphrase
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月7日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int sip_3456xxxx_answer_to_invite(int dialog_pos, int code, char* reasonphrase)
{
    int i = 0;
    osip_message_t* response = NULL;
    sip_dialog_t*    pSipDlg = NULL;
    osip_transaction_t* tr = NULL;

    if (!is_valid_dialog_index(dialog_pos))
    {
        //SIP_DEBUG_TRACE(LOG_DEBUG, "sip_3456xxxx_answer_to_invite() exit---: Dialog Index Error \r\n");
        return -1;
    }

    USED_UA_SMUTEX_LOCK();

    pSipDlg = get_dialog_sip_dialog2(dialog_pos);

    if (NULL == pSipDlg)
    {
        //SIP_DEBUG_TRACE(LOG_DEBUG, "sip_3456xxxx_answer_to_invite() exit---: Get SIP Dialog Error \r\n");
        USED_UA_SMUTEX_UNLOCK();
        return EV9000_SIPSTACK_INVITE_GET_UA_ERROR;
    }

    tr = pSipDlg->ist_tr;

    if (tr == NULL)
    {
        //SIP_DEBUG_TRACE(LOG_DEBUG, "sip_3456xxxx_answer_to_invite() exit---: SIP Dialog IST Tr Error \r\n");
        USED_UA_SMUTEX_UNLOCK();
        return EV9000_SIPSTACK_GET_TRANSACTION_ERROR;
    }

    i = generating_response_default(&response, pSipDlg, code, tr->orig_request, reasonphrase);

    if (i != 0)
    {
        //SIP_DEBUG_TRACE(LOG_DEBUG, "sip_3456xxxx_answer_to_invite() exit---: Generating Default Response Error \r\n");
        USED_UA_SMUTEX_UNLOCK();
        return EV9000_SIPSTACK_INVITE_GENERA_ERROR;
    }

    osip_message_set_content_length(response, (char*)"0");
    update_dialog_as_uas2(pSipDlg, tr, response, DLG_EVENT_REJECTED);

    USED_UA_SMUTEX_UNLOCK();

    i = ul_sendmessage(tr, response);

    if (i != 0)
    {
        return EV9000_SIPSTACK_SEND_MESSAGE_ERROR;
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : sip_answer_to_options
 功能描述  : 应答Options消息
 输入参数  : int dialog_pos
                            transaction_t *tr
                            int code
                            char *localip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月7日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void sip_answer_to_options(int dialog_pos, osip_transaction_t* tr, int code, char* localip)
{
    sip_dialog_t* pSipDlg = NULL;

    if (NULL == tr)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_answer_to_options() exit---: Param Error \r\n");
        return;
    }

    if (!is_valid_dialog_index(dialog_pos))
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_answer_to_options() exit---: Dialog Index Error \r\n");
        return;
    }

    pSipDlg = get_dialog_sip_dialog(dialog_pos);

    if (NULL == pSipDlg)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_answer_to_options() exit---: Get SIP Dialog Error \r\n");
        return;
    }

    if (code < 200)
    {
        generating_1xx_answer_to_options(pSipDlg, tr, code);
    }
    else if ((200 <= code) && (code <= 299))
    {
        generating_2xx_answer_to_options(pSipDlg, tr, code, localip);
    }
    else if (code > 299)
    {
        generating_3456xx_answer_to_options(pSipDlg, tr, code);
    }

    return;
}

/*****************************************************************************
 函 数 名  : sip_answer_to_refer
 功能描述  : 应答Refer消息
 输入参数  : int dialog_pos
                            transaction_t *tr
                            int code
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月7日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void sip_answer_to_refer(int dialog_pos, osip_transaction_t* tr, int code)
{
    sip_response_default(dialog_pos, tr, code, NULL);
    return;
}

/*****************************************************************************
 函 数 名  : sip_answer_to_info
 功能描述  : 应答Info消息
 输入参数  : int dialog_pos
                            transaction_t *tr
                            int code
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月7日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void sip_answer_to_info(int dialog_pos, osip_transaction_t* tr, int code)
{
    sip_dialog_t* pSipDlg = NULL;
    osip_message_t* response = NULL;
    int i = 0;

    if (tr == NULL)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_answer_to_info() exit---: Param Error \r\n");
        return;
    }

    if (!is_valid_dialog_index(dialog_pos))
    {
        pSipDlg = NULL;
    }
    else
    {
        pSipDlg = get_dialog_sip_dialog(dialog_pos);
    }

    i = generating_response_default(&response, pSipDlg, code, tr->orig_request, NULL);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "sip_answer_to_info() Generating Default Response Error \r\n");
    }
    else
    {
        //i = info_add_body(&response, pSipDlg);//yanghf 2005/03/07
        osip_message_set_content_length(response, (char*)"0");
        i = ul_sendmessage(tr, response);

        if (i != 0)
        {
            SIP_DEBUG_TRACE(LOG_ERROR, "sip_answer_to_info(): ul_sendmessage Error \r\n");
        }
    }

    return;
}

/*****************************************************************************
 函 数 名  : sip_answer_to_update
 功能描述  : 应答Update 消息
 输入参数  : int dialog_pos
                            transaction_t *tr
                            int code
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月7日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void sip_answer_to_update(int dialog_pos, osip_transaction_t* tr, int code, int session_expires)
{
    sip_dialog_t* pSipDlg = NULL;
    osip_message_t* response = NULL;
    int i = 0;
    char pcSessionExpiresParam[32] = {0};

    if (tr == NULL)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_answer_to_update() exit---: Param Error \r\n");
        return;
    }

    if (!is_valid_dialog_index(dialog_pos))
    {
        pSipDlg = NULL;
    }
    else
    {
        pSipDlg = get_dialog_sip_dialog(dialog_pos);
    }

    i = generating_response_default(&response, pSipDlg, code, tr->orig_request, NULL);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "sip_answer_to_info() Generating Default Response Error \r\n");
    }
    else
    {
        if (session_expires > 0)
        {
            osip_message_set_require(response, (char*)"timer");
            //msg_setsupported(response, (char*)"timer");
            snprintf(pcSessionExpiresParam, 32, "%d;refresher=uac", session_expires);
            msg_set_session_expires(response, pcSessionExpiresParam);
        }

        osip_message_set_content_length(response, (char*)"0");
        i = ul_sendmessage(tr, response);

        if (i != 0)
        {
            SIP_DEBUG_TRACE(LOG_ERROR, "sip_answer_to_info() ul_sendmessage Error \r\n");
        }
    }

    return;
}

/*****************************************************************************
 函 数 名  : sip_answer_to_bye
 功能描述  : 应答Bye消息
 输入参数  : int dialog_pos
                            transaction_t *tr
                            int code
                            char* reasonphrase
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月7日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void sip_answer_to_bye(int dialog_pos, osip_transaction_t* tr, int code, char* reasonphrase)
{
    sip_dialog_t* pSipDlg = NULL;
    osip_message_t* response;
    int i;

    if (NULL == tr)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_answer_to_bye() exit---: Param Error \r\n");
        return;
    }

    if (!is_valid_dialog_index(dialog_pos))
    {
        pSipDlg = NULL;
    }
    else
    {
        pSipDlg = get_dialog_sip_dialog(dialog_pos);
    }

    if (pSipDlg == NULL)
    {
        code = 481; /* dialog does not exist */
    }

    i = generating_response_default(&response, pSipDlg, code, tr->orig_request, reasonphrase);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "generating_response_default() Generating Default Response Error \r\n");
    }
    else
    {
        osip_message_set_content_length(response, (char*)"0");

        i = ul_sendmessage(tr, response);

        if (i != 0)
        {
            SIP_DEBUG_TRACE(LOG_ERROR, "generating_response_default(): ul_sendmessage Error \r\n");
        }
    }

    return;
}

/*****************************************************************************
 函 数 名  : sip_answer_to_cancel
 功能描述  : 应答Cancel消息
 输入参数  : int dialog_pos
                            transaction_t *tr
                            int code
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月7日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void sip_answer_to_cancel(int dialog_pos, osip_transaction_t* tr, int code)
{
    if (NULL == tr)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_answer_to_cancel() exit---: Param Error \r\n");
        return;
    }

    return;
}

/*****************************************************************************
 函 数 名  : sip_response_default
 功能描述  : 产生默认的消息回应
 输入参数  : int dialog_pos
                            transaction_t *tr
                            int code
                            char* reasonphrase
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月7日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void sip_response_default(int dialog_pos, osip_transaction_t* tr, int code, char* reasonphrase)
{
    sip_dialog_t* pSipDlg = NULL;
    osip_message_t* response = NULL;
    int i = 0;

    if (tr == NULL)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_response_default() exit---: Param Error \r\n");
        return;
    }

    if (!is_valid_dialog_index(dialog_pos))
    {
        pSipDlg = NULL;
    }
    else
    {
        pSipDlg = get_dialog_sip_dialog(dialog_pos);
    }

    i = generating_response_default(&response, pSipDlg, code, tr->orig_request, reasonphrase);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "sip_response_default() Generating Default Response Error \r\n");
    }
    else
    {
        osip_message_set_content_length(response, (char*)"0");
        i = ul_sendmessage(tr, response);

        if (i != 0)
        {
            SIP_DEBUG_TRACE(LOG_ERROR, "sip_response_default(): ul_sendmessage Error \r\n");
        }
    }

    return;
}
#endif

#if DECS("对外接口")
/*****************************************************************************
 函 数 名  : SIP_SendRegister
 功能描述  : 发送注册消息
 输入参数  : char* service_id
                            char* local_id
                            char* local_ip
                            int local_port
                            char* server_ip
                            int server_port
                            char* username
                            char* userpassword
                            int expires
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年5月23日 星期四
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int SIP_SendRegister(char* service_id, char* local_id, char* local_ip, int local_port, char* server_ip, int server_port, char* username, char* userpassword, int expires)
{
    int i = 0;
    int index = 0;
    uac_reg_info_t* pUacRegInfo = NULL;

    SIP_DEBUG_TRACE(LOG_INFO,  "SIP_SendRegister() \
    \r\n In Para: \
    \r\n service_id=%s \
    \r\n local_id=%s \
    \r\n local_ip=%s \
    \r\n local_port=%d \
    \r\n server_ip=%s \
    \r\n server_port=%d \
    \r\n username=%s \
    \r\n userpassword=%s \
    \r\n expires=%d \
    \r\n ", service_id, local_id, local_ip, local_port, server_ip, server_port, username, userpassword, expires);

    if ((NULL == service_id) || (service_id[0] == '\0')
        || (NULL == server_ip) || (server_ip[0] == '\0') || (server_port <= 0)
        || (NULL == local_id) || (local_id[0] == '\0')
        || (NULL == local_ip) || (local_ip[0] == '\0') || (local_port <= 0))
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "SIP_SendRegister() exit---: Param Error \r\n");
        return EV9000_SIPSTACK_PARAM_ERROR;
    }

    index = uac_reginfo_find_by_server_and_local_info(service_id, server_ip, server_port, local_id, local_ip, local_port);
    SIP_DEBUG_TRACE(LOG_INFO, "SIP_SendRegister() uac_reginfo_find_by_server_and_local_info:service_id=%s, server_ip=%s, server_port=%d, local_id=%s, local_ip=%s, local_port=%d, index=%d \r\n", service_id, server_ip, server_port, local_id, local_ip, local_port, index);

    if (index >= 0) /* 已经存在 */
    {
        pUacRegInfo = uac_reginfo_get(index);

        if (NULL == pUacRegInfo)
        {
            SIP_DEBUG_TRACE(LOG_ERROR, "SIP_SendRegister() exit---: Get UAC Register Info Error \r\n");
            return EV9000_SIPSTACK_REGISTER_GET_UAC_ERROR;
        }

        /* 上次的注册还没结束 */
        if (0 == pUacRegInfo->isReg && 1 == pUacRegInfo->isReging)
        {
            SIP_DEBUG_TRACE(LOG_ERROR, "SIP_SendRegister() exit---: Last Register Proc Not Complete \r\n");
            return EV9000_SIPSTACK_REGISTER_GET_UAC_ERROR;
        }

        /* 注册账户 */
        if ('\0' != pUacRegInfo->register_account[0])
        {
            if (NULL != username)
            {
                if (0 != sstrcmp(pUacRegInfo->register_account, username))
                {
                    memset(pUacRegInfo->register_account, 0, 132);
                    osip_strncpy(pUacRegInfo->register_account, username, 128);
                }
            }
            else
            {
                memset(pUacRegInfo->register_account, 0, 132);
            }
        }
        else
        {
            if (NULL != username)
            {
                osip_strncpy(pUacRegInfo->register_account, username, 128);
            }
        }

        /* 注册密码 */
        if ('\0' != pUacRegInfo->register_password[0])
        {
            if (NULL != userpassword)
            {
                if (0 != sstrcmp(pUacRegInfo->register_password, userpassword))
                {
                    memset(pUacRegInfo->register_password, 0, 132);
                    osip_strncpy(pUacRegInfo->register_password, userpassword, 128);
                }
            }
            else
            {
                memset(pUacRegInfo->register_password, 0, 132);
            }
        }
        else
        {
            if (NULL != userpassword)
            {
                osip_strncpy(pUacRegInfo->register_password, userpassword, 128);
            }
        }

        pUacRegInfo->register_cseq_number++;

        if (0 == expires)
        {
            i = sip_register(pUacRegInfo->register_id, pUacRegInfo->register_id, pUacRegInfo->proxy_id, pUacRegInfo->proxyip, pUacRegInfo->proxyport, pUacRegInfo->localip, pUacRegInfo->localport, expires, pUacRegInfo->register_callid_number, pUacRegInfo->register_cseq_number, pUacRegInfo->link_type);

            if (i != 0)
            {
                uac_reginfo_remove(index);
                SIP_DEBUG_TRACE(LOG_ERROR, "SIP_SendRegister() exit---: SIP Register Error \r\n");
                return i;
            }

            pUacRegInfo->isReg = 0;
            pUacRegInfo->isReging = 1;
            pUacRegInfo->expires = 0;

            SIP_DEBUG_TRACE(LOG_INFO, "SIP_SendRegister() Remove:register_id=%s, index=%d \r\n", pUacRegInfo->register_id, index);
            return i;
        }
        else if (expires > 0)
        {
            i = sip_register(pUacRegInfo->register_id, pUacRegInfo->register_id, pUacRegInfo->proxy_id, pUacRegInfo->proxyip, pUacRegInfo->proxyport, pUacRegInfo->localip, pUacRegInfo->localport, expires, pUacRegInfo->register_callid_number, pUacRegInfo->register_cseq_number, pUacRegInfo->link_type);

            if (i != 0)
            {
                uac_reginfo_remove(index);
                SIP_DEBUG_TRACE(LOG_ERROR, "SIP_SendRegister() exit---: SIP Register Error \r\n");
                return i;
            }

            pUacRegInfo->isReg = 0;
            pUacRegInfo->isReging = 1;
            pUacRegInfo->expires = expires;

            SIP_DEBUG_TRACE(LOG_INFO, "SIP_SendRegister() Refresh:register_id=%s, register_callid_number=%s, index=%d \r\n", pUacRegInfo->register_id, pUacRegInfo->register_callid_number, index);
        }
    }
    else  /* 新的注册请求 */
    {
        index = uac_reginfo_add(local_id);
        SIP_DEBUG_TRACE(LOG_INFO, "SIP_SendRegister() uac_reginfo_add:register_id=%s, index=%d \r\n", local_id, index);

        if (index < 0)
        {
            SIP_DEBUG_TRACE(LOG_ERROR, "SIP_SendRegister() exit---: UAC Register Info Add Error \r\n");
            return EV9000_SIPSTACK_REGISTER_GET_UAC_ERROR;
        }

        pUacRegInfo = uac_reginfo_get(index);

        if (NULL == pUacRegInfo)
        {
            SIP_DEBUG_TRACE(LOG_ERROR, "SIP_SendRegister() exit---: Get UAC Register Info Error \r\n");
            return EV9000_SIPSTACK_REGISTER_GET_UAC_ERROR;
        }

        osip_strncpy(pUacRegInfo->proxy_id, service_id, 128);

        osip_strncpy(pUacRegInfo->proxyip, server_ip, 16);
        pUacRegInfo->proxyport = server_port;

        osip_strncpy(pUacRegInfo->register_account, username, 128);
        osip_strncpy(pUacRegInfo->register_password, userpassword, 128);

        pUacRegInfo->expires = expires;

        osip_strncpy(pUacRegInfo->localip, local_ip, 16);
        pUacRegInfo->localport = local_port;

        if ('\0' == pUacRegInfo->register_callid_number[0])
        {
            char* temp =  new_callid();

            if (NULL != temp)
            {
                osip_strncpy(pUacRegInfo->register_callid_number, temp, 128);
                osip_free(temp);
                temp = NULL;
            }
            else
            {
                uac_reginfo_remove(index);
                SIP_DEBUG_TRACE(LOG_ERROR, "SIP_SendRegister() exit---: New Call ID Error \r\n");
                return EV9000_SIPSTACK_NEW_CALLID_ERROR;
            }
        }

        pUacRegInfo->register_cseq_number++;

        SIP_DEBUG_TRACE(LOG_INFO, "SIP_SendRegister() uac_reginfo_add:register_id=%s, register_callid_number=%s, register_cseq_number=%d \r\n", local_id, pUacRegInfo->register_callid_number, pUacRegInfo->register_cseq_number);

        i = sip_register(pUacRegInfo->register_id, pUacRegInfo->register_id, pUacRegInfo->proxy_id, pUacRegInfo->proxyip, pUacRegInfo->proxyport, pUacRegInfo->localip, pUacRegInfo->localport, pUacRegInfo->expires, pUacRegInfo->register_callid_number, pUacRegInfo->register_cseq_number, pUacRegInfo->link_type);

        if (i != 0)
        {
            uac_reginfo_remove(index);
            SIP_DEBUG_TRACE(LOG_ERROR, "SIP_SendRegister() exit---: SIP Register Error \r\n");
            return i;
        }

        pUacRegInfo->isReg = 0;
        pUacRegInfo->isReging = 1;

        SIP_DEBUG_TRACE(LOG_INFO, "SIP_SendRegister() Add:register_id=%s, index=%d \r\n", pUacRegInfo->register_id, index);
    }

    SIP_DEBUG_TRACE(LOG_INFO, "SIP_SendRegister() exit---: register_id=%s, index=%d \r\n", pUacRegInfo->register_id, index);
    return index;
}

/*****************************************************************************
 函 数 名  : SIP_SendRegisterForRoute
 功能描述  : 发送注册消息
 输入参数  : char* service_id
                            char* local_id
                            char* local_ip
                            int local_port
                            char* server_ip
                            int server_port
                            char* username
                            char* userpassword
                            int expires
                            int link_type
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年5月23日 星期四
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int SIP_SendRegisterForRoute(char* service_id, char* local_id, char* local_ip, int local_port, char* server_ip, int server_port, char* username, char* userpassword, int expires, int link_type)
{
    int i = 0;
    int index = 0;
    uac_reg_info_t* pUacRegInfo = NULL;

    SIP_DEBUG_TRACE(LOG_INFO,  "SIP_SendRegister() \
    \r\n In Para: \
    \r\n service_id=%s \
    \r\n local_id=%s \
    \r\n local_ip=%s \
    \r\n local_port=%d \
    \r\n server_ip=%s \
    \r\n server_port=%d \
    \r\n username=%s \
    \r\n userpassword=%s \
    \r\n expires=%d \
    \r\n ", service_id, local_id, local_ip, local_port, server_ip, server_port, username, userpassword, expires);

    if ((NULL == service_id) || (service_id[0] == '\0')
        || (NULL == server_ip) || (server_ip[0] == '\0') || (server_port == 0)
        || (NULL == local_id) || (local_id[0] == '\0')
        || (NULL == local_ip) || (local_ip[0] == '\0') || (local_port == 0))
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "SIP_SendRegisterForRoute() exit---: Param Error \r\n");
        return -1;
    }

    index = uac_reginfo_find_by_server_and_local_info(service_id, server_ip, server_port, local_id, local_ip, local_port);
    SIP_DEBUG_TRACE(LOG_INFO, "SIP_SendRegisterForRoute() uac_reginfo_find_by_server_and_local_info:usrpos=%d \r\n", index);

    if (index >= 0) /* 已经存在 */
    {
        pUacRegInfo = uac_reginfo_get(index);

        if (NULL == pUacRegInfo)
        {
            SIP_DEBUG_TRACE(LOG_ERROR, "SIP_SendRegisterForRoute() exit---: Get UAC Register Info Error \r\n");
            return -1;
        }

        /* 注册账户 */
        if ('\0' != pUacRegInfo->register_account[0])
        {
            if (NULL != username)
            {
                if (0 != sstrcmp(pUacRegInfo->register_account, username))
                {
                    memset(pUacRegInfo->register_account, 0, 132);
                    osip_strncpy(pUacRegInfo->register_account, username, 128);
                }
            }
            else
            {
                memset(pUacRegInfo->register_account, 0, 132);
            }
        }
        else
        {
            if (NULL != username)
            {
                osip_strncpy(pUacRegInfo->register_account, username, 128);
            }
        }

        /* 注册密码 */
        if ('\0' != pUacRegInfo->register_password[0])
        {
            if (NULL != userpassword)
            {
                if (0 != sstrcmp(pUacRegInfo->register_password, userpassword))
                {
                    memset(pUacRegInfo->register_password, 0, 132);
                    osip_strncpy(pUacRegInfo->register_password, username, 128);
                }
            }
            else
            {
                memset(pUacRegInfo->register_password, 0, 132);
            }
        }
        else
        {
            if (NULL != userpassword)
            {
                memset(pUacRegInfo->register_password, 0, 132);
            }
        }

        pUacRegInfo->register_cseq_number++;
        pUacRegInfo->link_type = link_type;

        if (0 == expires)
        {
            i = sip_register(pUacRegInfo->register_id, pUacRegInfo->register_id, pUacRegInfo->proxy_id, pUacRegInfo->proxyip, pUacRegInfo->proxyport, pUacRegInfo->localip, pUacRegInfo->localport, expires, pUacRegInfo->register_callid_number, pUacRegInfo->register_cseq_number, pUacRegInfo->link_type);

            if (i != 0)
            {
                uac_reginfo_remove(index);
                SIP_DEBUG_TRACE(LOG_ERROR, "SIP_SendRegisterForRoute() exit---: SIP Register Error \r\n");
                return -1;
            }

            pUacRegInfo->isReg = 0;
            pUacRegInfo->isReging = 1;
            pUacRegInfo->expires = 0;

            return i;
        }
        else if (expires > 0)
        {
            i = sip_register(pUacRegInfo->register_id, pUacRegInfo->register_id, pUacRegInfo->proxy_id, pUacRegInfo->proxyip, pUacRegInfo->proxyport, pUacRegInfo->localip, pUacRegInfo->localport, expires, pUacRegInfo->register_callid_number, pUacRegInfo->register_cseq_number, pUacRegInfo->link_type);

            if (i != 0)
            {
                uac_reginfo_remove(index);
                SIP_DEBUG_TRACE(LOG_ERROR, "SIP_SendRegisterForRoute() exit---: SIP Register Error \r\n");
                return -1;
            }

            pUacRegInfo->isReg = 0;
            pUacRegInfo->isReging = 1;
            pUacRegInfo->expires = expires;
        }
    }
    else  /* 新的注册请求 */
    {
        index = uac_reginfo_add(local_id);
        SIP_DEBUG_TRACE(LOG_INFO, "SIP_SendRegisterForRoute() uac_reginfo_add:usrpos=%d \r\n", index);

        if (index < 0)
        {
            SIP_DEBUG_TRACE(LOG_ERROR, "SIP_SendRegisterForRoute() exit---: UAC Register Info Add Error \r\n");
            return -1;
        }

        pUacRegInfo = uac_reginfo_get(index);

        if (NULL == pUacRegInfo)
        {
            SIP_DEBUG_TRACE(LOG_ERROR, "SIP_SendRegisterForRoute() exit---: Get UAC Register Info Error \r\n");
            return -1;
        }

        osip_strncpy(pUacRegInfo->proxy_id, service_id, 128);

        osip_strncpy(pUacRegInfo->proxyip, server_ip, 16);
        pUacRegInfo->proxyport = server_port;

        osip_strncpy(pUacRegInfo->register_account, username, 128);
        osip_strncpy(pUacRegInfo->register_password, userpassword, 128);

        pUacRegInfo->expires = expires;

        osip_strncpy(pUacRegInfo->localip, local_ip, 16);
        pUacRegInfo->localport = local_port;
        pUacRegInfo->link_type = link_type;

        if ('\0' == pUacRegInfo->register_callid_number[0])
        {
            char* temp =  new_callid();

            if (NULL != temp)
            {
                osip_strncpy(pUacRegInfo->register_callid_number, temp, 128);
                osip_free(temp);
                temp = NULL;
            }
            else
            {
                uac_reginfo_remove(index);
                SIP_DEBUG_TRACE(LOG_ERROR, "SIP_SendRegisterForRoute() exit---: New Call ID Error \r\n");
                return -1;
            }
        }

        pUacRegInfo->register_cseq_number++;

        i = sip_register(pUacRegInfo->register_id, pUacRegInfo->register_id, pUacRegInfo->proxy_id, pUacRegInfo->proxyip, pUacRegInfo->proxyport, pUacRegInfo->localip, pUacRegInfo->localport, pUacRegInfo->expires, pUacRegInfo->register_callid_number, pUacRegInfo->register_cseq_number, pUacRegInfo->link_type);

        if (i != 0)
        {
            uac_reginfo_remove(index);
            SIP_DEBUG_TRACE(LOG_ERROR, "SIP_SendRegisterForRoute() exit---: SIP Register Error \r\n");
            return -1;
        }

        pUacRegInfo->isReg = 0;
        pUacRegInfo->isReging = 1;
    }

    return index;
}

/*****************************************************************************
 函 数 名  : SIP_SendUnRegister
 功能描述  : 发送去注册 消息
 输入参数  :
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年2月14日 星期五
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int SIP_SendUnRegister(int reg_info_index)
{
    int i = 0;
    uac_reg_info_t* pUacRegInfo = NULL;

#if 0
    SIP_DEBUG_TRACE(LOG_INFO,  "SIP_SendRegisterForRefresh() \
    \r\n In Para: \
    \r\n reg_info_index=%d \
    \r\n ", reg_info_index);

#endif

    if (reg_info_index < 0)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "SIP_SendUnRegister() exit---: Param Error \r\n");
        return -1;
    }

    pUacRegInfo = uac_reginfo_get(reg_info_index);

    if (NULL == pUacRegInfo)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "SIP_SendUnRegister() exit---: Get UAC Register Info Error \r\n");
        return -1;
    }

    pUacRegInfo->register_cseq_number++;

    i = sip_register(pUacRegInfo->register_id, pUacRegInfo->register_id, pUacRegInfo->proxy_id, pUacRegInfo->proxyip, pUacRegInfo->proxyport, pUacRegInfo->localip, pUacRegInfo->localport, 0, pUacRegInfo->register_callid_number, pUacRegInfo->register_cseq_number, pUacRegInfo->link_type);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "SIP_SendUnRegister() exit---: SIP Register Error \r\n");
        return -1;
    }

    pUacRegInfo->isReg = 0;
    pUacRegInfo->isReging = 1;
    pUacRegInfo->expires = 0;

    return 0;
}

/*****************************************************************************
 函 数 名  : SIP_SendRegisterForRefresh
 功能描述  : 发送注册刷新消息
 输入参数  :
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年5月30日 星期四
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int SIP_SendRegisterForRefresh(int reg_info_index)
{
    int i = 0;
    uac_reg_info_t* pUacRegInfo = NULL;

    SIP_DEBUG_TRACE(LOG_INFO,  "SIP_SendRegisterForRefresh() \
    \r\n In Para: \
    \r\n reg_info_index=%d \
    \r\n ", reg_info_index);

    if (reg_info_index < 0)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "SIP_SendRegisterForRefresh() exit---: Param Error \r\n");
        return -1;
    }

    pUacRegInfo = uac_reginfo_get(reg_info_index);

    if (NULL == pUacRegInfo)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "SIP_SendRegisterForRefresh() exit---: Get UAC Register Info Error \r\n");
        return -1;
    }

    pUacRegInfo->register_cseq_number++;

    SIP_DEBUG_TRACE(LOG_INFO, "SIP_SendRegisterForRefresh() :service_id=%s, server_ip=%s, server_port=%d, local_id=%s, local_ip=%s, local_port=%d, register_callid_number=%s, register_cseq_number=%d \r\n", pUacRegInfo->proxy_id, pUacRegInfo->proxyip, pUacRegInfo->proxyport, pUacRegInfo->register_id, pUacRegInfo->localip, pUacRegInfo->localport, pUacRegInfo->register_callid_number, pUacRegInfo->register_cseq_number);

    i = sip_register(pUacRegInfo->register_id, pUacRegInfo->register_id, pUacRegInfo->proxy_id, pUacRegInfo->proxyip, pUacRegInfo->proxyport, pUacRegInfo->localip, pUacRegInfo->localport, pUacRegInfo->min_expires, pUacRegInfo->register_callid_number, pUacRegInfo->register_cseq_number, pUacRegInfo->link_type);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "SIP_SendRegisterForRefresh() exit---: SIP Register Error \r\n");
        return -1;
    }

    pUacRegInfo->isReg = 0;
    pUacRegInfo->isReging = 1;
    pUacRegInfo->expires = pUacRegInfo->min_expires;

    SIP_DEBUG_TRACE(LOG_INFO, "SIP_SendRegisterForRefresh() exit---: register_id=%s, index=%d \r\n", pUacRegInfo->register_id, reg_info_index);
    return 0;
}

/*****************************************************************************
 函 数 名  : SIP_ProxyBuildTargetAndSendInviteByIPAndPort
 功能描述  : SIP服务器根据IP地址和端口定位目的地并发送Invite 消息
 输入参数  : char* caller_id
                            char* callee_id
                            char* callee_register_id
                            char* callee_ip
                            int callee_port
                            sdp_message_t* local_sdp
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年8月29日 星期四
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int SIP_ProxyBuildTargetAndSendInviteByIPAndPort(char* caller_id, char* callee_id, char* callee_register_id, char* callee_ip, int callee_port, sdp_message_t* local_sdp)
{
    int i = 0;
    int callee_pos = -1;
    uas_reg_info_t* pUasRegInfo = NULL;

    if ((NULL == caller_id) || (caller_id[0] == '\0')
        || (NULL == callee_id) || (callee_id[0] == '\0')
        || (NULL == callee_register_id) || (callee_register_id[0] == '\0')
        || (NULL == callee_ip) || (callee_ip[0] == '\0')
        || callee_port <= 0 || NULL == local_sdp)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "SIP_ProxyBuildTargetAndSendInviteByIPAndPort() exit---: Param Error \r\n");
        return -1;
    }

    callee_pos = uas_reginfo_find(callee_register_id, callee_ip, callee_port);

    if (callee_pos < 0)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "SIP_ProxyBuildTargetAndSendInviteByIPAndPort() exit---: Find UAS Register Info Error \r\n");
        return -1;
    }

    pUasRegInfo = uas_reginfo_get(callee_pos);

    if (pUasRegInfo == NULL)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "SIP_ProxyBuildTargetAndSendInviteByIPAndPort() exit---: Get UAS Register Info Error \r\n");
        return -1;
    }

    i = SIP_SendInvite(caller_id, callee_id, pUasRegInfo->serverip, pUasRegInfo->serverport, callee_ip, callee_port, pUasRegInfo->register_account, pUasRegInfo->register_password, local_sdp);

    return i;
}

/*****************************************************************************
 函 数 名  : SIP_SendInvite
 功能描述  : 发送初始的呼叫消息
 输入参数  : char* caller_id
                            char* callee_id
                            char* local_ip
                            int local_port
                            char* server_ip
                            int server_port
                            char* username
                            char* userpassword
                            sdp_t* local_sdp
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年5月23日 星期四
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int SIP_SendInvite(char* caller_id, char* callee_id, char* local_ip, int local_port, char* server_ip, int server_port, char* username, char* userpassword, sdp_message_t* local_sdp)
{
    int i = 0;
    int index = 0;
    ua_dialog_t* pUaDialog = NULL;
    unsigned long local_rtp_addr = 0;
    int local_rtp_port = 0;
    char ipstr[16] = {0};

    SIP_DEBUG_TRACE(LOG_INFO,  "SIP_SendInvite() \
    \r\n In Para: \
    \r\n caller_id=%s \
    \r\n callee_id=%s \
    \r\n local_ip=%s \
    \r\n local_port=%d \
    \r\n server_ip=%s \
    \r\n server_port=%d \
    \r\n username=%s \
    \r\n userpassword=%s \
    \r\n ", caller_id, callee_id, local_ip, local_port, server_ip, server_port, username, userpassword);

    if ((NULL == caller_id) || (NULL == callee_id) || (NULL == local_ip) || (NULL == server_ip) || (NULL == local_sdp))
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "SIP_SendInvite() exit---: Param Error \r\n");
        return EV9000_SIPSTACK_PARAM_ERROR;
    }

    index = ua_dialog_add();
    //SIP_DEBUG_TRACE(LOG_INFO, "SIP_SendInvite() ua_dialog_add:index=%d \r\n", index);

    if (index < 0)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "SIP_SendInvite() exit---: UA Dialog Add Error \r\n");
        return EV9000_SIPSTACK_INVITE_GET_UA_ERROR;
    }

    pUaDialog = ua_dialog_get(index);

    if (NULL == pUaDialog)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "SIP_SendInvite() exit---: Get UA Dialog Error:dialog_index=%d \r\n", index);
        return EV9000_SIPSTACK_INVITE_GET_UA_ERROR;
    }

    osip_strncpy(pUaDialog->strCallerID, caller_id, 128);
    osip_strncpy(pUaDialog->strCalleeID, callee_id, 128);

    osip_strncpy(pUaDialog->strUserName, username, 128);
    osip_strncpy(pUaDialog->strPassword, userpassword, 128);

    osip_strncpy(pUaDialog->strRemoteIP, server_ip, 16);
    pUaDialog->iRemotePort = server_port;

    osip_strncpy(pUaDialog->strLocalIP, local_ip, 16);
    pUaDialog->iLocalPort = local_port;

    i = get_sdp_ip_and_port(local_sdp, &local_rtp_addr, &local_rtp_port);

    if (0 != i)
    {
        ua_dialog_remove(index);
        SIP_DEBUG_TRACE(LOG_ERROR, "SIP_SendInvite:ua_dialog_remove() index=%d \r\n", index);
        SIP_DEBUG_TRACE(LOG_ERROR, "SIP_SendInvite() exit---: Get SDP IP And Port Error \r\n");
        return EV9000_SIPSTACK_INVITE_GET_SDP_INFO_ERROR;
    }

    i = sdp_message_clone(local_sdp, &pUaDialog->pLocalSDP);

    if (0 != i)
    {
        ua_dialog_remove(index);
        SIP_DEBUG_TRACE(LOG_INFO, "SIP_SendInvite:ua_dialog_remove() index=%d \r\n", index);
        SIP_DEBUG_TRACE(LOG_ERROR, "SIP_SendInvite() exit---: SDP Message Clone Error \r\n");
        return EV9000_SIPSTACK_SDP_CLONE_ERROR;
    }

    ipaddr2str(ipstr, local_rtp_addr);
    osip_strncpy(pUaDialog->strLocalRTPIP, ipstr, 16);
    pUaDialog->iLocalRTPPort = local_rtp_port;

    i = sip_invite(index);

    if (i != 0)
    {
        ua_dialog_remove(index);
        SIP_DEBUG_TRACE(LOG_INFO, "SIP_SendInvite:ua_dialog_remove() index=%d \r\n", index);
        SIP_DEBUG_TRACE(LOG_ERROR, "SIP_SendInvite() exit---: SIP Invite Error \r\n");
        return i;
    }

    return index;
}

/*****************************************************************************
 函 数 名  : SIP_ProxyForwardInviteWithinDialog
 功能描述  : 转发会话内的呼叫消息
 输入参数  : int caller_dialog_pos
                            char* local_ip
                            int local_port
                            char* remote_ip
                            int remote_port
                            sdp_t* local_sdp
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年7月17日 星期三
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int SIP_ProxyForwardInviteWithinDialog(int caller_dialog_pos, char* local_id,  char* local_ip, int local_port, char* remote_id, char* remote_ip, int remote_port, sdp_message_t* local_sdp)
{
    int i = 0;
    int callee_dialog_pos = -1;
    ua_dialog_t* pCallerUaDialog = NULL;
    ua_dialog_t* pCalleeUaDialog = NULL;
    unsigned long local_rtp_addr = 0;
    int local_rtp_port = 0;
    char ipstr[16] = {0};

    SIP_DEBUG_TRACE(LOG_INFO,  "SIP_ProxyForwardInviteWithinDialog() \
    \r\n In Para: \
    \r\n caller_dialog_pos=%d \
    \r\n local_ip=%s \
    \r\n local_port=%d \
    \r\n remote_ip=%s \
    \r\n remote_port=%d \
    \r\n ", caller_dialog_pos, local_ip, local_port, remote_ip, remote_port);

    if (NULL == local_sdp || NULL == remote_ip || remote_ip[0] == '\0' || NULL == local_ip || local_ip[0] == '\0' || local_port <= 0 || remote_port <= 0)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "SIP_ProxyForwardInviteWithinDialog() exit---: Param Error \r\n");
        return -1;
    }

    if (!is_valid_dialog_index(caller_dialog_pos))
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "SIP_ProxyForwardInviteWithinDialog() exit---: Dialog Index Error \r\n");
        return -1;
    }

    pCallerUaDialog = ua_dialog_get(caller_dialog_pos);

    if (NULL == pCallerUaDialog)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "SIP_ProxyForwardInviteWithinDialog() exit---: Get Caller UA Dialog Error:dialog_index=%d \r\n", caller_dialog_pos);
        return -1;
    }

    callee_dialog_pos = ua_dialog_add();
    //SIP_DEBUG_TRACE(LOG_INFO, "SIP_ProxyForwardInviteWithinDialog() ua_dialog_add:index=%d \r\n", callee_dialog_pos);

    if (callee_dialog_pos < 0)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "SIP_ProxyForwardInviteWithinDialog() exit---: UA Dialog Add Error \r\n");
        return EV9000_SIPSTACK_INVITE_GET_UA_ERROR;
    }

    pCalleeUaDialog = ua_dialog_get(callee_dialog_pos);

    if (NULL == pCalleeUaDialog)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "SIP_ProxyForwardInviteWithinDialog() exit---: Get Callee UA Dialog Error:dialog_index=%d \r\n", callee_dialog_pos);
        return EV9000_SIPSTACK_INVITE_GET_UA_ERROR;
    }

    if (NULL != local_id)
    {
        osip_strncpy(pCalleeUaDialog->strCallerID, local_id, 128);
    }
    else
    {
        osip_strncpy(pCalleeUaDialog->strCallerID, pCallerUaDialog->strCallerID, 128);
    }

    if (NULL != remote_id)
    {
        osip_strncpy(pCalleeUaDialog->strCalleeID, remote_id, 128);
    }
    else
    {
        osip_strncpy(pCalleeUaDialog->strCalleeID, pCallerUaDialog->strCalleeID, 128);
    }

    osip_strncpy(pCalleeUaDialog->strUserName, pCallerUaDialog->strUserName, 128);
    osip_strncpy(pCalleeUaDialog->strPassword, pCallerUaDialog->strPassword, 128);

    osip_strncpy(pCalleeUaDialog->strLocalIP, local_ip, 16);
    pCalleeUaDialog->iLocalPort = local_port;

    osip_strncpy(pCalleeUaDialog->strRemoteIP, remote_ip, 16);
    pCalleeUaDialog->iRemotePort = remote_port;

    i = get_sdp_ip_and_port(local_sdp, &local_rtp_addr, &local_rtp_port);

    if (0 != i)
    {
        ua_dialog_remove(callee_dialog_pos);
        SIP_DEBUG_TRACE(LOG_ERROR, "SIP_ProxyForwardInviteWithinDialog:ua_dialog_remove() index=%d \r\n", callee_dialog_pos);
        SIP_DEBUG_TRACE(LOG_ERROR, "SIP_ProxyForwardInviteWithinDialog() exit---: Get SDP IP And Port Error \r\n");
        return EV9000_SIPSTACK_INVITE_GET_SDP_INFO_ERROR;
    }

    i = sdp_message_clone(local_sdp, &pCalleeUaDialog->pLocalSDP);

    if (0 != i)
    {
        ua_dialog_remove(callee_dialog_pos);
        SIP_DEBUG_TRACE(LOG_ERROR, "SIP_ProxyForwardInviteWithinDialog:ua_dialog_remove() index=%d \r\n", callee_dialog_pos);
        SIP_DEBUG_TRACE(LOG_ERROR, "SIP_ProxyForwardInviteWithinDialog() exit---: SDP Message Clone Error \r\n");
        return EV9000_SIPSTACK_SDP_CLONE_ERROR;
    }

    ipaddr2str(ipstr, local_rtp_addr);
    osip_strncpy(pCalleeUaDialog->strLocalRTPIP, ipstr, 16);
    pCalleeUaDialog->iLocalRTPPort = local_rtp_port;

    i = sip_forward_invite_within_dialog(caller_dialog_pos, callee_dialog_pos);

    if (i != 0)
    {
        ua_dialog_remove(callee_dialog_pos);
        SIP_DEBUG_TRACE(LOG_ERROR, "SIP_ProxyForwardInviteWithinDialog:ua_dialog_remove():caller_dialog_pos=%d \r\n", caller_dialog_pos);
        SIP_DEBUG_TRACE(LOG_ERROR, "SIP_ProxyForwardInviteWithinDialog() exit---: SIP Forward Invite Within Dialog Error \r\n");
        return i;
    }

    return callee_dialog_pos;
}

/*****************************************************************************
 函 数 名  : SIP_AcceptInvite
 功能描述  : 接收呼叫函数
 输入参数  : int dialog_index
             sdp_t* local_sdp
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年5月23日 星期四
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int SIP_AcceptInvite(int dialog_index, sdp_message_t* local_sdp)
{
    int i = 0;
    ua_dialog_t* pUaDialog = NULL;
    unsigned long local_rtp_addr = 0;
    int local_rtp_port = 0;
    char ipstr[16] = {0};

#if 0
    SIP_DEBUG_TRACE(LOG_INFO,  "SIP_AcceptInvite() \
    \r\n In Para: \
    \r\n dialog_index=%d \
    \r\n ", dialog_index);
#endif

    if (NULL == local_sdp)
    {
        //SIP_DEBUG_TRACE(LOG_DEBUG, "SIP_AcceptInvite() exit---: Param Error \r\n");
        return -1;
    }

    if (!is_valid_dialog_index(dialog_index))
    {
        //SIP_DEBUG_TRACE(LOG_DEBUG, "SIP_AcceptInvite() exit---: Dialog Index Error \r\n");
        return EV9000_SIPSTACK_INVITE_GET_UA_ERROR;
    }

    USED_UA_SMUTEX_LOCK();

    pUaDialog = ua_dialog_get2(dialog_index);

    if (NULL == pUaDialog)
    {
        USED_UA_SMUTEX_UNLOCK();
        //SIP_DEBUG_TRACE(LOG_DEBUG, "SIP_AcceptInvite() exit---: Get UA Dialog Error:dialog_index=%d \r\n", dialog_index);
        return EV9000_SIPSTACK_INVITE_GET_UA_ERROR;
    }

    i = get_sdp_ip_and_port(local_sdp, &local_rtp_addr, &local_rtp_port);

    if (0 != i)
    {
        USED_UA_SMUTEX_UNLOCK();
        //SIP_DEBUG_TRACE(LOG_DEBUG, "SIP_AcceptInvite() exit---: Get SDP IP And Port Error \r\n");
        return EV9000_SIPSTACK_INVITE_GET_SDP_INFO_ERROR;
    }

    i = sdp_message_clone(local_sdp, &pUaDialog->pLocalSDP);

    if (0 != i)
    {
        USED_UA_SMUTEX_UNLOCK();
        //SIP_DEBUG_TRACE(LOG_DEBUG, "SIP_AcceptInvite() exit---: SDP Message Clone Error \r\n");
        return EV9000_SIPSTACK_SDP_CLONE_ERROR;
    }

    USED_UA_SMUTEX_UNLOCK();

    ipaddr2str(ipstr, local_rtp_addr);
    osip_strncpy(pUaDialog->strLocalRTPIP, ipstr, 16);
    pUaDialog->iLocalRTPPort = local_rtp_port;

    if (pUaDialog->eUiState == UI_STATE_CALL_RCV)
    {
        i = do_accept_call(dialog_index, local_sdp);

        if (i == 0)
        {
            pUaDialog->eUiState = UI_STATE_CONNECTED;
        }
        else if (i == 1)
        {
            pUaDialog->eUiState = UI_STATE_CALL_ACCEPT;
        }
        else
        {
            pUaDialog->eUiState = UI_STATE_IDLE;
        }
    }

    return i;
}

/*****************************************************************************
 函 数 名  : SIP_SendCancel
 功能描述  : 发送Cancel消息
 输入参数  : int dialog_index
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年8月22日 星期四
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int SIP_SendCancel(int dialog_index)
{
    int i = 0;
    ua_dialog_t* pUaDialog = NULL;

#if 1
    SIP_DEBUG_TRACE(LOG_INFO,  "SIP_SendCancel() \
    \r\n In Para: \
    \r\n dialog_index=%d \
    \r\n ", dialog_index);

#endif

    if (!is_valid_dialog_index(dialog_index))
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "SIP_SendCancel() exit---: Dialog Index Error \r\n");
        return -1;
    }

    pUaDialog = ua_dialog_get(dialog_index);

    if (NULL == pUaDialog)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "SIP_SendCancel() exit---: Get UA Dialog Error:dialog_index=%d \r\n", dialog_index);
        return -1;
    }

    i = sip_cancel(dialog_index);

    pUaDialog->eUiState = UI_STATE_IDLE;

    return i;
}

/*****************************************************************************
 函 数 名  : SIP_SendAck
 功能描述  : 发送Ack消息
 输入参数  : int dialog_index
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年5月23日 星期四
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int SIP_SendAck(int dialog_index)
{
    int i = 0;
    ua_dialog_t* pUaDialog = NULL;

#if 1
    SIP_DEBUG_TRACE(LOG_INFO,  "SIP_SendAck() \
    \r\n In Para: \
    \r\n dialog_index=%d \
    \r\n ", dialog_index);

#endif

    if (!is_valid_dialog_index(dialog_index))
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "SIP_SendAck() exit---: Dialog Index Error \r\n");
        return -1;
    }

    pUaDialog = ua_dialog_get(dialog_index);

    if (NULL == pUaDialog)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "SIP_SendAck() exit---: Get UA Dialog Error:dialog_index=%d \r\n", dialog_index);
        return -1;
    }

    if (pUaDialog->eUiState == UI_STATE_200_RECEIVE)
    {
        i = sip_ack(dialog_index, pUaDialog->strLocalIP, pUaDialog->iLocalPort, pUaDialog->iSessionExpires);
        pUaDialog->eUiState = UI_STATE_CONNECTED;
    }
    else
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "SIP_SendAck() exit---: pUaDialog->eUiState=%d Error \r\n", pUaDialog->eUiState);
        return -1;
    }

    return i;
}

/*****************************************************************************
 函 数 名  : SIP_SendBye
 功能描述  : 发送bye消息
 输入参数  : int dialog_index
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年5月23日 星期四
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int SIP_SendBye(int dialog_index)
{
    int i = 0;
    ua_dialog_t* pUaDialog = NULL;

#if 1
    SIP_DEBUG_TRACE(LOG_INFO,  "SIP_SendBye() \
    \r\n In Para: \
    \r\n dialog_index=%d \
    \r\n ", dialog_index);

#endif

    if (!is_valid_dialog_index(dialog_index))
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "SIP_SendBye() exit---: Dialog Index Error \r\n");
        return -1;
    }

    pUaDialog = ua_dialog_get(dialog_index);

    if (NULL == pUaDialog)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "SIP_SendBye() exit---: Get UA Dialog Error:dialog_index=%d \r\n", dialog_index);
        return -1;
    }

    if ((pUaDialog->eUiState == UI_STATE_CONNECTED)
        || (pUaDialog->eUiState == UI_STATE_CALL_ACCEPT))
    {
        i = sip_bye(dialog_index, pUaDialog->strLocalIP, pUaDialog->iLocalPort);

        pUaDialog->eUiState = UI_STATE_IDLE;
    }
    else if (pUaDialog->eUiState == UI_STATE_200_RECEIVE)
    {
        i = sip_ack(dialog_index, pUaDialog->strLocalIP, pUaDialog->iLocalPort, pUaDialog->iSessionExpires);
        i = sip_bye(dialog_index, pUaDialog->strLocalIP, pUaDialog->iLocalPort);

        pUaDialog->eUiState = UI_STATE_IDLE;
    }
    else if (pUaDialog->eUiState == UI_STATE_CALL_SENT)
    {
        i = sip_cancel(dialog_index);

        pUaDialog->eUiState = UI_STATE_IDLE;
    }
    else if (pUaDialog->eUiState == UI_STATE_CALL_TERMINATED)
    {
        pUaDialog->eUiState = UI_STATE_IDLE;
    }

    //ua_dialog_remove(dialog_index);
    return i;
}

/*****************************************************************************
 函 数 名  : SIP_SendMessage
 功能描述  : 发送Message消息
 输入参数  : char* message_id
                            char* caller_id
                            char* callee_id
                            char* local_ip
                            int local_port
                            char* server_ip
                            int server_port
                            char* msg
                            int msg_len
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年5月23日 星期四
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int SIP_SendMessage(char* message_id, char* caller_id, char* callee_id, char* local_ip, int local_port, char* server_ip, int server_port, char* msg, int msg_len)
{
    int i = 0;

#if 1
    SIP_DEBUG_TRACE(LOG_INFO,  "SIP_SendMessage() \
    \r\n In Para: \
    \r\n message_id=%s \
    \r\n caller_id=%s \
    \r\n callee_id=%s \
    \r\n local_ip=%s \
    \r\n local_port=%d \
    \r\n server_ip=%s \
    \r\n server_port=%d \
    \r\n msg_len=%d \
    \r\n ", message_id, caller_id, callee_id, local_ip, local_port, server_ip, server_port, msg_len);

#endif

    if ((NULL == caller_id) || (caller_id[0] == '\0')
        || (NULL == callee_id) || (callee_id[0] == '\0')
        || (NULL == local_ip) || (local_ip[0] == '\0') || (local_port <= 0)
        || (NULL == server_ip) || (server_ip[0] == '\0') || (server_port <= 0))
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "SIP_SendMessage() exit---: Param Error \r\n");
        return EV9000_SIPSTACK_PARAM_ERROR;
    }

    i = sip_message(message_id, caller_id, callee_id, msg, msg_len, server_ip, server_port, local_ip, local_port);

    return i;
}

/*****************************************************************************
 函 数 名  : SIP_SendMessage_By_TCP
 功能描述  : 基于TCP发送Message消息
 输入参数  : char* message_id
             char* caller_id
             char* callee_id
             char* local_ip
             int local_port
             char* server_ip
             int server_port
             char* msg
             int msg_len
             int tcp_scoket
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年2月21日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int SIP_SendMessage_By_TCP(char* message_id, char* caller_id, char* callee_id, char* local_ip, int local_port, char* server_ip, int server_port, char* msg, int msg_len, int tcp_scoket)
{
    int i = 0;

#if 1
    SIP_DEBUG_TRACE(LOG_INFO,  "SIP_SendMessage_By_TCP() \
    \r\n In Para: \
    \r\n message_id=%s \
    \r\n caller_id=%s \
    \r\n callee_id=%s \
    \r\n local_ip=%s \
    \r\n local_port=%d \
    \r\n server_ip=%s \
    \r\n server_port=%d \
    \r\n msg_len=%d \
    \r\n tcp_scoket=%d \
    \r\n ", message_id, caller_id, callee_id, local_ip, local_port, server_ip, server_port, msg_len, tcp_scoket);

#endif

    if ((NULL == caller_id) || (caller_id[0] == '\0')
        || (NULL == callee_id) || (callee_id[0] == '\0')
        || (NULL == local_ip) || (local_ip[0] == '\0') || (local_port <= 0)
        || (NULL == server_ip) || (server_ip[0] == '\0') || (server_port <= 0))
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "SIP_SendMessage_By_TCP() exit---: Param Error \r\n");
        return EV9000_SIPSTACK_PARAM_ERROR;
    }

    if (tcp_scoket < 0)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "SIP_SendMessage_By_TCP() exit---: TCP Socket Error \r\n");
        return EV9000_SIPSTACK_PARAM_ERROR;
    }

    i = sip_message_for_tcp(message_id, caller_id, callee_id, msg, msg_len, server_ip, server_port, local_ip, local_port, tcp_scoket);

    return i;
}

/*****************************************************************************
 函 数 名  : SIP_SendMsgWithinDialog
 功能描述  : 发送会话内的Message消息
 输入参数  : int dialog_index
                            char* msg
                            int msg_len
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年5月23日 星期四
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int SIP_SendMsgWithinDialog(int dialog_index,  char* msg, int msg_len)
{
    int i = 0;
    ua_dialog_t* pUaDialog = NULL;

#if 0
    SIP_DEBUG_TRACE(LOG_INFO,  "SIP_SendMsgWithinDialog() \
    \r\n In Para: \
    \r\n dialog_index=%d \
    \r\n msg_len=%d \
    \r\n ", dialog_index, msg_len);
#endif

    if (!is_valid_dialog_index(dialog_index))
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "SIP_SendMsgWithinDialog() exit---: Dialog Index Error \r\n");
        return -1;
    }

    pUaDialog = ua_dialog_get(dialog_index);

    if (NULL == pUaDialog)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "SIP_SendMsgWithinDialog() exit---: Get UA Dialog Error:dialog_index=%d \r\n", dialog_index);
        return -1;
    }

    i = sip_message_within_dialog(dialog_index, msg, msg_len, pUaDialog->strLocalIP, pUaDialog->iLocalPort);

    return i;
}

/*****************************************************************************
 函 数 名  : SIP_ProxyBuildTargetAndSendMessage
 功能描述  : SIP服务器定位目的地并发送Message消息
 输入参数  : char* caller_id
                            char* callee_id
                            char* callee_register_id
                            char* msg
                            int msg_len
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年5月23日 星期四
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int SIP_ProxyBuildTargetAndSendMessage(char* caller_id, char* callee_id, char* callee_register_id, char* msg, int msg_len)
{
    int i = 0;
    int socket = 0;
    char* callee_host = NULL;
    int callee_port = 0;
    char* callid = NULL;
    osip_message_t* message = NULL;
    osip_transaction_t* transaction = NULL;
    uas_reg_info_t* pUasRegInfo = NULL;
    UAS_Reg_Info_Iterator Itr;

#if 0

    SIP_DEBUG_TRACE(LOG_INFO,  "SIP_ProxyBuildTargetAndSendMessage() \
    \r\n In Para: \
    \r\n caller_id=%s \
    \r\n callee_id=%s \
    \r\n callee_register_id=%s \
    \r\n msg_len=%d \
    \r\n ", caller_id, callee_id, callee_register_id, msg_len);
#endif

    if ((NULL == caller_id) || (caller_id[0] == '\0')
        || (NULL == callee_id) || (callee_id[0] == '\0')
        || (NULL == callee_register_id) || (callee_register_id[0] == '\0'))
    {
        //SIP_DEBUG_TRACE(LOG_DEBUG, "SIP_ProxyBuildTargetAndSendMessage() exit---: Param Error \r\n");
        return -1;
    }

    UAS_SMUTEX_LOCK();

    if (g_UasRegInfoMap.size() <= 0)
    {
        UAS_SMUTEX_UNLOCK();
        //SIP_DEBUG_TRACE(LOG_DEBUG, "SIP_ProxyBuildTargetAndSendMessage() exit---: UAS Register Info Map NULL \r\n");
        return -1;
    }

    for (Itr = g_UasRegInfoMap.begin(); Itr != g_UasRegInfoMap.end(); Itr++)
    {
        pUasRegInfo = Itr->second;

        if ((NULL == pUasRegInfo) || (0 == pUasRegInfo->iUsed) || (pUasRegInfo->register_id == '\0'))
        {
            continue;
        }

        if (sstrcmp(pUasRegInfo->register_id, callee_register_id) == 0)
        {
            callee_host = pUasRegInfo->from_host;
            callee_port = pUasRegInfo->from_port;

            callid = call_id_new_random();

            if (NULL == callid)
            {
                continue;
            }

            i = generating_message(&message, caller_id, callee_id, callid, msg, msg_len, callee_host, callee_port, pUasRegInfo->serverip, pUasRegInfo->serverport);

            if (i != 0)
            {
                osip_free(callid);
                callid = NULL;
                continue;
            }

            /* 获取socket */
            socket = get_socket_by_port(pUasRegInfo->serverport);

            if (socket <= 0)
            {
                osip_free(callid);
                callid = NULL;
                osip_message_free(message);
                message = NULL;
                //SIP_DEBUG_TRACE(LOG_DEBUG, "SIP_ProxyBuildTargetAndSendMessage() exit---: Get Socket Error \r\n");
                continue;
            }

            i = osip_transaction_init(&transaction,
                                      NICT,
                                      g_send_message_cell,
                                      message);

            if (0 != i)
            {
                osip_free(callid);
                callid = NULL;
                osip_message_free(message);
                message = NULL;
                //SIP_DEBUG_TRACE(LOG_DEBUG, "SIP_ProxyBuildTargetAndSendMessage() exit---: Transaction Init Error \r\n");
                continue;
            }

            /* 设置transaction的socket */
            osip_transaction_set_in_socket(transaction, socket);
            osip_transaction_set_out_socket(transaction, socket);

            i = ul_sendmessage(transaction, message);

            if (i != 0)
            {
                //SIP_DEBUG_TRACE(LOG_ERROR, "SIP_ProxyBuildTargetAndSendMessage(): ul_sendmessage Error \r\n");
            }

            osip_free(callid);
            callid = NULL;
        }
    }

    UAS_SMUTEX_UNLOCK();

    return 0;
}

/*****************************************************************************
 函 数 名  : SIP_ProxyBuildTargetAndSendMessageByIPAndPort
 功能描述  : SIP服务器根据IP地址和端口定位目的地并发送Message消息
 输入参数  : char* caller_id
                            char* callee_id
                            char* callee_register_id
                            char* callee_ip
                            int callee_port
                            char* msg
                            int msg_len
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年8月29日 星期四
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int SIP_ProxyBuildTargetAndSendMessageByIPAndPort(char* caller_id, char* callee_id, char* callee_register_id, char* callee_ip, int callee_port, char* msg, int msg_len)
{
    int i = 0;
    int socket = 0;
    int callee_pos = -1;
    char* callid = NULL;
    osip_message_t* message = NULL;
    osip_transaction_t* transaction = NULL;
    uas_reg_info_t* pUasRegInfo = NULL;

    if ((NULL == caller_id) || (caller_id[0] == '\0')
        || (NULL == callee_id) || (callee_id[0] == '\0')
        || (NULL == callee_register_id) || (callee_register_id[0] == '\0')
        || (NULL == callee_ip) || (callee_ip[0] == '\0')
        || callee_port <= 0)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "SIP_ProxyBuildTargetAndSendMessageByIPAndPort() exit---: Param Error \r\n");
        return -1;
    }

    callee_pos = uas_reginfo_find(callee_register_id, callee_ip, callee_port);

    if (callee_pos < 0)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "SIP_ProxyBuildTargetAndSendMessageByIPAndPort() exit---: Find UAS Register Info Error \r\n");
        return -1;
    }

    pUasRegInfo = uas_reginfo_get(callee_pos);

    if (pUasRegInfo == NULL)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "SIP_ProxyBuildTargetAndSendMessageByIPAndPort() exit---: Get UAS Register Info Error \r\n");
        return -1;
    }

    callid = call_id_new_random();

    if (NULL == callid)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "SIP_ProxyBuildTargetAndSendMessageByIPAndPort() exit---:  call_id_new_random Error \r\n");
        return -1;
    }

    i = generating_message(&message, caller_id, callee_id, callid, msg, msg_len, callee_ip, callee_port, pUasRegInfo->serverip, pUasRegInfo->serverport);

    if (i != 0)
    {
        osip_free(callid);
        callid = NULL;
        SIP_DEBUG_TRACE(LOG_DEBUG, "SIP_ProxyBuildTargetAndSendMessageByIPAndPort() exit---:  generating_message Error \r\n");
        return -1;
    }

    /* 获取socket */
    socket = get_socket_by_port(pUasRegInfo->serverport);

    if (socket <= 0)
    {
        osip_free(callid);
        callid = NULL;
        osip_message_free(message);
        message = NULL;
        SIP_DEBUG_TRACE(LOG_DEBUG, "SIP_ProxyBuildTargetAndSendMessageByIPAndPort() exit---: Get Socket Error \r\n");
        return -1;
    }

    i = osip_transaction_init(&transaction,
                              NICT,
                              g_send_message_cell,
                              message);

    if (0 != i)
    {
        osip_free(callid);
        callid = NULL;
        osip_message_free(message);
        message = NULL;
        SIP_DEBUG_TRACE(LOG_DEBUG, "SIP_ProxyBuildTargetAndSendMessageByIPAndPort() exit---: Transaction Init Error \r\n");
        return -1;
    }

    /* 设置transaction的socket */
    osip_transaction_set_in_socket(transaction, socket);
    osip_transaction_set_out_socket(transaction, socket);

    i = ul_sendmessage(transaction, message);

    if (0 != i)
    {
        osip_free(callid);
        callid = NULL;
        SIP_DEBUG_TRACE(LOG_ERROR, "SIP_ProxyBuildTargetAndSendMessageByIPAndPort() exit---: ul_sendmessage Error \r\n");
        return -1;
    }

    osip_free(callid);
    callid = NULL;

    return 0;
}

/*****************************************************************************
 函 数 名  : SIP_SendInfo
 功能描述  : 发送Info消息
 输入参数  : char* message_id
                            char* caller_id
                            char* callee_id
                            char* local_ip
                            int local_port
                            char* server_ip
                            int server_port
                            char* body
                            int body_len
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年5月23日 星期四
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int SIP_SendInfo(char* message_id, char* caller_id, char* callee_id, char* local_ip, int local_port, char* server_ip, int server_port, char* body, int body_len)
{
    int i = 0;
    osip_uri_t* url = NULL;
    char tmp[128] = {0};

#if 0
    SIP_DEBUG_TRACE(LOG_INFO,  "SIP_SendInfo() \
    \r\n In Para: \
    \r\n message_id=%s \
    \r\n caller_id=%s \
    \r\n callee_id=%s \
    \r\n local_ip=%s \
    \r\n local_port=%d \
    \r\n server_ip=%s \
    \r\n server_port=%d \
    \r\n body_len=%d \
    \r\n ", message_id, caller_id, callee_id, local_ip, local_port, server_ip, server_port, body_len);
#endif

    if ((NULL == message_id) || (message_id[0] == '\0')
        || (NULL == caller_id) || (caller_id[0] == '\0')
        || (NULL == callee_id) || (callee_id[0] == '\0')
        || (NULL == local_ip) || (local_ip[0] == '\0') || (0 == local_port)
        || (NULL == server_ip) || (server_ip[0] == '\0') || (0 == server_port))
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "SIP_SendInfo() exit---: Param Error \r\n");
        return -1;
    }

    i = osip_uri_init(&url);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "SIP_SendInfo() exit---: URL Init Error \r\n");
        return -1;
    }

    snprintf(tmp, 128, "sip:%s@%s:%i", callee_id, server_ip, server_port);
    i = osip_uri_parse(url, tmp);

    if (i != 0)
    {
        osip_uri_free(url);
        url = NULL;
        SIP_DEBUG_TRACE(LOG_ERROR, "SIP_SendInfo() exit---: URL Parse Error \r\n");
        return -1;
    }

    i = sip_info(message_id, caller_id, callee_id, body, body_len, url, server_ip, server_port, local_ip, local_port);

    return i;
}

/*****************************************************************************
 函 数 名  : SIP_SendInfoWithinDialog
 功能描述  : 发送会话内的Info消息
 输入参数  : int dialog_index
                            char* body
                            int body_len
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年5月23日 星期四
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int SIP_SendInfoWithinDialog(int dialog_index, char* body, int body_len)
{
    int i = 0;
    ua_dialog_t* pUaDialog = NULL;

#if 0
    SIP_DEBUG_TRACE(LOG_INFO,  "SIP_SendInfoWithinDialog() \
    \r\n In Para: \
    \r\n dialog_index=%d \
    \r\n body_len=%d \
    \r\n ", dialog_index, body_len);

#endif

    if (!is_valid_dialog_index(dialog_index))
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "SIP_SendInfoWithinDialog() exit---: Dialog Index Error \r\n");
        return -1;
    }

    pUaDialog = ua_dialog_get(dialog_index);

    if (NULL == pUaDialog)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "SIP_SendInfoWithinDialog() exit---: Get UA Dialog Error:dialog_index=%d \r\n", dialog_index);
        return -1;
    }

    i = sip_info_within_dialog(dialog_index, body, body_len, pUaDialog->strLocalIP, pUaDialog->iLocalPort);

    return i;
}

#if 0
/*****************************************************************************
 函 数 名  : SIP_SendSubscribe
 功能描述  : 发送订阅消息
 输入参数  : char* message_id
             char* caller_id
             char* callee_id
             char* event
             int event_id
             int expires
             char* local_ip
             int local_port
             char* server_ip
             int server_port
             char* msg
             int msg_len
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年6月10日 星期三
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int SIP_SendSubscribe(char* message_id, char* caller_id, char* callee_id, char* event, int event_id, int expires, char* local_ip, int local_port, char* server_ip, int server_port, char* msg, int msg_len)
{
    int i = 0;

#if 1
    SIP_DEBUG_TRACE(LOG_INFO,  "SIP_SendSubscribe() \
    \r\n In Para: \
    \r\n message_id=%s \
    \r\n caller_id=%s \
    \r\n callee_id=%s \
    \r\n local_ip=%s \
    \r\n local_port=%d \
    \r\n server_ip=%s \
    \r\n server_port=%d \
    \r\n msg_len=%d \
    \r\n ", message_id, caller_id, callee_id, local_ip, local_port, server_ip, server_port, msg_len);

#endif

    if ((NULL == caller_id) || (caller_id[0] == '\0')
        || (NULL == callee_id) || (callee_id[0] == '\0')
        || (NULL == local_ip) || (local_ip[0] == '\0') || (local_port <= 0)
        || (NULL == server_ip) || (server_ip[0] == '\0') || (server_port <= 0)
        || (NULL == event) || (event_id <= 0))
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "SIP_SendSubscribe() exit---: Param Error \r\n");
        return EV9000_SIPSTACK_PARAM_ERROR;
    }

    i = sip_subscribe(message_id, caller_id, callee_id, event, event_id, expires, server_ip, server_port, local_ip, local_port, msg, msg_len);

    return i;
}
#endif

/*****************************************************************************
 函 数 名  : SIP_SendSubscribe
 功能描述  : 发送订阅消息
 输入参数  : char* message_id
             char* caller_id
             char* callee_id
             char* event
             int event_id
             int expires
             char* local_ip
             int local_port
             char* server_ip
             int server_port
             char* msg
             int msg_len
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年6月10日 星期三
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int SIP_SendSubscribe(char* message_id, char* caller_id, char* callee_id, char* event, int event_id, int expires, char* local_ip, int local_port, char* server_ip, int server_port, char* msg, int msg_len)
{
    int i = 0;
    int index = 0;
    ua_dialog_t* pUaDialog = NULL;

    SIP_DEBUG_TRACE(LOG_INFO,  "SIP_SendSubscribe() \
    \r\n In Para: \
    \r\n message_id=%s \
    \r\n caller_id=%s \
    \r\n callee_id=%s \
    \r\n local_ip=%s \
    \r\n local_port=%d \
    \r\n server_ip=%s \
    \r\n server_port=%d \
    \r\n event=%s \
    \r\n event_id=%d \
    \r\n expires=%d \
    \r\n ", message_id, caller_id, callee_id, local_ip, local_port, server_ip, server_port, event, event_id, expires);

    if ((NULL == caller_id) || (NULL == callee_id) || (NULL == local_ip) || (NULL == server_ip) || (NULL == event))
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "SIP_SendSubscribe() exit---: Param Error \r\n");
        return EV9000_SIPSTACK_PARAM_ERROR;
    }

    index = ua_dialog_add();
    //SIP_DEBUG_TRACE(LOG_INFO, "SIP_SendInvite() ua_dialog_add:index=%d \r\n", index);

    if (index < 0)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "SIP_SendSubscribe() exit---: UA Dialog Add Error \r\n");
        return EV9000_SIPSTACK_INVITE_GET_UA_ERROR;
    }

    pUaDialog = ua_dialog_get(index);

    if (NULL == pUaDialog)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "SIP_SendSubscribe() exit---: Get UA Dialog Error:dialog_index=%d \r\n", index);
        return EV9000_SIPSTACK_INVITE_GET_UA_ERROR;
    }

    osip_strncpy(pUaDialog->strCallerID, caller_id, 128);
    osip_strncpy(pUaDialog->strCalleeID, callee_id, 128);

    osip_strncpy(pUaDialog->strRemoteIP, server_ip, 16);
    pUaDialog->iRemotePort = server_port;

    osip_strncpy(pUaDialog->strLocalIP, local_ip, 16);
    pUaDialog->iLocalPort = local_port;

    i = sip_subscribe(index, message_id, event, event_id, expires, msg, msg_len);

    if (i != 0)
    {
        ua_dialog_remove(index);
        SIP_DEBUG_TRACE(LOG_INFO, "SIP_SendSubscribe:ua_dialog_remove() index=%d \r\n", index);
        SIP_DEBUG_TRACE(LOG_ERROR, "SIP_SendSubscribe() exit---: sip_subscribe Error \r\n");
        return i;
    }

    return index;
}

int SIP_SubscribeRefresh(int dialog_index)
{
    int i = 0;
    sip_dialog_t* dialog = NULL;
    sip_subscription_t* sip_sub = NULL;
    ua_dialog_t* pUaDialog = NULL;

    osip_message_t* subscribe = NULL;
    osip_transaction_t* transaction = NULL;

    int socket = 0;
    char pcEventParam[32] = {0};
    char pcExpiresParam[32] = {0};

    if (!is_valid_dialog_index(dialog_index))
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "SIP_SubscribeRefresh() exit---: Dialog Index Error \r\n");
        return -1;
    }

    pUaDialog = ua_dialog_get(dialog_index);

    if (NULL == pUaDialog)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "SIP_SubscribeRefresh() exit---: Get UA Dialog Error:dialog_index=%d \r\n", dialog_index);
        return -1;
    }

    dialog = get_dialog_sip_dialog(dialog_index);

    if (dialog == NULL)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "SIP_SubscribeRefresh() exit---: Get UA Dialog Error:dialog_index=%d \r\n", dialog_index);
        return -1;
    }

    sip_sub = GetDialogSubscription(dialog_index);

    if (sip_sub == NULL)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "SIP_SubscribeRefresh() exit---: GetDialogSubscription Error:dialog_index=%d \r\n", dialog_index);
        return -1;
    }

    if (sip_sub->state != SUB_STATE_CONFIRMED)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "SIP_SubscribeRefresh() exit---: Subscription state Error:dialog_index=%d, state=%d \r\n", dialog_index, sip_sub->state);
        return -1;
    }

    i = generating_subscribe_with_dialog(&subscribe, dialog, pUaDialog->strCallerID, pUaDialog->strLocalIP, pUaDialog->iLocalPort);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "SIP_SubscribeRefresh() exit---: generating_subscribe_with_dialog Error:dialog_index=%d, state=%d \r\n", dialog_index, sip_sub->state);
        return -1;
    }

    snprintf(pcEventParam, 32, "%s;id=%s", sip_sub->event_type, sip_sub->id_param);
    msg_setevent(subscribe, pcEventParam);

    snprintf(pcExpiresParam, 32, "%d", sip_sub->duration);
    msg_set_expires(subscribe, pcExpiresParam);

    /* 获取socket */
    socket = get_socket_by_port(pUaDialog->iLocalPort);

    if (socket <= 0)
    {
        osip_message_free(subscribe);
        subscribe = NULL;
        SIP_DEBUG_TRACE(LOG_ERROR, "SIP_SubscribeRefresh() exit---: Get Socket Error \r\n");
        return EV9000_SIPSTACK_GET_SOCKET_ERROR;
    }

    i = osip_transaction_init(&transaction,
                              NICT,
                              g_recv_cell,
                              subscribe);

    if (0 != i)
    {
        osip_message_free(subscribe);
        subscribe = NULL;
        SIP_DEBUG_TRACE(LOG_ERROR, "SIP_SubscribeRefresh() exit---: Transaction Init Error \r\n");
        return EV9000_SIPSTACK_TRANSACTION_INIT_ERROR;
    }

    /* 设置transaction的socket */
    osip_transaction_set_in_socket(transaction, socket);
    osip_transaction_set_out_socket(transaction, socket);

    i = ul_sendmessage(transaction, subscribe);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "SIP_SubscribeRefresh(): ul_sendmessage Error \r\n");
        return EV9000_SIPSTACK_SEND_MESSAGE_ERROR;
    }

    update_dialog_as_uac(dialog_index, transaction, subscribe, DLG_EVENT_UPDATE);
    return 0;
}

int SIP_UnSubscribe(int dialog_index)
{
    int i = 0;
    sip_dialog_t* dialog = NULL;
    sip_subscription_t* sip_sub = NULL;
    ua_dialog_t* pUaDialog = NULL;

    osip_message_t* subscribe = NULL;
    osip_transaction_t* transaction = NULL;

    int socket = 0;
    char pcEventParam[32] = {0};

    if (!is_valid_dialog_index(dialog_index))
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "SIP_UnSubscribe() exit---: Dialog Index Error \r\n");
        return -1;
    }

    pUaDialog = ua_dialog_get(dialog_index);

    if (NULL == pUaDialog)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "SIP_UnSubscribe() exit---: Get UA Dialog Error:dialog_index=%d \r\n", dialog_index);
        return -1;
    }

    dialog = get_dialog_sip_dialog(dialog_index);

    if (dialog == NULL)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "SIP_UnSubscribe() exit---: Get UA Dialog Error:dialog_index=%d \r\n", dialog_index);
        return -1;
    }

    sip_sub = GetDialogSubscription(dialog_index);

    if (sip_sub == NULL)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "SIP_UnSubscribe() exit---: GetDialogSubscription Error:dialog_index=%d \r\n", dialog_index);
        return -1;
    }

    if (sip_sub->state != SUB_STATE_CONFIRMED)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "SIP_UnSubscribe() exit---: Subscription state Error:dialog_index=%d, state=%d \r\n", dialog_index, sip_sub->state);
        return -1;
    }

    i = generating_subscribe_with_dialog(&subscribe, dialog, pUaDialog->strCallerID, pUaDialog->strLocalIP, pUaDialog->iLocalPort);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "SIP_UnSubscribe() exit---: generating_subscribe_with_dialog Error:dialog_index=%d, state=%d \r\n", dialog_index, sip_sub->state);
        return -1;
    }

    snprintf(pcEventParam, 32, "%s;id=%s", sip_sub->event_type, sip_sub->id_param);
    msg_setevent(subscribe, pcEventParam);

    msg_set_expires(subscribe, (char*)"0");

    i = osip_transaction_init(&transaction,
                              NICT,
                              g_recv_cell,
                              subscribe);

    if (0 != i)
    {
        osip_message_free(subscribe);
        subscribe = NULL;
        SIP_DEBUG_TRACE(LOG_ERROR, "SIP_UnSubscribe() exit---: Transaction Init Error \r\n");
        return EV9000_SIPSTACK_TRANSACTION_INIT_ERROR;
    }

    /* 设置transaction的socket */
    osip_transaction_set_in_socket(transaction, socket);
    osip_transaction_set_out_socket(transaction, socket);

    i = ul_sendmessage(transaction, subscribe);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "SIP_UnSubscribe(): ul_sendmessage Error \r\n");
        return EV9000_SIPSTACK_SEND_MESSAGE_ERROR;
    }

    update_dialog_as_uac(dialog_index, transaction, subscribe, DLG_EVENT_UPDATE);
    return 0;
}

/*****************************************************************************
 函 数 名  : SIP_SendNotify
 功能描述  : 发送通知消息
 输入参数  : char* message_id
             char* caller_id
             char* callee_id
             char* event
             int event_id
             int expires
             char* local_ip
             int local_port
             char* server_ip
             int server_port
             char* msg
             int msg_len
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年6月10日 星期三
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int SIP_SendNotify(char* message_id, char* caller_id, char* callee_id, char* event, int event_id, int expires, char* local_ip, int local_port, char* server_ip, int server_port, char* msg, int msg_len)
{
    int i = 0;

#if 1
    SIP_DEBUG_TRACE(LOG_INFO,  "SIP_SendNotify() \
    \r\n In Para: \
    \r\n message_id=%s \
    \r\n caller_id=%s \
    \r\n callee_id=%s \
    \r\n local_ip=%s \
    \r\n local_port=%d \
    \r\n server_ip=%s \
    \r\n server_port=%d \
    \r\n event=%s \
    \r\n event_id=%d \
    \r\n msg_len=%d \
    \r\n ", message_id, caller_id, callee_id, local_ip, local_port, server_ip, server_port, event, event_id, msg_len);

#endif

    if ((NULL == caller_id) || (caller_id[0] == '\0')
        || (NULL == callee_id) || (callee_id[0] == '\0')
        || (NULL == local_ip) || (local_ip[0] == '\0') || (local_port <= 0)
        || (NULL == server_ip) || (server_ip[0] == '\0') || (server_port <= 0)
        || (NULL == event) || (event_id <= 0))
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "SIP_SendNotify() exit---: Param Error \r\n");
        return EV9000_SIPSTACK_PARAM_ERROR;
    }

    i = sip_notify(message_id, caller_id, callee_id, event, event_id, expires, server_ip, server_port, local_ip, local_port, msg, msg_len);

    return i;
}

/*****************************************************************************
 函 数 名  : SIP_SendNotify_By_TCP
 功能描述  : 基于TCP发送通知消息
 输入参数  : char* message_id
             char* caller_id
             char* callee_id
             char* event
             int event_id
             int expires
             char* local_ip
             int local_port
             char* server_ip
             int server_port
             char* msg
             int msg_len
             int tcp_scoket
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年2月21日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int SIP_SendNotify_By_TCP(char* message_id, char* caller_id, char* callee_id, char* event, int event_id, int expires, char* local_ip, int local_port, char* server_ip, int server_port, char* msg, int msg_len, int tcp_scoket)
{
    int i = 0;

#if 1
    SIP_DEBUG_TRACE(LOG_INFO,  "SIP_SendNotify_By_TCP() \
    \r\n In Para: \
    \r\n message_id=%s \
    \r\n caller_id=%s \
    \r\n callee_id=%s \
    \r\n local_ip=%s \
    \r\n local_port=%d \
    \r\n server_ip=%s \
    \r\n server_port=%d \
    \r\n event=%s \
    \r\n event_id=%d \
    \r\n msg_len=%d \
    \r\n tcp_scoket=%d \
    \r\n ", message_id, caller_id, callee_id, local_ip, local_port, server_ip, server_port, event, event_id, msg_len, tcp_scoket);

#endif

    if ((NULL == caller_id) || (caller_id[0] == '\0')
        || (NULL == callee_id) || (callee_id[0] == '\0')
        || (NULL == local_ip) || (local_ip[0] == '\0') || (local_port <= 0)
        || (NULL == server_ip) || (server_ip[0] == '\0') || (server_port <= 0)
        || (NULL == event) || (event_id <= 0))
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "SIP_SendNotify_By_TCP() exit---: Param Error \r\n");
        return EV9000_SIPSTACK_PARAM_ERROR;
    }

    if (tcp_scoket < 0)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "SIP_SendNotify_By_TCP() exit---: TCP Socket Error \r\n");
        return EV9000_SIPSTACK_PARAM_ERROR;
    }

    i = sip_notify_for_tcp(message_id, caller_id, callee_id, event, event_id, expires, server_ip, server_port, local_ip, local_port, msg, msg_len, tcp_scoket);

    return i;
}

int SIP_SendNotifyWithinDialog(int dialog_index, char* body, int body_len)
{
    int i = 0;
    ua_dialog_t* pUaDialog = NULL;

    SIP_DEBUG_TRACE(LOG_INFO,  "SIP_SendNotifyWithinDialog() \
    \r\n In Para: \
    \r\n dialog_index=%d \
    \r\n body_len=%d \
    \r\n ", dialog_index, body_len);

    if (!is_valid_dialog_index(dialog_index))
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "SIP_SendNotifyWithinDialog() exit---: Dialog Index Error \r\n");
        return -1;
    }

    pUaDialog = ua_dialog_get(dialog_index);

    if (NULL == pUaDialog)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "SIP_SendNotifyWithinDialog() exit---: Get UA Dialog Error:dialog_index=%d \r\n", dialog_index);
        return -1;
    }

    i = sip_notify_within_dialog(dialog_index, body, body_len);

    return i;
}

/*****************************************************************************
 函 数 名  : SIP_AnswerToInvite
 功能描述  : 应答Invite消息
 输入参数  : int dialog_index
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
int SIP_AnswerToInvite(int dialog_index, int code, char* reasonphrase)
{
#ifdef WIN32
    static int iExceptionCount = 0; //异常计数

    try
    {
#endif

        int i = 0;
        ua_dialog_t* pUaDialog = NULL;

#if 0

        SIP_DEBUG_TRACE(LOG_INFO,  "SIP_AnswerToInvite() \
    \r\n In Para: \
    \r\n dialog_index=%d \
    \r\n code=%d \
    \r\n ", dialog_index, code);

#endif

        if (!is_valid_dialog_index(dialog_index))
        {
            SIP_DEBUG_TRACE(LOG_ERROR, "SIP_AnswerToInvite() exit---: Dialog Index Error \r\n");
            return -1;
        }

        pUaDialog = ua_dialog_get(dialog_index);

        if (NULL == pUaDialog)
        {
            SIP_DEBUG_TRACE(LOG_ERROR, "SIP_AnswerToInvite() exit---: Get UA Dialog Error:dialog_index=%d \r\n", dialog_index);
            return -1;
        }

        if (407 == code)
        {
            if (NULL != pUaDialog->pServerSIP && NULL != pUaDialog->pServerTr)
            {
                osip_message_t* response = NULL;
                i = cs_generating_response_default(pUaDialog->pServerSIP, 407, NULL, &response);

                cs_response_add_proxy_authenticate(response, pUaDialog->strLocalIP);
                i = ul_sendmessage(pUaDialog->pServerTr, response);

                if (0 != i)
                {
                    SIP_DEBUG_TRACE(LOG_ERROR, "SIP_AnswerToInvite() exit---: ul_sendmessage Error \r\n");
                }
            }

            ua_dialog_remove(dialog_index);
            SIP_DEBUG_TRACE(LOG_INFO, "SIP_AnswerToInvite() ua_dialog_remove:dialog_index=%d \r\n", dialog_index);
        }
        else
        {
            i = sip_3456xxxx_answer_to_invite(dialog_index, code, reasonphrase);
        }

        return 0;

#ifdef WIN32
    }
    catch (...)
    {
        //i=-1;  // 异常
        iExceptionCount++;  //计数

        if (iExceptionCount > 100)
        {
            printf("Exception: CCenterCtrl::DoInviteCmd 异常超过100次，退出....\n");
            exit(-1);
        }

        printf("Exception: CCenterCtrl::DoInviteCmd SIP_AcceptInvite,异常次数:%d\n", iExceptionCount);
    }

#endif
}

/*****************************************************************************
 函 数 名  : SIP_AnswerToInviteForSessionExpires
 功能描述  : 在会话定时器间隔太小的情况下回应INVITE
 输入参数  : int dialog_index
                            int min_se
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年9月2日 星期一
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int SIP_AnswerToInviteForSessionExpires(int dialog_index, int min_se)
{
    int i = 0;
    ua_dialog_t* pUaDialog = NULL;
    char pcMinSessionExpires[16] = {0};

#if 0

    SIP_DEBUG_TRACE(LOG_INFO,  "SIP_AnswerToInviteForSessionExpires() \
    \r\n In Para: \
    \r\n dialog_index=%d \
    \r\n code=%d \
    \r\n ", dialog_index, code);

#endif

    if (!is_valid_dialog_index(dialog_index))
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "SIP_AnswerToInviteForSessionExpires() exit---: Dialog Index Error \r\n");
        return -1;
    }

    pUaDialog = ua_dialog_get(dialog_index);

    if (NULL == pUaDialog)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "SIP_AnswerToInviteForSessionExpires() exit---: Get UA Dialog Error:dialog_index=%d \r\n", dialog_index);
        return -1;
    }

    osip_message_t* response = NULL;
    i = cs_generating_response_default(pUaDialog->pServerSIP, 422, (char*)"Session Interval Too Small", &response);

    snprintf(pcMinSessionExpires, 16, "%d", min_se);
    msg_set_min_se(response, pcMinSessionExpires);

    i = ul_sendmessage(pUaDialog->pServerTr, response);

    if (0 != i)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "SIP_AnswerToInviteForSessionExpires(): ul_sendmessage Error \r\n");
    }

    ua_dialog_remove(dialog_index);
    SIP_DEBUG_TRACE(LOG_INFO, "SIP_AnswerToInviteForSessionExpires() ua_dialog_remove:dialog_index=%d \r\n", dialog_index);

    return 0;
}

/*****************************************************************************
 函 数 名  : SIP_AnswerToBye
 功能描述  : 应答Bye消息
 输入参数  : int dialog_index
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
int SIP_AnswerToBye(int dialog_index, int code, char* reasonphrase)
{
    int i = 0;
    ua_dialog_t* pUaDialog = NULL;

    return 0;

#if 0
    SIP_DEBUG_TRACE(LOG_INFO,  "SIP_AnswerToBye() \
    \r\n In Para: \
    \r\n dialog_index=%d \
    \r\n code=%d \
    \r\n ", dialog_index, code);

#endif

    if (!is_valid_dialog_index(dialog_index))
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "SIP_AnswerToBye() exit---: Dialog Index Error \r\n");
        return -1;
    }

    pUaDialog = ua_dialog_get(dialog_index);

    if (NULL == pUaDialog)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "SIP_AnswerToBye() exit---: Get UA Dialog Error:dialog_index=%d \r\n", dialog_index);
        return -1;
    }

    if (401 == code)
    {
        if (NULL != pUaDialog->pServerSIP && NULL != pUaDialog->pServerTr)
        {
            osip_message_t* response = NULL;
            i = cs_generating_response_default(pUaDialog->pServerSIP, 401, NULL, &response);

            cs_response_add_proxy_authenticate(response, pUaDialog->strLocalIP);
            i = ul_sendmessage(pUaDialog->pServerTr, response);

            if (0 != i)
            {
                SIP_DEBUG_TRACE(LOG_ERROR, "SIP_AnswerToBye(): ul_sendmessage Error \r\n");
            }
        }
    }
    else
    {
        if (NULL != pUaDialog->pServerSIP && NULL != pUaDialog->pServerTr)
        {
            osip_message_t* response = NULL;
            i = cs_generating_response_default(pUaDialog->pServerSIP, code, reasonphrase, &response);
            i = ul_sendmessage(pUaDialog->pServerTr, response);

            if (0 != i)
            {
                SIP_DEBUG_TRACE(LOG_ERROR, "SIP_AnswerToBye(): ul_sendmessage Error \r\n");
            }
        }
    }

    ua_dialog_remove(dialog_index);
    SIP_DEBUG_TRACE(LOG_INFO, "SIP_AnswerToBye() ua_dialog_remove:dialog_index=%d \r\n", dialog_index);

    return 0;
}

/*****************************************************************************
 函 数 名  : SIP_GetInviteDialogAuthorization
 功能描述  : 获取Invite的认证信息
 输入参数  : int dialog_index
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年5月23日 星期四
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
osip_authorization_t* SIP_GetInviteDialogAuthorization(int dialog_index)
{
    ua_dialog_t* pUaDialog = NULL;

#if 0
    SIP_DEBUG_TRACE(LOG_INFO,  "SIP_GetInviteDialogAuthorization() \
    \r\n In Para: \
    \r\n dialog_index=%d \
    \r\n ", dialog_index);
#endif

    if (!is_valid_dialog_index(dialog_index))
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "SIP_GetInviteDialogAuthorization() exit---: Dialog Index Error \r\n");
        return NULL;
    }

    pUaDialog = ua_dialog_get(dialog_index);

    if (NULL == pUaDialog)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "SIP_GetInviteDialogAuthorization() exit---: Get UA Dialog Error:dialog_index=%d \r\n", dialog_index);
        return NULL;
    }

    return pUaDialog->pAuthorization;
}

int SIP_GetInviteDialogSessionExpires(int dialog_index)
{
    ua_dialog_t* pUaDialog = NULL;

#if 0

    SIP_DEBUG_TRACE(LOG_INFO,  "SIP_GetInviteDialogAuthorization() \
    \r\n In Para: \
    \r\n dialog_index=%d \
    \r\n ", dialog_index);

#endif

    if (!is_valid_dialog_index(dialog_index))
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "SIP_GetInviteDialogSessionExpires() exit---: Dialog Index Error \r\n");
        return -1;
    }

    pUaDialog = ua_dialog_get(dialog_index);

    if (NULL == pUaDialog)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "SIP_GetInviteDialogSessionExpires() exit---: Get UA Dialog Error:dialog_index=%d \r\n", dialog_index);
        return -1;
    }

    return pUaDialog->iSessionExpires;
}

/*****************************************************************************
 函 数 名  : SIP_GetDialogFromHost
 功能描述  : 获取会话的来源host
 输入参数  : int dialog_index
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年5月23日 星期四
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
char* SIP_GetDialogFromHost(int dialog_index)
{
    ua_dialog_t* pUaDialog = NULL;

#if 0
    SIP_DEBUG_TRACE(LOG_INFO,  "SIP_GetDialogFromHost() \
    \r\n In Para: \
    \r\n dialog_index=%d \
    \r\n ", dialog_index);
#endif

    if (!is_valid_dialog_index(dialog_index))
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "SIP_GetDialogFromHost() exit---: Dialog Index Error \r\n");
        return NULL;
    }

    pUaDialog = ua_dialog_get(dialog_index);

    if (NULL == pUaDialog)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "SIP_GetDialogFromHost() exit---: Get UA Dialog Error:dialog_index=%d \r\n", dialog_index);
        return NULL;
    }

    return pUaDialog->strRemoteIP;
}

/*****************************************************************************
 函 数 名  : SIP_GetDialogFromPort
 功能描述  : 获取会话的来源端口
 输入参数  : int dialog_index
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年5月23日 星期四
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int SIP_GetDialogFromPort(int dialog_index)
{
    ua_dialog_t* pUaDialog = NULL;

#if 0
    SIP_DEBUG_TRACE(LOG_INFO,  "SIP_GetDialogFromPort() \
    \r\n In Para: \
    \r\n dialog_index=%d \
    \r\n ", dialog_index);
#endif

    if (!is_valid_dialog_index(dialog_index))
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "SIP_GetDialogFromPort() exit---: Dialog Index Error \r\n");
        return -1;
    }

    pUaDialog = ua_dialog_get(dialog_index);

    if (NULL == pUaDialog)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "SIP_GetDialogFromPort() exit---: Get UA Dialog Error:dialog_index=%d \r\n", dialog_index);
        return -1;
    }

    return pUaDialog->iRemotePort;
}

/*****************************************************************************
 函 数 名  : SIP_GetDialogCallID
 功能描述  : 获取Dialog的会话ID
 输入参数  : int dialog_index
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年8月14日 星期三
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
char* SIP_GetDialogCallID(int dialog_index)
{
    sip_dialog_t* pSipDlg = NULL;

#if 0

    SIP_DEBUG_TRACE(LOG_INFO,  "SIP_GetDialogCallID() \
    \r\n In Para: \
    \r\n dialog_index=%d \
    \r\n ", dialog_index);

#endif

    if (!is_valid_dialog_index(dialog_index))
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "SIP_GetDialogCallID() exit---: Dialog Index Error \r\n");
        return NULL;
    }

    pSipDlg = get_dialog_sip_dialog(dialog_index);

    if (NULL == pSipDlg)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "SIP_GetDialogCallID() exit---: Get SIP Dialog Error \r\n");
        return NULL;
    }

    return pSipDlg->call_id;
}

/*****************************************************************************
 函 数 名  : SIP_AnswerToSipMessage
 功能描述  : 应答SIP message消息
 输入参数  : char* call_id
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
int SIP_AnswerToSipMessage(char* call_id, int code, char* reasonphrase)
{
    int i = 0;
    int iRet = 0;
    sip_message_t* pSipMessage = NULL;
    int msg_pos = -1;
    osip_message_t* response = NULL;
    ua_dialog_t* pUaDialog = NULL;

    return 0;

#if 0
    SIP_DEBUG_TRACE(LOG_INFO,  "SIP_AnswerToSipMessage() \
    \r\n In Para: \
    \r\n call_id=%s \
    \r\n code=%d \
    \r\n ", call_id, code);

#endif

    if (call_id == NULL || call_id[0] == '\0')
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "SIP_AnswerToSipMessage() exit---: Param Error \r\n");
        return -1;
    }

    iRet = sip_message_list_lock();

    msg_pos = sip_message_find2(call_id);

    if (msg_pos < 0)
    {
        iRet = sip_message_list_unlock();

        //SIP_DEBUG_TRACE(LOG_DEBUG, "SIP_AnswerToSipMessage() exit---: Find SIP Message Error \r\n");
        return -1;
    }

    pSipMessage = sip_message_get(msg_pos);

    if (NULL == pSipMessage)
    {
        iRet = sip_message_list_unlock();

        //SIP_DEBUG_TRACE(LOG_DEBUG, "SIP_AnswerToSipMessage() exit---: Get SIP Message Error \r\n");
        return -1;
    }

    if (!is_valid_dialog_index(pSipMessage->dialog_index))
    {
        pUaDialog = NULL;
    }
    else
    {
        pUaDialog = ua_dialog_get(pSipMessage->dialog_index);
    }


    if (NULL == pUaDialog)
    {
        i = generating_response_default(&response, NULL, code, pSipMessage->sip, reasonphrase);

        if (i != 0)
        {
            iRet = sip_message_list_unlock();

            //SIP_DEBUG_TRACE(LOG_DEBUG, "SIP_AnswerToSipMessage() exit---: Generating Default Response Error \r\n");
            return -1;
        }
    }
    else
    {
        i = generating_response_default(&response, pUaDialog->pSipDialog, code, pSipMessage->sip, reasonphrase);

        if (i != 0)
        {
            iRet = sip_message_list_unlock();

            //SIP_DEBUG_TRACE(LOG_DEBUG, "SIP_AnswerToSipMessage() exit---: Generating Default Response Error \r\n");
            return -1;
        }
    }

    i = osip_message_set_content_length(response, (char*)"0");

    i = ul_sendmessage(pSipMessage->tr, response);

    if (0 != i)
    {
        //SIP_DEBUG_TRACE(LOG_ERROR, "SIP_AnswerToSipMessage(): ul_sendmessage Error \r\n");
    }

    i = sip_message_remove2(msg_pos);

    iRet = sip_message_list_unlock();

    return 0;
}

/*****************************************************************************
 函 数 名  : SIP_AnswerToSipInfo
 功能描述  : 应答SIP info消息
 输入参数  : char* call_id
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
int SIP_AnswerToSipInfo(char* call_id, int code, char* reasonphrase)
{
    int i = 0;
    int iRet = 0;
    sip_message_t* pSipMessage = NULL;
    int msg_pos = -1;
    osip_message_t* response = NULL;
    ua_dialog_t* pUaDialog = NULL;

    return 0;

#if 0
    SIP_DEBUG_TRACE(LOG_INFO,  "SIP_AnswerToSipInfo() \
    \r\n In Para: \
    \r\n call_id=%s \
    \r\n code=%d \
    \r\n ", call_id, code);

#endif

    if (call_id == NULL || call_id[0] == '\0')
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "SIP_AnswerToSipInfo() exit---: Param Error \r\n");
        return -1;
    }

    iRet = sip_message_list_lock();

    msg_pos = sip_message_find2(call_id);

    if (msg_pos < 0)
    {
        iRet = sip_message_list_unlock();

        //SIP_DEBUG_TRACE(LOG_DEBUG, "SIP_AnswerToSipInfo() exit---: Find SIP Message Error \r\n");
        return -1;
    }

    pSipMessage = sip_message_get(msg_pos);

    if (NULL == pSipMessage)
    {
        iRet = sip_message_list_unlock();

        //SIP_DEBUG_TRACE(LOG_DEBUG, "SIP_AnswerToSipInfo() exit---: Get SIP Message Error \r\n");
        return -1;
    }

    if (!is_valid_dialog_index(pSipMessage->dialog_index))
    {
        pUaDialog = NULL;
    }
    else
    {
        pUaDialog = ua_dialog_get(pSipMessage->dialog_index);
    }


    if (NULL == pUaDialog)
    {
        i = generating_response_default(&response, NULL, code, pSipMessage->tr->orig_request, reasonphrase);

        if (i != 0)
        {
            iRet = sip_message_list_unlock();

            //SIP_DEBUG_TRACE(LOG_DEBUG, "SIP_AnswerToSipInfo() exit---: Generating Default Response Error \r\n");
            return -1;
        }
    }
    else
    {
        i = generating_response_default(&response, pUaDialog->pSipDialog, code, pSipMessage->tr->orig_request, reasonphrase);

        if (i != 0)
        {
            iRet = sip_message_list_unlock();

            //SIP_DEBUG_TRACE(LOG_DEBUG, "SIP_AnswerToSipInfo() exit---: Generating Default Response Error \r\n");
            return -1;
        }
    }

    i = osip_message_set_content_length(response, (char*)"0");

    i = ul_sendmessage(pSipMessage->tr, response);

    if (0 != i)
    {
        //SIP_DEBUG_TRACE(LOG_ERROR, "SIP_AnswerToSipInfo(): ul_sendmessage Error \r\n");
    }

    i = sip_message_remove2(msg_pos);

    iRet = sip_message_list_unlock();

    return 0;
}

void SipAnswerToSubscribe(int dialog_index, sip_subscription_t* sip_sub, osip_transaction_t* tr, int code)
{
    ua_dialog_t* pUaDialog = NULL;
    osip_message_t* response = NULL;
    int i = 0;
    char sexpires[11] = {0};

    if (tr == NULL)
    {
        return;
    }

    if (!is_valid_dialog_index(dialog_index))
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "SipAnswerToSubscribe() exit---: Dialog Index Error \r\n");
        return;
    }

    pUaDialog = ua_dialog_get(dialog_index);

    if (NULL == pUaDialog)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "SipAnswerToSubscribe() exit---: Get UA Dialog Error:dialog_index=%d \r\n", dialog_index);
        return;
    }

    i = generating_response_default(&response, pUaDialog->pSipDialog, code, tr->orig_request, NULL);

    if (i != 0)
    {
        return;
    }

    if (pUaDialog)
    {
        if (code > 100 && code < 200)
        {
            update_dialog_as_uas(dialog_index, tr, response, DLG_EVENT_1XXTAG);
        }
        else if (code >= 200 && code < 300)
        {
            if (sip_sub)
            {
                snprintf(sexpires, 11, "%d", sip_sub->duration);
            }
            else
            {
                snprintf(sexpires, 11, "%d", "0");
            }

            osip_message_set_expires(response, sexpires);

            /* request that estabish a dialog: */
            /* 12.1.1 UAS Behavior */
            i = complete_answer_that_establish_a_dialog(response, tr->orig_request, pUaDialog->strCalleeID, pUaDialog->strLocalIP, pUaDialog->iLocalPort);

            update_dialog_as_uas(dialog_index, tr, response, DLG_EVENT_2XX);
        }
        else
        {
            update_dialog_as_uas(dialog_index, tr, response, DLG_EVENT_REJECTED);
        }
    }

    osip_message_set_content_length(response, "0");
    ul_sendmessage(tr, response);

    return;
}

/*****************************************************************************
 函 数 名  : SIP_GetInviteDialogRemoteSDP
 功能描述  : 获取会话的对端SDP信息
 输入参数  : int dialog_index
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年5月23日 星期四
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
sdp_message_t* SIP_GetInviteDialogRemoteSDP(int dialog_index)
{
    ua_dialog_t* pUaDialog = NULL;

#if 0
    SIP_DEBUG_TRACE(LOG_INFO,  "SIP_GetInviteDialogRemoteSDP() \
    \r\n In Para: \
    \r\n dialog_index=%d \
    \r\n ", dialog_index);
#endif

    if (!is_valid_dialog_index(dialog_index))
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "SIP_GetInviteDialogRemoteSDP() exit---: Dialog Index Error \r\n");
        return NULL;
    }

    pUaDialog = ua_dialog_get(dialog_index);

    if (NULL == pUaDialog)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "SIP_GetInviteDialogRemoteSDP() exit---: Ge UA Dialog Error:dialog_index=%d \r\n", dialog_index);
        return NULL;
    }

    if (NULL == pUaDialog->pRemoteSDP)
    {
        return NULL;
    }
    else
    {
        return pUaDialog->pRemoteSDP;
    }
}

/*****************************************************************************
 函 数 名  : SIP_GetOutDialogFromHost
 功能描述  : 获取会话外消息来源host
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
char* SIP_GetOutDialogFromHost(char* call_id)
{
    int iRet = 0;
    sip_message_t* pSipMessage = NULL;
    int msg_pos = -1;
    osip_message_t* sip = NULL;

#if 0

    SIP_DEBUG_TRACE(LOG_INFO,  "SIP_GetOutDialogFromHost() \
    \r\n In Para: \
    \r\n call_id=%s \
    \r\n ", call_id);

#endif

    if (call_id == NULL || call_id[0] == '\0')
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "SIP_GetOutDialogFromHost() exit---: Param Error \r\n");
        return NULL;
    }

    iRet = sip_message_list_lock();

    msg_pos = sip_message_find2(call_id);

    if (msg_pos < 0)
    {
        iRet = sip_message_list_unlock();

        //SIP_DEBUG_TRACE(LOG_DEBUG, "SIP_GetOutDialogFromHost() exit---: Find SIP Message Error \r\n");
        return NULL;
    }

    pSipMessage = sip_message_get(msg_pos);

    if (NULL == pSipMessage)
    {
        iRet = sip_message_list_unlock();

        //SIP_DEBUG_TRACE(LOG_DEBUG, "SIP_GetOutDialogFromHost() exit---: Get SIP Message Error \r\n");
        return NULL;
    }

    iRet = sip_message_list_unlock();

    sip = pSipMessage->sip;

    if (NULL == sip)
    {
        //SIP_DEBUG_TRACE(LOG_DEBUG, "SIP_GetOutDialogFromHost() exit---: SIP Message NULL \r\n");
        return NULL;
    }

    return get_message_from_host(sip);
}

/*****************************************************************************
 函 数 名  : SIP_GetOutDialogFromPort
 功能描述  : 获取会话外消息来源port
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
int SIP_GetOutDialogFromPort(char* call_id)
{
    int iRet = 0;
    sip_message_t* pSipMessage = NULL;
    int msg_pos = -1;
    osip_message_t* sip = NULL;

#if 0
    SIP_DEBUG_TRACE(LOG_INFO,  "SIP_GetOutDialogFromPort() \
    \r\n In Para: \
    \r\n call_id=%s \
    \r\n ", call_id);

#endif

    if (call_id == NULL || call_id[0] == '\0')
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "SIP_GetOutDialogFromPort() exit---: Param Error \r\n");
        return -1;
    }

    iRet = sip_message_list_lock();

    msg_pos = sip_message_find2(call_id);

    if (msg_pos < 0)
    {
        iRet = sip_message_list_unlock();

        //SIP_DEBUG_TRACE(LOG_DEBUG, "SIP_GetOutDialogFromPort() exit---: Find SIP Message Error \r\n");
        return -1;
    }

    pSipMessage = sip_message_get(msg_pos);

    if (NULL == pSipMessage)
    {
        iRet = sip_message_list_unlock();

        //SIP_DEBUG_TRACE(LOG_DEBUG, "SIP_GetOutDialogFromPort() exit---: Get SIP Message Error \r\n");
        return -1;
    }

    iRet = sip_message_list_unlock();

    sip = pSipMessage->sip;

    if (NULL == sip)
    {
        //SIP_DEBUG_TRACE(LOG_DEBUG, "SIP_GetOutDialogFromPort() exit---: SIP Message NULL \r\n");
        return -1;
    }

    return get_message_from_port(sip);
}

/*****************************************************************************
 函 数 名  : SIP_GetSipMsgDialogIndex
 功能描述  : 获取sip消息的index
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
int SIP_GetSipMsgDialogIndex(char* call_id)
{
    int iRet = 0;
    sip_message_t* pSipMessage = NULL;
    int msg_pos = -1;

#if 0
    SIP_DEBUG_TRACE(LOG_INFO,  "SIP_GetSipMsgDialogIndex() \
    \r\n In Para: \
    \r\n call_id=%s \
    \r\n ", call_id);
#endif

    if (call_id == NULL || call_id[0] == '\0')
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "SIP_GetSipMsgDialogIndex() exit---: Param Error \r\n");
        return -1;
    }

    iRet = sip_message_list_lock();

    msg_pos = sip_message_find2(call_id);

    if (msg_pos < 0)
    {
        iRet = sip_message_list_unlock();

        //SIP_DEBUG_TRACE(LOG_DEBUG, "SIP_GetSipMsgDialogIndex() exit---: Find SIP Message Error \r\n");
        return -1;
    }

    pSipMessage = sip_message_get(msg_pos);

    if (NULL == pSipMessage)
    {
        iRet = sip_message_list_unlock();

        //SIP_DEBUG_TRACE(LOG_DEBUG, "SIP_GetSipMsgDialogIndex() exit---: Get SIP Message Error \r\n");
        return -1;
    }

    iRet = sip_message_list_unlock();

    if (!is_valid_dialog_index(pSipMessage->dialog_index))
    {
        //SIP_DEBUG_TRACE(LOG_DEBUG, "SIP_GetSipMsgDialogIndex() exit---: Dialog Index Error \r\n");
        return -1;
    }
    else
    {
        return pSipMessage->dialog_index;
    }
}

/*****************************************************************************
 函 数 名  : SIP_GeneratingSDPAnswer
 功能描述  : 根据对端SDP信息协商生成本地SDP信息
 输入参数  : int dialog_index
                            sdp_t** local_sdp
                            char* audio_port
                            char* video_port
                            char* localip
                            char* s_name
                            int start_time
                            int end_time
                            int play_time,
                            int media_direction
                            int audio_code_type
                            int video_code_type
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年6月9日 星期日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int SIP_GeneratingSDPAnswer(int dialog_index, sdp_message_t** local_sdp, char* audio_port, char* video_port, char* localip, char* s_name, int start_time, int end_time, int play_time, int media_direction, int audio_code_type, int video_code_type, sdp_extend_param_t* pSDPExtendParm)
{
    int i = 0;
    ua_dialog_t* pUaDialog = NULL;
    char* onvif_url = NULL;
#if 0

    SIP_DEBUG_TRACE(LOG_INFO,  "SIP_GeneratingSDPAnswer() \
    \r\n In Para: \
    \r\n dialog_index=%d \
    \r\n audio_port=%s \
    \r\n video_port=%s \
    \r\n localip=%s \
    \r\n s_name=%s \
    \r\n start_time=%d \
    \r\n end_time=%d \
    \r\n media_direction=%d \
    \r\n audio_code_type=%d \
    \r\n video_code_type=%d \
    \r\n ", dialog_index, audio_port, video_port, localip, s_name, start_time, end_time, media_direction, audio_code_type, video_code_type);

#endif

    if (NULL == localip || localip[0] == '\0')
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "SIP_GeneratingSDPAnswer() exit---: Param Error \r\n");
        return -1;
    }

    if (!is_valid_dialog_index(dialog_index))
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "SIP_GeneratingSDPAnswer() exit---: Dialog Index Error \r\n");
        return -1;
    }

    pUaDialog = ua_dialog_get(dialog_index);

    if (NULL == pUaDialog)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "SIP_GeneratingSDPAnswer() exit---: Get UA Dialog Error:dialog_index=%d \r\n", dialog_index);
        return -1;
    }

    if (pUaDialog->pRemoteSDP != NULL)
    {
        i = generating_sdp_answer(pUaDialog->pRemoteSDP, local_sdp, audio_port, video_port, localip, audio_code_type, video_code_type);
    }
    else
    {
        i = sdp_build_offer(NULL, local_sdp, audio_port, video_port, localip, s_name, start_time, end_time, play_time, media_direction, audio_code_type, video_code_type);
    }

    if (NULL != pSDPExtendParm)
    {
        if (pSDPExtendParm->onvif_url[0] != '\0')
        {
            onvif_url = osip_getcopy(pSDPExtendParm->onvif_url);

            if (NULL != onvif_url)
            {
                i = sdp_message_u_uri_set(*local_sdp, onvif_url);
            }
        }
    }

    return i;
}

/*****************************************************************************
 函 数 名  : SIP_ShowSIPUATask
 功能描述  : 显示SIP UA Dialog 信息
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
void SIP_ShowSIPUATask(int sock)
{
    int index = -1;
    char strLine[] = "\r-------------------------------------------------------------------------------------------------------------------------\r\n";
    char strHead[] = "\rUA Index    CallerID             CalleeID             Remote IP        Local IP        SessionExpires SessionExpiresCount\r\n";
    ua_dialog_t* pUaDialog = NULL;
    sip_dialog_t* pSipDlg = NULL;
    used_UA_Dialog_Iterator Itr;
    char rbuf[256] = {0};

    if (sock > 0)
    {
        send(sock, strLine, strlen(strLine), 0);
        send(sock, strHead, strlen(strHead), 0);
    }

    USED_UA_SMUTEX_LOCK();

    if (g_UsedUaDialogQueue.size() <= 0)
    {
        USED_UA_SMUTEX_UNLOCK();
        return;
    }

    for (Itr = g_UsedUaDialogQueue.begin(); Itr != g_UsedUaDialogQueue.end();)
    {
        index = *Itr;

        if (!is_valid_dialog_index(index))
        {
            Itr = g_UsedUaDialogQueue.erase(Itr);
            continue;
        }

        pUaDialog = g_UaDialogMap[index];

        if (NULL == pUaDialog)
        {
            Itr++;
            continue;
        }

        pSipDlg = pUaDialog->pSipDialog;

        if (NULL == pSipDlg)
        {
            Itr++;
            continue;
        }

        if (pSipDlg->state == DLG_TERMINATED)
        {
            Itr++;
            continue;
        }

        snprintf(rbuf, 256, "\r%-11d %-20s %-20s %-16s %-16s %-14d %-19d\r\n", index, pUaDialog->strCallerID, pUaDialog->strCalleeID, pUaDialog->strRemoteIP, pUaDialog->strLocalIP, pUaDialog->iSessionExpires, pUaDialog->iSessionExpiresCount);

        if (sock > 0)
        {
            send(sock, rbuf, strlen(rbuf), 0);
        }

        Itr++;
    }

    if (sock > 0)
    {
        send(sock, strLine, strlen(strLine), 0);
    }

    USED_UA_SMUTEX_UNLOCK();
    return;
}

/*****************************************************************************
 函 数 名  : SIP_ShowSIPUADetail
 功能描述  : 显示显示SIP UA Dialog 详细信息
 输入参数  : int sock
                            int ua_index
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年7月29日 星期一
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void SIP_ShowSIPUADetail(int sock, int ua_index)
{
    char strLine[] = "\r-------------------------------------------------------------------------------\r\n";
    ua_dialog_t* pUaDialog = NULL;
    sip_dialog_t* pSipDlg = NULL;
    char rbuf[128] = {0};

    pUaDialog = ua_dialog_get(ua_index);

    if (NULL == pUaDialog)
    {
        return;
    }

    pSipDlg = pUaDialog->pSipDialog;

    if (NULL == pSipDlg)
    {
        return;
    }

    if (pSipDlg->state == DLG_TERMINATED)
    {
        return;
    }

    if (sock > 0)
    {
        send(sock, strLine, strlen(strLine), 0);
    }

    /* 索引 */
    snprintf(rbuf, 128, "\rUA Index                :%d\r\n", ua_index);

    if (sock > 0)
    {
        send(sock, rbuf, strlen(rbuf), 0);
    }

    /* Caller ID */
    snprintf(rbuf, 128, "\rCaller ID               :%s\r\n", pUaDialog->strCallerID);

    if (sock > 0)
    {
        send(sock, rbuf, strlen(rbuf), 0);
    }

    /* Callee ID */
    snprintf(rbuf, 128, "\rCallee ID               :%s\r\n", pUaDialog->strCalleeID);

    if (sock > 0)
    {
        send(sock, rbuf, strlen(rbuf), 0);
    }

    /* Remote IP */
    snprintf(rbuf, 128, "\rRemote IP               :%s\r\n", pUaDialog->strRemoteIP);

    if (sock > 0)
    {
        send(sock, rbuf, strlen(rbuf), 0);
    }

    /* Remote Port */
    snprintf(rbuf, 128, "\rRemote Port             :%d\r\n", pUaDialog->iRemotePort);

    if (sock > 0)
    {
        send(sock, rbuf, strlen(rbuf), 0);
    }

    /* Remote RTP IP */
    snprintf(rbuf, 128, "\rRemote RTP IP           :%s\r\n", pUaDialog->strRemoteRTPIP);

    if (sock > 0)
    {
        send(sock, rbuf, strlen(rbuf), 0);
    }

    /* Remote RTP Port */
    snprintf(rbuf, 128, "\rRemote RTP Port         :%d\r\n", pUaDialog->iRemoteRTPPort);

    if (sock > 0)
    {
        send(sock, rbuf, strlen(rbuf), 0);
    }

    /* Local IP */
    snprintf(rbuf, 128, "\rLocal IP                :%s\r\n", pUaDialog->strLocalIP);

    if (sock > 0)
    {
        send(sock, rbuf, strlen(rbuf), 0);
    }

    /* Local Port */
    snprintf(rbuf, 128, "\rLocal Port              :%d\r\n", pUaDialog->iLocalPort);

    if (sock > 0)
    {
        send(sock, rbuf, strlen(rbuf), 0);
    }

    /* Local RTP IP */
    snprintf(rbuf, 128, "\rLocal RTP IP            :%s\r\n", pUaDialog->strLocalRTPIP);

    if (sock > 0)
    {
        send(sock, rbuf, strlen(rbuf), 0);
    }

    /* Local RTP Port */
    snprintf(rbuf, 128, "\rLocal RTP Port          :%d\r\n", pUaDialog->iLocalRTPPort);

    if (sock > 0)
    {
        send(sock, rbuf, strlen(rbuf), 0);
    }

    if (sock > 0)
    {
        send(sock, strLine, strlen(strLine), 0);
    }

    return;
}

/*****************************************************************************
 函 数 名  : SIP_ReleaseAllSIPUAInfo
 功能描述  : 释放所有SIPUA会话信息
 输入参数  :
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年7月30日 星期二
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void SIP_ReleaseAllSIPUAInfo()
{
    return;
}
#endif
