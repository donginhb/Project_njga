/******************************************************************************

                  版权所有 (C), 2001-2013, 金智视讯技术有限公司

 ******************************************************************************
  文 件 名   : udp_tl.c
  版 本 号   : 初稿
  作    者   : yanghaifeng
  生成日期   : 2013年4月1日
  最近修改   :
  功能描述   : UDP线程管理
  函数列表   :
              send_message_using_udp
              SIP_Run
              sip_run_thread_execute
              sip_run_thread_free
              sip_run_thread_init
              sip_run_thread_start
              sip_run_thread_stop
              SIP_Stop
              udp_list_free
              udp_list_init
              udp_tl_execute
              udp_tl_free
              udp_tl_init
              udp_tl_start
              udp_tl_stop
              udp_tl_thread
              udp_transport_thread_start
              udp_transport_thread_stop
  修改历史   :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 创建文件

******************************************************************************/

/*----------------------------------------------*
 * 包含头文件                                   *
 *----------------------------------------------*/
#include <stdio.h>
#ifdef WIN32
#include <winsock2.h>
#include <sys/types.h>
#include <Ws2tcpip.h>

#define EWOULDBLOCK WSAEWOULDBLOCK

#else
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#endif

#include <osipparser2/osip_const.h>
#include <osip2/internal.h>
#include <osip2/osip.h>
#include <osip2/osip_fifo.h>
#include <osipparser2/osip_port.h>

#include "gbltype.h"
#include "udp_tl.inc"

#include "callback.inc"
#include "sipmsg.inc"
#include "csdbg.inc"
#include "timerproc.inc"
#include "sip_event.inc"
#include "registrar.inc"
#include "garbage.inc"

//added by chenyu 130522
#ifdef WIN32
#define vsnprintf _vsnprintf
#define snprintf  _snprintf
#endif

/*----------------------------------------------*
 * 外部变量说明                                 *
 *----------------------------------------------*/
extern osip_t* g_recv_cell;           /* 接收消息的SIP协议栈,非Message消息专用 */
extern osip_t* g_recv_register_cell;  /* 接收消息的SIP协议栈,Register消息专用 */
extern osip_t* g_recv_message_cell;   /* 接收消息的SIP协议栈,Message消息专用 */
extern osip_t* g_send_cell;           /* 发送消息的SIP协议栈,非Message消息专用 */
extern osip_t* g_send_message_cell;   /* 发送消息的SIP协议栈,Message消息专用 */

extern garbage_t * g_recv_garbage;               /* sip: garbage */
extern garbage_t * g_recv_register_garbage;      /* sip: garbage */
extern garbage_t * g_recv_msg_garbage;           /* sip: garbage */
extern garbage_t * g_send_garbage;               /* sip: garbage */
extern garbage_t * g_send_msg_garbage;           /* sip: garbage */

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
run_thread_t* g_SIPStackForRecvRunThread = NULL;         /* sip stack: main thread */
run_thread_t* g_SIPStackForRecvRegisterRunThread = NULL; /* sip stack: main thread */
run_thread_t* g_SIPStackForRecvMsgRunThread = NULL;      /* sip stack: main thread */
run_thread_t* g_SIPStackForSendRunThread = NULL;         /* sip stack: main thread */
run_thread_t* g_SIPStackForSendMsgRunThread = NULL;      /* sip stack: main thread */

run_thread_t* g_SIPAppRunThread = NULL;                  /* sip app: main thread */

udp_tl_list_t* g_SIPUdpThreadList = NULL;    /* sip消息接收线程队列 */

/*----------------------------------------------*
 * 常量定义                                     *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 宏定义                                       *
 *----------------------------------------------*/
#define IN_ADDRSTRLEN  16
#define STACK_EXE_LOOP  -1
#define UDP_EXE_LOOP     1

//added by chenyu 130523 begin-------
#ifdef WIN32
typedef int socklen_t;   //added by chenyu 131212
#pragma comment(lib,"ws2_32.lib")
//#pragma comment(lib,"WSOCK32.LIB")
const char* inet_ntop4(
    const u_char* src,
    char* dst,
    socklen_t size);
//vc6库中没有源码拷贝在此
/* char *
 * inet_ntop(af, src, dst, size)
 *  convert a network format address to presentation format.
 * return:
 *  pointer to presentation format address (`dst'), or NULL (see errno).
 * author:
 *  Paul Vixie, 1996.
 */
const char* inet_ntop(
    int af,
    const unsigned char* src,
    char* dst,
    socklen_t size)
{
    switch (af)
    {
        case AF_INET:
            return (inet_ntop4(src, dst, size));

        case AF_INET6:

            //    return (inet_ntop6(src, dst, size));
        default:
            //   __set_errno (EAFNOSUPPORT);
            return (NULL);
    }

    /* NOTREACHED */

}
/* const char *
 * inet_ntop4(src, dst, size)
 *  format an IPv4 address
 * return:
 *  `dst' (as a const)
 * notes:
 *  (1) uses no statics
 *  (2) takes a u_char* not an in_addr as input
 * author:
 *  Paul Vixie, 1996.
 */
const char* inet_ntop4(
    const u_char* src,
    char* dst,
    socklen_t size)
{
    static const char fmt[] = "%u.%u.%u.%u";
    //char tmp[sizeof "255.255.255.255"];
    char tmp[16] = {0};

    if (snprintf(tmp, 16, fmt, src[0], src[1], src[2], src[3]) >= size)
    {
        //__set_errno (ENOSPC);
        return (NULL);
    }

    return osip_strncpy(dst, tmp, 16);
}

/* const char *
 * inet_ntop6(src, dst, size)
 *  convert IPv6 binary address into presentation (printable) format
 * author:
 *  Paul Vixie, 1996.
 */
// static const char *inet_ntop6(
//                            const u_char *src,
//                            char *dst,
//                            socklen_t size)
// {
//     /*
//      * Note that int32_t and int16_t need only be "at least" large enough
//      * to contain a value of the specified size.  On some systems, like
//      * Crays, there is no such thing as an integer variable with 16 bits.
//      * Keep this in mind if you think this function should have been coded
//      * to use pointer overlays.  All the world's not a VAX.
//      */
//     char tmp[sizeof "ffff:ffff:ffff:ffff:ffff:ffff:255.255.255.255"], *tp;
//     struct { int base, len; } best, cur;
//     u_int words[NS_IN6ADDRSZ / NS_INT16SZ];
//     int i;
//
//     /*
//      * Preprocess:
//      *  Copy the input (bytewise) array into a wordwise array.
//      *  Find the longest run of 0x00's in src[] for :: shorthanding.
//      */
//     memset(words, '', sizeof words);
//     for (i = 0; i < NS_IN6ADDRSZ; i += 2)
//         words[i / 2] = (src[i] << 8) | src[i + 1];
//     best.base = -1;
//     cur.base = -1;
//     best.len = 0;
//     cur.len = 0;
//     for (i = 0; i < (NS_IN6ADDRSZ / NS_INT16SZ); i++) {
//         if (words[i] == 0) {
//             if (cur.base == -1)
//                 cur.base = i, cur.len = 1;
//             else
//                 cur.len++;
//         } else {
//             if (cur.base != -1) {
//                 if (best.base == -1 || cur.len > best.len)
//                     best = cur;
//                 cur.base = -1;
//             }
//         }
//     }
//     if (cur.base != -1) {
//         if (best.base == -1 || cur.len > best.len)
//             best = cur;
//     }
//     if (best.base != -1 && best.len < 2)
//         best.base = -1;
//
//     /*
//      * Format the result.
//      */
//     tp = tmp;
//     for (i = 0; i < (NS_IN6ADDRSZ / NS_INT16SZ); i++) {
//         /* Are we inside the best run of 0x00's? */
//         if (best.base != -1 && i >= best.base &&
//             i < (best.base + best.len)) {
//             if (i == best.base)
//                 *tp++ = ':';
//             continue;
//         }
//         /* Are we following an initial run of 0x00s or any real hex? */
//         if (i != 0)
//             *tp++ = ':';
//         /* Is this address an encapsulated IPv4? */
//         if (i == 6 && best.base == 0 &&
//             (best.len == 6 || (best.len == 5 && words[5] == 0xffff))) {
//             if (!inet_ntop4(src+12, tp, sizeof tmp - (tp - tmp)))
//                 return (NULL);
//             tp += strlen(tp);
//             break;
//         }
//         tp += sprintf((tp, "%x", words[i]));
//     }
//     /* Was it a trailing run of 0x00's? */
//     if (best.base != -1 && (best.base + best.len) ==
//         (NS_IN6ADDRSZ / NS_INT16SZ))
//         *tp++ = ':';
//     *tp++ = '';
//
//     /*
//      * Check for overflow, copy, and we're done.
//      */
//     if ((socklen_t)(tp - tmp) > size) {
//         __set_errno (ENOSPC);
//         return (NULL);
//     }
//     return strcpy(dst, tmp);
// }
#endif

//added by chenyu 130523 end---------

#if DECS("udp接收sip消息线程")
/*****************************************************************************
 Prototype    : udp_tl_execute
 Description  : udp接收消息运行线程
 Input        : udp_tl_t * udp
                osip_t* recv_cell
                osip_t* recv_register_cell
                osip_t* recv_msg_cell
                osip_t* send_cell
                osip_t* send_msg_cell
                int sec_max
                int usec_max
                int max_analysed
 Output       : None
 Return Value :
 Calls        :
 Called By    :

  History        :
  1.Date         : 2013/3/26
    Author       : yanghaifeng
    Modification : Created function

*****************************************************************************/
int udp_tl_execute(udp_tl_t* udp, osip_t* recv_cell, osip_t* recv_msg_cell, osip_t* send_msg_cell, int sec_max, int usec_max, int max_analysed)
{
    int i = 0;
    size_t message_len = 0;
    int max_fd = 0;
    char _from_ip[IN_ADDRSTRLEN] = {0};
    struct timeval val;
    fd_set cell_fdset;
    fd_set read_fdset;
    char buf[SIP_MESSAGE_MAX_LENGTH + 1] = {0};
    struct sockaddr_in _from;
    socklen_t _fromlen;
    osip_t* tmp_cell = NULL;

    if (udp == NULL || recv_cell == NULL || recv_msg_cell == NULL)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "udp_tl_execute() exit---: Param Error \r\n");
        return -1;
    }

    _fromlen = sizeof(_from);
    val.tv_sec = sec_max;
    val.tv_usec = usec_max;

    max_fd = udp->in_socket;
    FD_ZERO(&cell_fdset);
    FD_SET(udp->in_socket, &cell_fdset);

    memset(buf, 0, SIP_MESSAGE_MAX_LENGTH + 1);

    while (!udp->th_exit)
    {
        read_fdset = cell_fdset;
        i = select(max_fd + 1, &read_fdset, NULL, NULL, &val);

        if (i == 0)
        {
            goto loop;
        }

        if (i == -1)
        {
            SIP_DEBUG_TRACE(LOG_ERROR, "udp_tl_execute() select return =-1 max_fd =%d \r\n", max_fd);
            goto loop;
        }

        if (FD_ISSET(udp->in_socket, &read_fdset))
        {
            memset(buf, 0, SIP_MESSAGE_MAX_LENGTH + 1);
            message_len = recvfrom(udp->in_socket, buf, SIP_MESSAGE_MAX_LENGTH, 0,
                                   (struct sockaddr*) &_from, &_fromlen);



            if (message_len > 0 && message_len <= SIP_MESSAGE_MAX_LENGTH)
            {
                osip_transaction_t* transaction = NULL;
                osip_event_t* sipevent = NULL;

#ifdef WIN32   //modified by chenyu 130523

                if (NULL == inet_ntop(AF_INET, (unsigned char*) & (_from.sin_addr),
                                      _from_ip, sizeof(_from_ip)))
#else
                if (NULL == inet_ntop(AF_INET, (void*) & (_from.sin_addr),
                                      _from_ip, sizeof(_from_ip)))
#endif
                {
                    SIP_DEBUG_TRACE(LOG_ERROR, "udp_tl_execute() Get incoming message's IP error \r\n");
                    continue;
                }

                osip_strncpy(buf + message_len, "\0", 1);

                /*解析消息事件*/
                sipevent = osip_parse(buf, message_len);

                if (NULL == sipevent)
                {
                    //SIP_DEBUG_TRACE(LOG_ERROR, "\r\n +---------------------------------------------\r\n Parse Error: Received from IP=%s port=%d \r\n %s \r\n", _from_ip, ntohs(_from.sin_port), buf);
                    SIPMessageTrace(2, 1, _from_ip, ntohs(_from.sin_port), buf);
                }
                else
                {
                    if ((NULL == sipevent->sip->call_id) || (NULL == sipevent->sip->cseq) ||
                        (NULL == sipevent->sip->from) || (NULL == sipevent->sip->to) ||
                        (NULL == &sipevent->sip->vias))
                    {
                        //SIP_DEBUG_TRACE(LOG_ERROR, "\r\n +---------------------------------------------\r\n Message Error: Received from IP=%s, port=%d \r\n %s \r\n", _from_ip, ntohs(_from.sin_port), buf);
                        SIPMessageTrace(3, 1, _from_ip, ntohs(_from.sin_port), buf);

                        osip_message_free(sipevent->sip);
                        sipevent->sip = NULL;
                        osip_free(sipevent);
                        sipevent = NULL;
                    }
                    else
                    {
                        cs_fix_last_via_header(sipevent->sip, _from_ip, ntohs(_from.sin_port));

                        //SIP_DEBUG_TRACE(LOG_INFO, "\r\n +---------------------------------------------\r\n Received from IP=%s, port=%d \r\n %s \r\n", _from_ip, ntohs(_from.sin_port), buf);
                        SIPMessageTrace(0, 1, _from_ip, ntohs(_from.sin_port), buf);

                        //if (NULL != sipevent->sip->from && NULL != sipevent->sip->from->url && NULL != sipevent->sip->to && NULL != sipevent->sip->to->url)
                        //{
                        //if ((0 == sstrcmp(sipevent->sip->from->url->username, (char*)"wiscomCallerID"))
                        //&& (0 == sstrcmp(sipevent->sip->to->url->username, (char*)"wiscomCalleeID"))) /* 查询服务器ID和用户ID */
                        //{
                        //SIP_DEBUG_TRACE(LOG_DEBUG, "udp_tl_execute() Recv Get ServerID Message:From Host=%s \r\n", sipevent->sip->from->url->host);
                        //printf("\r\n ********** udp_tl_execute() Recv Get ServerID Message:From Host=%s \r\n", sipevent->sip->from->url->host);
                        //printf_system_time1();
                        //}
                        //}

                        //if (MSG_IS_REGISTER(sipevent->sip))
                        //{
                        //SIP_DEBUG_TRACE(LOG_DEBUG, "udp_tl_execute() Recv Register Message:From Host=%s \r\n", get_message_from_host(sipevent->sip));
                        //printf("\r\n ########## udp_tl_execute() Recv Register Message:From Host=%s \r\n", get_message_from_host(sipevent->sip));
                        //printf_system_time1();
                        //}

                        if (MSG_IS_REQUEST(sipevent->sip)) /* 请求消息 */
                        {
                            if (MSG_IS_MESSAGE(sipevent->sip))
                            {
                                tmp_cell = recv_msg_cell;
                            }
                            else
                            {
                                tmp_cell = recv_cell;
                            }
                        }
                        else
                        {
                            if (MSG_IS_RESPONSE_FOR(sipevent->sip, MESSAGE_METHOD)) /* 响应消息 */
                            {
                                tmp_cell = send_msg_cell;
                            }
                            else
                            {
                                tmp_cell = recv_cell;
                            }
                        }

                        i = osip_find_transaction_and_add_event(tmp_cell, sipevent);

                        if (i != 0)
                        {
                            if (MSG_IS_REQUEST(sipevent->sip))
                            {
                                if (MSG_IS_ACK(sipevent->sip))
                                {
                                    cs_cb_ist_ack_for2xx(sipevent->sip);
                                    osip_message_free(sipevent->sip);
                                    sipevent->sip = NULL;
                                    osip_free(sipevent);
                                    sipevent = NULL;
                                }
                                else
                                {
                                    transaction = osip_create_transaction(tmp_cell, sipevent);

                                    if (transaction == NULL)
                                    {
                                        //SIP_DEBUG_TRACE(LOG_ERROR, "\r\n +---------------------------------------------\r\n Create Transaction Error: Received from IP=%s, port=%d \r\n %s \r\n", _from_ip, ntohs(_from.sin_port), buf);
                                        SIPMessageTrace(4, 1, _from_ip, ntohs(_from.sin_port), buf);

                                        osip_message_free(sipevent->sip);
                                        sipevent->sip = NULL;
                                        osip_free(sipevent);
                                        sipevent = NULL;
                                        continue;
                                    }

                                    /* 设置transaction的socket */
                                    osip_transaction_set_in_socket(transaction, udp->in_socket);
                                    osip_transaction_set_out_socket(transaction, udp->out_socket);

                                    osip_transaction_add_event(transaction, sipevent);
                                }
                            }
                            else if (MSG_IS_RESPONSE(sipevent->sip))
                            {
                                if ((NULL == sipevent->sip->call_id)
                                    || (NULL == sipevent->sip->cseq)
                                    || (NULL == sipevent->sip->from)
                                    || (NULL == sipevent->sip->to))
                                {
                                    //SIP_DEBUG_TRACE(LOG_ERROR, "\r\n +---------------------------------------------\r\n Message Error: Received from IP=%s, port=%d \r\n %s \r\n", _from_ip, ntohs(_from.sin_port), buf);
                                    SIPMessageTrace(3, 1, _from_ip, ntohs(_from.sin_port), buf);

                                    osip_message_free(sipevent->sip);
                                    sipevent->sip = NULL;
                                    osip_free(sipevent);
                                    sipevent = NULL;
                                    goto loop;
                                }

                                if (MSG_IS_STATUS_2XX(sipevent->sip) && MSG_IS_RESPONSE_FOR(sipevent->sip, INVITE_METHOD))
                                {
                                    cs_cb_ict_2xx2(sipevent->sip);
                                    osip_message_free(sipevent->sip);
                                    sipevent->sip = NULL;
                                    osip_free(sipevent);
                                    sipevent = NULL;
                                }
                                else
                                {
                                    SIP_DEBUG_TRACE(LOG_WARN, "TODO: RETRANSMISSION OF A RESPONSE>200?\r\n");
                                    osip_message_free(sipevent->sip);
                                    sipevent->sip = NULL;
                                    osip_free(sipevent);
                                    sipevent = NULL;
                                }
                            }
                            else
                            {
                                osip_message_free(sipevent->sip);
                                sipevent->sip = NULL;
                                osip_free(sipevent);
                                sipevent = NULL;
                            }
                        }
                    }
                }
            }
            else
            {
                SIP_DEBUG_TRACE(LOG_ERROR, "udp_tl_execute() UDP Recv SIP Message Length Error:message_len=%d \r\n", message_len);
            }
        }
        else
        {
            SIP_DEBUG_TRACE(LOG_ERROR, "udp_tl_execute() UDP recv error \r\n");
        }

    loop:

        if (max_analysed == -1)
        {
            osip_usleep(5000);
            continue;
        }

        max_analysed--;

        if (max_analysed <= 0)
        {
            break;
        }
    }

    return 0;
}

/*****************************************************************************
 Prototype    : udp_tl_thread
 Description  : udp接收线程
 Input        : udp_tl_t * udp
 Output       : None
 Return Value :
 Calls        :
 Called By    :

  History        :
  1.Date         : 2013/3/26
    Author       : yanghaifeng
    Modification : Created function

*****************************************************************************/
// #ifdef WIN32   //modified by chenyu 131024
// unsigned int /*__stdcall*/ udp_tl_thread_win(void* ptr)
// {
//     udp_tl_t* udp = NULL;
//
//     udp = (udp_tl_t*)ptr;
//
//     if (udp == NULL)
//     {
//         SIP_DEBUG_TRACE(LOG_DEBUG, "udp_tl_thread_win() exit---: Param Error \r\n");
//         return -1;
//     }
//
//
//     udp_tl_execute(udp, g_cell, 0, 10, -1);
//     return 0;
// }
// #else
void* udp_tl_thread(void* ptr)
{
    udp_tl_t* udp = NULL;
    udp = (udp_tl_t*)ptr;

    if (udp == NULL)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "udp_tl_thread() exit---: Param Error \r\n");
        return NULL;
    }


    udp_tl_execute(udp, g_recv_cell, g_recv_message_cell, g_send_message_cell, 0, 10, -1);
    return NULL;
}
// #endif
/*****************************************************************************
 Prototype    : udp_tl_start
 Description  : 启动udp接收线程
 Input        : udp_tl_t * udp
 Output       : None
 Return Value :
 Calls        :
 Called By    :

  History        :
  1.Date         : 2013/3/26
    Author       : yanghaifeng
    Modification : Created function

*****************************************************************************/
int udp_tl_start(udp_tl_t* udp)
{
    if (udp == NULL)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "udp_tl_start() exit---: Param Error \r\n");
        return -1;
    }


    udp->thread = (osip_thread_t*)osip_thread_create(20000, udp_tl_thread, (void*)udp);

    if (udp->thread == NULL)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "udp_tl_start() exit---: UDP Thread 1 Create Error \r\n");
        return -1;
    }

    return 0;
}

/*****************************************************************************
 Prototype    : udp_tl_stop
 Description  : 停止udp接收线程
 Input        : udp_tl_t *udp
 Output       : None
 Return Value :
 Calls        :
 Called By    :

  History        :
  1.Date         : 2013/3/26
    Author       : yanghaifeng
    Modification : Created function

*****************************************************************************/
int udp_tl_stop(udp_tl_t* udp)
{
    int i = 0;

    if (udp == NULL)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "udp_tl_stop() exit---: Param Error \r\n");
        return -1;
    }

    udp->th_exit = 1;

    if (udp->thread != NULL)
    {
        i = osip_thread_join((struct osip_thread*)udp->thread);
    }

    return 0;
}

/*****************************************************************************
 Prototype    : udp_tl_init
 Description  : 初始化udp线程
 Input        : udp_tl_t** udp
                   int in_port
                   int out_port
 Output       : None
 Return Value :
 Calls        :
 Called By    :

  History        :
  1.Date         : 2013/3/26
    Author       : yanghaifeng
    Modification : Created function

*****************************************************************************/
int udp_tl_init(udp_tl_t** udp, int in_port, int out_port)
{
    struct sockaddr_in raddr;
    int option = 1;
    /*int i;*/
#ifdef WIN32
    int err;
    WSADATA wsdata;

    err = WSAStartup(MAKEWORD(2, 0), &wsdata);

    if (err != 0)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "udp_tl_init() exit---: Win Socket Init Error \r\n");
        return -1;
    }

#endif

    *udp = (udp_tl_t*) osip_malloc(sizeof(udp_tl_t));

    if (*udp == NULL)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "udp_tl_init() exit---: UDP Smalloc Error \r\n");
        return -1;
    }

    (*udp)->in_port = in_port;
    (*udp)->in_socket = (int)socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);

    if ((*udp)->in_socket == -1)
    {
        SIP_DEBUG_TRACE(LOG_FATAL, "udp_tl_init() Get in socket error\r\n");
        goto error1;
    }

    raddr.sin_addr.s_addr = htons(INADDR_ANY);
    //raddr.sin_addr.s_addr = htonl(get_ipaddr(ip));
    raddr.sin_port = htons((short)(*udp)->in_port);
    raddr.sin_family = AF_INET;

    if (bind((*udp)->in_socket, (struct sockaddr*) &raddr, sizeof(raddr)) < 0)
    {
        SIP_DEBUG_TRACE(LOG_FATAL, "udp_tl_init() bind in socket error\r\n");
        goto error2;
    }

#ifdef WIN32

    if (0 != setsockopt((*udp)->in_socket, SOL_SOCKET, SO_REUSEADDR,
                        (char*) &option, sizeof(option)))
#else
    if (0 != setsockopt((*udp)->in_socket, SOL_SOCKET, SO_REUSEADDR,
                        (void*) &option, sizeof(option)))
#endif
    {
        SIP_DEBUG_TRACE(LOG_FATAL, "udp_tl_init() in socket setsockopt error\r\n");
        goto error2;
    }

    if (out_port == in_port)
    {
        (*udp)->out_socket = (*udp)->in_socket;
        (*udp)->out_port = out_port;
    }
    else
    {
        (*udp)->out_port = out_port;
        (*udp)->out_socket = (int) socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);

        if ((*udp)->out_socket == -1)
        {
            SIP_DEBUG_TRACE(LOG_FATAL, "udp_tl_init() Get out socket error\r\n");
            goto error2;
        }

        raddr.sin_addr.s_addr = htons(INADDR_ANY);
        //raddr.sin_addr.s_addr = htonl(get_ipaddr(ip));
        raddr.sin_port = htons((short)(*udp)->out_port);
        raddr.sin_family = AF_INET;

        if (bind((*udp)->out_socket, (struct sockaddr*)&raddr, sizeof(raddr)) < 0)
        {
            SIP_DEBUG_TRACE(LOG_FATAL, "udp_tl_init() bind out socket error\r\n");
            goto error3;
        }
    }

    (*udp)->thread = NULL;
    (*udp)->th_exit = 0;

    return 0;

error3:
#ifdef WIN32
    closesocket((*udp)->out_socket);
#else
    close((*udp)->out_socket);
#endif

error2:

#ifdef WIN32
    closesocket((*udp)->in_socket);
#else
    close((*udp)->in_socket);
#endif

error1:
    osip_free(*udp);
    *udp = NULL;
    return -1;
}

/*****************************************************************************
 Prototype    : udp_tl_free
 Description  : 释放udp线程
 Input        : udp_tl_t *udp
 Output       : None
 Return Value :
 Calls        :
 Called By    :

  History        :
  1.Date         : 2013/3/26
    Author       : yanghaifeng
    Modification : Created function

*****************************************************************************/
void udp_tl_free(udp_tl_t* udp)
{
    if (NULL == udp)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "udp_tl_free() exit---: Param Error \r\n");
        return;
    }

    udp_tl_stop(udp);

    if (NULL != udp->thread)
    {
        osip_free(udp->thread);
        udp->thread = NULL;
    }

#ifdef WIN32    //modified by chenyu 130523
    closesocket(udp->in_socket);
#else
    close(udp->in_socket);
#endif

    udp->in_socket = -1;

    if (udp->in_port != udp->out_port)
    {
#ifdef WIN32    //modified by chenyu 130523
        closesocket(udp->out_socket);
#else
        close(udp->out_socket);
#endif
        udp->out_socket = -1;
    }

    osip_free(udp);
    udp = NULL;

    return;
}

/*****************************************************************************
 Prototype    : udp_list_init
 Description  : 初始化udp接收线程队列
 Input        : udp_tl_list_t ** sipudp_list
 Output       : None
 Return Value :
 Calls        :
 Called By    :

  History        :
  1.Date         : 2013/3/26
    Author       : yanghaifeng
    Modification : Created function

*****************************************************************************/
int udp_list_init()
{
    g_SIPUdpThreadList = (udp_tl_list_t*)osip_malloc(sizeof(udp_tl_list_t));

    if (g_SIPUdpThreadList == NULL)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "udp_list_init() exit---: g_SIPUdpThreadList Smalloc Error \r\n");
        return -1;
    }

    g_SIPUdpThreadList->udp_tl_list = (osip_list_t*)osip_malloc(sizeof(osip_list_t));

    if (g_SIPUdpThreadList->udp_tl_list == NULL)
    {
        osip_free(g_SIPUdpThreadList);
        g_SIPUdpThreadList = NULL;
        SIP_DEBUG_TRACE(LOG_DEBUG, "udp_list_init() exit---: UDP Tl List Init Error \r\n");
        return -1;
    }

    osip_list_init(g_SIPUdpThreadList->udp_tl_list);
    return 0;
}

/*****************************************************************************
 Prototype    : udp_list_free
 Description  : 释放udp接收线程队列
 Input        : udp_tl_list_t * sipudp_list
 Output       : None
 Return Value :
 Calls        :
 Called By    :

  History        :
  1.Date         : 2013/3/26
    Author       : yanghaifeng
    Modification : Created function

*****************************************************************************/
void udp_list_free()
{
    if (NULL == g_SIPUdpThreadList)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "udp_list_free() exit---: Param Error \r\n");
        return;
    }

    if (NULL != g_SIPUdpThreadList->udp_tl_list)
    {
        osip_list_special_free(g_SIPUdpThreadList->udp_tl_list, (void (*)(void*))&udp_tl_free);
        osip_free(g_SIPUdpThreadList->udp_tl_list);
        g_SIPUdpThreadList->udp_tl_list = NULL;
    }

    osip_free(g_SIPUdpThreadList);
    g_SIPUdpThreadList = NULL;
    return;
}

/*****************************************************************************
 Prototype    : udp_transport_thread_start
 Description  : 启动UDP接收线程
 Input        :  int* local_port
 Output       : None
 Return Value :int
 Calls        :
 Called By    :

  History        :
  1.Date         : 2013/3/26
    Author       : yanghaifeng
    Modification : Created function

*****************************************************************************/
int udp_transport_thread_start(int* local_port)
{
    int i = 0;
    udp_tl_t* sipudp = NULL;
    int iTmpLocalPort = 5062;

    /* 循环端口，初始化一路接收线程 */
    while (iTmpLocalPort <= 65535 && (i = udp_tl_init(&sipudp, iTmpLocalPort, iTmpLocalPort)) != 0)
    {
        iTmpLocalPort ++;
    }

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "udp_transport_thread_start() exit---: UDP Thread Init Error \r\n");
        return -1;
    }

    /* 启动一路接收线程 */
    i = udp_tl_start(sipudp);

    if (0 != i)
    {
        udp_tl_free(sipudp);
        sipudp = NULL;
        SIP_DEBUG_TRACE(LOG_DEBUG, "udp_transport_thread_start() exit---: UDP Thread Start Error \r\n");
        return -1;
    }

    //将sipudp 添加到接收线程队列中
    i = osip_list_add(g_SIPUdpThreadList->udp_tl_list, sipudp, -1); /* add to list tail */

    if (i < 0)
    {
        udp_tl_free(sipudp);
        sipudp = NULL;
        SIP_DEBUG_TRACE(LOG_DEBUG, "udp_transport_thread_start() exit---: List Add Error \r\n");
        return -1;
    }

    *local_port = sipudp->in_port;
    return 0;
}

/*****************************************************************************
 Prototype    : udp_transport_thread_start_by_port
 Description  : 在特定的端口上面启动UDP接收线程
 Input        :  int local_port
 Output       : None
 Return Value :int
 Calls        :
 Called By    :

  History        :
  1.Date         : 2013/3/26
    Author       : yanghaifeng
    Modification : Created function

*****************************************************************************/
int udp_transport_thread_start_by_port(int local_port)
{
    int i = 0;
    udp_tl_t* sipudp = NULL;

    /* 初始化一路接收线程 */
    i = udp_tl_init(&sipudp, local_port, local_port);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "udp_transport_thread_start_by_port() exit---: UDP Thread Init Error \r\n");
        return -1;
    }

    /* 启动一路接收线程 */
    i = udp_tl_start(sipudp);

    if (0 != i)
    {
        udp_tl_free(sipudp);
        sipudp = NULL;
        SIP_DEBUG_TRACE(LOG_DEBUG, "udp_transport_thread_start_by_port() exit---: UDP Thread Start Error \r\n");
        return -1;
    }

    //将sipudp 添加到接收线程队列中
    i = osip_list_add(g_SIPUdpThreadList->udp_tl_list, sipudp, -1); /* add to list tail */

    if (i < 0)
    {
        udp_tl_free(sipudp);
        sipudp = NULL;
        SIP_DEBUG_TRACE(LOG_DEBUG, "udp_transport_thread_start_by_port() exit---: List Add Error \r\n");
        return -1;
    }

    return 0;
}

/*****************************************************************************
 Prototype    : udp_transport_thread_stop
 Description  : 释放特定的udp接收线程
 Input        : int local_port
 Output       : None
 Return Value :
 Calls        :
 Called By    :

  History        :
  1.Date         : 2013/3/26
    Author       : yanghaifeng
    Modification : Created function

*****************************************************************************/
int udp_transport_thread_stop(int local_port)
{
    int pos = 0;
    udp_tl_t* sipudp = NULL;

    if (local_port <= 0)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "udp_transport_thread_stop() exit---: Param Error \r\n");
        return -1;
    }

    if ((NULL == g_SIPUdpThreadList) || (NULL == g_SIPUdpThreadList->udp_tl_list))
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "udp_transport_thread_stop() exit---: g_SIPUdpThreadList Error \r\n");
        return -1;
    }

    //查找队列，将sipudp从接收线程队列中移除
    for (pos = 0; pos < osip_list_size(g_SIPUdpThreadList->udp_tl_list); pos++)
    {
        sipudp = (udp_tl_t*)osip_list_get(g_SIPUdpThreadList->udp_tl_list, pos);

        if (sipudp == NULL)
        {
            continue;
        }

        if (sipudp->in_port <= 0)
        {
            continue;
        }

        if (sipudp->in_port == local_port)
        {
            osip_list_remove(g_SIPUdpThreadList->udp_tl_list, pos);
            udp_tl_free(sipudp);
            sipudp = NULL;
            return 0;
        }
    }

    SIP_DEBUG_TRACE(LOG_DEBUG, "udp_transport_thread_stop() exit---: Not Found \r\n");
    return -1;
}

/*****************************************************************************
 函 数 名  : get_socket_by_port
 功能描述  : 根据端口号获取对应的socket
 输入参数  : int port
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年5月23日 星期四
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int get_socket_by_port(int port)
{
    int pos = 0;
    udp_tl_t* sipudp = NULL;

    if ((NULL == g_SIPUdpThreadList) || (NULL == g_SIPUdpThreadList->udp_tl_list))
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "get_socket_by_port() exit---: Param Error \r\n");
        return -1;
    }

    for (pos = 0; pos < osip_list_size(g_SIPUdpThreadList->udp_tl_list); pos++)
    {
        sipudp = (udp_tl_t*)osip_list_get(g_SIPUdpThreadList->udp_tl_list, pos);

        if ((sipudp == NULL) || (-1 == sipudp->in_socket))
        {
            continue;
        }

        if (sipudp->in_port == port)
        {
            return sipudp->in_socket;
        }
        else if (sipudp->out_port == port)
        {
            return sipudp->out_socket;
        }
    }

    SIP_DEBUG_TRACE(LOG_DEBUG, "get_socket_by_port() exit---: Not Found \r\n");
    return -1;
}
#endif

#if DECS("SIP运行主线程")
/*****************************************************************************
 函 数 名  : sip_stack_for_recv_run_thread_execute
 功能描述  : 接收消息协议栈处理主函数
 输入参数  : void* p
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年12月11日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
void* sip_stack_for_recv_run_thread_execute(void* p)
{
    //time_t utc_time1, utc_time2, utc_time3, utc_time4, utc_time5, utc_time6, utc_time7, utc_time8, utc_time9, utc_time10;
    run_thread_t* run = (run_thread_t*)p;

    if (run == NULL)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_stack_for_recv_run_thread_execute() exit---: Param Error \r\n");
        return NULL;
    }

    while (!run->th_exit)
    {
        //utc_time1 = time(NULL);

        osip_timers_ict_execute(g_recv_cell);

        //utc_time2 = time(NULL);

        /*
          if (utc_time2 - utc_time1 >= 1)
          {
              printf("\r\n$$$$$$$$$$ osip_timers_ict_execute time=%d $$$$$$$$$$\r\n", utc_time2 - utc_time1);
              printf("\r\n########## osip_timers_ict_execute:osip_list_size(&osip->osip_ict_transactions)=%d #########\r\n", osip_list_size(&g_cell->osip_ict_transactions));
          }
          */

        osip_timers_ist_execute(g_recv_cell);

        /*
          utc_time3 = time(NULL);

          if (utc_time3 - utc_time2 >= 1)
          {
              printf("\r\n$$$$$$$$$$ osip_timers_ist_execute time=%d $$$$$$$$$$\r\n", utc_time3 - utc_time2);
              printf("\r\n########## osip_timers_ist_execute:osip_list_size(&osip->osip_ist_transactions)=%d #########\r\n", osip_list_size(&g_cell->osip_ist_transactions));
          }
          */

        osip_timers_nict_execute(g_recv_cell);

        /*
          utc_time4 = time(NULL);

          if (utc_time4 - utc_time3 >= 1)
          {
              printf("\r\n$$$$$$$$$$ osip_timers_nict_execute time=%d $$$$$$$$$$\r\n", utc_time4 - utc_time3);
              printf("\r\n########## osip_timers_nict_execute:osip_list_size(&osip->osip_nict_transactions)=%d #########\r\n", osip_list_size(&g_cell->osip_nict_transactions));
          }
          */

        osip_timers_nist_execute(g_recv_cell);

        /*
          utc_time5 = time(NULL);

          if (utc_time5 - utc_time4 >= 1)
          {
              printf("\r\n$$$$$$$$$$ osip_timers_nist_execute time=%d $$$$$$$$$$\r\n", utc_time5 - utc_time4);
              printf("\r\n########## osip_timers_nist_execute:osip_list_size(&osip->osip_nist_transactions)=%d #########\r\n", osip_list_size(&g_cell->osip_nist_transactions));
          }
          */

        osip_ict_execute(g_recv_cell);

        /*
          utc_time6 = time(NULL);

          if (utc_time6 - utc_time5 >= 1)
          {
              printf("\r\n$$$$$$$$$$ osip_ict_execute time=%d $$$$$$$$$$\r\n", utc_time6 - utc_time5);
              printf("\r\n########## osip_ict_execute:osip_list_size(&osip->osip_ict_transactions)=%d #########\r\n", osip_list_size(&g_cell->osip_ict_transactions));
          }
          */

        osip_ist_execute(g_recv_cell);

        /*
          utc_time7 = time(NULL);

          if (utc_time7 - utc_time6 >= 1)
          {
              printf("\r\n$$$$$$$$$$ osip_ist_execute time=%d $$$$$$$$$$\r\n", utc_time7 - utc_time6);
              printf("\r\n########## osip_ist_execute:osip_list_size(&osip->osip_ist_transactions)=%d #########\r\n", osip_list_size(&g_cell->osip_ist_transactions));
          }
          */

        osip_nict_execute(g_recv_cell);

        /*
          utc_time8 = time(NULL);

          if (utc_time8 - utc_time7 >= 1)
          {
              printf("\r\n$$$$$$$$$$ osip_nict_execute time=%d $$$$$$$$$$\r\n", utc_time8 - utc_time7);
              printf("\r\n########## osip_nict_execute:osip_list_size(&osip->osip_nict_transactions)=%d #########\r\n", osip_list_size(&g_cell->osip_nict_transactions));
          }
          */

        osip_nist_execute(g_recv_cell);

        /*
          utc_time9 = time(NULL);

          if (utc_time9 - utc_time8 >= 1)
          {
              printf("\r\n$$$$$$$$$$ osip_nist_execute time=%d $$$$$$$$$$\r\n", utc_time9 - utc_time8);
              printf("\r\n########## osip_nist_execute:osip_list_size(&osip->osip_nist_transactions)=%d #########\r\n", osip_list_size(&g_cell->osip_nist_transactions));
          }
          */

        clean_garbage(g_recv_garbage);

        /*
          utc_time10 = time(NULL);

          if (utc_time10 - utc_time9 >= 1)
          {
              printf("\r\n$$$$$$$$$$ clean_garbage time=%d $$$$$$$$$$\r\n", utc_time10 - utc_time9);
          }
          */

        //SIP_DEBUG_TRACE(LOG_TRACE, "sip_stack_run_thread_execute() ict transactions size=%d \r\n", osip_list_size(&g_cell->osip_ict_transactions));
        //SIP_DEBUG_TRACE(LOG_TRACE, "sip_stack_run_thread_execute() ist transactions size=%d \r\n", osip_list_size(&g_cell->osip_ist_transactions));
        //SIP_DEBUG_TRACE(LOG_TRACE, "sip_stack_run_thread_execute() nict transactions size=%d \r\n", osip_list_size(&g_cell->osip_nict_transactions));
        //SIP_DEBUG_TRACE(LOG_TRACE, "sip_stack_run_thread_execute() nist transactions size=%d \r\n", osip_list_size(&g_cell->osip_nist_transactions));

#ifdef  WIN32
        osip_usleep(5000);
#else
        osip_usleep(5000);
#endif
    }

    return NULL;
}

/*****************************************************************************
 函 数 名  : sip_stack_for_recv_register_run_thread_execute
 功能描述  : 接收Register消息协议栈处理主函数
 输入参数  : void* p
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年12月11日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
void* sip_stack_for_recv_register_run_thread_execute(void* p)
{
    //time_t utc_time1, utc_time2, utc_time3, utc_time4, utc_time5, utc_time6, utc_time7, utc_time8, utc_time9, utc_time10;
    run_thread_t* run = (run_thread_t*)p;

    if (run == NULL)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_stack_for_recv_register_run_thread_execute() exit---: Param Error \r\n");
        return NULL;
    }

    while (!run->th_exit)
    {
        //utc_time1 = time(NULL);

        osip_timers_ict_execute(g_recv_register_cell);

        //utc_time2 = time(NULL);

        /*
          if (utc_time2 - utc_time1 >= 1)
          {
              printf("\r\n$$$$$$$$$$ osip_timers_ict_execute time=%d $$$$$$$$$$\r\n", utc_time2 - utc_time1);
              printf("\r\n########## osip_timers_ict_execute:osip_list_size(&osip->osip_ict_transactions)=%d #########\r\n", osip_list_size(&g_cell->osip_ict_transactions));
          }
          */

        osip_timers_ist_execute(g_recv_register_cell);

        /*
          utc_time3 = time(NULL);

          if (utc_time3 - utc_time2 >= 1)
          {
              printf("\r\n$$$$$$$$$$ osip_timers_ist_execute time=%d $$$$$$$$$$\r\n", utc_time3 - utc_time2);
              printf("\r\n########## osip_timers_ist_execute:osip_list_size(&osip->osip_ist_transactions)=%d #########\r\n", osip_list_size(&g_cell->osip_ist_transactions));
          }
          */

        osip_timers_nict_execute(g_recv_register_cell);

        /*
          utc_time4 = time(NULL);

          if (utc_time4 - utc_time3 >= 1)
          {
              printf("\r\n$$$$$$$$$$ osip_timers_nict_execute time=%d $$$$$$$$$$\r\n", utc_time4 - utc_time3);
              printf("\r\n########## osip_timers_nict_execute:osip_list_size(&osip->osip_nict_transactions)=%d #########\r\n", osip_list_size(&g_cell->osip_nict_transactions));
          }
          */

        osip_timers_nist_execute(g_recv_register_cell);

        /*
          utc_time5 = time(NULL);

          if (utc_time5 - utc_time4 >= 1)
          {
              printf("\r\n$$$$$$$$$$ osip_timers_nist_execute time=%d $$$$$$$$$$\r\n", utc_time5 - utc_time4);
              printf("\r\n########## osip_timers_nist_execute:osip_list_size(&osip->osip_nist_transactions)=%d #########\r\n", osip_list_size(&g_cell->osip_nist_transactions));
          }
          */

        osip_ict_execute(g_recv_register_cell);

        /*
          utc_time6 = time(NULL);

          if (utc_time6 - utc_time5 >= 1)
          {
              printf("\r\n$$$$$$$$$$ osip_ict_execute time=%d $$$$$$$$$$\r\n", utc_time6 - utc_time5);
              printf("\r\n########## osip_ict_execute:osip_list_size(&osip->osip_ict_transactions)=%d #########\r\n", osip_list_size(&g_cell->osip_ict_transactions));
          }
          */

        osip_ist_execute(g_recv_register_cell);

        /*
          utc_time7 = time(NULL);

          if (utc_time7 - utc_time6 >= 1)
          {
              printf("\r\n$$$$$$$$$$ osip_ist_execute time=%d $$$$$$$$$$\r\n", utc_time7 - utc_time6);
              printf("\r\n########## osip_ist_execute:osip_list_size(&osip->osip_ist_transactions)=%d #########\r\n", osip_list_size(&g_cell->osip_ist_transactions));
          }
          */

        osip_nict_execute(g_recv_register_cell);

        /*
          utc_time8 = time(NULL);

          if (utc_time8 - utc_time7 >= 1)
          {
              printf("\r\n$$$$$$$$$$ osip_nict_execute time=%d $$$$$$$$$$\r\n", utc_time8 - utc_time7);
              printf("\r\n########## osip_nict_execute:osip_list_size(&osip->osip_nict_transactions)=%d #########\r\n", osip_list_size(&g_cell->osip_nict_transactions));
          }
          */

        osip_nist_execute(g_recv_register_cell);

        /*
          utc_time9 = time(NULL);

          if (utc_time9 - utc_time8 >= 1)
          {
              printf("\r\n$$$$$$$$$$ osip_nist_execute time=%d $$$$$$$$$$\r\n", utc_time9 - utc_time8);
              printf("\r\n########## osip_nist_execute:osip_list_size(&osip->osip_nist_transactions)=%d #########\r\n", osip_list_size(&g_cell->osip_nist_transactions));
          }
          */

        clean_garbage(g_recv_register_garbage);

        /*
          utc_time10 = time(NULL);

          if (utc_time10 - utc_time9 >= 1)
          {
              printf("\r\n$$$$$$$$$$ clean_garbage time=%d $$$$$$$$$$\r\n", utc_time10 - utc_time9);
          }
          */

        //SIP_DEBUG_TRACE(LOG_TRACE, "sip_stack_run_thread_execute() ict transactions size=%d \r\n", osip_list_size(&g_cell->osip_ict_transactions));
        //SIP_DEBUG_TRACE(LOG_TRACE, "sip_stack_run_thread_execute() ist transactions size=%d \r\n", osip_list_size(&g_cell->osip_ist_transactions));
        //SIP_DEBUG_TRACE(LOG_TRACE, "sip_stack_run_thread_execute() nict transactions size=%d \r\n", osip_list_size(&g_cell->osip_nict_transactions));
        //SIP_DEBUG_TRACE(LOG_TRACE, "sip_stack_run_thread_execute() nist transactions size=%d \r\n", osip_list_size(&g_cell->osip_nist_transactions));

#ifdef  WIN32
        osip_usleep(5000);
#else
        osip_usleep(5000);
#endif
    }

    return NULL;
}

/*****************************************************************************
 函 数 名  : sip_stack_for_recv_msg_run_thread_execute
 功能描述  : 接收Message消息协议栈处理主函数
 输入参数  : void* p
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年12月11日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
void* sip_stack_for_recv_msg_run_thread_execute(void* p)
{
    //time_t utc_time1, utc_time2, utc_time3, utc_time4, utc_time5, utc_time6, utc_time7, utc_time8, utc_time9, utc_time10;
    run_thread_t* run = (run_thread_t*)p;

    if (run == NULL)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_stack_for_recv_msg_run_thread_execute() exit---: Param Error \r\n");
        return NULL;
    }

    while (!run->th_exit)
    {
        //utc_time1 = time(NULL);

        osip_timers_ict_execute(g_recv_message_cell);

        //utc_time2 = time(NULL);

        /*
          if (utc_time2 - utc_time1 >= 1)
          {
              printf("\r\n$$$$$$$$$$ osip_timers_ict_execute time=%d $$$$$$$$$$\r\n", utc_time2 - utc_time1);
              printf("\r\n########## osip_timers_ict_execute:osip_list_size(&osip->osip_ict_transactions)=%d #########\r\n", osip_list_size(&g_cell->osip_ict_transactions));
          }
          */

        osip_timers_ist_execute(g_recv_message_cell);

        /*
          utc_time3 = time(NULL);

          if (utc_time3 - utc_time2 >= 1)
          {
              printf("\r\n$$$$$$$$$$ osip_timers_ist_execute time=%d $$$$$$$$$$\r\n", utc_time3 - utc_time2);
              printf("\r\n########## osip_timers_ist_execute:osip_list_size(&osip->osip_ist_transactions)=%d #########\r\n", osip_list_size(&g_cell->osip_ist_transactions));
          }
          */

        osip_timers_nict_execute(g_recv_message_cell);

        /*
          utc_time4 = time(NULL);

          if (utc_time4 - utc_time3 >= 1)
          {
              printf("\r\n$$$$$$$$$$ osip_timers_nict_execute time=%d $$$$$$$$$$\r\n", utc_time4 - utc_time3);
              printf("\r\n########## osip_timers_nict_execute:osip_list_size(&osip->osip_nict_transactions)=%d #########\r\n", osip_list_size(&g_cell->osip_nict_transactions));
          }
          */

        osip_timers_nist_execute(g_recv_message_cell);

        /*
          utc_time5 = time(NULL);

          if (utc_time5 - utc_time4 >= 1)
          {
              printf("\r\n$$$$$$$$$$ osip_timers_nist_execute time=%d $$$$$$$$$$\r\n", utc_time5 - utc_time4);
              printf("\r\n########## osip_timers_nist_execute:osip_list_size(&osip->osip_nist_transactions)=%d #########\r\n", osip_list_size(&g_cell->osip_nist_transactions));
          }
          */

        osip_ict_execute(g_recv_message_cell);

        /*
          utc_time6 = time(NULL);

          if (utc_time6 - utc_time5 >= 1)
          {
              printf("\r\n$$$$$$$$$$ osip_ict_execute time=%d $$$$$$$$$$\r\n", utc_time6 - utc_time5);
              printf("\r\n########## osip_ict_execute:osip_list_size(&osip->osip_ict_transactions)=%d #########\r\n", osip_list_size(&g_cell->osip_ict_transactions));
          }
          */

        osip_ist_execute(g_recv_message_cell);

        /*
          utc_time7 = time(NULL);

          if (utc_time7 - utc_time6 >= 1)
          {
              printf("\r\n$$$$$$$$$$ osip_ist_execute time=%d $$$$$$$$$$\r\n", utc_time7 - utc_time6);
              printf("\r\n########## osip_ist_execute:osip_list_size(&osip->osip_ist_transactions)=%d #########\r\n", osip_list_size(&g_cell->osip_ist_transactions));
          }
          */

        osip_nict_execute(g_recv_message_cell);

        /*
          utc_time8 = time(NULL);

          if (utc_time8 - utc_time7 >= 1)
          {
              printf("\r\n$$$$$$$$$$ osip_nict_execute time=%d $$$$$$$$$$\r\n", utc_time8 - utc_time7);
              printf("\r\n########## osip_nict_execute:osip_list_size(&osip->osip_nict_transactions)=%d #########\r\n", osip_list_size(&g_cell->osip_nict_transactions));
          }
          */

        osip_nist_execute(g_recv_message_cell);

        /*
          utc_time9 = time(NULL);

          if (utc_time9 - utc_time8 >= 1)
          {
              printf("\r\n$$$$$$$$$$ osip_nist_execute time=%d $$$$$$$$$$\r\n", utc_time9 - utc_time8);
              printf("\r\n########## osip_nist_execute:osip_list_size(&osip->osip_nist_transactions)=%d #########\r\n", osip_list_size(&g_cell->osip_nist_transactions));
          }
          */

        clean_garbage(g_recv_msg_garbage);

        /*
          utc_time10 = time(NULL);

          if (utc_time10 - utc_time9 >= 1)
          {
              printf("\r\n$$$$$$$$$$ clean_garbage time=%d $$$$$$$$$$\r\n", utc_time10 - utc_time9);
          }
          */

        //SIP_DEBUG_TRACE(LOG_TRACE, "sip_stack_run_thread_execute() ict transactions size=%d \r\n", osip_list_size(&g_cell->osip_ict_transactions));
        //SIP_DEBUG_TRACE(LOG_TRACE, "sip_stack_run_thread_execute() ist transactions size=%d \r\n", osip_list_size(&g_cell->osip_ist_transactions));
        //SIP_DEBUG_TRACE(LOG_TRACE, "sip_stack_run_thread_execute() nict transactions size=%d \r\n", osip_list_size(&g_cell->osip_nict_transactions));
        //SIP_DEBUG_TRACE(LOG_TRACE, "sip_stack_run_thread_execute() nist transactions size=%d \r\n", osip_list_size(&g_cell->osip_nist_transactions));

#ifdef  WIN32
        osip_usleep(5000);
#else
        osip_usleep(5000);
#endif
    }

    return NULL;
}

/*****************************************************************************
 函 数 名  : sip_stack_for_send_run_thread_execute
 功能描述  : 发送消息协议栈处理主函数
 输入参数  : void* p
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年12月11日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
void* sip_stack_for_send_run_thread_execute(void* p)
{
    //time_t utc_time1, utc_time2, utc_time3, utc_time4, utc_time5, utc_time6, utc_time7, utc_time8, utc_time9, utc_time10;
    run_thread_t* run = (run_thread_t*)p;

    if (run == NULL)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_stack_for_send_run_thread_execute() exit---: Param Error \r\n");
        return NULL;
    }

    while (!run->th_exit)
    {
        //utc_time1 = time(NULL);

        osip_timers_ict_execute(g_send_cell);

        //utc_time2 = time(NULL);

        /*
          if (utc_time2 - utc_time1 >= 1)
          {
              printf("\r\n$$$$$$$$$$ osip_timers_ict_execute time=%d $$$$$$$$$$\r\n", utc_time2 - utc_time1);
              printf("\r\n########## osip_timers_ict_execute:osip_list_size(&osip->osip_ict_transactions)=%d #########\r\n", osip_list_size(&g_cell->osip_ict_transactions));
          }
          */

        osip_timers_ist_execute(g_send_cell);

        /*
          utc_time3 = time(NULL);

          if (utc_time3 - utc_time2 >= 1)
          {
              printf("\r\n$$$$$$$$$$ osip_timers_ist_execute time=%d $$$$$$$$$$\r\n", utc_time3 - utc_time2);
              printf("\r\n########## osip_timers_ist_execute:osip_list_size(&osip->osip_ist_transactions)=%d #########\r\n", osip_list_size(&g_cell->osip_ist_transactions));
          }
          */

        osip_timers_nict_execute(g_send_cell);

        /*
          utc_time4 = time(NULL);

          if (utc_time4 - utc_time3 >= 1)
          {
              printf("\r\n$$$$$$$$$$ osip_timers_nict_execute time=%d $$$$$$$$$$\r\n", utc_time4 - utc_time3);
              printf("\r\n########## osip_timers_nict_execute:osip_list_size(&osip->osip_nict_transactions)=%d #########\r\n", osip_list_size(&g_cell->osip_nict_transactions));
          }
          */

        osip_timers_nist_execute(g_send_cell);

        /*
          utc_time5 = time(NULL);

          if (utc_time5 - utc_time4 >= 1)
          {
              printf("\r\n$$$$$$$$$$ osip_timers_nist_execute time=%d $$$$$$$$$$\r\n", utc_time5 - utc_time4);
              printf("\r\n########## osip_timers_nist_execute:osip_list_size(&osip->osip_nist_transactions)=%d #########\r\n", osip_list_size(&g_cell->osip_nist_transactions));
          }
          */

        osip_ict_execute(g_send_cell);

        /*
          utc_time6 = time(NULL);

          if (utc_time6 - utc_time5 >= 1)
          {
              printf("\r\n$$$$$$$$$$ osip_ict_execute time=%d $$$$$$$$$$\r\n", utc_time6 - utc_time5);
              printf("\r\n########## osip_ict_execute:osip_list_size(&osip->osip_ict_transactions)=%d #########\r\n", osip_list_size(&g_cell->osip_ict_transactions));
          }
          */

        osip_ist_execute(g_send_cell);

        /*
          utc_time7 = time(NULL);

          if (utc_time7 - utc_time6 >= 1)
          {
              printf("\r\n$$$$$$$$$$ osip_ist_execute time=%d $$$$$$$$$$\r\n", utc_time7 - utc_time6);
              printf("\r\n########## osip_ist_execute:osip_list_size(&osip->osip_ist_transactions)=%d #########\r\n", osip_list_size(&g_cell->osip_ist_transactions));
          }
          */

        osip_nict_execute(g_send_cell);

        /*
          utc_time8 = time(NULL);

          if (utc_time8 - utc_time7 >= 1)
          {
              printf("\r\n$$$$$$$$$$ osip_nict_execute time=%d $$$$$$$$$$\r\n", utc_time8 - utc_time7);
              printf("\r\n########## osip_nict_execute:osip_list_size(&osip->osip_nict_transactions)=%d #########\r\n", osip_list_size(&g_cell->osip_nict_transactions));
          }
          */

        osip_nist_execute(g_send_cell);

        /*
          utc_time9 = time(NULL);

          if (utc_time9 - utc_time8 >= 1)
          {
              printf("\r\n$$$$$$$$$$ osip_nist_execute time=%d $$$$$$$$$$\r\n", utc_time9 - utc_time8);
              printf("\r\n########## osip_nist_execute:osip_list_size(&osip->osip_nist_transactions)=%d #########\r\n", osip_list_size(&g_cell->osip_nist_transactions));
          }
          */

        clean_garbage(g_send_garbage);

        /*
          utc_time10 = time(NULL);

          if (utc_time10 - utc_time9 >= 1)
          {
              printf("\r\n$$$$$$$$$$ clean_garbage time=%d $$$$$$$$$$\r\n", utc_time10 - utc_time9);
          }
          */

        //SIP_DEBUG_TRACE(LOG_TRACE, "sip_stack_run_thread_execute() ict transactions size=%d \r\n", osip_list_size(&g_cell->osip_ict_transactions));
        //SIP_DEBUG_TRACE(LOG_TRACE, "sip_stack_run_thread_execute() ist transactions size=%d \r\n", osip_list_size(&g_cell->osip_ist_transactions));
        //SIP_DEBUG_TRACE(LOG_TRACE, "sip_stack_run_thread_execute() nict transactions size=%d \r\n", osip_list_size(&g_cell->osip_nict_transactions));
        //SIP_DEBUG_TRACE(LOG_TRACE, "sip_stack_run_thread_execute() nist transactions size=%d \r\n", osip_list_size(&g_cell->osip_nist_transactions));

#ifdef  WIN32
        osip_usleep(5000);
#else
        osip_usleep(5000);
#endif
    }

    return NULL;
}

/*****************************************************************************
 函 数 名  : sip_stack_for_send_msg_run_thread_execute
 功能描述  : 发送Message消息协议栈处理主函数
 输入参数  : void* p
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年12月11日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
void* sip_stack_for_send_msg_run_thread_execute(void* p)
{
    //time_t utc_time1, utc_time2, utc_time3, utc_time4, utc_time5, utc_time6, utc_time7, utc_time8, utc_time9, utc_time10;
    run_thread_t* run = (run_thread_t*)p;

    if (run == NULL)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_stack_for_send_msg_run_thread_execute() exit---: Param Error \r\n");
        return NULL;
    }

    while (!run->th_exit)
    {
        //utc_time1 = time(NULL);

        osip_timers_ict_execute(g_send_message_cell);

        //utc_time2 = time(NULL);

        /*
          if (utc_time2 - utc_time1 >= 1)
          {
              printf("\r\n$$$$$$$$$$ osip_timers_ict_execute time=%d $$$$$$$$$$\r\n", utc_time2 - utc_time1);
              printf("\r\n########## osip_timers_ict_execute:osip_list_size(&osip->osip_ict_transactions)=%d #########\r\n", osip_list_size(&g_cell->osip_ict_transactions));
          }
          */

        osip_timers_ist_execute(g_send_message_cell);

        /*
          utc_time3 = time(NULL);

          if (utc_time3 - utc_time2 >= 1)
          {
              printf("\r\n$$$$$$$$$$ osip_timers_ist_execute time=%d $$$$$$$$$$\r\n", utc_time3 - utc_time2);
              printf("\r\n########## osip_timers_ist_execute:osip_list_size(&osip->osip_ist_transactions)=%d #########\r\n", osip_list_size(&g_cell->osip_ist_transactions));
          }
          */

        osip_timers_nict_execute(g_send_message_cell);

        /*
          utc_time4 = time(NULL);

          if (utc_time4 - utc_time3 >= 1)
          {
              printf("\r\n$$$$$$$$$$ osip_timers_nict_execute time=%d $$$$$$$$$$\r\n", utc_time4 - utc_time3);
              printf("\r\n########## osip_timers_nict_execute:osip_list_size(&osip->osip_nict_transactions)=%d #########\r\n", osip_list_size(&g_cell->osip_nict_transactions));
          }
          */

        osip_timers_nist_execute(g_send_message_cell);

        /*
          utc_time5 = time(NULL);

          if (utc_time5 - utc_time4 >= 1)
          {
              printf("\r\n$$$$$$$$$$ osip_timers_nist_execute time=%d $$$$$$$$$$\r\n", utc_time5 - utc_time4);
              printf("\r\n########## osip_timers_nist_execute:osip_list_size(&osip->osip_nist_transactions)=%d #########\r\n", osip_list_size(&g_cell->osip_nist_transactions));
          }
          */

        osip_ict_execute(g_send_message_cell);

        /*
          utc_time6 = time(NULL);

          if (utc_time6 - utc_time5 >= 1)
          {
              printf("\r\n$$$$$$$$$$ osip_ict_execute time=%d $$$$$$$$$$\r\n", utc_time6 - utc_time5);
              printf("\r\n########## osip_ict_execute:osip_list_size(&osip->osip_ict_transactions)=%d #########\r\n", osip_list_size(&g_cell->osip_ict_transactions));
          }
          */

        osip_ist_execute(g_send_message_cell);

        /*
          utc_time7 = time(NULL);

          if (utc_time7 - utc_time6 >= 1)
          {
              printf("\r\n$$$$$$$$$$ osip_ist_execute time=%d $$$$$$$$$$\r\n", utc_time7 - utc_time6);
              printf("\r\n########## osip_ist_execute:osip_list_size(&osip->osip_ist_transactions)=%d #########\r\n", osip_list_size(&g_cell->osip_ist_transactions));
          }
          */

        osip_nict_execute(g_send_message_cell);

        /*
          utc_time8 = time(NULL);

          if (utc_time8 - utc_time7 >= 1)
          {
              printf("\r\n$$$$$$$$$$ osip_nict_execute time=%d $$$$$$$$$$\r\n", utc_time8 - utc_time7);
              printf("\r\n########## osip_nict_execute:osip_list_size(&osip->osip_nict_transactions)=%d #########\r\n", osip_list_size(&g_cell->osip_nict_transactions));
          }
          */

        osip_nist_execute(g_send_message_cell);

        /*
          utc_time9 = time(NULL);

          if (utc_time9 - utc_time8 >= 1)
          {
              printf("\r\n$$$$$$$$$$ osip_nist_execute time=%d $$$$$$$$$$\r\n", utc_time9 - utc_time8);
              printf("\r\n########## osip_nist_execute:osip_list_size(&osip->osip_nist_transactions)=%d #########\r\n", osip_list_size(&g_cell->osip_nist_transactions));
          }
          */

        clean_garbage(g_send_msg_garbage);

        /*
          utc_time10 = time(NULL);

          if (utc_time10 - utc_time9 >= 1)
          {
              printf("\r\n$$$$$$$$$$ clean_garbage time=%d $$$$$$$$$$\r\n", utc_time10 - utc_time9);
          }
          */

        //SIP_DEBUG_TRACE(LOG_TRACE, "sip_stack_run_thread_execute() ict transactions size=%d \r\n", osip_list_size(&g_cell->osip_ict_transactions));
        //SIP_DEBUG_TRACE(LOG_TRACE, "sip_stack_run_thread_execute() ist transactions size=%d \r\n", osip_list_size(&g_cell->osip_ist_transactions));
        //SIP_DEBUG_TRACE(LOG_TRACE, "sip_stack_run_thread_execute() nict transactions size=%d \r\n", osip_list_size(&g_cell->osip_nict_transactions));
        //SIP_DEBUG_TRACE(LOG_TRACE, "sip_stack_run_thread_execute() nist transactions size=%d \r\n", osip_list_size(&g_cell->osip_nist_transactions));

#ifdef  WIN32
        osip_usleep(5000);
#else
        osip_usleep(5000);
#endif
    }

    return NULL;
}

/*****************************************************************************
 函 数 名  : sip_app_run_thread_execute
 功能描述  : SIP协议栈应用层线程处理主函数
 输入参数  : void* p
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年12月11日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
void* sip_app_run_thread_execute(void* p)
{
    //time_t utc_time1, utc_time2, utc_time3, utc_time4;
    run_thread_t* run = (run_thread_t*)p;

    if (run == NULL)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_app_run_thread_execute() exit---: Param Error \r\n");
        return NULL;
    }

    while (!run->th_exit)
    {
        //utc_time1 = time(NULL);
        /*todo:scan ua timers */
        ua_scan_timer_list();

        //utc_time2 = time(NULL);

        /*
          if (utc_time2 - utc_time1 >= 1)
          {
              printf("\r\n$$$$$$$$$$ ua scan timer list time=%d $$$$$$$$$$\r\n", utc_time2 - utc_time1);
          }
          */

        cs_scan_timer_event();

        /*
          utc_time3 = time(NULL);

          if (utc_time3 - utc_time2 >= 1)
          {
              printf("\r\n$$$$$$$$$$ cs scan timer event time=%d $$$$$$$$$$\r\n", utc_time3 - utc_time2);
          }
          */

        scan_ua_dialogs();

        /*
          utc_time4 = time(NULL);

          if (utc_time4 - utc_time3 >= 1)
          {
              printf("\r\n$$$$$$$$$$ scan ua dialogs time=%d $$$$$$$$$$\r\n", utc_time4 - utc_time3);
          }
          */

#ifdef  WIN32
        osip_usleep(5000);
#else
        osip_usleep(5000);
#endif
    }

    return NULL;
}

/*****************************************************************************
 Prototype    : sip_run_thread_init
 Description  : SIP事务处理线程初始化
 Input        : run_thread_t ** run
 Output       : None
 Return Value :
 Calls        :
 Called By    :

  History        :
  1.Date         : 2013/3/27
    Author       : yanghaifeng
    Modification : Created function

*****************************************************************************/
int sip_run_thread_init(run_thread_t** run)
{
    *run = (run_thread_t*)osip_malloc(sizeof(run_thread_t));

    if (NULL == *run)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_run_thread_init() exit---: *run Smalloc Error \r\n");
        return -1;
    }

    (*run)->thread = NULL;
    (*run)->th_exit = 0;
    return 0;
}

/*****************************************************************************
 Prototype    : sip_run_thread_free
 Description  : SIP事务处理线程释放
 Input        : run_thread_t * run
 Output       : None
 Return Value :
 Calls        :
 Called By    :

  History        :
  1.Date         : 2013/3/27
    Author       : yanghaifeng
    Modification : Created function

*****************************************************************************/
void sip_run_thread_free(run_thread_t* run)
{
    if (run == NULL)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_run_thread_free() exit---: Param Error \r\n");
        return;
    }

    if (run->thread)
    {
        osip_free(run->thread);
        run->thread = NULL;
    }

    return;
}

/*****************************************************************************
 Prototype    : sip_run_thread_start
 Description  : 启动SIP事务处理主线程
 Input        : None
 Output       : None
 Return Value :
 Calls        :
 Called By    :

  History        :
  1.Date         : 2013/3/27
    Author       : yanghaifeng
    Modification : Created function

*****************************************************************************/
int sip_run_thread_start()
{
    int i;

    /* 初始化Stack 线程 */
    i = sip_run_thread_init(&g_SIPStackForRecvRunThread);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_run_thread_start() exit---: SIP Stack Run Thread For Recv Init Error \r\n");
        return -1;
    }

    i = sip_run_thread_init(&g_SIPStackForRecvRegisterRunThread);

    if (i != 0)
    {
        sip_run_thread_free(g_SIPStackForRecvRunThread);
        osip_free(g_SIPStackForRecvRunThread);
        g_SIPStackForRecvRunThread = NULL;
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_run_thread_start() exit---: SIP Stack Run Thread For Recv Register Init Error \r\n");
        return -1;
    }

    i = sip_run_thread_init(&g_SIPStackForRecvMsgRunThread);

    if (i != 0)
    {
        sip_run_thread_free(g_SIPStackForRecvRunThread);
        osip_free(g_SIPStackForRecvRunThread);
        g_SIPStackForRecvRunThread = NULL;

        sip_run_thread_free(g_SIPStackForRecvRegisterRunThread);
        osip_free(g_SIPStackForRecvRegisterRunThread);
        g_SIPStackForRecvRegisterRunThread = NULL;

        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_run_thread_start() exit---: SIP Stack Run Thread For Recv Msg Init Error \r\n");
        return -1;
    }

    i = sip_run_thread_init(&g_SIPStackForSendRunThread);

    if (i != 0)
    {
        sip_run_thread_free(g_SIPStackForRecvRunThread);
        osip_free(g_SIPStackForRecvRunThread);
        g_SIPStackForRecvRunThread = NULL;

        sip_run_thread_free(g_SIPStackForRecvRegisterRunThread);
        osip_free(g_SIPStackForRecvRegisterRunThread);
        g_SIPStackForRecvRegisterRunThread = NULL;

        sip_run_thread_free(g_SIPStackForRecvMsgRunThread);
        osip_free(g_SIPStackForRecvMsgRunThread);
        g_SIPStackForRecvMsgRunThread = NULL;

        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_run_thread_start() exit---: SIP Stack Run Thread For Send Init Error \r\n");
        return -1;
    }

    i = sip_run_thread_init(&g_SIPStackForSendMsgRunThread);

    if (i != 0)
    {
        sip_run_thread_free(g_SIPStackForRecvRunThread);
        osip_free(g_SIPStackForRecvRunThread);
        g_SIPStackForRecvRunThread = NULL;

        sip_run_thread_free(g_SIPStackForRecvRegisterRunThread);
        osip_free(g_SIPStackForRecvRegisterRunThread);
        g_SIPStackForRecvRegisterRunThread = NULL;

        sip_run_thread_free(g_SIPStackForRecvMsgRunThread);
        osip_free(g_SIPStackForRecvMsgRunThread);
        g_SIPStackForRecvMsgRunThread = NULL;

        sip_run_thread_free(g_SIPStackForSendRunThread);
        osip_free(g_SIPStackForSendRunThread);
        g_SIPStackForSendRunThread = NULL;

        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_run_thread_start() exit---: SIP Stack Run Thread For Send Msg Init Error \r\n");
        return -1;
    }

    /* 初始化App 线程 */
    i = sip_run_thread_init(&g_SIPAppRunThread);

    if (i != 0)
    {
        /* 释放Stack 线程 */
        sip_run_thread_free(g_SIPStackForRecvRunThread);
        osip_free(g_SIPStackForRecvRunThread);
        g_SIPStackForRecvRunThread = NULL;

        sip_run_thread_free(g_SIPStackForRecvRegisterRunThread);
        osip_free(g_SIPStackForRecvRegisterRunThread);
        g_SIPStackForRecvRegisterRunThread = NULL;

        sip_run_thread_free(g_SIPStackForRecvMsgRunThread);
        osip_free(g_SIPStackForRecvMsgRunThread);
        g_SIPStackForRecvMsgRunThread = NULL;

        sip_run_thread_free(g_SIPStackForSendRunThread);
        osip_free(g_SIPStackForSendRunThread);
        g_SIPStackForSendRunThread = NULL;

        sip_run_thread_free(g_SIPStackForSendMsgRunThread);
        osip_free(g_SIPStackForSendMsgRunThread);
        g_SIPStackForSendMsgRunThread = NULL;

        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_run_thread_start() exit---: SIP App Run Thread Init Error \r\n");
        return -1;
    }

    /* 创建Stack 线程 */
    g_SIPStackForRecvRunThread->thread = (osip_thread_t*)osip_thread_create(20000, sip_stack_for_recv_run_thread_execute, (void*)g_SIPStackForRecvRunThread);

    if (g_SIPStackForRecvRunThread->thread == NULL)
    {
        /* 释放Stack 线程 */
        sip_run_thread_free(g_SIPStackForRecvRunThread);
        osip_free(g_SIPStackForRecvRunThread);
        g_SIPStackForRecvRunThread = NULL;

        sip_run_thread_free(g_SIPStackForRecvRegisterRunThread);
        osip_free(g_SIPStackForRecvRegisterRunThread);
        g_SIPStackForRecvRegisterRunThread = NULL;

        sip_run_thread_free(g_SIPStackForRecvMsgRunThread);
        osip_free(g_SIPStackForRecvMsgRunThread);
        g_SIPStackForRecvMsgRunThread = NULL;

        sip_run_thread_free(g_SIPStackForSendRunThread);
        osip_free(g_SIPStackForSendRunThread);
        g_SIPStackForSendRunThread = NULL;

        sip_run_thread_free(g_SIPStackForSendMsgRunThread);
        osip_free(g_SIPStackForSendMsgRunThread);
        g_SIPStackForSendMsgRunThread = NULL;

        /* 释放App 线程 */
        sip_run_thread_free(g_SIPAppRunThread);
        osip_free(g_SIPAppRunThread);
        g_SIPAppRunThread = NULL;
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_run_thread_start() exit---: SIP Run Thread Create Error \r\n");
        return -1;
    }

    g_SIPStackForRecvRegisterRunThread->thread = (osip_thread_t*)osip_thread_create(20000, sip_stack_for_recv_register_run_thread_execute, (void*)g_SIPStackForRecvRegisterRunThread);

    if (g_SIPStackForRecvRegisterRunThread->thread == NULL)
    {
        /* 停止Stack线程 */
        g_SIPStackForRecvRunThread->th_exit = 1;

        if (g_SIPStackForRecvRunThread->thread != NULL)
        {
            i = osip_thread_join((struct osip_thread*)g_SIPStackForRecvRunThread->thread);
        }

        /* 释放Stack 线程 */
        sip_run_thread_free(g_SIPStackForRecvRunThread);
        osip_free(g_SIPStackForRecvRunThread);
        g_SIPStackForRecvRunThread = NULL;

        sip_run_thread_free(g_SIPStackForRecvRegisterRunThread);
        osip_free(g_SIPStackForRecvRegisterRunThread);
        g_SIPStackForRecvRegisterRunThread = NULL;

        sip_run_thread_free(g_SIPStackForRecvMsgRunThread);
        osip_free(g_SIPStackForRecvMsgRunThread);
        g_SIPStackForRecvMsgRunThread = NULL;

        sip_run_thread_free(g_SIPStackForSendRunThread);
        osip_free(g_SIPStackForSendRunThread);
        g_SIPStackForSendRunThread = NULL;

        sip_run_thread_free(g_SIPStackForSendMsgRunThread);
        osip_free(g_SIPStackForSendMsgRunThread);
        g_SIPStackForSendMsgRunThread = NULL;

        /* 释放App 线程 */
        sip_run_thread_free(g_SIPAppRunThread);
        osip_free(g_SIPAppRunThread);
        g_SIPAppRunThread = NULL;
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_run_thread_start() exit---: SIP Run Thread Create Error \r\n");
        return -1;
    }

    g_SIPStackForRecvMsgRunThread->thread = (osip_thread_t*)osip_thread_create(20000, sip_stack_for_recv_msg_run_thread_execute, (void*)g_SIPStackForRecvMsgRunThread);

    if (g_SIPStackForRecvMsgRunThread->thread == NULL)
    {
        /* 停止Stack线程 */
        g_SIPStackForRecvRunThread->th_exit = 1;

        if (g_SIPStackForRecvRunThread->thread != NULL)
        {
            i = osip_thread_join((struct osip_thread*)g_SIPStackForRecvRunThread->thread);
        }

        g_SIPStackForRecvRegisterRunThread->th_exit = 1;

        if (g_SIPStackForRecvRegisterRunThread->thread != NULL)
        {
            i = osip_thread_join((struct osip_thread*)g_SIPStackForRecvRegisterRunThread->thread);
        }

        /* 释放Stack 线程 */
        sip_run_thread_free(g_SIPStackForRecvRunThread);
        osip_free(g_SIPStackForRecvRunThread);
        g_SIPStackForRecvRunThread = NULL;

        sip_run_thread_free(g_SIPStackForRecvRegisterRunThread);
        osip_free(g_SIPStackForRecvRegisterRunThread);
        g_SIPStackForRecvRegisterRunThread = NULL;

        sip_run_thread_free(g_SIPStackForRecvMsgRunThread);
        osip_free(g_SIPStackForRecvMsgRunThread);
        g_SIPStackForRecvMsgRunThread = NULL;

        sip_run_thread_free(g_SIPStackForSendRunThread);
        osip_free(g_SIPStackForSendRunThread);
        g_SIPStackForSendRunThread = NULL;

        sip_run_thread_free(g_SIPStackForSendMsgRunThread);
        osip_free(g_SIPStackForSendMsgRunThread);
        g_SIPStackForSendMsgRunThread = NULL;

        /* 释放App 线程 */
        sip_run_thread_free(g_SIPAppRunThread);
        osip_free(g_SIPAppRunThread);
        g_SIPAppRunThread = NULL;
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_run_thread_start() exit---: SIP Run Thread Create Error \r\n");
        return -1;
    }

    g_SIPStackForSendRunThread->thread = (osip_thread_t*)osip_thread_create(20000, sip_stack_for_send_run_thread_execute, (void*)g_SIPStackForSendRunThread);

    if (g_SIPStackForSendRunThread->thread == NULL)
    {
        /* 停止Stack线程 */
        g_SIPStackForRecvRunThread->th_exit = 1;

        if (g_SIPStackForRecvRunThread->thread != NULL)
        {
            i = osip_thread_join((struct osip_thread*)g_SIPStackForRecvRunThread->thread);
        }

        g_SIPStackForRecvRegisterRunThread->th_exit = 1;

        if (g_SIPStackForRecvRegisterRunThread->thread != NULL)
        {
            i = osip_thread_join((struct osip_thread*)g_SIPStackForRecvRegisterRunThread->thread);
        }

        g_SIPStackForRecvMsgRunThread->th_exit = 1;

        if (g_SIPStackForRecvMsgRunThread->thread != NULL)
        {
            i = osip_thread_join((struct osip_thread*)g_SIPStackForRecvMsgRunThread->thread);
        }

        /* 释放Stack 线程 */
        sip_run_thread_free(g_SIPStackForRecvRunThread);
        osip_free(g_SIPStackForRecvRunThread);
        g_SIPStackForRecvRunThread = NULL;

        sip_run_thread_free(g_SIPStackForRecvRegisterRunThread);
        osip_free(g_SIPStackForRecvRegisterRunThread);
        g_SIPStackForRecvRegisterRunThread = NULL;

        sip_run_thread_free(g_SIPStackForRecvMsgRunThread);
        osip_free(g_SIPStackForRecvMsgRunThread);
        g_SIPStackForRecvMsgRunThread = NULL;

        sip_run_thread_free(g_SIPStackForSendRunThread);
        osip_free(g_SIPStackForSendRunThread);
        g_SIPStackForSendRunThread = NULL;

        sip_run_thread_free(g_SIPStackForSendMsgRunThread);
        osip_free(g_SIPStackForSendMsgRunThread);
        g_SIPStackForSendMsgRunThread = NULL;

        /* 释放App 线程 */
        sip_run_thread_free(g_SIPAppRunThread);
        osip_free(g_SIPAppRunThread);
        g_SIPAppRunThread = NULL;
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_run_thread_start() exit---: SIP Run Thread Create Error \r\n");
        return -1;
    }

    g_SIPStackForSendMsgRunThread->thread = (osip_thread_t*)osip_thread_create(20000, sip_stack_for_send_msg_run_thread_execute, (void*)g_SIPStackForSendMsgRunThread);

    if (g_SIPStackForSendMsgRunThread->thread == NULL)
    {
        /* 停止Stack线程 */
        g_SIPStackForRecvRunThread->th_exit = 1;

        if (g_SIPStackForRecvRunThread->thread != NULL)
        {
            i = osip_thread_join((struct osip_thread*)g_SIPStackForRecvRunThread->thread);
        }

        g_SIPStackForRecvRegisterRunThread->th_exit = 1;

        if (g_SIPStackForRecvRegisterRunThread->thread != NULL)
        {
            i = osip_thread_join((struct osip_thread*)g_SIPStackForRecvRegisterRunThread->thread);
        }

        g_SIPStackForRecvMsgRunThread->th_exit = 1;

        if (g_SIPStackForRecvMsgRunThread->thread != NULL)
        {
            i = osip_thread_join((struct osip_thread*)g_SIPStackForRecvMsgRunThread->thread);
        }

        g_SIPStackForSendRunThread->th_exit = 1;

        if (g_SIPStackForSendRunThread->thread != NULL)
        {
            i = osip_thread_join((struct osip_thread*)g_SIPStackForSendRunThread->thread);
        }

        /* 释放Stack 线程 */
        sip_run_thread_free(g_SIPStackForRecvRunThread);
        osip_free(g_SIPStackForRecvRunThread);
        g_SIPStackForRecvRunThread = NULL;

        sip_run_thread_free(g_SIPStackForRecvRegisterRunThread);
        osip_free(g_SIPStackForRecvRegisterRunThread);
        g_SIPStackForRecvRegisterRunThread = NULL;

        sip_run_thread_free(g_SIPStackForRecvMsgRunThread);
        osip_free(g_SIPStackForRecvMsgRunThread);
        g_SIPStackForRecvMsgRunThread = NULL;

        sip_run_thread_free(g_SIPStackForSendRunThread);
        osip_free(g_SIPStackForSendRunThread);
        g_SIPStackForSendRunThread = NULL;

        sip_run_thread_free(g_SIPStackForSendMsgRunThread);
        osip_free(g_SIPStackForSendMsgRunThread);
        g_SIPStackForSendMsgRunThread = NULL;

        /* 释放App 线程 */
        sip_run_thread_free(g_SIPAppRunThread);
        osip_free(g_SIPAppRunThread);
        g_SIPAppRunThread = NULL;
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_run_thread_start() exit---: SIP Run Thread Create Error \r\n");
        return -1;
    }

    /* 创建App 线程 */
    g_SIPAppRunThread->thread = (osip_thread_t*)osip_thread_create(20000, sip_app_run_thread_execute, (void*)g_SIPAppRunThread);

    if (g_SIPAppRunThread->thread == NULL)
    {
        /* 停止Stack线程 */
        g_SIPStackForRecvRunThread->th_exit = 1;

        if (g_SIPStackForRecvRunThread->thread != NULL)
        {
            i = osip_thread_join((struct osip_thread*)g_SIPStackForRecvRunThread->thread);
        }

        g_SIPStackForRecvMsgRunThread->th_exit = 1;

        if (g_SIPStackForRecvMsgRunThread->thread != NULL)
        {
            i = osip_thread_join((struct osip_thread*)g_SIPStackForRecvMsgRunThread->thread);
        }

        g_SIPStackForSendRunThread->th_exit = 1;

        if (g_SIPStackForSendRunThread->thread != NULL)
        {
            i = osip_thread_join((struct osip_thread*)g_SIPStackForSendRunThread->thread);
        }

        g_SIPStackForSendRunThread->th_exit = 1;

        if (g_SIPStackForSendRunThread->thread != NULL)
        {
            i = osip_thread_join((struct osip_thread*)g_SIPStackForSendRunThread->thread);
        }

        g_SIPStackForSendMsgRunThread->th_exit = 1;

        if (g_SIPStackForSendMsgRunThread->thread != NULL)
        {
            i = osip_thread_join((struct osip_thread*)g_SIPStackForSendMsgRunThread->thread);
        }

        /* 释放Stack 线程 */
        sip_run_thread_free(g_SIPStackForRecvRunThread);
        osip_free(g_SIPStackForRecvRunThread);
        g_SIPStackForRecvRunThread = NULL;

        sip_run_thread_free(g_SIPStackForRecvRegisterRunThread);
        osip_free(g_SIPStackForRecvRegisterRunThread);
        g_SIPStackForRecvRegisterRunThread = NULL;

        sip_run_thread_free(g_SIPStackForRecvMsgRunThread);
        osip_free(g_SIPStackForRecvMsgRunThread);
        g_SIPStackForRecvMsgRunThread = NULL;

        sip_run_thread_free(g_SIPStackForSendRunThread);
        osip_free(g_SIPStackForSendRunThread);
        g_SIPStackForSendRunThread = NULL;

        sip_run_thread_free(g_SIPStackForSendMsgRunThread);
        osip_free(g_SIPStackForSendMsgRunThread);
        g_SIPStackForSendMsgRunThread = NULL;

        /* 释放App 线程 */
        sip_run_thread_free(g_SIPAppRunThread);
        osip_free(g_SIPAppRunThread);
        g_SIPAppRunThread = NULL;
        SIP_DEBUG_TRACE(LOG_DEBUG, "sip_run_thread_start() exit---: SIP Run Thread Create Error \r\n");
        return -1;
    }

    return 0;
}

/*****************************************************************************
 Prototype    : sip_run_thread_stop
 Description  : 停止SIP事务处理主线程
 Input        : None
 Output       : None
 Return Value :
 Calls        :
 Called By    :

  History        :
  1.Date         : 2013/3/27
    Author       : yanghaifeng
    Modification : Created function

*****************************************************************************/
void  sip_run_thread_stop()
{
    int i = 0;

    if (g_SIPStackForRecvRunThread != NULL)
    {
        g_SIPStackForRecvRunThread->th_exit = 1;

        if (g_SIPStackForRecvRunThread->thread != NULL)
        {
            i = osip_thread_join((struct osip_thread*)g_SIPStackForRecvRunThread->thread);
        }

        sip_run_thread_free(g_SIPStackForRecvRunThread);
        osip_free(g_SIPStackForRecvRunThread);
        g_SIPStackForRecvRunThread = NULL;
    }

    if (g_SIPStackForRecvMsgRunThread != NULL)
    {
        g_SIPStackForRecvMsgRunThread->th_exit = 1;

        if (g_SIPStackForRecvMsgRunThread->thread != NULL)
        {
            i = osip_thread_join((struct osip_thread*)g_SIPStackForRecvMsgRunThread->thread);
        }

        sip_run_thread_free(g_SIPStackForRecvMsgRunThread);
        osip_free(g_SIPStackForRecvMsgRunThread);
        g_SIPStackForRecvMsgRunThread = NULL;
    }

    if (g_SIPStackForSendRunThread != NULL)
    {
        g_SIPStackForSendRunThread->th_exit = 1;

        if (g_SIPStackForSendRunThread->thread != NULL)
        {
            i = osip_thread_join((struct osip_thread*)g_SIPStackForSendRunThread->thread);
        }

        sip_run_thread_free(g_SIPStackForSendRunThread);
        osip_free(g_SIPStackForSendRunThread);
        g_SIPStackForSendRunThread = NULL;
    }

    if (g_SIPStackForSendMsgRunThread != NULL)
    {
        g_SIPStackForSendMsgRunThread->th_exit = 1;

        if (g_SIPStackForSendMsgRunThread->thread != NULL)
        {
            i = osip_thread_join((struct osip_thread*)g_SIPStackForSendMsgRunThread->thread);
        }

        sip_run_thread_free(g_SIPStackForSendMsgRunThread);
        osip_free(g_SIPStackForSendMsgRunThread);
        g_SIPStackForSendMsgRunThread = NULL;
    }

    if (g_SIPAppRunThread != NULL)
    {
        g_SIPAppRunThread->th_exit = 1;

        if (g_SIPAppRunThread->thread != NULL)
        {
            i = osip_thread_join((struct osip_thread*)g_SIPAppRunThread->thread);
        }

        sip_run_thread_free(g_SIPAppRunThread);
        osip_free(g_SIPAppRunThread);
        g_SIPAppRunThread = NULL;
    }

    return;
}
#endif

/*****************************************************************************
 Prototype    : send_message_using_udp
 Description  : 利用UDP发送sip消息
 Input        : transaction_t * trans
                sip_t * sip
                char * host
                int port
                int out_socket
 Output       : None
 Return Value :
 Calls        :
 Called By    :

  History        :
  1.Date         : 2013/3/26
    Author       : yanghaifeng
    Modification : Created function

*****************************************************************************/
int send_message_using_udp(osip_transaction_t* trans, osip_message_t* sip , char* host , int port, int out_socket)
{
    char* message = NULL;
    size_t length = 0;
    struct sockaddr_in addr;

    if (NULL == sip)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "send_message_using_udp() exit---: Param Error \r\n");
        return -1;
    }

    if (osip_message_to_str(sip, &message, &length) != 0)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "send_message_using_udp() exit---: Message 2 Char Error \r\n");
        return -1;
    }

    if (host == NULL || host[0] == '\0')
    {
        host = sip->req_uri->host;

        if (host == NULL || host[0] == '\0')
        {
            SIP_DEBUG_TRACE(LOG_DEBUG, "send_message_using_udp() exit---: Dest Host Error \r\n");
            return -1;
        }

        if (sip->req_uri->port != NULL)
        {
            port = osip_atoi(sip->req_uri->port);
        }
        else
        {
            port = 5060;
        }
    }

    //addr.sin_addr.s_addr = inet_addr(host);
    addr.sin_addr.s_addr = htonl(get_ipaddr(host));
    addr.sin_port = htons(port);
    addr.sin_family = AF_INET;

    if (out_socket >= 0)
    {
#ifdef WIN32  //modified by chenyu 130523

        if (0 > sendto(out_socket, (const char*) message, strlen(message),
                       0 /*flag */, (struct sockaddr*) &addr, sizeof(addr)))
#else
        if (0 > sendto(out_socket, (const void*) message, strlen(message),
                       0 /*flag */, (struct sockaddr*) &addr, sizeof(addr)))
#endif
        {
            //SIP_DEBUG_TRACE(LOG_ERROR, "\r\n +---------------------------------------------\r\n Error: Send to IP=%s, port=%d \r\n %s \r\n", host, port, message);
            SIPMessageTrace(1, 0, host, port, message);

            osip_free(message);
            message = NULL;
            return -1;
        }

        /*
           if (NULL != sip->from && NULL != sip->from->url && NULL != sip->to && NULL != sip->to->url)
           {
               if ((0 == sstrcmp(sip->from->url->username, (char*)"wiscomCallerID"))
                       && (0 == sstrcmp(sip->to->url->username, (char*)"wiscomCalleeID"))) //查询服务器ID和用户ID
               {
                   printf("\r\n ********** send_message_using_udp() sip_response_default 200:From Host=%s \r\n", sip->to->url->host);
                   printf_system_time1();
               }
           }
           */

        //SIP_DEBUG_TRACE(LOG_INFO, "\r\n +---------------------------------------------\r\n Send to: IP=%s, port=%d \r\n %s \r\n", host, port, message);
        SIPMessageTrace(0, 0, host, port, message);
    }

    osip_free(message);
    message = NULL;
    return 0;
}

/*****************************************************************************
 函 数 名  : send_message_using_tcp
 功能描述  : 利用TCP发送sip消息
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
int send_message_using_tcp(osip_message_t* sip, char* host, int port, int out_socket)
{
    char* message = NULL;
    size_t length = 0;
    char recvmsg[1024] = {0};
    int recvmsg_len = 0;

    if (NULL == sip)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "send_message_using_tcp() exit---: Param Error \r\n");
        return -1;
    }

    if (osip_message_to_str(sip, &message, &length) != 0)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "send_message_using_tcp() exit---: Message 2 Char Error \r\n");
        return -1;
    }

    if (out_socket >= 0)
    {
        /* 发送数据消息体 */
        if (send(out_socket, (char*)message, strlen(message), 0) < 0)
        {
            SIPMessageTrace(1, 0, host, port, message);

            osip_free(message);
            message = NULL;
            return -1;
        }

        /* 接收响应数据 */
        while (1)
        {
            memset(recvmsg, 0, 1024);
            recvmsg_len = recv(out_socket, recvmsg, 1024, 0);

            if (recvmsg_len > 0) /* 接收到数据大小 */
            {
                //printf("收到远端服务器返回的报文数据recvmsg_len=%d, recvmsg=\r\n%s \r\n", recvmsg_len, recvmsg);
                break;
            }
            else if (recvmsg_len == 0) /* 连接关闭 */
            {
                printf("读取远端服务器返回数据报文时发现远端关闭，关闭连接！errno=%d \n", errno);
                return -2;
            }
            else if (recvmsg_len < 0) /* 出错 */
            {
                if (errno != EINTR && errno != EWOULDBLOCK && errno != EAGAIN)
                {
                    printf("读取远端服务器返回数据报文时发现远连接出错，主动关闭连接！errno=%d \n", errno);
                    return -2;
                }
                else if (errno == EAGAIN)
                {
                    printf("10秒没有读取到远端服务器返回数据报文,超时发送下一条报文！errno=%d \n", errno);
                    break;
                }
            }
        }

        SIPMessageTrace(0, 0, host, port, message);
    }

    osip_free(message);
    message = NULL;
    return 0;
}

#if DECS("对外接口")
/*****************************************************************************
 Prototype    : SIP_UASStartUdpReceive
 Description  : 服务器端在特定的端口上启动SIP接收线程
 Input        :  int local_port
 Output       : None
 Return Value :int
 Calls        :
 Called By    :

  History        :
  1.Date         : 2013/3/26
    Author       : yanghaifeng
    Modification : Created function

*****************************************************************************/
int SIP_UASStartUdpReceive(int local_port)
{
    return udp_transport_thread_start_by_port(local_port);
}

/*****************************************************************************
 Prototype    : SIP_UACStartUdpReceive
 Description  : 客户端启动SIP接收线程
 Input        :  int* local_port
 Output       : None
 Return Value :int
 Calls        :
 Called By    :

  History        :
  1.Date         : 2013/3/26
    Author       : yanghaifeng
    Modification : Created function

*****************************************************************************/
int SIP_UACStartUdpReceive(int* local_port)
{
    return udp_transport_thread_start(local_port);
}

/*****************************************************************************
 Prototype    : SIP_StopUdpReceive
 Description  : 服务器端停止某个特定的SIP接收线程
 Input        : int local_port
 Output       : None
 Return Value :
 Calls        :
 Called By    :

  History        :
  1.Date         : 2013/3/26
    Author       : yanghaifeng
    Modification : Created function

*****************************************************************************/
int SIP_StopUdpReceive(int local_port)
{
    return udp_transport_thread_stop(local_port);
}

/*****************************************************************************
 函 数 名  : SIP_StopAllUdpReceive
 功能描述  : 停止所有的SIP接收线程
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月9日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void SIP_StopAllUdpReceive()
{
    int pos = 0;
    udp_tl_t* sipudp = NULL;

    if ((NULL == g_SIPUdpThreadList) || (NULL == g_SIPUdpThreadList->udp_tl_list))
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "SIP_StopAllUdpReceive() exit---: Param Error \r\n");
        return;
    }

    for (pos = 0; pos < osip_list_size(g_SIPUdpThreadList->udp_tl_list); pos++)
    {
        sipudp = (udp_tl_t*)osip_list_get(g_SIPUdpThreadList->udp_tl_list, pos);

        if (sipudp == NULL)
        {
            continue;
        }

        osip_list_remove(g_SIPUdpThreadList->udp_tl_list, pos);
        udp_tl_free(sipudp);
        sipudp = NULL;
    }

    return;
}
#endif

/*****************************************************************************
 函 数 名  : SIP_ShowSIPStackTransactionSize
 功能描述  : 显示服务器段的协议栈处理消息队列
 输入参数  : int sock
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年9月23日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
void SIP_ShowSIPStackTransactionSize(int sock)
{
    char strLine[] = "\r---------------------------------\r\n";

    char rbuf[128] = {0};

    if (sock > 0)
    {
        send(sock, strLine, strlen(strLine), 0);
    }

    snprintf(rbuf, 128, "\rRecv ict transactions size=%d\r\n", osip_list_size(&g_recv_cell->osip_ict_transactions));

    if (sock > 0)
    {
        send(sock, rbuf, strlen(rbuf), 0);
    }

    snprintf(rbuf, 128, "\rRecv ist transactions size=%d\r\n", osip_list_size(&g_recv_cell->osip_ist_transactions));

    if (sock > 0)
    {
        send(sock, rbuf, strlen(rbuf), 0);
    }

    snprintf(rbuf, 128, "\rRecv nict transactions size=%d\r\n", osip_list_size(&g_recv_cell->osip_nict_transactions));

    if (sock > 0)
    {
        send(sock, rbuf, strlen(rbuf), 0);
    }

    snprintf(rbuf, 128, "\rRecv nist transactions size=%d\r\n", osip_list_size(&g_recv_cell->osip_nist_transactions));

    if (sock > 0)
    {
        send(sock, rbuf, strlen(rbuf), 0);
    }

    if (sock > 0)
    {
        send(sock, strLine, strlen(strLine), 0);
    }

    snprintf(rbuf, 128, "\rMessage recv ict transactions size=%d\r\n", osip_list_size(&g_recv_message_cell->osip_ict_transactions));

    if (sock > 0)
    {
        send(sock, rbuf, strlen(rbuf), 0);
    }

    snprintf(rbuf, 128, "\rMessage recv ist transactions size=%d\r\n", osip_list_size(&g_recv_message_cell->osip_ist_transactions));

    if (sock > 0)
    {
        send(sock, rbuf, strlen(rbuf), 0);
    }

    snprintf(rbuf, 128, "\rMessage recv nict transactions size=%d\r\n", osip_list_size(&g_recv_message_cell->osip_nict_transactions));

    if (sock > 0)
    {
        send(sock, rbuf, strlen(rbuf), 0);
    }

    snprintf(rbuf, 128, "\rMessage recv nist transactions size=%d\r\n", osip_list_size(&g_recv_message_cell->osip_nist_transactions));

    if (sock > 0)
    {
        send(sock, rbuf, strlen(rbuf), 0);
    }

    if (sock > 0)
    {
        send(sock, strLine, strlen(strLine), 0);
    }

    snprintf(rbuf, 128, "\rMessage send ict transactions size=%d\r\n", osip_list_size(&g_send_message_cell->osip_ict_transactions));

    if (sock > 0)
    {
        send(sock, rbuf, strlen(rbuf), 0);
    }

    snprintf(rbuf, 128, "\rMessage send ist transactions size=%d\r\n", osip_list_size(&g_send_message_cell->osip_ist_transactions));

    if (sock > 0)
    {
        send(sock, rbuf, strlen(rbuf), 0);
    }

    snprintf(rbuf, 128, "\rMessage send nict transactions size=%d\r\n", osip_list_size(&g_send_message_cell->osip_nict_transactions));

    if (sock > 0)
    {
        send(sock, rbuf, strlen(rbuf), 0);
    }

    snprintf(rbuf, 128, "\rMessage send nist transactions size=%d\r\n", osip_list_size(&g_send_message_cell->osip_nist_transactions));

    if (sock > 0)
    {
        send(sock, rbuf, strlen(rbuf), 0);
    }

    if (sock > 0)
    {
        send(sock, strLine, strlen(strLine), 0);
    }

    UAS_SMUTEX_UNLOCK();

    return;
}
