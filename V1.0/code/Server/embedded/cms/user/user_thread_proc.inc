#ifndef __USER_THREAD_PROC_H__
#define __USER_THREAD_PROC_H__


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
#include "user/user_info_mgn.inc"

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

    /* 用户业务消息结构体 */
    typedef struct _user_srv_msg_t
    {
        msg_type_t msg_type;                            /* 消息类型 */
        char caller_id[MAX_ID_LEN + 4];                 /* 主叫用户统一编号 */
        char callee_id[MAX_ID_LEN + 4];                 /* 被叫用户统一编号 */
        int response_code;                              /* 回应消息码,只有回应消息有 */
        char reasonphrase[MAX_128CHAR_STRING_LEN + 4];  /* 回应的错误信息，只有回应消息有 */
        int ua_dialog_index;                            /* 对应dialog 索引，只有会话类消息有 */
        char msg_body[MAX_MSG_BODY_STRING_LEN + 4];     /* SIP 消息携带的消息体*/
        int msg_body_len;                               /* SIP 消息携带的消息体长度*/
        int cr_pos;                                     /* 对应的呼叫资源索引信息 */
    } user_srv_msg_t;

    /* 用户业务消息队列 */
    typedef std::deque <user_srv_msg_t*> user_srv_msg_queue;

    /* 用户业务处理线程结构体*/
    typedef struct _user_srv_proc_tl_t
    {
        int iUsed;                                       /* 是否启用 */
        user_info_t* pUserInfo;                          /* 用户信息 */
        osip_thread_t* thread;                           /* 用户业务处理线程 */
        int th_exit;                                     /* 用户业务处理线程标志 */
        time_t  run_time;                                /* 运行时间 */
        DBOper* pUser_Srv_dboper;                        /* 用户业务处理数据库连接 */
        DBOper* pUserLogDbOper;                          /* 日志数据库连接标识 */
        int iUserLogDBOperConnectStatus;                 /* 日志数据库连接状态 */
        user_srv_msg_queue* pUserSrvMsgQueue;            /* 用户业务消息队列 */
#ifdef MULTI_THR
        osip_mutex_t* pUserSrvMsgQueueLock;              /* 用户业务消息队列锁 */
#endif
    } user_srv_proc_tl_t;

    /* 用户业务处理线程队列 */
    typedef struct _user_srv_proc_tl_list_t
    {
        osip_list_t*    pUserSrvProcList;
#ifdef MULTI_THR
        osip_mutex_t* lock;
#endif
    } user_srv_proc_tl_list_t;

    /* 待处理用户业务线程队列 */
    typedef std::deque <user_srv_proc_tl_t*> needtoproc_usersrvproc_queue;

    extern int debug_user_srv_proc_thread_list_lock(const char* file, int line, const char* func);
    extern int debug_user_srv_proc_thread_list_unlock(const char* file, int line, const char* func);
    extern void free_unused_user_tcp_connect();
    extern user_srv_proc_tl_t* get_free_user_srv_proc_thread();
    extern user_srv_proc_tl_t* get_user_srv_proc_thread(char* user_id, char* user_ip, int user_port);
    extern user_srv_proc_tl_t* get_user_srv_proc_thread2(char* user_id, char* user_ip, int user_port);
    extern void scan_user_srv_proc_thread_list();
    extern void ShowConnectTCPUser(int sock);
    extern void show_user_srv_proc_thread(int sock, int type);
    extern void UserLoginServerFree();
    extern int  UserLoginServerInit();
    extern void UserLoginServerMain(DBOper* pDbOper);
    extern int user_srv_proc_thread_assign(user_info_t* pUserInfo);
    extern void* user_srv_proc_thread_execute(void* p);
    extern int user_srv_proc_thread_find(user_info_t* pUserInfo);
    extern void user_srv_proc_thread_free(user_srv_proc_tl_t* run);
    extern int user_srv_proc_thread_init(user_srv_proc_tl_t** run);
    extern void user_srv_proc_thread_list_free();
    extern int user_srv_proc_thread_list_init();
    extern int user_srv_proc_thread_list_lock();
    extern int user_srv_proc_thread_list_unlock();
    extern int user_srv_proc_thread_recycle(char* user_id, char* user_ip, int user_port);
    extern int user_srv_proc_thread_restart(user_info_t* pUserInfo);
    extern int user_srv_proc_thread_start_all();
    extern int user_srv_proc_thread_stop_all();

#ifdef ENABLE_DEBUG_LOCK
#define USER_SRV_PROC_THREAD_SMUTEX_LOCK() debug_user_srv_proc_thread_list_lock(__FILE__, __LINE__, __func__)
#define USER_SRV_PROC_THREAD_SMUTEX_UNLOCK() debug_user_srv_proc_thread_list_unlock(__FILE__, __LINE__, __func__)
#else
#define USER_SRV_PROC_THREAD_SMUTEX_LOCK() user_srv_proc_thread_list_lock()
#define USER_SRV_PROC_THREAD_SMUTEX_UNLOCK() user_srv_proc_thread_list_unlock()
#endif

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif /* __UDP_TL_H__ */
