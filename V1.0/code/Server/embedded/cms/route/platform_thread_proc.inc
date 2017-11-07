#ifndef __PLATFORM_THREAD_PROC_H__
#define __PLATFORM_THREAD_PROC_H__


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
#include "common/DBOper.h"
#include "route/platform_info_mgn.inc"

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

    /* 上级路由业务处理线程结构体*/
    typedef struct _platform_srv_proc_tl_t
    {
        int iUsed;                                  /* 是否启用 */
        unsigned int platform_index;                /* 索引 */
        char platform_ip[MAX_IP_LEN];               /* 管理平台 IP 地址*/
        osip_thread_t* thread;                      /* 上级路由业务处理线程 */
        int th_exit;                                /* 上级路由业务处理线程标志 */
        time_t  run_time;                           /* 运行时间 */
        DBOper* pPlatform_Srv_dboper;               /* 上级路由业务处理数据库连接 */
        DBOper* pPlatformLogDbOper;                 /* 日志数据库连接标识 */
        int iPlatformLogDBOperConnectStatus;        /* 日志数据库连接状态 */

        int iTaskMode;                              /* 任务获取模式 :0: 自动执行，1:手动执行，默认0 */
        int iTaskRunInterval;                       /* 任务执行间隔时间 */
        int iTaskGetInterval;                       /* 任务获取间隔时间 */

        int iTaskBeginTime;                         /* 手动模式下获取任务开始时间 */
        int iTaskEndTime;                           /* 手动模式下获取任务执行时间 */
        int iLastGetTaskTime;                       /* 上次获取任务时间 */

        int iLastTaskRunTime;                       /* 上次压缩执行时间 */

        int iCompressTaskStatus;                    /* 压缩任务状态:0:初始状态, 1:准备获取, 2:正在获取处理, 3:获取失败, 4:获取成功，准备分配, 5:正在分配处理, 6:没有分配成功, 7:分配成功, 8:此轮压缩任务完成 */
    } platform_srv_proc_tl_t;

    /* 上级路由业务处理线程队列 */
    typedef struct _platform_srv_proc_tl_list_t
    {
        osip_list_t*    pPlatformSrvProcList;
#ifdef MULTI_THR
        osip_mutex_t* lock;
#endif
    } platform_srv_proc_tl_list_t;

    /* 待处理上级路由业务线程队列 */
    typedef std::deque <platform_srv_proc_tl_t*> needtoproc_platformsrvproc_queue;

    extern int debug_platform_srv_proc_thread_list_lock(const char* file, int line, const char* func);
    extern int debug_platform_srv_proc_thread_list_unlock(const char* file, int line, const char* func);
    extern int get_compress_task_from_webservice_thread_execute(void* p);
    extern platform_srv_proc_tl_t* get_free_platform_srv_proc_thread();
    extern platform_srv_proc_tl_t* get_platform_srv_proc_thread(unsigned int platform_index);
    extern platform_srv_proc_tl_t* get_platform_srv_proc_thread2(unsigned int platform_index);
    extern int get_platform_thread_info_from_db(platform_srv_proc_tl_t* plat_thread);
    extern int platform_srv_proc_thread_assign(platform_info_t* pPlatformInfo);
    extern void* platform_srv_proc_thread_execute(void* p);
    extern int platform_srv_proc_thread_find(unsigned int platform_index);
    extern void platform_srv_proc_thread_free(platform_srv_proc_tl_t* run);
    extern int platform_srv_proc_thread_init(platform_srv_proc_tl_t** run);
    extern void platform_srv_proc_thread_list_free();
    extern int platform_srv_proc_thread_list_init();
    extern int platform_srv_proc_thread_list_lock();
    extern int platform_srv_proc_thread_list_unlock();
    extern int platform_srv_proc_thread_recycle(unsigned int platform_index);
    extern int platform_srv_proc_thread_start_all();
    extern int platform_srv_proc_thread_stop_all();
    extern void scan_platform_srv_proc_thread_list();
    extern void set_platform_srv_proc_thread_compress_task_status(int status);
    extern void show_platform_srv_proc_thread(int sock, int type);
    extern int UpdatePlatFormCompressTaskStatusToDB(char* platform_ip, int iLastGetTaskTime, int iCompressTaskStatus, int iErrorCode, DBOper* pDbOper);

#ifdef ENABLE_DEBUG_LOCK
#define PLATFORM_SRV_PROC_THREAD_SMUTEX_LOCK() debug_platform_srv_proc_thread_list_lock(__FILE__, __LINE__, __func__)
#define PLATFORM_SRV_PROC_THREAD_SMUTEX_UNLOCK() debug_platform_srv_proc_thread_list_unlock(__FILE__, __LINE__, __func__)
#else
#define PLATFORM_SRV_PROC_THREAD_SMUTEX_LOCK() platform_srv_proc_thread_list_lock()
#define PLATFORM_SRV_PROC_THREAD_SMUTEX_UNLOCK() platform_srv_proc_thread_list_unlock()
#endif

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif /* __UDP_TL_H__ */
