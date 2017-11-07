/***********************************************************************
* Copyright (C) 2013, WISCOM VISION TECHNOLOGY Corporation.
* 
* File Name: 	   EV9000_CommonManage.h
* Description:   本文件定义平台管理的事件类型及相关的结构仿
* Others:        敿
* Version:  	   dev-v1.01.01
* Author:        输入作者名字及单位
* Date:  	       2013.05.31
* 
* History 1:  		// 修改历史记录，包括修改日期、修改者、修改版本及修改内容
*                2013.05.31    qyg    dev-v1.01.01   首次创建  
* History 2: ?
**********************************************************************/


#ifndef EV9000_COMMON_MANAGE_H
#define EV9000_COMMON_MANAGE_H

#include "EV9000_BoardType.h"
#include "EV9000_InnerDef.h"
#include <time.h>

#define  SWAPL(x)  ((((x) & 0x000000ff) << 24) | \
                   (((x) & 0x0000ff00) << 8)  | \
                    (((x) & 0x00ff0000) >> 8)  | \
                    (((x) & 0xff000000) >> 24))

#define  SWAPS(x)  ((((x) & 0x00ff) << 8) | \
                    (((x) & 0xff00) >> 8) )


#define  CMS_PLAT_OK                0x00;
#define  CMS_PLAT_ERR               0xFFFFFFFF

#define  COMM_MSG_HEAD_LEN     0x20  
#define  HARDWARE_VERSION_LEN  0x20   /* 硬件版本长度 */
#define  SOFTWARE_VERSION_LEN  0x20   /* 软件版本长度 */
#define  PLATFORM_NAME_LEN     0x20
#define  DEVICE_ID_LEN         0x14   /* 单板ID长度 */
#define  DB_PATH_LEN           0x100

#define  ALARM_ADDINFO_STRING_MAX  128
#define  LOG_STRING_MAX_LEN  128


#define  EV9000_VERSION_1     0x01  

/* 定义网络类型 */
#define DEVICE_NET      0x00
#define VIDEO_NET       0x01
#define STORE_NET       0x02  
#define MS_CONNECT_NET  0x03

#define COMMON_DEVICE_NET   0x04
#define COMMON_VIDEO_NET    0x05
#define SLAVE_DEVICE_NET    0x06
#define SLAVE_VIDEO_NET     0x07
#define SLAVE_MS_NET        0x08
#define DB_CONF_NET         0x0A


#define ALL_NET       0x03 
/* 网络是否启用 */
#define NET_ENABLE     0x00
#define NET_UNENABLE 　0x01 

/* 时间服务 */
#define  LOCAL_TIMER   0x00
#define  NTP_TIMER     0x01


/* 统一ICE的服务名称和端口号，各服务名字：IP+端口号，端口号对应功能 */

#define CMS_ICE_ID            "CmsIceService"
#define CMS_PLAT_ICE_PORT     40010   //21456

#define CMS_RUN_STATE_ICE_ID            "RunStateService"
#define CMS_PLAT_RUN_STATE_ICE_PORT    40011   //21456

#define VER_UPDATE_ICE_ID     "VerUpdateIceService"

#if defined X86
	#define VER_UPDATE_ICE_PORT   21466
#else
	#define VER_UPDATE_ICE_PORT   21456
#endif	

/*公共消息头Socket通信时使使用兼容EV8000 */
typedef struct
{
    unsigned short  wVersions;      /* 版本 */
    unsigned short  wPayLoadLen;    /* payLoad长度*/
    unsigned int    dwEvent;        /* 事件卿*/
    unsigned short  wResved1;       /* 保留,在不同场合用作不同的用辿*/
    unsigned short  wResved2;       /* 保留 */
    unsigned int	dwComonID;        /* 命令ID */
    unsigned int    dwDestIP;      /* 目的IP */ 
    char            dummy[16];               /* 填充 */
   
}COMM_MSG_HEAD_T;

/* 时间 */
typedef struct
{
    unsigned short wYear;
    unsigned short wMonth;
    unsigned short wDay;
    unsigned short wHour;
    unsigned short wMinute;
    unsigned short wSecond;
	
}TIME_T;

/*  IP地址的结构体 */
typedef struct
{
    unsigned int  dwIPAddr;          /* IP地址 */
    unsigned int  dwIPMask;          /* IP子网掩码 */
    unsigned int  dwGetway;           /* IP 网关 */
    unsigned int  dwResved1;         /* 为IPV6预留，如果配置IPV6网络地址4个字节全部为IP地址 */
    unsigned int  dwPort;
    unsigned char ucHostname[64];             /* 主机名称 */
    
}IP_ADDR_T;

typedef struct
{
    unsigned int dwEth;
    unsigned int UsedFlag;
    IP_ADDR_T    tNetIP;    
}ETH_ATTR;

    
typedef struct
{
    unsigned char ucCmsID[24];
    unsigned int  dwMSFlag;   /* 0:非启用，1:启用 */
    ETH_ATTR tCmsVideoIP;    /* 视频专网的IP地址信息 */
    ETH_ATTR tCmsDeviceIp;   /* 设备网的IP地址信息 */
    ETH_ATTR tCmsCommonVideoIP;
    ETH_ATTR tCmsCommonDeviceIP;
    ETH_ATTR tCmsSVideoIP;
    ETH_ATTR tCmsSDeviceIP;
    ETH_ATTR tCmsDBIP;
    ETH_ATTR tCmsAlarmIP;
    ETH_ATTR tCmsNtpIP;
    ETH_ATTR tCmsSDBIP;
    ETH_ATTR tCmsUsingDeviceIP;
    ETH_ATTR tCmsUsingVideoIP;
}BOARD_NET_ATTR;

#define SHELF_FAN_NUM                     6
#define SHELF_TEMPERATURE_SENSOR          4
#define SHELF_SLOT_NUM_LX                17
#define SHELF_SLOT_NUM_MX                6

typedef struct
{

    unsigned short  u16Speed[SHELF_FAN_NUM]; 
    unsigned short  u16ShelfTemperature[SHELF_TEMPERATURE_SENSOR];
    unsigned char   ucShelfSlotCpuTemp[SHELF_SLOT_NUM_LX];
    unsigned char   u8Rsv[3];  
}SHELF_TEMPERATURE_INFO;
/* 通知类，用于OMMP-->CMS 数据库的异步修改 */

#define  EVT_DB_MODIFY_CMD               0x10000
#define  EVT_DB_MODIFY_LOCALIP           0x10001  /* CMS本地网络配置表发生改*/
#define  EVT_DB_MODIFY_ROUTER            0x10003  /* CMS的路由信息发生改卿*/
#define  EVT_DB_MODIFY_FU_GROUP_CONF     0x10005  /* 前端分组表发生改卿*/

/* 版本更新事件 */

#define     EVT_LOADVER_REQ                     0x10000
#define     EVT_VER_GET_INFO_REQ                0x10001
#define     EVT_VER_GET_INFO_ACK                0x10002
#define     EVT_VER_DELETE_REQ                  0x10003
#define     EVT_VER_DELETE_ACK                  0x10004
#define     EVT_VER_UPDATE_REQ                  0x10005
#define     EVT_VER_UPDATE_ACK                  0x10006
#define     EVT_VER_PAYLOAD_TRAN_REQ            0x10007
#define     EVT_VER_PAYLOAD_TRAN_ACK            0x10008
#define     EVT_VER_OPERATION_ACK               0x1000a

typedef enum
{
    VERSION_TYPE_APP0   = 0,
    VERSION_TYPE_APP1   = 1,
    VERSION_TYPE_BOOT   = 2,
    VERSION_TYPE_KERNEL = 3,
    VERSION_TYPE_ROOTFS = 4,
    VERSION_TYPE_MAX    = 5

}VERSION_TYPE;

typedef enum
{
    VERSION_MAIN    = 0,
    VERSION_BACKUP  = 1
}VERSION_FLAG;
#define VERSION_STRING_LENGTH   64

/* 版本头文件结朿*/
typedef struct 
{
    unsigned short u16Sign;   /* 0x4e59 */
    unsigned short u16CRC;       /* 版本文件的校验和*/   
    unsigned int   u32Len;        /* 版本的大小*/
    unsigned int   stTime;       /* 版本文件制作的时长*/
    VERSION_TYPE   enVerType;          /* 版本类型 */
    VERSION_FLAG   enFlag;     /*1：表示该版本为正在使用版本，0表示是备用版本。制作版本时该位0 */    
    char           s8VerStr[VERSION_STRING_LENGTH];
    char           s8Resv[44];
} TFileHeader;

/* 获取版本信息 wResved1 代表是否成功，1:失败，0成功 ， 3: 正在更新版本 */ 


/* 更新版本  wResved1 代表是否成功，1:失败，0成功*/ 


/* 传数据 wResved1 包序号，wResved2，CRC值；返回值， wResved2 1:失败，0成功 */ 

/* 单板控制*/

#define EVT_BOARD_CONTROL_CMD            0x30000
#define EVT_BOARD_CONTROL_RESET          0x30001    /* 单板软复位 */




/* 平台配置命令x40000  与之对应的查询命令（0x50000) */

#define  EVT_CONFIG_CMD      0x40000
#define  EVT_CONFIG_GET_CMD  0x50000


/*  单板ID号的配置或者查询   */

#define  EVT_CONFIG_BOARD_ID  0x40001
#define  EVT_GET_BOARD_ID     0x50001

/* IP 设置或者查询 */
#define  EVT_CONFIG_IP_REQ   0x40003

/* IP 查询 */
#define  EVT_CONFIG_GET_IP_REQ   0x50003

/* 地址配置结构 */
typedef struct
{
   unsigned char wEnableFlag;  /* 0:启用该网络，非零：禁止该网络 */
   unsigned char wIPV4orIPV6;  /* 0:IPV4, 非零：IPV6 */
   unsigned short wBordType;    /* 单板类型参考单板定义*/
   unsigned short wNetType;     /* 网络类型,参考网络类型定义；*/
   unsigned short wEthNO;       /* 端口编号  */
   unsigned char  anotherName[32];
   IP_ADDR_T      tIPConfig;
}IP_CONFIG_T;


/* CMS的路由设置 */

/* CMS的路由设置或者查询 */
#define  EVT_CONFIG_ROUTE_REQ   0x40005

/* CMS 查询 */
/************************************************************************/
#define  EVT_CONFIG_GET_ROUTE_REQ   0x50005
/*
   reserve1: 
   reserve2    几条路由	
*/

/************************************************************************/
/* 路由配置结构 */
typedef struct
{
   unsigned short wUporDownFlag;   /* 0:向上路由配置,1：接收下行的注册配置 */
   unsigned short wPort;           /* 向上路由配置的端口号 */
   unsigned char  ucServerID[DEVICE_ID_LEN];
   IP_ADDR_T      tIPConfig;

}CMS_ROUTE_CONFIG_T;

/* CMS 系统时间设置 */
#define  EVT_CONFIG_TIME_REQ      0x40007

#define  EVT_CONFIG_GET_TIME_REQ  0x50007

typedef struct
{
    unsigned int   dwSystermTimeType;   /* 时间配置类型0:手动设置时间，1：采用NTP时间 */
    TIME_T         tSystermTime;        /* 制定系统时间 */
    IP_ADDR_T      tNTPServer;          /* NTP时间服务器地址 */    

}PLATFORM_TIME_CONFIG_T;


/* 系统告警配置 */

#define  EVT_CONFIG_ALARM_REQ      0x40009

#define  EVT_CONFIG_GET_ALARM_REQ  0x50009

/* 故障告警服务器、级别配置 */
typedef struct
{
    unsigned int   dwAlarmLevel;         /* 需要推送显示的故障告警级别 */    
    IP_ADDR_T      tAlarmServer;        /* 报警推送的服务器地址 */	     
    	
}PLATFORM_ALARM_CONFIG_T;

/* 机框单板处理能力设置 */

#define EVT_CONFIG_MAX_CAPACITY_REQ 0x4000b
#define EVT_CONFIG_GET_CAPACITY_REQ 0x5000b
typedef struct
{
     unsigned short  wMaxAccessCnt;
     unsigned short  wMaxTransferCnt;
     unsigned short  wMaxStoreCnt;
     
}MAX_CAPACITY_CONFIG_T;

/* 数据库信息设置*/

#define EVT_CONFIG_DB_REQ 0x4000c
#define EVT_CONFIG_GET_DB_REQ 0x5000c
typedef struct
{
     unsigned int    dwDBIp;
     unsigned int    dwDBPort;
     unsigned char   ucDBPath[DB_PATH_LEN];
     
}DB_CONFIG_T;

/* 打印级别设置 */

#define EVT_CONFIG_LOGLEVEL_REQ 0x4000e

/* 打印级别通过Reserved子段设置  */



/*  基本信息及性能统计 */

#define  EVT_GETINFO_CMD      0x60000



/*  基本信息 业务查询 */
#define EVT_GETINFO_SHELF_REQ  0x60001
typedef struct
{
    unsigned short  wCabinetType;     /* 机框类型9U机 框  6U 机 框4U机框  */
    unsigned short  wMasterSlaveFlag; /* 主备是否在线  */
    unsigned short  wSlotNo;          /* 槽位*/
    unsigned short  wBordType;        /* 单板类型0：CMS, 1:TSU, 2：媒体网关 3:解码器 */
    	
}SHELF_BASE_INFO_T;

/*  单板基本运行信息 */

#define EVT_GETINFO_STAT_CMD    0x60003

typedef struct
{
    TIME_T          tSysTime;                                   /* 系统时间  */
    unsigned int    dwCPURate;                                  /* CPU 占用率 */
    unsigned int    dwMemoryRate;                               /* 内存占用率 */
    unsigned short  wRuningTime;                                /* 运行时间  */
    unsigned short  wMaxCapacity;                               /* 最大接入处理的能力 */
    unsigned short  wMaxStoreCapacity;                          /* 最大存储处理的能力 */
    unsigned char   aucDeviceID[DEVICE_ID_LEN];                 /*  单板的ID */
}BOARD_RUNNING_INFO_T;


#define EVT_GETINFO_SERVICE_CMD    0x60005

/*  CMS  业务基本信息 */
typedef struct
{    
    unsigned short  wUserCnt;                               /* 注册用户总数 */             
    unsigned short  wOnlineUserCnt;                         /* 在线用户揿*/            
    unsigned int    dwAnalogCameraCnt;                      /* 前端摄像机数 */             
    unsigned int    dwOnlineAnalogCameraCnt;                /* 当前在线的摄像机 */         
    unsigned int    dwIPCCnt;                                                        
    unsigned int    dwOnlineIPCnt;                                                   
    unsigned short  wDVRCnt;                                /* DVR数目 */              
    unsigned short  wOnlineDVRCnt;                          /* 在线的DVR数目 */           
    unsigned short  wNvrCnt;                                                         
    unsigned short  wOnlineNvrCnt;                                                   
    unsigned short  wEncoderCnt;                            /* 视频编码总数 */             
    unsigned short  wOnlineEncoderCnt;                      /* 在线的视频编码器 */           
    unsigned short  wDecoderCnt;                                                     
    unsigned short  wOnlineDecoderCnt;                                               
    unsigned short  wMGWCnt;                                 /* 网关数目 */              
    unsigned short  wOnlineMGWCnt;                                                   
    unsigned short  wTSUCnt;                                                         
    unsigned short  wOnlineTSUCnt;                                              
                                                                                     
    unsigned int    dwServiceCnt;                           /* 在线业务数 */         
    
    unsigned short  wAlarmEncoderCnt;
    unsigned short  wOnlineAlarmCoderCnt;
    unsigned short  wAlarmDecoderCnt;
    unsigned short  wOnlineAlarmDecoderCnt;
    unsigned short  wAlarmInputDeviceCnt;
    unsigned short  wOnlineAlarmInputDeviceCnt;
    unsigned short  wAlarmOutputDeviceCnt;
    unsigned short  wOnlineAlarmOutputDeviceCnt;   
	
}BASE_SERVICE_INFO_T;  


/* TSU  */

typedef struct
{
    unsigned int dwFrontDeviceAccessCnt;
    unsigned int dwTransferCnt;
    unsigned int dwRecoderCnt;
    unsigned int dwRecoderReplayCnt;     

}TSU_SERVICE_INFO_T;

#define EVT_GETINFO_ETH_STATISTICS_CMD    0x60007

/* 性能统计 */
typedef struct
{
    unsigned int   dwRcvCnt;
    unsigned int   dwRcvFailCnt;    
    unsigned int   dwSendCnt;
    unsigned int   dwSendFailCnt;
    unsigned int   dwNetUseRate;
    unsigned int   dwCtrolDataCnt;
    unsigned int   dwMediaDataCnt;
}ETH_STATISTICS_T;      /* 网口的性能统计  */


typedef struct
{
    unsigned short     wActionType;       /* 操作类型0:清零 1：查询 */  
    unsigned short     wNetType;          /* 查询/设置的网络类 */        
    ETH_STATISTICS_T   tNetStatistics;    /*    网口的性能统计  */
   
}NET_STATISTICS_T;



/* 故障告警管理  */

#define  EVT_ALARMINFO_CMD      0x70000


typedef struct
{
    unsigned int  dwAlarmIP;                                        /* 报警源的IP地址 */
    unsigned int  dwAlarmLevel;                                     /* 故障告警级别 */          
    unsigned int  dwAlarmID;                                        /*  故障告警卿*/          
    unsigned int  dwAlarmSTatus;                                    /* 故障告警上报、恢复 */     
    TIME_T        dwAlarmTime;                                         /* 故障告警、恢复的时间 */      
    char          aucBoardID[20];                                     /* 单板类型 */            
    unsigned char ucAlarmInfo[ALARM_ADDINFO_STRING_MAX];             /* 故障描述 */           
}ALARM_INFORMATION_T;


/*日志管理 */
#define  EVT_NETLOG_CMD      0x80000
typedef struct
{
    unsigned int  dwLogLevel;                                          /* 日志级别 */                
    unsigned int  dwLogType;                                           /*  日志类型 */               
    IP_ADDR_T      tBoardIP;                                           /* 报警推送的服务器地址 */	         
    unsigned char ucAlarmInfo[LOG_STRING_MAX_LEN];                     /* 故障描述 */                
}LOG_INFORMATION_T;

#define  EVT_NETLOG_TYPE_SYS          0x0002 0010                      /* 系统运行 */
#define  EVT_NETLOG_TYPE_ALARM        0x0002 0020                      /* 故障报警 */
#define  EVT_NETLOG_TYPE_OPERATION    0x0002 0030                      /* 操作日志 */
#define  EVT_NETLOG_TYPE_RESERVER     0x0002 00F0                      /* 0x0002 0030 ~F0 保留 */
/*日志打印级别 */
#define  EVT_NETLOG_LEVEL_URGENT         0X0000 0001
#define  EVT_NETLOG_LEVEL_IMPORTANT      0X0000 0002
#define  EVT_NETLOG_LEVEL_COMMON         0X0000 0003


#endif
