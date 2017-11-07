#ifndef __COMPRESS_TASK_PROC_H__
#define __COMPRESS_TASK_PROC_H__


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

    /* 市局数据库结构 */
    typedef struct _jly_yspb_t
    {
        char jlbh[MAX_TSU_TASK_LEN + 4];                 /*1、记录编号(jlbh)，字符串 */
        char wjmc[128 + 4];                              /*2、文件名称(wjmc)，字符串*/
        char kzm[32 + 4];                                /*3、文件后缀名称(kzm)，字符串*/
        int wjdx;                                        /*4、文件大小(wjdx)，整形*/
        char scdw[128 + 4];                              /*5、上传单位(scdw)，字符串*/
        int scsj;                                        /*6、上传时间(scsj)，格式：20170527120000 */
        char cclj[128 + 4];                              /*7、存储路径(cclj)，字符串*/
        int yshdx;                                       /*8、压缩后的文件大小(yshdx)，整形*/
        char yshlj[128 + 4];                             /*9、压缩后的存储路径(yshlj)，字符串*/
    } jly_yspb_t;

    /* 压缩任务数据块结构体 */
    typedef struct _compress_task_t
    {
        jly_yspb_t stYSPB;                               /* 对应的平台返回字段 */
        int iAssignFlag;                                 /* 是否分配:0:没有分配，1:已经分配 */
        char strPlatformIP[MAX_IP_LEN];                  /* 平台IP地址 */
        char strZRVDeviceIP[MAX_IP_LEN];                 /* ZRV设备IP地址 */
        int iTaskCreateTime;                             /* 任务创建时间 */
        int iTaskStatus;                                 /* 任务状态:0:未开始压缩任务,1:正在压缩任务，2:压缩完成，3:压缩超时 */
        int iTaskResult;                                 /* 任务结果:0:等待结果,1:成功，2:失败 */
        int wait_answer_expire;                          /* 等待响应超时时间,默认一个小时 */
        int resend_count;                                /* 重新下发次数 */
    } compress_task_t;

    /* 呼叫连接数据队列 */
    typedef std::map <string, compress_task_t*> COMPRESS_TASK_MAP;
    typedef std::map <string, compress_task_t*>::iterator COMPRESS_TASK_Iterator;

    extern int AddCompressTask(char* platform_ip, jly_yspb_t* pstYSPB, DBOper* ptDBoper);
    extern int AddCompressTaskToAssignDB(compress_task_t* compress_task, DBOper* ptDBoper);
    extern int AddCompressTaskToDB(char* task_id, int iTaskStatus, int iTaskResult, int iErrorCode, int iCompressBeginTime, int iCompressEndTime, int iYSHFileSize, char* pcDestUrl, DBOper* ptDBoper);
    extern int compress_task_add(compress_task_t* pCompressTaskData);
    extern compress_task_t* compress_task_find(char* task_id);
    extern void compress_task_free(compress_task_t* compress_task);
    extern int compress_task_init(compress_task_t** compress_task);
    extern void compress_task_list_free();
    extern int compress_task_list_init();
    extern int compress_task_list_lock();
    extern int compress_task_list_unlock();
    extern int compress_task_remove(char* task_id);
    extern int compress_task_set_task_assign_info(char* task_id, int iAssignFlag, char* zrv_device_ip);
    extern int compress_task_set_task_result_info(char* task_id, int iTaskStatus, int iTaskResult, int iYSHFileSize, char* pcDestUrl);
    extern int debug_compress_task_list_lock(const char* file, int line, const char* func);
    extern int debug_compress_task_list_unlock(const char* file, int line, const char* func);
    extern int DeleteCompressTask(char* platform_ip, DBOper* ptDBoper);
    extern int DeleteCompressTaskByPlatformIP(char* platform_ip);
    extern int DeleteCompressTaskFromDBByPlatformIP(char* platform_ip, DBOper* pDbOper);
    extern int DeleteCompressTaskFromDBForStart(DBOper* pDbOper);
    extern int GetAllAssignedCompressTaskByZRVDeviceIP(char* device_ip, vector<string>& CompressTaskID);
    extern int GetAllProcCompressTaskByPlatformIP(char* platform_ip, vector<string>& CompressTaskID);
    extern int GetCurrentCompressTaskCountByZRVDeviceIP(char* platform_ip, char* zrv_device_ip);
    extern int GetNeedAssignCompressTaskByPlatformIP(char* platform_ip, vector<string>& CompressTaskID);
    extern int get_complete_compress_task_from_assign_table(vector<string>& RecordNumVector, vector<int>& ErrorCodeVector, vector<int>& CompressBeginTimeVector, vector<int>& CompressEndTimeVector, DBOper* pDbOper);
    extern int HasCompressTaskNotComplete(char* platform_ip);
    extern int HasCompressTaskNotComplete2(char* platform_ip);
    extern int scan_compress_task_list_for_wait_expire(DBOper* pDbOper, int* run_thread_time);
    extern void ShowCompressTask(int sock);
    extern int UpdateCompressTaskAssignInfo(char* task_id, int iAssignFlag, char* zrv_device_ip, DBOper* pDbOper);
    extern int UpdateCompressTaskAssignInfoToDB(char* task_id, int iAssignFlag, char* zrv_device_ip, DBOper* pDbOper);
    extern int UpdateCompressTaskResultInfo(char* task_id, int iTaskStatus, int iTaskResult, int iErrorCode, int iCompressBeginTime, int iCompressEndTime, int iYSHFileSize, char* pcDestUrl, DBOper* pDbOper);
    extern int UpdateCompressTaskResultInfoToDB(char* task_id, int iTaskStatus, int iTaskResult, int iErrorCode, int iCompressBeginTime, int iCompressEndTime, int iYSHFileSize, char* pcDestUrl, DBOper* pDbOper);
    extern int update_compress_task_list_for_wait_expire(DBOper* pDbOper);

#ifdef ENABLE_DEBUG_LOCK
#define COMPRESS_TASK_SMUTEX_LOCK() debug_compress_task_list_lock(__FILE__, __LINE__, __func__)
#define COMPRESS_TASK_SMUTEX_UNLOCK() debug_compress_task_list_unlock(__FILE__, __LINE__, __func__)
#else
#define COMPRESS_TASK_SMUTEX_LOCK() compress_task_list_lock()
#define COMPRESS_TASK_SMUTEX_UNLOCK() compress_task_list_unlock()
#endif

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif /* __UDP_TL_H__ */
