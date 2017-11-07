#ifndef __DEVICE_INFO_MGN_H__
#define __DEVICE_INFO_MGN_H__


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

#include "user/user_info_mgn.inc"
#include "route/route_info_mgn.inc"

#include <vector>
#include <string>
#include <map>

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

//added by chenyu 131127
    /* 标准物理设备cfg 结构体 */
    typedef struct _GBDevice_cfg_t
    {
        int id;                              /* 设备索引 */
        string device_id;                    /* 设备统一编号 */
        int enable;                          /* 是否启用 */
        int device_type;                     /* 设备类型(前端摄像机、NVR、互联CMS、TSU) */
        string register_account;             /* 注册账号 */
        string register_password;            /* 注册密码 */

        string device_ip;                    /* 设备ip地址 */
        int device_max_camera;               /* 设备视频输入通道数 */
        int device_max_alarm;                /* 设备报警输入通道数 */
        string device_manufacturer;          /* 设备生产商 */
        string device_model;                 /* 设备型号 */
        string device_firmware;              /* 设备版本 */
        int link_type;                       /* 联网类型: 0:上下级，1：同级，默认0 */
        string cms_id;                       /* 本地CMS ID */
        int trans_protocol;                  /* 传输方式: 0:UDP，1：TCP，默认0 */

        string login_ip;                     /* 登录的ip地址*/
        int login_port;                      /* 登录的端口号*/
        int reg_status;                      /* 注册状态 */
        int reg_info_index;                  /* 对应的SIP协议栈中注册信息的Index */
        int auth_count;                      /* 认证失败次数 */
        _GBDevice_cfg_t()
        {
            id = 0;
            enable = 0;
            //device_type_t device_type;          /* 设备类型(前端摄像机、NVR、互联CMS、TSU) */
            register_account = "";
            register_password = "";
            device_ip = "";
            device_max_alarm = 0;
            device_manufacturer = "";
            device_model = "";
            device_firmware = "";
            link_type = 0;
            cms_id = "";
            trans_protocol = 0;

            login_ip = "";
            login_port = 0;
            reg_status = 0;
            reg_info_index = -1;
            auth_count = 0;
        }

    }
    GBDevice_cfg_t;

    /* 逻辑设备分组信息结构体 */
    typedef struct _LogicDeviceGroup_t
    {
        char GroupID[32 + 4];
        char CMSID[MAX_ID_LEN + 4];
        char Name[64 + 4];
        int SortID;
        char ParentID[32 + 4];
        char iChangeFlag;               /* 变化标识:0:无变化, 1:增加，2:修改，3:删除 */
    } LogicDeviceGroup_t;

    /* 逻辑设备分组信息队列 */
    typedef std::map <string, LogicDeviceGroup_t*> LogicDeviceGroup_MAP;
    typedef std::map <string, LogicDeviceGroup_t*>::iterator LogicDeviceGroup_Iterator;

    /* 逻辑设备分组关系信息结构体 */
    typedef struct _LogicDeviceMapGroup_t
    {
        char GroupID[32 + 4];
        char CMSID[MAX_ID_LEN + 4];
        unsigned int DeviceIndex;
        int SortID;
        char iChangeFlag;               /* 变化标识:0:无变化, 1:增加，2:修改，3:删除 */
    } LogicDeviceMapGroup_t;

    /* 逻辑设备分组关系信息队列 */
    typedef std::map <unsigned int, LogicDeviceMapGroup_t*> LogicDeviceMapGroup_MAP;
    typedef std::map <unsigned int, LogicDeviceMapGroup_t*>::iterator LogicDeviceMapGroup_Iterator;

    /* 标准物理设备信息结构体 */
    typedef struct _GBDevice_info_t
    {
        int id;                                          /* 设备索引 */
        char device_id[MAX_ID_LEN + 4];                  /* 设备统一编号 */
        int device_type;                                 /* 设备类型(前端摄像机、NVR、互联CMS、TSU) */
        int link_type;                                   /* 联网类型: 0:上下级，1：同级，默认0 */
        int access_method;                               /* 接入方式: 0:国标，1:私有协议，默认0 */
        int tcp_sock;                                    /* 私有协议接入时候的对应的TCP Socket */
        char login_ip[MAX_IP_LEN];                       /* 登录的ip地址*/
        int login_port;                                  /* 登录的端口号*/
        int reg_status;                                  /* 注册状态 */
        int reg_info_index;                              /* 国标接入的时候:对应的SIP协议栈中注册信息的Index, 私有协议接入的时候:对应的TCP连接中的Index */
        int auth_count;                                  /* 认证失败次数 */
        int last_keep_alive_time;                        /* 最后保活时间 */
        int keep_alive_expires;                          /* 保活超时时间 */
        int keep_alive_expires_count;                    /* 保活超时时间计数 */

        char manufacturer[MAX_128CHAR_STRING_LEN + 4];   /* 设备生产商 */

        int three_party_flag;                            /* 是否是第三方的平台:0:不是，1:是 */
        int catalog_subscribe_flag;                      /* 目录订阅标识:0:没有订阅，1:已订阅 */
        int catalog_subscribe_interval;                  /* 目录订阅未成功的重试间隔时间 */
        int catalog_subscribe_expires;                   /* 目录订阅超时时间*/
        int catalog_subscribe_expires_count;             /* 目录订阅时间计数 */
        unsigned int catalog_subscribe_event_id;         /* 目录订阅事件计数 */
        unsigned int catalog_subscribe_dialog_index;     /* 目录订阅对应的会话索引 */
        int trans_protocol;                              /* 媒体流传输类型:0:UDP，1:TCP */

        char strRegServerEthName[MAX_IP_LEN];            /* 注册的服务器IP地址网口名称 */
        char strRegServerIP[MAX_IP_LEN];                 /* 注册的服务器IP地址 */
        int iRegServerPort;                              /* 注册的服务器端口号 */

        char call_id[MAX_128CHAR_STRING_LEN + 4];        /* 注册的句柄callid,标识是否是刷新注册 */

        int CataLogNumCount;                             /* 物理设备上报的逻辑通道个数 */
        unsigned int CataLogSN;                          /* 获取物理设备XML消息的SN */
        int iGetCataLogStatus;                           /* 物理设备上报的逻辑通道状态:0:没有获取，1:已经获取，2:正在处理返回信息 */
        time_t iLastGetCataLogTime;                      /* 上次获取物理设备逻辑通道时间 */
        int iGetLogicDeviceStatusCount;                  /* 获取前端物理设备下面逻辑点位状态次数计数 */
        int keep_alive_count;                            /* 保活次数计数 */

        int LogicDeviceGroupConfigCount;                 /* 下级上报的逻辑设备分组配置个数 */
        unsigned int LogicDeviceGroupSN;                 /* 获取逻辑设备分组XML消息的SN */
        LogicDeviceGroup_MAP LogicDeviceGroupList;       /* 下级上报的逻辑设备分组信息队列 */

        int LogicDeviceMapGroupConfigCount;              /* 下级上报的逻辑设备分组关系配置个数 */
        unsigned int LogicDeviceMapGroupSN;              /* 获取逻辑设备分组关系XML消息的SN */
        LogicDeviceMapGroup_MAP LogicDeviceMapGroupList; /* 下级上报的逻辑设备分组关系信息队列 */

        unsigned int del_mark;                           /* 删除标识:0,不删除，1:删除 */

        unsigned int  nResved1;                          /* 保留1 */
        char   strResved2[MAX_32CHAR_STRING_LEN + 4];    /* 保留2 */
    } GBDevice_info_t;

    /* 标准物理设备信息队列 */
    typedef std::map <string, GBDevice_info_t*> GBDevice_Info_MAP;
    typedef std::map <string, GBDevice_info_t*>::iterator GBDevice_Info_Iterator;

    /* 待处理标准物理设备信息队列 */
    typedef std::deque <GBDevice_info_t*> needtoproc_GBDeviceinfo_queue;

    typedef struct _GBDevice_t
    {
        int stream_type;                      /* 媒体流类型：1:主视频流，2:从视频流，3:分析流 */
        GBDevice_info_t* ptGBDeviceInfo;      /* 对应的标准物理设备信息 */
    } GBDevice_t;

    /* 标准逻辑设备信息结构体 */
    typedef struct _GBLogicDevice_info_t
    {
        unsigned int id;                                    /* 点位索引 */
        char device_id[MAX_ID_LEN + 4];                     /* 点位统一编号 */
        char device_name[MAX_128CHAR_STRING_LEN + 4];       /* 点位名称 */
        int enable;                                         /* 是否启用 */
        int device_type;                                    /* 设备类型 */
        unsigned int alarm_device_sub_type;                 /* 报警设备子类型 EV9000_ALARM_DEVICE_MJ,使用数据库Resved1字段 */
        int ctrl_enable;                                    /* 摄像机类型：1-球机；2-半球；3-固定枪机； 4-遥控枪机。当目录项为摄像机时可选 */
        int mic_enable;                                     /* 是否支持对讲，默认值0 */
        int frame_count;                                    /* 帧率，默认25 */
        int alarm_duration;                                 /* 告警时长 */
        int phy_mediaDeviceIndex;                           /* 对应的媒体物理设备索引 */
        int phy_mediaDeviceChannel;                         /* 对应的媒体物理设备通道 */
        int phy_ctrlDeviceIndex;                            /* 对应的控制物理设备索引 */
        int phy_ctrlDeviceChannel;                          /* 对应的控制物理设备通道 */
        int stream_count;                                   /* 是否支持多码流 */
        int record_type;                                    /* 录像类型 */
        int guard_type;                                     /* 布放，撤防状态:0:撤防，1:布防,默认布防 */
        int other_realm;                                    /* 是否是其他域的点位，默认0，如果是1，表示是从上级CMS推送过来的其他域的点位  */

        char manufacturer[MAX_128CHAR_STRING_LEN + 4];      /* 设备生产商 */
        char model[MAX_128CHAR_STRING_LEN + 4];             /* 设备型号 */
        char owner[MAX_128CHAR_STRING_LEN + 4];             /* 设备归属 */
        char civil_code[MAX_128CHAR_STRING_LEN + 4];        /* 行政区域 */
        char block[MAX_128CHAR_STRING_LEN + 4];             /* 警区 */
        char address[MAX_128CHAR_STRING_LEN + 4];           /* 安装地址 */
        int parental;                                       /* 是否有子设备 */
        char parentID[MAX_128CHAR_STRING_LEN + 4];          /* 父设备/区域/系统ID */
        char virtualParentID[MAX_128CHAR_STRING_LEN + 4];   /* 虚拟组织ID,上报给第三方平台使用 */
        int safety_way;                                     /* 信令安全模式*/
        int register_way;                                   /* 注册方式 */

        char cert_num[MAX_128CHAR_STRING_LEN + 4];          /* 证书序列号*/
        int certifiable;                                    /* 证书有效标识 */
        int error_code;                                     /* 无效原因码 */
        char end_time[MAX_128CHAR_STRING_LEN + 4];          /* 证书终止有效期*/

        int secrecy;                                        /* 保密属性 */

        char ip_address[MAX_IP_LEN];                        /* IP地址*/
        int port;                                           /* 端口号 */
        char password[MAX_128CHAR_STRING_LEN + 4];          /* 密码*/

        int status;                                         /* 点位状态:0，掉线，1:在线，2: 没有码流,3: 不可达 */
        intelligent_status_t intelligent_status;            /* 智能分析状态:0，没有分析，1:分析 */
        alarm_status_t alarm_status;                        /* 报警状态:0，CLOSE，1:APART */
        lock_status_t lock_status;                          /* 锁定状态:0，不锁定，1:用户锁定，2:上级锁定 */
        user_info_t* pLockUserInfo;                         /* 对应的锁定的用户信息 */
        route_info_t* pLockRouteInfo;                       /* 对应的锁定的上级路由信息 */

        double longitude;                                   /* 经度 */
        double latitude;                                    /* 纬度 */
        char map_layer[MAX_128CHAR_STRING_LEN + 4];         /* 所属的图层 */

        char cms_id[MAX_ID_LEN + 4];                        /* 所属的CMS 统一编号 */

        osip_list_t* pGBDeviceInfoList;                     /* 对应的标准物理设备队列 */
#ifdef MULTI_THR
        osip_mutex_t* pGBDeviceInfoListLock;                /* 对应的标准物理设备队列锁 */
#endif

        unsigned int  no_stream_count;                      /* TSU 上报没有码流次数 */
        unsigned int  shdb_channel_id;                      /* 上海地标对应的通道ID */

        unsigned int del_mark;                              /* 删除标识:0,不删除，1:删除，2:智能分析删除 */

        char Value[EV9000_LONG_LONG_STRING_LEN];            /*遥信遥控：1~开，0~闭；遥测遥调：真实数据，浮点型；事件量命令量：字符串*/
        char Unit[EV9000_SHORT_STRING_LEN];                 /*遥测遥调时生效 单位*/
        int AlarmPriority;                                  /*0~无报警；1~3表示报警级别*/

        unsigned int  nResved1;                             /* 保留1 */
        char strResved2[MAX_32CHAR_STRING_LEN + 4];         /* 保留2 */
    } GBLogicDevice_info_t;

    /* 标准逻辑设备信息队列 */
    typedef std::map <string, GBLogicDevice_info_t*> GBLogicDevice_Info_MAP;
    typedef std::map <string, GBLogicDevice_info_t*>::iterator GBLogicDevice_Info_Iterator;

    /* ZRV设备信息结构体 */
    typedef struct _ZRVDevice_info_t
    {
        unsigned int id;                                 /* 设备索引 */
        char device_ip[MAX_IP_LEN];                      /* 设备ip地址*/
        int reg_status;                                  /* 状态:0:不在线，1:在线 */

        int tcp_sock;                                    /* 对应的TCP Socket */

        int last_register_time;                          /* 最后注册时间 */
        int register_expires;                            /* 注册超时时间 */
        int register_expires_count;                      /* 注册超时时间计数 */

        char strRegServerEthName[MAX_IP_LEN];            /* 注册的服务器IP地址网口名称 */
        char strRegServerIP[MAX_IP_LEN];                 /* 注册的服务器IP地址 */

        unsigned int del_mark;                           /* 删除标识:0,不删除，1:删除 */
    } ZRVDevice_info_t;

    /* ZRV设备信息队列 */
    typedef std::map <string, ZRVDevice_info_t*> ZRVDevice_Info_MAP;
    typedef std::map <string, ZRVDevice_info_t*>::iterator ZRVDevice_Info_Iterator;

    /* 待处理ZRV设备信息队列 */
    typedef std::deque <ZRVDevice_info_t*> needtoproc_ZRVDeviceinfo_queue;

    extern int AddAllGBLogicDeviceIDToVector(vector<string>& DeviceIDVector, DBOper* ptDBoper);
    extern int AddAllGBLogicDeviceIDToVectorForDevice(vector<string>& DeviceIDVector, int device_type, DBOper* ptDBoper);
    extern int AddAllGBLogicDeviceIDToVectorForRoute(vector<string>& DeviceIDVector, int route_index, int iThreePartyFlag, int link_type, DBOper* ptDBoper);
    extern int AddAllGBLogicDeviceIDToVectorForSubCMS(vector<string>& DeviceIDVector, int device_index, DBOper* ptDBoper);
    extern int AddAllGBLogicDeviceIDToVectorForUser(vector<string>& DeviceIDVector, DBOper* ptDBoper);
    extern int AddAllRCUGBLogicDeviceIDToVectorForUser(vector<string>& DeviceIDVector, DBOper* ptDBoper);
    extern void AddAllSubCMSIPToVector(vector<string>& SubCmsIPVector);
    extern int AddAssignCompressTaskToPerZRVDevice(vector<string>& CompressTaskID, int task_count, int task_begin_index, int task_end_index, vector<string>& PerCompressTaskID);
    extern int AddCivilCodeToGBLogicDeviceInfo(DBOper* pDevice_Srv_dboper);
    extern int AddGBLogicDeviceInfo2DB(char * device_id, DBOper * pDevice_Srv_dboper);
    extern int AddGBLogicDeviceInfo2DBForRoute(char * device_id, DBOper * pDevice_Srv_dboper);
    extern int AddGBLogicDeviceInfoByGBDeviceInfo(GBDevice_info_t * pGBDeviceInfo);
    extern int AddLogicDeviceGroup(GBDevice_info_t* pGBDeviceInfo, char* group_id, char* cms_id, char* group_name, int sort_id, char* parent_id);
    extern int AddLogicDeviceGroupAndMapDataDeviceInfo(DBOper* pDevice_Srv_dboper);
    extern int AddLogicDeviceGroupConfigToDeviceInfo(GBDevice_info_t* pGBDeviceInfo, DBOper* pDevice_Srv_dboper);
    extern int AddLogicDeviceGroupMapConfigToDeviceInfo(GBDevice_info_t* pGBDeviceInfo, DBOper* pDevice_Srv_dboper);
    extern int AddLogicDeviceMapGroup(GBDevice_info_t* pGBDeviceInfo, char* group_id, unsigned int device_index, char* cms_id, int sort_id);
    extern int checkGBDeviceIsOnline(char* device_ip);
    extern int CheckIsGBLogicDeviceInfoInDB(char * device_id, DBOper * pDevice_Srv_dboper);
    extern int checkNumberOfGBDeviceInfo();
    extern void check_GBDeviceConfig_need_to_reload(DBOper* pDboper);
    extern int check_GBDevice_info_from_db_to_list(DBOper* pdboper);
    extern void check_GBLogicDeviceConfig_need_to_reload(DBOper* pDboper);
    extern int check_GBLogicDevice_info_from_db_to_list(DBOper* pdboper);
    extern int check_shdb_channel_info_from_db(DBOper * pDboper);
    extern int CivilCodeGBLogicDeviceCatalogInfoProc(GBDevice_info_t * pGBDeviceInfo, char* civil_code, char* civil_name, char* parent_id, int iEvent, DBOper* pDboper, int iNeedToSync);
    extern int debug_GBDevice_info_list_lock(const char* file, int line, const char* func);
    extern int debug_GBDevice_info_list_unlock(const char* file, int line, const char* func);
    extern int debug_GBLogicDevice_info_list_lock(const char* file, int line, const char* func);
    extern int debug_GBLogicDevice_info_list_unlock(const char* file, int line, const char* func);
    extern int debug_ZRVDevice_info_list_lock(const char* file, int line, const char* func);
    extern int debug_ZRVDevice_info_list_unlock(const char* file, int line, const char* func);
    extern int DeleteExcessLogicDeviceMapGroupConfig(LogicDeviceMapGroup_t* pLogicDeviceMapGroup, DBOper* pDevice_Srv_dboper);
    extern int DeleteLogicDeviceGroupConfig(LogicDeviceGroup_t* pLogicDeviceGroup, DBOper* pDevice_Srv_dboper);
    extern int DeleteLogicDeviceMapGroupConfig(LogicDeviceMapGroup_t* pLogicDeviceMapGroup, DBOper* pDevice_Srv_dboper);
    extern int delete_GBDevice_info_from_list_by_mark();
    extern int delete_GBLogicDevice_info_from_list_by_mark();
    extern int DelLogicDeviceGroupInfo(GBDevice_info_t* pGBDeviceInfo);
    extern int DelLogicDeviceGroupInfo2(GBDevice_info_t* pGBDeviceInfo, LogicDeviceGroup_t* pLogicDeviceGroup);
    extern int DelLogicDeviceMapGroupInfo(GBDevice_info_t* pGBDeviceInfo);
    extern int DelLogicDeviceMapGroupInfo2(GBDevice_info_t* pGBDeviceInfo, LogicDeviceMapGroup_t* pLogicDeviceMapGroup);
    extern int DeviceGroupConfigInfoProc(GBDevice_info_t* pGBDeviceInfo, char* group_id, char* group_name, char* parent_id, int sort_id, int iEvent, DBOper* pDboper, int iNeedToSync);
    extern int DeviceGroupMapConfigInfoProc(GBDevice_info_t* pGBDeviceInfo, char* group_id, unsigned int device_index, int sort_id, int iEvent, DBOper* pDboper, int iNeedToSync);
    extern int ExecuteControlRCUDevice(unsigned int uDeviceIndex, char* ctrl_value, DBOper * pDBoper);
    extern int ExecuteDeviceAlarmRecord(unsigned int uDeviceIndex, int flag, DBOper * pDevice_Srv_dboper);
    extern int ExecuteDevicePresetByPresetID(unsigned int uPresetID, DBOper * pDevice_Srv_dboper);
    extern int ExecuteDevicePresetByPresetIDAndDeviceIndex(unsigned int uPresetID, unsigned int uDeviceIndex, DBOper * pDevice_Srv_dboper);
    extern int free_zrv_device_info_by_tcp_socket(int tcp_socket);
    extern int GBDeviceConfig_db_refresh_proc();
    extern int GBDevice_add(GBLogicDevice_info_t* pGBLogicDeviceInfo, int stream_type, GBDevice_info_t* pGBDeviceInfo);
    extern void GBDevice_free(GBDevice_t* gb_device);
    extern GBDevice_t* GBDevice_get_by_stream_type(GBLogicDevice_info_t* pGBLogicDeviceInfo, int stream_type);
    extern int GBDevice_info_add(GBDevice_info_t* pGBDeviceInfo);
    extern GBDevice_info_t* GBDevice_info_find(char* device_id);
    extern GBDevice_info_t* GBDevice_info_find_by_device_index(int device_index);
    extern GBDevice_info_t* GBDevice_info_find_by_id_ip_and_port(char* device_id, char* login_ip, int login_port);
    extern GBDevice_info_t* GBDevice_info_find_by_ip_and_port(char* login_ip, int login_port);
    extern void GBDevice_info_free(GBDevice_info_t* GBDevice_info);
    extern GBDevice_info_t* GBDevice_info_get_by_stream_type(GBLogicDevice_info_t* pGBLogicDeviceInfo, int stream_type);
    extern GBDevice_info_t* GBDevice_info_get_by_stream_type2(GBLogicDevice_info_t* pGBLogicDeviceInfo, int stream_type);
    extern int GBDevice_info_init(GBDevice_info_t** GBDevice_info);
    extern void GBDevice_info_list_free();
    extern int GBDevice_info_list_init();
    extern int GBDevice_info_list_lock();
    extern int GBDevice_info_list_unlock();
    extern int GBDevice_info_remove(char* device_id);
    extern int GBDevice_init(GBDevice_t** gb_device);
    extern int GBDevice_remove(GBLogicDevice_info_t* pGBLogicDeviceInfo, int stream_type);
    extern int GBLogicDeviceCatalogGroupMapProc(GBDevice_info_t * pGBDeviceInfo, GBLogicDevice_info_t* pGBLogicDeviceInfo, char* parent_id, DBOper* pDevice_Srv_dboper, int iNeedToSync);
    extern int GBLogicDeviceCatalogInfoProc(GBDevice_info_t* pGBDeviceInfo, GBLogicDevice_info_t* pNewGBLogicDeviceInfo, GBLogicDevice_info_t* pOldGBLogicDeviceInfo, DBOper* pDevice_Srv_dboper);
    extern int GBLogicDeviceCatalogInfoProcForRoute(GBLogicDevice_info_t* pNewGBLogicDeviceInfo, GBLogicDevice_info_t* pOldGBLogicDeviceInfo, DBOper* pDevice_Srv_dboper);
    extern int GBLogicDeviceConfig_db_refresh_proc();
    extern int GBLogicDevice_info_add(GBLogicDevice_info_t* pGBLogicDeviceInfo);
    extern GBLogicDevice_info_t* GBLogicDevice_info_find(char* device_id);
    extern GBLogicDevice_info_t* GBLogicDevice_info_find_by_device_index(unsigned int device_index);
    extern GBLogicDevice_info_t* GBLogicDevice_info_find_by_device_index2(unsigned int device_index);
    extern void GBLogicDevice_info_free(GBLogicDevice_info_t* GBLogicDevice_info);
    extern int GBLogicDevice_info_init(GBLogicDevice_info_t** GBLogicDevice_info);
    extern void GBLogicDevice_info_list_free();
    extern int GBLogicDevice_info_list_init();
    extern int GBLogicDevice_info_list_lock();
    extern int GBLogicDevice_info_list_unlock();
    extern int GBLogicDevice_info_remove(char* device_id);
    extern int GBLogicDevice_info_update(GBLogicDevice_info_t* pOldGBLogicDeviceInfo, GBLogicDevice_info_t* pNewGBLogicDeviceInfo, int change_type);
    extern int GBLogicDevice_info_update_for_route(GBLogicDevice_info_t* pOldGBLogicDeviceInfo, GBLogicDevice_info_t* pNewGBLogicDeviceInfo, int change_type);
    extern int GetGBDeviceListAndSendNotifyCatalogToSubCMS(GBDevice_info_t* pGBDeviceInfo, DBOper* pDevice_Srv_dboper);
    extern int GetGBLogicDeviceRecordTypeFromDB(char * device_id, DBOper * pDevice_Srv_dboper);
    extern LogicDeviceGroup_t* GetLogicDeviceGroup(GBDevice_info_t* pGBDeviceInfo, char* group_id);
    extern LogicDeviceMapGroup_t* GetLogicDeviceMapGroup(GBDevice_info_t* pGBDeviceInfo, unsigned int device_index);
    extern int GetZRVDeviceIPForAssginTask(vector<string>& ZRVDeviceIP);
    extern int Get_GBDevice_Index_By_Device_ID(char* device_id);
    extern unsigned int Get_GBLogicDevice_Index_By_Device_ID(char* device_id);
    extern int GroupCodeGBLogicDeviceCatalogInfoProc(GBDevice_info_t * pGBDeviceInfo, char* group_id, char* group_name, char* parent_id, int iEvent, DBOper* pDboper, int iNeedToSync);
    extern int InsertLogicDeviceGroupConfig(LogicDeviceGroup_t* pLogicDeviceGroup, DBOper* pDevice_Srv_dboper);
    extern int InsertLogicDeviceMapGroupConfig(LogicDeviceMapGroup_t* pLogicDeviceMapGroup, DBOper* pDevice_Srv_dboper);
    extern int IntelligentAnalysisGBLogicDeviceCatalogInfoProc(GBDevice_info_t * pGBDeviceInfo, char* device_id, DBOper* pDboper);
    extern int IsGBDeviceRegInfoHasChange(GBDevice_info_t * pGBDeviceInfo, int iDeviceType, char * pcLoginIP, int iLoginPort, int iRegInfoIndex);
    extern int IsGBLogicDeviceInfoHasChange(GBLogicDevice_info_t * pOldGBLogicDeviceInfo, GBLogicDevice_info_t * pNewGBLogicDeviceInfo, int change_type);
    extern int IsGBLogicDeviceInfoHasChangeForRCU(GBLogicDevice_info_t * pOldGBLogicDeviceInfo, GBLogicDevice_info_t * pNewGBLogicDeviceInfo);
    extern int IsGBLogicDeviceInfoHasChangeForRoute(GBLogicDevice_info_t * pOldGBLogicDeviceInfo, GBLogicDevice_info_t * pNewGBLogicDeviceInfo, int change_type);
    extern int IsIPInSubCMS(vector<string>& SubCmsIPVector, char* ip_addr);
    extern int is_zrv_device_tcp_socket_in_use(int tcp_socket);
    extern int load_db_data_to_LogicDevice_info_list_by_device_id(DBOper * ptDBOper, char* device_id);
    extern int load_GBLogicDevice_info_from_db_by_device_id(DBOper * ptDBOper, char* device_id, GBLogicDevice_info_t** pGBLogicDeviceInfo);
    extern void LogicDeviceGroup_free(LogicDeviceGroup_t* logic_device_group);
    extern int LogicDeviceGroup_init(LogicDeviceGroup_t** logic_device_group);
    extern void LogicDeviceMapGroup_free(LogicDeviceMapGroup_t* logic_device_map_group);
    extern int LogicDeviceMapGroup_init(LogicDeviceMapGroup_t** logic_device_map_group);
    extern int ModifyLogicDeviceGroup(LogicDeviceGroup_t* pLogicDeviceGroup, char* group_name, int sort_id, char* parent_id);
    extern int ModifyLogicDeviceMapGroup(LogicDeviceMapGroup_t* pLogicDeviceMapGroup, char* group_id, char* cms_id, int sort_id);
    extern int RemoveGBLogicDevice(int device_index);
    extern int RemoveGBLogicDeviceLockInfoByRouteInfo(route_info_t* pRouteInfo);
    extern int RemoveGBLogicDeviceLockInfoByUserInfo(user_info_t* pUserInfo);
    extern void scan_GBDevice_info_list_for_expires();
    extern void scan_GBDevice_info_list_for_subscribe();
    extern void scan_ZRVDevice_info_list_for_expires();
    extern int SendAllDeviceStatusToSubCMS(vector<string>& DeviceIDVector, int status, DBOper* ptDBoper);
    extern int SendAllGBLogicDeviceIntelligentStatusOffAlarmToAllClientUser(int device_index);
    extern int SendAllGBLogicDeviceStatusOffAlarmToAllClientUser(int device_index);
    extern int SendAllGBLogicDeviceStatusProc(int device_index, int status, int enable, DBOper* ptDBoper);
    extern int SendAllIntelligentGBLogicDeviceStatusProc(int device_index, int status, DBOper* ptDBoper);
    extern int SendAllNotifyCatalogToSubCMS(vector<string>& DeviceIDVector, int iEvent, DBOper* ptDBoper);
    extern int SendDeviceControlAlarmMessage(char * device_id, GBDevice_info_t * pGBDeviceInfo, int iuCmdType);
    extern int SendDeviceNoStreamAlarmToAllClientUser(char * device_id);
    extern int SendDeviceOffLineAlarmToAllClientUser(char * device_id);
    extern int SendDeviceStatusMessageProc(GBLogicDevice_info_t* pGBLogicDeviceInfo, int status, DBOper* pDboper);
    extern int SendDeviceStatusToSubCMS(GBLogicDevice_info_t* pGBLogicDeviceInfo, int status, DBOper* ptDBoper);
    extern int SendDeviceTeleBootMessage(GBDevice_info_t * pGBDeviceInfo);
    extern int SendExecuteDevicePresetMessage(GBLogicDevice_info_t* pGBLogicDeviceInfo, GBDevice_info_t * pGBDeviceInfo, unsigned int uPresetID);
    extern int SendExecuteDevicePresetMessageToRoute(GBLogicDevice_info_t* pGBLogicDeviceInfo, route_info_t * pRouteInfo, unsigned int uPresetID);
    extern int SendExecuteDevicePresetMessageToSubCMS(char* strPresetID);
    extern int SendGBPhyDeviceOffLineAlarmToAllClientUser(unsigned int uType, unsigned int device_index);
    extern int SendIntelligentDeviceOffLineAlarmToAllClientUser(char * device_id);
    extern int SendNotifyCatalogMessageToAllRoute(GBLogicDevice_info_t* pGBLogicDeviceInfo, int iEvent, DBOper* ptDBoper);
    extern int SendNotifyCatalogMessageToSubCMS(char* strCMSID);
    extern int SendNotifyCatalogToRouteCMS(GBLogicDevice_info_t* pGBLogicDeviceInfo, int iEvent, DBOper* pDboper);
    extern int SendNotifyCatalogToSubCMS(GBLogicDevice_info_t* pGBLogicDeviceInfo, int iEvent, DBOper* ptDBoper);
    extern int SendNotifyGroupMapCatalogTo3PartyRouteCMS(GBLogicDevice_info_t* pGBLogicDeviceInfo, int iEvent, DBOper* pDboper);
    extern int SendNotifyRestartMessageToAllSubCMS();
    extern int SendNotifyRestartMessageToSubCMS(GBDevice_info_t * pGBDeviceInfo);
    extern int SendQueryAllOfflineLogicDeviceStatusMessage(GBDevice_info_t * pGBDeviceInfo);
    extern int SendQueryDECDeviceMediaPortMessage(GBDevice_info_t * pGBDeviceInfo, char* dc_id, char* camera_id, int iStreamType);
    extern int SendQueryDeviceCatalogMessage(GBDevice_info_t * pGBDeviceInfo);
    extern int SendQueryDeviceGroupInfoMessage(GBDevice_info_t * pGBDeviceInfo);
    extern int SendQueryDeviceGroupMapInfoMessage(GBDevice_info_t * pGBDeviceInfo);
    extern int SendQueryDeviceInfoMessage(GBDevice_info_t * pGBDeviceInfo);
    extern int SendQueryDeviceStatusMessage(GBDevice_info_t * pGBDeviceInfo);
    extern int SendQueryGBLogicDeviceStatusMessage(char* device_id, GBDevice_info_t * pGBDeviceInfo);
    extern int SendQuerySubCMSDBIPMessage(GBDevice_info_t * pGBDeviceInfo);
    extern int SendQuerySubCMSTopologyPhyDeviceConfigMessage(GBDevice_info_t * pGBDeviceInfo);
    extern int SendRCUDeviceStatusToRouteCMS(GBLogicDevice_info_t* pGBLogicDeviceInfo, DBOper* pDboper);
    extern int SendRCUDeviceStatusToSubCMS(GBLogicDevice_info_t* pGBLogicDeviceInfo, DBOper* ptDBoper);
    extern int SendSubscribeMessageToSubGBDevice(GBDevice_info_t* pGBDeviceInfo, int iFlag);
    extern int SendTSUInfoMessageToDEC(char* callee_id, char* local_ip, int local_port, char* remote_ip, int remote_port, char* strSN, char* device_id, char* tsu_ip, int tsu_port);
    extern int SetGBLogicDeviceInfoDelFlag(GBDevice_info_t* pGBDeviceInfo);
    extern int SetGBLogicDeviceInfoEnableFlagByDelMark(GBDevice_info_t* pGBDeviceInfo, DBOper* pDevice_Srv_dboper);
    extern int SetGBLogicDeviceIntelligentStatus(int device_index, intelligent_status_t status);
    extern int SetGBLogicDeviceStatus(int device_index, int status, DBOper * pDevice_Srv_dboper);
    extern int SetLogicDeviceGroupChangeFlag(GBDevice_info_t* pGBDeviceInfo, int iChangeFlag);
    extern int SetLogicDeviceMapGroupChangeFlag(GBDevice_info_t* pGBDeviceInfo, int iChangeFlag);
    extern unsigned int SetZRVDeviceToConfigTable(ZRVDevice_info_t* pZRVDeviceInfo, DBOper* ptDBoper);
    extern int set_GBDevice_info_list_del_mark(int del_mark);
    extern int set_GBLogicDevice_info_list_del_mark(int del_mark);
    extern int set_vms_nvr_GBLogicDevice_info_enable_mark(char* device_id, int enable_mark);
    extern void ShowGBDeviceInfo(int sock, int type);
    extern void ShowLogicDeviceInfo(int sock, int type);
    extern void ShowZRVDeviceInfo(int sock, int type);
    extern int SynLogicDeviceGroupInfoToDB(GBDevice_info_t* pGBDeviceInfo, DBOper* pDevice_Srv_dboper);
    extern int SynLogicDeviceGroupInfoToDB2(LogicDeviceGroup_t* pLogicDeviceGroup, DBOper* pDevice_Srv_dboper);
    extern int SynLogicDeviceMapGroupInfoToDB(GBDevice_info_t* pGBDeviceInfo, DBOper* pDevice_Srv_dboper);
    extern int SynLogicDeviceMapGroupInfoToDB2(LogicDeviceMapGroup_t* pLogicDeviceMapGroup, DBOper* pDevice_Srv_dboper);
    extern int UpdateGBDeviceCMSID2DB(char* device_id, DBOper * pDevice_Srv_dboper);
    extern int UpdateGBDeviceInfo2DB(GBDevice_cfg_t & new_GBDevice_cfg, DBOper * pDevice_Srv_dboper);
    extern int UpdateGBDeviceLinkType2DB(GBDevice_info_t * pGBDeviceInfo, DBOper * pDevice_Srv_dboper);
    extern int UpdateGBDeviceRegStatus2DB(GBDevice_info_t * pGBDeviceInfo, DBOper * pDevice_Srv_dboper);
    extern int UpdateGBLogicDeviceRegStatus2DB(char * strDeviceID, int status, DBOper * pDevice_Srv_dboper);
    extern int UpdateGBLogicDeviceXYParam2DB(char * strDeviceID, double longitude, double latitude, char * strMapLayer, DBOper * pDevice_Srv_dboper);
    extern int UpdateLogicDeviceGroupConfig(LogicDeviceGroup_t* pLogicDeviceGroup, DBOper* pDevice_Srv_dboper);
    extern int UpdateLogicDeviceMapGroupConfig(LogicDeviceMapGroup_t* pLogicDeviceMapGroup, DBOper* pDevice_Srv_dboper);
    extern int ZRVDevice_info_add(ZRVDevice_info_t* pZRVDeviceInfo);
    extern ZRVDevice_info_t* ZRVDevice_info_find(char* device_ip);
    extern ZRVDevice_info_t* ZRVDevice_info_find_by_tcp_sock(int tcp_sock);
    extern void ZRVDevice_info_free(ZRVDevice_info_t* ZRVDevice_info);
    extern int ZRVDevice_info_init(ZRVDevice_info_t** ZRVDevice_info);
    extern void ZRVDevice_info_list_free();
    extern int ZRVDevice_info_list_init();
    extern int ZRVDevice_info_list_lock();
    extern int ZRVDevice_info_list_unlock();
    extern int ZRVDevice_info_remove(char* device_ip);
    extern char* get_zrv_device_ip_by_min_task_count(char* platform_ip);

#ifdef ENABLE_DEBUG_LOCK
#define GBDEVICE_SMUTEX_LOCK() debug_GBDevice_info_list_lock(__FILE__, __LINE__, __func__)
#define GBDEVICE_SMUTEX_UNLOCK() debug_GBDevice_info_list_unlock(__FILE__, __LINE__, __func__)
#define GBLOGICDEVICE_SMUTEX_LOCK() debug_GBLogicDevice_info_list_lock(__FILE__, __LINE__, __func__)
#define GBLOGICDEVICE_SMUTEX_UNLOCK() debug_GBLogicDevice_info_list_unlock(__FILE__, __LINE__, __func__)
#define ZRVDEVICE_SMUTEX_LOCK() debug_ZRVDevice_info_list_lock(__FILE__, __LINE__, __func__)
#define ZRVDEVICE_SMUTEX_UNLOCK() debug_ZRVDevice_info_list_unlock(__FILE__, __LINE__, __func__)
#else
#define GBDEVICE_SMUTEX_LOCK() GBDevice_info_list_lock()
#define GBDEVICE_SMUTEX_UNLOCK() GBDevice_info_list_unlock()
#define GBLOGICDEVICE_SMUTEX_LOCK() GBLogicDevice_info_list_lock()
#define GBLOGICDEVICE_SMUTEX_UNLOCK() GBLogicDevice_info_list_unlock()
#define ZRVDEVICE_SMUTEX_LOCK() ZRVDevice_info_list_lock()
#define ZRVDEVICE_SMUTEX_UNLOCK() ZRVDevice_info_list_unlock()
#endif

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif /* __UDP_TL_H__ */
