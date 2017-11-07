/******************************************************************************

                  版权所有 (C), 2001-2013, 金智视讯技术有限公司

 ******************************************************************************
  文 件 名   : EV9000_DBDef.h
  版 本 号   : 初稿
  作    者   : yanghaifeng
  生成日期   : 2013年7月2日 星期二
  最近修改   :
  功能描述   : EV9000系统公共错误码定义
  函数列表   :
  修改历史   :
  1.日    期   : 2013年7月2日 星期二
    作    者   : yanghaifeng
    修改内容   : 创建文件

******************************************************************************/
#ifndef EV9000_ERR_CODE_DEF_H
#define EV9000_ERR_CODE_DEF_H

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
#define EV9000_SUCCESS                    0

/* 0X F100 1XXX 硬件、网络、版本错误类型 */
/* 0x F100 FXXX   保留给BSP */

/* 0X F2XX XXXX	CMS */
#define ERR_GBLOGIC_DEVICE_ERROR           0XF2000001 /* 逻辑设备ID 错误 */
#define ERR_GET_IDLE_TSU_INDEX_ERROR       0XF2000002 /* 获取可用的TSU 索引失败 */
#define ERR_GET_TSU_INFO_ERROR             0XF2000003 /* 获取可用的TSU 信息失败 */
#define ERR_RECORD_TIME_INFO_ERROR         0XF2000004 /* 录像时间信息错误*/
#define ERR_TSU_RETURN_ERROR               0XF2000005  /* TSU 返回失败 */


/* 0X F3XX XXXX	应用端 */
#define EV9000_APPS_ERROR_NOMEMORY                       0xF3010000          // 内存不足
#define EV9000_APPS_ERROR_CREATETHREAD_FAILD             0xF3010001          // 创建线程失败
#define EV9000_APPS_ERROR_PARAM_INVALID                  0xF3010002          // 参数错误

//应用模块错误定义
#define EV9000APP_EROR_NOCTRLRESOURCE                    0xF3020000          // 没有服务资源
#define EV9000APP_EROR_NOFINDRESOURCE                    0xF3020001          // 没有查询资源
#define EV9000APP_EROR_NOPLAYRESOURCE                    0xF3020002          // 没有播放资源
#define EV9000APP_EROR_GETUSERINFOFAILD                  0xF3020003          // 获取用户信息失败,用户名密码不正确

//应用模块播放部分错误定义
#define EV9000APP_EROR_OPENPORT_FAILD                    0xF3021000          // 打开视频接收端口失败
#define EV9000APP_EROR_DATABUFFERFULL                    0xF3021001          // 缓存满
#define EV9000APP_EROR_OPENFILE_FAILD                    0xF3021002          // 打开文件失败
#define EV9000APP_EROR_NOSUPPORT                         0xF3021003          // 不支持
#define EV9000APP_EROR_NODATA                            0xF3021004          // 没有数据
#define EV9000APP_EROR_OPENAD                            0xF3021005          // 打开音频设备失败 
#define EV9000APP_EROR_LOADENC                           0xF3021006          // 加载解码器失败
//系统信息通信模块错误定义
#define EV9000SystemInfo_ERROR                           0xF3040000          // SystemInfo模块错误代码定义
#define EV9000SystemInfo_ERROR_OPENCLIENT_FAILED         0xF3040001          // 连接服务端失败
#define EV9000SystemInfo_ERROR_SENDDATA_FAILED           0xF3040002          // 发送数据失败
#define EV9000SystemInfo_ERROR_PARSEXML_FAILED           0xF3040003          // 解析XML失败
#define EV9000SystemInfo_ERROR_RESPONSE                  0xF3040004          // 设备回应错误信息

//编码模块错误定义
#define EV9000ENC_EROR_NORESOURCE                        0xF3050000          // 没有编码资源
#define EV9000ENC_EROR_NOENCMODE                         0xF3050001          // 没有找到编码模块
#define EV9000ENC_EROR_CREATECONTEX_FAILD                0xF3050002          // 创建编码器上下文失败
#define EV9000ENC_EROR_OPENENCMODEL_FAILD                0xF3050003          // 打开编码模块失败
#define EV9000ENC_EROR_CREATEAVFRAME_FAILD               0xF3050004          // 创建编码缓存失败
#define EV9000ENC_EROR_ENCMODE_INVALID                   0xF3050005          // 编码对象不存在

//解码模块错误定义
#define EV9000DEC_EROR_NORESOURCE                        0xF3060000          // 没有解码资源
#define EV9000DEC_EROR_NODECRESOURCE                     0xF3060001          // 没有解码模块资源
#define EV9000DEC_EROR_LOADDECMODE                       0xF3060002          // 加载解码模块失败
#define EV9000DEC_EROR_LOADFUN_FAILD                     0xF3060003          // 解码模块函数加载不全
#define EV9000DEC_EROR_NODECMODE                         0xF3060004          // 未加载解码模块
#define EV9000DEC_EROR_NODECHANDLE                       0xF3060005          // 未打开解码通道
#define EV9000DEC_EROR_NOAVDECMODE                       0xF3060006          // 解码模块没有找到解码器
#define EV9000DEC_EROR_CREATEPAS_FAILD                   0xF3060007          // 创建解码解析器失败
#define EV9000DEC_EROR_CREATECTX_FAILD                   0xF3060008          // 创建解码上下文失败
#define EV9000DEC_EROR_CREATEAVFRAME                     0xF3060009          // 创建解码缓存帧失败
#define EV9000DEC_EROR_OPENDEC_FAILD                     0xF306000A          // 打开解码器失败
#define EV9000DEC_EROR_BUFFERFULL                        0xF306000B          // 解码缓存满
#define EV9000DEC_EROR_CREATECALLBACK_FAILD              0xF306000C          // 创建解码回调失败


//播放模块错误定义
#define EV9000PLAY_EROR_NORESOURCE                       0xF3070000          // 播放模块没有资源
#define EV9000PLAY_EROR_CREATESUR_FAILD                  0xF3070001          // 创建表面失败,可能是显卡不支持
#define EV9000PLAY_EROR_LOCKSUR_FAILD                    0xF3070002          // 锁定播放表面失败
#define EV9000PLAY_EROR_OPENWAVE_FAILD                   0xF3070003          // 打开音频播放失败

//SIP通信模块错误定义
#define EV9000SIPMODE_EROR_NORESOURCE                    0xF3080000          // 没有SIP通信资源
#define EV9000SIPMODE_EROR_STARTLOCAL_FAILD              0xF3080001          // 启动本地SIP通信失败
#define EV9000SIPMODE_EROR_REGISTER_TIMEOUT              0xF3080002          // 注册超时
#define EV9000SIPMODE_EROR_NOMESSAGE                     0xF3080003          // 没有找到SN对应的MESSAGE体
#define EV9000SIPMODE_EROR_MESSAGE_TIMEOUT               0xF3080004          // 等待MESSAGE超时
#define EV9000SIPMODE_EROR_NOINVITE                      0xF3080005          // 没有找到dialog_index对应的INVITE体
#define EV9000SIPMODE_EROR_INVITE_TIMEOUT                0xF3080006          // 等待INVITE超时
#define EV9000SIPMODE_EROR_NOINVITERESOURCE              0xF3080007          // 没有INVITE资源
#define EV9000SIPMODE_ERROR_INVITE_480                   0xF3080008          // invite 480 错误 点位不在线
#define EV9000SIPMODE_ERROR_INVITE_503                   0xF3080009          // invite 503 错误 服务器错误
#define EV9000SIPMODE_ERROR_INVITE_404                   0xF308000A          // invite 404 错误 未找到点位
#define EV9000SIPMODE_ERROR_INVITE_500                   0xF308000B          // invite 500 错误 服务器错误
#define EV9000SIPMODE_ERROR_INVITE_ELSE                  0xF308000C          // invite 其他错误
#define EV9000SIPMODE_ERROR_GETSDP_FAILD                 0xF308000D          // 获取对端SDP失败
#define EV9000SIPMODE_ERROR_SENDINFO_FAILD               0xF308000E          // 发送info 消息失败
#define EV9000SIPMODE_ERROR_INVITEACCEPT_FAILD           0xF308000F          // 回复Invite失败
#define EV9000SIPMODE_ERROR_REFRESH_FAILD                0xF3080010          // 刷新注册失败
#define EV9000SIPMODE_ERROR_KEEPALIVE_FAILD              0xF3080011          // 保活失败

//PS模块错误定义
#define EV9000PS_EROR_NORESOURCE                         0xF3090000          // 没有PS解析资源

//设备模块错误定义
#define EV9000DEVICE_EROR_NORESOURCE                     0xF30A0000          // 没有设备资源
#define EV9000DEVICE_EROR_NODEVICERESOURCE               0xF30A0001          // 没有设备模块资源
#define EV9000DEVICE_EROR_LOADDEVICEMODE                 0xF30A0002          // 加载设备模块失败
#define EV9000DEVICE_EROR_LOADFUN_FAILD                  0xF30A0003          // 设备模块函数加载不全
#define EV9000DEVICE_EROR_NODEVICEMODE                   0xF30A0004          // 未加载设备模块
#define EV9000DEVICE_EROR_NODEVICEHANDLE                 0xF30A0005          // 未打开设备通道
#define EV9000DEVICE_EROR_CONNECTFAILD                   0xF30A0006          // 连接设备失败
#define EV9000DEVICE_EROR_NOPLAYRESOURCE                 0xF30A0007          // 没有播放资源
#define EV9000DEVICE_EROR_OPENREALPLAYFAILD              0xF30A0008          // 打开实时视频失败

//协议层错误定义
#define EV9000PROTOCOL_EROR_NORESOURCE                   0xF30B0000          // 没有协议资源

/* 0X F4XX XXXX	网管客户端 */


/* 0X F5XX XXXX	TSU */

#define TSU_ERROR_DB_DISCONNECT                          0xF5000001          //  数据库未连接
#define TSU_ERROR_FINDRECORD_0                           0xF5000002          //  查到0条录像记录
#define TSU_ERROR_DB_EXCEPTION                           0xF5000003          //  数据库操作异常
#define TSU_ERROR_ICE_EXCEPTION                          0xF5000004          //  ICE异常



/* 0X F6XX XXXX	媒体网关 */


/* 0X F7XX XXXX	DC解码器 */


/* 0X F8XX XXXX	数据库 */
#ifndef DB_ERROR_DEF
#define DB_ERROR_DEF
#define DB_ERR_OK                       0
#define DB_ERR_BASE                     0XF8000000     //基本值

#define DB_ERR_ICE                      0XF8010000     //ICE错误
#define DB_ERR_DATABUS                  0XF8020000     //databus返回错误
#define DB_ERR_BDB                      0XF8040000     //BDB错误
#define DB_ERR_OTHER                    0XF8080000     //其它错误
#define DB_ERR_INVALUDHANDLE            0XF8100000     //数据库句柄无效

/* 0X F9XX XXXX	SIP协议栈 */
#define	EV9000_SIPSTACK_PARAM_ERROR                          0XF9010001  //协议栈参数错误
#define	EV9000_SIPSTACK_NEW_CALLID_ERROR                     0XF9010002  //协议栈生成新的ID错误
#define	EV9000_SIPSTACK_GET_SOCKET_ERROR                     0XF9010003  //协议栈发送消息获取Socket错误
#define	EV9000_SIPSTACK_TRANSACTION_INIT_ERROR               0XF9010003  //协议栈发送消息初始化事物错误
#define	EV9000_SIPSTACK_SEND_MESSAGE_ERROR                   0XF9010004  //协议栈发送消息错误

#define	EV9000_SIPSTACK_UA_TIMER_INIT_ERROR                  0XF9020001  //协议栈UA定时器初始化失败
#define	EV9000_SIPSTACK_SIP_TIMER_INIT_ERROR                 0XF9020002  //协议栈SIP定时器初始化失败
#define	EV9000_SIPSTACK_UAC_TIMER_INIT_ERROR                 0XF9020103  //协议栈UAC定时器初始化失败
#define	EV9000_SIPSTACK_UAS_TIMER_INIT_ERROR                 0XF9020104  //协议栈UAS定时器初始化失败
#define	EV9000_SIPSTACK_UA_INIT_ERROR                        0XF9020105  //协议栈UA初始化失败
#define	EV9000_SIPSTACK_SIP_MESSAGE_INIT_ERROR               0XF9020106  //协议栈SIP MESSAGE初始化失败
#define	EV9000_SIPSTACK_CALL_BACK_INIT_ERROR                 0XF9020107  //协议栈Call Back初始化失败
#define	EV9000_SIPSTACK_SIPSTACK_INIT_ERROR                  0XF9020108  //协议栈sip stack初始化失败
#define	EV9000_SIPSTACK_RUN_THREAD_INIT_ERROR                0XF9020109  //协议栈线程初始化失败
#define	EV9000_SIPSTACK_UDP_LIST_INIT_ERROR                  0XF9020110  //协议栈udp接收初始化失败

#define	EV9000_SIPSTACK_REGISTER_GET_UAC_ERROR               0XF9030101  //协议栈发送注册消息获取UAC错误
#define	EV9000_SIPSTACK_REGISTER_GENERA_ERROR                0XF9030102  //协议栈发送注册消息生成注册消息失败

#define	EV9000_SIPSTACK_INVITE_GET_UA_ERROR                  0XF9040101  //协议栈发送Invite消息获取UA错误
#define	EV9000_SIPSTACK_INVITE_GET_SDP_INFO_ERROR            0XF9040102  //协议栈发送Invite消息获取SDP错误
#define	EV9000_SIPSTACK_INVITE_GENERA_ERROR                  0XF9040103  //协议栈发送Invite消息生成Invite消息失败

#define	EV9000_SIPSTACK_MESSAGE_GENERA_ERROR                 0XF9050101  //协议栈发送Message消息生成Message消息失败

//ICE
#define DB_ERR_ICE_EXCEPTION           (DB_ERR_ICE | 1)

//DATABUS
#define DB_ERR_DATABUS_DBNOTEXIST      (DB_ERR_DATABUS | 1)
#define DB_ERR_DATABUS_FIELDNOTEXIST   (DB_ERR_DATABUS | 2)
#define DB_ERR_DATABUS_NOTGETREADY     (DB_ERR_DATABUS | 4)
#define DB_ERR_DATABUS_SQLEXCEPTION    (DB_ERR_DATABUS | 8)

//BDB


//OTHER
#define DB_ERR_OTHER_UNCONNECT         (DB_ERR_OTHER | 1)
#define DB_ERR_OTHER_STRISNULL         (DB_ERR_OTHER | 2)     //字符串为空
#define DB_ERR_OTHER_OVERMAXROW        (DB_ERR_OTHER | 3)    //行号超出范围
#define DB_ERR_OTHER_MOVENEXTFAIL      (DB_ERR_OTHER | 4)   //movenext 失败
#define DB_ERR_OTHER_GETVALUEFAIL      (DB_ERR_OTHER | 5)    //取值失败
#define DB_ERR_OTHER_FIELDNUMERROR     (DB_ERR_OTHER | 6)
#define DB_ERR_OTHER_UNKNOWN           (DB_ERR_OTHER | 7) 


#define DB_PING_ERROR_RECON            -10000

#endif

#endif
