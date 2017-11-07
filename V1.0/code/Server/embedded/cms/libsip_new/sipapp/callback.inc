/******************************************************************************

                  版权所有 (C), 2001-2013, 金智视讯技术有限公司

 ******************************************************************************
  文 件 名   : callback.h
  版 本 号   : 初稿
  作    者   : yanghaifeng
  生成日期   : 2013年4月1日
  最近修改   :
  功能描述   : callback.c 的头文件
  函数列表   :
  修改历史   :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 创建文件

******************************************************************************/
#ifndef __CALLBACK_H__
#define __CALLBACK_H__

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
    /* 上层应用设置的回调函数 */
    typedef struct _app_callback_t
    {
        /* 服务器端收到注册消息回调函数
             参数:proxy_id, 服务器ID
                  register_id, 注册的id
                  from_ip, 来源IP
                  from_port, 来源port
                  user_name 注册用户名
                  reg_info_index,注册句柄索引
                  expires,时间
                  link_type,联网类型
           */
        int (*uas_register_received_cb)(char*, char*, char*, int, char*, int, int, int);

        /* 服务端没有收到客户端刷新注册超时
             参数:proxy_id, 服务器ID
                  register_id, 注册的id

                  from_ip, 来源IP
                  from_port, 来源port
                  reg_info_index,注册句柄索引
          */
        int (*uas_register_received_timeout_cb)(char*, char*, char*, int, int);

        /* 客户端发送注册消息后收到注册响应消息回调函数
             参数:reg_info_index,注册句柄索引
                  expires 超时时间
                  status_code,注册响应消息码
                  reasonphrase, 注册响应的具体原因
                  iTime, 服务器返回的校时时间
           */
        int (*uac_register_response_received_cb)(int, int, int, char*, unsigned int, int);
        int uac_register_response_received_cb_user_data;

        /* 收到呼叫消息回调函数
             参数:        caller_id,会话来源id
                          callee_id,会话目的id
                          call_id,会话唯一id
                          dialog_index,会话句柄索引
                          body:携带的body具体内容
                          body_len:携带的body长度
           */
        int (*invite_received_cb)(char*, char*, char*, int, char*, int, int);
        int invite_received_cb_user_data;

        /* 收到呼叫响应消息回调函数
             参数:        caller_id,会话来源id
                          callee_id,会话目的id
                          call_id,会话唯一id
                          dialog_index,会话句柄索引
                          status_code,呼叫响应消息码
                          reasonphrase, 呼叫响应的具体原因
                          body:携带的body具体内容
                          body_len:携带的body长度
        */
        int (*invite_response_received_cb)(char*, char*, char*, int, int, char*, char*, int, int);
        int invite_response_received_cb_user_data;

        /* 收到Cancel 消息回调函数
             参数:caller_id,会话来源id
                          callee_id,会话目的id
                          call_id,会话唯一id
                          dialog_index,会话句柄索引
           */
        int (*cancel_received_cb)(char*, char*, char*, int, int);
        int cancel_received_cb_user_data;

        /* 收到ACK 消息回调函数
             参数:caller_id,会话来源id
                          callee_id,会话目的id
                          call_id,会话唯一id
                          dialog_index,会话句柄索引
           */
        int (*ack_received_cb)(char*, char*, char*, int, int);
        int ack_received_cb_user_data;

        /* 收到呼叫结束消息回调函数
             参数:caller_id,会话来源id
                          callee_id,会话目的id
                          call_id,会话唯一id
                          dialog_index,会话句柄索引
        */
        int (*bye_received_cb)(char*, char*, char*, int, int);
        int bye_received_cb_user_data;

        /* 收到呼叫结束响应消息回调函数
             参数:caller_id,会话来源id
                          callee_id,会话目的id
                          call_id,会话唯一id
                          dialog_index,会话句柄索引
                          status_code,呼叫响应消息码
        */
        int (*bye_response_received_cb)(char*, char*, char*, int, int, int);
        int bye_response_received_cb_user_data;

        /* 收到Message 消息回调函数
             参数:        caller_id,发送方的device id
                          caller_ip 发送方的IP地址
                          caller_port 发送方的端口号
                          callee_id,接收方的device id
                          callee_ip 接收方的IP地址
                          callee_port 接收方的端口号
                          call_id:该message消息唯一的call id
                          dialog_index,会话句柄索引
                          msg:携带的msg具体内容
                          msg_len:携带的msg长度
        */
        int (*message_received_cb)(char*, char*, int, char*, char*, int, char*, int, char*, int, int);
        int message_received_cb_user_data;

        /* 收到Message 响应消息回调函数
             参数:caller_id,发送方的device id
                          callee_id,接收方的device id
                          call_id:该message消息唯一的call id
                          status_code:Message响应消息码
        */
        int (*message_response_received_cb)(char*, char*, char*, int, int);
        int message_response_received_cb_user_data;

        /* 收到Subscribe 消息回调函数
             参数:        caller_id,发送方的device id
                          caller_ip 发送方的IP地址
                          caller_port 发送方的端口号
                          callee_id,接收方的device id
                          callee_ip 接收方的IP地址
                          callee_port 接收方的端口号
                          call_id:该message消息唯一的call id
                          event_type 事件类型
                          id_param 事件类型ID
                          subscribe_expires 超时时间
                          msg:携带的msg具体内容
                          msg_len:携带的msg长度
        */
        int (*subscribe_received_cb)(char*, char*, int, char*, char*, int, char*, char*, char*, int, char*, int, int);
        int subscribe_received_cb_user_data;

        /* 收到Subscribe 响应消息回调函数
             参数:caller_id,发送方的device id
                          callee_id,接收方的device id
                          call_id:该message消息唯一的call id
                          expires:订阅超时时间
                          status_code:Message响应消息码
        */
        int (*subscribe_response_received_cb)(char*, char*, char*, int, int, int);
        int subscribe_response_received_cb_user_data;

        /* 收到Subscribe within dialog 消息回调函数
             参数:        caller_id,会话来源id
                          caller_ip 发送方的IP地址
                          caller_port 发送方的端口号
                          callee_id,会话目的id
                          call_id,会话唯一id
                          dialog_index,会话句柄索引
                          subscribe_expires 超时时间
                          body:携带的body具体内容
                          body_len:携带的body长度
            */
        int (*subscribe_within_dialog_received_cb)(char*, char*, int, char*, char*, int, int, char*, int);

        /* 收到Subscribe 响应消息回调函数
             参数:        caller_id,发送方的device id
                          callee_id,接收方的device id
                          call_id:该message消息唯一的call id
                          dialog_index,会话句柄索引
                          expires:订阅超时时间
                          status_code:Message响应消息码
            */
        int (*subscribe_within_dialog_response_received_cb)(char*, char*, char*, int, int, int);

        /* 收到 Notify 消息回调函数
             参数:        caller_id,发送方的device id
                          caller_ip 发送方的IP地址
                          caller_port 发送方的端口号
                          callee_id,接收方的device id
                          callee_ip 接收方的IP地址
                          callee_port 接收方的端口号
                          call_id:该message消息唯一的call id
                          msg:携带的msg具体内容
                          msg_len:携带的msg长度
        */
        int (*notify_received_cb)(char*, char*, int, char*, char*, int, char*, char*, int, int);
        int notify_received_cb_user_data;

        /* 收到 Notify 响应消息回调函数
             参数:caller_id,发送方的device id
                          callee_id,接收方的device id
                          call_id:该message消息唯一的call id
                          status_code:Message响应消息码
        */
        int (*notify_response_received_cb)(char*, char*, char*, int, int);
        int notify_response_received_cb_user_data;

        /* 收到Info 消息回调函数
             参数:caller_id,发送方的device id
                          caller_ip 发送方的IP地址
                          caller_port 发送方的端口号
                          callee_id,接收方的device id
                          call_id:该Info消息唯一的call id
                          dialog_index,会话句柄索引
                          msg:携带的msg具体内容
                          msg_len:携带的msg长度
        */
        int (*info_received_cb)(char*, char*, int, char*, char*, int, char*, int, int);
        int info_received_cb_user_data;

        /* 收到Info 响应消息回调函数
             参数:caller_id,发送方的device id
                          callee_id,接收方的device id
                          call_id:该Info消息唯一的call id
                          status_code:Info响应消息码
        */
        int (*info_response_received_cb)(char*, char*, char*, int, int);
        int info_response_received_cb_user_data;

        /* UA 会话超时回调函数
             参数:dialog_index,会话句柄索引
           */
        int (*ua_session_expires_cb)(int);

        /* 调试打印函数
             参数:const char*
                          ...
            */
        void (*dbg_printf_cb)(int, const char*, const char*, int, const char*);

        /* SIP消息调试打印函数
             参数  : int type:
                            0,正确的
                            1:发送错误的
                            2:接收解析错误的
                            3:接收消息错误的
                            4:接收创建事务错误的
                            int iDirect:
                            0:发送的
                            1:接收的
                            char* ipaddr
                            int port
                            char* msg
            */
        void (*sip_message_trace_cb)(int, int, char*, int, char*);

    } app_callback_t;

    /*----------------------------------------------*
     * 宏定义                                       *
     *----------------------------------------------*/
    extern void app_callback_free();
    extern int app_callback_init();
    EV9000SIP_API void app_set_ack_received_cb(int (*cb)(char*, char*, char*, int, int), int user_data);
    EV9000SIP_API void app_set_bye_received_cb(int (*cb)(char*, char*, char*, int, int), int user_data);
    EV9000SIP_API void app_set_bye_response_received_cb(int (*cb)(char*, char*, char*, int, int, int), int user_data);
    EV9000SIP_API void app_set_cancel_received_cb(int (*cb)(char*, char*, char*, int, int), int user_data);
    EV9000SIP_API void app_set_dbg_printf_cb(void (*cb)(int, const char*, const char*, int, const char*));
    EV9000SIP_API void app_set_info_received_cb(int (*cb)(char*, char*, int, char*, char*, int, char*, int, int), int user_data);
    EV9000SIP_API void app_set_info_response_received_cb(int (*cb)(char*, char*, char*, int, int), int user_data);
    EV9000SIP_API void app_set_invite_received_cb(int (*cb)(char*, char*, char*, int, char*, int, int), int user_data);
    EV9000SIP_API void app_set_invite_response_received_cb(int (*cb)(char*, char*, char*, int, int, char*, char*, int, int), int user_data);
    EV9000SIP_API void app_set_message_received_cb(int (*cb)(char*, char*, int, char*, char*, int, char*, int, char*, int, int), int user_data);
    EV9000SIP_API void app_set_message_response_received_cb(int (*cb)(char*, char*, char*, int, int), int user_data);
    EV9000SIP_API void app_set_notify_received_cb(int (*cb)(char*, char*, int, char*, char*, int, char*, char*, int, int), int user_data);
    EV9000SIP_API void app_set_notify_response_received_cb(int (*cb)(char*, char*, char*, int, int), int user_data);
    EV9000SIP_API void app_set_sip_message_trace_cb(void (*cb)(int, int, char*, int, char*));
    EV9000SIP_API void app_set_subscribe_received_cb(int (*cb)(char*, char*, int, char*, char*, int, char*, char*, char*, int, char*, int, int), int user_data);
    EV9000SIP_API void app_set_subscribe_response_received_cb(int (*cb)(char*, char*, char*, int, int, int), int user_data);
    EV9000SIP_API void app_set_subscribe_within_dialog_received_cb(int (*cb)(char*, char*, int, char*, char*, int, int, char*, int));
    EV9000SIP_API void app_set_subscribe_within_dialog_response_received_cb(int (*cb)(char*, char*, char*, int, int, int));
    EV9000SIP_API void app_set_uac_register_response_received_cb(int (*cb)(int, int, int, char*, unsigned int, int), int user_data);
    EV9000SIP_API void app_set_uas_register_received_cb(int (*cb)(char*, char*, char*, int, char*, int, int, int));
    EV9000SIP_API void app_set_uas_register_received_timeout_cb(int (*cb)(char*, char*, char*, int, int));
    EV9000SIP_API void app_set_ua_session_expires_cb(int (*cb)(int));
    extern void cs_cb_ict_2xx2(osip_message_t* sip);
    extern void cs_cb_ict_rcv1xx(int type, osip_transaction_t* tr , osip_message_t* sip);
    extern void cs_cb_ict_rcv2xx(int type, osip_transaction_t* tr, osip_message_t* sip);
    extern void cs_cb_ict_rcv3xx(int type, osip_transaction_t* tr, osip_message_t* sip);
    extern void cs_cb_ict_rcv456xx(int type, osip_transaction_t* tr, osip_message_t* sip);
    extern void cs_cb_ict_timeout(int type, osip_transaction_t* tr, osip_message_t* sip);
    extern void cs_cb_ist_ack_for2xx(osip_message_t* sip);
    extern void cs_cb_ist_snd1xx(int type, osip_transaction_t* tr, osip_message_t* sip);
    extern void cs_cb_ist_snd2xx(int type, osip_transaction_t* tr, osip_message_t* sip);
    extern void cs_cb_ist_snd3456xx(int type, osip_transaction_t* tr, osip_message_t* sip);
    extern void cs_cb_kill_ict_transaction_for_rcv(int type, osip_transaction_t* tr);
    extern void cs_cb_kill_ict_transaction_for_rcv_msg(int type, osip_transaction_t* tr);
    extern void cs_cb_kill_ict_transaction_for_rcv_register(int type, osip_transaction_t* tr);
    extern void cs_cb_kill_ict_transaction_for_send(int type, osip_transaction_t* tr);
    extern void cs_cb_kill_ict_transaction_for_send_msg(int type, osip_transaction_t* tr);
    extern void cs_cb_kill_ist_transaction_for_rcv(int type, osip_transaction_t* tr);
    extern void cs_cb_kill_ist_transaction_for_rcv_msg(int type, osip_transaction_t* tr);
    extern void cs_cb_kill_ist_transaction_for_rcv_register(int type, osip_transaction_t* tr);
    extern void cs_cb_kill_ist_transaction_for_send(int type, osip_transaction_t* tr);
    extern void cs_cb_kill_ist_transaction_for_send_msg(int type, osip_transaction_t* tr);
    extern void cs_cb_kill_nict_transaction_for_rcv(int type, osip_transaction_t* tr);
    extern void cs_cb_kill_nict_transaction_for_rcv_msg(int type, osip_transaction_t* tr);
    extern void cs_cb_kill_nict_transaction_for_rcv_register(int type, osip_transaction_t* tr);
    extern void cs_cb_kill_nict_transaction_for_send(int type, osip_transaction_t* tr);
    extern void cs_cb_kill_nict_transaction_for_send_msg(int type, osip_transaction_t* tr);
    extern void cs_cb_kill_nist_transaction_for_rcv(int type, osip_transaction_t* tr);
    extern void cs_cb_kill_nist_transaction_for_rcv_msg(int type, osip_transaction_t* tr);
    extern void cs_cb_kill_nist_transaction_for_rcv_register(int type, osip_transaction_t* tr);
    extern void cs_cb_kill_nist_transaction_for_send(int type, osip_transaction_t* tr);
    extern void cs_cb_kill_nist_transaction_for_send_msg(int type, osip_transaction_t* tr);
    extern void cs_cb_nict_rcv1xx(int type, osip_transaction_t* tr, osip_message_t* sip);
    extern void cs_cb_nict_rcv2xx(int type, osip_transaction_t* tr, osip_message_t* sip);
    extern void cs_cb_nict_rcv3xx(int type, osip_transaction_t* tr, osip_message_t* sip);
    extern void cs_cb_nict_rcv456xx(int type, osip_transaction_t* tr, osip_message_t* sip);
    extern void cs_cb_nict_timeout(int type, osip_transaction_t* tr, osip_message_t* sip);
    extern void cs_cb_nist_snd1xx(int type, osip_transaction_t* tr, osip_message_t* sip);
    extern void cs_cb_nist_snd2xx(int type, osip_transaction_t* tr, osip_message_t* sip);
    extern void cs_cb_nist_snd3456xx(int type, osip_transaction_t* tr, osip_message_t* sip);
    extern void cs_cb_rcvack(int type, osip_transaction_t* tr, osip_message_t* sip);
    extern void cs_cb_rcvack2(int type, osip_transaction_t* tr, osip_message_t* sip);
    extern void cs_cb_rcvbye(int type, osip_transaction_t* tr, osip_message_t* sip);
    extern void cs_cb_rcvcancel(int type, osip_transaction_t* tr, osip_message_t* sip);
    extern void cs_cb_rcvinfo(int type, osip_transaction_t* tr, osip_message_t* sip);
    extern void cs_cb_rcvinvite(int type, osip_transaction_t* tr, osip_message_t* sip);
    extern void cs_cb_rcvmessage(int type, osip_transaction_t* tr, osip_message_t* sip);
    extern void cs_cb_rcvnotify(int type, osip_transaction_t* tr, osip_message_t* sip);
    extern void cs_cb_rcvoptions(int type, osip_transaction_t* tr, osip_message_t* sip);
    extern void cs_cb_rcvrefer(int type, osip_transaction_t* tr, osip_message_t* sip);
    extern void cs_cb_rcvregister(int type, osip_transaction_t* tr, osip_message_t* sip);
    extern void cs_cb_rcvreq_retransmission(int type, osip_transaction_t* tr, osip_message_t* sip);
    extern void cs_cb_rcvresp_retransmission(int type, osip_transaction_t* tr, osip_message_t* sip);
    extern void cs_cb_rcvsubscribe(int type, osip_transaction_t* tr, osip_message_t* sip);
    extern void cs_cb_rcvunkrequest(int type, osip_transaction_t* tr, osip_message_t* sip);
    extern void cs_cb_rcvupdate(int type, osip_transaction_t* tr, osip_message_t* sip);
    extern void cs_cb_sndack(int type, osip_transaction_t* tr, osip_message_t* sip);
    extern void cs_cb_sndbye(int type, osip_transaction_t* tr, osip_message_t* sip);
    extern void cs_cb_sndcancel(int type, osip_transaction_t* tr, osip_message_t* sip);
    extern void cs_cb_sndinfo(int type, osip_transaction_t* tr, osip_message_t* sip);
    extern void cs_cb_sndinvite(int type, osip_transaction_t* tr, osip_message_t* sip);
    extern void cs_cb_sndoptions(int type, osip_transaction_t* tr, osip_message_t* sip);
    extern void cs_cb_sndregister(int type, osip_transaction_t* tr, osip_message_t* sip);
    extern void cs_cb_sndreq_retransmission(int type, osip_transaction_t* tr, osip_message_t* sip);
    extern void cs_cb_sndresp_retransmission(int type, osip_transaction_t* tr, osip_message_t* sip);
    extern void cs_cb_sndunkrequest(int type, osip_transaction_t* tr, osip_message_t* sip);
    extern void cs_cb_transport_error(int type, osip_transaction_t* tr, int error);
    extern int sip_callback_init(osip_t* cell);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif /* __CALLBACK_H__ */
