#ifndef __LIBSIP_H__
#define __LIBSIP_H__

/* modified by chenyu 130522 */
#ifdef WIN32

#ifdef EV9000SIP_EXPORTS
#define EV9000SIP_API    __declspec(dllexport)
#else
#define EV9000SIP_API    __declspec(dllimport)
#endif

#else
#define EV9000SIP_API    extern
#endif //end WIN32

#include <vector>

using namespace std;

#include <osipparser2/osip_const.h>
#include <osip2/internal.h>
#include <osip2/osip.h>
#include <osip2/osip_fifo.h>
#include <osipparser2/osip_list.h>
#include <osipparser2/osip_uri.h>
#include <osipparser2/osip_port.h>
#include <osipparser2/sdp_message.h>
#include <sipapp/mansrtsp.inc>
#include <sipapp/sdp_negoc.inc>

/* 错误码定义 */
#define EV9000_SIPSTACK_PARAM_ERROR                          0XF9010001  //协议栈参数错误
#define EV9000_SIPSTACK_NEW_CALLID_ERROR                     0XF9010002  //协议栈生成新的ID错误
#define EV9000_SIPSTACK_GET_SOCKET_ERROR                     0XF9010003  //协议栈发送消息获取Socket错误
#define EV9000_SIPSTACK_TRANSACTION_INIT_ERROR               0XF9010003  //协议栈发送消息初始化事物错误
#define EV9000_SIPSTACK_SEND_MESSAGE_ERROR                   0XF9010004  //协议栈发送消息错误
#define EV9000_SIPSTACK_GET_TRANSACTION_ERROR                0XF9010005  //协议栈获取协议栈事务失败

#define EV9000_SIPSTACK_UA_TIMER_INIT_ERROR                  0XF9020001  //协议栈UA定时器初始化失败
#define EV9000_SIPSTACK_SIP_TIMER_INIT_ERROR                 0XF9020002  //协议栈SIP定时器初始化失败
#define EV9000_SIPSTACK_UAC_TIMER_INIT_ERROR                 0XF9020103  //协议栈UAC定时器初始化失败
#define EV9000_SIPSTACK_UAS_TIMER_INIT_ERROR                 0XF9020104  //协议栈UAS定时器初始化失败
#define EV9000_SIPSTACK_UA_INIT_ERROR                        0XF9020105  //协议栈UA初始化失败
#define EV9000_SIPSTACK_SIP_MESSAGE_INIT_ERROR               0XF9020106  //协议栈SIP MESSAGE初始化失败
#define EV9000_SIPSTACK_CALL_BACK_INIT_ERROR                 0XF9020107  //协议栈Call Back初始化失败
#define EV9000_SIPSTACK_SIPSTACK_INIT_ERROR                  0XF9020108  //协议栈sip stack初始化失败
#define EV9000_SIPSTACK_RUN_THREAD_INIT_ERROR                0XF9020109  //协议栈线程初始化失败
#define EV9000_SIPSTACK_UDP_LIST_INIT_ERROR                  0XF9020110  //协议栈udp接收初始化失败

#define EV9000_SIPSTACK_REGISTER_GET_UAC_ERROR               0XF9030101  //协议栈发送注册消息获取UAC错误
#define EV9000_SIPSTACK_REGISTER_GENERA_ERROR                0XF9030102  //协议栈发送注册消息生成注册消息失败

#define EV9000_SIPSTACK_INVITE_GET_UA_ERROR                  0XF9040101  //协议栈发送Invite消息获取UA错误
#define EV9000_SIPSTACK_INVITE_GET_SDP_INFO_ERROR            0XF9040102  //协议栈发送Invite消息获取SDP错误
#define EV9000_SIPSTACK_INVITE_GENERA_ERROR                  0XF9040103  //协议栈发送Invite消息生成Invite消息失败
#define EV9000_SIPSTACK_INVITE_GET_SIPDLG_ERROR              0XF9040104  //协议栈发送Invite消息获取SIP会话错误
#define EV9000_SIPSTACK_INVITE_GET_REMOTE_SDP_ERROR          0XF9040105  //协议栈发送Invite消息获取对端SDP失败

#define EV9000_SIPSTACK_MESSAGE_GENERA_ERROR                 0XF9050101  //协议栈发送Message消息生成Message消息失败

#define EV9000_SIPSTACK_SDP_INIT_ERROR                       0XF9060101  //协议栈SDP初始化失败
#define EV9000_SIPSTACK_SDP_TO_STR_ERROR                     0XF9060102  //协议栈SDP转成字符串失败
#define EV9000_SIPSTACK_SDP_CLONE_ERROR                      0XF9060103  //协议栈SDP拷贝失败
#define EV9000_SIPSTACK_SDP_SET_REMOTE_SDP_ERROR             0XF9060104  //协议栈SDP设置对端的SDP失败

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

    /* SDP消息字段结构体 */
    typedef struct _sdp_param_t
    {
        char o_username[32];   /* 会话产生者名称，一般是发送Invite者的ID号 */
        char s_name[32];       /* s字段名称:"Play"代表实时视频，"Playback"代表回放，"Download"代表下载 */
        char sdp_ip[16];       /* 媒体IP地址 */
        int audio_port;        /* 音频端口号 */
        int audio_code_type;   /* 音频编码格式, EV9000_STREAMDATA_TYPE_AUDIO_G723 */
        int video_port;        /* 视频频端口号 */
        int video_code_type;   /* 视频频编码格式, EV9000_STREAMDATA_TYPE_PS */
        int start_time;        /* 录像回放开始时间 */
        int end_time;          /* 录像回放结束时间 */
        int play_time;         /* 录像回放播放时间 */
        int media_direction;   /* 媒体流方向: 0:inactive, 1:sendonly, 2:recvonly, 3:sendrecv */
        int stream_type;       /* 媒体流类型: 1:主视频流，2:从视频流，10:分析流, 默认主视频流 */
        int record_type;       /* 录像类型: 1:普通录像，2:智能录像，3:报警录像, 4:备份录像, 默认普通录像 */
        int trans_type;        /* 传输方式: 1:UDP，2:TCP */
        int file_size;         /* 文件大小，下载录像的时候，回应给客户端的 */
        int download_speed;    /* 下载倍速，用于下载时候的进度计算 */
        char y_ssrc[32];       /* SSRC 值，由CMS产生 */

        /* f字段 f=v/编码格式/分辨率/帧率/码率类型/码率大小a/编码格式/码率大小/采样率 */
        int f_v_code_type;     /* 视频编码格式: 十进制整数字符串表示:1 –MPEG-4 2 –H.264 3 – SVAC 4 –3GP */
        int f_v_ratio;         /* 视频分辨率：十进制整数字符串表示:1 – QCIF 2 – CIF 3 – 4CIF 4 – D1 5 –720P 6 –1080P/I */
        int f_v_frame_speed;   /* 视频帧率：十进制整数字符串表示 0 ~ 99 */
        int f_v_code_rate_type;/* 视频码率类型：十进制整数字符串表示:1 – 固定码率（CBR） 2 – 可变码率（VBR） */
        int f_v_code_rate_size;/* 视频码率大小：十进制整数字符串表示0 ~ 100000 （注：如1表示1kbps）*/
        int f_a_code_type;     /* 音频编码格式: 十进制整数字符串表示:1 – G.711 2 – G.723.1 3 – G.729 4 – G.722.1 */
        int f_a_code_rate_size;/* 音频码率大小：十进制整数字符串表示
                                               音频编码码率： 1 — 5.3 kbps （注：G.723.1中使用）
                                                2 — 6.3 kbps （注：G.723.1中使用）
                                                3 — 8 kbps （注：G.7 2 9中使用）
                                                4 — 16 kbps （注：G.722.1中使用）
                                                5 — 24 kbps （注：G.722.1中使用）
                                                6 — 32 kbps （注：G.722.1中使用）
                                                7 — 48 kbps （注：G.722.1中使用）
                                                8 — 64 kbps （注：G.7 11中使用） */
        int f_a_sample_rate;   /* 音频采样率：十进制整数字符串表示
                                               1- 8 kHz （注：G.711/ G.723.1/ G.729中使用）
                                               2—14 kHz （注：G.722.1中使用）
                                               3—16 kHz（注：G.722.1中使用）
                                               4—32 kHz（注：G.722.1中使用） */
    } sdp_param_t;

    /* SDP消息扩展字段结构体 */
    typedef struct _sdp_extend_param_t
    {
        char onvif_url[256];   /* Onvif回应时候传输的URL,带给TSU */
    } sdp_extend_param_t;

//初始化函数
    EV9000SIP_API int SIP_Init();

    /* 回调函数设置 */

    /* 服务器端收到客户端注册消息回调函数 设置,服务器使用
         参数:proxy_id, 注册的服务器id
              register_id, 注册的id
              from_ip, 来源IP
              from_port, 来源port
              user_name 注册用户名
              reg_info_index,注册句柄索引
              expires,时间
              link_type,联网类型
    */
    EV9000SIP_API void app_set_uas_register_received_cb(int (*cb)(char*, char*, char*, int, char*, int, int, int));

    /* 服务器端未收到客户端注册消息超时回调函数 设置
         参数:proxy_id, 注册的服务器id
              register_id, 注册的id

              from_ip, 来源IP
              from_port, 来源port
              reg_info_index,注册句柄索引
    */
    EV9000SIP_API void app_set_uas_register_received_timeout_cb(int (*cb)(char*, char*, char*, int, int));

    /* 客户端发送注册消息后收到服务端注册响应消息回调函数设置
         参数:        reg_info_index,注册句柄索引
                      expires 超时时间
                      status_code,注册响应消息码,408代表发送超时
                      reasonphrase, 注册响应的具体原因
                      iTime,服务器返回的校时时间
     */
    EV9000SIP_API void app_set_uac_register_response_received_cb(int (*cb)(int, int, int, char*, unsigned int, int), int user_data);

    /* 收到呼叫消息回调函数
         参数:       caller_id,呼叫来源的id
                     callee_id,呼叫目的的id
                     call_id,呼叫的唯一id
                     dialog_index,会话句柄索引
                     body:携带的body具体内容
                     body_len:携带的body长度
    */
    EV9000SIP_API void app_set_invite_received_cb(int (*cb)(char*, char*, char*, int, char*, int, int), int user_data);

    /* 收到呼叫响应消息回调函数
         参数:       caller_id,呼叫来源的id
                     callee_id,呼叫目的的id
                     call_id,呼叫的唯一id
                     dialog_index,会话句柄索引
                     status_code,呼叫响应消息码
                     reasonphrase, 呼叫响应的具体原因
                     body:携带的body具体内容
                     body_len:携带的body长度
    */
    EV9000SIP_API void app_set_invite_response_received_cb(int (*cb)(char*, char*, char*, int, int, char*, char*, int, int), int user_data);


    /* 收到Cancel 消息回调函数
         参数:       caller_id,呼叫来源的id
                     callee_id,呼叫目的的id
                     call_id,呼叫的唯一id
                     dialog_index,会话句柄索引
    */
    EV9000SIP_API void app_set_cancel_received_cb(int (*cb)(char*, char*, char*, int, int), int user_data);

    /* 收到ACK 消息回调函数
         参数:       caller_id,呼叫来源的id
                     callee_id,呼叫目的的id
                     call_id,呼叫的唯一id
                     dialog_index,会话句柄索引
    */
    EV9000SIP_API void app_set_ack_received_cb(int (*cb)(char*, char*, char*, int, int), int user_data);

    /* 收到呼叫结束消息回调函数
         参数:       caller_id,呼叫来源的id
                     callee_id,呼叫目的的id
                     call_id,呼叫的唯一id
                     dialog_index,会话句柄索引
    */
    EV9000SIP_API void app_set_bye_received_cb(int (*cb)(char*, char*, char*, int, int), int user_data);

    /* 收到呼叫结束响应消息回调函数
         参数:       caller_id,呼叫来源的id
                     callee_id,呼叫目的的id
                     call_id,呼叫的唯一id
                     dialog_index,会话句柄索引
                     status_code,呼叫响应消息码
    */
    EV9000SIP_API void app_set_bye_response_received_cb(int (*cb)(char*, char*, char*, int, int, int), int user_data);

    /* 收到Message 消息回调函数
         参数:        caller_id,发送方的device id
                      caller_ip,发送方的IP地址
                      caller_port,发送方的端口号
                      callee_id,接收方的device id
                      callee_ip,接收方的IP地址
                      callee_port,接收方的端口号
                      call_id:该message消息唯一的call id
                      dialog_index,会话句柄索引
                      msg:携带的msg具体内容
                      msg_len:携带的msg长度
    */
    EV9000SIP_API void app_set_message_received_cb(int (*cb)(char*, char*, int, char*, char*, int, char*, int, char*, int, int), int user_data);

    /* 收到Message 响应消息回调函数
         参数:        caller_id,发送方的device id
                      callee_id,接收方的device id
                      call_id:该message消息唯一的call id
                      status_code:Message响应消息码
    */
    EV9000SIP_API void app_set_message_response_received_cb(int (*cb)(char*, char*, char*, int, int), int user_data);

    /* 收到Subscribe 消息回调函数
         参数:        caller_id,发送方的device id
                      caller_ip,发送方的IP地址
                      caller_port,发送方的端口号
                      callee_id,接收方的device id
                      callee_ip,接收方的IP地址
                      callee_port,接收方的端口号
                      call_id:该message消息唯一的call id
                      event_type 事件类型
                      id_param 事件类型ID
                      subscribe_expires 超时时间
                      msg:携带的msg具体内容
                      msg_len:携带的msg长度
    */
    EV9000SIP_API void app_set_subscribe_received_cb(int (*cb)(char*, char*, int, char*, char*, int, char*, char*, char*, int, char*, int, int), int user_data);

    /* 收到Subscribe 响应消息回调函数
         参数:caller_id,发送方的device id
              callee_id,接收方的device id
              call_id:该message消息唯一的call id
              expires:超时时间
              status_code:Message响应消息码
    */
    EV9000SIP_API void app_set_subscribe_response_received_cb(int (*cb)(char*, char*, char*, int, int, int), int user_data);

    /* 收到Subscribe within dialog 消息回调函数
         参数:       caller_id,呼叫来源的id
                     caller_ip,发送方的IP地址
                     caller_port,发送方的端口号
                     callee_id,呼叫目的的id
                     call_id,呼叫的唯一id
                     dialog_index,会话句柄索引
                     subscribe_expires 超时时间
                     body:携带的body具体内容
                     body_len:携带的body长度
    */
    EV9000SIP_API void app_set_subscribe_within_dialog_received_cb(int (*cb)(char*, char*, int, char*, char*, int, int, char*, int));

    /* 收到Subscribe within dialog 响应消息回调函数
         参数:caller_id,发送方的device id
              callee_id,接收方的device id
              call_id:该message消息唯一的call id
              dialog_index,会话句柄索引
              expires:超时时间
              status_code:Message响应消息码
    */
    EV9000SIP_API void app_set_subscribe_within_dialog_response_received_cb(int (*cb)(char*, char*, char*, int, int, int));

    /* 收到 Notify 消息回调函数
         参数:        caller_id,发送方的device id
                      caller_ip,发送方的IP地址
                      caller_port,发送方的端口号
                      callee_id,接收方的device id
                      callee_ip,接收方的IP地址
                      callee_port,接收方的端口号
                      call_id:该message消息唯一的call id
                      msg:携带的msg具体内容
                      msg_len:携带的msg长度
    */
    EV9000SIP_API void app_set_notify_received_cb(int (*cb)(char*, char*, int, char*, char*, int, char*, char*, int, int), int user_data);

    /* 收到 Notify 响应消息回调函数
         参数:caller_id,发送方的device id
                      callee_id,接收方的device id
                      call_id:该message消息唯一的call id
                      status_code:Notify响应消息码
    */
    EV9000SIP_API void app_set_notify_response_received_cb(int (*cb)(char*, char*, char*, int, int), int user_data);

    /* 收到Info 消息回调函数
         参数:caller_id,发送方的device id
                      callee_id,接收方的device id
                      call_id:该message消息唯一的call id
                      msg:携带的msg具体内容
                      msg_len:携带的msg长度
    */
    EV9000SIP_API void app_set_info_received_cb(int (*cb)(char*, char*, int, char*, char*, int, char*, int, int), int user_data);

    /* 收到Info 响应消息回调函数
         参数:caller_id,发送方的device id
                      callee_id,接收方的device id
                      call_id:该message消息唯一的call id
                      status_code:Info响应消息码
    */
    EV9000SIP_API void app_set_info_response_received_cb(int (*cb)(char*, char*, char*, int, int), int user_data);

    /* UA 会话超时回调函数
         参数: dialog_index,会话句柄索引
    */
    EV9000SIP_API void app_set_ua_session_expires_cb(int (*cb)(int));

    /* 调试打印函数
         参数:const char*
                      ...
    */
    EV9000SIP_API void app_set_dbg_printf_cb(void (*cb)(int, const char*, const char*, int, const char*));

    /* SIP消息跟踪调试回调函数设置
      */
    EV9000SIP_API void app_set_sip_message_trace_cb(void (*cb)(int, int, char*, int, char*));

    /* 服务器端在特定的端口上启动SIP接收线程 */
    EV9000SIP_API int SIP_UASStartUdpReceive(int local_port);

    /* 客户端启动SIP接收线程，返回启动的端口号 */
    EV9000SIP_API int SIP_UACStartUdpReceive(int* local_port);

    /* 停止SIP接收线程 */
    EV9000SIP_API int SIP_StopUdpReceive(int local_port);

//SIP 释放函数，最终关闭服务时调用；通话过程不可调用此函数
    EV9000SIP_API void SIP_Free();

//发送注册消息
    EV9000SIP_API int SIP_SendRegister(char* service_id, char* local_id, char* local_ip, int local_port, char* server_ip, int server_port, char* username, char* userpassword, int expires);

//发送注册消息
    EV9000SIP_API int SIP_SendRegisterForRoute(char* service_id, char* local_id, char* local_ip, int local_port, char* server_ip, int server_port, char* username, char* userpassword, int expires, int link_type);

//发送去注册消息
    EV9000SIP_API int SIP_SendUnRegister(int reg_info_index);

//发送刷新注册消息
    EV9000SIP_API int SIP_SendRegisterForRefresh(int reg_info_index);

//发送初始呼叫消息
    EV9000SIP_API int SIP_SendInvite(char* caller_id, char* callee_id, char* local_ip, int local_port, char* server_ip, int server_port, char* username, char* userpassword, sdp_message_t* local_sdp);

//发送呼叫消息到目的地
    EV9000SIP_API int SIP_ProxyBuildTargetAndSendInviteByIPAndPort(char* caller_id, char* callee_id, char* callee_register_id, char* callee_ip, int callee_port, sdp_message_t* local_sdp);

//转发会话内呼叫消息
    EV9000SIP_API int SIP_ProxyForwardInviteWithinDialog(int caller_dialog_pos, char* local_id,  char* local_ip, int local_port, char* remote_id, char* remote_ip, int remote_port, sdp_message_t* local_sdp);

//接收呼叫消息
    EV9000SIP_API int SIP_AcceptInvite(int dialog_index, sdp_message_t* local_sdp);

//发送Cancel 消息
    EV9000SIP_API int SIP_SendCancel(int dialog_index);

//发送Ack 消息
    EV9000SIP_API int SIP_SendAck(int dialog_index);

//发送结束呼叫消息
    EV9000SIP_API int SIP_SendBye(int dialog_index);

//发送会话外Message消息
    EV9000SIP_API int SIP_SendMessage(char* message_id, char* caller_id, char* callee_id, char* local_ip, int local_port, char* server_ip, int server_port, char* msg, int msg_len);

//通过TCP发送会话外Message消息
    EV9000SIP_API int SIP_SendMessage_By_TCP(char* message_id, char* caller_id, char* callee_id, char* local_ip, int local_port, char* server_ip, int server_port, char* msg, int msg_len, int tcp_scoket);

//发送会话内Message消息
    EV9000SIP_API int SIP_SendMsgWithinDialog(int dialog_index,  char* msg, int msg_len);

//服务器端查找目的地转发Message消息
    EV9000SIP_API int SIP_ProxyBuildTargetAndSendMessage(char* caller_id, char* callee_id, char* callee_register_id, char* msg, int msg_len);

//服务器端根据IP和端口查找目的地转发Message消息
    EV9000SIP_API int SIP_ProxyBuildTargetAndSendMessageByIPAndPort(char* caller_id, char* callee_id, char* callee_register_id, char* callee_ip, int callee_port, char* msg, int msg_len);

//发送会话外Subscribe消息
    EV9000SIP_API int SIP_SendSubscribe(char* message_id, char* caller_id, char* callee_id, char* event, int event_id, int expires, char* local_ip, int local_port, char* server_ip, int server_port, char* msg, int msg_len);

//发送会话外Subscribe刷新消息
    EV9000SIP_API int SIP_SubscribeRefresh(int dialog_index);

//发送会话外Subscribe取消消息
    EV9000SIP_API int SIP_UnSubscribe(int dialog_index);

//发送会话外Notify消息
    EV9000SIP_API int SIP_SendNotify(char* message_id, char* caller_id, char* callee_id, char* event, int event_id, int expires, char* local_ip, int local_port, char* server_ip, int server_port, char* msg, int msg_len);

//通过TCP发送会话外Notify消息
    EV9000SIP_API int SIP_SendNotify_By_TCP(char* message_id, char* caller_id, char* callee_id, char* event, int event_id, int expires, char* local_ip, int local_port, char* server_ip, int server_port, char* msg, int msg_len, int tcp_scoket);

//发送会话内Notify消息
    EV9000SIP_API int SIP_SendNotifyWithinDialog(int dialog_index, char* body, int body_len);

//发送会话外Info消息
    EV9000SIP_API int SIP_SendInfo(char* message_id, char* caller_id, char* callee_id, char* local_ip, int local_port, char* server_ip, int server_port, char* body, int body_len);

//发送会话内Info消息
    EV9000SIP_API int SIP_SendInfoWithinDialog(int dialog_index, char* body, int body_len);

//应答会话
    EV9000SIP_API int SIP_AnswerToInvite(int dialog_index, int code, char* reasonphrase);

//在会话定时器间隔太小的情况下回应INVITE
    EV9000SIP_API int SIP_AnswerToInviteForSessionExpires(int dialog_index, int min_se);

//应答Bye消息
    EV9000SIP_API int SIP_AnswerToBye(int dialog_index, int code, char* reasonphrase);

//服务端应答注册消息
    EV9000SIP_API int SIP_UASAnswerToRegister(int reg_info_index, int code, char* reasonphrase);

//服务端应答注册认证消息
    EV9000SIP_API int SIP_UASAnswerToRegister4Auth(int reg_info_index,  char* realm);

//服务端应答注册刷新时间太短消息
    EV9000SIP_API int SIP_UASAnswerToRegister4RegExpire(int reg_info_index,  int iMinRegExpire);

//sip认证
    EV9000SIP_API int SIP_Auth(osip_authorization_t* authorization, char* username, char* password, char* method);

//应答SIP Message消息
    EV9000SIP_API int SIP_AnswerToSipMessage(char* call_id, int code, char* reasonphrase);

//应答SIP Info消息
    EV9000SIP_API int SIP_AnswerToSipInfo(char* call_id, int code, char* reasonphrase);

//获取会话的认证信息
    EV9000SIP_API osip_authorization_t* SIP_GetInviteDialogAuthorization(int dialog_index);

//获取会话的超时时间
    EV9000SIP_API int SIP_GetInviteDialogSessionExpires(int dialog_index);

//获取会话的来源host
    EV9000SIP_API char* SIP_GetDialogFromHost(int dialog_index);

//获取会话的来源port
    EV9000SIP_API int SIP_GetDialogFromPort(int dialog_index);

//获取会话的Call ID
    EV9000SIP_API char* SIP_GetDialogCallID(int dialog_index);

//获取会话的远端SDP信息
    EV9000SIP_API sdp_message_t* SIP_GetInviteDialogRemoteSDP(int dialog_index);

//获取会话外的消息的来源host
    EV9000SIP_API char* SIP_GetOutDialogFromHost(char* call_id);

//获取会话外的消息的来源port
    EV9000SIP_API int SIP_GetOutDialogFromPort(char* call_id);

//获取sip消息所在的会话index
    EV9000SIP_API int SIP_GetSipMsgDialogIndex(char* call_id);

//服务端更新注册的超时时间
    EV9000SIP_API int SIP_UASUpdateRegisterExpires(int reg_info_index);

//服务端移除注册信息
    EV9000SIP_API int SIP_UASRemoveRegisterInfo(int reg_info_index);

//服务端获取注册的认证信息
    EV9000SIP_API osip_authorization_t* SIP_UASGetRegisterAuthorization(int reg_info_index);

//获取当前使用的所有SIPUA的会话句柄
    EV9000SIP_API int SIP_GetAllUsedSIPUAIndex(vector<int>& SIPUAIndexVector);

//释放没有使用的SIPUA会话
    EV9000SIP_API int SIP_ReleaseUnUsedSIPUA(int index);


//SDP相关函数
    /*s_name:"Play",         实时视频播放
                    "Playback",  历史视频回放
                    "Download", 文件下载
        start_time,end_time:
                    实时视频的时候都填0
                    录像回放的时候填写开始时间和结束时间，格式是time_t
        play_time:
                    实时视频的时候都填0
                    录像回放的时候填的开始播放时间，格式是time_t
        media_direction:媒体流方向
                                0:inactive
                                1:sendonly
                                2:recvonly
                                3:sendrecv
    */
    EV9000SIP_API int sdp_build_offer(sdp_context_t* con, sdp_message_t** sdp, char* audio_port,
                                      char* video_port, char* localip, char* s_name, int start_time, int end_time, int play_time, int media_direction, int audio_code_type, int video_code_type);
    EV9000SIP_API int sdp_message_init(sdp_message_t** sdp);
    EV9000SIP_API int sdp_message_parse(sdp_message_t* sdp, const char* buf);
    EV9000SIP_API int sdp_message_to_str(sdp_message_t* sdp, char** dest);
    EV9000SIP_API void sdp_message_free(sdp_message_t* sdp);
    EV9000SIP_API int sdp_message_clone(sdp_message_t* sdp, sdp_message_t** dest);
    EV9000SIP_API int SIP_GetSDPVideoInfo(sdp_message_t* sdp, unsigned long* addr, int* port, int* codetype, int* flag);
    EV9000SIP_API int SIP_GeneratingSDPAnswer(int dialog_index, sdp_message_t** local_sdp, char* audio_port, char* video_port, char* localip, char* s_name, int start_time, int end_time, int play_time, int media_direction, int audio_code_type, int video_code_type, sdp_extend_param_t* pSDPExtendParm = NULL);

    /* 构建本地SDP信息 */
    EV9000SIP_API int SIP_BuildSDPInfoEx(sdp_message_t** sdp, sdp_param_t* sdp_param, sdp_extend_param_t* pSDPExtendParm);

    /* 获取SDP信息 */
    EV9000SIP_API int SIP_GetSDPInfoEx(sdp_message_t* sdp, sdp_param_t* sdp_param, sdp_extend_param_t* pSDPExtendParm);

    /* 生成应答SDP信息 */
    EV9000SIP_API int SIP_GeneratingSDPAnswerEx(int dialog_index, sdp_message_t** local_sdp, sdp_param_t* pSDPParm, sdp_extend_param_t* pSDPExtendParm);

//其他函数
    EV9000SIP_API char* new_callid();
    EV9000SIP_API void free_callid(char*p);  //added by chenyu 151012

    EV9000SIP_API void SIP_ShowSIPUATask(int sock);
    EV9000SIP_API void SIP_ShowSIPUADetail(int sock, int ua_index);
    EV9000SIP_API void SIP_ShowUASRegisterInfo(int sock);
    EV9000SIP_API void SIP_ShowUACRegisterInfo(int sock);
    EV9000SIP_API void SIP_ReleaseAllSIPUAInfo();
    EV9000SIP_API void SIP_ReleaseAllUASRegisterInfo();
    EV9000SIP_API void SIP_ReleaseAllUACRegisterInfo();
    EV9000SIP_API char* SIP_GetUASServerIP(char* register_id, char* login_ip, int login_port);
    EV9000SIP_API int SIP_GetUASServerPort(char* register_id, char* login_ip, int login_port);
    EV9000SIP_API int SIP_GetUASRegExpires(char* register_id, char* login_ip, int login_port);
    EV9000SIP_API char* SIP_GetUASCallID(char* register_id, char* login_ip, int login_port);
    EV9000SIP_API void SIP_ShowSIPStackTransactionSize(int sock);

    /* 协议栈1秒定时器通知函数 */
    EV9000SIP_API void SIP_1sTimerNotify();

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* __SIPUA_H__ */
