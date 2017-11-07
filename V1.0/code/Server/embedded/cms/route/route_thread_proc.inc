#ifndef __ROUTE_THREAD_PROC_H__
#define __ROUTE_THREAD_PROC_H__


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
#include "route/route_srv_proc.inc"

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
    typedef struct _route_srv_proc_tl_t
    {
        int iUsed;                                              /* 是否启用 */
        route_info_t* pRouteInfo;                               /* 上级路由信息 */
        osip_thread_t* thread;                                  /* 上级路由业务处理线程 */
        int th_exit;                                            /* 上级路由业务处理线程标志 */
        time_t  run_time;                                       /* 运行时间 */
        DBOper* pRoute_Srv_dboper;                              /* 上级路由业务处理数据库连接 */
        DBOper* pRouteLogDbOper;                                /* 日志数据库连接标识 */
        int iRouteLogDBOperConnectStatus;                       /* 日志数据库连接状态 */
        route_srv_msg_queue* pRouteSrvMsgQueue;                 /* 上级路由业务消息队列 */
#ifdef MULTI_THR
        osip_mutex_t* pRouteSrvMsgQueueLock;                    /* 上级路由业务消息队列锁 */
#endif
    } route_srv_proc_tl_t;

    /* 上级路由业务处理线程队列 */
    typedef struct _route_srv_proc_tl_list_t
    {
        osip_list_t*    pRouteSrvProcList;
#ifdef MULTI_THR
        osip_mutex_t* lock;
#endif
    } route_srv_proc_tl_list_t;

    /* 待处理上级路由业务线程队列 */
    typedef std::deque <route_srv_proc_tl_t*> needtoproc_routesrvproc_queue;

    extern void appoint_route_srv_msg_list_clean(route_srv_proc_tl_t* run);
    extern int debug_route_srv_proc_thread_list_lock(const char* file, int line, const char* func);
    extern int debug_route_srv_proc_thread_list_unlock(const char* file, int line, const char* func);
    extern route_srv_proc_tl_t* get_free_route_srv_proc_thread();
    extern route_srv_proc_tl_t* get_route_srv_proc_thread(char* route_id, char* route_ip, int route_port);
    extern route_srv_proc_tl_t* get_route_srv_proc_thread2(char* route_id, char* route_ip, int route_port);
    extern int route_srv_msg_add_for_appoint(route_srv_proc_tl_t* pRouteSrvProcThd, msg_type_t msg_type, char* caller_id, char* callee_id, int response_code, char* reasonphrase, int ua_dialog_index, char* msg_body, int msg_body_len, int cr_pos);
    extern int route_srv_proc_thread_assign(route_info_t* pRouteInfo);
    extern int route_srv_proc_thread_find(route_info_t* pRouteInfo);
    extern void* route_srv_proc_thread_for_appoint_execute(void* p);
    extern void route_srv_proc_thread_free(route_srv_proc_tl_t* run);
    extern int route_srv_proc_thread_init(route_srv_proc_tl_t** run);
    extern void route_srv_proc_thread_list_free();
    extern int route_srv_proc_thread_list_init();
    extern int route_srv_proc_thread_list_lock();
    extern int route_srv_proc_thread_list_unlock();
    extern int route_srv_proc_thread_recycle(char* route_id, char* route_ip, int route_port);
    extern int route_srv_proc_thread_restart(route_info_t* pRouteInfo);
    extern int route_srv_proc_thread_start_all();
    extern int route_srv_proc_thread_stop_all();
    extern int scan_appoint_route_srv_msg_list(route_srv_proc_tl_t* run);
    extern void scan_route_srv_proc_thread_list();
    extern void show_route_srv_proc_thread(int sock, int type);

#ifdef ENABLE_DEBUG_LOCK
#define ROUTE_SRV_PROC_THREAD_SMUTEX_LOCK() debug_route_srv_proc_thread_list_lock(__FILE__, __LINE__, __func__)
#define ROUTE_SRV_PROC_THREAD_SMUTEX_UNLOCK() debug_route_srv_proc_thread_list_unlock(__FILE__, __LINE__, __func__)
#else
#define ROUTE_SRV_PROC_THREAD_SMUTEX_LOCK() route_srv_proc_thread_list_lock()
#define ROUTE_SRV_PROC_THREAD_SMUTEX_UNLOCK() route_srv_proc_thread_list_unlock()
#endif

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif /* __UDP_TL_H__ */
