/******************************************************************************

                  版权所有 (C), 2001-2013, 金智视讯技术有限公司

 ******************************************************************************
  文 件 名   : sipmsg.h
  版 本 号   : 初稿
  作    者   : yanghaifeng
  生成日期   : 2013年4月1日
  最近修改   :
  功能描述   : sipmsg.c 的头文件
  函数列表   :
  修改历史   :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 创建文件

******************************************************************************/
#ifndef __SIPMSG_H__
#define __SIPMSG_H__

/*----------------------------------------------*
 * 包含头文件                                   *
 *----------------------------------------------*/
#include <osip2/internal.h>

#include "gblfunc.inc"
#include "sipua.inc"

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
#define ACK_METHOD        "ACK"
#define BYE_METHOD        "BYE"
#define CANCEL_METHOD     "CANCEL"
#define INFO_METHOD       "INFO"
#define INVITE_METHOD     "INVITE"
#define MESSAGE_METHOD    "MESSAGE"
#define NOTIFY_METHOD     "NOTIFY"
#define OPTIONS_METHOD    "OPTIONS"
#define PRACK_METHOD      "PRACK"
#define PUBLISH_METHOD    "PUBLISH"
#define REFER_METHOD      "REFER"
#define REGISTER_METHOD   "REGISTER"
#define SUBSCRIBE_METHOD  "SUBSCRIBE"
#define MESSAGE_METHOD    "MESSAGE"
#define UPDATE_METHOD     "UPDATE"

#define APPLICATION_SDP   "Application/SDP"

    typedef enum _msg_type_t
    {
        SIP_MESSAGE,
        SIP_INFO,
        MSG_NULL
    }
    msg_type_t;

    typedef struct _sip_message_t
    {
        msg_type_t msg_type;
        int dialog_index;
        char* call_id;
        osip_transaction_t* tr;
        osip_message_t* sip;
    } sip_message_t;

    typedef struct _sip_message_list_t
    {
        osip_list_t*    pSipMessageList;
#ifdef MULTI_THR
        osip_mutex_t* lock;
#endif
    } sip_message_list_t;


#define RESPONSE_IS_FOR(msg,requestname)  ( \
                                            0==sstrcmp((msg)->cseq->method,requestname))

#define REQUEST_TEST(msg, req) \
    (0==strncmp((msg)->sip_method, req, strlen(req)+1))

    extern void init_random_number();
    extern unsigned int new_random_number();
    extern char* new_to_tag();
    extern char* new_from_tag();
    EV9000SIP_API char* new_callid();   //modified by chenyu 131025

    
    extern char* call_id_new_random();
    extern int cancel_match_st(osip_transaction_t* tr, osip_message_t* cancel);
    extern int complete_answer_that_establish_a_dialog(osip_message_t* response, osip_message_t* request, char* callee, char* localip, int localport);
    extern int  cseqnum_match(osip_cseq_t* cseq1, osip_cseq_t* cseq2);
    extern int cs_fix_last_via_header(osip_message_t* msg, char* ip_addr, int port);
    extern int cs_generating_answer(osip_transaction_t* tr, osip_message_t* request, int code, char* reasonphrase);
    extern int cs_generating_response_default(osip_message_t* request, int status, char* reasonphrase, osip_message_t** dest);
    extern void cs_response_add_proxy_authenticate(osip_message_t* resp, char* localhost);
    extern void cs_response_add_www_authenticate(osip_message_t* resp, char* localhost);
    extern int dialog_fill_route_set(sip_dialog_t* dialog, osip_message_t* request);
    extern char* from_tag_new_random();
    extern void generating_1xx_answer_to_options(sip_dialog_t* dialog, osip_transaction_t* tr, int code);
    extern void generating_2xx_answer_to_options(sip_dialog_t* dialog, osip_transaction_t* tr, int code, char* localip);
    extern void generating_3456xx_answer_to_options(sip_dialog_t* dialog, osip_transaction_t* tr, int code);
    extern int generating_ack_for_2xx(osip_message_t** ack, sip_dialog_t* dialog, char* localip, int localport);
    extern int generating_bye(osip_message_t** bye, sip_dialog_t* dialog, char* localip, int localport);
    extern int generating_cancel(osip_message_t** dest, osip_message_t* request_cancelled);
    extern int generating_digest_auth(osip_authorization_t* authorization, char* password, char* method);
    extern int generating_forward_ack_for_2xx(osip_message_t** ack, sip_dialog_t* dialog, char* localip, int localport);
    extern int generating_forward_bye(osip_message_t** bye, sip_dialog_t* dialog, char* localip, int localport);
    extern int generating_forward_invite_within_dialog(osip_message_t** invite, sip_dialog_t* dialog, char* sdp, char* caller, char* localip, int localport);
    extern int generating_forward_request_within_dialog(osip_message_t** dest, char* method_name, sip_dialog_t* dialog, char* localip, int localport);
    extern int generating_info(osip_message_t** info, char* caller, char* callee, char* callid, char* body, int body_len, char* proxyip, int proxyport, char* localip, int localport);
    extern int generating_info_within_dialog(osip_message_t** info, sip_dialog_t* dialog, char* msg, int msg_len, char* localip, int localport);
    extern int generating_invite(osip_message_t** invite, char* caller, char* callee, char* sdp, char* proxyip, int proxyport, char* localip, int localport);
    extern int generating_invite_within_dialog(osip_message_t** invite, sip_dialog_t* dialog, char* sdp, char* caller, char* localip, int localport);
    extern int generating_message(osip_message_t** message, char* caller, char* callee, char* callid, char* msg, int msg_len, char* proxyip, int proxyport, char* localip, int localport);
    extern int generating_message_for_tcp(osip_message_t** message, char* caller, char* callee, char* callid, char* msg, int msg_len, char* proxyip, int proxyport, char* localip, int localport);
    extern int generating_message_within_dialog(osip_message_t** info, sip_dialog_t* dialog, char* msg, int msg_len, char* localip, int localport);
    extern int generating_options(osip_message_t** options, char* caller, char* callee, char* sdp, char* proxyip, int proxyport, char* localip, int localport);
    extern int generating_options_within_dialog(osip_message_t** options, sip_dialog_t* dialog, char* sdp, char* localip, int localport);
    extern int generating_refer_within_dialog(osip_message_t** refer, sip_dialog_t* dialog, char* caller, char* callee, char* proxyip, int proxyport, char* localip, int localport);
    extern int generating_register(osip_message_t** reg, char* caller, char* username, char* proxyid, char* proxyip, int proxyport, char* localip, int localport, int period, char* register_callid_number, int register_cseq_number);
    extern int generating_request_fromrequest(osip_message_t* old_request, osip_message_t** new_request, char* localip, int localport);
    extern int generating_request_out_of_dialog(osip_message_t** dest, char* method_name, char* from, char* to, char* route, char* localip, int localport, char* registrator, char* call_id, int register_cseq_number);
    extern int generating_request_out_of_dialog_for_tcp(osip_message_t** dest, char* method_name, char* from, char* to, char* route, char* localip, int localport, char* registrator, char* call_id, int register_cseq_number);
    extern int generating_request_within_dialog(osip_message_t** dest, char* method_name, sip_dialog_t* dialog, char* localip, int localport);
    extern int generating_response_default(osip_message_t** dest, sip_dialog_t* dialog, int status, osip_message_t* request, char* reasonphrase);
    extern int generating_sdp_answer(sdp_message_t* remote_sdp, sdp_message_t** local_sdp, char* audio_port, char* video_port, char* localip, int audio_code_type, int video_code_type);
    extern int generating_subscribe(osip_message_t** subscribe, char* caller, char* callee, char* callid, char* msg, int msg_len, char* proxyip, int proxyport, char* localip, int localport);
    extern int generating_subscribe_with_dialog(osip_message_t** subscribe, sip_dialog_t* dialog, char* caller, char* localip, int localport);
    extern int generating_notify(osip_message_t** notify, char* caller, char* callee, char* callid, char* msg, int msg_len, char* proxyip, int proxyport, char* localip, int localport);
    extern int generating_notify_for_tcp(osip_message_t** notify, char* caller, char* callee, char* callid, char* msg, int msg_len, char* proxyip, int proxyport, char* localip, int localport);
    extern int generating_notify_within_dialog(osip_message_t** notify, sip_dialog_t* dialog, sip_subscription_t* sip_sub, char* msg, int msg_len, char* callee, char* localip, int localport);
    extern int generating_update_within_dialog(osip_message_t** update, sip_dialog_t* dialog, char* localip, int localport);
    extern int get_contact(char** contact, char* username, char* localip, int localport);
    extern int get_from(char** from, char* username, char* proxyip, int proxyport);
    extern char* get_message_from_host(osip_message_t* sip);
    extern int get_message_from_port(osip_message_t* sip);
    extern int get_message_sdp(osip_message_t* sip, sdp_message_t** sdpr);
    extern char* get_message_to_host(osip_message_t* sip);
    extern int get_message_to_port(osip_message_t* sip);
    extern int get_registrator(char** registrator, char* proxyid, char* proxyip, int proxyport);
    extern int get_sdp_ip_and_port(sdp_message_t* sdp, unsigned long* addr, int* port);
    extern int get_sdp_videoinfo(sdp_message_t* sdp, unsigned long* addr, int* port, int* codetype, int* flag);
    extern int get_to(char** to, char* username, char* proxyip, int proxyport);
    extern char* nonce_new_random();
    extern int request_add_authorization(osip_message_t* request, osip_www_authenticate_t* auth, char* user, char* password, char* proxy, int proxy_port);
    extern int request_add_proxy_authorization(osip_message_t* request, osip_proxy_authenticate_t* auth, char* user, char* password);
    extern int request_get_destination(osip_message_t* request, char** address, int* portnum);
    extern int response_get_destination(osip_message_t* response, char** address, int* portnum);
    EV9000SIP_API int SIP_GetSDPVideoInfo(sdp_message_t* sdp, unsigned long* addr, int* port, int* codetype, int* flag);
    extern int SIP_GetSDPVideoInfoEx(sdp_message_t* sdp, char* video_addr, int* video_port, int* codetype, int* media_direction, int* stream_type, int* record_type, int* trans_type, int* file_size, int* downloadspeed);
    extern int SIP_GetSDPAudioInfo(sdp_message_t* sdp, char* audio_addr, int *audio_port, int *codetype, int* media_direction, int* stream_type, int* trans_type);
    extern int sip_message_add(msg_type_t msg_type, char* call_id, int dialog_index, osip_transaction_t* tr, osip_message_t* sip);
    extern int sip_message_find(char* call_id);
    extern int sip_message_find2(char* call_id);
    extern void sip_message_free(sip_message_t* sip_message);
    extern sip_message_t* sip_message_get(int pos);
    extern int sip_message_init(sip_message_t** sip_message);
    extern void sip_message_list_free();
    extern int sip_message_list_init();
    extern int sip_message_list_lock();
    extern int sip_message_list_unlock();
    extern int sip_message_remove(int pos);
    extern int sip_message_remove2(int pos);
    extern int tl_sendmessage(osip_message_t* sip,  char* host,  int port,  int out_socket);
    extern int tl_sendmessage_by_tcp(osip_message_t* sip, char* host, int port, int out_socket);
    extern char* to_tag_new_random();
    extern int ul_sendmessage(osip_transaction_t* transaction, osip_message_t* msg);
    extern int url_compare(osip_uri_t* url1, osip_uri_t* url2);
    extern int url_match_simply(osip_uri_t* url1, osip_uri_t* url2);
    extern unsigned int via_branch_new_random();
    extern int analyze_f_param(char* f_param, int* f_v_code_type, int* f_v_ratio, int* f_v_frame_speed, int* f_v_code_rate_type, int* f_v_code_rate_size, int* f_a_code_type, int* f_a_code_rate_size, int* f_a_sample_rate);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif /* __SIPMSG_H__ */
