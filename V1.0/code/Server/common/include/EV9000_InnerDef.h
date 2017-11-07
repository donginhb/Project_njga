/******************************************************************************

                  版权所有 (C), 2001-2013, 金智视讯技术有限公司

 ******************************************************************************
  文 件 名   : EV9000_InnerDef.h
  版 本 号   : 初稿
  作    者   : yanghaifeng
  生成日期   : 2013年7月2日 星期二
  最近修改   :
  功能描述   : EV9000系统对内公共数据定义头文件
  函数列表   :
  修改历史   :
  1.日    期   : 2013年7月2日 星期二
    作    者   : yanghaifeng
    修改内容   : 创建文件

******************************************************************************/
#ifndef EV9000_INNER_DEF_H
#define EV9000_INNER_DEF_H

/*----------------------------------------------*
 * 包含头文件                                   *
 *----------------------------------------------*/
#include "EV9000_ExtDef.h"

#define  EV9000_VERSION        "V4.04B13"

#define  EV9000_FTP_USERNAME   "ftpuser"
#define  EV9000_FTP_PASSWORD   "ftppasswd"
#define  EV9000_FTP_DIR        "ftp"

#define  EV9000_CTS_NAME       "CTS"
#define  EV9000_MAP_NAME       "MAP"

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

/*----------------------------------------------*
 * 数据库结构定义                                *
 *----------------------------------------------*/
//协议类型定义 
#define EV9000_STACKTYPE_GB28181           0     //GB28181协议
#define EV9000_STACKTYPE_ONVIF             1     //ONVIF协议

//111~130表示类型为前端主设备//
#define EV9000_DEVICETYPE_DVR              111   //DVR
#define EV9000_DEVICETYPE_VIDEOSERVER      112   //视频服务器
#define EV9000_DEVICETYPE_CODER            113   //编码器
#define EV9000_DEVICETYPE_DECODER          114   //解码器
#define EV9000_DEVICETYPE_VIDEOMITRIX      115   //视频切换矩阵
#define EV9000_DEVICETYPE_AUDIOMITRIX      116   //音频切换矩阵
#define EV9000_DEVICETYPE_ALARMCONTROLER   117   //报警编码器
#define EV9000_DEVICETYPE_NVR              118   //网络视频录像机

//131~191表示类型为前端外围设备//
#define EV9000_DEVICETYPE_CAMERA           131   //摄像机
#define EV9000_DEVICETYPE_IPC              132   //网络摄像机
#define EV9000_DEVICETYPE_SCREEN           133   //显示器
#define EV9000_DEVICETYPE_ALARMINPUT       134   //报警输入设备
#define EV9000_DEVICETYPE_ALARMOUTPUT      135   //报警输出设备
#define EV9000_DEVICETYPE_AUDIOINPUT       136   //语音输入设备
#define EV9000_DEVICETYPE_AUDIOOUTPUT      137   //语音输出设备
#define EV9000_DEVICETYPE_MOBILE           138   //移动传输设备
#define EV9000_DEVICETYPE_PERIPHERY        139   //其他外围设备

//RCU定义的逻辑设备类型
#define EV9000_DEVICETYPE_RCU_YAOXIN       180   //遥信
#define EV9000_DEVICETYPE_RCU_YAOKONG      181   //遥控
#define EV9000_DEVICETYPE_RCU_YAOCE        182   //遥测
#define EV9000_DEVICETYPE_RCU_YAOTIAO      183   //遥调
#define EV9000_DEVICETYPE_RCU_SHIJIAN      184   //事件量
#define EV9000_DEVICETYPE_RCU_MINGLIN      185   //命令量

//200~299表示类型为平台设备//
#define EV9000_DEVICETYPE_SIPSERVER        200   //sip服务器
#define EV9000_DEVICETYPE_WEBSERVER        201   //Web/应用服务器
#define EV9000_DEVICETYPE_MEDIASERVER      202   //媒体服务器
#define EV9000_DEVICETYPE_PROCSERVER       203   //代理服务器
#define EV9000_DEVICETYPE_SAFESERVER       204   //安全服务器
#define EV9000_DEVICETYPE_ALARMSERVER      205   //报警服务器
#define EV9000_DEVICETYPE_DBSERVER         206   //数据库服务器
#define EV9000_DEVICETYPE_GISSERVER        207   //GIS服务器
#define EV9000_DEVICETYPE_MANAGERSERVER    208   //管理服务器
#define EV9000_DEVICETYPE_MGWSERVER        209   //接入网关
#define EV9000_DEVICETYPE_MEDIA_STORE_SERVER    210   //媒体存储服务器编码
#define EV9000_DEVICETYPE_SIGNAL_SEC_GW    211   //信令安全路由网关编码
#define EV9000_DEVICETYPE_TRAFFIC_GROUP    215   //业务分组编码
#define EV9000_DEVICETYPE_VIRTURAL_ORGAN   216   //虚拟组织编码

//Onfiv代理设备编码
#define EV9000_DEVICETYPE_ONVIFPROXY             501   //ONVIF代理

//视频质量诊断设备编码
#define EV9000_DEVICETYPE_VIDEODIAGNOSIS         601   //视频质量诊断

//智能行为分析设备编码
#define EV9000_DEVICETYPE_INTELLIGENTANALYSIS    602   //智能行为分析

//Onvif设备的类型定义
#define EV9000_ONVIF_DEVICETYPE_DEVICE	10001	//Onvif设备

//单板状态
#define EV9000_BOARD_STATUS_ONLINE          0x00000001    //在线
#define EV9000_BOARD_STATUS_OFFLINE         0x00000002    //离线
#define EV9000_BOARD_STATUS_ALARM           0x00000004    //报警

//日志来源
#define EV9000_LOG_FROMTYPE_CMS             1    //CMS
#define EV9000_LOG_FROMTYPE_TSU             2    //TSU
#define EV9000_LOG_FROMTYPE_ONVIFPROXY      3    //ONVIF代理
#define EV9000_LOG_FROMTYPE_DM8168DEC       4    //嵌入式解码器
#define EV9000_LOG_FROMTYPE_DM8168IVA       5    //智能分析板卡-视频行为分析
#define EV9000_LOG_FROMTYPE_DM8168IVD       6    //智能分析板卡-视频诊断

//日志类型
#define EV9000_LOG_TYPE_USER                1    //用户操作
#define EV9000_LOG_TYPE_SYSTEM              2    //系统运行
#define EV9000_LOG_TYPE_ALARM               3    //报警

//日志级别
#define EV9000_LOG_LEVEL_NORMAL             1    //一般
#define EV9000_LOG_LEVEL_WARNING            2    //警告
#define EV9000_LOG_LEVEL_ERROR              3    //错误
#define EV9000_LOG_LEVEL_IMPORTANT          4    //重要

//报警设备类型
#define  EV9000_ALARM_DEVICE_MJ             0x00010001   //门禁
#define  EV9000_ALARM_DEVICE_DZXG           0x00010002   //电子巡更
#define  EV9000_ALARM_DEVICE_HWBJ           0x00010003   //红外报警
#define  EV9000_ALARM_DEVICE_DZWL           0x00010004   //电子围栏
#define  EV9000_ALARM_DEVICE_SGBJ           0x00010005   //声光报警
#define  EV9000_ALARM_DEVICE_HJ             0x00010006   //火警
#define  EV9000_ALARM_DEVICE_TEMPERATURE    0x00010007   //温度报警
#define  EV9000_ALARM_DEVICE_HUMIDITY       0x00010008   //湿度报警
#define  EV9000_ALARM_DEVICE_YWCGQ          0x00010009   //烟雾传感器
#define  EV9000_ALARM_DEVICE_TYKG           0x0001000A   //通用开关
#define  EV9000_ALARM_DEVICE_KTKG           0x0001000B   //空调开关
#define  EV9000_ALARM_DEVICE_KTGZMS         0x0001000C   //空调工作模式

//报警设备类型，添加RCU宏定义
#define  EV9000_ALARM_DEVICE_TYUNDEFINE     0x00000000   //通用,未定义
#define  EV9000_ALARM_DEVICE_TYSTATUS       0x00000001   //通用,设备状态
#define  EV9000_ALARM_DEVICE_YAOXIN         0x00010000   //通用,遥信量
#define  EV9000_ALARM_DEVICE_YAOKONG        0x00010001   //通用,遥控量
#define  EV9000_ALARM_DEVICE_YAOCE          0x00010002   //通用,遥测量
#define  EV9000_ALARM_DEVICE_YAOTIAO        0x00010003   //通用,遥调量
#define  EV9000_ALARM_DEVICE_SHIJIAN        0x00010004   //通用,事件量
#define  EV9000_ALARM_DEVICE_MINGLING       0x00010005   //通用,命令量
#define  EV9000_ALARM_DEVICE_HWDS           0x00020000   //报警,红外对射
#define  EV9000_ALARM_DEVICE_DZWL           0x00020001   //报警,电子围栏
#define  EV9000_ALARM_DEVICE_BJSC           0x00020002   //报警,报警输出
#define  EV9000_ALARM_DEVICE_MCZT           0x00030000   //门禁,门磁状态
#define  EV9000_ALARM_DEVICE_CMAN           0x00030001   //门禁,出门按钮
#define  EV9000_ALARM_DEVICE_XPZT           0x00030002   //门禁,胁迫状态
#define  EV9000_ALARM_DEVICE_BJSR           0x00030003   //门禁,报警输入
#define  EV9000_ALARM_DEVICE_MJBJSC         0x00030004   //门禁,报警输出
#define  EV9000_ALARM_DEVICE_SKJL           0x00030005   //门禁,刷卡记录
#define  EV9000_ALARM_DEVICE_FKSQ           0x00030006   //门禁,发卡授权
#define  EV9000_ALARM_DEVICE_SKSQ           0x00030007   //门禁,收卡撤权
#define  EV9000_ALARM_DEVICE_YCKM           0x00030008   //门禁,远程开门
#define  EV9000_ALARM_DEVICE_HJWD           0x00040000   //环境量,温度
#define  EV9000_ALARM_DEVICE_HJSD           0x00040001   //环境量,湿度
#define  EV9000_ALARM_DEVICE_HJSF6          0x00040002   //环境量,SF6浓度
#define  EV9000_ALARM_DEVICE_HJCO2          0x00040003   //环境量,CO2浓度
#define  EV9000_ALARM_DEVICE_HJO2           0x00040004   //环境量,氧气浓度
#define  EV9000_ALARM_DEVICE_XFYG           0x00050000   //消防,烟感探测器
#define  EV9000_ALARM_DEVICE_XFWG           0x00050001   //消防,温感探测器
#define  EV9000_ALARM_DEVICE_KTKG           0x00060000   //空调,开关
#define  EV9000_ALARM_DEVICE_KTFS           0x00060001   //空调,风速
#define  EV9000_ALARM_DEVICE_KTMS           0x00060002   //空调,模式1-送风|2-制热|3-制冷|4-自动|5-除湿|其他无效';
#define  EV9000_ALARM_DEVICE_FJQT           0x00070001   //风机,起停

//TCP端口
#define EV9000_TCP_SERVERPORT               20001        //连接服务端端口

//系统信息类型
typedef enum
{
	EV9000_SYSTEMINFO_VERSERVER = 0,     //系统版本
		EV9000_SYSTEMINFO_VERCLIENT,         //FTP服务器上最新的客户端版本,
		EV9000_SYSTEMINFO_VERMAP,            //FTP服务器上最新的地图版本,
		EV9000_SYSTEMINFO_SERVERPORT,
		EV9000_SYSTEMINFO_SERVERADDR,
		EV9000_SYSTEMINFO_CLIENTFORCEDUODATE, //客户端是否强制更新
		EV9000_SYSTEMINFO_MAPFORCEDUODATE,    //地图是否强制更新
		EV9000_SYSTEMINFO_CLIENTDESCRIPTION,  //客户端更新描述
		EV9000_SYSTEMINFO_MAPDESCRIPTION,     //地图更新描述
		EV9000_SYSTEMINFO_SYSCURTIME,            //当前系统时间
}EV9000_SYSTEMINFO_TYPE;

//EV9000单板配置信息结构
typedef struct EV9000_BoardConfig
{
	unsigned int         nID;                                      //记录编号
	unsigned int         nBoardType;                               //单板类型
	unsigned int         nEnable;                                  //是否启用
	char                 strBoardID[EV9000_IDCODE_LEN];            //单板编码
	unsigned int         nSlotID;                                  //槽位号
	unsigned int         nStatus;                                  //单板状态
	char                 strCMSID[EV9000_IDCODE_LEN];              //CMS 编号
	unsigned int         nResved1;                                 //保留1
	char                 strResved2[EV9000_SHORT_STRING_LEN];      //保留2
}EV9000_BoardConfig, *LPEV9000_BoardConfig;

typedef struct EV9000_BoardNetConfig
{
	unsigned int         nID;                                      //记录编号
	unsigned int         nBoardIndex;                             //单板索引
	unsigned int         nEnable;                                  //是否启用
	unsigned int         nEthID;                                   //网络口编号
	unsigned int         nPort;                                    //SIP端口号
	char                 strIP[EV9000_SHORT_STRING_LEN];           //单板地址
	char                 strMask[EV9000_SHORT_STRING_LEN];         //单板掩码
	char                 strGateWay[EV9000_SHORT_STRING_LEN];      //单板网关
	char                 strHost[EV9000_SHORT_STRING_LEN];         //单板域名
	unsigned int         nStatus;                                  //单板端口状态
	char                 strCMSID[EV9000_IDCODE_LEN];              //CMS 编号
	unsigned int         nResved1;                                 //保留1
	char                 strResved2[EV9000_SHORT_STRING_LEN];      //保留2
}EV9000_BoardNetConfig, *LPEV9000_BoardNetConfig;

//互联路由表
typedef struct EV9000_RouteNetConfig
{
	unsigned int         nID;                                      //记录编号
	char                 strServerID[EV9000_IDCODE_LEN];           //目的服务器编码
	char                 strServerIP[EV9000_SHORT_STRING_LEN];     //目的服务器地址
	char                 strServerHost[EV9000_SHORT_STRING_LEN];   //目的服务器域名
	unsigned int         nServerPort;                              //目的服务器端口
	char                 strUserName[EV9000_SHORT_STRING_LEN];     //注册用户名
	char                 strPassword[EV9000_SHORT_STRING_LEN];     //注册密码
	unsigned int         nLinkType;                                //联网类型:0:上下级，1：同级，默认0
	unsigned int         nResved1;                                 //保留1
	char                 strResved2[EV9000_SHORT_STRING_LEN];      //保留2
}EV9000_RouteNetConfig, *LPEV9000_RouteNetConfig;

//用户逻辑设备分组配置表
// typedef struct EV9000_UserLogicDeviceGroupConfig
// {
// 	unsigned int         nID;                                      //记录编号
// 	char                 strUserID[EV9000_IDCODE_LEN];              //用户ID
// 	char                 strName[EV9000_LONG_STRING_LEN];          //组名称
// 	unsigned int         nSortID;                                  //同一父节点下组排序编号，默认0不排序
// 	unsigned int         nParentID;                                //父节点编号
// 	unsigned int         nResved1;                                 //保留1
// 	char                 strResved2[EV9000_SHORT_STRING_LEN];      //保留2
// }EV9000_UserLogicDeviceGroupConfig, *LPEV9000_UserLogicDeviceGroupConfig;

//用户逻辑设备分组关系表
// typedef struct EV9000_UserLogicDeviceMapGroupConfig
// {
// 	unsigned int         nID;                                       //记录编号
//     char                 strUserID[EV9000_IDCODE_LEN];              //用户ID
// 	int                  nGroupID;                                  //点位组编号
// 	int                  nDeviceIndex;                              //设备ID
// 	unsigned int         nSortID;                                   //同一父节点下组排序编号，默认0不排序
// 	unsigned int         nResved1;                                  //保留1
// 	char                 strResved2[EV9000_SHORT_STRING_LEN];       //保留2 
// }EV9000_UserLogicDeviceMapGroupConfig, *LPEV9000_UserLogicDeviceMapGroupConfig; 

//系统信息表
typedef struct EV9000_SystemConfig
{
	unsigned int         nID;                                       //记录编号
	char                 strKeyName[EV9000_SHORT_STRING_LEN];       //关键字段名称
	char                 strKeyValue[EV9000_NORMAL_STRING_LEN];     //关键字段值
	unsigned int         nResved1;                                  //保留1
	char                 strResved2[EV9000_SHORT_STRING_LEN];       //保留2 
}EV9000_SystemConfig, *LPEV9000_SystemConfig;   

//标准物理设备表
typedef struct EV9000_GBPhyDeviceConfig
{
	unsigned int         nID;                                       //记录编号
	char                 strDeviceID[EV9000_IDCODE_LEN];            //设备ID
	char                 strCMSID[EV9000_IDCODE_LEN];               //服务器编号
	unsigned int         nEnable;                                   //是否启用
	unsigned int         nDeviceType;                               //设备类型
	char                 strDeviceIP[EV9000_SHORT_STRING_LEN];      //设备IP
	unsigned int         nMaxCamera;                                //设备通道数
	unsigned int         nMaxAlarm;                                 //设备报警通道数
	char                 strUserName[EV9000_SHORT_STRING_LEN];      //注册用户名
	char                 strPassword[EV9000_SHORT_STRING_LEN];      //注册密码
	unsigned int         nStatus;                                   //设备状态
	char                 strModel[EV9000_SHORT_STRING_LEN];         //设备型号
	char                 strFirmware[EV9000_SHORT_STRING_LEN];      //设备版本
	char                 strManufacturer[EV9000_NORMAL_STRING_LEN]; //设备生产商
	unsigned int         nLinkType;                                 //联网类型:0:上下级，1：同级，默认0
	unsigned int         nResved1;                                  //保留1
	char                 strResved2[EV9000_SHORT_STRING_LEN];       //保留2 
}EV9000_GBPhyDeviceConfig, *LPEV9000_GBPhyDeviceConfig;

//标准物理设备临时表
typedef struct EV9000_GBPhyDeviceTempConfig
{
	unsigned int         nID;                                       //记录编号
	char                 strDeviceID[EV9000_IDCODE_LEN];            //点位编号
	char                 strCMSID[EV9000_IDCODE_LEN];               //服务器编号
	unsigned int         nDeviceType;                               //设备类型
	char                 strDeviceIP[EV9000_SHORT_STRING_LEN];      //设备IP
	unsigned int         nMaxCamera;                                //设备通道数
	unsigned int         nMaxAlarm;                                 //设备报警通道数
	char                 strUserName[EV9000_SHORT_STRING_LEN];      //注册用户名
	char                 strPassword[EV9000_SHORT_STRING_LEN];      //注册密码
	unsigned int         nStatus;                                   //设备状态
	char                 strModel[EV9000_SHORT_STRING_LEN];         //设备型号
	char                 strFirmware[EV9000_SHORT_STRING_LEN];      //设备版本
	char                 strManufacturer[EV9000_NORMAL_STRING_LEN]; //设备生产商
	unsigned int         nLinkType;                                 //联网类型:0:上下级，1：同级，默认0
	unsigned int         nResved1;                                  //保留1
	char                 strResved2[EV9000_SHORT_STRING_LEN];       //保留2 
}EV9000_GBPhyDeviceTempConfig, *LPEV9000_GBPhyDeviceTempConfig;

//外围设备配置表
typedef struct EV9000_OuterDeviceConfig
{
	unsigned int         nID;                                       //记录编号
	char                 strDeviceID[EV9000_IDCODE_LEN];            //点位编号
	char                 strDeviceName[EV9000_SHORT_STRING_LEN];	//关键字段名称
	char                 strDeviceType[EV9000_NORMAL_STRING_LEN];   //关键字段值
	char                 strDeviceParam[EV9000_NORMAL_STRING_LEN];  //设备参数
	unsigned int         nResved1;                                  //保留1
	char                 strResved2[EV9000_SHORT_STRING_LEN];       //保留2 
}EV9000_OuterDeviceConfig, *LPEV9000_OuterDeviceConfig;

//非标物理设备表
typedef struct EV9000_UNGBPhyDeviceConfig
{
	unsigned int         nID;                                       //记录编号
	unsigned int         nDeviceType;                               //设备类型
	char                 strDeviceIP[EV9000_SHORT_STRING_LEN];      //设备IP
	unsigned int         nDevicePort;                               //设备端口
	char                 strUserName[EV9000_SHORT_STRING_LEN];      //用户名
	char                 strPassword[EV9000_SHORT_STRING_LEN];      //密码
	unsigned int         nStreamType;                               //主从流标示
	unsigned int         nRecordType;                               //是否前端录像
	unsigned int         nResved1;                                  //保留1
	char                 strResved2[EV9000_SHORT_STRING_LEN];       //保留2 
}EV9000_UNGBPhyDeviceConfig, *LPEV9000_UNGBPhyDeviceConfig;

//非标物理设备通道配置表
typedef struct EV9000_UNGBPhyDeviceChannelConfig
{
	unsigned int         nID;                                       //记录编号
	unsigned int         nDeviceID;                                 //设备编号
	char                 strDeviceChannel[EV9000_NORMAL_STRING_LEN];//设备通道
	unsigned int         nMapChannel;                               //映射通道
	unsigned int         nResved1;                                  //保留1
	char                 strResved2[EV9000_SHORT_STRING_LEN];       //保留2 
}EV9000_UNGBPhyDeviceChannelConfig, *LPEV9000_UNGBPhyDeviceChannelConfig;

//录像策略表
typedef struct EV9000_RecordSchedConfig
{
	unsigned int         nID;                                       //记录编号
	int                  nDeviceIndex;                              //点位编号
	int                  nRecordEnable;                             //是否录像
	unsigned int         nDays;                                     //录像天数
	unsigned int         nTimeLength;                               //录像时长
	unsigned int         nType;                                     //录像类型
	unsigned int         nStreamType;                                //码流类型
	unsigned int         nTimeOfAllWeek;                            //全录
	unsigned int         nBandWidth;                                //带宽
	unsigned int         nResved1;                                  //保留1
	char                 strResved2[EV9000_SHORT_STRING_LEN];       //保留2 
}EV9000_RecordSchedConfig, *LPEV9000_RecordSchedConfig;

//录像时刻策略表
typedef struct EV9000_RecordTimeSchedConfig
{
	unsigned int         nID;                                       //记录编号
	unsigned int         nRecordSchedIndex;                         //策略编号
	unsigned int         nDayInWeek;                                //星期几
	unsigned int         nBeginTime;                                //录像开始时间
	unsigned int         nEndTime;                                  //录像结束时间
	unsigned int         nResved1;                                  //保留1
	char                 strResved2[EV9000_SHORT_STRING_LEN];       //保留2 
    
}EV9000_RecordTimeSchedConfig, *LPEV9000_RecordTimeSchedConfig;

//用户设备权限表
typedef struct EV9000_UserDevicePermConfig
{
	unsigned int         nID;                                       //记录编号
	unsigned int         nUserIndex;                                //用户编号
	unsigned int         nDeviceIndex;                              //设备编号
	unsigned int         nResved1;                                  //保留1
	char                 strResved2[EV9000_SHORT_STRING_LEN];       //保留2 
}EV9000_UserDevicePermConfig, *LPEV9000_UserDevicePermConfig;

//用户组表
typedef struct EV9000_UserGroupConfig
{
	unsigned int         nID;                                       //记录编号
	char                 strGroupName[EV9000_SHORT_STRING_LEN];     //组名
	unsigned int         nPermission;                               //操作权限
	unsigned int         nResved1;                                  //保留1
	char                 strResved2[EV9000_SHORT_STRING_LEN];       //保留2 
}EV9000_UserGroupConfig, *LPEV9000_UserGroupConfig;

//用户组设备权限表
typedef struct EV9000_GroupDevicePermConfig
{
	unsigned int         nID;                                       //记录编号
	int                  nGroupID;                                  //点位组编号
	int                  nDeviceIndex;                              //设备编号
	unsigned int         nResved1;                                  //保留1
	char                 strResved2[EV9000_SHORT_STRING_LEN];       //保留2 
}EV9000_GroupDevicePermConfig, *LPEV9000_GroupDevicePermConfig;

//用户及用户组关系
typedef struct EV9000_UserMapGroupConfig
{
	unsigned int         nID;                                       //记录编号
	int                  nGroupID;                                  //点位组编号
	unsigned int         nUserIndex;                                //用户编号
	unsigned int         nResved1;                                  //保留1
	char                 strResved2[EV9000_SHORT_STRING_LEN];       //保留2 
}EV9000_UserMapGroupConfig, *LPEV9000_UserMapGroupConfig;

//轮巡权限表
typedef struct EV9000_PollPermissionConfig
{
	unsigned int         nID;                                       //记录编号
	unsigned int         nPollID;                                   //轮巡ID
	char                 strUserID[EV9000_IDCODE_LEN];              //用户ID
	unsigned int         nResved1;                                  //保留1
	char                 strResved2[EV9000_SHORT_STRING_LEN];       //保留2 
}EV9000_PollPermissionConfig, *LPEV9000_PollPermissionConfig;


//预案权限表
typedef struct EV9000_PlanPermissionConfig
{
	unsigned int         nID;                                       //记录编号
	unsigned int         nPlanID;                                   //预案编号
	char                 strUserID[EV9000_IDCODE_LEN];              //用户编号
	unsigned int         nResved1;                                  //保留1
	char                 strResved2[EV9000_SHORT_STRING_LEN];       //保留2 
}EV9000_PlanPermissionConfig, *LPEV9000_PlanPermissionConfig;

//预案联动表
typedef struct EV9000_PlanLinkageConfig
{
	unsigned int         nID;                                       //记录编号
	unsigned int         nAlarmSourceID;                            //报警源编号
	unsigned int         nStartPlanID;                              //报警开始联动预案编号
	unsigned int         nStopPlanID;                               //报警结束联动预案编号
	unsigned int         nType;                                     //报警类型
	unsigned int         nLevel;                                    //报警级别
	unsigned int         nRepeatEnable;                             //重复触发
	unsigned int         nResved1;                                  //保留1
	char                 strResved2[EV9000_SHORT_STRING_LEN];       //保留2 
}EV9000_PlanLinkageConfig, *LPEV9000_PlanLinkageConfig;


//巡航权限表
typedef struct EV9000_CruisePermissionConfig
{
	unsigned int         nID;                                       //记录编号
	unsigned int         nCruiseID;                                 //巡航编号
	char                 strUserID[EV9000_IDCODE_LEN];              //用户编号
	unsigned int         nResved1;                                  //保留1
	char                 strResved2[EV9000_SHORT_STRING_LEN];       //保留2 
}EV9000_CruisePermissionConfig, *LPEV9000_CruisePermissionConfig;



//用户操作记录表
typedef struct EV9000_UserLogRecord
{
	unsigned int         nID;                                       //记录编号
	char                 strLogID[EV9000_SHORT_STRING_LEN];         //日志编号
	char                 strUserID[EV9000_IDCODE_LEN];              //用户编号
	unsigned int         nType;                                     //日志类型
	unsigned int         nLevel;                                    //日志级别
	unsigned int         nTime;                                     //产生时间
	char                 strInfo[EV9000_LONG_STRING_LEN];           //日志描述
	unsigned int         nResved1;                                  //保留1
	char                 strResved2[EV9000_SHORT_STRING_LEN];       //保留2 
}EV9000_UserLogRecord, *LPEV9000_UserLogRecord;

//录像标记表
typedef struct EV9000_FileTagRecord
{
	unsigned int         nID;                                       //记录编号
	char                 strTagID[EV9000_SHORT_STRING_LEN];         //标记编号
	int                  nDeviceIndex;                              //设备编号
	unsigned int         nType;                                     //标记类型
	unsigned int         nLevel;                                    //标记级别
	unsigned int         nTime;                                     //产生时间
	char                 strInfo[EV9000_LONG_STRING_LEN];           //标记描述
	unsigned int         nResved1;                                  //保留1
	char                 strResved2[EV9000_SHORT_STRING_LEN];       //保留2 
}EV9000_FileTagRecord, *LPEV9000_FileTagRecord;

//设备运维表
typedef struct EV9000_DeviceStatusRecord
{
	unsigned int         nID;                                       //记录编号
	char                 strStatusID[EV9000_SHORT_STRING_LEN];      //状态编号
	int                  nDeviceIndex;                              //设备编号
	unsigned int         nStatus;                                   //状态类型
	unsigned int         nTime;                                     //产生时间
	unsigned int         nResved1;                                  //保留1
	char                 strResved2[EV9000_SHORT_STRING_LEN];       //保留2 
}EV9000_DeviceStatusRecord, *LPEV9000_DeviceStatusRecord;

typedef struct
{
    unsigned int  nIndex;                                          /* 设备索引号 */
	unsigned int  nDeviceType;                                     // 设备类型
    unsigned int  nAlarmIP;                                        /* 报警源的IP地址 */
    unsigned int  nAlarmLevel;                                     /* 故障告警级别 */          
    unsigned int  nAlarmID;                                        /*  故障告警编码*/     
    int           nAlarmTime;                                         /* 故障告警、恢复的时间 */      
    char          aucBoardID[24];                                     /* 单板类型 */            
    unsigned char ucAlarmInfo[EV9000_LONG_STRING_LEN];             /* 故障描述 */           
}EV9000_LOG;

typedef struct
{
    unsigned int         nID;                                            /* 记录编号 */
	char                 strCode[EV9000_SHORT_STRING_LEN];               /* 编号 */
	char                 strName[EV9000_SHORT_STRING_LEN];               /* 名称 */
    unsigned int         nResved1;                                       //保留1
	char                 strResved2[EV9000_SHORT_STRING_LEN];            //保留2           
}EV9000_GBCode;

//Onvif物理设备表
typedef struct _EV9000_OnvifPhyDeviceConfig
{
	unsigned int         nID;                                       //记录编号
	char                 strDeviceID[EV9000_Onvif_UUID_Len];        //设备ID(UUID)
	char                 strCMSID[EV9000_IDCODE_LEN];               //服务器编号
	char                 strOnvifProxyID[EV9000_IDCODE_LEN];        //ProxyID
	unsigned int         nEnable;                                   //是否启用
	unsigned int         nDeviceType;                               //设备类型
	char                 strDeviceIP[EV9000_SHORT_STRING_LEN];      //设备IP
	unsigned int         nMaxCamera;                                //设备通道数
	unsigned int         nMaxAlarm;                                 //设备报警通道数
	char                 strUserName[EV9000_SHORT_STRING_LEN];      //注册用户名
	char                 strPassword[EV9000_SHORT_STRING_LEN];      //注册密码
	unsigned int         nStatus;                                   //设备状态
	char                 strModel[EV9000_SHORT_STRING_LEN];         //设备型号
	char                 strFirmware[EV9000_SHORT_STRING_LEN];      //设备版本
	char                 strManufacturer[EV9000_NORMAL_STRING_LEN]; //设备生产商
	unsigned int         nLinkType;                                 //联网类型:0:上下级，1：同级，默认0
	unsigned int         nResved1;                                  //保留1
	char                 strResved2[EV9000_SHORT_STRING_LEN];       //保留2
}EV9000_OnvifPhyDeviceConfig, *LPEV9000_OnvifPhyDeviceConfig;

//Onvif逻辑设备表
typedef struct _EV9000_OnvifLogicDeviceConfig
{
	unsigned int         nID;                                       //记录编号
	char                 strDeviceID[EV9000_IDCODE_LEN];            //点位编号
	char                 strCMSID[EV9000_IDCODE_LEN];               //CMS 编号
	unsigned int         nEnable;                                   //是否启用	
	unsigned int         nDeviceType;                               //设备类型
	char                 strDeviceName[EV9000_NORMAL_STRING_LEN];   //点位名称	
	unsigned int         nPhyDeviceIndex;                           //媒体物理设备ID
	unsigned int         nPhyDeviceChannel;                         //媒体物理设备通道
    char                 strPhyDeviceChannelMark[EV9000_LONG_STRING_LEN];   //媒体物理设备通道标识
	unsigned int         nCtrlEnable;                               //是否可控
	unsigned int         nMicEnable;                                //是否支持对讲
	unsigned int         nCtrlDeviceIndex;                          //控制设备ID
	unsigned int         nCtrlDeviceChannel;                        //控制设备通道
	unsigned int         nStreamCount;                              //是否支持多码流
	unsigned int         nStreamType;                               //码流类型
	unsigned int         nNeedCodec;                                //是否需要编解码
	unsigned int         nRecordType;                               //录像类型(前端、中心)
	char                 strManufacturer[EV9000_NORMAL_STRING_LEN]; //设备生产商
	char                 strModel[EV9000_SHORT_STRING_LEN];         //设备型号
	char                 strOwner[EV9000_SHORT_STRING_LEN];         //设备归属
	char                 strCivilCode[EV9000_SHORT_STRING_LEN];     //行政区域
	char                 strBlock[EV9000_SHORT_STRING_LEN];         //警区
	char                 strAddress[EV9000_NORMAL_STRING_LEN];      //安装地址
	unsigned int         nParental;                                 //是否有子设备
	char                 strParentID[EV9000_IDCODE_LEN];            //父设备/区域/系统ID
	unsigned int         nSafetyWay;                                //信令安全模式
	unsigned int         nRegisterWay;                              //注册方式
	char                 strCertNum[EV9000_SHORT_STRING_LEN];       //证书序列号
	unsigned int         nCertifiable;                              //证书有效标识
	unsigned int         nErrCode;                                  //无效原因码
	char                 strEndTime[EV9000_SHORT_STRING_LEN];       //证书终止有效期
	unsigned int         nSecrecy;                                  //保密属性
	char                 strIPAddress[EV9000_SHORT_STRING_LEN];     //IP地址
	unsigned int         nPort;                                     //端口号
	char                 strPassword[EV9000_SHORT_STRING_LEN];      //密码
	unsigned int         nStatus;                                   //设备状态
	double               dLongitude;                                //经度
	double               dLatitude;                                 //纬度
	unsigned int         nResved1;                                  //保留1
	char                 strResved2[EV9000_SHORT_STRING_LEN];       //保留2 
	unsigned int         nResved3;                                  //保留3
	char                 strResved4[EV9000_SHORT_STRING_LEN];       //保留4
}EV9000_OnvifLogicDeviceConfig, *LPEV9000_OnvifLogicDeviceConfig;

typedef enum
{
	WEBSTYTLE_PLAN = 0,
		WEBSTYTLE_POLL,
		WEBSTYTLE_ALARMLINKAGE,
		WEBSTYTLE_MANAGER,
		WEBSTYTLE_CRUISE,
		WEBSTYTLE_NPETVWALL,
		WEBSTYTLE_CLOUDREG,
}WEBSTYTLE_TYPE;

//新增结构体：
typedef struct EV9000_WebInterFaceConfig
{
	unsigned int          nID;                                   //记录编号
	WEBSTYTLE_TYPE        eWebStytle;                            //页面类型
	char                  strServerIP[EV9000_LONG_STRING_LEN];   //服务器IP
    unsigned int          nPort;                                 //端口号
	char                  strURL[EV9000_LONG_STRING_LEN];         // URL地址
	unsigned int          nResved1;                              //保留1
	char                  strResved2[EV9000_SHORT_STRING_LEN];   //保留2 
}EV9000_WebInterFaceConfig, *LPEV9000_WebInterFaceConfig;

//TCP接收数据head
typedef struct EV9000_TCP_Head 
{
	char                   mark;                                  //使用$表示每个rtp的开始
	unsigned short         length;                                //后续包的长度 
	unsigned int           nResved1;                              //保留1
	char                   strResved2[EV9000_SHORT_STRING_LEN];   //保留2 
}EV9000_TCP_Head, *LPEV9000_TCP_Head;
#endif
