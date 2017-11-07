#ifndef __ROUTE_SRV_PROC_H__
#define __ROUTE_SRV_PROC_H__


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
#include "route/route_info_mgn.inc"
#include "service/call_func_proc.inc"
#include "service/compress_task_proc.inc"

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

    /* 互联路由业务消息结构体 */
    typedef struct _route_srv_msg_t
    {
        msg_type_t msg_type;                                /* 消息类型 */
        route_info_t* pRouteInfo;                           /* 上级互联路由信息 */
        char caller_id[MAX_ID_LEN + 4];                     /* 主叫用户统一编号 */
        char callee_id[MAX_ID_LEN + 4];                     /* 被叫用户统一编号 */
        int response_code;                                  /* 回应消息码,只有回应消息有 */
        char reasonphrase[MAX_128CHAR_STRING_LEN + 4];      /* 回应的错误信息，只有回应消息有 */
        int ua_dialog_index;                                /* 对应dialog 索引，只有会话类消息有 */
        char msg_body[MAX_MSG_BODY_STRING_LEN + 4];         /* SIP 消息携带的消息体*/
        int msg_body_len;                                   /* SIP 消息携带的消息体长度*/
        int cr_pos;                                         /* 对应的呼叫资源索引信息 */
    } route_srv_msg_t;

    /* 互联路由业务消息队列 */
    typedef std::deque <route_srv_msg_t*> route_srv_msg_queue;

    extern int AddLogicDeviceGroupInfoToXMLItemForRoute(CPacket* pOutPacket, DOMElement* ListAccNode, char* group_code, char* group_name, char* parent_code);
    extern int AddLogicDeviceInfoToXMLItemForRoute(CPacket* pOutPacket, DOMElement* ListAccNode, char* device_id, int iThreePartyFlag, DBOper* pRoute_Srv_dboper);
    extern int AddLogicDeviceInfoToXMLItemForRouteNotify(CPacket* pOutPacket, DOMElement* ListAccNode, char* device_id, int iThreePartyFlag, DBOper* pRoute_Srv_dboper);
    extern int AddRecordInfoToXMLItemForRoute(CPacket* pOutPacket, DOMElement* ListAccNode, VideoRecord& stVideoRecord, char* strDeviceID, char* strDeviceName);
    extern int analysis_return_fileds(char* strReturnFiles, jly_yspb_t* pstYSPB);
    extern int analysis_time_for_compress(char* strTime);
    extern int checkIfHasDBRefresh();
    extern int CreateGBLogicDeviceCatalogNotifyXMLHeadForRoute(CPacket** pOutPacket, int query_count, int record_count, char* strSN, char* strDeviceID, DOMElement** ListAccNode);
    extern int CreateGBLogicDeviceCatalogResponseXMLHeadForRoute(CPacket** pOutPacket, int query_count, int record_count, char* strSN, char* strDeviceID, DOMElement** ListAccNode);
    extern int CreateRecordInfoQueryResponseXMLHeadForRoute(CPacket** pOutPacket, int query_count, int record_count, char* strSN, char* strDeviceID, char* strDeviceName, DOMElement** ListAccNode);
    extern int get_compress_task_by_http(char* platform_ip, int iTaskBeginTime, int iTaskEndTime, DBOper* ptDBoper);
    extern int get_compress_task_from_webservice_proc(char* platform_ip, int iTaskBeginTime, int iTaskEndTime, DBOper* ptDBoper);
    extern int RouteGetGBDeviceListAndSendCataLogTo3PartyCMS(route_info_t* pRouteInfo, char* caller_id, char* strDeviceID, char* strSN, DBOper* pRoute_Srv_dboper);
    extern int RouteGetGBDeviceListAndSendCataLogToCMS(route_info_t* pRouteInfo, char* caller_id, char* strDeviceID, char* strSN, DBOper* pRoute_Srv_dboper);
    extern int RouteGetGBDeviceListAndSendCataLogToOwnerCMS(route_info_t* pRouteInfo, char* caller_id, char* strDeviceID, char* strSN, DBOper* pRoute_Srv_dboper);
    extern int RouteLockDeviceProc(char* strLockCmd, char* strDeviceID, route_info_t* pRouteInfo);
    extern int route_ack_msg_proc(int cr_pos, int ua_dialog_index);
    extern int route_bye_audio_msg_proc(int cr_pos, int ua_dialog_index, cr_t* pCrData, GBLogicDevice_info_t* pCalleeGBLogicDeviceInfo);
    extern int route_bye_msg_proc(int cr_pos, int ua_dialog_index);
    extern int route_bye_response_msg_proc(int cr_pos, int ua_dialog_index, int response_code);
    extern int route_bye_route_video_msg_proc(int cr_pos, int ua_dialog_index, cr_t* pCrData, GBLogicDevice_info_t* pCalleeGBLogicDeviceInfo);
    extern int route_bye_sub_video_msg_proc(int cr_pos, int ua_dialog_index, cr_t* pCrData, GBLogicDevice_info_t* pCalleeGBLogicDeviceInfo);
    extern int route_bye_video_msg_proc(int cr_pos, int ua_dialog_index, cr_t* pCrData, GBLogicDevice_info_t* pCalleeGBLogicDeviceInfo);
    extern int route_cancel_msg_proc(int cr_pos, int ua_dialog_index);
    extern int route_control_autozoomin_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, CPacket& inPacket);
    extern int route_device_config_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, CPacket& inPacket);
    extern int route_device_control_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, CPacket& inPacket, DBOper* pRoute_Srv_dboper);
    extern int route_device_control_response_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, CPacket& inPacket);
    extern int route_device_info_response_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, CPacket& inPacket);
    extern int route_device_status_response_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, CPacket& inPacket);
    extern int route_execute_preset_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, CPacket& inPacket, DBOper* pRoute_Srv_dboper);
    extern int route_get_db_ip_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, CPacket& inPacket);
    extern int route_get_service_id_response_proc(char* caller_id, char* caller_ip, int caller_port, char* callee_id, char* local_ip, int local_port, char* pcSN, char* pcServerID, DBOper* pRoute_Srv_dboper);
    extern int route_info_msg_proc(char* caller_id, char* callee_id, int dialog_index, char* msg_body, int msg_body_len);
    extern int route_invite_audio_msg_proc(route_info_t* pRouteInfo, sdp_message_t* pClientSDP, sdp_param_t* pClientSDPParam, GBLogicDevice_info_t* pCalleeGBLogicDeviceInfo, char* caller_id, char* callee_id, int ua_dialog_index);
    extern int route_invite_audio_response_msg_proc(int cr_pos, int ua_dialog_index, cr_t* pCrData, sdp_message_t* pRemoteSDP, GBLogicDevice_info_t* pCalleeGBLogicDeviceInfo);
    extern int route_invite_msg_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, int ua_dialog_index, char* msg_body, int msg_body_len);
    extern int route_invite_response_msg_proc(int cr_pos, int ua_dialog_index, int response_code, char* reasonphrase, char* msg_body, int msg_body_len);
    extern int route_invite_route_video_msg_proc(route_info_t* pRouteInfo, sdp_message_t* pClientSDP, sdp_param_t* pClientSDPParam, GBLogicDevice_info_t* pCalleeGBLogicDeviceInfo, char* caller_id, char* callee_id, int ua_dialog_index);
    extern int route_invite_sub_video_msg_proc(route_info_t* pRouteInfo, sdp_message_t* pClientSDP, sdp_param_t* pClientSDPParam, GBLogicDevice_info_t* pCalleeGBLogicDeviceInfo, char* caller_id, char* callee_id, int ua_dialog_index);
    extern int route_invite_video_msg_proc(route_info_t* pRouteInfo, sdp_message_t* pClientSDP, sdp_param_t* pClientSDPParam, GBLogicDevice_info_t* pCalleeGBLogicDeviceInfo, char* caller_id, char* callee_id, int ua_dialog_index);
    extern int route_invite_video_response_msg_proc(int cr_pos, int ua_dialog_index, cr_t* pCrData, sdp_message_t* pRemoteSDP, GBLogicDevice_info_t* pCalleeGBLogicDeviceInfo);
    extern int route_invite_video_route_response_msg_proc(int cr_pos, int ua_dialog_index, cr_t* pCrData, sdp_message_t* pRemoteSDP, GBLogicDevice_info_t* pCalleeGBLogicDeviceInfo);
    extern int route_invite_video_sub_response_msg_proc(int cr_pos, int ua_dialog_index, cr_t* pCrData, sdp_message_t* pRemoteSDP, GBLogicDevice_info_t* pCalleeGBLogicDeviceInfo);
    extern int route_message_msg_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, char* msg_body, int msg_body_len, DBOper* pRoute_Srv_dboper);
    extern int route_message_srv_msg_add(route_info_t* pRouteInfo, msg_type_t msg_type, char* caller_id, char* callee_id, int response_code, int ua_dialog_index, char* msg_body, int msg_body_len, int cr_pos);
    extern void route_message_srv_msg_list_clean();
    extern void route_message_srv_msg_list_free();
    extern int route_message_srv_msg_list_init();
    extern int route_notify_alarm_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, CPacket& inPacket, DBOper* pRoute_Srv_dboper);
    extern int route_notify_alarm_response_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, CPacket& inPacket);
    extern int route_notify_catalog_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, CPacket& inPacket, DBOper* pRoute_Srv_dboper);
    extern int route_notify_cms_restart_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, CPacket& inPacket);
    extern int route_notify_keep_alive_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, CPacket& inPacket);
    extern int route_notify_msg_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, char* msg_body, int msg_body_len, DBOper* pRoute_Srv_dboper);
    extern int route_notify_status_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, CPacket& inPacket, DBOper* pRoute_Srv_dboper);
    extern int route_notify_tv_status_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, CPacket& inPacket);
    extern int route_preset_info_response_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, CPacket& inPacket);
    extern int route_query_catalog_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, CPacket& inPacket, DBOper* pRoute_Srv_dboper);
    extern int route_query_catalog_response_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, CPacket& inPacket, DBOper* pRoute_Srv_dboper);
    extern int route_query_device_config_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, CPacket& inPacket);
    extern int route_query_device_group_config_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, CPacket& inPacket, DBOper* pRoute_Srv_dboper);
    extern int route_query_device_info_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, CPacket& inPacket, DBOper* pRoute_Srv_dboper);
    extern int route_query_device_map_group_config_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, CPacket& inPacket, DBOper* pRoute_Srv_dboper);
    extern int route_query_device_preset_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, CPacket& inPacket);
    extern int route_query_device_status_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, CPacket& inPacket);
    extern int route_query_device_video_param_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, CPacket& inPacket);
    extern int route_query_preset_info_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, CPacket& inPacket, DBOper* pRoute_Srv_dboper);
    extern int route_query_record_info_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, CPacket& inPacket, DBOper* pRoute_Srv_dboper);
    extern int route_query_topology_phydevice_config_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, CPacket& inPacket, DBOper* pRoute_Srv_dboper);
    extern int route_record_info_response_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, CPacket& inPacket);
    extern int route_request_ifram_data_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, CPacket& inPacket);
    extern int route_set_device_video_param_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, CPacket& inPacket);
    extern int route_set_device_xy_param_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, CPacket& inPacket, DBOper* pRoute_Srv_dboper);
    extern int route_srv_msg_add(route_info_t* pRouteInfo, msg_type_t msg_type, char* caller_id, char* callee_id, int response_code, char* reasonphrase, int ua_dialog_index, char* msg_body, int msg_body_len, int cr_pos);
    extern void route_srv_msg_free(route_srv_msg_t* route_srv_msg);
    extern int route_srv_msg_init(route_srv_msg_t** route_srv_msg);
    extern void route_srv_msg_list_clean();
    extern void route_srv_msg_list_free();
    extern int route_srv_msg_list_init();
    extern int route_srv_msg_proc(route_srv_msg_t* pRouteSrvMsg, DBOper* pRoute_Srv_dboper);
    extern int route_subscribe_msg_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, int event_id, int subscribe_expires, char* msg_body, int msg_body_len, DBOper* pRoute_Srv_dboper);
    extern int route_subscribe_query_catalog_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, int event_id, int subscribe_expires, CPacket& inPacket, DBOper* pRoute_Srv_dboper);
    extern int route_subscribe_within_dialog_msg_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, int dialog_index, int subscribe_expires, char* msg_body, int msg_body_len, DBOper* pRoute_Srv_dboper);
    extern int route_subscribe_witin_dialog_query_catalog_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, int dialog_index, int subscribe_expires, CPacket& inPacket, DBOper* pRoute_Srv_dboper);
    extern void scan_route_message_srv_msg_list(DBOper* pRoute_Srv_dboper);
    extern void scan_route_srv_msg_list(DBOper* pRoute_Srv_dboper);
    extern int StopRouteService(route_info_t* pRouteInfo, char * device_id, int stream_type);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif /* __UDP_TL_H__ */
