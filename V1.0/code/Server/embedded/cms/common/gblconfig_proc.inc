/******************************************************************************

                  版权所有 (C), 2001-2013, 金智视讯技术有限公司

 ******************************************************************************
  文 件 名   : rcconfig.h
  版 本 号   : 初稿
  作    者   : yanghaifeng
  生成日期   : 2013年4月1日
  最近修改   :
  功能描述   : rcconfig.c 的头文件
  函数列表   :
  修改历史   :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 创建文件

******************************************************************************/
#ifndef __GBLCONFIG_PROC_H__
#define __GBLCONFIG_PROC_H__

/*----------------------------------------------*
 * 包含头文件                                   *
 *----------------------------------------------*/
#include "../../../common/include/EV9000_CommonManage.h"
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

    /* 本地IP地址结构信息 */
    typedef struct ip_pair_t
    {
        char eth_name[MAX_IP_LEN];       /* 本地网卡名称 */
        ip_addr_type_t ip_type;          /* ip地址类型 */
        char local_ip[MAX_IP_LEN];       /* 本地IP地址 */
        int local_port;                  /* 本地端口号*/
    } ip_pair_t;

    /* 基层分组结构信息 */
    typedef struct primary_group_t
    {
        char group_id[32 + 4];                  /* 分组 ID */
        char group_name[64 + 4];                /* 分组名称，基层接入名称(派出所) */
        char parent_id[32 + 4];                 /* 上级 ID */
        int iSortID;                            /* 排序ID */
        char civil_code[8 + 4];                 /* 基层接入编码, 8位 */
        char group_code[MAX_ID_LEN + 4];        /* 分组编码(虚拟组织编码)，20位 */
        char parent_code[MAX_ID_LEN + 4];       /* 上级分组编码(虚拟组织编码)，20位 */
        int iNeedToUpLoad;                      /* 是否需要上报给上级平台 */
        int del_mark;                           /* 删除标识 */
    } primary_group_t;

    /* 逻辑设备分组信息队列 */
    typedef std::map <string, primary_group_t*> PrimaryGroup_MAP;
    typedef std::map <string, primary_group_t*>::iterator PrimaryGroup_Iterator;

    /* 逻辑设备分组关系信息结构体 */
    typedef struct _device_group_map_t
    {
        char group_id[32 + 4];                  /* 分组ID */
        unsigned int device_index;              /* 点位索引 */
        int iSortID;                            /* 排序ID */
        int del_mark;                           /* 删除标识 */
    } device_group_map_t;

    /* 逻辑设备分组关系信息队列 */
    typedef std::map <unsigned int, device_group_map_t*> DeviceGroupMap_MAP;
    typedef std::map <unsigned int, device_group_map_t*>::iterator DeviceGroupMap_Iterator;

    typedef struct _gbl_conf_t
    {
        unsigned int board_index;                           /* CMS单板索引 */
        char board_id[MAX_ID_LEN + 4];                      /* CMS 单板编号 */
        char mms_id[MAX_ID_LEN + 4];                        /* MMS 单板编号 */

        char center_code[12];                               /* 中心编码 */

        char province_code[4];                              /* 省级编码,2位 */

        char city_code[8];                                  /* 市级编码,4位 */

        char region_code[12];                               /* 区县编码,6位 */

        char civil_code[8 + 4];                             /* 行政区域编码 */

        PrimaryGroup_MAP PrimaryGroupList;                  /* 本地基层分组队列 */
        DeviceGroupMap_MAP DeviceGroupMapList;              /* 本地的逻辑设备分组关系队列 */

        char trade_code[4];                                 /* 行业编码 */
        char type_code[4];                                  /* 类型编码 */
        char serial_number[8];                              /* 序号 */

        char register_region[12];                           /* 注册域 */

        char alarm_server_ip[MAX_IP_LEN];                   /* 告警服务器ip地址 */
        char ntp_server_ip[MAX_IP_LEN];                     /* NTP 服务器ip地址 */
        char db_server_ip[MAX_IP_LEN];                      /* 数据库 服务器ip地址 */

        osip_list_t* pLocalIPAddrList;                      /* 本地IP地址队列 */

        char default_eth_name[MAX_IP_LEN];                  /* 默认的IP地址网口名称 */

        char cms_name[MAX_128CHAR_STRING_LEN + 4];          /* CMS别名 */

        /* SPY */
        char spy_username[MAX_128CHAR_STRING_LEN + 4];      /* telnet 登录用户名 */
        char spy_password[MAX_128CHAR_STRING_LEN + 4];      /* telnet 登录密码 */
        int spy_port;                                       /* telnet 服务端口*/

        unsigned int iAuthFlag;                             /* 认证标识:0:不需要认证，1:需要认证 */

        unsigned int uPresetBackTime;                       /* 预置位自动归位时间 */
        unsigned int uDeviceUnLockTime;                     /* 点位自动解锁时间 */

        unsigned int register_retry_interval;               /* 未注册成功，重新注册时间间隔, 默认60秒 */
        unsigned int registry_cleanuprate;                  /* 注册刷新周期，默认180秒 */
        unsigned int session_expires;                       /* 会话超时时间，默认300秒 */
        unsigned int subscribe_expires;                     /* 订阅超时时间，默认3600秒 */
        unsigned int keep_alive_interval;                   /* 保活间隔时间，默认60秒 */
        unsigned int failed_keep_alive_count;               /* 保活失败次数，默认3次 */
        unsigned int keep_alive_expires;                    /* 保活超时时间，默认3分钟 */
        unsigned int alarm_duration;                        /* 告警延时时间，默认30秒 */

        char shdb_agent_id[MAX_16CHAR_STRING_LEN + 4];      /* 上海地标监管平台注册ID */
        char shdb_server_ip[MAX_IP_LEN];                    /* 上海地标监管平台服务器IP */
        unsigned int shdb_prex_seconds;                     /* 上海地标报警图片上传前N秒, 默认60秒 */
        unsigned int shdb_next_seconds;                     /* 上海地标报警图片上传后M秒，默认60秒 */
        unsigned int shdb_interval_seconds;                 /* 上海地标报警图片上传间隔P秒，默认3秒 */

        unsigned int sys_exit_flag;                         /* 系统退出标识，0，正常退出；1，异常退出 */
        unsigned int showcode;                              /* showcode = 0 非国标版本，ShowCode = 1和无配置 为国标版本*/
    } gbl_conf_t;

    /*----------------------------------------------*
     * 宏定义                                       *
     *----------------------------------------------*/

#define MIN_REGISTER_RETRY_INTERVAL         60      /* 最小重新注册间隔时间,单位:秒 */
#define MIN_REGISTER_EXPIRE                 3600    /* 最小注册间隔时间,单位:秒 */
#define MIN_SESSION_EXPIRE                  300     /* 最小会话刷新时间,单位:秒 */
#define MIN_SUBSCRIBE_EXPIRE                3600    /* 最小订阅刷新时间,单位:秒 */
#define MIN_KEEP_ALIVE_INTERVAL             60      /* 最小保活时间,单位:秒 */

#define DEFAULT_DEVICE_KEEP_ALIVE_EXPIRES   3       /* 设备默认保活超时时间,单位:分钟 */

#define DEFAULT_PRESET_BACK_TIME            30      /* 默认的预置位自动归位时间,单位:秒 */
#define DEFAULT_DEVICE_UNLOCK_TIME          60      /* 默认的点位自动解锁时间,单位:秒 */
#define DEFAULT_ALARM_DURATION              30      /* 默认的告警延时时间,单位:秒 */
#define DEFAULT_TSU_SESSION_EXPIRE          30      /* 默认的和TSU会话的保活超时时间,单位:分钟 */
#define DEFAULT_FAILED_KEEP_ALIVE_COUNT     3       /* 默认保活失败次数 */

#define DEFAULT_SHDB_DURATION_SECONDS       60      /* 上海地标报警图片上传默认持续时间,单位:秒 */
#define DEFAULT_SHDB_INTERVAL_SECONDS       3       /* 上海地标报警图片上传默认间隔时间,单位:秒 */

    extern int AddGblLogicDeviceGroupToVectorForRoute(vector<string>& GroupIDVector);
    extern int change_conf_to_config_file(char* pcName, char* pcValue);
    extern int change_conf_to_web_config_file(char* pcName, char* pcValue);
    extern int checkExpireDay(char *ExpireDay);
    extern int checklicence();
    extern int checkLoginIPIsSlaveIP(char* host);
    extern int checkTmpLicence();
    extern void check_GBLogicDeviceGroupInfoConfig_need_to_reload(DBOper* pDboper);
    extern void check_GBLogicDeviceGroupMapInfoConfig_need_to_reload(DBOper* pDboper);
    extern int conf_reset(gbl_conf_t* conf, char* pname, char* pvalue, int iRefresh);
    extern int createNewTmpLicence();
    extern int debug_device_map_group_info_list_lock(const char* file, int line, const char* func);
    extern int debug_device_map_group_info_list_unlock(const char* file, int line, const char* func);
    extern int debug_primary_group_info_list_lock(const char* file, int line, const char* func);
    extern int debug_primary_group_info_list_unlock(const char* file, int line, const char* func);
    extern char* default_eth_name_get();
    extern int DelGblLogicDeviceGroupInfoByDelMark();
    extern int DelGblLogicDeviceGroupMapInfoByDelMark();
    extern void device_group_map_free(device_group_map_t* device_group_map);
    extern int device_group_map_init(device_group_map_t** device_group_map);
    extern int device_map_group_add(char* group_id, unsigned int device_index, int sort_id);
    extern int device_map_group_info_list_lock();
    extern int device_map_group_info_list_unlock();
    extern int GBLogicDeviceGroupInfoConfig_db_refresh_proc();
    extern int GBLogicDeviceGroupMapInfoConfig_db_refresh_proc();
    extern void gbl_conf_free();
    extern int gbl_conf_init();
    extern int gbl_conf_load();
    extern int gbl_conf_reload();
    extern char* GetGBLogicDeviceCivilCode(unsigned int device_index, DBOper* pDbOper);
    extern primary_group_t* GetPrimaryGroupInfoByGBLogicDeviceIndex(unsigned int device_index, DBOper* pDbOper);
    extern char *GetRegPasswd(char *name);
    extern device_group_map_t* get_device_map_group(unsigned int device_index);
    extern char* get_ip_eth_name(char* server_ip);
    extern ip_addr_type_t get_local_ip_type(char* eth_name);
    extern primary_group_t* get_primary_group(char* group_id);
    extern int ip_pair_add(char* eth_name, ip_addr_type_t ip_type, char* local_ip, int local_port);
    extern int ip_pair_clone(const ip_pair_t* uparam, ip_pair_t** dest);
    extern void ip_pair_free(ip_pair_t* ip_pair);
    extern int ip_pair_init(ip_pair_t** ip_pair);
    extern int is_need_auth();
    extern int load_file_config();
    extern int local_alarm_duration_get();
    extern char* local_civil_code_get();
    extern char* local_cms_id_get();
    extern unsigned int local_cms_index_get();
    extern char* local_cms_name_get();
    extern int local_device_unlock_time_get();
    extern int local_failed_keep_alive_count_get();
    extern char* local_ip_get(char* eth_name);
    extern int local_keep_alive_expires_get();
    extern int local_keep_alive_interval_get();
    extern char* local_mms_id_get();
    extern int local_port_get(char* eth_name);
    extern int local_preset_back_time_get();
    extern int local_register_retry_interval_get();
    extern int local_registry_cleanuprate_get();
    extern int local_session_expires_get();
    extern int local_subscribe_expires_get();
    extern char* ntp_server_ip_get();
    extern int primary_group_add(char* group_id, char* group_name, char* civil_code, char* group_code, char* parent_code, int sort_id, char* parent_id, int iNeedToUpLoad);
    extern void primary_group_free(primary_group_t* primary_group);
    extern int primary_group_info_list_lock();
    extern int primary_group_info_list_unlock();
    extern int primary_group_init(primary_group_t** primary_group);
    extern void refresh_config_file();
    extern int reload_file_config();
    extern int SetGblLogicDeviceGroupDelFlag(int iDelFlag);
    extern int SetGblLogicDeviceGroupMapDelFlag(int iDelFlag);
    extern char* shdb_agent_id_get();
    extern int shdb_interval_sec_get();
    extern int shdb_next_sec_get();
    extern int shdb_prex_sec_get();
    extern char* shdb_server_ip_get();
    extern void show_system_civil_info(int sock);
    extern void show_system_gbl_param(int sock);
    extern void show_system_group_map_info(int sock);
    extern void show_system_ip_info(int sock);
    extern int sys_exit_flag_get();
    extern int sys_show_code_flag_get();
    extern int write_default_config_file();
    extern int write_default_web_config_file();

#ifdef ENABLE_DEBUG_LOCK
#define PRIMARY_GROUP_SMUTEX_LOCK() debug_primary_group_info_list_lock(__FILE__, __LINE__, __func__)
#define PRIMARY_GROUP_SMUTEX_UNLOCK() debug_primary_group_info_list_unlock(__FILE__, __LINE__, __func__)
#define DEVICE_MAP_GROUP_MAP_SMUTEX_LOCK() debug_device_map_group_info_list_lock(__FILE__, __LINE__, __func__)
#define DEVICE_MAP_GROUP_MAP_SMUTEX_UNLOCK() debug_device_map_group_info_list_unlock(__FILE__, __LINE__, __func__)
#else
#define PRIMARY_GROUP_SMUTEX_LOCK() primary_group_info_list_lock()
#define PRIMARY_GROUP_SMUTEX_UNLOCK() primary_group_info_list_unlock()
#define DEVICE_MAP_GROUP_MAP_SMUTEX_LOCK() device_map_group_info_list_lock()
#define DEVICE_MAP_GROUP_MAP_SMUTEX_UNLOCK() device_map_group_info_list_unlock()
#endif

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif /* __RCCONFIG_H__ */
