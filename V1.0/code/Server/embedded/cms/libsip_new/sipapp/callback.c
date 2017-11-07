/******************************************************************************

                  版权所有 (C), 2001-2013, 金智视讯技术有限公司

 ******************************************************************************
  文 件 名   : callback.c
  版 本 号   : 初稿
  作    者   : yanghaifeng
  生成日期   : 2013年4月1日
  最近修改   :
  功能描述   : 回调函数设置
  函数列表   :
              app_callback_free
              app_callback_init
              app_set_bye_received_cb
              app_set_bye_response_received_cb
              app_set_info_received_cb
              app_set_info_response_received_cb
              app_set_invite_received_cb
              app_set_invite_response_received_cb
              app_set_message_received_cb
              app_set_message_response_received_cb
              app_set_register_received_cb
              app_set_register_response_received_cb
              cs_cb_ict_2xx2
              cs_cb_ict_rcv1xx
              cs_cb_ict_rcv2xx
              cs_cb_ict_rcv3xx
              cs_cb_ict_rcv456xx
              cs_cb_ist_ack_for2xx
              cs_cb_ist_snd1xx
              cs_cb_ist_snd2xx
              cs_cb_ist_snd3456xx
              cs_cb_kill_ict_transaction
              cs_cb_kill_ist_transaction
              cs_cb_kill_nict_transaction
              cs_cb_kill_nist_transaction
              cs_cb_nict_rcv1xx
              cs_cb_nict_rcv2xx
              cs_cb_nict_rcv3xx
              cs_cb_nict_rcv456xx
              cs_cb_nist_snd1xx
              cs_cb_nist_snd2xx
              cs_cb_nist_snd3456xx
              cs_cb_rcvack
              cs_cb_rcvack2
              cs_cb_rcvbye
              cs_cb_rcvcancel
              cs_cb_rcvinfo
              cs_cb_rcvinvite
              cs_cb_rcvmessage
              cs_cb_rcvnotify
              cs_cb_rcvoptions
              cs_cb_rcvrefer
              cs_cb_rcvregister
              cs_cb_rcvreq_retransmission
              cs_cb_rcvresp_retransmission
              cs_cb_rcvsubscribe
              cs_cb_rcvunkrequest
              cs_cb_sndack
              cs_cb_sndbye
              cs_cb_sndcancel
              cs_cb_sndinfo
              cs_cb_sndinvite
              cs_cb_sndoptions
              cs_cb_sndregister
              cs_cb_sndreq_retransmission
              cs_cb_sndresp_retransmission
              cs_cb_sndunkrequest
              cs_cb_transport_error
              sip_callback_init
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
#else
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#endif

#include <osipparser2/osip_const.h>
#include <osip2/internal.h>
#include <osip2/osip.h>
#include <osip2/osip_fifo.h>
#include <osipparser2/osip_port.h>

#include "gbltype.h"
#include "callback.inc"

#include "sip_event.inc"
#include "sipmsg.inc"
#include "udp_tl.inc"
#include "csdbg.inc"

/*----------------------------------------------*
 * 外部变量说明                                 *
 *----------------------------------------------*/
extern osip_t* g_recv_cell;           /* 接收消息的SIP协议栈,非Message消息专用 */
extern osip_t* g_recv_register_cell;  /* 接收消息的SIP协议栈,Register消息专用 */
extern osip_t* g_recv_message_cell;   /* 接收消息的SIP协议栈,Message消息专用 */
extern osip_t* g_send_cell;           /* 发送消息的SIP协议栈,非Message消息专用 */
extern osip_t* g_send_message_cell;   /* 发送消息的SIP协议栈,Message消息专用 */

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
app_callback_t* g_AppCallback = NULL;

/*----------------------------------------------*
 * 常量定义                                     *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 宏定义                                       *
 *----------------------------------------------*/

#if DECS("协议栈回调函数")
/*****************************************************************************
 函 数 名  : cs_cb_rcvinvite
 功能描述  : 接收INVITE消息回调函数
 输入参数  : transaction_t * tr
                            sip_t * sip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void cs_cb_rcvinvite(int type, osip_transaction_t* tr, osip_message_t* sip)
{
    if (NULL == tr || NULL == sip)
    {
        return;
    }

    //SIP_DEBUG_TRACE(LOG_DEBUG, "cs_cb_rcvinvite() Enter--- \r\n");

    OnRcvInvite(tr, sip);

    return;
}

/*****************************************************************************
 函 数 名  : cs_cb_rcvack
 功能描述  : 接收ACK消息回调函数
 输入参数  : transaction_t * tr
                            sip_t * sip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void cs_cb_rcvack(int type, osip_transaction_t* tr, osip_message_t* sip)
{
    if (NULL == tr || NULL == sip)
    {
        return;
    }

    //SIP_DEBUG_TRACE(LOG_DEBUG, "cs_cb_rcvack() Enter--- \r\n");

    OnRcvAck(tr, sip);

    return;
}

/*****************************************************************************
 函 数 名  : cs_cb_rcvack2
 功能描述  : 接收ACK消息回调函数
 输入参数  : transaction_t * tr
                            sip_t * sip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void cs_cb_rcvack2(int type, osip_transaction_t* tr, osip_message_t* sip)
{
    //SIP_DEBUG_TRACE(LOG_DEBUG, "cs_cb_rcvack2() Enter--- \r\n");

    return;
}

/*****************************************************************************
 函 数 名  : cs_cb_rcvregister
 功能描述  : 接收REGISTER消息回调函数
 输入参数  : transaction_t * tr
                            osip_message_t * sip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void cs_cb_rcvregister(int type, osip_transaction_t* tr, osip_message_t* sip)
{
    if (NULL == tr || NULL == sip)
    {
        return;
    }

    //SIP_DEBUG_TRACE(LOG_DEBUG, "cs_cb_rcvregister() Recv Register Message:From Host=%s \r\n", get_message_from_host(sip));
    //printf("\r\n ########## cs_cb_rcvregister() Recv Register Message:From Host=%s \r\n", get_message_from_host(sip));
    //printf_system_time1();

    OnRcvRegister(tr, sip);

    return;
}

/*****************************************************************************
 函 数 名  : cs_cb_rcvbye
 功能描述  : 接收BYE消息回调函数
 输入参数  : transaction_t * tr
                            sip_t * sip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void cs_cb_rcvbye(int type, osip_transaction_t* tr, osip_message_t* sip)
{
    if (NULL == tr || NULL == sip)
    {
        return;
    }

    //SIP_DEBUG_TRACE(LOG_DEBUG, "cs_cb_rcvbye() Enter--- \r\n");

    OnRcvBye(tr, sip);

    return;
}

/*****************************************************************************
 函 数 名  : cs_cb_rcvoptions
 功能描述  : 接收OPTIONS消息回调函数
 输入参数  : transaction_t * tr
                            sip_t * sip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void cs_cb_rcvoptions(int type, osip_transaction_t* tr, osip_message_t* sip)
{
    if (NULL == tr || NULL == sip)
    {
        return;
    }

    //SIP_DEBUG_TRACE(LOG_DEBUG, "cs_cb_rcvoptions() Enter--- \r\n");

    OnRcvOptions(tr, sip);

    return;
}

/*****************************************************************************
 函 数 名  : cs_cb_rcvinfo
 功能描述  : 接收INFO消息回调函数
 输入参数  : transaction_t * tr
                            sip_t * sip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void cs_cb_rcvinfo(int type, osip_transaction_t* tr, osip_message_t* sip)
{
    if (NULL == tr || NULL == sip)
    {
        return;
    }

    //SIP_DEBUG_TRACE(LOG_DEBUG, "cs_cb_rcvinfo() Enter--- \r\n");

    OnRcvInfo(tr, sip);

    return;
}

/*****************************************************************************
 函 数 名  : cs_cb_rcvupdate
 功能描述  : 接收UPDATE消息回调函数
 输入参数  : transaction_t* tr
                            sip_t* sip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年9月2日 星期一
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void cs_cb_rcvupdate(int type, osip_transaction_t* tr, osip_message_t* sip)
{
    if (NULL == tr || NULL == sip)
    {
        return;
    }

    //SIP_DEBUG_TRACE(LOG_DEBUG, "cs_cb_rcvupdate() Enter--- \r\n");

    OnRcvUpdate(tr, sip);

    return;
}

/*****************************************************************************
 函 数 名  : cs_cb_rcvcancel
 功能描述  : 接收CANCEL消息回调函数
 输入参数  : transaction_t * tr
                            sip_t * sip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void cs_cb_rcvcancel(int type, osip_transaction_t* tr, osip_message_t* sip)
{
    if (NULL == tr || NULL == sip)
    {
        return;
    }

    //SIP_DEBUG_TRACE(LOG_DEBUG, "cs_cb_rcvcancel() Enter--- \r\n");

    OnRcvCancel(tr, sip);

    return;
}

/*****************************************************************************
 函 数 名  : cs_cb_rcvnotify
 功能描述  : 接收NODIFY消息回调函数
 输入参数  : transaction_t * tr
                            sip_t * sip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void cs_cb_rcvnotify(int type, osip_transaction_t* tr, osip_message_t* sip)
{
    if (NULL == tr || NULL == sip)
    {
        return;
    }

    //SIP_DEBUG_TRACE(LOG_DEBUG, "cs_cb_rcvnotify() Enter--- \r\n");

    OnRcvNotify(tr, sip);

    return;
}

/*****************************************************************************
 函 数 名  : cs_cb_rcvsubscribe
 功能描述  : 接收SUBSCRIBE消息回调函数
 输入参数  : transaction_t * tr
                            sip_t * sip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void cs_cb_rcvsubscribe(int type, osip_transaction_t* tr, osip_message_t* sip)
{
    if (NULL == tr || NULL == sip)
    {
        return;
    }

    //SIP_DEBUG_TRACE(LOG_DEBUG, "cs_cb_rcvsubscribe() Enter--- \r\n");

    OnRcvSubscribe(tr, sip);

    return;
}

/*****************************************************************************
 函 数 名  : cs_cb_rcvrefer
 功能描述  : 接收REFER消息回调函数
 输入参数  : transaction_t * tr
                            sip_t * sip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void cs_cb_rcvrefer(int type, osip_transaction_t* tr, osip_message_t* sip)
{
    if (NULL == tr || NULL == sip)
    {
        return;
    }

    //SIP_DEBUG_TRACE(LOG_DEBUG, "cs_cb_rcvrefer() Enter--- \r\n");

    OnRcvRefer(tr, sip);

    return;
}

/*****************************************************************************
 函 数 名  : cs_cb_rcvmessage
 功能描述  : 接收MESSAGE消息回调函数
 输入参数  : transaction_t * tr
                            sip_t * sip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void cs_cb_rcvmessage(int type, osip_transaction_t* tr, osip_message_t* sip)
{
    if (NULL == tr || NULL == sip)
    {
        return;
    }

    //if (NULL != sip->from && NULL != sip->from->url && NULL != sip->to && NULL != sip->to->url)
    //{
    //if ((0 == sstrcmp(sip->from->url->username, (char*)"wiscomCallerID"))
    //&& (0 == sstrcmp(sip->to->url->username, (char*)"wiscomCalleeID"))) /* 查询服务器ID和用户ID */
    //{
    //SIP_DEBUG_TRACE(LOG_DEBUG, "cs_cb_rcvmessage() Recv Get ServerID Message:From Host=%s \r\n", sip->from->url->host);
    //printf("\r\n ********** cs_cb_rcvmessage() Recv Get ServerID Message:From Host=%s \r\n", sip->from->url->host);
    //printf_system_time1();
    //}
    //}

    OnRcvMessage(tr, sip);

    return;
}

/*****************************************************************************
 函 数 名  : cs_cb_rcvunkrequest
 功能描述  : 接收其他未知类型消息回调函数
 输入参数  : transaction_t * tr
                            sip_t * sip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void cs_cb_rcvunkrequest(int type, osip_transaction_t* tr, osip_message_t* sip)
{
    if (NULL == tr || NULL == sip)
    {
        return;
    }

    //SIP_DEBUG_TRACE(LOG_DEBUG, "cs_cb_rcvunkrequest() Enter--- \r\n");

    OnRcvUnknown(tr, sip);

    return;
}

/*****************************************************************************
 函 数 名  : cs_cb_ict_rcv1xx
 功能描述  : ICT事务接收1XX回应消息回调函数
 输入参数  : transaction_t * tr
                            sip_t * sip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void cs_cb_ict_rcv1xx(int type, osip_transaction_t* tr , osip_message_t* sip)
{
    if (NULL == tr || NULL == sip)
    {
        return;
    }

    //SIP_DEBUG_TRACE(LOG_DEBUG, "cs_cb_ict_rcv1xx() Enter--- \r\n");

    OnRcv1xxForInvite(tr, sip);

    return;
}

/*****************************************************************************
 函 数 名  : cs_cb_ict_rcv2xx
 功能描述  : ICT事务接收2XX回应消息回调函数
 输入参数  : transaction_t * tr
                            sip_t * sip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void cs_cb_ict_rcv2xx(int type, osip_transaction_t* tr, osip_message_t* sip)
{
    if (NULL == tr || NULL == sip)
    {
        return;
    }

    //SIP_DEBUG_TRACE(LOG_DEBUG, "cs_cb_ict_rcv2xx() Enter--- \r\n");

    OnRcv2xxForInvite(tr, sip);

    return;
}

/*****************************************************************************
 函 数 名  : cs_cb_ict_rcv3xx
 功能描述  : ICT事务接收3XX回应消息回调函数
 输入参数  : transaction_t * tr
                            sip_t * sip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void cs_cb_ict_rcv3xx(int type, osip_transaction_t* tr, osip_message_t* sip)
{
    if (NULL == tr || NULL == sip)
    {
        return;
    }

    //SIP_DEBUG_TRACE(LOG_DEBUG, "cs_cb_ict_rcv3xx() Enter--- \r\n");

    OnRcv3xxForInvite(tr, sip);

    return;
}

/*****************************************************************************
 函 数 名  : cs_cb_ict_rcv456xx
 功能描述  : ICT事务接收456XX回应消息回调函数
 输入参数  : transaction_t * tr
                            sip_t * sip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void cs_cb_ict_rcv456xx(int type, osip_transaction_t* tr, osip_message_t* sip)
{
    if (NULL == tr || NULL == sip)
    {
        return;
    }

    //SIP_DEBUG_TRACE(LOG_DEBUG, "cs_cb_ict_rcv456xx() Enter--- \r\n");

    OnRcv456xxForInvite(tr, sip);

    return;
}

/*****************************************************************************
 函 数 名  : cs_cb_nict_rcv1xx
 功能描述  : NICT事务接收1XX回应消息回调函数
 输入参数  : transaction_t * tr
                            sip_t * sip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void cs_cb_nict_rcv1xx(int type, osip_transaction_t* tr, osip_message_t* sip)
{
    if (NULL == tr || NULL == sip)
    {
        return;
    }

    //SIP_DEBUG_TRACE(LOG_DEBUG, "cs_cb_nict_rcv1xx() Enter--- \r\n");

    OnRcv1xxForRequest(tr, sip);

    return;
}

/*****************************************************************************
 函 数 名  : cs_cb_nict_rcv2xx
 功能描述  : NICT事务接收2XX回应消息回调函数
 输入参数  : transaction_t * tr
                            sip_t * sip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void cs_cb_nict_rcv2xx(int type, osip_transaction_t* tr, osip_message_t* sip)
{
    if (NULL == tr || NULL == sip)
    {
        return;
    }

    //SIP_DEBUG_TRACE(LOG_DEBUG, "cs_cb_nict_rcv2xx() Enter--- \r\n");

    if (MSG_IS_RESPONSE_FOR(sip, REGISTER_METHOD))
    {
        OnRcv2xxForRegister(tr, sip);
    }
    else if (MSG_IS_RESPONSE_FOR(sip, CANCEL_METHOD))
    {
        OnRcv2xxForCancel(tr, sip);
    }
    else if (MSG_IS_RESPONSE_FOR(sip, BYE_METHOD))
    {
        OnRcv2xxForBye(tr, sip);
    }
    else if (MSG_IS_RESPONSE_FOR(sip, UPDATE_METHOD))
    {
        OnRcv2xxForUpdate(tr, sip);
    }
    else if (MSG_IS_RESPONSE_FOR(sip, INFO_METHOD))
    {
        OnRcv2xxForInfo(tr, sip);
    }
    else if (MSG_IS_RESPONSE_FOR(sip, OPTIONS_METHOD))
    {
        OnRcv2xxForOptions(tr, sip);
    }
    else if (MSG_IS_RESPONSE_FOR(sip, SUBSCRIBE_METHOD))
    {
        OnRcv2xxForSubscribe(tr, sip);
    }
    else if (MSG_IS_RESPONSE_FOR(sip, NOTIFY_METHOD))
    {
        OnRcv2xxForNotify(tr, sip);
    }
    else if (MSG_IS_RESPONSE_FOR(sip, REFER_METHOD))
    {
        OnRcv2xxForRefer(tr, sip);
    }
    else if (MSG_IS_RESPONSE_FOR(sip, MESSAGE_METHOD))
    {
        OnRcv2xxForMessage(tr, sip);
    }

    return;
}

/*****************************************************************************
 函 数 名  : cs_cb_nict_rcv3xx
 功能描述  : NICT事务接收3XX回应消息回调函数
 输入参数  : transaction_t * tr
                            sip_t * sip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void cs_cb_nict_rcv3xx(int type, osip_transaction_t* tr, osip_message_t* sip)
{
    if (NULL == tr || NULL == sip)
    {
        return;
    }

    //SIP_DEBUG_TRACE(LOG_DEBUG, "cs_cb_nict_rcv3xx() Enter--- \r\n");

    if (MSG_IS_RESPONSE_FOR(sip, REGISTER_METHOD))
    {
        OnRcv3xxForRegister(tr, sip);
    }
    else if (MSG_IS_RESPONSE_FOR(sip, CANCEL_METHOD))
    {
        OnRcv3xxForCancel(tr, sip);
    }
    else if (MSG_IS_RESPONSE_FOR(sip, BYE_METHOD))
    {
        OnRcv3xxForBye(tr, sip);
    }
    else if (MSG_IS_RESPONSE_FOR(sip, UPDATE_METHOD))
    {
        OnRcv3xxForUpdate(tr, sip);
    }
    else if (MSG_IS_RESPONSE_FOR(sip, INFO_METHOD))
    {
        OnRcv3xxForInfo(tr, sip);
    }
    else if (MSG_IS_RESPONSE_FOR(sip, OPTIONS_METHOD))
    {
        OnRcv3xxForOptions(tr, sip);
    }
    else if (MSG_IS_RESPONSE_FOR(sip, SUBSCRIBE_METHOD))
    {
        OnRcv3xxForSubscribe(tr, sip);
    }
    else if (MSG_IS_RESPONSE_FOR(sip, NOTIFY_METHOD))
    {
        OnRcv3xxForNotify(tr, sip);
    }
    else if (MSG_IS_RESPONSE_FOR(sip, REFER_METHOD))
    {
        OnRcv3xxForRefer(tr, sip);
    }

    return;
}

/*****************************************************************************
 函 数 名  : cs_cb_nict_rcv456xx
 功能描述  : NICT事务接收456XX回应消息回调函数
 输入参数  : transaction_t * tr
                            sip_t * sip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void cs_cb_nict_rcv456xx(int type, osip_transaction_t* tr, osip_message_t* sip)
{
    if (NULL == tr || NULL == sip)
    {
        return;
    }

    //SIP_DEBUG_TRACE(LOG_DEBUG, "cs_cb_nict_rcv456xx() Enter--- \r\n");

    if (MSG_IS_RESPONSE_FOR(sip, REGISTER_METHOD))
    {
        OnRcv456xxForRegister(tr, sip);
    }
    else if (MSG_IS_RESPONSE_FOR(sip, CANCEL_METHOD))
    {
        OnRcv456xxForCancel(tr, sip);
    }
    else if (MSG_IS_RESPONSE_FOR(sip, BYE_METHOD))
    {
        OnRcv456xxForBye(tr, sip);
    }
    else if (MSG_IS_RESPONSE_FOR(sip, UPDATE_METHOD))
    {
        OnRcv456xxForUpdate(tr, sip);
    }
    else if (MSG_IS_RESPONSE_FOR(sip, INFO_METHOD))
    {
        OnRcv456xxForInfo(tr, sip);
    }
    else if (MSG_IS_RESPONSE_FOR(sip, OPTIONS_METHOD))
    {
        OnRcv456xxForOptions(tr, sip);
    }
    else if (MSG_IS_RESPONSE_FOR(sip, SUBSCRIBE_METHOD))
    {
        OnRcv456xxForSubscribe(tr, sip);
    }
    else if (MSG_IS_RESPONSE_FOR(sip, NOTIFY_METHOD))
    {
        OnRcv456xxForNotify(tr, sip);
    }
    else if (MSG_IS_RESPONSE_FOR(sip, REFER_METHOD))
    {
        OnRcv456xxForRefer(tr, sip);
    }
    else if (MSG_IS_RESPONSE_FOR(sip, MESSAGE_METHOD))
    {
        OnRcv456xxForMessage(tr, sip);
    }

    return;
}

/*****************************************************************************
 函 数 名  : cs_cb_ist_ack_for2xx
 功能描述  : IST事务接收ACK for 2xx回应消息回调函数
 输入参数  : sip_t * sip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void cs_cb_ist_ack_for2xx(osip_message_t* sip)
{
    if (NULL == sip)
    {
        return;
    }

    //SIP_DEBUG_TRACE(LOG_DEBUG, "cs_cb_ist_ack_for2xx() Enter--- \r\n");

    OnRcvIstAckfor2xx(sip);

    return;
}

/*****************************************************************************
 函 数 名  : cs_cb_ict_2xx2
 功能描述  : ICT事务接收2xx重发消息回调函数
 输入参数  : sip_t * sip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void cs_cb_ict_2xx2(osip_message_t* sip)
{
    if (NULL == sip)
    {
        return;
    }

    //SIP_DEBUG_TRACE(LOG_DEBUG, "cs_cb_ict_2xx2() Enter--- \r\n");

    OnRcvIct2xx2(sip);

    return;
}

/*****************************************************************************
 函 数 名  : cs_cb_ist_snd1xx
 功能描述  : IST事务发送1xx消息回调函数
 输入参数  : transaction_t * tr
                            sip_t * sip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void cs_cb_ist_snd1xx(int type, osip_transaction_t* tr, osip_message_t* sip)
{
    return;
}

/*****************************************************************************
 函 数 名  : cs_cb_ist_snd2xx
 功能描述  : IST事务发送2xx消息回调函数
 输入参数  : transaction_t * tr
                            sip_t * sip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void cs_cb_ist_snd2xx(int type, osip_transaction_t* tr, osip_message_t* sip)
{
    return;
}

/*****************************************************************************
 函 数 名  : cs_cb_ist_snd3456xx
 功能描述  : IST事务发送3456xx消息回调函数
 输入参数  : transaction_t * tr
                            sip_t * sip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void cs_cb_ist_snd3456xx(int type, osip_transaction_t* tr, osip_message_t* sip)
{
    return;
}

/*****************************************************************************
 函 数 名  : cs_cb_nist_snd1xx
 功能描述  : NIST事务发送1xx消息回调函数
 输入参数  : transaction_t * tr
                            sip_t * sip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void cs_cb_nist_snd1xx(int type, osip_transaction_t* tr, osip_message_t* sip)
{
    return;
}

/*****************************************************************************
 函 数 名  : cs_cb_nist_snd2xx
 功能描述  : NIST事务发送2xx消息回调函数
 输入参数  : transaction_t * tr
                            sip_t * sip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void cs_cb_nist_snd2xx(int type, osip_transaction_t* tr, osip_message_t* sip)
{
    return;
}

/*****************************************************************************
 函 数 名  : cs_cb_nist_snd3456xx
 功能描述  : NIST事务发送3456xx消息回调函数
 输入参数  : transaction_t * tr
                            sip_t * sip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void cs_cb_nist_snd3456xx(int type, osip_transaction_t* tr, osip_message_t* sip)
{
    return;
}

/*****************************************************************************
 函 数 名  : cs_cb_kill_ict_transaction_for_rcv
 功能描述  :  删除ICT事务回调函数
 输入参数  : transaction_t * tr
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void cs_cb_kill_ict_transaction_for_rcv(int type, osip_transaction_t* tr)
{
    if (NULL == tr)
    {
        return;
    }

    osip_remove_transaction(g_recv_cell, tr);

    //SIP_DEBUG_TRACE(LOG_DEBUG, "cs_cb_kill_ict_transaction() Enter--- \r\n");

    OnKillIctTransactionForRecv(tr);

    return;
}

/*****************************************************************************
 函 数 名  : cs_cb_kill_ist_transaction_for_rcv
 功能描述  : 删除IST事务回调函数
 输入参数  : transaction_t * tr
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void cs_cb_kill_ist_transaction_for_rcv(int type, osip_transaction_t* tr)
{
    if (NULL == tr)
    {
        return;
    }

    osip_remove_transaction(g_recv_cell, tr);

    //SIP_DEBUG_TRACE(LOG_DEBUG, "cs_cb_kill_ist_transaction() Enter--- \r\n");

    OnKillIstTransactionForRecv(tr);

    return;
}

/*****************************************************************************
 函 数 名  : cs_cb_kill_nict_transaction_for_rcv
 功能描述  : 删除NICT事务回调函数
 输入参数  : transaction_t * tr
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void cs_cb_kill_nict_transaction_for_rcv(int type, osip_transaction_t* tr)
{
    if (NULL == tr)
    {
        return;
    }

    osip_remove_transaction(g_recv_cell, tr);

    //SIP_DEBUG_TRACE(LOG_DEBUG, "cs_cb_kill_nict_transaction() Enter--- \r\n");

    OnKillNictTransactionForRecv(tr);

    return;
}

/*****************************************************************************
 函 数 名  : cs_cb_kill_nist_transaction_for_rcv
 功能描述  : 删除NIST事务回调函数
 输入参数  : transaction_t * tr
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void cs_cb_kill_nist_transaction_for_rcv(int type, osip_transaction_t* tr)
{
    if (NULL == tr)
    {
        return;
    }

    osip_remove_transaction(g_recv_cell, tr);

    //SIP_DEBUG_TRACE(LOG_DEBUG, "cs_cb_kill_nist_transaction() Enter--- \r\n");

    OnKillNistTransactionForRecv(tr);

    return;
}

/*****************************************************************************
 函 数 名  : cs_cb_kill_ict_transaction_for_rcv_register
 功能描述  :  删除ICT事务回调函数
 输入参数  : transaction_t * tr
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void cs_cb_kill_ict_transaction_for_rcv_register(int type, osip_transaction_t* tr)
{
    if (NULL == tr)
    {
        return;
    }

    osip_remove_transaction(g_recv_register_cell, tr);

    //SIP_DEBUG_TRACE(LOG_DEBUG, "cs_cb_kill_ict_transaction() Enter--- \r\n");

    OnKillIctTransactionForRecvRegister(tr);

    return;
}

/*****************************************************************************
 函 数 名  : cs_cb_kill_ist_transaction_for_rcv_register
 功能描述  : 删除IST事务回调函数
 输入参数  : transaction_t * tr
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void cs_cb_kill_ist_transaction_for_rcv_register(int type, osip_transaction_t* tr)
{
    if (NULL == tr)
    {
        return;
    }

    osip_remove_transaction(g_recv_register_cell, tr);

    //SIP_DEBUG_TRACE(LOG_DEBUG, "cs_cb_kill_ist_transaction() Enter--- \r\n");

    OnKillIstTransactionForRecvRegister(tr);

    return;
}

/*****************************************************************************
 函 数 名  : cs_cb_kill_nict_transaction_for_rcv_register
 功能描述  : 删除NICT事务回调函数
 输入参数  : transaction_t * tr
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void cs_cb_kill_nict_transaction_for_rcv_register(int type, osip_transaction_t* tr)
{
    if (NULL == tr)
    {
        return;
    }

    osip_remove_transaction(g_recv_register_cell, tr);

    //SIP_DEBUG_TRACE(LOG_DEBUG, "cs_cb_kill_nict_transaction() Enter--- \r\n");

    OnKillNictTransactionForRecvRegister(tr);

    return;
}

/*****************************************************************************
 函 数 名  : cs_cb_kill_nist_transaction_for_rcv_register
 功能描述  : 删除NIST事务回调函数
 输入参数  : transaction_t * tr
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void cs_cb_kill_nist_transaction_for_rcv_register(int type, osip_transaction_t* tr)
{
    if (NULL == tr)
    {
        return;
    }

    osip_remove_transaction(g_recv_register_cell, tr);

    //SIP_DEBUG_TRACE(LOG_DEBUG, "cs_cb_kill_nist_transaction() Enter--- \r\n");

    OnKillNistTransactionForRecvRegister(tr);

    return;
}

/*****************************************************************************
 函 数 名  : cs_cb_kill_ict_transaction_for_rcv_msg
 功能描述  :  删除ICT事务回调函数
 输入参数  : transaction_t * tr
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void cs_cb_kill_ict_transaction_for_rcv_msg(int type, osip_transaction_t* tr)
{
    if (NULL == tr)
    {
        return;
    }

    osip_remove_transaction(g_recv_message_cell, tr);

    //SIP_DEBUG_TRACE(LOG_DEBUG, "cs_cb_kill_ict_transaction() Enter--- \r\n");

    OnKillIctTransactionForRecvMsg(tr);

    return;
}

/*****************************************************************************
 函 数 名  : cs_cb_kill_ist_transaction_for_rcv_msg
 功能描述  : 删除IST事务回调函数
 输入参数  : transaction_t * tr
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void cs_cb_kill_ist_transaction_for_rcv_msg(int type, osip_transaction_t* tr)
{
    if (NULL == tr)
    {
        return;
    }

    osip_remove_transaction(g_recv_message_cell, tr);

    //SIP_DEBUG_TRACE(LOG_DEBUG, "cs_cb_kill_ist_transaction() Enter--- \r\n");

    OnKillIstTransactionForRecvMsg(tr);

    return;
}

/*****************************************************************************
 函 数 名  : cs_cb_kill_nict_transaction_for_rcv_msg
 功能描述  : 删除NICT事务回调函数
 输入参数  : transaction_t * tr
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void cs_cb_kill_nict_transaction_for_rcv_msg(int type, osip_transaction_t* tr)
{
    if (NULL == tr)
    {
        return;
    }

    osip_remove_transaction(g_recv_message_cell, tr);

    //SIP_DEBUG_TRACE(LOG_DEBUG, "cs_cb_kill_nict_transaction() Enter--- \r\n");

    OnKillNictTransactionForRecvMsg(tr);

    return;
}

/*****************************************************************************
 函 数 名  : cs_cb_kill_nist_transaction_for_rcv_msg
 功能描述  : 删除NIST事务回调函数
 输入参数  : transaction_t * tr
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void cs_cb_kill_nist_transaction_for_rcv_msg(int type, osip_transaction_t* tr)
{
    if (NULL == tr)
    {
        return;
    }

    osip_remove_transaction(g_recv_message_cell, tr);

    //SIP_DEBUG_TRACE(LOG_DEBUG, "cs_cb_kill_nist_transaction() Enter--- \r\n");

    OnKillNistTransactionForRecvMsg(tr);

    return;
}

/*****************************************************************************
 函 数 名  : cs_cb_kill_ict_transaction_for_send
 功能描述  :  删除ICT事务回调函数
 输入参数  : transaction_t * tr
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void cs_cb_kill_ict_transaction_for_send(int type, osip_transaction_t* tr)
{
    if (NULL == tr)
    {
        return;
    }

    osip_remove_transaction(g_send_cell, tr);

    //SIP_DEBUG_TRACE(LOG_DEBUG, "cs_cb_kill_ict_transaction() Enter--- \r\n");

    OnKillIctTransactionForSend(tr);

    return;
}

/*****************************************************************************
 函 数 名  : cs_cb_kill_ist_transaction_for_send
 功能描述  : 删除IST事务回调函数
 输入参数  : transaction_t * tr
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void cs_cb_kill_ist_transaction_for_send(int type, osip_transaction_t* tr)
{
    if (NULL == tr)
    {
        return;
    }

    osip_remove_transaction(g_send_cell, tr);

    //SIP_DEBUG_TRACE(LOG_DEBUG, "cs_cb_kill_ist_transaction() Enter--- \r\n");

    OnKillIstTransactionForSend(tr);

    return;
}

/*****************************************************************************
 函 数 名  : cs_cb_kill_nict_transaction_for_send
 功能描述  : 删除NICT事务回调函数
 输入参数  : transaction_t * tr
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void cs_cb_kill_nict_transaction_for_send(int type, osip_transaction_t* tr)
{
    if (NULL == tr)
    {
        return;
    }

    osip_remove_transaction(g_send_cell, tr);

    //SIP_DEBUG_TRACE(LOG_DEBUG, "cs_cb_kill_nict_transaction() Enter--- \r\n");

    OnKillNictTransactionForSend(tr);

    return;
}

/*****************************************************************************
 函 数 名  : cs_cb_kill_nist_transaction_for_send
 功能描述  : 删除NIST事务回调函数
 输入参数  : transaction_t * tr
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void cs_cb_kill_nist_transaction_for_send(int type, osip_transaction_t* tr)
{
    if (NULL == tr)
    {
        return;
    }

    osip_remove_transaction(g_send_cell, tr);

    //SIP_DEBUG_TRACE(LOG_DEBUG, "cs_cb_kill_nist_transaction() Enter--- \r\n");

    OnKillNistTransactionForSend(tr);

    return;
}

/*****************************************************************************
 函 数 名  : cs_cb_kill_ict_transaction_for_send_msg
 功能描述  :  删除ICT事务回调函数
 输入参数  : transaction_t * tr
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void cs_cb_kill_ict_transaction_for_send_msg(int type, osip_transaction_t* tr)
{
    if (NULL == tr)
    {
        return;
    }

    osip_remove_transaction(g_send_message_cell, tr);

    //SIP_DEBUG_TRACE(LOG_DEBUG, "cs_cb_kill_ict_transaction() Enter--- \r\n");

    OnKillIctTransactionForSendMsg(tr);

    return;
}

/*****************************************************************************
 函 数 名  : cs_cb_kill_ist_transaction_for_send_msg
 功能描述  : 删除IST事务回调函数
 输入参数  : transaction_t * tr
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void cs_cb_kill_ist_transaction_for_send_msg(int type, osip_transaction_t* tr)
{
    if (NULL == tr)
    {
        return;
    }

    osip_remove_transaction(g_send_message_cell, tr);

    //SIP_DEBUG_TRACE(LOG_DEBUG, "cs_cb_kill_ist_transaction() Enter--- \r\n");

    OnKillIstTransactionForSendMsg(tr);

    return;
}

/*****************************************************************************
 函 数 名  : cs_cb_kill_nict_transaction_for_send_msg
 功能描述  : 删除NICT事务回调函数
 输入参数  : transaction_t * tr
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void cs_cb_kill_nict_transaction_for_send_msg(int type, osip_transaction_t* tr)
{
    if (NULL == tr)
    {
        return;
    }

    osip_remove_transaction(g_send_message_cell, tr);

    //SIP_DEBUG_TRACE(LOG_DEBUG, "cs_cb_kill_nict_transaction() Enter--- \r\n");

    OnKillNictTransactionForSendMsg(tr);

    return;
}

/*****************************************************************************
 函 数 名  : cs_cb_kill_nist_transaction_for_send_msg
 功能描述  : 删除NIST事务回调函数
 输入参数  : transaction_t * tr
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void cs_cb_kill_nist_transaction_for_send_msg(int type, osip_transaction_t* tr)
{
    if (NULL == tr)
    {
        return;
    }

    osip_remove_transaction(g_send_message_cell, tr);

    //SIP_DEBUG_TRACE(LOG_DEBUG, "cs_cb_kill_nist_transaction() Enter--- \r\n");

    OnKillNistTransactionForSendMsg(tr);

    return;
}

/*****************************************************************************
 函 数 名  : cs_cb_rcvresp_retransmission
 功能描述  : 收到回应消息重传回调函数
 输入参数  : transaction_t * tr
                            sip_t * sip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void cs_cb_rcvresp_retransmission(int type, osip_transaction_t* tr, osip_message_t* sip)
{
    return;
}

/*****************************************************************************
 函 数 名  : cs_cb_rcvreq_retransmission
 功能描述  : 收到请求消息重传回调函数
 输入参数  : transaction_t * tr
                            sip_t * sip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void cs_cb_rcvreq_retransmission(int type, osip_transaction_t* tr, osip_message_t* sip)
{
    return;
}

/*****************************************************************************
 函 数 名  : cs_cb_sndresp_retransmission
 功能描述  : 发送回应消息重传回调函数
 输入参数  : transaction_t * tr
                            sip_t * sip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void cs_cb_sndresp_retransmission(int type, osip_transaction_t* tr, osip_message_t* sip)
{
    return;
}

/*****************************************************************************
 函 数 名  : cs_cb_sndreq_retransmission
 功能描述  : 发送请求消息重传回调函数
 输入参数  : transaction_t * tr
                            sip_t * sip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void cs_cb_sndreq_retransmission(int type, osip_transaction_t* tr, osip_message_t* sip)
{
    return;
}

/*****************************************************************************
 函 数 名  : cs_cb_transport_error
 功能描述  : 传输错误回调函数
 输入参数  : transaction_t * tr
                            sip_t * sip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void cs_cb_transport_error(int type, osip_transaction_t* tr, int error)
{
    return;
}

/*****************************************************************************
 函 数 名  : cs_cb_sndinvite
 功能描述  : 发送INVITE回调函数
 输入参数  : transaction_t * tr
                            sip_t * sip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void cs_cb_sndinvite(int type, osip_transaction_t* tr, osip_message_t* sip)
{
    return;
}

/*****************************************************************************
 函 数 名  : cs_cb_sndack
 功能描述  : 发送ACK回调函数
 输入参数  : transaction_t * tr
                            sip_t * sip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void cs_cb_sndack(int type, osip_transaction_t* tr, osip_message_t* sip)
{
    return;
}

/*****************************************************************************
 函 数 名  : cs_cb_sndregister
 功能描述  : 发送REGISTER回调函数
 输入参数  : transaction_t * tr
                            sip_t * sip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void cs_cb_sndregister(int type, osip_transaction_t* tr, osip_message_t* sip)
{
    return;
}

/*****************************************************************************
 函 数 名  : cs_cb_sndbye
 功能描述  : 发送BYE回调函数
 输入参数  : transaction_t * tr
                            sip_t * sip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void cs_cb_sndbye(int type, osip_transaction_t* tr, osip_message_t* sip)
{
    return;
}

/*****************************************************************************
 函 数 名  : cs_cb_sndcancel
 功能描述  : 发送CANCEL回调函数
 输入参数  : transaction_t * tr
                            sip_t * sip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void cs_cb_sndcancel(int type, osip_transaction_t* tr, osip_message_t* sip)
{
    return;
}

/*****************************************************************************
 函 数 名  : cs_cb_sndinfo
 功能描述  : 发送INFO回调函数
 输入参数  : transaction_t * tr
                            sip_t * sip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void cs_cb_sndinfo(int type, osip_transaction_t* tr, osip_message_t* sip)
{
    return;
}

/*****************************************************************************
 函 数 名  : cs_cb_sndoptions
 功能描述  : 发送OPTIONS回调函数
 输入参数  : transaction_t * tr
                            sip_t * sip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void cs_cb_sndoptions(int type, osip_transaction_t* tr, osip_message_t* sip)
{
    return;
}

/*****************************************************************************
 函 数 名  : cs_cb_sndunkrequest
 功能描述  : 发送其他未知请求消息回调函数
 输入参数  : transaction_t * tr
                            sip_t * sip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void cs_cb_sndunkrequest(int type, osip_transaction_t* tr, osip_message_t* sip)
{
    return;
}

void cs_cb_ict_timeout(int type, osip_transaction_t* tr, osip_message_t* sip)
{
    return;
}

void cs_cb_nict_timeout(int type, osip_transaction_t* tr, osip_message_t* sip)
{
    return;
}
/*****************************************************************************
 函 数 名  : sip_callback_init
 功能描述  : SIP协议栈回调函数设置
 输入参数  : cell_t * cell
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int sip_callback_init(osip_t* cell)
{
    /*int i;*/
    if (cell == NULL)
    {
        return -1;
    }

    osip_set_cb_send_message(cell, &send_message_using_udp);

    osip_set_transport_error_callback(cell, OSIP_ICT_TRANSPORT_ERROR, &cs_cb_transport_error);
    osip_set_transport_error_callback(cell, OSIP_IST_TRANSPORT_ERROR, &cs_cb_transport_error);
    osip_set_transport_error_callback(cell, OSIP_NICT_TRANSPORT_ERROR, &cs_cb_transport_error);
    osip_set_transport_error_callback(cell, OSIP_NIST_TRANSPORT_ERROR, &cs_cb_transport_error);

    osip_set_message_callback(cell, OSIP_ICT_INVITE_SENT, &cs_cb_sndinvite);
    osip_set_message_callback(cell, OSIP_ICT_INVITE_SENT_AGAIN, &cs_cb_sndreq_retransmission);
    osip_set_message_callback(cell, OSIP_ICT_ACK_SENT, &cs_cb_sndack);
    osip_set_message_callback(cell, OSIP_ICT_ACK_SENT_AGAIN, &cs_cb_sndreq_retransmission);

    osip_set_message_callback(cell, OSIP_ICT_STATUS_1XX_RECEIVED, &cs_cb_ict_rcv1xx);
    osip_set_message_callback(cell, OSIP_ICT_STATUS_2XX_RECEIVED, &cs_cb_ict_rcv2xx);
    osip_set_message_callback(cell, OSIP_ICT_STATUS_2XX_RECEIVED_AGAIN, &cs_cb_rcvresp_retransmission);
    osip_set_message_callback(cell, OSIP_ICT_STATUS_3XX_RECEIVED, &cs_cb_ict_rcv3xx);
    osip_set_message_callback(cell, OSIP_ICT_STATUS_4XX_RECEIVED, &cs_cb_ict_rcv456xx);
    osip_set_message_callback(cell, OSIP_ICT_STATUS_5XX_RECEIVED, &cs_cb_ict_rcv456xx);
    osip_set_message_callback(cell, OSIP_ICT_STATUS_6XX_RECEIVED, &cs_cb_ict_rcv456xx);
    osip_set_message_callback(cell, OSIP_ICT_STATUS_3456XX_RECEIVED_AGAIN, &cs_cb_rcvresp_retransmission);

    osip_set_message_callback(cell, OSIP_IST_INVITE_RECEIVED, &cs_cb_rcvinvite);
    osip_set_message_callback(cell, OSIP_IST_INVITE_RECEIVED_AGAIN, &cs_cb_rcvreq_retransmission);
    osip_set_message_callback(cell, OSIP_IST_ACK_RECEIVED, &cs_cb_rcvack);
    osip_set_message_callback(cell, OSIP_IST_ACK_RECEIVED_AGAIN, &cs_cb_rcvreq_retransmission);

    osip_set_message_callback(cell, OSIP_IST_STATUS_1XX_SENT, &cs_cb_ist_snd1xx);
    osip_set_message_callback(cell, OSIP_IST_STATUS_2XX_SENT, &cs_cb_ist_snd2xx);
    osip_set_message_callback(cell, OSIP_IST_STATUS_2XX_SENT_AGAIN, &cs_cb_sndresp_retransmission);
    osip_set_message_callback(cell, OSIP_IST_STATUS_3XX_SENT, &cs_cb_ist_snd3456xx);
    osip_set_message_callback(cell, OSIP_IST_STATUS_4XX_SENT, &cs_cb_ist_snd3456xx);
    osip_set_message_callback(cell, OSIP_IST_STATUS_5XX_SENT, &cs_cb_ist_snd3456xx);
    osip_set_message_callback(cell, OSIP_IST_STATUS_6XX_SENT, &cs_cb_ist_snd3456xx);
    osip_set_message_callback(cell, OSIP_IST_STATUS_3456XX_SENT_AGAIN, &cs_cb_sndresp_retransmission);

    osip_set_message_callback(cell, OSIP_NICT_REGISTER_SENT, &cs_cb_sndregister);
    osip_set_message_callback(cell, OSIP_NICT_BYE_SENT, &cs_cb_sndbye);
    osip_set_message_callback(cell, OSIP_NICT_OPTIONS_SENT, &cs_cb_sndoptions);
    osip_set_message_callback(cell, OSIP_NICT_INFO_SENT, &cs_cb_sndinfo);
    osip_set_message_callback(cell, OSIP_NICT_CANCEL_SENT, &cs_cb_sndcancel);
    osip_set_message_callback(cell, OSIP_NICT_NOTIFY_SENT, &cs_cb_sndoptions);
    osip_set_message_callback(cell, OSIP_NICT_SUBSCRIBE_SENT, &cs_cb_sndoptions);
    osip_set_message_callback(cell, OSIP_NICT_UNKNOWN_REQUEST_SENT, &cs_cb_sndunkrequest);
    osip_set_message_callback(cell, OSIP_NICT_REQUEST_SENT_AGAIN, &cs_cb_sndreq_retransmission);

    osip_set_message_callback(cell, OSIP_NICT_STATUS_1XX_RECEIVED, &cs_cb_nict_rcv1xx);
    osip_set_message_callback(cell, OSIP_NICT_STATUS_2XX_RECEIVED, &cs_cb_nict_rcv2xx);
    osip_set_message_callback(cell, OSIP_NICT_STATUS_2XX_RECEIVED_AGAIN, &cs_cb_rcvresp_retransmission);
    osip_set_message_callback(cell, OSIP_NICT_STATUS_3XX_RECEIVED, &cs_cb_nict_rcv3xx);
    osip_set_message_callback(cell, OSIP_NICT_STATUS_4XX_RECEIVED, &cs_cb_nict_rcv456xx);
    osip_set_message_callback(cell, OSIP_NICT_STATUS_5XX_RECEIVED, &cs_cb_nict_rcv456xx);
    osip_set_message_callback(cell, OSIP_NICT_STATUS_6XX_RECEIVED, &cs_cb_nict_rcv456xx);
    osip_set_message_callback(cell, OSIP_NICT_STATUS_3456XX_RECEIVED_AGAIN, &cs_cb_rcvresp_retransmission);

    osip_set_message_callback(cell, OSIP_NIST_REGISTER_RECEIVED, &cs_cb_rcvregister);
    osip_set_message_callback(cell, OSIP_NIST_BYE_RECEIVED, &cs_cb_rcvbye);
    osip_set_message_callback(cell, OSIP_NIST_OPTIONS_RECEIVED, &cs_cb_rcvoptions);
    osip_set_message_callback(cell, OSIP_NIST_INFO_RECEIVED, &cs_cb_rcvinfo);
    osip_set_message_callback(cell, OSIP_NIST_CANCEL_RECEIVED, &cs_cb_rcvcancel);
    osip_set_message_callback(cell, OSIP_NIST_NOTIFY_RECEIVED, &cs_cb_rcvnotify);
    osip_set_message_callback(cell, OSIP_NIST_SUBSCRIBE_RECEIVED, &cs_cb_rcvsubscribe);
    osip_set_message_callback(cell, OSIP_NIST_MESSAGE_RECEIVED, &cs_cb_rcvmessage);
    osip_set_message_callback(cell, OSIP_NIST_UPDATE_RECEIVED, &cs_cb_rcvupdate);
    osip_set_message_callback(cell, OSIP_NIST_REFER_RECEIVED, &cs_cb_rcvrefer);
    osip_set_message_callback(cell, OSIP_NIST_UNKNOWN_REQUEST_RECEIVED, &cs_cb_rcvunkrequest);
    osip_set_message_callback(cell, OSIP_NIST_REQUEST_RECEIVED_AGAIN, &cs_cb_rcvreq_retransmission);

    osip_set_message_callback(cell, OSIP_NIST_STATUS_1XX_SENT, &cs_cb_nist_snd1xx);
    osip_set_message_callback(cell, OSIP_NIST_STATUS_2XX_SENT, &cs_cb_nist_snd2xx);
    osip_set_message_callback(cell, OSIP_NIST_STATUS_2XX_SENT_AGAIN, &cs_cb_nist_snd2xx);
    osip_set_message_callback(cell, OSIP_NIST_STATUS_3XX_SENT, &cs_cb_nist_snd3456xx);
    osip_set_message_callback(cell, OSIP_NIST_STATUS_4XX_SENT, &cs_cb_nist_snd3456xx);
    osip_set_message_callback(cell, OSIP_NIST_STATUS_5XX_SENT, &cs_cb_nist_snd3456xx);
    osip_set_message_callback(cell, OSIP_NIST_STATUS_6XX_SENT, &cs_cb_nist_snd3456xx);
    osip_set_message_callback(cell, OSIP_NIST_STATUS_3456XX_SENT_AGAIN, &cs_cb_sndresp_retransmission);

    osip_set_message_callback(cell, OSIP_ICT_STATUS_TIMEOUT, &cs_cb_ict_timeout);
    osip_set_message_callback(cell, OSIP_NICT_STATUS_TIMEOUT, &cs_cb_nict_timeout);

    return 0;
}
#endif

#if DECS("上层应用回调函数")
/*****************************************************************************
 函 数 名  : app_callback_init
 功能描述  : 应用层回调函数结构初始化
 输入参数  : app_callback_t** appcallback
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int app_callback_init()
{
    g_AppCallback = (app_callback_t*) osip_malloc(sizeof(app_callback_t));

    if (g_AppCallback == NULL)
    {
        return -1;
    }

    g_AppCallback->uas_register_received_cb = NULL;
    g_AppCallback->uas_register_received_timeout_cb = NULL;
    g_AppCallback->uac_register_response_received_cb = NULL;
    g_AppCallback->uac_register_response_received_cb_user_data = 0;
    g_AppCallback->invite_received_cb = NULL;
    g_AppCallback->invite_received_cb_user_data = 0;
    g_AppCallback->invite_response_received_cb = NULL;
    g_AppCallback->invite_response_received_cb_user_data = 0;
    g_AppCallback->cancel_received_cb = NULL;
    g_AppCallback->ack_received_cb = NULL;
    g_AppCallback->ack_received_cb_user_data = 0;
    g_AppCallback->bye_received_cb = NULL;
    g_AppCallback->bye_received_cb_user_data = 0;
    g_AppCallback->bye_response_received_cb = NULL;
    g_AppCallback->bye_response_received_cb_user_data = 0;
    g_AppCallback->message_received_cb = NULL;
    g_AppCallback->message_received_cb_user_data = 0;
    g_AppCallback->message_response_received_cb = NULL;
    g_AppCallback->message_response_received_cb_user_data = 0;
    g_AppCallback->info_received_cb = NULL;
    g_AppCallback->info_received_cb_user_data = 0;
    g_AppCallback->info_response_received_cb = NULL;
    g_AppCallback->info_response_received_cb_user_data = 0;
    g_AppCallback->ua_session_expires_cb = NULL;
    g_AppCallback->dbg_printf_cb = NULL;
    g_AppCallback->sip_message_trace_cb = NULL;

    return 0;
}

/*****************************************************************************
 函 数 名  : app_callback_free
 功能描述  : 应用层回调函数结构释放
 输入参数  : app_callback_t* appcallback
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void app_callback_free()
{
    if (g_AppCallback == NULL)
    {
        return;
    }

    g_AppCallback->uas_register_received_cb = NULL;
    g_AppCallback->uas_register_received_timeout_cb = NULL;
    g_AppCallback->uac_register_response_received_cb = NULL;
    g_AppCallback->uac_register_response_received_cb_user_data = 0;
    g_AppCallback->invite_received_cb = NULL;
    g_AppCallback->invite_received_cb_user_data = 0;
    g_AppCallback->invite_response_received_cb = NULL;
    g_AppCallback->invite_response_received_cb_user_data = 0;
    g_AppCallback->cancel_received_cb = NULL;
    g_AppCallback->ack_received_cb = NULL;
    g_AppCallback->ack_received_cb_user_data = 0;
    g_AppCallback->bye_received_cb = NULL;
    g_AppCallback->bye_received_cb_user_data = 0;
    g_AppCallback->bye_response_received_cb = NULL;
    g_AppCallback->bye_response_received_cb_user_data = 0;
    g_AppCallback->message_received_cb = NULL;
    g_AppCallback->message_received_cb_user_data = 0;
    g_AppCallback->message_response_received_cb = NULL;
    g_AppCallback->message_response_received_cb_user_data = 0;
    g_AppCallback->info_received_cb = NULL;
    g_AppCallback->info_received_cb_user_data = 0;
    g_AppCallback->info_response_received_cb = NULL;
    g_AppCallback->info_response_received_cb_user_data = 0;
    g_AppCallback->ua_session_expires_cb = NULL;
    g_AppCallback->dbg_printf_cb = NULL;
    g_AppCallback->sip_message_trace_cb = NULL;

    osip_free(g_AppCallback);
    g_AppCallback = NULL;
    return;
}

/*****************************************************************************
 函 数 名  : app_set_uas_register_received_cb
 功能描述  : 服务端收到注册消息回调函数设置
 输入参数  : int (*cb) (char*, char*, char*, int, char*, int, int, int)
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void app_set_uas_register_received_cb(int (*cb)(char*, char*, char*, int, char*, int, int, int))
{
    if ((NULL == cb) || (NULL == g_AppCallback))
    {
        return;
    }

    g_AppCallback->uas_register_received_cb = cb;
    return;
}

/*****************************************************************************
 函 数 名  : app_set_uas_register_received_timeout_cb
 功能描述  : 设置服务器端没有收到客户端刷新注册超时通知函数
 输入参数  : int (*cb) (char*, char*, char*, int, int)
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月17日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void app_set_uas_register_received_timeout_cb(int (*cb)(char*, char*, char*, int, int))
{
    if ((NULL == cb) || (NULL == g_AppCallback))
    {
        return;
    }

    g_AppCallback->uas_register_received_timeout_cb = cb;
    return;
}

/*****************************************************************************
 函 数 名  : app_set_uac_register_response_received_cb
 功能描述  : 客户端发送注册消息后收到注册响应消息回调函数设置
 输入参数  : int (*cb) (int, int, int, char*, unsigned int, int)
             int user_data
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void app_set_uac_register_response_received_cb(int (*cb)(int, int, int, char*, unsigned int, int), int user_data)
{
    if ((NULL == cb) || (NULL == g_AppCallback))
    {
        return;
    }

    g_AppCallback->uac_register_response_received_cb = cb;
    g_AppCallback->uac_register_response_received_cb_user_data = user_data;
    return;
}

/*****************************************************************************
 函 数 名  : app_set_invite_received_cb
 功能描述  : 收到呼叫消息回调函数
 输入参数  : int (*cb) (char*, char*, char*, int, int)
                            int user_data
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void app_set_invite_received_cb(int (*cb)(char*, char*, char*, int, char*, int, int), int user_data)
{
    if ((NULL == cb) || (NULL == g_AppCallback))
    {
        return;
    }

    g_AppCallback->invite_received_cb = cb;
    g_AppCallback->invite_received_cb_user_data = user_data;
    return;
}

/*****************************************************************************
 函 数 名  : app_set_invite_response_received_cb
 功能描述  : 收到呼叫响应消息回调函数
 输入参数  : int (*cb) (char*, char*, char*, int, int, char*, char*, int, int)
                            int user_data
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void app_set_invite_response_received_cb(int (*cb)(char*, char*, char*, int, int, char*, char*, int, int), int user_data)
{
    if ((NULL == cb) || (NULL == g_AppCallback))
    {
        return;
    }

    g_AppCallback->invite_response_received_cb = cb;
    g_AppCallback->invite_response_received_cb_user_data = user_data;
    return;
}

/*****************************************************************************
 函 数 名  : app_set_cancel_received_cb
 功能描述  : 收到Cancel 消息回调函数
 输入参数  : int (*cb)(char*
                            char*
                            char*
                            int
                            int)
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
void app_set_cancel_received_cb(int (*cb)(char*, char*, char*, int, int), int user_data)
{
    if ((NULL == cb) || (NULL == g_AppCallback))
    {
        return;
    }

    g_AppCallback->cancel_received_cb = cb;
    g_AppCallback->cancel_received_cb_user_data = user_data;
    return;
}

/*****************************************************************************
 函 数 名  : app_set_ack_received_cb
 功能描述  : 收到ACK 消息回调函数
 输入参数  : int (*cb) (char*, char*, char*, int,int)
                            int user_data
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void app_set_ack_received_cb(int (*cb)(char*, char*, char*, int, int), int user_data)
{
    if ((NULL == cb) || (NULL == g_AppCallback))
    {
        return;
    }

    g_AppCallback->ack_received_cb = cb;
    g_AppCallback->ack_received_cb_user_data = user_data;
    return;
}

/*****************************************************************************
 函 数 名  : app_set_bye_received_cb
 功能描述  : 收到呼叫结束消息回调函数
 输入参数  : int (*cb) (char*, char*, char*, int,int)
                            int user_data
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void app_set_bye_received_cb(int (*cb)(char*, char*, char*, int, int), int user_data)
{
    if ((NULL == cb) || (NULL == g_AppCallback))
    {
        return;
    }

    g_AppCallback->bye_received_cb = cb;
    g_AppCallback->bye_received_cb_user_data = user_data;
    return;
}

/*****************************************************************************
 函 数 名  : app_set_bye_response_received_cb
 功能描述  : 收到呼叫结束响应消息回调函数
 输入参数  : int (*cb) (char*, char*, int, int,int)
                            int user_data
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void app_set_bye_response_received_cb(int (*cb)(char*, char*, char*, int, int, int), int user_data)
{
    if ((NULL == cb) || (NULL == g_AppCallback))
    {
        return;
    }

    g_AppCallback->bye_response_received_cb = cb;
    g_AppCallback->bye_response_received_cb_user_data = user_data;
    return;
}

/*****************************************************************************
 函 数 名  : app_set_message_received_cb
 功能描述  : 收到Message 消息回调函数
 输入参数  : int (*cb) (char*
                            char*
                            char*
                            char*
                            int
                            int)
                            int user_data
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void app_set_message_received_cb(int (*cb)(char*, char*, int, char*, char*, int, char*, int, char*, int, int), int user_data)
{
    if ((NULL == cb) || (NULL == g_AppCallback))
    {
        return;
    }

    g_AppCallback->message_received_cb = cb;
    g_AppCallback->message_received_cb_user_data = user_data;
    return;
}

/*****************************************************************************
 函 数 名  : app_set_message_response_received_cb
 功能描述  : 收到Message 响应消息回调函数
 输入参数  : int (*cb) (char*
                            char*
                            char*
                            int
                            int)
                            int user_data
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void app_set_message_response_received_cb(int (*cb)(char*, char*, char*, int, int), int user_data)
{
    if ((NULL == cb) || (NULL == g_AppCallback))
    {
        return;
    }

    g_AppCallback->message_response_received_cb = cb;
    g_AppCallback->message_response_received_cb_user_data = user_data;
    return;
}

/*****************************************************************************
 函 数 名  : app_set_subscribe_received_cb
 功能描述  : 收到Subscribe 消息回调函数
 输入参数  : int (*cb) (char*
                            char*
                            char*
                            char*
                            char*
                            int,
                            char*
                            int
                            int)
                            int user_data
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void app_set_subscribe_received_cb(int (*cb)(char*, char*, int, char*, char*, int, char*, char*, char*, int, char*, int, int), int user_data)
{
    if ((NULL == cb) || (NULL == g_AppCallback))
    {
        return;
    }

    g_AppCallback->subscribe_received_cb = cb;
    g_AppCallback->subscribe_received_cb_user_data = user_data;
    return;
}

/*****************************************************************************
 函 数 名  : app_set_subscribe_response_received_cb
 功能描述  : 收到Subscribe 响应消息回调函数
 输入参数  : int (*cb) (char*
                            char*
                            char*
                            int
                            int)
                            int user_data
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void app_set_subscribe_response_received_cb(int (*cb)(char*, char*, char*, int, int, int), int user_data)
{
    if ((NULL == cb) || (NULL == g_AppCallback))
    {
        return;
    }

    g_AppCallback->subscribe_response_received_cb = cb;
    g_AppCallback->subscribe_response_received_cb_user_data = user_data;
    return;
}

void app_set_subscribe_within_dialog_received_cb(int (*cb)(char*, char*, int, char*, char*, int, int, char*, int))
{
    if ((NULL == cb) || (NULL == g_AppCallback))
    {
        return;
    }

    g_AppCallback->subscribe_within_dialog_received_cb = cb;
    return;
}

void app_set_subscribe_within_dialog_response_received_cb(int (*cb)(char*, char*, char*, int, int, int))
{
    if ((NULL == cb) || (NULL == g_AppCallback))
    {
        return;
    }

    g_AppCallback->subscribe_within_dialog_response_received_cb = cb;
    return;
}

/*****************************************************************************
 函 数 名  : app_set_notify_received_cb
 功能描述  : 收到 Notify 消息回调函数
 输入参数  : int (*cb) (char*
                            char*
                            char*
                            char*
                            int
                            int)
                            int user_data
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void app_set_notify_received_cb(int (*cb)(char*, char*, int, char*, char*, int, char*, char*, int, int), int user_data)
{
    if ((NULL == cb) || (NULL == g_AppCallback))
    {
        return;
    }

    g_AppCallback->notify_received_cb = cb;
    g_AppCallback->notify_received_cb_user_data = user_data;
    return;
}

/*****************************************************************************
 函 数 名  : app_set_notify_response_received_cb
 功能描述  : 收到 Notify 响应消息回调函数
 输入参数  : int (*cb) (char*
                            char*
                            char*
                            int
                            int)
                            int user_data
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void app_set_notify_response_received_cb(int (*cb)(char*, char*, char*, int, int), int user_data)
{
    if ((NULL == cb) || (NULL == g_AppCallback))
    {
        return;
    }

    g_AppCallback->notify_response_received_cb = cb;
    g_AppCallback->notify_response_received_cb_user_data = user_data;
    return;
}

/*****************************************************************************
 函 数 名  : app_set_info_received_cb
 功能描述  : 收到Info 消息回调函数
 输入参数  : int (*cb) (char*
                            char*
                            char*
                            int
                            char*
                            int
                            int)
                            int user_data
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void app_set_info_received_cb(int (*cb)(char*, char*, int, char*, char*, int, char*, int, int), int user_data)
{
    if ((NULL == cb) || (NULL == g_AppCallback))
    {
        return;
    }

    g_AppCallback->info_received_cb = cb;
    g_AppCallback->info_received_cb_user_data = user_data;
    return;
}

/*****************************************************************************
 函 数 名  : app_set_info_response_received_cb
 功能描述  : 收到Info 响应消息回调函数
 输入参数  : int (*cb) (char*
                            char*
                            char*
                            int
                            int)
                            int user_data
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void app_set_info_response_received_cb(int (*cb)(char*, char*, char*, int, int), int user_data)
{
    if ((NULL == cb) || (NULL == g_AppCallback))
    {
        return;
    }

    g_AppCallback->info_response_received_cb = cb;
    g_AppCallback->info_response_received_cb_user_data = user_data;
    return;
}

/*****************************************************************************
 函 数 名  : app_set_ua_session_expires_cb
 功能描述  : UA会话超时回调函数
 输入参数  : int (*cb)(int)
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年9月3日 星期二
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void app_set_ua_session_expires_cb(int (*cb)(int))
{
    if ((NULL == cb) || (NULL == g_AppCallback))
    {
        return;
    }

    g_AppCallback->ua_session_expires_cb = cb;
    return;
}

/*****************************************************************************
 函 数 名  : app_set_dbg_printf_cb
 功能描述  : debug调试打印函数设置
 输入参数  : void (*cb) (int, char*, int, const char*, ...)
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void app_set_dbg_printf_cb(void (*cb)(int, const char*, const char*, int, const char*))
{
    if ((NULL == cb) || (NULL == g_AppCallback))
    {
        return;
    }

    g_AppCallback->dbg_printf_cb = cb;
    return;
}

/*****************************************************************************
 函 数 名  : app_set_sip_message_trace_cb
 功能描述  : SIP消息跟踪调试回调函数设置
 输入参数  : void (*cb)(int
                             int
                             char*
                             int
                             char*)
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年9月4日 星期三
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void app_set_sip_message_trace_cb(void (*cb)(int, int, char*, int, char*))
{
    if ((NULL == cb) || (NULL == g_AppCallback))
    {
        return;
    }

    g_AppCallback->sip_message_trace_cb = cb;
    return;
}
#endif
