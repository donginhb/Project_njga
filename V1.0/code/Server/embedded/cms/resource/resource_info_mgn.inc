#ifndef __RESOURCE_INFO_MGN_H__
#define __RESOURCE_INFO_MGN_H__


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

#include "record/record_info_mgn.inc"
#include "service/call_func_proc.inc"

#include <vector>
#include <string>
#include <map>

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
    /* TSU注册消息结构体 */
    typedef struct _tsu_register_t
    {
        int iMsgType;                   /* 消息类型0:TSU注册 1:音频TSU注册 */
        char pcTsuID[MAX_ID_LEN + 4];   /* TSU ID */
        int iSlotID;                    /* TSU所在槽位号 */
        char pcTsuVideoIP[16];          /* TSU 视频网IP */
        int iVideoIPEth;                /* TSU 视频网网口 */
        char pcTsuDeviceIP[16];         /* TSU 设备网IP */
        int iDeviceIPEth;               /* TSU 设备网网口 */
        int iExpires;                   /* TSU 超时时间*/
        int iRefresh;                   /* 是否是刷新*/
        int iTsuType;                   /* TSU 的类型*/
    } tsu_register_t;

    /* TSU注册消息队列 */
    typedef std::deque <tsu_register_t*> tsu_register_queue;

    /* TSU任务消息结构体 */
    typedef struct _tsu_task_attribute_t
    {
        int iMsgType;                           /* 消息类型0:上报任务 1:没有码流2: 暂停播放，3: 恢复播放 */
        char pcTsuID[MAX_ID_LEN + 4];           /* TSU ID */
        int iTaskType;                          /* 任务类型*/
        char pcTaskID[MAX_TSU_TASK_LEN + 4];    /* 任务ID */
    } tsu_task_attribute_t;

    /* TSU任务消息队列 */
    typedef std::deque <tsu_task_attribute_t*> tsu_task_attribute_queue;

    /* TSU上报的音频转发任务 */
    typedef struct _tsu_audio_task_attribute_t
    {
        char pcTsuID[MAX_ID_LEN + 4];           /* TSU ID */
        char pcReceiveIP[16];                   /* 接收的IP地址 */
        int iReceivePort;                       /* 接收的端口号 */
    } tsu_audio_task_attribute_t;

    /* TSU音频任务消息队列 */
    typedef std::deque <tsu_audio_task_attribute_t*> tsu_audio_task_attribute_queue;

    /* TSU上报没有码流消息结构体 */
    typedef struct _tsu_no_stream_msg_t
    {
        char pcDeviceID[MAX_ID_LEN + 2 + 4];   /* 源设备ID */
    } tsu_no_stream_msg_t;

    /* TSU任务消息队列 */
    typedef std::deque <tsu_no_stream_msg_t*> tsu_no_stream_msg_queue;

    /* 资源信息结构体 */
    typedef struct _tsu_resource_info_t
    {
        int iUsed;                                /* 是否启用  */
        int iTsuIndex;                            /* TSU设备索引 */
        int iTsuType;                             /* TSU类型:0:通用，1:指定录像专用 */
        char tsu_device_id[MAX_ID_LEN + 4];       /* TSU设备统一编号 */
        osip_list_t* pTSUIPAddrList;              /* TSU IP地址队列 */
        int iStatus;                              /* 状态:0未注册，1:已注册2:录像路数达到上限,3:没有挂载磁阵  */
        int iExpires;                             /* 注册刷新时间  */

        int iAudioStatus;                         /* 音频转发状态:0未注册，1:已注册 */
        int iAudioExpires;                        /* 音频注册刷新时间  */

        unsigned int iUpLoadFlag;                 /* 是否需要通知TSU重新上报录像文件  */

        /* 录像路数信息 */
#ifdef MULTI_THR
        osip_mutex_t* pRecordInfoListLock;
#endif
        osip_list_t* pRecordInfoList;            /* 该TSU上面的录像信息 */
    } tsu_resource_info_t;

    /* TSU 资源信息队列 */
    typedef std::map <int, tsu_resource_info_t*> TSU_Resource_Info_MAP;
    typedef std::map <int, tsu_resource_info_t*>::iterator TSU_Resource_Info_Iterator;

    /* TSU状态信息 */
    typedef struct _TsuStateInfo
    {
        int DiskMaxSize;                     /* 磁盘总容量 A,单位:MB */
        int DiskMaxWriteWidth;               /* 磁盘写入能力,单位:MB */
        int TsuMaxBandwidth;                 /* TSU存储总带宽B,单位:KB  */
        int IMTsuRecvTotalBandwidth;         /* TSU的即时接收带宽E ,单位:KB  */
        int IMTsuTransferTotalBandwidth;     /* TSU的即时转发带宽E ,单位:KB  */
        int IMTsuStorageTotalBandwidth;      /* TSU的即时存储带宽E ,单位:KB  */
        int IMTsuReplayTotalBandwidth;       /* TSU的即时回放带宽E ,单位:KB  */
    } TsuStateInfo;

    /* 录像状态信息 */
    typedef struct _RecordStateInfo
    {
        int storagelife;                      /* 每路图像存储时间 C */
        int bandwidth;                        /* 每路视频带宽D */
    } RecordStateInfo;

    /* 录像状态信息队列 */
    typedef std::vector<RecordStateInfo> RecordStateInfoList;
    typedef std::vector<RecordStateInfo>::iterator RecordStateInfoList_Iterator;

    /* TSU 创建任务结果消息结构体 */
    typedef struct _tsu_creat_task_result_msg_t
    {
        char strZRVDeviceIP[MAX_IP_LEN];         /* ZRV设备IP地址 */
        char pcTaskID[MAX_TSU_TASK_LEN + 4];     /* 任务ID */
        int iResult;                             /* 任务结果 */
        int iErrCode;                            /* 错误码 */
        int iCompressBeginTime;                  /* 压缩开始时间 */
        int iCompressEndTime;                    /* 压缩结束时间 */
        int iYSHFileSize;                        /* 压缩后的文件大小 */
        char pcDestUrl[128 + 4];                 /* 压缩后的文件路径 */
    } tsu_creat_task_result_msg_t;

    /* TSU 创建任务结果消息队列 */
    typedef std::deque <tsu_creat_task_result_msg_t*> tsu_creat_task_result_msg_queue;

    /* TSU 告警消息结构体 */
    typedef struct _tsu_alarm_msg_t
    {
        int iTSUIndex;                  /* TSU索引 */
        int iType;                      /* 报警类别 */
        int iLevel;                     /* 报警级别 */
        int iTime;                      /* 报警时间 */
        char strInfo[128 + 4];          /* 详细内容 */
    } tsu_alarm_msg_t;

    /* TSU 告警消息队列 */
    typedef std::deque <tsu_alarm_msg_t*> tsu_alarm_msg_queue;

    extern int AddRecordInfoToTSU(tsu_resource_info_t* pTsuResourceInfo, record_info_t* pRecodInfo);
    extern int AddTSUIPAddrToTSUInfoList(DBOper* pDBoper);
    extern int debug_tsu_record_info_lock(tsu_resource_info_t* tsu_resource_info, const char* file, int line, const char* func);
    extern int debug_tsu_record_info_unlock(tsu_resource_info_t* tsu_resource_info, const char* file, int line, const char* func);
    extern int debug_tsu_resource_info_list_lock(const char* file, int line, const char* func);
    extern int debug_tsu_resource_info_list_unlock(const char* file, int line, const char* func);
    extern int get_all_tsu_disk_info(int& iTotalDisk, int& iRemainDisk);
    extern int get_idle_tsu_by_resource_balance_for_query_record();
    extern int get_idle_tsu_by_resource_balance_for_record(int storagelife, int iBandwidth);
    extern int get_idle_tsu_by_resource_balance_for_replay();
    extern int get_idle_tsu_by_resource_balance_for_transfer();
    extern int get_idle_tsu_for_audio_transfer();
    extern int get_max_remain_disk_tsu(TsuStateInfo& stMaxTsuStateInfo, long& iMaxRemain, long iOldMaxRemain);
    extern int get_min_record_info_tsu();
    extern int get_recv_port_by_tsu_resource(char* tsp_ip);
    extern int get_send_port_by_tsu_resource(char* tsp_ip);
    extern int get_tsu_audio_recv_port(char* tsu_ip);
    extern int get_tsu_audio_send_port(char* tsu_ip);
    extern int get_tsu_disk_info(char* tsu_ip, int& iTotalSize, int& iFreeSize);
    extern int get_tsu_free_cpu(char* tsu_ip);
    extern int get_tsu_free_mem(char* tsu_ip);
    extern unsigned long get_tsu_has_record_total_disk(tsu_resource_info_t* pTsuResourceInfo);
    extern char* get_tsu_ip(tsu_resource_info_t* pTsuResourceInfo, char* eth_name);
    extern char * get_tsu_resource_id(int pos);
    extern int get_tsu_state_info(tsu_resource_info_t* pTsuResourceInfo, TsuStateInfo& stTsuStateInfo, RecordStateInfoList& stRecordStateInfoList);
    extern int is_local_tsu(tsu_resource_info_t* pTsuResourceInfo, char* ip_addr);
    extern int is_valid_tsu_resource_info_index(int index);
    extern int notify_set_replay_speed(char* tsp_ip, char* task_id, float iSpeed);
    extern int notify_tsu_add_audio_transfer_task(cr_t* pCrData);
    extern int notify_tsu_add_record_task(cr_t* pCrData, int service_type, int stream_type, int storagelife, int bandwidth);
    extern int notify_tsu_add_replay_task(cr_t* pCrData, int service_type, int stream_type, int start_time, int end_time, int play_time);
    extern int notify_tsu_add_transfer_for_replay_task(cr_t* pCrData, int service_type, int stream_type);
    extern int notify_tsu_add_transfer_task(cr_t* pCrData, int service_type, int stream_type);
    extern int notify_tsu_capture_image(char* tsu_ip, int iType, char* pcSenderID, int iChannelID, char* pcGuid, int iBeforeTime, int iAfterTime, int iInterval, int iTotalNum);
    extern int notify_tsu_delete_audio_transfer_task(char* tsu_ip, char* receive_ip, int receive_port);
    extern int notify_tsu_delete_record_task(char* tsp_ip, char* task_id);
    extern int notify_tsu_delete_replay_task(char* tsp_ip, char* task_id);
    extern int notify_tsu_delete_transfer_task(char* tsp_ip, char* task_id);
    extern int notify_tsu_pause_record(char* tsp_ip, char* task_id);
    extern int notify_tsu_pause_replay(char* tsp_ip, char* task_id);
    extern int notify_tsu_query_replay_list(tsu_resource_info_t* pTsuResourceInfo, char* device_id, int service_type, int stream_type, int start_time, int end_time, VideoRecordList& stVideoRecordList);
    extern int notify_tsu_resume_record(char* tsp_ip, char* task_id);
    extern int notify_tsu_resume_replay(char* tsp_ip, char* task_id);
    extern int notify_tsu_seek_replay(char* tsp_ip, char* task_id, int timeSeek);
    extern int notify_tsu_set_alarm_record(char* tsu_ip, char* task_id, int iType);
    extern int notify_tsu_start_replay(char* tsp_ip, char* task_id);
    extern int notify_tsu_stop_replay(char* tsp_ip, char* task_id);
    extern int notify_tsu_update_mysql_record_stoptime(char* tsp_ip, char* task_id);
    extern int notify_tsu_upload_file_record(char* tsu_ip);
    extern int RemoveRecordInfoFromTSU(tsu_resource_info_t* pTsuResourceInfo, unsigned int device_index, int stream_type);
    extern void scan_resource_info_list(DBOper* pDboper);
    extern void scan_tsu_alarm_msg_list(thread_proc_t* run);
    extern void scan_tsu_audio_task_attribute_msg_list();
    extern void scan_tsu_creat_task_result_msg_list(DBOper* pDbOper);
    extern void scan_tsu_no_stream_msg_list();
    extern void scan_tsu_reg_msg_list(DBOper* pTsu_Reg_dboper);
    extern void scan_tsu_task_attribute_msg_list();
    extern int SendTSUOffLineAlarmToAllClientUser(int tsu_resource_index);
    extern int SetTSUStatus(int tsu_resource_index, int Status);
    extern int set_tsu_index_id(char* tsu_ip, int iTsuIndexID);
    extern int set_tsu_index_id2(int tsu_resource_index);
    extern void ShowTSUInfo(int sock, int type);
    extern int TSUResourceIPAddrListClone(const osip_list_t* src, cr_t* pCurrentCrData);
    extern int tsu_alarm_msg_add(int iTSUIndex, int iType, int iLevel, int iTime, char* pcInfo);
    extern void tsu_alarm_msg_free(tsu_alarm_msg_t* tsu_alarm_msg);
    extern int tsu_alarm_msg_init(tsu_alarm_msg_t** tsu_alarm_msg);
    extern void tsu_alarm_msg_list_free();
    extern int tsu_alarm_msg_list_init();
    extern int tsu_alarm_msg_proc(tsu_alarm_msg_t* pTsuAlarmMsg, DBOper* pDBoper);
    extern int tsu_audio_register(const ::std::string& sTsuID, int iExpires, int iRefresh);
    extern int tsu_audio_reg_msg_proc(char* pcTsuID, int iExpires, int iRefresh);
    extern int tsu_audio_task_attribute_msg_add(char* pcTsuID, char* pcReceiveIP, int iReceivePort);
    extern void tsu_audio_task_attribute_msg_free(tsu_audio_task_attribute_t* tsu_audio_task_attribute_msg);
    extern int tsu_audio_task_attribute_msg_init(tsu_audio_task_attribute_t** tsu_audio_task_attribute_msg);
    extern void tsu_audio_task_attribute_msg_list_free();
    extern int tsu_audio_task_attribute_msg_list_init();
    extern int tsu_audio_task_attribute_msg_proc(char* pcTsuID, char* pcReceiveIP, int iReceivePort);
    extern int tsu_creat_task_result_msg_add(char* zrv_device_ip, char* pcTaskID, int iResult, int iErrCode, int iCompressBeginTime, int iCompressEndTime, int iYSHFileSize, char* pcDestUrl);
    extern void tsu_creat_task_result_msg_free(tsu_creat_task_result_msg_t* tsu_creat_task_result_msg);
    extern int tsu_creat_task_result_msg_init(tsu_creat_task_result_msg_t** tsu_creat_task_result_msg);
    extern void tsu_creat_task_result_msg_list_free();
    extern int tsu_creat_task_result_msg_list_init();
    extern int tsu_creat_task_result_msg_proc(char* pcZRVDeviceIP, char* pcTaskID, int iResult, int iErrCode, int iCompressBeginTime, int iCompressEndTime, int iYSHFileSize, char* pcDestUrl, DBOper* pDbOper);
    extern int tsu_get_time();
    extern char* tsu_ip_get_by_ethname(tsu_resource_info_t* pTsuResourceInfo, char* eth_name);
    extern char* tsu_ip_get_by_type(tsu_resource_info_t* pTsuResourceInfo, ip_addr_type_t ip_type);
    extern int tsu_ip_pair_add(tsu_resource_info_t* pTsuResourceInfo, char* eth_name, ip_addr_type_t ip_type, char* local_ip, int local_port);
    extern int tsu_notify_alarm_msg();
    extern void tsu_notify_cpu_temperature(int iSlotID, int iTemperature);
    extern int tsu_notify_current_task();
    extern int tsu_notify_device_no_stream(const ::std::string& sDeviceID);
    extern int tsu_notify_pause_play();
    extern int tsu_notify_play_end();
    extern int tsu_notify_resume_play();
    extern int tsu_notify_tcp_transfer_end(const ::std::string& strTranferSessionID, int iType);
    extern int tsu_no_stream_msg_add(char* pcDeviceID);
    extern void tsu_no_stream_msg_free(tsu_no_stream_msg_t* tsu_no_stream_msg);
    extern int tsu_no_stream_msg_init(tsu_no_stream_msg_t** tsu_no_stream_msg);
    extern void tsu_no_stream_msg_list_free();
    extern int tsu_no_stream_msg_list_init();
    extern int tsu_no_stream_msg_proc(char* pcDeviceID);
    extern int tsu_record_info_lock(tsu_resource_info_t* tsu_resource_info);
    extern int tsu_record_info_unlock(tsu_resource_info_t* tsu_resource_info);
    extern int tsu_register(const ::std::string& sTsuID, int iSlotID, const ::std::string& sTsuVideoIP, int iVideoIPEth, const ::std::string& sTsuDeviceIP, int iDeviceIPEth, int iExpires, int iRefresh, int iTsuType, int& iTsuIndex);
    extern int tsu_reg_msg_add(int iMsgType, char* pcTsuID, int iSlotID, char* pcTsuVideoIP, int iVideoIPEth, char* pcTsuDeviceIP, int iDeviceIPEth, int iExpires, int iRefresh, int iTsuType);
    extern void tsu_reg_msg_free(tsu_register_t* tsu_reg_msg);
    extern int tsu_reg_msg_init(tsu_register_t** tsu_reg_msg);
    extern void tsu_reg_msg_list_free();
    extern int tsu_reg_msg_list_init();
    extern int tsu_reg_msg_proc(int iMsgType, char* pcTsuID, int iSlotID, char* pcTsuVideoIP, int iVideoIPEth, char* pcTsuDeviceIP, int iDeviceIPEth, int iExpires, int iRefresh, int iTsuType, DBOper* pTsu_Reg_dboper);
    extern int tsu_resource_info_add(char* tsu_device_id, int iTsuIndex, int iTsuType);
    extern int tsu_resource_info_find(char* tsu_id);
    extern int tsu_resource_info_find_by_index(int tsu_index);
    extern void tsu_resource_info_free(tsu_resource_info_t* tsu_resource_info);
    extern tsu_resource_info_t* tsu_resource_info_get(int pos);
    extern tsu_resource_info_t* tsu_resource_info_get_by_index(int tsu_index);
    extern int tsu_resource_info_init(tsu_resource_info_t** tsu_resource_info);
    extern void tsu_resource_info_list_free();
    extern int tsu_resource_info_list_init();
    extern int tsu_resource_info_list_lock();
    extern int tsu_resource_info_list_unlock();
    extern int tsu_send_image_result(int iType, int iResult, const ::std::string& strDeviceID, int iChannelID, const ::std::string& strGuid, int iPicCount);
    extern int tsu_task_attribute_msg_add(int iMsgType, char* pcTsuID, int iTaskType, char* pcTaskID);
    extern void tsu_task_attribute_msg_free(tsu_task_attribute_t* tsu_task_attribute_msg);
    extern int tsu_task_attribute_msg_init(tsu_task_attribute_t** tsu_task_attribute_msg);
    extern void tsu_task_attribute_msg_list_free();
    extern int tsu_task_attribute_msg_list_init();
    extern int tsu_task_attribute_msg_proc(int iMsgType, char* pcTsuID, int iTaskType, char* pcTaskID);
    extern int tsu_video_reg_msg_proc(char* pcTsuID, int iSlotID, char* pcTsuVideoIP, int iVideoIPEth, char* pcTsuDeviceIP, int iDeviceIPEth, int iExpires, int iRefresh, int iTsuType, DBOper* pTsu_Reg_dboper);
    extern int write_tsu_alarm_to_db_proc(char* pcTSUIndex, char* pcType, char* pcLevel, char* pcTime, char* pcInfo, DBOper* pDboper);

#ifdef ENABLE_DEBUG_LOCK
#define TSU_RECORD_SMUTEX_LOCK(s) debug_tsu_record_info_lock(s, __FILE__, __LINE__, __func__)
#define TSU_RECORD_SMUTEX_UNLOCK(s) debug_tsu_record_info_unlock(s, __FILE__, __LINE__, __func__)
#define TSU_SMUTEX_LOCK() debug_tsu_resource_info_list_lock(__FILE__, __LINE__, __func__)
#define TSU_SMUTEX_UNLOCK() debug_tsu_resource_info_list_unlock(__FILE__, __LINE__, __func__)
#else
#define TSU_RECORD_SMUTEX_LOCK(s) tsu_record_info_lock(s)
#define TSU_RECORD_SMUTEX_UNLOCK(s) tsu_record_info_unlock(s)
#define TSU_SMUTEX_LOCK() tsu_resource_info_list_lock()
#define TSU_SMUTEX_UNLOCK() tsu_resource_info_list_unlock()
#endif

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif /* __UDP_TL_H__ */
