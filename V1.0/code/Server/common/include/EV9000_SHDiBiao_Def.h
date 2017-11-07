/******************************************************************************

                  版权所有 (C), 2001-2016, 金智视讯技术有限公司

 ******************************************************************************
  文 件 名   : SHDiBiao.h
  版 本 号   : 初稿
  作    者   : zb
  生成日期   : 2016年3月14日 星期一
  最近修改   :
  功能描述   : 上海地标相关定义
  函数列表   :
  修改历史   :
  1.日    期   : 2016年3月14日 星期一
    作    者   : zb
    修改内容   : 创建文件

******************************************************************************/

/*****************************Agent Message Type******************************/

#define EV9000_SHDB_ALARMLINK_PIC                     1                       //报警联动图像
#define EV9000_SHDB_NURTURING_PIC                     2                       //保养上传图像
#define EV9000_SHDB_DAILY_PIC                         3		     			  //日常上传图像
#define EV9000_SHDB_TEST_PIC						  4						  //测试上传图像
#define EV9000_SHDB_OTHER_SYSTEM_ERROR				  11					  //其他系统故障	
#define EV9000_SHDB_PERIMETER_ALARM_ERROR             12                      //周界报警故障
#define EV9000_SHDB_NETWORK_ALARM_ERROR               13                      //联网报警故障
#define EV9000_SHDB_LOCAL_ALARM_ERROR                 14                      //本地报警故障
#define EV9000_SHDB_VIDEO_ERROR                       15                      //视频监控故障
#define EV9000_SHDB_BUILDING_INTERCOM_ERROR           16                      //楼宇对讲故障
#define EV9000_SHDB_ACCESS_CONTROL_ERROR              17                      //出入控制故障
#define EV9000_SHDB_ELE_PATROL                        18                      //电子巡更故障
#define EV9000_SHDB_DVR_SYSTEM_START                  19                      //DVR系统启动
#define EV9000_SHDB_DVR_SYSTEM_STOP                   20                      //DVR系统退出
#define EV9000_SHDB_DVR_ABNORMAL_STOP                 21                      //DVR异常退出
#define EV9000_SHDB_DVR_PARAM_CONFIG                  22                      //DVR参数设置
#define EV9000_SHDB_DVR_PARAM_COMMIT                  23                      //DVR参数保存
#define EV9000_SHDB_DVR_VIDEO_LOSS                    24                      //DVR视频丢失
#define EV9000_SHDB_DVR_MOTION_DETECTION              25                      //DVR移动侦测
#define EV9000_SHDB_DVR_EXTERN_TRIGGER                26                      //DVR外部触发
#define EV9000_SHDB_DVR_SYSTEM_ALARM_RESET            27                      //系统报警解除                
#define EV9000_SHDB_DVR_ILLEGAL_STOP                  28                      //DVR非法退出                
#define EV9000_SHDB_SYSTEM_SERVICE_CHECK              29                      //系统维修签到
#define EV9000_SHDB_SYSTEM_MAINTENANCE_CHECK          30                      //系统维保签到
#define EV9000_SHDB_DVR_LOCAL_PLAY_BACK               31                      //DVR本地回放
#define EV9000_SHDB_DVR_REMOTE_PLAY_BACK              32                      //DVR远程回放
#define EV9000_SHDB_ACCEPTANCE_PIC                    33                      //验收上传图像
#define EV9000_SHDB_ALARM_PIC                         36                      //报警上传图像(未录像或移动侦测)
#define EV9000_SHDB_NURTURING_PIC_EX                  37                      //保养上传图像(未录像或移动侦测)
#define EV9000_SHDB_DAILY_PIC_EX                      38                      //日常上传图像(当日未录像)                                          
#define EV9000_SHDB_TEST_PIC_EX                       39                      //测试上传图像(未录像或移动侦测)                     
#define EV9000_SHDB_ACCEPTANCE_PIC_EX                 40                      //验收上传图像(未录像或移动侦测)
#define EV9000_SHDB_DVR_DISK_ERROR                    41                      //DVR磁盘错误
#define EV9000_SHDB_SYSTEM_KEEPLIVE_TIMEOUT           42                      //系统心跳超时
#define EV9000_SHDB_SYSTEM_KEEPLIVE_RECOVER           43                      //系统心跳恢复
#define EV9000_SHDB_OTHER_VIDEO_EVENT                 44                      //系统其它事件

/**********************验收图像上传**************************/
/* 
CPacket pack;
pack.SetRootTag("Notify");
pack.CreateElement("CmdType", "SHDB");
pack.CreateElement("SubType", "33");
pack.CreateElement("DeviceID", "320XXXXXXXXXXX");
pack.CreateElement("Format", "CIF");
pack.CreateElement("PIC", "");
pack.CreateElement("Port", "");
pack.CreateElement("UserID", "");
*/

/**********************维护保养**************************/
/* 

  1，维修签到
CPacket pack;
pack.SetRootTag("Notify");
pack.CreateElement("CmdType", "SHDB");
pack.CreateElement("SubType", "29");

  2，保养签到
 CPacket pack;
 pack.SetRootTag("Notify");
 pack.CreateElement("CmdType", "SHDB");
 pack.CreateElement("SubType", "30");
 pack.CreateElement("Info", ".....");

*/

/*********************故障报修**************************/
/*

//视频监控
CPacket pack;
pack.SetRootTag("Notify");
pack.CreateElement("CmdType", "SHDB");
pack.CreateElement("SubType", "15");
pack.CreateElement("Info", ".....")

 //
 CPacket pack;
 pack.SetRootTag("Notify");
 pack.CreateElement("CmdType", "SHDB");
 pack.CreateElement("SubType", "16");
 pack.CreateElement("Info", ".....");

 */

