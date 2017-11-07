#ifndef __DEVICE_SRV_PROC_H__
#define __DEVICE_SRV_PROC_H__


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

#include <vector>
#include <string>
#include <queue>
#include "device/device_info_mgn.inc"
#include "service/call_func_proc.inc"
#include "resource/resource_info_mgn.inc"

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

    /* 智能分析，目标计数结果消息结构体 */
    typedef struct _analysis_target_count_msg_t
    {
        char strSN[32];                                   /* 消息序列号 */
        char strDeviceID[MAX_ID_LEN + 4];                 /* 诊断设备编码 */
        char strChannelID[MAX_ID_LEN + 4];                /* 视频通道编号 */

        char strAreaIndex[32];                            /* 分析区域 */
        char strAreaName[32];                             /* 分析区域名称 */
        char strTargetCountTime[32];                      /* 当前时间 */

        char strEnterPeople[32];                          /* 进入分析区域人数（必选） */
        char strEnterCar[32];                             /* 进入分析区域车辆数（必选） */
        char strLeavePeople[32];                          /* 离开分析区域人数（必选） */
        char strLeaveCar[32];                             /* 离开分析区域车辆数（必选） */
    } analysis_target_count_msg_t;

    /* 智能分析，目标计数结果结果消息队列 */
    typedef std::deque <analysis_target_count_msg_t*> analysis_target_count_msg_queue;

    /* 诊断结果消息结构体 */
    typedef struct _diagnosis_msg_t
    {
        char strSN[32];                                   /* 消息序列号 */
        char strDeviceID[MAX_ID_LEN + 4];                 /* 设备统一编号 */
        char strDeviceIP[16];                             /* 设备IP地址 */
        char strChannelID[MAX_ID_LEN + 4];                /* 视频通道编号 */
        char strDiagnosisTime[32];                        /* 诊断时间 */
        char strImageFile[256];                           /* 抓图存放名称 */
        char strVideoLost[32];                            /* 视频丢失 */
        char strVideoOcclusion[32];                       /* 镜头遮挡 */
        char strVideoFrozen[32];                          /* 画面冻结 */
        char strVideoLum[32];                             /* 亮度异常 */
        char strVideoShake[32];                           /* 镜头抖动 */
        char strVideoSnow[32];                            /* 雪花噪声 */
        char strVideoBlur[32];                            /* 镜头模糊 */
        char strVideoHue[32];                             /* 画面偏色 */
        char strVideoNightMode[32];                       /* 黑白图像 */
        char strVideoLumLow[32];                          /* 图像过暗 */
        char strVideoContrastLow[32];                     /* 低对比度 */
        char strVideoUpheaval[32];                        /* 视频剧变 */
        char strVideoMosaic[32];                          /* 马赛克 */
        char strVideoStripe[32];                          /* 条纹干扰 */
        char strVideoPTZ[32];                             /* PTZ异常 */
        char strVideoSceneChange[32];                     /* 场景变更 */
        char strVideoNetWork[32];                         /* 网络异常 */
        char strVideoLogIn[32];                           /* 登录失败 */
        char strVideoNoStream[32];                        /* 取流失败 */
    } diagnosis_msg_t;

    /* 诊断结果消息队列 */
    typedef std::deque <diagnosis_msg_t*> diagnosis_msg_queue;

    /* 标准设备业务消息结构体 */
    typedef struct _device_srv_msg_t
    {
        msg_type_t msg_type;                                /* 消息类型 */
        GBDevice_info_t* pGBDeviceInfo;                     /* 物理设备信息 */
        char caller_id[MAX_ID_LEN + 4];                     /* 主叫用户统一编号 */
        char callee_id[MAX_ID_LEN + 4];                     /* 被叫用户统一编号 */
        int response_code;                                  /* 回应消息码,只有回应消息有 */
        char reasonphrase[MAX_128CHAR_STRING_LEN + 4];      /* 回应的错误信息，只有回应消息有 */
        int ua_dialog_index;                                /* 对应dialog 索引，只有会话类消息有 */
        char msg_body[MAX_MSG_BODY_STRING_LEN + 4];         /* SIP 消息携带的消息体*/
        int msg_body_len;                                   /* SIP 消息携带的消息体长度*/
        int cr_pos;                                         /* 对应的呼叫资源索引信息 */
    } device_srv_msg_t;

    /* 标准设备业务消息队列 */
    typedef std::deque <device_srv_msg_t*> device_srv_msg_queue;

    extern int AddCompressTaskInfoToXMLItem(CPacket* pOutPacket, DOMElement* ListAccNode, char* task_id);
    extern int AddLogicDeviceInfoToXMLItemForDevice(CPacket * pOutPacket, DOMElement * ListAccNode, char * device_id, DBOper * pRoute_Srv_dboper);
    extern int AddTopologyPhyDeviceInfo2DB(char * strDeviceID, char * strDeviceName, char * strDeviceType, char * strDeviceIP, char * strStatus, char * strCMSID, char * strLinkType, DBOper * pDevice_Srv_dboper);
    extern int add_compress_task_for_test(char* platform_ip, DBOper* ptDBoper);
    extern int analysis_target_count_msg_add(analysis_target_count_msg_t* pAnalysisTargetCountMsg);
    extern void analysis_target_count_msg_free(analysis_target_count_msg_t* analysis_target_count_msg);
    extern int analysis_target_count_msg_init(analysis_target_count_msg_t** analysis_target_count_msg);
    extern void analysis_target_count_msg_list_clean();
    extern void analysis_target_count_msg_list_free();
    extern int analysis_target_count_msg_list_init();
    extern int analysis_target_count_msg_proc(analysis_target_count_msg_t* pAnalysisTargetCountMsg, DBOper* pDboper);
    extern int assign_compress_task_to_zrv_device_proc(char* platform_ip, DBOper* ptDBoper, int* run_thread_time);
    extern int check_need_to_re_assign(char* platform_ip, int per_task_count_zrvdevice, vector<string>& ZRVDeviceIP, vector<string>& DestZRVDeviceIP, vector<int>& DestZRVDeviceTaskCount, int* remove_task_count);
    extern int CreateAssignCompressTaskXMLHead(CPacket** pOutPacket, int query_count, int record_count, DOMElement** ListAccNode);
    extern int CreateGBLogicDeviceCatalogResponseXMLHeadForDevice(CPacket** pOutPacket, int query_count, int record_count, char * strSN, char * strDeviceID, DOMElement** ListAccNode);
    extern int DeleteLogicDeviceGroupConfigByCMSID(char * cms_id, DBOper * pDevice_Srv_dboper);
    extern int DeleteLogicDeviceMapGroupConfigByCMSID(char * cms_id, DBOper * pDevice_Srv_dboper);
    extern int DeleteTopologyPhyDeviceInfoFromDB(char * device_id, DBOper * pDevice_Srv_dboper);
    extern int device_ack_msg_proc(int cr_pos, int ua_dialog_index);
    extern int device_answer_dc_invite_response_proc(cr_t * pCrData, int response_code, char * msg_body, int msg_body_len);
    extern int device_answer_source_invite_response_for_dc_proc(cr_t * pCrData, int response_code, char * msg_body, int msg_body_len);
    extern int device_bye_audio_msg_proc(int cr_pos, int ua_dialog_index, cr_t* pCrData, GBLogicDevice_info_t* pCalleeGBLogicDeviceInfo);
    extern int device_bye_msg_proc(int cr_pos, int ua_dialog_index);
    extern int device_bye_response_msg_proc(int cr_pos, int ua_dialog_index, int response_code);
    extern int device_bye_route_video_msg_proc(int cr_pos, int ua_dialog_index, cr_t* pCrData, GBLogicDevice_info_t* pCalleeGBLogicDeviceInfo);
    extern int device_bye_sub_video_msg_proc(int cr_pos, int ua_dialog_index, cr_t* pCrData, GBLogicDevice_info_t* pCalleeGBLogicDeviceInfo);
    extern int device_bye_video_msg_proc(int cr_pos, int ua_dialog_index, cr_t* pCrData, GBLogicDevice_info_t* pCalleeGBLogicDeviceInfo);
    extern int device_cancel_msg_proc(int cr_pos, int ua_dialog_index);
    extern int device_channel_status_response_proc(GBDevice_info_t * pGBDeviceInfo, char * caller_id, char * callee_id, CPacket & inPacket, DBOper * pDevice_Srv_dboper);
    extern int device_data_value_response_proc(GBDevice_info_t * pGBDeviceInfo, char * caller_id, char * callee_id, CPacket & inPacket, DBOper * pDevice_Srv_dboper);
    extern int device_device_config_response_proc(GBDevice_info_t * pGBDeviceInfo, char * caller_id, char * callee_id, CPacket & inPacket);
    extern int device_device_control_proc(GBDevice_info_t* pCallerGBDeviceInfo, char* caller_id, char* callee_id, CPacket& inPacket, DBOper* pDevice_Srv_dboper);
    extern int device_device_control_response_proc(GBDevice_info_t * pGBDeviceInfo, char * caller_id, char * callee_id, CPacket & inPacket);
    extern int device_device_info_response_proc(GBDevice_info_t * pGBDeviceInfo, char * caller_id, char * callee_id, CPacket & inPacket, DBOper * pDevice_Srv_dboper);
    extern int device_device_status_response_proc(GBDevice_info_t * pGBDeviceInfo, char * caller_id, char * callee_id, CPacket & inPacket, DBOper * pDevice_Srv_dboper);
    extern int device_get_dbip_response_proc(GBDevice_info_t * pGBDeviceInfo, char * caller_id, char * callee_id, CPacket & inPacket, DBOper * pDevice_Srv_dboper);
    extern int device_get_device_video_param_response_proc(GBDevice_info_t * pGBDeviceInfo, char * caller_id, char * callee_id, CPacket & inPacket);
    extern int device_get_service_id_proc(char * caller_id, char * caller_ip, int caller_port, char * callee_id, char * local_ip, int local_port, char * pcSN, char * pcServerIP, DBOper * pDevice_Srv_dboper);
    extern int device_info_msg_proc(char* caller_id, char* callee_id, int dialog_index, char* msg_body, int msg_body_len);
    extern int device_invite_audio_msg_proc(GBDevice_info_t* pGBDeviceInfo, sdp_message_t* pClientSDP, sdp_param_t* pClientSDPParam, GBLogicDevice_info_t* pCalleeGBLogicDeviceInfo, char* caller_id, char* callee_id, int ua_dialog_index);
    extern int device_invite_audio_response_msg_proc(int cr_pos, int ua_dialog_index, cr_t* pCrData, sdp_message_t* pRemoteSDP, GBLogicDevice_info_t* pCalleeGBLogicDeviceInfo);
    extern int device_invite_dc_response_msg_proc(int cr_pos, int ua_dialog_index, int response_code, cr_t* pCrData, char* msg_body, int msg_body_len);
    extern int device_invite_msg_proc(GBDevice_info_t* pGBDeviceInfo, char* caller_id, char* callee_id, int ua_dialog_index, char* msg_body, int msg_body_len);
    extern int device_invite_response_msg_proc(int cr_pos, int ua_dialog_index, int response_code, char* reasonphrase, char* msg_body, int msg_body_len);
    extern int device_invite_route_video_msg_proc(GBDevice_info_t* pGBDeviceInfo, sdp_message_t* pClientSDP, sdp_param_t* pClientSDPParam, GBLogicDevice_info_t* pCalleeGBLogicDeviceInfo, char* caller_id, char* callee_id, int ua_dialog_index);
    extern int device_invite_sub_video_msg_proc(GBDevice_info_t* pGBDeviceInfo, sdp_message_t* pClientSDP, sdp_param_t* pClientSDPParam, GBLogicDevice_info_t* pCalleeGBLogicDeviceInfo, char* caller_id, char* callee_id, int ua_dialog_index);
    extern int device_invite_video_msg_proc(GBDevice_info_t* pGBDeviceInfo, sdp_message_t* pClientSDP, sdp_param_t* pClientSDPParam, GBLogicDevice_info_t* pCalleeGBLogicDeviceInfo, char* caller_id, char* callee_id, int ua_dialog_index);
    extern int device_invite_video_response_msg_proc(int cr_pos, int ua_dialog_index, cr_t* pCrData, sdp_message_t* pRemoteSDP, GBLogicDevice_info_t* pCalleeGBLogicDeviceInfo);
    extern int device_invite_video_route_response_msg_proc(int cr_pos, int ua_dialog_index, cr_t* pCrData, sdp_message_t* pRemoteSDP, GBLogicDevice_info_t* pCalleeGBLogicDeviceInfo);
    extern int device_invite_video_sub_response_msg_proc(int cr_pos, int ua_dialog_index, cr_t* pCrData, sdp_message_t* pRemoteSDP, GBLogicDevice_info_t* pCalleeGBLogicDeviceInfo);
    extern int device_message_msg_proc(GBDevice_info_t* pGBDeviceInfo, char* caller_id, char* callee_id, int ua_dialog_index, char* msg_body, int msg_body_len, DBOper* pDevice_Srv_dboper);
    extern int device_message_request_proc(char* device_ip, int device_port, int device_sock, CPacket& inPacket, DBOper* pDbOper);
    extern int device_message_response_proc(char* device_ip, int device_port, int device_sock, CPacket& inPacket, DBOper* pDbOper);
    extern int device_message_srv_msg_add(GBDevice_info_t* pGBDeviceInfo, msg_type_t msg_type, char* caller_id, char* callee_id, int response_code, int ua_dialog_index, char* msg_body, int msg_body_len, int cr_pos);
    extern void device_message_srv_msg_list_clean();
    extern void device_message_srv_msg_list_free();
    extern int device_message_srv_msg_list_init();
    extern int device_notify_alarm_proc(GBDevice_info_t * pGBDeviceInfo, char * caller_id, char * callee_id, CPacket & inPacket, DBOper * pDevice_Srv_dboper);
    extern int device_notify_alarm_response_proc(GBDevice_info_t * pGBDeviceInfo, char * caller_id, char * callee_id, CPacket & inPacket);
    extern int device_notify_catalog_proc(GBDevice_info_t * pGBDeviceInfo, char * caller_id, char * callee_id, CPacket & inPacket, DBOper * pDevice_Srv_dboper);
    extern int device_notify_channel_status_proc(GBDevice_info_t * pGBDeviceInfo, char * caller_id, char * callee_id, CPacket & inPacket, DBOper * pDevice_Srv_dboper);
    extern int device_notify_cms_restart_proc(GBDevice_info_t * pGBDeviceInfo, char * caller_id, char * callee_id, CPacket & inPacket);
    extern int device_notify_data_value_proc(GBDevice_info_t * pGBDeviceInfo, char * caller_id, char * callee_id, CPacket & inPacket, DBOper * pDevice_Srv_dboper);
    extern int device_notify_device_group_config_proc(GBDevice_info_t * pGBDeviceInfo, char * caller_id, char * callee_id, CPacket & inPacket, DBOper * pDevice_Srv_dboper);
    extern int device_notify_device_group_map_config_proc(GBDevice_info_t * pGBDeviceInfo, char * caller_id, char * callee_id, CPacket & inPacket, DBOper * pDevice_Srv_dboper);
    extern int device_notify_diagnosis_proc(GBDevice_info_t * pGBDeviceInfo, char * caller_id, char * callee_id, CPacket & inPacket, DBOper * pDevice_Srv_dboper);
    extern int device_notify_keep_alive_proc(GBDevice_info_t * pGBDeviceInfo, char * caller_id, char * callee_id, CPacket & inPacket, DBOper * pDevice_Srv_dboper);
    extern int device_notify_media_status_proc(GBDevice_info_t * pGBDeviceInfo, char * caller_id, char * callee_id, int ua_dialog_index, CPacket & inPacket, DBOper * pDevice_Srv_dboper);
    extern int device_notify_msg_proc(GBDevice_info_t* pGBDeviceInfo, char* caller_id, char* callee_id, char* msg_body, int msg_body_len, DBOper* pDevice_Srv_dboper);
    extern int device_notify_RCU_status_proc(GBDevice_info_t * pGBDeviceInfo, char * caller_id, char * callee_id, CPacket & inPacket, DBOper * pDevice_Srv_dboper);
    extern int device_notify_reopen_proc(GBDevice_info_t * pGBDeviceInfo, char * caller_id, char * callee_id, CPacket & inPacket, DBOper * pDevice_Srv_dboper);
    extern int device_notify_request_proc(char* device_ip, int device_port, int device_sock, CPacket& inPacket, DBOper* pDbOper);
    extern int device_notify_response_proc(char* device_ip, int device_port, int device_sock, CPacket& inPacket, DBOper* pDbOper);
    extern int device_notify_status_proc(GBDevice_info_t * pGBDeviceInfo, char * caller_id, char * callee_id, CPacket & inPacket, DBOper * pDevice_Srv_dboper);
    extern int device_notify_target_tount_proc(GBDevice_info_t * pGBDeviceInfo, char * caller_id, char * callee_id, CPacket & inPacket, DBOper * pDevice_Srv_dboper);
    extern int device_notify_topology_phydevice_config_proc(GBDevice_info_t * pGBDeviceInfo, char * caller_id, char * callee_id, CPacket & inPacket, DBOper * pDevice_Srv_dboper);
    extern int device_notify_tv_status_response_proc(GBDevice_info_t * pGBDeviceInfo, char * caller_id, char * callee_id, CPacket & inPacket);
    extern int device_preset_info_response_proc(GBDevice_info_t * pGBDeviceInfo, char * caller_id, char * callee_id, CPacket & inPacket);
    extern int device_query_catalog_proc(GBDevice_info_t* pGBDeviceInfo, char* caller_id, char* callee_id, CPacket& inPacket, DBOper* pDevice_Srv_dboper);
    extern int device_query_catalog_response_proc(GBDevice_info_t * pGBDeviceInfo, char * caller_id, char * callee_id, CPacket & inPacket, DBOper * pDevice_Srv_dboper);
    extern int device_query_dec_media_port_response_proc(GBDevice_info_t * pGBDeviceInfo, char * caller_id, char * callee_id, CPacket & inPacket, DBOper * pDevice_Srv_dboper);
    extern int device_query_device_config_response_proc(GBDevice_info_t * pGBDeviceInfo, char * caller_id, char * callee_id, CPacket & inPacket);
    extern int device_query_device_group_config_proc(GBDevice_info_t * pGBDeviceInfo, char * caller_id, char * callee_id, CPacket & inPacket, DBOper * pDevice_Srv_dboper);
    extern int device_query_device_group_response_proc(GBDevice_info_t * pGBDeviceInfo, char * caller_id, char * callee_id, CPacket & inPacket, DBOper * pDevice_Srv_dboper);
    extern int device_query_device_info_proc(GBDevice_info_t* pGBDeviceInfo, char* caller_id, char* callee_id, CPacket& inPacket);
    extern int device_query_device_map_group_config_proc(GBDevice_info_t * pGBDeviceInfo, char * caller_id, char * callee_id, CPacket & inPacket, DBOper * pDevice_Srv_dboper);
    extern int device_query_device_map_group_response_proc(GBDevice_info_t * pGBDeviceInfo, char * caller_id, char * callee_id, CPacket & inPacket, DBOper * pDevice_Srv_dboper);
    extern int device_query_device_status_proc(GBDevice_info_t* pGBDeviceInfo, char* caller_id, char* callee_id, CPacket& inPacket);
    extern int device_query_preset_info_proc(GBDevice_info_t * pCallerGBDeviceInfo, char * caller_id, char * callee_id, CPacket & inPacket, DBOper * pDevice_Srv_dboper);
    extern int device_query_record_info_proc(GBDevice_info_t* pCallerGBDeviceInfo, char* caller_id, char* callee_id, CPacket& inPacket, DBOper* pDevice_Srv_dboper);
    extern int device_query_topology_phydevice_config_response_proc(GBDevice_info_t * pGBDeviceInfo, char * caller_id, char * callee_id, CPacket & inPacket, DBOper * pDevice_Srv_dboper);
    extern int device_query_tsu_ip_and_port_proc(GBDevice_info_t * pGBDeviceInfo, char * caller_id, char * callee_id, CPacket & inPacket);
    extern int device_record_info_response_proc(GBDevice_info_t * pGBDeviceInfo, char * caller_id, char * callee_id, CPacket & inPacket);
    extern int device_register_request_proc(char* device_ip, int device_port, int device_sock, CPacket& inPacket, DBOper* pDbOper, int* run_thread_time);
    extern int device_register_response_proc(char* device_ip, int device_port, int device_sock, CPacket& inPacket, DBOper* pDbOper);
    extern int device_send_invite_to_dc(cr_t * pCrData);
    extern int device_set_device_video_param_response_proc(GBDevice_info_t * pGBDeviceInfo, char * caller_id, char * callee_id, CPacket & inPacket);
    extern int device_srv_msg_add(GBDevice_info_t* pGBDeviceInfo, msg_type_t msg_type, char* caller_id, char* callee_id, int response_code, char* reasonphrase, int ua_dialog_index, char* msg_body, int msg_body_len, int cr_pos);
    extern void device_srv_msg_free(device_srv_msg_t* device_srv_msg);
    extern int device_srv_msg_init(device_srv_msg_t** device_srv_msg);
    extern void device_srv_msg_list_clean();
    extern void device_srv_msg_list_free();
    extern int device_srv_msg_list_init();
    extern int device_srv_msg_proc(device_srv_msg_t* pDeviceSrvMsg, DBOper* pDevice_Srv_dboper);
    extern int diagnosis_msg_add(diagnosis_msg_t* pDiagnosisMsg);
    extern void diagnosis_msg_free(diagnosis_msg_t* diagnosis_msg);
    extern int diagnosis_msg_init(diagnosis_msg_t** diagnosis_msg);
    extern void diagnosis_msg_list_clean();
    extern void diagnosis_msg_list_free();
    extern int diagnosis_msg_list_init();
    extern int diagnosis_msg_proc(diagnosis_msg_t* pDiagnosisMsg, DBOper* pDboper);
    extern int GeneratingResponseXMLForZRVRegister(char* pcGUID, int iResult, int iErrorCode, std::string & outBuff);
    extern int GeneratingResponseXMLForZRVTaskResult(char* pcGUID, int iResult, int iErrorCode, std::string & outBuff);
    extern int GetGBDeviceListAndSendCataLogToDeviceClient(GBDevice_info_t * pGBDeviceInfo, char * caller_id, char * strDeviceID, char * strSN, DBOper * pDevice_Srv_dboper);
    extern void scan_analysis_target_count_msg_list(DBOper* pDboper);
    extern void scan_device_message_srv_msg_list(DBOper* pDevice_Srv_dboper);
    extern void scan_device_srv_msg_list(DBOper* pDevice_Srv_dboper);
    extern void scan_diagnosis_msg_list(DBOper* pDboper);
    extern int SendAssignTaskToZRVDevice(vector<string>& CompressTaskID, char* device_ip, DBOper* pdboper, int* run_thread_time);
    extern int SendNotCompleteTaskToZRVDeviceForFristRegister(char* device_ip, int* run_thread_time);
    extern int SendNotCompleteTaskToZRVDeviceForTimeOut(char* task_id, int* run_thread_time);
    extern int SendResponseToZRVRegister(char* pcGUID, int iResult, int iErrorCode, int tcp_sock);
    extern int SendResponseToZRVTaskResult(char* pcGUID, int iResult, int iErrorCode, int tcp_sock);
    extern int StopDecService(char * device_id, char * dec_id);
    extern int StopDecServiceByDecInfo(char * dec_id, char * dec_sdp_ip, int dec_sdp_port);
    extern int StopDiagnosisService(char * device_id, char * caller_id);
    extern int StringVectorClone(vector<string>& SourceVectorString, vector<string>& DestVectorString);
    extern int UpdateAllTopologyPhyDeviceStatus2DB(char * strStatus, DBOper * pDevice_Srv_dboper);
    extern int UpdateTopologyPhyDeviceStatus2DB(char * strDeviceID, char * strStatus, DBOper * pDevice_Srv_dboper);
    extern int write_analysis_target_count_msg_to_db_proc(analysis_target_count_msg_t* pAnalysisTargetCountMsg, DBOper* pDboper);
    extern int write_diagnosis_msg_to_db_proc(diagnosis_msg_t* pDiagnosisMsg, DBOper* pDboper);
    extern int ZRVDeviceParseTCPSocketDataProc(char* device_ip, int device_port, int device_sock, char* buf, int len, DBOper* pDbOper, int* run_thread_time);
    extern int zrv_device_compress_task_result_request_proc(char* device_ip, int device_port, int device_sock, CPacket& inPacket, DBOper* pDbOper);
    extern int zrv_device_register_request_proc(char* device_ip, int device_port, int device_sock, CPacket& inPacket, DBOper* pDbOper, int* run_thread_time);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif /* __UDP_TL_H__ */
