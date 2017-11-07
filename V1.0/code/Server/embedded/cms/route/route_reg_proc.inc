#ifndef __ROUTE_REG_PROC_H__
#define __ROUTE_REG_PROC_H__


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

    /* 路由注册消息结构体 */
    typedef struct _route_reg_msg_t
    {
        char* route_id;
        int reg_info_index;
    } route_reg_msg_t;

    /* 路由注册消息队列 */
    typedef struct _route_reg_msg_list_t
    {
        osip_list_t* pRouteRegMsgList;
#ifdef MULTI_THR
        osip_mutex_t* lock;
#endif
    } route_reg_msg_list_t;

    extern void scan_route_reg_msg_list();
    extern int route_reg_msg_add(char* route_id, int reg_info_index);
    extern void route_reg_msg_free(route_reg_msg_t* route_reg_msg);
    extern int route_reg_msg_init(route_reg_msg_t** route_reg_msg);
    extern void route_reg_msg_list_free();
    extern int route_reg_msg_list_init();
    extern int route_reg_msg_list_lock();
    extern int route_reg_msg_list_unlock();
    extern int route_reg_msg_proc(char* route_id, int reg_info_index);
    extern int route_reg_msg_remove(int pos);
    extern int debug_route_reg_msg_list_lock(const char* file, int line, const char* func);
    extern int debug_route_reg_msg_list_unlock(const char* file, int line, const char* func);

#ifdef ENABLE_DEBUG_LOCK
#define ROUTE_REGMSG_SMUTEX_LOCK() debug_route_reg_msg_list_lock(__FILE__, __LINE__, __func__)
#define ROUTE_REGMSG_SMUTEX_UNLOCK() debug_route_reg_msg_list_unlock(__FILE__, __LINE__, __func__)
#else
#define ROUTE_REGMSG_SMUTEX_LOCK() route_reg_msg_list_lock()
#define ROUTE_REGMSG_SMUTEX_UNLOCK() route_reg_msg_list_unlock()
#endif

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif /* __UDP_TL_H__ */
