/******************************************************************************

                  版权所有 (C), 2001-2013, 金智视讯技术有限公司

 ******************************************************************************
  文 件 名   : udp_tl.h
  版 本 号   : 初稿
  作    者   : yanghaifeng
  生成日期   : 2013年4月1日
  最近修改   :
  功能描述   : udp_tl.c 的头文件
  函数列表   :
  修改历史   :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 创建文件

******************************************************************************/
#ifndef __UDP_TL_H__
#define __UDP_TL_H__

//modified by chenyu 130522
#ifdef WIN32

#ifdef EV9000SIP_EXPORTS  
#define EV9000SIP_API    __declspec(dllexport) 
#else  
#define EV9000SIP_API    __declspec(dllimport) 
#endif

#else
#define EV9000SIP_API    extern
#endif //end WIN32

/*----------------------------------------------*
 * 包含头文件                                   *
 *----------------------------------------------*/
#include <sys/types.h>
#ifdef WIN32
#include <winsock2.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#endif


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

    /*----------------------------------------------*
     * 外部变量说明                                 *
     *----------------------------------------------*/

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

    /* sip运行主线程结构体 */
    typedef struct _run_thread_t
    {
        osip_thread_t* thread;
        int         th_exit;
    } run_thread_t;

    /* 接收sip消息的udp线程结构体*/
    typedef struct _udp_tl_t
    {
        int in_port;
        int in_socket;
        int out_port;
        int out_socket;
        osip_thread_t* thread;
        int th_exit;
    } udp_tl_t;

    /* 接收sip消息的udp线程队列 */
    typedef struct udp_tl_list_t
    {
        osip_list_t*    udp_tl_list;
    } udp_tl_list_t;

    extern int send_message_using_udp(osip_transaction_t* trans, osip_message_t* sip , char* host , int port , int out_socket);
    extern int send_message_using_tcp(osip_message_t* sip, char* host, int port, int out_socket);
    EV9000SIP_API int SIP_UASStartUdpReceive(int local_port);
    EV9000SIP_API int SIP_UACStartUdpReceive(int* local_port);
    extern void* sip_stack_run_thread_execute(void* p);
    extern void* sip_app_run_thread_execute(void* p);
    extern void sip_run_thread_free(run_thread_t* run);
    extern int sip_run_thread_init(run_thread_t** run);
    extern int sip_run_thread_start();
    extern void  sip_run_thread_stop();
    EV9000SIP_API int  SIP_StopUdpReceive(int local_port);
    extern void SIP_StopAllUdpReceive();
    extern void udp_list_free();
    extern int udp_list_init();
    extern int udp_tl_execute(udp_tl_t* udp, osip_t* recv_cell, osip_t* recv_msg_cell, osip_t* send_msg_cell, int sec_max, int usec_max, int max_analysed);
    extern void udp_tl_free(udp_tl_t* udp);
    extern int udp_tl_init(udp_tl_t** udp, int in_port, int out_port);
    extern int udp_tl_start(udp_tl_t* udp);
    extern int udp_tl_stop(udp_tl_t* udp);
    extern void* udp_tl_thread(void* ptr);
    extern int udp_transport_thread_start(int* local_port);
    extern int udp_transport_thread_start_by_port(int local_port);
    extern int udp_transport_thread_stop(int local_port);
    extern int get_socket_by_port(int port);
    EV9000SIP_API void SIP_ShowSIPStackTransactionSize(int sock);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif /* __UDP_TL_H__ */
