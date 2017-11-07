/******************************************************************************

                  版权所有 (C), 2001-2013, 金智视讯技术有限公司

 ******************************************************************************
  文 件 名   : sipua.h
  版 本 号   : 初稿
  作    者   : yanghaifeng
  生成日期   : 2013年4月7日
  最近修改   :
  功能描述   : sipua.c 的头文件
  函数列表   :
  修改历史   :
  1.日    期   : 2013年4月7日
    作    者   : yanghaifeng
    修改内容   : 创建文件

******************************************************************************/
#ifndef __SIPUA_H__
#define __SIPUA_H__

/*----------------------------------------------*
 * 包含头文件                                   *
 *----------------------------------------------*/
#include <osip2/internal.h>
#include <osip2/osip.h>

#include "sipheader.inc"
#include "sdp_negoc.inc"

#include <vector>
#include <string>
#include <map>
#include <queue>
#include <list>

using namespace std;

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
    typedef enum ui_state
    {
        UI_STATE_IDLE,
        UI_STATE_CONNECTED,
        UI_STATE_CONFERENCE,
        UI_STATE_CALL_SENT,       /* call sent */
        UI_STATE_200_RECEIVE,     /* 200 OK receive */
        UI_STATE_BYE_RECEIVE,       /* bye receive */
        UI_STATE_CALL_TERMINATED, /* call terminated (receive 3456xx or bye or ) */
        UI_STATE_CALL_RCV,        /* call received */
        UI_STATE_CALL_ACCEPT,     /* call received and accept */
    }
    ui_state_t;

    typedef enum _dialog_event_t
    {
        DLG_EVENT_NULL,
        DLG_EVENT_1XXNOTAG,
        DLG_EVENT_1XXTAG,
        DLG_EVENT_2XX,
        DLG_EVENT_CANCELLED,
        DLG_EVENT_REJECTED,
        DLG_EVENT_REPLACED,
        DLG_EVENT_ERROR,
        DLG_EVENT_TIMEOUT,
        DLG_EVENT_LOCALBYE,
        DLG_EVENT_REMOTEBYE,
        DLG_EVENT_UPDATE,
    } dialog_event_t;

    typedef enum _dialog_state_t
    {
        DLG_TRYING = 1,
        DLG_PROCEEDDING,
        DLG_EARLY,
        DLG_CONFIRMED,
        DLG_TERMINATED
    } dialog_state_t; //

    typedef enum _dlg_type1_t
    {
        DLG_CALLER,
        DLG_CALLEE,
    } dlg_type1_t;

    typedef enum _dlg_type2_t
    {
        DLG_INVITE,
        DLG_SUBSCRIBE,
        DLG_REFER
    } dlg_type2_t;

    typedef enum subscription_state
    {
        SUB_STATE_PRE,
        SUB_STATE_CONFIRMED,
        SUB_STATE_TERMINATED,
        SUB_STATE_CLEAR
    } subscription_state;

    typedef struct sip_subscription sip_subscription_t;

    struct sip_subscription
    {
        subscription_state state;
        char* event_type;
        char* sub_state;
        char* id_param;
        time_t begin;
        int duration;

        osip_uri* remote_contact_uri;
    };

    typedef struct _sip_dialog_t
    {
        char* call_id;
        char* local_tag;
        char* remote_tag;
        osip_list_t* route_set;
        int local_cseq;
        int remote_cseq;
        osip_to_t* remote_uri;
        osip_from_t* local_uri;
        osip_contact_t* remote_contact_uri;
        dlg_type1_t type1;       /* type of dialog (DLG_CALLEE or DLG_CALLER) */
        dlg_type2_t type2;       /* type of dialog created by Method (DLG_INVITE,DLG_SUBSCRIBE,DLG_REFER)*/
        dialog_state_t state;    /* */

        /* transaction */
        osip_transaction_t*      ict_tr;   /*last ict transaction*/
        osip_transaction_t*      ist_tr;   /*last ist transaction*/
        osip_transaction_t*      nict_tr;  /*last nict transaction*/
        osip_transaction_t*      nist_tr;  /*last nist transaction*/
    } sip_dialog_t;

    typedef struct _ua_dialog_t
    {
        int iStatus;                     /*0:default,1:recv INVITE,wating for respoonse,2:have respoonse*/

        char strCallerID[128 + 4];       /* 主叫id  */
        char strCalleeID[128 + 4];       /* 被叫id  */

        char strUserName[128 + 4];       /* 认证账户 */
        char strPassword[128 + 4];       /* 认证密码 */

        char strRemoteIP[16];            /* 对端ip */
        int iRemotePort;                 /*  对端端口号*/

        char strRemoteRTPIP[16];         /* 对端媒体  ip */
        int iRemoteRTPPort;              /*  对端媒体 端口号*/
        sdp_message_t* pRemoteSDP;       /*  对端SDP 信息*/

        char strLocalIP[16];             /* 本地ip */
        int iLocalPort;                  /* 本地端口号 */

        char strLocalRTPIP[16];          /* 本地媒体 IP 地址*/
        int iLocalRTPPort;               /* 本地媒体 端口号 */
        sdp_message_t* pLocalSDP;        /*  本端SDP 信息*/

        osip_transaction_t* pServerTr;
        osip_message_t* pServerSIP;
        osip_authorization_t* pAuthorization;

        int iStatusCode;
        ui_state_t eUiState;

        int iReInviteCnt;

        int iSessionExpires;

        int iSessionExpiresCount;

        int iUpdateSendCount;

        sip_dialog_t* pSipDialog;

        /* subscriptions with the dialog */
        sip_subscription_t* subscription;

        int uSeqNnumber;
        int ulTimeStamp;
    } ua_dialog_t;

    typedef std::map <int, ua_dialog_t*> UA_Dialog_MAP;

    typedef std::deque<int> free_UA_Dialog_Queue;
    typedef std::deque<int>::iterator free_UA_Dialog_Iterator;

    typedef std::list<int> used_UA_Dialog_Queue;
    typedef std::list<int>::iterator used_UA_Dialog_Iterator;

    extern int AddDialogSubscription(int index, sip_subscription_t * sip_sub);
    extern int debug_free_ua_dialog_list_lock(const char* file, int line, const char* func);
    extern int debug_free_ua_dialog_list_unlock(const char* file, int line, const char* func);
    extern int debug_used_ua_dialog_list_lock(const char* file, int line, const char* func);
    extern int debug_used_ua_dialog_list_unlock(const char* file, int line, const char* func);
    extern int do_accept_call(int index, sdp_message_t* local_sdp);
    extern int find_dialog_as_uac(osip_message_t* answer);
    extern int find_dialog_as_uas(osip_message_t* request);
    extern int free_ua_dialog_list_lock();
    extern int free_ua_dialog_list_unlock();
    extern sip_subscription_t *GetDialogSubscription(int index);
    extern sip_dialog_t* get_dialog_sip_dialog(int index);
    extern sip_dialog_t* get_dialog_sip_dialog2(int index);
    extern ui_state_t  get_dialog_ui_state(int index);
    extern int is_valid_dialog_index(int index);
    extern void scan_ua_dialogs();
    extern int sdp_accept_audio_codec(sdp_context_t * context, char * port, char * number_of_port, int audio_qty, char *payload);
    extern int sdp_accept_video_codec(sdp_context_t* context, char* port, char* number_of_port, int video_qty, char* payload);
    extern int sdp_local_config();
    extern sip_subscription_t * SearchSubscription(int dialog_index, char *event_type, char *id_param);
    extern int set_dialog_ui_state(int index,  ui_state_t state);
    extern void SipAnswerToSubscribe(int dialog_index, sip_subscription_t* sip_sub, osip_transaction_t* tr, int code);
    extern int sip_2xx_answer_to_invite(int dialog_pos, char* callee, char* localip, int localport, int session_expires, sdp_message_t* local_sdp);
    extern int sip_3456xxxx_answer_to_invite(int dialog_pos, int code, char* reasonphrase);
    EV9000SIP_API int SIP_AcceptInvite(int dialog_index, sdp_message_t* local_sdp);
    extern int sip_ack(int dialog_pos, char* localip, int localport, int iSessionExpires);
    EV9000SIP_API int SIP_AnswerToBye(int dialog_index, int code, char* reasonphrase);
    EV9000SIP_API int SIP_AnswerToInvite(int dialog_index, int code, char* reasonphrase);
    EV9000SIP_API int SIP_AnswerToInviteForSessionExpires(int dialog_index, int min_se);
    EV9000SIP_API int SIP_AnswerToSipInfo(char* call_id, int code, char* reasonphrase);
    EV9000SIP_API int SIP_AnswerToSipMessage(char* call_id, int code, char* reasonphrase);
    extern void sip_answer_to_bye(int dialog_pos, osip_transaction_t* tr, int code, char* reasonphrase);
    extern void sip_answer_to_cancel(int dialog_pos, osip_transaction_t* tr, int code);
    extern void sip_answer_to_info(int dialog_pos, osip_transaction_t* tr, int code);
    extern void sip_answer_to_options(int dialog_pos, osip_transaction_t* tr, int code, char* localip);
    extern void sip_answer_to_refer(int dialog_pos, osip_transaction_t* tr, int code);
    extern void sip_answer_to_update(int dialog_pos, osip_transaction_t* tr, int code, int session_expires);
    extern int sip_bye(int dialog_pos, char* localip, int localport);
    extern int sip_bye2(osip_message_t* request, osip_message_t* resp, char* proxyip, int proxyport, char* localip, int localport, char* username, char* userpassword);
    extern int sip_cancel(int dialog_pos);
    extern void sip_dialog_free(sip_dialog_t* dialog);
    extern int sip_dialog_init_as_uac(sip_dialog_t** dialog, osip_transaction_t* tr, osip_message_t* sip);
    extern int sip_dialog_init_as_uas(sip_dialog_t** dialog, osip_transaction_t* tr, osip_message_t* sip);
    extern int  sip_dialog_match_as_uac(sip_dialog_t* dlg, osip_message_t* answer);
    extern int  sip_dialog_match_as_uas(sip_dialog_t* dlg, osip_message_t* request);
    extern int sip_dialog_set_localcseq(sip_dialog_t* dialog, int cseq);
    extern int sip_dialog_set_localtag(sip_dialog_t* dialog, char* tag);
    extern int sip_dialog_set_localuri(sip_dialog_t* dialog, osip_from_t* local);
    extern int sip_dialog_set_remotecontact(sip_dialog_t* dialog, osip_contact_t* contact);
    extern int sip_dialog_set_remotecseq(sip_dialog_t* dialog, int cseq);
    extern int sip_dialog_set_remotetag(sip_dialog_t* dialog, char* tag);
    extern int sip_dialog_set_remoteuri(sip_dialog_t* dialog, osip_to_t* remote);
    extern int sip_dialog_set_routeset(sip_dialog_t* dialog, osip_list_t* rset);
    extern int sip_forward_invite_within_dialog(int caller_dialog_pos, int callee_dialog_pos);
    EV9000SIP_API int SIP_GetAllUsedSIPUAIndex(vector<int>& SIPUAIndexVector);
    EV9000SIP_API char* SIP_GetDialogCallID(int dialog_index);
    EV9000SIP_API char* SIP_GetDialogFromHost(int dialog_index);
    EV9000SIP_API int SIP_GetDialogFromPort(int dialog_index);
    EV9000SIP_API osip_authorization_t* SIP_GetInviteDialogAuthorization(int dialog_index);
    EV9000SIP_API sdp_message_t* SIP_GetInviteDialogRemoteSDP(int dialog_index);
    EV9000SIP_API int SIP_GetInviteDialogSessionExpires(int dialog_index);
    EV9000SIP_API char* SIP_GetOutDialogFromHost(char* call_id);
    EV9000SIP_API int SIP_GetOutDialogFromPort(char* call_id);
    EV9000SIP_API int SIP_GetSipMsgDialogIndex(char* call_id);
    extern int sip_info(char* callid, char* caller, char* callee, char* body, int body_len, osip_uri_t* requrl, char* proxyip, int proxyport, char* localip, int localport);
    extern int sip_info_within_dialog(int dialog_pos, char* body, int body_len, char* localip, int localport);
    extern int sip_invite(int ua_dialog_pos);
    extern int sip_invite2(osip_message_t* request, osip_message_t* resp, char* csAuthname, char* csAuthpassword, char* proxyip, int proxyport, char* localip, int localport);
    extern int sip_message(char* callid, char* caller, char* callee, char* msg, int msg_len, char* proxyip, int proxyport, char* localip, int localport);
    extern int sip_message_for_tcp(char* callid, char* caller, char* callee, char* msg, int msg_len, char* proxyip, int proxyport, char* localip, int localport, int tcp_socket);
    extern int sip_message_within_dialog(int dialog_pos, char* msg, int msg_len, char* localip, int localport);
    extern int sip_notify(char* callid, char* caller, char* callee, char* event, int event_id, int expires, char* proxyip, int proxyport, char* localip, int localport, char* msg, int msg_len);
    extern int sip_notify_for_tcp(char* callid, char* caller, char* callee, char* event, int event_id, int expires, char* proxyip, int proxyport, char* localip, int localport, char* msg, int msg_len, int tcp_socket);
    extern int sip_notify_within_dialog(int dialog_pos, char* body, int body_len);
    extern int sip_options(int dialog_pos, char* caller, char* callee, char* proxyip, int proxyport, char* localip, int localport);
    EV9000SIP_API int SIP_ProxyBuildTargetAndSendInviteByIPAndPort(char* caller_id, char* callee_id, char* callee_register_id, char* callee_ip, int callee_port, sdp_message_t* local_sdp);
    EV9000SIP_API int SIP_ProxyBuildTargetAndSendMessage(char* caller_id, char* callee_id, char* callee_register_id, char* msg, int msg_len);
    EV9000SIP_API int SIP_ProxyBuildTargetAndSendMessageByIPAndPort(char* caller_id, char* callee_id, char* callee_register_id, char* callee_ip, int callee_port, char* msg, int msg_len);
    EV9000SIP_API int SIP_ProxyForwardInviteWithinDialog(int caller_dialog_pos, char* local_id,  char* local_ip, int local_port, char* remote_id, char* remote_ip, int remote_port, sdp_message_t* local_sdp);
    extern int sip_register(char* caller, char* username, char* proxyid, char* proxyip, int proxyport, char* localip, int localport, int expires, char* register_callid_number, int register_cseq_number, int link_type);
    extern int sip_register2(osip_message_t* request, osip_message_t* resp, char* proxyip, int proxyport, char* localip, int localport, char* username, char* userpassword, int link_type);
    EV9000SIP_API void SIP_ReleaseAllSIPUAInfo();
    EV9000SIP_API int SIP_ReleaseUnUsedSIPUA(int index);
    extern void sip_response_default(int dialog_pos, osip_transaction_t* tr, int code, char* reasonphrase);
    EV9000SIP_API int SIP_SendAck(int dialog_index);
    EV9000SIP_API int SIP_SendBye(int dialog_index);
    EV9000SIP_API int SIP_SendCancel(int dialog_index);
    EV9000SIP_API int SIP_SendInfo(char* message_id, char* caller_id, char* callee_id, char* local_ip, int local_port, char* server_ip, int server_port, char* body, int body_len);
    EV9000SIP_API int SIP_SendInfoWithinDialog(int dialog_index, char* body, int body_len);
    EV9000SIP_API int SIP_SendInvite(char* caller_id, char* callee_id, char* local_ip, int local_port, char* server_ip, int server_port, char* username, char* userpassword, sdp_message_t* local_sdp);
    EV9000SIP_API int SIP_SendMessage(char* message_id, char* caller_id, char* callee_id, char* local_ip, int local_port, char* server_ip, int server_port, char* msg, int msg_len);
    EV9000SIP_API int SIP_SendMessage_By_TCP(char* message_id, char* caller_id, char* callee_id, char* local_ip, int local_port, char* server_ip, int server_port, char* msg, int msg_len, int tcp_scoket);
    EV9000SIP_API int SIP_SendMsgWithinDialog(int dialog_index,  char* msg, int msg_len);
    EV9000SIP_API int SIP_SendNotify(char* message_id, char* caller_id, char* callee_id, char* event, int event_id, int expires, char* local_ip, int local_port, char* server_ip, int server_port, char* msg, int msg_len);
    EV9000SIP_API int SIP_SendNotifyWithinDialog(int dialog_index, char* body, int body_len);
    EV9000SIP_API int SIP_SendNotify_By_TCP(char* message_id, char* caller_id, char* callee_id, char* event, int event_id, int expires, char* local_ip, int local_port, char* server_ip, int server_port, char* msg, int msg_len, int tcp_scoket);
    EV9000SIP_API int SIP_SendRegister(char* service_id, char* local_id, char* local_ip, int local_port, char* server_ip, int server_port, char* username, char* userpassword, int expires);
    EV9000SIP_API int SIP_SendRegisterForRefresh(int reg_info_index);
    EV9000SIP_API int SIP_SendRegisterForRoute(char* service_id, char* local_id, char* local_ip, int local_port, char* server_ip, int server_port, char* username, char* userpassword, int expires, int link_type);
    EV9000SIP_API int SIP_SendSubscribe(char* message_id, char* caller_id, char* callee_id, char* event, int event_id, int expires, char* local_ip, int local_port, char* server_ip, int server_port, char* msg, int msg_len);
    EV9000SIP_API int SIP_SendUnRegister(int reg_info_index);
    EV9000SIP_API void SIP_ShowSIPUADetail(int sock, int ua_index);
    EV9000SIP_API void SIP_ShowSIPUATask(int sock);
    extern int sip_subscribe(int dialog_pos, char* callid, char* event, int event_id, int expires, char* body, int body_len);
    EV9000SIP_API int SIP_SubscribeRefresh(int dialog_index);
    extern void sip_subscription_free(sip_subscription_t * sub);
    extern int sip_subscription_init(sip_subscription_t ** sub);
    extern int sip_subscription_match(sip_subscription_t * sub, char *event_type, char *id);
    EV9000SIP_API int SIP_UnSubscribe(int dialog_index);
    extern int sip_update(int dialog_pos);
    extern int sip_update_within_dialog(int dialog_pos, char* localip, int localport, int session_expires);
    extern int uas_check8_2(osip_transaction_t* tr, osip_message_t* sip);
    extern int uas_check8_2_2(osip_transaction_t* tr, osip_message_t* sip);
    extern int uas_check8_2_3(osip_transaction_t* tr, osip_message_t* sip);
    extern int uas_check8_2_4(osip_transaction_t* tr, osip_message_t* sip);
    extern int uas_check_invite_within_dialog(int index, osip_transaction_t* tr, osip_message_t* sip);
    extern int ua_dialog_add();
    extern void ua_dialog_free(ua_dialog_t* ua_dialog);
    extern ua_dialog_t* ua_dialog_get(int pos);
    extern ua_dialog_t* ua_dialog_get2(int pos);
    extern int ua_dialog_init(ua_dialog_t** ua_dialog);
    extern void ua_dialog_list_free();
    extern int  ua_dialog_list_init();
    extern void ua_dialog_remove(int pos);
    extern int UpdateSubscription(sip_subscription_t * sip_sub , subscription_state state);
    extern int update_dialog_as_uac(int index, osip_transaction_t* tr, osip_message_t* sip, dialog_event_t event);
    extern int update_dialog_as_uac2(sip_dialog_t* pSipDlg, osip_transaction_t* tr, osip_message_t* sip, dialog_event_t event);
    extern int update_dialog_as_uas(int index, osip_transaction_t* tr, osip_message_t* sip, dialog_event_t event);
    extern int update_dialog_as_uas2(sip_dialog_t* pSipDlg, osip_transaction_t* tr, osip_message_t* sip, dialog_event_t event);
    extern int used_ua_dialog_list_lock();
    extern int used_ua_dialog_list_unlock();

#ifdef ENABLE_DEBUG_LOCK
#define FREE_UA_SMUTEX_LOCK() debug_free_ua_dialog_list_lock(__FILE__, __LINE__, __func__)
#define FREE_UA_SMUTEX_UNLOCK() debug_free_ua_dialog_list_unlock(__FILE__, __LINE__, __func__)
#define USED_UA_SMUTEX_LOCK() debug_used_ua_dialog_list_lock(__FILE__, __LINE__, __func__)
#define USED_UA_SMUTEX_UNLOCK() debug_used_ua_dialog_list_unlock(__FILE__, __LINE__, __func__)
#else
#define FREE_UA_SMUTEX_LOCK() free_ua_dialog_list_lock()
#define FREE_UA_SMUTEX_UNLOCK() free_ua_dialog_list_unlock()
#define USED_UA_SMUTEX_LOCK() used_ua_dialog_list_lock()
#define USED_UA_SMUTEX_UNLOCK() used_ua_dialog_list_unlock()
#endif

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif /* __SIPUA_H__ */
