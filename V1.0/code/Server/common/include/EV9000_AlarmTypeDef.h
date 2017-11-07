/******************************************************************************

                  版权所有 (C), 2001-2013, 金智视讯技术有限公司

 ******************************************************************************
  文 件 名   : EV9000_AlarmTypeDef.h
  版 本 号   : 初稿
  作    者   : zb
  生成日期   : 2014年9月26日 星期五
  最近修改   :
  功能描述   : EV9000系统报警
  函数列表   :
  修改历史   :
  1.日    期   : 2013年9月26日 星期五
    作    者   : zb
    修改内容   : 创建文件

******************************************************************************/
#ifndef EV9000_ALARM_TYPE_DEF_H
#define EV9000_ALARM_TYPE_DEF_H

/*----------------------------------------------*
 * 包含头文件                                   *
 *----------------------------------------------*/

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

/* 0x 01XX XXXX CMS */
#define	EV9000_CMS_SYSTEM_RUN_INFO                 0x01100000  //系统运行信息
#define	EV9000_CMS_SIPSTACK_RUN_INFO               0x01200000  //SIP协议栈运行信息
#define	EV9000_CMS_USER_OPERATION_INFO             0x01300000  //用户操作记录

/* 系统 */
#define	EV9000_CMS_ICESTARTABNORMALITIES           0x01100001  //ICE服务启动异常
#define	EV9000_CMS_BOARTINITFAIL                   0x01100002  //单板初始化失败
#define	EV9000_CMS_STITCHDECETIONFAIL              0x01100003  //主备倒换检测失败
#define	EV9000_CMS_BUSINESSTHREADINITFAIL          0x01100004  //业务线程初始化失败
#define	EV9000_CMS_BUSINESSDATAINITFAIL            0x01100005  //业务数据初始化失败
#define	EV9000_CMS_MEMORYALLOCFAIL                 0x01100006  //内存分配失败
#define	EV9000_CMS_RESOURCEAPPLICATIONFAIL         0x01100007  //资源申请失败
#define	EV9000_CMS_SEARCHBOARDFAIL                 0x01100008  //单板搜索失败
#define	EV9000_CMS_SETINGLOCALIPFAIL               0x01100009  //CMS设置本地IP地址失败
#define	EV9000_CMS_GETLOCALIPFAIL                  0x01100010  //CMS获取本地IP地址失败
#define	EV9000_CMS_DBCONNECTFAIL                   0x01100011  //数据库连接失败
#define	EV9000_CMS_DBQUERYFAIL                     0x01100012  //数据库查询失败
#define	EV9000_CMS_DBINSERTFAIL                    0x01100013  //数据库添加失败
#define	EV9000_CMS_DBMODIFYFAIL                    0x01100014  //数据库修改失败
#define	EV9000_CMS_DBDELFAIL                       0x01100015  //数据库删除失败
#define	EV9000_CMS_CPUUSAGEHIGH                    0x01100016  //CPU负荷过高
#define	EV9000_CMS_RAMUSAGEHIGH                    0x01100017  //内存负荷过高
#define	EV9000_CMS_FANTEMPERATUREEHIGH             0x01100018  //风扇温度过高
                                                               
/* 协议栈 */                                                   
#define	EV9000_CMS_PROTOCALINITFAIL                0x01200001  //SIP协议栈初始化失败
#define	EV9000_CMS_PROTOCALLISTENFAIL              0x01200002  //SIP协议栈监听消息端口失败
#define	EV9000_CMS_PROTOCALRECVMESSAGEFAIL         0x01200003  //SIP协议栈接收消息解析失败
#define	EV9000_CMS_SENDSIPMESSAGEFAIL              0x01200004  //SIP发送SIP消息失败
#define	EV9000_CMS_BUILDLOCALSDPFAIL               0x01200005  //SIP构建本地SDP消息失败
#define	EV9000_CMS_ANALYSELOCALSDPFAIL             0x01200006  //SIP解析本地SDP消息失败

/* TSU */
#define	EV9000_CMS_TSU_ICE_ERROR                   0x01300001  //TSU ICE接口异常
#define	EV9000_CMS_TSU_REGISTER_ERROR              0x01300002  //TSU注册失败
#define	EV9000_CMS_TSU_OFFLINE                     0x01300003  //TSU掉线
#define	EV9000_CMS_TSU_NOTIFY_NOSTREAM             0x01300004  //TSU通知前端设备没有码流
#define	EV9000_CMS_TSU_NOTIFY_PLAYCLOSE            0x01300005  //TSU通知播放结束

/* 业务 */
#define	EV9000_CMS_DEVICE_REG_FAILED               0x01400001  //设备注册失败
#define	EV9000_CMS_DEVICE_OFFLINE                  0x01400002  //设备掉线
#define	EV9000_CMS_NETWORK_UNREACHED               0x01400003  //网络不可达
#define	EV9000_CMS_USER_REG_FAILED                 0x01400004  //用户登录失败
#define	EV9000_CMS_GET_SERVERID_ERROR              0x01400005  //获取服务器ID失败
#define	EV9000_CMS_GET_DBIP_ERROR                  0x01400006  //获取数据库IP地址失败
#define	EV9000_CMS_START_RECORD_ERROR              0x01400007  //录像业务失败
#define	EV9000_CMS_CONNECT_TV_ERROR                0x01400008  //电视墙业务失败
#define	EV9000_CMS_VIDEO_REQUEST_ERROR             0x01400009  //实时视频业务失败
#define	EV9000_CMS_DEVICE_CONTROL_ERROR            0x01400010  //前端设备控制失败
#define	EV9000_CMS_SET_DEVICE_CONFIG_ERROR         0x01400011  //前端设备配置失败
#define	EV9000_CMS_SET_VIDEO_PARAM_ERROR           0x01400012  //前端图像参数设置失败
#define	EV9000_CMS_REQUEST_IFRAM_ERROR             0x01400013  //请求I帧失败
#define	EV9000_CMS_CONTROL_AUTOZOMMIN_ERROR        0x01400014  //前端点击放大失败
#define	EV9000_CMS_SET_XY_PARAM_ERROR              0x01400015  //设置前端经纬度失败
#define	EV9000_CMS_GET_VIDEO_PARAM_ERROR           0x01400016  //获取前端图像参数失败
#define	EV9000_CMS_GET_PRESET_ERROR                0x01400017  //获取前端预置位失败
#define	EV9000_CMS_GET_CATALOG_ERROR               0x01400018  //获取逻辑设备目录信息失败
#define	EV9000_CMS_NOTIFY_CATALOG_ERROR            0x01400019  //通知逻辑设备目录信息失败
#define	EV9000_CMS_GET_DEVICE_INFO_ERROR           0x01400020  //获取设备信息失败
#define	EV9000_CMS_GET_DEVICE_STATUS_ERROR         0x01400021  //获取设备状态失败
#define	EV9000_CMS_GET_DEVICE_CONFIG_ERROR         0x01400022  //获取前端配置失败
#define	EV9000_CMS_QUERY_RECORD_INFO_ERROR         0x01400023  //录像查询失败
#define	EV9000_CMS_GET_USER_INFO_ERROR             0x01400024  //获取用户信息失败
#define	EV9000_CMS_GET_DEIVCE_GROUP_ERROR          0x01400025  //获取逻辑设备分组信息失败
#define	EV9000_CMS_GET_DEIVCE_MAP_GROUP_ERROR      0x01400026  //获取逻辑设备分组关系信息失败
#define	EV9000_CMS_GET_TOP_DEVICE_CONFIG_ERROR     0x01400027  //获取拓扑物理设备配置信息失败
#define	EV9000_CMS_GET_POLL_CONFIG_ERROR           0x01400028  //获取轮巡配置信息失败
#define	EV9000_CMS_GET_PLAN_CONFIG_ERROR           0x01400029  //获取预案配置信息失败
#define	EV9000_CMS_GET_URL_CONFIG_ERROR            0x01400030  //获取WEB URL配置信息失败
#define	EV9000_CMS_QUERY_ALARM_RECORD_ERROR        0x01400031  //获取报警记录信息失败
#define	EV9000_CMS_GET_ONLINE_USER_ERROR           0x01400032  //获取在线用户信息失败
#define	EV9000_CMS_NOTIFY_CONNECT_TV_ERROR         0x01400033  //通知连接电视墙失败
#define	EV9000_CMS_NOTIFY_ALARM_ERROR              0x01400034  //通知报警消息失败
#define	EV9000_CMS_NOTIFY_STATUS_ERROR             0x01400035  //通知状态信息失败
#define	EV9000_CMS_EXECUTE_PLAN_ERROR              0x01400036  //预案执行失败
#define	EV9000_CMS_EXECUTE_POLL_ERROR              0x01400037  //轮训执行失败
#define	EV9000_CMS_SEND_NOTIFY_ERROR               0x01400038  //发送通知失败
#define	EV9000_CMS_QUERY_DEC_STATUS_ERROR          0x01400039  //获取解码器通道状态失败
#define	EV9000_CMS_NOTIFY_DEC_STATUS_ERROR         0x01400040  //通知解码器通道状态失败

//用户操作日志
#define	EV9000_USER_LOG_LOGIN                      0x10100001 //用户登录
#define	EV9000_USER_LOG_VIDEO_PLAY                 0x10100002 //点播视频
#define	EV9000_USER_LOG_DEVICE_CONTROL             0x10100003 //控球
#define	EV9000_USER_LOG_EXECUTE_PLAN               0x10100004 //执行预案
#define	EV9000_USER_LOG_EXECUTE_POLL               0x10100005 //执行轮巡
#define	EV9000_USER_LOG_STOP_POLL                  0x10100006 //停止轮巡
#define	EV9000_USER_LOG_CONNECT_TVWALL             0x10100007 //切换电视墙
#define	EV9000_USER_LOG_MODIFY_PWD                 0x10100008 //修改用户名密码
#define	EV9000_USER_LOG_VIDEO_PLAYBACK             0x10100009 //回放录像
#define	EV9000_USER_LOG_VIDEO_DOWNLOAD             0x10100010 //下载录像
#define	EV9000_USER_LOG_LOCAL_RECORD               0x10100011 //本地录像
#define	EV9000_USER_LOG_LOCAL_CAPTURE              0x10100012 //抓拍
#define	EV9000_USER_LOG_ADD_PRESET                 0x10100013 //增加预置位
#define	EV9000_USER_LOG_EXECUTE_PRESET             0x10100014 //执行预置位
#define	EV9000_USER_LOG_DEL_PRESET                 0x10100015 //删除预置位
#define	EV9000_USER_LOG_SET_AUTOBACK               0x10100016 //设置归位点
#define	EV9000_USER_LOG_DEL_AUTOBACK               0x10100017 //取消归位点
#define	EV9000_USER_LOG_SET_CAMERA_PARAM           0x10100018 //设置相机参数
#define	EV9000_USER_LOG_SET_CRUISE                 0x10100019 //设置巡航
#define	EV9000_USER_LOG_VERSION_DOWNLOAD           0x10100020 //下载版本

/* 0X 02XX XXXX	TSU */
#define TSU_SYS_RUN_LOG 0x02050000 //系统运行日志
#define TSU_MOUNT_DISK_FAILED 0x02050001 //磁阵挂载失败及磁阵丢失
#define TSU_DISK_NO_FREE_SPACE 0x02050002 //磁阵空间不足
#define TSU_DISK_WRITE_FAILED 0x02050003//tsu写盘持续失败

#define TSU_RECV_SENDER_OVER_MAX 0x02060001 //TSU接入点位数达到最大
#define TSU_RECORD_OVER_MAX 0x02060002 //TSU录像数达到最大
#define TSU_REALTRANSFER_OVER_MAX 0x02060003 //TSU实时视频数达到最大

#define TSU_OPEN_RECORD_FAILED  0x02070401 //录像回放打开失败
#define TSU_NO_FIND_RECORD_TSUDISK 0x02070402 //未找到录像文件对应的存储TSU即磁阵

#define TSU_OPEN_REALTIME_TRANSFER_FAILED 0x02070301 //实时视频打开失败
#define TSU_RECV_SENDER_NO_DATA 0x02070201 //前段无码流

#define TSU_WRITE_DISK_FAILED 0x02070103 //写入磁阵失败
#define TSU_WRITE_LOST_DATA 0x02070102 //写入磁阵丢包
// #define	0x02050001	//磁阵挂载失败及磁阵丢失
// #define	0x02050002	//磁阵空间不足
// #define	0x02030001	//TSU链接数据库失败   
// #define	0x02030002	//TSU插入数据库失败
// #define	0x02030003	//TSU修改数据库失败
// #define	0x02030004	//TSU删除数据库失败
// #define	0x02030005	//TSU查询数据库失败
// #define	0x02020001	//TSU设置本地IP地址失败
// #define	0x02020002	//TSU获取本地IP地址失败
// #define	0x02060001	//TSU接入点位数达到最大
// #define	0x02060002	//TSU录像数达到最大
// #define	0x02060003	//TSU实时视频数达到最大
// #define	0x02070101	//接受码流丢包
// #define	0x02070102	//写入磁阵丢包
// #define	0x02070103	//写入磁阵失败
// #define	0x02070104	//本点位转发缓存已满发送丢包
// #define	0x02070201	//前段无码流
// #define	0x02070301	//实时视频打开失败
// #define	0x02070401	//录像回放打开失败
// #define	0x02070402	//未找到录像文件对应的存储TSU即磁阵

/* 0X 04XX XXXX	Windows MGW */
// #define	0x04070001	//数据库连接失败
// #define	0x04070002	//设备连接失败
// #define	0x04070003	//点位无视频信号

/* 0X 05XX XXXX	Onvif 网关 */
// #define	0x05070001	//数据库连接失败
// #define	0x05070002	//设备连接失败

/* 0X 06XX XXXX	Windows解码器 */
// #define	0x06010001	//VGA通道异常
// #define	0x06010002	//解码卡通道异常
// #define	0x06020001	//网络异常
// #define	0x06020002	//丢包率过大
// #define	0x06060001	//内存使用率过高
// #define	0x06060002	//CPU温度过高
// #define	0x06060003	//CPU使用率过高  
// #define	0x06070001	//打开端口失败
// #define	0x06070002	//长时间无码流
// #define	0x06070003	//灌流持续失败

/* 0X 07XX XXXX 嵌入式解码器 */
// #define	0x070101xx	//VGA通道异常   注 xx 为 具体的物理通道ID 如果 为0则不注明具体的通道。
// #define	0x070102xx	//HDMI通道异常  注 xx 为 具体的物理通道ID 如果 为0则不注明具体的通道。
// #define	0x070103xx	//DVI通道异常  注 xx 为 具体的物理通道ID 如果 为0则不注明具体的通道。
// #define	0x070104xx	//BNC通道异常  注 xx 为 具体的物理通道ID 如果 为0则不注明具体的通道。
// #define	0x070105xx	//音频通道异常  注 xx 为 具体的物理通道ID 如果 为0则不注明具体的通道。
// #define	0x070106xx	//串口通道异常  注 xx 为 具体的物理通道ID 如果 为0则不注明具体的通道。
// #define	0x070107xx	//开关量通道异常  注 xx 为 具体的物理通道ID 如果 为0则不注明具体的通道。
// #define	0x070108xx	//声光告警异常  注 xx 为 具体的物理通道ID 如果 为0则不注明具体的通道。
// #define	0x07020100	//网络异常
// #define	0x07020200	//丢包率过大
// #define	0x07020300	//非法访问
// #define	0x07020400	//网线松动
// #define	0x07020500	//网线脱落
// #define	0x07020600	//IP冲突
// #define	0x07030100	//内存使用率过高
// #define	0x07030200	//CPU温度过高
// #define	0x07030300	//CPU使用率过高  
// #define	0x07030400	//主板电压不稳  
// #define	0x07040100	//注册失败
// #define	0x070402xx	//打开端口失败  注 xx 为 具体的解码通道ID 如果 为0则不注明具体的通道。
// #define	0x070403xx	//申请内存资源失败  注 xx 为 具体的解码通道ID 如果 为0则不注明具体的通道。
// #define	0x070404xx	//创建解码通道失败  注 xx 为 具体的解码通道ID 如果 为0则不注明具体的通道。
// #define	0x070405xx	//长时间无码流  注 xx 为 具体的解码通道ID 如果 为0则不注明具体的通道。
// #define	0x070406xx	//解码失败  注 xx 为 具体的解码通道ID 如果 为0则不注明具体的通道。

/* 0X 08XX XXXX 智能行为分析 */
#define	EV9000_BEHAVIORANALYSIS_SYSTEMABNORMALITIES                     0x08010000	//系统异常
#define	EV9000_BEHAVIORANALYSIS_VGAABNORMALITIES                        0x08010001	//VGA通道异常
#define	EV9000_BEHAVIORANALYSIS_HDMIABNORMALITIES                       0x08010002	//HDMI通道异常
#define	EV9000_BEHAVIORANALYSIS_DVIABNORMALITIES                        0x08010003	//DVI通道异常
#define	EV9000_BEHAVIORANALYSIS_BNCABNORMALITIES                        0x08010004	//BNC通道异常
#define	EV9000_BEHAVIORANALYSIS_AUDIOABNORMALITIES                      0x08010005	//音频通道异常
#define	EV9000_BEHAVIORANALYSIS_SERIALABNORMALITIES                     0x08010006	//串口通道异常
#define	EV9000_BEHAVIORANALYSIS_SWITCHABNORMALITIES                     0x08010007	//开关量通道异常
#define	EV9000_BEHAVIORANALYSIS_SLALARMABNORMALITIES                    0x08010008	//声光告警异常
#define	EV9000_BEHAVIORANALYSIS_NETABNORMALITIES                        0x08020000	//网络异常
#define	EV9000_BEHAVIORANALYSIS_UNCONNECTABNORMALITIES                  0x08020001	//未连接网络
#define	EV9000_BEHAVIORANALYSIS_IPCONFLICTABNORMALITIES                 0x08020002	//IP冲突
#define	EV9000_BEHAVIORANALYSIS_NETINSTABILITYABNORMALITIES             0x08020003	//网络不稳定
#define	EV9000_BEHAVIORANALYSIS_DBABNORMALITIES                         0x08030000	//数据库异常
#define	EV9000_BEHAVIORANALYSIS_DBCONNECTABNORMALITIES                  0x08030001	//数据库连接异常
#define	EV9000_BEHAVIORANALYSIS_DBLOGINABNORMALITIES                    0x08030002	//数据库登录异常
#define	EV9000_BEHAVIORANALYSIS_DBWORKABNORMALITIES                     0x08030003	//数据库操作异常
#define	EV9000_BEHAVIORANALYSIS_PERFORMANCEABNORMALITIES                0x08060000	//性能异常
#define	EV9000_BEHAVIORANALYSIS_CPUUSAGEHIGH                            0x08060001	//CPU使用率过高
#define	EV9000_BEHAVIORANALYSIS_RAMUSAGEHIGH                            0x08060002	//内存使用率过高
#define	EV9000_BEHAVIORANALYSIS_CPUTEMPERATUREEHIGH                     0x08060003	//CPU温度过高
#define	EV9000_BEHAVIORANALYSIS_BOARDVOLINSTABILTY                      0x08060004	//主板电压不稳
#define	EV9000_BEHAVIORANALYSIS_BUSINESSABNORMALITIES                   0x08070000	//业务异常
#define	EV9000_BEHAVIORANALYSIS_OVERRALLABNORMALITIES                   0x08070100	//总体流程异常
#define	EV9000_BEHAVIORANALYSIS_PROGRESSCRASHES                         0x08070101	//程序奔溃
#define	EV9000_BEHAVIORANALYSIS_PROGRAMHANGS                            0x08070102	//程序挂死
#define	EV9000_BEHAVIORANALYSIS_SIPABNORMALITIES                        0x08070200	//SIP异常
#define	EV9000_BEHAVIORANALYSIS_PROTOCALINITFAIL                        0x08070201	//协议栈初始化失败
#define	EV9000_BEHAVIORANALYSIS_PROTOCALLISTENFAIL                      0x08070202	//协议栈监听消息端口失败
#define	EV9000_BEHAVIORANALYSIS_PROTOCALRECVMESSAGEFAIL                 0x08070203	//协议栈接收消息解析失败
#define	EV9000_BEHAVIORANALYSIS_SENDSIPMESSAGEFAIL                      0x08070204	//发送SIP消息失败
#define	EV9000_BEHAVIORANALYSIS_BUILDLOCALSDPFAIL                       0x08070205	//构建本地SDP消息失败
#define	EV9000_BEHAVIORANALYSIS_ANALYSELOCALSDPFAIL                     0x08070206	//解析本地SDP消息失败
#define	EV9000_BEHAVIORANALYSIS_STREAMABNORMALITIES                     0x08070300	//码流异常
#define	EV9000_BEHAVIORANALYSIS_GETSTREAMFAIL                           0x08070301	//获取流失败
#define	EV9000_BEHAVIORANALYSIS_PACKLOSSRATEABNORMALITIES               0x08020002	//丢包率异常
#define	EV9000_BEHAVIORANALYSIS_STREAMANALYSISABNORMALITIES             0x08020003	//码流解析异常
#define	EV9000_BEHAVIORANALYSIS_STREAMPACKAGINGABNORMALITIES            0x08020004	//码流打包异常
#define	EV9000_BEHAVIORANALYSIS_CODEORENCODEABNORMALITIES               0x08070400	//编解码异常
#define	EV9000_BEHAVIORANALYSIS_CODEPARAMABNORMALITIES                  0x08070401	//编码参数异常
#define	EV9000_BEHAVIORANALYSIS_ENCODEPARAMABNORMALITIES                0x08070402	//解码参数异常
#define	EV9000_BEHAVIORANALYSIS_CODEDATAABNORMALITIES                   0x08070403	//编码数据错误
#define	EV9000_BEHAVIORANALYSIS_ENCODEDATAABNORMALITIES                 0x08070404	//解码数据错误
#define	EV9000_BEHAVIORANALYSIS_OVERLAYINFORABNORMALITIES               0x08070405	//叠加信息异常
#define	EV9000_BEHAVIORANALYSIS_ANALYSISALGORITHMSABNORMALITIES         0x08070500	//分析算法异常
#define	EV9000_BEHAVIORANALYSIS_ALGORITHINITPARAMSABNORMALITIES         0x08070501	//算法初始化参数异常
#define	EV9000_BEHAVIORANALYSIS_ALGORITHINITSOURCESABNORMALITIES        0x08070502	//算法初始化资源异常
#define	EV9000_BEHAVIORANALYSIS_ALGORITHDATAABNORMALITIES               0x08070503	//算法数据异常
#define	EV9000_BEHAVIORANALYSIS_ANALYSISRESULTABNORMALITIES             0x08070600	//分析结果异常
#define	EV9000_BEHAVIORANALYSIS_UNDIRECTEDTRIPWIRE                      0x08070601	//无向绊线
#define	EV9000_BEHAVIORANALYSIS_DIRECTEDTRIPWIRE                        0x08070602	//有向绊线
#define	EV9000_BEHAVIORANALYSIS_IRRUPT                                  0x08070603	//进入区域（入侵）
#define	EV9000_BEHAVIORANALYSIS_TRANSBOUNDARY                           0x08070604	//离开区域（越界）
#define EV9000_BEHAVIORANALYSIS_ABANDON                                 0x08070605  //物品遗留检测
#define EV9000_BEHAVIORANALYSIS_OBJLOST                                 0x08070606  //物品丢失检测
#define EV9000_BEHAVIORANALYSIS_WANDER                                  0x08070607  //徘徊检测
#define EV9000_BEHAVIORANALYSIS_STOPCAR                                 0x08070608  //非法停车
#define EV9000_BEHAVIORANALYSIS_OUTDOOR                                 0x08070609  //出门检测
#define EV9000_BEHAVIORANALYSIS_LEAVEPOS                                0x0807060a  //离岗检测 

#define	EV9000_BEHAVIORANALYSIS_PRIVATEPROTOCOLANALYSISABNORMALITIES    0x08070700	//私有协议解析异常
#define	EV9000_BEHAVIORANALYSIS_CONFIGMODYNOTIFYPROTOCOLABNORMALITIES   0x08070701	//配置修改通知协议异常
#define	EV9000_BEHAVIORANALYSIS_GETPLATFORMPROTOCOL                     0x08070702	//获取平台信息协议

/* 0X 09XX XXXX 视频质量诊断 */
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_SYSTEMABNORMALITIES                    0x09010000	//系统异常
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_VGAABNORMALITIES                       0x09010001	//VGA通道异常
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_HDMIABNORMALITIES                      0x09010002	//HDMI通道异常
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_DVIABNORMALITIES                       0x09010003	//DVI通道异常
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_BNCABNORMALITIES                       0x09010004	//BNC通道异常
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_AUDIOABNORMALITIES                     0x09010005	//音频通道异常
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_SERIALABNORMALITIES                    0x09010006	//串口通道异常
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_SWITCHABNORMALITIES                    0x09010007	//开关量通道异常
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_SLALARMABNORMALITIES                   0x09010008	//声光告警异常
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_NETABNORMALITIES                       0x09020000	//网络异常
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_UNCONNECTABNORMALITIES                 0x09020001	//未连接网络
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_IPCONFLICTABNORMALITIES                0x09020002	//IP冲突
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_NETINSTABILITY                         0x09020003	//网络不稳定
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_DBABNORMALITIES                        0x09030000	//数据库异常
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_DBCONNECTABNORMALITIES                 0x09030001	//数据库连接异常
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_DBLOGINABNORMALITIES                   0x09030002	//数据库登录异常
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_DBWORKABNORMALITIES                    0x09030003	//数据库操作异常
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_PERFORMANCEABNORMALITIES               0x09060000	//性能异常
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_CPUUSAGEHIGH                           0x09060001	//CPU使用率过高
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_RAMUSAGEHIGH                           0x09060002	//内存使用率过高
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_CPUTEMPERATUREEHIGH                    0x09060003	//CPU温度过高
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_BOARDVOLINSTABILTY                     0x09060004	//主板电压不稳
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_BUSINESSABNORMALITIES                  0x09070000	//业务异常
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_OVERRALLABNORMALITIES                  0x09070100	//总体流程异常
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_PROGRESSCRASHES                        0x09070101	//程序奔溃
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_PROGRAMHANGS                           0x09070102	//程序挂死
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_SIPABNORMALITIES                       0x09070200	//SIP异常
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_PROTOCALINITFAIL                       0x09070201	//协议栈初始化失败
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_PROTOCALLISTENFAIL                     0x09070202	//协议栈监听消息端口失败
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_PROTOCALRECVMESSAGEFAIL                0x09070203	//协议栈接收消息解析失败
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_SENDSIPMESSAGEFAIL                     0x09070204	//发送SIP消息失败
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_BUILDLOCALSDPFAIL                      0x09070205	//构建本地SDP消息失败
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_ANALYSELOCALSDPFAIL                    0x09070206	//解析本地SDP消息失败
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_STREAMABNORMALITIES                    0x09070300	//码流异常
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_GETSTREAMFAIL                          0x09070301	//获取流失败
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_PACKLOSSRATEABNORMALITIES              0x09020002	//丢包率异常
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_STREAMANALYSISABNORMALITIES            0x09020003	//码流解析异常
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_STREAMPACKAGINGABNORMALITIES           0x09020004	//码流打包异常
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_CODEORENCODEABNORMALITIES              0x09070400	//编解码异常
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_CODEPARAMABNORMALITIES                 0x09070401	//编码参数异常
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_ENCODEPARAMABNORMALITIES               0x09070402	//解码参数异常
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_CODEDATAABNORMALITIES                  0x09070403	//编码数据错误
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_ENCODEDATAABNORMALITIES                0x09070404	//解码数据错误
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_OVERLAYINFORABNORMALITIES              0x09070405	//叠加信息异常
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_ANALYSISALGORITHMSABNORMALITIES        0x09070500	//分析算法异常
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_ALGORITHINITPARAMSABNORMALITIES        0x09070501	//算法初始化参数异常
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_ALGORITHINITSOURCESABNORMALITIES       0x09070502	//算法初始化资源异常
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_ALGORITHDATAABNORMALITIES              0x09070503	//算法数据异常
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_ANALYSISRESULTABNORMALITIES            0x09070600	//分析结果异常
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_DEFINITIONABNORMALITIES                0x09070601	//清晰度异常（视频模糊）
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_LIGHTABNORMALITIES                     0x09070602	//亮度异常(曝光）
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_COLORFULABNORMALITIES                  0x09070603	//颜色异常(偏色）
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_SNOWABNORMALITIES                      0x09070604	//雪花异常
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_VIDEOHUNGS                             0x09070605	//画面冻结(视频冻结）
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_SIGNALLOSSABNORMALITIES                0x09070606	//信号丢失异常
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_SCREENTINGLEABNORMALITIES              0x09070607	//画面抖动异常(视频抖动）
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_VIDEOOCCLUSIONABNORMALITIES            0x09070608	//视频遮挡异常
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_NIGHTMODE	                             0x09070609 //图像黑白
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_LUMLOW  		                         0x0907060A //亮度过暗
#define EV9000_VIDEOQUALITYDIAGNOSTIC_CONTRASTLOW	                         0x0907060B	//对比度过暗
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_UPHEAVAL		                         0x0907060C	//视频剧变
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_MOSAIC  		                         0x0907060D	//视频马赛克
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_STRIPE                                 0x0907060E	//视频条纹
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_PTZ    		                         0x0907060F	//PTZ异常
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_SCENECHANGE	                         0x09070610	//场景变更
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_NETWORK 		                         0x09070611	//丢包
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_LOGIN   		                         0x09070612	//申请码流失败
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_NOSTREAM                               0x09070613	//无码流

#define	EV9000_VIDEOQUALITYDIAGNOSTIC_PRIVATEPROTOCOLANALYSISABNORMALITIES   0x09070700	//私有协议解析异常
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_CONFIGMODYNOTIFYPROTOCOLABNORMALITIES  0x09070701	//配置修改通知协议异常
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_GETPLATFORMPROTOCOL                    0x09070702	//获取平台信息协议

/*----------------------------------------------*
 * 告警类型查询说明                             *
 *----------------------------------------------*/

/*查询的告警信息采用unsigned int型来表示，每一位表示一种类型。最大可表示32种
	
右边为第一位 对应关系如下：

1	EV9000_BEHAVIORANALYSIS_ANALYSISRESULTABNORMALITIES 
2	EV9000_BEHAVIORANALYSIS_UNDIRECTEDTRIPWIRE    
3	EV9000_BEHAVIORANALYSIS_DIRECTEDTRIPWIRE         
4	EV9000_BEHAVIORANALYSIS_IRRUPT             
5	EV9000_BEHAVIORANALYSIS_TRANSBOUNDARY    
6	EV9000_VIDEOQUALITYDIAGNOSTIC_ANALYSISRESULTABNORMALITIES   
7	EV9000_VIDEOQUALITYDIAGNOSTIC_DEFINITIONABNORMALITIES         
8	EV9000_VIDEOQUALITYDIAGNOSTIC_LIGHTABNORMALITIES           
9	EV9000_VIDEOQUALITYDIAGNOSTIC_COLORFULABNORMALITIES              
10	EV9000_VIDEOQUALITYDIAGNOSTIC_SNOWABNORMALITIES            
11	EV9000_VIDEOQUALITYDIAGNOSTIC_VIDEOHUNGS                       
12	EV9000_VIDEOQUALITYDIAGNOSTIC_SIGNALLOSSABNORMALITIES        
13	EV9000_VIDEOQUALITYDIAGNOSTIC_SCREENTINGLEABNORMALITIES       
14	EV9000_VIDEOQUALITYDIAGNOSTIC_VIDEOOCCLUSIONABNORMALITIES   

*/

/* 设备故障告警类型 */
#define	EV9000_ALARM_CMS_ERROR                     0x90000001  //管理服务器异常
#define	EV9000_ALARM_TSU_ERROR                     0x90000002  //流媒体服务器异常
#define	EV9000_ALARM_STORAGE_ERROR                 0x90000003  //存储设备异常
#define	EV9000_ALARM_DEC_TSU_ERROR                 0x90000004  //解码器异常
#define	EV9000_ALARM_DIAGNOSE_ERROR                0x90000005  //诊断服务器异常
#define	EV9000_ALARM_INTELLIGENT_ERROR             0x90000006  //智能分析服务器异常
#define	EV9000_ALARM_LOGIC_DEVICE_ERROR            0x90000007  //点位状态异常
#define	EV9000_ALARM_ACCESS_DEVICE_ERROR           0x90000008  //接入设备异常（MGW/DVR/NVR）
#define	EV9000_ALARM_MOTION_DETECTION              0x90000009  //移动侦测报警

#endif
