#ifndef __USER_SRV_PROC_H__
#define __USER_SRV_PROC_H__


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

#include "libsip.h"
#include "libxml/Packet.h"

#include "common/gbldef.inc"
#include "common/DBOper.h"

#include "common/gblconfig_proc.inc"
#include "device/device_info_mgn.inc"
#include "record/record_info_mgn.inc"
#include "user/user_info_mgn.inc"
#include "user/user_thread_proc.inc"
#include "service/call_func_proc.inc"

#include <vector>
#include <string>
#include <queue>

using namespace std;

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

    /* 需要回应的用户Message 消息结构体 */
    typedef struct _user_response_msg_t
    {
        xml_type_t msg_type;                            /* XML 消息类型 */
        char message_sn[MAX_32CHAR_STRING_LEN + 4];     /* 消息序列号 */
        user_info_t* pUserInfo;                         /* 用户信息 */
        char source_id[MAX_ID_LEN + 4];                 /* 源设备统一编号 */
        char dc_id[MAX_ID_LEN + 4];                     /* DEC 设备统一编号 */
        int dialog_index;                               /* 呼叫Dialog 索引*/
    } user_response_msg_t;

    /* 需要回应的用户Message 消息队列 */
    typedef struct _user_response_msg_list_t
    {
        osip_list_t* pResponseMsgList;
#ifdef MULTI_THR
        osip_mutex_t* lock;
#endif
    } user_response_msg_list_t;

    extern int AddAlarmRecordToXMLItem(CPacket* pOutPacket, DOMElement* ListAccNode, unsigned int uID, string& strDeviceID, int iAlarmType, int iAlarmLevel, int iStartTime, int iStopTime, string& strInfo);
    extern int AddCruiseActionConfigToXMLItem(CPacket* pOutPacket, DOMElement* ListAccNode, int id, int iCruiseID, unsigned int uDeviceIndex, int iPresetID, int iLiveTime, int iSortID);
    extern int AddCruiseConfigToXMLItem(CPacket* pOutPacket, DOMElement* ListAccNode, int id, string& strCruiseName, unsigned int uDeviceIndex, int iScheduledRun, int iStartTime, int iDurationTime, int iResved1);
    extern int AddDeviceGroupConfigToXMLItem(CPacket* pOutPacket, DOMElement* ListAccNode, int id, string& strGroupID, string& strCMSID, string& strName, int SortID, string& strParentID);
    extern int AddDeviceMapGroupConfigToXMLItem(CPacket* pOutPacket, DOMElement* ListAccNode, int id, string& strGroupID, unsigned int DeviceIndex, string& strCMSID, int SortID);
    extern int AddLableRecordConfigToXMLItem(CPacket* pOutPacket, DOMElement* ListAccNode, unsigned int uID, unsigned int uLabelTime, string& strInfo, int iType);
    extern int AddOnlineUserConfigToXMLItem(CPacket* pOutPacket, DOMElement* ListAccNode, unsigned int uUserIndex, string& strUserName, string& strRealName, string& strTel, string& strResved2, string& strLoginIP, int iPort);
    extern int AddPlanActionConfigToXMLItem(CPacket* pOutPacket, DOMElement* ListAccNode, int id, int iPlanID, int iType, unsigned int uDeviceIndex, unsigned int uDestID, unsigned int uScreenID, unsigned int uControlData, unsigned int uStreamType);
    extern int AddPlanConfigToXMLItem(CPacket* pOutPacket, DOMElement* ListAccNode, int id, string& strPlanName, int iStartTime, int iScheduledRun, string& strUserID, int iType, int iUserLevel);
    extern int AddPollActionConfigToXMLItem(CPacket* pOutPacket, DOMElement* ListAccNode, int id, int iPollID, int iType, string& strSourceID, string& strDestID, int iScreenID, int iLiveTime, int iDestSortID, int iSourceSortID, int iStreamType);
    extern int AddPollConfigToXMLItem(CPacket* pOutPacket, DOMElement* ListAccNode, int id, string& strPollName, int iScheduledRun, int iStartTime, int iDurationTime, string& strUserID, int iType, int iResved1);
    extern int AddPresetConfigToXMLItem(CPacket* pOutPacket, DOMElement* ListAccNode, int id, unsigned int nDeviceIndex, int nPresetID, string& strPresetName, int nResved1, string& strResved2);
    extern int AddRecordInfoToXMLItem(CPacket* pOutPacket, DOMElement* ListAccNode, VideoRecord& stVideoRecord, char* strDeviceID, char* strDeviceName);
    extern int AddTopologyPhyDeviceConfigToXMLItem(CPacket* pOutPacket, DOMElement* ListAccNode, string& strDeviceID, string& strDeviceName, int iDeviceType, string& strDeviceIP, int iStatus, string& strCMSID, int iLinkType);
    extern int AddUserDeviceGroupConfigToXMLItem(CPacket* pOutPacket, DOMElement* ListAccNode, int id, string& strGroupID, char* strUserIndex, string& strName, int SortID, string& strParentID);
    extern int AddUserDeviceMapGroupConfigToXMLItem(CPacket* pOutPacket, DOMElement* ListAccNode, int id, string& strGroupID, unsigned int DeviceIndex, int SortID);
    extern int AddWebInterfaceConfigToXMLItem(CPacket* pOutPacket, DOMElement* ListAccNode, int id, int iWebStytle, string& strURL, string& strServerIP, int iPort);
    extern int CreateAlarmRecordResponseXMLHead(CPacket** pOutPacket, int query_count, int record_count, char* strSN, char* strDeviceID, DOMElement** ListAccNode);
    extern int CreateCruiseActionConfigResponseXMLHead(CPacket** pOutPacket, int query_count, int record_count, char* strSN, char* strUserID, DOMElement** ListAccNode);
    extern int CreateCruiseConfigResponseXMLHead(CPacket** pOutPacket, int query_count, int record_count, char* strSN, char* strUserID, DOMElement** ListAccNode);
    extern int CreateDeviceGroupConfigResponseXMLHead(CPacket** pOutPacket, int query_count, int record_count, char* strSN, char* strDeviceID, DOMElement** ListAccNode);
    extern int CreateDeviceMapGroupConfigResponseXMLHead(CPacket** pOutPacket, int query_count, int record_count, char* strSN, char* strDeviceID, DOMElement** ListAccNode);
    extern int CreateLableRecordConfigResponseXMLHead(CPacket** pOutPacket, int query_count, int record_count, char* strSN, char* strDeviceID, DOMElement** ListAccNode);
    extern int CreateOnlineUserConfigResponseXMLHead(CPacket** pOutPacket, int query_count, int record_count, char* strSN, DOMElement** ListAccNode);
    extern int CreatePlanActionConfigResponseXMLHead(CPacket** pOutPacket, int query_count, int record_count, char* strSN, char* strUserID, DOMElement** ListAccNode);
    extern int CreatePlanConfigResponseXMLHead(CPacket** pOutPacket, int query_count, int record_count, char* strSN, char* strUserID, DOMElement** ListAccNode);
    extern int CreatePollActionConfigResponseXMLHead(CPacket** pOutPacket, int query_count, int record_count, char* strSN, char* strUserID, DOMElement** ListAccNode);
    extern int CreatePollConfigResponseXMLHead(CPacket** pOutPacket, int query_count, int record_count, char* strSN, char* strUserID, DOMElement** ListAccNode);
    extern int CreatePresetConfigResponseXMLHead(CPacket** pOutPacket, int query_count, int record_count, char* strSN, char* strDeviceID, DOMElement** ListAccNode);
    extern int CreateRecordInfoQueryResponseXMLHead(CPacket** pOutPacket, int query_count, int record_count, char* strSN, char* strDeviceID, char* strDeviceName, DOMElement** ListAccNode);
    extern int CreateTopologyPhyDeviceConfigResponseXMLHead(CPacket** pOutPacket, int query_count, int record_count, char* strSN, char* strDeviceID, DOMElement** ListAccNode);
    extern int CreateUserDeviceGroupConfigResponseXMLHead(CPacket** pOutPacket, int query_count, int record_count, char* strSN, char* strUserIndex, DOMElement** ListAccNode);
    extern int CreateUserDeviceMapGroupConfigResponseXMLHead(CPacket** pOutPacket, int query_count, int record_count, char* strSN, char* strUserIndex, DOMElement** ListAccNode);
    extern int CreateWebInterfaceConfigResponseXMLHead(CPacket** pOutPacket, int query_count, int record_count, char* strSN, char* strUserID, DOMElement** ListAccNode);
    extern int debug_user_response_msg_list_lock(const char* file, int line, const char* func);
    extern int debug_user_response_msg_list_unlock(const char* file, int line, const char* func);
    extern int preset_record(CPacket& inPacket, unsigned int device_index, DBOper* pdb);
    extern int scan_user_srv_msg_list(user_srv_proc_tl_t* run);
    extern int SendConnectTVResponseToUser(int iResult, int dialog_index);
    extern int SendConnectTVResponseToUserByDeviceIDAndDECID(int iResult, char* source_id, char* dc_id);
    extern int update_user_response_msg_dialog_index(xml_type_t msg_type, char* source_id, char* dc_id, int dialog_index);
    extern int UserLockDeviceProc(char* strLockCmd, char* strDeviceID, user_info_t* pUserInfo);
    extern int UserTCPLoginParseClientData(char* user_ip, int user_port, int user_sock, char* buf, int len, DBOper* pDbOper);
    extern int user_ack_msg_proc(int cr_pos, int ua_dialog_index);
    extern int user_add_label_record_config_proc(user_info_t* pUserInfo, char* caller_id, char* callee_id, CPacket& inPacket, DBOper* pUser_Srv_dboper);
    extern int user_add_poll_config_proc(user_info_t* pUserInfo, char* caller_id, char* callee_id, CPacket& inPacket, DBOper* pUser_Srv_dboper);
    extern int user_answer_invite_for_playback_proc(cr_t* pCrData, int record_type, int start_time, int end_time, int play_time);
    extern int user_answer_invite_for_realplay_proc(cr_t* pCurrentCrData, cr_t* pCalleeCrData, int service_type, int stream_type);
    extern int user_bye_audio_msg_proc(int cr_pos, int ua_dialog_index, cr_t* pCrData, GBLogicDevice_info_t* pCalleeGBLogicDeviceInfo);
    extern int user_bye_msg_proc(int cr_pos, int ua_dialog_index);
    extern int user_bye_response_msg_proc(int cr_pos, int ua_dialog_index, int response_code);
    extern int user_bye_route_video_msg_proc(int cr_pos, int ua_dialog_index, cr_t* pCrData, GBLogicDevice_info_t* pCalleeGBLogicDeviceInfo);
    extern int user_bye_sub_video_msg_proc(int cr_pos, int ua_dialog_index, cr_t* pCrData, GBLogicDevice_info_t* pCalleeGBLogicDeviceInfo);
    extern int user_bye_video_msg_proc(int cr_pos, int ua_dialog_index, cr_t* pCrData, GBLogicDevice_info_t* pCalleeGBLogicDeviceInfo);
    extern int user_cancel_msg_proc(int cr_pos, int ua_dialog_index);
    extern int user_change_password_proc(user_info_t* pUserInfo, char* caller_id, char* callee_id, CPacket& inPacket, DBOper* pUser_Srv_dboper);
    extern int user_confirm_alarm_proc(user_info_t* pUserInfo, char* caller_id, char* callee_id, CPacket& inPacket, DBOper* pUser_Srv_dboper, DBOper* pUserLogDbOper);
    extern int user_control_autozoomin_proc(user_info_t* pUserInfo, char* caller_id, char* callee_id, CPacket& inPacket);
    extern int user_del_label_record_config_proc(user_info_t* pUserInfo, char* caller_id, char* callee_id, CPacket& inPacket, DBOper* pUser_Srv_dboper);
    extern int user_del_poll_config_proc(user_info_t* pUserInfo, char* caller_id, char* callee_id, CPacket& inPacket, DBOper* pUser_Srv_dboper);
    extern int user_device_config_proc(user_info_t* pUserInfo, char* caller_id, char* callee_id, CPacket& inPacket);
    extern int user_device_control_proc(user_info_t* pUserInfo, char* caller_id, char* callee_id, CPacket& inPacket, DBOper* pUser_Srv_dboper, DBOper* pUserLogDbOper);
    extern int user_device_control_response_proc(user_info_t* pUserInfo, char* caller_id, char* callee_id, CPacket& inPacket);
    extern int user_device_info_response_proc(user_info_t* pUserInfo, char* caller_id, char* callee_id, CPacket& inPacket);
    extern int user_device_status_response_proc(user_info_t* pUserInfo, char* caller_id, char* callee_id, CPacket& inPacket);
    extern int user_get_db_ip_proc(user_info_t* pUserInfo, char* caller_id, char* callee_id, CPacket& inPacket);
    extern int user_get_service_id_proc(char* caller_id, char* caller_ip, int caller_port, char* callee_id, char* local_ip, int local_port, char* pcSN, char* pcServerIP, char* pcUserName, DBOper* pUser_Srv_dboper);
    extern int user_get_system_info_proc(user_info_t* pUserInfo, char* caller_id, char* callee_id, CPacket& inPacket, DBOper* pUser_Srv_dboper);
    extern int user_info_msg_proc(char* caller_id, char* callee_id, int dialog_index, char* msg_body, int msg_body_len);
    extern int user_invite_audio_msg_proc(user_info_t* pUserInfo, sdp_message_t* pClientSDP, sdp_param_t* pClientSDPParam, GBLogicDevice_info_t* pCalleeGBLogicDeviceInfo, char* caller_id, char* callee_id, int ua_dialog_index);
    extern int user_invite_audio_response_msg_proc(int cr_pos, int ua_dialog_index, cr_t* pCrData, sdp_message_t* pRemoteSDP, GBLogicDevice_info_t* pCalleeGBLogicDeviceInfo);
    extern int user_invite_msg_proc(user_info_t* pUserInfo, char* caller_id, char* callee_id, int ua_dialog_index, char* msg_body, int msg_body_len);
    extern int user_invite_response_msg_proc(int cr_pos, int ua_dialog_index, int response_code, char* reasonphrase, char* msg_body, int msg_body_len);
    extern int user_invite_route_video_msg_proc(user_info_t* pUserInfo, sdp_message_t* pClientSDP, sdp_param_t* pClientSDPParam, GBLogicDevice_info_t* pCalleeGBLogicDeviceInfo, char* caller_id, char* callee_id, int ua_dialog_index);
    extern int user_invite_sub_video_msg_proc(user_info_t* pUserInfo, sdp_message_t* pClientSDP, sdp_param_t* pClientSDPParam, GBLogicDevice_info_t* pCalleeGBLogicDeviceInfo, char* caller_id, char* callee_id, int ua_dialog_index);
    extern int user_invite_video_msg_proc(user_info_t* pUserInfo, sdp_message_t* pClientSDP, sdp_param_t* pClientSDPParam, GBLogicDevice_info_t* pCalleeGBLogicDeviceInfo, char* caller_id, char* callee_id, int ua_dialog_index);
    extern int user_invite_video_response_msg_proc(int cr_pos, int ua_dialog_index, cr_t* pCrData, sdp_message_t* pRemoteSDP, GBLogicDevice_info_t* pCalleeGBLogicDeviceInfo);
    extern int user_invite_video_route_response_msg_proc(int cr_pos, int ua_dialog_index, cr_t* pCrData, sdp_message_t* pRemoteSDP, GBLogicDevice_info_t* pCalleeGBLogicDeviceInfo);
    extern int user_invite_video_sub_response_msg_proc(int cr_pos, int ua_dialog_index, cr_t* pCrData, sdp_message_t* pRemoteSDP, GBLogicDevice_info_t* pCalleeGBLogicDeviceInfo);
    extern int user_message_msg_proc(user_info_t* pUserInfo, char* caller_id, char* callee_id, char* msg_body, int msg_body_len, DBOper* pUser_Srv_dboper, DBOper* pUserLogDbOper);
    extern int user_notify_alarm_proc(user_info_t* pUserInfo, char* caller_id, char* callee_id, CPacket& inPacket);
    extern int user_notify_alarm_response_proc(user_info_t* pUserInfo, char* caller_id, char* callee_id, CPacket& inPacket);
    extern int user_notify_connect_tv_proc(user_info_t* pUserInfo, char* caller_id, char* callee_id, CPacket& inPacket);
    extern int user_notify_execute_cruise_proc(user_info_t* pUserInfo, char* caller_id, char* callee_id, CPacket& inPacket, DBOper* pUser_Srv_dboper);
    extern int user_notify_execute_plan_proc(user_info_t* pUserInfo, char* caller_id, char* callee_id, CPacket& inPacket, DBOper* pUser_Srv_dboper);
    extern int user_notify_execute_poll_proc(user_info_t* pUserInfo, char* caller_id, char* callee_id, CPacket& inPacket, DBOper* pUser_Srv_dboper);
    extern int user_notify_keep_alive_proc(user_info_t* pUserInfo, char* caller_id, char* callee_id, CPacket& inPacket);
    extern int user_notify_send_notify_proc(user_info_t* pUserInfo, char* caller_id, char* callee_id, CPacket& inPacket, DBOper* pUser_Srv_dboper);
    extern int user_notify_shdb_cmd_proc(user_info_t* pUserInfo, char* caller_id, char* callee_id, CPacket& inPacket, DBOper* pUser_Srv_dboper);
    extern int user_notify_switch_alarm_proc(user_info_t* pUserInfo, char* caller_id, char* callee_id, CPacket& inPacket, DBOper* pUser_Srv_dboper);
    extern int user_notify_tv_status_proc(user_info_t* pUserInfo, char* caller_id, char* callee_id, CPacket& inPacket);
    extern int user_query_alarm_record_proc(user_info_t* pUserInfo, char* caller_id, char* callee_id, CPacket& inPacket, DBOper* pUser_Srv_dboper, DBOper* pUserLogDbOper);
    extern int user_query_catalog_proc(user_info_t* pUserInfo, char* caller_id, char* callee_id, CPacket& inPacket, DBOper* pUser_Srv_dboper);
    extern int user_query_catalog_response_proc(user_info_t* pUserInfo, char* caller_id, char* callee_id, CPacket& inPacket);
    extern int user_query_channel_status_proc(user_info_t* pUserInfo, char* caller_id, char* callee_id, CPacket& inPacket, DBOper* pUser_Srv_dboper);
    extern int user_query_cruise_action_config_proc(user_info_t* pUserInfo, char* caller_id, char* callee_id, CPacket& inPacket, DBOper* pUser_Srv_dboper);
    extern int user_query_cruise_config_proc(user_info_t* pUserInfo, char* caller_id, char* callee_id, CPacket& inPacket, DBOper* pUser_Srv_dboper);
    extern int user_query_data_value_proc(user_info_t* pUserInfo, char* caller_id, char* callee_id, CPacket& inPacket, DBOper* pUser_Srv_dboper);
    extern int user_query_device_config_proc(user_info_t* pUserInfo, char* caller_id, char* callee_id, CPacket& inPacket);
    extern int user_query_device_group_config_proc(user_info_t* pUserInfo, char* caller_id, char* callee_id, CPacket& inPacket, DBOper* pUser_Srv_dboper);
    extern int user_query_device_info_proc(user_info_t* pUserInfo, char* caller_id, char* callee_id, CPacket& inPacket, DBOper* pUser_Srv_dboper);
    extern int user_query_device_map_group_config_proc(user_info_t* pUserInfo, char* caller_id, char* callee_id, CPacket& inPacket, DBOper* pUser_Srv_dboper);
    extern int user_query_device_preset_proc(user_info_t* pUserInfo, char* caller_id, char* callee_id, CPacket& inPacket);
    extern int user_query_device_status_proc(user_info_t* pUserInfo, char* caller_id, char* callee_id, CPacket& inPacket);
    extern int user_query_device_video_param_proc(user_info_t* pUserInfo, char* caller_id, char* callee_id, CPacket& inPacket);
    extern int user_query_label_record_proc(user_info_t* pUserInfo, char* caller_id, char* callee_id, CPacket& inPacket, DBOper* pUser_Srv_dboper);
    extern int user_query_online_user_proc(user_info_t* pUserInfo, char* caller_id, char* callee_id, CPacket& inPacket, DBOper* pUser_Srv_dboper);
    extern int user_query_plan_action_config_proc(user_info_t* pUserInfo, char* caller_id, char* callee_id, CPacket& inPacket, DBOper* pUser_Srv_dboper);
    extern int user_query_plan_config_proc(user_info_t* pUserInfo, char* caller_id, char* callee_id, CPacket& inPacket, DBOper* pUser_Srv_dboper);
    extern int user_query_poll_action_config_proc(user_info_t* pUserInfo, char* caller_id, char* callee_id, CPacket& inPacket, DBOper* pUser_Srv_dboper);
    extern int user_query_poll_config_proc(user_info_t* pUserInfo, char* caller_id, char* callee_id, CPacket& inPacket, DBOper* pUser_Srv_dboper);
    extern int user_query_preset_info_proc(user_info_t* pUserInfo, char* caller_id, char* callee_id, CPacket& inPacket, DBOper* pUser_Srv_dboper);
    extern int user_query_record_info_proc(user_info_t* pUserInfo, char* caller_id, char* callee_id, CPacket& inPacket, DBOper* pUser_Srv_dboper);
    extern int user_query_user_config_proc(user_info_t* pUserInfo, char* caller_id, char* callee_id, CPacket& inPacket, DBOper* pUser_Srv_dboper);
    extern int user_query_user_device_group_config_proc(user_info_t* pUserInfo, char* caller_id, char* callee_id, CPacket& inPacket, DBOper* pUser_Srv_dboper);
    extern int user_query_user_device_map_group_config_proc(user_info_t* pUserInfo, char* caller_id, char* callee_id, CPacket& inPacket, DBOper* pUser_Srv_dboper);
    extern int user_query_web_interface_config_proc(user_info_t* pUserInfo, char* caller_id, char* callee_id, CPacket& inPacket, DBOper* pUser_Srv_dboper);
    extern int user_RCU_device_control_proc(user_info_t* pUserInfo, char* caller_id, char* callee_id, CPacket& inPacket, DBOper* pUser_Srv_dboper);
    extern int user_record_info_response_proc(user_info_t* pUserInfo, char* caller_id, char* callee_id, CPacket& inPacket);
    extern int user_request_ifram_data_proc(user_info_t* pUserInfo, char* caller_id, char* callee_id, CPacket& inPacket);
    extern int user_response_msg_add(xml_type_t msg_type, char* message_sn, user_info_t* pUserInfo, char* source_id, char* dc_id, int dialog_index);
    extern int user_response_msg_find(xml_type_t msg_type, int dialog_index);
    extern int user_response_msg_find_by_source_id_and_dc_id(xml_type_t msg_type, char* source_id, char* dc_id);
    extern void user_response_msg_free(user_response_msg_t* user_response_msg);
    extern user_response_msg_t* user_response_msg_get(int pos);
    extern int user_response_msg_init(user_response_msg_t** user_response_msg);
    extern void user_response_msg_list_free();
    extern int user_response_msg_list_init();
    extern int user_response_msg_list_lock();
    extern int user_response_msg_list_unlock();
    extern int user_response_msg_remove(int pos);
    extern int user_set_device_video_param_proc(user_info_t* pUserInfo, char* caller_id, char* callee_id, CPacket& inPacket);
    extern int user_set_device_xy_param_proc(user_info_t* pUserInfo, char* caller_id, char* callee_id, CPacket& inPacket, DBOper* pUser_Srv_dboper);
    extern int user_srv_msg_add(user_srv_proc_tl_t* pUserSrvProcThd, msg_type_t msg_type, char* caller_id, char* callee_id, int response_code, char* reasonphrase, int ua_dialog_index, char* msg_body, int msg_body_len, int cr_pos);
    extern void user_srv_msg_free(user_srv_msg_t* user_srv_msg);
    extern int user_srv_msg_init(user_srv_msg_t** user_srv_msg);
    extern void user_srv_msg_list_clean(user_srv_proc_tl_t* run);
    extern int user_srv_msg_proc(user_info_t* pUserInfo, user_srv_msg_t* pUserSrvMsg, DBOper* pUser_Srv_dboper, DBOper* pUserLogDbOper);
    extern int user_tcp_get_service_id_proc(char* user_ip, int user_port, int user_sock, CPacket& inPacket, DBOper* pDbOper);
    extern int user_tcp_notify_cmd_proc(char* user_ip, int user_port, int user_sock, CPacket& inPacket, DBOper* pDbOper);
    extern int user_tcp_notify_keep_alive_proc(char* user_ip, int user_port, int user_sock, CPacket& inPacket, DBOper* pDbOper);
    extern int user_tcp_notify_tcp_connect_info_proc(char* user_ip, int user_port, int user_sock, CPacket& inPacket, DBOper* pDbOper);
    extern int user_tcp_query_catalog_proc(char* user_ip, int user_port, int user_sock, CPacket& inPacket, DBOper* pDbOper);
    extern int user_tcp_query_device_group_config_proc(char* user_ip, int user_port, int user_sock, CPacket& inPacket, DBOper* pDbOper);
    extern int user_tcp_query_device_map_group_config_proc(char* user_ip, int user_port, int user_sock, CPacket& inPacket, DBOper* pDbOper);
    extern int user_tcp_query_RCU_catalog_proc(char* user_ip, int user_port, int user_sock, CPacket& inPacket, DBOper* pDbOper);
    extern int user_tcp_query_user_config_proc(char* user_ip, int user_port, int user_sock, CPacket& inPacket, DBOper* pDbOper);
    extern int user_tcp_query_user_device_group_config_proc(char* user_ip, int user_port, int user_sock, CPacket& inPacket, DBOper* pDbOper);
    extern int user_tcp_query_user_device_map_group_config_proc(char* user_ip, int user_port, int user_sock, CPacket& inPacket, DBOper* pDbOper);
    extern int user_tcp_query_web_interface_config_proc(char* user_ip, int user_port, int user_sock, CPacket& inPacket, DBOper* pDbOper);
    extern int user_transfer_invite_to_dest_for_audio_proc(cr_t* pCrData, sdp_message_t* pClientSDP);
    extern int user_transfer_invite_to_dest_for_record_play_proc(cr_t* pCrData, sdp_message_t* pClientSDP);
    extern int user_transfer_invite_to_dest_proc(cr_t* pCrData, sdp_message_t* pClientSDP);
    extern int user_video_sevice_current_cms_proc(cr_t* pCrData, sdp_message_t* pClientSDP, call_type_t eCallType, int start_time, int end_time, int play_time, int is_callee_record, int record_type);
    extern int user_video_sevice_sub_cms_proc(cr_t* pCrData, sdp_message_t* pClientSDP, call_type_t eCallType);
    extern int user_wiscomv_proc(user_info_t* pUserInfo, char* caller_id, char* callee_id, char* msg_body, int msg_body_len, DBOper* pUser_Srv_dboper);

#ifdef ENABLE_DEBUG_LOCK
#define USER_RESPMSG_SMUTEX_LOCK() debug_user_response_msg_list_lock(__FILE__, __LINE__, __func__)
#define USER_RESPMSG_SMUTEX_UNLOCK() debug_user_response_msg_list_unlock(__FILE__, __LINE__, __func__)
#else
#define USER_RESPMSG_SMUTEX_LOCK() user_response_msg_list_lock()
#define USER_RESPMSG_SMUTEX_UNLOCK() user_response_msg_list_unlock()
#endif

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif /* __UDP_TL_H__ */
