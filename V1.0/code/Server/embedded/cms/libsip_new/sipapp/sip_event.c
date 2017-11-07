/******************************************************************************

                  版权所有 (C), 2001-2013, 金智视讯技术有限公司

 ******************************************************************************
  文 件 名   : sip_event.c
  版 本 号   : 初稿
  作    者   : yanghaifeng
  生成日期   : 2013年4月1日
  最近修改   :
  功能描述   : SIP EVENT处理
  函数列表   :
              OnIctTransportError
              OnIstTransportError
              OnKillIctTransaction
              OnKillIstTransaction
              OnKillNictTransaction
              OnKillNistTransaction
              OnNictTransportError
              OnNistTransportError
              OnRcv1xxForInvite
              OnRcv1xxForRequest
              OnRcv2xxForBye
              OnRcv2xxForCancel
              OnRcv2xxForInfo
              OnRcv2xxForInvite
              OnRcv2xxForMessage
              OnRcv2xxForNotify
              OnRcv2xxForOptions
              OnRcv2xxForRefer
              OnRcv2xxForRegister
              OnRcv2xxForSubscribe
              OnRcv3xxForBye
              OnRcv3xxForCancel
              OnRcv3xxForInfo
              OnRcv3xxForInvite
              OnRcv3xxForNotify
              OnRcv3xxForOptions
              OnRcv3xxForRefer
              OnRcv3xxForRegister
              OnRcv3xxForSubscribe
              OnRcv456xxForBye
              OnRcv456xxForCancel
              OnRcv456xxForInfo
              OnRcv456xxForInvite
              OnRcv456xxForMessage
              OnRcv456xxForNotify
              OnRcv456xxForOptions
              OnRcv456xxForRefer
              OnRcv456xxForRegister
              OnRcv456xxForSubscribe
              OnRcvAck
              OnRcvBye
              OnRcvCancel
              OnRcvIct2xx2
              OnRcvInfo
              OnRcvInvite
              OnRcvIstAckfor2xx
              OnRcvMessage
              OnRcvNotify
              OnRcvOptions
              OnRcvRefer
              OnRcvRegister
              OnRcvSubscribe
              OnRcvUnknown
              OnTimeoutBye
              OnTimeoutCancel
              OnSend1xx
              OnSend2xx
              OnSend3456xx
              OnSendInvite
              OnSendRequest
  修改历史   :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 创建文件

******************************************************************************/

/*----------------------------------------------*
 * 包含头文件                                   *
 *----------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
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

#include <osipparser2/osip_const.h>
#include <osip2/internal.h>
#include <osip2/osip.h>
#include <osip2/osip_fifo.h>
#include <osipparser2/osip_list.h>
#include <osipparser2/osip_uri.h>
#include <osipparser2/osip_port.h>
#include <src/osip2/xixt.h>

#include "gbltype.h"
#include "sip_event.inc"

#include "csdbg.inc"
#include "sipua.inc"
#include "sipmsg.inc"
#include "callback.inc"
#include "registrar.inc"
#include "timerproc.inc"
#include "udp_tl.inc"
#include "garbage.inc"


/*----------------------------------------------*
 * 外部变量说明                                 *
 *----------------------------------------------*/
extern osip_t* g_recv_cell;           /* 接收消息的SIP协议栈,非Message消息专用 */

extern garbage_t * g_recv_garbage;               /* sip: recv garbage */
extern garbage_t * g_recv_register_garbage;      /* sip: recv register garbage */
extern garbage_t * g_recv_msg_garbage;           /* sip: recv msg garbage */
extern garbage_t * g_send_garbage;               /* sip: send garbage */
extern garbage_t * g_send_msg_garbage;           /* sip: send msg garbage */

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

/*----------------------------------------------*
 * 常量定义                                     *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 宏定义                                       *
 *----------------------------------------------*/
#define ACT_NOPAY       "1"
#define ACT_BUSY        "2"
#define ACT_NONEXISTENT "3"
#define ACT_OFFLINE     "4"
#define ACT_DECLINE     "5"
#define MIN_SUB_EXPIRE  3600

#if DECS("消息处理钩子函数")
/*****************************************************************************
 函 数 名  : OnRcvUnknown
 功能描述  : 收到未知类型的消息处理函数
 输入参数  : osip_transaction_t* tr, osip_message_t* sip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int OnRcvUnknown(osip_transaction_t* tr, osip_message_t* sip)
{
    int i = 0;

    if (NULL == tr || NULL == sip)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcvUnknown() exit---: Message NULL \r\n");
        return -1;
    }

    //SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcvUnknown() Enter--- \r\n");

    i = uas_check8_2(tr, sip);

    if (0 != i)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcvUnknown() exit---: uas_check8_2 Error \r\n");
        return -1;
    }

    sip_response_default(-1, tr, 501, (char*)"Unknow Message");

    return 0;
}

/*****************************************************************************
 函 数 名  : OnRcvInvite
 功能描述  : 收到INVITE消息处理函数
 输入参数  : osip_transaction_t* tr, osip_message_t* sip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int OnRcvInvite(osip_transaction_t* tr, osip_message_t* sip)
{
    int i = 0;
    int pos = 0;
    osip_proxy_authorization_t* proxy_authorization = NULL;
    osip_uri_t* url = NULL;
    int index = 0;
    ua_dialog_t* pUaDialog = NULL;
    sip_dialog_t* dialog = NULL;
    sdp_message_t* remote_sdp = NULL;
    unsigned long remote_rtp_addr = 0;
    int remote_rtp_port = 0;
    char ipstr[16] = {0};
    char* call_id = NULL;
    char* caller_id = NULL;
    char* callee_id = NULL;
    osip_header_t* support_header = NULL;
    osip_header_t* session_expires_header = NULL;
    int iSessionExpires = 0;
    char* strSDP = NULL;
    char* pcLocalIP = NULL;
    char* pcRemoteIP = NULL;

    if (NULL == tr || NULL == sip)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcvInvite() exit---: Message NULL \r\n");
        return -1;
    }

    url = sip->from->url;

    if (NULL == url || NULL == url->username || NULL == url->host)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcvInvite() exit---: URL Error \r\n");
        return -1;
    }

    /* response 100 trying first */
    sip_response_default(-1, tr, 100, NULL);

    /* 语法检查 */
    i = uas_check8_2(tr, sip);

    if (0 != i)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcvInvite() exit---: uas_check8_2 Error \r\n");
        return -1;
    }

    /* 检查主被叫是否一样 */
    if (!url_match_simply(url, sip->req_uri))
    {
        sip_response_default(-1, tr, 403, (char*)"Caller and Callee The Same");
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcvInvite() exit---: Caller and Callee The Same \r\n");
        return -1;
    }

    /* 获取认证信息*/
    pos = 0;

    while (!osip_list_eol(&sip->proxy_authorizations, pos))
    {
        proxy_authorization = (osip_proxy_authorization_t*)osip_list_get(&sip->proxy_authorizations, pos);

        if (proxy_authorization != NULL && proxy_authorization->realm != NULL)
        {
            break;
        }

        pos++;
        proxy_authorization = NULL;
    }

    /* 检查是否支持会话刷新*/
    pos = 0;

    while (!osip_list_eol(&sip->headers, pos))
    {
        osip_message_get_supported(sip, pos, &support_header);

        if (NULL != support_header && NULL != support_header->hvalue)
        {
            if (0 == sstrcmp(support_header->hvalue, (char*)"timer"))
            {
                msg_get_session_expires(sip, 0, &session_expires_header);

                if (NULL != session_expires_header && NULL != session_expires_header->hvalue)
                {
                    iSessionExpires = osip_atoi(session_expires_header->hvalue);
                    //SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcvInvite() iSessionExpires=%d \r\n", iSessionExpires);
                    break;
                }
            }
        }

        pos++;
    }

    /* 查找会话资源*/
    index = find_dialog_as_uas(sip);
    //SIP_DEBUG_TRACE(LOG_INFO, "OnRcvInvite() find_dialog_as_uas:index=%d \r\n", index);

    if (is_valid_dialog_index(index))
    {
        pUaDialog = ua_dialog_get(index);

        if (NULL == pUaDialog)
        {
            sip_response_default(-1, tr, 503, (char*)"Get UA Dialog Error");
            SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcvInvite() exit---: Get UA Dialog Error:dialog_index=%d \r\n", index);
            return -1;
        }

        pUaDialog->pServerTr = tr;
        pUaDialog->pServerSIP = sip;

        if (NULL != proxy_authorization)
        {
            osip_authorization_clone((osip_authorization_t*)proxy_authorization, &pUaDialog->pAuthorization);
        }

        pUaDialog->iSessionExpires = iSessionExpires;
        pUaDialog->iSessionExpiresCount = iSessionExpires;
        pUaDialog->iUpdateSendCount = iSessionExpires / 2;

        //有匹配的dialog资源，调用钩子函数
        if (NULL != g_AppCallback && NULL != g_AppCallback->invite_received_cb)
        {
            if ((NULL != pUaDialog) && (NULL != pUaDialog->pSipDialog) && (NULL != pUaDialog->pSipDialog->call_id))
            {
                call_id = osip_getcopy(pUaDialog->pSipDialog->call_id);
            }
            else
            {
                call_id = osip_getcopy(sip->call_id->number);
            }

            if (NULL != pUaDialog->pRemoteSDP)
            {
                sdp_message_to_str(pUaDialog->pRemoteSDP, &strSDP);
            }

            if ((NULL != sip) && (NULL != sip->from) && (NULL != sip->from->url) && (NULL != sip->from->url->username))
            {
                caller_id = osip_getcopy(sip->from->url->username);
            }

            if ((NULL != sip) && (NULL != sip->to) && (NULL != sip->to->url) && (NULL != sip->to->url->username))
            {
                if (NULL != sip->req_uri && NULL != sip->req_uri->username)
                {
                    if (0 != sstrcmp(sip->req_uri->username, sip->to->url->username))
                    {
                        callee_id = osip_getcopy(sip->req_uri->username);
                    }
                    else
                    {
                        callee_id = osip_getcopy(sip->to->url->username);
                    }
                }
                else
                {
                    callee_id = osip_getcopy(sip->to->url->username);
                }
            }
            else if (NULL != sip->req_uri && NULL != sip->req_uri->username)
            {
                callee_id = osip_getcopy(sip->req_uri->username);
            }

            if (NULL != strSDP)
            {
                g_AppCallback->invite_received_cb(caller_id, callee_id, call_id, index, strSDP, strlen(strSDP), g_AppCallback->invite_received_cb_user_data);
            }
            else
            {
                g_AppCallback->invite_received_cb(caller_id, callee_id, call_id, index, NULL, 0, g_AppCallback->invite_received_cb_user_data);
            }

            if (NULL != caller_id)
            {
                osip_free(caller_id);
                caller_id = NULL;
            }

            if (NULL != callee_id)
            {
                osip_free(callee_id);
                callee_id = NULL;
            }

            if (NULL != strSDP)
            {
                osip_free(strSDP);
                strSDP = NULL;
            }

            if (NULL != call_id)
            {
                osip_free(call_id);
                call_id = NULL;
            }
        }

        return 0;
    }
    else
    {
        /* check is to header has tag paramter*/
        {
            osip_uri_param_t* to_tag = NULL;

            if (NULL != sip->to)
            {
                osip_to_get_tag(sip->to, &to_tag);

                if (NULL != to_tag)
                {
                    sip_response_default(-1, tr, 481, NULL);
                    SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcvInvite() exit---: Get To Tag Error \r\n");
                    return -1;
                }
            }
        }

        /* 获取一个空闲的dialog资源 */
        index = ua_dialog_add();
        //SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcvInvite() ua_dialog_add:index=%d \r\n", index);

        if (!is_valid_dialog_index(index))
        {
            sip_response_default(-1, tr, 486, NULL);
            SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcvInvite() exit---: UA Dialog Add Error \r\n");
            return -1;
        }

        pUaDialog = ua_dialog_get(index);

        if (NULL == pUaDialog)
        {
            sip_response_default(-1, tr, 503, (char*)"Get UA Dialog Error");
            SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcvInvite() exit---: Get UA Dialog Error:dialog_index=%d \r\n", index);
            return -1;
        }

        /* 初始化sip dialog */
        i = sip_dialog_init_as_uas(&dialog, tr, sip);

        if (i != 0)
        {
            ua_dialog_remove(index);
            SIP_DEBUG_TRACE(LOG_ERROR, "OnRcvInvite:ua_dialog_remove():index=%d \r\n", index);
            sip_response_default(-1, tr, 503, (char*)"SIP Dialog Init Error");
            SIP_DEBUG_TRACE(LOG_ERROR, "OnRcvInvite() exit---: SIP Dialog Init Error \r\n");
            return -1;
        }

        /* 设置ua dialog的参数 */
        pUaDialog->pServerTr = tr;
        pUaDialog->pServerSIP = sip;

        if (NULL != proxy_authorization)
        {
            osip_authorization_clone((osip_authorization_t*)proxy_authorization, &pUaDialog->pAuthorization);
        }

        pUaDialog->pSipDialog = dialog;
        pUaDialog->eUiState = UI_STATE_CALL_RCV;

        /* 设置主被叫ID */
        if ((NULL != sip) && (NULL != sip->from) && (NULL != sip->from->url) && (NULL != sip->from->url->username))
        {
            osip_strncpy(pUaDialog->strCallerID, sip->from->url->username, 128);
        }

        if ((NULL != sip) && (NULL != sip->to) && (NULL != sip->to->url) && (NULL != sip->to->url->username))
        {
            if (NULL != sip->req_uri && NULL != sip->req_uri->username)
            {
                if (0 != sstrcmp(sip->req_uri->username, sip->to->url->username))
                {
                    osip_strncpy(pUaDialog->strCalleeID, sip->req_uri->username, 128);
                }
                else
                {
                    osip_strncpy(pUaDialog->strCalleeID, sip->to->url->username, 128);
                }
            }
            else
            {
                osip_strncpy(pUaDialog->strCalleeID, sip->to->url->username, 128);
            }
        }
        else if (NULL != sip->req_uri && NULL != sip->req_uri->username)
        {
            osip_strncpy(pUaDialog->strCalleeID, sip->req_uri->username, 128);
        }

        /* 设置对端的ip地址和端口号 */
        pcRemoteIP = get_message_from_host(sip);

        if (NULL != pcRemoteIP)
        {
            osip_strncpy(pUaDialog->strRemoteIP, pcRemoteIP, 16);
        }

        pUaDialog->iRemotePort = get_message_from_port(sip);

        /* 设置本地ip地址和端口号 */
        pcLocalIP = get_message_to_host(sip);

        if (NULL != pcLocalIP)
        {
            osip_strncpy(pUaDialog->strLocalIP, pcLocalIP, 16);
        }

        pUaDialog->iLocalPort = get_message_to_port(sip);

        pUaDialog->iSessionExpires = iSessionExpires;
        pUaDialog->iSessionExpiresCount = iSessionExpires;
        pUaDialog->iUpdateSendCount = iSessionExpires / 2;

        /* 远端的媒体ip地址，端口，sdp信息 */
        i = get_message_sdp(sip, &remote_sdp);

        if (i == 0)
        {
            pUaDialog->pRemoteSDP = remote_sdp; /*add new sdp */
            get_sdp_ip_and_port(remote_sdp, &remote_rtp_addr, &remote_rtp_port);

            ipaddr2str(ipstr, remote_rtp_addr);
            osip_strncpy(pUaDialog->strRemoteRTPIP, ipstr, 16);
            pUaDialog->iRemoteRTPPort = remote_rtp_port;
        }
        else
        {
            ua_dialog_remove(index);
            SIP_DEBUG_TRACE(LOG_ERROR, "OnRcvInvite:ua_dialog_remove():index=%d \r\n", index);
            sip_response_default(-1, tr, 503, (char*)"Get Message SDP Error");
            SIP_DEBUG_TRACE(LOG_ERROR, "OnRcvInvite() exit---: Get Message SDP Error \r\n");
            return -1;
        }

        //调用钩子函数
        if (NULL != g_AppCallback && NULL != g_AppCallback->invite_received_cb)
        {
            if ((NULL != pUaDialog) && (NULL != pUaDialog->pSipDialog) && (NULL != pUaDialog->pSipDialog->call_id))
            {
                call_id = osip_getcopy(pUaDialog->pSipDialog->call_id);
            }
            else
            {
                call_id = osip_getcopy(sip->call_id->number);
            }

            if (NULL != pUaDialog->pRemoteSDP)
            {
                sdp_message_to_str(pUaDialog->pRemoteSDP, &strSDP);
            }

            if ((NULL != sip) && (NULL != sip->from) && (NULL != sip->from->url) && (NULL != sip->from->url->username))
            {
                caller_id = osip_getcopy(sip->from->url->username);
            }

            if ((NULL != sip) && (NULL != sip->to) && (NULL != sip->to->url) && (NULL != sip->to->url->username))
            {
                if (NULL != sip->req_uri && NULL != sip->req_uri->username)
                {
                    if (0 != sstrcmp(sip->req_uri->username, sip->to->url->username))
                    {
                        callee_id = osip_getcopy(sip->req_uri->username);
                    }
                    else
                    {
                        callee_id = osip_getcopy(sip->to->url->username);
                    }
                }
                else
                {
                    callee_id = osip_getcopy(sip->to->url->username);
                }
            }
            else if (NULL != sip->req_uri && NULL != sip->req_uri->username)
            {
                callee_id = osip_getcopy(sip->req_uri->username);
            }

            if (NULL != strSDP)
            {
                g_AppCallback->invite_received_cb(caller_id, callee_id, call_id, index, strSDP, strlen(strSDP), g_AppCallback->invite_received_cb_user_data);
            }
            else
            {
                g_AppCallback->invite_received_cb(caller_id, callee_id, call_id, index, NULL, 0, g_AppCallback->invite_received_cb_user_data);
            }

            if (NULL != caller_id)
            {
                osip_free(caller_id);
                caller_id = NULL;
            }

            if (NULL != callee_id)
            {
                osip_free(callee_id);
                callee_id = NULL;
            }

            if (NULL != strSDP)
            {
                osip_free(strSDP);
                strSDP = NULL;
            }

            if (NULL != call_id)
            {
                osip_free(call_id);
                call_id = NULL;
            }
        }

        return 0;
    }

    //ua_timer_use(UA_INVITE_TIMEOUT, index, NULL, NULL); /*add INVITE timeout timer*/

    return 0;
}

/*****************************************************************************
 函 数 名  : OnRcvAck
 功能描述  : 收到ACK消息的处理函数
 输入参数  : osip_transaction_t* tr, osip_message_t* sip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int OnRcvAck(osip_transaction_t* tr, osip_message_t* sip)
{
    int i = 0;
    int index = 0;
    char* call_id = NULL;
    char* caller_id = NULL;
    char* callee_id = NULL;
    ua_dialog_t* pUaDialog = NULL;

    if (NULL == tr || NULL == sip)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcvAck() exit---: Message NULL \r\n");
        return -1;
    }

    //SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcvAck() Enter--- \r\n");

    index = find_dialog_as_uas(sip);
    SIP_DEBUG_TRACE(LOG_INFO, "OnRcvAck() find_dialog_as_uas:index=%d \r\n", index);

    if (is_valid_dialog_index(index))
    {
        pUaDialog = ua_dialog_get(index);

        if (NULL == pUaDialog)
        {
            //SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcvAck() exit---: UA Dialog NULL \r\n");
            return -1;
        }

        i = ua_timer_remove(UA_ACK2XX_RETRANSMIT, index, NULL, sip);

        if (i == 0) /*stop retransmit 2xx */
        {
            update_dialog_as_uas(index, NULL, sip, DLG_EVENT_UPDATE);

            if (pUaDialog->eUiState == UI_STATE_CALL_ACCEPT)
            {
                pUaDialog->eUiState = UI_STATE_CONNECTED;
            }
        }

        if (NULL != g_AppCallback && NULL != g_AppCallback->ack_received_cb)
        {
            if ((NULL != pUaDialog) && (NULL != pUaDialog->pSipDialog) && (NULL != pUaDialog->pSipDialog->call_id))
            {
                call_id = osip_getcopy(pUaDialog->pSipDialog->call_id);
            }
            else
            {
                call_id = osip_getcopy(sip->call_id->number);
            }

            if ((NULL != sip) && (NULL != sip->from) && (NULL != sip->from->url) && (NULL != sip->from->url->username))
            {
                caller_id = osip_getcopy(sip->from->url->username);
            }

            if ((NULL != sip) && (NULL != sip->to) && (NULL != sip->to->url) && (NULL != sip->to->url->username))
            {
                if (NULL != sip->req_uri && NULL != sip->req_uri->username)
                {
                    if (0 != sstrcmp(sip->req_uri->username, sip->to->url->username))
                    {
                        callee_id = osip_getcopy(sip->req_uri->username);
                    }
                    else
                    {
                        callee_id = osip_getcopy(sip->to->url->username);
                    }
                }
                else
                {
                    callee_id = osip_getcopy(sip->to->url->username);
                }
            }
            else if (NULL != sip->req_uri && NULL != sip->req_uri->username)
            {
                callee_id = osip_getcopy(sip->req_uri->username);
            }

            g_AppCallback->ack_received_cb(caller_id, callee_id, call_id, index, g_AppCallback->ack_received_cb_user_data);

            if (NULL != caller_id)
            {
                osip_free(caller_id);
                caller_id = NULL;
            }

            if (NULL != callee_id)
            {
                osip_free(callee_id);
                callee_id = NULL;
            }

            if (NULL != call_id)
            {
                osip_free(call_id);
                call_id = NULL;
            }
        }

        return 0;
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : OnRcvBye
 功能描述  : 收到BYE消息处理函数
 输入参数  : osip_transaction_t* tr, osip_message_t* sip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int OnRcvBye(osip_transaction_t* tr, osip_message_t* sip)
{
    int i = 0, index = 0;
    ua_dialog_t* pUaDialog = NULL;
    char* call_id = NULL;
    char* caller_id = NULL;
    char* callee_id = NULL;

    if (NULL == tr || NULL == sip)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcvBye() exit---: Message NULL \r\n");
        return -1;
    }

    //SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcvBye() Enter--- \r\n");

    /* rfc3261 8.2*/
    i = uas_check8_2(tr, sip);

    if (0 != i)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcvBye() exit---: uas_check8_2 Error \r\n");
        return -1;
    }

    index = find_dialog_as_uas(sip);
    SIP_DEBUG_TRACE(LOG_INFO, "OnRcvBye() find_dialog_as_uas:index=%d \r\n", index);

    if (is_valid_dialog_index(index))
    {
        pUaDialog = ua_dialog_get(index);

        if (NULL == pUaDialog)
        {
            sip_response_default(-1, tr, 503, (char*)"Get UA Dialog Error");
            SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcvBye() exit---: Get UA Dialog Error:dialog_index=%d \r\n", index);
            return -1;
        }

        //pUaDialog->pServerTr = event->tr;
        //pUaDialog->pServerSIP = event->msg;

        if (NULL != g_AppCallback && NULL != g_AppCallback->bye_received_cb)
        {
            if ((NULL != pUaDialog) && (NULL != pUaDialog->pSipDialog) && (NULL != pUaDialog->pSipDialog->call_id))
            {
                call_id = osip_getcopy(pUaDialog->pSipDialog->call_id);
            }
            else
            {
                call_id = osip_getcopy(sip->call_id->number);
            }

            if ((NULL != sip) && (NULL != sip->from) && (NULL != sip->from->url) && (NULL != sip->from->url->username))
            {
                caller_id = osip_getcopy(sip->from->url->username);
            }

            if ((NULL != sip) && (NULL != sip->to) && (NULL != sip->to->url) && (NULL != sip->to->url->username))
            {
                if (NULL != sip->req_uri && NULL != sip->req_uri->username)
                {
                    if (0 != sstrcmp(sip->req_uri->username, sip->to->url->username))
                    {
                        callee_id = osip_getcopy(sip->req_uri->username);
                    }
                    else
                    {
                        callee_id = osip_getcopy(sip->to->url->username);
                    }
                }
                else
                {
                    callee_id = osip_getcopy(sip->to->url->username);
                }
            }
            else if (NULL != sip->req_uri && NULL != sip->req_uri->username)
            {
                callee_id = osip_getcopy(sip->req_uri->username);
            }

            //调用钩子函数
            g_AppCallback->bye_received_cb(caller_id, callee_id, call_id, index, g_AppCallback->bye_received_cb_user_data);

            if (NULL != caller_id)
            {
                osip_free(caller_id);
                caller_id = NULL;
            }

            if (NULL != callee_id)
            {
                osip_free(callee_id);
                callee_id = NULL;
            }

            if (NULL != call_id)
            {
                osip_free(call_id);
                call_id = NULL;
            }
        }

        /* dialog*/
        update_dialog_as_uas(index, tr, sip, DLG_EVENT_REMOTEBYE);
        sip_response_default(index, tr, 200, NULL);
        pUaDialog->eUiState = UI_STATE_BYE_RECEIVE;
        cs_timer_remove(UA_SESSION_EXPIRE, index, NULL);
        cs_timer_remove(UAC_SEND_UPDATE, index, NULL);
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcvBye() cs_timer_remove:UAC_SEND_UPDATE:pos=%d \r\n", index);
    }
    else
    {
        /*error*/
        SIP_DEBUG_TRACE(LOG_ERROR, "OnRcvBye() exit---: Find UA Dialog Error \r\n");
        sip_response_default(-1, tr, 481, NULL);
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : OnRcvRegister
 功能描述  : 收到REGISTER消息处理函数
 输入参数  : osip_transaction_t* tr, osip_message_t* sip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int OnRcvRegister(osip_transaction_t* tr, osip_message_t* sip)
{
    int i = 0;
    osip_uri_t* reg_uri = NULL;

    if (NULL == tr || NULL == sip)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcvRegister() exit---: Message NULL \r\n");
        return -1;
    }

    reg_uri = osip_message_get_uri(sip);

    if (NULL == reg_uri || NULL == reg_uri->host)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcvRegister() exit---: Get Message URL Error \r\n");
        return -1;
    }

    i = register_msg_proc(tr, sip);

    return 0;
}

/*****************************************************************************
 函 数 名  : OnRcvOptions
 功能描述  : 收到OPTIONS消息处理函数
 输入参数  : osip_transaction_t* tr, osip_message_t* sip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int OnRcvOptions(osip_transaction_t* tr, osip_message_t* sip)
{
    int i = 0, index = 0;

    if (NULL == tr || NULL == sip)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcvOptions() exit---: Message NULL \r\n");
        return -1;
    }

    //SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcvOptions() Enter--- \r\n");

    i = uas_check8_2(tr, sip);

    if (0 != i)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcvOptions() exit---: uas_check8_2 Error \r\n");
        return -1;
    }

    index = find_dialog_as_uas(sip);
    //SIP_DEBUG_TRACE(LOG_INFO, "OnRcvOptions() find_dialog_as_uas:index=%d \r\n", index);

    if (is_valid_dialog_index(index))
    {
        /*request within dialog*/
        update_dialog_as_uas(index, tr, sip, DLG_EVENT_UPDATE);
        //sip_answer_to_options(index, tr, 200);
    }
    else
    {
        //sip_answer_to_options(-1, tr, 200);
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : OnRcvInfo
 功能描述  : 收到INFO消息处理函数
 输入参数  : osip_transaction_t* tr, osip_message_t* sip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int OnRcvInfo(osip_transaction_t* tr, osip_message_t* sip)
{
    int i = 0, index = 0;
    osip_body_t* body = NULL;
    char* caller_host = NULL;
    int caller_port = 0;
    char* call_id = NULL;
    char* caller_id = NULL;
    char* callee_id = NULL;

    if (NULL == tr || NULL == sip)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcvInfo() exit---: Message NULL \r\n");
        return -1;
    }

    //SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcvInfo() Enter--- \r\n");

    i = uas_check8_2(tr, sip);

    if (0 != i)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcvInfo() exit---: uas_check8_2 Error \r\n");
        return -1;
    }

    i = osip_message_get_body(sip, 0, &body);

    if (i == -1)
    {
        sip_response_default(-1, tr, 405, NULL);
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcvInfo() exit---: Message Get Body Error \r\n");
        return -1;
    }

    if (NULL == body || NULL == body->body || body->length <= 0)
    {
        sip_response_default(-1, tr, 405, NULL);
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcvInfo() exit---: Message Body Error \r\n");
        return -1;
    }

    index = find_dialog_as_uas(sip);
    //SIP_DEBUG_TRACE(LOG_INFO, "OnRcvInfo() find_dialog_as_uas:index=%d \r\n", index);

    if (is_valid_dialog_index(index))
    {
        update_dialog_as_uas(index, tr, sip, DLG_EVENT_UPDATE);
    }

    if (NULL != g_AppCallback && NULL != g_AppCallback->info_received_cb)
    {
        caller_host = osip_getcopy(get_message_from_host(sip));
        caller_port = get_message_from_port(sip);

        if ((NULL != sip) && (NULL != sip->call_id) && (NULL != sip->call_id->number))
        {
            call_id = osip_getcopy(sip->call_id->number);
        }

        if ((NULL != sip) && (NULL != sip->from) && (NULL != sip->from->url) && (NULL != sip->from->url->username))
        {
            caller_id = osip_getcopy(sip->from->url->username);
        }

        if ((NULL != sip) && (NULL != sip->to) && (NULL != sip->to->url) && (NULL != sip->to->url->username))
        {
            if (NULL != sip->req_uri && NULL != sip->req_uri->username)
            {
                if (0 != sstrcmp(sip->req_uri->username, sip->to->url->username))
                {
                    callee_id = osip_getcopy(sip->req_uri->username);
                }
                else
                {
                    callee_id = osip_getcopy(sip->to->url->username);
                }
            }
            else
            {
                callee_id = osip_getcopy(sip->to->url->username);
            }
        }
        else if (NULL != sip->req_uri && NULL != sip->req_uri->username)
        {
            callee_id = osip_getcopy(sip->req_uri->username);
        }

        /* 调用钩子函数 */
        g_AppCallback->info_received_cb(caller_id, caller_host, caller_port, callee_id, call_id, index, body->body, body->length, g_AppCallback->info_received_cb_user_data);

        if (NULL != caller_id)
        {
            osip_free(caller_id);
            caller_id = NULL;
        }

        if (NULL != callee_id)
        {
            osip_free(callee_id);
            callee_id = NULL;
        }

        if (NULL != call_id)
        {
            osip_free(call_id);
            call_id = NULL;
        }

        if (NULL != caller_host)
        {
            osip_free(caller_host);
            caller_host = NULL;
        }
    }

    sip_response_default(-1, tr, 200, NULL);

    return 0;
}

/*****************************************************************************
 函 数 名  : OnRcvUpdate
 功能描述  : 收到UPDATE消息处理函数
 输入参数  : osip_transaction_t* tr, osip_message_t* sip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年9月2日 星期一
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int OnRcvUpdate(osip_transaction_t* tr, osip_message_t* sip)
{
    int pos = 0;
    int i = 0, index = 0;
    ua_dialog_t* pUaDialog = NULL;
    osip_header_t* support_header = NULL;
    osip_header_t* session_expires_header = NULL;
    int iSessionExpires = 0;
    char* pExpires = NULL, *tmp = NULL;

    if (NULL == tr || NULL == sip)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcvUpdate() exit---: Message NULL \r\n");
        return -1;
    }

    //SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcvUpdate() Enter--- \r\n");

    i = uas_check8_2(tr, sip);

    if (0 != i)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcvUpdate() exit---: uas_check8_2 Error \r\n");
        return -1;
    }

    /* 获取会话刷新时间 */
    pos = 0;

    while (!osip_list_eol(&sip->headers, pos))
    {
        osip_message_get_supported(sip, pos, &support_header);

        if (NULL != support_header && NULL != support_header->hvalue)
        {
            if (0 == sstrcmp(support_header->hvalue, (char*)"timer"))
            {
                msg_get_session_expires(sip, 0, &session_expires_header);

                if (NULL != session_expires_header && NULL != session_expires_header->hvalue)
                {
                    tmp = strchr(session_expires_header->hvalue, ';'); /*find '=' */

                    if (tmp != NULL)
                    {
                        if (tmp - session_expires_header->hvalue > 0)
                        {
                            pExpires = (char*) osip_malloc(tmp - session_expires_header->hvalue + 1);

                            if (NULL != pExpires)
                            {
                                osip_strncpy(pExpires, session_expires_header->hvalue, tmp - session_expires_header->hvalue);
                                iSessionExpires = osip_atoi(pExpires);
                                osip_free(pExpires);
                                pExpires = NULL;
                            }
                        }
                        else
                        {
                            //printf("\r\n OnRcvUpdate:tmp - session_expires_header->hvalue=%d \r\n", tmp - session_expires_header->hvalue);
                        }
                    }
                    else
                    {
                        iSessionExpires = osip_atoi(session_expires_header->hvalue);
                    }

                    //SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcvUpdate() iSessionExpires=%d \r\n", iSessionExpires);
                    break;
                }
            }
        }

        pos++;
    }

    index = find_dialog_as_uas(sip);
    //SIP_DEBUG_TRACE(LOG_INFO, "OnRcvUpdate() find_dialog_as_uas:index=%d \r\n", index);

    if (is_valid_dialog_index(index))
    {
        pUaDialog = ua_dialog_get(index);

        if (NULL != pUaDialog)
        {
            pUaDialog->iSessionExpires = iSessionExpires;
            pUaDialog->iSessionExpiresCount = iSessionExpires;
            pUaDialog->iUpdateSendCount = iSessionExpires / 2;
            sip_answer_to_update(index, tr, 200, iSessionExpires);
            SIP_DEBUG_TRACE(LOG_INFO, "OnRcvUpdate() OK: index=%d, iSessionExpires=%d \r\n", index, iSessionExpires);
        }
        else
        {
            sip_answer_to_update(-1, tr, 404, iSessionExpires);
            SIP_DEBUG_TRACE(LOG_ERROR, "OnRcvUpdate() Error: index=%d, iSessionExpires=%d \r\n", index, iSessionExpires);
        }
    }
    else
    {
        sip_answer_to_update(-1, tr, 404, iSessionExpires);
        SIP_DEBUG_TRACE(LOG_ERROR, "OnRcvUpdate() Error: index=%d, iSessionExpires=%d \r\n", index, iSessionExpires);
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : OnRcvCancel
 功能描述  : 收到CANCEL消息处理函数
 输入参数  : osip_transaction_t* tr, osip_message_t* sip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int OnRcvCancel(osip_transaction_t* tr, osip_message_t* sip)
{
    int index = 0;
    sip_dialog_t* pSipDlg = NULL;
    char* call_id = NULL;
    char* caller_id = NULL;
    char* callee_id = NULL;
    ua_dialog_t* pUaDialog = NULL;

    if (NULL == tr || NULL == sip)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcvCancel() exit---: Message NULL \r\n");
        return -1;
    }

    //SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcvCancel() Enter--- \r\n");

    index = find_dialog_as_uas(sip);
    SIP_DEBUG_TRACE(LOG_INFO, "OnRcvCancel() find_dialog_as_uas:index=%d \r\n", index);

    if (is_valid_dialog_index(index))
    {
        pUaDialog = ua_dialog_get(index);

        if (NULL == pUaDialog)
        {
            SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcvCancel() exit---: Get UA Dialog Error:dialog_index=%d \r\n", index);
            return -1;
        }

        /* 通知上层 */
        pSipDlg = pUaDialog->pSipDialog;

        if (NULL != pSipDlg)
        {
            if (NULL != g_AppCallback && NULL != g_AppCallback->cancel_received_cb)
            {
                if (NULL != pSipDlg->call_id)
                {
                    call_id = osip_getcopy(pSipDlg->call_id);
                }
                else
                {
                    call_id = osip_getcopy(sip->call_id->number);
                }

                if ((NULL != sip) && (NULL != sip->from) && (NULL != sip->from->url) && (NULL != sip->from->url->username))
                {
                    caller_id = osip_getcopy(sip->from->url->username);
                }

                if ((NULL != sip) && (NULL != sip->to) && (NULL != sip->to->url) && (NULL != sip->to->url->username))
                {
                    if (NULL != sip->req_uri && NULL != sip->req_uri->username)
                    {
                        if (0 != sstrcmp(sip->req_uri->username, sip->to->url->username))
                        {
                            callee_id = osip_getcopy(sip->req_uri->username);
                        }
                        else
                        {
                            callee_id = osip_getcopy(sip->to->url->username);
                        }
                    }
                    else
                    {
                        callee_id = osip_getcopy(sip->to->url->username);
                    }
                }
                else if (NULL != sip->req_uri && NULL != sip->req_uri->username)
                {
                    callee_id = osip_getcopy(sip->req_uri->username);
                }

                g_AppCallback->cancel_received_cb(caller_id, callee_id, call_id, index, g_AppCallback->cancel_received_cb_user_data);

                if (NULL != caller_id)
                {
                    osip_free(caller_id);
                    caller_id = NULL;
                }

                if (NULL != callee_id)
                {
                    osip_free(callee_id);
                    callee_id = NULL;
                }

                if (NULL != call_id)
                {
                    osip_free(call_id);
                    call_id = NULL;
                }
            }
        }

        update_dialog_as_uas(index,  tr, sip, DLG_EVENT_CANCELLED);
#ifdef WIN32
        ;  //windows不处理
#else
        sip_response_default(index, tr, 200, NULL);
#endif
        sip_3456xxxx_answer_to_invite(index, 487, NULL);
        pUaDialog->eUiState = UI_STATE_IDLE;
        //ua_dialog_remove(index);
    }

    return 0;
}

#if 0
/*****************************************************************************
 函 数 名  : OnRcvSubscribe
 功能描述  : 收到SUBSCRIBE消息处理函数
 输入参数  : osip_transaction_t* tr, osip_message_t* sip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int OnRcvSubscribe(osip_transaction_t* tr, osip_message_t* sip)
{
    int i = 0;
    int user_pos = -1;
    osip_body_t* body = NULL;
    char* caller_id = NULL;
    char* caller_host = NULL;
    int caller_port = 0;
    char* call_id = NULL;
    char* callee_id = NULL;
    char* callee_host = NULL;
    int callee_port = 0;

    char* event_type = NULL, *id_param = NULL;
    osip_generic_param_t* gen_param = NULL;
    event_t* event = NULL;
    osip_header_t* expires_header = NULL;
    int subscribe_expires = 0;

    if (NULL == tr || NULL == sip)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcvSubscribe() exit---: Message NULL \r\n");
        return -1;
    }

    i = uas_check8_2(tr, sip);

    if (0 != i)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcvSubscribe() exit---: uas_check8_2 Error \r\n");
        return -1;
    }

    /*Event header*/
    i = msg_getevent_if1(sip, &event);

    if (i != 1)
    {
        //sip_response_default(-1, tr, 400);
        //SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcvNotify() exit---: msg_getevent_if1 Error \r\n");
        //return -1;
    }

    if (event != NULL)
    {
        event_type = osip_getcopy(event->event_type);

        i = osip_generic_param_get_byname(event->gen_params, (char*)"id", &gen_param);

        if (i == 0)
        {
            id_param = osip_getcopy(gen_param->gvalue);
        }
    }

    /* 获取 expires 头 */
    msg_get_expires(sip, 0, &expires_header);

    if (expires_header != NULL)
    {
        subscribe_expires = osip_atoi(expires_header->hvalue);
    }

    i =  osip_message_get_body(sip, 0, &body);

    if (i == -1)
    {
        sip_response_default(-1, tr, 405, NULL);
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcvSubscribe() exit---: Get Message Body Error \r\n");
        return -1;
    }

    if (NULL == body || NULL == body->body || body->length <= 0)
    {
        sip_response_default(-1, tr, 405, NULL);
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcvSubscribe() exit---: Message Body Error \r\n");
        return -1;
    }

    //index = find_dialog_as_uas(sip);
    //SIP_DEBUG_TRACE(LOG_INFO, "OnRcvMessage() find_dialog_as_uas:index=%d \r\n", index);

    caller_host = osip_getcopy(get_message_from_host(sip));
    caller_port = get_message_from_port(sip);
    callee_host = osip_getcopy(get_message_to_host(sip));
    callee_port = get_message_to_port(sip);

#ifndef WIN32 //modified by chenyu 131024

    if (NULL != sip->from && NULL != sip->from->url && NULL != sip->to && NULL != sip->to->url)
    {
        if ((0 == sstrcmp(sip->from->url->username, (char*)"wiscomCallerID"))
            && (0 == sstrcmp(sip->to->url->username, (char*)"wiscomCalleeID"))) /* 查询服务器ID和用户ID */
        {
            sip_response_default(-1, tr, 200, NULL);
            //printf("\r\n ********** OnRcvMessage() sip_response_default 200:From Host=%s \r\n", sip->from->url->host);
            //printf_system_time1();
        }
        else
        {
            user_pos = uas_reginfo_find_by_caller_host_and_port(caller_host, caller_port);

            if (user_pos >= 0)
            {
                sip_response_default(-1, tr, 200, NULL);
            }
            else
            {
                user_pos = uac_reginfo_find_by_server_host_and_port(caller_host, caller_port);

                if (user_pos >= 0)
                {
                    sip_response_default(-1, tr, 200, NULL);
                }
                else
                {
                    sip_response_default(-1, tr, 403, NULL);
                    SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcvSubscribe() exit---: Found Matching User Error \r\n");
                    goto normal;
                }
            }
        }
    }
    else
    {
        sip_response_default(-1, tr, 403, NULL);
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcvSubscribe() exit---: Message Error \r\n");
        goto normal;
    }

#else
    sip_response_default(-1, tr, 200, NULL);
#endif

    /* 调用钩子函数 */
    if (NULL != g_AppCallback && NULL != g_AppCallback->subscribe_received_cb)
    {
        if ((NULL != sip) && (NULL != sip->call_id) && (NULL != sip->call_id->number))
        {
            call_id = osip_getcopy(sip->call_id->number);
        }

        if ((NULL != sip) && (NULL != sip->from) && (NULL != sip->from->url) && (NULL != sip->from->url->username))
        {
            caller_id = osip_getcopy(sip->from->url->username);
        }

        if ((NULL != sip) && (NULL != sip->to) && (NULL != sip->to->url) && (NULL != sip->to->url->username))
        {
            if (NULL != sip->req_uri && NULL != sip->req_uri->username)
            {
                if (0 != sstrcmp(sip->req_uri->username, sip->to->url->username))
                {
                    callee_id = osip_getcopy(sip->req_uri->username);
                }
                else
                {
                    callee_id = osip_getcopy(sip->to->url->username);
                }
            }
            else
            {
                callee_id = osip_getcopy(sip->to->url->username);
            }
        }
        else if (NULL != sip->req_uri && NULL != sip->req_uri->username)
        {
            callee_id = osip_getcopy(sip->req_uri->username);
        }

        g_AppCallback->subscribe_received_cb(caller_id, caller_host, caller_port, callee_id, callee_host, callee_port, call_id, event_type, id_param, subscribe_expires, body->body, body->length, g_AppCallback->subscribe_received_cb_user_data);

        if (NULL != caller_id)
        {
            osip_free(caller_id);
            caller_id = NULL;
        }

        if (NULL != callee_id)
        {
            osip_free(callee_id);
            callee_id = NULL;
        }

        if (NULL != call_id)
        {
            osip_free(call_id);
            call_id = NULL;
        }
    }

normal:

    if (NULL != caller_host)
    {
        osip_free(caller_host);
        caller_host = NULL;
    }

    if (NULL != callee_host)
    {
        osip_free(callee_host);
        callee_host = NULL;
    }

    if (NULL != event_type)
    {
        osip_free(event_type);
        event_type = NULL;
    }

    if (NULL != id_param)
    {
        osip_free(id_param);
        id_param = NULL;
    }

    if (event != NULL)
    {
        event_free(event);
        osip_free(event);
        event = NULL;
    }

    return 0;
}
#endif

int OnRcvSubscribe(osip_transaction_t* tr, osip_message_t* sip)
{
    int pos = 0;
    int i = 0, index = 0, expires = 0;
    char *event_type = NULL, *id_param = NULL;
    event_t *event = NULL;
    osip_generic_param_t *gen_param = NULL;
    sip_subscription_t *sip_sub = NULL;
    ua_dialog_t* pUaDialog = NULL;
    sip_dialog_t* dialog = NULL;
    osip_contact_t *contact = NULL;

    osip_body_t* body = NULL;
    char* pcLocalIP = NULL;
    char* pcRemoteIP = NULL;
    char* call_id = NULL;
    char ss[100] = {0};

    i = uas_check8_2(tr, sip);

    if (0 != i)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "OnRcvSubscribe() exit---: uas_check8_2 Error \r\n");
        return -1;
    }

    /*only one contact header */
    pos = 0;

    while (osip_message_get_contact(sip, pos, &contact) != -1)
    {
        pos++;
    }

    if (pos != 1)
    {
        sip_response_default(-1, tr, 400, NULL);
        SIP_DEBUG_TRACE(LOG_ERROR, "OnRcvSubscribe() exit---: uas_check8_2 Error \r\n");
        return -1;
    }

    /*Event header*/
    i = msg_getevent_if1(sip, &event);

    if (i != 1)
    {
        sip_response_default(-1, tr, 400, NULL);
        SIP_DEBUG_TRACE(LOG_ERROR, "OnRcvSubscribe() exit---: msg_getevent_if1 Error \r\n");
        goto error;
    }

    if (event != NULL)
    {
        event_type = osip_getcopy(event->event_type);

        /* check event type */
        if (0 != sstrcmp(event_type, "presence")
            && 0 != sstrcmp(event_type, "Catalog"))
        {
            sip_response_default(-1, tr, 489, NULL);
            SIP_DEBUG_TRACE(LOG_ERROR, "OnRcvSubscribe() exit---: event_type Error \r\n");
            goto error;
        }

        id_param = NULL;
        i = osip_generic_param_get_byname(event->gen_params, (char*)"id", &gen_param);

        if (i == 0)
        {
            id_param = osip_getcopy(gen_param->gvalue);
        }
    }

    /* get Expires header */
    osip_message_get_expires(sip, 0, &gen_param);

    if (gen_param && gen_param->gvalue)
    {
        expires = osip_atoi(gen_param->gvalue);
    }
    else
    {
        expires = MIN_SUB_EXPIRE;
    }

    /* get body */
    i =  osip_message_get_body(sip, 0, &body);

    if (i == -1)
    {
        sip_response_default(-1, tr, 405, NULL);
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcvSubscribe() exit---: Get Message Body Error \r\n");
        return -1;
    }

    if (NULL == body || NULL == body->body || body->length <= 0)
    {
        sip_response_default(-1, tr, 405, NULL);
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcvSubscribe() exit---: Message Body Error \r\n");
        return -1;
    }

    /* 查找会话信息 */
    index = find_dialog_as_uas(sip);
    SIP_DEBUG_TRACE(LOG_TRACE, "OnRcvSubscribe() find_dialog_as_uas:index=%d \r\n", index);

    if (is_valid_dialog_index(index))
    {
        pUaDialog = ua_dialog_get(index);

        if (NULL == pUaDialog)
        {
            ua_dialog_remove(index);
            SIP_DEBUG_TRACE(LOG_ERROR, "OnRcvSubscribe:ua_dialog_remove():index=%d \r\n", index);
            sip_response_default(-1, tr, 400, NULL);
            SIP_DEBUG_TRACE(LOG_ERROR, "OnRcvSubscribe() exit---: Get UA Dialog Error:dialog_index=%d \r\n", index);
            goto error;
        }

        update_dialog_as_uas(index, tr, sip, DLG_EVENT_UPDATE);

        sip_sub = SearchSubscription(index, event_type, id_param);

        if (sip_sub)
        {
            /* refresh */
            SipAnswerToSubscribe(index, sip_sub, tr, 200);

            /* check unsubscribe or refresh */
            if (expires == 0) /* close */
            {
                /* sub_state */
                snprintf(ss, 100, "terminated;reason=timeout");

                if (NULL != sip_sub->sub_state)
                {
                    osip_free(sip_sub->sub_state);
                    sip_sub->sub_state = NULL;
                }

                sip_sub->sub_state = osip_getcopy(ss);

                sip_sub->begin = time(NULL);
                sip_sub->duration = 0;
            }
            else /* refresh */
            {
                /* sub_state */
                snprintf(ss, 100, "active;expires=%i", expires);

                if (NULL != sip_sub->sub_state)
                {
                    osip_free(sip_sub->sub_state);
                    sip_sub->sub_state = NULL;
                }

                sip_sub->sub_state = osip_getcopy(ss);

                sip_sub->begin = time(NULL);
                sip_sub->duration = expires;
            }

            /* 调用钩子函数 */
            if (NULL != g_AppCallback && NULL != g_AppCallback->subscribe_within_dialog_received_cb)
            {
                if ((NULL != sip) && (NULL != sip->call_id) && (NULL != sip->call_id->number))
                {
                    call_id = osip_getcopy(sip->call_id->number);
                }

                g_AppCallback->subscribe_within_dialog_received_cb(pUaDialog->strCallerID, pUaDialog->strRemoteIP, pUaDialog->iRemotePort, pUaDialog->strCalleeID, call_id, index, expires, body->body, body->length);

                if (NULL != call_id)
                {
                    osip_free(call_id);
                    call_id = NULL;
                }
            }

            goto ret;
        }
    }
    else
    {
        /* create dialog */
        index = ua_dialog_add();

        if (!is_valid_dialog_index(index))
        {
            sip_response_default(-1, tr, 400, NULL);
            SIP_DEBUG_TRACE(LOG_ERROR, "OnRcvSubscribe() exit---: UA Dialog Add Error \r\n");
            goto error;
        }

        pUaDialog = ua_dialog_get(index);

        if (NULL == pUaDialog)
        {
            ua_dialog_remove(index);
            SIP_DEBUG_TRACE(LOG_ERROR, "OnRcvSubscribe:ua_dialog_remove():index=%d \r\n", index);
            sip_response_default(-1, tr, 400, NULL);
            SIP_DEBUG_TRACE(LOG_ERROR, "OnRcvSubscribe() exit---: Get UA Dialog Error:dialog_index=%d \r\n", index);
            goto error;
        }

        i = sip_dialog_init_as_uas(&dialog, tr, sip);

        if (i != 0)
        {
            ua_dialog_remove(index);
            SIP_DEBUG_TRACE(LOG_ERROR, "OnRcvSubscribe:ua_dialog_remove():index=%d \r\n", index);
            sip_response_default(-1, tr, 400, NULL);
            SIP_DEBUG_TRACE(LOG_ERROR, "OnRcvSubscribe() exit---: sip_dialog_init_as_uas Error:dialog_index=%d \r\n", index);
            goto error;
        }

        pUaDialog->pSipDialog = dialog;

        /* 设置主被叫ID */
        if ((NULL != sip) && (NULL != sip->from) && (NULL != sip->from->url) && (NULL != sip->from->url->username))
        {
            osip_strncpy(pUaDialog->strCallerID, sip->from->url->username, 128);
        }

        if ((NULL != sip) && (NULL != sip->to) && (NULL != sip->to->url) && (NULL != sip->to->url->username))
        {
            if (NULL != sip->req_uri && NULL != sip->req_uri->username)
            {
                if (0 != sstrcmp(sip->req_uri->username, sip->to->url->username))
                {
                    osip_strncpy(pUaDialog->strCalleeID, sip->req_uri->username, 128);
                }
                else
                {
                    osip_strncpy(pUaDialog->strCalleeID, sip->to->url->username, 128);
                }
            }
            else
            {
                osip_strncpy(pUaDialog->strCalleeID, sip->to->url->username, 128);
            }
        }
        else if (NULL != sip->req_uri && NULL != sip->req_uri->username)
        {
            osip_strncpy(pUaDialog->strCalleeID, sip->req_uri->username, 128);
        }

        /* 设置对端的ip地址和端口号 */
        pcRemoteIP = get_message_from_host(sip);

        if (NULL != pcRemoteIP)
        {
            osip_strncpy(pUaDialog->strRemoteIP, pcRemoteIP, 16);
        }

        pUaDialog->iRemotePort = get_message_from_port(sip);

        /* 设置本地ip地址和端口号 */
        pcLocalIP = get_message_to_host(sip);

        if (NULL != pcLocalIP)
        {
            osip_strncpy(pUaDialog->strLocalIP, pcLocalIP, 16);
        }

        pUaDialog->iLocalPort = get_message_to_port(sip);
    }

    if (expires <= 0) /* close */
    {
        ua_dialog_remove(index);
        SIP_DEBUG_TRACE(LOG_ERROR, "OnRcvSubscribe:ua_dialog_remove():index=%d \r\n", index);
        sip_response_default(-1, tr, 400, NULL);
        SIP_DEBUG_TRACE(LOG_ERROR, "OnRcvSubscribe() exit---: expires Error:dialog_index=%d, expires=%d \r\n", index, expires);
        goto error;
    }

    /* create new subscription */
    i = sip_subscription_init(&sip_sub);
    sip_sub->state = SUB_STATE_PRE;
    sip_sub->event_type = osip_getcopy(event_type);
    sip_sub->id_param = osip_getcopy(id_param);
    sip_sub->begin = time(NULL);
    sip_sub->duration = expires;

    /* sub_state */
    snprintf(ss, 100, "active;expires=%i", expires);
    sip_sub->sub_state = osip_getcopy(ss);

    if (NULL != contact && NULL != contact->url)
    {
        osip_uri_clone(contact->url, &sip_sub->remote_contact_uri);
    }

    i = AddDialogSubscription(index, sip_sub);

    if (i == 0)
    {
        UpdateSubscription(sip_sub, SUB_STATE_CONFIRMED);
        SipAnswerToSubscribe(index, sip_sub, tr, 200);

        /* 调用钩子函数 */
        if (NULL != g_AppCallback && NULL != g_AppCallback->subscribe_within_dialog_received_cb)
        {
            if ((NULL != sip) && (NULL != sip->call_id) && (NULL != sip->call_id->number))
            {
                call_id = osip_getcopy(sip->call_id->number);
            }

            g_AppCallback->subscribe_within_dialog_received_cb(pUaDialog->strCallerID, pUaDialog->strRemoteIP, pUaDialog->iRemotePort, pUaDialog->strCalleeID, call_id, index, expires, body->body, body->length);

            if (NULL != call_id)
            {
                osip_free(call_id);
                call_id = NULL;
            }
        }
    }
    else
    {
        ua_dialog_remove(index);
        SIP_DEBUG_TRACE(LOG_ERROR, "OnRcvSubscribe:ua_dialog_remove():index=%d \r\n", index);
        sip_subscription_free(sip_sub);
        osip_free(sip_sub);
        sip_response_default(-1, tr, 400, NULL);
        SIP_DEBUG_TRACE(LOG_ERROR, "OnRcvSubscribe() exit---: AddDialogSubscription Error:dialog_index=%d \r\n", index);
    }

ret:
error:

    if (event_type != NULL)
    {
        osip_free(event_type);
        event_type = NULL;
    }

    if (id_param != NULL)
    {
        osip_free(id_param);
        id_param = NULL;
    }

    if (event != NULL)
    {
        event_free(event);
        osip_free(event);
        event = NULL;
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : OnRcvNotify
 功能描述  : 收到NOTIFY消息处理函数
 输入参数  : osip_transaction_t* tr, osip_message_t* sip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int OnRcvNotify(osip_transaction_t* tr, osip_message_t* sip)
{
    int i = 0;
    //int index = -1;
    int user_pos = -1;
    osip_body_t* body = NULL;
    char* caller_id = NULL;
    char* caller_host = NULL;
    int caller_port = 0;
    char* call_id = NULL;
    char* callee_id = NULL;
    char* callee_host = NULL;
    int callee_port = 0;

    char* event_type = NULL, *id_param = NULL, *subscribe_state = NULL;
    osip_generic_param_t* gen_param = NULL;
    event_t* event = NULL;
    osip_header_t* state_header = NULL;

    if (NULL == tr || NULL == sip)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcvNotify() exit---: Message NULL \r\n");
        return -1;
    }

    i = uas_check8_2(tr, sip);

    if (0 != i)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcvNotify() exit---: uas_check8_2 Error \r\n");
        return -1;
    }

    /*Event header*/
    i = msg_getevent_if1(sip, &event);

    if (i != 1)
    {
        //sip_response_default(-1, tr, 400);
        //SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcvNotify() exit---: msg_getevent_if1 Error \r\n");
        //return -1;
    }

    if (event != NULL)
    {
        event_type = osip_getcopy(event->event_type);

        i = osip_generic_param_get_byname(event->gen_params, (char*)"id", &gen_param);

        if (i == 0)
        {
            id_param = osip_getcopy(gen_param->gvalue);
        }
    }

    /* 获取Subscription-State头 */
    msg_getsubscription_state(sip, 0, &state_header);

    if (state_header != NULL)
    {
        subscribe_state = osip_getcopy(state_header->hvalue);
    }

    i =  osip_message_get_body(sip, 0, &body);

    if (i == -1)
    {
        sip_response_default(-1, tr, 405, NULL);
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcvNotify() exit---: Get Message Body Error \r\n");
        return -1;
    }

    if (NULL == body || NULL == body->body || body->length <= 0)
    {
        sip_response_default(-1, tr, 405, NULL);
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcvNotify() exit---: Message Body Error \r\n");
        return -1;
    }

    //index = find_dialog_as_uas(sip);
    //SIP_DEBUG_TRACE(LOG_INFO, "OnRcvMessage() find_dialog_as_uas:index=%d \r\n", index);

    caller_host = osip_getcopy(get_message_from_host(sip));
    caller_port = get_message_from_port(sip);
    callee_host = osip_getcopy(get_message_to_host(sip));
    callee_port = get_message_to_port(sip);

#ifndef WIN32 //modified by chenyu 131024

    if (NULL != sip->from && NULL != sip->from->url && NULL != sip->to && NULL != sip->to->url)
    {
        if ((0 == sstrcmp(sip->from->url->username, (char*)"wiscomCallerID"))
            && (0 == sstrcmp(sip->to->url->username, (char*)"wiscomCalleeID"))) /* 查询服务器ID和用户ID */
        {
            sip_response_default(-1, tr, 200, NULL);
            //printf("\r\n ********** OnRcvMessage() sip_response_default 200:From Host=%s \r\n", sip->from->url->host);
            //printf_system_time1();
        }
        else
        {
            user_pos = uas_reginfo_find_by_caller_host_and_port(caller_host, caller_port);

            if (user_pos >= 0)
            {
                sip_response_default(-1, tr, 200, NULL);
            }
            else
            {
                user_pos = uac_reginfo_find_by_server_host_and_port(caller_host, caller_port);

                if (user_pos >= 0)
                {
                    sip_response_default(-1, tr, 200, NULL);
                }
                else
                {
                    sip_response_default(-1, tr, 403, NULL);
                    SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcvNotify() exit---: Found Matching User Error \r\n");
                    goto normal;
                }
            }
        }
    }
    else
    {
        sip_response_default(-1, tr, 403, NULL);
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcvNotify() exit---: Message Error \r\n");
        goto normal;
    }

#else
    sip_response_default(-1, tr, 200, NULL);
#endif

    /* 调用钩子函数 */
    if (NULL != g_AppCallback && NULL != g_AppCallback->notify_received_cb)
    {
        if ((NULL != sip) && (NULL != sip->call_id) && (NULL != sip->call_id->number))
        {
            call_id = osip_getcopy(sip->call_id->number);
        }

        if ((NULL != sip) && (NULL != sip->from) && (NULL != sip->from->url) && (NULL != sip->from->url->username))
        {
            caller_id = osip_getcopy(sip->from->url->username);
        }

        if ((NULL != sip) && (NULL != sip->to) && (NULL != sip->to->url) && (NULL != sip->to->url->username))
        {
            if (NULL != sip->req_uri && NULL != sip->req_uri->username)
            {
                if (0 != sstrcmp(sip->req_uri->username, sip->to->url->username))
                {
                    callee_id = osip_getcopy(sip->req_uri->username);
                }
                else
                {
                    callee_id = osip_getcopy(sip->to->url->username);
                }
            }
            else
            {
                callee_id = osip_getcopy(sip->to->url->username);
            }
        }
        else if (NULL != sip->req_uri && NULL != sip->req_uri->username)
        {
            callee_id = osip_getcopy(sip->req_uri->username);
        }

        g_AppCallback->notify_received_cb(caller_id, caller_host, caller_port, callee_id, callee_host, callee_port, call_id, body->body, body->length, g_AppCallback->notify_received_cb_user_data);

        if (NULL != caller_id)
        {
            osip_free(caller_id);
            caller_id = NULL;
        }

        if (NULL != callee_id)
        {
            osip_free(callee_id);
            callee_id = NULL;
        }

        if (NULL != call_id)
        {
            osip_free(call_id);
            call_id = NULL;
        }
    }

normal:

    if (NULL != caller_host)
    {
        osip_free(caller_host);
        caller_host = NULL;
    }

    if (NULL != callee_host)
    {
        osip_free(callee_host);
        callee_host = NULL;
    }

    if (NULL != event_type)
    {
        osip_free(event_type);
        event_type = NULL;
    }

    if (NULL != id_param)
    {
        osip_free(id_param);
        id_param = NULL;
    }

    if (event != NULL)
    {
        event_free(event);
        osip_free(event);
        event = NULL;
    }

    if (NULL != subscribe_state)
    {
        osip_free(subscribe_state);
        subscribe_state = NULL;
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : OnRcvRefer
 功能描述  : 收到REFER消息处理函数
 输入参数  : osip_transaction_t* tr, osip_message_t* sip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int OnRcvRefer(osip_transaction_t* tr, osip_message_t* sip)
{
#if 0
    int i, index, pos;
    char* tmp;
    contact_t* contact;
    char* event_type = NULL, *id_param = NULL;
    refer_to_t* refer_to = NULL;
    referred_by_t* referred_by = NULL;
    generic_param_t* gen_param;
    sip_subscription_t* sip_sub;
    event_t* event = NULL;

    SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcvRefer() entry--- \r\n");

    if (NULL == tr || NULL == sip)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcvRefer() exit---: Message NULL \r\n");
        return -1;
    }

    /* rfc3261 8.2*/
    i = uas_check8_2(tr, sip);
    SIP_DEBUG_TRACE(LOG_INFO, "OnRcvRefer() uas_check8_2:i=%d \r\n", i);

    if (0 != i)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcvRefer() exit---: ERROR 3 \r\n");
        return -1;
    }

    /*only one contact header */
    pos = 0;

    while (msg_getcontact(sip, pos, &contact) != -1)
    {
        pos++;
    }

    if (pos != 1)
    {
        sip_response_default(-1, tr, 400);
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcvRefer() exit---: ERROR 4 \r\n");
        return -1;
    }

    /*only one Refer-To header */
    i = msg_getrefer_to_if1(sip, &refer_to);

    if (i != 1)
    {
        sip_response_default(-1, tr, 400);
        goto error;
    }

    /* get Referred-By header*/
    i = msg_getreferred_by_if1(sip, &referred_by);

    if (i > 1)
    {
        sip_response_default(-1, tr, 400);
        goto error;
    }

    /*Event header*/
    i = msg_getevent_if1(sip, &event);

//  if (i != 1)
    if (i > 1)
    {
        sip_response_default(-1, tr, 400);
        goto error;
    }

    if (event != NULL)
    {
        event_type = sgetcopy(event->event_type);

        if (strncmp(event_type, "refer", 6) != 0)
        {
            goto error;
        }

        id_param = NULL;
        i = generic_param_getbyname(event->gen_params, (char*)"id", &gen_param);

        if (i == 0)
        {
            id_param = sgetcopy(gen_param->gvalue);
        }
    }
    else
    {
        event_type = sgetcopy("refer");
        id_param = NULL;
    }

    index = find_dialog_as_uas(sip);

    if (IsValidSessIndex(index))
    {

        if (g_dialogs[index].m_ui_state == UI_STATE_CONFERENCE)
        {
            sip_response_default(-1, tr, 486);
            goto error;
        }

        update_dialog_as_uas(index, tr, sip, DLG_EVENT_UPDATE);

        sip_sub = find_subscription(index, sip->to->url, REFER_NOTIFIER, event_type, id_param);

        if (sip_sub)
        {
            sip_answer_to_refer(index, tr, 202);
        }
        else
        {
            sip_answer_to_refer(index, tr, 202);
            sip_bye(index);
            /* invite refer_to */
            url_2char(refer_to->url, &tmp);
            process_rcv_dialout_from_ivr(g_dialogs[index].dialog->local_uri->url->username, tmp, 0 , NULL);
            sfree(tmp);

            if (i >= 0)
            {
                g_dialogs[index].m_ui_state = UI_STATE_CALL_SENT;
            }
        }

    }
    else
    {
        /*error*/
        sip_response_default(-1, tr, 481);
    }

error:
    sfree(event_type);
    sfree(id_param);

    if (event != NULL)
    {
        event_free(event);
        sfree(event);
    }

    if (refer_to != NULL)
    {
        refer_to_free(refer_to);
        sfree(refer_to);
    }

    if (referred_by != NULL)
    {
        referred_by_free(referred_by);
        sfree(referred_by);
    }

#endif
    return 0;
}

/*****************************************************************************
 函 数 名  : OnRcvMessage
 功能描述  : 收到MESSAGE消息处理函数
 输入参数  : osip_transaction_t* tr, osip_message_t* sip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int OnRcvMessage(osip_transaction_t* tr, osip_message_t* sip)
{
    int i = 0;
    int index = -1;
    int user_pos = -1;
    osip_body_t* body = NULL;
    char* caller_id = NULL;
    char* caller_host = NULL;
    int caller_port = 0;
    char* call_id = NULL;
    char* callee_id = NULL;
    char* callee_host = NULL;
    int callee_port = 0;

    if (NULL == tr || NULL == sip)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcvMessage() exit---: Message NULL \r\n");
        return -1;
    }

    i = uas_check8_2(tr, sip);

    if (0 != i)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcvMessage() exit---: uas_check8_2 Error \r\n");
        return -1;
    }

    i =  osip_message_get_body(sip, 0, &body);

    if (i == -1)
    {
        sip_response_default(-1, tr, 405, NULL);
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcvMessage() exit---: Get Message Body Error \r\n");
        return -1;
    }

    if (NULL == body || NULL == body->body || body->length <= 0)
    {
        sip_response_default(-1, tr, 405, NULL);
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcvMessage() exit---: Message Body Error \r\n");
        return -1;
    }

    caller_host = osip_getcopy(get_message_from_host(sip));
    caller_port = get_message_from_port(sip);
    callee_host = osip_getcopy(get_message_to_host(sip));
    callee_port = get_message_to_port(sip);

    index = find_dialog_as_uas(sip);
    SIP_DEBUG_TRACE(LOG_TRACE, "OnRcvMessage() find_dialog_as_uas:caller_host=%s, caller_port=%d, callee_host=%s, callee_port=%d, index=%d \r\n", caller_host, caller_port, callee_host, callee_port, index);

#ifndef WIN32 //modified by chenyu 131024

    if (NULL != sip->from && NULL != sip->from->url && NULL != sip->to && NULL != sip->to->url)
    {
        if ((0 == sstrcmp(sip->from->url->username, (char*)"wiscomCallerID"))
            && (0 == sstrcmp(sip->to->url->username, (char*)"wiscomCalleeID"))) /* 查询服务器ID和用户ID */
        {
            sip_response_default(-1, tr, 200, NULL);
            //printf("\r\n ********** OnRcvMessage() sip_response_default 200:From Host=%s \r\n", sip->from->url->host);
            //printf_system_time1();
        }
        else
        {
            user_pos = uas_reginfo_find_by_caller_host_and_port(caller_host, caller_port);

            if (user_pos >= 0)
            {
                sip_response_default(-1, tr, 200, NULL);
            }
            else
            {
                user_pos = uac_reginfo_find_by_server_host_and_port(caller_host, caller_port);

                if (user_pos >= 0)
                {
                    sip_response_default(-1, tr, 200, NULL);
                }
                else
                {
                    sip_response_default(-1, tr, 403, NULL);
                    SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcvMessage() exit---: Found Matching User Error \r\n");
                    goto normal;
                }
            }
        }
    }
    else
    {
        sip_response_default(-1, tr, 403, NULL);
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcvMessage() exit---: Message Error \r\n");
        goto normal;
    }

#else
    sip_response_default(-1, tr, 200, NULL);
#endif

    /* 调用钩子函数 */
    if (NULL != g_AppCallback && NULL != g_AppCallback->message_received_cb)
    {
        if ((NULL != sip) && (NULL != sip->call_id) && (NULL != sip->call_id->number))
        {
            call_id = osip_getcopy(sip->call_id->number);
        }

        if ((NULL != sip) && (NULL != sip->from) && (NULL != sip->from->url) && (NULL != sip->from->url->username))
        {
            caller_id = osip_getcopy(sip->from->url->username);
        }

        if ((NULL != sip) && (NULL != sip->to) && (NULL != sip->to->url) && (NULL != sip->to->url->username))
        {
            if (NULL != sip->req_uri && NULL != sip->req_uri->username)
            {
                if (0 != sstrcmp(sip->req_uri->username, sip->to->url->username))
                {
                    callee_id = osip_getcopy(sip->req_uri->username);
                }
                else
                {
                    callee_id = osip_getcopy(sip->to->url->username);
                }
            }
            else
            {
                callee_id = osip_getcopy(sip->to->url->username);
            }
        }
        else if (NULL != sip->req_uri && NULL != sip->req_uri->username)
        {
            callee_id = osip_getcopy(sip->req_uri->username);
        }

        g_AppCallback->message_received_cb(caller_id, caller_host, caller_port, callee_id, callee_host, callee_port, call_id, index, body->body, body->length, g_AppCallback->message_received_cb_user_data);

        if (NULL != caller_id)
        {
            osip_free(caller_id);
            caller_id = NULL;
        }

        if (NULL != callee_id)
        {
            osip_free(callee_id);
            callee_id = NULL;
        }

        if (NULL != call_id)
        {
            osip_free(call_id);
            call_id = NULL;
        }
    }

normal:

    if (NULL != caller_host)
    {
        osip_free(caller_host);
        caller_host = NULL;
    }

    if (NULL != callee_host)
    {
        osip_free(callee_host);
        callee_host = NULL;
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : OnRcv1xxForInvite
 功能描述  : 收到INVITE 1xx响应消息处理函数
 输入参数  : osip_transaction_t* tr, osip_message_t* sip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int OnRcv1xxForInvite(osip_transaction_t* tr, osip_message_t* sip)
{
    int index = -1;
    osip_uri_param_t* to_tag = NULL;
    int statuscode = 0;
    char* caller_id = NULL;
    char* callee_id = NULL;
    char* call_id = NULL;
    ua_dialog_t* pUaDialog = NULL;

    if (NULL == tr || NULL == sip)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv1xxForInvite() exit---: Message NULL \r\n");
        return -1;
    }

    //SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv1xxForInvite() Enter--- \r\n");

    if (MSG_TEST_CODE(sip, 100) || MSG_TEST_CODE(sip, 101))
    {
        return 0;
    }

    index = find_dialog_as_uac(sip);
    //SIP_DEBUG_TRACE(LOG_INFO, "OnRcv1xxForInvite() find_dialog_as_uac:index=%d \r\n", index);

    if (!is_valid_dialog_index(index))
    {
        //SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv1xxForInvite() exit---: Dialog Index Error \r\n");
        return -1;
    }

    pUaDialog = ua_dialog_get(index);

    if (NULL == pUaDialog)
    {
        //SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv1xxForInvite() exit---: UA Dialog NULL \r\n");
        return -1;
    }

    statuscode = sip->status_code;

    /* 调用钩子函数 */
    if ((!MSG_TEST_CODE(sip, 100)) && (!MSG_TEST_CODE(sip, 101)) && (!MSG_TEST_CODE(sip, 180)))
    {
        if (NULL != g_AppCallback && NULL != g_AppCallback->invite_response_received_cb)
        {
            if ((NULL != pUaDialog) && (NULL != pUaDialog->pSipDialog) && (NULL != pUaDialog->pSipDialog->call_id))
            {
                call_id = osip_getcopy(pUaDialog->pSipDialog->call_id);
            }
            else
            {
                call_id = osip_getcopy(sip->call_id->number);
            }

            if ((NULL != sip) && (NULL != sip->from) && (NULL != sip->from->url) && (NULL != sip->from->url->username))
            {
                caller_id = osip_getcopy(sip->from->url->username);
            }

            if ((NULL != sip) && (NULL != sip->to) && (NULL != sip->to->url) && (NULL != sip->to->url->username))
            {
                if (NULL != sip->req_uri && NULL != sip->req_uri->username)
                {
                    if (0 != sstrcmp(sip->req_uri->username, sip->to->url->username))
                    {
                        callee_id = osip_getcopy(sip->req_uri->username);
                    }
                    else
                    {
                        callee_id = osip_getcopy(sip->to->url->username);
                    }
                }
                else
                {
                    callee_id = osip_getcopy(sip->to->url->username);
                }
            }
            else if (NULL != sip->req_uri && NULL != sip->req_uri->username)
            {
                callee_id = osip_getcopy(sip->req_uri->username);
            }

            g_AppCallback->invite_response_received_cb(caller_id, callee_id, call_id, index, statuscode, sip->reason_phrase, NULL, 0, g_AppCallback->invite_response_received_cb_user_data);

            if (NULL != caller_id)
            {
                osip_free(caller_id);
                caller_id = NULL;
            }

            if (NULL != callee_id)
            {
                osip_free(callee_id);
                callee_id = NULL;
            }

            if (NULL != call_id)
            {
                osip_free(call_id);
                call_id = NULL;
            }
        }
    }

    if (MSG_TEST_CODE(sip, 100))
    {

    }
    else if (MSG_TEST_CODE(sip, 180) || MSG_TEST_CODE(sip, 181) || MSG_TEST_CODE(sip, 182))
    {

    }
    else if (MSG_TEST_CODE(sip, 183))
    {

    }

    if (NULL != sip->to)
    {
        osip_to_get_tag(sip->to, &to_tag);

        if (to_tag)
        {
            update_dialog_as_uac(index, tr, sip, DLG_EVENT_1XXTAG);
        }
        else
        {
            update_dialog_as_uac(index, tr, sip, DLG_EVENT_1XXNOTAG);
        }
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : OnRcv2xxForInvite
 功能描述  : 收到INVITE 2xx响应消息处理函数
 输入参数  : osip_transaction_t* tr, osip_message_t* sip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int OnRcv2xxForInvite(osip_transaction_t* tr, osip_message_t* sip)
{
    int i = -1;
    int index = 0;
    int statuscode = 0;
    ua_dialog_t* pUaDialog = NULL;
    sdp_message_t* remote_sdp = NULL;
    unsigned long remote_rtp_addr = 0;
    int remote_rtp_port = 0;
    char ipstr[16] = {0};
    ui_state_t eUiState = UI_STATE_IDLE;
    char* caller_id = NULL;
    char* callee_id = NULL;
    char* call_id = NULL;
    osip_header_t* support_header = NULL;
    osip_header_t* session_expires_header = NULL;
    int iSessionExpires = 0;
    char* pExpires = NULL, *tmp = NULL;
    char* strSDP = NULL;

    if (NULL == tr || NULL == sip)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv2xxForInvite() exit---: Message NULL \r\n");
        return -1;
    }

    //SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv2xxForInvite() Enter--- \r\n");

    index = find_dialog_as_uac(sip);
    //SIP_DEBUG_TRACE(LOG_INFO, "OnRcv2xxForInvite() find_dialog_as_uac:index=%d \r\n", index);

    if (!is_valid_dialog_index(index))
    {
        /* todo: remove CANCEL timeout timer */
        ua_timer_remove(UA_CANCEL_TIMEOUT, -1, tr, NULL);
        //SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv2xxForInvite() exit---: Dialog Index Error \r\n");
        return -1;
    }

    pUaDialog = ua_dialog_get(index);

    if (NULL == pUaDialog)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv2xxForInvite() exit---: Get UA Dialog Error:dialog_index=%d \r\n", index);
        return -1;
    }

    statuscode = sip->status_code;

    /* todo: remove INVITE timeout timer */
    ua_timer_remove(UA_INVITE_TIMEOUT, index, tr, NULL);

    update_dialog_as_uac(index, tr, sip, DLG_EVENT_2XX);

    if ((NULL != pUaDialog) && (NULL != pUaDialog->pSipDialog) && (NULL != pUaDialog->pSipDialog->call_id))
    {
        call_id = osip_getcopy(pUaDialog->pSipDialog->call_id);
    }
    else
    {
        call_id = osip_getcopy(sip->call_id->number);
    }

    /* 远端的媒体ip地址，端口，sdp信息 */
    i = get_message_sdp(sip, &remote_sdp);

    if (i == 0)
    {
        pUaDialog->pRemoteSDP = remote_sdp; /*add new sdp */
        get_sdp_ip_and_port(remote_sdp, &remote_rtp_addr, &remote_rtp_port);

        ipaddr2str(ipstr, remote_rtp_addr);
        osip_strncpy(pUaDialog->strRemoteRTPIP, ipstr, 16);
        pUaDialog->iRemoteRTPPort = remote_rtp_port;
    }
    else
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "OnRcv2xxForInvite() exit---: Get Message SDP Error \r\n");

        i = sip_ack(index, pUaDialog->strLocalIP, pUaDialog->iLocalPort, pUaDialog->iSessionExpires);
        i = sip_bye(index, pUaDialog->strLocalIP, pUaDialog->iLocalPort);
        pUaDialog->eUiState = UI_STATE_IDLE;

        //调用钩子函数
        if (NULL != g_AppCallback && NULL != g_AppCallback->invite_response_received_cb)
        {
            if ((NULL != sip) && (NULL != sip->from) && (NULL != sip->from->url) && (NULL != sip->from->url->username))
            {
                caller_id = osip_getcopy(sip->from->url->username);
            }

            if ((NULL != sip) && (NULL != sip->to) && (NULL != sip->to->url) && (NULL != sip->to->url->username))
            {
                if (NULL != sip->req_uri && NULL != sip->req_uri->username)
                {
                    if (0 != sstrcmp(sip->req_uri->username, sip->to->url->username))
                    {
                        callee_id = osip_getcopy(sip->req_uri->username);
                    }
                    else
                    {
                        callee_id = osip_getcopy(sip->to->url->username);
                    }
                }
                else
                {
                    callee_id = osip_getcopy(sip->to->url->username);
                }
            }
            else if (NULL != sip->req_uri && NULL != sip->req_uri->username)
            {
                callee_id = osip_getcopy(sip->req_uri->username);
            }

            g_AppCallback->invite_response_received_cb(caller_id, callee_id, call_id, index, 503, (char*)"Get Remote SDP Info Error", NULL, 0, g_AppCallback->invite_response_received_cb_user_data);

            if (NULL != caller_id)
            {
                osip_free(caller_id);
                caller_id = NULL;
            }

            if (NULL != callee_id)
            {
                osip_free(callee_id);
                callee_id = NULL;
            }
        }

        if (NULL != call_id)
        {
            osip_free(call_id);
            call_id = NULL;
        }

        return -1;
    }

    /* 获取会话时间参数*/
    int pos = 0;

    while (!osip_list_eol(&sip->headers, pos))
    {
        osip_message_get_supported(sip, pos, &support_header);

        if (NULL != support_header && NULL != support_header->hvalue)
        {
            if (0 == sstrcmp(support_header->hvalue, (char*)"timer"))
            {
                msg_get_session_expires(sip, 0, &session_expires_header);

                if (NULL != session_expires_header && NULL != session_expires_header->hvalue)
                {
                    tmp = strchr(session_expires_header->hvalue, ';'); /*find '=' */

                    if (tmp != NULL)
                    {
                        if (tmp - session_expires_header->hvalue > 0)
                        {
                            pExpires = (char*) osip_malloc(tmp - session_expires_header->hvalue + 1);

                            if (NULL != pExpires)
                            {
                                osip_strncpy(pExpires, session_expires_header->hvalue, tmp - session_expires_header->hvalue);
                                iSessionExpires = osip_atoi(pExpires);
                                osip_free(pExpires);
                                pExpires = NULL;
                            }
                        }
                        else
                        {
                            //printf("\r\n OnRcv2xxForInvite:tmp - session_expires_header->hvalue=%d \r\n", tmp - session_expires_header->hvalue);
                        }
                    }
                    else
                    {
                        iSessionExpires = osip_atoi(session_expires_header->hvalue);
                    }

                    //SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv2xxForInvite() iSessionExpires=%d \r\n", iSessionExpires);
                    break;
                }
            }
        }

        pos++;
    }

    pUaDialog->iSessionExpires = iSessionExpires;
    pUaDialog->iSessionExpiresCount = iSessionExpires;
    pUaDialog->iUpdateSendCount = iSessionExpires / 2;

    eUiState = pUaDialog->eUiState;

    if (eUiState == UI_STATE_CALL_SENT)
    {
        pUaDialog->eUiState = UI_STATE_200_RECEIVE; //UI_STATE_CONNECTED;
    }

    if (NULL != pUaDialog->pRemoteSDP)
    {
        sdp_message_to_str(pUaDialog->pRemoteSDP, &strSDP);
    }

#ifdef WIN32
    i = sip_ack(index, pUaDialog->strLocalIP, pUaDialog->iLocalPort, pUaDialog->iSessionExpires);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv2xxForInvite() SIP Ack Error:index=%d \r\n", index);

        //调用钩子函数
        if (NULL != g_AppCallback && NULL != g_AppCallback->bye_received_cb)
        {
            if ((NULL != sip) && (NULL != sip->from) && (NULL != sip->from->url) && (NULL != sip->from->url->username))
            {
                caller_id = osip_getcopy(sip->from->url->username);
            }

            if ((NULL != sip) && (NULL != sip->to) && (NULL != sip->to->url) && (NULL != sip->to->url->username))
            {
                if (NULL != sip->req_uri && NULL != sip->req_uri->username)
                {
                    if (0 != sstrcmp(sip->req_uri->username, sip->to->url->username))
                    {
                        callee_id = osip_getcopy(sip->req_uri->username);
                    }
                    else
                    {
                        callee_id = osip_getcopy(sip->to->url->username);
                    }
                }
                else
                {
                    callee_id = osip_getcopy(sip->to->url->username);
                }
            }
            else if (NULL != sip->req_uri && NULL != sip->req_uri->username)
            {
                callee_id = osip_getcopy(sip->req_uri->username);
            }

            g_AppCallback->bye_received_cb(caller_id, callee_id, call_id, index, g_AppCallback->bye_received_cb_user_data);

            if (NULL != caller_id)
            {
                osip_free(caller_id);
                caller_id = NULL;
            }

            if (NULL != callee_id)
            {
                osip_free(callee_id);
                callee_id = NULL;
            }
        }

        i = sip_bye(index, pUaDialog->strLocalIP, pUaDialog->iLocalPort);

    }
    else
    {
        /* 调用钩子函数 */
        if (NULL != g_AppCallback && NULL != g_AppCallback->invite_response_received_cb)
        {
            if ((NULL != sip) && (NULL != sip->from) && (NULL != sip->from->url) && (NULL != sip->from->url->username))
            {
                caller_id = osip_getcopy(sip->from->url->username);
            }

            if ((NULL != sip) && (NULL != sip->to) && (NULL != sip->to->url) && (NULL != sip->to->url->username))
            {
                if (NULL != sip->req_uri && NULL != sip->req_uri->username)
                {
                    if (0 != sstrcmp(sip->req_uri->username, sip->to->url->username))
                    {
                        callee_id = osip_getcopy(sip->req_uri->username);
                    }
                    else
                    {
                        callee_id = osip_getcopy(sip->to->url->username);
                    }
                }
                else
                {
                    callee_id = osip_getcopy(sip->to->url->username);
                }
            }
            else if (NULL != sip->req_uri && NULL != sip->req_uri->username)
            {
                callee_id = osip_getcopy(sip->req_uri->username);
            }

            if (NULL != strSDP)
            {
                g_AppCallback->invite_response_received_cb(caller_id, callee_id, call_id, index, statuscode, sip->reason_phrase, strSDP, strlen(strSDP), g_AppCallback->invite_response_received_cb_user_data);
            }
            else
            {
                g_AppCallback->invite_response_received_cb(caller_id, callee_id, call_id, index, statuscode, sip->reason_phrase, NULL, 0, g_AppCallback->invite_response_received_cb_user_data);
            }

            if (NULL != caller_id)
            {
                osip_free(caller_id);
                caller_id = NULL;
            }

            if (NULL != callee_id)
            {
                osip_free(callee_id);
                callee_id = NULL;
            }
        }
    }

#else

    /* 调用钩子函数 */
    if (NULL != g_AppCallback && NULL != g_AppCallback->invite_response_received_cb)
    {
        if ((NULL != sip) && (NULL != sip->from) && (NULL != sip->from->url) && (NULL != sip->from->url->username))
        {
            caller_id = osip_getcopy(sip->from->url->username);
        }

        if ((NULL != sip) && (NULL != sip->to) && (NULL != sip->to->url) && (NULL != sip->to->url->username))
        {
            if (NULL != sip->req_uri && NULL != sip->req_uri->username)
            {
                if (0 != sstrcmp(sip->req_uri->username, sip->to->url->username))
                {
                    callee_id = osip_getcopy(sip->req_uri->username);
                }
                else
                {
                    callee_id = osip_getcopy(sip->to->url->username);
                }
            }
            else
            {
                callee_id = osip_getcopy(sip->to->url->username);
            }
        }
        else if (NULL != sip->req_uri && NULL != sip->req_uri->username)
        {
            callee_id = osip_getcopy(sip->req_uri->username);
        }

        if (NULL != strSDP)
        {
            g_AppCallback->invite_response_received_cb(caller_id, callee_id, call_id, index, statuscode, sip->reason_phrase, strSDP, strlen(strSDP), g_AppCallback->invite_response_received_cb_user_data);
        }
        else
        {
            g_AppCallback->invite_response_received_cb(caller_id, callee_id, call_id, index, statuscode, sip->reason_phrase, NULL, 0, g_AppCallback->invite_response_received_cb_user_data);
        }

        if (NULL != caller_id)
        {
            osip_free(caller_id);
            caller_id = NULL;
        }

        if (NULL != callee_id)
        {
            osip_free(callee_id);
            callee_id = NULL;
        }
    }

#endif

    if (NULL != strSDP)
    {
        osip_free(strSDP);
        strSDP = NULL;
    }

    if (NULL != call_id)
    {
        osip_free(call_id);
        call_id = NULL;
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : OnRcv3xxForInvite
 功能描述  : 收到INVITE 3xx响应消息处理函数
 输入参数  : osip_transaction_t* tr, osip_message_t* sip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int OnRcv3xxForInvite(osip_transaction_t* tr, osip_message_t* sip)
{
    int index = -1;
    int statuscode = 0;
    ui_state_t eUiState = UI_STATE_IDLE;
    char* caller_id = NULL;
    char* callee_id = NULL;
    char* call_id = NULL;
    ua_dialog_t* pUaDialog = NULL;

    if (NULL == tr || NULL == sip)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv3xxForInvite() exit---: Message NULL \r\n");
        return -1;
    }

    //SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv3xxForInvite() Enter--- \r\n");

    index = find_dialog_as_uac(sip);
    //SIP_DEBUG_TRACE(LOG_INFO, "OnRcv3xxForInvite() find_dialog_as_uac:index=%d \r\n", index);

    if (!is_valid_dialog_index(index))
    {
        /* todo: remove CANCEL timeout timer */
        ua_timer_remove(UA_CANCEL_TIMEOUT, -1, tr, NULL);
        //SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv3xxForInvite() exit---: Dialog Index Error \r\n");
        return -1;
    }

    pUaDialog = ua_dialog_get(index);

    if (NULL == pUaDialog)
    {
        //SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv3xxForInvite() exit---: UA Dialog NULL \r\n");
        return -1;
    }

    statuscode = sip->status_code;

    /* todo: remove INVITE timeout timer */
    ua_timer_remove(UA_INVITE_TIMEOUT, index, tr, NULL);

    /* 调用钩子函数 */
    if (NULL != g_AppCallback && NULL != g_AppCallback->invite_response_received_cb)
    {
        if ((NULL != pUaDialog) && (NULL != pUaDialog->pSipDialog) && (NULL != pUaDialog->pSipDialog->call_id))
        {
            call_id = osip_getcopy(pUaDialog->pSipDialog->call_id);
        }
        else
        {
            call_id = osip_getcopy(sip->call_id->number);
        }

        if ((NULL != sip) && (NULL != sip->from) && (NULL != sip->from->url) && (NULL != sip->from->url->username))
        {
            caller_id = osip_getcopy(sip->from->url->username);
        }

        if ((NULL != sip) && (NULL != sip->to) && (NULL != sip->to->url) && (NULL != sip->to->url->username))
        {
            if (NULL != sip->req_uri && NULL != sip->req_uri->username)
            {
                if (0 != sstrcmp(sip->req_uri->username, sip->to->url->username))
                {
                    callee_id = osip_getcopy(sip->req_uri->username);
                }
                else
                {
                    callee_id = osip_getcopy(sip->to->url->username);
                }
            }
            else
            {
                callee_id = osip_getcopy(sip->to->url->username);
            }
        }
        else if (NULL != sip->req_uri && NULL != sip->req_uri->username)
        {
            callee_id = osip_getcopy(sip->req_uri->username);
        }

        g_AppCallback->invite_response_received_cb(caller_id, callee_id, call_id, index, statuscode, sip->reason_phrase, NULL, 0, g_AppCallback->invite_response_received_cb_user_data);

        if (NULL != caller_id)
        {
            osip_free(caller_id);
            caller_id = NULL;
        }

        if (NULL != callee_id)
        {
            osip_free(callee_id);
            callee_id = NULL;
        }

        if (NULL != call_id)
        {
            osip_free(call_id);
            call_id = NULL;
        }
    }

    eUiState = get_dialog_ui_state(index);

    if (eUiState == UI_STATE_CALL_SENT)
    {
        set_dialog_ui_state(index, UI_STATE_CALL_TERMINATED);
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : OnRcv456xxForInvite
 功能描述  : 收到INVITE 456xx响应消息处理函数
 输入参数  : osip_transaction_t* tr, osip_message_t* sip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int OnRcv456xxForInvite(osip_transaction_t* tr, osip_message_t* sip)
{
    int i = 0;
    int index = -1;
    int socket = 0;
    int statuscode = 0;
    ui_state_t eUiState = UI_STATE_IDLE;
    int iReInviteCnt = 0;
    ua_dialog_t* pUaDialog = NULL;
    char* caller_id = NULL;
    char* callee_id = NULL;
    char* call_id = NULL;
    osip_header_t* session_expires_header = NULL;
    int se_pos = -1;

    if (NULL == tr || NULL == sip)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv456xxForInvite() exit---: Message NULL \r\n");
        return -1;
    }

    //SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv456xxForInvite() Enter--- \r\n");

    index = find_dialog_as_uac(sip);
    //SIP_DEBUG_TRACE(LOG_INFO, "OnRcv456xxForInvite() find_dialog_as_uac:index=%d \r\n", index);

    if (!is_valid_dialog_index(index))
    {
        /* todo: remove CANCEL timeout timer */
        ua_timer_remove(UA_INVITE_TIMEOUT, -1, tr, NULL);
        //SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv456xxForInvite() exit---: Dialog Index Error \r\n");
        return -1;
    }

    pUaDialog = ua_dialog_get(index);

    if (NULL == pUaDialog)
    {
        ua_timer_remove(UA_INVITE_TIMEOUT, -1, tr, NULL);
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv456xxForInvite() exit---: Get UA Dialog Error:dialog_index=%d \r\n", index);
        return -1;
    }

    /* todo: remove INVITE timeout timer */
    ua_timer_remove(UA_INVITE_TIMEOUT, index, tr, NULL);

    iReInviteCnt = pUaDialog->iReInviteCnt;

    if (iReInviteCnt > 1)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv456xxForInvite() exit---: iReInviteCnt=%d Error \r\n", iReInviteCnt);
        goto release_sess;
    }

    if (sip->status_code == 401)
    {
        int cseq_num = 0;
        osip_transaction_t* tr2 = NULL;
        osip_message_t* request2 = NULL;
        osip_www_authenticate_t* www_authenticate = NULL;
        sip_dialog_t* dialog = NULL;

        if (&sip->www_authenticates != NULL && !osip_list_eol(&sip->www_authenticates, 0))
        {
            www_authenticate = (osip_www_authenticate_t*)osip_list_get(&sip->www_authenticates, 0);
        }

        if (www_authenticate == NULL)
        {
            SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv456xxForInvite() exit---: Get WWWW Authenticate Error \r\n");
            goto release_sess;
        }

        i = generating_request_fromrequest(tr->orig_request, &request2, pUaDialog->strLocalIP, pUaDialog->iLocalPort);

        if (i != 0)
        {
            SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv456xxForInvite() exit---: Generating Request From Request Error \r\n");
            goto release_sess;
        }

        /* remove all old authorizations and proxy authorizations*/
        {
            int pos = 0;
            osip_authorization_t* authorization = NULL;
            osip_proxy_authorization_t* proxy_authorization = NULL;
            pos = 0;

            while (!osip_list_eol(&request2->authorizations, pos))
            {
                authorization = (osip_authorization_t*)osip_list_get(&request2->authorizations, pos);

                if (NULL == authorization)
                {
                    pos++;
                    continue;
                }

                osip_list_remove(&request2->authorizations, pos);
                osip_authorization_free(authorization);
                authorization = NULL;
                pos++;
            }

            pos = 0;

            while (!osip_list_eol(&request2->proxy_authorizations, pos))
            {
                proxy_authorization = (osip_proxy_authorization_t*)osip_list_get(&request2->proxy_authorizations, pos);

                if (NULL == proxy_authorization)
                {
                    pos++;
                    continue;
                }

                osip_list_remove(&request2->proxy_authorizations, pos);
                osip_authorization_free(proxy_authorization);
                proxy_authorization = NULL;
                pos++;
            }
        }

        if ('\0' != pUaDialog->strUserName[0])
        {
            i = request_add_proxy_authorization(request2, www_authenticate, pUaDialog->strUserName, pUaDialog->strPassword);
        }
        else
        {
            i = request_add_proxy_authorization(request2, www_authenticate, (char*)"anonymous", (char*)"");
        }

        if (i != 0)
        {
            osip_message_free(request2);
            request2 = NULL;
            SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv456xxForInvite() exit---: Request Add Proxy Authorization Error \r\n");
            goto release_sess;
        }

        /* 获取socket */
        socket = get_socket_by_port(pUaDialog->iLocalPort);

        if (socket <= 0)
        {
            osip_message_free(request2);
            request2 = NULL;
            SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv456xxForInvite() exit---: Get Socket Error \r\n");
            goto release_sess;
        }

        if (osip_transaction_init(&tr2, ICT , g_recv_cell , request2) != 0)
        {
            osip_message_free(request2);
            request2 = NULL;
            SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv456xxForInvite() exit---: Transaction Init Error \r\n");
            goto release_sess;
        }

        /* 设置transaction的socket */
        osip_transaction_set_in_socket(tr2, socket);
        osip_transaction_set_out_socket(tr2, socket);

        i = ul_sendmessage(tr2, request2);

        if (0 != i)
        {
            SIP_DEBUG_TRACE(LOG_ERROR, "OnRcv456xxForInvite(): ul_sendmessage Error \r\n");
        }

        dialog = get_dialog_sip_dialog(index);
        cseq_num = osip_atoi(request2->cseq->number);
        sip_dialog_set_localcseq(dialog, cseq_num);
        dialog->ict_tr = tr2;
        ua_timer_use(UA_INVITE_TIMEOUT, index, tr2, NULL); /*add INVITE timeout timer*/

        pUaDialog->iReInviteCnt++;

        return 0;
    }
    else if (sip->status_code == 407)
    {
        int cseq_num = 0;
        osip_transaction_t* tr2 = NULL;
        osip_message_t* request2 = NULL;
        osip_proxy_authenticate_t* proxy_authenticate = NULL;
        sip_dialog_t* dialog = NULL;

        if (&sip->proxy_authenticates != NULL && !osip_list_eol(&sip->proxy_authenticates, 0))
        {
            proxy_authenticate = (osip_proxy_authenticate_t*)osip_list_get(&sip->proxy_authenticates, 0);
        }

        if (proxy_authenticate == NULL)
        {
            SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv456xxForInvite() exit---: Get Proxy Authorization Error \r\n");
            goto release_sess;
        }

        i = generating_request_fromrequest(tr->orig_request, &request2, pUaDialog->strLocalIP, pUaDialog->iLocalPort);

        if (i != 0)
        {
            SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv456xxForInvite() exit---: Generating Request From Request Error \r\n");
            goto release_sess;
        }

        /* remove all old authorizations and proxy authorizations*/
        {
            int pos = 0;
            osip_authorization_t* authorization = NULL;
            osip_proxy_authorization_t* proxy_authorization = NULL;
            pos = 0;

            while (!osip_list_eol(&request2->authorizations, pos))
            {
                authorization = (osip_authorization_t*)osip_list_get(&request2->authorizations, pos);

                if (NULL == authorization)
                {
                    pos++;
                    continue;
                }

                osip_list_remove(&request2->authorizations, pos);
                osip_authorization_free(authorization);
                authorization = NULL;
                pos++;
            }

            pos = 0;

            while (!osip_list_eol(&request2->proxy_authorizations, pos))
            {
                proxy_authorization = (osip_proxy_authorization_t*)osip_list_get(&request2->proxy_authorizations, pos);

                if (NULL == proxy_authorization)
                {
                    pos++;
                    continue;
                }

                osip_list_remove(&request2->proxy_authorizations, pos);
                osip_authorization_free(proxy_authorization);
                proxy_authorization = NULL;
                pos++;
            }
        }

        if ('\0' != pUaDialog->strUserName[0])
        {
            i = request_add_proxy_authorization(request2, proxy_authenticate, pUaDialog->strUserName, pUaDialog->strPassword);
        }
        else
        {
            i = request_add_proxy_authorization(request2, proxy_authenticate, (char*)"anonymous", (char*)"");
        }

        if (i != 0)
        {
            osip_message_free(request2);
            request2 = NULL;
            SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv456xxForInvite() exit---: Request Add Proxy Authorization Error \r\n");
            goto release_sess;
        }

        /* 获取socket */
        socket = get_socket_by_port(pUaDialog->iLocalPort);

        if (socket <= 0)
        {
            osip_message_free(request2);
            request2 = NULL;
            SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv456xxForInvite() exit---: Get Socket Error \r\n");
            goto release_sess;
        }

        if (osip_transaction_init(&tr2, ICT , g_recv_cell , request2) != 0)
        {
            osip_message_free(request2);
            request2 = NULL;
            SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv456xxForInvite() exit---: Transaction Init Error \r\n");
            goto release_sess;
        }

        /* 设置transaction的socket */
        osip_transaction_set_in_socket(tr2, socket);
        osip_transaction_set_out_socket(tr2, socket);

        i = ul_sendmessage(tr2, request2);

        if (0 != i)
        {
            SIP_DEBUG_TRACE(LOG_ERROR, "OnRcv456xxForInvite(): ul_sendmessage Error \r\n");
        }

        dialog = get_dialog_sip_dialog(index);
        cseq_num = osip_atoi(request2->cseq->number);
        sip_dialog_set_localcseq(dialog, cseq_num);
        dialog->ict_tr = tr2;
        ua_timer_use(UA_INVITE_TIMEOUT, index, tr2, NULL); /*add INVITE timeout timer*/

        pUaDialog->iReInviteCnt++;

        return 0;
    }
    else if (sip->status_code == 422)
    {
        int min_expires = 0;
        osip_header_t* min_expires_header = NULL;
        msg_get_min_se(sip, 0, &min_expires_header);

        if (NULL == min_expires_header || NULL == min_expires_header->hvalue)
        {
            SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv456xxForInvite() exit---: Get Min Session Expires Error \r\n");
            goto release_sess;
        }

        min_expires = osip_atoi(min_expires_header->hvalue);

        if (min_expires <= 0 || min_expires <= pUaDialog->iSessionExpires)
        {
            SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv456xxForInvite() exit---: Get Min Session Expires Error \r\n");
            goto release_sess;
        }

        pUaDialog->iSessionExpires = min_expires;
        pUaDialog->iSessionExpiresCount = min_expires;
        pUaDialog->iUpdateSendCount = min_expires / 2;

        int cseq_num = 0;
        osip_transaction_t* tr2 = NULL;
        osip_message_t* request2 = NULL;
        sip_dialog_t* dialog = NULL;

        i = generating_request_fromrequest(tr->orig_request, &request2, pUaDialog->strLocalIP, pUaDialog->iLocalPort);

        if (i != 0)
        {
            SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv456xxForInvite() exit---: Generating Request From Request Error \r\n");
            goto release_sess;
        }

        /* 获取消息中的Session Expires 头域 */
        se_pos = msg_get_session_expires(request2, 0, &session_expires_header);

        if (se_pos < 0)
        {
            osip_message_free(request2);
            request2 = NULL;
            SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv456xxForInvite() exit---: Request Get Session Expires Error \r\n");
            goto release_sess;
        }

        /* 移除老的Session Expires 头域 */
        osip_list_remove(&request2->headers, se_pos);
        osip_header_free(session_expires_header);
        session_expires_header = NULL;

        /* 添加新的Session Expires 头域 */
        i = msg_set_session_expires(request2, min_expires_header->hvalue);

        if (i != 0)
        {
            osip_message_free(request2);
            request2 = NULL;
            SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv456xxForInvite() exit---: Request Set Session Expires Error \r\n");
            goto release_sess;
        }

        i = msg_set_min_se(request2, min_expires_header->hvalue);

        if (i != 0)
        {
            osip_message_free(request2);
            request2 = NULL;
            SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv456xxForInvite() exit---: Request Set Min Session Expires Error \r\n");
            goto release_sess;
        }

        /* 获取socket */
        socket = get_socket_by_port(pUaDialog->iLocalPort);

        if (socket <= 0)
        {
            osip_message_free(request2);
            request2 = NULL;
            SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv456xxForInvite() exit---: Get Socket Error \r\n");
            goto release_sess;
        }

        if (osip_transaction_init(&tr2, ICT , g_recv_cell , request2) != 0)
        {
            osip_message_free(request2);
            request2 = NULL;
            SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv456xxForInvite() exit---: Transaction Init Error \r\n");
            goto release_sess;
        }

        /* 设置transaction的socket */
        osip_transaction_set_in_socket(tr2, socket);
        osip_transaction_set_out_socket(tr2, socket);

        i = ul_sendmessage(tr2, request2);

        if (0 != i)
        {
            SIP_DEBUG_TRACE(LOG_ERROR, "OnRcv456xxForInvite(): ul_sendmessage Error \r\n");
        }

        dialog = get_dialog_sip_dialog(index);
        cseq_num = osip_atoi(request2->cseq->number);
        sip_dialog_set_localcseq(dialog, cseq_num);
        dialog->ict_tr = tr2;
        ua_timer_use(UA_INVITE_TIMEOUT, index, tr2, NULL); /*add INVITE timeout timer*/

        pUaDialog->iReInviteCnt++;

        return 0;
    }

release_sess:

    statuscode = sip->status_code;

    /* 调用钩子函数 */
    if (NULL != g_AppCallback && NULL != g_AppCallback->invite_response_received_cb)
    {
        if ((NULL != pUaDialog) && (NULL != pUaDialog->pSipDialog) && (NULL != pUaDialog->pSipDialog->call_id))
        {
            call_id = osip_getcopy(pUaDialog->pSipDialog->call_id);
        }
        else
        {
            call_id = osip_getcopy(sip->call_id->number);
        }

        if ((NULL != sip) && (NULL != sip->from) && (NULL != sip->from->url) && (NULL != sip->from->url->username))
        {
            caller_id = osip_getcopy(sip->from->url->username);
        }

        if ((NULL != sip) && (NULL != sip->to) && (NULL != sip->to->url) && (NULL != sip->to->url->username))
        {
            if (NULL != sip->req_uri && NULL != sip->req_uri->username)
            {
                if (0 != sstrcmp(sip->req_uri->username, sip->to->url->username))
                {
                    callee_id = osip_getcopy(sip->req_uri->username);
                }
                else
                {
                    callee_id = osip_getcopy(sip->to->url->username);
                }
            }
            else
            {
                callee_id = osip_getcopy(sip->to->url->username);
            }
        }
        else if (NULL != sip->req_uri && NULL != sip->req_uri->username)
        {
            callee_id = osip_getcopy(sip->req_uri->username);
        }

        g_AppCallback->invite_response_received_cb(caller_id, callee_id, call_id, index, statuscode, sip->reason_phrase, NULL, 0, g_AppCallback->invite_response_received_cb_user_data);

        if (NULL != caller_id)
        {
            osip_free(caller_id);
            caller_id = NULL;
        }

        if (NULL != callee_id)
        {
            osip_free(callee_id);
            callee_id = NULL;
        }

        if (NULL != call_id)
        {
            osip_free(call_id);
            call_id = NULL;
        }
    }

    update_dialog_as_uac(index, tr, sip, DLG_EVENT_REJECTED);
    eUiState = pUaDialog->eUiState;

    if (eUiState == UI_STATE_CALL_SENT)
    {
        pUaDialog->eUiState = UI_STATE_CALL_TERMINATED;
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : OnRcv1xxForRequest
 功能描述  : 收到REQUEST 1xx响应消息处理函数
 输入参数  : osip_transaction_t* tr, osip_message_t* sip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int OnRcv1xxForRequest(osip_transaction_t* tr, osip_message_t* sip)
{
    if (NULL == tr || NULL == sip)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv1xxForRequest() exit---: Message NULL \r\n");
        return -1;
    }

    //SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv1xxForRequest() Enter--- \r\n");

    return 0;
}

/*****************************************************************************
 函 数 名  : OnRcv2xxForRegister
 功能描述  : 收到REGISTER 2xx响应消息处理函数
 输入参数  : osip_transaction_t* tr, osip_message_t* sip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int OnRcv2xxForRegister(osip_transaction_t* tr, osip_message_t* sip)
{
    int pos = -1, usrpos = -1;
    int expires = 0;
    osip_header_t*  header = NULL;
    osip_header_t*  time_header = NULL;
    osip_generic_param_t* gen_param = NULL;
    osip_contact_t* contact = NULL;
    uac_reg_info_t* pUacRegInfo = NULL;
    int statuscode = 0;
    unsigned int iTime = 0;

    if (NULL == tr || NULL == sip)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv2xxForRegister() exit---: Message NULL \r\n");
        return -1;
    }

    //SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv2xxForRegister() Enter--- \r\n");

    osip_message_get_expires(sip, 0, &header);

    usrpos = uac_reginfo_find(sip->call_id->number);
    SIP_DEBUG_TRACE(LOG_INFO, "OnRcv2xxForRegister() uac_reginfo_find:number=%s, pos=%d \r\n", sip->call_id->number, usrpos);

    if (usrpos < 0)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv2xxForRegister() exit---: Find UAC Register Info Error \r\n");
        return -1;
    }

    pUacRegInfo = uac_reginfo_get(usrpos);

    if (NULL == pUacRegInfo)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv2xxForRegister() exit---: Get UAC Register Info Error \r\n");
        return -1;
    }

    SIP_DEBUG_TRACE(LOG_INFO, "OnRcv2xxForRegister() UacRegInfo:service_id=%s, server_ip=%s, server_port=%d, local_id=%s, local_ip=%s, local_port=%d, register_callid_number=%s \r\n", pUacRegInfo->proxy_id, pUacRegInfo->proxyip, pUacRegInfo->proxyport, pUacRegInfo->register_id, pUacRegInfo->localip, pUacRegInfo->localport, pUacRegInfo->register_callid_number);

    pos = 0;

    while (osip_message_get_contact(sip, pos, &contact) != -1)
    {
        if (NULL == contact->url->username)
        {
            pos++;
            continue;
        }

        if ((sstrcmp(pUacRegInfo->register_account, contact->url->username) == 0)
            || (sstrcmp(pUacRegInfo->register_id, contact->url->username) == 0))
        {
            osip_contact_param_get_byname(contact, (char*)"expires", &gen_param);

            if (gen_param != NULL)
            {
                expires = osip_atoi(gen_param->gvalue);
            }
            else if (header != NULL)
            {
                expires = osip_atoi(header->hvalue);
            }
            else
            {
                expires = 0;
            }

            if (expires == 0)
            {
                pUacRegInfo->isReg = 0;
                pUacRegInfo->isReging = 0;
                pUacRegInfo->expires = 0;
                pUacRegInfo->min_expires = 0;
                SIP_DEBUG_TRACE(LOG_INFO, "OnRcv2xxForRegister() Remove: register_id=%s, pos=%d, expires=%d \r\n", pUacRegInfo->register_id, usrpos, pUacRegInfo->expires);
            }
            else
            {
                cs_timer_use(OUT_REG_TIMER, usrpos, NULL);
                pUacRegInfo->isReg = 1;
                pUacRegInfo->isReging = 0;
                pUacRegInfo->expires = expires;
                pUacRegInfo->min_expires = expires;
                SIP_DEBUG_TRACE(LOG_INFO, "OnRcv2xxForRegister() OUT_REG_TIMER USE: register_id=%s, pos=%d, expires=%d \r\n", pUacRegInfo->register_id, usrpos, pUacRegInfo->expires);
            }

            break;
        }

        pos++;
    }

    if (contact == NULL && header != NULL) /* compatible with our call server */
    {
        expires = osip_atoi(header->hvalue);

        if (expires == 0)
        {
            pUacRegInfo->isReg = 0;
            pUacRegInfo->isReging = 0;
            pUacRegInfo->expires = 0;
            pUacRegInfo->min_expires = 0;
            SIP_DEBUG_TRACE(LOG_INFO, "OnRcv2xxForRegister() Remove: register_id=%s, pos=%d, expires=%d \r\n", pUacRegInfo->register_id, usrpos, pUacRegInfo->expires);
        }
        else
        {
            cs_timer_use(OUT_REG_TIMER, usrpos, NULL);
            pUacRegInfo->isReg = 1;
            pUacRegInfo->isReging = 0;
            pUacRegInfo->expires = expires;
            pUacRegInfo->min_expires = expires;
            SIP_DEBUG_TRACE(LOG_INFO, "OnRcv2xxForRegister() OUT_REG_TIMER USE: register_id=%s, pos=%d, expires=%d \r\n", pUacRegInfo->register_id, usrpos, pUacRegInfo->expires);
        }
    }
    else if (contact == NULL && header == NULL)
    {
        cs_timer_use(OUT_REG_TIMER, usrpos, NULL);
        pUacRegInfo->isReg = 1;
        pUacRegInfo->isReging = 0;
        pUacRegInfo->expires = DEFAULT_REG_CLEARATE;
        pUacRegInfo->min_expires = DEFAULT_REG_CLEARATE;
        SIP_DEBUG_TRACE(LOG_INFO, "OnRcv2xxForRegister() OUT_REG_TIMER USE: register_id=%s, pos=%d, expires=%d \r\n", pUacRegInfo->register_id, usrpos, pUacRegInfo->expires);
    }

    statuscode = sip->status_code;

    osip_message_get_date(sip, 0, &time_header);

    if (NULL != time_header)
    {
        iTime = analysis_time1(time_header->hvalue);
    }

    /* 调用钩子函数 */
    if (NULL != g_AppCallback && NULL != g_AppCallback->uac_register_response_received_cb)
    {
        g_AppCallback->uac_register_response_received_cb(usrpos, pUacRegInfo->expires, statuscode, NULL, iTime, g_AppCallback->uac_register_response_received_cb_user_data);
    }

    if ((pUacRegInfo->expires == 0) && (pUacRegInfo->isReg == 0) && (pUacRegInfo->isReging == 0))
    {
        uac_reginfo_remove(usrpos);
        SIP_DEBUG_TRACE(LOG_INFO, "OnRcv2xxForRegister() uac_reginfo_remove:pos=%d \r\n", usrpos);
    }

    SIP_DEBUG_TRACE(LOG_INFO, "OnRcv2xxForRegister() Exit---:pos=%d \r\n", usrpos);
    return 0;
}

/*****************************************************************************
 函 数 名  : OnRcv3xxForRegister
 功能描述  : 收到REGISTER 3xx响应消息处理函数
 输入参数  : osip_transaction_t* tr, osip_message_t* sip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int OnRcv3xxForRegister(osip_transaction_t* tr, osip_message_t* sip)
{
    int usrpos = -1;
    uac_reg_info_t* pUacRegInfo = NULL;
    int statuscode = 0;

    if (NULL == tr || NULL == sip)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv3xxForRegister() exit---: Message NULL \r\n");
        return -1;
    }

    //SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv3xxForRegister() Enter--- \r\n");

    usrpos = uac_reginfo_find(sip->call_id->number);
    SIP_DEBUG_TRACE(LOG_INFO, "OnRcv3xxForRegister() uac_reginfo_find:number=%s, usrpos=%d \r\n", sip->call_id->number, usrpos);

    if (usrpos < 0)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv3xxForRegister() exit---: Find UAC Register Info Error \r\n");
        return -1;
    }

    pUacRegInfo = uac_reginfo_get(usrpos);

    if (NULL == pUacRegInfo)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv3xxForRegister() exit---: Get UAC Register Info Error \r\n");
        return -1;
    }

    pUacRegInfo->isReg = 0;
    pUacRegInfo->isReging = 1;

    statuscode = sip->status_code;

    /* 调用钩子函数 */
    if (NULL != g_AppCallback && NULL != g_AppCallback->uac_register_response_received_cb)
    {
        g_AppCallback->uac_register_response_received_cb(usrpos, 0, statuscode, NULL, 0, g_AppCallback->uac_register_response_received_cb_user_data);
    }

    uac_reginfo_remove(usrpos);
    SIP_DEBUG_TRACE(LOG_INFO, "OnRcv3xxForRegister() uac_reginfo_remove:usrpos=%d \r\n", usrpos);

    return 0;
}

/*****************************************************************************
 函 数 名  : OnRcv456xxForRegister
 功能描述  : 收到REGISTER 456xx响应消息处理函数
 输入参数  : osip_transaction_t* tr, osip_message_t* sip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int OnRcv456xxForRegister(osip_transaction_t* tr, osip_message_t* sip)
{
    int i = 0;
    int usrpos = -1;
    uac_reg_info_t* pUacRegInfo = NULL;
    int statuscode = 0;

    if (NULL == tr || NULL == sip)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv456xxForRegister() exit---: Message NULL \r\n");
        return -1;
    }

    //SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv456xxForRegister() Enter--- \r\n");

    usrpos = uac_reginfo_find(sip->call_id->number);
    SIP_DEBUG_TRACE(LOG_INFO, "OnRcv456xxForRegister() uac_reginfo_find:number=%s, usrpos=%d \r\n", sip->call_id->number, usrpos);

    if (usrpos < 0)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv456xxForRegister() exit---: Find UAC Register Info Error \r\n");
        return -1;
    }

    pUacRegInfo = uac_reginfo_get(usrpos);

    if (NULL == pUacRegInfo)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv456xxForRegister() exit---: Get UAC Register Info Error \r\n");
        return -1;
    }

    statuscode = sip->status_code;

    pUacRegInfo->isReg = 0;
    pUacRegInfo->isReging = 1;

    if (pUacRegInfo->isReging)
    {
        if (statuscode == 401 || statuscode == 407)
        {
            int expires = 0;
            osip_header_t* header = NULL;

            osip_message_get_expires(sip, 0, &header);

            if (header && header->hvalue)
            {
                expires = osip_atoi(header->hvalue);
            }
            else
            {
                expires = DEFAULT_REG_CLEARATE;

                if (expires == 0)
                {
                    expires = 600;
                }
            }

            i = sip_register2(tr->orig_request, sip, pUacRegInfo->proxyip, pUacRegInfo->proxyport, pUacRegInfo->localip, pUacRegInfo->localport, pUacRegInfo->register_account, pUacRegInfo->register_password, pUacRegInfo->link_type);

            if (i == 0)
            {
                pUacRegInfo->register_cseq_number++;

                SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv456xxForRegister() exit---: SIP Register2 OK \r\n");
                return 0;
            }
        }
        else if (statuscode == 423)
        {
            int min_expires = 0;
            osip_header_t* header = NULL;
            i = msg_getmin_expires(sip, 0, &header);

            if (i == 0)
            {
                min_expires = osip_atoi(header->hvalue);
                pUacRegInfo->expires = min_expires;
                pUacRegInfo->min_expires = min_expires;
                i = sip_register(pUacRegInfo->register_id, pUacRegInfo->register_account, pUacRegInfo->proxy_id, pUacRegInfo->proxyip, pUacRegInfo->proxyport, pUacRegInfo->localip, pUacRegInfo->localport, min_expires, pUacRegInfo->register_callid_number, pUacRegInfo->register_cseq_number, pUacRegInfo->link_type);

                if (i == 0)
                {
                    SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv456xxForRegister() exit---: SIP Register Error \r\n");
                    return 0;
                }
            }
        }
        else
        {
            /* 调用钩子函数 */
            if (NULL != g_AppCallback && NULL != g_AppCallback->uac_register_response_received_cb)
            {
                g_AppCallback->uac_register_response_received_cb(usrpos, pUacRegInfo->expires, statuscode, sip->reason_phrase, 0, g_AppCallback->uac_register_response_received_cb_user_data);
            }

            uac_reginfo_remove(usrpos);
            SIP_DEBUG_TRACE(LOG_INFO, "OnRcv456xxForRegister() uac_reginfo_remove:usrpos=%d \r\n", usrpos);
            return 0;
        }
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : OnRcv2xxForCancel
 功能描述  : 收到CANCEL 2xx响应消息处理函数
 输入参数  : osip_transaction_t* tr, osip_message_t* sip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int OnRcv2xxForCancel(osip_transaction_t* tr, osip_message_t* sip)
{
    if (NULL == tr || NULL == sip)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv2xxForCancel() exit---: Message NULL \r\n");
        return -1;
    }

    //SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv2xxForCancel() Enter--- \r\n");

    return 0;
}

/*****************************************************************************
 函 数 名  : OnRcv3xxForCancel
 功能描述  : 收到CANCEL 3xx响应消息处理函数
 输入参数  : osip_transaction_t* tr, osip_message_t* sip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int OnRcv3xxForCancel(osip_transaction_t* tr, osip_message_t* sip)
{
    if (NULL == tr || NULL == sip)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv3xxForCancel() exit---: Message NULL \r\n");
        return -1;
    }

    //SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv3xxForCancel() Enter--- \r\n");

    return 0;
}

/*****************************************************************************
 函 数 名  : OnRcv456xxForCancel
 功能描述  : 收到CANCEL 456xx响应消息处理函数
 输入参数  : osip_transaction_t* tr, osip_message_t* sip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int OnRcv456xxForCancel(osip_transaction_t* tr, osip_message_t* sip)
{
    if (NULL == tr || NULL == sip)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv456xxForCancel() exit---: Message NULL \r\n");
        return -1;
    }

    //SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv456xxForCancel() Enter--- \r\n");

    return 0;
}

/*****************************************************************************
 函 数 名  : OnRcv2xxForBye
 功能描述  : 收到BYE 2xx响应消息处理函数
 输入参数  : osip_transaction_t* tr, osip_message_t* sip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int OnRcv2xxForBye(osip_transaction_t* tr, osip_message_t* sip)
{
    if (NULL == tr || NULL == sip)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv2xxForBye() exit---: Message NULL \r\n");
        return -1;
    }

    //SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv2xxForBye() Enter--- \r\n");

    return 0;
}

/*****************************************************************************
 函 数 名  : OnRcv3xxForBye
 功能描述  : 收到BYE 3xx响应消息处理函数
 输入参数  : osip_transaction_t* tr, osip_message_t* sip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int OnRcv3xxForBye(osip_transaction_t* tr, osip_message_t* sip)
{
    if (NULL == tr || NULL == sip)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv3xxForBye() exit---: Message NULL \r\n");
        return -1;
    }

    //SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv3xxForBye() Enter--- \r\n");

    return 0;
}

/*****************************************************************************
 函 数 名  : OnRcv456xxForBye
 功能描述  : 收到BYE 456xx响应消息处理函数
 输入参数  : osip_transaction_t* tr, osip_message_t* sip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int OnRcv456xxForBye(osip_transaction_t* tr, osip_message_t* sip)
{
    int index = -1;
    int statuscode = 0;
    char* call_id = NULL;
    char* caller_id = NULL;
    char* callee_id = NULL;
    ua_dialog_t* pUaDialog = NULL;

    if (NULL == tr || NULL == sip)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv456xxForBye() exit---: Message NULL \r\n");
        return -1;
    }

    //SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv456xxForBye() Enter--- \r\n");

    index = find_dialog_as_uac(sip);
    //SIP_DEBUG_TRACE(LOG_INFO, "OnRcv456xxForBye() find_dialog_as_uac:index=%d \r\n", index);

    if (!is_valid_dialog_index(index))
    {
        //SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv456xxForBye() exit---: Dialog Index Error \r\n");
        return -1;
    }

    pUaDialog = ua_dialog_get(index);

    if (NULL == pUaDialog)
    {
        //SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv456xxForBye() exit---: UA Dialog NULL \r\n");
        return -1;
    }

    statuscode = sip->status_code;

    /* 调用钩子函数 */
    if (NULL != g_AppCallback && NULL != g_AppCallback->bye_response_received_cb)
    {
        if ((NULL != pUaDialog) && (NULL != pUaDialog->pSipDialog) && (NULL != pUaDialog->pSipDialog->call_id))
        {
            call_id = osip_getcopy(pUaDialog->pSipDialog->call_id);
        }
        else
        {
            call_id = osip_getcopy(sip->call_id->number);
        }

        if ((NULL != sip) && (NULL != sip->from) && (NULL != sip->from->url) && (NULL != sip->from->url->username))
        {
            caller_id = osip_getcopy(sip->from->url->username);
        }

        if ((NULL != sip) && (NULL != sip->to) && (NULL != sip->to->url) && (NULL != sip->to->url->username))
        {
            if (NULL != sip->req_uri && NULL != sip->req_uri->username)
            {
                if (0 != sstrcmp(sip->req_uri->username, sip->to->url->username))
                {
                    callee_id = osip_getcopy(sip->req_uri->username);
                }
                else
                {
                    callee_id = osip_getcopy(sip->to->url->username);
                }
            }
            else
            {
                callee_id = osip_getcopy(sip->to->url->username);
            }
        }
        else if (NULL != sip->req_uri && NULL != sip->req_uri->username)
        {
            callee_id = osip_getcopy(sip->req_uri->username);
        }

        g_AppCallback->bye_response_received_cb(caller_id, callee_id, call_id, index, statuscode, g_AppCallback->bye_response_received_cb_user_data);

        if (NULL != caller_id)
        {
            osip_free(caller_id);
            caller_id = NULL;
        }

        if (NULL != callee_id)
        {
            osip_free(callee_id);
            callee_id = NULL;
        }

        if (NULL != call_id)
        {
            osip_free(call_id);
            call_id = NULL;
        }
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : OnRcv2xxForUpdate
 功能描述  : 收到UPDATE 2xx响应消息处理函数
 输入参数  : osip_transaction_t* tr, osip_message_t* sip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年9月2日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int OnRcv2xxForUpdate(osip_transaction_t* tr, osip_message_t* sip)
{
    int index = 0;
    osip_header_t* session_expires_header = NULL;
    int iSessionExpires = 0;
    char* pExpires = NULL, *tmp = NULL;
    ua_dialog_t* pUaDialog = NULL;

    if (NULL == tr || NULL == sip)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv2xxForUpdate() exit---: Message NULL \r\n");
        return -1;
    }

    SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv2xxForUpdate() Enter--- \r\n");

    index = find_dialog_as_uac(sip);
    SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv2xxForUpdate() find_dialog_as_uac:index=%d \r\n", index);

    if (is_valid_dialog_index(index))
    {
        msg_get_session_expires(sip, 0, &session_expires_header);

        if (NULL != session_expires_header && NULL != session_expires_header->hvalue)
        {
            tmp = strchr(session_expires_header->hvalue, ';'); /*find '=' */

            if (tmp != NULL)
            {
                if (tmp - session_expires_header->hvalue > 0)
                {
                    pExpires = (char*) osip_malloc(tmp - session_expires_header->hvalue + 1);

                    if (NULL != pExpires)
                    {
                        osip_strncpy(pExpires, session_expires_header->hvalue, tmp - session_expires_header->hvalue);
                        iSessionExpires = osip_atoi(pExpires);
                        osip_free(pExpires);
                        pExpires = NULL;
                    }
                }
                else
                {
                    //printf("\r\n OnRcv2xxForUpdate:tmp - session_expires_header->hvalue=%d \r\n", tmp - session_expires_header->hvalue);
                }
            }
            else
            {
                iSessionExpires = osip_atoi(session_expires_header->hvalue);
            }

            //SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv2xxForUpdate() iSessionExpires=%d \r\n", iSessionExpires);
        }

        pUaDialog = ua_dialog_get(index);

        if (pUaDialog != NULL)
        {
            pUaDialog->iSessionExpires = iSessionExpires;
            pUaDialog->iSessionExpiresCount = iSessionExpires;
            pUaDialog->iUpdateSendCount = iSessionExpires / 2;
            SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv2xxForUpdate() OK: index=%d, iSessionExpires=%d \r\n", index, iSessionExpires);
        }
    }
    else
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "OnRcv2xxForUpdate() unvalid dialog index=%d \r\n", index);
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : OnRcv3xxForUpdate
 功能描述  : 收到UPDATE 3xx响应消息处理函数
 输入参数  : osip_transaction_t* tr, osip_message_t* sip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年9月2日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int OnRcv3xxForUpdate(osip_transaction_t* tr, osip_message_t* sip)
{
    int index = -1;
    ua_dialog_t* pUaDialog = NULL;

    if (NULL == tr || NULL == sip)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv3xxForUpdate() exit---: Message NULL \r\n");
        return -1;
    }

    //SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv3xxForUpdate() Enter--- \r\n");

    index = find_dialog_as_uac(sip);
    //SIP_DEBUG_TRACE(LOG_INFO, "OnRcv3xxForUpdate() find_dialog_as_uas:index=%d \r\n", index);

    if (is_valid_dialog_index(index))
    {
        pUaDialog = ua_dialog_get(index);

        if (NULL != pUaDialog)
        {

        }
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : OnRcv456xxForUpdate
 功能描述  : 收到UPDATE 456xx响应消息处理函数
 输入参数  : osip_transaction_t* tr, osip_message_t* sip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年9月2日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int OnRcv456xxForUpdate(osip_transaction_t* tr, osip_message_t* sip)
{
    int index = -1;
    ua_dialog_t* pUaDialog = NULL;

    if (NULL == tr || NULL == sip)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv456xxForUpdate() exit---: Message NULL \r\n");
        return -1;
    }

    //SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv456xxForUpdate() Enter--- \r\n");

    index = find_dialog_as_uac(sip);
    //SIP_DEBUG_TRACE(LOG_INFO, "OnRcv456xxForUpdate() find_dialog_as_uas:index=%d \r\n", index);

    if (is_valid_dialog_index(index))
    {
        pUaDialog = ua_dialog_get(index);

        if (NULL != pUaDialog)
        {

        }
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : OnRcv2xxForInfo
 功能描述  : 收到INFO 2xx响应消息处理函数
 输入参数  : osip_transaction_t* tr, osip_message_t* sip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int OnRcv2xxForInfo(osip_transaction_t* tr, osip_message_t* sip)
{
    int index = -1;

    if (NULL == tr || NULL == sip)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv2xxForInfo() exit---: Message NULL \r\n");
        return -1;
    }

    //SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv2xxForInfo() Enter--- \r\n");

    index = find_dialog_as_uac(sip);
    //SIP_DEBUG_TRACE(LOG_INFO, "OnRcv2xxForInfo() find_dialog_as_uac:index=%d \r\n", index);

    if (is_valid_dialog_index(index))
    {
        /* dialog*/
        update_dialog_as_uac(index, tr, sip, DLG_EVENT_UPDATE);
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : OnRcv3xxForInfo
 功能描述  : 收到INFO 3xx响应消息处理函数
 输入参数  : osip_transaction_t* tr, osip_message_t* sip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int OnRcv3xxForInfo(osip_transaction_t* tr, osip_message_t* sip)
{
    if (NULL == tr || NULL == sip)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv3xxForInfo() exit---: Message NULL \r\n");
        return -1;
    }

    //SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv3xxForInfo() Enter--- \r\n");

    return 0;
}

/*****************************************************************************
 函 数 名  : OnRcv456xxForInfo
 功能描述  : 收到INFO 456xx响应消息处理函数
 输入参数  : osip_transaction_t* tr, osip_message_t* sip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int OnRcv456xxForInfo(osip_transaction_t* tr, osip_message_t* sip)
{
    if (NULL == tr || NULL == sip)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv456xxForInfo() exit---: Message NULL \r\n");
        return -1;
    }

    //SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv456xxForInfo() Enter--- \r\n");

    return 0;
}

/*****************************************************************************
 函 数 名  : OnRcv2xxForOptions
 功能描述  : 收到OPTIONS 2xx响应消息处理函数
 输入参数  : osip_transaction_t* tr, osip_message_t* sip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int OnRcv2xxForOptions(osip_transaction_t* tr, osip_message_t* sip)
{
    if (NULL == tr || NULL == sip)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv2xxForOptions() exit---: Message NULL \r\n");
        return -1;
    }

    //SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv2xxForOptions() Enter--- \r\n");

    return 0;
}

/*****************************************************************************
 函 数 名  : OnRcv3xxForOptions
 功能描述  : 收到OPTIONS 3xx响应消息处理函数
 输入参数  : osip_transaction_t* tr, osip_message_t* sip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int OnRcv3xxForOptions(osip_transaction_t* tr, osip_message_t* sip)
{
    if (NULL == tr || NULL == sip)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv3xxForOptions() exit---: Message NULL \r\n");
        return -1;
    }

    //SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv3xxForOptions() Enter--- \r\n");

    return 0;
}

/*****************************************************************************
 函 数 名  : OnRcv456xxForOptions
 功能描述  : 收到OPTIONS 456xx响应消息处理函数
 输入参数  : osip_transaction_t* tr, osip_message_t* sip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int OnRcv456xxForOptions(osip_transaction_t* tr, osip_message_t* sip)
{
    if (NULL == tr || NULL == sip)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv456xxForOptions() exit---: Message NULL \r\n");
        return -1;
    }

    //SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv456xxForOptions() Enter--- \r\n");

    return 0;
}

#if 0
/*****************************************************************************
 函 数 名  : OnRcv2xxForSubscribe
 功能描述  : 收到SUBSCRIBE 2xx响应消息处理函数
 输入参数  : osip_transaction_t* tr, osip_message_t* sip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int OnRcv2xxForSubscribe(osip_transaction_t* tr, osip_message_t* sip)
{
    char* caller_id = NULL;
    char* callee_id = NULL;
    char* call_id = NULL;

    osip_header_t* expires_header = NULL;
    int subscribe_expires = 0;

    if (NULL == tr || NULL == sip)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv2xxForSubscribe() exit---: Message NULL \r\n");
        return -1;
    }

    /* 获取 expires 头 */
    msg_get_expires(sip, 0, &expires_header);

    if (expires_header != NULL)
    {
        subscribe_expires = osip_atoi(expires_header->hvalue);
    }

    //SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv456xxForMessage() Enter--- \r\n");
    if (NULL != g_AppCallback && NULL != g_AppCallback->subscribe_response_received_cb)
    {
        if (NULL != sip && NULL != sip->from && NULL != sip->from->url && NULL != sip->from->url->username)
        {
            caller_id = osip_getcopy(sip->from->url->username);
        }

        if ((NULL != sip) && (NULL != sip->to) && (NULL != sip->to->url) && (NULL != sip->to->url->username))
        {
            if (NULL != sip->req_uri && NULL != sip->req_uri->username)
            {
                if (0 != sstrcmp(sip->req_uri->username, sip->to->url->username))
                {
                    callee_id = osip_getcopy(sip->req_uri->username);
                }
                else
                {
                    callee_id = osip_getcopy(sip->to->url->username);
                }
            }
            else
            {
                callee_id = osip_getcopy(sip->to->url->username);
            }
        }
        else if (NULL != sip->req_uri && NULL != sip->req_uri->username)
        {
            callee_id = osip_getcopy(sip->req_uri->username);
        }

        if (NULL != sip && NULL != sip->call_id && NULL != sip->call_id->number)
        {
            call_id = osip_getcopy(sip->call_id->number);
        }

        /* 通知上层应用 */
        /* 调用钩子函数 */
        g_AppCallback->subscribe_response_received_cb(caller_id, callee_id, call_id, subscribe_expires, sip->status_code, g_AppCallback->message_response_received_cb_user_data);

        if (NULL != caller_id)
        {
            osip_free(caller_id);
            caller_id = NULL;
        }

        if (NULL != callee_id)
        {
            osip_free(callee_id);
            callee_id = NULL;
        }

        if (NULL != call_id)
        {
            osip_free(call_id);
            call_id = NULL;
        }
    }

    return 0;
}
#endif

void OnRcv2xxForSubscribe(osip_transaction_t* tr, osip_message_t* sip)
{
    int index = 0;
    ua_dialog_t* pUaDialog = NULL;
    sip_subscription_t* sip_sub = NULL;

    char* caller_id = NULL;
    char* callee_id = NULL;
    char* call_id = NULL;

    if (NULL == tr || NULL == sip)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv2xxForSubscribe() exit---: Message NULL \r\n");
        return;
    }

    index = find_dialog_as_uac(sip);
    SIP_DEBUG_TRACE(LOG_INFO, "OnRcv2xxForSubscribe() find_dialog_as_uac:index=%d \r\n", index);

    if (!is_valid_dialog_index(index))
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv2xxForSubscribe() exit---: index Error \r\n");
        return;
    }

    pUaDialog = ua_dialog_get(index);

    if (NULL == pUaDialog)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv2xxForSubscribe() exit---: Get UA Dialog Error:dialog_index=%d \r\n", index);
        return;
    }

    sip_sub = GetDialogSubscription(index);

    if (sip_sub == NULL)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "OnRcv2xxForSubscribe() exit---: GetDialogSubscription Error:dialog_index=%d \r\n", index);
        return;
    }

    update_dialog_as_uac(index, tr, sip, DLG_EVENT_2XX);

    if (sip_sub->state == SUB_STATE_PRE)
    {
        sip_sub->state = SUB_STATE_CONFIRMED;
    }

    /* 调用回调 */
    if (NULL != g_AppCallback && NULL != g_AppCallback->subscribe_within_dialog_response_received_cb)
    {
        if (NULL != sip && NULL != sip->from && NULL != sip->from->url && NULL != sip->from->url->username)
        {
            caller_id = osip_getcopy(sip->from->url->username);
        }

        if ((NULL != sip) && (NULL != sip->to) && (NULL != sip->to->url) && (NULL != sip->to->url->username))
        {
            if (NULL != sip->req_uri && NULL != sip->req_uri->username)
            {
                if (0 != sstrcmp(sip->req_uri->username, sip->to->url->username))
                {
                    callee_id = osip_getcopy(sip->req_uri->username);
                }
                else
                {
                    callee_id = osip_getcopy(sip->to->url->username);
                }
            }
            else
            {
                callee_id = osip_getcopy(sip->to->url->username);
            }
        }
        else if (NULL != sip->req_uri && NULL != sip->req_uri->username)
        {
            callee_id = osip_getcopy(sip->req_uri->username);
        }

        if (NULL != sip && NULL != sip->call_id && NULL != sip->call_id->number)
        {
            call_id = osip_getcopy(sip->call_id->number);
        }

        /* 通知上层应用 */
        /* 调用钩子函数 */
        g_AppCallback->subscribe_within_dialog_response_received_cb(caller_id, callee_id, call_id, index, sip_sub->duration, sip->status_code);

        if (NULL != caller_id)
        {
            osip_free(caller_id);
            caller_id = NULL;
        }

        if (NULL != callee_id)
        {
            osip_free(callee_id);
            callee_id = NULL;
        }

        if (NULL != call_id)
        {
            osip_free(call_id);
            call_id = NULL;
        }
    }

    return;
}

#if 0
/*****************************************************************************
 函 数 名  : OnRcv3xxForSubscribe
 功能描述  : 收到SUBSCRIBE 3xx响应消息处理函数
 输入参数  : osip_transaction_t* tr, osip_message_t* sip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int OnRcv3xxForSubscribe(osip_transaction_t* tr, osip_message_t* sip)
{
    char* caller_id = NULL;
    char* callee_id = NULL;
    char* call_id = NULL;

    if (NULL == tr || NULL == sip)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv3xxForSubscribe() exit---: Message NULL \r\n");
        return -1;
    }

    //SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv456xxForMessage() Enter--- \r\n");
    if (NULL != g_AppCallback && NULL != g_AppCallback->subscribe_response_received_cb)
    {
        if (NULL != sip && NULL != sip->from && NULL != sip->from->url && NULL != sip->from->url->username)
        {
            caller_id = osip_getcopy(sip->from->url->username);
        }

        if ((NULL != sip) && (NULL != sip->to) && (NULL != sip->to->url) && (NULL != sip->to->url->username))
        {
            if (NULL != sip->req_uri && NULL != sip->req_uri->username)
            {
                if (0 != sstrcmp(sip->req_uri->username, sip->to->url->username))
                {
                    callee_id = osip_getcopy(sip->req_uri->username);
                }
                else
                {
                    callee_id = osip_getcopy(sip->to->url->username);
                }
            }
            else
            {
                callee_id = osip_getcopy(sip->to->url->username);
            }
        }
        else if (NULL != sip->req_uri && NULL != sip->req_uri->username)
        {
            callee_id = osip_getcopy(sip->req_uri->username);
        }

        if (NULL != sip && NULL != sip->call_id && NULL != sip->call_id->number)
        {
            call_id = osip_getcopy(sip->call_id->number);
        }

        /* 通知上层应用 */
        /* 调用钩子函数 */
        g_AppCallback->subscribe_response_received_cb(caller_id, callee_id, call_id, 0, sip->status_code, g_AppCallback->message_response_received_cb_user_data);

        if (NULL != caller_id)
        {
            osip_free(caller_id);
            caller_id = NULL;
        }

        if (NULL != callee_id)
        {
            osip_free(callee_id);
            callee_id = NULL;
        }

        if (NULL != call_id)
        {
            osip_free(call_id);
            call_id = NULL;
        }
    }

    return 0;
}
#endif

void OnRcv3xxForSubscribe(osip_transaction_t* tr, osip_message_t* sip)
{
    int index = 0;
    ua_dialog_t* pUaDialog = NULL;
    sip_subscription_t* sip_sub = NULL;

    char* caller_id = NULL;
    char* callee_id = NULL;
    char* call_id = NULL;

    if (NULL == tr || NULL == sip)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv3xxForSubscribe() exit---: Message NULL \r\n");
        return;
    }

    index = find_dialog_as_uac(sip);
    SIP_DEBUG_TRACE(LOG_INFO, "OnRcv3xxForSubscribe() find_dialog_as_uac:index=%d \r\n", index);

    if (!is_valid_dialog_index(index))
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv3xxForSubscribe() exit---: index Error \r\n");
        return;
    }

    pUaDialog = ua_dialog_get(index);

    if (NULL == pUaDialog)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv3xxForSubscribe() exit---: Get UA Dialog Error:dialog_index=%d \r\n", index);
        return;
    }

    sip_sub = GetDialogSubscription(index);

    if (sip_sub == NULL)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "OnRcv3xxForSubscribe() exit---: GetDialogSubscription Error:dialog_index=%d \r\n", index);
        return;
    }

    /* 调用回调 */
    if (NULL != g_AppCallback && NULL != g_AppCallback->subscribe_within_dialog_response_received_cb)
    {
        if (NULL != sip && NULL != sip->from && NULL != sip->from->url && NULL != sip->from->url->username)
        {
            caller_id = osip_getcopy(sip->from->url->username);
        }

        if ((NULL != sip) && (NULL != sip->to) && (NULL != sip->to->url) && (NULL != sip->to->url->username))
        {
            if (NULL != sip->req_uri && NULL != sip->req_uri->username)
            {
                if (0 != sstrcmp(sip->req_uri->username, sip->to->url->username))
                {
                    callee_id = osip_getcopy(sip->req_uri->username);
                }
                else
                {
                    callee_id = osip_getcopy(sip->to->url->username);
                }
            }
            else
            {
                callee_id = osip_getcopy(sip->to->url->username);
            }
        }
        else if (NULL != sip->req_uri && NULL != sip->req_uri->username)
        {
            callee_id = osip_getcopy(sip->req_uri->username);
        }

        if (NULL != sip && NULL != sip->call_id && NULL != sip->call_id->number)
        {
            call_id = osip_getcopy(sip->call_id->number);
        }

        /* 通知上层应用 */
        /* 调用钩子函数 */
        g_AppCallback->subscribe_within_dialog_response_received_cb(caller_id, callee_id, call_id, index, 0, sip->status_code);

        if (NULL != caller_id)
        {
            osip_free(caller_id);
            caller_id = NULL;
        }

        if (NULL != callee_id)
        {
            osip_free(callee_id);
            callee_id = NULL;
        }

        if (NULL != call_id)
        {
            osip_free(call_id);
            call_id = NULL;
        }
    }

    if (sip_sub->state == SUB_STATE_PRE)
    {
        sip_sub->state = SUB_STATE_CLEAR;
    }

    return;
}

#if 0
/*****************************************************************************
 函 数 名  : OnRcv456xxForSubscribe
 功能描述  : 收到SUBSCRIBE 456xx响应消息处理函数
 输入参数  : osip_transaction_t* tr, osip_message_t* sip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int OnRcv456xxForSubscribe(osip_transaction_t* tr, osip_message_t* sip)
{
    char* caller_id = NULL;
    char* callee_id = NULL;
    char* call_id = NULL;

    if (NULL == tr || NULL == sip)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv456xxForSubscribe() exit---: Message NULL \r\n");
        return -1;
    }

    //SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv456xxForMessage() Enter--- \r\n");
    if (NULL != g_AppCallback && NULL != g_AppCallback->subscribe_response_received_cb)
    {
        if (NULL != sip && NULL != sip->from && NULL != sip->from->url && NULL != sip->from->url->username)
        {
            caller_id = osip_getcopy(sip->from->url->username);
        }

        if ((NULL != sip) && (NULL != sip->to) && (NULL != sip->to->url) && (NULL != sip->to->url->username))
        {
            if (NULL != sip->req_uri && NULL != sip->req_uri->username)
            {
                if (0 != sstrcmp(sip->req_uri->username, sip->to->url->username))
                {
                    callee_id = osip_getcopy(sip->req_uri->username);
                }
                else
                {
                    callee_id = osip_getcopy(sip->to->url->username);
                }
            }
            else
            {
                callee_id = osip_getcopy(sip->to->url->username);
            }
        }
        else if (NULL != sip->req_uri && NULL != sip->req_uri->username)
        {
            callee_id = osip_getcopy(sip->req_uri->username);
        }

        if (NULL != sip && NULL != sip->call_id && NULL != sip->call_id->number)
        {
            call_id = osip_getcopy(sip->call_id->number);
        }

        /* 通知上层应用 */
        /* 调用钩子函数 */
        g_AppCallback->subscribe_response_received_cb(caller_id, callee_id, call_id, 0, sip->status_code, g_AppCallback->message_response_received_cb_user_data);

        if (NULL != caller_id)
        {
            osip_free(caller_id);
            caller_id = NULL;
        }

        if (NULL != callee_id)
        {
            osip_free(callee_id);
            callee_id = NULL;
        }

        if (NULL != call_id)
        {
            osip_free(call_id);
            call_id = NULL;
        }
    }

    return 0;
}
#endif

void OnRcv456xxForSubscribe(osip_transaction_t* tr, osip_message_t* sip)
{
    int index = 0;
    ua_dialog_t* pUaDialog = NULL;
    sip_subscription_t* sip_sub = NULL;

    char* caller_id = NULL;
    char* callee_id = NULL;
    char* call_id = NULL;

    if (NULL == tr || NULL == sip)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv456xxForSubscribe() exit---: Message NULL \r\n");
        return;
    }

    index = find_dialog_as_uac(sip);
    SIP_DEBUG_TRACE(LOG_INFO, "OnRcv456xxForSubscribe() find_dialog_as_uac:index=%d \r\n", index);

    if (!is_valid_dialog_index(index))
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv456xxForSubscribe() exit---: index Error \r\n");
        return;
    }

    pUaDialog = ua_dialog_get(index);

    if (NULL == pUaDialog)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv456xxForSubscribe() exit---: Get UA Dialog Error:dialog_index=%d \r\n", index);
        return;
    }

    sip_sub = GetDialogSubscription(index);

    if (sip_sub == NULL)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "OnRcv456xxForSubscribe() exit---: GetDialogSubscription Error:dialog_index=%d \r\n", index);
        return;
    }

    /* 调用回调 */
    if (NULL != g_AppCallback && NULL != g_AppCallback->subscribe_within_dialog_response_received_cb)
    {
        if (NULL != sip && NULL != sip->from && NULL != sip->from->url && NULL != sip->from->url->username)
        {
            caller_id = osip_getcopy(sip->from->url->username);
        }

        if ((NULL != sip) && (NULL != sip->to) && (NULL != sip->to->url) && (NULL != sip->to->url->username))
        {
            if (NULL != sip->req_uri && NULL != sip->req_uri->username)
            {
                if (0 != sstrcmp(sip->req_uri->username, sip->to->url->username))
                {
                    callee_id = osip_getcopy(sip->req_uri->username);
                }
                else
                {
                    callee_id = osip_getcopy(sip->to->url->username);
                }
            }
            else
            {
                callee_id = osip_getcopy(sip->to->url->username);
            }
        }
        else if (NULL != sip->req_uri && NULL != sip->req_uri->username)
        {
            callee_id = osip_getcopy(sip->req_uri->username);
        }

        if (NULL != sip && NULL != sip->call_id && NULL != sip->call_id->number)
        {
            call_id = osip_getcopy(sip->call_id->number);
        }

        /* 通知上层应用 */
        /* 调用钩子函数 */
        g_AppCallback->subscribe_within_dialog_response_received_cb(caller_id, callee_id, call_id, index, 0, sip->status_code);

        if (NULL != caller_id)
        {
            osip_free(caller_id);
            caller_id = NULL;
        }

        if (NULL != callee_id)
        {
            osip_free(callee_id);
            callee_id = NULL;
        }

        if (NULL != call_id)
        {
            osip_free(call_id);
            call_id = NULL;
        }
    }

    if (sip_sub->state == SUB_STATE_PRE)
    {
        sip_sub->state = SUB_STATE_CLEAR;
    }

    return;
}

void OnRcv408ForSubscribe(osip_transaction_t* tr, osip_message_t* sip)
{
    int index = 0;
    ua_dialog_t* pUaDialog = NULL;
    sip_subscription_t* sip_sub = NULL;

    char* caller_id = NULL;
    char* callee_id = NULL;
    char* call_id = NULL;

    if (NULL == tr || NULL == sip)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv408ForSubscribe() exit---: Message NULL \r\n");
        return;
    }

    index = find_dialog_as_uac(sip);
    SIP_DEBUG_TRACE(LOG_INFO, "OnRcv408ForSubscribe() find_dialog_as_uac:index=%d \r\n", index);

    if (!is_valid_dialog_index(index))
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv408ForSubscribe() exit---: index Error \r\n");
        return;
    }

    pUaDialog = ua_dialog_get(index);

    if (NULL == pUaDialog)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv408ForSubscribe() exit---: Get UA Dialog Error:dialog_index=%d \r\n", index);
        return;
    }

    sip_sub = GetDialogSubscription(index);

    if (sip_sub == NULL)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "OnRcv408ForSubscribe() exit---: GetDialogSubscription Error:dialog_index=%d \r\n", index);
        return;
    }

    /* 调用回调 */
    if (NULL != g_AppCallback && NULL != g_AppCallback->subscribe_within_dialog_response_received_cb)
    {
        if (NULL != sip && NULL != sip->from && NULL != sip->from->url && NULL != sip->from->url->username)
        {
            caller_id = osip_getcopy(sip->from->url->username);
        }

        if ((NULL != sip) && (NULL != sip->to) && (NULL != sip->to->url) && (NULL != sip->to->url->username))
        {
            if (NULL != sip->req_uri && NULL != sip->req_uri->username)
            {
                if (0 != sstrcmp(sip->req_uri->username, sip->to->url->username))
                {
                    callee_id = osip_getcopy(sip->req_uri->username);
                }
                else
                {
                    callee_id = osip_getcopy(sip->to->url->username);
                }
            }
            else
            {
                callee_id = osip_getcopy(sip->to->url->username);
            }
        }
        else if (NULL != sip->req_uri && NULL != sip->req_uri->username)
        {
            callee_id = osip_getcopy(sip->req_uri->username);
        }

        if (NULL != sip && NULL != sip->call_id && NULL != sip->call_id->number)
        {
            call_id = osip_getcopy(sip->call_id->number);
        }

        /* 通知上层应用 */
        /* 调用钩子函数 */
        g_AppCallback->subscribe_within_dialog_response_received_cb(caller_id, callee_id, call_id, index, 0, 408);

        if (NULL != caller_id)
        {
            osip_free(caller_id);
            caller_id = NULL;
        }

        if (NULL != callee_id)
        {
            osip_free(callee_id);
            callee_id = NULL;
        }

        if (NULL != call_id)
        {
            osip_free(call_id);
            call_id = NULL;
        }
    }

    if (sip_sub->state == SUB_STATE_PRE)
    {
        sip_sub->state = SUB_STATE_CLEAR;
    }

    return;
}

/*****************************************************************************
 函 数 名  : OnRcv2xxForNotify
 功能描述  : 收到NOTIFY 2xx响应消息处理函数
 输入参数  : osip_transaction_t* tr, osip_message_t* sip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int OnRcv2xxForNotify(osip_transaction_t* tr, osip_message_t* sip)
{
    if (NULL == tr || NULL == sip)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv2xxForNotify() exit---: Message NULL \r\n");
        return -1;
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : OnRcv3xxForNotify
 功能描述  : 收到NOTIFY 3xx响应消息处理函数
 输入参数  : osip_transaction_t* tr, osip_message_t* sip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int OnRcv3xxForNotify(osip_transaction_t* tr, osip_message_t* sip)
{
    int index = 0;
    ua_dialog_t* pUaDialog = NULL;
    sip_subscription_t* sip_sub = NULL;
    osip_header_t* header = NULL;

    if (NULL == tr || NULL == sip)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv3xxForNotify() exit---: Message NULL \r\n");
        return -1;
    }

    index = find_dialog_as_uac(sip);
    SIP_DEBUG_TRACE(LOG_INFO, "OnRcv3xxForNotify() find_dialog_as_uac:index=%d \r\n", index);

    if (!is_valid_dialog_index(index))
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv3xxForNotify() exit---: index Error \r\n");
        return -1;
    }

    pUaDialog = ua_dialog_get(index);

    if (NULL == pUaDialog)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv3xxForNotify() exit---: Get UA Dialog Error:dialog_index=%d \r\n", index);
        return -1;
    }

    sip_sub = GetDialogSubscription(index);

    if (sip_sub == NULL)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "OnRcv3xxForNotify() exit---: GetDialogSubscription Error:dialog_index=%d \r\n", index);
        return -1;
    }

    osip_message_get_retry_after(sip, 0, &header);

    if (header == NULL || header->hvalue == NULL)
    {
        sip_sub->state = SUB_STATE_CLEAR;
    }

    if (sip_sub->state != SUB_STATE_TERMINATED)
    {
        /* retry after */
    }
    else
    {
        sip_sub->state = SUB_STATE_CLEAR;
    }
}

/*****************************************************************************
 函 数 名  : OnRcv456xxForNotify
 功能描述  : 收到NOTIFY 456xx响应消息处理函数
 输入参数  : osip_transaction_t* tr, osip_message_t* sip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int OnRcv456xxForNotify(osip_transaction_t* tr, osip_message_t* sip)
{
    int index = 0;
    ua_dialog_t* pUaDialog = NULL;
    sip_subscription_t* sip_sub = NULL;
    osip_header_t* header = NULL;

    if (NULL == tr || NULL == sip)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv456xxForNotify() exit---: Message NULL \r\n");
        return -1;
    }

    index = find_dialog_as_uac(sip);
    SIP_DEBUG_TRACE(LOG_INFO, "OnRcv456xxForNotify() find_dialog_as_uac:index=%d \r\n", index);

    if (!is_valid_dialog_index(index))
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv456xxForNotify() exit---: index Error \r\n");
        return -1;
    }

    pUaDialog = ua_dialog_get(index);

    if (NULL == pUaDialog)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv456xxForNotify() exit---: Get UA Dialog Error:dialog_index=%d \r\n", index);
        return -1;
    }

    sip_sub = GetDialogSubscription(index);

    if (sip_sub == NULL)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "OnRcv456xxForNotify() exit---: GetDialogSubscription Error:dialog_index=%d \r\n", index);
        return -1;
    }

    if (MSG_TEST_CODE(sip, 481))
    {
        /* delete subscription */
        sip_sub->state = SUB_STATE_CLEAR;
    }
    else
    {
        osip_message_get_retry_after(sip, 0, &header);

        if (header == NULL || header->hvalue == NULL)
        {
            /* delete subscription */
            sip_sub->state = SUB_STATE_CLEAR;
        }

        /* retry after */
        if (sip_sub->state != SUB_STATE_TERMINATED)
        {

        }
        else
        {
            sip_sub->state = SUB_STATE_CLEAR;
        }
    }
}

int OnRcv408ForNotify(osip_transaction_t* tr, osip_message_t* sip)
{
    int index = 0;
    ua_dialog_t* pUaDialog = NULL;
    sip_subscription_t* sip_sub = NULL;
    osip_header_t* header = NULL;

    if (NULL == tr || NULL == sip)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv408ForNotify() exit---: Message NULL \r\n");
        return -1;
    }

    index = find_dialog_as_uac(sip);
    SIP_DEBUG_TRACE(LOG_INFO, "OnRcv408ForNotify() find_dialog_as_uac:index=%d \r\n", index);

    if (!is_valid_dialog_index(index))
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv408ForNotify() exit---: index Error \r\n");
        return -1;
    }

    pUaDialog = ua_dialog_get(index);

    if (NULL == pUaDialog)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv408ForNotify() exit---: Get UA Dialog Error:dialog_index=%d \r\n", index);
        return -1;
    }

    sip_sub = GetDialogSubscription(index);

    if (sip_sub == NULL)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "OnRcv408ForNotify() exit---: GetDialogSubscription Error:dialog_index=%d \r\n", index);
        return -1;
    }

    if (MSG_TEST_CODE(sip, 481))
    {
        /* delete subscription */
        sip_sub->state = SUB_STATE_CLEAR;
    }
    else
    {
        osip_message_get_retry_after(sip, 0, &header);

        if (header == NULL || header->hvalue == NULL)
        {
            /* delete subscription */
            sip_sub->state = SUB_STATE_CLEAR;
        }

        /* retry after */
        if (sip_sub->state != SUB_STATE_TERMINATED)
        {

        }
        else
        {
            sip_sub->state = SUB_STATE_CLEAR;
        }
    }
}

/*****************************************************************************
 函 数 名  : OnRcv2xxForRefer
 功能描述  : 收到REFER 2xx响应消息处理函数
 输入参数  : osip_transaction_t* tr, osip_message_t* sip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int OnRcv2xxForRefer(osip_transaction_t* tr, osip_message_t* sip)
{
    if (NULL == tr || NULL == sip)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv2xxForRefer() exit---: Message NULL \r\n");
        return -1;
    }

    //SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv2xxForRefer() Enter--- \r\n");

    return 0;
}

/*****************************************************************************
 函 数 名  : OnRcv3xxForRefer
 功能描述  : 收到REFER 3xx响应消息处理函数
 输入参数  : osip_transaction_t* tr, osip_message_t* sip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int OnRcv3xxForRefer(osip_transaction_t* tr, osip_message_t* sip)
{
    if (NULL == tr || NULL == sip)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv3xxForRefer() exit---: Message NULL \r\n");
        return -1;
    }

    //SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv3xxForRefer() Enter--- \r\n");

    return 0;
}

/*****************************************************************************
 函 数 名  : OnRcv456xxForRefer
 功能描述  : 收到REFER 456xx响应消息处理函数
 输入参数  : osip_transaction_t* tr, osip_message_t* sip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int OnRcv456xxForRefer(osip_transaction_t* tr, osip_message_t* sip)
{
    if (NULL == tr || NULL == sip)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv456xxForRefer() exit---: Message NULL \r\n");
        return -1;
    }

    //SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv456xxForRefer() Enter--- \r\n");

    return 0;
}

/*****************************************************************************
 函 数 名  : OnRcv2xxForMessage
 功能描述  : 收到MESSAGE 2xx响应消息处理函数
 输入参数  : osip_transaction_t* tr, osip_message_t* sip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int OnRcv2xxForMessage(osip_transaction_t* tr, osip_message_t* sip)
{
    char* caller_id = NULL;
    char* callee_id = NULL;
    char* call_id = NULL;

    if (NULL == tr || NULL == sip)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv456xxForMessage() exit---: Message NULL \r\n");
        return -1;
    }

    //SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv456xxForMessage() Enter--- \r\n");
    if (NULL != g_AppCallback && NULL != g_AppCallback->message_response_received_cb)
    {
        if (NULL != sip && NULL != sip->from && NULL != sip->from->url && NULL != sip->from->url->username)
        {
            caller_id = osip_getcopy(sip->from->url->username);
        }

        if ((NULL != sip) && (NULL != sip->to) && (NULL != sip->to->url) && (NULL != sip->to->url->username))
        {
            if (NULL != sip->req_uri && NULL != sip->req_uri->username)
            {
                if (0 != sstrcmp(sip->req_uri->username, sip->to->url->username))
                {
                    callee_id = osip_getcopy(sip->req_uri->username);
                }
                else
                {
                    callee_id = osip_getcopy(sip->to->url->username);
                }
            }
            else
            {
                callee_id = osip_getcopy(sip->to->url->username);
            }
        }
        else if (NULL != sip->req_uri && NULL != sip->req_uri->username)
        {
            callee_id = osip_getcopy(sip->req_uri->username);
        }

        if (NULL != sip && NULL != sip->call_id && NULL != sip->call_id->number)
        {
            call_id = osip_getcopy(sip->call_id->number);
        }

        /* 通知上层应用 */
        /* 调用钩子函数 */
        g_AppCallback->message_response_received_cb(caller_id, callee_id, call_id, sip->status_code, g_AppCallback->message_response_received_cb_user_data);

        if (NULL != caller_id)
        {
            osip_free(caller_id);
            caller_id = NULL;
        }

        if (NULL != callee_id)
        {
            osip_free(callee_id);
            callee_id = NULL;
        }

        if (NULL != call_id)
        {
            osip_free(call_id);
            call_id = NULL;
        }
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : OnRcv456xxForMessage
 功能描述  : 收到MESSAGE 3xx响应消息处理函数
 输入参数  : osip_transaction_t* tr, osip_message_t* sip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int OnRcv456xxForMessage(osip_transaction_t* tr, osip_message_t* sip)
{
    char* caller_id = NULL;
    char* callee_id = NULL;
    char* call_id = NULL;

    if (NULL == tr || NULL == sip)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv456xxForMessage() exit---: Message NULL \r\n");
        return -1;
    }

    //SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcv456xxForMessage() Enter--- \r\n");
    if (NULL != g_AppCallback && NULL != g_AppCallback->message_response_received_cb)
    {
        if (NULL != sip && NULL != sip->from && NULL != sip->from->url && NULL != sip->from->url->username)
        {
            caller_id = osip_getcopy(sip->from->url->username);
        }

        if ((NULL != sip) && (NULL != sip->to) && (NULL != sip->to->url) && (NULL != sip->to->url->username))
        {
            if (NULL != sip->req_uri && NULL != sip->req_uri->username)
            {
                if (0 != sstrcmp(sip->req_uri->username, sip->to->url->username))
                {
                    callee_id = osip_getcopy(sip->req_uri->username);
                }
                else
                {
                    callee_id = osip_getcopy(sip->to->url->username);
                }
            }
            else
            {
                callee_id = osip_getcopy(sip->to->url->username);
            }
        }
        else if (NULL != sip->req_uri && NULL != sip->req_uri->username)
        {
            callee_id = osip_getcopy(sip->req_uri->username);
        }

        if (NULL != sip && NULL != sip->call_id && NULL != sip->call_id->number)
        {
            call_id = osip_getcopy(sip->call_id->number);
        }

        /* 通知上层应用 */
        /* 调用钩子函数 */
        g_AppCallback->message_response_received_cb(caller_id, callee_id, call_id, sip->status_code, g_AppCallback->message_response_received_cb_user_data);

        if (NULL != caller_id)
        {
            osip_free(caller_id);
            caller_id = NULL;
        }

        if (NULL != callee_id)
        {
            osip_free(callee_id);
            callee_id = NULL;
        }

        if (NULL != call_id)
        {
            osip_free(call_id);
            call_id = NULL;
        }
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : OnRcvIct2xx2
 功能描述  : 收到ICT 2xx2响应消息处理函数
 输入参数  : osip_transaction_t* tr, osip_message_t* sip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int OnRcvIct2xx2(osip_message_t* sip)
{
    int index = 0;
    ixt_t1* ixt;
    ua_timer_t* timer;

    if (NULL == sip)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcvIct2xx2() exit---: Message NULL \r\n");
        return -1;
    }

    //SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcvIct2xx2() Enter--- \r\n");

    index = find_dialog_as_uac(sip);
    //SIP_DEBUG_TRACE(LOG_INFO, "OnRcvIct2xx2() find_dialog_as_uac:index=%d \r\n", index);

    if (is_valid_dialog_index(index))
    {
        timer = ua_timer_find(UA_ACK2XX_RETRANSMIT, index, NULL, sip);

        if (timer != NULL)       /* retransmit Ack */
        {
            ixt = (ixt_t1*)timer->timer;
            ixt->retransmit_interval = 0;
        }
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : OnRcvIstAckfor2xx
 功能描述  : 收到IST ACK for 2xx响应消息处理函数
 输入参数  : osip_transaction_t* tr, osip_message_t* sip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int OnRcvIstAckfor2xx(osip_message_t* sip)
{
    int i = 0;
    int index = 0;
    char* call_id = NULL;
    char* caller_id = NULL;
    char* callee_id = NULL;
    ua_dialog_t* pUaDialog = NULL;

    if (NULL == sip)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcvIstAckfor2xx() exit---: Message NULL \r\n");
        return -1;
    }

    //SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcvIstAckfor2xx() Enter--- \r\n");

    index = find_dialog_as_uas(sip);
    //SIP_DEBUG_TRACE(LOG_INFO, "OnRcvIstAckfor2xx() find_dialog_as_uas:index=%d \r\n", index);

    if (!is_valid_dialog_index(index))
    {
        //SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcvIstAckfor2xx() exit---: Dialog Index Error \r\n");
        return -1;
    }

    pUaDialog = ua_dialog_get(index);

    if (NULL == pUaDialog)
    {
        //SIP_DEBUG_TRACE(LOG_DEBUG, "OnRcvIstAckfor2xx() exit---: UA Dialog NULL \r\n");
        return -1;
    }

    if (NULL != g_AppCallback && NULL != g_AppCallback->ack_received_cb)
    {
        if ((NULL != pUaDialog) && (NULL != pUaDialog->pSipDialog) && (NULL != pUaDialog->pSipDialog->call_id))
        {
            call_id = osip_getcopy(pUaDialog->pSipDialog->call_id);
        }
        else
        {
            call_id = osip_getcopy(sip->call_id->number);
        }

        if ((NULL != sip) && (NULL != sip->from) && (NULL != sip->from->url) && (NULL != sip->from->url->username))
        {
            caller_id = osip_getcopy(sip->from->url->username);
        }

        if ((NULL != sip) && (NULL != sip->to) && (NULL != sip->to->url) && (NULL != sip->to->url->username))
        {
            if (NULL != sip->req_uri && NULL != sip->req_uri->username)
            {
                if (0 != sstrcmp(sip->req_uri->username, sip->to->url->username))
                {
                    callee_id = osip_getcopy(sip->req_uri->username);
                }
                else
                {
                    callee_id = osip_getcopy(sip->to->url->username);
                }
            }
            else
            {
                callee_id = osip_getcopy(sip->to->url->username);
            }
        }
        else if (NULL != sip->req_uri && NULL != sip->req_uri->username)
        {
            callee_id = osip_getcopy(sip->req_uri->username);
        }

        g_AppCallback->ack_received_cb(caller_id, callee_id, call_id, index, g_AppCallback->ack_received_cb_user_data);

        if (NULL != caller_id)
        {
            osip_free(caller_id);
            caller_id = NULL;
        }

        if (NULL != callee_id)
        {
            osip_free(callee_id);
            callee_id = NULL;
        }

        if (NULL != call_id)
        {
            osip_free(call_id);
            call_id = NULL;
        }
    }

    i = ua_timer_remove(UA_ACK2XX_RETRANSMIT, index, NULL, sip);

    if (i == 0) /*stop retransmit 2xx */
    {
        update_dialog_as_uas(index, NULL, sip, DLG_EVENT_UPDATE);

        if (pUaDialog->eUiState == UI_STATE_CALL_ACCEPT)
        {
            pUaDialog->eUiState = UI_STATE_CONNECTED;
        }
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : OnIctTransportError
 功能描述  : ICT传输错误处理函数
 输入参数  : osip_transaction_t* tr
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int OnIctTransportError(osip_transaction_t* tr)
{
    if (NULL == tr)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnIctTransportError() exit---: Transaction NULL \r\n");
        return -1;
    }

    //SIP_DEBUG_TRACE(LOG_DEBUG, "OnIctTransportError() Enter--- \r\n");

    return 0;
}

/*****************************************************************************
 函 数 名  : OnIstTransportError
 功能描述  : IST传输错误处理函数
 输入参数  : osip_transaction_t* tr
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int OnIstTransportError(osip_transaction_t* tr)
{
    if (NULL == tr)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnIstTransportError() exit---: Transaction NULL \r\n");
        return -1;
    }

    //SIP_DEBUG_TRACE(LOG_DEBUG, "OnIstTransportError() Enter--- \r\n");

    return 0;
}

/*****************************************************************************
 函 数 名  : OnNictTransportError
 功能描述  : NICT传输错误处理函数
 输入参数  : osip_transaction_t* tr
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int OnNictTransportError(osip_transaction_t* tr)
{
    if (NULL == tr)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnNictTransportError() exit---: Transaction NULL \r\n");
        return -1;
    }

    //SIP_DEBUG_TRACE(LOG_DEBUG, "OnNictTransportError() Enter--- \r\n");

    return 0;
}

/*****************************************************************************
 函 数 名  : OnNistTransportError
 功能描述  : NIST传输错误处理函数
 输入参数  : osip_transaction_t* tr
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int OnNistTransportError(osip_transaction_t* tr)
{
    if (NULL == tr)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnNistTransportError() exit---: Transaction NULL \r\n");
        return -1;
    }

    //SIP_DEBUG_TRACE(LOG_DEBUG, "OnNistTransportError() Enter--- \r\n");

    return 0;
}

/*****************************************************************************
 函 数 名  : OnKillIctTransactionForRecv
 功能描述  : ICT事务删除处理
 输入参数  : osip_transaction_t* tr
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int OnKillIctTransactionForRecv(osip_transaction_t* tr)
{
    int index = 0;
    char* caller_id = NULL;
    char* callee_id = NULL;
    char* call_id = NULL;
    sip_dialog_t* pSipDlg = NULL;
    ui_state_t eUiState = UI_STATE_IDLE;
    ua_dialog_t* pUaDialog = NULL;

    if (NULL == tr)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnKillIctTransactionForRecv() exit---: Transaction NULL \r\n");
        return -1;
    }

    throw_2garbage(g_recv_garbage, tr);

    ua_timer_remove(UA_CANCEL_TIMEOUT, -1, tr, NULL);

    index = find_dialog_as_uac(tr->orig_request);
    //SIP_DEBUG_TRACE(LOG_INFO, "OnKillIctTransaction() find_dialog_as_uac:index=%d \r\n", index);

    if (is_valid_dialog_index(index))
    {
        pUaDialog = ua_dialog_get(index);

        if (NULL == pUaDialog)
        {
            SIP_DEBUG_TRACE(LOG_DEBUG, "OnKillIctTransactionForRecv() exit---: Get UA Dialog Error:dialog_index=%d \r\n", index);
            return -1;
        }

        if (NULL == tr->last_response || MSG_IS_STATUS_1XX(tr->last_response))
        {
            pSipDlg = get_dialog_sip_dialog(index);

            if (NULL == pSipDlg)
            {
                SIP_DEBUG_TRACE(LOG_DEBUG, "OnKillIctTransactionForRecv() exit---: Get SIP Dialog Error \r\n");
                return -1;
            }

            /* 通知上层应用，收到Invite 回应超时 */
            /* 调用钩子函数 */
            if (NULL != g_AppCallback && NULL != g_AppCallback->invite_response_received_cb)
            {
                if (NULL != pSipDlg->call_id)
                {
                    call_id = osip_getcopy(pSipDlg->call_id);
                }

                if (NULL != pSipDlg->local_uri && NULL != pSipDlg->local_uri->url && pSipDlg->local_uri->url->username)
                {
                    caller_id = osip_getcopy(pSipDlg->local_uri->url->username);
                }

                if (NULL != pSipDlg->remote_uri && NULL != pSipDlg->remote_uri->url && pSipDlg->remote_uri->url->username)
                {
                    callee_id = osip_getcopy(pSipDlg->remote_uri->url->username);
                }

                g_AppCallback->invite_response_received_cb(caller_id, callee_id, call_id, index, 408, (char*)"Request Timeout", NULL, 0, g_AppCallback->invite_response_received_cb_user_data);

                if (NULL != caller_id)
                {
                    osip_free(caller_id);
                    caller_id = NULL;
                }

                if (NULL != callee_id)
                {
                    osip_free(callee_id);
                    callee_id = NULL;
                }

                if (NULL != call_id)
                {
                    osip_free(call_id);
                    call_id = NULL;
                }
            }

            ua_timer_remove(UA_INVITE_TIMEOUT, index, tr, NULL);
            update_dialog_as_uac(index, NULL, NULL, DLG_EVENT_ERROR);

            eUiState = pUaDialog->eUiState;

            if (eUiState == UI_STATE_CALL_SENT)
            {
                /* as receive 408 */
                pUaDialog->eUiState = UI_STATE_CALL_TERMINATED;
            }
        }
    }

    //osip_event_t* se = __osip_event_new(KILL_TRANSACTION, tr->transactionid);
    //i = osip_transaction_add_event(tr, se);
    //osip_transaction_free(tr);
    //tr = NULL;

    return 0;
}

/*****************************************************************************
 函 数 名  : OnKillIstTransactionForRecv
 功能描述  : IST事务删除处理
 输入参数  : osip_transaction_t* tr
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int OnKillIstTransactionForRecv(osip_transaction_t* tr)
{
    if (NULL == tr)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnKillIstTransactionForRecv() exit---: Transaction NULL \r\n");
        return -1;
    }

    throw_2garbage(g_recv_garbage, tr);

    return 0;
}

/*****************************************************************************
 函 数 名  : OnKillNictTransactionForRecv
 功能描述  : NICT事务删除处理
 输入参数  : osip_transaction_t* tr
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int OnKillNictTransactionForRecv(osip_transaction_t* tr)
{
    int index = -1;
    int usrpos = -1;
    ua_dialog_t* pUaDialog = NULL;
    char* call_id = NULL;
    char* caller_id = NULL;
    char* callee_id = NULL;

    if (NULL == tr)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnKillNictTransactionForRecv() exit---: Transaction NULL \r\n");
        return -1;
    }

    throw_2garbage(g_recv_garbage, tr);

    if (MSG_IS_REGISTER(tr->orig_request))
    {
        if (tr->last_response == NULL || MSG_IS_STATUS_1XX(tr->last_response))
        {
            if (NULL != tr->orig_request->call_id
                && NULL != tr->orig_request->call_id->number)
            {
                usrpos = uac_reginfo_find(tr->orig_request->call_id->number);
                SIP_DEBUG_TRACE(LOG_DEBUG, "OnKillNictTransactionForRecv() uac_reginfo_find:number=%s, usrpos=%d \r\n", tr->orig_request->call_id->number, usrpos);

                if (usrpos < 0)
                {
                    SIP_DEBUG_TRACE(LOG_DEBUG, "OnKillNictTransactionForRecv() exit---: Find UAC Register Info Error \r\n");
                    return -1;
                }

                uac_reg_info_t* pUacRegInfo = NULL;
                pUacRegInfo = uac_reginfo_get(usrpos);

                if (NULL != pUacRegInfo)
                {
                    pUacRegInfo->isReg = 0;
                    pUacRegInfo->isReging = 0;
                }

                /* 通知上层应用，收到注册回应超时 */
                /* 调用钩子函数 */
                if (NULL != g_AppCallback && NULL != g_AppCallback->uac_register_response_received_cb)
                {
                    g_AppCallback->uac_register_response_received_cb(usrpos, 0, 408, NULL, 0, g_AppCallback->uac_register_response_received_cb_user_data);
                }

                uac_reginfo_remove(usrpos);
                SIP_DEBUG_TRACE(LOG_INFO, "OnKillNictTransactionForRecv() uac_reginfo_remove:usrpos=%d \r\n", usrpos);
            }
        }
    }
    else if (MSG_IS_NOTIFY(tr->orig_request))
    {
        if (tr->last_response == NULL || MSG_IS_STATUS_1XX(tr->last_response))
        {
            OnRcv408ForNotify(tr, tr->orig_request);
        }
    }
    else if (MSG_IS_SUBSCRIBE(tr->orig_request))
    {
        if (tr->last_response == NULL || MSG_IS_STATUS_1XX(tr->last_response))
        {
            OnRcv408ForSubscribe(tr, tr->orig_request);
        }
    }
    else if (MSG_IS_MESSAGE(tr->orig_request))
    {
        if (tr->last_response == NULL || MSG_IS_STATUS_1XX(tr->last_response))
        {
            if (NULL != g_AppCallback && NULL != g_AppCallback->message_response_received_cb)
            {
                if (NULL != tr && NULL != tr->orig_request && NULL != tr->orig_request->from && NULL != tr->orig_request->from->url && NULL != tr->orig_request->from->url->username)
                {
                    caller_id = osip_getcopy(tr->orig_request->from->url->username);
                }

                if (NULL != tr && NULL != tr->orig_request && NULL != tr->orig_request->to && NULL != tr->orig_request->to->url && NULL != tr->orig_request->to->url->username)
                {
                    callee_id = osip_getcopy(tr->orig_request->to->url->username);
                }

                if (NULL != tr && NULL != tr->orig_request && NULL != tr->orig_request->call_id && NULL != tr->orig_request->call_id->number)
                {
                    call_id = osip_getcopy(tr->orig_request->call_id->number);
                }

                /* 通知上层应用，收到Message 回应超时 */
                /* 调用钩子函数 */
                g_AppCallback->message_response_received_cb(caller_id, callee_id, call_id, 408, g_AppCallback->message_response_received_cb_user_data);

                if (NULL != caller_id)
                {
                    osip_free(caller_id);
                    caller_id = NULL;
                }

                if (NULL != callee_id)
                {
                    osip_free(callee_id);
                    callee_id = NULL;
                }

                if (NULL != call_id)
                {
                    osip_free(call_id);
                    call_id = NULL;
                }
            }
        }
    }
    else if (MSG_IS_INFO(tr->orig_request))
    {
        if (tr->last_response == NULL || MSG_IS_STATUS_1XX(tr->last_response))
        {
            if (NULL != g_AppCallback && NULL != g_AppCallback->info_response_received_cb)
            {
                if (NULL != tr && NULL != tr->orig_request && NULL != tr->orig_request->from && NULL != tr->orig_request->from->url && NULL != tr->orig_request->from->url->username)
                {
                    caller_id = osip_getcopy(tr->orig_request->from->url->username);
                }

                if (NULL != tr && NULL != tr->orig_request && NULL != tr->orig_request->to && NULL != tr->orig_request->to->url && NULL != tr->orig_request->to->url->username)
                {
                    callee_id = osip_getcopy(tr->orig_request->to->url->username);
                }

                if (NULL != tr && NULL != tr->orig_request && NULL != tr->orig_request->call_id && NULL != tr->orig_request->call_id->number)
                {
                    call_id = osip_getcopy(tr->orig_request->call_id->number);
                }

                /* 通知上层应用，收到Info 回应超时 */
                /* 调用钩子函数 */
                g_AppCallback->info_response_received_cb(caller_id, callee_id, call_id, 408, g_AppCallback->info_response_received_cb_user_data);

                if (NULL != caller_id)
                {
                    osip_free(caller_id);
                    caller_id = NULL;
                }

                if (NULL != callee_id)
                {
                    osip_free(callee_id);
                    callee_id = NULL;
                }

                if (NULL != call_id)
                {
                    osip_free(call_id);
                    call_id = NULL;
                }
            }
        }
    }
    else if (MSG_IS_UPDATE(tr->orig_request))
    {
        if (tr->last_response == NULL || MSG_IS_STATUS_1XX(tr->last_response))
        {
            index = find_dialog_as_uac(tr->orig_request);
            //SIP_DEBUG_TRACE(LOG_INFO, "OnKillNictTransaction() find_dialog_as_uac:index=%d \r\n", index);

            if (index >= 0)
            {
                pUaDialog = ua_dialog_get(index);

                if (NULL != pUaDialog)
                {

                }
            }
        }
    }

    //osip_event_t* se = __osip_event_new(KILL_TRANSACTION, tr->transactionid);
    //i = osip_transaction_add_event(tr, se);
    //osip_transaction_free(tr);
    //tr = NULL;

    return 0;
}

/*****************************************************************************
 函 数 名  : OnKillNistTransactionForRecv
 功能描述  : NIST事务删除处理
 输入参数  : osip_transaction_t* tr
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int OnKillNistTransactionForRecv(osip_transaction_t* tr)
{
    if (NULL == tr)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnKillNistTransactionForRecv() exit---: Transaction NULL \r\n");
        return -1;
    }

    throw_2garbage(g_recv_garbage, tr);

    return 0;
}

/*****************************************************************************
 函 数 名  : OnKillIctTransactionForRecvRegister
 功能描述  : ICT事务删除处理
 输入参数  : osip_transaction_t* tr
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int OnKillIctTransactionForRecvRegister(osip_transaction_t* tr)
{
    int index = 0;
    char* caller_id = NULL;
    char* callee_id = NULL;
    char* call_id = NULL;
    sip_dialog_t* pSipDlg = NULL;
    ui_state_t eUiState = UI_STATE_IDLE;
    ua_dialog_t* pUaDialog = NULL;

    if (NULL == tr)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnKillIctTransactionForRecvRegister() exit---: Transaction NULL \r\n");
        return -1;
    }

    throw_2garbage(g_recv_register_garbage, tr);

    ua_timer_remove(UA_CANCEL_TIMEOUT, -1, tr, NULL);

    index = find_dialog_as_uac(tr->orig_request);
    //SIP_DEBUG_TRACE(LOG_INFO, "OnKillIctTransaction() find_dialog_as_uac:index=%d \r\n", index);

    if (is_valid_dialog_index(index))
    {
        pUaDialog = ua_dialog_get(index);

        if (NULL == pUaDialog)
        {
            SIP_DEBUG_TRACE(LOG_DEBUG, "OnKillIctTransactionForRecvRegister() exit---: Get UA Dialog Error:dialog_index=%d \r\n", index);
            return -1;
        }

        if (NULL == tr->last_response || MSG_IS_STATUS_1XX(tr->last_response))
        {
            pSipDlg = get_dialog_sip_dialog(index);

            if (NULL == pSipDlg)
            {
                SIP_DEBUG_TRACE(LOG_DEBUG, "OnKillIctTransactionForRecvRegister() exit---: Get SIP Dialog Error \r\n");
                return -1;
            }

            /* 通知上层应用，收到Invite 回应超时 */
            /* 调用钩子函数 */
            if (NULL != g_AppCallback && NULL != g_AppCallback->invite_response_received_cb)
            {
                if (NULL != pSipDlg->call_id)
                {
                    call_id = osip_getcopy(pSipDlg->call_id);
                }

                if (NULL != pSipDlg->local_uri && NULL != pSipDlg->local_uri->url && pSipDlg->local_uri->url->username)
                {
                    caller_id = osip_getcopy(pSipDlg->local_uri->url->username);
                }

                if (NULL != pSipDlg->remote_uri && NULL != pSipDlg->remote_uri->url && pSipDlg->remote_uri->url->username)
                {
                    callee_id = osip_getcopy(pSipDlg->remote_uri->url->username);
                }

                g_AppCallback->invite_response_received_cb(caller_id, callee_id, call_id, index, 408, (char*)"Request Timeout", NULL, 0, g_AppCallback->invite_response_received_cb_user_data);

                if (NULL != caller_id)
                {
                    osip_free(caller_id);
                    caller_id = NULL;
                }

                if (NULL != callee_id)
                {
                    osip_free(callee_id);
                    callee_id = NULL;
                }

                if (NULL != call_id)
                {
                    osip_free(call_id);
                    call_id = NULL;
                }
            }

            ua_timer_remove(UA_INVITE_TIMEOUT, index, tr, NULL);
            update_dialog_as_uac(index, NULL, NULL, DLG_EVENT_ERROR);

            eUiState = pUaDialog->eUiState;

            if (eUiState == UI_STATE_CALL_SENT)
            {
                /* as receive 408 */
                pUaDialog->eUiState = UI_STATE_CALL_TERMINATED;
            }
        }
    }

    //osip_event_t* se = __osip_event_new(KILL_TRANSACTION, tr->transactionid);
    //i = osip_transaction_add_event(tr, se);
    //osip_transaction_free(tr);
    //tr = NULL;

    return 0;
}

/*****************************************************************************
 函 数 名  : OnKillIstTransactionForRecvRegister
 功能描述  : IST事务删除处理
 输入参数  : osip_transaction_t* tr
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int OnKillIstTransactionForRecvRegister(osip_transaction_t* tr)
{
    if (NULL == tr)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnKillIstTransactionForRecvRegister() exit---: Transaction NULL \r\n");
        return -1;
    }

    throw_2garbage(g_recv_register_garbage, tr);

    return 0;
}

/*****************************************************************************
 函 数 名  : OnKillNictTransactionForRecvRegister
 功能描述  : NICT事务删除处理
 输入参数  : osip_transaction_t* tr
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int OnKillNictTransactionForRecvRegister(osip_transaction_t* tr)
{
    int index = -1;
    int usrpos = -1;
    ua_dialog_t* pUaDialog = NULL;
    char* call_id = NULL;
    char* caller_id = NULL;
    char* callee_id = NULL;

    if (NULL == tr)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnKillNictTransactionForRecvRegister() exit---: Transaction NULL \r\n");
        return -1;
    }

    throw_2garbage(g_recv_register_garbage, tr);

    if (MSG_IS_REGISTER(tr->orig_request))
    {
        if (tr->last_response == NULL || MSG_IS_STATUS_1XX(tr->last_response))
        {
            if (NULL != tr->orig_request->call_id
                && NULL != tr->orig_request->call_id->number)
            {
                usrpos = uac_reginfo_find(tr->orig_request->call_id->number);
                SIP_DEBUG_TRACE(LOG_DEBUG, "OnKillNictTransactionForRecvRegister() uac_reginfo_find:number=%s, usrpos=%d \r\n", tr->orig_request->call_id->number, usrpos);

                if (usrpos < 0)
                {
                    SIP_DEBUG_TRACE(LOG_DEBUG, "OnKillNictTransactionForRecvRegister() exit---: Find UAC Register Info Error \r\n");
                    return -1;
                }

                uac_reg_info_t* pUacRegInfo = NULL;
                pUacRegInfo = uac_reginfo_get(usrpos);

                if (NULL != pUacRegInfo)
                {
                    pUacRegInfo->isReg = 0;
                    pUacRegInfo->isReging = 0;
                }

                /* 通知上层应用，收到注册回应超时 */
                /* 调用钩子函数 */
                if (NULL != g_AppCallback && NULL != g_AppCallback->uac_register_response_received_cb)
                {
                    g_AppCallback->uac_register_response_received_cb(usrpos, 0, 408, NULL, 0, g_AppCallback->uac_register_response_received_cb_user_data);
                }

                uac_reginfo_remove(usrpos);
                SIP_DEBUG_TRACE(LOG_INFO, "OnKillNictTransactionForRecvRegister() uac_reginfo_remove:usrpos=%d \r\n", usrpos);
            }
        }
    }
    else if (MSG_IS_NOTIFY(tr->orig_request))
    {
        if (tr->last_response == NULL || MSG_IS_STATUS_1XX(tr->last_response))
        {
            OnRcv408ForNotify(tr, tr->orig_request);
        }
    }
    else if (MSG_IS_SUBSCRIBE(tr->orig_request))
    {
        if (tr->last_response == NULL || MSG_IS_STATUS_1XX(tr->last_response))
        {
            OnRcv408ForSubscribe(tr, tr->orig_request);
        }
    }
    else if (MSG_IS_MESSAGE(tr->orig_request))
    {
        if (tr->last_response == NULL || MSG_IS_STATUS_1XX(tr->last_response))
        {
            if (NULL != g_AppCallback && NULL != g_AppCallback->message_response_received_cb)
            {
                if (NULL != tr && NULL != tr->orig_request && NULL != tr->orig_request->from && NULL != tr->orig_request->from->url && NULL != tr->orig_request->from->url->username)
                {
                    caller_id = osip_getcopy(tr->orig_request->from->url->username);
                }

                if (NULL != tr && NULL != tr->orig_request && NULL != tr->orig_request->to && NULL != tr->orig_request->to->url && NULL != tr->orig_request->to->url->username)
                {
                    callee_id = osip_getcopy(tr->orig_request->to->url->username);
                }

                if (NULL != tr && NULL != tr->orig_request && NULL != tr->orig_request->call_id && NULL != tr->orig_request->call_id->number)
                {
                    call_id = osip_getcopy(tr->orig_request->call_id->number);
                }

                /* 通知上层应用，收到Message 回应超时 */
                /* 调用钩子函数 */
                g_AppCallback->message_response_received_cb(caller_id, callee_id, call_id, 408, g_AppCallback->message_response_received_cb_user_data);

                if (NULL != caller_id)
                {
                    osip_free(caller_id);
                    caller_id = NULL;
                }

                if (NULL != callee_id)
                {
                    osip_free(callee_id);
                    callee_id = NULL;
                }

                if (NULL != call_id)
                {
                    osip_free(call_id);
                    call_id = NULL;
                }
            }
        }
    }
    else if (MSG_IS_INFO(tr->orig_request))
    {
        if (tr->last_response == NULL || MSG_IS_STATUS_1XX(tr->last_response))
        {
            if (NULL != g_AppCallback && NULL != g_AppCallback->info_response_received_cb)
            {
                if (NULL != tr && NULL != tr->orig_request && NULL != tr->orig_request->from && NULL != tr->orig_request->from->url && NULL != tr->orig_request->from->url->username)
                {
                    caller_id = osip_getcopy(tr->orig_request->from->url->username);
                }

                if (NULL != tr && NULL != tr->orig_request && NULL != tr->orig_request->to && NULL != tr->orig_request->to->url && NULL != tr->orig_request->to->url->username)
                {
                    callee_id = osip_getcopy(tr->orig_request->to->url->username);
                }

                if (NULL != tr && NULL != tr->orig_request && NULL != tr->orig_request->call_id && NULL != tr->orig_request->call_id->number)
                {
                    call_id = osip_getcopy(tr->orig_request->call_id->number);
                }

                /* 通知上层应用，收到Info 回应超时 */
                /* 调用钩子函数 */
                g_AppCallback->info_response_received_cb(caller_id, callee_id, call_id, 408, g_AppCallback->info_response_received_cb_user_data);

                if (NULL != caller_id)
                {
                    osip_free(caller_id);
                    caller_id = NULL;
                }

                if (NULL != callee_id)
                {
                    osip_free(callee_id);
                    callee_id = NULL;
                }

                if (NULL != call_id)
                {
                    osip_free(call_id);
                    call_id = NULL;
                }
            }
        }
    }
    else if (MSG_IS_UPDATE(tr->orig_request))
    {
        if (tr->last_response == NULL || MSG_IS_STATUS_1XX(tr->last_response))
        {
            index = find_dialog_as_uac(tr->orig_request);
            //SIP_DEBUG_TRACE(LOG_INFO, "OnKillNictTransaction() find_dialog_as_uac:index=%d \r\n", index);

            if (index >= 0)
            {
                pUaDialog = ua_dialog_get(index);

                if (NULL != pUaDialog)
                {

                }
            }
        }
    }

    //osip_event_t* se = __osip_event_new(KILL_TRANSACTION, tr->transactionid);
    //i = osip_transaction_add_event(tr, se);
    //osip_transaction_free(tr);
    //tr = NULL;

    return 0;
}

/*****************************************************************************
 函 数 名  : OnKillNistTransactionForRecvRegister
 功能描述  : NIST事务删除处理
 输入参数  : osip_transaction_t* tr
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int OnKillNistTransactionForRecvRegister(osip_transaction_t* tr)
{
    if (NULL == tr)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnKillNistTransactionForRecvRegister() exit---: Transaction NULL \r\n");
        return -1;
    }

    throw_2garbage(g_recv_register_garbage, tr);

    return 0;
}

/*****************************************************************************
 函 数 名  : OnKillIctTransactionForRecvMsg
 功能描述  : ICT事务删除处理
 输入参数  : osip_transaction_t* tr
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int OnKillIctTransactionForRecvMsg(osip_transaction_t* tr)
{
    int index = 0;
    char* caller_id = NULL;
    char* callee_id = NULL;
    char* call_id = NULL;
    sip_dialog_t* pSipDlg = NULL;
    ui_state_t eUiState = UI_STATE_IDLE;
    ua_dialog_t* pUaDialog = NULL;

    if (NULL == tr)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnKillIctTransactionForRecvMsg() exit---: Transaction NULL \r\n");
        return -1;
    }

    throw_2garbage(g_recv_msg_garbage, tr);

    ua_timer_remove(UA_CANCEL_TIMEOUT, -1, tr, NULL);

    index = find_dialog_as_uac(tr->orig_request);
    //SIP_DEBUG_TRACE(LOG_INFO, "OnKillIctTransaction() find_dialog_as_uac:index=%d \r\n", index);

    if (is_valid_dialog_index(index))
    {
        pUaDialog = ua_dialog_get(index);

        if (NULL == pUaDialog)
        {
            SIP_DEBUG_TRACE(LOG_DEBUG, "OnKillIctTransactionForRecvMsg() exit---: Get UA Dialog Error:dialog_index=%d \r\n", index);
            return -1;
        }

        if (NULL == tr->last_response || MSG_IS_STATUS_1XX(tr->last_response))
        {
            pSipDlg = get_dialog_sip_dialog(index);

            if (NULL == pSipDlg)
            {
                SIP_DEBUG_TRACE(LOG_DEBUG, "OnKillIctTransactionForRecvMsg() exit---: Get SIP Dialog Error \r\n");
                return -1;
            }

            /* 通知上层应用，收到Invite 回应超时 */
            /* 调用钩子函数 */
            if (NULL != g_AppCallback && NULL != g_AppCallback->invite_response_received_cb)
            {
                if (NULL != pSipDlg->call_id)
                {
                    call_id = osip_getcopy(pSipDlg->call_id);
                }

                if (NULL != pSipDlg->local_uri && NULL != pSipDlg->local_uri->url && pSipDlg->local_uri->url->username)
                {
                    caller_id = osip_getcopy(pSipDlg->local_uri->url->username);
                }

                if (NULL != pSipDlg->remote_uri && NULL != pSipDlg->remote_uri->url && pSipDlg->remote_uri->url->username)
                {
                    callee_id = osip_getcopy(pSipDlg->remote_uri->url->username);
                }

                g_AppCallback->invite_response_received_cb(caller_id, callee_id, call_id, index, 408, (char*)"Request Timeout", NULL, 0, g_AppCallback->invite_response_received_cb_user_data);

                if (NULL != caller_id)
                {
                    osip_free(caller_id);
                    caller_id = NULL;
                }

                if (NULL != callee_id)
                {
                    osip_free(callee_id);
                    callee_id = NULL;
                }

                if (NULL != call_id)
                {
                    osip_free(call_id);
                    call_id = NULL;
                }
            }

            ua_timer_remove(UA_INVITE_TIMEOUT, index, tr, NULL);
            update_dialog_as_uac(index, NULL, NULL, DLG_EVENT_ERROR);

            eUiState = pUaDialog->eUiState;

            if (eUiState == UI_STATE_CALL_SENT)
            {
                /* as receive 408 */
                pUaDialog->eUiState = UI_STATE_CALL_TERMINATED;
            }
        }
    }

    //osip_event_t* se = __osip_event_new(KILL_TRANSACTION, tr->transactionid);
    //i = osip_transaction_add_event(tr, se);
    //osip_transaction_free(tr);
    //tr = NULL;

    return 0;
}

/*****************************************************************************
 函 数 名  : OnKillIstTransactionForRecvMsg
 功能描述  : IST事务删除处理
 输入参数  : osip_transaction_t* tr
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int OnKillIstTransactionForRecvMsg(osip_transaction_t* tr)
{
    if (NULL == tr)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnKillIstTransactionForRecvMsg() exit---: Transaction NULL \r\n");
        return -1;
    }

    throw_2garbage(g_recv_msg_garbage, tr);

    return 0;
}

/*****************************************************************************
 函 数 名  : OnKillNictTransactionForRecvMsg
 功能描述  : NICT事务删除处理
 输入参数  : osip_transaction_t* tr
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int OnKillNictTransactionForRecvMsg(osip_transaction_t* tr)
{
    int index = -1;
    int usrpos = -1;
    ua_dialog_t* pUaDialog = NULL;
    char* call_id = NULL;
    char* caller_id = NULL;
    char* callee_id = NULL;

    if (NULL == tr)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnKillNictTransactionForRecvMsg() exit---: Transaction NULL \r\n");
        return -1;
    }

    throw_2garbage(g_recv_msg_garbage, tr);

    if (MSG_IS_REGISTER(tr->orig_request))
    {
        if (tr->last_response == NULL || MSG_IS_STATUS_1XX(tr->last_response))
        {
            if (NULL != tr->orig_request->call_id
                && NULL != tr->orig_request->call_id->number)
            {
                usrpos = uac_reginfo_find(tr->orig_request->call_id->number);
                SIP_DEBUG_TRACE(LOG_DEBUG, "OnKillNictTransactionForRecvMsg() uac_reginfo_find:number=%s, usrpos=%d \r\n", tr->orig_request->call_id->number, usrpos);

                if (usrpos < 0)
                {
                    SIP_DEBUG_TRACE(LOG_DEBUG, "OnKillNictTransactionForRecvMsg() exit---: Find UAC Register Info Error \r\n");
                    return -1;
                }

                uac_reg_info_t* pUacRegInfo = NULL;
                pUacRegInfo = uac_reginfo_get(usrpos);

                if (NULL != pUacRegInfo)
                {
                    pUacRegInfo->isReg = 0;
                    pUacRegInfo->isReging = 0;
                }

                /* 通知上层应用，收到注册回应超时 */
                /* 调用钩子函数 */
                if (NULL != g_AppCallback && NULL != g_AppCallback->uac_register_response_received_cb)
                {
                    g_AppCallback->uac_register_response_received_cb(usrpos, 0, 408, NULL, 0, g_AppCallback->uac_register_response_received_cb_user_data);
                }

                uac_reginfo_remove(usrpos);
                SIP_DEBUG_TRACE(LOG_INFO, "OnKillNictTransactionForRecvMsg() uac_reginfo_remove:usrpos=%d \r\n", usrpos);
            }
        }
    }
    else if (MSG_IS_NOTIFY(tr->orig_request))
    {
        if (tr->last_response == NULL || MSG_IS_STATUS_1XX(tr->last_response))
        {
            OnRcv408ForNotify(tr, tr->orig_request);
        }
    }
    else if (MSG_IS_SUBSCRIBE(tr->orig_request))
    {
        if (tr->last_response == NULL || MSG_IS_STATUS_1XX(tr->last_response))
        {
            OnRcv408ForSubscribe(tr, tr->orig_request);
        }
    }
    else if (MSG_IS_MESSAGE(tr->orig_request))
    {
        if (tr->last_response == NULL || MSG_IS_STATUS_1XX(tr->last_response))
        {
            if (NULL != g_AppCallback && NULL != g_AppCallback->message_response_received_cb)
            {
                if (NULL != tr && NULL != tr->orig_request && NULL != tr->orig_request->from && NULL != tr->orig_request->from->url && NULL != tr->orig_request->from->url->username)
                {
                    caller_id = osip_getcopy(tr->orig_request->from->url->username);
                }

                if (NULL != tr && NULL != tr->orig_request && NULL != tr->orig_request->to && NULL != tr->orig_request->to->url && NULL != tr->orig_request->to->url->username)
                {
                    callee_id = osip_getcopy(tr->orig_request->to->url->username);
                }

                if (NULL != tr && NULL != tr->orig_request && NULL != tr->orig_request->call_id && NULL != tr->orig_request->call_id->number)
                {
                    call_id = osip_getcopy(tr->orig_request->call_id->number);
                }

                /* 通知上层应用，收到Message 回应超时 */
                /* 调用钩子函数 */
                g_AppCallback->message_response_received_cb(caller_id, callee_id, call_id, 408, g_AppCallback->message_response_received_cb_user_data);

                if (NULL != caller_id)
                {
                    osip_free(caller_id);
                    caller_id = NULL;
                }

                if (NULL != callee_id)
                {
                    osip_free(callee_id);
                    callee_id = NULL;
                }

                if (NULL != call_id)
                {
                    osip_free(call_id);
                    call_id = NULL;
                }
            }
        }
    }
    else if (MSG_IS_INFO(tr->orig_request))
    {
        if (tr->last_response == NULL || MSG_IS_STATUS_1XX(tr->last_response))
        {
            if (NULL != g_AppCallback && NULL != g_AppCallback->info_response_received_cb)
            {
                if (NULL != tr && NULL != tr->orig_request && NULL != tr->orig_request->from && NULL != tr->orig_request->from->url && NULL != tr->orig_request->from->url->username)
                {
                    caller_id = osip_getcopy(tr->orig_request->from->url->username);
                }

                if (NULL != tr && NULL != tr->orig_request && NULL != tr->orig_request->to && NULL != tr->orig_request->to->url && NULL != tr->orig_request->to->url->username)
                {
                    callee_id = osip_getcopy(tr->orig_request->to->url->username);
                }

                if (NULL != tr && NULL != tr->orig_request && NULL != tr->orig_request->call_id && NULL != tr->orig_request->call_id->number)
                {
                    call_id = osip_getcopy(tr->orig_request->call_id->number);
                }

                /* 通知上层应用，收到Info 回应超时 */
                /* 调用钩子函数 */
                g_AppCallback->info_response_received_cb(caller_id, callee_id, call_id, 408, g_AppCallback->info_response_received_cb_user_data);

                if (NULL != caller_id)
                {
                    osip_free(caller_id);
                    caller_id = NULL;
                }

                if (NULL != callee_id)
                {
                    osip_free(callee_id);
                    callee_id = NULL;
                }

                if (NULL != call_id)
                {
                    osip_free(call_id);
                    call_id = NULL;
                }
            }
        }
    }
    else if (MSG_IS_UPDATE(tr->orig_request))
    {
        if (tr->last_response == NULL || MSG_IS_STATUS_1XX(tr->last_response))
        {
            index = find_dialog_as_uac(tr->orig_request);
            //SIP_DEBUG_TRACE(LOG_INFO, "OnKillNictTransaction() find_dialog_as_uac:index=%d \r\n", index);

            if (index >= 0)
            {
                pUaDialog = ua_dialog_get(index);

                if (NULL != pUaDialog)
                {

                }
            }
        }
    }

    //osip_event_t* se = __osip_event_new(KILL_TRANSACTION, tr->transactionid);
    //i = osip_transaction_add_event(tr, se);
    //osip_transaction_free(tr);
    //tr = NULL;

    return 0;
}

/*****************************************************************************
 函 数 名  : OnKillNistTransactionForRecvMsg
 功能描述  : NIST事务删除处理
 输入参数  : osip_transaction_t* tr
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int OnKillNistTransactionForRecvMsg(osip_transaction_t* tr)
{
    if (NULL == tr)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnKillNistTransactionForRecvMsg() exit---: Transaction NULL \r\n");
        return -1;
    }

    throw_2garbage(g_recv_msg_garbage, tr);

    return 0;
}

/*****************************************************************************
 函 数 名  : OnKillIctTransactionForSend
 功能描述  : ICT事务删除处理
 输入参数  : osip_transaction_t* tr
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int OnKillIctTransactionForSend(osip_transaction_t* tr)
{
    int index = 0;
    char* caller_id = NULL;
    char* callee_id = NULL;
    char* call_id = NULL;
    sip_dialog_t* pSipDlg = NULL;
    ui_state_t eUiState = UI_STATE_IDLE;
    ua_dialog_t* pUaDialog = NULL;

    if (NULL == tr)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnKillIctTransactionForSend() exit---: Transaction NULL \r\n");
        return -1;
    }

    throw_2garbage(g_send_garbage, tr);

    ua_timer_remove(UA_CANCEL_TIMEOUT, -1, tr, NULL);

    index = find_dialog_as_uac(tr->orig_request);
    //SIP_DEBUG_TRACE(LOG_INFO, "OnKillIctTransaction() find_dialog_as_uac:index=%d \r\n", index);

    if (is_valid_dialog_index(index))
    {
        pUaDialog = ua_dialog_get(index);

        if (NULL == pUaDialog)
        {
            SIP_DEBUG_TRACE(LOG_DEBUG, "OnKillIctTransactionForSend() exit---: Get UA Dialog Error:dialog_index=%d \r\n", index);
            return -1;
        }

        if (NULL == tr->last_response || MSG_IS_STATUS_1XX(tr->last_response))
        {
            pSipDlg = get_dialog_sip_dialog(index);

            if (NULL == pSipDlg)
            {
                SIP_DEBUG_TRACE(LOG_DEBUG, "OnKillIctTransactionForSend() exit---: Get SIP Dialog Error \r\n");
                return -1;
            }

            /* 通知上层应用，收到Invite 回应超时 */
            /* 调用钩子函数 */
            if (NULL != g_AppCallback && NULL != g_AppCallback->invite_response_received_cb)
            {
                if (NULL != pSipDlg->call_id)
                {
                    call_id = osip_getcopy(pSipDlg->call_id);
                }

                if (NULL != pSipDlg->local_uri && NULL != pSipDlg->local_uri->url && pSipDlg->local_uri->url->username)
                {
                    caller_id = osip_getcopy(pSipDlg->local_uri->url->username);
                }

                if (NULL != pSipDlg->remote_uri && NULL != pSipDlg->remote_uri->url && pSipDlg->remote_uri->url->username)
                {
                    callee_id = osip_getcopy(pSipDlg->remote_uri->url->username);
                }

                g_AppCallback->invite_response_received_cb(caller_id, callee_id, call_id, index, 408, (char*)"Request Timeout", NULL, 0, g_AppCallback->invite_response_received_cb_user_data);

                if (NULL != caller_id)
                {
                    osip_free(caller_id);
                    caller_id = NULL;
                }

                if (NULL != callee_id)
                {
                    osip_free(callee_id);
                    callee_id = NULL;
                }

                if (NULL != call_id)
                {
                    osip_free(call_id);
                    call_id = NULL;
                }
            }

            ua_timer_remove(UA_INVITE_TIMEOUT, index, tr, NULL);
            update_dialog_as_uac(index, NULL, NULL, DLG_EVENT_ERROR);

            eUiState = pUaDialog->eUiState;

            if (eUiState == UI_STATE_CALL_SENT)
            {
                /* as receive 408 */
                pUaDialog->eUiState = UI_STATE_CALL_TERMINATED;
            }
        }
    }

    //osip_event_t* se = __osip_event_new(KILL_TRANSACTION, tr->transactionid);
    //i = osip_transaction_add_event(tr, se);
    //osip_transaction_free(tr);
    //tr = NULL;

    return 0;
}

/*****************************************************************************
 函 数 名  : OnKillIstTransactionForSend
 功能描述  : IST事务删除处理
 输入参数  : osip_transaction_t* tr
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int OnKillIstTransactionForSend(osip_transaction_t* tr)
{
    if (NULL == tr)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnKillIstTransactionForSend() exit---: Transaction NULL \r\n");
        return -1;
    }

    throw_2garbage(g_send_garbage, tr);

    return 0;
}

/*****************************************************************************
 函 数 名  : OnKillNictTransactionForSend
 功能描述  : NICT事务删除处理
 输入参数  : osip_transaction_t* tr
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int OnKillNictTransactionForSend(osip_transaction_t* tr)
{
    int index = -1;
    int usrpos = -1;
    ua_dialog_t* pUaDialog = NULL;
    char* call_id = NULL;
    char* caller_id = NULL;
    char* callee_id = NULL;

    if (NULL == tr)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnKillNictTransactionForSend() exit---: Transaction NULL \r\n");
        return -1;
    }

    throw_2garbage(g_send_garbage, tr);

    if (MSG_IS_REGISTER(tr->orig_request))
    {
        if (tr->last_response == NULL || MSG_IS_STATUS_1XX(tr->last_response))
        {
            if (NULL != tr->orig_request->call_id
                && NULL != tr->orig_request->call_id->number)
            {
                usrpos = uac_reginfo_find(tr->orig_request->call_id->number);
                SIP_DEBUG_TRACE(LOG_DEBUG, "OnKillNictTransactionForSend() uac_reginfo_find:number=%s, usrpos=%d \r\n", tr->orig_request->call_id->number, usrpos);

                if (usrpos < 0)
                {
                    SIP_DEBUG_TRACE(LOG_DEBUG, "OnKillNictTransactionForSend() exit---: Find UAC Register Info Error \r\n");
                    return -1;
                }

                uac_reg_info_t* pUacRegInfo = NULL;
                pUacRegInfo = uac_reginfo_get(usrpos);

                if (NULL != pUacRegInfo)
                {
                    pUacRegInfo->isReg = 0;
                    pUacRegInfo->isReging = 0;
                }

                /* 通知上层应用，收到注册回应超时 */
                /* 调用钩子函数 */
                if (NULL != g_AppCallback && NULL != g_AppCallback->uac_register_response_received_cb)
                {
                    g_AppCallback->uac_register_response_received_cb(usrpos, 0, 408, NULL, 0, g_AppCallback->uac_register_response_received_cb_user_data);
                }

                uac_reginfo_remove(usrpos);
                SIP_DEBUG_TRACE(LOG_INFO, "OnKillNictTransactionForSend() uac_reginfo_remove:usrpos=%d \r\n", usrpos);
            }
        }
    }
    else if (MSG_IS_NOTIFY(tr->orig_request))
    {
        if (tr->last_response == NULL || MSG_IS_STATUS_1XX(tr->last_response))
        {
            OnRcv408ForNotify(tr, tr->orig_request);
        }
    }
    else if (MSG_IS_SUBSCRIBE(tr->orig_request))
    {
        if (tr->last_response == NULL || MSG_IS_STATUS_1XX(tr->last_response))
        {
            OnRcv408ForSubscribe(tr, tr->orig_request);
        }
    }
    else if (MSG_IS_MESSAGE(tr->orig_request))
    {
        if (tr->last_response == NULL || MSG_IS_STATUS_1XX(tr->last_response))
        {
            if (NULL != g_AppCallback && NULL != g_AppCallback->message_response_received_cb)
            {
                if (NULL != tr && NULL != tr->orig_request && NULL != tr->orig_request->from && NULL != tr->orig_request->from->url && NULL != tr->orig_request->from->url->username)
                {
                    caller_id = osip_getcopy(tr->orig_request->from->url->username);
                }

                if (NULL != tr && NULL != tr->orig_request && NULL != tr->orig_request->to && NULL != tr->orig_request->to->url && NULL != tr->orig_request->to->url->username)
                {
                    callee_id = osip_getcopy(tr->orig_request->to->url->username);
                }

                if (NULL != tr && NULL != tr->orig_request && NULL != tr->orig_request->call_id && NULL != tr->orig_request->call_id->number)
                {
                    call_id = osip_getcopy(tr->orig_request->call_id->number);
                }

                /* 通知上层应用，收到Message 回应超时 */
                /* 调用钩子函数 */
                g_AppCallback->message_response_received_cb(caller_id, callee_id, call_id, 408, g_AppCallback->message_response_received_cb_user_data);

                if (NULL != caller_id)
                {
                    osip_free(caller_id);
                    caller_id = NULL;
                }

                if (NULL != callee_id)
                {
                    osip_free(callee_id);
                    callee_id = NULL;
                }

                if (NULL != call_id)
                {
                    osip_free(call_id);
                    call_id = NULL;
                }
            }
        }
    }
    else if (MSG_IS_INFO(tr->orig_request))
    {
        if (tr->last_response == NULL || MSG_IS_STATUS_1XX(tr->last_response))
        {
            if (NULL != g_AppCallback && NULL != g_AppCallback->info_response_received_cb)
            {
                if (NULL != tr && NULL != tr->orig_request && NULL != tr->orig_request->from && NULL != tr->orig_request->from->url && NULL != tr->orig_request->from->url->username)
                {
                    caller_id = osip_getcopy(tr->orig_request->from->url->username);
                }

                if (NULL != tr && NULL != tr->orig_request && NULL != tr->orig_request->to && NULL != tr->orig_request->to->url && NULL != tr->orig_request->to->url->username)
                {
                    callee_id = osip_getcopy(tr->orig_request->to->url->username);
                }

                if (NULL != tr && NULL != tr->orig_request && NULL != tr->orig_request->call_id && NULL != tr->orig_request->call_id->number)
                {
                    call_id = osip_getcopy(tr->orig_request->call_id->number);
                }

                /* 通知上层应用，收到Info 回应超时 */
                /* 调用钩子函数 */
                g_AppCallback->info_response_received_cb(caller_id, callee_id, call_id, 408, g_AppCallback->info_response_received_cb_user_data);

                if (NULL != caller_id)
                {
                    osip_free(caller_id);
                    caller_id = NULL;
                }

                if (NULL != callee_id)
                {
                    osip_free(callee_id);
                    callee_id = NULL;
                }

                if (NULL != call_id)
                {
                    osip_free(call_id);
                    call_id = NULL;
                }
            }
        }
    }
    else if (MSG_IS_UPDATE(tr->orig_request))
    {
        if (tr->last_response == NULL || MSG_IS_STATUS_1XX(tr->last_response))
        {
            index = find_dialog_as_uac(tr->orig_request);
            //SIP_DEBUG_TRACE(LOG_INFO, "OnKillNictTransaction() find_dialog_as_uac:index=%d \r\n", index);

            if (index >= 0)
            {
                pUaDialog = ua_dialog_get(index);

                if (NULL != pUaDialog)
                {

                }
            }
        }
    }

    //osip_event_t* se = __osip_event_new(KILL_TRANSACTION, tr->transactionid);
    //i = osip_transaction_add_event(tr, se);
    //osip_transaction_free(tr);
    //tr = NULL;

    return 0;
}

/*****************************************************************************
 函 数 名  : OnKillNistTransactionForSend
 功能描述  : NIST事务删除处理
 输入参数  : osip_transaction_t* tr
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int OnKillNistTransactionForSend(osip_transaction_t* tr)
{
    if (NULL == tr)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnKillNistTransactionForSend() exit---: Transaction NULL \r\n");
        return -1;
    }

    throw_2garbage(g_send_garbage, tr);

    return 0;
}

/*****************************************************************************
 函 数 名  : OnKillIctTransactionForSendMsg
 功能描述  : ICT事务删除处理
 输入参数  : osip_transaction_t* tr
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int OnKillIctTransactionForSendMsg(osip_transaction_t* tr)
{
    int index = 0;
    char* caller_id = NULL;
    char* callee_id = NULL;
    char* call_id = NULL;
    sip_dialog_t* pSipDlg = NULL;
    ui_state_t eUiState = UI_STATE_IDLE;
    ua_dialog_t* pUaDialog = NULL;

    if (NULL == tr)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnKillIctTransactionForSendMsg() exit---: Transaction NULL \r\n");
        return -1;
    }

    throw_2garbage(g_send_msg_garbage, tr);

    ua_timer_remove(UA_CANCEL_TIMEOUT, -1, tr, NULL);

    index = find_dialog_as_uac(tr->orig_request);
    //SIP_DEBUG_TRACE(LOG_INFO, "OnKillIctTransaction() find_dialog_as_uac:index=%d \r\n", index);

    if (is_valid_dialog_index(index))
    {
        pUaDialog = ua_dialog_get(index);

        if (NULL == pUaDialog)
        {
            SIP_DEBUG_TRACE(LOG_DEBUG, "OnKillIctTransactionForSendMsg() exit---: Get UA Dialog Error:dialog_index=%d \r\n", index);
            return -1;
        }

        if (NULL == tr->last_response || MSG_IS_STATUS_1XX(tr->last_response))
        {
            pSipDlg = get_dialog_sip_dialog(index);

            if (NULL == pSipDlg)
            {
                SIP_DEBUG_TRACE(LOG_DEBUG, "OnKillIctTransactionForSendMsg() exit---: Get SIP Dialog Error \r\n");
                return -1;
            }

            /* 通知上层应用，收到Invite 回应超时 */
            /* 调用钩子函数 */
            if (NULL != g_AppCallback && NULL != g_AppCallback->invite_response_received_cb)
            {
                if (NULL != pSipDlg->call_id)
                {
                    call_id = osip_getcopy(pSipDlg->call_id);
                }

                if (NULL != pSipDlg->local_uri && NULL != pSipDlg->local_uri->url && pSipDlg->local_uri->url->username)
                {
                    caller_id = osip_getcopy(pSipDlg->local_uri->url->username);
                }

                if (NULL != pSipDlg->remote_uri && NULL != pSipDlg->remote_uri->url && pSipDlg->remote_uri->url->username)
                {
                    callee_id = osip_getcopy(pSipDlg->remote_uri->url->username);
                }

                g_AppCallback->invite_response_received_cb(caller_id, callee_id, call_id, index, 408, (char*)"Request Timeout", NULL, 0, g_AppCallback->invite_response_received_cb_user_data);

                if (NULL != caller_id)
                {
                    osip_free(caller_id);
                    caller_id = NULL;
                }

                if (NULL != callee_id)
                {
                    osip_free(callee_id);
                    callee_id = NULL;
                }

                if (NULL != call_id)
                {
                    osip_free(call_id);
                    call_id = NULL;
                }
            }

            ua_timer_remove(UA_INVITE_TIMEOUT, index, tr, NULL);
            update_dialog_as_uac(index, NULL, NULL, DLG_EVENT_ERROR);

            eUiState = pUaDialog->eUiState;

            if (eUiState == UI_STATE_CALL_SENT)
            {
                /* as receive 408 */
                pUaDialog->eUiState = UI_STATE_CALL_TERMINATED;
            }
        }
    }

    //osip_event_t* se = __osip_event_new(KILL_TRANSACTION, tr->transactionid);
    //i = osip_transaction_add_event(tr, se);
    //osip_transaction_free(tr);
    //tr = NULL;

    return 0;
}

/*****************************************************************************
 函 数 名  : OnKillIstTransactionForSendMsg
 功能描述  : IST事务删除处理
 输入参数  : osip_transaction_t* tr
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int OnKillIstTransactionForSendMsg(osip_transaction_t* tr)
{
    if (NULL == tr)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnKillIstTransactionForSendMsg() exit---: Transaction NULL \r\n");
        return -1;
    }

    throw_2garbage(g_send_msg_garbage, tr);

    return 0;
}

/*****************************************************************************
 函 数 名  : OnKillNictTransaction
 功能描述  : NICT事务删除处理
 输入参数  : osip_transaction_t* tr
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int OnKillNictTransactionForSendMsg(osip_transaction_t* tr)
{
    int index = -1;
    int usrpos = -1;
    ua_dialog_t* pUaDialog = NULL;
    char* call_id = NULL;
    char* caller_id = NULL;
    char* callee_id = NULL;

    if (NULL == tr)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnKillNictTransactionForSendMsg() exit---: Transaction NULL \r\n");
        return -1;
    }

    throw_2garbage(g_send_msg_garbage, tr);

    if (MSG_IS_REGISTER(tr->orig_request))
    {
        if (tr->last_response == NULL || MSG_IS_STATUS_1XX(tr->last_response))
        {
            if (NULL != tr->orig_request->call_id
                && NULL != tr->orig_request->call_id->number)
            {
                usrpos = uac_reginfo_find(tr->orig_request->call_id->number);
                SIP_DEBUG_TRACE(LOG_DEBUG, "OnKillNictTransactionForSendMsg() uac_reginfo_find:number=%s, usrpos=%d \r\n", tr->orig_request->call_id->number, usrpos);

                if (usrpos < 0)
                {
                    SIP_DEBUG_TRACE(LOG_DEBUG, "OnKillNictTransactionForSendMsg() exit---: Find UAC Register Info Error \r\n");
                    return -1;
                }

                uac_reg_info_t* pUacRegInfo = NULL;
                pUacRegInfo = uac_reginfo_get(usrpos);

                if (NULL != pUacRegInfo)
                {
                    pUacRegInfo->isReg = 0;
                    pUacRegInfo->isReging = 0;
                }

                /* 通知上层应用，收到注册回应超时 */
                /* 调用钩子函数 */
                if (NULL != g_AppCallback && NULL != g_AppCallback->uac_register_response_received_cb)
                {
                    g_AppCallback->uac_register_response_received_cb(usrpos, 0, 408, NULL, 0, g_AppCallback->uac_register_response_received_cb_user_data);
                }

                uac_reginfo_remove(usrpos);
                SIP_DEBUG_TRACE(LOG_INFO, "OnKillNictTransactionForSendMsg() uac_reginfo_remove:usrpos=%d \r\n", usrpos);
            }
        }
    }
    else if (MSG_IS_NOTIFY(tr->orig_request))
    {
        if (tr->last_response == NULL || MSG_IS_STATUS_1XX(tr->last_response))
        {
            OnRcv408ForNotify(tr, tr->orig_request);
        }
    }
    else if (MSG_IS_SUBSCRIBE(tr->orig_request))
    {
        if (tr->last_response == NULL || MSG_IS_STATUS_1XX(tr->last_response))
        {
            OnRcv408ForSubscribe(tr, tr->orig_request);
        }
    }
    else if (MSG_IS_MESSAGE(tr->orig_request))
    {
        if (tr->last_response == NULL || MSG_IS_STATUS_1XX(tr->last_response))
        {
            if (NULL != g_AppCallback && NULL != g_AppCallback->message_response_received_cb)
            {
                if (NULL != tr && NULL != tr->orig_request && NULL != tr->orig_request->from && NULL != tr->orig_request->from->url && NULL != tr->orig_request->from->url->username)
                {
                    caller_id = osip_getcopy(tr->orig_request->from->url->username);
                }

                if (NULL != tr && NULL != tr->orig_request && NULL != tr->orig_request->to && NULL != tr->orig_request->to->url && NULL != tr->orig_request->to->url->username)
                {
                    callee_id = osip_getcopy(tr->orig_request->to->url->username);
                }

                if (NULL != tr && NULL != tr->orig_request && NULL != tr->orig_request->call_id && NULL != tr->orig_request->call_id->number)
                {
                    call_id = osip_getcopy(tr->orig_request->call_id->number);
                }

                /* 通知上层应用，收到Message 回应超时 */
                /* 调用钩子函数 */
                g_AppCallback->message_response_received_cb(caller_id, callee_id, call_id, 408, g_AppCallback->message_response_received_cb_user_data);

                if (NULL != caller_id)
                {
                    osip_free(caller_id);
                    caller_id = NULL;
                }

                if (NULL != callee_id)
                {
                    osip_free(callee_id);
                    callee_id = NULL;
                }

                if (NULL != call_id)
                {
                    osip_free(call_id);
                    call_id = NULL;
                }
            }
        }
    }
    else if (MSG_IS_INFO(tr->orig_request))
    {
        if (tr->last_response == NULL || MSG_IS_STATUS_1XX(tr->last_response))
        {
            if (NULL != g_AppCallback && NULL != g_AppCallback->info_response_received_cb)
            {
                if (NULL != tr && NULL != tr->orig_request && NULL != tr->orig_request->from && NULL != tr->orig_request->from->url && NULL != tr->orig_request->from->url->username)
                {
                    caller_id = osip_getcopy(tr->orig_request->from->url->username);
                }

                if (NULL != tr && NULL != tr->orig_request && NULL != tr->orig_request->to && NULL != tr->orig_request->to->url && NULL != tr->orig_request->to->url->username)
                {
                    callee_id = osip_getcopy(tr->orig_request->to->url->username);
                }

                if (NULL != tr && NULL != tr->orig_request && NULL != tr->orig_request->call_id && NULL != tr->orig_request->call_id->number)
                {
                    call_id = osip_getcopy(tr->orig_request->call_id->number);
                }

                /* 通知上层应用，收到Info 回应超时 */
                /* 调用钩子函数 */
                g_AppCallback->info_response_received_cb(caller_id, callee_id, call_id, 408, g_AppCallback->info_response_received_cb_user_data);

                if (NULL != caller_id)
                {
                    osip_free(caller_id);
                    caller_id = NULL;
                }

                if (NULL != callee_id)
                {
                    osip_free(callee_id);
                    callee_id = NULL;
                }

                if (NULL != call_id)
                {
                    osip_free(call_id);
                    call_id = NULL;
                }
            }
        }
    }
    else if (MSG_IS_UPDATE(tr->orig_request))
    {
        if (tr->last_response == NULL || MSG_IS_STATUS_1XX(tr->last_response))
        {
            index = find_dialog_as_uac(tr->orig_request);
            //SIP_DEBUG_TRACE(LOG_INFO, "OnKillNictTransaction() find_dialog_as_uac:index=%d \r\n", index);

            if (index >= 0)
            {
                pUaDialog = ua_dialog_get(index);

                if (NULL != pUaDialog)
                {

                }
            }
        }
    }

    //osip_event_t* se = __osip_event_new(KILL_TRANSACTION, tr->transactionid);
    //i = osip_transaction_add_event(tr, se);
    //osip_transaction_free(tr);
    //tr = NULL;

    return 0;
}

/*****************************************************************************
 函 数 名  : OnKillNistTransactionForSendMsg
 功能描述  : NIST事务删除处理
 输入参数  : osip_transaction_t* tr
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int OnKillNistTransactionForSendMsg(osip_transaction_t* tr)
{
    if (NULL == tr)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnKillNistTransactionForSendMsg() exit---: Transaction NULL \r\n");
        return -1;
    }

    throw_2garbage(g_send_msg_garbage, tr);

    return 0;
}

/*****************************************************************************
 函 数 名  : OnTimeoutBye
 功能描述  : BYE消息超时的处理
 输入参数  : int index
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int OnTimeoutBye(int index)
{
    ui_state_t eUiState = UI_STATE_IDLE;

    if (!is_valid_dialog_index(index))
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnTimeoutBye() exit---: Dialog Index Error \r\n");
        return -1;
    }

    eUiState = get_dialog_ui_state(index);

    if (eUiState == UI_STATE_CONNECTED)
    {
        set_dialog_ui_state(index, UI_STATE_IDLE);
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : OnTimeoutCancel
 功能描述  : Cancel消息超时的处理
 输入参数  : int index
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int OnTimeoutCancel(int index)
{
    ui_state_t eUiState = UI_STATE_IDLE;

    if (!is_valid_dialog_index(index))
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "OnTimeoutCancel() exit---: Dialog Index Error \r\n");
        return -1;
    }

    eUiState = get_dialog_ui_state(index);

    if (eUiState == UI_STATE_CALL_SENT)
    {
        set_dialog_ui_state(index, UI_STATE_IDLE);
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : OnSend1xx
 功能描述  : 发送1xx的处理函数
 输入参数  : osip_transaction_t* tr, osip_message_t* sip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int OnSend1xx(osip_transaction_t* tr, osip_message_t* sip)
{
    return 0;
}

/*****************************************************************************
 函 数 名  : OnSend2xx
 功能描述  : 发送2xx的处理函数
 输入参数  : osip_transaction_t* tr, osip_message_t* sip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int OnSend2xx(osip_transaction_t* tr, osip_message_t* sip)
{
    return 0;
}


/*****************************************************************************
 函 数 名  : OnSend3456xx
 功能描述  : 发送3456xx的处理函数
 输入参数  : osip_transaction_t* tr, osip_message_t* sip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int OnSend3456xx(osip_transaction_t* tr, osip_message_t* sip)
{
    return 0;
}

/*****************************************************************************
 函 数 名  : OnSendInvite
 功能描述  : 发送INVITE的处理函数
 输入参数  : osip_transaction_t* tr, osip_message_t* sip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int OnSendInvite(osip_transaction_t* tr, osip_message_t* sip)
{
    return 0;
}

/*****************************************************************************
 函 数 名  : OnSendRequest
 功能描述  : 发送REQUEST的处理函数
 输入参数  : osip_transaction_t* tr, osip_message_t* sip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int OnSendRequest(osip_transaction_t* tr, osip_message_t* sip)
{
    return 0;
}
#endif
