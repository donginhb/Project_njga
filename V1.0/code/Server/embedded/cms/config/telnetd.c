/******************************************************************************

                  版权所有 (C), 2001-2013, 金智视讯技术有限公司

 ******************************************************************************
  文 件 名   : telnetd.c
  版 本 号   : 初稿
  作    者   : yanghaifeng
  生成日期   : 2013年7月25日 星期四
  最近修改   :
  功能描述   : Telnet登录管理
  函数列表   :
              telnetauth
              TelnetConfigSaveStateProc
              TelnetConfigSetStateProc
              TelnetSysRunInfoStateProc
              TelnetSysDiagnoseStateProc
              TelnetDebugLevelSetStateProc
              TelnetDebugSendStateProc
              TelnetExitConfirmStateProc
              TelnetLoginStateProc
              TelnetLogoutConfirmStateProc
              TelnetParseClientData
              TelnetPortSetStateProc
              TelnetPromptPasswordStateProc
              TelnetPromptUserStateProc
              TelnetRestartSystemConfirmStateProc
              TelnetSend
              TelnetServerFree
              TelnetServerInit
              TelnetServerMain
              TelnetStopSystemConfirmStateProc
              TelnetUserNameSetStateProc
              TelnetUserPasswordSetStateProc
              telnet_client_init
  修改历史   :
  1.日    期   : 2013年7月25日 星期四
    作    者   : yanghaifeng
    修改内容   : 创建文件

******************************************************************************/

/*----------------------------------------------*
 * 包含头文件                                   *
 *----------------------------------------------*/
#ifdef WIN32
#include <winsock.h>
#include <sys/types.h>
#else

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#endif

#include "common/gblconfig_proc.inc"
#include "common/gblfunc_proc.inc"
#include "common/log_proc.inc"
#include "common/common_thread_proc.inc"

#include "user/user_info_mgn.inc"
#include "device/device_info_mgn.inc"
#include "device/device_thread_proc.inc"
#include "resource/resource_info_mgn.inc"
#include "record/record_srv_proc.inc"
#include "record/record_info_mgn.inc"
#include "service/call_func_proc.inc"
#include "service/poll_srv_proc.inc"
#include "service/cruise_srv_proc.inc"
#include "service/compress_task_proc.inc"
#include "service/preset_proc.inc"
#include "route/route_info_mgn.inc"
#include "route/route_thread_proc.inc"
#include "route/platform_info_mgn.inc"
#include "route/platform_thread_proc.inc"
#include "user/user_thread_proc.inc"

#include "libsip.h"

#include "telnetd.inc"

/*----------------------------------------------*
 * 外部变量说明                                        *
 *----------------------------------------------*/
extern gbl_conf_t* pGblconf;              /* 全局配置信息 */
extern int stop_all;

extern int g_CommonDbgLevel;
extern int g_SIPStackDbgLevel;
extern int g_UserDbgLevel;
extern int g_DeviceDbgLevel;
extern int g_RouteDbgLevel;
extern int g_RecordDbgLevel;
extern int g_ResourceDbgLevel;
extern int g_CruiseDbgLevel;
extern int g_PlanDbgLevel;
extern int g_PollDbgLevel;
extern int g_IsLog2File;
extern int g_IsLog2DB;

extern int g_SystemLogLevel;

/*----------------------------------------------*
 * 外部函数原型说明                             *
 *----------------------------------------------*/
extern void do_restart();

/*----------------------------------------------*
 * 内部函数原型说明                             *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 全局变量                                     *
 *----------------------------------------------*/
int TelnetServSock = 0;

/*----------------------------------------------*
 * 模块级变量                                   *
 *----------------------------------------------*/
#define MAX_TELNETCLIENTS 3
#define MAX_TELNETUSERS   5

#define MAX_TELNET_LOGINRETRY 3
#define MAX_LOGINTIME      300  /* seconds */

typedef enum _telnet_client_state
{
    TSCS_NULL,                                /* 空状态 */
    TSCS_PROMPT_USER,                         /* 输入用户名状态 */
    TSCS_PROMPT_PASSWORD,                     /* 输入密码状态 */
    TSCS_LOGIN,                               /* 登录成功状态 */
    TSCS_DBG_SEND,                            /* 调试信息输出状态 */
    TSCS_RUNTRACE_SEND,                       /* 运行信息输出状态 */
    TSCS_DBG_SEND_MSG,                        /* 调试信息输出SIP 消息状态 */
    TSCS_SYS_SHELL,                           /* 系统shell交互状态 */
    TSCS_CONFIG_SET,                          /* 系统配置状态 */
    TSCS_SYS_RUN_INFO,                        /* 系统运行信息查询*/
    TSCS_SYS_CALL_TASK_DETAL,                 /* 系统运行呼叫任务查询*/
    TSCS_SYS_SIPUA_INFO,                      /* 系统运行SIPUA 会话信息查询*/
    TSCS_SYS_DIAGONSE,                        /* 系统诊断信息查询*/
    TSCS_SYS_RELEASE_RECORD_TASK,             /* 释放录像任务*/
    TSCS_SYS_RELEASE_RECORD_TASK_CONFIRM,     /* 释放录像任务确认*/
    TSCS_SYS_RELEASE_CALL_TASK,               /* 释放呼叫任务*/
    TSCS_SYS_RELEASE_CALL_TASK_CONFIRM,       /* 释放呼叫任务确认*/
    TSCS_SYS_STOP_POLL_TASK,                  /* 停止轮巡任务*/
    TSCS_SYS_STOP_POLL_TASK_CONFIRM,          /* 停止轮巡任务确认*/
    TSCS_SYS_STOP_CRUISE_TASK,                /* 停止巡航任务*/
    TSCS_SYS_STOP_CRUISE_TASK_CONFIRM,        /* 停止巡航任务确认*/
    TSCS_SYS_RELEASE_UA_INFO,                 /* 释放UA 信息*/
    TSCS_SYS_RELEASE_UA_INFO_CONFIRM,         /* 释放UA 信息确认*/
    TSCS_SYS_RELEASE_UAS_INFO,                /* 释放UAS 信息*/
    TSCS_SYS_RELEASE_UAS_INFO_CONFIRM,        /* 释放UAS 信息确认*/
    TSCS_SYS_RELEASE_UAC_INFO,                /* 释放UAC 信息*/
    TSCS_SYS_RELEASE_UAC_INFO_CONFIRM,        /* 释放UAC 信息确认*/
    TSCS_DBGLEVEL_SET,                        /* 配置调试等级*/
    TSCS_LOGLEVEL_SET,                        /* 配置日志等级*/
    TSCS_LOG2FILE_SET,                        /* 配置日志文件记录开关*/
    TSCS_LOG2DB_SET,                          /* 配置日志数据库记录开关*/
    TSCS_TELUSERNAME_SET,                     /* 配置Telnet用户名*/
    TSCS_TELPASSWORD_SET,                     /* 配置Telnet密码*/
    TSCS_TELPORT_SET,                         /* 配置Telnet端口*/
    TSCS_CONFIG_SAVE,                         /* 保存配置*/
    TSCS_EXIT_SYS_CONFIRM,                    /* 停止CMS系统确认*/
    TSCS_RESTART_CONFIRM,                     /* 重启CMS系统确认*/
    TSCS_LOGOUT,                              /* Telnet 登出*/
    TSCS_LOGOUT_CONFIRM,                      /* Telnet 登出确认*/
    TSCS_CLOSE,                               /* Telnet 关闭*/
    TSCS_CLOSE_CONFIRM                        /* Telnet 确认*/
} telnet_client_state;

typedef struct _telnet_client_t
{
    int sock;
    telnet_client_state state;      /* is login ? */
    char user[32];
    char password[32];
    char cmd[32];
    char cmd_para[32];
    int  login_retry;
    int  expires_time;
} telnet_client_t;

telnet_client_t TelnetClients[MAX_TELNETCLIENTS];

/*----------------------------------------------*
 * 常量定义                                     *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 宏定义                                       *
 *----------------------------------------------*/
/* define telnet command and option */
#define   TTC_SE     240    /*End of subnegotiation parameters.*/
#define   TTC_NOP    241    /*No operation.*/
#define   TTC_DM     242    /*Data Mark*/
#define   TTC_BRK    243    /*NVT character BRK.*/
#define   TTC_IP     244    /*The function IP.*/
#define   TTC_AO     245    /*The function AO.*/
#define   TTC_AYT    246    /*The function AYT.*/
#define   TTC_EC     247    /*The function EC.*/
#define   TTC_EL     248    /*The function EL.*/
#define   TTC_GA     249    /*The GA signal.*/
#define   TTC_SB     250    /*Indicates that what follows is subnegotiation of the indicated option.*/
#define   TTC_WILL   251
#define   TTC_WONT   252
#define   TTC_DO     253
#define   TTC_DONT   254
#define   TTC_IAC    255

#define COMMOND_SYS_CFG                          "config"
#define COMMOND_SYS_RUN                          "runinfo"
#define COMMOND_SYS_DIAG                         "diagnose"
#define COMMOND_SYS_VER                          "version"
#define COMMOND_SYS_DBG                          "debug"
#define COMMOND_SYS_RUNDBG                       "rundebug"
#define COMMOND_SIP_DBG                          "sipdebug"
#define COMMOND_SYS_SHELL                        "sysshell"
#define COMMOND_SYS_TIME                         "time"
#define COMMOND_SYS_STOP                         "stop"
#define COMMOND_SYS_RESTART                      "restart"
#define COMMOND_LOG_OUT                          "logout"
#define COMMOND_EXIT                             "exit"
#define COMMOND_HELP                             "help"
#define COMMOND_SAVE                             "save"
#define COMMOND_QUIT                             "quit"

#define COMMOND_RUN_ALL_ROUTE_INFO               "platform info"

#define COMMOND_RUN_HAS_USED_ROUTE_THREAD        "has used platform thread"
#define COMMOND_RUN_NO_USED_ROUTE_THREAD         "no used platform thread"
#define COMMOND_RUN_ALL_ROUTE_THREAD             "all platform thread"

#define COMMOND_RUN_ONLINE_DEVICE                "online zrv device"
#define COMMOND_RUN_OFFLINE_DEVICE               "offline zrv device"
#define COMMOND_RUN_ALL_DEVICE                   "all zrv device"

#define COMMOND_RUN_HAS_USED_DEVICE_THREAD       "has used device thread"
#define COMMOND_RUN_NO_USED_DEVICE_THREAD        "no used device thread"
#define COMMOND_RUN_ALL_DEVICE_THREAD            "all device thread"

#define COMMOND_RUN_REALPLAY_TASK                "compress task"

#define COMMOND_RUN_SYSTEM_PARAM                 "show system param"
#define COMMOND_RUN_SYSTEM_IP_INFO               "show system ip info"
#define COMMOND_RUN_SYSTEM_THREAD                "show system thread"

#define COMMOND_DIAG_RELEASE_RECORD_TASK         "release record task"
#define COMMOND_DIAG_RELEASE_CALL_TASK           "release call task"
#define COMMOND_DIAG_STOP_POLL_TASK              "stop poll task"
#define COMMOND_DIAG_STOP_CRUISE_TASK            "stop cruise task"
#define COMMOND_DIAG_RELEASE_UA_DIAG             "release all ua dialog"
#define COMMOND_DIAG_RELEASE_UAS_REGISTER        "release all uas register"
#define COMMOND_DIAG_RELEASE_UAC_REGISTER        "release all uac register"

#define COMMOND_CFG_DBG_LEVEL                    "debug level"
#define COMMOND_CFG_LOG_LEVEL                    "log level"
#define COMMOND_CFG_LOG2FILE                     "log2file switch"
#define COMMOND_CFG_LOG2DB                       "log2db switch"
#define COMMOND_CFG_TELNET_USERNAME              "telnet username"
#define COMMOND_CFG_TELNET_PASSWORD              "telnet password"
#define COMMOND_CFG_TELNET_PORT                  "telnet port"
#define COMMOND_CFG_PRINT                        "print"

char main_menu[] = "\r----- 金智视讯欢迎您使用 * iEV9000 CMS 系统 * -----\r\n\
                \r简单命令如下: \r\n\
                \rconfig(c)    : 系统配置\r\n\
                \rruninfo(r)   : 查看系统运行信息\r\n\
                \rversion(v)   : 显示系统版本号\r\n\
                \rdebug(d)     : 远程监视模式\r\n\
                \rrundebug(rd) : 运行信息远程监视模式\r\n\
                \rsysshell(ss) : 系统Shell模式\r\n\
                \rtime(t)      : 当前时间\r\n\
                \rstop         : 结束命令(停止CMS系统)\r\n\
                \rrestart      : 重启系统(重新启动CMS系统)\r\n\
                \rlogout       : Telnet登出\r\n\
                \rexit         : Telnet退出\r\n\
                \rhelp(h/?)    : 帮助\r\n$";

char main_menu_eng[] = "\r----- Wiscom welcomes your use of * iEV9000 CMS System * -----\r\n\
                \rSimple commands are as follows: \r\n\
                \rconfig(c)    : System Config\r\n\
                \rruninfo(r)   : View system running information\r\n\
                \rversion(v)   : Display system version\r\n\
                \rdebug(d)     : Remote monitoring mode\r\n\
                \rrundebug(rd) : Running information remote monitoring mode\r\n\
                \rsysshell(ss) : System Shell mode\r\n\
                \rtime(t)      : System current time\r\n\
                \rstop         : Stop command (Stop CMS system)\r\n\
                \rrestart      : Restart system (Restart CMS system)\r\n\
                \rlogout       : Telnet logout\r\n\
                \rexit         : Telnet exit\r\n\
                \rhelp(h/?)    : Help\r\n$";

char runinfo_menu[] = "\r----- * iEV9000 CMS 系统运行信息 * -----\r\n\
                \r简单命令如下: \r\n\
                \r---------------------------------------------------------------------\r\n\
                \rplatform info(pi)                  : 上级平台信息\r\n\
                \r---------------------------------------------------------------------\r\n\
                \rhas used platform thread(hupt)     : 已经使用的上级平台线程\r\n\
                \rno used platform thread(nupt)      : 没有使用的上级平台线程\r\n\
                \rall platform thread(apt)           : 所有的上级平台线程\r\n\
                \r---------------------------------------------------------------------\r\n\
                \ronline zrv device(ond)             : 在线zrv设备\r\n\
                \roffline zrv device(ofd)            : 离线zrv设备\r\n\
                \rall zrv device(ad)                 : 所有zrv设备\r\n\
                \r---------------------------------------------------------------------\r\n\
                \rhas used device thread(hudt)       : 已经使用的设备线程\r\n\
                \rno used device thread(nudt)        : 没有使用的设备线程\r\n\
                \rall device thread(adt)             : 所有的设备线程\r\n\
                \r---------------------------------------------------------------------\r\n\
                \rcompress task(ct)                  : 当前系统所有压缩任务\r\n\
                \r---------------------------------------------------------------------\r\n\
                \rshow system param(ssp)             : 显示系统全局配置参数\r\n\
                \rshow system ip info(ssii)          : 显示系统IP地址信息\r\n\
                \rshow system thread(sst)            : 显示系统处理线程\r\n\
                \r---------------------------------------------------------------------\r\n\
                \rquit(q)                            : 返回上级目录\r\n\
                \rhelp(h/?)                          : 帮助\r\n$";

char runinfo_menu_eng[] = "\r----- * iEV9000 CMS System running information * -----\r\n\
                \rSimple commands are as follows: \r\n\
                \r---------------------------------------------------------------------\r\n\
                \rplatform info(pi)                  : Superior platform information\r\n\
                \r---------------------------------------------------------------------\r\n\
                \rhas used platform thread(hupt)     : Platform thread that has been used\r\n\
                \rno used platform thread(nupt)      : Platform thread that is not used\r\n\
                \rall platform thread(apt)           : All platform threads\r\n\
                \r---------------------------------------------------------------------\r\n\
                \ronline zrv device(ond)             : Online zrv equipment\r\n\
                \roffline zrv device(ofd)            : Offline zrv device\r\n\
                \rall zrv device(ad)                 : All zrv devices\r\n\
                \r---------------------------------------------------------------------\r\n\
                \rhas used device thread(hudt)       : Device thread that has been used\r\n\
                \rno used device thread(nudt)        : Device thread that is not used\r\n\
                \rall device thread(adt)             : All device threads\r\n\
                \r---------------------------------------------------------------------\r\n\
                \rcompress task(ct)                  : All compress task\r\n\
                \r---------------------------------------------------------------------\r\n\
                \rshow system param(ssp)             : Display system global configuration parameter\r\n\
                \rshow system ip info(ssii)          : Display system IP address information\r\n\
                \rshow system thread(sst)            : Display system processing thread\r\n\
                \r---------------------------------------------------------------------\r\n\
                \rquit(q)                            : To return to a higher level\r\n\
                \rhelp(h/?)                          : Help\r\n$";

char diagnose_menu[] = "\r----- * iEV9000 CMS 系统调试 * -----\r\n\
                \r简单命令如下(注意:以下命令用于系统维护调试，可能引起系统异常，请谨慎使用): \r\n\
                \rrelease record task        : 释放录像任务\r\n\
                \rrelease call task          : 释放呼叫任务\r\n\
                \rstop poll task             : 停止轮巡任务\r\n\
                \rstop cruise task           : 停止巡航任务\r\n\
                \rrelease all ua dialog      : 释放所有SIP UA对话信息\r\n\
                \rrelease all uas register   : 释放所有服务端注册信息\r\n\
                \rrelease all uac register   : 释放所有客户端注册信息\r\n\
                \rquit(q)                    : 返回上级目录\r\n\
                \rhelp(h/?)                  : 帮助\r\n$";

char diagnose_menu_eng[] = "\r----- * iEV9000 CMS system debug * -----\r\n\
                \rSimple commands are as follows: (Note: the following commands are used for system maintenance and debugging, may cause system exceptions, please use caution): \r\n\
                \rrelease record task        : Release record video task\r\n\
                \rrelease call task          : Release call task\r\n\
                \rstop poll task             : Stop poll task\r\n\
                \rstop cruise task           : Stop cruise task\r\n\
                \rrelease all ua dialog      : Release all SIP UA dialog information\r\n\
                \rrelease all uas register   : Release all server registration information\r\n\
                \rrelease all uac register   : Release all client registration information\r\n\
                \rquit(q)                    : To return to a higher level\r\n\
                \rhelp(h/?)                  : Help\r\n$";

char config_menu[] = "\r----- * iEV9000 CMS 系统配置 * -----\r\n\
                \r简单配置命令如下,请在退出前按s 保存所做的修改：\r\n\
                \rdebug level(dl)      : Debug 级别设置(1:All 2:Debug 3:Info 4:Warning 5:Error 6:Fatal 7:Off)\r\n\
                \rlog level(ll)        : Log日志级别设置(1:NORMAL 2:WARNING 3:ERROR)\r\n\
                \rlog2file switch(lfs) : 日志记录文件开关设置(On/Off)\r\n\
                \rlog2db switch(lds)   : 日志记录数据库开关设置(On/Off)\r\n\
                \rtelnet username(tu)  : Telnet 登录用户名设置(Telnet Username)\r\n\
                \rtelnet password(tp)  : Telnet 登录密码设置(Telnet Password)\r\n\
                \rtelnet port(td)      : Telnet 登录端口设置(Telnet Port)\r\n\
                \rsave(s)              : 保存所做的修改(Save Config)\r\n\
                \rquit(q)              : 返回上级目录\r\n\
                \rprint(p)             : 显示当前配置信息\r\n\
                \rhelp(h/?)            : 帮助\r\n$";

char config_menu_eng[] = "\r----- * iEV9000 CMS system configuration * -----\r\n\
                \rSimple configuration command is as follows, please press s to save the changes before exiting：\r\n\
                \rdebug level(dl)      : Debug level settings(1:All 2:Debug 3:Info 4:Warning 5:Error 6:Fatal 7:Off)\r\n\
                \rlog level(ll)        : Log log level settings(1:NORMAL 2:WARNING 3:ERROR)\r\n\
                \rlog2file switch(lfs) : Log file switch settings(On/Off)\r\n\
                \rlog2db switch(lds)   : Log record database switch settings(On/Off)\r\n\
                \rtelnet username(tu)  : Telnet login user name set(Telnet Username)\r\n\
                \rtelnet password(tp)  : Telnet login password settings(Telnet Password)\r\n\
                \rtelnet port(td)      : Telnet login port settings(Telnet Port)\r\n\
                \rsave(s)              : Save the changes made(Save Config)\r\n\
                \rquit(q)              : To return to a higher level\r\n\
                \rprint(p)             : Display current configuration information\r\n\
                \rhelp(h/?)            : Help\r\n$";

/*****************************************************************************
 函 数 名  : telnet_client_init
 功能描述  : telnet客户端管理结构初始化
 输入参数  : telnet_client_t * sc
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年7月25日 星期四
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void telnet_client_init(telnet_client_t* sc)
{
    if (sc == NULL)
    {
        return;
    }

    sc->sock = -1;
    sc->state = TSCS_NULL;
    memset(sc->user, 0, sizeof(sc->user));
    memset(sc->password, 0, sizeof(sc->password));
    memset(sc->cmd, 0, sizeof(sc->cmd));
    memset(sc->cmd_para, 0, sizeof(sc->cmd_para));
    sc->login_retry = 0;
    sc->expires_time = MAX_LOGINTIME;
    return;
}

/*****************************************************************************
 函 数 名  : TelnetServerInit
 功能描述  : telnet服务端初始化
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年7月25日 星期四
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int  TelnetServerInit()
{
    int i;
    int sock;                        /* socket to create */
    struct sockaddr_in ServAddr; /* Local address */
    int val = 1;

    for (i = 0; i < MAX_TELNETCLIENTS; i++)
    {
        telnet_client_init(&TelnetClients[i]);
    }

    /* Create socket for incoming connections */
    if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    {
        perror("TelnetServerInit create socket error!!!     reason:");
        return -1;
    }

    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

    /* Construct local address structure */
    memset(&ServAddr, 0, sizeof(ServAddr));   /* Zero out structure */
    ServAddr.sin_family = AF_INET;                /* Internet address family */
    ServAddr.sin_addr.s_addr = htonl(INADDR_ANY); /* Any incoming interface */
    ServAddr.sin_port = htons(pGblconf->spy_port);   /* Local port */

    /* Bind to the local address */
    if (bind(sock, (struct sockaddr*) &ServAddr, sizeof(ServAddr)) < 0)
    {
        perror("TelnetServerInit bind socket error!!!     reason:");
        close(sock);
        return -1;
    }

    /* Mark the socket so it will listen for incoming connections */
    if (listen(sock, MAX_TELNETCLIENTS) < 0)
    {
        perror("TelnetServerInit listen socket error!!!     reason:");
        close(sock);
        return -1;
    }

    TelnetServSock = sock;

    return 0;
}

/*****************************************************************************
 函 数 名  : TelnetServerFree
 功能描述  : telnet服务端释放
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年7月25日 星期四
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void TelnetServerFree()
{
    int i;

    if (TelnetServSock <= 0)
    {
        return;
    }

    close(TelnetServSock);
    TelnetServSock = -1;

    for (i = 0; i < MAX_TELNETCLIENTS; i++)
    {
        if (TelnetClients[i].sock != -1)
        {
            close(TelnetClients[i].sock);
        }

        telnet_client_init(&TelnetClients[i]);
    }

    return;
}

/*****************************************************************************
 函 数 名  : TelnetServerMain
 功能描述  : telnet服务端主程序
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年7月25日 星期四
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void TelnetServerMain()
{
    int i = 0;
    int p = 0;
    int  maxDescriptor = 0;      /* Maximum socket descriptor value */
    fd_set sockSet;              /* Set of socket descriptors for select() */
    struct timeval val;          /* Timeout for select() */
    unsigned char buff[512];
    int  clntSock = -1;          /* Socket descriptor for client */
    int  recvSize = 0;
    int iSendBuf = 1024 * 1024;
    struct timeval iTimeout;

    unsigned char rbuf[] = {TTC_IAC, TTC_DO, 1, \
                            TTC_IAC, TTC_WILL, 1, \
                            TTC_IAC, TTC_WILL, 3
                           };

    if (TelnetServSock <= 0)
    {
        TelnetServerInit();

        if (TelnetServSock <= 0)
        {
            return;
        }
    }

    FD_ZERO(&sockSet);
    FD_SET(TelnetServSock, &sockSet);
    maxDescriptor = TelnetServSock;

    for (i = 0; i < MAX_TELNETCLIENTS; i++)
    {
        if (TelnetClients[i].sock != -1)
        {
            FD_SET(TelnetClients[i].sock, &sockSet);

            if (TelnetClients[i].sock > maxDescriptor)
            {
                maxDescriptor = TelnetClients[i].sock;
            }
        }
    }

    val.tv_sec = 0;       /* timeout (secs.) */
    val.tv_usec = 10;    /* 10 microseconds */

    i = select(maxDescriptor + 1, &sockSet, NULL, NULL, &val);

    if (i == 0)
    {
        return;
    }

    if (i == -1)
    {
        return;
    }

    if (FD_ISSET(TelnetServSock, &sockSet))
    {
        struct sockaddr_in ClntAddr;     /* Client address */
        unsigned int clntLen;            /* Length of client address data structure */
        clntLen = sizeof(ClntAddr);

        /* Wait for a client to connect */
        if ((clntSock = accept(TelnetServSock, (struct sockaddr*) &ClntAddr,
                               &clntLen)) < 0)
        {
            return;
        }

        /* 设置发送超时时间 */
        iTimeout.tv_sec = 1; /* 1秒超时 */
        iTimeout.tv_usec = 0;
        setsockopt(clntSock, SOL_SOCKET, SO_SNDTIMEO, (char *)&iTimeout, sizeof(struct timeval));

        /* 设置发送缓冲区 */
        setsockopt(clntSock, SOL_SOCKET, SO_SNDBUF, (char *)&iSendBuf, sizeof(int));


        for (p = 0; p < MAX_TELNETCLIENTS; p++)
        {
            if (TelnetClients[p].sock == -1)
            {
                break;
            }
        }

        if (p >= MAX_TELNETCLIENTS)
        {
            close(clntSock);

            /* 重新初始化所有客户端信息 */
            for (i = 0; i < MAX_TELNETCLIENTS; i++)
            {
                if (TelnetClients[i].sock != -1)
                {
                    close(TelnetClients[i].sock);
                }

                telnet_client_init(&TelnetClients[i]);
            }

            return;
        }
        else
        {
            if (send(clntSock, rbuf, sizeof(rbuf), 0) < 0)
            {
                close(clntSock);
                return;
            }
            else
            {
                TelnetClients[p].sock = clntSock;
            }
        }

        if (TelnetClients[p].state == TSCS_NULL)
        {
            char rbuf1[] = "\r\nCMS Telnet Login:";
            send(clntSock, rbuf, sizeof(rbuf), 0);
            send(clntSock, rbuf1, sizeof(rbuf1) - 1, 0);
            TelnetClients[p].state = TSCS_PROMPT_USER;
        }
    }

    for (p = 0; p < MAX_TELNETCLIENTS; p++)
    {
        if (TelnetClients[p].sock == -1)
        {
            continue;
        }

        if (FD_ISSET(TelnetClients[p].sock, &sockSet))
        {
            clntSock = TelnetClients[p].sock;

            memset(buff, 0, 512);

            if ((recvSize = recv(clntSock, buff, sizeof(buff), 0)) <= 0)
            {
                close(clntSock);
                telnet_client_init(&TelnetClients[p]);
                continue;
            }

            TelnetParseClientData(p, buff, recvSize);
            TelnetClients[p].expires_time = MAX_LOGINTIME;
        }
    }

    return;
}

/*****************************************************************************
 函 数 名  : ScanClientIfExpires
 功能描述  : 扫描客户端是否超时
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年7月31日 星期三
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void ScanClientIfExpires()
{
    int p = 0;
    int  clntSock = -1;      /* Socket descriptor for client */

    for (p = 0; p < MAX_TELNETCLIENTS; p++)
    {
        if (TelnetClients[p].sock == -1)
        {
            continue;
        }

        if ((TSCS_NULL == TelnetClients[p].state)
            || (TSCS_PROMPT_USER == TelnetClients[p].state)
            || (TSCS_PROMPT_PASSWORD == TelnetClients[p].state)
            || (TSCS_DBG_SEND == TelnetClients[p].state)
            || (TSCS_RUNTRACE_SEND == TelnetClients[p].state)
            || (TSCS_DBG_SEND_MSG == TelnetClients[p].state)
            || (TSCS_SYS_SHELL == TelnetClients[p].state))
        {
            continue;
        }

        if (TelnetClients[p].sock == -1)
        {
            continue;
        }

        TelnetClients[p].expires_time--;

        if (0 == TelnetClients[p].expires_time)
        {
            clntSock = TelnetClients[p].sock;

            char rbuf0[] = "\r登录超时，Telnet自动登出，请重新登录\r\n";
            char rbuf1[] = "\rCMS Telnet Login:";

            send(clntSock, rbuf0, sizeof(rbuf0) - 1, 0);
            send(clntSock, rbuf1, sizeof(rbuf1) - 1, 0);

            TelnetClients[p].state = TSCS_PROMPT_USER;
            memset(TelnetClients[p].user, 0, sizeof(TelnetClients[p].user));
            memset(TelnetClients[p].password, 0, sizeof(TelnetClients[p].password));
            memset(TelnetClients[p].cmd, 0, sizeof(TelnetClients[p].cmd));
            TelnetClients[p].login_retry = 0;
        }
    }

    return;
}

/*****************************************************************************
 函 数 名  : TelnetParseClientData
 功能描述  : 解析客户端命令
 输入参数  : int cpos
             unsigned char* buf
             int len
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年7月25日 星期四
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int TelnetParseClientData(int cpos, unsigned char* buf, int len)
{
    int sock;

    if (cpos < 0 || cpos >= MAX_TELNETCLIENTS)
    {
        return -1;
    }

    if (buf == NULL || len <= 0)
    {
        return -1;
    }

    sock = TelnetClients[cpos].sock;

    if (sock == -1)
    {
        return -1;
    }

    switch (buf[0])
    {
        case TTC_IAC:
            switch (buf[1])
            {
                default:
                    if (TelnetClients[cpos].state == TSCS_NULL)
                    {
                        /* send login */
                        char rbuf[] = "\r\nCMS Telnet Login:";
                        send(sock, rbuf, sizeof(rbuf) - 1, 0);
                        TelnetClients[cpos].state = TSCS_PROMPT_USER;
                    }

                    break;
            }

            break;

        default:
            switch (TelnetClients[cpos].state)
            {
                case TSCS_NULL:
                    break;

                case TSCS_PROMPT_USER:
                    TelnetPromptUserStateProc(cpos, buf, len);
                    break;

                case TSCS_PROMPT_PASSWORD:
                    TelnetPromptPasswordStateProc(cpos, buf, len);
                    break;

                case TSCS_LOGIN:
                    TelnetLoginStateProc(cpos, buf, len);
                    break;

                case TSCS_DBG_SEND:
                    TelnetDebugSendStateProc(cpos, buf, len);
                    break;

                case TSCS_RUNTRACE_SEND:
                    TelnetRunTraceSendStateProc(cpos, buf, len);
                    break;

                case TSCS_DBG_SEND_MSG:
                    TelnetSIPDebugSendStateProc(cpos, buf, len);
                    break;

                case TSCS_SYS_SHELL:
                    TelnetSystemShellStateProc(cpos, buf, len);
                    break;

                case TSCS_CONFIG_SET:
                    TelnetConfigSetStateProc(cpos, buf, len);
                    break;

                case TSCS_SYS_RUN_INFO:
                    TelnetSysRunInfoStateProc(cpos, buf, len);
                    break;

                case TSCS_SYS_CALL_TASK_DETAL:
                    TelnetSysCallTaskDetalStateProc(cpos, buf, len);
                    break;

                case TSCS_SYS_SIPUA_INFO:
                    TelnetSysSIPUAInfoStateProc(cpos, buf, len);
                    break;

                case TSCS_SYS_DIAGONSE:
                    TelnetSysDiagnoseStateProc(cpos, buf, len);
                    break;

                case TSCS_SYS_RELEASE_RECORD_TASK:
                    TelnetReleaseRecordTaskStateProc(cpos, buf, len);
                    break;

                case TSCS_SYS_RELEASE_RECORD_TASK_CONFIRM:
                    TelnetReleaseRecordTaskConfirmStateProc(cpos, buf, len);
                    break;

                case TSCS_SYS_RELEASE_CALL_TASK:
                    TelnetReleaseCallTaskStateProc(cpos, buf, len);
                    break;

                case TSCS_SYS_RELEASE_CALL_TASK_CONFIRM:
                    TelnetReleaseCallTaskConfirmStateProc(cpos, buf, len);
                    break;

                case TSCS_SYS_STOP_POLL_TASK:
                    TelnetStopPollTaskStateProc(cpos, buf, len);
                    break;

                case TSCS_SYS_STOP_POLL_TASK_CONFIRM:
                    TelnetStopPollTaskConfirmStateProc(cpos, buf, len);
                    break;

                case TSCS_SYS_STOP_CRUISE_TASK:
                    TelnetStopCruiseTaskStateProc(cpos, buf, len);
                    break;

                case TSCS_SYS_STOP_CRUISE_TASK_CONFIRM:
                    TelnetStopCruiseTaskConfirmStateProc(cpos, buf, len);
                    break;

                case TSCS_SYS_RELEASE_UA_INFO_CONFIRM:
                    TelnetReleaseAllUADialogInfoConfirmStateProc(cpos, buf, len);
                    break;

                case TSCS_SYS_RELEASE_UAS_INFO_CONFIRM:
                    TelnetReleaseAllUASInfoConfirmStateProc(cpos, buf, len);
                    break;

                case TSCS_SYS_RELEASE_UAC_INFO_CONFIRM:
                    TelnetReleaseAllUACInfoConfirmStateProc(cpos, buf, len);
                    break;

                case TSCS_DBGLEVEL_SET:
                    TelnetDebugLevelSetStateProc(cpos, buf, len);
                    break;

                case TSCS_LOGLEVEL_SET:
                    TelnetLogLevelSetStateProc(cpos, buf, len);
                    break;

                case TSCS_LOG2FILE_SET:
                    TelnetLog2FileSetStateProc(cpos, buf, len);
                    break;

                case TSCS_LOG2DB_SET:
                    TelnetLog2DBSetStateProc(cpos, buf, len);
                    break;

                case TSCS_TELUSERNAME_SET:
                    TelnetUserNameSetStateProc(cpos, buf, len);
                    break;

                case TSCS_TELPASSWORD_SET:
                    TelnetUserPasswordSetStateProc(cpos, buf, len);
                    break;

                case TSCS_TELPORT_SET:
                    TelnetPortSetStateProc(cpos, buf, len);
                    break;

                case TSCS_CONFIG_SAVE:
                    TelnetConfigSaveStateProc(cpos, buf, len);
                    break;

                case TSCS_EXIT_SYS_CONFIRM:
                    TelnetStopSystemConfirmStateProc(cpos, buf, len);
                    break;

                case TSCS_RESTART_CONFIRM:
                    TelnetRestartSystemConfirmStateProc(cpos, buf, len);
                    break;

                case TSCS_LOGOUT_CONFIRM:
                    TelnetLogoutConfirmStateProc(cpos, buf, len);
                    break;

                case TSCS_CLOSE_CONFIRM:
                    TelnetExitConfirmStateProc(cpos, buf, len);
                    break;

                default:
                    if (1 == g_Language)
                    {
                        char rbuf[] = "\r\nSystem error, please log in again!";
                        send(sock, rbuf, sizeof(rbuf) - 1, 0);
                    }
                    else
                    {
                        char rbuf[] = "\r\n系统错误，请重新登录!";
                        send(sock, rbuf, sizeof(rbuf) - 1, 0);
                    }

                    close(sock);
                    TelnetClients[cpos].sock = -1;
                    TelnetClients[cpos].state = TSCS_NULL;
                    memset(TelnetClients[cpos].user, 0, sizeof(TelnetClients[cpos].user));
                    memset(TelnetClients[cpos].password, 0, sizeof(TelnetClients[cpos].password));
                    break;
            }

            break;
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : TelnetPromptUserStateProc
 功能描述  : 输入用户名阶段的处理
 输入参数  : int cpos
                            unsigned char* buf
                            int len
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年7月25日 星期四
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int TelnetPromptUserStateProc(int cpos, unsigned char* buf, int len)
{
    int sock;

    if (cpos < 0 || cpos >= MAX_TELNETCLIENTS)
    {
        return -1;
    }

    if (buf == NULL || len <= 0)
    {
        return -1;
    }

    sock = TelnetClients[cpos].sock;

    if (sock == -1)
    {
        return -1;
    }

    /* echo first */
    send(sock, buf, len, 0);

    if ((len == 2 && !strncmp((char*)buf, "\r\n", len))
        || (len == 1 && !strncmp((char*)buf, "\r", len)))
    {
        if (TelnetClients[cpos].user[0] != '\0')
        {
            char rbuf[] = "\r\nPassword:";
            send(sock, rbuf, sizeof(rbuf) - 1, 0);
            TelnetClients[cpos].state = TSCS_PROMPT_PASSWORD;
            return 0;
        }

        if (TelnetClients[cpos].login_retry >= MAX_TELNET_LOGINRETRY)
        {
            char rbuf[] = "\r\nCMS Telnet Login Retry 3 Times!!!";
            send(sock, rbuf, sizeof(rbuf) - 1, 0);
            close(sock);
            telnet_client_init(&TelnetClients[cpos]);
        }
        else
        {
            char rbuf[] = "\r\nCMS Telnet Login:";
            send(sock, rbuf, sizeof(rbuf) - 1, 0);
            memset(TelnetClients[cpos].user, 0, sizeof(TelnetClients[cpos].user));
            TelnetClients[cpos].login_retry++;
        }
    }
    else
    {
        //int i = 0;

        if (1 == len && buf[0] == '\b')
        {
            //printf("\r\n backspace \r\n");
            int len = strlen(TelnetClients[cpos].user);
            TelnetClients[cpos].user[len - 1] = '\0';
        }
        else
        {
            //printf("\r\n not backspace \r\n");
            strncat(TelnetClients[cpos].user, (char*)buf, len);
        }

#if 0
        int len1 = strlen(TelnetClients[cpos].user);

        for (i = 0; i < len1; i++)
        {
            if (TelnetClients[cpos].user[i] == 0x08)
            {
                printf("\r\n TelnetClients[cpos].user[%d]=\\b \r\n", i);
            }

            printf("\r\n TelnetClients[cpos].user[%d]=%c \r\n", i, TelnetClients[cpos].user[i]);
        }

        printf("\r\n buf[0]=%c, len=%d, TelnetClients[cpos].user=%s, strlen(TelnetClients[cpos].user)=%d \r\n", buf[0], len, TelnetClients[cpos].user, strlen(TelnetClients[cpos].user));
#endif
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : TelnetPromptPasswordStateProc
 功能描述  : 输入密码阶段的处理
 输入参数  : int cpos
                            unsigned char* buf
                            int len
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年7月25日 星期四
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int TelnetPromptPasswordStateProc(int cpos, unsigned char* buf, int len)
{
    int sock;

    if (cpos < 0 || cpos >= MAX_TELNETCLIENTS)
    {
        return -1;
    }

    if (buf == NULL || len <= 0)
    {
        return -1;
    }

    sock = TelnetClients[cpos].sock;

    if (sock == -1)
    {
        return -1;
    }

    if ((len == 2 && !strncmp((char*)buf, "\r\n", len))
        || (len == 1 && !strncmp((char*)buf, "\r", len)))
    {
        /* find user and check password */
        if (telnetauth(TelnetClients[cpos].user, TelnetClients[cpos].password))
        {
            char rbuf[] = "\r\nLogin OK!\r\n";
            send(sock, rbuf, sizeof(rbuf) - 1, 0);

            if (1 == g_Language)
            {
                send(sock, main_menu_eng, sizeof(main_menu_eng) - 1, 0);
            }
            else
            {
                send(sock, main_menu, sizeof(main_menu) - 1, 0);
            }

            TelnetClients[cpos].state = TSCS_LOGIN;
        }
        else
        {
            if (TelnetClients[cpos].login_retry >= MAX_TELNET_LOGINRETRY)
            {
                char rbuf[] = "\r\nCMS Telnet Login Retry 3 Times!!!";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
                close(sock);
                telnet_client_init(&TelnetClients[cpos]);
            }
            else
            {
                char rbuf[] = "\r\nLogin Incorrect!\r\n\r\nCMS Telnet Login:";
                TelnetClients[cpos].state = TSCS_PROMPT_USER;
                memset(TelnetClients[cpos].user, 0, sizeof(TelnetClients[cpos].user));
                memset(TelnetClients[cpos].password, 0, sizeof(TelnetClients[cpos].password));
                memset(TelnetClients[cpos].cmd, 0, sizeof(TelnetClients[cpos].cmd));
                TelnetClients[cpos].login_retry++;
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }

        }
    }
    else
    {
        if (1 == len && buf[0] == '\b')
        {
            int len = strlen(TelnetClients[cpos].password);
            TelnetClients[cpos].password[len - 1] = '\0';
        }
        else
        {
            strncat(TelnetClients[cpos].password, (char*)buf, len);
        }
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : TelnetLoginStateProc
 功能描述  : 登录成功状态下的处理
 输入参数  : int cpos
                            unsigned char* buf
                            int len
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年7月25日 星期四
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int TelnetLoginStateProc(int cpos, unsigned char* buf, int len)
{
    int sock;

    if (cpos < 0 || cpos >= MAX_TELNETCLIENTS)
    {
        return -1;
    }

    if (buf == NULL || len <= 0)
    {
        return -1;
    }

    sock = TelnetClients[cpos].sock;

    if (sock == -1)
    {
        return -1;
    }

    /* echo first */
    send(sock, buf, len, 0);

    if ((len == 2 && !strncmp((char*)buf, "\r\n", len))
        || (len == 1 && !strncmp((char*)buf, "\r", len)))
    {
        /* parse command */
        if (!strcmp(TelnetClients[cpos].cmd, COMMOND_EXIT))
        {
            if (1 == g_Language)
            {
                char rbuf[] = "\rAre you sure you want to exit Telnet ?('y' or 'n'):\r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }
            else
            {
                char rbuf[] = "\r你确定要退出Telnet登录吗 ?('y' or 'n'):\r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }

            TelnetClients[cpos].state = TSCS_CLOSE_CONFIRM;
        }
        else if ((!strcmp(TelnetClients[cpos].cmd, COMMOND_SYS_CFG))
                 || (!strcmp(TelnetClients[cpos].cmd, "c")))
        {
            char rbuf[] = "\r";
            send(sock, rbuf, sizeof(rbuf) - 1, 0);

            if (1 == g_Language)
            {
                send(sock, config_menu_eng, sizeof(config_menu_eng) - 1, 0);
            }
            else
            {
                send(sock, config_menu, sizeof(config_menu) - 1, 0);
            }

            TelnetClients[cpos].state = TSCS_CONFIG_SET;
        }
        else if ((!strcmp(TelnetClients[cpos].cmd, COMMOND_SYS_RUN))
                 || (!strcmp(TelnetClients[cpos].cmd, "r")))
        {
            char rbuf[] = "\r";
            send(sock, rbuf, sizeof(rbuf) - 1, 0);

            if (1 == g_Language)
            {
                send(sock, runinfo_menu_eng, sizeof(runinfo_menu_eng) - 1, 0);
            }
            else
            {
                send(sock, runinfo_menu, sizeof(runinfo_menu) - 1, 0);
            }

            TelnetClients[cpos].state = TSCS_SYS_RUN_INFO;
        }
        else if ((!strcmp(TelnetClients[cpos].cmd, COMMOND_SYS_DIAG))
                 || (!strcmp(TelnetClients[cpos].cmd, "g")))
        {
            char rbuf[] = "\r";
            send(sock, rbuf, sizeof(rbuf) - 1, 0);

            //send(sock, (char*)"暂未启用\r\n$", sizeof("暂未启用\r\n$") - 1, 0);
            if (1 == g_Language)
            {
                send(sock, diagnose_menu_eng, sizeof(diagnose_menu_eng) - 1, 0);
            }
            else
            {
                send(sock, diagnose_menu, sizeof(diagnose_menu) - 1, 0);
            }

            TelnetClients[cpos].state = TSCS_SYS_DIAGONSE;
            //TelnetClients[cpos].state = TSCS_LOGIN;
        }
        else if ((!strcmp(TelnetClients[cpos].cmd, COMMOND_SYS_VER))
                 || (!strcmp(TelnetClients[cpos].cmd, "v")))
        {
            char rbuf[128] = {0};
            char strCMSVer[256] = {0};
            //GetAppVerInfo(rbuf);
            snprintf(strCMSVer, 256, "\rCMS Version: %s\r\n$", rbuf);
            send(sock, strCMSVer, strlen(strCMSVer), 0);
        }
        else if ((!strcmp(TelnetClients[cpos].cmd, COMMOND_SYS_DBG))
                 || (!strcmp(TelnetClients[cpos].cmd, "d")))
        {
            if (1 == g_Language)
            {
                char rbuf[] = "\r Enter the debug information to the remote view state, you can enter the Q exit at any time.\r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }
            else
            {
                char rbuf[] = "\r 进入调试信息远端查看状态，可以随时输入q 退出.\r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }

            TelnetClients[cpos].state = TSCS_DBG_SEND;
        }
        else if ((!strcmp(TelnetClients[cpos].cmd, COMMOND_SYS_RUNDBG))
                 || (!strcmp(TelnetClients[cpos].cmd, "rd")))
        {
            if (1 == g_Language)
            {
                char rbuf[] = "\r Into the remote viewing of the running information, you can enter the Q exit.\r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }
            else
            {
                char rbuf[] = "\r 进入运行信息远端查看状态，可以随时输入q 退出.\r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }

            TelnetClients[cpos].state = TSCS_RUNTRACE_SEND;
        }
        else if ((!strcmp(TelnetClients[cpos].cmd, COMMOND_SIP_DBG))
                 || (!strcmp(TelnetClients[cpos].cmd, "sd")))
        {
            if (1 == g_Language)
            {
                char rbuf[] = "\r Enter the SIP message debug information remote view state, you can enter the Q exit.\r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }
            else
            {
                char rbuf[] = "\r 进入SIP消息调试信息远端查看状态，可以随时输入q 退出.\r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }

            TelnetClients[cpos].state = TSCS_DBG_SEND_MSG;
        }
        else if ((!strcmp(TelnetClients[cpos].cmd, COMMOND_SYS_SHELL))
                 || (!strcmp(TelnetClients[cpos].cmd, "ss")))
        {
            if (1 == g_Language)
            {
                char rbuf[] = "\r Enter the System Shell state, you can enter the Q exit.\r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }
            else
            {
                char rbuf[] = "\r 进入系统Shell模式，可以随时输入q 退出.\r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }

            TelnetClients[cpos].state = TSCS_SYS_SHELL;
        }
        else if ((!strcmp(TelnetClients[cpos].cmd, COMMOND_SYS_TIME))
                 || (!strcmp(TelnetClients[cpos].cmd, "t")))
        {
            char rbuf[32] = { 0 };
            char strtm[20] = { 0 };
            time_t now = time(NULL);
            struct tm tp = { 0 };
            localtime_r(&now, &tp); /* 获取当前时间 */
            strftime(strtm, 20, "%Y/%m/%d %H:%M:%S", &tp);

            sprintf(rbuf, "\rTIME:%s\r\n$", strtm);
            send(sock, rbuf, strlen(rbuf), 0);
        }
        else if (!strcmp(TelnetClients[cpos].cmd, COMMOND_SYS_STOP))
        {
            if (1 == g_Language)
            {
                char rbuf[] = "\rAre you sure you want to stop the CMS system ?('y' or 'n'):\r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }
            else
            {
                char rbuf[] = "\r你确定要停止CMS系统吗 ?('y' or 'n'):\r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }

            TelnetClients[cpos].state = TSCS_EXIT_SYS_CONFIRM;
        }
        else if (!strcmp(TelnetClients[cpos].cmd, COMMOND_SYS_RESTART))
        {
            if (1 == g_Language)
            {
                char rbuf[] = "\rAre you sure you want to restart the CMS system ?('y' or 'n'):\r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }
            else
            {
                char rbuf[] = "\r你确定要重启CMS系统吗 ?('y' or 'n'):\r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }

            TelnetClients[cpos].state = TSCS_RESTART_CONFIRM;
        }
        else if (!strcmp(TelnetClients[cpos].cmd, COMMOND_LOG_OUT))
        {
            if (1 == g_Language)
            {
                char rbuf[] = "\rAre you sure you want to log off Telnet ?('y' or 'n'):\r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }
            else
            {
                char rbuf[] = "\r你确定要注销Telnet登录吗 ?('y' or 'n'):\r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }

            TelnetClients[cpos].state = TSCS_LOGOUT_CONFIRM;

        }
        else if ((!strcmp(TelnetClients[cpos].cmd, COMMOND_HELP))
                 || (!strcmp(TelnetClients[cpos].cmd, "h"))
                 || (!strcmp(TelnetClients[cpos].cmd, "?")))
        {
            char rbuf[] = "\r";
            send(sock, rbuf, sizeof(rbuf) - 1, 0);

            if (1 == g_Language)
            {
                send(sock, main_menu_eng, sizeof(main_menu_eng) - 1, 0);
            }
            else
            {
                send(sock, main_menu, sizeof(main_menu) - 1, 0);
            }
        }
        else if (TelnetClients[cpos].cmd[0] == '\0')
        {
            char rbuf[] = "\r$";
            send(sock, rbuf, sizeof(rbuf) - 1, 0);
        }
        else
        {
            if (1 == g_Language)
            {
                char rbuf[] = "\rThe command you entered is not correct!\r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
                send(sock, main_menu_eng, sizeof(main_menu_eng) - 1, 0);
            }
            else
            {
                char rbuf[] = "\r你输入的命令不正确!\r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
                send(sock, main_menu, sizeof(main_menu) - 1, 0);
            }
        }

        memset(TelnetClients[cpos].cmd, 0, sizeof(TelnetClients[cpos].cmd));
    }
    else
    {
        if (1 == len && buf[0] == '\b')
        {
            int len = strlen(TelnetClients[cpos].cmd);
            TelnetClients[cpos].cmd[len - 1] = '\0';
        }
        else
        {
            strncat(TelnetClients[cpos].cmd, (char*)buf, len);
        }
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : TelnetDebugSendStateProc
 功能描述  : 调试信息输出状态下的处理
 输入参数  : int cpos
                            unsigned char* buf
                            int len
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年7月25日 星期四
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int TelnetDebugSendStateProc(int cpos, unsigned char* buf, int len)
{
    int sock;

    if (cpos < 0 || cpos >= MAX_TELNETCLIENTS)
    {
        return -1;
    }

    if (buf == NULL || len <= 0)
    {
        return -1;
    }

    sock = TelnetClients[cpos].sock;

    if (sock == -1)
    {
        return -1;
    }

    send(sock, buf, len, 0);

    if ((len == 2 && !strncmp((char*)buf, "\r\n", len))
        || (len == 1 && !strncmp((char*)buf, "\r", len)))
    {
        /* parse command */
        if (!strcmp(TelnetClients[cpos].cmd, "q"))
        {
            TelnetClients[cpos].state = TSCS_LOGIN;

            if (1 == g_Language)
            {
                send(sock, main_menu_eng, sizeof(main_menu_eng) - 1, 0);
            }
            else
            {
                send(sock, main_menu, sizeof(main_menu) - 1, 0);
            }
        }
        else if (!strcmp(TelnetClients[cpos].cmd, COMMOND_HELP))
        {
            if (1 == g_Language)
            {
                char rbuf[] = "\rPlease enter the Q exit remote monitor state!\r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }
            else
            {
                char rbuf[] = "\r请输入q 退出远程监视状态!\r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }
        }
        else if (TelnetClients[cpos].cmd[0] == '\0')
        {
            if (1 == g_Language)
            {
                char rbuf[] = "\rPlease enter the Q exit remote monitor state!\r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }
            else
            {
                char rbuf[] = "\r请输入q 退出远程监视状态!\r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }
        }
        else
        {
            if (1 == g_Language)
            {
                char rbuf[] = "\rThe command you entered is incorrect. Please enter the Q to exit the remote monitor state!\r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }
            else
            {
                char rbuf[] = "\r你输入的命令不正确,请输入q 退出远程监视状态!\r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }
        }

        memset(TelnetClients[cpos].cmd, 0, sizeof(TelnetClients[cpos].cmd));
    }
    else
    {
        if (1 == len && buf[0] == '\b')
        {
            int len = strlen(TelnetClients[cpos].cmd);
            TelnetClients[cpos].cmd[len - 1] = '\0';
        }
        else
        {
            strncat(TelnetClients[cpos].cmd, (char*)buf, len);
        }
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : TelnetRunTraceSendStateProc
 功能描述  : 运行信息输出状态下的处理
 输入参数  : int cpos
                            unsigned char* buf
                            int len
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年7月25日 星期四
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int TelnetRunTraceSendStateProc(int cpos, unsigned char* buf, int len)
{
    int sock;

    if (cpos < 0 || cpos >= MAX_TELNETCLIENTS)
    {
        return -1;
    }

    if (buf == NULL || len <= 0)
    {
        return -1;
    }

    sock = TelnetClients[cpos].sock;

    if (sock == -1)
    {
        return -1;
    }

    send(sock, buf, len, 0);

    if ((len == 2 && !strncmp((char*)buf, "\r\n", len))
        || (len == 1 && !strncmp((char*)buf, "\r", len)))
    {
        /* parse command */
        if (!strcmp(TelnetClients[cpos].cmd, "q"))
        {
            TelnetClients[cpos].state = TSCS_LOGIN;

            if (1 == g_Language)
            {
                send(sock, main_menu_eng, sizeof(main_menu_eng) - 1, 0);
            }
            else
            {
                send(sock, main_menu, sizeof(main_menu) - 1, 0);
            }
        }
        else if (!strcmp(TelnetClients[cpos].cmd, COMMOND_HELP))
        {
            if (1 == g_Language)
            {
                char rbuf[] = "\rPlease enter the Q exit to monitor the status of the operation!\r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }
            else
            {
                char rbuf[] = "\r请输入q 退出运行信息监视状态!\r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }
        }
        else if (TelnetClients[cpos].cmd[0] == '\0')
        {
            if (1 == g_Language)
            {
                char rbuf[] = "\rPlease enter the Q exit to monitor the status of the operation!\r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }
            else
            {
                char rbuf[] = "\r请输入q 退出运行信息监视状态!\r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }
        }
        else
        {
            if (1 == g_Language)
            {
                char rbuf[] = "\rThe command you entered is incorrect. Please enter the Q to exit the runtime information monitor!\r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }
            else
            {
                char rbuf[] = "\r你输入的命令不正确,请输入q 退出运行信息监视状态!\r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }
        }

        memset(TelnetClients[cpos].cmd, 0, sizeof(TelnetClients[cpos].cmd));
    }
    else
    {
        if (1 == len && buf[0] == '\b')
        {
            int len = strlen(TelnetClients[cpos].cmd);
            TelnetClients[cpos].cmd[len - 1] = '\0';
        }
        else
        {
            strncat(TelnetClients[cpos].cmd, (char*)buf, len);
        }
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : TelnetSIPDebugSendStateProc
 功能描述  : SIP消息调试信息输出状态下的处理
 输入参数  : int cpos
                            unsigned char* buf
                            int len
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年9月4日 星期三
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int TelnetSIPDebugSendStateProc(int cpos, unsigned char* buf, int len)
{
    int sock;

    if (cpos < 0 || cpos >= MAX_TELNETCLIENTS)
    {
        return -1;
    }

    if (buf == NULL || len <= 0)
    {
        return -1;
    }

    sock = TelnetClients[cpos].sock;

    if (sock == -1)
    {
        return -1;
    }

    send(sock, buf, len, 0);

    if ((len == 2 && !strncmp((char*)buf, "\r\n", len))
        || (len == 1 && !strncmp((char*)buf, "\r", len)))
    {
        /* parse command */
        if (!strcmp(TelnetClients[cpos].cmd, "q"))
        {
            TelnetClients[cpos].state = TSCS_LOGIN;

            if (1 == g_Language)
            {
                send(sock, main_menu_eng, sizeof(main_menu_eng) - 1, 0);
            }
            else
            {
                send(sock, main_menu, sizeof(main_menu) - 1, 0);
            }
        }
        else if (!strcmp(TelnetClients[cpos].cmd, COMMOND_HELP))
        {
            if (1 == g_Language)
            {
                char rbuf[] = "\rPlease enter a Q exit remote SIP message monitor state!\r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }
            else
            {
                char rbuf[] = "\r请输入q 退出远程SIP消息监视状态!\r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }
        }
        else if (TelnetClients[cpos].cmd[0] == '\0')
        {
            if (1 == g_Language)
            {
                char rbuf[] = "\rPlease enter a Q exit remote SIP message monitor state!\r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }
            else
            {
                char rbuf[] = "\r请输入q 退出远程SIP消息监视状态!\r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }
        }
        else
        {
            if (1 == g_Language)
            {
                char rbuf[] = "\rThe command you entered is incorrect. Please enter a Q to exit the remote SIP message monitor state!\r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }
            else
            {
                char rbuf[] = "\r你输入的命令不正确,请输入q 退出远程SIP消息监视状态!\r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }
        }

        memset(TelnetClients[cpos].cmd, 0, sizeof(TelnetClients[cpos].cmd));
    }
    else
    {
        if (1 == len && buf[0] == '\b')
        {
            int len = strlen(TelnetClients[cpos].cmd);
            TelnetClients[cpos].cmd[len - 1] = '\0';
        }
        else
        {
            strncat(TelnetClients[cpos].cmd, (char*)buf, len);
        }
    }

    return 0;
}


/*****************************************************************************
 函 数 名  : TelnetSystemShellStateProc
 功能描述  : 系统Shell模式下的处理
 输入参数  : int cpos
             unsigned char* buf
             int len
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年3月8日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int TelnetSystemShellStateProc(int cpos, unsigned char* buf, int len)
{
    int sock;

    if (cpos < 0 || cpos >= MAX_TELNETCLIENTS)
    {
        return -1;
    }

    if (buf == NULL || len <= 0)
    {
        return -1;
    }

    sock = TelnetClients[cpos].sock;

    if (sock == -1)
    {
        return -1;
    }

    send(sock, buf, len, 0);

    if ((len == 2 && !strncmp((char*)buf, "\r\n", len))
        || (len == 1 && !strncmp((char*)buf, "\r", len)))
    {
        /* parse command */
        if (!strcmp(TelnetClients[cpos].cmd, "q"))
        {
            TelnetClients[cpos].state = TSCS_LOGIN;

            if (1 == g_Language)
            {
                send(sock, main_menu_eng, sizeof(main_menu_eng) - 1, 0);
            }
            else
            {
                send(sock, main_menu, sizeof(main_menu) - 1, 0);
            }
        }
        else if (!strcmp(TelnetClients[cpos].cmd, COMMOND_HELP))
        {
            if (1 == g_Language)
            {
                char rbuf[] = "\rPlease enter a Q exit system shell state!\r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }
            else
            {
                char rbuf[] = "\r请输入q 退出系统shell模式!\r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }
        }
        else if (!strcmp(TelnetClients[cpos].cmd, (char*)"top"))
        {
            if (1 == g_Language)
            {
                char rbuf[] = "\rNot Support Top Command, Please enter other command or Q exit system shell state!\r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }
            else
            {
                char rbuf[] = "\r不支持TOP命令，请输入其他命令，或者输入q 退出系统shell模式!\r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }
        }
        else if (TelnetClients[cpos].cmd[0] == '\0')
        {
            if (1 == g_Language)
            {
                char rbuf[] = "\rPlease enter command you want execute or Q exit system shell state!\r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }
            else
            {
                char rbuf[] = "\r请输入你要执行的命令，或者输入q 退出系统shell模式!\r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }
        }
        else
        {
            FILE   *stream = NULL;
            char   buf[256] = {0};

            memset(buf, '\0', 256);  //初始化buf,以免后面写如乱码到文件中
            stream = popen(TelnetClients[cpos].cmd, "r");   //将“ls －l”命令的输出 通过管道读取（“r”参数）到FILE* stream

            if (NULL != stream)
            {
                while ((fgets(buf, 256, stream)) != NULL)
                {
                    send(sock, buf, strlen(buf) - 1, 0);
                    send(sock, "\r\n", 2, 0);
                    memset(buf, '\0', 256);  //初始化buf,以免后面写如乱码到文件中
                }

                char rbuf[] = "\r\n$";
                send(sock, rbuf, 3, 0);

                pclose(stream);
                stream = NULL;
            }
            else
            {
                if (1 == g_Language)
                {
                    char rbuf[] = "\rPlease enter command you want execute or Q exit system shell state!\r\n$";
                    send(sock, rbuf, sizeof(rbuf) - 1, 0);
                }
                else
                {
                    char rbuf[] = "\r请输入你要执行的命令，或者输入q 退出系统shell模式!\r\n$";
                    send(sock, rbuf, sizeof(rbuf) - 1, 0);
                }
            }
        }

        memset(TelnetClients[cpos].cmd, 0, sizeof(TelnetClients[cpos].cmd));
    }
    else
    {
        if (1 == len && buf[0] == '\b')
        {
            int len = strlen(TelnetClients[cpos].cmd);
            TelnetClients[cpos].cmd[len - 1] = '\0';
        }
        else
        {
            strncat(TelnetClients[cpos].cmd, (char*)buf, len);
        }
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : TelnetConfigSetStateProc
 功能描述  : 配置状态下的处理
 输入参数  : int cpos
                            unsigned char* buf
                            int len
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年7月25日 星期四
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int TelnetConfigSetStateProc(int cpos, unsigned char* buf, int len)
{
    int sock;

    if (cpos < 0 || cpos >= MAX_TELNETCLIENTS)
    {
        return -1;
    }

    if (buf == NULL || len <= 0)
    {
        return -1;
    }

    sock = TelnetClients[cpos].sock;

    if (sock == -1)
    {
        return -1;
    }

    /* echo first */
    send(sock, buf, len, 0);

    if ((len == 2 && !strncmp((char*)buf, "\r\n", len))
        || (len == 1 && !strncmp((char*)buf, "\r", len)))
    {
        /* parse command */
        if ((!strcmp(TelnetClients[cpos].cmd, COMMOND_CFG_DBG_LEVEL))
            || (!strcmp(TelnetClients[cpos].cmd, "dl")))
        {
            char curbuf[32];

            if (1 == g_Language)
            {
                sprintf(curbuf, "\rCurrent Debug level:%d\r\n", g_CommonDbgLevel);
                char rbuf[] = "\rEnter the Debug level you want to set(1:All 2:Debug 3:Trace 4:Info 5:Warning 6:Error 7:Fatal 8:Off),Enter the key to confirm the input, the direct return is not modified\r\n$";
                TelnetClients[cpos].state = TSCS_DBGLEVEL_SET;
                send(sock, curbuf, strlen(curbuf), 0);
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }
            else
            {
                sprintf(curbuf, "\r当前Debug级别是:%d\r\n", g_CommonDbgLevel);
                char rbuf[] = "\r输入你要设定的Debug级别(1:All 2:Debug 3:Trace 4:Info 5:Warning 6:Error 7:Fatal 8:Off),回车键确认输入,直接回车不修改\r\n$";
                TelnetClients[cpos].state = TSCS_DBGLEVEL_SET;
                send(sock, curbuf, strlen(curbuf), 0);
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }
        }
        else if ((!strcmp(TelnetClients[cpos].cmd, COMMOND_CFG_LOG_LEVEL))
                 || (!strcmp(TelnetClients[cpos].cmd, "ll")))
        {
            char curbuf[32];

            if (1 == g_Language)
            {
                sprintf(curbuf, "\rCurrent Log level:%d\r\n", g_SystemLogLevel);
                char rbuf[] = "\rEnter the Log level you want to set(1:NORMAL 2:WARNING 3:ERROR),Enter the key to confirm the input, the direct return is not modified\r\n$";
                TelnetClients[cpos].state = TSCS_LOGLEVEL_SET;
                send(sock, curbuf, strlen(curbuf), 0);
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }
            else
            {
                sprintf(curbuf, "\r当前Log日志级别是:%d\r\n", g_SystemLogLevel);
                char rbuf[] = "\r输入你要设定的Log日志级别(1:NORMAL 2:WARNING 3:ERROR),回车键确认输入,直接回车不修改\r\n$";
                TelnetClients[cpos].state = TSCS_LOGLEVEL_SET;
                send(sock, curbuf, strlen(curbuf), 0);
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }
        }
        else if ((!strcmp(TelnetClients[cpos].cmd, COMMOND_CFG_LOG2FILE))
                 || (!strcmp(TelnetClients[cpos].cmd, "lfs")))
        {
            char curbuf[32] = {0};

            if (1 == g_Language)
            {
                if (g_IsLog2File)
                {
                    sprintf(curbuf, "\rThe current log records to the file identification:%s\r\n", (char*)"On");
                }
                else
                {
                    sprintf(curbuf, "\rThe current log records to the file identification:%s\r\n", (char*)"Off");
                }

                char rbuf[] = "\rEnter the log record to the file identifier(On/Off),Enter the key to confirm the input, the direct return is not modified\r\n$";
                TelnetClients[cpos].state = TSCS_LOG2FILE_SET;
                send(sock, curbuf, strlen(curbuf), 0);
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }
            else
            {
                if (g_IsLog2File)
                {
                    sprintf(curbuf, "\r当前日志记录到文件标识是:%s\r\n", (char*)"On");
                }
                else
                {
                    sprintf(curbuf, "\r当前日志记录到文件标识是:%s\r\n", (char*)"Off");
                }

                char rbuf[] = "\r输入日志记录到文件标识(On/Off),回车键确认输入,直接回车不修改\r\n$";
                TelnetClients[cpos].state = TSCS_LOG2FILE_SET;
                send(sock, curbuf, strlen(curbuf), 0);
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }
        }
        else if ((!strcmp(TelnetClients[cpos].cmd, COMMOND_CFG_LOG2DB))
                 || (!strcmp(TelnetClients[cpos].cmd, "lds")))
        {
            char curbuf[32] = {0};

            if (1 == g_Language)
            {
                if (g_IsLog2DB)
                {
                    sprintf(curbuf, "\rThe current log records to the database identity:%s\r\n", (char*)"On");
                }
                else
                {
                    sprintf(curbuf, "\rThe current log records to the database identity:%s\r\n", (char*)"Off");
                }

                char rbuf[] = "\rEnter the log record to the database identity(On/Off),Enter the key to confirm the input, the direct return is not modified\r\n$";
                TelnetClients[cpos].state = TSCS_LOG2DB_SET;
                send(sock, curbuf, strlen(curbuf), 0);
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }
            else
            {
                if (g_IsLog2DB)
                {
                    sprintf(curbuf, "\r当前日志记录到数据库标识是:%s\r\n", (char*)"On");
                }
                else
                {
                    sprintf(curbuf, "\r当前日志记录到数据库标识是:%s\r\n", (char*)"Off");
                }

                char rbuf[] = "\r输入日志记录到数据库标识(On/Off),回车键确认输入,直接回车不修改\r\n$";
                TelnetClients[cpos].state = TSCS_LOG2DB_SET;
                send(sock, curbuf, strlen(curbuf), 0);
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }
        }
        else if ((!strcmp(TelnetClients[cpos].cmd, COMMOND_CFG_TELNET_USERNAME))
                 || (!strcmp(TelnetClients[cpos].cmd, "tu")))
        {
            char curbuf[32] = {0};

            if (1 == g_Language)
            {
                sprintf(curbuf, "\rCurrent Telnet login user name is:%s\r\n", pGblconf->spy_username);
                char rbuf[] = "\rModify this parameter, after saving the data, it will automatically exit Telnet login!!!\r\n\
                           \rEnter the Telnet you want to set the user login user name, enter the key to confirm the input, the direct return is not modified\r\n$";
                TelnetClients[cpos].state = TSCS_TELUSERNAME_SET;
                send(sock, curbuf, strlen(curbuf), 0);
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }
            else
            {
                sprintf(curbuf, "\r当前Telnet登录用户名是:%s\r\n", pGblconf->spy_username);
                char rbuf[] = "\r修改此参数，在保存数据之后，将会自动退出Telnet登录!!!\r\n\
                           \r输入你要设定的Telnet登录用户名,回车键确认输入,直接回车不修改\r\n$";
                TelnetClients[cpos].state = TSCS_TELUSERNAME_SET;
                send(sock, curbuf, strlen(curbuf), 0);
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }
        }
        else if ((!strcmp(TelnetClients[cpos].cmd, COMMOND_CFG_TELNET_PASSWORD))
                 || (!strcmp(TelnetClients[cpos].cmd, "tp")))
        {
            char curbuf[32] = {0};

            if (1 == g_Language)
            {
                sprintf(curbuf, "\rCurrent Telnet login password is:%s\r\n", pGblconf->spy_password);
                char rbuf[] = "\rModify this parameter, after saving the data, it will automatically exit Telnet login!!!\r\n\
                          \rEnter the Telnet you want to set the password, enter the key to confirm the input, the direct return is not modified\r\n$";
                TelnetClients[cpos].state = TSCS_TELPASSWORD_SET;
                send(sock, curbuf, strlen(curbuf), 0);
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }
            else
            {
                sprintf(curbuf, "\r当前Telnet登录密码是:%s\r\n", pGblconf->spy_password);
                char rbuf[] = "\r修改此参数，在保存数据之后，将会自动退出Telnet登录!!!\r\n\
                          \r输入你要设定的Telnet登录密码,回车键确认输入,直接回车不修改\r\n$";
                TelnetClients[cpos].state = TSCS_TELPASSWORD_SET;
                send(sock, curbuf, strlen(curbuf), 0);
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }
        }
        else if ((!strcmp(TelnetClients[cpos].cmd, COMMOND_CFG_TELNET_PORT))
                 || (!strcmp(TelnetClients[cpos].cmd, "td")))
        {
            char curbuf[32] = {0};

            if (1 == g_Language)
            {
                sprintf(curbuf, "\r当前Telnet登录端口是:%d\r\n", pGblconf->spy_port);
                char rbuf[] = "\r修改此参数，在保存数据之后，将会使所有Telnet用户退出登录!!!\r\n\
            \r输入你要设定的Telnet登录端口,回车键确认输入,直接回车不修改\r\n$";
                TelnetClients[cpos].state = TSCS_TELPORT_SET;
                send(sock, curbuf, strlen(curbuf), 0);
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }
            else
            {
                sprintf(curbuf, "\r当前Telnet登录端口是:%d\r\n", pGblconf->spy_port);
                char rbuf[] = "\r修改此参数，在保存数据之后，将会使所有Telnet用户退出登录!!!\r\n\
            \r输入你要设定的Telnet登录端口,回车键确认输入,直接回车不修改\r\n$";
                TelnetClients[cpos].state = TSCS_TELPORT_SET;
                send(sock, curbuf, strlen(curbuf), 0);
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }
        }
        else if ((!strcmp(TelnetClients[cpos].cmd, COMMOND_SAVE))
                 || (!strcmp(TelnetClients[cpos].cmd, "s")))
        {
            //char rbuf[] = "\r确认保存参数吗?('y' or 'n'):\r\n$";
            //TelnetClients[cpos].state = TSCS_CONFIG_SAVE;

            if (1 == g_Language)
            {
                char rbuf[] = "\rNot enabled\r\n$";
                TelnetClients[cpos].state = TSCS_CONFIG_SET;
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }
            else
            {
                char rbuf[] = "\r暂未启用\r\n$";
                TelnetClients[cpos].state = TSCS_CONFIG_SET;
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }
        }
        else if ((!strcmp(TelnetClients[cpos].cmd, COMMOND_CFG_PRINT))
                 || (!strcmp(TelnetClients[cpos].cmd, "p")))
        {
            char current_config[256] = {0};
            char strLog2File[8] = {0};
            char strLog2DB[8] = {0};

            if (g_IsLog2File)
            {
                sprintf(strLog2File, "%s", (char*)"On");
            }
            else
            {
                sprintf(strLog2File, "%s", (char*)"Off");
            }

            if (g_IsLog2DB)
            {
                sprintf(strLog2DB, "%s", (char*)"On");
            }
            else
            {
                sprintf(strLog2DB, "%s", (char*)"Off");
            }

            if (1 == g_Language)
            {
                sprintf(current_config, "\rDebug level:%d\r\n\
                            \rLog file switch:%s\r\n\
                            \rLog record to database switch:%s\r\n\
                            \rTelnet Login user name:%s\r\n\
                            \rTelnet Login password:%s\r\n\
                            \rTelnet Login port:%d\r\n$", g_CommonDbgLevel, strLog2File, strLog2DB, pGblconf->spy_username, pGblconf->spy_password, pGblconf->spy_port);
            }
            else
            {
                sprintf(current_config, "\rDebug调试级别:%d\r\n\
                            \r日志记录到文件开关:%s\r\n\
                            \r日志记录到数据库开关:%s\r\n\
                            \rTelnet 登录用户名:%s\r\n\
                            \rTelnet 登录密码:%s\r\n\
                            \rTelnet 登录端口:%d\r\n$", g_CommonDbgLevel, strLog2File, strLog2DB, pGblconf->spy_username, pGblconf->spy_password, pGblconf->spy_port);
            }


            send(sock, current_config, strlen(current_config), 0);
        }
        else if ((!strcmp(TelnetClients[cpos].cmd, COMMOND_QUIT))
                 || (!strcmp(TelnetClients[cpos].cmd, "q")))
        {
            char rbuf[] = "\r";
            TelnetClients[cpos].state = TSCS_LOGIN;
            send(sock, rbuf, sizeof(rbuf) - 1, 0);

            if (1 == g_Language)
            {
                send(sock, main_menu_eng, sizeof(main_menu_eng) - 1, 0);
            }
            else
            {
                send(sock, main_menu, sizeof(main_menu) - 1, 0);
            }
        }
        else if ((!strcmp(TelnetClients[cpos].cmd, COMMOND_HELP))
                 || (!strcmp(TelnetClients[cpos].cmd, "h"))
                 || (!strcmp(TelnetClients[cpos].cmd, "?")))
        {
            char rbuf[] = "\r";
            send(sock, rbuf, sizeof(rbuf) - 1, 0);

            if (1 == g_Language)
            {
                send(sock, config_menu_eng, sizeof(config_menu_eng) - 1, 0);
            }
            else
            {
                send(sock, config_menu, sizeof(config_menu) - 1, 0);
            }
        }
        else if (TelnetClients[cpos].cmd[0] == '\0')
        {
            char rbuf[] = "\r$";
            send(sock, rbuf, sizeof(rbuf) - 1, 0);
        }
        else
        {
            if (1 == g_Language)
            {
                char rbuf[] = "\rThe command you entered is not correct!\r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
                send(sock, config_menu_eng, sizeof(config_menu_eng) - 1, 0);
            }
            else
            {
                char rbuf[] = "\r你输入的命令不正确!\r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
                send(sock, config_menu, sizeof(config_menu) - 1, 0);
            }
        }

        memset(TelnetClients[cpos].cmd, 0, sizeof(TelnetClients[cpos].cmd));
    }
    else
    {
        if (1 == len && buf[0] == '\b')
        {
            int len = strlen(TelnetClients[cpos].cmd);
            TelnetClients[cpos].cmd[len - 1] = '\0';
        }
        else
        {
            strncat(TelnetClients[cpos].cmd, (char*)buf, len);
        }
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : TelnetSysRunInfoStateProc
 功能描述  : 系统运行信息查看下的处理
 输入参数  : int cpos
                            unsigned char* buf
                            int len
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年7月25日 星期四
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int TelnetSysRunInfoStateProc(int cpos, unsigned char* buf, int len)
{
    int sock;

    if (cpos < 0 || cpos >= MAX_TELNETCLIENTS)
    {
        return -1;
    }

    if (buf == NULL || len <= 0)
    {
        return -1;
    }

    sock = TelnetClients[cpos].sock;

    if (sock == -1)
    {
        return -1;
    }

    /* echo first */
    send(sock, buf, len, 0);

    if ((len == 2 && !strncmp((char*)buf, "\r\n", len))
        || (len == 1 && !strncmp((char*)buf, "\r", len)))
    {
        /* parse command */
        if ((!strcmp(TelnetClients[cpos].cmd, COMMOND_RUN_ALL_ROUTE_INFO))
            || (!strcmp(TelnetClients[cpos].cmd, "pi")))
        {
            ShowPlatformInfo(sock, 2);
            char rbuf[] = "\r$";
            send(sock, rbuf, sizeof(rbuf) - 1, 0);
        }
        else if ((!strcmp(TelnetClients[cpos].cmd, COMMOND_RUN_ONLINE_DEVICE))
                 || (!strcmp(TelnetClients[cpos].cmd, "ond")))
        {
            ShowZRVDeviceInfo(sock, 1);
            char rbuf[] = "\r$";
            send(sock, rbuf, sizeof(rbuf) - 1, 0);
        }
        else if ((!strcmp(TelnetClients[cpos].cmd, COMMOND_RUN_OFFLINE_DEVICE))
                 || (!strcmp(TelnetClients[cpos].cmd, "ofd")))
        {
            ShowZRVDeviceInfo(sock, 0);
            char rbuf[] = "\r$";
            send(sock, rbuf, sizeof(rbuf) - 1, 0);
        }
        else if ((!strcmp(TelnetClients[cpos].cmd, COMMOND_RUN_ALL_DEVICE))
                 || (!strcmp(TelnetClients[cpos].cmd, "ad")))
        {
            ShowZRVDeviceInfo(sock, 2);
            char rbuf[] = "\r$";
            send(sock, rbuf, sizeof(rbuf) - 1, 0);
        }
        else if ((!strcmp(TelnetClients[cpos].cmd, COMMOND_RUN_HAS_USED_DEVICE_THREAD))
                 || (!strcmp(TelnetClients[cpos].cmd, "hudt")))
        {
            ShowConnectTCPZRVDevice(sock);
            char rbuf[] = "\r$";
            send(sock, rbuf, sizeof(rbuf) - 1, 0);
        }
        else if ((!strcmp(TelnetClients[cpos].cmd, COMMOND_RUN_NO_USED_DEVICE_THREAD))
                 || (!strcmp(TelnetClients[cpos].cmd, "nudt")))
        {
            ShowConnectTCPZRVDevice(sock);
            char rbuf[] = "\r$";
            send(sock, rbuf, sizeof(rbuf) - 1, 0);
        }
        else if ((!strcmp(TelnetClients[cpos].cmd, COMMOND_RUN_ALL_DEVICE_THREAD))
                 || (!strcmp(TelnetClients[cpos].cmd, "adt")))
        {
            ShowConnectTCPZRVDevice(sock);
            char rbuf[] = "\r$";
            send(sock, rbuf, sizeof(rbuf) - 1, 0);
        }
        else if ((!strcmp(TelnetClients[cpos].cmd, COMMOND_RUN_REALPLAY_TASK))
                 || (!strcmp(TelnetClients[cpos].cmd, "ct")))
        {
            ShowCompressTask(sock);
            char rbuf[] = "\r$";
            send(sock, rbuf, sizeof(rbuf) - 1, 0);
        }
        else if ((!strcmp(TelnetClients[cpos].cmd, COMMOND_RUN_HAS_USED_ROUTE_THREAD))
                 || (!strcmp(TelnetClients[cpos].cmd, "hupt")))
        {
            show_platform_srv_proc_thread(sock, 1);
            char rbuf[] = "\r$";
            send(sock, rbuf, sizeof(rbuf) - 1, 0);
        }
        else if ((!strcmp(TelnetClients[cpos].cmd, COMMOND_RUN_NO_USED_ROUTE_THREAD))
                 || (!strcmp(TelnetClients[cpos].cmd, "nupt")))
        {
            show_platform_srv_proc_thread(sock, 0);
            char rbuf[] = "\r$";
            send(sock, rbuf, sizeof(rbuf) - 1, 0);
        }
        else if ((!strcmp(TelnetClients[cpos].cmd, COMMOND_RUN_ALL_ROUTE_THREAD))
                 || (!strcmp(TelnetClients[cpos].cmd, "apt")))
        {
            show_platform_srv_proc_thread(sock, 2);
            char rbuf[] = "\r$";
            send(sock, rbuf, sizeof(rbuf) - 1, 0);
        }
        else if ((!strcmp(TelnetClients[cpos].cmd, COMMOND_RUN_SYSTEM_PARAM))
                 || (!strcmp(TelnetClients[cpos].cmd, "ssp")))
        {
            show_system_gbl_param(sock);
            char rbuf[] = "\r$";
            send(sock, rbuf, sizeof(rbuf) - 1, 0);
        }
        else if ((!strcmp(TelnetClients[cpos].cmd, COMMOND_RUN_SYSTEM_IP_INFO))
                 || (!strcmp(TelnetClients[cpos].cmd, "ssii")))
        {
            show_system_ip_info(sock);
            char rbuf[] = "\r$";
            send(sock, rbuf, sizeof(rbuf) - 1, 0);
        }
        else if ((!strcmp(TelnetClients[cpos].cmd, COMMOND_RUN_SYSTEM_THREAD))
                 || (!strcmp(TelnetClients[cpos].cmd, "sst")))
        {
            show_system_proc_thread(sock);
            char rbuf[] = "\r$";
            send(sock, rbuf, sizeof(rbuf) - 1, 0);
        }
        else if ((!strcmp(TelnetClients[cpos].cmd, COMMOND_QUIT))
                 || (!strcmp(TelnetClients[cpos].cmd, "q")))
        {
            char rbuf[] = "\r";
            TelnetClients[cpos].state = TSCS_LOGIN;
            send(sock, rbuf, sizeof(rbuf) - 1, 0);

            if (1 == g_Language)
            {
                send(sock, main_menu_eng, sizeof(main_menu_eng) - 1, 0);
            }
            else
            {
                send(sock, main_menu, sizeof(main_menu) - 1, 0);
            }
        }
        else if ((!strcmp(TelnetClients[cpos].cmd, COMMOND_HELP))
                 || (!strcmp(TelnetClients[cpos].cmd, "h"))
                 || (!strcmp(TelnetClients[cpos].cmd, "?")))
        {
            char rbuf[] = "\r";
            send(sock, rbuf, sizeof(rbuf) - 1, 0);

            if (1 == g_Language)
            {
                send(sock, runinfo_menu_eng, sizeof(runinfo_menu_eng) - 1, 0);
            }
            else
            {
                send(sock, runinfo_menu, sizeof(runinfo_menu) - 1, 0);
            }
        }
        else if (TelnetClients[cpos].cmd[0] == '\0')
        {
            char rbuf[] = "\r$";
            send(sock, rbuf, sizeof(rbuf) - 1, 0);
        }
        else
        {
            if (1 == g_Language)
            {
                char rbuf[] = "\rThe command you entered is not correct!\r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
                send(sock, runinfo_menu_eng, sizeof(runinfo_menu_eng) - 1, 0);
            }
            else
            {
                char rbuf[] = "\r你输入的命令不正确!\r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
                send(sock, runinfo_menu, sizeof(runinfo_menu) - 1, 0);
            }
        }

        memset(TelnetClients[cpos].cmd, 0, sizeof(TelnetClients[cpos].cmd));
    }
    else
    {
        if (1 == len && buf[0] == '\b')
        {
            int len = strlen(TelnetClients[cpos].cmd);
            TelnetClients[cpos].cmd[len - 1] = '\0';
        }
        else
        {
            strncat(TelnetClients[cpos].cmd, (char*)buf, len);
        }
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : TelnetSysCallTaskDetalStateProc
 功能描述  : 查看系统呼叫任务状态下的处理
 输入参数  : int cpos
                            unsigned char* buf
                            int len
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年7月29日 星期一
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int TelnetSysCallTaskDetalStateProc(int cpos, unsigned char* buf, int len)
{
    int sock;

    if (cpos < 0 || cpos >= MAX_TELNETCLIENTS)
    {
        return -1;
    }

    if (buf == NULL || len <= 0)
    {
        return -1;
    }

    sock = TelnetClients[cpos].sock;

    if (sock == -1)
    {
        return -1;
    }

    /* echo first */
    send(sock, buf, len, 0);

    if ((len == 2 && !strncmp((char*)buf, "\r\n", len))
        || (len == 1 && !strncmp((char*)buf, "\r", len)))
    {
        /* parse command */
        if (TelnetClients[cpos].cmd[0] != '\0')
        {
            if ((!strcmp(TelnetClients[cpos].cmd, COMMOND_QUIT))
                || (!strcmp(TelnetClients[cpos].cmd, "q")))
            {
                char rbuf[] = "\r";
                TelnetClients[cpos].state = TSCS_SYS_RUN_INFO;
                send(sock, rbuf, sizeof(rbuf) - 1, 0);

                if (1 == g_Language)
                {
                    send(sock, runinfo_menu_eng, sizeof(runinfo_menu_eng) - 1, 0);
                }
                else
                {
                    send(sock, runinfo_menu, sizeof(runinfo_menu) - 1, 0);
                }
            }
            else
            {
                int cr_index = osip_atoi(TelnetClients[cpos].cmd);

                if (cr_index >= 0)
                {
                    ShowCallTaskDetail(sock, cr_index);
                    TelnetClients[cpos].state = TSCS_SYS_RUN_INFO;
                    char rbuf1[] = "\r$";
                    send(sock, rbuf1, sizeof(rbuf1) - 1, 0);
                }
                else
                {
                    if (1 == g_Language)
                    {
                        char rbuf[] = "\rYour input parameters are incorrect, please re-enter!\r\n$";
                        send(sock, rbuf, sizeof(rbuf) - 1, 0);
                    }
                    else
                    {
                        char rbuf[] = "\r你输入的参数有误，请重新输入!\r\n$";
                        send(sock, rbuf, sizeof(rbuf) - 1, 0);
                    }

                    TelnetClients[cpos].state = TSCS_SYS_CALL_TASK_DETAL;
                    char rbuf1[] = "\r$";
                    send(sock, rbuf1, sizeof(rbuf1) - 1, 0);
                }
            }
        }
        else
        {
            char rbuf[] = "\r";
            TelnetClients[cpos].state = TSCS_SYS_RUN_INFO;
            send(sock, rbuf, sizeof(rbuf) - 1, 0);

            if (1 == g_Language)
            {
                send(sock, runinfo_menu_eng, sizeof(runinfo_menu_eng) - 1, 0);
            }
            else
            {
                send(sock, runinfo_menu, sizeof(runinfo_menu) - 1, 0);
            }
        }

        memset(TelnetClients[cpos].cmd, 0, sizeof(TelnetClients[cpos].cmd));
    }
    else
    {
        if (1 == len && buf[0] == '\b')
        {
            int len = strlen(TelnetClients[cpos].cmd);
            TelnetClients[cpos].cmd[len - 1] = '\0';
        }
        else
        {
            strncat(TelnetClients[cpos].cmd, (char*)buf, len);
        }
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : TelnetSysSIPUAInfoStateProc
 功能描述  : 查看SIPUA会话信息状态下的处理
 输入参数  : int cpos
                            unsigned char* buf
                            int len
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年7月30日 星期二
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int TelnetSysSIPUAInfoStateProc(int cpos, unsigned char* buf, int len)
{
    int sock;

    if (cpos < 0 || cpos >= MAX_TELNETCLIENTS)
    {
        return -1;
    }

    if (buf == NULL || len <= 0)
    {
        return -1;
    }

    sock = TelnetClients[cpos].sock;

    if (sock == -1)
    {
        return -1;
    }

    /* echo first */
    send(sock, buf, len, 0);

    if ((len == 2 && !strncmp((char*)buf, "\r\n", len))
        || (len == 1 && !strncmp((char*)buf, "\r", len)))
    {
        /* parse command */
        if (TelnetClients[cpos].cmd[0] != '\0')
        {
            if ((!strcmp(TelnetClients[cpos].cmd, COMMOND_QUIT))
                || (!strcmp(TelnetClients[cpos].cmd, "q")))
            {
                char rbuf[] = "\r";
                TelnetClients[cpos].state = TSCS_SYS_RUN_INFO;
                send(sock, rbuf, sizeof(rbuf) - 1, 0);

                if (1 == g_Language)
                {
                    send(sock, runinfo_menu_eng, sizeof(runinfo_menu_eng) - 1, 0);
                }
                else
                {
                    send(sock, runinfo_menu, sizeof(runinfo_menu) - 1, 0);
                }
            }
            else
            {
                int ua_index = osip_atoi(TelnetClients[cpos].cmd);

                if (ua_index >= 0)
                {
                    SIP_ShowSIPUADetail(sock, ua_index);
                    TelnetClients[cpos].state = TSCS_SYS_RUN_INFO;
                    char rbuf1[] = "\r$";
                    send(sock, rbuf1, sizeof(rbuf1) - 1, 0);
                }
                else
                {
                    if (1 == g_Language)
                    {
                        char rbuf[] = "\rYour input parameters are incorrect, please re-enter!\r\n$";
                        send(sock, rbuf, sizeof(rbuf) - 1, 0);
                    }
                    else
                    {
                        char rbuf[] = "\r你输入的参数有误，请重新输入!\r\n$";
                        send(sock, rbuf, sizeof(rbuf) - 1, 0);
                    }

                    TelnetClients[cpos].state = TSCS_SYS_SIPUA_INFO;
                    char rbuf1[] = "\r$";
                    send(sock, rbuf1, sizeof(rbuf1) - 1, 0);
                }
            }
        }
        else
        {
            SIP_ShowSIPUATask(sock);
            TelnetClients[cpos].state = TSCS_SYS_RUN_INFO;
            char rbuf1[] = "\r$";
            send(sock, rbuf1, sizeof(rbuf1) - 1, 0);
        }

        memset(TelnetClients[cpos].cmd, 0, sizeof(TelnetClients[cpos].cmd));
    }
    else
    {
        if (1 == len && buf[0] == '\b')
        {
            int len = strlen(TelnetClients[cpos].cmd);
            TelnetClients[cpos].cmd[len - 1] = '\0';
        }
        else
        {
            strncat(TelnetClients[cpos].cmd, (char*)buf, len);
        }
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : TelnetSysDiagnoseStateProc
 功能描述  : 系统诊断信息查看下的处理
 输入参数  : int cpos
                            unsigned char* buf
                            int len
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年7月25日 星期四
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int TelnetSysDiagnoseStateProc(int cpos, unsigned char* buf, int len)
{
    int sock;

    if (cpos < 0 || cpos >= MAX_TELNETCLIENTS)
    {
        return -1;
    }

    if (buf == NULL || len <= 0)
    {
        return -1;
    }

    sock = TelnetClients[cpos].sock;

    if (sock == -1)
    {
        return -1;
    }

    /* echo first */
    send(sock, buf, len, 0);

    if ((len == 2 && !strncmp((char*)buf, "\r\n", len))
        || (len == 1 && !strncmp((char*)buf, "\r", len)))
    {
        /* parse command */
        if (!strcmp(TelnetClients[cpos].cmd, COMMOND_DIAG_RELEASE_RECORD_TASK))
        {
            if (1 == g_Language)
            {
                char rbuf[] = "\rEnter the video tasks you want to release, enter the Enter key to confirm the input, and enter the all to release all of the video tasks\r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }
            else
            {
                char rbuf[] = "\r输入你要释放的录像任务索引，回车键确认输入，输入all则表示释放所有的录像任务\r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }

            TelnetClients[cpos].state = TSCS_SYS_RELEASE_RECORD_TASK;
            char rbuf1[] = "\r$";
            send(sock, rbuf1, sizeof(rbuf1) - 1, 0);
        }
        else if (!strcmp(TelnetClients[cpos].cmd, COMMOND_DIAG_RELEASE_CALL_TASK))
        {
            if (1 == g_Language)
            {
                char rbuf[] = "\rEnter the call task index that you want to release, enter the Enter key to confirm the input, and enter the all to release all call tasks\r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }
            else
            {
                char rbuf[] = "\r输入你要释放的呼叫任务索引，回车键确认输入，输入all则表示释放所有的呼叫任务\r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }

            TelnetClients[cpos].state = TSCS_SYS_RELEASE_CALL_TASK;
            char rbuf1[] = "\r$";
            send(sock, rbuf1, sizeof(rbuf1) - 1, 0);
        }
        else if (!strcmp(TelnetClients[cpos].cmd, COMMOND_DIAG_STOP_POLL_TASK))
        {
            if (1 == g_Language)
            {
                char rbuf[] = "\rEnter you want to stop the poll ID, enter the key to confirm the input, the input all is said to stop all of poll task\r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }
            else
            {
                char rbuf[] = "\r输入你要停止的轮巡ID，回车键确认输入，输入all则表示停止所有的轮巡任务\r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }

            TelnetClients[cpos].state = TSCS_SYS_STOP_POLL_TASK;
            char rbuf1[] = "\r$";
            send(sock, rbuf1, sizeof(rbuf1) - 1, 0);
        }
        else if (!strcmp(TelnetClients[cpos].cmd, COMMOND_DIAG_STOP_CRUISE_TASK))
        {
            if (1 == g_Language)
            {
                char rbuf[] = "\rEnter you want to stop the cruise ID, enter the key to confirm the input, the input all is said to stop of cruise task\r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }
            else
            {
                char rbuf[] = "\r输入你要停止的巡航ID，回车键确认输入，输入all则表示停止所有的巡航任务\r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }

            TelnetClients[cpos].state = TSCS_SYS_STOP_CRUISE_TASK;
            char rbuf1[] = "\r$";
            send(sock, rbuf1, sizeof(rbuf1) - 1, 0);
        }
        else if (!strcmp(TelnetClients[cpos].cmd, COMMOND_DIAG_RELEASE_UA_DIAG))
        {
            if (1 == g_Language)
            {
                char rbuf[] = "\rAre you sure you want to release all the UA SIP dialog information ?('y' or 'n'):\r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }
            else
            {
                char rbuf[] = "\r你确定要释放所有的SIP UA对话信息吗 ?('y' or 'n'):\r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }

            TelnetClients[cpos].state = TSCS_SYS_RELEASE_UA_INFO_CONFIRM;
        }
        else if (!strcmp(TelnetClients[cpos].cmd, COMMOND_DIAG_RELEASE_UAS_REGISTER))
        {
            if (1 == g_Language)
            {
                char rbuf[] = "\rAre you sure you want to release all the service registry information UAS ?('y' or 'n'):\r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }
            else
            {
                char rbuf[] = "\r你确定要释放所有的服务端注册信息UAS吗 ?('y' or 'n'):\r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }

            TelnetClients[cpos].state = TSCS_SYS_RELEASE_UAS_INFO_CONFIRM;
        }
        else if (!strcmp(TelnetClients[cpos].cmd, COMMOND_DIAG_RELEASE_UAC_REGISTER))
        {
            if (1 == g_Language)
            {
                char rbuf[] = "\rAre you sure you want to release all the client registration information UAC ?('y' or 'n'):\r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }
            else
            {
                char rbuf[] = "\r你确定要释放所有的客户端注册信息UAC吗 ?('y' or 'n'):\r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }

            TelnetClients[cpos].state = TSCS_SYS_RELEASE_UAC_INFO_CONFIRM;
        }
        else if ((!strcmp(TelnetClients[cpos].cmd, COMMOND_QUIT))
                 || (!strcmp(TelnetClients[cpos].cmd, "q")))
        {
            char rbuf[] = "\r";
            TelnetClients[cpos].state = TSCS_LOGIN;
            send(sock, rbuf, sizeof(rbuf) - 1, 0);

            if (1 == g_Language)
            {
                send(sock, main_menu_eng, sizeof(main_menu_eng) - 1, 0);
            }
            else
            {
                send(sock, main_menu, sizeof(main_menu) - 1, 0);
            }
        }
        else if ((!strcmp(TelnetClients[cpos].cmd, COMMOND_HELP))
                 || (!strcmp(TelnetClients[cpos].cmd, "h"))
                 || (!strcmp(TelnetClients[cpos].cmd, "?")))
        {
            char rbuf[] = "\r";
            send(sock, rbuf, sizeof(rbuf) - 1, 0);

            if (1 == g_Language)
            {
                send(sock, diagnose_menu_eng, sizeof(diagnose_menu_eng) - 1, 0);
            }
            else
            {
                send(sock, diagnose_menu, sizeof(diagnose_menu) - 1, 0);
            }
        }
        else if (TelnetClients[cpos].cmd[0] == '\0')
        {
            char rbuf[] = "\r$";
            send(sock, rbuf, sizeof(rbuf) - 1, 0);
        }
        else
        {
            if (1 == g_Language)
            {
                char rbuf[] = "\rThe command you entered is not correct!\r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
                send(sock, diagnose_menu_eng, sizeof(diagnose_menu_eng) - 1, 0);
            }
            else
            {
                char rbuf[] = "\r你输入的命令不正确!\r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
                send(sock, diagnose_menu, sizeof(diagnose_menu) - 1, 0);
            }
        }

        memset(TelnetClients[cpos].cmd, 0, sizeof(TelnetClients[cpos].cmd));
    }
    else
    {
        if (1 == len && buf[0] == '\b')
        {
            int len = strlen(TelnetClients[cpos].cmd);
            TelnetClients[cpos].cmd[len - 1] = '\0';
        }
        else
        {
            strncat(TelnetClients[cpos].cmd, (char*)buf, len);
        }
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : TelnetReleaseRecordTaskStateProc
 功能描述  : 删除录像任务状态下的处理
 输入参数  : int cpos
                            unsigned char* buf
                            int len
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年7月31日 星期三
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int TelnetReleaseRecordTaskStateProc(int cpos, unsigned char* buf, int len)
{
    int sock;

    if (cpos < 0 || cpos >= MAX_TELNETCLIENTS)
    {
        return -1;
    }

    if (buf == NULL || len <= 0)
    {
        return -1;
    }

    sock = TelnetClients[cpos].sock;

    if (sock == -1)
    {
        return -1;
    }

    /* echo first */
    send(sock, buf, len, 0);

    if ((len == 2 && !strncmp((char*)buf, "\r\n", len))
        || (len == 1 && !strncmp((char*)buf, "\r", len)))
    {
        /* parse command */
        if (TelnetClients[cpos].cmd[0] != '\0')
        {
            if (!strcmp(TelnetClients[cpos].cmd, "all"))
            {
                if (1 == g_Language)
                {
                    char rbuf[] = "\rAre you sure you want to release all the record video tasks ?('y' or 'n'):\r\n$";
                    send(sock, rbuf, sizeof(rbuf) - 1, 0);
                }
                else
                {
                    char rbuf[] = "\r你确定要释放所有的录像任务吗 ?('y' or 'n'):\r\n$";
                    send(sock, rbuf, sizeof(rbuf) - 1, 0);
                }

                TelnetClients[cpos].state = TSCS_SYS_RELEASE_RECORD_TASK_CONFIRM;
                strncpy(TelnetClients[cpos].cmd_para, TelnetClients[cpos].cmd, sizeof(TelnetClients[cpos].cmd));
            }
            else if ((!strcmp(TelnetClients[cpos].cmd, COMMOND_QUIT))
                     || (!strcmp(TelnetClients[cpos].cmd, "q")))
            {
                char rbuf[] = "\r";
                TelnetClients[cpos].state = TSCS_SYS_DIAGONSE;
                send(sock, rbuf, sizeof(rbuf) - 1, 0);

                if (1 == g_Language)
                {
                    send(sock, diagnose_menu_eng, sizeof(diagnose_menu_eng) - 1, 0);
                }
                else
                {
                    send(sock, diagnose_menu, sizeof(diagnose_menu) - 1, 0);
                }
            }
            else
            {
                int record_cr_index = osip_atoi(TelnetClients[cpos].cmd);

                if (record_cr_index >= 0)
                {
                    if (1 == g_Language)
                    {
                        char rbuf[] = "\rAre you sure you want to release the video ?('y' or 'n'):\r\n$";
                        send(sock, rbuf, sizeof(rbuf) - 1, 0);
                    }
                    else
                    {
                        char rbuf[] = "\r你确定要释放该条录像任务吗 ?('y' or 'n'):\r\n$";
                        send(sock, rbuf, sizeof(rbuf) - 1, 0);
                    }

                    TelnetClients[cpos].state = TSCS_SYS_RELEASE_RECORD_TASK_CONFIRM;
                    strncpy(TelnetClients[cpos].cmd_para, TelnetClients[cpos].cmd, sizeof(TelnetClients[cpos].cmd));
                }
                else
                {
                    if (1 == g_Language)
                    {
                        char rbuf[] = "\rYour input parameters are incorrect, please re-enter!\r\n$";
                        send(sock, rbuf, sizeof(rbuf) - 1, 0);
                    }
                    else
                    {
                        char rbuf[] = "\r你输入的参数有误，请重新输入!\r\n$";
                        send(sock, rbuf, sizeof(rbuf) - 1, 0);
                    }

                    TelnetClients[cpos].state = TSCS_SYS_RELEASE_RECORD_TASK;
                    char rbuf1[] = "\r$";
                    send(sock, rbuf1, sizeof(rbuf1) - 1, 0);
                }
            }
        }
        else
        {
            if (1 == g_Language)
            {
                char rbuf[] = "\rEnter the video tasks you want to release, enter the Enter key to confirm the input, and enter the all to release all of the video tasks\r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }
            else
            {
                char rbuf[] = "\r输入你要释放的录像任务索引，回车键确认输入，输入all则表示释放所有的录像任务\r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }

            TelnetClients[cpos].state = TSCS_SYS_RELEASE_RECORD_TASK;
            char rbuf1[] = "\r$";
            send(sock, rbuf1, sizeof(rbuf1) - 1, 0);
        }

        memset(TelnetClients[cpos].cmd, 0, sizeof(TelnetClients[cpos].cmd));
    }
    else
    {
        if (1 == len && buf[0] == '\b')
        {
            int len = strlen(TelnetClients[cpos].cmd);
            TelnetClients[cpos].cmd[len - 1] = '\0';
        }
        else
        {
            strncat(TelnetClients[cpos].cmd, (char*)buf, len);
        }
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : TelnetReleaseRecordTaskConfirmStateProc
 功能描述  : 释放所有录像任务确认状态下的处理
 输入参数  : int cpos
                            unsigned char* buf
                            int len
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年7月30日 星期二
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int TelnetReleaseRecordTaskConfirmStateProc(int cpos, unsigned char* buf, int len)
{
    int sock;

    if (cpos < 0 || cpos >= MAX_TELNETCLIENTS)
    {
        return -1;
    }

    if (buf == NULL || len <= 0)
    {
        return -1;
    }

    sock = TelnetClients[cpos].sock;

    if (sock == -1)
    {
        return -1;
    }

    /* echo first */
    send(sock, buf, len, 0);

    if ((len == 2 && !strncmp((char*)buf, "\r\n", len))
        || (len == 1 && !strncmp((char*)buf, "\r", len)))
    {
        /* parse command */
        if (!strcmp(TelnetClients[cpos].cmd, "y"))
        {
            if (!strcmp(TelnetClients[cpos].cmd_para, "all"))
            {
                if (1 == g_Language)
                {
                    char rbuf[] = "\rCMS system is being released all the video tasks...\r\n";
                    send(sock, rbuf, sizeof(rbuf) - 1, 0);
                }
                else
                {
                    char rbuf[] = "\rCMS系统正在释放所有的录像任务...\r\n";
                    send(sock, rbuf, sizeof(rbuf) - 1, 0);
                }

                StopAllRecordTask(sock);

                if (1 == g_Language)
                {
                    char rbuf1[] = "\rCMS system successfully released all of the video tasks!\r\n$";
                    send(sock, rbuf1, sizeof(rbuf1) - 1, 0);
                }
                else
                {
                    char rbuf1[] = "\rCMS系统成功释放所有的录像任务!\r\n$";
                    send(sock, rbuf1, sizeof(rbuf1) - 1, 0);
                }

                TelnetClients[cpos].state = TSCS_SYS_DIAGONSE;
            }
            else
            {
                int record_cr_index = osip_atoi(TelnetClients[cpos].cmd_para);

                if (record_cr_index >= 0)
                {
                    StopRecordTask(sock, record_cr_index);
                    TelnetClients[cpos].state = TSCS_SYS_DIAGONSE;
                }
            }
        }
        else if (!strcmp(TelnetClients[cpos].cmd, "n"))
        {
            if (1 == g_Language)
            {
                char rbuf[] = "\rCancel release record task!\r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }
            else
            {
                char rbuf[] = "\r取消释放录像任务!\r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }

            TelnetClients[cpos].state = TSCS_SYS_DIAGONSE;
        }
        else if ((!strcmp(TelnetClients[cpos].cmd, COMMOND_QUIT))
                 || (!strcmp(TelnetClients[cpos].cmd, "q")))
        {
            char rbuf[] = "\r";
            TelnetClients[cpos].state = TSCS_SYS_DIAGONSE;
            send(sock, rbuf, sizeof(rbuf) - 1, 0);

            if (1 == g_Language)
            {
                send(sock, diagnose_menu_eng, sizeof(diagnose_menu_eng) - 1, 0);
            }
            else
            {
                send(sock, diagnose_menu, sizeof(diagnose_menu) - 1, 0);
            }
        }
        else
        {
            if (1 == g_Language)
            {
                char rbuf[] = "\rPlease enter y or n \r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }
            else
            {
                char rbuf[] = "\r请输入y或者n \r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }
        }

        memset(TelnetClients[cpos].cmd_para, 0, sizeof(TelnetClients[cpos].cmd_para));
        memset(TelnetClients[cpos].cmd, 0, sizeof(TelnetClients[cpos].cmd));
    }
    else
    {
        if (1 == len && buf[0] == '\b')
        {
            int len = strlen(TelnetClients[cpos].cmd);
            TelnetClients[cpos].cmd[len - 1] = '\0';
        }
        else
        {
            strncat(TelnetClients[cpos].cmd, (char*)buf, len);
        }
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : TelnetReleaseCallTaskStateProc
 功能描述  : 删除呼叫任务状态下的处理
 输入参数  : int cpos
                            unsigned char* buf
                            int len
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年7月31日 星期三
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int TelnetReleaseCallTaskStateProc(int cpos, unsigned char* buf, int len)
{
    int sock;

    if (cpos < 0 || cpos >= MAX_TELNETCLIENTS)
    {
        return -1;
    }

    if (buf == NULL || len <= 0)
    {
        return -1;
    }

    sock = TelnetClients[cpos].sock;

    if (sock == -1)
    {
        return -1;
    }

    /* echo first */
    send(sock, buf, len, 0);

    if ((len == 2 && !strncmp((char*)buf, "\r\n", len))
        || (len == 1 && !strncmp((char*)buf, "\r", len)))
    {
        /* parse command */
        if (TelnetClients[cpos].cmd[0] != '\0')
        {
            if (!strcmp(TelnetClients[cpos].cmd, "all"))
            {
                if (1 == g_Language)
                {
                    char rbuf[] = "\rAre you sure you want to release all of the call tasks ?('y' or 'n'):\r\n$";
                    send(sock, rbuf, sizeof(rbuf) - 1, 0);
                }
                else
                {
                    char rbuf[] = "\r你确定要释放所有的呼叫任务吗 ?('y' or 'n'):\r\n$";
                    send(sock, rbuf, sizeof(rbuf) - 1, 0);
                }

                TelnetClients[cpos].state = TSCS_SYS_RELEASE_CALL_TASK_CONFIRM;
                strncpy(TelnetClients[cpos].cmd_para, TelnetClients[cpos].cmd, sizeof(TelnetClients[cpos].cmd));
            }
            else if ((!strcmp(TelnetClients[cpos].cmd, COMMOND_QUIT))
                     || (!strcmp(TelnetClients[cpos].cmd, "q")))
            {
                char rbuf[] = "\r";
                TelnetClients[cpos].state = TSCS_SYS_DIAGONSE;
                send(sock, rbuf, sizeof(rbuf) - 1, 0);

                if (1 == g_Language)
                {
                    send(sock, diagnose_menu_eng, sizeof(diagnose_menu_eng) - 1, 0);
                }
                else
                {
                    send(sock, diagnose_menu, sizeof(diagnose_menu) - 1, 0);
                }
            }
            else
            {
                int call_index = osip_atoi(TelnetClients[cpos].cmd);

                if (call_index >= 0)
                {
                    if (1 == g_Language)
                    {
                        char rbuf[] = "\rAre you sure you want to release the call task ?('y' or 'n'):\r\n$";
                        send(sock, rbuf, sizeof(rbuf) - 1, 0);
                    }
                    else
                    {
                        char rbuf[] = "\r你确定要释放该条呼叫任务吗 ?('y' or 'n'):\r\n$";
                        send(sock, rbuf, sizeof(rbuf) - 1, 0);
                    }

                    TelnetClients[cpos].state = TSCS_SYS_RELEASE_CALL_TASK_CONFIRM;
                    strncpy(TelnetClients[cpos].cmd_para, TelnetClients[cpos].cmd, sizeof(TelnetClients[cpos].cmd));
                }
                else
                {
                    if (1 == g_Language)
                    {
                        char rbuf[] = "\rYour input parameters are incorrect, please re-enter!\r\n$";
                        send(sock, rbuf, sizeof(rbuf) - 1, 0);
                    }
                    else
                    {
                        char rbuf[] = "\r你输入的参数有误，请重新输入!\r\n$";
                        send(sock, rbuf, sizeof(rbuf) - 1, 0);
                    }

                    TelnetClients[cpos].state = TSCS_SYS_RELEASE_CALL_TASK;
                    char rbuf1[] = "\r$";
                    send(sock, rbuf1, sizeof(rbuf1) - 1, 0);
                }
            }
        }
        else
        {
            if (1 == g_Language)
            {
                char rbuf[] = "\rEnter the call task index that you want to release, enter the Enter key to confirm the input, and enter the all to release all call tasks\r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }
            else
            {
                char rbuf[] = "\r输入你要释放的呼叫任务索引，回车键确认输入，输入all则表示释放所有的呼叫任务\r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }

            TelnetClients[cpos].state = TSCS_SYS_RELEASE_CALL_TASK;
            char rbuf1[] = "\r$";
            send(sock, rbuf1, sizeof(rbuf1) - 1, 0);
        }

        memset(TelnetClients[cpos].cmd, 0, sizeof(TelnetClients[cpos].cmd));
    }
    else
    {
        if (1 == len && buf[0] == '\b')
        {
            int len = strlen(TelnetClients[cpos].cmd);
            TelnetClients[cpos].cmd[len - 1] = '\0';
        }
        else
        {
            strncat(TelnetClients[cpos].cmd, (char*)buf, len);
        }
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : TelnetReleaseCallTaskConfirmStateProc
 功能描述  : 释放呼叫任务确认状态下的处理
 输入参数  : int cpos
                            unsigned char* buf
                            int len
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年7月30日 星期二
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int TelnetReleaseCallTaskConfirmStateProc(int cpos, unsigned char* buf, int len)
{
    int sock;

    if (cpos < 0 || cpos >= MAX_TELNETCLIENTS)
    {
        return -1;
    }

    if (buf == NULL || len <= 0)
    {
        return -1;
    }

    sock = TelnetClients[cpos].sock;

    if (sock == -1)
    {
        return -1;
    }

    /* echo first */
    send(sock, buf, len, 0);

    if ((len == 2 && !strncmp((char*)buf, "\r\n", len))
        || (len == 1 && !strncmp((char*)buf, "\r", len)))
    {
        /* parse command */
        if (!strcmp(TelnetClients[cpos].cmd, "y"))
        {
            if (!strcmp(TelnetClients[cpos].cmd_para, "all"))
            {
                if (1 == g_Language)
                {
                    char rbuf[] = "\rCMS system is releasing all the call tasks...\r\n";
                    send(sock, rbuf, sizeof(rbuf) - 1, 0);
                }
                else
                {
                    char rbuf[] = "\rCMS系统正在释放所有的呼叫任务...\r\n";
                    send(sock, rbuf, sizeof(rbuf) - 1, 0);
                }

                StopAllServiceTask(sock);

                if (1 == g_Language)
                {
                    char rbuf1[] = "\rCMS system successfully releases all of the call tasks!\r\n$";
                    send(sock, rbuf1, sizeof(rbuf1) - 1, 0);
                }
                else
                {
                    char rbuf1[] = "\rCMS系统成功释放所有的呼叫任务!\r\n$";
                    send(sock, rbuf1, sizeof(rbuf1) - 1, 0);
                }

                TelnetClients[cpos].state = TSCS_SYS_DIAGONSE;
            }
            else
            {
                int call_index = osip_atoi(TelnetClients[cpos].cmd_para);

                if (call_index >= 0)
                {
                    StopCallTask(sock, call_index);
                    TelnetClients[cpos].state = TSCS_SYS_DIAGONSE;
                }
            }
        }
        else if (!strcmp(TelnetClients[cpos].cmd, "n"))
        {
            if (1 == g_Language)
            {
                char rbuf[] = "\rCancel release call task!\r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }
            else
            {
                char rbuf[] = "\r取消释放呼叫任务!\r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }

            TelnetClients[cpos].state = TSCS_SYS_DIAGONSE;
        }
        else if ((!strcmp(TelnetClients[cpos].cmd, COMMOND_QUIT))
                 || (!strcmp(TelnetClients[cpos].cmd, "q")))
        {
            char rbuf[] = "\r";
            TelnetClients[cpos].state = TSCS_SYS_DIAGONSE;
            send(sock, rbuf, sizeof(rbuf) - 1, 0);

            if (1 == g_Language)
            {
                send(sock, diagnose_menu_eng, sizeof(diagnose_menu_eng) - 1, 0);
            }
            else
            {
                send(sock, diagnose_menu, sizeof(diagnose_menu) - 1, 0);
            }
        }
        else
        {
            if (1 == g_Language)
            {
                char rbuf[] = "\rPlease enter y or n \r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }
            else
            {
                char rbuf[] = "\r请输入y或者n \r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }
        }

        memset(TelnetClients[cpos].cmd_para, 0, sizeof(TelnetClients[cpos].cmd_para));
        memset(TelnetClients[cpos].cmd, 0, sizeof(TelnetClients[cpos].cmd));
    }
    else
    {
        if (1 == len && buf[0] == '\b')
        {
            int len = strlen(TelnetClients[cpos].cmd);
            TelnetClients[cpos].cmd[len - 1] = '\0';
        }
        else
        {
            strncat(TelnetClients[cpos].cmd, (char*)buf, len);
        }
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : TelnetStopPollTaskStateProc
 功能描述  : 停止轮巡任务状态下的处理
 输入参数  : int cpos
             unsigned char* buf
             int len
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年8月7日 星期四
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int TelnetStopPollTaskStateProc(int cpos, unsigned char* buf, int len)
{
    int sock;

    if (cpos < 0 || cpos >= MAX_TELNETCLIENTS)
    {
        return -1;
    }

    if (buf == NULL || len <= 0)
    {
        return -1;
    }

    sock = TelnetClients[cpos].sock;

    if (sock == -1)
    {
        return -1;
    }

    /* echo first */
    send(sock, buf, len, 0);

    if ((len == 2 && !strncmp((char*)buf, "\r\n", len))
        || (len == 1 && !strncmp((char*)buf, "\r", len)))
    {
        /* parse command */
        if (TelnetClients[cpos].cmd[0] != '\0')
        {
            if (!strcmp(TelnetClients[cpos].cmd, "all"))
            {
                if (1 == g_Language)
                {
                    char rbuf[] = "\rAre you sure you want to stop all poll task ?('y' or 'n'):\r\n$";
                    send(sock, rbuf, sizeof(rbuf) - 1, 0);
                }
                else
                {
                    char rbuf[] = "\r你确定要停止所有的轮巡任务吗 ?('y' or 'n'):\r\n$";
                    send(sock, rbuf, sizeof(rbuf) - 1, 0);
                }

                TelnetClients[cpos].state = TSCS_SYS_STOP_POLL_TASK_CONFIRM;
                strncpy(TelnetClients[cpos].cmd_para, TelnetClients[cpos].cmd, sizeof(TelnetClients[cpos].cmd));
            }
            else if ((!strcmp(TelnetClients[cpos].cmd, COMMOND_QUIT))
                     || (!strcmp(TelnetClients[cpos].cmd, "q")))
            {
                char rbuf[] = "\r";
                TelnetClients[cpos].state = TSCS_SYS_DIAGONSE;
                send(sock, rbuf, sizeof(rbuf) - 1, 0);

                if (1 == g_Language)
                {
                    send(sock, diagnose_menu_eng, sizeof(diagnose_menu_eng) - 1, 0);
                }
                else
                {
                    send(sock, diagnose_menu, sizeof(diagnose_menu) - 1, 0);
                }
            }
            else
            {
                int call_index = osip_atoi(TelnetClients[cpos].cmd);

                if (call_index >= 0)
                {
                    if (1 == g_Language)
                    {
                        char rbuf[] = "\rAre you sure you want to stop the poll task?('y' or 'n'):\r\n$";
                        send(sock, rbuf, sizeof(rbuf) - 1, 0);
                    }
                    else
                    {
                        char rbuf[] = "\r你确定要停止该轮巡任务吗 ?('y' or 'n'):\r\n$";
                        send(sock, rbuf, sizeof(rbuf) - 1, 0);
                    }

                    TelnetClients[cpos].state = TSCS_SYS_STOP_POLL_TASK_CONFIRM;
                    strncpy(TelnetClients[cpos].cmd_para, TelnetClients[cpos].cmd, sizeof(TelnetClients[cpos].cmd));
                }
                else
                {
                    if (1 == g_Language)
                    {
                        char rbuf[] = "\rYour input parameters are incorrect, please re-enter!\r\n$";
                        send(sock, rbuf, sizeof(rbuf) - 1, 0);
                    }
                    else
                    {
                        char rbuf[] = "\r你输入的参数有误，请重新输入!\r\n$";
                        send(sock, rbuf, sizeof(rbuf) - 1, 0);
                    }

                    TelnetClients[cpos].state = TSCS_SYS_STOP_POLL_TASK;
                    char rbuf1[] = "\r$";
                    send(sock, rbuf1, sizeof(rbuf1) - 1, 0);
                }
            }
        }
        else
        {
            if (1 == g_Language)
            {
                char rbuf[] = "\rEnter you want to stop the wheel of the ID, enter the key to confirm the input, the input all is said to stop all of the round patrol task\r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }
            else
            {
                char rbuf[] = "\r输入你要停止的轮巡ID，回车键确认输入，输入all则表示停止所有的轮巡任务\r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }

            TelnetClients[cpos].state = TSCS_SYS_STOP_POLL_TASK;
            char rbuf1[] = "\r$";
            send(sock, rbuf1, sizeof(rbuf1) - 1, 0);
        }

        memset(TelnetClients[cpos].cmd, 0, sizeof(TelnetClients[cpos].cmd));
    }
    else
    {
        if (1 == len && buf[0] == '\b')
        {
            int len = strlen(TelnetClients[cpos].cmd);
            TelnetClients[cpos].cmd[len - 1] = '\0';
        }
        else
        {
            strncat(TelnetClients[cpos].cmd, (char*)buf, len);
        }
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : TelnetStopPollTaskConfirmStateProc
 功能描述  : 停止轮巡任务确认状态下的处理
 输入参数  : int cpos
             unsigned char* buf
             int len
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年8月7日 星期四
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int TelnetStopPollTaskConfirmStateProc(int cpos, unsigned char* buf, int len)
{
    int sock;

    if (cpos < 0 || cpos >= MAX_TELNETCLIENTS)
    {
        return -1;
    }

    if (buf == NULL || len <= 0)
    {
        return -1;
    }

    sock = TelnetClients[cpos].sock;

    if (sock == -1)
    {
        return -1;
    }

    /* echo first */
    send(sock, buf, len, 0);

    if ((len == 2 && !strncmp((char*)buf, "\r\n", len))
        || (len == 1 && !strncmp((char*)buf, "\r", len)))
    {
        /* parse command */
        if (!strcmp(TelnetClients[cpos].cmd, "y"))
        {
            if (!strcmp(TelnetClients[cpos].cmd_para, "all"))
            {
                if (1 == g_Language)
                {
                    char rbuf[] = "\rThe CMS system is stopping all poll task....\r\n";
                    send(sock, rbuf, sizeof(rbuf) - 1, 0);
                }
                else
                {
                    char rbuf[] = "\rCMS系统正在停止所有的轮巡任务...\r\n";
                    send(sock, rbuf, sizeof(rbuf) - 1, 0);
                }

                StopAllPollTask(sock);

                if (1 == g_Language)
                {
                    char rbuf1[] = "\rCMS system successfully stopped all poll task!\r\n$";
                    send(sock, rbuf1, sizeof(rbuf1) - 1, 0);
                }
                else
                {
                    char rbuf1[] = "\rCMS系统成功停止所有的轮巡任务!\r\n$";
                    send(sock, rbuf1, sizeof(rbuf1) - 1, 0);
                }

                TelnetClients[cpos].state = TSCS_SYS_DIAGONSE;
            }
            else
            {
                int poll_id = osip_atoi(TelnetClients[cpos].cmd_para);

                if (poll_id >= 0)
                {
                    StopPollTask(sock, poll_id);
                    TelnetClients[cpos].state = TSCS_SYS_DIAGONSE;
                }
            }
        }
        else if (!strcmp(TelnetClients[cpos].cmd, "n"))
        {
            if (1 == g_Language)
            {
                char rbuf[] = "\rCancel stop poll task!\r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }
            else
            {
                char rbuf[] = "\r取消停止轮巡任务!\r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }

            TelnetClients[cpos].state = TSCS_SYS_DIAGONSE;
        }
        else if ((!strcmp(TelnetClients[cpos].cmd, COMMOND_QUIT))
                 || (!strcmp(TelnetClients[cpos].cmd, "q")))
        {
            char rbuf[] = "\r";
            TelnetClients[cpos].state = TSCS_SYS_DIAGONSE;
            send(sock, rbuf, sizeof(rbuf) - 1, 0);

            if (1 == g_Language)
            {
                send(sock, diagnose_menu_eng, sizeof(diagnose_menu_eng) - 1, 0);
            }
            else
            {
                send(sock, diagnose_menu, sizeof(diagnose_menu) - 1, 0);
            }
        }
        else
        {
            if (1 == g_Language)
            {
                char rbuf[] = "\rPlease enter y or n \r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }
            else
            {
                char rbuf[] = "\r请输入y或者n \r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }
        }

        memset(TelnetClients[cpos].cmd_para, 0, sizeof(TelnetClients[cpos].cmd_para));
        memset(TelnetClients[cpos].cmd, 0, sizeof(TelnetClients[cpos].cmd));
    }
    else
    {
        if (1 == len && buf[0] == '\b')
        {
            int len = strlen(TelnetClients[cpos].cmd);
            TelnetClients[cpos].cmd[len - 1] = '\0';
        }
        else
        {
            strncat(TelnetClients[cpos].cmd, (char*)buf, len);
        }
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : TelnetStopCruiseTaskStateProc
 功能描述  : 停止巡航任务状态下的处理
 输入参数  : int cpos
             unsigned char* buf
             int len
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年8月7日 星期四
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int TelnetStopCruiseTaskStateProc(int cpos, unsigned char* buf, int len)
{
    int sock;

    if (cpos < 0 || cpos >= MAX_TELNETCLIENTS)
    {
        return -1;
    }

    if (buf == NULL || len <= 0)
    {
        return -1;
    }

    sock = TelnetClients[cpos].sock;

    if (sock == -1)
    {
        return -1;
    }

    /* echo first */
    send(sock, buf, len, 0);

    if ((len == 2 && !strncmp((char*)buf, "\r\n", len))
        || (len == 1 && !strncmp((char*)buf, "\r", len)))
    {
        /* parse command */
        if (TelnetClients[cpos].cmd[0] != '\0')
        {
            if (!strcmp(TelnetClients[cpos].cmd, "all"))
            {
                if (1 == g_Language)
                {
                    char rbuf[] = "\rAre you sure you want to stop all of the cruise tasks ?('y' or 'n'):\r\n$";
                    send(sock, rbuf, sizeof(rbuf) - 1, 0);
                }
                else
                {
                    char rbuf[] = "\r你确定要停止所有的巡航任务吗 ?('y' or 'n'):\r\n$";
                    send(sock, rbuf, sizeof(rbuf) - 1, 0);
                }

                TelnetClients[cpos].state = TSCS_SYS_STOP_CRUISE_TASK_CONFIRM;
                strncpy(TelnetClients[cpos].cmd_para, TelnetClients[cpos].cmd, sizeof(TelnetClients[cpos].cmd));
            }
            else if ((!strcmp(TelnetClients[cpos].cmd, COMMOND_QUIT))
                     || (!strcmp(TelnetClients[cpos].cmd, "q")))
            {
                char rbuf[] = "\r";
                TelnetClients[cpos].state = TSCS_SYS_DIAGONSE;
                send(sock, rbuf, sizeof(rbuf) - 1, 0);

                if (1 == g_Language)
                {
                    send(sock, diagnose_menu_eng, sizeof(diagnose_menu_eng) - 1, 0);
                }
                else
                {
                    send(sock, diagnose_menu, sizeof(diagnose_menu) - 1, 0);
                }
            }
            else
            {
                int call_index = osip_atoi(TelnetClients[cpos].cmd);

                if (call_index >= 0)
                {
                    if (1 == g_Language)
                    {
                        char rbuf[] = "\rAre you sure you want to stop the cruise ?('y' or 'n'):\r\n$";
                        send(sock, rbuf, sizeof(rbuf) - 1, 0);
                    }
                    else
                    {
                        char rbuf[] = "\r你确定要停止该巡航任务吗 ?('y' or 'n'):\r\n$";
                        send(sock, rbuf, sizeof(rbuf) - 1, 0);
                    }

                    TelnetClients[cpos].state = TSCS_SYS_STOP_CRUISE_TASK_CONFIRM;
                    strncpy(TelnetClients[cpos].cmd_para, TelnetClients[cpos].cmd, sizeof(TelnetClients[cpos].cmd));
                }
                else
                {
                    if (1 == g_Language)
                    {
                        char rbuf[] = "\rYour input parameters are incorrect, please re-enter!\r\n$";
                        send(sock, rbuf, sizeof(rbuf) - 1, 0);
                    }
                    else
                    {
                        char rbuf[] = "\r你输入的参数有误，请重新输入!\r\n$";
                        send(sock, rbuf, sizeof(rbuf) - 1, 0);
                    }

                    TelnetClients[cpos].state = TSCS_SYS_STOP_CRUISE_TASK;
                    char rbuf1[] = "\r$";
                    send(sock, rbuf1, sizeof(rbuf1) - 1, 0);
                }
            }
        }
        else
        {
            if (1 == g_Language)
            {
                char rbuf[] = "\rEnter you want to stop the cruise ID, enter the key to confirm the input, the input all is said to stop all the cruise\r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }
            else
            {
                char rbuf[] = "\r输入你要停止的巡航ID，回车键确认输入，输入all则表示停止所有的巡航任务\r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }

            TelnetClients[cpos].state = TSCS_SYS_STOP_CRUISE_TASK;
            char rbuf1[] = "\r$";
            send(sock, rbuf1, sizeof(rbuf1) - 1, 0);
        }

        memset(TelnetClients[cpos].cmd, 0, sizeof(TelnetClients[cpos].cmd));
    }
    else
    {
        if (1 == len && buf[0] == '\b')
        {
            int len = strlen(TelnetClients[cpos].cmd);
            TelnetClients[cpos].cmd[len - 1] = '\0';
        }
        else
        {
            strncat(TelnetClients[cpos].cmd, (char*)buf, len);
        }
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : TelnetStopCruiseTaskConfirmStateProc
 功能描述  : 停止巡航任务确认状态下的处理
 输入参数  : int cpos
             unsigned char* buf
             int len
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年8月7日 星期四
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int TelnetStopCruiseTaskConfirmStateProc(int cpos, unsigned char* buf, int len)
{
    int sock;

    if (cpos < 0 || cpos >= MAX_TELNETCLIENTS)
    {
        return -1;
    }

    if (buf == NULL || len <= 0)
    {
        return -1;
    }

    sock = TelnetClients[cpos].sock;

    if (sock == -1)
    {
        return -1;
    }

    /* echo first */
    send(sock, buf, len, 0);

    if ((len == 2 && !strncmp((char*)buf, "\r\n", len))
        || (len == 1 && !strncmp((char*)buf, "\r", len)))
    {
        /* parse command */
        if (!strcmp(TelnetClients[cpos].cmd, "y"))
        {
            if (!strcmp(TelnetClients[cpos].cmd_para, "all"))
            {
                if (1 == g_Language)
                {
                    char rbuf[] = "\rCMS system is stopping all of the cruise tasks...\r\n";
                    send(sock, rbuf, sizeof(rbuf) - 1, 0);
                }
                else
                {
                    char rbuf[] = "\rCMS系统正在停止所有的巡航任务...\r\n";
                    send(sock, rbuf, sizeof(rbuf) - 1, 0);
                }

                StopAllCruiseTask(sock);

                if (1 == g_Language)
                {
                    char rbuf1[] = "\rCMS system successfully stopped all of the cruise tasks!\r\n$";
                    send(sock, rbuf1, sizeof(rbuf1) - 1, 0);
                }
                else
                {
                    char rbuf1[] = "\rCMS系统成功停止所有的巡航任务!\r\n$";
                    send(sock, rbuf1, sizeof(rbuf1) - 1, 0);
                }

                TelnetClients[cpos].state = TSCS_SYS_DIAGONSE;
            }
            else
            {
                int poll_id = osip_atoi(TelnetClients[cpos].cmd_para);

                if (poll_id >= 0)
                {
                    StopCruiseTask(sock, poll_id);
                    TelnetClients[cpos].state = TSCS_SYS_DIAGONSE;
                }
            }
        }
        else if (!strcmp(TelnetClients[cpos].cmd, "n"))
        {
            if (1 == g_Language)
            {
                char rbuf[] = "\rCancel stop cruise task!\r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }
            else
            {
                char rbuf[] = "\r取消停止巡航任务!\r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }

            TelnetClients[cpos].state = TSCS_SYS_DIAGONSE;
        }
        else if ((!strcmp(TelnetClients[cpos].cmd, COMMOND_QUIT))
                 || (!strcmp(TelnetClients[cpos].cmd, "q")))
        {
            char rbuf[] = "\r";
            TelnetClients[cpos].state = TSCS_SYS_DIAGONSE;
            send(sock, rbuf, sizeof(rbuf) - 1, 0);

            if (1 == g_Language)
            {
                send(sock, diagnose_menu_eng, sizeof(diagnose_menu_eng) - 1, 0);
            }
            else
            {
                send(sock, diagnose_menu, sizeof(diagnose_menu) - 1, 0);
            }
        }
        else
        {
            if (1 == g_Language)
            {
                char rbuf[] = "\rPlease enter y or n \r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }
            else
            {
                char rbuf[] = "\r请输入y或者n \r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }
        }

        memset(TelnetClients[cpos].cmd_para, 0, sizeof(TelnetClients[cpos].cmd_para));
        memset(TelnetClients[cpos].cmd, 0, sizeof(TelnetClients[cpos].cmd));
    }
    else
    {
        if (1 == len && buf[0] == '\b')
        {
            int len = strlen(TelnetClients[cpos].cmd);
            TelnetClients[cpos].cmd[len - 1] = '\0';
        }
        else
        {
            strncat(TelnetClients[cpos].cmd, (char*)buf, len);
        }
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : TelnetReleaseAllUADialogInfoConfirmStateProc
 功能描述  : 释放所有SIPUA会话信息确认状态下的处理
 输入参数  : int cpos
             unsigned char* buf
             int len
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年7月30日 星期二
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int TelnetReleaseAllUADialogInfoConfirmStateProc(int cpos, unsigned char* buf, int len)
{
    int sock;

    if (cpos < 0 || cpos >= MAX_TELNETCLIENTS)
    {
        return -1;
    }

    if (buf == NULL || len <= 0)
    {
        return -1;
    }

    sock = TelnetClients[cpos].sock;

    if (sock == -1)
    {
        return -1;
    }

    /* echo first */
    send(sock, buf, len, 0);

    if ((len == 2 && !strncmp((char*)buf, "\r\n", len))
        || (len == 1 && !strncmp((char*)buf, "\r", len)))
    {
        /* parse command */
        if (!strcmp(TelnetClients[cpos].cmd, "y"))
        {
            if (1 == g_Language)
            {
                char rbuf[] = "\rCMS system is releasing all of the UA SIP session information...\r\n";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }
            else
            {
                char rbuf[] = "\rCMS系统正在释放所有的SIP UA会话信息...\r\n";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }

            SIP_ReleaseAllSIPUAInfo();

            if (1 == g_Language)
            {
                char rbuf1[] = "\rCMS system successfully releases all of the UA SIP session information!\r\n$";
                send(sock, rbuf1, sizeof(rbuf1) - 1, 0);
            }
            else
            {
                char rbuf1[] = "\rCMS系统成功释放所有的SIP UA会话信息!\r\n$";
                send(sock, rbuf1, sizeof(rbuf1) - 1, 0);
            }

            TelnetClients[cpos].state = TSCS_SYS_DIAGONSE;
        }
        else if (!strcmp(TelnetClients[cpos].cmd, "n"))
        {
            if (1 == g_Language)
            {
                char rbuf[] = "\rCancel release all UA SIP session information!\r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }
            else
            {
                char rbuf[] = "\r取消释放所有的SIP UA会话信息!\r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }

            TelnetClients[cpos].state = TSCS_SYS_DIAGONSE;
        }
        else if ((!strcmp(TelnetClients[cpos].cmd, COMMOND_QUIT))
                 || (!strcmp(TelnetClients[cpos].cmd, "q")))
        {
            char rbuf[] = "\r";
            TelnetClients[cpos].state = TSCS_SYS_DIAGONSE;
            send(sock, rbuf, sizeof(rbuf) - 1, 0);

            if (1 == g_Language)
            {
                send(sock, diagnose_menu_eng, sizeof(diagnose_menu_eng) - 1, 0);
            }
            else
            {
                send(sock, diagnose_menu, sizeof(diagnose_menu) - 1, 0);
            }
        }
        else
        {
            if (1 == g_Language)
            {
                char rbuf[] = "\rPlease enter y or n \r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }
            else
            {
                char rbuf[] = "\r请输入y或者n \r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }
        }

        memset(TelnetClients[cpos].cmd, 0, sizeof(TelnetClients[cpos].cmd));
    }
    else
    {
        if (1 == len && buf[0] == '\b')
        {
            int len = strlen(TelnetClients[cpos].cmd);
            TelnetClients[cpos].cmd[len - 1] = '\0';
        }
        else
        {
            strncat(TelnetClients[cpos].cmd, (char*)buf, len);
        }
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : TelnetReleaseAllUASInfoConfirmStateProc
 功能描述  : 释放所有客户端注册信息UAS确认状态下的处理
 输入参数  : int cpos
             unsigned char* buf
             int len
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年7月30日 星期二
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int TelnetReleaseAllUASInfoConfirmStateProc(int cpos, unsigned char* buf, int len)
{
    int sock;

    if (cpos < 0 || cpos >= MAX_TELNETCLIENTS)
    {
        return -1;
    }

    if (buf == NULL || len <= 0)
    {
        return -1;
    }

    sock = TelnetClients[cpos].sock;

    if (sock == -1)
    {
        return -1;
    }

    /* echo first */
    send(sock, buf, len, 0);

    if ((len == 2 && !strncmp((char*)buf, "\r\n", len))
        || (len == 1 && !strncmp((char*)buf, "\r", len)))
    {
        /* parse command */
        if (!strcmp(TelnetClients[cpos].cmd, "y"))
        {
            if (1 == g_Language)
            {
                char rbuf[] = "\rCMS system is releasing all of the UAS server registration information...\r\n";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }
            else
            {
                char rbuf[] = "\rCMS系统正在释放所有的UAS服务端注册信息...\r\n";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }

            SIP_ReleaseAllUASRegisterInfo();

            if (1 == g_Language)
            {
                char rbuf1[] = "\rCMS system successfully releases all of the UAS server registration information!\r\n$";
                send(sock, rbuf1, sizeof(rbuf1) - 1, 0);
            }
            else
            {
                char rbuf1[] = "\rCMS系统成功释放所有的UAS服务端注册信息!\r\n$";
                send(sock, rbuf1, sizeof(rbuf1) - 1, 0);
            }

            TelnetClients[cpos].state = TSCS_SYS_DIAGONSE;
        }
        else if (!strcmp(TelnetClients[cpos].cmd, "n"))
        {
            if (1 == g_Language)
            {
                char rbuf[] = "\rCancel release all UAS server registration information!\r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }
            else
            {
                char rbuf[] = "\r取消释放所有的UAS服务端注册信息!\r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }

            TelnetClients[cpos].state = TSCS_SYS_DIAGONSE;
        }
        else if ((!strcmp(TelnetClients[cpos].cmd, COMMOND_QUIT))
                 || (!strcmp(TelnetClients[cpos].cmd, "q")))
        {
            char rbuf[] = "\r";
            TelnetClients[cpos].state = TSCS_SYS_DIAGONSE;
            send(sock, rbuf, sizeof(rbuf) - 1, 0);

            if (1 == g_Language)
            {
                send(sock, diagnose_menu_eng, sizeof(diagnose_menu_eng) - 1, 0);
            }
            else
            {
                send(sock, diagnose_menu, sizeof(diagnose_menu) - 1, 0);
            }
        }
        else
        {
            if (1 == g_Language)
            {
                char rbuf[] = "\rPlease enter y or n \r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }
            else
            {
                char rbuf[] = "\r请输入y或者n \r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }
        }

        memset(TelnetClients[cpos].cmd, 0, sizeof(TelnetClients[cpos].cmd));
    }
    else
    {
        if (1 == len && buf[0] == '\b')
        {
            int len = strlen(TelnetClients[cpos].cmd);
            TelnetClients[cpos].cmd[len - 1] = '\0';
        }
        else
        {
            strncat(TelnetClients[cpos].cmd, (char*)buf, len);
        }
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : TelnetReleaseAllUACInfoConfirmStateProc
 功能描述  : 释放所有客户端注册信息UAC确认状态下的处理
 输入参数  : int cpos
             unsigned char* buf
             int len
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年7月30日 星期二
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int TelnetReleaseAllUACInfoConfirmStateProc(int cpos, unsigned char* buf, int len)
{
    int sock;

    if (cpos < 0 || cpos >= MAX_TELNETCLIENTS)
    {
        return -1;
    }

    if (buf == NULL || len <= 0)
    {
        return -1;
    }

    sock = TelnetClients[cpos].sock;

    if (sock == -1)
    {
        return -1;
    }

    /* echo first */
    send(sock, buf, len, 0);

    if ((len == 2 && !strncmp((char*)buf, "\r\n", len))
        || (len == 1 && !strncmp((char*)buf, "\r", len)))
    {
        /* parse command */
        if (!strcmp(TelnetClients[cpos].cmd, "y"))
        {
            if (1 == g_Language)
            {
                char rbuf[] = "\rCMS system is releasing all UAC client registration information...\r\n";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }
            else
            {
                char rbuf[] = "\rCMS系统正在释放所有的UAC客户端注册信息...\r\n";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }

            SIP_ReleaseAllUACRegisterInfo();

            if (1 == g_Language)
            {
                char rbuf1[] = "\rCMS system successfully releases all UAC client registration information!\r\n$";
                send(sock, rbuf1, sizeof(rbuf1) - 1, 0);
            }
            else
            {
                char rbuf1[] = "\rCMS系统成功释放所有的UAC客户端注册信息!\r\n$";
                send(sock, rbuf1, sizeof(rbuf1) - 1, 0);
            }

            TelnetClients[cpos].state = TSCS_SYS_DIAGONSE;
        }
        else if (!strcmp(TelnetClients[cpos].cmd, "n"))
        {
            if (1 == g_Language)
            {
                char rbuf[] = "\rCancel release all UAC client registration information!\r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }
            else
            {
                char rbuf[] = "\r取消释放所有的UAC客户端注册信息!\r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }

            TelnetClients[cpos].state = TSCS_SYS_DIAGONSE;
        }
        else if ((!strcmp(TelnetClients[cpos].cmd, COMMOND_QUIT))
                 || (!strcmp(TelnetClients[cpos].cmd, "q")))
        {
            char rbuf[] = "\r";
            TelnetClients[cpos].state = TSCS_SYS_DIAGONSE;
            send(sock, rbuf, sizeof(rbuf) - 1, 0);

            if (1 == g_Language)
            {
                send(sock, diagnose_menu_eng, sizeof(diagnose_menu_eng) - 1, 0);
            }
            else
            {
                send(sock, diagnose_menu, sizeof(diagnose_menu) - 1, 0);
            }
        }
        else
        {
            if (1 == g_Language)
            {
                char rbuf[] = "\rPlease enter y or n \r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }
            else
            {
                char rbuf[] = "\r请输入y或者n \r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }
        }

        memset(TelnetClients[cpos].cmd, 0, sizeof(TelnetClients[cpos].cmd));
    }
    else
    {
        if (1 == len && buf[0] == '\b')
        {
            int len = strlen(TelnetClients[cpos].cmd);
            TelnetClients[cpos].cmd[len - 1] = '\0';
        }
        else
        {
            strncat(TelnetClients[cpos].cmd, (char*)buf, len);
        }
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : TelnetDebugLevelSetStateProc
 功能描述  : Debug调试等级设置状态下的处理
 输入参数  : int cpos
                            unsigned char* buf
                            int len
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年7月25日 星期四
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int TelnetDebugLevelSetStateProc(int cpos, unsigned char* buf, int len)
{
    int sock;

    if (cpos < 0 || cpos >= MAX_TELNETCLIENTS)
    {
        return -1;
    }

    if (buf == NULL || len <= 0)
    {
        return -1;
    }

    sock = TelnetClients[cpos].sock;

    if (sock == -1)
    {
        return -1;
    }

    /* echo first */
    send(sock, buf, len, 0);

    if ((len == 2 && !strncmp((char*)buf, "\r\n", len))
        || (len == 1 && !strncmp((char*)buf, "\r", len)))
    {
        /* parse command */
        if (TelnetClients[cpos].cmd[0] != '\0')
        {
            if (!strcmp(TelnetClients[cpos].cmd, "q"))
            {
                char rbuf[] = "\r";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);

                if (1 == g_Language)
                {
                    send(sock, config_menu_eng, sizeof(config_menu_eng) - 1, 0);
                }
                else
                {
                    send(sock, config_menu, sizeof(config_menu) - 1, 0);
                }

                TelnetClients[cpos].state = TSCS_CONFIG_SET;
            }
            else
            {
                int tmpvalue = osip_atoi(TelnetClients[cpos].cmd);

                if (tmpvalue < 1 || tmpvalue > 8)
                {
                    if (1 == g_Language)
                    {
                        char rbuf[] = "\rYour input parameters are incorrect, and the system will use the default value!\r\n$";
                        send(sock, rbuf, sizeof(rbuf) - 1, 0);
                    }
                    else
                    {
                        char rbuf[] = "\r你输入的参数有误，系统将采用默认值!\r\n$";
                        send(sock, rbuf, sizeof(rbuf) - 1, 0);
                    }

                    TelnetClients[cpos].state = TSCS_CONFIG_SET;

                    g_CommonDbgLevel = LOG_ERROR;
                    g_SIPStackDbgLevel = LOG_ERROR;
                    g_UserDbgLevel = LOG_ERROR;
                    g_DeviceDbgLevel = LOG_ERROR;
                    g_RouteDbgLevel = LOG_ERROR;
                    g_RecordDbgLevel = LOG_ERROR;
                    g_ResourceDbgLevel = LOG_ERROR;
                    g_CruiseDbgLevel = LOG_ERROR;
                    g_PlanDbgLevel = LOG_ERROR;
                    g_PollDbgLevel = LOG_ERROR;
                }
                else
                {
                    if (tmpvalue != g_CommonDbgLevel)
                    {
                        if (1 == g_Language)
                        {
                            char rbuf[] = "\rSuccessfully modified Debug debug level!\r\n$";
                            send(sock, rbuf, sizeof(rbuf) - 1, 0);
                        }
                        else
                        {
                            char rbuf[] = "\r成功修改Debug调试等级!\r\n$";
                            send(sock, rbuf, sizeof(rbuf) - 1, 0);
                        }

                        TelnetClients[cpos].state = TSCS_CONFIG_SET;
                        g_CommonDbgLevel = tmpvalue;
                        g_SIPStackDbgLevel = tmpvalue;
                        g_UserDbgLevel = tmpvalue;
                        g_DeviceDbgLevel = tmpvalue;
                        g_RouteDbgLevel = tmpvalue;
                        g_RecordDbgLevel = tmpvalue;
                        g_ResourceDbgLevel = tmpvalue;
                        g_CruiseDbgLevel = tmpvalue;
                        g_PlanDbgLevel = tmpvalue;
                        g_PollDbgLevel = tmpvalue;
                    }
                    else if (tmpvalue == g_CommonDbgLevel)
                    {
                        if (1 == g_Language)
                        {
                            char rbuf[] = "\rThis parameter has not been modified!\r\n$";
                            send(sock, rbuf, sizeof(rbuf) - 1, 0);
                        }
                        else
                        {
                            char rbuf[] = "\r此参数未修改!\r\n$";
                            send(sock, rbuf, sizeof(rbuf) - 1, 0);
                        }

                        TelnetClients[cpos].state = TSCS_CONFIG_SET;
                    }
                }
            }
        }
        else
        {
            if (1 == g_Language)
            {
                char rbuf[] = "\rThis parameter has not been modified!\r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }
            else
            {
                char rbuf[] = "\r此参数未修改!\r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }

            TelnetClients[cpos].state = TSCS_CONFIG_SET;
        }

        memset(TelnetClients[cpos].cmd, 0, sizeof(TelnetClients[cpos].cmd));
    }
    else
    {
        if (1 == len && buf[0] == '\b')
        {
            int len = strlen(TelnetClients[cpos].cmd);
            TelnetClients[cpos].cmd[len - 1] = '\0';
        }
        else
        {
            strncat(TelnetClients[cpos].cmd, (char*)buf, len);
        }
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : TelnetLogLevelSetStateProc
 功能描述  : 日志等级设置状态下的处理
 输入参数  : int cpos
             unsigned char* buf
             int len
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年8月11日 星期一
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int TelnetLogLevelSetStateProc(int cpos, unsigned char* buf, int len)
{
    int sock;

    if (cpos < 0 || cpos >= MAX_TELNETCLIENTS)
    {
        return -1;
    }

    if (buf == NULL || len <= 0)
    {
        return -1;
    }

    sock = TelnetClients[cpos].sock;

    if (sock == -1)
    {
        return -1;
    }

    /* echo first */
    send(sock, buf, len, 0);

    if ((len == 2 && !strncmp((char*)buf, "\r\n", len))
        || (len == 1 && !strncmp((char*)buf, "\r", len)))
    {
        /* parse command */
        if (TelnetClients[cpos].cmd[0] != '\0')
        {
            if (!strcmp(TelnetClients[cpos].cmd, "q"))
            {
                char rbuf[] = "\r";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);

                if (1 == g_Language)
                {
                    send(sock, config_menu_eng, sizeof(config_menu_eng) - 1, 0);
                }
                else
                {
                    send(sock, config_menu, sizeof(config_menu) - 1, 0);
                }

                TelnetClients[cpos].state = TSCS_CONFIG_SET;
            }
            else
            {
                int tmpvalue = osip_atoi(TelnetClients[cpos].cmd);

                if (tmpvalue < 1 || tmpvalue > 3)
                {
                    if (1 == g_Language)
                    {
                        char rbuf[] = "\rYour input parameters are incorrect, and the system will use the default value!\r\n$";
                        send(sock, rbuf, sizeof(rbuf) - 1, 0);
                    }
                    else
                    {
                        char rbuf[] = "\r你输入的参数有误，系统将采用默认值!\r\n$";
                        send(sock, rbuf, sizeof(rbuf) - 1, 0);
                    }

                    TelnetClients[cpos].state = TSCS_CONFIG_SET;
                    g_SystemLogLevel = EV9000_LOG_LEVEL_ERROR;
                }
                else
                {
                    if (tmpvalue != g_SystemLogLevel)
                    {
                        if (1 == g_Language)
                        {
                            char rbuf[] = "\rSuccessfully modify Log level!\r\n$";
                            send(sock, rbuf, sizeof(rbuf) - 1, 0);
                        }
                        else
                        {
                            char rbuf[] = "\r成功修Log日志等级!\r\n$";
                            send(sock, rbuf, sizeof(rbuf) - 1, 0);
                        }

                        TelnetClients[cpos].state = TSCS_CONFIG_SET;
                        g_SystemLogLevel = tmpvalue;
                    }
                    else if (tmpvalue == g_SystemLogLevel)
                    {
                        if (1 == g_Language)
                        {
                            char rbuf[] = "\rThis parameter has not been modified!\r\n$";
                            send(sock, rbuf, sizeof(rbuf) - 1, 0);
                        }
                        else
                        {
                            char rbuf[] = "\r此参数未修改!\r\n$";
                            send(sock, rbuf, sizeof(rbuf) - 1, 0);
                        }

                        TelnetClients[cpos].state = TSCS_CONFIG_SET;
                    }
                }
            }
        }
        else
        {
            if (1 == g_Language)
            {
                char rbuf[] = "\rThis parameter has not been modified!\r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }
            else
            {
                char rbuf[] = "\r此参数未修改!\r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }

            TelnetClients[cpos].state = TSCS_CONFIG_SET;
        }

        memset(TelnetClients[cpos].cmd, 0, sizeof(TelnetClients[cpos].cmd));
    }
    else
    {
        if (1 == len && buf[0] == '\b')
        {
            int len = strlen(TelnetClients[cpos].cmd);
            TelnetClients[cpos].cmd[len - 1] = '\0';
        }
        else
        {
            strncat(TelnetClients[cpos].cmd, (char*)buf, len);
        }
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : TelnetLog2FileSetStateProc
 功能描述  : 日志记录到文件开关设置状态下的处理
 输入参数  : int cpos
                            unsigned char* buf
                            int len
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年7月25日 星期四
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int TelnetLog2FileSetStateProc(int cpos, unsigned char* buf, int len)
{
    int sock;

    if (cpos < 0 || cpos >= MAX_TELNETCLIENTS)
    {
        return -1;
    }

    if (buf == NULL || len <= 0)
    {
        return -1;
    }

    sock = TelnetClients[cpos].sock;

    if (sock == -1)
    {
        return -1;
    }

    /* echo first */
    send(sock, buf, len, 0);

    if ((len == 2 && !strncmp((char*)buf, "\r\n", len))
        || (len == 1 && !strncmp((char*)buf, "\r", len)))
    {
        /* parse command */
        if (TelnetClients[cpos].cmd[0] != '\0')
        {
            int tmpvalue = 0;

            if (!strcmp(TelnetClients[cpos].cmd, "On"))
            {
                tmpvalue = 1;

                if (tmpvalue != g_IsLog2File)
                {
                    if (1 == g_Language)
                    {
                        char rbuf[] = "\rLog file record label changes successfully!\r\n$";
                        send(sock, rbuf, sizeof(rbuf) - 1, 0);
                    }
                    else
                    {
                        char rbuf[] = "\r日志文件记录标识修改成功!\r\n$";
                        send(sock, rbuf, sizeof(rbuf) - 1, 0);
                    }

                    TelnetClients[cpos].state = TSCS_CONFIG_SET;
                    g_IsLog2File = tmpvalue;
                }
                else if (tmpvalue == g_IsLog2File)
                {
                    if (1 == g_Language)
                    {
                        char rbuf[] = "\rThis parameter has not been modified!\r\n$";
                        send(sock, rbuf, sizeof(rbuf) - 1, 0);
                    }
                    else
                    {
                        char rbuf[] = "\r此参数未修改!\r\n$";
                        send(sock, rbuf, sizeof(rbuf) - 1, 0);
                    }

                    TelnetClients[cpos].state = TSCS_CONFIG_SET;
                }
            }
            else if (!strcmp(TelnetClients[cpos].cmd, "Off"))
            {
                tmpvalue = 0;

                if (tmpvalue != g_IsLog2File)
                {
                    if (1 == g_Language)
                    {
                        char rbuf[] = "\rLog file record label changes successfully!\r\n$";
                        send(sock, rbuf, sizeof(rbuf) - 1, 0);
                    }
                    else
                    {
                        char rbuf[] = "\r日志文件记录标识修改成功!\r\n$";
                        send(sock, rbuf, sizeof(rbuf) - 1, 0);
                    }

                    TelnetClients[cpos].state = TSCS_CONFIG_SET;
                    g_IsLog2File = tmpvalue;
                }
                else if (tmpvalue == g_IsLog2File)
                {
                    if (1 == g_Language)
                    {
                        char rbuf[] = "\rThis parameter has not been modified!\r\n$";
                        send(sock, rbuf, sizeof(rbuf) - 1, 0);
                    }
                    else
                    {
                        char rbuf[] = "\r此参数未修改!\r\n$";
                        send(sock, rbuf, sizeof(rbuf) - 1, 0);
                    }

                    TelnetClients[cpos].state = TSCS_CONFIG_SET;
                }
            }
            else if (!strcmp(TelnetClients[cpos].cmd, "q"))
            {
                char rbuf[] = "\r";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);

                if (1 == g_Language)
                {
                    send(sock, config_menu_eng, sizeof(config_menu_eng) - 1, 0);
                }
                else
                {
                    send(sock, config_menu, sizeof(config_menu) - 1, 0);
                }

                TelnetClients[cpos].state = TSCS_CONFIG_SET;
            }
            else
            {
                if (1 == g_Language)
                {
                    char rbuf[] = "\rYour input is incorrect, please input again!\r\n$";
                    send(sock, rbuf, sizeof(rbuf) - 1, 0);
                }
                else
                {
                    char rbuf[] = "\r你的输入有误，请从新输入!\r\n$";
                    send(sock, rbuf, sizeof(rbuf) - 1, 0);
                }

                TelnetClients[cpos].state = TSCS_LOG2FILE_SET;
            }
        }
        else
        {
            if (1 == g_Language)
            {
                char rbuf[] = "\rThis parameter has not been modified!\r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }
            else
            {
                char rbuf[] = "\r此参数未修改!\r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }

            TelnetClients[cpos].state = TSCS_CONFIG_SET;
        }

        memset(TelnetClients[cpos].cmd, 0, sizeof(TelnetClients[cpos].cmd));
    }
    else
    {
        if (1 == len && buf[0] == '\b')
        {
            int len = strlen(TelnetClients[cpos].cmd);
            TelnetClients[cpos].cmd[len - 1] = '\0';
        }
        else
        {
            strncat(TelnetClients[cpos].cmd, (char*)buf, len);
        }
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : TelnetLog2DBSetStateProc
 功能描述  : 日志记录到数据库开关设置状态下的处理
 输入参数  : int cpos
             unsigned char* buf
             int len
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年8月7日 星期四
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int TelnetLog2DBSetStateProc(int cpos, unsigned char* buf, int len)
{
    int sock;

    if (cpos < 0 || cpos >= MAX_TELNETCLIENTS)
    {
        return -1;
    }

    if (buf == NULL || len <= 0)
    {
        return -1;
    }

    sock = TelnetClients[cpos].sock;

    if (sock == -1)
    {
        return -1;
    }

    /* echo first */
    send(sock, buf, len, 0);

    if ((len == 2 && !strncmp((char*)buf, "\r\n", len))
        || (len == 1 && !strncmp((char*)buf, "\r", len)))
    {
        /* parse command */
        if (TelnetClients[cpos].cmd[0] != '\0')
        {
            int tmpvalue = 0;

            if (!strcmp(TelnetClients[cpos].cmd, "On"))
            {
                tmpvalue = 1;

                if (tmpvalue != g_IsLog2DB)
                {
                    if (1 == g_Language)
                    {
                        char rbuf[] = "\rLog database record label modification success!\r\n$";
                        send(sock, rbuf, sizeof(rbuf) - 1, 0);
                    }
                    else
                    {
                        char rbuf[] = "\r日志数据库记录标识修改成功!\r\n$";
                        send(sock, rbuf, sizeof(rbuf) - 1, 0);
                    }

                    TelnetClients[cpos].state = TSCS_CONFIG_SET;
                    g_IsLog2DB = tmpvalue;
                }
                else if (tmpvalue == g_IsLog2DB)
                {
                    if (1 == g_Language)
                    {
                        char rbuf[] = "\rThis parameter has not been modified!\r\n$";
                        send(sock, rbuf, sizeof(rbuf) - 1, 0);
                    }
                    else
                    {
                        char rbuf[] = "\r此参数未修改!\r\n$";
                        send(sock, rbuf, sizeof(rbuf) - 1, 0);
                    }

                    TelnetClients[cpos].state = TSCS_CONFIG_SET;
                }
            }
            else if (!strcmp(TelnetClients[cpos].cmd, "Off"))
            {
                tmpvalue = 0;

                if (tmpvalue != g_IsLog2DB)
                {
                    if (1 == g_Language)
                    {
                        char rbuf[] = "\rLog database record label modification success!\r\n$";
                        send(sock, rbuf, sizeof(rbuf) - 1, 0);
                    }
                    else
                    {
                        char rbuf[] = "\r日志数据库记录标识修改成功!\r\n$";
                        send(sock, rbuf, sizeof(rbuf) - 1, 0);
                    }

                    TelnetClients[cpos].state = TSCS_CONFIG_SET;
                    g_IsLog2DB = tmpvalue;
                }
                else if (tmpvalue == g_IsLog2DB)
                {
                    if (1 == g_Language)
                    {
                        char rbuf[] = "\rThis parameter has not been modified!\r\n$";
                        send(sock, rbuf, sizeof(rbuf) - 1, 0);
                    }
                    else
                    {
                        char rbuf[] = "\r此参数未修改!\r\n$";
                        send(sock, rbuf, sizeof(rbuf) - 1, 0);
                    }

                    TelnetClients[cpos].state = TSCS_CONFIG_SET;
                }
            }
            else if (!strcmp(TelnetClients[cpos].cmd, "q"))
            {
                char rbuf[] = "\r";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);

                if (1 == g_Language)
                {
                    send(sock, config_menu_eng, sizeof(config_menu_eng) - 1, 0);
                }
                else
                {
                    send(sock, config_menu, sizeof(config_menu) - 1, 0);
                }

                TelnetClients[cpos].state = TSCS_CONFIG_SET;
            }
            else
            {
                if (1 == g_Language)
                {
                    char rbuf[] = "\rYour input is incorrect, please input again!\r\n$";
                    send(sock, rbuf, sizeof(rbuf) - 1, 0);
                }
                else
                {
                    char rbuf[] = "\r你的输入有误，请从新输入!\r\n$";
                    send(sock, rbuf, sizeof(rbuf) - 1, 0);
                }

                TelnetClients[cpos].state = TSCS_LOG2DB_SET;
            }
        }
        else
        {
            if (1 == g_Language)
            {
                char rbuf[] = "\rThis parameter has not been modified!\r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }
            else
            {
                char rbuf[] = "\r此参数未修改!\r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }

            TelnetClients[cpos].state = TSCS_CONFIG_SET;
        }

        memset(TelnetClients[cpos].cmd, 0, sizeof(TelnetClients[cpos].cmd));
    }
    else
    {
        if (1 == len && buf[0] == '\b')
        {
            int len = strlen(TelnetClients[cpos].cmd);
            TelnetClients[cpos].cmd[len - 1] = '\0';
        }
        else
        {
            strncat(TelnetClients[cpos].cmd, (char*)buf, len);
        }
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : TelnetUserNameSetStateProc
 功能描述  : 配置Telnet用户名状态下的处理
 输入参数  : int cpos
                            unsigned char* buf
                            int len
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年7月25日 星期四
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int TelnetUserNameSetStateProc(int cpos, unsigned char* buf, int len)
{
    int sock;

    if (cpos < 0 || cpos >= MAX_TELNETCLIENTS)
    {
        return -1;
    }

    if (buf == NULL || len <= 0)
    {
        return -1;
    }

    sock = TelnetClients[cpos].sock;

    if (sock == -1)
    {
        return -1;
    }

    /* echo first */
    send(sock, buf, len, 0);

    if ((len == 2 && !strncmp((char*)buf, "\r\n", len))
        || (len == 1 && !strncmp((char*)buf, "\r", len)))
    {
        /* parse command */
        if (TelnetClients[cpos].cmd[0] != '\0')
        {
            if ((pGblconf->spy_username[0] != '\0') && strcmp(TelnetClients[cpos].cmd, pGblconf->spy_username))
            {
                if (1 == g_Language)
                {
                    char rbuf[] = "\rThis parameter has been modified, please enter the s save parameter, save the configuration will be automatically canceled!\r\n$";
                    send(sock, rbuf, sizeof(rbuf) - 1, 0);
                }
                else
                {
                    char rbuf[] = "\r此参数已经修改，请输入s 保存参数，保存配置之后将自动注销!\r\n$";
                    send(sock, rbuf, sizeof(rbuf) - 1, 0);
                }

                TelnetClients[cpos].state = TSCS_CONFIG_SET;
                memset(pGblconf->spy_username, 0, MAX_128CHAR_STRING_LEN + 4);
                snprintf(pGblconf->spy_username, MAX_128CHAR_STRING_LEN + 4, TelnetClients[cpos].cmd);

#if 0
                char rbuf2[] = "\r\nCMS Telnet Login:";
                TelnetClients[cpos].state = TSCS_PROMPT_USER;
                memset(TelnetClients[cpos].user, 0, sizeof(TelnetClients[cpos].user));
                memset(TelnetClients[cpos].password, 0, sizeof(TelnetClients[cpos].password));
                memset(TelnetClients[cpos].cmd, 0, sizeof(TelnetClients[cpos].cmd));
                TelnetClients[cpos].login_retry = 0;
                send(sock, rbuf2, sizeof(rbuf2) - 1, 0);
#endif
            }
            else if (pGblconf->spy_username[0] == '\0')
            {
                if (1 == g_Language)
                {
                    char rbuf[] = "\rThis parameter has been modified, please enter the s save parameter, save the configuration will be automatically canceled!\r\n$";
                    send(sock, rbuf, sizeof(rbuf) - 1, 0);
                }
                else
                {
                    char rbuf[] = "\r此参数已经修改，请输入s 保存参数，保存配置之后将自动注销!\r\n$";
                    send(sock, rbuf, sizeof(rbuf) - 1, 0);
                }

                TelnetClients[cpos].state = TSCS_CONFIG_SET;
                memset(pGblconf->spy_username, 0, MAX_128CHAR_STRING_LEN + 4);
                snprintf(pGblconf->spy_username, MAX_128CHAR_STRING_LEN + 4, TelnetClients[cpos].cmd);

#if 0
                char rbuf2[] = "\r\nCMS Telnet Login:";
                TelnetClients[cpos].state = TSCS_PROMPT_USER;
                memset(TelnetClients[cpos].user, 0, sizeof(TelnetClients[cpos].user));
                memset(TelnetClients[cpos].password, 0, sizeof(TelnetClients[cpos].password));
                memset(TelnetClients[cpos].cmd, 0, sizeof(TelnetClients[cpos].cmd));
                TelnetClients[cpos].login_retry = 0;
                send(sock, rbuf2, sizeof(rbuf2) - 1, 0);
#endif
            }
            else
            {
                if (1 == g_Language)
                {
                    char rbuf[] = "\rThis parameter has not been modified!\r\n$";
                    send(sock, rbuf, sizeof(rbuf) - 1, 0);
                }
                else
                {
                    char rbuf[] = "\r此参数未修改!\r\n$";
                    send(sock, rbuf, sizeof(rbuf) - 1, 0);
                }

                TelnetClients[cpos].state = TSCS_CONFIG_SET;
            }
        }
        else
        {
            if (1 == g_Language)
            {
                char rbuf[] = "\rThis parameter has not been modified!\r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }
            else
            {
                char rbuf[] = "\r此参数未修改!\r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }

            TelnetClients[cpos].state = TSCS_CONFIG_SET;
        }

        memset(TelnetClients[cpos].cmd, 0, sizeof(TelnetClients[cpos].cmd));
    }
    else
    {
        if (1 == len && buf[0] == '\b')
        {
            int len = strlen(TelnetClients[cpos].cmd);
            TelnetClients[cpos].cmd[len - 1] = '\0';
        }
        else
        {
            strncat(TelnetClients[cpos].cmd, (char*)buf, len);
        }
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : TelnetUserPasswordSetStateProc
 功能描述  : Telnet用户密码修改状态的处理
 输入参数  : int cpos
                            unsigned char* buf
                            int len
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年7月25日 星期四
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int TelnetUserPasswordSetStateProc(int cpos, unsigned char* buf, int len)
{
    int sock;

    if (cpos < 0 || cpos >= MAX_TELNETCLIENTS)
    {
        return -1;
    }

    if (buf == NULL || len <= 0)
    {
        return -1;
    }

    sock = TelnetClients[cpos].sock;

    if (sock == -1)
    {
        return -1;
    }

    /* echo first */
    send(sock, buf, len, 0);

    if ((len == 2 && !strncmp((char*)buf, "\r\n", len))
        || (len == 1 && !strncmp((char*)buf, "\r", len)))
    {
        /* parse command */
        if (TelnetClients[cpos].cmd[0] != '\0')
        {
            if ((pGblconf->spy_password[0] != '\0') && strcmp(TelnetClients[cpos].cmd, pGblconf->spy_password))
            {
                if (1 == g_Language)
                {
                    char rbuf[] = "\rThis parameter has been modified, please enter the s save parameter, save the configuration will be automatically canceled!\r\n$";
                    send(sock, rbuf, sizeof(rbuf) - 1, 0);
                }
                else
                {
                    char rbuf[] = "\r此参数已经修改，请输入s 保存参数，保存配置之后将自动注销!\r\n$";
                    send(sock, rbuf, sizeof(rbuf) - 1, 0);
                }

                TelnetClients[cpos].state = TSCS_CONFIG_SET;
                memset(pGblconf->spy_password, 0, MAX_128CHAR_STRING_LEN + 4);
                snprintf(pGblconf->spy_password, MAX_128CHAR_STRING_LEN + 4, TelnetClients[cpos].cmd);

#if 0
                char rbuf2[] = "\r\nCMS Telnet Login:";
                TelnetClients[cpos].state = TSCS_PROMPT_USER;
                memset(TelnetClients[cpos].user, 0, sizeof(TelnetClients[cpos].user));
                memset(TelnetClients[cpos].password, 0, sizeof(TelnetClients[cpos].password));
                memset(TelnetClients[cpos].cmd, 0, sizeof(TelnetClients[cpos].cmd));
                TelnetClients[cpos].login_retry = 0;
                send(sock, rbuf2, sizeof(rbuf2) - 1, 0);
#endif
            }
            else if (pGblconf->spy_password[0] == '\0')
            {
                if (1 == g_Language)
                {
                    char rbuf[] = "\rThis parameter has been modified, please enter the s save parameter, save the configuration will be automatically canceled!\r\n$";
                    send(sock, rbuf, sizeof(rbuf) - 1, 0);
                }
                else
                {
                    char rbuf[] = "\r此参数已经修改，请输入s 保存参数，保存配置之后将自动注销!\r\n$";
                    send(sock, rbuf, sizeof(rbuf) - 1, 0);
                }

                TelnetClients[cpos].state = TSCS_CONFIG_SET;
                memset(pGblconf->spy_password, 0, MAX_128CHAR_STRING_LEN + 4);
                snprintf(pGblconf->spy_password, MAX_128CHAR_STRING_LEN + 4, TelnetClients[cpos].cmd);

#if 0
                char rbuf2[] = "\r\nCMS Telnet Login:";
                TelnetClients[cpos].state = TSCS_PROMPT_USER;
                memset(TelnetClients[cpos].user, 0, sizeof(TelnetClients[cpos].user));
                memset(TelnetClients[cpos].password, 0, sizeof(TelnetClients[cpos].password));
                memset(TelnetClients[cpos].cmd, 0, sizeof(TelnetClients[cpos].cmd));
                TelnetClients[cpos].login_retry = 0;
                send(sock, rbuf2, sizeof(rbuf2) - 1, 0);
#endif
            }
            else
            {
                if (1 == g_Language)
                {
                    char rbuf[] = "\rThis parameter has not been modified!\r\n$";
                    send(sock, rbuf, sizeof(rbuf) - 1, 0);
                }
                else
                {
                    char rbuf[] = "\r此参数未修改!\r\n$";
                    send(sock, rbuf, sizeof(rbuf) - 1, 0);
                }

                TelnetClients[cpos].state = TSCS_CONFIG_SET;
            }
        }
        else
        {
            if (1 == g_Language)
            {
                char rbuf[] = "\rThis parameter has not been modified!\r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }
            else
            {
                char rbuf[] = "\r此参数未修改!\r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }

            TelnetClients[cpos].state = TSCS_CONFIG_SET;
        }

        memset(TelnetClients[cpos].cmd, 0, sizeof(TelnetClients[cpos].cmd));
    }
    else
    {
        if (1 == len && buf[0] == '\b')
        {
            int len = strlen(TelnetClients[cpos].cmd);
            TelnetClients[cpos].cmd[len - 1] = '\0';
        }
        else
        {
            strncat(TelnetClients[cpos].cmd, (char*)buf, len);
        }
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : TelnetPortSetStateProc
 功能描述  : Telnet登录端口号修改状态的处理
 输入参数  : int cpos
                            unsigned char* buf
                            int len
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年7月25日 星期四
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int TelnetPortSetStateProc(int cpos, unsigned char* buf, int len)
{
    int sock;

    if (cpos < 0 || cpos >= MAX_TELNETCLIENTS)
    {
        return -1;
    }

    if (buf == NULL || len <= 0)
    {
        return -1;
    }

    sock = TelnetClients[cpos].sock;

    if (sock == -1)
    {
        return -1;
    }

    /* echo first */
    send(sock, buf, len, 0);

    if ((len == 2 && !strncmp((char*)buf, "\r\n", len))
        || (len == 1 && !strncmp((char*)buf, "\r", len)))
    {
        /* parse command */
        if (TelnetClients[cpos].cmd[0] != '\0')
        {
            int tmpvalue = osip_atoi(TelnetClients[cpos].cmd);

            if (tmpvalue > 0 && tmpvalue != pGblconf->spy_port)
            {
                if (1 == g_Language)
                {
                    char rbuf[] = "\rThis parameter has been modified, please enter the s save parameter, save the configuration will be automatically canceled!\r\n$";
                    send(sock, rbuf, sizeof(rbuf) - 1, 0);
                }
                else
                {
                    char rbuf[] = "\r此参数已经修改，请输入s 保存参数，保存配置之后将自动注销!\r\n$";
                    send(sock, rbuf, sizeof(rbuf) - 1, 0);
                }

                TelnetClients[cpos].state = TSCS_CONFIG_SET;
                pGblconf->spy_port = tmpvalue;
            }
            else if (tmpvalue <= 0)
            {
                if (1 == g_Language)
                {
                    char rbuf[] = "\rYour input parameters are incorrect, the system will use the default value of 2000 port!\r\n$";
                    send(sock, rbuf, sizeof(rbuf) - 1, 0);
                }
                else
                {
                    char rbuf[] = "\r你输入的参数有误，系统将采用默认值2000 端口!\r\n$";
                    send(sock, rbuf, sizeof(rbuf) - 1, 0);
                }

                TelnetClients[cpos].state = TSCS_CONFIG_SET;
                pGblconf->spy_port = 2000;
            }
            else
            {
                if (1 == g_Language)
                {
                    char rbuf[] = "\rThis parameter has not been modified!\r\n$";
                    send(sock, rbuf, sizeof(rbuf) - 1, 0);
                }
                else
                {
                    char rbuf[] = "\r此参数未修改!\r\n$";
                    send(sock, rbuf, sizeof(rbuf) - 1, 0);
                }

                TelnetClients[cpos].state = TSCS_CONFIG_SET;
            }
        }
        else
        {
            if (1 == g_Language)
            {
                char rbuf[] = "\rThis parameter has not been modified!\r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }
            else
            {
                char rbuf[] = "\r此参数未修改!\r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }

            TelnetClients[cpos].state = TSCS_CONFIG_SET;
        }

        memset(TelnetClients[cpos].cmd, 0, sizeof(TelnetClients[cpos].cmd));
    }
    else
    {
        if (1 == len && buf[0] == '\b')
        {
            int len = strlen(TelnetClients[cpos].cmd);
            TelnetClients[cpos].cmd[len - 1] = '\0';
        }
        else
        {
            strncat(TelnetClients[cpos].cmd, (char*)buf, len);
        }
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : TelnetConfigSaveStateProc
 功能描述  : 保存配置状态下的处理
 输入参数  : int cpos
                            unsigned char* buf
                            int len
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年7月25日 星期四
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int TelnetConfigSaveStateProc(int cpos, unsigned char* buf, int len)
{
    int sock;

    if (cpos < 0 || cpos >= MAX_TELNETCLIENTS)
    {
        return -1;
    }

    if (buf == NULL || len <= 0)
    {
        return -1;
    }

    sock = TelnetClients[cpos].sock;

    if (sock == -1)
    {
        return -1;
    }

    /* echo first */
    send(sock, buf, len, 0);

    if ((len == 2 && !strncmp((char*)buf, "\r\n", len))
        || (len == 1 && !strncmp((char*)buf, "\r", len)))
    {
        /* parse command */
        if (!strcmp(TelnetClients[cpos].cmd, "y"))
        {
            if (/*write_conf_to_config_file() == 0*/1)
            {
                if (1 == g_Language)
                {
                    char rbuf[] = "\rParameter save successfully!\r\n$";
                    send(sock, rbuf, sizeof(rbuf) - 1, 0);
                }
                else
                {
                    char rbuf[] = "\r参数保存成功!\r\n$";
                    send(sock, rbuf, sizeof(rbuf) - 1, 0);
                }

                TelnetClients[cpos].state = TSCS_CONFIG_SET;
            }
            else
            {
                if (1 == g_Language)
                {
                    char rbuf[] = "\rParameter save failed!\r\n$";
                    send(sock, rbuf, sizeof(rbuf) - 1, 0);
                }
                else
                {
                    char rbuf[] = "\r参数保存失败!\r\n$";
                    send(sock, rbuf, sizeof(rbuf) - 1, 0);
                }

                TelnetClients[cpos].state = TSCS_CONFIG_SET;
            }
        }
        else if (!strcmp(TelnetClients[cpos].cmd, "n"))
        {
            if (1 == g_Language)
            {
                char rbuf[] = "\rParameter is not saved!\r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }
            else
            {
                char rbuf[] = "\r参数未保存!\r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }

            TelnetClients[cpos].state = TSCS_CONFIG_SET;
        }
        else
        {
            if (1 == g_Language)
            {
                char rbuf[] = "\rPlease enter y or n \r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }
            else
            {
                char rbuf[] = "\r请输入y 或者n \r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }
        }

        memset(TelnetClients[cpos].cmd, 0, sizeof(TelnetClients[cpos].cmd));
    }
    else
    {
        if (1 == len && buf[0] == '\b')
        {
            int len = strlen(TelnetClients[cpos].cmd);
            TelnetClients[cpos].cmd[len - 1] = '\0';
        }
        else
        {
            strncat(TelnetClients[cpos].cmd, (char*)buf, len);
        }
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : TelnetStopSystemConfirmStateProc
 功能描述  : 停止CMS系统确认状态下的处理
 输入参数  : int cpos
                            unsigned char* buf
                            int len
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年7月25日 星期四
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int TelnetStopSystemConfirmStateProc(int cpos, unsigned char* buf, int len)
{
    int sock;

    if (cpos < 0 || cpos >= MAX_TELNETCLIENTS)
    {
        return -1;
    }

    if (buf == NULL || len <= 0)
    {
        return -1;
    }

    sock = TelnetClients[cpos].sock;

    if (sock == -1)
    {
        return -1;
    }

    /* echo first */
    send(sock, buf, len, 0);

    if ((len == 2 && !strncmp((char*)buf, "\r\n", len))
        || (len == 1 && !strncmp((char*)buf, "\r", len)))
    {
        /* parse command */
        if (!strcmp(TelnetClients[cpos].cmd, "y"))
        {
            if (1 == g_Language)
            {
                char rbuf[] = "\rCMS system is being stopped...\r\n";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }
            else
            {
                char rbuf[] = "\rCMS 系统正在停止...\r\n";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }

            stop_all = 1;
        }
        else if (!strcmp(TelnetClients[cpos].cmd, "n"))
        {
            if (1 == g_Language)
            {
                char rbuf[] = "\rCancel stop CMS system!\r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }
            else
            {
                char rbuf[] = "\r取消停止CMS系统!\r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }

            TelnetClients[cpos].state = TSCS_LOGIN;
        }
        else
        {
            if (1 == g_Language)
            {
                char rbuf[] = "\rPlease enter y or n \r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }
            else
            {
                char rbuf[] = "\r请输入y 或者n \r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }
        }

        memset(TelnetClients[cpos].cmd, 0, sizeof(TelnetClients[cpos].cmd));
    }
    else
    {
        if (1 == len && buf[0] == '\b')
        {
            int len = strlen(TelnetClients[cpos].cmd);
            TelnetClients[cpos].cmd[len - 1] = '\0';
        }
        else
        {
            strncat(TelnetClients[cpos].cmd, (char*)buf, len);
        }
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : TelnetRestartSystemConfirmStateProc
 功能描述  : 重启CMS系统确认状态下的处理
 输入参数  : int cpos
                            unsigned char* buf
                            int len
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年7月25日 星期四
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int TelnetRestartSystemConfirmStateProc(int cpos, unsigned char* buf, int len)
{
    int sock;

    if (cpos < 0 || cpos >= MAX_TELNETCLIENTS)
    {
        return -1;
    }

    if (buf == NULL || len <= 0)
    {
        return -1;
    }

    sock = TelnetClients[cpos].sock;

    if (sock == -1)
    {
        return -1;
    }

    /* echo first */
    send(sock, buf, len, 0);

    if ((len == 2 && !strncmp((char*)buf, "\r\n", len))
        || (len == 1 && !strncmp((char*)buf, "\r", len)))
    {
        /* parse command */
        if (!strcmp(TelnetClients[cpos].cmd, "y"))
        {
            if (1 == g_Language)
            {
                char rbuf[] = "\rCMS system is being restarted...\r\n";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }
            else
            {
                char rbuf[] = "\rCMS 系统正在重启...\r\n";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }

            do_restart();
        }
        else if (!strcmp(TelnetClients[cpos].cmd, "n"))
        {
            if (1 == g_Language)
            {
                char rbuf[] = "\rCancel restart CMS system!\r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }
            else
            {
                char rbuf[] = "\r取消重启CMS系统!\r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }

            TelnetClients[cpos].state = TSCS_LOGIN;
        }
        else
        {
            if (1 == g_Language)
            {
                char rbuf[] = "\rPlease enter y or n \r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }
            else
            {
                char rbuf[] = "\r请输入y 或者n \r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }
        }

        memset(TelnetClients[cpos].cmd, 0, sizeof(TelnetClients[cpos].cmd));
    }
    else
    {
        if (1 == len && buf[0] == '\b')
        {
            int len = strlen(TelnetClients[cpos].cmd);
            TelnetClients[cpos].cmd[len - 1] = '\0';
        }
        else
        {
            strncat(TelnetClients[cpos].cmd, (char*)buf, len);
        }
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : TelnetLogoutConfirmStateProc
 功能描述  : Telnet注销登录确认状态下的处理
 输入参数  : int cpos
                            unsigned char* buf
                            int len
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年7月25日 星期四
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int TelnetLogoutConfirmStateProc(int cpos, unsigned char* buf, int len)
{
    int sock;

    if (cpos < 0 || cpos >= MAX_TELNETCLIENTS)
    {
        return -1;
    }

    if (buf == NULL || len <= 0)
    {
        return -1;
    }

    sock = TelnetClients[cpos].sock;

    if (sock == -1)
    {
        return -1;
    }

    /* echo first */
    send(sock, buf, len, 0);

    if ((len == 2 && !strncmp((char*)buf, "\r\n", len))
        || (len == 1 && !strncmp((char*)buf, "\r", len)))
    {
        /* parse command */
        if (!strcmp(TelnetClients[cpos].cmd, "y"))
        {
            char rbuf[] = "\rCMS Telnet Login:";
            send(sock, rbuf, sizeof(rbuf) - 1, 0);
            TelnetClients[cpos].state = TSCS_PROMPT_USER;
            memset(TelnetClients[cpos].user, 0, sizeof(TelnetClients[cpos].user));
            memset(TelnetClients[cpos].password, 0, sizeof(TelnetClients[cpos].password));
            memset(TelnetClients[cpos].cmd, 0, sizeof(TelnetClients[cpos].cmd));
            TelnetClients[cpos].login_retry = 0;
        }
        else if (!strcmp(TelnetClients[cpos].cmd, "n"))
        {
            if (1 == g_Language)
            {
                char rbuf[] = "\rCancel Telnet login!\r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }
            else
            {
                char rbuf[] = "\r取消注销Telnet 登录!\r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }

            TelnetClients[cpos].state = TSCS_LOGIN;
        }
        else
        {
            if (1 == g_Language)
            {
                char rbuf[] = "\rPlease enter y or n \r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }
            else
            {
                char rbuf[] = "\r请输入y 或者n \r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }
        }

        memset(TelnetClients[cpos].cmd, 0, sizeof(TelnetClients[cpos].cmd));
    }
    else
    {
        if (1 == len && buf[0] == '\b')
        {
            int len = strlen(TelnetClients[cpos].cmd);
            TelnetClients[cpos].cmd[len - 1] = '\0';
        }
        else
        {
            strncat(TelnetClients[cpos].cmd, (char*)buf, len);
        }
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : TelnetExitConfirmStateProc
 功能描述  : Telnet退出登录确认状态下的处理
 输入参数  : int cpos
                            unsigned char* buf
                            int len
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年7月25日 星期四
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int TelnetExitConfirmStateProc(int cpos, unsigned char* buf, int len)
{
    int sock;

    if (cpos < 0 || cpos >= MAX_TELNETCLIENTS)
    {
        return -1;
    }

    if (buf == NULL || len <= 0)
    {
        return -1;
    }

    sock = TelnetClients[cpos].sock;

    if (sock == -1)
    {
        return -1;
    }

    /* echo first */
    send(sock, buf, len, 0);

    if ((len == 2 && !strncmp((char*)buf, "\r\n", len))
        || (len == 1 && !strncmp((char*)buf, "\r", len)))
    {
        /* parse command */
        if (!strcmp(TelnetClients[cpos].cmd, "y"))
        {
            close(sock);
            TelnetClients[cpos].sock = -1;
            TelnetClients[cpos].state = TSCS_NULL;
            memset(TelnetClients[cpos].user, 0, sizeof(TelnetClients[cpos].user));
            memset(TelnetClients[cpos].password, 0, sizeof(TelnetClients[cpos].password));
        }
        else if (!strcmp(TelnetClients[cpos].cmd, "n"))
        {
            if (1 == g_Language)
            {
                char rbuf[] = "\rCancel exit Telnet login!\r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }
            else
            {
                char rbuf[] = "\r取消退出Telnet 登录!\r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }

            TelnetClients[cpos].state = TSCS_LOGIN;
        }
        else
        {
            if (1 == g_Language)
            {
                char rbuf[] = "\rPlease enter y or n \r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }
            else
            {
                char rbuf[] = "\r请输入y 或者n \r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }
        }

        memset(TelnetClients[cpos].cmd, 0, sizeof(TelnetClients[cpos].cmd));
    }
    else
    {
        if (1 == len && buf[0] == '\b')
        {
            int len = strlen(TelnetClients[cpos].cmd);
            TelnetClients[cpos].cmd[len - 1] = '\0';
        }
        else
        {
            strncat(TelnetClients[cpos].cmd, (char*)buf, len);
        }
    }

    return 0;
}

#if 0
int TelnetProcConfigData(int cpos, unsigned char* buf, int len)
{
    int sock;

    if (cpos < 0 || cpos >= MAX_TELNETCLIENTS)
    {
        return -1;
    }

    if (buf == NULL || len <= 0)
    {
        return -1;
    }

    sock = TelnetClients[cpos].sock;

    if (sock == -1)
    {
        return -1;
    }
}
#endif

/*****************************************************************************
 函 数 名  : telnetauth
 功能描述  : telnet登录认证
 输入参数  : char* user
             char* password
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年7月25日 星期四
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int telnetauth(char* user, char* password)
{
    if (user == NULL || password == NULL)
    {
        return 0;
    }

    return(sstrcmp(pGblconf->spy_username, user) == 0
           && sstrcmp(pGblconf->spy_password, password) == 0);
}

/*****************************************************************************
 函 数 名  : TelnetSend
 功能描述  : telnet远端发送函数
 输入参数  : char* msg
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年7月25日 星期四
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void TelnetSend(char* msg)
{
    int p = 0;

    if (NULL == msg)
    {
        return;
    }

    for (p = 0; p < MAX_TELNETCLIENTS; p++)
    {
        if (TelnetClients[p].sock <= 0)
        {
            continue;
        }

        if (TelnetClients[p].state != TSCS_DBG_SEND)
        {
            continue;
        }

        if (send(TelnetClients[p].sock, msg, strlen(msg), 0) < 0)
        {
            close(TelnetClients[p].sock);
            telnet_client_init(&TelnetClients[p]);
            continue;
        }
    }

    return;
}

/*****************************************************************************
 函 数 名  : TelnetRunTraceSend
 功能描述  : telnet运行信息远端发送函数
 输入参数  : char* msg
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年7月25日 星期四
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void TelnetRunTraceSend(char* msg)
{
    int p = 0;

    if (NULL == msg)
    {
        return;
    }

    for (p = 0; p < MAX_TELNETCLIENTS; p++)
    {
        if (TelnetClients[p].sock <= 0)
        {
            continue;
        }

        if (TelnetClients[p].state != TSCS_RUNTRACE_SEND)
        {
            continue;
        }

        if (send(TelnetClients[p].sock, msg, strlen(msg), 0) < 0)
        {
            close(TelnetClients[p].sock);
            telnet_client_init(&TelnetClients[p]);
            continue;
        }
    }

    return;
}

/*****************************************************************************
 函 数 名  : TelnetSendSIPMessage
 功能描述  : Telnet远端发送SIP消息
 输入参数  : const char* fmt
             ...
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年9月4日 星期三
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void TelnetSendSIPMessage(const char* fmt, ...)
{
    int p = 0;
    int len = 0;
    va_list args;
    char s[MAX_2048CHAR_STRING_LEN + 4] = {0};

    va_start(args, fmt);
    len = vsnprintf(s, MAX_2048CHAR_STRING_LEN, fmt, args);
    va_end(args);

    for (p = 0; p < MAX_TELNETCLIENTS; p++)
    {
        if (TelnetClients[p].sock <= 0)
        {
            continue;
        }

        if (TelnetClients[p].state != TSCS_DBG_SEND_MSG)
        {
            continue;
        }

        if (send(TelnetClients[p].sock, s, len, 0) < 0)
        {
            close(TelnetClients[p].sock);
            telnet_client_init(&TelnetClients[p]);
            continue;
        }
    }

    return;
}
