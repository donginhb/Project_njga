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

/* �����붨�� */
#define EV9000_SIPSTACK_PARAM_ERROR                          0XF9010001  //Э��ջ��������
#define EV9000_SIPSTACK_NEW_CALLID_ERROR                     0XF9010002  //Э��ջ�����µ�ID����
#define EV9000_SIPSTACK_GET_SOCKET_ERROR                     0XF9010003  //Э��ջ������Ϣ��ȡSocket����
#define EV9000_SIPSTACK_TRANSACTION_INIT_ERROR               0XF9010003  //Э��ջ������Ϣ��ʼ���������
#define EV9000_SIPSTACK_SEND_MESSAGE_ERROR                   0XF9010004  //Э��ջ������Ϣ����
#define EV9000_SIPSTACK_GET_TRANSACTION_ERROR                0XF9010005  //Э��ջ��ȡЭ��ջ����ʧ��

#define EV9000_SIPSTACK_UA_TIMER_INIT_ERROR                  0XF9020001  //Э��ջUA��ʱ����ʼ��ʧ��
#define EV9000_SIPSTACK_SIP_TIMER_INIT_ERROR                 0XF9020002  //Э��ջSIP��ʱ����ʼ��ʧ��
#define EV9000_SIPSTACK_UAC_TIMER_INIT_ERROR                 0XF9020103  //Э��ջUAC��ʱ����ʼ��ʧ��
#define EV9000_SIPSTACK_UAS_TIMER_INIT_ERROR                 0XF9020104  //Э��ջUAS��ʱ����ʼ��ʧ��
#define EV9000_SIPSTACK_UA_INIT_ERROR                        0XF9020105  //Э��ջUA��ʼ��ʧ��
#define EV9000_SIPSTACK_SIP_MESSAGE_INIT_ERROR               0XF9020106  //Э��ջSIP MESSAGE��ʼ��ʧ��
#define EV9000_SIPSTACK_CALL_BACK_INIT_ERROR                 0XF9020107  //Э��ջCall Back��ʼ��ʧ��
#define EV9000_SIPSTACK_SIPSTACK_INIT_ERROR                  0XF9020108  //Э��ջsip stack��ʼ��ʧ��
#define EV9000_SIPSTACK_RUN_THREAD_INIT_ERROR                0XF9020109  //Э��ջ�̳߳�ʼ��ʧ��
#define EV9000_SIPSTACK_UDP_LIST_INIT_ERROR                  0XF9020110  //Э��ջudp���ճ�ʼ��ʧ��

#define EV9000_SIPSTACK_REGISTER_GET_UAC_ERROR               0XF9030101  //Э��ջ����ע����Ϣ��ȡUAC����
#define EV9000_SIPSTACK_REGISTER_GENERA_ERROR                0XF9030102  //Э��ջ����ע����Ϣ����ע����Ϣʧ��

#define EV9000_SIPSTACK_INVITE_GET_UA_ERROR                  0XF9040101  //Э��ջ����Invite��Ϣ��ȡUA����
#define EV9000_SIPSTACK_INVITE_GET_SDP_INFO_ERROR            0XF9040102  //Э��ջ����Invite��Ϣ��ȡSDP����
#define EV9000_SIPSTACK_INVITE_GENERA_ERROR                  0XF9040103  //Э��ջ����Invite��Ϣ����Invite��Ϣʧ��
#define EV9000_SIPSTACK_INVITE_GET_SIPDLG_ERROR              0XF9040104  //Э��ջ����Invite��Ϣ��ȡSIP�Ự����
#define EV9000_SIPSTACK_INVITE_GET_REMOTE_SDP_ERROR          0XF9040105  //Э��ջ����Invite��Ϣ��ȡ�Զ�SDPʧ��

#define EV9000_SIPSTACK_MESSAGE_GENERA_ERROR                 0XF9050101  //Э��ջ����Message��Ϣ����Message��Ϣʧ��

#define EV9000_SIPSTACK_SDP_INIT_ERROR                       0XF9060101  //Э��ջSDP��ʼ��ʧ��
#define EV9000_SIPSTACK_SDP_TO_STR_ERROR                     0XF9060102  //Э��ջSDPת���ַ���ʧ��
#define EV9000_SIPSTACK_SDP_CLONE_ERROR                      0XF9060103  //Э��ջSDP����ʧ��
#define EV9000_SIPSTACK_SDP_SET_REMOTE_SDP_ERROR             0XF9060104  //Э��ջSDP���öԶ˵�SDPʧ��

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

    /* SDP��Ϣ�ֶνṹ�� */
    typedef struct _sdp_param_t
    {
        char o_username[32];   /* �Ự���������ƣ�һ���Ƿ���Invite�ߵ�ID�� */
        char s_name[32];       /* s�ֶ�����:"Play"����ʵʱ��Ƶ��"Playback"����طţ�"Download"�������� */
        char sdp_ip[16];       /* ý��IP��ַ */
        int audio_port;        /* ��Ƶ�˿ں� */
        int audio_code_type;   /* ��Ƶ�����ʽ, EV9000_STREAMDATA_TYPE_AUDIO_G723 */
        int video_port;        /* ��ƵƵ�˿ں� */
        int video_code_type;   /* ��ƵƵ�����ʽ, EV9000_STREAMDATA_TYPE_PS */
        int start_time;        /* ¼��طſ�ʼʱ�� */
        int end_time;          /* ¼��طŽ���ʱ�� */
        int play_time;         /* ¼��طŲ���ʱ�� */
        int media_direction;   /* ý��������: 0:inactive, 1:sendonly, 2:recvonly, 3:sendrecv */
        int stream_type;       /* ý��������: 1:����Ƶ����2:����Ƶ����10:������, Ĭ������Ƶ�� */
        int record_type;       /* ¼������: 1:��ͨ¼��2:����¼��3:����¼��, 4:����¼��, Ĭ����ͨ¼�� */
        int trans_type;        /* ���䷽ʽ: 1:UDP��2:TCP */
        int file_size;         /* �ļ���С������¼���ʱ�򣬻�Ӧ���ͻ��˵� */
        int download_speed;    /* ���ر��٣���������ʱ��Ľ��ȼ��� */
        char y_ssrc[32];       /* SSRC ֵ����CMS���� */

        /* f�ֶ� f=v/�����ʽ/�ֱ���/֡��/��������/���ʴ�Сa/�����ʽ/���ʴ�С/������ */
        int f_v_code_type;     /* ��Ƶ�����ʽ: ʮ���������ַ�����ʾ:1 �CMPEG-4 2 �CH.264 3 �C SVAC 4 �C3GP */
        int f_v_ratio;         /* ��Ƶ�ֱ��ʣ�ʮ���������ַ�����ʾ:1 �C QCIF 2 �C CIF 3 �C 4CIF 4 �C D1 5 �C720P 6 �C1080P/I */
        int f_v_frame_speed;   /* ��Ƶ֡�ʣ�ʮ���������ַ�����ʾ 0 ~ 99 */
        int f_v_code_rate_type;/* ��Ƶ�������ͣ�ʮ���������ַ�����ʾ:1 �C �̶����ʣ�CBR�� 2 �C �ɱ����ʣ�VBR�� */
        int f_v_code_rate_size;/* ��Ƶ���ʴ�С��ʮ���������ַ�����ʾ0 ~ 100000 ��ע����1��ʾ1kbps��*/
        int f_a_code_type;     /* ��Ƶ�����ʽ: ʮ���������ַ�����ʾ:1 �C G.711 2 �C G.723.1 3 �C G.729 4 �C G.722.1 */
        int f_a_code_rate_size;/* ��Ƶ���ʴ�С��ʮ���������ַ�����ʾ
                                               ��Ƶ�������ʣ� 1 �� 5.3 kbps ��ע��G.723.1��ʹ�ã�
                                                2 �� 6.3 kbps ��ע��G.723.1��ʹ�ã�
                                                3 �� 8 kbps ��ע��G.7 2 9��ʹ�ã�
                                                4 �� 16 kbps ��ע��G.722.1��ʹ�ã�
                                                5 �� 24 kbps ��ע��G.722.1��ʹ�ã�
                                                6 �� 32 kbps ��ע��G.722.1��ʹ�ã�
                                                7 �� 48 kbps ��ע��G.722.1��ʹ�ã�
                                                8 �� 64 kbps ��ע��G.7 11��ʹ�ã� */
        int f_a_sample_rate;   /* ��Ƶ�����ʣ�ʮ���������ַ�����ʾ
                                               1- 8 kHz ��ע��G.711/ G.723.1/ G.729��ʹ�ã�
                                               2��14 kHz ��ע��G.722.1��ʹ�ã�
                                               3��16 kHz��ע��G.722.1��ʹ�ã�
                                               4��32 kHz��ע��G.722.1��ʹ�ã� */
    } sdp_param_t;

    /* SDP��Ϣ��չ�ֶνṹ�� */
    typedef struct _sdp_extend_param_t
    {
        char onvif_url[256];   /* Onvif��Ӧʱ�����URL,����TSU */
    } sdp_extend_param_t;

//��ʼ������
    EV9000SIP_API int SIP_Init();

    /* �ص��������� */

    /* ���������յ��ͻ���ע����Ϣ�ص����� ����,������ʹ��
         ����:proxy_id, ע��ķ�����id
              register_id, ע���id
              from_ip, ��ԴIP
              from_port, ��Դport
              user_name ע���û���
              reg_info_index,ע��������
              expires,ʱ��
              link_type,��������
    */
    EV9000SIP_API void app_set_uas_register_received_cb(int (*cb)(char*, char*, char*, int, char*, int, int, int));

    /* ��������δ�յ��ͻ���ע����Ϣ��ʱ�ص����� ����
         ����:proxy_id, ע��ķ�����id
              register_id, ע���id

              from_ip, ��ԴIP
              from_port, ��Դport
              reg_info_index,ע��������
    */
    EV9000SIP_API void app_set_uas_register_received_timeout_cb(int (*cb)(char*, char*, char*, int, int));

    /* �ͻ��˷���ע����Ϣ���յ������ע����Ӧ��Ϣ�ص���������
         ����:        reg_info_index,ע��������
                      expires ��ʱʱ��
                      status_code,ע����Ӧ��Ϣ��,408�����ͳ�ʱ
                      reasonphrase, ע����Ӧ�ľ���ԭ��
                      iTime,���������ص�Уʱʱ��
     */
    EV9000SIP_API void app_set_uac_register_response_received_cb(int (*cb)(int, int, int, char*, unsigned int, int), int user_data);

    /* �յ�������Ϣ�ص�����
         ����:       caller_id,������Դ��id
                     callee_id,����Ŀ�ĵ�id
                     call_id,���е�Ψһid
                     dialog_index,�Ự�������
                     body:Я����body��������
                     body_len:Я����body����
    */
    EV9000SIP_API void app_set_invite_received_cb(int (*cb)(char*, char*, char*, int, char*, int, int), int user_data);

    /* �յ�������Ӧ��Ϣ�ص�����
         ����:       caller_id,������Դ��id
                     callee_id,����Ŀ�ĵ�id
                     call_id,���е�Ψһid
                     dialog_index,�Ự�������
                     status_code,������Ӧ��Ϣ��
                     reasonphrase, ������Ӧ�ľ���ԭ��
                     body:Я����body��������
                     body_len:Я����body����
    */
    EV9000SIP_API void app_set_invite_response_received_cb(int (*cb)(char*, char*, char*, int, int, char*, char*, int, int), int user_data);


    /* �յ�Cancel ��Ϣ�ص�����
         ����:       caller_id,������Դ��id
                     callee_id,����Ŀ�ĵ�id
                     call_id,���е�Ψһid
                     dialog_index,�Ự�������
    */
    EV9000SIP_API void app_set_cancel_received_cb(int (*cb)(char*, char*, char*, int, int), int user_data);

    /* �յ�ACK ��Ϣ�ص�����
         ����:       caller_id,������Դ��id
                     callee_id,����Ŀ�ĵ�id
                     call_id,���е�Ψһid
                     dialog_index,�Ự�������
    */
    EV9000SIP_API void app_set_ack_received_cb(int (*cb)(char*, char*, char*, int, int), int user_data);

    /* �յ����н�����Ϣ�ص�����
         ����:       caller_id,������Դ��id
                     callee_id,����Ŀ�ĵ�id
                     call_id,���е�Ψһid
                     dialog_index,�Ự�������
    */
    EV9000SIP_API void app_set_bye_received_cb(int (*cb)(char*, char*, char*, int, int), int user_data);

    /* �յ����н�����Ӧ��Ϣ�ص�����
         ����:       caller_id,������Դ��id
                     callee_id,����Ŀ�ĵ�id
                     call_id,���е�Ψһid
                     dialog_index,�Ự�������
                     status_code,������Ӧ��Ϣ��
    */
    EV9000SIP_API void app_set_bye_response_received_cb(int (*cb)(char*, char*, char*, int, int, int), int user_data);

    /* �յ�Message ��Ϣ�ص�����
         ����:        caller_id,���ͷ���device id
                      caller_ip,���ͷ���IP��ַ
                      caller_port,���ͷ��Ķ˿ں�
                      callee_id,���շ���device id
                      callee_ip,���շ���IP��ַ
                      callee_port,���շ��Ķ˿ں�
                      call_id:��message��ϢΨһ��call id
                      dialog_index,�Ự�������
                      msg:Я����msg��������
                      msg_len:Я����msg����
    */
    EV9000SIP_API void app_set_message_received_cb(int (*cb)(char*, char*, int, char*, char*, int, char*, int, char*, int, int), int user_data);

    /* �յ�Message ��Ӧ��Ϣ�ص�����
         ����:        caller_id,���ͷ���device id
                      callee_id,���շ���device id
                      call_id:��message��ϢΨһ��call id
                      status_code:Message��Ӧ��Ϣ��
    */
    EV9000SIP_API void app_set_message_response_received_cb(int (*cb)(char*, char*, char*, int, int), int user_data);

    /* �յ�Subscribe ��Ϣ�ص�����
         ����:        caller_id,���ͷ���device id
                      caller_ip,���ͷ���IP��ַ
                      caller_port,���ͷ��Ķ˿ں�
                      callee_id,���շ���device id
                      callee_ip,���շ���IP��ַ
                      callee_port,���շ��Ķ˿ں�
                      call_id:��message��ϢΨһ��call id
                      event_type �¼�����
                      id_param �¼�����ID
                      subscribe_expires ��ʱʱ��
                      msg:Я����msg��������
                      msg_len:Я����msg����
    */
    EV9000SIP_API void app_set_subscribe_received_cb(int (*cb)(char*, char*, int, char*, char*, int, char*, char*, char*, int, char*, int, int), int user_data);

    /* �յ�Subscribe ��Ӧ��Ϣ�ص�����
         ����:caller_id,���ͷ���device id
              callee_id,���շ���device id
              call_id:��message��ϢΨһ��call id
              expires:��ʱʱ��
              status_code:Message��Ӧ��Ϣ��
    */
    EV9000SIP_API void app_set_subscribe_response_received_cb(int (*cb)(char*, char*, char*, int, int, int), int user_data);

    /* �յ�Subscribe within dialog ��Ϣ�ص�����
         ����:       caller_id,������Դ��id
                     caller_ip,���ͷ���IP��ַ
                     caller_port,���ͷ��Ķ˿ں�
                     callee_id,����Ŀ�ĵ�id
                     call_id,���е�Ψһid
                     dialog_index,�Ự�������
                     subscribe_expires ��ʱʱ��
                     body:Я����body��������
                     body_len:Я����body����
    */
    EV9000SIP_API void app_set_subscribe_within_dialog_received_cb(int (*cb)(char*, char*, int, char*, char*, int, int, char*, int));

    /* �յ�Subscribe within dialog ��Ӧ��Ϣ�ص�����
         ����:caller_id,���ͷ���device id
              callee_id,���շ���device id
              call_id:��message��ϢΨһ��call id
              dialog_index,�Ự�������
              expires:��ʱʱ��
              status_code:Message��Ӧ��Ϣ��
    */
    EV9000SIP_API void app_set_subscribe_within_dialog_response_received_cb(int (*cb)(char*, char*, char*, int, int, int));

    /* �յ� Notify ��Ϣ�ص�����
         ����:        caller_id,���ͷ���device id
                      caller_ip,���ͷ���IP��ַ
                      caller_port,���ͷ��Ķ˿ں�
                      callee_id,���շ���device id
                      callee_ip,���շ���IP��ַ
                      callee_port,���շ��Ķ˿ں�
                      call_id:��message��ϢΨһ��call id
                      msg:Я����msg��������
                      msg_len:Я����msg����
    */
    EV9000SIP_API void app_set_notify_received_cb(int (*cb)(char*, char*, int, char*, char*, int, char*, char*, int, int), int user_data);

    /* �յ� Notify ��Ӧ��Ϣ�ص�����
         ����:caller_id,���ͷ���device id
                      callee_id,���շ���device id
                      call_id:��message��ϢΨһ��call id
                      status_code:Notify��Ӧ��Ϣ��
    */
    EV9000SIP_API void app_set_notify_response_received_cb(int (*cb)(char*, char*, char*, int, int), int user_data);

    /* �յ�Info ��Ϣ�ص�����
         ����:caller_id,���ͷ���device id
                      callee_id,���շ���device id
                      call_id:��message��ϢΨһ��call id
                      msg:Я����msg��������
                      msg_len:Я����msg����
    */
    EV9000SIP_API void app_set_info_received_cb(int (*cb)(char*, char*, int, char*, char*, int, char*, int, int), int user_data);

    /* �յ�Info ��Ӧ��Ϣ�ص�����
         ����:caller_id,���ͷ���device id
                      callee_id,���շ���device id
                      call_id:��message��ϢΨһ��call id
                      status_code:Info��Ӧ��Ϣ��
    */
    EV9000SIP_API void app_set_info_response_received_cb(int (*cb)(char*, char*, char*, int, int), int user_data);

    /* UA �Ự��ʱ�ص�����
         ����: dialog_index,�Ự�������
    */
    EV9000SIP_API void app_set_ua_session_expires_cb(int (*cb)(int));

    /* ���Դ�ӡ����
         ����:const char*
                      ...
    */
    EV9000SIP_API void app_set_dbg_printf_cb(void (*cb)(int, const char*, const char*, int, const char*));

    /* SIP��Ϣ���ٵ��Իص���������
      */
    EV9000SIP_API void app_set_sip_message_trace_cb(void (*cb)(int, int, char*, int, char*));

    /* �����������ض��Ķ˿�������SIP�����߳� */
    EV9000SIP_API int SIP_UASStartUdpReceive(int local_port);

    /* �ͻ�������SIP�����̣߳����������Ķ˿ں� */
    EV9000SIP_API int SIP_UACStartUdpReceive(int* local_port);

    /* ֹͣSIP�����߳� */
    EV9000SIP_API int SIP_StopUdpReceive(int local_port);

//SIP �ͷź��������չرշ���ʱ���ã�ͨ�����̲��ɵ��ô˺���
    EV9000SIP_API void SIP_Free();

//����ע����Ϣ
    EV9000SIP_API int SIP_SendRegister(char* service_id, char* local_id, char* local_ip, int local_port, char* server_ip, int server_port, char* username, char* userpassword, int expires);

//����ע����Ϣ
    EV9000SIP_API int SIP_SendRegisterForRoute(char* service_id, char* local_id, char* local_ip, int local_port, char* server_ip, int server_port, char* username, char* userpassword, int expires, int link_type);

//����ȥע����Ϣ
    EV9000SIP_API int SIP_SendUnRegister(int reg_info_index);

//����ˢ��ע����Ϣ
    EV9000SIP_API int SIP_SendRegisterForRefresh(int reg_info_index);

//���ͳ�ʼ������Ϣ
    EV9000SIP_API int SIP_SendInvite(char* caller_id, char* callee_id, char* local_ip, int local_port, char* server_ip, int server_port, char* username, char* userpassword, sdp_message_t* local_sdp);

//���ͺ�����Ϣ��Ŀ�ĵ�
    EV9000SIP_API int SIP_ProxyBuildTargetAndSendInviteByIPAndPort(char* caller_id, char* callee_id, char* callee_register_id, char* callee_ip, int callee_port, sdp_message_t* local_sdp);

//ת���Ự�ں�����Ϣ
    EV9000SIP_API int SIP_ProxyForwardInviteWithinDialog(int caller_dialog_pos, char* local_id,  char* local_ip, int local_port, char* remote_id, char* remote_ip, int remote_port, sdp_message_t* local_sdp);

//���պ�����Ϣ
    EV9000SIP_API int SIP_AcceptInvite(int dialog_index, sdp_message_t* local_sdp);

//����Cancel ��Ϣ
    EV9000SIP_API int SIP_SendCancel(int dialog_index);

//����Ack ��Ϣ
    EV9000SIP_API int SIP_SendAck(int dialog_index);

//���ͽ���������Ϣ
    EV9000SIP_API int SIP_SendBye(int dialog_index);

//���ͻỰ��Message��Ϣ
    EV9000SIP_API int SIP_SendMessage(char* message_id, char* caller_id, char* callee_id, char* local_ip, int local_port, char* server_ip, int server_port, char* msg, int msg_len);

//ͨ��TCP���ͻỰ��Message��Ϣ
    EV9000SIP_API int SIP_SendMessage_By_TCP(char* message_id, char* caller_id, char* callee_id, char* local_ip, int local_port, char* server_ip, int server_port, char* msg, int msg_len, int tcp_scoket);

//���ͻỰ��Message��Ϣ
    EV9000SIP_API int SIP_SendMsgWithinDialog(int dialog_index,  char* msg, int msg_len);

//�������˲���Ŀ�ĵ�ת��Message��Ϣ
    EV9000SIP_API int SIP_ProxyBuildTargetAndSendMessage(char* caller_id, char* callee_id, char* callee_register_id, char* msg, int msg_len);

//�������˸���IP�Ͷ˿ڲ���Ŀ�ĵ�ת��Message��Ϣ
    EV9000SIP_API int SIP_ProxyBuildTargetAndSendMessageByIPAndPort(char* caller_id, char* callee_id, char* callee_register_id, char* callee_ip, int callee_port, char* msg, int msg_len);

//���ͻỰ��Subscribe��Ϣ
    EV9000SIP_API int SIP_SendSubscribe(char* message_id, char* caller_id, char* callee_id, char* event, int event_id, int expires, char* local_ip, int local_port, char* server_ip, int server_port, char* msg, int msg_len);

//���ͻỰ��Subscribeˢ����Ϣ
    EV9000SIP_API int SIP_SubscribeRefresh(int dialog_index);

//���ͻỰ��Subscribeȡ����Ϣ
    EV9000SIP_API int SIP_UnSubscribe(int dialog_index);

//���ͻỰ��Notify��Ϣ
    EV9000SIP_API int SIP_SendNotify(char* message_id, char* caller_id, char* callee_id, char* event, int event_id, int expires, char* local_ip, int local_port, char* server_ip, int server_port, char* msg, int msg_len);

//ͨ��TCP���ͻỰ��Notify��Ϣ
    EV9000SIP_API int SIP_SendNotify_By_TCP(char* message_id, char* caller_id, char* callee_id, char* event, int event_id, int expires, char* local_ip, int local_port, char* server_ip, int server_port, char* msg, int msg_len, int tcp_scoket);

//���ͻỰ��Notify��Ϣ
    EV9000SIP_API int SIP_SendNotifyWithinDialog(int dialog_index, char* body, int body_len);

//���ͻỰ��Info��Ϣ
    EV9000SIP_API int SIP_SendInfo(char* message_id, char* caller_id, char* callee_id, char* local_ip, int local_port, char* server_ip, int server_port, char* body, int body_len);

//���ͻỰ��Info��Ϣ
    EV9000SIP_API int SIP_SendInfoWithinDialog(int dialog_index, char* body, int body_len);

//Ӧ��Ự
    EV9000SIP_API int SIP_AnswerToInvite(int dialog_index, int code, char* reasonphrase);

//�ڻỰ��ʱ�����̫С������»�ӦINVITE
    EV9000SIP_API int SIP_AnswerToInviteForSessionExpires(int dialog_index, int min_se);

//Ӧ��Bye��Ϣ
    EV9000SIP_API int SIP_AnswerToBye(int dialog_index, int code, char* reasonphrase);

//�����Ӧ��ע����Ϣ
    EV9000SIP_API int SIP_UASAnswerToRegister(int reg_info_index, int code, char* reasonphrase);

//�����Ӧ��ע����֤��Ϣ
    EV9000SIP_API int SIP_UASAnswerToRegister4Auth(int reg_info_index,  char* realm);

//�����Ӧ��ע��ˢ��ʱ��̫����Ϣ
    EV9000SIP_API int SIP_UASAnswerToRegister4RegExpire(int reg_info_index,  int iMinRegExpire);

//sip��֤
    EV9000SIP_API int SIP_Auth(osip_authorization_t* authorization, char* username, char* password, char* method);

//Ӧ��SIP Message��Ϣ
    EV9000SIP_API int SIP_AnswerToSipMessage(char* call_id, int code, char* reasonphrase);

//Ӧ��SIP Info��Ϣ
    EV9000SIP_API int SIP_AnswerToSipInfo(char* call_id, int code, char* reasonphrase);

//��ȡ�Ự����֤��Ϣ
    EV9000SIP_API osip_authorization_t* SIP_GetInviteDialogAuthorization(int dialog_index);

//��ȡ�Ự�ĳ�ʱʱ��
    EV9000SIP_API int SIP_GetInviteDialogSessionExpires(int dialog_index);

//��ȡ�Ự����Դhost
    EV9000SIP_API char* SIP_GetDialogFromHost(int dialog_index);

//��ȡ�Ự����Դport
    EV9000SIP_API int SIP_GetDialogFromPort(int dialog_index);

//��ȡ�Ự��Call ID
    EV9000SIP_API char* SIP_GetDialogCallID(int dialog_index);

//��ȡ�Ự��Զ��SDP��Ϣ
    EV9000SIP_API sdp_message_t* SIP_GetInviteDialogRemoteSDP(int dialog_index);

//��ȡ�Ự�����Ϣ����Դhost
    EV9000SIP_API char* SIP_GetOutDialogFromHost(char* call_id);

//��ȡ�Ự�����Ϣ����Դport
    EV9000SIP_API int SIP_GetOutDialogFromPort(char* call_id);

//��ȡsip��Ϣ���ڵĻỰindex
    EV9000SIP_API int SIP_GetSipMsgDialogIndex(char* call_id);

//����˸���ע��ĳ�ʱʱ��
    EV9000SIP_API int SIP_UASUpdateRegisterExpires(int reg_info_index);

//������Ƴ�ע����Ϣ
    EV9000SIP_API int SIP_UASRemoveRegisterInfo(int reg_info_index);

//����˻�ȡע�����֤��Ϣ
    EV9000SIP_API osip_authorization_t* SIP_UASGetRegisterAuthorization(int reg_info_index);

//��ȡ��ǰʹ�õ�����SIPUA�ĻỰ���
    EV9000SIP_API int SIP_GetAllUsedSIPUAIndex(vector<int>& SIPUAIndexVector);

//�ͷ�û��ʹ�õ�SIPUA�Ự
    EV9000SIP_API int SIP_ReleaseUnUsedSIPUA(int index);


//SDP��غ���
    /*s_name:"Play",         ʵʱ��Ƶ����
                    "Playback",  ��ʷ��Ƶ�ط�
                    "Download", �ļ�����
        start_time,end_time:
                    ʵʱ��Ƶ��ʱ����0
                    ¼��طŵ�ʱ����д��ʼʱ��ͽ���ʱ�䣬��ʽ��time_t
        play_time:
                    ʵʱ��Ƶ��ʱ����0
                    ¼��طŵ�ʱ����Ŀ�ʼ����ʱ�䣬��ʽ��time_t
        media_direction:ý��������
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

    /* ��������SDP��Ϣ */
    EV9000SIP_API int SIP_BuildSDPInfoEx(sdp_message_t** sdp, sdp_param_t* sdp_param, sdp_extend_param_t* pSDPExtendParm);

    /* ��ȡSDP��Ϣ */
    EV9000SIP_API int SIP_GetSDPInfoEx(sdp_message_t* sdp, sdp_param_t* sdp_param, sdp_extend_param_t* pSDPExtendParm);

    /* ����Ӧ��SDP��Ϣ */
    EV9000SIP_API int SIP_GeneratingSDPAnswerEx(int dialog_index, sdp_message_t** local_sdp, sdp_param_t* pSDPParm, sdp_extend_param_t* pSDPExtendParm);

//��������
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

    /* Э��ջ1�붨ʱ��֪ͨ���� */
    EV9000SIP_API void SIP_1sTimerNotify();

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* __SIPUA_H__ */
