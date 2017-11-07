#ifndef __ROUTE_INFO_MGN_H__
#define __ROUTE_INFO_MGN_H__


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
    typedef struct _route_info_t
    {
        unsigned int id;                                        /* 路由索引 */
        char server_id[MAX_ID_LEN + 4];                         /* 上级服务器CMS统一编号ID */
        char server_ip[MAX_IP_LEN];                             /* 上级服务器CMS IP 地址*/
        char server_host[MAX_128CHAR_STRING_LEN + 4];           /* 上级服务器CMS 域名地址 */
        int server_port;                                        /* 上级服务器CMS 端口号*/
        char register_account[MAX_128CHAR_STRING_LEN + 4];      /* 注册账号 */
        char register_password[MAX_128CHAR_STRING_LEN + 4];     /* 注册密码 */
        int link_type;                                          /* 联网类型:0:上下级，1：同级，默认0 */
        int three_party_flag;                                   /* 是否是第三方的平台:0:不是，1:是 */
        int trans_protocol;                                     /* 媒体流传输类型:0:UDP，1:TCP */
        int access_method;                                      /* 接入方式: 0:国标，1:私有协议，默认0 */
        int tcp_sock;                                           /* 私有协议接入时候的对应的TCP Socket */
        int ip_is_in_sub;                                       /* IP地址是否是下级设备的IP地址:0:不是，1:是  */
        char strRegLocalEthName[MAX_IP_LEN];                    /* 注册的本地IP地址网口名称 */
        char strRegLocalIP[MAX_IP_LEN];                         /* 注册的本地IP地址 */
        int iRegLocalPort;                                      /* 注册的本地端口号 */
        int del_mark;                                           /* 删除标识 :0: 无变化，1:需要重新注册，2:需要删除 */
        int keep_alive_count;                                   /* 保活时间计数 */
        int failed_keep_alive_count;                            /* 保活失败计数 */
        int catalog_subscribe_flag;                             /* 目录订阅标识:0:没有订阅，1:已订阅 */
        int catalog_subscribe_expires;                          /* 目录订阅超时时间 */
        int catalog_subscribe_expires_count;                    /* 目录订阅时间计数 */
        int catalog_subscribe_event_id;                         /* 目录订阅事件ID */
        int catalog_subscribe_dialog_index;                     /* 目录订阅事件会话索引 */
        int reg_status;                                         /* 注册状态 */
        int reg_interval;                                       /* 注册间隔时间*/
        int expires;                                            /* 注册超时时间*/
        int min_expires;                                        /* 最小注册超时时间*/
        int reg_info_index;                                     /* 对应的SIP协议栈中注册信息的Index */
        unsigned int keep_alive_sn;                             /* 保活消息的sn */

        int catlog_get_status;                                  /* 上级CMS获取catalog状态 */

        int CataLogNumCount;                                    /* 上级CMS推送的逻辑通道个数 */
        unsigned int CataLogSN;                                 /* 上级CMS推送的Catalog XML消息的SN */
    } route_info_t;

    /* 路由信息队列 */
    typedef struct _route_info_list_t
    {
        osip_list_t* pRouteInfoList;
#ifdef MULTI_THR
        osip_mutex_t* lock;
#endif
    } route_info_list_t;

    /* 待处理路由信息队列 */
    typedef std::deque <route_info_t*> needtoproc_routeinfo_queue;

    extern int checkRoutIfHasSendCataProc();
    extern void check_MMSRouteInfoConfig_need_to_reload_begin();
    extern void check_MMSRouteInfoConfig_need_to_reload_end();
    extern void check_RouteInfoConfig_need_to_reload_begin(DBOper* pDboper);
    extern void check_RouteInfoConfig_need_to_reload_end();
    extern int check_route_info_from_db_to_list(DBOper* pRoute_Srv_dboper);
    extern int debug_route_info_list_lock(const char* file, int line, const char* func);
    extern int debug_route_info_list_unlock(const char* file, int line, const char* func);
    extern int delete_mms_route_info_from_list_by_mark();
    extern int delete_route_info_from_list_by_mark();
    extern int IsRouteHasPermissionForDevice(unsigned int device_index, route_info_t* pRouteInfo, DBOper* pDboper);
    extern int MMSRouteInfoConfig_refresh_proc();
    extern int RouteGetGBDeviceListAndSendNotifyCatalog(route_info_t* pRouteInfo, DBOper* pRoute_Srv_dboper);
    extern int RouteGetGBDeviceListAndSendNotifyCatalogTo3PartyCMS(route_info_t* pRouteInfo, DBOper* pRoute_Srv_dboper);
    extern int RouteGetGBDeviceListAndSendNotifyCatalogToOwnerCMS(route_info_t* pRouteInfo, DBOper* pRoute_Srv_dboper);
    extern int RouteInfoConfig_db_refresh_proc();
    extern int route_info_add(route_info_t* pRouteInfo);
    extern int route_info_find(char* server_id);
    extern int route_info_find_by_host_and_port(char* server_host, int server_port);
    extern int route_info_find_by_reg_index(int reg_info_index);
    extern int route_info_find_by_route_index(unsigned int route_index);
    extern void route_info_free(route_info_t* route_info);
    extern route_info_t* route_info_get(int pos);
    extern int route_info_init(route_info_t** route_info);
    extern void route_info_list_free();
    extern int route_info_list_init();
    extern int route_info_list_lock();
    extern int route_info_list_unlock();
    extern int route_info_remove(int pos);
    extern void scan_route_info_list();
    extern int SendAllDeviceCatalogTo3PartyRouteCMS(route_info_t* pRouteInfo, vector<string>& DeviceIDVector, int iEvent, DBOper* pDboper);
    extern int SendAllDeviceCatalogToOwnerRouteCMS(route_info_t* pRouteInfo, vector<string>& DeviceIDVector, int iEvent, DBOper* pDboper);
    extern int SendAllDeviceCatalogToRouteCMS(vector<string>& DeviceIDVector, int iEvent, DBOper* pDboper);
    extern int SendAllDeviceStatusTo3PartyRouteCMS(route_info_t* pRouteInfo, vector<string>& DeviceIDVector, int status);
    extern int SendAllDeviceStatusToOwnerRouteCMS(route_info_t* pRouteInfo, vector<string>& DeviceIDVector, int status);
    extern int SendAllDeviceStatusToRouteCMS(vector<string>& DeviceIDVector, int status);
    extern int SendAllOfflineDeviceStatusTo3PartyRouteCMS(route_info_t* pRouteInfo);
    extern int SendDeviceInfoMessageToRouteCMS(char* callee_id, char* local_ip, int local_port, char* remote_ip, int remote_port, char* strSN, char* device_id, int three_party_flag, int trans_protocol);
    extern int SendDeviceStatusToRouteCMS(unsigned int device_index, char* device_id, int status, DBOper* pDboper);
    extern int SendGetServerIDMessageToRouteCMS(route_info_t* pRouteInfo);
    extern int SendKeepAliveMessageToRouteCMS(route_info_t* pRouteInfo);
    extern int SendMessageToOwnerRouteCMS(unsigned int device_index, char* callee_id, char* msg, int msg_len, DBOper* pDboper);
    extern int SendMessageToOwnerRouteCMS2(char* callee_id, char* msg, int msg_len, DBOper* pDboper);
    extern int SendMessageToOwnerRouteCMSExceptMMS(char* callee_id, char* msg, int msg_len);
    extern int SendMessageToOwnerRouteCMSExceptMMS2(unsigned int device_index, char* callee_id, char* msg, int msg_len, DBOper* pDboper);
    extern int SendMessageToRouteCMS(unsigned int device_index, char* callee_id, char* msg, int msg_len, DBOper* pDboper);
    extern int SendNotifyCatalogMessageToRouteCMS(unsigned int route_index);
    extern int SendNotifyCatalogStatusTo3PartyRouteCMS(route_info_t* pRouteInfo, char* device_id, int status);
    extern int SendNotifyCatalogStatusTo3PartyRouteCMSByTCP(route_info_t* pRouteInfo, char* device_id, int status, int tcp_socket);
    extern int SendNotifyGroupCatalogTo3PartyRouteCMS(char* group_id, char* group_name, char* parent_id, int iEvent);
    extern int SendNotifyGroupCatalogToOwnerRouteCMS(char* group_id, char* group_name, char* parent_id, int sort_id, int iEvent);
    extern int SendNotifyGroupMapToOwnerRouteCMS(char* group_id, unsigned int device_index, int sort_id, int iEvent);
    extern int SendNotifyRestartMessageToAllRoutCMS();
    extern int SendNotifyTo3PartyRouteCMS(char* callee_id, char* msg, int msg_len);
    extern int SendNotifyTo3PartyRouteCMS2(unsigned int device_index, char* callee_id, char* msg, int msg_len, DBOper* pDboper);
    extern int SendNotifyTopologyPhyDeviceToRouteCMS(int event, char* strDeviceID, char* strDeviceName, char* strDeviceType, char* strDeviceIP, char* strStatus, char* strCMSID, char* strLinkType);
    extern int SendUnRegisterToAllRouteCMS();
    extern int SetGBLogicDeviceInfoDelFlagForRoute(route_info_t* pRouteInfo);
    extern int SetGBLogicDeviceInfoEnableFlagByDelMarkForRoute(route_info_t* pRouteInfo, DBOper* pDevice_Srv_dboper);
    extern int set_mms_route_info_list_del_mark(int del_mark);
    extern int set_route_info_list_del_mark(int del_mark);
    extern void ShowRouteInfo(int sock, int type);

#ifdef ENABLE_DEBUG_LOCK
#define ROUTE_INFO_SMUTEX_LOCK() debug_route_info_list_lock(__FILE__, __LINE__, __func__)
#define ROUTE_INFO_SMUTEX_UNLOCK() debug_route_info_list_unlock(__FILE__, __LINE__, __func__)
#else
#define ROUTE_INFO_SMUTEX_LOCK() route_info_list_lock()
#define ROUTE_INFO_SMUTEX_UNLOCK() route_info_list_unlock()
#endif

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif /* __UDP_TL_H__ */
