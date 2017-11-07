#ifndef __PLATFORM_INFO_MGN_H__
#define __PLATFORM_INFO_MGN_H__


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

    /* 路由信息结构体 */
    typedef struct _platform_info_t
    {
        unsigned int id;                                        /* 索引 */
        char platform_ip[MAX_IP_LEN];                           /* 管理平台 IP 地址*/
        int del_mark;                                           /* 删除标识 :0: 无变化，1:ip地址有变化，2:需要删除, 3:等待删除 */
    } platform_info_t;

    /* 路由信息队列 */
    typedef struct _platform_info_list_t
    {
        osip_list_t* pPlatformInfoList;
#ifdef MULTI_THR
        osip_mutex_t* lock;
#endif
    } platform_info_list_t;

    /* 待处理路由信息队列 */
    typedef std::deque <platform_info_t*> needtoproc_platforminfo_queue;

    extern void check_PlatformInfoConfig_need_to_reload_begin(DBOper* pDboper);
    extern void check_PlatformInfoConfig_need_to_reload_end();
    extern int check_platform_info_from_db_to_list(DBOper* pPlatform_Srv_dboper);
    extern int debug_platform_info_list_lock(const char* file, int line, const char* func);
    extern int debug_platform_info_list_unlock(const char* file, int line, const char* func);
    extern int delete_platform_info_from_list_by_mark();
    extern int PlatformInfoConfig_db_refresh_proc();
    extern int platform_info_add(platform_info_t* pPlatformInfo);
    extern int platform_info_find(char* platform_ip);
    extern int platform_info_find_by_platform_index(unsigned int platform_index);
    extern void platform_info_free(platform_info_t* platform_info);
    extern platform_info_t* platform_info_get(int pos);
    extern int platform_info_init(platform_info_t** platform_info);
    extern void platform_info_list_free();
    extern int platform_info_list_init();
    extern int platform_info_list_lock();
    extern int platform_info_list_unlock();
    extern int platform_info_remove(int pos);
    extern void scan_platform_info_list();
    extern int set_platform_info_list_del_mark(int del_mark);
    extern void ShowPlatformInfo(int sock, int type);

#ifdef ENABLE_DEBUG_LOCK
#define PLATFORM_INFO_SMUTEX_LOCK() debug_platform_info_list_lock(__FILE__, __LINE__, __func__)
#define PLATFORM_INFO_SMUTEX_UNLOCK() debug_platform_info_list_unlock(__FILE__, __LINE__, __func__)
#else
#define PLATFORM_INFO_SMUTEX_LOCK() platform_info_list_lock()
#define PLATFORM_INFO_SMUTEX_UNLOCK() platform_info_list_unlock()
#endif

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif /* __UDP_TL_H__ */
