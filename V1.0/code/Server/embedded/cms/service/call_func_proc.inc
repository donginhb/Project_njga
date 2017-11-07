#ifndef __CALL_FUNC_PROC_H__
#define __CALL_FUNC_PROC_H__


/*----------------------------------------------*
 * ����ͷ�ļ�                                   *
 *----------------------------------------------*/
#include <sys/types.h>
#ifdef WIN32
#include <winsock2.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#endif

#include "libsip.h"

#include "common/gblconfig_proc.inc"
#include "device/device_info_mgn.inc"
#include "route/route_info_mgn.inc"

#include <queue>
#include <vector>
#include <string>
using namespace std;

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

    /*----------------------------------------------*
     * �ⲿ����˵��                                 *
     *----------------------------------------------*/

    /*----------------------------------------------*
     * �ⲿ����ԭ��˵��                             *
     *----------------------------------------------*/

    /*----------------------------------------------*
     * �ڲ�����ԭ��˵��                             *
     *----------------------------------------------*/

    /*----------------------------------------------*
     * ȫ�ֱ���                                     *
     *----------------------------------------------*/

    /*----------------------------------------------*
     * ģ�鼶����                                   *
     *----------------------------------------------*/

    /*----------------------------------------------*
     * ��������                                     *
     *----------------------------------------------*/

    /*----------------------------------------------*
     * �궨��                                       *
     *----------------------------------------------*/

    typedef enum _call_type_t
    {
        CALL_TYPE_REALTIME,                        /* ʵʱ��Ƶ */
        CALL_TYPE_RECORD,                          /* ¼��ҵ�� */
        CALL_TYPE_DC,                              /* DC��ǽҵ�� */
        CALL_TYPE_RECORD_PLAY,                     /* ��ʷͼ��ط� */
        CALL_TYPE_DOWNLOAD,                        /* �ļ����� */
        CALL_TYPE_DIAGNOSIS,                       /* ����������� */
        CALL_TYPE_INTELLIGENT,                     /* ���ܷ������� */
        CALL_TYPE_AUDIO,                           /* ��Ƶ�Խ� */

        CALL_TYPE_NULL
    }
    call_type_t;

    typedef enum _call_status_t
    {
        CALL_STATUS_WAIT_ANSWER,                   /* �ȴ���Ӧ */
        CALL_STATUS_WAIT_RELEASE,                  /* �ȴ��ͷ� */
        CALL_STATUS_RELEASE_COMPLETE,              /* �ͷ���� */

        CALL_STATUS_NULL
    }
    call_status_t;

    /* �����������ݿ�ṹ�� */
    typedef struct _cr_t
    {
        int iUsed;                                       /* �Ƿ�����*/
        call_type_t call_type;                           /* ����ҵ������� */

        char caller_id[MAX_ID_LEN + 4];                  /* ���в��û�ID */
        char caller_ip[MAX_IP_LEN];                      /* ���в��û�IP */
        int caller_port;                                 /* ���в��û��˿� */
        char caller_sdp_ip[MAX_IP_LEN];                  /* ���в��û�ý��IP */
        int caller_sdp_port;                             /* ���в��û�ý�� �˿� */
        int caller_ua_index;                             /* ���в��û���������Index */
        char caller_server_ip_ethname[MAX_IP_LEN];       /* ���в������IP��ַ�������� */
        char caller_server_ip[MAX_IP_LEN];               /* ���в��û�������IP */
        int caller_server_port;                          /* ���в��û��������˿� */
        transfer_protocol_type_t caller_transfer_type;   /* ���в��û�����Э������ */

        char callee_id[MAX_ID_LEN + 4];                  /* ���в�ID */
        char callee_ip[MAX_IP_LEN];                      /* ���в��û�IP */
        int callee_port;                                 /* ���в��û��˿� */
        char callee_sdp_ip[MAX_IP_LEN];                  /* ���в��û�ý��IP */
        int callee_sdp_port;                             /* ���в��û�ý�� �˿� */
        int callee_ua_index;                             /* ���в��û���������Index */
        char callee_server_ip_ethname[MAX_IP_LEN];       /* ���в������IP��ַ�������� */
        char callee_server_ip[MAX_IP_LEN];               /* ���в��û�������IP */
        int callee_server_port;                          /* ���в��û��������˿� */
        transfer_protocol_type_t callee_transfer_type;   /* ���в��û�����Э������ */

        int callee_service_type;                         /* ���в�ҵ������:0:ʵʱ��Ƶ��1:¼�� */
        int callee_record_type;                          /* ���в�¼������:��ͨ¼��,����¼��,����¼��,����¼�� */
        int callee_stream_type;                          /* ���в�ý��������:������������������ */
        int callee_framerate;                            /* ���в�֡�� */
        char callee_onvif_url[256];                      /* ���в�ONVIFЭ���URL */
        int callee_gb_device_type;                       /* ���в������豸���� */

        float iScale;                                    /* ¼��ط�ʱ��ı��� */
        int iPlaybackTimeGap;                            /* ¼��ط�ʱ��Ĳ���ʱ��ƫ����,����ʱ��-��ʼʱ�� */

        char tsu_device_id[MAX_ID_LEN + 4];              /* ��ʹ�õ� TSU�豸ͳһ��� */
        int tsu_resource_index;                          /* TSU�������� */
        char tsu_ip[MAX_IP_LEN];                         /* ��TSU ͨ�ŵ�IP��ַ */
        ip_pair_t TSUVideoIP;                            /* TSU ��Ƶ��IP��ַ */
        ip_pair_t TSUDeviceIP;                           /* TSU �豸��IP��ַ */

        int tsu_code;                                    /* TSU ʹ�õı������� */
        int tsu_recv_port;                               /* TSU ʹ�õĽ��ն˿ں�*/
        int tsu_send_port;                               /* TSU ʹ�õķ��Ͷ˿ں�*/
        char task_id[MAX_TSU_TASK_LEN + 4];              /* ��Ӧ��TSU ʹ�õ�����ID*/

        int tsu_session_expire;                          /* ��TSU�ı��ʱʱ��,Ĭ��10���� */
        int wait_answer_expire;                          /* �ȴ���Ӧ��ʱʱ��,Ĭ��15���� */

        call_status_t call_status;                       /* ����״̬: */

        unsigned int nResved1;                           /* ����1 */
        char   strResved2[MAX_32CHAR_STRING_LEN + 4];    /* ����2 */
    } cr_t;

    /* �����������ݶ��� */
    typedef std::map <int, cr_t*> CR_Data_MAP;
    typedef std::map <int, cr_t*>::iterator CR_Data_Iterator;


    /* Ack ��Ӧ���нڵ�ṹ */
    typedef struct  _ack_send_t
    {
        int cr_index;               /* ��Ӧ�ĺ�����Դ����*/
        int caller_ua_index;        /* ���в�ĻỰ����*/
        int caller_time_count;      /* ���в��Ack ��ʱ*/
        int callee_ua_index;        /* ���в�ĻỰ����*/
        int callee_time_count;      /* ���в��Ack ��ʱ*/
    } ack_send_t;

    /* Ack ��Ӧ����*/
    typedef struct _ack_send_list_t
    {
        osip_list_t* ack_list;

        /* the lock */
#ifdef MULTI_THR
        osip_mutex_t*  lock;
#endif

    } ack_send_list_t;

    /* ��Ҫת���� XML ��Ϣ�ṹ�� */
    typedef struct _transfer_xml_msg_t
    {
        xml_type_t msg_type;             /* XML ��Ϣ���� */
        unsigned int old_xml_sn;         /* XML ��Ϣԭ�������к� */
        unsigned int transfer_xml_sn;    /* XML ��Ϣת���µ����к� */
        char device_id[MAX_ID_LEN + 4];  /* ǰ���豸ͳһ��� */
        time_t  recevice_time;           /* ��Ϣ����ʱ�� */
        char source_id[MAX_ID_LEN + 4];  /* ����Դ��ID */
        char source_ip[MAX_IP_LEN];      /* ����Դ��IP */
        int source_port;                 /* ����Դ�Ķ˿ں� */
        char local_ip[MAX_IP_LEN];       /* CMS���ص�IP */
        int local_port;                  /* CMS���صĶ˿ں� */
        int iSumNum;                     /* ��ѯ��Ӧ������ */
        int iListNum;                    /* ÿ����¼������ */
    } transfer_xml_msg_t;

    /* ��Ҫת���� XML ��Ϣ���� */
    typedef struct _transfer_xml_msg_list_t
    {
        osip_list_t* pXMLMsgList;
#ifdef MULTI_THR
        osip_mutex_t* lock;
#endif
    } transfer_xml_msg_list_t;

    extern int ack_send_find(int cr_index);
    extern void ack_send_free(ack_send_t* node);
    extern int ack_send_init(ack_send_t** node);
    extern int ack_send_list_clean();
    extern void ack_send_list_free();
    extern int ack_send_list_init();
    extern int ack_send_proc(int cr_index, int iResult);
    extern int ack_send_remove(int cr_index);
    extern int ack_send_use(int cr_index, int caller_ua_index, int callee_ua_index);
    extern int audio_call_record_find_by_send_info(char* caller_ip, int caller_port);
    extern int call_record_add();
    extern int call_record_find_by_callee_index(int callee_ua_index);
    extern int call_record_find_by_callerid_and_calleeid(char* caller_id, char* callee_id);
    extern int call_record_find_by_callerinfo(char* caller_id, char* caller_sdp_ip, int caller_sdp_port);
    extern int call_record_find_by_callerinfo_and_calleeid(char* caller_id, char* caller_ip, int caller_port, char* callee_id, int stream_type);
    extern int call_record_find_by_caller_id(char* caller_id);
    extern int call_record_find_by_caller_index(int caller_ua_index);
    extern int call_record_find_by_task_id(char* task_id);
    extern void call_record_free(cr_t* call_record);
    extern void call_record_free2(cr_t* call_record);
    extern cr_t* call_record_get(int pos);
    extern int call_record_init(cr_t** call_record);
    extern void call_record_list_free();
    extern int call_record_list_init();
    extern int call_record_list_lock();
    extern int call_record_list_unlock();
    extern int call_record_remove(int pos);
    extern int call_record_set_call_status(int pos, call_status_t call_status);
    extern int check_sipua_is_in_call_record_list(int sipua_index, vector<int>& CRSIPUAIndexVector);
    extern int cms_send_invite_to_callee_for_dc_proc(cr_t* pCurrentCrData);
    extern int cms_send_invite_to_caller_for_dc_proc(cr_t* pCurrentCrData, cr_t* pCalleeCrData);
    extern int connect_tv_service_proc(cr_t* pCrData);
    extern int CrDateIPAddrListClone(const osip_list_t* src, osip_list_t* dst);
    extern char* cr_tsu_ip_get_by_ethname(cr_t* pCrData, char* eth_name);
    extern char* cr_tsu_ip_get_by_type(cr_t* pCrData, ip_addr_type_t ip_type);
    extern int dc_call_record_find_by_caller_info(char* caller_id, char* caller_ip, int caller_port);
    extern int debug_call_record_list_lock(const char* file, int line, const char* func);
    extern int debug_call_record_list_unlock(const char* file, int line, const char* func);
    extern int debug_transfer_xml_msg_list_lock(const char* file, int line, const char* func);
    extern int debug_transfer_xml_msg_list_unlock(const char* file, int line, const char* func);
    extern int DelSDPMediaAttributeByName(sdp_message_t* sdp, char* attribute_name);
    extern int DelSDPTimeRepeatInfo(sdp_message_t* sdp);
    extern int find_GBLogic_device_has_realplay_service(char* device_id, int stream_type);
    extern int find_GBLogic_device_has_realplay_service2(char* device_id, int stream_type);
    extern int find_GBLogic_device_has_record_service(char* device_id, int stream_type);
    extern int find_GBLogic_device_has_video_service(char* device_id, int stream_type);
    extern int find_recordinfo_has_other_cr_data(char* device_id, int stream_type, int record_cr_index);
    extern int get_all_sipua_in_call_record_list(vector<int>& SIPUAIndexVector);
    extern int get_all_sipua_in_gb_device_list(vector<int>& SIPUAIndexVector);
    extern int get_all_sipua_in_route_info_list(vector<int>& SIPUAIndexVector);
    extern char* get_cr_sdp_tsu_ip(cr_t* pCrData, char* eth_name);
    extern int is_GBLogic_device_has_other_service(char* device_id, int stream_type, int cr_pos);
    extern int ModifySDPIPAndPort(sdp_message_t* sdp, char* ip_addr, int port);
    extern int ModifySDPRecordPlayTime(sdp_message_t* sdp);
    extern int ModifySDPSName(sdp_message_t* sdp, call_type_t call_type);
    extern int ModifySDPTransProtocol(sdp_message_t* sdp, int protocol);
    extern int record_call_record_find_by_calleeid_and_streamtype(char* callee_id, int stream_type);
    extern int resumed_wait_answer_call_record1(cr_t* pCalleeCrData);
    extern int resumed_wait_answer_call_record2(cr_t* pCalleeCrData, int record_type);
    extern int return_error_for_wait_answer_call_record(cr_t* pCalleeCrData, int iErroCode);
    extern void scan_ack_send_list(DBOper* pAck_dboper);
    extern int scan_call_record_list();
    extern int scan_call_record_list_for_sipua();
    extern int scan_call_record_list_for_wait_expire();
    extern void scan_transfer_message_list();
    extern int SendInfoToCalleeByTaskID(char* pcTsuID, int iMsgType, char* pcTaskID);
    extern int send_bye_to_all_other_caller_by_callee_id(char* callee_id, int cr_pos);
    extern int send_bye_to_all_other_caller_by_callee_id_and_streamtype(char* callee_id, int stream_type, int cr_pos);
    extern int set_call_record_tsu_expire(int pos);
    extern void ShowCallTask(int sock, call_type_t call_type, int streamp_type);
    extern void ShowCallTaskDetail(int sock, int cr_index);
    extern int start_connect_tv_proc(char* device_id, int stream_type, char* dc_id, int dc_media_port);
    extern int start_connect_tv_route_proc(GBLogicDevice_info_t* pSourceGBLogicDeviceInfo, route_info_t* pSourceRouteInfo, GBLogicDevice_info_t* pDCGBLogicDeviceInfo, GBDevice_info_t* pDCGBDeviceInfo, int stream_type, int dc_media_port);
    extern int start_connect_tv_sub_proc(GBLogicDevice_info_t* pSourceGBLogicDeviceInfo, GBDevice_info_t* pSourceGBDeviceInfo, GBLogicDevice_info_t* pDCGBLogicDeviceInfo, GBDevice_info_t* pDCGBDeviceInfo, int stream_type, int dc_media_port);
    extern int StopAllServiceTask(int sock);
    extern int StopAllServiceTaskByCalleeIPAndPort(char* callee_ip, int callee_port);
    extern int StopAllServiceTaskByCalleeUAIndex(int callee_ua_index);
    extern int StopAllServiceTaskByCallerID(char* caller_id);
    extern int StopAllServiceTaskByCallerIDForTVWall(char* caller_id);
    extern int StopAllServiceTaskByCallerIPAndPort(char* caller_ip, int caller_port);
    extern int StopAllServiceTaskByCallerUAIndex(int caller_ua_index);
    extern int StopAllServiceTaskByLogicDeviceID(char* logic_device_id);
    extern int StopAllServiceTaskByLogicDeviceIDAndStreamType(char* device_id, int stream_type);
    extern int StopAllServiceTaskByTaskID(char* task_id);
    extern int StopAllServiceTaskByTSUID(char* tsu_id);
    extern int StopAllServiceTaskByUserInfo(char* user_id, char* login_ip, int login_port);
    extern int StopAllServiceTaskWhenExit();
    extern int StopAudioCallService(cr_t* pCrData);
    extern int StopAudioServiceTaskByLogicDeviceID(char* logic_device_id);
    extern int StopAudioServiceTaskByTSUID(char* tsu_id);
    extern int StopCallService(cr_t* pCrData);
    extern int StopCallTask(int sock, int cr_index);
    extern int StopRecordPlayServiceByTaskID(char* pcTsuID, int iTaskType, char* pcTaskID);
    extern int StopRecordServiceTaskByTSUID(char* tsu_id);
    extern int transfer_message_by_xml_sn(xml_type_t msg_type, char* device_id, unsigned int transfer_xml_sn, int iSumNum, int iListNum, CPacket& inPacket);
    extern int transfer_xml_message_to_dest(int xml_pos, int iSumNum, int iListNum, CPacket& inPacket);
    extern int transfer_xml_message_to_dest2(int xml_pos, CPacket& inPacket);
    extern int transfer_xml_msg_add(xml_type_t msg_type, unsigned int old_xml_sn, unsigned int transfer_xml_sn, char* source_id, char* source_ip, int source_port, char* local_ip, int local_port, char* device_id);
    extern int transfer_xml_msg_count_get(xml_type_t msg_type, char* device_id, unsigned int transfer_xml_sn, int* msg_pos);
    extern int transfer_xml_msg_find(xml_type_t msg_type, char* device_id, unsigned int transfer_xml_sn);
    extern void transfer_xml_msg_free(transfer_xml_msg_t* transfer_xml_msg);
    extern transfer_xml_msg_t* transfer_xml_msg_get(int pos);
    extern int transfer_xml_msg_init(transfer_xml_msg_t** transfer_xml_msg);
    extern void transfer_xml_msg_list_clean();
    extern void transfer_xml_msg_list_free();
    extern int transfer_xml_msg_list_init();
    extern int transfer_xml_msg_list_lock();
    extern int transfer_xml_msg_list_unlock();
    extern int transfer_xml_msg_remove(int pos);

#ifdef ENABLE_DEBUG_LOCK
#define TRANSFER_XML_SMUTEX_LOCK() debug_transfer_xml_msg_list_lock(__FILE__, __LINE__, __func__)
#define TRANSFER_XML_SMUTEX_UNLOCK() debug_transfer_xml_msg_list_unlock(__FILE__, __LINE__, __func__)
#else
#define TRANSFER_XML_SMUTEX_LOCK() transfer_xml_msg_list_lock()
#define TRANSFER_XML_SMUTEX_UNLOCK() transfer_xml_msg_list_unlock()
#endif

#ifdef ENABLE_DEBUG_LOCK
#define CR_SMUTEX_LOCK() debug_call_record_list_lock(__FILE__, __LINE__, __func__)
#define CR_SMUTEX_UNLOCK() debug_call_record_list_unlock(__FILE__, __LINE__, __func__)
#else
#define CR_SMUTEX_LOCK() call_record_list_lock()
#define CR_SMUTEX_UNLOCK() call_record_list_unlock()
#endif

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif /* __UDP_TL_H__ */