#ifndef _GBL_DEF_H_
#define _GBL_DEF_H_

#include "EV9000_InnerDef.h"
#include "EV9000_ErrCodeDef.h"
#include "EV9000_ExtDef.h"
#include "EV9000_AlarmTypeDef.h"
#include "InnerProtocolDef.h"

#define SYS_VERSION ("1.01")
#if defined(__DATE__) && defined(__TIME__)
static const char server_built[] = __DATE__ " " __TIME__;
#else
static const char server_built[] = "unknown";
#endif

extern int g_Language;  /* 语言，0:中文；1:英文，默认中文 */

#define DECS(X) 1

#define IN
#define OUT

/* IP地址类型 */
typedef enum _ip_addr_type_t
{
    IP_ADDR_VIDEO,                 /* 视频网地址 */
    IP_ADDR_DEVICE,                /* 设备网地址 */
    IP_ADDR_MGMT,                  /* 管理网地址，SX专用 */
    IP_ADDR_NULL
} ip_addr_type_t;

/* 写入标准物理设备临时表的错误原因 */
typedef enum _device_error_reason_type_t
{
    DEVICE_ERROR_REASON_NOT_FOUND,
    DEVICE_ERROR_REASON_IP_CONFLICT,
    DEVICE_ERROR_REASON_NULL
} device_error_reason_type_t;

/* 消息类型 */
typedef enum _msg_type_t
{
    MSG_REGISTER,
    MSG_REGISTER_RESPONSE,
    MSG_INVITE,
    MSG_INVITE_RESPONSE,
    MSG_CANCEL,
    MSG_ACK,
    MSG_BYE,
    MSG_BYE_RESPONSE,
    MSG_INFO,
    MSG_INFO_RESPONSE,
    MSG_MESSAGE,
    MSG_MESSAGE_RESPONSE,
    MSG_SUBSCRIBE,
    MSG_SUBSCRIBE_RESPONSE,
    MSG_NOTIFY,
    MSG_NOTIFY_RESPONSE,
    MSG_TYPE_NULL
} msg_type_t;

/* XML 消息类型 */
typedef enum _xml_type_t
{
    XML_CONTROL_DEVICECONTROL,           /* 设备控制 */
    XML_CONTROL_DEVICECONFIG,            /* 设备配置 */
    XML_CONTROL_SETDEVICEVIDEOPARAM,     /* 设置前端图像参数 */
    XML_CONTROL_REQUESTIFRAMDATA,        /* 请求I帧*/
    XML_CONTROL_AUTOZOOMIN,              /* 点击放大*/
    XML_CONTROL_SETDEVICEXYPARAM,        /* 设置前端经纬度参数 */
    XML_CONTROL_EXECUTEPRESET,           /* 执行预置位 */
    XML_QUERY_CATALOG,                   /* 查询设备目录 */
    XML_QUERY_DEVICEINFO,                /* 查询设备信息 */
    XML_QUERY_DEVICESTATUS,              /* 查询设备状态 */
    XML_QUERY_DEVICECONFIG,              /* 查询设备配置 */
    XML_QUERY_RECORDINFO,                /* 查询设备录像信息 */
    XML_QUERY_USERCONFIG,                /* 用户配置获取 */
    XML_QUERY_DEVICEGROUP,               /* 逻辑设备分组信息 */
    XML_QUERY_USERDEVICEGROUP,           /* 逻辑设备重新分组信息 */
    XML_QUERY_DEVICEMAPGROUP,            /* 逻辑设备分组映射关系 */
    XML_QUERY_USERDEVICEMAPGROUP,        /* 逻辑设备重新分组映射关系 */
    XML_QUERY_SERVERID,                  /* 查询服务器ID */
    XML_QUERY_POLLCONFIG,                /* 用户查询轮巡 */
    XML_QUERY_POLLACTION_CONFIG,         /* 用户查询轮巡动作 */
    XML_QUERY_PLANCONFIG,                /* 用户查询预案 */
    XML_QUERY_PLANACTION_CONFIG,         /* 用户查询预案动作 */
    XML_QUERY_CRUISE_CONFIG,             /* 用户查询巡航 */
    XML_QUERY_CRUISE_ACTION_CONFIG,      /* 用户查询巡航动作 */
    XML_QUERY_DEVICEVIDEOPARAM,          /* 获取前端图像参数 */
    XML_QUERY_DEVICEPRESET,              /* 获取前端预置位 */
    XML_QUERY_GETDBIP,                   /* 获取数据库IP地址 */
    XML_QUERY_TOPOLOGYPHYDEVICE,         /* 获取拓扑物理设备配置表 */
    XML_QUERY_GETPRESET,                 /* 获取预置位配置 */
    XML_QUERY_CHANNELSTATUS,             /* 获取解码器通道状态 */
    XML_QUERY_WEBINTERFACECONFIG,        /* 获取 URL信息 */
    XML_QUERY_ALARMRECORD,               /* 获取报警信息 */
    XML_QUERY_ONLINEUSER,                /* 获取在线用户信息 */
    XML_QUERY_TSUINFO,                   /* 获取TCP方式情况下的IP地址和端口号 */
    XML_QUERY_DATA_VALUE,                /* 获取前端点位数据值 */
    XML_QUERY_LABEL_RECORD,              /* 获取标签录像数据 */
    XML_NOTIFY_ALARM,                    /* 告警通知 */
    XML_NOTIFY_KEEPLIVE,                 /* 保活通知 */
    XML_NOTIFY_CATALOG,                  /* 点位变化通知 */
    XML_NOTIFY_CONNECT_TV,               /* 用户通知连接解码器 */
    XML_NOTIFY_EXECUTE_PLAN,             /* 用户通知执行预案 */
    XML_NOTIFY_EXECUTE_POLL,             /* 用户通知执行轮巡 */
    XML_NOTIFY_EXECUTE_CRUISE,           /* 用户通知执行巡航 */
    XML_NOTIFY_REOPEN,                   /* DEC通知CMS重新连接 */
    XML_NOTIFY_STATUS,                   /* 前端设备通知设备状态*/
    XML_NOTIFY_TV_STATUS,                /* 电视墙状态*/
    XML_NOTIFY_TVWALL_STATUS,            /* 电视墙通道状态 */
    XML_NOTIFY_CMS_RESTART,              /* CMS重启命令 */
    XML_NOTIFY_DIAGNOSIS_NOTICE,         /* 智能诊断结果上报命令 */
    XML_NOTIFY_TARGET_COUNT,             /* 目标结果的计数上报 */
    XML_NOTIFY_SEND_NOTIFY,              /* 高级别用户通知命令 */
    XML_NOTIFY_TOPOLOGYPHYDEVICE,        /* 通知拓扑结构更新消息 */
    XML_NOTIFY_MEDIA_STATUS,             /* 通知媒体状态消息 */
    XML_NOTIFY_SWITCH_ALARM,             /* 客户端切换是否发送告警消息 */
    XML_NOTIFY_DATA_VALUE,               /* 前端设备通知数据值 */
    XML_NOTIFY_SHDB_CMD,                 /* 用户通知上海地标协议操作 */
    XML_NOTIFY_TCP_CONNECT_INFO,         /* 通知TCP连接信息 */
    XML_RESPONSE_DEVICECONTROL,          /* 设备控制应答 */
    XML_RESPONSE_ALARM,                  /* 告警应答 */
    XML_RESPONSE_CATALOG,                /* 查询设备目录应答 */
    XML_RESPONSE_DEVICEINFO,             /* 查询设备信息应答 */
    XML_RESPONSE_DEVICESTATUS,           /* 查询设备状态应答 */
    XML_RESPONSE_QUERY_DEVICECONFIG,     /* 查询设备配置应答 */
    XML_RESPONSE_CONTROL_DEVICECONFIG,   /* 设备配置应答 */
    XML_RESPONSE_RECORDINFO,             /* 查询录像信息应答 */
    XML_RESPONSE_SETDEVICEVIDEOPARAM,    /* 设置前端图像参数应答  */
    XML_RESPONSE_GETDEVICEVIDEOPARAM,    /* 获取前端图像参数应答 */
    XML_RESPONSE_GETDEVICEPRESET,        /* 获取前端预置位应答  */
    XML_RESPONSE_GETDBIP,                /* 获取数据库IP地址应答 */
    XML_RESPONSE_TOPOLOGYPHYDEVICE,      /* 获取拓扑物理设备配置表应答 */
    XML_RESPONSE_TV_STATUS,              /* 电视墙状态应答  */
    XML_RESPONSE_QUERY_DEVICEGROUP,      /* 逻辑设备分组信息应答 */
    XML_RESPONSE_QUERY_DEVICEMAPGROUP,   /* 逻辑设备分组映射关系应答 */
    XML_RESPONSE_QUERY_GETPRESET,        /* 获取预置位配置应答 */
    XML_RESPONSE_QUERY_CHANNELSTATUS,    /* 获取解码器通道状态应答 */
    XML_RESPONSE_QUERY_DATA_VALUE,       /* 获取前端点位数据值应答 */
    XML_RESPONSE_QUERY_SERVERID,         /* 查询服务器ID返回 */
    XML_RESPONSE_QUERY_DEC_MEDIA_PORT,   /* 查询解码器媒体端口返回 */
    XML_ADD_POLLCONFIG,                  /* 增加轮巡配置 */
    XML_DEL_POLLCONFIG,                  /* 删除轮巡配置 */
    XML_ADD_LABEL_RECORD_CONFIG,         /* 增加标签录像配置 */
    XML_DEL_LABEL_RECORD_CONFIG,         /* 删除标签录像配置 */
    XML_CHANGE_USER_PASSWORD,            /* 修改用户密码 */
    XML_GET_SYSTEMINFO,                  /* 获取系统信息 */
    XML_GET_USERINFO,                    /* 获取用户信息 */
    XML_GET_DEVICEINFO,                  /* 获取设备信息 */
    XML_GET_TSUINFO,                     /* 获取TSU 信息 */
    XML_GET_TASKINFO,                    /* 获取任务信息 */
    XML_SET_IPADDR,                      /* 设置IP 地址 */
    XML_SEARCH_BOARD,                    /* 单板搜索 */
    XML_CONFIRM_ALARM,                   /* 确认报警信息 */
    XML_NOTIFY_SENDCATALOG_TO_ROUTE_CMS, /* 通知向上级CMS发送目录 */
    XML_NOTIFY_SENDCATALOG_TO_SUB_CMS,   /* 通知向下级CMS通知发送目录 */
    XML_NOTIFY_REBOOTCMS,                /* 通知重启CMS */
    XML_NOTIFY_REBOOTDB,                 /* 通知重启数据库 */
    XML_NOTIFY_LOCATION,                 /* 单板定位 */
    XML_NOTIFY_REBOOT,                   /* 通知重启 */
    XML_NOTIFY_SHUTDOWN,                 /* 通知关闭 */
    XML_NOTIFY_REBOOTKEYBOARD,           /* 通知重启键盘服务程序 */
    XML_NOTIFY_REBOOTMEDIASERVICE,       /* 通知重启自带媒体网关程序 */
    XML_NOTIFY_RESTORE_FACTORY,          /* 通知恢复出厂设置 */
    XML_NOTIFY_EXECUTE_CMD,              /* Web通知执行命令 */
    XML_NOTIFY_REFRESH_DBCONFIG,         /* Web通知刷新数据库配置 */
    XML_NOTIFY_SYNC_DATABASE,            /* Web通知同步数据库配置 */
    XML_NOTIFY_ENTER_SYSCONFIG,          /* Web通知CMS进入配置 */
    XML_NOTIFY_SAVE_SYSCONFIG,           /* Web通知CMS保存配置 */
    XML_NOTIFY_GET_VMS_CHANNEL,          /* Web通知获取VMS的通道信息 */
    XML_NOTIFY_GET_DEVICE_INFO,          /* Web通知获取前端设备信息 */
    XML_NOTIFY_GET_DEVICE_STATUS,        /* Web通知获取前端设备状态 */
    XML_NOTIFY_GET_DEVICE_CATALOG,       /* Web通知获取前端设备目录 */
    XML_NOTIFY_DEVICE_TELEBOOT,          /* Web通知前端设备远程重启 */
    XML_NOTIFY_DEVICEGROUP,              /* 通知逻辑设备分组信息变化 */
    XML_NOTIFY_DEVICEMAPGROUP,           /* 通知逻辑设备分组映射关系变化 */
    XML_QUERY_CATALOG_RCU,                /* RCU查询设备目录 */
    XML_NOTIFY_RCU_DEVICE_STATUS,        /* RCU通知设备状态 */
    XML_CONTROL_LOGICDEVICECONTROL,      /*RCU设备控制*/
    XML_TYPE_NULL
} xml_type_t;

/* XML 消息类型 */
typedef enum _xml_type_t2
{
    XML_REQUEST_REGISTER,            /* 注册请求 */
    XML_RESPONSE_REGISTER,           /* 注册响应 */
    XML_REQUEST_NOTIFY,              /* 通知请求 */
    XML_RESPONSE_NOTIFY,             /* 通知响应 */
    XML_REQUEST_MESSAGE,             /* 消息请求 */
    XML_RESPONSE_MESSAGE,            /* 消息响应 */
    XML_TYPE2_NULL
} xml_type_t2;

/* XML 消息类型 */
typedef enum _xml_sub_type_t
{
    XML_SUB_ZRV_REGISTER,               /* ZRV设备注册 */
    XML_SUB_ZRV_COMPRESS_TASK,          /* ZRV设备压缩任务 */
    XML_SUB_ZRV_COMPRESS_TASK_RESULT,   /* ZRV设备压缩任务结果 */
    XML_SUB_TYPE_NULL
} xml_sub_type_t;

/* 码流方向类型 */
typedef enum _media_direction_type_t
{
    MEDIA_DIRECTION_TYPE_INACTIVE,    /* 0:    inactive */
    MEDIA_DIRECTION_TYPE_SENDONLY,    /* 1:    sendonly */
    MEDIA_DIRECTION_TYPE_RECVONLY,    /* 2:    recvonly */
    MEDIA_DIRECTION_TYPE_SENDRECV,    /* 3:    sendrecv */
    MEDIA_DIRECTION_TYPE_NULL
} media_direction_type_t;

/* 视频调看类型 */
typedef enum _video_play_type_t
{
    VIDEO_RECORD_PLAY,                 /* 录像查看 */
    VIDEO_REAL_PLAY,                   /* 实时视频查看 */
    VIDEO_DOWNLOAD,                    /* 录像文件下载 */
    VIDEO_PLAY_NULL
} video_play_type_t;

/* 协议传输类型 */
typedef enum _transfer_protocol_type_t
{
    TRANSFER_PROTOCOL_UDP,
    TRANSFER_PROTOCOL_RTSP,
    TRANSFER_PROTOCOL_TCP,
    TRANSFER_PROTOCOL_NULL
} transfer_protocol_type_t;

#define MAX_ID_LEN                              20   /* 国标 ID 最大长度 */
#define MAX_IP_LEN                              16   /* IP地址长度长度 */

#define MAX_16CHAR_STRING_LEN          16       //16字符串长度
#define MAX_32CHAR_STRING_LEN          32       //32字符串长度
#define MAX_64CHAR_STRING_LEN          64       //64字符串长度
#define MAX_128CHAR_STRING_LEN         128      //128字符串长度
#define MAX_256CHAR_STRING_LEN         256      //256字符串长度
#define MAX_512CHAR_STRING_LEN         512      //512字符串长度
#define MAX_1024CHAR_STRING_LEN        1024     //1024字符串长度
#define MAX_2048CHAR_STRING_LEN        2048     //2048字符串长度
#define MAX_MSG_BODY_STRING_LEN        1024*10  //SIP消息体最大长度

#define MAX_TSU_TASK_LEN                        64   /* TSU任务 ID 最大长度 */

#define MAX_USER_RECORD_INFO_COUT_SEND          30   /* 发送给用户每次发送的录像记录条数 */
#define MAX_ROUTE_RECORD_INFO_COUT_SEND         10   /* 发送给上级CMS每次发送的录像记录条数 */

#define MAX_USER_CATALOG_COUT_SEND              13   /* 用户每次发送的逻辑点位条数 */
#define MAX_DEVICE_CATALOG_COUT_SEND            25   /* 前端设备请求每次发送的逻辑点位条数 */
#define MAX_ROUTE_CATALOG_COUT_SEND             2    /* 互连情况每次发送的逻辑点位条数 */

#define MAX_DEVICE_GROUP_COUT_SEND              15   /* 每次发送的逻辑设备分组信息条数 */
#define MAX_DEVICE_MAP_GROUP_COUT_SEND          25   /* 每次发送的逻辑设备分组关系信息条数 */

#define MAX_USER_DEVICE_GROUP_COUT_SEND         15   /* 每次发送的用户自定义逻辑设备分组信息条数 */
#define MAX_USER_DEVICE_MAP_GROUP_COUT_SEND     25   /* 每次发送的用户自定义逻辑设备分组关系信息条数 */

#define MAX_POLL_CONFIG_COUT_SEND               10   /* 每次发送的轮巡配置数据条数 */
#define MAX_POLL_ACTION_CONFIG_COUT_SEND        10   /* 每次发送的轮巡动作配置数据条数 */

#define MAX_PLAN_CONFIG_COUT_SEND               10   /* 每次发送的预案配置数据条数 */
#define MAX_PLAN_ACTION_CONFIG_COUT_SEND        10   /* 每次发送的预案动作配置数据条数 */

#define MAX_CRUISE_CONFIG_COUT_SEND             10   /* 每次发送的巡航配置数据条数 */
#define MAX_CRUISE_ACTION_CONFIG_COUT_SEND      10   /* 每次发送的巡航动作配置数据条数 */

#define MAX_DEVICE_PRESET_COUT_SEND             10   /* 每次发送的逻辑点位预置位信息条数 */

#define MAX_WEB_INTERFACE_CONFIG_COUT_SEND      20   /* 每次发送的web接口配置数据条数 */

#define MAX_ALARM_RECORD_COUT_SEND              15   /* 每次发送的报警记录数据条数 */

#define MAX_ONLINE_USER_CONFIG_COUT_SEND        20   /* 每次发送的在线用户数据条数 */

#define MAX_TOPOLOGY_DEVICE_CONFIG_COUT_SEND    10   /* 每次发送的拓扑物理设备配置数据条数 */

#define MAX_LABLE_RECORD_COUT_SEND              20   /* 每次发送的标签录像数据条数 */

#define MAX_ZRV_COMPRESS_TASL_COUT_SEND         2   /* 每次发送给ZRV的任务数 */

#define  PTZCMD_28181_LEN 8

/* 时间配置结构体 */
typedef struct _time_config_t
{
    unsigned int uID;                  /* 数据库索引 */
    int iBeginTime;                    /* 开始时间 */
    int iEndTime;                      /* 结束时间 */
    int del_mark;                      /* 删除标识 */
} time_config_t;

/* 点位状态定义 */
typedef enum _logic_device_status_t
{
    LOGIC_DEVICE_STATUS_OFF,       /* 0:    OFF */
    LOGIC_DEVICE_STATUS_ON,        /* 1:    ON */
    LOGIC_DEVICE_STATUS_NOSTREAM,  /* 2:    NOSTREAM */
    LOGIC_DEVICE_STATUS_UNREACHED, /* 3:    UNREACHED */
    LOGIC_DEVICE_STATUS_NULL
} logic_device_status_t;

/* 智能分析状态定义 */
typedef enum _intelligent_status_t
{
    INTELLIGENT_STATUS_OFF,    /* 0:    OFF */
    INTELLIGENT_STATUS_ON,     /* 1:    ON */
    INTELLIGENT_STATUS_NULL
} intelligent_status_t;

/* 报警状态定义 */
typedef enum _alarm_status_t
{
    ALARM_STATUS_CLOSE,    /* 0:    闭合 */
    ALARM_STATUS_APART,    /* 1:    断开的 */
    ALARM_STATUS_NULL
} alarm_status_t;

/* 智能分析状态定义 */
typedef enum _lock_status_t
{
    LOCK_STATUS_OFF,           /* 0:    OFF */
    LOCK_STATUS_USER_LOCK,     /* 1:    User Lock ON */
    LOCK_STATUS_ROUTE_LOCK,    /* 2:    Route Lock ON */
    LOCK_STATUS_NULL
} lock_status_t;

/* 处理线程类型 */
typedef enum _proc_thread_type_t
{
    THREAD_CONFIG_MGN_PROC,                /* 配置管理处理线程 */
    THREAD_TELNET_CLIENT_MONITOR_PROC,     /* Telnet 客户端监控处理线程 */
    THREAD_COMMON_DB_REFRESH_PROC,         /* 公共的数据库刷新处理线程 */
    THREAD_ZRV_DEVICE_MGN_PROC,            /* ZRV设备管理处理线程 */
    THREAD_ZRV_DEVICE_TCP_MGN_PROC,        /* ZRV设备TCP连接管理处理线程 */
    THREAD_ZRV_TASK_RESULT_MESSAGE_PROC,   /* ZRV设备任务结果消息上报处理线程 */
    THREAD_PLATFORM_MGN_PROC,              /* 上级平台管理处理线程 */
    THREAD_COMPRESS_TASK_MONITOR_PROC,     /* 压缩任务监控处理线程 */
    THREAD_NULL
} proc_thread_type_t;

//TCP数据结构体
typedef struct EV9000_TCP_Data
{
    EV9000_TCP_Head stTCPHead;
    char            stTCPBody[MAX_MSG_BODY_STRING_LEN + 4];
} EV9000_TCP_Data, *LPEV9000_TCP_Data;


#endif
