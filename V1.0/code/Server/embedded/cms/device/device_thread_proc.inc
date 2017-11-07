#ifndef __DEVICE_THREAD_PROC_H__
#define __DEVICE_THREAD_PROC_H__


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
#include "device/device_srv_proc.inc"

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

    /* 前端设备业务处理线程结构体*/
    typedef struct _device_srv_proc_tl_t
    {
        int iUsed;                                              /* 是否启用 */
        GBDevice_info_t* pDeviceInfo;                           /* 前端设备信息 */
        osip_thread_t* thread;                                  /* 前端设备业务处理线程 */
        int th_exit;                                            /* 前端设备业务处理线程标志 */
        time_t  run_time;                                       /* 运行时间 */
        DBOper* pDevice_Srv_dboper;                             /* 前端设备业务处理数据库连接 */
        DBOper* pDeviceLogDbOper;                               /* 日志数据库连接标识 */
        int iDeviceLogDBOperConnectStatus;                      /* 日志数据库连接状态 */
        device_srv_msg_queue* pDeviceSrvMsgQueue;               /* 前端设备业务消息队列 */
#ifdef MULTI_THR
        osip_mutex_t* pDeviceSrvMsgQueueLock;                   /* 前端设备业务消息队列锁 */
#endif
    } device_srv_proc_tl_t;

    /* 前端设备业务处理线程队列 */
    typedef struct _device_srv_proc_tl_list_t
    {
        osip_list_t*    pDeviceSrvProcList;
#ifdef MULTI_THR
        osip_mutex_t* lock;
#endif
    } device_srv_proc_tl_list_t;

    /* 待处理前端设备业务线程队列 */
    typedef std::deque <device_srv_proc_tl_t*> needtoproc_devicesrvproc_queue;

    extern void appoint_device_srv_msg_list_clean(device_srv_proc_tl_t* run);
    extern int debug_device_srv_proc_thread_list_lock(const char* file, int line, const char* func);
    extern int debug_device_srv_proc_thread_list_unlock(const char* file, int line, const char* func);
    extern int device_srv_msg_add_for_appoint(device_srv_proc_tl_t* pDeviceSrvProcThd, msg_type_t msg_type, char* caller_id, char* callee_id, int response_code, char* reasonphrase, int ua_dialog_index, char* msg_body, int msg_body_len, int cr_pos);
    extern int device_srv_proc_thread_assign(GBDevice_info_t* pDeviceInfo);
    extern int device_srv_proc_thread_find(GBDevice_info_t* pDeviceInfo);
    extern void* device_srv_proc_thread_for_appoint_execute(void* p);
    extern void device_srv_proc_thread_free(device_srv_proc_tl_t* run);
    extern int device_srv_proc_thread_init(device_srv_proc_tl_t** run);
    extern void device_srv_proc_thread_list_free();
    extern int device_srv_proc_thread_list_init();
    extern int device_srv_proc_thread_list_lock();
    extern int device_srv_proc_thread_list_unlock();
    extern int device_srv_proc_thread_recycle(char* device_id, char* device_ip, int device_port);
    extern int device_srv_proc_thread_restart(GBDevice_info_t* pDeviceInfo);
    extern int device_srv_proc_thread_start_all();
    extern int device_srv_proc_thread_stop_all();
    extern void free_unused_zrv_device_tcp_connect();
    extern void free_zrv_device_tcp_connect_by_socket(int socket);
    extern device_srv_proc_tl_t* get_device_srv_proc_thread(char* device_id, char* device_ip, int device_port);
    extern device_srv_proc_tl_t* get_device_srv_proc_thread2(char* device_id, char* device_ip, int device_port);
    extern device_srv_proc_tl_t* get_free_device_srv_proc_thread();
    extern int scan_appoint_device_srv_msg_list(device_srv_proc_tl_t* run);
    extern void scan_device_srv_proc_thread_list();
    extern void ShowConnectTCPZRVDevice(int sock);
    extern void show_device_srv_proc_thread(int sock, int type);
    extern void ZRVDeviceLoginServerFree();
    extern int ZRVDeviceLoginServerInit();
    extern void ZRVDeviceLoginServerMain(DBOper* pDbOper, int* run_thread_time);

#ifdef ENABLE_DEBUG_LOCK
#define DEVICE_SRV_PROC_THREAD_SMUTEX_LOCK() debug_device_srv_proc_thread_list_lock(__FILE__, __LINE__, __func__)
#define DEVICE_SRV_PROC_THREAD_SMUTEX_UNLOCK() debug_device_srv_proc_thread_list_unlock(__FILE__, __LINE__, __func__)
#else
#define DEVICE_SRV_PROC_THREAD_SMUTEX_LOCK() device_srv_proc_thread_list_lock()
#define DEVICE_SRV_PROC_THREAD_SMUTEX_UNLOCK() device_srv_proc_thread_list_unlock()
#endif

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif /* __UDP_TL_H__ */
