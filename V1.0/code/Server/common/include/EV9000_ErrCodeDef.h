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
/* 通用 */
#define EV9000_CMS_ERR_LOCAL_CMS_ID_ERROR                                               0XF2000001 /* 本地CMS ID没有配置 */
#define EV9000_CMS_ERR_GET_LOCALIP_ERROR                                                0XF2000002 /* 获取本地IP地址错误 */
#define EV9000_CMS_ERR_GET_LOCALPORT_ERROR                                              0XF2000003 /* 获取本地端口错误 */
#define EV9000_CMS_ERR_NO_DEAL_THREAD_ERROR                                             0XF2000004 /* 找不到对应的处理线程 */
#define EV9000_CMS_ERR_PARAM_ERROR                                                      0XF2000005 /* 参数错误 */
#define EV9000_CMS_ERR_PAY_ERROR                                                        0XF2000006 /* 没有付费 */
#define EV9000_CMS_ERR_SYSTEM_ERROR                                                     0XF2000007 /* 系统错误 */
#define EV9000_CMS_ERR_SYSTEM_LICENSE_ERROR                                             0XF2000008 /* 系统没有License授权 */

/* XML */
#define EV9000_CMS_ERR_XML_BUILD_TREE_ERROR                                             0XF2010001 /* 构建XML 树失败 */
#define EV9000_CMS_ERR_XML_GET_NODE_ERROR                                               0XF2010002 /* 获取XML Node失败 */
#define EV9000_CMS_ERR_XML_GET_MSG_TYPE_ERROR                                           0XF2010003 /* 获取XML 消息类型失败 */

/* SDP */
#define EV9000_CMS_ERR_SDP_MSG_INIT_ERROR                                               0XF2020001 /* SDP初始化失败 */
#define EV9000_CMS_ERR_SDP_MSG_PARSE_ERROR                                              0XF2020002 /* SDP解析失败 */
#define EV9000_CMS_ERR_SDP_GET_VIDEO_INFO_ERROR                                         0XF2020003 /* 获取SDP视频参数失败 */
#define EV9000_CMS_ERR_SDP_NOT_SUPPORT_S_TYPE_ERROR                                     0XF2020004 /* 不支持的S SDP消息类型 */
#define EV9000_CMS_ERR_SDP_MODIFY_S_NAME_ERROR                                          0XF2020005 /* 修改SDP消息S字段名称失败 */
#define EV9000_CMS_ERR_SDP_MODIFY_IP_ERROR                                              0XF2020006 /* 修改SDP消息IP地址字段名称失败 */
#define EV9000_CMS_ERR_SDP_MODIFY_PROTOCOL_ERROR                                        0XF2020007 /* 修改SDP消息协议字段名称失败 */
#define EV9000_CMS_ERR_SDP_GENERAL_MSG_ERROR                                            0XF2020008 /* 生成SDP应答消息失败 */

/* 数据库 */
#define EV9000_CMS_ERR_DB_OPER_ERROR                                                    0XF2030001 /* 数据库查询失败 */
#define EV9000_CMS_ERR_DB_NORECORD_ERROR                                                0XF2030002 /* 数据库没有查询到记录 */
#define EV9000_CMS_ERR_DB_USER_NOTENABLE_ERROR                                          0XF2030003 /* 数据库用户没有启用 */
#define EV9000_CMS_ERR_DB_USERID_EMPTY_ERROR                                            0XF2030004 /* 数据库用户ID为空 */

/* 用户 */
#define EV9000_CMS_ERR_USER_FIND_USER_INFO_ERROR                                        0XF2040001 /* 查找用户失败 */
#define EV9000_CMS_ERR_USER_GET_USER_INFO_ERROR                                         0XF2040002 /* 获取用户失败 */
#define EV9000_CMS_ERR_USER_CREAT_USER_INFO_ERROR                                       0XF2040003 /* 创建用户失败 */
#define EV9000_CMS_ERR_USER_NOT_ENABLE_ERROR                                            0XF2040004 /* 用户未启用 */
#define EV9000_CMS_ERR_USER_OFFLINE_ERROR                                               0XF2040005 /* 用户不在线 */

/* 物理设备 */
#define EV9000_CMS_ERR_DEVICE_FIND_DEVICE_INFO_ERROR                                    0XF2050001 /* 查找物理设备失败 */
#define EV9000_CMS_ERR_DEVICE_GET_DEVICE_INFO_ERROR                                     0XF2050002 /* 获取物理设备失败 */
#define EV9000_CMS_ERR_DEVICE_CREAT_DEVICE_INFO_ERROR                                   0XF2050003 /* 创建物理设备失败 */
#define EV9000_CMS_ERR_DEVICE_NOT_ENABLE_ERROR                                          0XF2050004 /* 物理设备未启用 */
#define EV9000_CMS_ERR_DEVICE_OFFLINE_ERROR                                             0XF2050005 /* 物理设备不在线 */

/* 逻辑设备 */
#define EV9000_CMS_ERR_DEVICE_FIND_LOGIC_DEVICE_INFO_ERROR                              0XF2060001 /* 查找逻辑设备失败 */
#define EV9000_CMS_ERR_DEVICE_GET_LOGIC_DEVICE_INFO_ERROR                               0XF2060002 /* 获取逻辑设备失败 */
#define EV9000_CMS_ERR_DEVICE_CREAT_LOGIC_DEVICE_INFO_ERROR                             0XF2060003 /* 创建逻辑设备失败 */
#define EV9000_CMS_ERR_DEVICE_LOGIC_DEVICE_NOT_ENABLE_ERROR                             0XF2060004 /* 逻辑设备未启用 */
#define EV9000_CMS_ERR_DEVICE_LOGIC_DEVICE_OFFLINE_ERROR                                0XF2060005 /* 逻辑设备不在线 */
#define EV9000_CMS_ERR_DEVICE_LOGIC_DEVICE_UNREACHED_ERROR                              0XF2060006 /* 逻辑设备不可达 */

/* 互连路由 */
#define EV9000_CMS_ERR_ROUTE_FIND_ROUTE_INFO_ERROR                                      0XF2070001 /* 查找路由信息错误 */
#define EV9000_CMS_ERR_ROUTE_GET_ROUTE_INFO_ERROR                                       0XF2070002 /* 获取路由信息错误 */
#define EV9000_CMS_ERR_ROUTE_CREAT_ROUTE_INFO_ERROR                                     0XF2070003 /* 创建路由信息失败 */
#define EV9000_CMS_ERR_ROUTE_NOT_ENABLE_ERROR                                           0XF2070004 /* 路由信息未启用 */

/* TSU */
#define EV9000_CMS_ERR_TSU_GET_IDLE_TSU_INDEX_ERROR                                     0XF2080001 /* 获取TSU 索引失败 */
#define EV9000_CMS_ERR_TSU_GET_TSU_INFO_ERROR                                           0XF2080002 /* 获取TSU 信息失败 */
#define EV9000_CMS_ERR_TSU_GET_IP_ERROR                                                 0XF2080003 /* 获取TSU IP地址失败 */
#define EV9000_CMS_ERR_TSU_GET_SEND_PORT_ERROR                                          0XF2080004 /* 获取TSU发送端口失败 */
#define EV9000_CMS_ERR_TSU_GET_RECV_PORT_ERROR                                          0XF2080005 /* 获取TSU接收端口失败 */
#define EV9000_CMS_ERR_TSU_IP_CLONE_ERROR                                               0XF2080006 /* TSUIP地址拷贝失败 */
#define EV9000_CMS_ERR_TSU_GET_CALLER_TSU_IP_ERROR                                      0XF2080007 /* 获取已有视频业务使用的TSU IP地址失败 */
#define EV9000_CMS_ERR_TSU_NOTIFY_ADD_REPLAY_ERROR                                      0XF2080008 /* 通知TSU添加回放任务失败 */
#define EV9000_CMS_ERR_TSU_NOTIFY_START_REPLAY_ERROR                                    0XF2080009 /* 通知TSU开始发送录像文件码流失败 */
#define EV9000_CMS_ERR_TSU_NOTIFY_ADD_TRANSFER_ERROR                                    0XF208000A /* 通知TSU开始发转发码流失败 */
#define EV9000_CMS_ERR_TSU_ICE_ERROR                                                    0XF208000B /* TSU ICE通信异常 */

/* 注册 */
#define EV9000_CMS_ERR_REG_SERVER_ID_NOT_MATCH_ERROR                                    0XF2090001 /* 注册的CMS ID和本地不匹配 */
#define EV9000_CMS_ERR_REG_SERVER_IP_NOT_MATCH_ERROR                                    0XF2090002 /* 注册的IP地址和本地不匹配 */
#define EV9000_CMS_ERR_REG_DEVICE_TYPE_NOT_SUPPORT_ERROR                                0XF2090003 /* 注册的设备类型不支持 */
#define EV9000_CMS_ERR_REG_AUTH_REALM_NOT_LOCAL_ERROR                                   0XF2090004 /* 认证域名非本级CMS */
#define EV9000_CMS_ERR_REG_AUTH_FAILD_ERROR                                             0XF2090005 /* 认证失败 */
#define EV9000_CMS_ERR_REG_GET_SERVER_IP_ERROR                                          0XF2090006 /* 获取注册的服务器IP地址失败 */
#define EV9000_CMS_ERR_REG_GET_SERVER_PORT_ERROR                                        0XF2090007 /* 获取注册的服务器端口地址失败 */
#define EV9000_CMS_ERR_REG_GET_SERVER_IP_ETHNAME_ERROR                                  0XF2090008 /* 获取注册的服务器IP地址的网口名称失败 */
#define EV9000_CMS_ERR_REG_MSG_ERROR                                                    0XF2090009 /* 注册消息错误 */
#define EV9000_CMS_ERR_REG_EXPIRE_ERROR                                                 0XF209000A /* 注册消息超时时间错误 */
#define EV9000_CMS_ERR_REG_CALLID_ERROR                                                 0XF209000B /* 注册消息CallID字段错误 */
#define EV9000_CMS_ERR_REG_ID_NOT_MATCH_ERROR                                           0XF209000C /* 注册的设备ID和本地CMS ID不匹配 */

/* 用户注册 */
#define EV9000_CMS_ERR_USER_REG_ASSIGN_THREAD_ERROR                                     0XF209000B /* 分配用户业务处理线程失败 */

/* 设备注册 */
#define EV9000_CMS_ERR_DEVICE_REG_IP_CONFLICT_ERROR                                     0XF209000C /* 设备IP地址冲突 */

/* 视频 */
#define EV9000_CMS_ERR_INVITE_GET_IDLE_CR_DATA_ERROR                                    0XF20A0001 /* 获取空闲的呼叫资源失败 */
#define EV9000_CMS_ERR_INVITE_GET_CR_DATA_ERROR                                         0XF20A0002 /* 获取呼叫资源失败 */

#define EV9000_CMS_ERR_INVITE_CALLER_IP_ERROR                                           0XF20A0003 /* 获取视频请求方IP地址错误 */
#define EV9000_CMS_ERR_INVITE_CALLER_PORT_ERROR                                         0XF20A0004 /* 获取视频请求方端口错误 */

#define EV9000_CMS_ERR_INVITE_CALLER_USER_INFO_ERROR                                    0XF20A0005 /* 获取视频请求方用户信息错误 */
#define EV9000_CMS_ERR_INVITE_CALLER_DEVICE_INFO_ERROR                                  0XF20A0006 /* 获取视频请求方物理设备信息错误 */
#define EV9000_CMS_ERR_INVITE_CALLER_ROUTE_INFO_ERROR                                   0XF20A0007 /* 获取视频请求方上级路由信息错误 */

#define EV9000_CMS_ERR_INVITE_CALLER_MSG_BODY_ERROR                                     0XF20A0008 /* 获取视频请求方的Invite消息体错误 */
#define EV9000_CMS_ERR_INVITE_CALLER_SDP_MSG_ERROR                                      0XF20A0009 /* 获取视频请求方的SDP消息错误 */
#define EV9000_CMS_ERR_INVITE_CALLER_SDP_PARAM_ERROR                                    0XF20A000A /* 获取视频请求方的SDP参数信息错误 */

#define EV9000_CMS_ERR_INVITE_CALLEE_IP_ERROR                                           0XF20A000B /* 获取视频请求点位的IP地址错误 */
#define EV9000_CMS_ERR_INVITE_CALLEE_PORT_ERROR                                         0XF20A000C /* 获取视频请求点位端口错误 */

#define EV9000_CMS_ERR_INVITE_CALLEE_DEVICE_INFO_ERROR                                  0XF20A000D /* 获取视频请求点位物理设备信息错误 */
#define EV9000_CMS_ERR_INVITE_CALLEE_ROUTE_INFO_ERROR                                   0XF20A000E /* 获取视频请求点位上级路由信息错误 */

#define EV9000_CMS_ERR_INVITE_CALLEE_MSG_BODY_ERROR                                     0XF20A000F /* 获取视频请求点位的Invite消息体错误 */
#define EV9000_CMS_ERR_INVITE_CALLEE_SDP_MSG_ERROR                                      0XF20A0010 /* 获取视频请求点位的SDP消息错误 */
#define EV9000_CMS_ERR_INVITE_CALLEE_SDP_PARAM_ERROR                                    0XF20A0011 /* 获取视频请求点位的SDP参数信息错误 */


#define EV9000_CMS_ERR_INVITE_CALLEE_LOGIC_DEVICE_INFO_ERROR                            0XF20A0012 /* 获取视频请求点位设备信息错误 */
#define EV9000_CMS_ERR_INVITE_CALLEE_NOT_SUPPORT_MULTI_STREAM_ERROR                     0XF20A0013 /* 视频请求点位设备不支持多码流 */
#define EV9000_CMS_ERR_INVITE_CALLEE_RECORD_NOT_START_ERROR                             0XF20A0014 /* 视频请求点位的录像业务没有启动 */
#define EV9000_CMS_ERR_INVITE_CALLEE_RECORD_NOT_COMPLETE_ERROR                          0XF20A0015 /* 视频请求点位的录像业务没有结束 */
#define EV9000_CMS_ERR_INVITE_CALLEE_VIDEO_NOT_COMPLETE_ERROR                           0XF20A0016 /* 视频请求点位已有的视频业务没有结束 */

#define EV9000_CMS_ERR_INVITE_GET_CALLEE_CR_DATA_ERROR                                  0XF20A0017 /* 获取请求点位已有的呼叫资源失败 */

#define EV9000_CMS_ERR_INVITE_FIND_CALLEE_RECORD_INFO_ERROR                             0XF20A0018 /* 查找视频请求点位已有的录像信息失败 */
#define EV9000_CMS_ERR_INVITE_GET_CALLEE_RECORD_INFO_ERROR                              0XF20A0019 /* 获取视频请求点位已有的录像信息失败 */

#define EV9000_CMS_ERR_INVITE_TRANSFER_MSG_TO_DEST_ERROR                                0XF20A001A /* 转发Invite消息到目的地失败 */
#define EV9000_CMS_ERR_INVITE_ACCEPT_ERROR                                              0XF20A001B /* 接收发送请求侧的Invite消息失败 */
#define EV9000_CMS_ERR_INVITE_FRONT_RETURN_ERROR                                        0XF20A001C /* 前端Invite回应错误响应消息 */
#define EV9000_CMS_ERR_INVITE_GET_VIDEO_STREAM_ERROR                                    0XF20A001D /* 视频请求获取前端码流失败 */
#define EV9000_CMS_ERR_INVITE_TCP_SETUP_ERROR                                           0XF20A001E /* 视频请求TCP连接方式失败 */
#define EV9000_CMS_ERR_INVITE_TCP_CONNECTION_ERROR                                      0XF20A001F /* 视频请求TCP传输方式失败  */

/* 录像 */
#define EV9000_CMS_ERR_RECORD_FIND_RECORD_INFO_ERROR                                    0XF20B0001 /* 查找录像信息失败 */
#define EV9000_CMS_ERR_RECORD_GET_RECORD_INFO_ERROR                                     0XF20B0002 /* 获取录像信息失败 */
#define EV9000_CMS_ERR_RECORD_TIME_INFO_ERROR                                           0XF20B0003 /* 录像时间信息错误 */

/* 0X F3XX XXXX	应用端 */
#define EV9000_APPS_ERROR_NOMEMORY                       0xF3010000          // 内存不足
#define EV9000_APPS_ERROR_CREATETHREAD_FAILD             0xF3010001          // 创建线程失败
#define EV9000_APPS_ERROR_PARAM_INVALID                  0xF3010002          // 参数错误

//应用模块错误定义
#define EV9000APP_EROR_NOCTRLRESOURCE                    0xF3020000          // 没有服务资源
#define EV9000APP_EROR_NOFINDRESOURCE                    0xF3020001          // 没有查询资源
#define EV9000APP_EROR_NOPLAYRESOURCE                    0xF3020002          // 没有播放资源
#define EV9000APP_EROR_GETUSERINFOFAILD                  0xF3020003          // 获取用户信息失败,用户名密码不正确
#define EV9000APP_EROR_SERVERIDEMPTY                     0xF3020004          // CMSID为空     

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
#define EV9000SIPMODE_ERROR_INVITE_520                   0xF3080012          // invite 520

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


#endif
