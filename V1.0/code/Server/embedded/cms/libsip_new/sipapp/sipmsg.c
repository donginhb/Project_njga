/******************************************************************************

                  版权所有 (C), 2001-2013, 金智视讯技术有限公司

 ******************************************************************************
  文 件 名   : sipmsg.c
  版 本 号   : 初稿
  作    者   : yanghaifeng
  生成日期   : 2013年4月3日
  最近修改   :
  功能描述   : sip消息
  函数列表   :
              call_id_new_random
              cancel_match_st
              complete_answer_that_establish_a_dialog
              cseqnum_match
              cs_fix_last_via_header
              cs_generating_answer
              cs_generating_response_default
              cs_response_add_proxy_authenticate
              cs_response_add_www_authenticate
              dialog_fill_route_set
              from_tag_new_random
              generating_1xx_answer_to_options
              generating_2xx_answer_to_options
              generating_3456xx_answer_to_options
              generating_ack_for_2xx
              generating_bye
              generating_cancel
              generating_digest_auth
              generating_info_within_dialog
              generating_invite
              generating_message
              generating_notify_within_dialog
              generating_options
              generating_options_within_dialog
              generating_refer_within_dialog
              generating_register
              generating_request_fromrequest
              generating_request_out_of_dialog
              generating_request_within_dialog
              generating_response_default
              generating_sdp_answer
              generating_sdp_answer2
              generating_subscribe
              generating_subscribe_with_dialog
              get_contact
              get_from
              get_message_sdp
              get_registrator
              get_sdp_videoinfo
              get_to
              nonce_new_random
              request_add_authorization
              request_add_proxy_authorization
              request_get_destination
              response_get_destination
              sip_notification_free
              sip_notification_init
              sip_subscription_free
              sip_subscription_init
              tl_sendmessage
              to_tag_new_random
              ul_sendmessage
              url_compare
              url_match_simply
              via_branch_new_random
  修改历史   :
  1.日    期   : 2013年4月3日
    作    者   : yanghaifeng
    修改内容   : 创建文件

******************************************************************************/

/*----------------------------------------------*
 * 包含头文件                                   *
 *----------------------------------------------*/
#include <malloc.h>
#ifdef WIN32
#include <winsock2.h>
#include <windows.h>
#include <sys/types.h>
#else
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/stat.h>
#endif

#include "include/libsip.h"

#include "gbltype.h"
#include "gblfunc.inc"
#include "sipmsg.inc"

#include "csdbg.inc"
#include "udp_tl.inc"
#include "sipauth.inc"


//added by chenyu 130522
#ifdef WIN32
#define vsnprintf _vsnprintf
#define snprintf  _snprintf
#endif

/*----------------------------------------------*
 * 外部变量说明                                 *
 *----------------------------------------------*/
extern sdp_config_t* config;

/*----------------------------------------------*
 * 外部函数原型说明                             *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 内部函数原型说明                             *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 全局变量                                     *
 *----------------------------------------------*/
sip_message_list_t* g_SipMessageList = NULL;

/*----------------------------------------------*
 * 模块级变量                                   *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 常量定义                                     *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 宏定义                                       *
 *----------------------------------------------*/
static unsigned int a = 0;
static unsigned int b = 0;
static unsigned int x = 0;

void init_random_number()
{
    srand((unsigned int) time(NULL));
    a = 4 * (1 + (int)(20000000.0 * rand() / (RAND_MAX + 1.0))) + 1;
    b = 2 * (1 + (int)(30324000.0 * rand() / (RAND_MAX + 1.0))) + 1;
    x = (1 + (int)(23445234.0 * rand() / (RAND_MAX + 1.0)));
}

unsigned int new_random_number()
{
    x = a * x + b;
    return x;
}

char* new_to_tag()
{
    char* _to_tag = NULL;
    unsigned int _i = 0;

    /*
     *      *  As per RFC3261, the To tag is 32 bits long
     *              *
     *                      */
    if ((_to_tag = (char*) osip_malloc(sizeof(char) * 33)) == NULL)
    {
        perror(" new_to_tag() malloc memory error!");
    }

    _i = new_random_number();

    snprintf(_to_tag, sizeof(char) * 33, "%u", _i);

    return _to_tag;

}

char* new_from_tag()
{
    return new_to_tag();
}

char* new_callid()
{
    char* _to_tag = NULL;
    unsigned int _i = 0;

    /*
     *      *  As per RFC3261, the To tag is 32 bits long
     *              *
     *                      */
    if ((_to_tag = (char*) osip_malloc(sizeof(char) * 33)) == NULL)
    {
        perror(" new_to_tag() malloc memory error!");
    }

    _i = new_random_number();

    snprintf(_to_tag, sizeof(char) * 33, "%u", _i);

    return _to_tag;
}

void free_callid(char*p)
{
    if (p)
    {
        osip_free(p);
    }

    return ;
}

unsigned int new_via_branch()
{
    return new_random_number();
}

/*****************************************************************************
 函 数 名  : to_tag_new_random
 功能描述  : 产生新的to tag
 输入参数  : 无
 输出参数  : 无
 返 回 值  : char
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月3日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
char* to_tag_new_random()
{
#ifndef NEW_RANDOM
    return new_to_tag();
#else

    char* _to_tag = NULL;
    unsigned int _i1, _i2, _i3, _i4;

    if ((_to_tag = (char*) osip_malloc(sizeof(char) * 34)) == NULL)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "to_tag_new_random() exit---: Malloc Memory Error \r\n");
        return NULL;
    }

    _i1 = new_random_number();
    _i2 = new_random_number();
    _i3 = new_random_number();
    _i4 = new_random_number();
    snprintf(_to_tag, sizeof(char) * 34, "%.8x%.8x%.8x-%.8x", _i1, _i2, _i3, _i4);

    return _to_tag;

#endif
}

/*****************************************************************************
 函 数 名  : from_tag_new_random
 功能描述  : 产生新的from tag
 输入参数  : 无
 输出参数  : 无
 返 回 值  : char
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月3日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
char* from_tag_new_random()
{
    return to_tag_new_random();
}

/*****************************************************************************
 函 数 名  : call_id_new_random
 功能描述  : 产生新的call id
 输入参数  : 无
 输出参数  : 无
 返 回 值  : char
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月3日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
char* call_id_new_random()
{
#ifndef  NEW_RANDOM
    return new_callid();
#else
    char* _to_tag;
    unsigned int _i1, _i2, _i3, _i4;

    if ((_to_tag = (char*) osip_malloc(sizeof(char) * 36)) == NULL)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "call_id_new_random() exit---: Malloc Memory Error \r\n");
        return NULL;
    }

    _i1 = new_random_number();
    _i2 = new_random_number();
    _i3 = new_random_number();
    _i4 = new_random_number();
    snprintf(_to_tag, sizeof(char) * 36, "%.8x-%.8x-%.8x-%.8x", _i1, _i2, _i3, _i4);

    return _to_tag;
#endif
}

/*****************************************************************************
 函 数 名  : nonce_new_random
 功能描述  : 产生新的nonce
 输入参数  : 无
 输出参数  : 无
 返 回 值  : char
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月3日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
char* nonce_new_random()
{
#ifndef NEW_RANDOM
    return new_to_tag();
#else

    char* _to_tag = NULL;
    unsigned int _i1, _i2, _i3, _i4;

    if ((_to_tag = (char*) osip_malloc(sizeof(char) * 33)) == NULL)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "nonce_new_random() exit---: Malloc Memory Error \r\n");
        return NULL;
    }

    _i1 = new_random_number();
    _i2 = new_random_number();
    _i3 = new_random_number();
    _i4 = new_random_number();
    snprintf(_to_tag, sizeof(char) * 33, "%.8x%.8x%.8x%.8x", _i1, _i2, _i3, _i4);

    return _to_tag;

#endif
}

/*****************************************************************************
 函 数 名  : via_branch_new_random
 功能描述  : 产生新的via branch
 输入参数  : 无
 输出参数  : 无
 返 回 值  : unsigned
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月3日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
unsigned int via_branch_new_random()
{
    return new_random_number();
}

/*****************************************************************************
 函 数 名  : cs_fix_last_via_header
 功能描述  : 匹配最后的via头部
 输入参数  : sip_t *msg
             char *ip_addr
             int port
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月3日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int cs_fix_last_via_header(osip_message_t* msg, char* ip_addr, int port)
{
    osip_generic_param_t* rport = NULL;
    osip_via_t* via = NULL;

    /* get Top most Via header: */
    if (msg == NULL)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "cs_fix_last_via_header() exit---: Param Error \r\n");
        return -1;
    }

    if (MSG_IS_RESPONSE(msg))
    {
        //SIP_DEBUG_TRACE(LOG_DEBUG, "cs_fix_last_via_header() exit---: MSG_IS_RESPONSE \r\n");
        return 0;           /* Don't fix Via header */
    }

    via = (osip_via_t*)osip_list_get(&msg->vias, 0);

    if (via == NULL || via->host == NULL)
    {
        /* Hey, we could build it? */
        SIP_DEBUG_TRACE(LOG_DEBUG, "cs_fix_last_via_header() exit---: via NULL \r\n");
        return -1;
    }

    osip_via_param_get_byname(via, (char*)"rport", &rport);

    if (rport != NULL)
    {
        if (rport->gvalue == NULL)
        {
            rport->gvalue = (char*) osip_malloc(9);

            if (rport->gvalue == NULL)
            {
                SIP_DEBUG_TRACE(LOG_DEBUG, "cs_fix_last_via_header() exit---: rport malloc error \r\n");
                return -1;
            }

#ifdef WIN32
            _snprintf(rport->gvalue, 8, "%d", port);
#else
            snprintf(rport->gvalue, 8, "%d", port);
#endif
        }           /* else bug? */
    }
    else if (sstrcmp(via->host, ip_addr))
    {
        int i = 0;
        char* gname = NULL, *gvalue = NULL;
        gname = osip_getcopy("rport");
        gvalue = (char*)osip_malloc(9);

        if (gname != NULL && gvalue != NULL)
        {
#ifdef WIN32
            _snprintf(gvalue, 8, "%d", port);
#else
            snprintf(gvalue, 8, "%d", port);
#endif
            i = osip_via_param_add(via, gname, gvalue);

            if (i != 0)
            {
                osip_free(gname);
                gname = NULL;
                osip_free(gvalue);
                gvalue = NULL;
            }
        }
    }

    /* only add the received parameter if the 'sent-by' value does not contains
    this ip address */
    if (0 == sstrcmp(via->host, ip_addr))   /* don't need the received parameter */
    {
        //SIP_DEBUG_TRACE(LOG_DEBUG, "cs_fix_last_via_header() via->host=%s \r\n", via->host);
        //SIP_DEBUG_TRACE(LOG_DEBUG, "cs_fix_last_via_header() ip_addr=%s \r\n", ip_addr);
        return 0;
    }

    osip_via_set_received(via, osip_getcopy(ip_addr));

    return 0;
}

/*****************************************************************************
 函 数 名  : cseqnum_match
 功能描述  : cseq号码匹配
 输入参数  : cseq_t *cseq1
             cseq_t *cseq2
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月3日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int  cseqnum_match(osip_cseq_t* cseq1, osip_cseq_t* cseq2)
{
    if (cseq1 == NULL || cseq2 == NULL)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "cseqnum_match() exit---: Param Error \r\n");
        return -1;
    }

    if (cseq1->number == NULL || cseq2->number == NULL
        || cseq1->method == NULL || cseq2->method == NULL)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "cseqnum_match() exit---: Cseq Error \r\n");
        return -1;
    }

    if (0 == sstrcmp(cseq1->number, cseq2->number))
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "cseqnum_match() exit---: Match \r\n");
        return 0;
    }

    SIP_DEBUG_TRACE(LOG_DEBUG, "cseqnum_match() exit---: No Match \r\n");
    return -1;

}

/*****************************************************************************
 函 数 名  : cancel_match_st
 功能描述  : CANCEL message match server transaction
 输入参数  : transaction_t *tr
                            sip_t *cancel
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月3日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int cancel_match_st(osip_transaction_t* tr, osip_message_t* cancel)
{
    osip_generic_param_t* br = NULL;
    osip_generic_param_t* br2 = NULL;
    osip_via_t* via = NULL;
    osip_via_t* via2 = NULL;
    osip_message_t* request = NULL;

    if (tr == NULL || cancel == NULL)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "cancel_match_st() exit---: Param Error \r\n");
        return -1;
    }

    request = tr->orig_request;

    if (request == NULL)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "cancel_match_st() exit---: Request NULL \r\n");
        return -1;
    }

    via = (osip_via_t*)osip_list_get(&request->vias, 0);

    if (via == NULL)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "cancel_match_st() exit---: Request Via NULL \r\n");
        return -1;
    }

    osip_via_param_get_byname(via, (char*)"branch", &br);

    via2 = (osip_via_t*)osip_list_get(&cancel->vias, 0);

    if (via2 == NULL)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "cancel_match_st() exit---: Request Via NULL \r\n");
        return -1; /* request without via??? */
    }

    osip_via_param_get_byname(via2, (char*)"branch", &br2);

    if (br != NULL && br2 == NULL)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "cancel_match_st() exit---: Request Branch NULL \r\n");
        return -1;
    }

    if (br2 != NULL && br == NULL)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "cancel_match_st() exit---: Request Branch NULL \r\n");
        return -1;
    }

    if (br2 != NULL && br != NULL) /* compliant UA  :) */
    {
        if (br->gvalue != NULL && br2->gvalue != NULL
            && 0 == sstrcmp(br->gvalue, br2->gvalue))
        {
            SIP_DEBUG_TRACE(LOG_DEBUG, "cancel_match_st() exit---: OK 1 \r\n");
            return 0;
        }

        SIP_DEBUG_TRACE(LOG_DEBUG, "cancel_match_st() exit---: Request Branch Not Match \r\n");
        return -1;
    }

    /* old backward compatibility mechanism */
    if (0 != osip_call_id_match(request->call_id, cancel->call_id))
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "cancel_match_st() exit---: Call ID Not Match \r\n");
        return -1;
    }

    if (0 != osip_to_tag_match(request->to, cancel->to))
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "cancel_match_st() exit---: To Tag Not Match \r\n");
        return -1;
    }

    if (0 != osip_from_tag_match(request->from, cancel->from))
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "cancel_match_st() exit---: From Tag Not Match \r\n");
        return -1;
    }

    if (0 != cseqnum_match(request->cseq, cancel->cseq))
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "cancel_match_st() exit---: Cseq Num Not Match \r\n");
        return -1;
    }

    if (0 != osip_via_match(via, via2))
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "cancel_match_st() exit---: Via Not Match \r\n");
        return -1;
    }

    SIP_DEBUG_TRACE(LOG_DEBUG, "cancel_match_st() exit---: OK 2 \r\n");
    return 0;
}

/*****************************************************************************
 函 数 名  : url_match_simply
 功能描述  : compare the url_t header.
 输入参数  : url_t * url1
             url_t * url2
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月3日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int url_match_simply(osip_uri_t* url1, osip_uri_t* url2)
{
    if (url1 == NULL || url2 == NULL)
    {
        //SIP_DEBUG_TRACE(LOG_DEBUG, "url_match_simply() exit---: URL NULL \r\n");
        return -1;
    }

    if (url1->username == NULL || url2->username == NULL)
    {
        //SIP_DEBUG_TRACE(LOG_DEBUG, "url_match_simply() exit---: Username NULL \r\n");
        return -1;
    }

    if (url1->host == NULL || url2->host == NULL)
    {
        //SIP_DEBUG_TRACE(LOG_DEBUG, "url_match_simply() exit---: Host NULL \r\n");
        return -1;
    }

    if (sstrcmp(url1->username, url2->username))
    {
        //SIP_DEBUG_TRACE(LOG_DEBUG, "url_match_simply() exit---: Username Not Equal \r\n");
        return -1;
    }

    return host_match(url1->host, url2->host);
}

/*****************************************************************************
 函 数 名  : url_compare
 功能描述  : url结构比较
 输入参数  : url_t *url1
             url_t *url2
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月3日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int url_compare(osip_uri_t* url1, osip_uri_t* url2)
{
    if (url1 == NULL || url2 == NULL)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "url_compare() exit---: Param Error \r\n");
        return -1;
    }

    /* we could have a sip or sips url, but if string!=NULL,
    host part will be NULL. */
    if (url1->host == NULL && url2->host == NULL)
    {
        if (url1->string == NULL || url2->string == NULL)
        {
            SIP_DEBUG_TRACE(LOG_DEBUG, "url_compare() exit---: URL String NULL \r\n");
            return -1;
        }

        if (0 == sstrcmp(url1->string, url2->string))
        {
            //SIP_DEBUG_TRACE(LOG_DEBUG, "url_compare() exit---: OK 1 \r\n");
            return 0;
        }
    }

    if (url1->host == NULL || url2->host == NULL)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "url_compare() exit---: URL Host NULL \r\n");
        return -1;
    }

    /* compare url */
    if (0 != sstrcmp(url1->host, url2->host))
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "url_compare() exit---: URL Host Not Match \r\n");
        return -1;
    }

    if (url1->username != NULL && url2->username == NULL)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "url_compare() exit---: URL2 Username NULL \r\n");
        return -1;
    }

    if (url1->username == NULL && url2->username != NULL)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "url_compare() exit---: URL1 Username NULL \r\n");
        return -1;
    }

    if (url1->username == NULL && url2->username == NULL)
    {
        //SIP_DEBUG_TRACE(LOG_DEBUG, "url_compare() exit---: URL12 Username NULL \r\n");
        return 0;
    }

    if (0 != sstrcmp(url1->username, url2->username))
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "url_compare() exit---: URL Username Not Match \r\n");
        return -1;
    }

    //SIP_DEBUG_TRACE(LOG_DEBUG, "url_compare() exit---: OK 2 \r\n");
    return 0;
}

/*****************************************************************************
 函 数 名  : ul_sendmessage
 功能描述  : send message by transaction user layer
 输入参数  : transaction_t* transaction
             sip_t* msg
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月3日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int ul_sendmessage(osip_transaction_t* transaction, osip_message_t* msg)
{
    int i = 0;
    osip_event_t* sipevent = NULL;

    if (transaction == NULL || msg == NULL)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "ul_sendmessage() exit---: Param Error \r\n");
        return -1;
    }

    sipevent = osip_new_outgoing_sipmessage(msg);

    if (sipevent == NULL)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "ul_sendmessage() exit---: SIP Event NULL \r\n");
        return -1;
    }

    sipevent->transactionid = transaction->transactionid;
    i = osip_transaction_add_event(transaction, sipevent);

    return i;
}

/*****************************************************************************
 函 数 名  : tl_sendmessage
 功能描述  : send message by transport  layer
 输入参数  : sip_t *sip
             char *host
             int port
             int out_socket
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月3日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int tl_sendmessage(osip_message_t* sip, char* host, int port, int out_socket)
{
    return send_message_using_udp(NULL, sip, host, port, out_socket);
}

/*****************************************************************************
 函 数 名  : tl_sendmessage_by_tcp
 功能描述  : 使用TCP协议发送SIP消息
 输入参数  : osip_message_t* sip
             char* host
             int port
             int out_socket
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年2月21日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int tl_sendmessage_by_tcp(osip_message_t* sip, char* host, int port, int out_socket)
{
    return send_message_using_tcp(sip, host, port, out_socket);
}


#if DECS("认证")
/*****************************************************************************
 函 数 名  : response_get_destination
 功能描述  : 从回应消息中获取目的信息
 输入参数  : sip_t* response
             char** address
             int* portnum
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月3日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int response_get_destination(osip_message_t* response, char** address, int* portnum)
{
    osip_via_t* via = NULL;
    char* host = NULL;
    int port = 0;

    *address = NULL;

    if (NULL == response)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "response_get_destination() exit---: Param Error \r\n");
        return -1;
    }

    if (MSG_IS_REQUEST(response))
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "response_get_destination() exit---: MSG_IS_REQUEST \r\n");
        return -1;
    }

    via = (osip_via_t*) osip_list_get(&response->vias, 0);

    if (via)
    {
        osip_generic_param_t* maddr = NULL;
        osip_generic_param_t* received = NULL;
        osip_generic_param_t* rport = NULL;
        osip_via_param_get_byname(via, (char*)"maddr", &maddr);
        osip_via_param_get_byname(via, (char*)"received", &received);
        osip_via_param_get_byname(via, (char*)"rport", &rport);

        /* 1: user should not use the provided information
                (host and port) if they are using a reliable
                transport. Instead, they should use the already
                open socket attached to this transaction. */
        /* 2: check maddr and multicast usage */
        if (maddr != NULL)
        {
            host = maddr->gvalue;
        }
        /* we should check if this is a multicast address and use
                    set the "ttl" in this case. (this must be done in the
                UDP message (not at the SIP layer) */
        else if (received != NULL)
        {
            host = received->gvalue;
        }
        else
        {
            host = via->host;
        }

        if (rport == NULL || rport->gvalue == NULL)
        {
            if (via->port != NULL)
            {
                port = osip_atoi(via->port);
            }
            else
            {
                port = 5060;
            }
        }
        else
        {
            port = osip_atoi(rport->gvalue);
        }
    }

    *portnum = port;

    if (host == NULL)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "response_get_destination() exit---: Host NULL \r\n");
        return -1;
    }

    *address = osip_getcopy(host);

    return 0;
}

/*****************************************************************************
 函 数 名  : request_get_destination
 功能描述  : 从请求消息中获取目的信息
 输入参数  : sip_t* request
             char** address
             int* portnum
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月3日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int request_get_destination(osip_message_t* request, char** address, int* portnum)
{
    int port = 0;
    char* dest = NULL;
    osip_route_t* route = NULL;

    *address = NULL;

    if (NULL == request)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "request_get_destination() exit---: Param Error \r\n");
        return -1;
    }

    if (MSG_IS_RESPONSE(request))
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "request_get_destination() exit---: MSG_IS_RESPONSE \r\n");
        return -1;
    }

    osip_message_get_route(request, 0, &route);

    if (route != NULL)
    {
        port = 5060;

        if (route->url->port != NULL)
        {
            port = osip_atoi(route->url->port);
        }

        dest = osip_getcopy(route->url->host);
    }
    else
    {
        port = 5060;

        if (request->req_uri->port != NULL)
        {
            port = osip_atoi(request->req_uri->port);
        }

        dest = osip_getcopy(request->req_uri->host);
    }

    *address = dest;
    *portnum = port;

    return 0;
}

/*****************************************************************************
 函 数 名  : generating_digest_auth
 功能描述  : 生成认证信息
 输入参数  : authorization_t *authorization
             char * password
             char *method
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月3日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int generating_digest_auth(osip_authorization_t* authorization, char* password, char* method)
{
    char* pszNonce = NULL;
    char* pszCNonce = NULL;
    char* pszUser = NULL;
    char* pszRealm = NULL;
    char* pszPass = NULL;
    char* pszAlg = NULL;
    char* pszNonceCount = NULL;
    char* pszMethod = NULL;
    char* pszQop = NULL;
    char* pszURI = NULL;
    HASHHEX HA1;
    HASHHEX HA2;
    HASHHEX Response;
    int len = 0;

    pszPass = password;
    pszMethod = method;
    pszNonce = osip_getcopy_unquoted_string(authorization->nonce);
    pszUser = osip_getcopy_unquoted_string(authorization->username);
    pszRealm = osip_getcopy_unquoted_string(authorization->realm);
    pszURI = osip_getcopy_unquoted_string(authorization->uri);
    pszAlg = osip_getcopy("md5");

    if (authorization->nonce_count != NULL)
    {
        pszNonceCount = osip_getcopy(authorization->nonce_count);
    }

    if (authorization->message_qop != NULL)
    {
        pszQop = osip_getcopy(authorization->message_qop);
    }

    if (authorization->cnonce != NULL)
    {
        pszCNonce = osip_getcopy_unquoted_string(authorization->cnonce);
    }

    DigestCalcHA1(pszAlg, pszUser, pszRealm, pszPass, pszNonce, pszCNonce, HA1);
    DigestCalcResponse(HA1, pszNonce, pszNonceCount, pszCNonce, pszQop, pszMethod, pszURI, HA2, Response);

    len = strlen(Response) + 3;
    authorization->response = (char*)osip_malloc(len);

    if (NULL != authorization->response)
    {
        snprintf(authorization->response, len, "\"%s\"", Response);
    }


    osip_free(pszUser);
    pszUser = NULL;
    osip_free(pszRealm);
    pszRealm = NULL;
    osip_free(pszNonce);
    pszNonce = NULL;
    osip_free(pszAlg);
    pszAlg = NULL;
    osip_free(pszQop);
    pszQop = NULL;
    osip_free(pszURI);
    pszURI = NULL;

    return 0;
}

/*****************************************************************************
 函 数 名  : request_add_proxy_authorization
 功能描述  : 在请求消息中加入proxy认证信息
 输入参数  : sip_t *request
             proxy_authenticate_t *auth
             char *user
             char *password
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月3日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int request_add_proxy_authorization(osip_message_t* request, osip_proxy_authenticate_t* auth, char* user, char* password)
{
    char* tmp = NULL;
    char* method = NULL;
    char csUser[128] = {0};
    char csUri[128] = {0};
    char csCNonce[128] = {0};
    osip_proxy_authorization_t* authorization = NULL;

    if (NULL == request || NULL == auth)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "request_add_proxy_authorization() exit---: Param Error \r\n");
        return -1;
    }

    stolowercase(auth->auth_type); //2004/11/25

    if (sstrcmp(auth->auth_type, "digest") != 0)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "request_add_proxy_authorization() exit---: Auth Type Is Not Support \r\n");
        return -2;
    }

    tmp = osip_getcopy(auth->algorithm);

    if (tmp != NULL)
    {
        stolowercase(tmp);

        if (sstrcmp(tmp , "md5") != 0)
        {
            osip_free(tmp);
            tmp = NULL;
            SIP_DEBUG_TRACE(LOG_DEBUG, "request_add_proxy_authorization() exit---: Auth Method Is Not Support \r\n");
            return -3;
        }
    }

    osip_free(tmp);
    tmp = NULL;

    osip_proxy_authorization_init(&authorization);

    if (auth->qop_options)
    {
        tmp = osip_getcopy_unquoted_string(auth->qop_options);

        if (strstr(tmp, "auth") != NULL)
        {
            authorization->message_qop = osip_getcopy("auth");
        }
        else if (strstr(tmp, "auth-int") != NULL)
        {
            authorization->message_qop = osip_getcopy("auth-int");
        }
        else
        {
            osip_free(tmp);
            tmp = NULL;
            osip_proxy_authorization_free(authorization);
            authorization = NULL;
            SIP_DEBUG_TRACE(LOG_DEBUG, "request_add_proxy_authorization() exit---: Auth Options Is Not Support \r\n");
            return -4;
        }

        osip_free(tmp);
        tmp = NULL;
        tmp = new_to_tag();
        snprintf(csCNonce, 128, "\"%s\"", tmp);
        osip_free(tmp);
        tmp = NULL;
        authorization->cnonce = osip_getcopy(csCNonce);
        authorization->nonce_count = osip_getcopy("00000002");
    }

    authorization->auth_type = osip_getcopy("Digest");
    snprintf(csUser, 128, "\"%s\"", user);
    authorization->username = osip_getcopy(csUser);
    authorization->realm = osip_getcopy(auth->realm);
    authorization->nonce = osip_getcopy(auth->nonce);
    authorization->opaque = osip_getcopy(auth->opaque);
    tmp = osip_getcopy_unquoted_string(auth->realm);  /*??? */
    snprintf(csUri, 128, "\"%s\"", tmp);
    osip_free(tmp);
    tmp = NULL;
    authorization->uri = osip_getcopy(csUri);
    authorization->algorithm = osip_getcopy("md5");
    method = osip_message_get_method(request);
    generating_digest_auth(authorization, password, method);
    osip_list_add((osip_list_t*)&request->proxy_authorizations, authorization, -1);

    return 0;
}

/*****************************************************************************
 函 数 名  : request_add_authorization
 功能描述  : 在请求消息中添加www认证信息
 输入参数  : sip_t *request
             www_authenticate_t *auth
             char *user
             char *password
             char *proxy
             int proxy_port
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月3日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int request_add_authorization(osip_message_t* request, osip_www_authenticate_t* auth, char* user, char* password, char* proxy, int proxy_port)
{
    char* tmp = NULL, *method = NULL;
    char csUser[128] = {0};
    char csUri[128] = {0};
    char csCNonce[128] = {0};
    osip_authorization_t* authorization = NULL;

    if (NULL == request || MSG_IS_RESPONSE(request) || NULL == auth)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "request_add_authorization() exit---: Param Error \r\n");
        return -1;
    }

    stolowercase(auth->auth_type); //2004/11/25

    if (sstrcmp(auth->auth_type, "digest") != 0)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "request_add_authorization() exit---: Auth Type Is Not Support \r\n");
        return -2;
    }

    tmp = osip_getcopy(auth->algorithm);

    if (tmp != NULL)
    {
        stolowercase(tmp);

        if (sstrcmp(tmp , "md5") != 0)
        {
            osip_free(tmp);
            tmp = NULL;
            SIP_DEBUG_TRACE(LOG_DEBUG, "request_add_authorization() exit---: Auth Method Is Not Support \r\n");
            return -3;
        }
    }

    osip_free(tmp);
    tmp = NULL;

    osip_authorization_init(&authorization);

    if (auth->qop_options)
    {
        tmp = osip_getcopy_unquoted_string(auth->qop_options);

        if (strstr(tmp, "auth") != NULL)
        {
            authorization->message_qop = osip_getcopy("auth");
        }
        else if (strstr(tmp, "auth-int") != NULL)
        {
            authorization->message_qop = osip_getcopy("auth-int");
        }
        else
        {
            osip_free(tmp);
            tmp = NULL;
            osip_authorization_free(authorization);
            authorization = NULL;
            SIP_DEBUG_TRACE(LOG_DEBUG, "request_add_authorization() exit---: Auth Options Is Not Support \r\n");
            return -4;
        }

        osip_free(tmp);
        tmp = NULL;
        tmp = new_to_tag();
        snprintf(csCNonce, 128, "\"%s\"", tmp);
        osip_free(tmp);
        tmp = NULL;
        authorization->cnonce = osip_getcopy(csCNonce);
        authorization->nonce_count = osip_getcopy("00000002");
    }

    authorization->auth_type = osip_getcopy("Digest");
    snprintf(csUser, 128, "\"%s\"", user);
    authorization->username = osip_getcopy(csUser);
    authorization->realm = osip_getcopy(auth->realm);
    authorization->nonce = osip_getcopy(auth->nonce);
    authorization->opaque = osip_getcopy(auth->opaque);

    snprintf(csUri, 128, "\"%s\"", proxy);
    authorization->uri = osip_getcopy(csUri);
    authorization->algorithm = osip_getcopy("md5");
    method = osip_message_get_method(request);
    generating_digest_auth(authorization, password, method);
    osip_list_add((osip_list_t*)&request->authorizations, authorization, -1);

    return 0;
}

/*****************************************************************************
 函 数 名  : cs_response_add_www_authenticate
 功能描述  : 服务端添加www认证信息
 输入参数  : sip_t * resp
                            char* localhost
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月3日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void cs_response_add_www_authenticate(osip_message_t* resp, char* localhost)
{
    int i = 0;
    int len = 0;
    char* tmp = NULL;
    char* realm_tmp = NULL;
    char* nonce_tmp = NULL;
    osip_www_authenticate_t* www_authenticate = NULL;

    if (NULL == localhost)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "cs_response_add_www_authenticate() exit---: Local Host Error \r\n");
        return;
    }

    if (resp == NULL || MSG_IS_REQUEST(resp))
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "cs_response_add_www_authenticate() exit---: Param Error \r\n");
        return;
    }

    i = osip_www_authenticate_init(&www_authenticate);

    if (i == -1)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "cs_response_add_www_authenticate() exit---: WWW Authenticate Init Error \r\n");
        return;
    }

    osip_www_authenticate_set_auth_type(www_authenticate, osip_getcopy("Digest"));
    osip_www_authenticate_set_stale(www_authenticate, osip_getcopy("FALSE"));
    osip_www_authenticate_set_algorithm(www_authenticate, osip_getcopy("MD5"));

    len = strlen(localhost) + 3;
    realm_tmp = (char*)osip_malloc(len);

    if (NULL == realm_tmp)
    {
        osip_www_authenticate_free(www_authenticate);
        www_authenticate = NULL;
        return;
    }

    snprintf(realm_tmp, len, "\"%s\"", localhost);
    osip_www_authenticate_set_realm(www_authenticate, realm_tmp);

    tmp = nonce_new_random();
    len = strlen(tmp) + 3;
    nonce_tmp = (char*)osip_malloc(len);

    if (NULL == nonce_tmp)
    {
        osip_www_authenticate_free(www_authenticate);
        www_authenticate = NULL;
        osip_free(realm_tmp);
        realm_tmp = NULL;
        return;
    }

    snprintf(nonce_tmp, len, "\"%s\"", tmp);
    osip_www_authenticate_set_nonce(www_authenticate, nonce_tmp);
    osip_free(tmp);
    tmp = NULL;

    osip_list_add((osip_list_t*)&resp->www_authenticates, www_authenticate, -1);

    return;
}

/*****************************************************************************
 函 数 名  : cs_response_add_proxy_authenticate
 功能描述  : 服务端添加proxy认证信息
 输入参数  : sip_t * resp
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月3日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void cs_response_add_proxy_authenticate(osip_message_t* resp, char* localhost)
{
    int i = 0;
    int len = 0;
    char* tmp = NULL;
    char* realm_tmp = NULL;
    char* nonce_tmp = NULL;
    osip_proxy_authenticate_t* proxy_authenticate = NULL;

    if (NULL == localhost)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "cs_response_add_proxy_authenticate() exit---: Local Host NULL \r\n");
        return;
    }

    if (resp == NULL || MSG_IS_REQUEST(resp))
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "cs_response_add_proxy_authenticate() exit---: Param Error \r\n");
        return;
    }

    i = osip_proxy_authenticate_init(&proxy_authenticate);

    if (i == -1)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "cs_response_add_proxy_authenticate() exit---: Proxy Authenticate Init Error \r\n");
        return;
    }

    osip_proxy_authenticate_set_auth_type(proxy_authenticate, osip_getcopy("Digest"));
    osip_proxy_authenticate_set_stale(proxy_authenticate, osip_getcopy("FALSE"));
    osip_proxy_authenticate_set_algorithm(proxy_authenticate, osip_getcopy("MD5"));

    len = strlen(localhost) + 3;
    realm_tmp = (char*)osip_malloc(len);

    if (NULL == realm_tmp)
    {
        osip_proxy_authenticate_free(proxy_authenticate);
        proxy_authenticate = NULL;
        return;
    }

    snprintf(realm_tmp, len, "\"%s\"", localhost);
    osip_proxy_authenticate_set_realm(proxy_authenticate, realm_tmp);

    tmp = nonce_new_random();
    len = strlen(tmp) + 3;
    nonce_tmp = (char*)osip_malloc(len);

    if (NULL == nonce_tmp)
    {
        osip_proxy_authenticate_free(proxy_authenticate);
        proxy_authenticate = NULL;
        osip_free(realm_tmp);
        realm_tmp = NULL;
        return;
    }

    snprintf(nonce_tmp, len, "\"%s\"", tmp);
    osip_proxy_authenticate_set_nonce(proxy_authenticate, nonce_tmp);
    osip_free(tmp);
    tmp = NULL;

    osip_list_add((osip_list_t*)&resp->proxy_authenticates, proxy_authenticate, -1);

    return;
}
#endif

#if DECS("请求消息")
/*****************************************************************************
 函 数 名  : get_registrator
 功能描述  : 生成registrator结构
 输入参数  : char **registrator
             char* proxyid
             char *proxyip
             int proxyport
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月3日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int get_registrator(char** registrator, char* proxyid, char* proxyip, int proxyport)
{
    *registrator = NULL;

    if (NULL == proxyid || NULL == proxyip || proxyport <= 0)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "get_registrator() exit---: Param Error \r\n");
        return -1;
    }

    *registrator = (char*)osip_malloc(strlen(proxyid) + strlen(proxyip) + 20);

    if (NULL == *registrator)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "get_registrator() exit---: *registrator Smalloc Error \r\n");
        return -1;
    }

    snprintf(*registrator, strlen(proxyid) + strlen(proxyip) + 20, "sip:%s@%s:%u", proxyid, proxyip, proxyport);

    return 0;
}

/*****************************************************************************
 函 数 名  : get_contact
 功能描述  : 生成contact结构
 输入参数  : char **contact
                            char *username
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
int get_contact(char** contact, char* username, char* localip, int localport)
{
    *contact = NULL;

    if (NULL == localip || NULL == username)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "get_contact() exit---: Param Error \r\n");
        return -1;
    }

    *contact = (char*)osip_malloc(strlen(localip) + strlen(username) + 25);

    if (NULL == *contact)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "get_contact() exit---: *contact Smalloc Error \r\n");
        return -1;
    }

    if (localport == 0)
    {
        localport = 5060;
    }

    if (localport > 65535)
    {
        localport = 65535;
    }

    snprintf(*contact, strlen(localip) + strlen(username) + 25, "sip:%s@%s:%u", username, localip, localport);

    return 0;
}

/*****************************************************************************
 函 数 名  : get_from
 功能描述  : 生成from结构
 输入参数  : char **from
                            char *username
                            char *proxy
                            int proxy_port
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月3日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int get_from(char** from, char* username, char* proxyip, int proxyport)
{
    *from = NULL;

    if ((NULL == username) || (NULL == proxyip))
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "get_from() exit---: Param Error \r\n");
        return -1;
    }

    if (proxyport == 0)
    {
        proxyport = 5060;
    }

    if (proxyport > 65535)
    {
        proxyport = 65535;
    }

    if (proxyport == 5060)
    {
        *from = (char*)osip_malloc(strlen(username) + strlen(proxyip) + 6);
    }
    else
    {
        *from = (char*)osip_malloc(strlen(username) + strlen(proxyip) + 12);
    }

    if (NULL == *from)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "get_from() exit---: *from Smalloc Error \r\n");
        return -1;
    }

    if (proxyport == 0 || proxyport == 5060)
    {
        snprintf(*from, strlen(username) + strlen(proxyip) + 6, "sip:%s@%s", username, proxyip);
    }
    else
    {
        snprintf(*from, strlen(username) + strlen(proxyip) + 12, "sip:%s@%s:%u", username, proxyip, proxyport);
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : get_to
 功能描述  : 生成to结构
 输入参数  : char **to
                            char *username
                            char *proxyip
                            int proxyport
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月3日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int get_to(char** to, char* username, char* proxyip, int proxyport)
{
    int len = 0;
    *to = NULL;

    if ((username == NULL) || (proxyip == NULL))
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "get_to() exit---: Param Error \r\n");
        return -1;
    }

    if (proxyport == 0)
    {
        proxyport = 5060;
    }

    if (proxyport > 65535)
    {
        proxyport = 65535;
    }

    if (proxyport == 5060)
    {
        len = strlen(username) + strlen(proxyip) + 6;
        *to = (char*)osip_malloc(len);

        if (NULL == *to)
        {
            return -1;
        }

        snprintf(*to, len, "sip:%s@%s", username, proxyip);
    }
    else
    {
        len = strlen(username) + strlen(proxyip) + 12;
        *to = (char*)osip_malloc(len);

        if (NULL == *to)
        {
            return -1;
        }

        snprintf(*to, len, "sip:%s@%s:%d", username, proxyip, proxyport);
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : dialog_fill_route_set
 功能描述  : 在sip dialog中填充route set
 输入参数  : sip_dialog_t *dialog
             sip_t *request
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月3日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int dialog_fill_route_set(sip_dialog_t* dialog, osip_message_t* request)
{
    /* if the pre-existing route set contains a "lr" (compliance
    with bis-08) then the rquri should contains the remote target
        URI */
    int i = 0;
    int pos =  -1;
    osip_uri_param_t* lr_param = NULL;
    osip_route_t* route = NULL;
    char* last_route = NULL;

    if (dialog->route_set == NULL)
    {
        if (dialog->remote_contact_uri == NULL)
        {
            SIP_DEBUG_TRACE(LOG_DEBUG, "dialog_fill_route_set() exit---: Param Error \r\n");
            return -1;
        }

        i = osip_uri_clone(dialog->remote_contact_uri->url, &(request->req_uri));

        if (i != 0)
        {
            SIP_DEBUG_TRACE(LOG_DEBUG, "dialog_fill_route_set() exit---: URL Clone Error \r\n");
            return -1;
        }

        if (0 == sstrcmp(request->sip_method, (char*)"ACK"))
        {
            //printf("\r\n dialog_fill_route_set:dialog->remote_contact_uri->url->host=%s, port=%s \r\n", dialog->remote_contact_uri->url->host, dialog->remote_contact_uri->url->port);
            //printf("\r\n dialog_fill_route_set:request->req_uri->host=%s, port=%s \r\n", request->req_uri->host, request->req_uri->port);
        }

        return 0;
    }

    if (dialog->type1 == DLG_CALLER)
    {
        pos = osip_list_size(dialog->route_set) - 1;
        route = (osip_route_t*)osip_list_get(dialog->route_set, pos);  /* last */
    }
    else
    {
        route = (osip_route_t*)osip_list_get(dialog->route_set, 0);    /* first */
    }

    if (route == NULL)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "dialog_fill_route_set() exit---: Get Route Error \r\n");
        return -1;
    }

    osip_uri_uparam_get_byname(route->url, (char*)"lr", &lr_param);

    if (lr_param != NULL) /* the remote target URI is the rquri! */
    {
        if (dialog->remote_contact_uri == NULL)
        {
            SIP_DEBUG_TRACE(LOG_DEBUG, "dialog_fill_route_set() exit---: Get Remote Contact URL Error \r\n");
            return -1;
        }

        i = osip_uri_clone(dialog->remote_contact_uri->url, &(request->req_uri));

        if (i != 0)
        {
            SIP_DEBUG_TRACE(LOG_DEBUG, "dialog_fill_route_set() exit---: URL Clone Error \r\n");
            return -1;
        }

        /* "[request] MUST includes a Route header field containing
        the route set values in order." */
        /* AMD bug: fixed 17/06/2002 */
        pos = 0; /* first element is at index 0 */

        while (!osip_list_eol(dialog->route_set, pos))
        {
            osip_route_t* route2 = NULL;
            route = (osip_route_t*)osip_list_get(dialog->route_set, pos);

            if (NULL == route)
            {
                pos++;
                continue;
            }

            i = osip_route_clone(route, &route2);

            if (i != 0)
            {
                SIP_DEBUG_TRACE(LOG_DEBUG, "dialog_fill_route_set() exit---: Route Clone Error \r\n");
                return -1;
            }

            if (dialog->type1 == DLG_CALLER)
            {
                osip_list_add((osip_list_t*)&request->routes, route2, 0);
            }
            else
            {
                osip_list_add((osip_list_t*)&request->routes, route2, -1);
            }

            pos++;
        }

        return 0;
    }

    /* if the first URI of route set does not contain "lr", the rquri
    is set to the first uri of route set */

    i = osip_uri_clone(route->url, &(request->req_uri));

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "dialog_fill_route_set() exit---: URL Clone Error \r\n");
        return -1;
    }

    /* add the route set */
    /* "The UAC MUST add a route header field containing
    the remainder of the route set values in order. */
    pos = 0; /* yes it is */

    while (dialog->route_set != NULL && !osip_list_eol(dialog->route_set, pos)) /* not the first one in the list */
    {
        osip_route_t* route2 = NULL;
        route = (osip_route_t*)osip_list_get(dialog->route_set, pos);

        if (NULL == route)
        {
            pos++;
            continue;
        }

        i = osip_route_clone(route, &route2);

        if (i != 0)
        {
            SIP_DEBUG_TRACE(LOG_DEBUG, "dialog_fill_route_set() exit---: Route Clone Error \r\n");
            return -1;
        }

        if (dialog->type1 == DLG_CALLER)
        {
            if (pos != 0)
            {
                osip_list_add(&request->routes, route2, 0);
            }
        }
        else
        {
            if (!osip_list_eol(dialog->route_set, pos + 1))
            {
                osip_list_add(&request->routes, route2, -1);
            }
        }

        pos++;
    }

    /* The UAC MUST then place the remote target URI into
    the route header field as the last value */
    i = osip_uri_to_str(dialog->remote_contact_uri->url, &last_route);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "dialog_fill_route_set() exit---: URL 2 Char Error \r\n");
        return -1;
    }

    i = osip_message_set_route(request, last_route);

    if (i != 0)
    {
        osip_free(last_route);
        last_route = NULL;
        SIP_DEBUG_TRACE(LOG_DEBUG, "dialog_fill_route_set() exit---: Message Set Route Error \r\n");
        return -1;
    }

    /* route header and rquri set */
    return 0;
}

/*****************************************************************************
 函 数 名  : generating_request_out_of_dialog
 功能描述  : 会话外产生请求消息
 输入参数  : sip_t **dest
                            char *method_name
                            char *from
                            char *to
                            char *route
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
int generating_request_out_of_dialog(osip_message_t** dest, char* method_name, char* from, char* to, char* route, char* localip, int localport, char* registrator, char* call_id, int register_cseq_number)
{
    /* Section 8.1:
    A valid request contains at a minimum "To, From, Call-iD, Cseq,
    Max-Forwards and Via
        */
    int i = 0;
    char* max_fwd = NULL;
    char* proxy_require = NULL;
    osip_message_t* request = NULL;

    i = osip_message_init(&request);

    if (0 != i)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "generating_request_out_of_dialog() exit---: Message Init Error \r\n");
        return -1;
    }

    /* prepare the request-line */
    request->sip_method  = osip_getcopy(method_name);
    request->sip_version = osip_getcopy(SIP_VERSION);
    request->status_code   = 0;
    request->reason_phrase = NULL;

    if (0 == strncmp("REGISTER", method_name, 9))
    {
        if (NULL == registrator)
        {
            SIP_DEBUG_TRACE(LOG_ERROR, "generating_request_out_of_dialog() exit---: Registrator NULL \r\n");
            goto brood_error_1;
        }

        osip_uri_init(&(request->req_uri));
        osip_uri_parse(request->req_uri, registrator);

        i = osip_message_set_to(request, from);

        if (i != 0)
        {
            SIP_DEBUG_TRACE(LOG_ERROR, "generating_request_out_of_dialog() exit---: Message Set To Error:to=%s \r\n", from);
            goto brood_error_1;
        }

        if (route != NULL)
        {
            /* equal to a pre-existing route set */
            /* if the pre-existing route set contains a "lr" (compliance
            with bis-08) then the rquri should contains the remote target
             URI */
            osip_uri_param_t* lr_param = NULL;
            osip_route_t* o_proxy = NULL;
            osip_route_init(&o_proxy);
            osip_route_parse(o_proxy, route);
            osip_uri_uparam_get_byname(o_proxy->url, (char*)"lr", &lr_param);

            if (lr_param != NULL) /* to is the remote target URI in this case! */
            {
                osip_list_add(&request->routes, o_proxy, 0);
            }
            else
            {
                osip_route_free(o_proxy);
                o_proxy = NULL;
            }
        }
    }
    else
    {
        /* in any cases except REGISTER: */
        i = osip_message_set_to(request, to);

        if (i != 0)
        {
            SIP_DEBUG_TRACE(LOG_ERROR, "generating_request_out_of_dialog() exit---: Message Set To Error:to=%s \r\n", to);
            goto brood_error_1;
        }

        if (route != NULL)
        {
            /* equal to a pre-existing route set */
            /* if the pre-existing route set contains a "lr" (compliance
            with bis-08) then the rquri should contains the remote target
             URI */
            osip_uri_param_t* lr_param = NULL;
            osip_route_t* o_proxy = NULL;
            osip_route_init(&o_proxy);
            osip_route_parse(o_proxy, route);
            osip_uri_uparam_get_byname(o_proxy->url, (char*)"lr", &lr_param);

            if (lr_param != NULL) /* to is the remote target URI in this case! */
            {
                osip_uri_clone(request->to->url, &(request->req_uri));
                /* "[request] MUST includes a Route header field containing
                the route set values in order." */
                osip_list_add(&request->routes, o_proxy, 0);
            }
            else
                /* if the first URI of route set does not contain "lr", the rquri
                is set to the first uri of route set */
            {
                request->req_uri = o_proxy->url;
                o_proxy->url = NULL;
                osip_route_free(o_proxy);
                o_proxy = NULL;
                /* add the route set */
                /* "The UAC MUST add a route header field containing
                the remainder of the route set values in order.
                The UAC MUST then place the remote target URI into
                the route header field as the last value
                */
                osip_message_set_route(request, to);
            }
        }
        else /* No route set (outbound proxy) is used */
        {
            /* The UAC must put the remote target URI (to field) in the rquri */
            i = osip_uri_clone(request->to->url, &(request->req_uri));

            if (i != 0)
            {
                SIP_DEBUG_TRACE(LOG_ERROR, "generating_request_out_of_dialog() exit---: URL Clone Error \r\n");
                goto brood_error_1;
            }
        }
    }

    /* set To and From */
    i = osip_message_set_from(request, from);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "generating_request_out_of_dialog() exit---: Message Set From Error \r\n");
        goto brood_error_1;
    }

    /* add a tag */
    osip_from_set_tag(request->from, from_tag_new_random());

    /* set the cseq and call_id header */
    if (0 == sstrcmp("REGISTER", method_name))
    {
        osip_call_id_t* callid = NULL;
        osip_cseq_t* cseq = NULL;
        char* num = NULL;

        /* call-id is always the same for REGISTRATIONS */
        i = osip_call_id_init(&callid);

        if (i != 0)
        {
            SIP_DEBUG_TRACE(LOG_ERROR, "generating_request_out_of_dialog() exit---: Call ID Init Error \r\n");
            goto brood_error_1;
        }

        if (NULL != call_id)
        {
            osip_call_id_set_number(callid, osip_getcopy(call_id));
        }
        else
        {
            osip_call_id_set_number(callid, call_id_new_random());
        }

        osip_call_id_set_host(callid, osip_getcopy(localip));
        request->call_id = callid;

        i = osip_cseq_init(&cseq);

        if (i != 0)
        {
            SIP_DEBUG_TRACE(LOG_ERROR, "generating_request_out_of_dialog() exit---: Cseq Init Error \r\n");
            goto brood_error_1;
        }

        num = (char*)osip_malloc(20);  /* should never be more than 10 chars... */

        if (NULL == num)
        {
            goto brood_error_1;
        }

        snprintf(num, 20, "%d", register_cseq_number);
        osip_cseq_set_number(cseq, num);
        osip_cseq_set_method(cseq, osip_getcopy(method_name));
        request->cseq = cseq;
    }
    else
    {
        /* set the call-id */
        osip_call_id_t* callid = NULL;
        osip_cseq_t* cseq = NULL;
        i = osip_call_id_init(&callid);

        if (i != 0)
        {
            SIP_DEBUG_TRACE(LOG_ERROR, "generating_request_out_of_dialog() exit---: Call ID Init Error \r\n");
            goto brood_error_1;
        }

        if (NULL != call_id)
        {
            osip_call_id_set_number(callid, osip_getcopy(call_id));
        }
        else
        {
            osip_call_id_set_number(callid, call_id_new_random());
        }

        osip_call_id_set_host(callid, osip_getcopy(localip));
        request->call_id = callid;

        i = osip_cseq_init(&cseq);

        if (i != 0)
        {
            SIP_DEBUG_TRACE(LOG_ERROR, "generating_request_out_of_dialog() exit---: Cseq Init Error \r\n");
            goto brood_error_1;
        }

        osip_cseq_set_number(cseq, osip_getcopy("20")); /* always start with 20... :-> */
        osip_cseq_set_method(cseq, osip_getcopy(method_name));
        request->cseq = cseq;
    }

    /* always add the Max-Forward header */
    //  max_fwd = josua_config_get_element("max_forward");
    max_fwd = NULL;

    if (max_fwd == NULL)
    {
        osip_message_set_max_forwards(request, (char*)"70"); /* a UA should start a request with 70 */
    }
    else
    {
        osip_message_set_max_forwards(request, max_fwd);
    }

    /* Add Proxy-Require Features */
    //  proxy_require = josua_config_get_element("proxy_require");
    proxy_require = NULL;

    if (proxy_require != NULL)
    {
        osip_message_set_proxy_require(request, proxy_require);
    }

    {
        osip_via_t* via = NULL;
        char* tmp = (char*)osip_malloc(90 * sizeof(char));

        if (NULL == tmp)
        {
            goto brood_error_1;
        }

        snprintf(tmp, 90 * sizeof(char), "%s/%s %s:%u;branch=z9hG4bK%u", SIP_VERSION, "UDP",
                 localip,
                 localport,
                 via_branch_new_random());
        osip_message_set_via(request, tmp);
        osip_free(tmp);
        tmp = NULL;
        via = (osip_via_t*)osip_list_get(&request->vias, 0);

        if (via == NULL)
        {
            SIP_DEBUG_TRACE(LOG_DEBUG, "generating_request_out_of_dialog() exit---: Get Via Error \r\n");
            goto brood_error_1;
        }

        osip_generic_param_add(&via->via_params, osip_getcopy("rport"), NULL);
    }

    /* add specific headers for each kind of request... */

    osip_message_set_user_agent(request, (char*)SIPUA_VERSION);
    /*  else if ... */
    *dest = request;
    return 0;

brood_error_1:
    osip_message_free(request);
    request = NULL;
    *dest = NULL;
    return -1;
}

/*****************************************************************************
 函 数 名  : generating_request_out_of_dialog_for_tcp
 功能描述  : 会话外产生请求消息,基于TCP协议
 输入参数  : osip_message_t** dest
             char* method_name
             char* from
             char* to
             char* route
             char* localip
             int localport
             char* registrator
             char* call_id
             int register_cseq_number
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年2月21日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int generating_request_out_of_dialog_for_tcp(osip_message_t** dest, char* method_name, char* from, char* to, char* route, char* localip, int localport, char* registrator, char* call_id, int register_cseq_number)
{
    /* Section 8.1:
    A valid request contains at a minimum "To, From, Call-iD, Cseq,
    Max-Forwards and Via
        */
    int i = 0;
    char* max_fwd = NULL;
    char* proxy_require = NULL;
    osip_message_t* request = NULL;

    i = osip_message_init(&request);

    if (0 != i)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "generating_request_out_of_dialog_for_tcp() exit---: Message Init Error \r\n");
        return -1;
    }

    /* prepare the request-line */
    request->sip_method  = osip_getcopy(method_name);
    request->sip_version = osip_getcopy(SIP_VERSION);
    request->status_code   = 0;
    request->reason_phrase = NULL;

    if (0 == strncmp("REGISTER", method_name, 9))
    {
        if (NULL == registrator)
        {
            SIP_DEBUG_TRACE(LOG_ERROR, "generating_request_out_of_dialog_for_tcp() exit---: Registrator NULL \r\n");
            goto brood_error_1;
        }

        osip_uri_init(&(request->req_uri));
        osip_uri_parse(request->req_uri, registrator);

        i = osip_message_set_to(request, from);

        if (i != 0)
        {
            SIP_DEBUG_TRACE(LOG_ERROR, "generating_request_out_of_dialog_for_tcp() exit---: Message Set To Error:to=%s \r\n", from);
            goto brood_error_1;
        }

        if (route != NULL)
        {
            /* equal to a pre-existing route set */
            /* if the pre-existing route set contains a "lr" (compliance
            with bis-08) then the rquri should contains the remote target
             URI */
            osip_uri_param_t* lr_param = NULL;
            osip_route_t* o_proxy = NULL;
            osip_route_init(&o_proxy);
            osip_route_parse(o_proxy, route);
            osip_uri_uparam_get_byname(o_proxy->url, (char*)"lr", &lr_param);

            if (lr_param != NULL) /* to is the remote target URI in this case! */
            {
                osip_list_add(&request->routes, o_proxy, 0);
            }
            else
            {
                osip_route_free(o_proxy);
                o_proxy = NULL;
            }
        }
    }
    else
    {
        /* in any cases except REGISTER: */
        i = osip_message_set_to(request, to);

        if (i != 0)
        {
            SIP_DEBUG_TRACE(LOG_ERROR, "generating_request_out_of_dialog_for_tcp() exit---: Message Set To Error:to=%s \r\n", to);
            goto brood_error_1;
        }

        if (route != NULL)
        {
            /* equal to a pre-existing route set */
            /* if the pre-existing route set contains a "lr" (compliance
            with bis-08) then the rquri should contains the remote target
             URI */
            osip_uri_param_t* lr_param = NULL;
            osip_route_t* o_proxy = NULL;
            osip_route_init(&o_proxy);
            osip_route_parse(o_proxy, route);
            osip_uri_uparam_get_byname(o_proxy->url, (char*)"lr", &lr_param);

            if (lr_param != NULL) /* to is the remote target URI in this case! */
            {
                osip_uri_clone(request->to->url, &(request->req_uri));
                /* "[request] MUST includes a Route header field containing
                the route set values in order." */
                osip_list_add(&request->routes, o_proxy, 0);
            }
            else
                /* if the first URI of route set does not contain "lr", the rquri
                is set to the first uri of route set */
            {
                request->req_uri = o_proxy->url;
                o_proxy->url = NULL;
                osip_route_free(o_proxy);
                o_proxy = NULL;
                /* add the route set */
                /* "The UAC MUST add a route header field containing
                the remainder of the route set values in order.
                The UAC MUST then place the remote target URI into
                the route header field as the last value
                */
                osip_message_set_route(request, to);
            }
        }
        else /* No route set (outbound proxy) is used */
        {
            /* The UAC must put the remote target URI (to field) in the rquri */
            i = osip_uri_clone(request->to->url, &(request->req_uri));

            if (i != 0)
            {
                SIP_DEBUG_TRACE(LOG_ERROR, "generating_request_out_of_dialog_for_tcp() exit---: URL Clone Error \r\n");
                goto brood_error_1;
            }
        }
    }

    /* set To and From */
    i = osip_message_set_from(request, from);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "generating_request_out_of_dialog_for_tcp() exit---: Message Set From Error \r\n");
        goto brood_error_1;
    }

    /* add a tag */
    osip_from_set_tag(request->from, from_tag_new_random());

    /* set the cseq and call_id header */
    if (0 == sstrcmp("REGISTER", method_name))
    {
        osip_call_id_t* callid = NULL;
        osip_cseq_t* cseq = NULL;
        char* num = NULL;

        /* call-id is always the same for REGISTRATIONS */
        i = osip_call_id_init(&callid);

        if (i != 0)
        {
            SIP_DEBUG_TRACE(LOG_ERROR, "generating_request_out_of_dialog_for_tcp() exit---: Call ID Init Error \r\n");
            goto brood_error_1;
        }

        if (NULL != call_id)
        {
            osip_call_id_set_number(callid, osip_getcopy(call_id));
        }
        else
        {
            osip_call_id_set_number(callid, call_id_new_random());
        }

        osip_call_id_set_host(callid, osip_getcopy(localip));
        request->call_id = callid;

        i = osip_cseq_init(&cseq);

        if (i != 0)
        {
            SIP_DEBUG_TRACE(LOG_ERROR, "generating_request_out_of_dialog_for_tcp() exit---: Cseq Init Error \r\n");
            goto brood_error_1;
        }

        num = (char*)osip_malloc(20);  /* should never be more than 10 chars... */

        if (NULL == num)
        {
            goto brood_error_1;
        }

        snprintf(num, 20, "%d", register_cseq_number);
        osip_cseq_set_number(cseq, num);
        osip_cseq_set_method(cseq, osip_getcopy(method_name));
        request->cseq = cseq;
    }
    else
    {
        /* set the call-id */
        osip_call_id_t* callid = NULL;
        osip_cseq_t* cseq = NULL;
        i = osip_call_id_init(&callid);

        if (i != 0)
        {
            SIP_DEBUG_TRACE(LOG_ERROR, "generating_request_out_of_dialog_for_tcp() exit---: Call ID Init Error \r\n");
            goto brood_error_1;
        }

        if (NULL != call_id)
        {
            osip_call_id_set_number(callid, osip_getcopy(call_id));
        }
        else
        {
            osip_call_id_set_number(callid, call_id_new_random());
        }

        osip_call_id_set_host(callid, osip_getcopy(localip));
        request->call_id = callid;

        i = osip_cseq_init(&cseq);

        if (i != 0)
        {
            SIP_DEBUG_TRACE(LOG_ERROR, "generating_request_out_of_dialog_for_tcp() exit---: Cseq Init Error \r\n");
            goto brood_error_1;
        }

        osip_cseq_set_number(cseq, osip_getcopy("20")); /* always start with 20... :-> */
        osip_cseq_set_method(cseq, osip_getcopy(method_name));
        request->cseq = cseq;
    }

    /* always add the Max-Forward header */
    //  max_fwd = josua_config_get_element("max_forward");
    max_fwd = NULL;

    if (max_fwd == NULL)
    {
        osip_message_set_max_forwards(request, (char*)"70"); /* a UA should start a request with 70 */
    }
    else
    {
        osip_message_set_max_forwards(request, max_fwd);
    }

    /* Add Proxy-Require Features */
    //  proxy_require = josua_config_get_element("proxy_require");
    proxy_require = NULL;

    if (proxy_require != NULL)
    {
        osip_message_set_proxy_require(request, proxy_require);
    }

    {
        osip_via_t* via = NULL;
        char* tmp = (char*)osip_malloc(90 * sizeof(char));

        if (NULL == tmp)
        {
            goto brood_error_1;
        }

        snprintf(tmp, 90 * sizeof(char), "%s/%s %s:%u;branch=z9hG4bK%u", SIP_VERSION, "tcp",
                 localip,
                 localport,
                 via_branch_new_random());
        osip_message_set_via(request, tmp);
        osip_free(tmp);
        tmp = NULL;
        via = (osip_via_t*)osip_list_get(&request->vias, 0);

        if (via == NULL)
        {
            SIP_DEBUG_TRACE(LOG_DEBUG, "generating_request_out_of_dialog_for_tcp() exit---: Get Via Error \r\n");
            goto brood_error_1;
        }

        osip_generic_param_add(&via->via_params, osip_getcopy("rport"), NULL);
    }

    /* add specific headers for each kind of request... */

    osip_message_set_user_agent(request, (char*)SIPUA_VERSION);
    /*  else if ... */
    *dest = request;
    return 0;

brood_error_1:
    osip_message_free(request);
    request = NULL;
    *dest = NULL;
    return -1;
}

/*****************************************************************************
 函 数 名  : generating_request_within_dialog
 功能描述  : 会话内产生请求消息
 输入参数  : sip_t **dest
                            char *method_name
                            sip_dialog_t *dialog
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
int generating_request_within_dialog(osip_message_t** dest, char* method_name, sip_dialog_t* dialog, char* localip, int localport)
{
    int i = 0;
    osip_message_t* request = NULL;
    char* max_fwd = NULL;

    if (NULL == dialog || NULL == method_name || NULL == localip || localport <= 0)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "generating_request_within_dialog() exit---: Param Error \r\n");
        return -1;
    }

    *dest = NULL;
    i = osip_message_init(&request);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "generating_request_within_dialog() exit---: Message Init Error \r\n");
        return -1;
    }

    if (dialog->remote_contact_uri == NULL)
    {
        /* this dialog is probably not established! or the remote UA
              is not compliant with the latest RFC
            */
        osip_message_free(request);
        request = NULL;
        SIP_DEBUG_TRACE(LOG_ERROR, "generating_request_within_dialog() exit---: Dialog Remote contact URL Error \r\n");
        return -1;
    }

    /* prepare the request-line */
    request->sip_method  = osip_getcopy(method_name);
    request->sip_version = osip_getcopy(SIP_VERSION);
    request->status_code   = 0;
    request->reason_phrase = NULL;

    /* and the request uri???? */
    if (dialog->route_set != NULL && osip_list_eol(dialog->route_set, 0))
    {
        /* The UAC must put the remote target URI (to field) in the rquri */
        i = osip_uri_clone(dialog->remote_contact_uri->url, &(request->req_uri));

        if (i != 0)
        {
            SIP_DEBUG_TRACE(LOG_ERROR, "generating_request_within_dialog() exit---: URL Clone Error \r\n");
            goto grwd_error_1;
        }

        if (0 == sstrcmp(request->sip_method, (char*)"ACK"))
        {
            //printf("\r\n generating_request_within_dialog:dialog->remote_contact_uri->url->host=%s, port=%s \r\n", dialog->remote_contact_uri->url->host, dialog->remote_contact_uri->url->port);
            //printf("\r\n generating_request_within_dialog:request->req_uri->host=%s, port=%s \r\n", request->req_uri->host, request->req_uri->port);
        }
    }
    else
    {
        /* fill the request-uri, and the route headers. */
        dialog_fill_route_set(dialog, request);
    }

    if (NULL == dialog->remote_uri)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "generating_request_within_dialog() exit---: Dialog Remote URL Error \r\n");
        goto grwd_error_1;
    }

    /* To and From already contains the proper tag! */
    i = osip_to_clone(dialog->remote_uri, &(request->to));

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "generating_request_within_dialog() exit---: To Clone Error \r\n");
        goto grwd_error_1;
    }

    if (NULL == dialog->local_uri)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "generating_request_within_dialog() exit---: Dialog Local URL Error \r\n");
        goto grwd_error_1;
    }

    i = osip_from_clone(dialog->local_uri, &(request->from));

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "generating_request_within_dialog() exit---: From Clone Error \r\n");
        goto grwd_error_1;
    }

    /* set the cseq and call_id header */
    osip_message_set_call_id(request, dialog->call_id);

    if (0 == sstrcmp("ACK", method_name))
    {
        osip_cseq_t* cseq = NULL;
        char* tmp = NULL;
        i = osip_cseq_init(&cseq);

        if (i != 0)
        {
            SIP_DEBUG_TRACE(LOG_ERROR, "generating_request_within_dialog() exit---: Cseq Init Error \r\n");
            goto grwd_error_1;
        }

        tmp = (char*)osip_malloc(12);

        if (NULL == tmp)
        {
            goto grwd_error_1;
        }

        snprintf(tmp, 12, "%d", dialog->local_cseq);
        osip_cseq_set_number(cseq, tmp);
        osip_cseq_set_method(cseq, osip_getcopy(method_name));
        request->cseq = cseq;
    }
    else
    {
        osip_cseq_t* cseq = NULL;
        char* tmp = NULL;
        i = osip_cseq_init(&cseq);

        if (i != 0)
        {
            SIP_DEBUG_TRACE(LOG_ERROR, "generating_request_within_dialog() exit---: Cseq Init Error \r\n");
            goto grwd_error_1;
        }

        dialog->local_cseq++; /* we should we do that?? */
        tmp = (char*)osip_malloc(12);

        if (NULL == tmp)
        {
            goto grwd_error_1;
        }

        snprintf(tmp, 12, "%d", dialog->local_cseq);
        osip_cseq_set_number(cseq, tmp);
        osip_cseq_set_method(cseq, osip_getcopy(method_name));
        request->cseq = cseq;
    }

    /* always add the Max-Forward header */
//  max_fwd = josua_config_get_element("max_forward");
    max_fwd = NULL;

    if (max_fwd == NULL)
    {
        osip_message_set_max_forwards(request, (char*)"70"); /* a UA should start a request with 70 */
    }
    else
    {
        osip_message_set_max_forwards(request, max_fwd);
    }

    /* Add Proxy-Require Features */
//    proxy_require = josua_config_get_element("proxy_require");
//    if (proxy_require!=NULL)
//    {
//        msg_setproxy_require(request, proxy_require);
//    }

    /* even for ACK for 2xx (ACK within a dialog), the branch ID MUST
    be a new ONE! */
    {
        osip_via_t* via = NULL;
        char tmp[128] = {0};
        snprintf(tmp, 128, "%s/%s %s:%u;branch=z9hG4bK%u", SIP_VERSION, "UDP",
                 localip,
                 localport,
                 via_branch_new_random());

        osip_message_set_via(request, tmp);
        via = (osip_via_t*)osip_list_get(&request->vias, 0);

        if (via == NULL)
        {
            SIP_DEBUG_TRACE(LOG_ERROR, "generating_request_within_dialog() exit---: Message Set Via Error \r\n");
            goto grwd_error_1;
        }

        osip_generic_param_add(&via->via_params, osip_getcopy("rport"), NULL);
    }

    /* add specific headers for each kind of request... */


    osip_message_set_user_agent(request, (char*)SIPUA_VERSION);
    /*  else if ... */
    *dest = request;
    return 0;

    /* grwd_error_2: */
    dialog->local_cseq--;
grwd_error_1:
    osip_message_free(request);
    request = NULL;
    *dest = NULL;
    return -1;
}

/*****************************************************************************
 函 数 名  : generating_forward_request_within_dialog
 功能描述  : 会话内产生转发请求消息
 输入参数  : sip_t **dest
                            char *method_name
                            sip_dialog_t *dialog
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
int generating_forward_request_within_dialog(osip_message_t** dest, char* method_name, sip_dialog_t* dialog, char* localip, int localport)
{
    int i = 0;
    osip_message_t* request = NULL;
    char* max_fwd = NULL;

    if (NULL == dialog)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "generating_forward_request_within_dialog() exit---: Param Error \r\n");
        return -1;
    }

    *dest = NULL;
    i = osip_message_init(&request);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "generating_forward_request_within_dialog() exit---: Message Init Error \r\n");
        return -1;
    }

    if (dialog->remote_contact_uri == NULL)
    {
        /* this dialog is probably not established! or the remote UA
        is not compliant with the latest RFC
            */
        osip_message_free(request);
        request = NULL;
        SIP_DEBUG_TRACE(LOG_DEBUG, "generating_forward_request_within_dialog() exit---: Dialog Remote Contact URL Error \r\n");
        return -1;
    }

    /* prepare the request-line */
    request->sip_method  = osip_getcopy(method_name);
    request->sip_version = osip_getcopy(SIP_VERSION);
    request->status_code   = 0;
    request->reason_phrase = NULL;

    /* and the request uri???? */
    if (dialog->route_set != NULL && osip_list_eol(dialog->route_set, 0))
    {
        /* The UAC must put the remote target URI (to field) in the rquri */
        i = osip_uri_clone(dialog->remote_contact_uri->url, &(request->req_uri));

        if (i != 0)
        {
            SIP_DEBUG_TRACE(LOG_DEBUG, "generating_forward_request_within_dialog() exit---: URL Clone Error \r\n");
            goto grwd_error_1;
        }
    }
    else
    {
        /* fill the request-uri, and the route headers. */
        dialog_fill_route_set(dialog, request);
    }

    if (NULL == dialog->local_uri)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "generating_forward_request_within_dialog() exit---: Dialog Local URL Error \r\n");
        goto grwd_error_1;
    }

    /* To and From already contains the proper tag! */
    i = osip_to_clone(dialog->local_uri, &(request->to));

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "generating_forward_request_within_dialog() exit---: To Clone Error \r\n");
        goto grwd_error_1;
    }

    if (NULL == dialog->remote_uri)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "generating_forward_request_within_dialog() exit---: Dialog Remote URL Error \r\n");
        goto grwd_error_1;
    }

    i = osip_from_clone(dialog->remote_uri, &(request->from));

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "generating_forward_request_within_dialog() exit---: From Clone Error \r\n");
        goto grwd_error_1;
    }

    /* set the cseq and call_id header */
    osip_message_set_call_id(request, dialog->call_id);

    if (0 == sstrcmp("ACK", method_name))
    {
        osip_cseq_t* cseq = NULL;
        char* tmp = NULL;
        i = osip_cseq_init(&cseq);

        if (i != 0)
        {
            SIP_DEBUG_TRACE(LOG_DEBUG, "generating_forward_request_within_dialog() exit---: Cseq Init Error \r\n");
            goto grwd_error_1;
        }

        tmp = (char*)osip_malloc(12);

        if (NULL == tmp)
        {
            goto grwd_error_1;
        }

        snprintf(tmp, 12, "%d", dialog->local_cseq);
        osip_cseq_set_number(cseq, tmp);
        osip_cseq_set_method(cseq, osip_getcopy(method_name));
        request->cseq = cseq;
    }
    else
    {
        osip_cseq_t* cseq = NULL;
        char* tmp = NULL;
        i = osip_cseq_init(&cseq);

        if (i != 0)
        {
            SIP_DEBUG_TRACE(LOG_DEBUG, "generating_forward_request_within_dialog() exit---: Cseq Init Error \r\n");
            goto grwd_error_1;
        }

        dialog->local_cseq++; /* we should we do that?? */
        tmp = (char*)osip_malloc(12);

        if (NULL == tmp)
        {
            goto grwd_error_1;
        }

        snprintf(tmp, 12, "%d", dialog->local_cseq);
        osip_cseq_set_number(cseq, tmp);
        osip_cseq_set_method(cseq, osip_getcopy(method_name));
        request->cseq = cseq;
    }

    /* always add the Max-Forward header */
//  max_fwd = josua_config_get_element("max_forward");
    max_fwd = NULL;

    if (max_fwd == NULL)
    {
        osip_message_set_max_forwards(request, (char*)"70"); /* a UA should start a request with 70 */
    }
    else
    {
        osip_message_set_max_forwards(request, max_fwd);
    }

    /* Add Proxy-Require Features */
//    proxy_require = josua_config_get_element("proxy_require");
//    if (proxy_require!=NULL)
//    {
//        msg_setproxy_require(request, proxy_require);
//    }

    /* even for ACK for 2xx (ACK within a dialog), the branch ID MUST
    be a new ONE! */
    {
        osip_via_t* via = NULL;
        char tmp[128] = {0};
        snprintf(tmp, 128, "%s/%s %s:%u;branch=z9hG4bK%u", SIP_VERSION, "UDP",
                 localip,
                 localport,
                 via_branch_new_random());

        osip_message_set_via(request, tmp);
        via = (osip_via_t*)osip_list_get(&request->vias, 0);

        if (via == NULL)
        {
            SIP_DEBUG_TRACE(LOG_DEBUG, "generating_forward_request_within_dialog() exit---: Message Set Via Error \r\n");
            goto grwd_error_1;
        }

        osip_generic_param_add(&via->via_params, osip_getcopy("rport"), NULL);
    }

    /* add specific headers for each kind of request... */


    osip_message_set_user_agent(request, (char*)SIPUA_VERSION);
    /*  else if ... */
    *dest = request;
    return 0;

    /* grwd_error_2: */
    dialog->local_cseq--;
grwd_error_1:
    osip_message_free(request);
    request = NULL;
    *dest = NULL;
    return -1;
}


/*****************************************************************************
 函 数 名  : generating_request_fromrequest
 功能描述  : 根据老的请求消息生成新的请求消息
 输入参数  : sip_t *old_request
                            sip_t **new_request
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
int generating_request_fromrequest(osip_message_t* old_request, osip_message_t** new_request, char* localip, int localport)
{
    osip_message_t* request = NULL;
    char*  cseqnum = NULL;
    char*  _via = NULL;
    osip_via_t* via = NULL;
    int   num = 0;

    *new_request = NULL;

    if (NULL == old_request)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "generating_request_fromrequest() exit---: Param Error \r\n");
        return -1;
    }

    osip_message_clone(old_request, &request);

    if (NULL == request)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "generating_request_fromrequest() exit---: Generating Request Error \r\n");
        goto error1;
    }

    cseqnum = request->cseq->number;
    num = osip_atoi(cseqnum);
    osip_free(cseqnum);
    cseqnum = NULL;
    num++;
    cseqnum = (char*)osip_malloc(12);

    if (NULL == cseqnum)
    {
        goto error1;
    }

    snprintf(cseqnum, 12, "%u", num);
    request->cseq->number = cseqnum;
    osip_list_special_free(&request->vias, (void (*)(void*))&osip_via_free);

    if ((_via = (char*)osip_malloc(sizeof(char) * 100)) == NULL)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "generating_request_fromrequest() exit---: Via Malloc Error \r\n");
        goto error1;
    }

    snprintf(_via, sizeof(char) * 100, "%s/%s %s:%u;branch=z9hG4bK%u", SIP_VERSION,
             "UDP",
             localip,
             localport,
             via_branch_new_random());
    osip_message_set_via(request, _via);
    osip_free(_via);
    _via = NULL;
    via = (osip_via_t*)osip_list_get(&request->vias, 0);

    if (via == NULL)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "generating_request_fromrequest() exit---: Message Set Via Error \r\n");
        goto error1;
    }

    osip_generic_param_add(&via->via_params, osip_getcopy("rport"), NULL);

    *new_request = request;
    return 0;
error1:
    osip_message_free(request);
    request = NULL;
    return -1;
}

/*****************************************************************************
 函 数 名  : generating_invite
 功能描述  : 生成INVITE消息
 输入参数  : sip_t **invite
                            char *caller
                            char *callee
                            char *sdp
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
int generating_invite(osip_message_t** invite, char* caller, char* callee, char* sdp, char* proxyip, int proxyport, char* localip, int localport)
{
    char* size = NULL;
    char* contact = NULL;
    char* from = NULL;
    char* route = NULL;
    char* to = NULL;
    int i = 0;

    *invite = NULL;

    if (NULL == caller || NULL == localip || localport <= 0 || NULL == callee || NULL == proxyip || proxyport <= 0)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "generating_invite() exit---: Param Error:caller=%s, localip=%s, localport=%d, callee=%s, proxyip=%s, proxyport=%d \r\n", caller, localip, localport, callee, proxyip, proxyport);
        return -1;
    }

    i = get_to(&to, callee, proxyip, proxyport);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "generating_invite() exit---: Get To Error \r\n");
        goto error1;
    }

    i = get_contact(&contact, caller, localip, localport);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "generating_invite() exit---: Get Contact Error \r\n");
        goto error1;
    }

    i = get_from(&from, caller, localip, localport);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "generating_invite() exit---: Get From Error \r\n");
        goto error1;
    }

    i = generating_request_out_of_dialog(invite, (char*)"INVITE", from, to, route, localip, localport, NULL, NULL, 0);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "generating_invite() exit---: Generating Request Out Of Dialog Error \r\n");
        goto error1;
    }

    if (sdp != NULL)
    {
        size = (char*)osip_malloc(8 * sizeof(char));

        if (NULL == size)
        {
            goto error1;
        }

        snprintf(size, 8 * sizeof(char), "%d", (int)strlen(sdp));
        osip_message_set_content_length(*invite, size);
        osip_free(size);
        size = NULL;

        osip_message_set_body(*invite, sdp, strlen(sdp));
        osip_message_set_content_type(*invite, (char*)"Application/SDP");
    }
    else
    {
        osip_message_set_content_length(*invite, (char*)"0");
    }

    /* About content-length:
    in case a body is added after this method has been called, the
    application MUST take care of removing this header before
    replacing it.
    It should also take care of content-disposition and mime-type headers
    */

    osip_message_set_allow(*invite, (char*)"INVITE");
    osip_message_set_allow(*invite, (char*)"ACK");
    osip_message_set_allow(*invite, (char*)"OPTIONS");
    osip_message_set_allow(*invite, (char*)"CANCEL");
    osip_message_set_allow(*invite, (char*)"BYE");
    osip_message_set_allow(*invite, (char*)"REFER");

    osip_message_set_contact(*invite, contact);

    osip_free(contact);
    contact = NULL;
    osip_free(from);
    from = NULL;
    osip_free(route);
    route = NULL;
    osip_free(to);
    to = NULL;
    return 0;


error1:
    contact = NULL;
    osip_free(from);
    from = NULL;
    osip_free(route);
    route = NULL;
    osip_free(to);
    to = NULL;
    return -1;
}

/*****************************************************************************
 函 数 名  : generating_invite_within_dialog
 功能描述  : 生成会话内的invite消息
 输入参数  : sip_t** invite
                            sip_dialog_t* dialog
                            char* sdp
                            char* caller
                            char* localip
                            int localport
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年5月24日 星期五
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int generating_invite_within_dialog(osip_message_t** invite, sip_dialog_t* dialog, char* sdp, char* caller, char* localip, int localport)
{
    int i;
    char* contact = NULL;

    i = get_contact(&contact, caller, localip, localport);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "generating_invite_within_dialog() exit---: Get Contact Error \r\n");
        goto error1;
    }

    i = generating_request_within_dialog(invite, (char*)"INVITE", dialog, localip, localport);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "generating_invite_within_dialog() exit---: Generating Request Within Dialog Error \r\n");
        goto error1;
    }

    if (sdp != NULL)
    {
        char* size = NULL;
        size = (char*)osip_malloc(8 * sizeof(char));

        if (NULL == size)
        {
            goto error1;
        }

        snprintf(size, 8 * sizeof(char), "%d", (int)strlen(sdp));
        osip_message_set_content_length(*invite, size);
        osip_free(size);
        size = NULL;

        osip_message_set_content_type(*invite, (char*)"Application/SDP");
        osip_message_set_body(*invite, sdp, strlen(sdp));
    }
    else
    {
        osip_message_set_content_length(*invite, (char*)"0");
    }

    /* About content-length:
    in case a body is added after this method has been called, the
    application MUST take care of removing this header before
    replacing it.
    It should also take care of content-disposition and mime-type headers
    */

    osip_message_set_allow(*invite, (char*)"INVITE");
    osip_message_set_allow(*invite, (char*)"ACK");
    osip_message_set_allow(*invite, (char*)"OPTIONS");
    osip_message_set_allow(*invite, (char*)"CANCEL");
    osip_message_set_allow(*invite, (char*)"BYE");
    osip_message_set_allow(*invite, (char*)"REFER");

    osip_message_set_contact(*invite, contact);

    osip_free(contact);
    contact = NULL;
    return 0;

error1:
    osip_free(contact);
    contact = NULL;
    return -1;
}

/*****************************************************************************
 函 数 名  : generating_forward_invite_within_dialog
 功能描述  : 生成会话内转发的invite消息
 输入参数  : sip_t** invite
                            sip_dialog_t* dialog
                            char* sdp
                            char* caller
                            char* localip
                            int localport
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年5月24日 星期五
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int generating_forward_invite_within_dialog(osip_message_t** invite, sip_dialog_t* dialog, char* sdp, char* caller, char* localip, int localport)
{
    int i;
    char* contact = NULL;

    i = get_contact(&contact, caller, localip, localport);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "generating_forward_invite_within_dialog() exit---: Get Contact Error \r\n");
        goto error1;
    }

    i = generating_forward_request_within_dialog(invite, (char*)"INVITE", dialog, localip, localport);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "generating_forward_invite_within_dialog() exit---: Generating Forward Request Within Dialog Error \r\n");
        goto error1;
    }

    if (sdp != NULL)
    {
        char* size = NULL;
        size = (char*)osip_malloc(8 * sizeof(char));

        if (NULL == size)
        {
            goto error1;
        }

        snprintf(size, 8 * sizeof(char), "%d", (int)strlen(sdp));
        osip_message_set_content_length(*invite, size);
        osip_free(size);
        size = NULL;

        osip_message_set_content_type(*invite, (char*)"Application/SDP");
        osip_message_set_body(*invite, sdp, strlen(sdp));
    }
    else
    {
        osip_message_set_content_length(*invite, (char*)"0");
    }

    /* About content-length:
    in case a body is added after this method has been called, the
    application MUST take care of removing this header before
    replacing it.
    It should also take care of content-disposition and mime-type headers
    */

    osip_message_set_allow(*invite, (char*)"INVITE");
    osip_message_set_allow(*invite, (char*)"ACK");
    osip_message_set_allow(*invite, (char*)"OPTIONS");
    osip_message_set_allow(*invite, (char*)"CANCEL");
    osip_message_set_allow(*invite, (char*)"BYE");
    osip_message_set_allow(*invite, (char*)"REFER");

    osip_message_set_contact(*invite, contact);

    osip_free(contact);
    contact = NULL;
    return 0;

error1:
    osip_free(contact);
    contact = NULL;
    return -1;
}

/*****************************************************************************
 函 数 名  : generating_cancel
 功能描述  : 生成cancel消息
 输入参数  : sip_t **dest
             sip_t *request_cancelled
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月3日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int generating_cancel(osip_message_t** dest, osip_message_t* request_cancelled)
{
    int i = 0;
    osip_message_t* request = NULL;

    i = osip_message_init(&request);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "generating_cancel() exit---: Message Init Error \r\n");
        return -1;
    }

    /* prepare the request-line */
    request->sip_method = osip_getcopy("CANCEL");
    request->sip_version = osip_getcopy(SIP_VERSION);
    request->status_code   = 0;
    request->reason_phrase = NULL;

    i = osip_uri_clone(request_cancelled->req_uri, &(request->req_uri));

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "generating_cancel() exit---: URL Clone Error \r\n");
        goto gc_error_1;
    }

    i = osip_to_clone(request_cancelled->to, &(request->to));

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "generating_cancel() exit---: To Clone Error \r\n");
        goto gc_error_1;
    }

    i = osip_from_clone(request_cancelled->from, &(request->from));

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "generating_cancel() exit---: From Clone Error \r\n");
        goto gc_error_1;
    }

    /* set the cseq and call_id header */
    i = osip_call_id_clone(request_cancelled->call_id, &(request->call_id));

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "generating_cancel() exit---: Call ID Clone Error \r\n");
        goto gc_error_1;
    }

    i = osip_cseq_clone(request_cancelled->cseq, &(request->cseq));

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "generating_cancel() exit---: Cseq Clone Error \r\n");
        goto gc_error_1;
    }

    osip_free(request->cseq->method);
    request->cseq->method = NULL;

    request->cseq->method = osip_getcopy("CANCEL");

    /* copy ONLY the top most Via Field (this method is also used by proxy) */
    {
        osip_via_t* via = NULL;
        osip_via_t* via2 = NULL;
        i = osip_message_get_via(request_cancelled, 0, &via);

        if (i != 0)
        {
            SIP_DEBUG_TRACE(LOG_DEBUG, "generating_cancel() exit---: Message Get Via Error \r\n");
            goto gc_error_1;
        }

        i = osip_via_clone(via, &via2);

        if (i != 0)
        {
            SIP_DEBUG_TRACE(LOG_DEBUG, "generating_cancel() exit---: Via Clone Error \r\n");
            goto gc_error_1;
        }

        osip_list_add(&request->vias, via2, -1);

        if (via2 == NULL)
        {
            SIP_DEBUG_TRACE(LOG_DEBUG, "generating_cancel() exit---: List Add Error \r\n");
            goto gc_error_1;
        }
    }

    /* add the same route-set than in the previous request */
    if (&request_cancelled->routes != NULL)
    {
        int pos = 0;
        osip_route_t* route = NULL;
        osip_route_t* route2 = NULL;

        while (!osip_list_eol(&request_cancelled->routes, pos))
        {
            route = (osip_route_t*) osip_list_get(&request_cancelled->routes, pos);

            if (NULL == route)
            {
                pos++;
                continue;
            }

            i = osip_route_clone(route, &route2);

            if (i != 0)
            {
                SIP_DEBUG_TRACE(LOG_DEBUG, "generating_cancel() exit---: Route Clone Error \r\n");
                goto gc_error_1;
            }

            osip_list_add(&request->routes, route2, -1);
            pos++;
        }
    }

    osip_message_set_content_length(request, (char*)"0");
    osip_message_set_max_forwards(request, (char*)"70"); /* a UA should start a request with 70 */
    osip_message_set_user_agent(request, (char*)SIPUA_VERSION);

    *dest = request;
    return 0;

gc_error_1:
    osip_message_free(request);
    request = NULL;
    *dest = NULL;
    return -1;
}

/*****************************************************************************
 函 数 名  : generating_bye
 功能描述  : 生成bye消息
 输入参数  : sip_t **bye
                            sip_dialog_t *dialog
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
int generating_bye(osip_message_t** bye, sip_dialog_t* dialog, char* localip, int localport)
{
    int i;

    if (NULL == dialog || NULL == localip || localport <= 0)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "generating_bye() exit---: Param Error \r\n");
        return -1;
    }

    i = generating_request_within_dialog(bye, (char*)"BYE", dialog, localip, localport);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "generating_bye() exit---: Generating Request Within Dialog Error \r\n");
        return -1;
    }

    osip_message_set_content_length(*bye, (char*)"0");
    return 0;
}

/*****************************************************************************
 函 数 名  : generating_forward_bye
 功能描述  : 生成转发的Bye 消息
 输入参数  : sip_t **bye
                            sip_dialog_t *dialog
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
int generating_forward_bye(osip_message_t** bye, sip_dialog_t* dialog, char* localip, int localport)
{
    int i;

    i = generating_forward_request_within_dialog(bye, (char*)"BYE", dialog, localip, localport);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "generating_forward_bye() exit---: Generating Forward Request Within Dialog Error \r\n");
        return -1;
    }

    osip_message_set_content_length(*bye, (char*)"0");
    return 0;
}

/*****************************************************************************
 函 数 名  : generating_register
 功能描述  : 生成注册消息
 输入参数  : sip_t **reg
                            char* caller
                            char* username
                            char* proxyid
                            char* proxyip
                            int proxyport
                            char* localip
                            int localport
                            int period
                            char* register_callid_number
                            int register_cseq_number
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月3日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int generating_register(osip_message_t** reg, char* caller, char* username, char* proxyid, char* proxyip, int proxyport, char* localip, int localport, int period, char* register_callid_number, int register_cseq_number)
{
    int i = 0;
    char* expires = NULL;
    char* contact = NULL;
    char* from = NULL;
    char* route = NULL;
    char* registrator = NULL;

    if (NULL == caller || NULL == username || NULL == proxyid || NULL == proxyip || NULL == localip || NULL == register_callid_number)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "generating_register() exit---: Param Error \r\n");
        return -1;
    }

    i = get_contact(&contact, username, localip, localport);

    if (i != 0)
    {
        osip_free(contact);
        contact = NULL;
        SIP_DEBUG_TRACE(LOG_ERROR, "generating_register() exit---: Get Contact Error \r\n");
        return -1;
    }

    i = get_from(&from, caller, localip, localport);

    if (i != 0)
    {
        osip_free(contact);
        contact = NULL;
        osip_free(from);
        from = NULL;
        SIP_DEBUG_TRACE(LOG_ERROR, "generating_register() exit---: Get From Error \r\n");
        return -1;
    }

    i = get_registrator(&registrator, proxyid, proxyip, proxyport);

    if (i != 0)
    {
        osip_free(contact);
        contact = NULL;
        osip_free(from);
        from = NULL;
        osip_free(registrator);
        registrator = NULL;
        SIP_DEBUG_TRACE(LOG_ERROR, "generating_register() exit---: Get Registrator Error \r\n");
        return -1;
    }

    expires = (char*)osip_malloc(20);

    if (NULL == expires)
    {
        osip_free(contact);
        contact = NULL;
        osip_free(from);
        from = NULL;
        osip_free(registrator);
        registrator = NULL;
        SIP_DEBUG_TRACE(LOG_ERROR, "generating_register() exit---: Expires Malloc Error \r\n");
        return -1;
    }

    snprintf(expires, 20, "%u", period);

    i = generating_request_out_of_dialog(reg, (char*)"REGISTER", from, NULL, route, localip, localport, registrator, register_callid_number, register_cseq_number);

    if (i != 0)
    {
        osip_free(contact);
        contact = NULL;
        osip_free(from);
        from = NULL;
        osip_free(expires);
        expires = NULL;
        osip_free(registrator);
        registrator = NULL;
        SIP_DEBUG_TRACE(LOG_ERROR, "generating_register() exit---: Generating Request Out Of Dialog Error \r\n");
        return -1;;
    }

    osip_message_set_contact(*reg, contact);
    osip_message_set_expires(*reg, expires);
    osip_message_set_content_length(*reg, (char*)"0");

    osip_free(contact);
    contact = NULL;
    osip_free(from);
    from = NULL;
    osip_free(expires);
    expires = NULL;
    osip_free(registrator);
    registrator = NULL;
    return 0;
}

/*****************************************************************************
 函 数 名  : generating_options
 功能描述  :  在会话外生成options消息
 输入参数  : sip_t **options
                            char *caller
                            char *callee
                            char *sdp
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
int generating_options(osip_message_t** options, char* caller, char* callee, char* sdp, char* proxyip, int proxyport, char* localip, int localport)
{
    int i = 0;
    char* route = NULL;
    char* from = NULL;
    char* to = NULL;

    i = get_from(&from, caller, localip, localport);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "generating_options() exit---: Get From Error \r\n");
        goto error1;
    }

    i = get_to(&to, callee, proxyip, proxyport);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "generating_options() exit---: Get To Error \r\n");
        goto error1;
    }

    i = generating_request_out_of_dialog(options, (char*)"OPTIONS", from, to, route, localip, localport, NULL, NULL, 0);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "generating_options() exit---: Generating Request Out Of Dialog Error \r\n");
        goto error1;
    }

    if (sdp != NULL)
    {
        char* size = NULL;
        size = (char*)osip_malloc(8 * sizeof(char));

        if (NULL == size)
        {
            goto error1;
        }

        snprintf(size, 8 * sizeof(char), "%d", (int)strlen(sdp));
        osip_message_set_content_length(*options, size);
        osip_free(size);
        size = NULL;

        osip_message_set_content_type(*options, (char*)"Application/SDP");
        osip_message_set_body(*options, sdp, strlen(sdp));
    }
    else
    {
        osip_message_set_content_length(*options, (char*)"0");
    }

    return 0;
error1:
    osip_free(route);
    route = NULL;
    return -1;
}

/*****************************************************************************
 函 数 名  : generating_subscribe
 功能描述  : 在会话外生成subscribe消息
 输入参数  : sip_t ** subscribe
             char* caller
             char* callee
             char* callid
             char* msg
             int msg_len
             char* proxyip
             int proxyport
             char* localip
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
int generating_subscribe(osip_message_t** subscribe, char* caller, char* callee, char* callid, char* msg, int msg_len, char* proxyip, int proxyport, char* localip, int localport)
{
    char* from = NULL;
    char* route = NULL;
    char* to = NULL;
    char* contact = NULL;
    char* size = NULL;
    int i = 0;

    *subscribe = NULL;

    i = get_contact(&contact, caller, localip, localport);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "generating_message() exit---: Get Contact Error \r\n");
        return -1;
    }

    i = get_to(&to, callee, proxyip, proxyport);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "generating_message() exit---: Get To Error \r\n");
        return -1;
    }

    i = get_from(&from, caller, localip, localport);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "generating_message() exit---: Get From Error \r\n");
        return -1;
    }

    i = generating_request_out_of_dialog(subscribe, (char*)"SUBSCRIBE", from, to, route, localip, localport, NULL, callid, 0);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "generating_message() exit---: Generating Request Out Of Dialog Error \r\n");
        goto error1;
    }

    if ((msg != NULL) && (0 != msg_len))
    {
        size = (char*)osip_malloc(8 * sizeof(char));

        if (NULL == size)
        {
            goto error1;
        }

        snprintf(size, 8 * sizeof(char), "%d", msg_len);
        osip_message_set_content_length(*subscribe, size);
        osip_free(size);
        size = NULL;

        osip_message_set_body(*subscribe, msg, strlen(msg));
        osip_message_set_content_type(*subscribe, (char*)"Application/MANSCDP+xml");
        osip_message_set_contact(*subscribe, contact);
    }
    else
    {
        osip_message_set_content_length(*subscribe, (char*)"0");
    }

    osip_free(from);
    from = NULL;
    osip_free(route);
    route = NULL;
    osip_free(to);
    to = NULL;
    osip_free(contact);
    contact = NULL;
    return 0;

error1:
    osip_free(from);
    from = NULL;
    osip_free(route);
    route = NULL;
    osip_free(to);
    to = NULL;
    return -1;
}

/*****************************************************************************
 函 数 名  : generating_notify
 功能描述  : 在会话外生成notify消息
 输入参数  : osip_message_t** notify
             char* caller
             char* callee
             char* callid
             char* msg
             int msg_len
             char* proxyip
             int proxyport
             char* localip
             int localport
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年1月14日 星期三
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int generating_notify(osip_message_t** notify, char* caller, char* callee, char* callid, char* msg, int msg_len, char* proxyip, int proxyport, char* localip, int localport)
{
    char* from = NULL;
    char* route = NULL;
    char* to = NULL;
    char* contact = NULL;
    char* size = NULL;
    int i = 0;

    *notify = NULL;

    i = get_contact(&contact, caller, localip, localport);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "generating_notify() exit---: Get Contact Error \r\n");
        return -1;
    }

    i = get_to(&to, callee, proxyip, proxyport);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "generating_notify() exit---: Get To Error \r\n");
        return -1;
    }

    i = get_from(&from, caller, localip, localport);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "generating_notify() exit---: Get From Error \r\n");
        return -1;
    }

    i = generating_request_out_of_dialog(notify, (char*)"NOTIFY", from, to, route, localip, localport, NULL, callid, 0);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "generating_notify() exit---: Generating Request Out Of Dialog Error \r\n");
        goto error1;
    }

    if ((msg != NULL) && (0 != msg_len))
    {
        size = (char*)osip_malloc(8 * sizeof(char));

        if (NULL == size)
        {
            goto error1;
        }

        snprintf(size, 8 * sizeof(char), "%d", msg_len);
        osip_message_set_content_length(*notify, size);
        osip_free(size);
        size = NULL;

        osip_message_set_body(*notify, msg, strlen(msg));
        osip_message_set_content_type(*notify, (char*)"Application/MANSCDP+xml");
        osip_message_set_contact(*notify, contact);
    }
    else
    {
        osip_message_set_content_length(*notify, (char*)"0");
    }

    osip_free(from);
    from = NULL;
    osip_free(route);
    route = NULL;
    osip_free(to);
    to = NULL;
    osip_free(contact);
    contact = NULL;
    return 0;

error1:
    osip_free(from);
    from = NULL;
    osip_free(route);
    route = NULL;
    osip_free(to);
    to = NULL;
    return -1;
}

/*****************************************************************************
 函 数 名  : generating_notify_for_tcp
 功能描述  : 在会话外生成notify消息，基于TCP协议
 输入参数  : osip_message_t** notify
             char* caller
             char* callee
             char* callid
             char* msg
             int msg_len
             char* proxyip
             int proxyport
             char* localip
             int localport
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年2月21日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int generating_notify_for_tcp(osip_message_t** notify, char* caller, char* callee, char* callid, char* msg, int msg_len, char* proxyip, int proxyport, char* localip, int localport)
{
    char* from = NULL;
    char* route = NULL;
    char* to = NULL;
    char* contact = NULL;
    char* size = NULL;
    int i = 0;

    *notify = NULL;

    i = get_contact(&contact, caller, localip, localport);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "generating_notify_for_tcp() exit---: Get Contact Error \r\n");
        return -1;
    }

    i = get_to(&to, callee, proxyip, proxyport);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "generating_notify_for_tcp() exit---: Get To Error \r\n");
        return -1;
    }

    i = get_from(&from, caller, localip, localport);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "generating_notify_for_tcp() exit---: Get From Error \r\n");
        return -1;
    }

    i = generating_request_out_of_dialog_for_tcp(notify, (char*)"NOTIFY", from, to, route, localip, localport, NULL, callid, 0);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "generating_notify_for_tcp() exit---: Generating Request Out Of Dialog Error \r\n");
        goto error1;
    }

    if ((msg != NULL) && (0 != msg_len))
    {
        size = (char*)osip_malloc(8 * sizeof(char));

        if (NULL == size)
        {
            goto error1;
        }

        snprintf(size, 8 * sizeof(char), "%d", msg_len);
        osip_message_set_content_length(*notify, size);
        osip_free(size);
        size = NULL;

        osip_message_set_body(*notify, msg, strlen(msg));
        osip_message_set_content_type(*notify, (char*)"Application/MANSCDP+xml");
        osip_message_set_contact(*notify, contact);
    }
    else
    {
        osip_message_set_content_length(*notify, (char*)"0");
    }

    osip_free(from);
    from = NULL;
    osip_free(route);
    route = NULL;
    osip_free(to);
    to = NULL;
    osip_free(contact);
    contact = NULL;
    return 0;

error1:
    osip_free(from);
    from = NULL;
    osip_free(route);
    route = NULL;
    osip_free(to);
    to = NULL;
    return -1;
}

int generating_notify_within_dialog(osip_message_t** notify, sip_dialog_t* dialog, sip_subscription_t* sip_sub, char* msg, int msg_len, char* callee, char* localip, int localport)
{
    int i = 0;
    char* size = NULL;
    char event[100] = {0};
    char* contact = NULL;

    i = generating_request_within_dialog(notify, (char*)"NOTIFY", dialog, localip, localport);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "generating_notify_within_dialog() exit---: Get Contact Error \r\n");
        return -1;
    }

    msg_setsubscription_state(*notify, sip_sub->sub_state);

    if (sip_sub->event_type)
    {
        if (sip_sub->id_param)
        {
            snprintf(event, 100, "%s;id=%s", sip_sub->event_type, sip_sub->id_param);
        }
        else
        {
            snprintf(event, 100, "%s", sip_sub->event_type);
        }

        msg_setevent(*notify, event);
    }

    if (NULL != sip_sub->remote_contact_uri)
    {
        i = get_contact(&contact, callee, localip, localport);

        if (i != 0)
        {
            SIP_DEBUG_TRACE(LOG_ERROR, "generating_notify_within_dialog() exit---: Get Contact Error \r\n");
            return -1;
        }

        osip_message_set_contact(*notify, contact);
    }

    if ((msg != NULL) && (0 != msg_len))
    {
        size = (char*)osip_malloc(8 * sizeof(char));

        if (NULL == size)
        {
            SIP_DEBUG_TRACE(LOG_ERROR, "generating_info_within_dialog() exit---: Malloc Error \r\n");
            goto error1;
        }

        snprintf(size, 8 * sizeof(char), "%d", msg_len);
        osip_message_set_content_length(*notify, size);
        osip_free(size);
        size = NULL;

        osip_message_set_body(*notify, msg, strlen(msg));
        osip_message_set_content_type(*notify, (char*)"Application/MANSCDP+xml");
    }
    else
    {
        osip_message_set_content_length(*notify, (char*)"0");
    }

    osip_free(contact);
    contact = NULL;
    return 0;

error1:
    osip_free(contact);
    contact = NULL;
    return -1;
}

/*****************************************************************************
 函 数 名  : generating_message
 功能描述  : 生成message消息
 输入参数  : sip_t **message
             char* caller
             char* callee
             char* callid
             char* msg
             int msg_len
             char* proxyip
             int proxyport
             char* localip
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
int generating_message(osip_message_t** message, char* caller, char* callee, char* callid, char* msg, int msg_len, char* proxyip, int proxyport, char* localip, int localport)
{
    char* from = NULL;
    char* route = NULL;
    char* to = NULL;
    char* contact = NULL;
    char* size = NULL;
    int i = 0;

    *message = NULL;

    i = get_contact(&contact, caller, localip, localport);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "generating_message() exit---: Get Contact Error \r\n");
        return -1;
    }

    i = get_to(&to, callee, proxyip, proxyport);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "generating_message() exit---: Get To Error \r\n");
        return -1;
    }

    i = get_from(&from, caller, localip, localport);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "generating_message() exit---: Get From Error \r\n");
        return -1;
    }

    i = generating_request_out_of_dialog(message, (char*)"MESSAGE", from, to, route, localip, localport, NULL, callid, 0);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "generating_message() exit---: Generating Request Out Of Dialog Error \r\n");
        goto error1;
    }

    if ((msg != NULL) && (0 != msg_len))
    {
        size = (char*)osip_malloc(8 * sizeof(char));

        if (NULL == size)
        {
            goto error1;
        }

        snprintf(size, 8 * sizeof(char), "%d", msg_len);
        osip_message_set_content_length(*message, size);
        osip_free(size);
        size = NULL;

        osip_message_set_body(*message, msg, strlen(msg));
        osip_message_set_content_type(*message, (char*)"Application/MANSCDP+xml");
        osip_message_set_contact(*message, contact);
    }
    else
    {
        osip_message_set_content_length(*message, (char*)"0");
    }

    osip_free(from);
    from = NULL;
    osip_free(route);
    route = NULL;
    osip_free(to);
    to = NULL;
    osip_free(contact);
    contact = NULL;
    return 0;

error1:
    osip_free(from);
    from = NULL;
    osip_free(route);
    route = NULL;
    osip_free(to);
    to = NULL;
    return -1;
}

/*****************************************************************************
 函 数 名  : generating_message_for_tcp
 功能描述  : 生成message消息，基于SIP协议
 输入参数  : osip_message_t** message
             char* caller
             char* callee
             char* callid
             char* msg
             int msg_len
             char* proxyip
             int proxyport
             char* localip
             int localport
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年2月21日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int generating_message_for_tcp(osip_message_t** message, char* caller, char* callee, char* callid, char* msg, int msg_len, char* proxyip, int proxyport, char* localip, int localport)
{
    char* from = NULL;
    char* route = NULL;
    char* to = NULL;
    char* contact = NULL;
    char* size = NULL;
    int i = 0;

    *message = NULL;

    i = get_contact(&contact, caller, localip, localport);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "generating_message_for_tcp() exit---: Get Contact Error \r\n");
        return -1;
    }

    i = get_to(&to, callee, proxyip, proxyport);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "generating_message_for_tcp() exit---: Get To Error \r\n");
        return -1;
    }

    i = get_from(&from, caller, localip, localport);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "generating_message_for_tcp() exit---: Get From Error \r\n");
        return -1;
    }

    i = generating_request_out_of_dialog_for_tcp(message, (char*)"MESSAGE", from, to, route, localip, localport, NULL, callid, 0);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "generating_message_for_tcp() exit---: generating_request_out_of_dialog_for_tcp Error \r\n");
        goto error1;
    }

    if ((msg != NULL) && (0 != msg_len))
    {
        size = (char*)osip_malloc(8 * sizeof(char));

        if (NULL == size)
        {
            goto error1;
        }

        snprintf(size, 8 * sizeof(char), "%d", msg_len);
        osip_message_set_content_length(*message, size);
        osip_free(size);
        size = NULL;

        osip_message_set_body(*message, msg, strlen(msg));
        osip_message_set_content_type(*message, (char*)"Application/MANSCDP+xml");
        osip_message_set_contact(*message, contact);
    }
    else
    {
        osip_message_set_content_length(*message, (char*)"0");
    }

    osip_free(from);
    from = NULL;
    osip_free(route);
    route = NULL;
    osip_free(to);
    to = NULL;
    osip_free(contact);
    contact = NULL;
    return 0;

error1:
    osip_free(from);
    from = NULL;
    osip_free(route);
    route = NULL;
    osip_free(to);
    to = NULL;
    return -1;
}

/*****************************************************************************
 函 数 名  : generating_message_within_dialog
 功能描述  : 生成会话内message消息
 输入参数  : sip_t **info
                            sip_dialog_t *dialog
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
int generating_message_within_dialog(osip_message_t** info, sip_dialog_t* dialog, char* msg, int msg_len, char* localip, int localport)
{
    int i = 0;
    char* size = NULL;

    i = generating_request_within_dialog(info, (char*)"MESSAGE", dialog, localip, localport);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "generating_message_within_dialog() exit---: Generating Request Within Dialog Error \r\n");
        return -1;
    }

    if ((msg != NULL) && (0 != msg_len))
    {
        size = (char*)osip_malloc(8 * sizeof(char));

        if (NULL == size)
        {
            SIP_DEBUG_TRACE(LOG_DEBUG, "generating_message_within_dialog() exit---: Malloc Error \r\n");
            return -1;
        }

        snprintf(size, 8 * sizeof(char), "%d", msg_len);
        osip_message_set_content_length(*info, size);
        osip_free(size);
        size = NULL;

        osip_message_set_body(*info, msg, strlen(msg));
        osip_message_set_content_type(*info, (char*)"Application/MANSCDP+xml");
    }
    else
    {
        osip_message_set_content_length(*info, (char*)"0");
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : generating_info
 功能描述  : 生成info消息
 输入参数  : sip_t **info
                            char *caller
                            char *callee
                            char *body
                            int body_len
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
int generating_info(osip_message_t** info, char* caller, char* callee, char* callid, char* body, int body_len, char* proxyip, int proxyport, char* localip, int localport)
{
    char* from = NULL;
    char* route = NULL;
    char* to = NULL;
    char* contact = NULL;
    char* size = NULL;
    int i = 0;

    *info = NULL;

    i = get_contact(&contact, caller, localip, localport);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "generating_info() exit---: Get Contact Error \r\n");
        return -1;
    }

    i = get_to(&to, callee, proxyip, proxyport);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "generating_info() exit---: Get To Error \r\n");
        return -1;
    }

    i = get_from(&from, caller, localip, localport);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "generating_info() exit---: Get From Error \r\n");
        return -1;
    }

    i = generating_request_out_of_dialog(info, (char*)"INFO", from, to, route, localip, localport, NULL, callid, 0);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "generating_info() exit---: Generating Request Out Of Dialog Error \r\n");
        goto error1;
    }

    if ((body != NULL) && (0 != body_len))
    {
        size = (char*)osip_malloc(8 * sizeof(char));

        if (NULL == size)
        {
            goto error1;
        }

        snprintf(size, 8 * sizeof(char), "%d", body_len);
        osip_message_set_content_length(*info, size);
        osip_free(size);
        size = NULL;

        osip_message_set_body(*info, body, strlen(body));
        osip_message_set_content_type(*info, (char*)"Application/MANSRTSP");
        osip_message_set_contact(*info, contact);
    }
    else
    {
        osip_message_set_content_length(*info, (char*)"0");
    }

    osip_free(from);
    from = NULL;
    osip_free(route);
    route = NULL;
    osip_free(to);
    to = NULL;
    osip_free(contact);
    contact = NULL;
    return 0;

error1:
    osip_free(from);
    from = NULL;
    osip_free(route);
    route = NULL;
    osip_free(to);
    to = NULL;
    return -1;
}

/*****************************************************************************
 函 数 名  : generating_info_within_dialog
 功能描述  : 生成会话内info消息
 输入参数  : sip_t **info
                            sip_dialog_t *dialog
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
int generating_info_within_dialog(osip_message_t** info, sip_dialog_t* dialog, char* msg, int msg_len, char* localip, int localport)
{
    int i = 0;
    char* size = NULL;
    
    *info = NULL;

    i = generating_request_within_dialog(info, (char*)"INFO", dialog, localip, localport);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "generating_info_within_dialog() exit---: Generating Request Within Dialog Error \r\n");
        return -1;
    }

    if ((msg != NULL) && (0 != msg_len))
    {
        size = (char*)osip_malloc(8 * sizeof(char));

        if (NULL == size)
        {
            SIP_DEBUG_TRACE(LOG_ERROR, "generating_info_within_dialog() exit---: Malloc Error \r\n");
            return -1;
        }

        snprintf(size, 8 * sizeof(char), "%d", msg_len);
        osip_message_set_content_length(*info, size);
        osip_free(size);
        size = NULL;

        osip_message_set_body(*info, msg, strlen(msg));
        osip_message_set_content_type(*info, (char*)"Application/MANSRTSP");
    }
    else
    {
        osip_message_set_content_length(*info, (char*)"0");
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : generating_options_within_dialog
 功能描述  : 在会话内生成options消息
 输入参数  : sip_t **options
                            sip_dialog_t *dialog
                            char *sdp
                            char* localip
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
int generating_options_within_dialog(osip_message_t** options, sip_dialog_t* dialog, char* sdp, char* localip, int localport)
{
    int i = 0;

    *options = NULL;

    i = generating_request_within_dialog(options, (char*)"OPTIONS", dialog, localip, localport);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "generating_options_within_dialog() exit---: Generating Request Within Dialog Error \r\n");
        return -1;
    }

    if (sdp != NULL)
    {
        char* size = NULL;
        size = (char*)osip_malloc(8 * sizeof(char));

        if (NULL == size)
        {
            SIP_DEBUG_TRACE(LOG_DEBUG, "generating_options_within_dialog() exit---: Malloc Error \r\n");
            return -1;
        }

        snprintf(size, 8 * sizeof(char), "%d", (int)strlen(sdp));
        osip_message_set_content_length(*options, size);
        osip_free(size);
        size = NULL;

        osip_message_set_content_type(*options, (char*)"Application/SDP");
        osip_message_set_body(*options, sdp, strlen(sdp));
    }
    else
    {
        osip_message_set_content_length(*options, (char*)"0");
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : generating_update_within_dialog
 功能描述  : 生成会话内update消息
 输入参数  : sip_t **info
                            sip_dialog_t *dialog
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
int generating_update_within_dialog(osip_message_t** update, sip_dialog_t* dialog, char* localip, int localport)
{
    int i = 0;

    *update = NULL;

    i = generating_request_within_dialog(update, (char*)"UPDATE", dialog, localip, localport);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "generating_update_within_dialog() exit---: Generating Request Within Dialog Error \r\n");
        return -1;
    }

    osip_message_set_content_length(*update, (char*)"0");

    return 0;
}

/*****************************************************************************
 函 数 名  : generating_subscribe_with_dialog
 功能描述  : 在会话内生成subscribe消息
 输入参数  : osip_message_t** subscribe
             sip_dialog_t* dialog
             char* caller
             char* localip
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
int generating_subscribe_with_dialog(osip_message_t** subscribe, sip_dialog_t* dialog, char* caller, char* localip, int localport)
{
    int i = 0;
    char* contact = NULL;

    *contact = NULL;

    i = get_contact(&contact, caller, localip, localport);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "generating_subscribe_with_dialog() exit---: Get Contact Error \r\n");
        return -1;
    }

    i = generating_request_within_dialog(subscribe, (char*)"SUBSCRIBE", dialog, localip, localport);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "generating_subscribe_with_dialog() exit---: Generating Request Within Dialog Error \r\n");
        return -1;
    }

    osip_message_set_contact(*subscribe, contact);

    osip_free(contact);
    contact = NULL;
    return 0;
}

/*****************************************************************************
 函 数 名  : generating_refer_within_dialog
 功能描述  : 在会话内生成refer消息
 输入参数  : sip_t **refer
                            sip_dialog_t *dialog
                            char* caller
                            char* callee
                            char* proxyip
                            int proxyport
                            char* localip
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
int generating_refer_within_dialog(osip_message_t** refer, sip_dialog_t* dialog, char* caller, char* callee, char* proxyip, int proxyport, char* localip, int localport)
{
    int i = 0;
    char* to = NULL;

    *refer = NULL;

    if (dialog == NULL)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "generating_refer_within_dialog() exit---: Param Error \r\n");
        return -1;
    }

    i = get_to(&to, callee, proxyip, proxyport);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "generating_refer_within_dialog() exit---: Get To Error \r\n");
        return -1;
    }

    i = generating_request_within_dialog(refer, (char*)"REFER", dialog, localip, localport);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "generating_refer_within_dialog() exit---: Generating Request Within Dialog Error \r\n");
        return -1;
    }

    osip_message_set_header(*refer, (char*)"Refer-To", to);
    osip_free(to);
    to = NULL;

    osip_uri_to_str(dialog->local_uri->url, &to);
    osip_message_set_header(*refer, (char*)"Referred-By", to);
    osip_free(to);
    to = NULL;

    get_contact(&to, caller, localip, localport);
    osip_message_set_contact(*refer, to);
    osip_free(to);
    to = NULL;

    return 0;
}

/*****************************************************************************
 函 数 名  : generating_ack_for_2xx
 功能描述  : 产生200消息的回应Ack消息
 输入参数  : sip_t **ack
                            sip_dialog_t *dialog
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
int generating_ack_for_2xx(osip_message_t** ack, sip_dialog_t* dialog, char* localip, int localport)
{
    int i = 0;

    i = generating_request_within_dialog(ack, (char*)"ACK", dialog, localip, localport);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "generating_ack_for_2xx() exit---: Generating Request Within Dialog Error \r\n");
        return -1;
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : generating_forward_ack_for_2xx
 功能描述  : 产生200消息的回应转发的Ack消息
 输入参数  : sip_t **ack
                            sip_dialog_t *dialog
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
int generating_forward_ack_for_2xx(osip_message_t** ack, sip_dialog_t* dialog, char* localip, int localport)
{
    int i = 0;

    i = generating_forward_request_within_dialog(ack, (char*)"ACK", dialog, localip, localport);
    SIP_DEBUG_TRACE(LOG_DEBUG, "generating_forward_ack_for_2xx() generating_forward_request_within_dialog:i=%d\r\n", i);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "generating_forward_ack_for_2xx() exit---: Generating Forward Request Within Dialog Error \r\n");
        return -1;
    }

    return 0;
}
#endif

#if DECS("应答消息")
/*****************************************************************************
 函 数 名  : get_message_sdp
 功能描述  : 获取消息中的SDP信息
 输入参数  : sip_t *sip
             sdp_t **sdpr
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月3日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int get_message_sdp(osip_message_t* sip, sdp_message_t** sdpr)
{
    int i = 0;
    int pos = 0;
    sdp_message_t* sdp = NULL;
    osip_body_t* body = NULL;

    *sdpr = NULL;

    if (sip == NULL)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "get_message_sdp() exit---: Param Error \r\n");
        return -1;
    }

    /* Is this a "on hold" message? */
    sdp = NULL;
    pos = 0;
    i = 500;

    while (!osip_list_eol(&sip->bodies, pos))
    {
        body = (osip_body_t*)osip_list_get(&sip->bodies, pos);

        if (NULL == body)
        {
            pos++;
            continue;
        }

        pos++;

        i = sdp_message_init(&sdp);

        if (i != 0)
        {
            break;
        }

        /* WE ASSUME IT IS A SDP BODY AND THAT    */
        /* IT IS THE ONLY ONE, OF COURSE, THIS IS */
        /* NOT TRUE */
        if (body->body != NULL)
        {
            i = sdp_message_parse(sdp, body->body);

            if (i == 0)
            {
                i = 200;
                break;
            }
        }

        sdp_message_free(sdp);
        sdp = NULL;
    }

    if (pos != 0 && i != 200) /* if INVITE have body ,but not SDP */
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "get_message_sdp() exit---: INVITE Have Body, But No SDP \r\n");
        return -1;
    }

    if (NULL == sdp) /* no SDP in INVTIE request */
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "get_message_sdp() exit---: No SDP In INVTIE Request \r\n");
        return -1;
    }

    *sdpr = sdp;

    return 0;
}

/*****************************************************************************
 函 数 名  : get_sdp_ip_and_port
 功能描述  : 获取SDP信息中的IP和端口号
 输入参数  : sdp_t* sdp
             unsigned long* addr
             int* port
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年6月9日 星期日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int get_sdp_ip_and_port(sdp_message_t* sdp, unsigned long* addr, int* port)
{
    int  pos = -1;
    int pos_media = -1;
    int found = 0;
    char* media_ipaddr = NULL;
    char* session_ipaddr = NULL;
    char* tmp = NULL;
    unsigned long ipaddr = 0;
    int  ipport = 0;

    if (sdp == NULL)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "get_sdp_ip_and_port() exit---: Param Error \r\n");
        return -1;
    }

    pos = 0;
    pos_media = -1;
    found = 0;

    session_ipaddr = sdp_message_c_addr_get(sdp, pos_media, pos); /*if pos_media=-1*/

    while (!sdp_message_endof_media(sdp, pos_media))  /* is have media */
    {
        /* media type */
        tmp = sdp_message_m_media_get(sdp, pos_media);

        if (0 == sstrcmp(tmp, "video"))
        {
            found = 1;
            break;
        }

        pos_media++;
    }

    if (!found) /* 如果没有找到视频信息，再查找音频信息 */
    {
        pos = 0;
        pos_media = -1;

        while (!sdp_message_endof_media(sdp, pos_media))  /* is have media */
        {
            /* media type */
            tmp = sdp_message_m_media_get(sdp, pos_media);

            if (0 == sstrcmp(tmp, "audio"))
            {
                found = 1;
                break;
            }

            pos_media++;
        }

        if (!found)
        {
            SIP_DEBUG_TRACE(LOG_DEBUG, "SIP_GetSDPAudioInfo() exit---: Not Video And Audio Found \r\n");
            return -1;
        }
    }

    if (session_ipaddr != NULL && 0 == strncmp("0.0.0.0", session_ipaddr, 8)) /* HOLD */
    {
        media_ipaddr = session_ipaddr;
    }
    else
    {
        media_ipaddr = sdp_message_c_addr_get(sdp, pos_media, pos);

        if (NULL == media_ipaddr)
        {
            media_ipaddr = session_ipaddr;
        }

        if (NULL == media_ipaddr)  /* error */
        {
            SIP_DEBUG_TRACE(LOG_DEBUG, "get_sdp_ip_and_port() exit---: Media IP Addr NULL \r\n");
            return -1;
        }
    }

    /* ip address */
    SIP_DEBUG_TRACE(LOG_DEBUG, "get_sdp_ip_and_port() media_ipaddr=%s \r\n", media_ipaddr);

    ipaddr = ntohl(inet_addr(media_ipaddr));
    //ipaddr = inet_addr(media_ipaddr);

    /* ip port */
    tmp = sdp_message_m_port_get(sdp, pos_media);

    if (tmp != NULL)
    {
        ipport = osip_atoi(tmp);
    }

    if (addr != NULL)
    {
        *addr = ipaddr;
    }

    if (port != NULL)
    {
        *port = ipport;
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : get_sdp_videoinfo
 功能描述  : 获取sdp中的Video信息
 输入参数  : sdp_t *sdp
             unsigned long *addr
             int *port
             int *codetype
             int *flag
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月3日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int get_sdp_videoinfo(sdp_message_t* sdp, unsigned long* addr, int* port, int* codetype, int* flag)
{
    int  pos = -1;
    int pos_media = -1;
    int found = 0;
    char* media_ipaddr = NULL;
    char* session_ipaddr = NULL;
    char* tmp = NULL;
    char* session_sndrcv = NULL;
    char* media_sndrcv = NULL;
    char* flag_sndrcv = NULL;
    unsigned long ipaddr = 0;
    int  ipport = 0;
    int  code = 0;

    if (sdp == NULL)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "get_sdp_videoinfo() exit---: Param Error \r\n");
        return -1;
    }

    found = 0;
    pos_media = -1;
    pos = 0;
    session_ipaddr = sdp_message_c_addr_get(sdp, pos_media, pos); /*if pos_media=-1*/
    session_sndrcv = sdp_message_a_att_field_get(sdp, pos_media, pos); /* get session sendrcv attribute */

    while (session_sndrcv != NULL)
    {
        if (0 == strncmp("inactive", session_sndrcv, 9)
            || 0 == strncmp("sendonly", session_sndrcv, 9)
            || 0 == strncmp("recvonly", session_sndrcv, 9)
            || 0 == strncmp("sendrecv", session_sndrcv, 9))
        {
            break;
        }

        pos++;
        session_sndrcv = sdp_message_a_att_field_get(sdp, pos_media, pos);
    }

    pos_media = 0;
    pos = 0;

    while (!sdp_message_endof_media(sdp, pos_media))  /* is have media */
    {
        /* media type */
        tmp = sdp_message_m_media_get(sdp, pos_media);

        if (0 == sstrcmp(tmp, "video"))
        {
            found = 1;
            break;
        }

        pos_media++;
    }

    if (!found)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "get_sdp_videoinfo() exit---: Not Found \r\n");
        return -1;
    }

    if (session_ipaddr != NULL && 0 == strncmp("0.0.0.0", session_ipaddr, 8)) /* HOLD */
    {
        media_ipaddr = session_ipaddr;
    }
    else
    {
        media_ipaddr = sdp_message_c_addr_get(sdp, pos_media, pos);

        if (NULL == media_ipaddr)
        {
            media_ipaddr = session_ipaddr;
        }

        if (NULL == media_ipaddr)  /* error */
        {
            SIP_DEBUG_TRACE(LOG_DEBUG, "get_sdp_videoinfo() exit---: Media IP Addr NULL \r\n");
            return -1;
        }
    }

    pos = 0;
    media_sndrcv = sdp_message_a_att_field_get(sdp, pos_media, pos); /* get session sendrcv attribute */

    while (media_sndrcv != NULL)
    {
        if (0 == strncmp("inactive", media_sndrcv, 9)
            || 0 == strncmp("sendonly", media_sndrcv, 9)
            || 0 == strncmp("recvonly", media_sndrcv, 9)
            || 0 == strncmp("sendrecv", media_sndrcv, 9))
        {
            break;
        }

        pos++;
        media_sndrcv = sdp_message_a_att_field_get(sdp, pos_media, pos);
    }

    if (media_sndrcv == NULL && session_sndrcv != NULL)
    {
        flag_sndrcv = session_sndrcv;
    }
    else
    {
        flag_sndrcv = media_sndrcv;
    }

    /* ip address */
    SIP_DEBUG_TRACE(LOG_DEBUG, "get_sdp_videoinfo() media_ipaddr=%s \r\n", media_ipaddr);

    ipaddr = ntohl(inet_addr(media_ipaddr));
    //ipaddr = inet_addr(media_ipaddr);

    /* ip port */
    tmp = sdp_message_m_port_get(sdp, pos_media);

    if (tmp != NULL)
    {
        ipport = osip_atoi(tmp);
    }

    pos = 0;
    tmp = sdp_message_m_payload_get(sdp, pos_media, pos);

    if (tmp == NULL)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "get_sdp_videoinfo() exit---: Get Payload Error \r\n");
        return -1;
    }

    if (!strncmp(tmp, "0", 2)) /* G.711 u law */
    {
        code = 0;
    }
    else if (!strncmp(tmp, "4", 2)) /* G.723 */
    {
        code = 2;
    }
    else if (!strncmp(tmp, "18", 3))  /* G.729 */
    {
        code = 1;
    }
    else if (!strncmp(tmp, "8", 2))   /* G.711 a law */
    {
        code = 3;
    }
    else
    {
        code = -1;
    }

    if (addr != NULL)
    {
        *addr = ipaddr;
    }

    if (port != NULL)
    {
        *port = ipport;
    }

    if (codetype != NULL)
    {
        *codetype = code;
    }

    if (flag != NULL)
    {
        if (flag_sndrcv == NULL || 0 == strncmp("sendrecv", flag_sndrcv, 9))
        {
            *flag = 3;
        }
        else if (0 == strncmp("inactive", flag_sndrcv, 9))
        {
            *flag = 0;
        }
        else if (0 == strncmp("sendonly", flag_sndrcv, 9))
        {
            *flag = 1;
        }
        else if (0 == strncmp("recvonly", flag_sndrcv, 9))
        {
            *flag = 2;
        }
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : cs_generating_response_default
 功能描述  : 服务端生成默认的响应消息
 输入参数  : sip_t * request
             int status
             char * reasonphrase
             sip_t ** dest
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月3日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int cs_generating_response_default(osip_message_t* request, int status, char* reasonphrase, osip_message_t** dest)
{
    int i = 0;
    int pos = -1;
    /*header_t *url_params;*/
    osip_generic_param_t* tag = NULL;
    osip_message_t* response = NULL;

    *dest = NULL;

    if (NULL == request)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "cs_generating_response_default() exit---: Request Message NULL \r\n");
        return -1;
    }

    //SIP_DEBUG_TRACE(LOG_DEBUG, "cs_generating_response_default() request->strtline->sipmethod=%s \r\n", request->strtline->sipmethod);
    //SIP_DEBUG_TRACE(LOG_DEBUG, "cs_generating_response_default() status=%d \r\n", status);

    i = osip_message_init(&response);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "cs_generating_response_default() exit---: Message Init Error \r\n");
        return -1;
    }

    response->sip_version = (char*) osip_malloc(8 * sizeof(char));

    if (NULL == response->sip_version)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "cs_generating_response_default() exit---: Malloc Error \r\n");
        return -1;
    }

    snprintf(response->sip_version, 8 * sizeof(char), "%s", SIP_VERSION);
    response->status_code = status;

    if (reasonphrase == NULL)
    {
        response->reason_phrase = osip_getcopy(osip_message_get_reason(status));
    }
    else
    {
        response->reason_phrase = osip_getcopy(reasonphrase);
    }

    response->req_uri = NULL;
    response->sip_method = NULL;

    i = osip_to_clone(request->to, &(response->to));

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "cs_generating_response_default() exit---: To Clone Error \r\n");
        goto grd_error_1;
    }

    if (NULL != response->to)
    {
        i = osip_to_get_tag(response->to, &tag);

        if (i != 0)
        {
            if (status != 100)
            {
                osip_to_set_tag(response->to, to_tag_new_random());
                i = osip_to_get_tag(response->to, &tag);

                if (i != 0)
                {
                    SIP_DEBUG_TRACE(LOG_DEBUG, "cs_generating_response_default() exit---: To Set Tag Error \r\n");
                    goto grd_error_1;
                }
            }

        }
    }

    i = osip_from_clone(request->from, &(response->from));

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "cs_generating_response_default() exit---: From Clone Error \r\n");
        goto grd_error_1;
    }

    /* copy vias */
    pos = 0;

    while (!osip_list_eol(&request->vias, pos))
    {
        osip_via_t* via = NULL;
        osip_via_t* via2 = NULL;
        via = (osip_via_t*) osip_list_get(&request->vias, pos);

        if (NULL == via)
        {
            pos++;
            continue;
        }

        i = osip_via_clone(via, &via2);

        if (i != -0)
        {
            SIP_DEBUG_TRACE(LOG_DEBUG, "cs_generating_response_default() exit---: Via Clone Error \r\n");
            goto grd_error_1;
        }

        osip_list_add(&response->vias, via2, -1);
        pos++;
    }

    i = osip_call_id_clone(request->call_id, &(response->call_id));

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "cs_generating_response_default() exit---: Call ID Clone Error \r\n");
        goto grd_error_1;
    }

    i = osip_cseq_clone(request->cseq, &(response->cseq));

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "cs_generating_response_default() exit---: Cseq Clone Error \r\n");
        goto grd_error_1;
    }

    if (status == 100)
    {
        osip_header_t* th = NULL;
        osip_message_get_timestamp(request, 0, &th);

        if (th != NULL)
        {
            osip_message_set_timestamp(response, th->hvalue);
        }
    }

    *dest = response;
    return 0;

grd_error_1:
    osip_message_free(response);
    response = NULL;
    *dest = NULL;
    return -1;
}

/*****************************************************************************
 函 数 名  : cs_generating_answer
 功能描述  : 服务端回应
 输入参数  : transaction_t * tr
             sip_t *request
             int code
             char * reasonphrase
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月3日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int cs_generating_answer(osip_transaction_t* tr, osip_message_t* request, int code, char* reasonphrase)
{
    int i = 0;
    osip_message_t* resp = NULL;
    osip_message_t* requ = NULL;

    if (request != NULL)
    {
        requ = request;
    }
    else
    {
        if (tr == NULL)
        {
            SIP_DEBUG_TRACE(LOG_ERROR, "cs_generating_answer() exit---: Transaction NULL \r\n");
            return -1;
        }

        requ = tr->orig_request;
    }

    if (requ == NULL || MSG_IS_ACK(requ)) /* ingore ACK */
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "cs_generating_answer() exit---: Request NULL \r\n");
        return  -1;
    }

    i = cs_generating_response_default(requ, code, reasonphrase, &resp);
    //SIP_DEBUG_TRACE(LOG_DEBUG, "cs_generating_answer() cs_generating_response_default:i=%d \r\n", i);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "cs_generating_answer() exit---: CS Generating Default Response Error \r\n");
        return -1;
    }

    if (tr != NULL)
    {
        i = ul_sendmessage(tr, resp);
        //SIP_DEBUG_TRACE(LOG_DEBUG, "cs_generating_answer() ul_sendmessage:i=%d \r\n", i);

        if (i != 0)
        {
            SIP_DEBUG_TRACE(LOG_ERROR, "cs_generating_answer() exit---: ul_sendmessage Error \r\n");
            goto error1;
        }
    }
    else
    {
        char* dest_host = NULL;
        char* dest_port = NULL;
        int port = 0;
        osip_via_t* via = NULL;
        osip_generic_param_t* param = NULL;
        osip_message_get_via(resp, 0, &via);

        if (via == NULL)
        {
            SIP_DEBUG_TRACE(LOG_ERROR, "cs_generating_answer() exit---: Message Get Via Error \r\n");
            goto error1;
        }

        osip_via_param_get_byname(via, (char*)"received", &param);

        if (param != NULL && param->gvalue != NULL)
        {
            dest_host = param->gvalue;
        }
        else
        {
            dest_host = via->host;
        }

        osip_via_param_get_byname(via, (char*)"rport", &param);

        if (param != NULL && param->gvalue != NULL)
        {
            dest_port = param->gvalue;
        }
        else
        {
            dest_port = via->port;
        }

        if (dest_host == NULL)
        {
            SIP_DEBUG_TRACE(LOG_ERROR, "cs_generating_answer() exit---: Dest Host NULL \r\n");
            goto error1;
        }

        if (dest_port == NULL)
        {
            port = 5060;
        }
        else
        {
            port = osip_atoi(dest_port);
        }

        i = tl_sendmessage(resp, dest_host, port, 0);
        //SIP_DEBUG_TRACE(LOG_DEBUG, "cs_generating_answer() tl_sendmessage:i=%d \r\n", i);

        if (i != 0)
        {
            SIP_DEBUG_TRACE(LOG_ERROR, "cs_generating_answer() exit---: Send Message Error \r\n");
            goto error1;
        }
    }

    return 0;

error1:
    osip_message_free(resp);
    resp = NULL;
    return -1;
}

/*****************************************************************************
 函 数 名  : generating_response_default
 功能描述  : 生成默认的响应消息
 输入参数  : sip_t **dest
             sip_dialog_t *dialog
             int status
             sip_t *request
             char* reasonphrase
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月3日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int generating_response_default(osip_message_t** dest, sip_dialog_t* dialog, int status, osip_message_t* request, char* reasonphrase)
{
    osip_generic_param_t* tag = NULL;
    osip_message_t* response = NULL;
    int pos = -1;
    int i = 0;

    if (NULL == request)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "generating_response_default() exit---: Param Error \r\n");
        return -1;
    }

    i = osip_message_init(&response);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "generating_response_default() exit---: Message Init Error \r\n");
        return -1;
    }

    response->sip_version = (char*)osip_malloc(8 * sizeof(char));

    if (NULL == response->sip_version)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "generating_response_default() exit---: Malloc Error \r\n");
        return -1;
    }

    snprintf(response->sip_version, 8 * sizeof(char), "%s", SIP_VERSION);
    response->status_code = status;

    if (reasonphrase == NULL)
    {
        response->reason_phrase = osip_getcopy(osip_message_get_reason(status));
    }
    else
    {
        response->reason_phrase = osip_getcopy(reasonphrase);
    }

    response->req_uri     = NULL;
    response->sip_method = NULL;

    i = osip_to_clone(request->to, &(response->to));

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "generating_response_default() exit---: To Clone Error \r\n");
        goto grd_error_1;
    }

    if (NULL != response->to)
    {
        i = osip_to_get_tag(response->to, &tag);

        if (i != 0)
        {
            /* we only add a tag if it does not already contains one! */
            if ((dialog != NULL) && (dialog->local_tag != NULL))
                /* it should contain the local TAG we created */
            {
                osip_to_set_tag(response->to, osip_getcopy(dialog->local_tag));
            }
            else
            {
                if (status != 100)
                {
                    osip_to_set_tag(response->to, to_tag_new_random());
                }
            }
        }
    }

    i = osip_from_clone(request->from, &(response->from));

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "generating_response_default() exit---: From Clone Error \r\n");
        goto grd_error_1;
    }

    pos = 0;

    while (&request->vias != NULL && !osip_list_eol(&request->vias, pos))
    {
        osip_via_t* via = NULL;
        osip_via_t* via2 = NULL;
        via = (osip_via_t*)osip_list_get(&request->vias, pos);

        if (NULL == via)
        {
            pos++;
            continue;
        }

        i = osip_via_clone(via, &via2);

        if (i != -0)
        {
            SIP_DEBUG_TRACE(LOG_DEBUG, "generating_response_default() exit---: Via Clone Error \r\n");
            goto grd_error_1;
        }

        osip_list_add(&response->vias, via2, -1);
        pos++;
    }

    i = osip_call_id_clone(request->call_id, &(response->call_id));

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "generating_response_default() exit---: Call ID Clone Error \r\n");
        goto grd_error_1;
    }

    i = osip_cseq_clone(request->cseq, &(response->cseq));

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "generating_response_default() exit---: Cseq Clone Error \r\n");
        goto grd_error_1;
    }

    *dest = response;
    return 0;

grd_error_1:
    osip_message_free(response);
    response = NULL;
    return -1;
}

/*****************************************************************************
 函 数 名  : generating_sdp_answer
 功能描述  : 生成SDP应答
 输入参数  : sip_t *request
                            char* audio_port
                            char* video_port
                            char *localip
                            int audio_code_type
                            int video_code_type
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月3日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int generating_sdp_answer(sdp_message_t* remote_sdp, sdp_message_t** local_sdp, char* audio_port, char* video_port, char* localip, int audio_code_type, int video_code_type)
{
    int i = 0;
    sdp_context_t* context = NULL;
    sdp_message_t* local_sdp_tmp = NULL;
    sdp_message_t* remote_sdp_tmp = NULL;

    if (NULL == remote_sdp)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "generating_sdp_answer() exit---: Param Error \r\n");
        return -1;
    }

    i = sdp_context_init(&context);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "generating_sdp_answer() exit---: SDP Context Init Error \r\n");
        return EV9000_SIPSTACK_SDP_INIT_ERROR;
    }

    i = sdp_message_clone(remote_sdp, &remote_sdp_tmp);

    if (i != 0)
    {
        sdp_context_free(context);
        osip_free(context);
        context = NULL;
        SIP_DEBUG_TRACE(LOG_ERROR, "generating_sdp_answer() exit---: SDP Clone Error \r\n");
        return EV9000_SIPSTACK_SDP_CLONE_ERROR;
    }

    i = sdp_context_set_remote_sdp(context, remote_sdp_tmp);

    if (i != 0)
    {
        sdp_context_free(context);
        osip_free(context);
        context = NULL;
        SIP_DEBUG_TRACE(LOG_ERROR, "generating_sdp_answer() exit---: SDP Context Set Remote SDP \r\n");
        return EV9000_SIPSTACK_SDP_SET_REMOTE_SDP_ERROR;
    }

    i = sdp_context_execute_negociation(context, audio_port, video_port, localip, audio_code_type, video_code_type);

    if (i == 200)
    {
        local_sdp_tmp = sdp_context_get_local_sdp(context);

        i = sdp_message_clone(local_sdp_tmp, local_sdp);

        if (i != 0)
        {
            sdp_context_free(context);
            osip_free(context);
            context = NULL;
            SIP_DEBUG_TRACE(LOG_ERROR, "generating_sdp_answer() exit---: SDP Clone Error \r\n");
            return EV9000_SIPSTACK_SDP_CLONE_ERROR;
        }

        sdp_context_free(context);
        osip_free(context);
        context = NULL;
        return 0;
    }

    sdp_context_free(context);
    osip_free(context);
    context = NULL;
    return i;
}

/*****************************************************************************
 函 数 名  : complete_answer_that_establish_a_dialog
 功能描述  : 完成应答并创建会话
 输入参数  : sip_t *response
                            sip_t *request
                            char *caller
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
int complete_answer_that_establish_a_dialog(osip_message_t* response, osip_message_t* request, char* callee, char* localip, int localport)
{
    int i = 0;
    int pos = 0;
    char* contact = NULL;

    if (NULL == request)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "complete_answer_that_establish_a_dialog() exit---: Param Error \r\n");
        return -1;
    }

    //char *newcontact = NULL;
    /* 12.1.1:
    copy all record-route in response
    add a contact with global scope
    */
    while (&request->record_routes != NULL && !osip_list_eol(&request->record_routes, pos))
    {
        osip_record_route_t* rr = NULL;
        osip_record_route_t* rr2 = NULL;
        rr = (osip_record_route_t*)osip_list_get(&request->record_routes, pos);

        if (NULL == rr)
        {
            pos++;
            continue;
        }

        i = osip_record_route_clone(rr, &rr2);

        if (i != 0)
        {
            SIP_DEBUG_TRACE(LOG_DEBUG, "complete_answer_that_establish_a_dialog() exit---: Record Route Clone Error \r\n");
            return -1;
        }

        osip_list_add(&response->record_routes, rr2, -1);
        pos++;
    }

    i = get_contact(&contact, callee, localip, localport);

    if (i != 0)
    {
        osip_free(contact);
        contact = NULL;
        SIP_DEBUG_TRACE(LOG_DEBUG, "complete_answer_that_establish_a_dialog() exit---: Get Contact Error \r\n");
        return -1;
    }

    osip_message_set_contact(response, contact);

    osip_free(contact);
    contact = NULL;
    return 0;
}

/*****************************************************************************
 函 数 名  : generating_1xx_answer_to_options
 功能描述  : 生成options 的 1xx响应消息
 输入参数  : sip_dialog_t *dialog
                            transaction_t *tr
                            int code
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月3日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void generating_1xx_answer_to_options(sip_dialog_t* dialog, osip_transaction_t* tr, int code)
{
    osip_message_t* response = NULL;
    int i = 0;

    if (NULL == dialog || NULL == tr)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "generating_1xx_answer_to_options() exit---: Param Error \r\n");
        return;
    }

    i = generating_response_default(&response, dialog, code, tr->orig_request, NULL);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "generating_1xx_answer_to_options() exit---: Generating Default Response Error \r\n");
        return;
    }

    osip_message_set_content_length(response, (char*)"0");

    i = ul_sendmessage(tr, response);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "generating_1xx_answer_to_options() exit---: ul_sendmessage Error \r\n");
    }

    return;
}

/*****************************************************************************
 函 数 名  : generating_2xx_answer_to_options
 功能描述  : 生成options 的 2xx响应消息
 输入参数  : sip_dialog_t *dialog
                            transaction_t *tr
                            int code
                            char *localip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月3日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void generating_2xx_answer_to_options(sip_dialog_t* dialog, osip_transaction_t* tr, int code, char* localip)
{
    osip_message_t* response = NULL;
    int i = 0;

    if (NULL == dialog || NULL == tr)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "generating_2xx_answer_to_options() exit---: Param Error \r\n");
        return;
    }

    i = generating_response_default(&response, dialog, code, tr->orig_request, NULL);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "generating_2xx_answer_to_options() exit---: Generating Default Response Error \r\n");
        return;
    }

    /* response should contains the allow and supported headers */
    osip_message_set_allow(response, (char*)"INVITE");
    osip_message_set_allow(response, (char*)"ACK");
    osip_message_set_allow(response, (char*)"OPTIONS");
    osip_message_set_allow(response, (char*)"CANCEL");
    osip_message_set_allow(response, (char*)"BYE");
    osip_message_set_allow(response, (char*)"REFER");
    osip_message_set_content_length(response, (char*)"0");

    i = ul_sendmessage(tr, response);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "generating_2xx_answer_to_options() exit---: ul_sendmessage Error \r\n");
    }

    return;
}

/*****************************************************************************
 函 数 名  : generating_3456xx_answer_to_options
 功能描述  : 生成options 的 3456xx响应消息
 输入参数  : sip_dialog_t *dialog
                            transaction_t *tr
                            int code
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月3日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void generating_3456xx_answer_to_options(sip_dialog_t* dialog, osip_transaction_t* tr, int code)
{
    osip_message_t* response = NULL;
    int i = 0;

    if (NULL == dialog || NULL == tr)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "generating_3456xx_answer_to_options() exit---: Param Error \r\n");
        return;
    }

    i = generating_response_default(&response, dialog, code, tr->orig_request, NULL);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "generating_3456xx_answer_to_options() exit---: Generating Default Response Error \r\n");
        return;
    }

    if ((300 <= code) && (code <= 399))
    {
        /* Should add contact fields */
        /* ... */
    }

    osip_message_set_content_length(response, (char*)"0");
    /*  send message to transaction layer */

    i = ul_sendmessage(tr, response);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "generating_3456xx_answer_to_options() exit---: ul_sendmessage Error \r\n");
    }

    return;
}
#endif

#if DECS("消息队列")
/*****************************************************************************
 函 数 名  : sip_message_init
 功能描述  : SIP消息结构初始化
 输入参数  : sip_message_t** sip_message
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年6月9日 星期日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int sip_message_init(sip_message_t** sip_message)
{
    *sip_message = (sip_message_t*)osip_malloc(sizeof(sip_message_t));

    if (*sip_message == NULL)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_message_init() exit---: *sip_message Smalloc Error \r\n");
        return -1;
    }

    (*sip_message)->msg_type = MSG_NULL;
    (*sip_message)->dialog_index = -1;
    (*sip_message)->call_id = NULL;
    (*sip_message)->tr = NULL;
    (*sip_message)->sip = NULL;

    return 0;
}

/*****************************************************************************
 函 数 名  : sip_message_free
 功能描述  : SIP消息结构释放
 输入参数  : sip_message_t* sip_message
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年6月9日 星期日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void sip_message_free(sip_message_t* sip_message)
{
    if (sip_message == NULL)
    {
        //SIP_DEBUG_TRACE(LOG_DEBUG, "sip_message_free() exit---: Param Error \r\n");
        return;
    }

    sip_message->msg_type = MSG_NULL;

    sip_message->dialog_index = -1;

    if (NULL != sip_message->call_id)
    {
        osip_free(sip_message->call_id);
        sip_message->call_id = NULL;
    }

    sip_message->tr = NULL;
    sip_message->sip = NULL;

    osip_free(sip_message);
    sip_message = NULL;

    return;
}

/*****************************************************************************
 函 数 名  : sip_message_list_init
 功能描述  : SIP消息队列初始化
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年6月9日 星期日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int sip_message_list_init()
{
    g_SipMessageList = (sip_message_list_t*)osip_malloc(sizeof(sip_message_list_t));

    if (g_SipMessageList == NULL)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_message_list_init() exit---: g_SipMessageList Smalloc Error \r\n");
        return -1;
    }

    g_SipMessageList->pSipMessageList = (osip_list_t*)osip_malloc(sizeof(osip_list_t));

    if (NULL == g_SipMessageList->pSipMessageList)
    {
        osip_free(g_SipMessageList);
        g_SipMessageList = NULL;
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_message_list_init() exit---: SIP Message List Init Error \r\n");
        return -1;
    }

    osip_list_init(g_SipMessageList->pSipMessageList);

#ifdef MULTI_THR
    /* init smutex */
    g_SipMessageList->lock = (osip_mutex_t*)osip_mutex_init();

    if (NULL == g_SipMessageList->lock)
    {
        osip_free(g_SipMessageList->pSipMessageList);
        g_SipMessageList->pSipMessageList = NULL;
        osip_free(g_SipMessageList);
        g_SipMessageList = NULL;
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_message_list_init() exit---: SIP Message List Lock Init Error \r\n");
        return -1;
    }

#endif
    return 0;
}

/*****************************************************************************
 函 数 名  : sip_message_list_free
 功能描述  : SIP消息队列释放
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年6月9日 星期日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void sip_message_list_free()
{
    if (NULL == g_SipMessageList)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_message_list_free() exit---: Param Error \r\n");
        return;
    }

    if (NULL != g_SipMessageList->pSipMessageList)
    {
        osip_list_special_free(g_SipMessageList->pSipMessageList, (void (*)(void*))&sip_message_free);
        osip_free(g_SipMessageList->pSipMessageList);
        g_SipMessageList->pSipMessageList = NULL;
    }

#ifdef MULTI_THR

    if (NULL != g_SipMessageList->lock)
    {
        osip_mutex_destroy((struct osip_mutex*)g_SipMessageList->lock);
        g_SipMessageList->lock = NULL;
    }

#endif
    osip_free(g_SipMessageList);
    g_SipMessageList = NULL;
    return;
}

/*****************************************************************************
 函 数 名  : sip_message_list_lock
 功能描述  : 路由信息队列锁定
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
int sip_message_list_lock()
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_SipMessageList == NULL || g_SipMessageList->lock == NULL)
    {
        //SIP_DEBUG_TRACE(LOG_DEBUG, "sip_message_list_lock() exit---: Param Error \r\n");
        return -1;
    }

    iRet = osip_mutex_lock((struct osip_mutex*)g_SipMessageList->lock);

#endif
    return iRet;
}

/*****************************************************************************
 函 数 名  : sip_message_list_unlock
 功能描述  : 路由信息队列解锁
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
int sip_message_list_unlock()
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_SipMessageList == NULL || g_SipMessageList->lock == NULL)
    {
        //SIP_DEBUG_TRACE(LOG_DEBUG, "sip_message_list_unlock() exit---: Param Error \r\n");
        return -1;
    }

    iRet = osip_mutex_unlock((struct osip_mutex*)g_SipMessageList->lock);

#endif
    return iRet;
}

/*****************************************************************************
 函 数 名  : sip_message_add
 功能描述  : 添加SIP消息到SIP消息队列
 输入参数  : msg_type_t msg_type
                            char* call_id
                            int dialog_index
                            transaction_t* tr
                            sip_t* sip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年6月9日 星期日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int sip_message_add(msg_type_t msg_type, char* call_id, int dialog_index, osip_transaction_t* tr, osip_message_t* sip)
{
    sip_message_t* pSipMessage = NULL;
    int i = 0;
    int iRet = 0;

    if (g_SipMessageList == NULL || call_id == NULL)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_message_add() exit---: Param Error \r\n");
        return -1;
    }

    i = sip_message_init(&pSipMessage);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_message_add() exit---: SIP Message Init Error \r\n");
        return -1;
    }

    pSipMessage->msg_type = msg_type;
    pSipMessage->dialog_index = dialog_index;
    pSipMessage->call_id = call_id;
    pSipMessage->tr = tr;
    pSipMessage->sip = sip;

    iRet = sip_message_list_lock();

    i = osip_list_add(g_SipMessageList->pSipMessageList, pSipMessage, -1); /* add to list tail */

    if (i < 0)
    {
        iRet = sip_message_list_unlock();

        sip_message_free(pSipMessage);
        pSipMessage = NULL;
        //SIP_DEBUG_TRACE(LOG_DEBUG, "sip_message_add() exit---: List Add Error \r\n");
        return -1;
    }

    iRet = sip_message_list_unlock();

    return i - 1;
}

/*****************************************************************************
 函 数 名  : sip_message_remove
 功能描述  : 从SIP消息队列中移除SIP消息
 输入参数  : int pos
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年6月9日 星期日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int sip_message_remove(int pos)
{
    sip_message_t* pSipMessage = NULL;
    int iRet = 0;

    iRet = sip_message_list_lock();

    if (g_SipMessageList == NULL || pos < 0 || (pos >= osip_list_size(g_SipMessageList->pSipMessageList)))
    {
        //SIP_DEBUG_TRACE(LOG_DEBUG, "sip_message_remove() exit---: Param Error \r\n");
        return -1;
    }

    pSipMessage = (sip_message_t*)osip_list_get(g_SipMessageList->pSipMessageList, pos);

    if (NULL == pSipMessage)
    {
        iRet = sip_message_list_unlock();

        //SIP_DEBUG_TRACE(LOG_DEBUG, "sip_message_remove() exit---: List Get Error \r\n");
        return -1;
    }

    osip_list_remove(g_SipMessageList->pSipMessageList, pos);
    sip_message_free(pSipMessage);
    pSipMessage = NULL;
    iRet = sip_message_list_unlock();

    return 0;
}

/*****************************************************************************
 函 数 名  : sip_message_remove2
 功能描述  : 从SIP消息队列中移除SIP消息
 输入参数  : int pos
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年9月5日 星期四
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int sip_message_remove2(int pos)
{
    sip_message_t* pSipMessage = NULL;

    if (g_SipMessageList == NULL || pos < 0 || (pos >= osip_list_size(g_SipMessageList->pSipMessageList)))
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_message_remove2() exit---: Param Error \r\n");
        return -1;
    }

    pSipMessage = (sip_message_t*)osip_list_get(g_SipMessageList->pSipMessageList, pos);

    if (NULL == pSipMessage)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_message_remove2() exit---: List Get Error \r\n");
        return -1;
    }

    osip_list_remove(g_SipMessageList->pSipMessageList, pos);
    sip_message_free(pSipMessage);
    pSipMessage = NULL;

    return 0;
}

/*****************************************************************************
 函 数 名  : sip_message_find
 功能描述  : 在SIP消息队列中查询SIP消息
 输入参数  : char* call_id
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年6月9日 星期日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int sip_message_find(char* call_id)
{
    int pos = -1;
    sip_message_t* pSipMessage = NULL;
    int iRet = 0;

    if (NULL == g_SipMessageList || NULL == call_id)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_message_find() exit---: Param Error \r\n");
        return -1;
    }

    iRet = sip_message_list_lock();

    for (pos = 0; pos < osip_list_size(g_SipMessageList->pSipMessageList); pos++)
    {
        pSipMessage = (sip_message_t*)osip_list_get(g_SipMessageList->pSipMessageList, pos);

        if (NULL == pSipMessage || NULL == pSipMessage->call_id)
        {
            continue;
        }

        if (sstrcmp(pSipMessage->call_id, call_id) == 0)
        {
            iRet = sip_message_list_unlock();

            return pos;
        }
    }

    iRet = sip_message_list_unlock();

    return -1;
}

/*****************************************************************************
 函 数 名  : sip_message_find2
 功能描述  : 在SIP消息队列中查询SIP消息
 输入参数  : char* call_id
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年9月5日 星期四
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int sip_message_find2(char* call_id)
{
    int pos = -1;
    sip_message_t* pSipMessage = NULL;

    if (NULL == g_SipMessageList || NULL == call_id)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_message_find2() exit---: Param Error \r\n");
        return -1;
    }

    for (pos = 0; pos < osip_list_size(g_SipMessageList->pSipMessageList); pos++)
    {
        pSipMessage = (sip_message_t*)osip_list_get(g_SipMessageList->pSipMessageList, pos);

        if (NULL == pSipMessage || NULL == pSipMessage->call_id)
        {
            continue;
        }

        if (sstrcmp(pSipMessage->call_id, call_id) == 0)
        {
            return pos;
        }
    }

    return -1;
}

/*****************************************************************************
 函 数 名  : sip_message_get
 功能描述  : 从SIP消息队列中获取SIP消息
 输入参数  : int pos
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年6月9日 星期日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
sip_message_t* sip_message_get(int pos)
{
    sip_message_t* pSipMessage = NULL;

    if (g_SipMessageList == NULL || pos < 0 || pos >= osip_list_size(g_SipMessageList->pSipMessageList))
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_message_get() exit---: Param Error \r\n");
        return NULL;
    }

    pSipMessage = (sip_message_t*)osip_list_get(g_SipMessageList->pSipMessageList, pos);

    return pSipMessage;
}
#endif

/*****************************************************************************
 函 数 名  : SIP_GetSDPVideoInfo
 功能描述  : 获取sdp中的Video信息
 输入参数  : sdp_t *sdp
                            unsigned long *addr
                            int *port
                            int *codetype
                            int *flag
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月3日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int SIP_GetSDPVideoInfo(sdp_message_t* sdp, unsigned long* addr, int* port, int* codetype, int* flag)
{
    int  pos = -1;
    int pos_media = -1;
    int found = 0;
    char* media_ipaddr = NULL;
    char* session_ipaddr = NULL;
    char* tmp = NULL;
    char* session_sndrcv = NULL;
    char* media_sndrcv = NULL;
    char* flag_sndrcv = NULL;
    unsigned long ipaddr = 0;
    int  ipport = 0;
    int  code = 0;

    if (sdp == NULL)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "SIP_GetSDPVideoInfo() exit---: Param Error \r\n");
        return -1;
    }

    found = 0;
    pos_media = -1;
    pos = 0;
    session_ipaddr = sdp_message_c_addr_get(sdp, pos_media, pos); /*if pos_media=-1*/
    session_sndrcv = sdp_message_a_att_field_get(sdp, pos_media, pos); /* get session sendrcv attribute */

    while (session_sndrcv != NULL)
    {
        if (0 == strncmp("inactive", session_sndrcv, 9)
            || 0 == strncmp("sendonly", session_sndrcv, 9)
            || 0 == strncmp("recvonly", session_sndrcv, 9)
            || 0 == strncmp("sendrecv", session_sndrcv, 9))
        {
            break;
        }

        pos++;
        session_sndrcv = sdp_message_a_att_field_get(sdp, pos_media, pos);
    }

    pos_media = 0;
    pos = 0;

    while (!sdp_message_endof_media(sdp, pos_media))  /* is have media */
    {
        /* media type */
        tmp = sdp_message_m_media_get(sdp, pos_media);

        if (0 == sstrcmp(tmp, "video"))
        {
            found = 1;
            break;
        }

        pos_media++;
    }

    if (!found)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "SIP_GetSDPVideoInfo() exit---: Not Found \r\n");
        return -1;
    }

    if (session_ipaddr != NULL && 0 == strncmp("0.0.0.0", session_ipaddr, 8)) /* HOLD */
    {
        media_ipaddr = session_ipaddr;
    }
    else
    {
        media_ipaddr = sdp_message_c_addr_get(sdp, pos_media, pos);

        if (NULL == media_ipaddr)
        {
            media_ipaddr = session_ipaddr;
        }

        if (NULL == media_ipaddr)  /* error */
        {
            SIP_DEBUG_TRACE(LOG_DEBUG, "SIP_GetSDPVideoInfo() exit---: Media IP Addr NULL \r\n");
            return -1;
        }
    }

    pos = 0;
    media_sndrcv = sdp_message_a_att_field_get(sdp, pos_media, pos); /* get session sendrcv attribute */

    while (media_sndrcv != NULL)
    {
        if (0 == strncmp("inactive", media_sndrcv, 9)
            || 0 == strncmp("sendonly", media_sndrcv, 9)
            || 0 == strncmp("recvonly", media_sndrcv, 9)
            || 0 == strncmp("sendrecv", media_sndrcv, 9))
        {
            break;
        }

        pos++;
        media_sndrcv = sdp_message_a_att_field_get(sdp, pos_media, pos);
    }

    if (media_sndrcv == NULL && session_sndrcv != NULL)
    {
        flag_sndrcv = session_sndrcv;
    }
    else
    {
        flag_sndrcv = media_sndrcv;
    }

    /* ip address */
    SIP_DEBUG_TRACE(LOG_DEBUG, "SIP_GetSDPVideoInfo() media_ipaddr=%s \r\n", media_ipaddr);

    ipaddr = ntohl(inet_addr(media_ipaddr));
    //ipaddr = inet_addr(media_ipaddr);

    /* ip port */
    tmp = sdp_message_m_port_get(sdp, pos_media);

    if (tmp != NULL)
    {
        ipport = osip_atoi(tmp);
    }

    pos = 0;
    tmp = sdp_message_m_payload_get(sdp, pos_media, pos);

    if (tmp == NULL)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "SIP_GetSDPVideoInfo() exit---: Get Payload Error \r\n");
        return -1;
    }

    if (!strncmp(tmp, "96", 2)) /* PS stream */
    {
        code = EV9000_STREAMDATA_TYPE_PS;
    }
    else if (!strncmp(tmp, "97", 2)) /* MPEG-4 */
    {
        code = EV9000_STREAMDATA_TYPE_VIDEO_MPEG4;
    }
    else if (!strncmp(tmp, "98", 2))  /* H.264 */
    {
        code = EV9000_STREAMDATA_TYPE_VIDEO_H264;
    }
    else if (!strncmp(tmp, "99", 2))  /* SVAC */
    {
        code = EV9000_STREAMDATA_TYPE_VIDEO_SVAC;
    }
    else if (!strncmp(tmp, "500", 3))  /* HIK */
    {
        code = EV9000_STREAMDATA_TYPE_VIDEO_HIK;
    }
    else if (!strncmp(tmp, "501", 3))  /* DAH */
    {
        code = EV9000_STREAMDATA_TYPE_VIDEO_DAH;
    }
    else if (!strncmp(tmp, "502", 3))  /* NETPOSA */
    {
        code = EV9000_STREAMDATA_TYPE_VIDEO_NETPOSA;
    }
    else if (!strncmp(tmp, "503", 3))  /* 北京文安 */
    {
        code = EV9000_STREAMDATA_TYPE_VIDEO_WENAN;
    }
    else
    {
        code = -1;
    }

    if (addr != NULL)
    {
        *addr = ipaddr;
    }

    if (port != NULL)
    {
        *port = ipport;
    }

    if (codetype != NULL)
    {
        *codetype = code;
    }

    if (flag != NULL)
    {
        if (flag_sndrcv == NULL || 0 == strncmp("sendrecv", flag_sndrcv, 9))
        {
            *flag = 3;
        }
        else if (0 == strncmp("inactive", flag_sndrcv, 9))
        {
            *flag = 0;
        }
        else if (0 == strncmp("sendonly", flag_sndrcv, 9))
        {
            *flag = 1;
        }
        else if (0 == strncmp("recvonly", flag_sndrcv, 9))
        {
            *flag = 2;
        }
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : SIP_GetSDPVideoInfoEx
 功能描述  : 获取SDP中的视频信息
 输入参数  : sdp_message_t* sdp
             char* video_addr
             int* video_port
             int* codetype
             int* media_direction
             int* stream_type
             int* record_type
             int* trans_type
             int* file_size
             int* downloadspeed
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年8月22日 星期五
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int SIP_GetSDPVideoInfoEx(sdp_message_t* sdp, char* video_addr, int* video_port, int* codetype, int* media_direction, int* stream_type, int* record_type, int* trans_type, int* file_size, int* downloadspeed)
{
    int  pos = -1;
    int pos_media = -1;
    int found = 0;
    char* media_ipaddr = NULL;
    char* session_ipaddr = NULL;
    char* tmp = NULL;
    char* session_sndrcv = NULL;
    char* media_sndrcv = NULL;
    char* flag_sndrcv = NULL;
    char* proto_type = NULL;
    //unsigned long ipaddr = 0;
    int  ipport = 0;
    int  code = 0;
    sdp_attribute_t* attr = NULL;

    if (sdp == NULL)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "SIP_GetSDPVideoInfoEx() exit---: Param Error \r\n");
        return -1;
    }

    found = 0;
    pos_media = -1;
    pos = 0;
    session_ipaddr = sdp_message_c_addr_get(sdp, pos_media, pos); /*if pos_media=-1*/
    session_sndrcv = sdp_message_a_att_field_get(sdp, pos_media, pos); /* get session sendrcv attribute */

    while (session_sndrcv != NULL)
    {
        if (0 == strncmp("inactive", session_sndrcv, 9)
            || 0 == strncmp("sendonly", session_sndrcv, 9)
            || 0 == strncmp("recvonly", session_sndrcv, 9)
            || 0 == strncmp("sendrecv", session_sndrcv, 9))
        {
            break;
        }

        pos++;
        session_sndrcv = sdp_message_a_att_field_get(sdp, pos_media, pos);
    }

    pos_media = 0;
    pos = 0;

    while (!sdp_message_endof_media(sdp, pos_media))  /* is have media */
    {
        /* media type */
        tmp = sdp_message_m_media_get(sdp, pos_media);

        if (0 == sstrcmp(tmp, "video"))
        {
            found = 1;
            break;
        }

        pos_media++;
    }

    if (!found)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "SIP_GetSDPVideoInfoEx() exit---: Not Found \r\n");
        return -1;
    }

    if (session_ipaddr != NULL && 0 == strncmp("0.0.0.0", session_ipaddr, 8)) /* HOLD */
    {
        media_ipaddr = session_ipaddr;
    }
    else
    {
        media_ipaddr = sdp_message_c_addr_get(sdp, pos_media, pos);

        if (NULL == media_ipaddr)
        {
            media_ipaddr = session_ipaddr;
        }

        if (NULL == media_ipaddr)  /* error */
        {
            SIP_DEBUG_TRACE(LOG_DEBUG, "SIP_GetSDPVideoInfoEx() exit---: Media IP Addr NULL \r\n");
            return -1;
        }
    }

    pos = 0;
    media_sndrcv = sdp_message_a_att_field_get(sdp, pos_media, pos); /* get session sendrcv attribute */

    while (media_sndrcv != NULL)
    {
        if (0 == strncmp("inactive", media_sndrcv, 9)
            || 0 == strncmp("sendonly", media_sndrcv, 9)
            || 0 == strncmp("recvonly", media_sndrcv, 9)
            || 0 == strncmp("sendrecv", media_sndrcv, 9))
        {
            break;
        }

        pos++;
        media_sndrcv = sdp_message_a_att_field_get(sdp, pos_media, pos);
    }

    if (media_sndrcv == NULL && session_sndrcv != NULL)
    {
        flag_sndrcv = session_sndrcv;
    }
    else
    {
        flag_sndrcv = media_sndrcv;
    }

    /* ip address */
    SIP_DEBUG_TRACE(LOG_DEBUG, "SIP_GetSDPVideoInfoEx() media_ipaddr=%s \r\n", media_ipaddr);

    //ipaddr = ntohl(inet_addr(media_ipaddr));
    //ipaddr = inet_addr(media_ipaddr);
    if (NULL != video_addr)
    {
        osip_strncpy(video_addr, media_ipaddr, 15);
    }

    /* ip port */
    tmp = sdp_message_m_port_get(sdp, pos_media);

    if (tmp != NULL)
    {
        ipport = osip_atoi(tmp);
    }

    SIP_DEBUG_TRACE(LOG_DEBUG, "SIP_GetSDPVideoInfoEx() ipport=%d \r\n", ipport);

    pos = 0;
    tmp = sdp_message_m_payload_get(sdp, pos_media, pos);

    if (tmp == NULL)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "SIP_GetSDPVideoInfoEx() exit---: Get Payload Error \r\n");
        return -1;
    }

    SIP_DEBUG_TRACE(LOG_DEBUG, "SIP_GetSDPVideoInfoEx() payload=%s \r\n", tmp);

    if (!strncmp(tmp, "96", 2)) /* PS stream */
    {
        code = EV9000_STREAMDATA_TYPE_PS;
    }
    else if (!strncmp(tmp, "97", 2)) /* MPEG-4 */
    {
        code = EV9000_STREAMDATA_TYPE_VIDEO_MPEG4;
    }
    else if (!strncmp(tmp, "98", 2))  /* H.264 */
    {
        code = EV9000_STREAMDATA_TYPE_VIDEO_H264;
    }
    else if (!strncmp(tmp, "99", 2))  /* SVAC */
    {
        code = EV9000_STREAMDATA_TYPE_VIDEO_SVAC;
    }
    else if (!strncmp(tmp, "500", 3))  /* HIK */
    {
        code = EV9000_STREAMDATA_TYPE_VIDEO_HIK;
    }
    else if (!strncmp(tmp, "501", 3))  /* DAH */
    {
        code = EV9000_STREAMDATA_TYPE_VIDEO_DAH;
    }
    else if (!strncmp(tmp, "502", 3))  /* NETPOSA */
    {
        code = EV9000_STREAMDATA_TYPE_VIDEO_NETPOSA;
    }
    else if (!strncmp(tmp, "503", 3))  /* 北京文安 */
    {
        code = EV9000_STREAMDATA_TYPE_VIDEO_WENAN;
    }
    else
    {
        code = -1;
    }

    /*
        if (addr != NULL)
        {
            *addr = ipaddr;
        }
    */

    if (video_port != NULL)
    {
        *video_port = ipport;
    }

    if (codetype != NULL)
    {
        *codetype = code;
    }

    if (media_direction != NULL)
    {
        if (flag_sndrcv == NULL || 0 == strncmp("sendrecv", flag_sndrcv, 9))
        {
            *media_direction = 3;
        }
        else if (0 == strncmp("inactive", flag_sndrcv, 9))
        {
            *media_direction = 0;
        }
        else if (0 == strncmp("sendonly", flag_sndrcv, 9))
        {
            *media_direction = 1;
        }
        else if (0 == strncmp("recvonly", flag_sndrcv, 9))
        {
            *media_direction = 2;
        }
    }

    /* 获取媒体流类型 */
    *stream_type = 0;
    pos = 0;
    attr = sdp_message_attribute_get(sdp, pos_media, pos);

    while (attr != NULL)
    {
        if (0 == strncmp("streamtype", attr->a_att_field, 10))
        {
            *stream_type = osip_atoi(attr->a_att_value);
            break;
        }

        pos++;
        attr = sdp_message_attribute_get(sdp, pos_media, pos);
    }

    /* 获取录像类型 */
    *record_type = 0;
    pos = 0;
    attr = sdp_message_attribute_get(sdp, pos_media, pos);

    while (attr != NULL)
    {
        if (0 == strncmp("recordtype", attr->a_att_field, 10))
        {
            *record_type = osip_atoi(attr->a_att_value);
            break;
        }

        pos++;
        attr = sdp_message_attribute_get(sdp, pos_media, pos);
    }

    /* 获取文件大小 */
    *file_size = 0;
    pos = 0;
    attr = sdp_message_attribute_get(sdp, pos_media, pos);

    while (attr != NULL)
    {
        if (0 == strncmp("filesize", attr->a_att_field, 8))
        {
            *file_size = osip_atoi(attr->a_att_value);
            break;
        }

        pos++;
        attr = sdp_message_attribute_get(sdp, pos_media, pos);
    }

    /* 获取下载速度 */
    *downloadspeed = 0;
    pos = 0;
    attr = sdp_message_attribute_get(sdp, pos_media, pos);

    while (attr != NULL)
    {
        if (0 == strncmp("downloadspeed", attr->a_att_field, 13))
        {
            *downloadspeed = osip_atoi(attr->a_att_value);
            break;
        }

        pos++;
        attr = sdp_message_attribute_get(sdp, pos_media, pos);
    }

    /* 获取协议类型 */
    proto_type = sdp_message_m_proto_get(sdp, pos_media);

    if (NULL != proto_type)
    {
        if (0 == sstrcmp(proto_type, (char*)"UDP") || 0 == sstrcmp(proto_type, (char*)"udp"))
        {
            *trans_type = 1;
        }
        else if (0 == sstrcmp(proto_type, (char*)"RTP/AVP") || 0 == sstrcmp(proto_type, (char*)"rtp/avp"))
        {
            *trans_type = 1;
        }
        else if (0 == sstrcmp(proto_type, (char*)"TCP") || 0 == sstrcmp(proto_type, (char*)"tcp"))
        {
            *trans_type = 2;
        }
        else
        {
            *trans_type = 0;
        }
    }
    else
    {
        *trans_type = 0;
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : SIP_GetSDPAudioInfo
 功能描述  : 获取SDP中的音频信息
 输入参数  : sdp_message_t* sdp
             char* audio_addr
             int *audio_port
             int *codetype
             int* media_direction
             int* stream_type
             int* trans_type
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年8月22日 星期五
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int SIP_GetSDPAudioInfo(sdp_message_t* sdp, char* audio_addr, int *audio_port, int *codetype, int* media_direction, int* stream_type, int* trans_type)
{
    int  pos = -1;
    int pos_media = -1;
    int found = 0;
    char* media_ipaddr = NULL;
    char* session_ipaddr = NULL;
    char* tmp = NULL;
    char* session_sndrcv = NULL;
    char* media_sndrcv = NULL;
    char* flag_sndrcv = NULL;
    char* proto_type = NULL;
    //unsigned long ipaddr = 0;
    int  ipport = 0;
    int  code = 0;

    if (sdp == NULL)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "SIP_GetSDPAudioInfo() exit---: Param Error \r\n");
        return -1;
    }

    found = 0;
    pos_media = -1;
    pos = 0;
    session_ipaddr = sdp_message_c_addr_get(sdp, pos_media, pos); /*if pos_media=-1*/
    session_sndrcv = sdp_message_a_att_field_get(sdp, pos_media, pos); /* get session sendrcv attribute */

    while (session_sndrcv != NULL)
    {
        if (0 == strncmp("inactive", session_sndrcv, 9)
            || 0 == strncmp("sendonly", session_sndrcv, 9)
            || 0 == strncmp("recvonly", session_sndrcv, 9)
            || 0 == strncmp("sendrecv", session_sndrcv, 9))
        {
            break;
        }

        pos++;
        session_sndrcv = sdp_message_a_att_field_get(sdp, pos_media, pos);
    }

    pos_media = 0;
    pos = 0;

    while (!sdp_message_endof_media(sdp, pos_media))  /* is have media */
    {
        /* media type */
        tmp = sdp_message_m_media_get(sdp, pos_media);

        if (0 == sstrcmp(tmp, "audio"))
        {
            found = 1;
            break;
        }

        pos_media++;
    }

    if (!found)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "SIP_GetSDPAudioInfo() exit---: Not Found \r\n");
        return -1;
    }

    if (session_ipaddr != NULL && 0 == strncmp("0.0.0.0", session_ipaddr, 8)) /* HOLD */
    {
        media_ipaddr = session_ipaddr;
    }
    else
    {
        media_ipaddr = sdp_message_c_addr_get(sdp, pos_media, pos);

        if (NULL == media_ipaddr)
        {
            media_ipaddr = session_ipaddr;
        }

        if (NULL == media_ipaddr)  /* error */
        {
            SIP_DEBUG_TRACE(LOG_DEBUG, "SIP_GetSDPAudioInfo() exit---: Media IP Addr NULL \r\n");
            return -1;
        }
    }

    pos = 0;
    media_sndrcv = sdp_message_a_att_field_get(sdp, pos_media, pos); /* get session sendrcv attribute */

    while (media_sndrcv != NULL)
    {
        if (0 == strncmp("inactive", media_sndrcv, 9)
            || 0 == strncmp("sendonly", media_sndrcv, 9)
            || 0 == strncmp("recvonly", media_sndrcv, 9)
            || 0 == strncmp("sendrecv", media_sndrcv, 9))
        {
            break;
        }

        pos++;
        media_sndrcv = sdp_message_a_att_field_get(sdp, pos_media, pos);
    }

    if (media_sndrcv == NULL && session_sndrcv != NULL)
    {
        flag_sndrcv = session_sndrcv;
    }
    else
    {
        flag_sndrcv = media_sndrcv;
    }

    /* ip address */
    SIP_DEBUG_TRACE(LOG_DEBUG, "SIP_GetSDPAudioInfo() media_ipaddr=%s \r\n", media_ipaddr);

    //ipaddr = ntohl(inet_addr(media_ipaddr));
    //ipaddr = inet_addr(media_ipaddr);
    if (NULL != audio_addr)
    {
        osip_strncpy(audio_addr, media_ipaddr, 15);
    }

    /* ip port */
    tmp = sdp_message_m_port_get(sdp, pos_media);

    if (tmp != NULL)
    {
        ipport = osip_atoi(tmp);
    }

    SIP_DEBUG_TRACE(LOG_DEBUG, "SIP_GetSDPAudioInfo() ipport=%d \r\n", ipport);

    pos = 0;
    tmp = sdp_message_m_payload_get(sdp, pos_media, pos);

    if (tmp == NULL)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "SIP_GetSDPAudioInfo() exit---: Get Payload Error \r\n");
        return -1;
    }

    SIP_DEBUG_TRACE(LOG_DEBUG, "SIP_GetSDPAudioInfo() payload=%s \r\n", tmp);

    if (!strncmp(tmp, "4", 1)) /* G.723 */
    {
        code = EV9000_STREAMDATA_TYPE_AUDIO_G723;
    }
    else if (!strncmp(tmp, "8", 1)) /* G.711 A */
    {
        code = EV9000_STREAMDATA_TYPE_AUDIO_G711A;
    }
    else if (!strncmp(tmp, "9", 1)) /* G.722 */
    {
        code = EV9000_STREAMDATA_TYPE_AUDIO_G722;
    }
    else if (!strncmp(tmp, "18", 2))  /* G.729 */
    {
        code = EV9000_STREAMDATA_TYPE_AUDIO_G729;
    }
    else if (!strncmp(tmp, "20", 2))   /* SVAC */
    {
        code = EV9000_STREAMDATA_TYPE_AUDIO_SVAC;
    }
    else
    {
        code = -1;
    }

    /*
        if (addr != NULL)
        {
            *addr = ipaddr;
        }
      */

    if (audio_port != NULL)
    {
        *audio_port = ipport;
    }

    if (codetype != NULL)
    {
        *codetype = code;
    }

    if (media_direction != NULL)
    {
        if (flag_sndrcv == NULL || 0 == strncmp("sendrecv", flag_sndrcv, 9))
        {
            *media_direction = 3;
        }
        else if (0 == strncmp("inactive", flag_sndrcv, 9))
        {
            *media_direction = 0;
        }
        else if (0 == strncmp("sendonly", flag_sndrcv, 9))
        {
            *media_direction = 1;
        }
        else if (0 == strncmp("recvonly", flag_sndrcv, 9))
        {
            *media_direction = 2;
        }
    }

    /* 获取媒体流类型 */
    *stream_type = 0;
    pos = 0;
    sdp_attribute_t* attr = sdp_message_attribute_get(sdp, pos_media, pos);

    while (attr != NULL)
    {
        if (0 == strncmp("streamtype", attr->a_att_field, 10))
        {
            *stream_type = osip_atoi(attr->a_att_value);
            break;
        }

        pos++;
        attr = sdp_message_attribute_get(sdp, pos_media, pos);
    }

    /* 获取协议类型 */
    proto_type = sdp_message_m_proto_get(sdp, pos_media);

    if (NULL != proto_type)
    {
        if (0 == sstrcmp(proto_type, (char*)"UDP") || 0 == sstrcmp(proto_type, (char*)"udp"))
        {
            *trans_type = 1;
        }
        else if (0 == sstrcmp(proto_type, (char*)"RTP/AVP") || 0 == sstrcmp(proto_type, (char*)"rtp/avp"))
        {
            *trans_type = 1;
        }
        else if (0 == sstrcmp(proto_type, (char*)"TCP") || 0 == sstrcmp(proto_type, (char*)"tcp"))
        {
            *trans_type = 2;
        }
        else
        {
            *trans_type = 0;
        }
    }
    else
    {
        *trans_type = 0;
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : SIP_GetSDPInfoEx
 功能描述  : 获取SDP中的音视频信息
 输入参数  : sdp_message_t* sdp
             sdp_param_t* sdp_param
             sdp_extend_param_t* pSDPExtendParm
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年8月22日 星期五
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int SIP_GetSDPInfoEx(sdp_message_t* sdp, sdp_param_t* sdp_param, sdp_extend_param_t* pSDPExtendParm)
{
    int i = 0;
    char* o_username = NULL;
    char* s_name = NULL;
    char video_addr[16] = {0};
    int video_port = 0;
    int video_code = 0;
    int video_media_direction = 0;
    int video_stream_type = 0;
    int video_record_type = 0;
    int video_trans_type = 0;
    int video_file_size = 0;
    int video_download_speed = 0;

    char audio_addr[16] = {0};
    int audio_port = 0;
    int audio_code = 0;
    int audio_media_direction = 0;
    int audio_media_type = 0;
    int audio_trans_type = 0;

    int f_v_code_type = 0;
    int f_v_ratio = 0;
    int f_v_frame_speed = 0;
    int f_v_code_rate_type = 0;
    int f_v_code_rate_size = 0;
    int f_a_code_type = 0;
    int f_a_code_rate_size = 0;
    int f_a_sample_rate = 0;

    if (sdp == NULL || NULL == sdp_param)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "SIP_GetSDPInfoEx() exit---: Param Error \r\n");
        return -1;
    }

    /* 获取会话创建者名称 */
    o_username = sdp_message_o_username_get(sdp);

    if (NULL != o_username)
    {
        osip_strncpy(sdp_param->o_username, o_username, 32);
    }

    /* 获取S名称 */
    s_name = sdp_message_s_name_get(sdp);

    if (NULL != s_name)
    {
        osip_strncpy(sdp_param->s_name, s_name, 32);
    }

    /* 获取视频信息 */
    i = SIP_GetSDPVideoInfoEx(sdp, video_addr, &video_port, &video_code, &video_media_direction, &video_stream_type, &video_record_type, &video_trans_type, &video_file_size, &video_download_speed);

    if (0 != i)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "SIP_GetSDPInfoEx() exit---: SIP_GetSDPVideoInfoEx No Video Info \r\n");
        //return -1; /* 不能返回错误，可能SDP中没有视频信息 */
    }

    /* 获取音频信息 */
    i = SIP_GetSDPAudioInfo(sdp, audio_addr, &audio_port, &audio_code, &audio_media_direction, &audio_media_type, &audio_trans_type);

    if (0 != i)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "SIP_GetSDPInfoEx() exit---: SIP_GetSDPAudioInfo No Audio Info \r\n");
        //return -1; /* 不能返回错误，可能SDP中没有音频信息 */
    }

    if (video_port <= 0 && audio_port <= 0)
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "SIP_GetSDPInfoEx() exit---: SIP_GetSDPAudioInfo No Video And Audio Info \r\n");
        return -1;
    }

    if (audio_port > 0) /* 音频请求 */
    {
        osip_strncpy(sdp_param->sdp_ip, audio_addr, 15);
    }
    else
    {
        osip_strncpy(sdp_param->sdp_ip, video_addr, 15);
    }

    sdp_param->video_port = video_port;
    sdp_param->video_code_type = video_code;
    sdp_param->audio_port = audio_port;
    sdp_param->audio_code_type = audio_code;

    sdp_param->media_direction = video_media_direction;
    sdp_param->stream_type = video_stream_type;
    sdp_param->record_type = video_record_type;
    sdp_param->trans_type = video_trans_type;
    sdp_param->file_size = video_file_size;
    sdp_param->download_speed = video_download_speed;

    /* 获取时间信息 */
    char* tmp1 = sdp_message_t_start_time_get(sdp, 0);

    if (NULL != tmp1)
    {
        sdp_param->start_time = osip_atoi(tmp1);
    }
    else
    {
        sdp_param->start_time = 0;
    }

    char* tmp2 = sdp_message_t_stop_time_get(sdp, 0);

    if (NULL != tmp2)
    {
        sdp_param->end_time = osip_atoi(tmp2);
    }
    else
    {
        sdp_param->end_time = 0;
    }

    char* tmp3 = sdp_message_r_repeat_get(sdp, 0, 0);

    if (NULL != tmp3)
    {
        sdp_param->play_time = osip_atoi(tmp3);
    }
    else
    {
        sdp_param->play_time = 0;
    }

    /* 获取y_ssrc */
    char* tmp4 = sdp_message_y_ssrc_get(sdp);


    if (NULL != tmp4)
    {
        osip_strncpy(sdp_param->y_ssrc, tmp4, 32);
    }

    /* 获取f字段 */
    char* tmp5 = sdp_message_f_param_get(sdp);

    if (NULL != tmp5)
    {
        /* 解析存入f的各个字段 */
        i = analyze_f_param(tmp5, &f_v_code_type, &f_v_ratio, &f_v_frame_speed, &f_v_code_rate_type, &f_v_code_rate_size, &f_a_code_type, &f_a_code_rate_size, &f_a_sample_rate);

        if (0 == i)
        {
            sdp_param->f_v_code_type = f_v_code_type;
            sdp_param->f_v_ratio = f_v_ratio;
            sdp_param->f_v_frame_speed = f_v_frame_speed;
            sdp_param->f_v_code_rate_type = f_v_code_rate_type;
            sdp_param->f_v_code_rate_size = f_v_code_rate_size;
            sdp_param->f_a_code_type = f_a_code_type;
            sdp_param->f_a_code_rate_size = f_a_code_rate_size;
            sdp_param->f_a_sample_rate = f_a_sample_rate;
        }
    }

    /* 获取扩展参数 */
    if (NULL != pSDPExtendParm)
    {
        char* tmp6 = sdp_message_u_uri_get(sdp);

        if (NULL != tmp6)
        {
            osip_strncpy(pSDPExtendParm->onvif_url, tmp6, 256);
        }
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : SIP_BuildSDPInfoEx
 功能描述  : 构建SDP信息
 输入参数  : sdp_message_t** sdp
             sdp_param_t* sdp_param
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年8月22日 星期五
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int SIP_BuildSDPInfoEx(sdp_message_t** sdp, sdp_param_t* sdp_param, sdp_extend_param_t* pSDPExtendParm)
{
    int i;
    int media_line = 0;
    char audio_port[16] = {0};
    char video_port[16] = {0};
    char stream_type[16] = {0};
    char record_type[16] = {0};
    char file_size[32] = {0};
    char download_speed[16] = {0};
    char* y_ssrc = NULL;
    char tmp_f[128] = {0};
    char* f_param = NULL;
    char* onvif_url = NULL;

    if (NULL == sdp_param)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "SIP_GetSDPInfoEx() exit---: SDP Param Error \r\n");
        return -1;
    }

    i = sdp_message_init(sdp);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "SIP_GetSDPInfoEx() exit---: SDP Message Init Error \r\n");
        return -1;
    }

    sdp_message_v_version_set(*sdp, osip_getcopy("0"));

    /* those fields MUST be set */
    if (NULL != sdp_param->o_username && sdp_param->o_username[0] != '\0')
    {
        sdp_message_o_origin_set(*sdp,
                                 osip_getcopy(sdp_param->o_username),
                                 osip_getcopy(config->o_session_id),
                                 osip_getcopy(config->o_session_version),
                                 osip_getcopy(config->o_nettype),
                                 osip_getcopy(config->o_addrtype), osip_getcopy(sdp_param->sdp_ip));
    }
    else
    {
        sdp_message_o_origin_set(*sdp,
                                 osip_getcopy(config->o_username),
                                 osip_getcopy(config->o_session_id),
                                 osip_getcopy(config->o_session_version),
                                 osip_getcopy(config->o_nettype),
                                 osip_getcopy(config->o_addrtype), osip_getcopy(sdp_param->sdp_ip));
    }

    if (NULL != sdp_param->s_name && sdp_param->s_name[0] != '\0')
    {
        sdp_message_s_name_set(*sdp, osip_getcopy(sdp_param->s_name));
    }
    else
    {
        sdp_message_s_name_set(*sdp, osip_getcopy("Play"));
    }

    if (config->c_nettype != NULL)
    {
        sdp_message_c_connection_add(*sdp, -1,
                                     osip_getcopy(config->c_nettype),
                                     osip_getcopy(config->c_addrtype),
                                     osip_getcopy(sdp_param->sdp_ip),
                                     osip_getcopy(config->c_addr_multicast_ttl),
                                     osip_getcopy(config->c_addr_multicast_int));
    }

    {
        /* offer-answer draft says we must copy the "t=" line */
        char* tmp = (char*)osip_malloc(16);

        if (NULL == tmp)
        {
            SIP_DEBUG_TRACE(LOG_DEBUG, "SIP_GetSDPInfoEx() exit---: TMP Malloc Error \r\n");
            return -1;
        }

        char* tmp2 = (char*)osip_malloc(16);

        if (NULL == tmp2)
        {
            osip_free(tmp);
            tmp = NULL;
            SIP_DEBUG_TRACE(LOG_DEBUG, "SIP_GetSDPInfoEx() exit---: TMP2 Malloc Error \r\n");
            return -1;
        }

        char* tmp3 = (char*)osip_malloc(16);

        if (NULL == tmp3)
        {
            osip_free(tmp);
            tmp = NULL;
            osip_free(tmp2);
            tmp2 = NULL;
            SIP_DEBUG_TRACE(LOG_DEBUG, "SIP_GetSDPInfoEx() exit---: TMP3 Malloc Error \r\n");
            return -1;
        }

        snprintf(tmp, 16, "%i", sdp_param->start_time);
        snprintf(tmp2, 16, "%i", sdp_param->end_time);

        i = sdp_message_t_time_descr_add(*sdp, tmp, tmp2);

        if (i != 0)
        {
            osip_free(tmp);
            tmp = NULL;
            osip_free(tmp2);
            tmp2 = NULL;
            osip_free(tmp3);
            tmp3 = NULL;
            SIP_DEBUG_TRACE(LOG_DEBUG, "SIP_GetSDPInfoEx() exit---: Time Descr Add Error \r\n");
            return -1;
        }

        if (sdp_param->play_time > 0)
        {
            snprintf(tmp3, 16, "%i", sdp_param->play_time);

            i = sdp_message_r_repeat_add(*sdp, 0, tmp3);

            if (i != 0)
            {
                osip_free(tmp);
                tmp = NULL;
                osip_free(tmp2);
                tmp2 = NULL;
                osip_free(tmp3);
                tmp3 = NULL;
                SIP_DEBUG_TRACE(LOG_DEBUG, "SIP_GetSDPInfoEx() exit---: Time Repeat Add Error \r\n");
                return -1;
            }
        }
        else
        {
            osip_free(tmp3);
            tmp3 = NULL;
        }
    }

    /* 音频编码信息 */
    if (sdp_param->audio_port > 0)
    {
        memset(audio_port, 0, 16);
        snprintf(audio_port, 16, "%i", sdp_param->audio_port);

        if (sdp_param->audio_code_type >= 0)
        {
            payload_t* my = NULL;

            if (0 == sdp_param->audio_code_type || EV9000_STREAMDATA_TYPE_AUDIO_G723 == sdp_param->audio_code_type)
            {
                my = (payload_t*) osip_list_get(config->audio_codec, 0);

                if (NULL == my)
                {
                    return -1;
                }
            }
            else if (1 == sdp_param->audio_code_type || EV9000_STREAMDATA_TYPE_AUDIO_G711A == sdp_param->audio_code_type)
            {
                my = (payload_t*) osip_list_get(config->audio_codec, 1);

                if (NULL == my)
                {
                    return -1;
                }
            }
            else if (2 == sdp_param->audio_code_type || EV9000_STREAMDATA_TYPE_AUDIO_G722 == sdp_param->audio_code_type)
            {
                my = (payload_t*) osip_list_get(config->audio_codec, 2);

                if (NULL == my)
                {
                    return -1;
                }
            }
            else if (3 == sdp_param->audio_code_type || EV9000_STREAMDATA_TYPE_AUDIO_G729 == sdp_param->audio_code_type)
            {
                my = (payload_t*) osip_list_get(config->audio_codec, 3);

                if (NULL == my)
                {
                    return -1;
                }
            }
            else if (4 == sdp_param->audio_code_type || EV9000_STREAMDATA_TYPE_AUDIO_SVAC == sdp_param->audio_code_type)
            {
                my = (payload_t*) osip_list_get(config->audio_codec, 4);

                if (NULL == my)
                {
                    return -1;
                }
            }
            else
            {
                return -1;
            }

            /* all media MUST have the same PROTO, PORT. */
            if (2 == sdp_param->trans_type)
            {
                sdp_message_m_media_add(*sdp, osip_getcopy("audio"), osip_getcopy(audio_port),
                                        /*my->number_of_port, sgetcopy (my->proto));*/
                                        osip_getcopy(my->number_of_port), osip_getcopy((char*)"TCP"));   /* update to 2.0.6 */
            }
            else
            {
                sdp_message_m_media_add(*sdp, osip_getcopy("audio"), osip_getcopy(audio_port),
                                        /*my->number_of_port, sgetcopy (my->proto));*/
                                        osip_getcopy(my->number_of_port), osip_getcopy((char*)"RTP/AVP"));   /* update to 2.0.6 */
            }

            sdp_message_m_payload_add(*sdp, media_line, osip_getcopy(my->payload));

            if (my->a_rtpmap != NULL)
            {
                sdp_message_a_attribute_add(*sdp, media_line, osip_getcopy("rtpmap"),
                                            osip_getcopy(my->a_rtpmap));
            }

            switch (sdp_param->media_direction)
            {
                case 0 :
                    sdp_message_a_attribute_add(*sdp, media_line, osip_getcopy("inactive"), NULL);
                    break;

                case 1 :
                    sdp_message_a_attribute_add(*sdp, media_line, osip_getcopy("sendonly"), NULL);
                    break;

                case 2 :
                    sdp_message_a_attribute_add(*sdp, media_line, osip_getcopy("recvonly"), NULL);
                    break;

                case 3 :
                    sdp_message_a_attribute_add(*sdp, media_line, osip_getcopy("sendrecv"), NULL);
                    break;

                default:
                    sdp_message_a_attribute_add(*sdp, media_line, osip_getcopy("sendrecv"), NULL);
            }

            media_line++;
        }
        else
        {
            /* add all audio codec */
            if (!osip_list_eol(config->audio_codec, 0))
            {
                int pos = 0;
                payload_t* my = (payload_t*) osip_list_get(config->audio_codec, pos);

                /* all media MUST have the same PROTO, PORT. */
                if (2 == sdp_param->trans_type)
                {
                    sdp_message_m_media_add(*sdp, osip_getcopy("audio"), osip_getcopy(audio_port),
                                            /*my->number_of_port, sgetcopy (my->proto));*/
                                            osip_getcopy(my->number_of_port), osip_getcopy((char*)"TCP"));   /* update to 2.0.6 */
                }
                else
                {
                    sdp_message_m_media_add(*sdp, osip_getcopy("audio"), osip_getcopy(audio_port),
                                            /*my->number_of_port, sgetcopy (my->proto));*/
                                            osip_getcopy(my->number_of_port), osip_getcopy((char*)"RTP/AVP"));   /* update to 2.0.6 */
                }

                while (!osip_list_eol(config->audio_codec, pos))
                {
                    my = (payload_t*) osip_list_get(config->audio_codec, pos);

                    if (NULL == my)
                    {
                        pos++;
                        continue;
                    }

                    sdp_message_m_payload_add(*sdp, media_line, osip_getcopy(my->payload));

                    if (my->a_rtpmap != NULL)
                    {
                        sdp_message_a_attribute_add(*sdp, media_line, osip_getcopy("rtpmap"),
                                                    osip_getcopy(my->a_rtpmap));
                    }

                    pos++;
                }

                switch (sdp_param->media_direction)
                {
                    case 0 :
                        sdp_message_a_attribute_add(*sdp, media_line, osip_getcopy("inactive"), NULL);
                        break;

                    case 1 :
                        sdp_message_a_attribute_add(*sdp, media_line, osip_getcopy("sendonly"), NULL);
                        break;

                    case 2 :
                        sdp_message_a_attribute_add(*sdp, media_line, osip_getcopy("recvonly"), NULL);
                        break;

                    case 3 :
                        sdp_message_a_attribute_add(*sdp, media_line, osip_getcopy("sendrecv"), NULL);
                        break;

                    default:
                        sdp_message_a_attribute_add(*sdp, media_line, osip_getcopy("sendrecv"), NULL);
                }

                media_line++;
            }
        }
    }

    /* 视频编码信息 */
    if (sdp_param->video_port > 0)
    {
        memset(video_port, 0, 16);
        snprintf(video_port, 16, "%i", sdp_param->video_port);

        if (sdp_param->video_code_type >= 0)
        {
            payload_t* my = NULL;

            if (0 == sdp_param->video_code_type || EV9000_STREAMDATA_TYPE_PS == sdp_param->video_code_type)
            {
                my = (payload_t*) osip_list_get(config->video_codec, 0);

                if (NULL == my)
                {
                    return -1;
                }
            }
            else if (1 == sdp_param->video_code_type || EV9000_STREAMDATA_TYPE_VIDEO_MPEG4 == sdp_param->video_code_type)
            {
                my = (payload_t*) osip_list_get(config->video_codec, 1);

                if (NULL == my)
                {
                    return -1;
                }
            }
            else if (2 == sdp_param->video_code_type || EV9000_STREAMDATA_TYPE_VIDEO_H264 == sdp_param->video_code_type)
            {
                my = (payload_t*) osip_list_get(config->video_codec, 2);

                if (NULL == my)
                {
                    return -1;
                }
            }
            else if (3 == sdp_param->video_code_type || EV9000_STREAMDATA_TYPE_VIDEO_SVAC == sdp_param->video_code_type)
            {
                my = (payload_t*) osip_list_get(config->video_codec, 3);

                if (NULL == my)
                {
                    return -1;
                }
            }
            else if (4 == sdp_param->video_code_type || EV9000_STREAMDATA_TYPE_VIDEO_HIK == sdp_param->video_code_type)
            {
                my = (payload_t*) osip_list_get(config->video_codec, 4);

                if (NULL == my)
                {
                    return -1;
                }
            }
            else if (5 == sdp_param->video_code_type || EV9000_STREAMDATA_TYPE_VIDEO_DAH == sdp_param->video_code_type)
            {
                my = (payload_t*) osip_list_get(config->video_codec, 5);

                if (NULL == my)
                {
                    return -1;
                }
            }
            else if (6 == sdp_param->video_code_type || EV9000_STREAMDATA_TYPE_VIDEO_NETPOSA == sdp_param->video_code_type)
            {
                my = (payload_t*) osip_list_get(config->video_codec, 6);

                if (NULL == my)
                {
                    return -1;
                }
            }
            else if (7 == sdp_param->video_code_type || EV9000_STREAMDATA_TYPE_VIDEO_WENAN == sdp_param->video_code_type)
            {
                my = (payload_t*) osip_list_get(config->video_codec, 7);

                if (NULL == my)
                {
                    return -1;
                }
            }
            else
            {
                return -1;
            }

            /* all media MUST have the same PROTO, PORT. */
            if (2 == sdp_param->trans_type)
            {
                sdp_message_m_media_add(*sdp, osip_getcopy("video"), osip_getcopy(video_port),
                                        /*my->number_of_port, sgetcopy (my->proto));*/
                                        osip_getcopy(my->number_of_port), osip_getcopy((char*)"TCP"));  /* update to 2.0.6 */
            }
            else
            {
                sdp_message_m_media_add(*sdp, osip_getcopy("video"), osip_getcopy(video_port),
                                        /*my->number_of_port, sgetcopy (my->proto));*/
                                        osip_getcopy(my->number_of_port), osip_getcopy((char*)"RTP/AVP"));  /* update to 2.0.6 */
            }

            sdp_message_m_payload_add(*sdp, media_line, osip_getcopy(my->payload));

            if (my->a_rtpmap != NULL)
            {
                sdp_message_a_attribute_add(*sdp, media_line, osip_getcopy("rtpmap"),
                                            osip_getcopy(my->a_rtpmap));
            }

            switch (sdp_param->media_direction)
            {
                case 0 :
                    sdp_message_a_attribute_add(*sdp, media_line, osip_getcopy("inactive"), NULL);
                    break;

                case 1 :
                    sdp_message_a_attribute_add(*sdp, media_line, osip_getcopy("sendonly"), NULL);
                    break;

                case 2 :
                    sdp_message_a_attribute_add(*sdp, media_line, osip_getcopy("recvonly"), NULL);
                    break;

                case 3 :
                    sdp_message_a_attribute_add(*sdp, media_line, osip_getcopy("sendrecv"), NULL);
                    break;

                default:
                    sdp_message_a_attribute_add(*sdp, media_line, osip_getcopy("sendrecv"), NULL);
            }

            /* 媒体流类型 */
            if (sdp_param->stream_type > 0)
            {
                memset(stream_type, 0, 16);
                snprintf(stream_type, 16, "%i", sdp_param->stream_type);
                sdp_message_a_attribute_add(*sdp, media_line, osip_getcopy("streamtype"), osip_getcopy(stream_type));
            }

            /* 录像类型 */
            if (sdp_param->record_type > 0)
            {
                memset(record_type, 0, 16);
                snprintf(record_type, 16, "%i", sdp_param->record_type);
                sdp_message_a_attribute_add(*sdp, media_line, osip_getcopy("recordtype"), osip_getcopy(record_type));
            }

            /* 文件大小 */
            if (sdp_param->file_size > 0)
            {
                memset(file_size, 0, 32);
                snprintf(file_size, 32, "%i", sdp_param->file_size);
                sdp_message_a_attribute_add(*sdp, media_line, osip_getcopy("filesize"), osip_getcopy(file_size));
            }

            /* 下载速度 */
            if (sdp_param->download_speed > 0)
            {
                memset(download_speed, 0, 16);
                snprintf(download_speed, 16, "%i", sdp_param->download_speed);
                sdp_message_a_attribute_add(*sdp, media_line, osip_getcopy("downloadspeed"), osip_getcopy(download_speed));
            }

            media_line++;
        }
        else
        {
            /* add all video codec */
            if (!osip_list_eol(config->video_codec, 0))
            {
                int pos = 0;
                payload_t* my = (payload_t*) osip_list_get(config->video_codec, pos);

                /* all media MUST have the same PROTO, PORT. */
                if (2 == sdp_param->trans_type)
                {
                    sdp_message_m_media_add(*sdp, osip_getcopy("video"), osip_getcopy(video_port),
                                            /*my->number_of_port, sgetcopy (my->proto));*/
                                            osip_getcopy(my->number_of_port), osip_getcopy((char*)"TCP"));  /* update to 2.0.6 */
                }
                else
                {
                    sdp_message_m_media_add(*sdp, osip_getcopy("video"), osip_getcopy(video_port),
                                            /*my->number_of_port, sgetcopy (my->proto));*/
                                            osip_getcopy(my->number_of_port), osip_getcopy((char*)"RTP/AVP"));  /* update to 2.0.6 */
                }

                while (!osip_list_eol(config->video_codec, pos))
                {
                    my = (payload_t*) osip_list_get(config->video_codec, pos);

                    if (NULL == my)
                    {
                        pos++;
                        continue;
                    }

                    sdp_message_m_payload_add(*sdp, media_line, osip_getcopy(my->payload));

                    if (my->a_rtpmap != NULL)
                    {
                        sdp_message_a_attribute_add(*sdp, media_line, osip_getcopy("rtpmap"),
                                                    osip_getcopy(my->a_rtpmap));
                    }

                    pos++;
                }

                switch (sdp_param->media_direction)
                {
                    case 0 :
                        sdp_message_a_attribute_add(*sdp, media_line, osip_getcopy("inactive"), NULL);
                        break;

                    case 1 :
                        sdp_message_a_attribute_add(*sdp, media_line, osip_getcopy("sendonly"), NULL);
                        break;

                    case 2 :
                        sdp_message_a_attribute_add(*sdp, media_line, osip_getcopy("recvonly"), NULL);
                        break;

                    case 3 :
                        sdp_message_a_attribute_add(*sdp, media_line, osip_getcopy("sendrecv"), NULL);
                        break;

                    default:
                        sdp_message_a_attribute_add(*sdp, media_line, osip_getcopy("sendrecv"), NULL);
                }

                /* 媒体流类型 */
                if (sdp_param->stream_type > 0)
                {
                    memset(stream_type, 0, 16);
                    snprintf(stream_type, 16, "%i", sdp_param->stream_type);
                    sdp_message_a_attribute_add(*sdp, media_line, osip_getcopy("streamtype"), osip_getcopy(stream_type));
                }

                /* 录像类型 */
                if (sdp_param->record_type > 0)
                {
                    memset(record_type, 0, 16);
                    snprintf(record_type, 16, "%i", sdp_param->record_type);
                    sdp_message_a_attribute_add(*sdp, media_line, osip_getcopy("recordtype"), osip_getcopy(record_type));
                }

                /* 文件大小 */
                if (sdp_param->file_size > 0)
                {
                    memset(file_size, 0, 32);
                    snprintf(file_size, 32, "%i", sdp_param->file_size);
                    sdp_message_a_attribute_add(*sdp, media_line, osip_getcopy("filesize"), osip_getcopy(file_size));
                }

                /* 下载速度 */
                if (sdp_param->download_speed > 0)
                {
                    memset(download_speed, 0, 16);
                    snprintf(download_speed, 16, "%i", sdp_param->download_speed);
                    sdp_message_a_attribute_add(*sdp, media_line, osip_getcopy("downloadspeed"), osip_getcopy(download_speed));
                }

                media_line++;
            }
        }
    }

    /* y_ssrc */
    if (NULL != sdp_param->y_ssrc && sdp_param->y_ssrc[0] != '\0')
    {
        y_ssrc = osip_getcopy(sdp_param->y_ssrc);

        if (NULL != y_ssrc)
        {
            i = sdp_message_y_ssrc_set(*sdp, y_ssrc);
        }
    }

    /* f字段 */
    if (sdp_param->f_v_frame_speed > 0)
    {
        snprintf(tmp_f, 128, "v/%d/%d/%d/%d/%da/%d/%d/%d", sdp_param->f_v_code_type, sdp_param->f_v_ratio, sdp_param->f_v_frame_speed, sdp_param->f_v_code_rate_type, sdp_param->f_v_code_rate_size, sdp_param->f_a_code_type, sdp_param->f_a_code_rate_size, sdp_param->f_a_sample_rate);

        f_param = osip_getcopy(tmp_f);

        if (NULL != f_param)
        {
            i = sdp_message_f_param_set(*sdp, f_param);
        }
    }

    /* 添加扩展参数 */
    if (NULL != pSDPExtendParm)
    {
        if (NULL != pSDPExtendParm->onvif_url && pSDPExtendParm->onvif_url[0] != '\0')
        {
            onvif_url = osip_getcopy(pSDPExtendParm->onvif_url);

            if (NULL != onvif_url)
            {
                i = sdp_message_u_uri_set(*sdp, onvif_url);
            }
        }
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : SIP_GeneratingSDPAnswerEx
 功能描述  : 生成SDP应答信息
 输入参数  : int dialog_index
             sdp_message_t** local_sdp
             sdp_param_t* pSDPParm
             sdp_extend_param_t* pSDPExtendParm
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年8月22日 星期五
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int SIP_GeneratingSDPAnswerEx(int dialog_index, sdp_message_t** local_sdp, sdp_param_t* pSDPParm, sdp_extend_param_t* pSDPExtendParm)
{
    int i = 0;
    char* tmp = NULL;
    int pos_media = -1;
    ua_dialog_t* pUaDialog = NULL;
    char* y_ssrc = NULL;
    char tmp_f[128] = {0};
    char* f_param = NULL;
    char* onvif_url = NULL;
    char video_port[16] = {0};
    char audio_port[16] = {0};
    char file_size[32];
    char download_speed[16];
    sdp_message_t* pRemoteSDP = NULL;        /*  对端SDP 信息*/

    if (NULL == pSDPParm || NULL == pSDPParm->sdp_ip || pSDPParm->sdp_ip[0] == '\0')
    {
        //SIP_DEBUG_TRACE(LOG_DEBUG, "SIP_GeneratingSDPAnswerEx() exit---: Param Error \r\n");
        return -1;
    }

    if (!is_valid_dialog_index(dialog_index))
    {
        //SIP_DEBUG_TRACE(LOG_DEBUG, "SIP_GeneratingSDPAnswerEx() exit---: Dialog Index Error \r\n");
        return EV9000_SIPSTACK_INVITE_GET_UA_ERROR;
    }

    USED_UA_SMUTEX_LOCK();

    pUaDialog = ua_dialog_get2(dialog_index);

    if (NULL == pUaDialog)
    {
        USED_UA_SMUTEX_UNLOCK();
        //SIP_DEBUG_TRACE(LOG_DEBUG, "SIP_GeneratingSDPAnswerEx() exit---: Get UA Dialog Error:dialog_index=%d \r\n", dialog_index);
        return EV9000_SIPSTACK_INVITE_GET_UA_ERROR;
    }

    if (pUaDialog->pRemoteSDP == NULL)
    {
        USED_UA_SMUTEX_UNLOCK();
        //SIP_DEBUG_TRACE(LOG_DEBUG, "SIP_GeneratingSDPAnswerEx() exit---: Get UA Dialog Remote SDP Error \r\n");
        return EV9000_SIPSTACK_INVITE_GET_REMOTE_SDP_ERROR;
    }
    else
    {
        i = sdp_message_clone(pUaDialog->pRemoteSDP, &pRemoteSDP);

        if (i != 0)
        {
            USED_UA_SMUTEX_UNLOCK();
            //SIP_DEBUG_TRACE(LOG_DEBUG, "SIP_GeneratingSDPAnswerEx() exit---: SDP Clone Error \r\n");
            return EV9000_SIPSTACK_SDP_CLONE_ERROR;
        }
    }

    USED_UA_SMUTEX_UNLOCK();

    memset(audio_port, 0, 16);
    snprintf(audio_port, 16, "%i", pSDPParm->audio_port);
    memset(video_port, 0, 16);
    snprintf(video_port, 16, "%i", pSDPParm->video_port);

    i = generating_sdp_answer(pRemoteSDP, local_sdp, audio_port, video_port, pSDPParm->sdp_ip, pSDPParm->audio_code_type, pSDPParm->video_code_type);

    /* 获取Video的属性 */
    while (!sdp_message_endof_media(*local_sdp, pos_media))  /* is have media */
    {
        /* media type */
        tmp = sdp_message_m_media_get(*local_sdp, pos_media);

        if (0 == sstrcmp(tmp, "video"))
        {
            break;
        }

        pos_media++;
    }

    /* 文件大小 */
    if (pSDPParm->file_size > 0)
    {
        memset(file_size, 0, 32);
        snprintf(file_size, 32, "%i", pSDPParm->file_size);
        sdp_message_a_attribute_add(*local_sdp, pos_media, osip_getcopy("filesize"), osip_getcopy(file_size));
    }

    /* 下载速度 */
    if (pSDPParm->download_speed > 0)
    {
        memset(download_speed, 0, 16);
        snprintf(download_speed, 16, "%i", pSDPParm->download_speed);
        sdp_message_a_attribute_add(*local_sdp, pos_media, osip_getcopy("downloadspeed"), osip_getcopy(download_speed));
    }

    /* y_ssrc */
    if (pSDPParm->y_ssrc[0] != '\0')
    {
        y_ssrc = osip_getcopy(pSDPParm->y_ssrc);

        if (NULL != y_ssrc)
        {
            i = sdp_message_y_ssrc_set(*local_sdp, y_ssrc);
        }
    }

    /* f字段 */
    if (pSDPParm->f_v_frame_speed > 0)
    {
        snprintf(tmp_f, 128, "v/%d/%d/%d/%d/%da/%d/%d/%d", pSDPParm->f_v_code_type, pSDPParm->f_v_ratio, pSDPParm->f_v_frame_speed, pSDPParm->f_v_code_rate_type, pSDPParm->f_v_code_rate_size, pSDPParm->f_a_code_type, pSDPParm->f_a_code_rate_size, pSDPParm->f_a_sample_rate);

        f_param = osip_getcopy(tmp_f);

        if (NULL != f_param)
        {
            i = sdp_message_f_param_set(*local_sdp, f_param);
        }
    }

    /* 添加扩展参数 */
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

    sdp_message_free(pRemoteSDP);
    pRemoteSDP = NULL;

    return i;
}

/*****************************************************************************
 函 数 名  : analyze_f_param
 功能描述  : 分析SDP中的f字段
 输入参数  : char* f_param
             int* f_v_code_type
             int* f_v_ratio
             int* f_v_frame_speed
             int* f_v_code_rate_type
             int* f_v_code_rate_size
             int* f_a_code_type
             int* f_a_code_rate_size
             int* f_a_sample_rate
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年8月22日 星期五
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int analyze_f_param(char* f_param, int* f_v_code_type, int* f_v_ratio, int* f_v_frame_speed, int* f_v_code_rate_type, int* f_v_code_rate_size, int* f_a_code_type, int* f_a_code_rate_size, int* f_a_sample_rate)
{
    //char f_param[] = "v/2/2/33/2/123a3/2/3";
    char v_param[64] = {0};
    char v_param_tmp1[64] = {0};
    char v_param_tmp2[64] = {0};
    char v_param_tmp3[64] = {0};

    char v_param1[64] = {0};
    char v_param2[64] = {0};
    char v_param3[64] = {0};
    char v_param4[64] = {0};
    char v_param5[64] = {0};

    char a_param[64] = {0};
    char a_param_tmp1[64] = {0};

    char a_param1[64] = {0};
    char a_param2[64] = {0};
    char a_param3[64] = {0};
    char *tmp = NULL;

    tmp = strchr(f_param, 'a'); /*find 'a' */

    if (tmp != NULL)
    {
        if (tmp - f_param <= 0 || f_param + strlen(f_param) - tmp <= 0)
        {
            return -1;
        }

        osip_strncpy(v_param, f_param + 2, tmp - f_param - 2);
        osip_strncpy(a_param, tmp + 1, f_param + strlen(f_param) - tmp - 1);

        /* 解析视频字段 */
        /* 1、解析编码格式 */
        tmp = strchr(v_param, '/'); /*find '/' */

        if (tmp - v_param <= 0 || v_param + strlen(v_param) - tmp <= 0)
        {
            return -1;
        }

        osip_strncpy(v_param1, v_param, tmp - v_param);
        osip_strncpy(v_param_tmp1, tmp + 1, v_param + strlen(v_param) - tmp - 1);

        *f_v_code_type = atoi(v_param1);

        /* 2、解析分辨率 */
        tmp = strchr(v_param_tmp1, '/'); /*find '/' */

        if (tmp - v_param_tmp1 <= 0 || v_param_tmp1 + strlen(v_param_tmp1) - tmp <= 0)
        {
            return -1;
        }

        osip_strncpy(v_param2, v_param_tmp1, tmp - v_param_tmp1);
        osip_strncpy(v_param_tmp2, tmp + 1, v_param_tmp1 + strlen(v_param_tmp1) - tmp - 1);

        *f_v_ratio = atoi(v_param2);

        /* 3、解析帧率 */
        tmp = strchr(v_param_tmp2, '/'); /*find '/' */

        if (tmp - v_param_tmp2 <= 0 || v_param_tmp2 + strlen(v_param_tmp2) - tmp <= 0)
        {
            return -1;
        }

        osip_strncpy(v_param3, v_param_tmp2, tmp - v_param_tmp2);
        osip_strncpy(v_param_tmp3, tmp + 1, v_param_tmp2 + strlen(v_param_tmp2) - tmp - 1);

        *f_v_frame_speed = atoi(v_param3);

        /* 4、解析码率类型、码率大小 */
        tmp = strchr(v_param_tmp3, '/'); /*find '/' */

        if (tmp - v_param_tmp3 <= 0 || v_param_tmp3 + strlen(v_param_tmp3) - tmp <= 0)
        {
            return -1;
        }

        osip_strncpy(v_param4, v_param_tmp3, tmp - v_param_tmp3);
        osip_strncpy(v_param5, tmp + 1, v_param_tmp3 + strlen(v_param_tmp3) - tmp - 1);

        *f_v_code_rate_type = atoi(v_param4);
        *f_v_code_rate_size = atoi(v_param5);

        /* 解析音频字段 */
        tmp = strchr(a_param, '/'); /*find '/' */

        if (tmp - a_param <= 0 || a_param + strlen(a_param) - tmp <= 0)
        {
            return -1;
        }

        /* 1、解析编码格式 */
        tmp = strchr(a_param, '/'); /*find '/' */

        if (tmp - a_param <= 0 || a_param + strlen(a_param) - tmp <= 0)
        {
            return -1;
        }

        osip_strncpy(a_param1, a_param, tmp - a_param);
        osip_strncpy(a_param_tmp1, tmp + 1, a_param + strlen(a_param) - tmp - 1);

        *f_a_code_type = atoi(a_param1);

        /* 2、解析音频编码码率、采样率  */
        tmp = strchr(a_param_tmp1, '/'); /*find '/' */

        if (tmp - a_param_tmp1 <= 0 || a_param_tmp1 + strlen(a_param_tmp1) - tmp <= 0)
        {
            return -1;
        }

        osip_strncpy(a_param2, a_param_tmp1, tmp - a_param_tmp1);
        osip_strncpy(a_param3, tmp + 1, a_param_tmp1 + strlen(a_param_tmp1) - tmp - 1);

        *f_a_code_rate_size = atoi(a_param2);
        *f_a_sample_rate = atoi(a_param3);
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : get_message_from_host
 功能描述  : 获取sip消息的来源host
 输入参数  : sip_t* sip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年5月23日 星期四
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
char* get_message_from_host(osip_message_t* sip)
{
    int i = 0;
    osip_uri_t* url = NULL;
    osip_via_t* topvia = NULL;
    osip_generic_param_t* gen_param = NULL;
    char* tmp = NULL;

    if (NULL == sip)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "get_message_from_host() exit---: Param Error \r\n");
        return NULL;
    }

    i = osip_message_get_via(sip, 0, &topvia);

    if (i == -1 || topvia == NULL)
    {
        if (NULL == sip->from)
        {
            SIP_DEBUG_TRACE(LOG_DEBUG, "get_message_from_host() exit---: SIP From NULL \r\n");
            return NULL;
        }

        url = sip->from->url;

        if (url == NULL)
        {
            SIP_DEBUG_TRACE(LOG_DEBUG, "get_message_from_host() exit---: SIP From URL NULL \r\n");
            return NULL;
        }

        if (NULL == url->host)
        {
            SIP_DEBUG_TRACE(LOG_DEBUG, "get_message_from_host() exit---: SIP From URL Host NULL \r\n");
            return NULL;
        }
        else
        {
            return url->host;
        }
    }
    else
    {
        osip_via_param_get_byname(topvia, (char*)"maddr", &gen_param);

        if (gen_param != NULL)
        {
            tmp = gen_param->gvalue;
        }
        else
        {
            tmp = NULL;
        }

        if (NULL != tmp)
        {
            return tmp;
        }
        else
        {
            osip_via_param_get_byname(topvia, (char*)"received", &gen_param);

            if (gen_param != NULL)
            {
                tmp = gen_param->gvalue;
            }
            else
            {
                tmp = NULL;
            }

            /* 如果经过路由的IP地址为空，获取原始的IP地址 */
            if (NULL != tmp)
            {
                return tmp;
            }
            else if (NULL != topvia->host)
            {
                return topvia->host;
            }
            else
            {
                SIP_DEBUG_TRACE(LOG_DEBUG, "get_message_from_host() exit---: Top Via Host NULL \r\n");
                return NULL;
            }
        }
    }

    SIP_DEBUG_TRACE(LOG_DEBUG, "get_message_from_host() exit---: NULL \r\n");
    return NULL;
}

/*****************************************************************************
 函 数 名  : get_message_from_port
 功能描述  : 获取sip消息的来源端口
 输入参数  : sip_t* sip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年5月23日 星期四
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int get_message_from_port(osip_message_t* sip)
{
    int i = 0;
    osip_uri_t* url = NULL;
    osip_via_t* topvia = NULL;
    osip_generic_param_t* gen_param = NULL;
    char* tmp = NULL;

    if (NULL == sip)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "get_message_from_port() exit---: Param Error \r\n");
        return -1;
    }

    i = osip_message_get_via(sip, 0, &topvia);

    if (i == -1 || topvia == NULL)
    {
        url = sip->from->url;

        if (NULL == sip->from)
        {
            SIP_DEBUG_TRACE(LOG_DEBUG, "get_message_from_port() exit---: SIP From NULL \r\n");
            return -1;
        }

        if (url == NULL)
        {
            SIP_DEBUG_TRACE(LOG_DEBUG, "get_message_from_port() exit---: SIP From URL NULL \r\n");
            return -1;
        }

        if (NULL == url->port)
        {
            //SIP_DEBUG_TRACE(LOG_DEBUG, "get_message_from_port() exit---: DEFAULT 1 \r\n");
            return 5060;
        }
        else
        {
            return osip_atoi(url->port);
        }
    }
    else
    {
        osip_via_param_get_byname(topvia, (char*)"rport", &gen_param);

        if (gen_param != NULL)
        {
            tmp = gen_param->gvalue;
        }

        /* 如果经过路由的端口号不为空 */
        if (tmp != NULL)
        {
            return osip_atoi(tmp);
        }
        else if (NULL != topvia->port)
        {
            return osip_atoi(topvia->port);
        }
        else
        {
            return 5060;
        }
    }

    SIP_DEBUG_TRACE(LOG_DEBUG, "get_message_from_port() exit---: NULL \r\n");
    return -1;
}

/*****************************************************************************
 函 数 名  : get_message_to_host
 功能描述  : 获取sip消息的目的host
 输入参数  : sip_t* sip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年6月26日 星期四
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
char* get_message_to_host(osip_message_t* sip)
{
    int i = 0;
    osip_uri_t* url = NULL;
    osip_uri_t* rquri = NULL;
    osip_route_t* route = NULL;

    if (NULL == sip)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "get_message_to_host() exit---: Param Error \r\n");
        return NULL;
    }

    if (NULL != sip)
    {
        i = osip_message_get_route(sip, 0, &route);

        if (route != NULL)
        {
            if (NULL != route->url && route->url->host != NULL)
            {
                return route->url->host;
            }
        }
        else
        {
            rquri = sip->req_uri;

            if (NULL != rquri)
            {
                if (NULL != rquri->host)
                {
                    return rquri->host;
                }
                else if (NULL != sip->to)
                {
                    url = sip->to->url;

                    if (url != NULL)
                    {
                        if (NULL != url->host)
                        {
                            return url->host;
                        }
                    }
                }
            }
            else if (NULL != sip->to)
            {
                url = sip->to->url;

                if (url != NULL)
                {
                    if (NULL != url->host)
                    {
                        return url->host;
                    }
                }
            }
        }
    }
    else if (NULL != sip->to)
    {
        url = sip->to->url;

        if (url != NULL)
        {
            if (NULL != url->host)
            {
                return url->host;
            }
        }
    }

    SIP_DEBUG_TRACE(LOG_DEBUG, "get_message_to_host() exit---: NULL \r\n");
    return NULL;
}

/*****************************************************************************
 函 数 名  : get_message_to_port
 功能描述  : 获取sip消息的目的端口
 输入参数  : sip_t* sip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年6月26日 星期四
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int get_message_to_port(osip_message_t* sip)
{
    int i = 0;
    osip_uri_t* url = NULL;
    osip_uri_t* rquri = NULL;
    osip_route_t* route = NULL;

    if (NULL == sip)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "get_message_to_port() exit---: Param Error \r\n");
        return -1;
    }

    if (NULL != sip)
    {
        i = osip_message_get_route(sip, 0, &route);

        if (route != NULL)
        {
            if (NULL != route->url && route->url->port != NULL)
            {
                return osip_atoi(route->url->port);
            }
            else
            {
                return 5060;
            }
        }
        else
        {
            rquri = sip->req_uri;

            if (NULL != rquri)
            {
                if (NULL != rquri->port)
                {
                    return osip_atoi(rquri->port);
                }
                else
                {
                    //SIP_DEBUG_TRACE(LOG_DEBUG, "get_message_to_port() exit---: DEFAULT 1 \r\n");
                    return 5060;
                }
            }
            else if (NULL != sip->to)
            {
                url = sip->to->url;

                if (NULL != url)
                {
                    if (NULL != url->port)
                    {
                        return osip_atoi(url->port);
                    }
                    else
                    {
                        //SIP_DEBUG_TRACE(LOG_DEBUG, "get_message_to_port() exit---: DEFAULT 2 \r\n");
                        return 5060;
                    }
                }
            }
        }
    }
    else if (NULL != sip->to)
    {
        url = sip->to->url;

        if (NULL != url)
        {
            if (NULL != url->port)
            {
                return osip_atoi(url->port);
            }
            else
            {
                //SIP_DEBUG_TRACE(LOG_DEBUG, "get_message_to_port() exit---: DEFAULT 3 \r\n");
                return 5060;
            }
        }
    }

    SIP_DEBUG_TRACE(LOG_DEBUG, "get_message_to_port() exit---: NULL \r\n");
    return -1;
}

