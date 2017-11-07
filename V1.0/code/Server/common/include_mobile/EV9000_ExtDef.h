/******************************************************************************

                  版权所有 (C), 2001-2013, 金智视讯技术有限公司

 ******************************************************************************
  文 件 名   : EV9000_ExtDef.h
  版 本 号   : 初稿
  作    者   : yanghaifeng
  生成日期   : 2013年7月2日 星期二
  最近修改   :
  功能描述   : EV9000系统对外公共数据定义头文件
  函数列表   :
  修改历史   :
  1.日    期   : 2013年7月2日 星期二
    作    者   : yanghaifeng
    修改内容   : 创建文件

******************************************************************************/
#ifndef EV9000_EXT_DEF_H
#define EV9000_EXT_DEF_H

#if defined(_WIN32) //windows

#define EV9000APP_API  extern "C" __declspec(dllexport)
typedef  unsigned __int64 unsigned int64;

#elif defined(__linux__)

#define EV9000APP_API     extern "C"

#endif

/*----------------------------------------------*
 * 常量定义                                     *
 *----------------------------------------------*/


/*----------------------------------------------*
 * 宏定义                                       *
 *----------------------------------------------*/

#define EV9000_IDCODE_LEN             20+4    //统一编码最大长度
#define EV9000_SHORT_STRING_LEN       32+4    //短字符串长度
#define EV9000_NORMAL_STRING_LEN      64+4    //一般字符串长度
#define EV9000_LONG_STRING_LEN        128+4   //长字符串长度
#define EV9000_MAX_CHANNDELID         1024    //最大设备通道号
#define EV9000_Onvif_UUID_Len		  36+4    //Onvif 使用的设备的 UUID的长度.包括'-'字符

//用户权限
#define EV9000_USER_PERMISSION_REALPLAY     0x00000001    //实时视频
#define EV9000_USER_PERMISSION_CAMERACTRL   0x00000002    //球机控制
#define EV9000_USER_PERMISSION_PARAMCTRL    0x00000004    //参数调节

#define EV9000_USER_PERMISSION_PLAYBACK     0x00000008    //录像回放
#define EV9000_USER_PERMISSION_DOWNLOAD     0x00000010    //录像下载

#define EV9000_USER_PERMISSION_GIS          0x00000020    //电子地图

#define EV9000_USER_PERMISSION_TVWALL       0x00000040    //电视墙控制

#define EV9000_USER_PERMISSION_PLAN         0x00000080    //预案控制

#define EV9000_USER_PERMISSION_POLL         0x00000100    //轮巡控制

#define EV9000_USER_PERMISSION_ALARMLINKAGE 0x00000200    //报警联动

#define EV9000_USER_PERMISSION_MANAGER      0x00000400    //管理配置

//用户级别
#define EV9000_USER_LEVEL_SUPERADMIN        0             //超级管理员
#define EV9000_USER_LEVEL_ADMIN             1             //管理员
#define EV9000_USER_LEVEL_OPERATOR          2             //操作员
#define EV9000_USER_LEVEL_GENERAL           3             //一般用户

//流类型分三种
//1、GB28181里携带的PS数据中标示的
//2、RTP码流里携带的类型
//3、EV9000扩展的类型

//PS流音视频混在一个通道中发送
//其他类型音视频是在两个通道中发送

//GB28181中定义的流类型(用于协商)
#define EV9000_STREAMDATA_TYPE_PS             96

#define EV9000_STREAMDATA_TYPE_VIDEO_MPEG4    97
#define EV9000_STREAMDATA_TYPE_VIDEO_H264     98
#define	EV9000_STREAMDATA_TYPE_VIDEO_SVAC     99
		
#define	EV9000_STREAMDATA_TYPE_VIDEO_HIK      500
#define	EV9000_STREAMDATA_TYPE_VIDEO_DAH      501

#define	EV9000_STREAMDATA_TYPE_AUDIO_G723     4		
#define	EV9000_STREAMDATA_TYPE_AUDIO_G711A    8
#define	EV9000_STREAMDATA_TYPE_AUDIO_G722     9
#define	EV9000_STREAMDATA_TYPE_AUDIO_G729     18
#define	EV9000_STREAMDATA_TYPE_AUDIO_SVAC     20

//视频编码类型(用于编解码)
#define EV9000APP_VIDEOENCODE_TYPE_MPEG4      0x10
#define	EV9000APP_VIDEOENCODE_TYPE_H264       0x1B
#define EV9000APP_VIDEOENCODE_TYPE_GPU        0x6001
#define	EV9000APP_VIDEOENCODE_TYPE_SVAC       0x80
#define EV9000APP_VIDEOENCODE_TYPE_H265       0x48
 		
#define	EV9000APP_VIDEOENCODE_TYPE_HIK        0x500
#define	EV9000APP_VIDEOENCODE_TYPE_DAH        0x501

// 音频编码类型(用于编解码)
#define EV9000APP_AUDIOENCODE_TYPE_G711       0x90
#define	EV9000APP_AUDIOENCODE_TYPE_G722       0x92
#define EV9000APP_AUDIOENCODE_TYPE_G723       0x93
#define EV9000APP_AUDIOENCODE_TYPE_G729       0x99
#define EV9000APP_AUDIOENCODE_TYPE_SAVC       0x9B

//点位状态
#define EV9000_LOGICDEVICE_STATUS_ONLINE      0x00000000  //点位在线
#define EV9000_LOGICDEVICE_STATUS_INVAILED    0x00000001  //点位无效
#define EV9000_LOGICDEVICE_STATUS_OFFLINE     0x00000002  //点位离线
#define EV9000_LOGICDEVICE_STATUS_NOVIDEO     0x00000004  //点位无视频信号
#define EV9000_LOGICDEVICE_STATUS_INTEL       0x00000008  //点位正在做智能分析
#define EV9000_LOGICDEVICE_STATUS_ALARM       0x00000010  //点位正在报警

//录像类型
#define EV9000_RECORD_TYPE_NORMAL           1    //普通录像
#define EV9000_RECORD_TYPE_INTELLIGENCE     2    //智能录像
#define EV9000_RECORD_TYPE_ALARM            3    //报警录像
#define EV9000_RECORD_TYPE_BACKUP           4    //备份录像

//码流类型
#define EV9000_STREAM_TYPE_MASTER           1    //主流
#define EV9000_STREAM_TYPE_SLAVE            2    //从流
#define EV9000_STREAM_TYPE_TSC            	3    //转码码流

#define EV9000_STREAM_TYPE_INTELLIGENCE     10   //智能分析流

/*----------------------------------------------*
 * 数据结构定义                           *
 *----------------------------------------------*/

// 时间结构
typedef struct EV9000_TIME
{
	unsigned short   usYear;            //年
	unsigned short   usMonth;           //月
	unsigned short   usDay;             //日
	unsigned short   usHour;            //时
	unsigned short   usMinute;          //分
	unsigned short   usSecond;          //秒
}EV9000_TIME, *LPEV9000_TIME;

typedef struct  
{
	unsigned short   usLeft;            //左
	unsigned short   usRight;           //右
	unsigned short   usTop;             //上
	unsigned short   usBottom;          //下
}EV9000_RECT;

//用户信息
typedef struct EV9000_UserConfig
{
	unsigned int         nID;                                       //记录编号
	char                 strUserID[EV9000_IDCODE_LEN];              //用户编号
	unsigned int         nEbable;                                   //是否启用
	char                 strUserName[EV9000_SHORT_STRING_LEN];      //注册用户名
	char                 strPassword[EV9000_SHORT_STRING_LEN];      //注册密码
	unsigned int         nLevel;                                    //用户级别
	unsigned int         nPermission;                               //用户权限
	char                 strLogInIP[EV9000_SHORT_STRING_LEN];       //用户指定登录IP
	char                 strLogInMAC[EV9000_SHORT_STRING_LEN];      //用户指定登录MAC
	char                 strRealName[EV9000_SHORT_STRING_LEN];      //用户真实姓名
	char                 strTel[EV9000_SHORT_STRING_LEN];           //用户联系方式
	unsigned int         nResved1;                                  //保留1
	char                 strResved2[EV9000_SHORT_STRING_LEN];       //保留2 
}EV9000_UserConfig, *LPEV9000_UserConfig;

typedef struct EV9000_OnLineUserConfig
{
	unsigned int          nID;                                        //用户记录编号
	char                  strUserName[EV9000_SHORT_STRING_LEN];       //用户名
	char                  strRealName[EV9000_SHORT_STRING_LEN];       //用户真实姓名
	char                  strLogInIP[EV9000_SHORT_STRING_LEN];        //用户登录IP
	unsigned int          nPort;                                      //用户登录端口号
	char                  strTel[EV9000_SHORT_STRING_LEN];            //用户联系方式
	unsigned int          nResved1;                                   //保留1
	char                  strResved2[EV9000_SHORT_STRING_LEN];        //保留2
}EV9000_OnLineUserConfig, *LPEV9000_OnLineUserConfig;

//逻辑点位
typedef struct EV9000_GBLogicDeviceConfig
{
	unsigned int         nID;                                       //记录编号
	char                 strDeviceID[EV9000_IDCODE_LEN];            //点位编号
	char                 strCMSID[EV9000_IDCODE_LEN];               //CMS 编号
	unsigned int         nEnable;                                   //是否启用	
	unsigned int         nDeviceType;                               //设备类型
	char                 strDeviceName[EV9000_NORMAL_STRING_LEN];   //点位名称	
	unsigned int         nPhyDeviceIndex;                           //媒体物理设备ID
	unsigned int         nPhyDeviceChannel;                         //媒体物理设备通道
	unsigned int         nCtrlEnable;                               //是否可控
	unsigned int         nMicEnable;                                //是否支持对讲
	unsigned int         nFrameCount;                               //帧率
	unsigned int         nCtrlDeviceIndex;                          //控制设备ID
	unsigned int         nCtrlDeviceChannel;                        //控制设备通道
	unsigned int         nStreamCount;                              //码流数
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
	char                 strParam[EV9000_LONG_STRING_LEN];          //参数
	unsigned int         nIAEnable;                                 //是否正在进行智能分析
	unsigned int         nRecordByTimeEnable;                       //是否支持按时间回放
	unsigned int         nResved1;                                  //保留1
	char                 strResved2[EV9000_SHORT_STRING_LEN];       //保留2 
	unsigned int         nResved3;                                  //保留3
	char                 strResved4[EV9000_SHORT_STRING_LEN];       //保留4
}EV9000_GBLogicDeviceConfig, *LPEV9000_GBLogicDeviceConfig;

//逻辑设备分组配置表
typedef struct EV9000_LogicDeviceGroupConfig
{
	unsigned int         nID;                                      //记录编号
	char                 strGroupID[EV9000_SHORT_STRING_LEN];      //组编号
	char                 strName[EV9000_LONG_STRING_LEN];          //组名称
	unsigned int         nSortID;                                  //同一父节点下组排序编号，默认0不排序
	char                 strParentID[EV9000_SHORT_STRING_LEN];     //父节点编号
	unsigned int         nResved1;                                 //保留1
	char                 strResved2[EV9000_SHORT_STRING_LEN];      //保留2
}EV9000_LogicDeviceGroupConfig, *LPEV9000_LogicDeviceGroupConfig;

//逻辑设备分组关系表
typedef struct EV9000_LogicDeviceMapGroupConfig
{
	unsigned int         nID;                                      //记录编号
	char                 strGroupID[EV9000_SHORT_STRING_LEN];      //点位组编号
	unsigned int         nDeviceIndex;                             //设备ID
	char                 strCMSID[EV9000_IDCODE_LEN];              //CMS 编号
	unsigned int         nSortID;                                  //同一父节点下组排序编号，默认0不排序
	unsigned int         nResved1;                                 //保留1
	char                 strResved2[EV9000_SHORT_STRING_LEN];      //保留2        
}EV9000_LogicDeviceMapGroupConfig, *LPEV9000_LogicDeviceMapGroupConfig;

//报警信息
typedef struct EV9000_AlarmRecord
{
	unsigned int         nID;                                       //记录编号
//	char                 strAlarmID[EV9000_SHORT_STRING_LEN];       //录像编号
	unsigned int         nDeviceIndex;                              //设备ID
	unsigned int         nType;                                     //报警类型
	unsigned int         nLevel;                                    //报警级别
	unsigned int         nStartTime;                                //报警产生时间
	unsigned int         nStopTime;                                 //报警结束时间
	char                 strInfo[EV9000_LONG_STRING_LEN];           //报警描述
	unsigned int         nResved1;                                  //保留1
	char                 strResved2[EV9000_SHORT_STRING_LEN];       //保留2 
}EV9000_AlarmRecord, *LPEV9000_AlarmRecord;

//录像信息
typedef struct EV9000_FileRecord
{
	unsigned int         nID;                                       //记录编号
	unsigned int         nDeviceIndex;                              //逻辑设备序号
	unsigned int         nStorageIndex;                             //存储设备编号
	unsigned int         nStartTime;                                //开始时间
	unsigned int         nStopTime;                                 //结束时间
	unsigned int         nSize;                                     //大小(字节)
	char                 strStorageIP[EV9000_SHORT_STRING_LEN];     //磁阵地址
	char                 strStoragePath[EV9000_LONG_STRING_LEN];    //存储路径
	unsigned int         nType;                                     //录像类型
	unsigned int         nFileOver;                                 //录像结束标记 默认为0
	unsigned int         nResved1;                                  //保留1
	char                 strResved2[EV9000_SHORT_STRING_LEN];       //保留2 
}EV9000_FileRecord, *LPEV9000_FileRecord;

//系统日志记录表
typedef struct EV9000_SystemLogRecord
{
	unsigned int         nID;                                       //记录编号
	unsigned int         nFromType;                                 //日志来源(CMS、TSU、ONVIF代理)
	unsigned int         nDeviceIndex;                              //设备ID
	char                 strDeviceIP[EV9000_SHORT_STRING_LEN];      //设备地址
	unsigned int         nLogType;                                  //日志类型(用户操作、系统运行、报警)
	unsigned int         nLogLevel;                                 //日志级别(一般、警告、错误)
	unsigned int         nLogTime;                                  //产生时间
	char                 strInfo[EV9000_LONG_STRING_LEN];           //日志描述
	unsigned int         nResved1;                                  //保留1
	char                 strResved2[EV9000_SHORT_STRING_LEN];       //保留2 
}EV9000_SystemLogRecord, *LPEV9000_SystemLogRecord;

//轮巡
typedef struct EV9000_PollConfig
{
	unsigned int         nID;                                       //记录编号
	char                 strPollName[EV9000_NORMAL_STRING_LEN];     //轮巡名称
	unsigned int         nScheduledRun;                             //是否定时执行
	unsigned int         nStartTime;                                //开始时间
	unsigned int         nDurationTime;                             //持续时间
	unsigned int         nUserID;                                   //用户ID
	unsigned int         nType;                                     //轮询类型：前台轮询或后台轮询
	unsigned int         nResved1;                                  //保留1 1:轮询开始 0:轮询停止
	char                 strResved2[EV9000_SHORT_STRING_LEN];       //保留2 
}EV9000_PollConfig, *LPEV9000_PollConfig;

//轮巡动作
typedef struct EV9000_PollActionConfig
{
	unsigned int         nID;                                       //记录编号
	unsigned int         nPollID;                                   //轮巡编号
	unsigned int         nType;                                     //动作类型
	char                 strSourceID[EV9000_IDCODE_LEN];            //源设备编号
	char                 strDestID[EV9000_IDCODE_LEN];              //目的设备编号
	unsigned int         nScreenID;                                 //客户端画面编号
	unsigned int         nLiveTime;                                 //停留时间
	unsigned int         nDestSortID;                               //该动作中的目的对象排序号
	unsigned int         nSourceSortID;                             //该动作中的源对象排序号
    unsigned int         nStreamType;                               //码流类型
	unsigned int         nResved1;                                  //保留1
	char                 strResved2[EV9000_SHORT_STRING_LEN];       //保留2 
}EV9000_PollActionConfig, *LPEV9000_PollActionConfig;

//预案
typedef struct EV9000_PlanConfig
{
	unsigned int         nID;                                       //记录编号
	char                 strPlanName[EV9000_NORMAL_STRING_LEN];     //预案名称
	unsigned int         nScheduledRun;                             //是否定时执行
	unsigned int         nStartTime;                                //开始时间
	unsigned int         nUserID;                                   //用户ID
	unsigned int         nType;                                     //预案类型:前台预案或后台预案
	unsigned int         nUserLevel;                                //被动执行用户级别
	unsigned int         nResved1;                                  //保留1
	char                 strResved2[EV9000_SHORT_STRING_LEN];       //保留2 
}EV9000_PlanConfig, *LPEV9000_PlanConfig;

// 预案动作类型
typedef enum
{
	PLANACTION_PC = 0,
		PLANACTION_TVWALL,
		PLANACTION_PRESET,
}PLANACTION_TYPE;

//预案动作
typedef struct EV9000_PlanActionConfig
{
	unsigned int         nID;                                       //记录编号
	unsigned int         nPlanID;                                   //预案编号
	unsigned int         nType;                                     //动作类型
	unsigned int         nDeviceIndex;                              //设备ID
	unsigned int         nDestID;                                   //目的设备编号
	unsigned int         nScreenID;                                 //客户端画面编号
	unsigned int         nControlData;                              //控制值
	unsigned int         nStreamType;                               //码流类型
	unsigned int         nResved1;                                  //保留1
	char                 strResved2[EV9000_SHORT_STRING_LEN];       //保留2 
}EV9000_PlanActionConfig, *LPEV9000_PlanActionConfig;

//巡航
typedef struct EV9000_CruiseConfig
{
	unsigned int         nID;                                       //记录编号
	char                 strCruiseName[EV9000_NORMAL_STRING_LEN];   //巡航名称
	unsigned int         nStartTime;                                //开始时间
	unsigned int         nDurationTime;                             //持续时间
	unsigned int         nDeviceIndex;                              //设备索引
	char                 strDeviceID[EV9000_IDCODE_LEN];            //设备ID
	unsigned int         nResved1;                                  //保留1
	char                 strResved2[EV9000_SHORT_STRING_LEN];       //保留2 
}EV9000_CruiseConfig, *LPEV9000_CruiseConfig;

//巡航动作
typedef struct EV9000_CruiseActionConfig
{
	unsigned int         nID;                                       //记录编号
	unsigned int         nCruiseID;                                 //巡航编号
	int                  nDeviceIndex;                              //设备ID
	unsigned int         nPresetID;                                 //预置位编号
	unsigned int         nLiveTime;                                 //持续时间
	unsigned int         nResved1;                                  //保留1
	char                 strResved2[EV9000_SHORT_STRING_LEN];       //保留2 
}EV9000_CruiseActionConfig, *LPEV9000_CruiseActionConfig;

//预置位
typedef struct EV9000_PresetConfig
{
	unsigned int         nID;                                       //记录编号
	int                  nDeviceIndex;                              //设备ID(系统内部使用,用户不关心)
	unsigned int         nPresetID;                                 //预置位编号
	char                 strPresetName[EV9000_NORMAL_STRING_LEN];   //预置位名称
	unsigned int         nResved1;                                  //保留1
	char                 strResved2[EV9000_SHORT_STRING_LEN];       //保留2 
}EV9000_PresetConfig, *LPEV9000_PresetConfig;

//警报布防表
typedef struct EV9000_AlarmDeployment
{
	char                 strUUID[EV9000_SHORT_STRING_LEN];           
	unsigned int         nLogicDeviceIndex;                         //逻辑设备ID
	unsigned int         nDayOfWeek;                                //星期索引
	unsigned int         nBeginTime;                                //开始时间
	unsigned int         nEndTime;                                  //结束时间
	unsigned int         nResved1;                                  //保留1
	char                 strResved2[EV9000_SHORT_STRING_LEN];       //保留2 
}EV9000_AlarmDeployment, *LPEV9000_AlarmDeployment;

//故障日志表
typedef struct EV9000_FaultRecord
{
	unsigned int          nID;                                      //ID
	unsigned int          nLogicDeviceIndex;                        //逻辑设备ID
	unsigned int          nType;                                    //故障类别
	unsigned int          nLevel;                                   //故障级别
	unsigned int          nBegintime;                               //开始时间
	unsigned int          nEndTime;                                 //结束时间
	char                  strInfo[EV9000_LONG_STRING_LEN];          //详细说明
	unsigned int          nResved1;                                  //保留1
	char                  strResved2[EV9000_SHORT_STRING_LEN];       //保留2 
}EV9000_FaultRecord, *LPEV9000_FaultRecord;
#endif
