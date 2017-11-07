#ifndef __RECORD_INFO_MGN_H__
#define __RECORD_INFO_MGN_H__


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
    typedef enum _record_status_t
    {
        RECORD_STATUS_INIT,
        RECORD_STATUS_OFFLINE,
        RECORD_STATUS_NOSTREAM,
        RECORD_STATUS_NETWORK_ERROR,
        RECORD_STATUS_NO_TSU,
        RECORD_STATUS_NOT_SUPPORT_MULTI_STREAM,
        RECORD_STATUS_PROC,
        RECORD_STATUS_COMPLETE,
        RECORD_STATUS_NULL
    }
    record_status_t;

    /* 时间配置结构体 */
    typedef struct _record_time_config_t
    {
        unsigned int uID;                  /* 数据库索引 */
        int iBeginTime;                    /* 开始时间 */
        int iEndTime;                      /* 结束时间 */
        int iStatus;                       /* 状态:0:暂停，1:恢复 */
        int del_mark;                      /* 删除标识 */
    } record_time_config_t;

    /* 录像时刻策略结构体 */
    typedef struct _record_time_sched_t
    {
        unsigned int uID;                  /* 数据库索引 */
        int record_cr_index;               /* 录像呼叫信息索引 */
        int del_mark;                      /* 删除标识 */

        osip_list_t* pDayOfWeekTimeList;   /* 每天时刻列表 */

    } record_time_sched_t;

    /* 待处理录像时刻信息队列 */
    typedef std::deque <record_time_sched_t*> needtoproc_recordtimesched_queue;

    /* 录像时间策略队列 */
    typedef struct _record_time_sched_list_t
    {
        osip_list_t* pRecordTimeSchedList;
#ifdef MULTI_THR
        osip_mutex_t* lock;
#endif
    } record_time_sched_list_t;

    /* 录像信息结构体 */
    typedef struct _record_info_t
    {
        unsigned int uID;                  /* 数据库索引 */
        unsigned int device_index;         /* 逻辑设备索引 */
        int stream_type;                   /* 媒体流类型:主流，从流，分析流 */
        int record_enable;                 /* 是否启动录像 */
        int record_days;                   /* 录像天数 */
        int record_timeLen;                /* 录像时长 */
        int record_type;                   /* 录像类型:EV9000_RECORD_TYPE_NORMAL,EV9000_RECORD_TYPE_INTELLIGENCE,EV9000_RECORD_TYPE_ALARM,EV9000_RECORD_TYPE_BACKUP */
        int assign_record;                 /* 是否指定TSU录像 */
        int assign_tsu_index;              /* 指定TSU录像的TSU索引 */
        int bandwidth;                     /* 前端带宽*/
        int TimeOfAllWeek;                 /* 是否全周录像 */
        int hasRecordDays;                 /* 已经录像天数 */
        int tsu_index;                     /* 录像的TSU索引 */
        int record_cr_index;               /* 录像呼叫信息索引 */
        int record_try_count;              /* 录像尝试次数 */
        int record_retry_interval;         /* 录像尝试间隔 */
        record_status_t record_status;     /* 录像状态 */
        time_t record_start_time;          /* 录像启动时间 */
        int iTSUPauseStatus;               /* 分段录像情况下，TSU暂停状态:0:没有暂停，1:已经暂停 */
        int iTSUResumeStatus;              /* 分段录像情况下，TSU恢复状态:0:没有恢复，1:已经恢复 */
        int iTSUAlarmRecordStatus;         /* 报警录像情况下，TSU报警录像状态:0:没有录像，1:已经录像 */
        int del_mark;                      /* 删除标识:0:不删除，1:需要删除，2:录像需要重新调度, 3:需要调用TSU恢复录像接口, 4:需要调用TSU暂停录像接口 */
    } record_info_t;

    /* 录像信息队列 */
    typedef struct _record_info_list_t
    {
        osip_list_t* pRecordInfoList;
#ifdef MULTI_THR
        osip_mutex_t* lock;
#endif
    } record_info_list_t;

    /* 待处理录像信息队列 */
    typedef std::deque <record_info_t*> needtoproc_recordinfo_queue;

    /* 录像记录结构 */
    typedef struct _VideoRecord
    {
        int DeviceIndex;
        int StorageIndex;
        int StartTime;
        int StopTime;
        int Size;
        string StorageIP;
        string StoragePath;
        int Type;
        int Resved1;
        string Resved2;
    } VideoRecord;

    /* 录像记录队列 */
    typedef std::vector<VideoRecord> VideoRecordList;
    typedef std::vector<VideoRecord>::iterator VideoRecordList_Iterator;

    extern int AddRecordInfo2DB(record_info_t * pRecordInfo, DBOper * pRecord_Srv_dboper);
    extern int add_record_info_by_message_cmd(unsigned int device_index, DBOper* pRecord_Srv_dboper);
    extern int check_plan_action_record_info_from_db_to_list(DBOper* pRecord_Srv_dboper);
    extern void check_RecordInfo_need_to_reload_begin(DBOper* pDboper);
    extern void check_RecordInfo_need_to_reload_end();
    extern void check_RecordTimeSchedConfig_need_to_reload(DBOper* pDboper);
    extern int check_record_info_from_db_to_list(DBOper* pRecord_Srv_dboper);
    extern int check_record_time_sched_config_from_db_to_list(record_time_sched_t * pRecordTimeSched, DBOper * pDBOper);
    extern int check_record_time_sched_from_db_to_list(DBOper * pDBOper);
    extern int check_shdb_alarm_upload_pic_record_info_from_db_to_list(DBOper* pRecord_Srv_dboper);
    extern int check_shdb_daily_upload_pic_record_info_from_db_to_list(DBOper* pRecord_Srv_dboper);
    extern int debug_record_info_list_lock(const char* file, int line, const char* func);
    extern int debug_record_info_list_unlock(const char* file, int line, const char* func);
    extern int debug_record_time_sched_list_lock(const char * file, int line, const char * func);
    extern int debug_record_time_sched_list_unlock(const char * file, int line, const char * func);
    extern int delete_record_info_from_list_by_mark();
    extern int delete_record_time_sched_from_list_by_mark();
    extern int delRecordInfo2DB(unsigned int device_index, DBOper * pRecord_Srv_dboper);
    extern int del_record_info_by_message_cmd(unsigned int device_index, DBOper* pRecord_Srv_dboper);
    extern int find_is_cr_index_used_by_other_record_info(int current_cr_index, int current_pos);
    extern int find_record_config_by_device_index(unsigned int uDeviceIndex, DBOper * pDboper);
    extern int get_record_cr_index_by_record_index(unsigned int uID);
    extern int get_record_info_index_from_list(vector<unsigned int>& RecordInfoIndexVector);
    extern int get_record_status_from_record_time_config(osip_list_t * pDayOfWeekTimeList);
    extern int has_record_info_find_by_stream_type(unsigned int device_index, int stream_type, int record_type);
    extern int RecordInfo_db_refresh_proc();
    extern int RecordTimeSchedConfigProc(record_time_sched_t * pRecordTimeSched);
    extern int RecordTimeSchedConfig_db_refresh_proc();
    extern int record_info_add(record_info_t* pRecordInfo);
    extern int record_info_find_by_cr_index(int cr_index);
    extern int record_info_find_by_cr_index_for_response(int cr_index);
    extern int record_info_find_by_stream_type(unsigned int device_index, int stream_type);
    extern int record_info_find_by_stream_type2(unsigned int device_index, int stream_type);
    extern void record_info_free(record_info_t* record_info);
    extern record_info_t* record_info_get(int pos);
    extern record_info_t* record_info_get_by_record_index(unsigned int index);
    extern record_info_t* record_info_get_by_record_type(unsigned int device_index, int record_type);
    extern record_info_t* record_info_get_by_record_type2(unsigned int device_index, int record_type);
    extern record_info_t* record_info_get_by_stream_type_and_record_type(unsigned int device_index, int stream_type, int record_type);
    extern record_info_t* record_info_get_by_stream_type_and_record_type2(unsigned int device_index, int stream_type, int record_type);
    extern int record_info_init(record_info_t** record_info);
    extern void record_info_list_free();
    extern int record_info_list_init();
    extern int record_info_list_lock();
    extern int record_info_list_unlock();
    extern int record_info_remove(int pos);
    extern int record_time_config_add(osip_list_t * pDayOfWeekTimeList, unsigned int uID, int iBeginTime, int iEndTime);
    extern void record_time_config_free(record_time_config_t * time_config);
    extern record_time_config_t* record_time_config_get(osip_list_t * pDayOfWeekTimeList, unsigned int uID);
    extern int record_time_config_init(record_time_config_t** time_config);
    extern int record_time_sched_add(unsigned int uID, int record_cr_index);
    extern void record_time_sched_free(record_time_sched_t * record_time_sched);
    extern record_time_sched_t* record_time_sched_get(unsigned int uID);
    extern int record_time_sched_init(record_time_sched_t** record_time_sched);
    extern void record_time_sched_list_free();
    extern int record_time_sched_list_init();
    extern int record_time_sched_list_lock();
    extern int record_time_sched_list_unlock();
    extern int RemoveDeviceRecordInfo(char * device_id, int stream_type, int tsu_resource_index);
    extern int RemoveRecordTimeSchedConfig(record_time_sched_t * pRecordTimeSched);
    extern int RemoveRecordTimeSchedMark(unsigned int uID);
    extern void scan_record_info_list();
    extern void scan_record_info_list_for_monitor_print();
    extern void scan_record_time_sched_list();
    extern int set_record_info_list_del_mark(int del_mark);
    extern int set_record_time_sched_list_del_mark(int del_mark);
    extern void ShowRecordInfo(int sock, int type);
    extern void ShowRecordTimeSchedInfo(int sock);

#ifdef ENABLE_DEBUG_LOCK
#define RECORD_INFO_SMUTEX_LOCK() debug_record_info_list_lock(__FILE__, __LINE__, __func__)
#define RECORD_INFO_SMUTEX_UNLOCK() debug_record_info_list_unlock(__FILE__, __LINE__, __func__)
#define RECORD_TIME_SCHED_SMUTEX_LOCK() debug_record_time_sched_list_lock(__FILE__, __LINE__, __func__)
#define RECORD_TIME_SCHED_SMUTEX_UNLOCK() debug_record_time_sched_list_unlock(__FILE__, __LINE__, __func__)
#else
#define RECORD_INFO_SMUTEX_LOCK() record_info_list_lock()
#define RECORD_INFO_SMUTEX_UNLOCK() record_info_list_unlock()
#define RECORD_TIME_SCHED_SMUTEX_LOCK() record_time_sched_list_lock()
#define RECORD_TIME_SCHED_SMUTEX_UNLOCK() record_time_sched_list_unlock()
#endif

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif /* __UDP_TL_H__ */
