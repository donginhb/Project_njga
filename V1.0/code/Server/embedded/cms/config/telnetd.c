/******************************************************************************

                  ��Ȩ���� (C), 2001-2013, ������Ѷ�������޹�˾

 ******************************************************************************
  �� �� ��   : telnetd.c
  �� �� ��   : ����
  ��    ��   : yanghaifeng
  ��������   : 2013��7��25�� ������
  ����޸�   :
  ��������   : Telnet��¼����
  �����б�   :
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
  �޸���ʷ   :
  1.��    ��   : 2013��7��25�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ļ�

******************************************************************************/

/*----------------------------------------------*
 * ����ͷ�ļ�                                   *
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
 * �ⲿ����˵��                                        *
 *----------------------------------------------*/
extern gbl_conf_t* pGblconf;              /* ȫ��������Ϣ */
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
 * �ⲿ����ԭ��˵��                             *
 *----------------------------------------------*/
extern void do_restart();

/*----------------------------------------------*
 * �ڲ�����ԭ��˵��                             *
 *----------------------------------------------*/

/*----------------------------------------------*
 * ȫ�ֱ���                                     *
 *----------------------------------------------*/
int TelnetServSock = 0;

/*----------------------------------------------*
 * ģ�鼶����                                   *
 *----------------------------------------------*/
#define MAX_TELNETCLIENTS 3
#define MAX_TELNETUSERS   5

#define MAX_TELNET_LOGINRETRY 3
#define MAX_LOGINTIME      300  /* seconds */

typedef enum _telnet_client_state
{
    TSCS_NULL,                                /* ��״̬ */
    TSCS_PROMPT_USER,                         /* �����û���״̬ */
    TSCS_PROMPT_PASSWORD,                     /* ��������״̬ */
    TSCS_LOGIN,                               /* ��¼�ɹ�״̬ */
    TSCS_DBG_SEND,                            /* ������Ϣ���״̬ */
    TSCS_RUNTRACE_SEND,                       /* ������Ϣ���״̬ */
    TSCS_DBG_SEND_MSG,                        /* ������Ϣ���SIP ��Ϣ״̬ */
    TSCS_SYS_SHELL,                           /* ϵͳshell����״̬ */
    TSCS_CONFIG_SET,                          /* ϵͳ����״̬ */
    TSCS_SYS_RUN_INFO,                        /* ϵͳ������Ϣ��ѯ*/
    TSCS_SYS_CALL_TASK_DETAL,                 /* ϵͳ���к��������ѯ*/
    TSCS_SYS_SIPUA_INFO,                      /* ϵͳ����SIPUA �Ự��Ϣ��ѯ*/
    TSCS_SYS_DIAGONSE,                        /* ϵͳ�����Ϣ��ѯ*/
    TSCS_SYS_RELEASE_RECORD_TASK,             /* �ͷ�¼������*/
    TSCS_SYS_RELEASE_RECORD_TASK_CONFIRM,     /* �ͷ�¼������ȷ��*/
    TSCS_SYS_RELEASE_CALL_TASK,               /* �ͷź�������*/
    TSCS_SYS_RELEASE_CALL_TASK_CONFIRM,       /* �ͷź�������ȷ��*/
    TSCS_SYS_STOP_POLL_TASK,                  /* ֹͣ��Ѳ����*/
    TSCS_SYS_STOP_POLL_TASK_CONFIRM,          /* ֹͣ��Ѳ����ȷ��*/
    TSCS_SYS_STOP_CRUISE_TASK,                /* ֹͣѲ������*/
    TSCS_SYS_STOP_CRUISE_TASK_CONFIRM,        /* ֹͣѲ������ȷ��*/
    TSCS_SYS_RELEASE_UA_INFO,                 /* �ͷ�UA ��Ϣ*/
    TSCS_SYS_RELEASE_UA_INFO_CONFIRM,         /* �ͷ�UA ��Ϣȷ��*/
    TSCS_SYS_RELEASE_UAS_INFO,                /* �ͷ�UAS ��Ϣ*/
    TSCS_SYS_RELEASE_UAS_INFO_CONFIRM,        /* �ͷ�UAS ��Ϣȷ��*/
    TSCS_SYS_RELEASE_UAC_INFO,                /* �ͷ�UAC ��Ϣ*/
    TSCS_SYS_RELEASE_UAC_INFO_CONFIRM,        /* �ͷ�UAC ��Ϣȷ��*/
    TSCS_DBGLEVEL_SET,                        /* ���õ��Եȼ�*/
    TSCS_LOGLEVEL_SET,                        /* ������־�ȼ�*/
    TSCS_LOG2FILE_SET,                        /* ������־�ļ���¼����*/
    TSCS_LOG2DB_SET,                          /* ������־���ݿ��¼����*/
    TSCS_TELUSERNAME_SET,                     /* ����Telnet�û���*/
    TSCS_TELPASSWORD_SET,                     /* ����Telnet����*/
    TSCS_TELPORT_SET,                         /* ����Telnet�˿�*/
    TSCS_CONFIG_SAVE,                         /* ��������*/
    TSCS_EXIT_SYS_CONFIRM,                    /* ֹͣCMSϵͳȷ��*/
    TSCS_RESTART_CONFIRM,                     /* ����CMSϵͳȷ��*/
    TSCS_LOGOUT,                              /* Telnet �ǳ�*/
    TSCS_LOGOUT_CONFIRM,                      /* Telnet �ǳ�ȷ��*/
    TSCS_CLOSE,                               /* Telnet �ر�*/
    TSCS_CLOSE_CONFIRM                        /* Telnet ȷ��*/
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
 * ��������                                     *
 *----------------------------------------------*/

/*----------------------------------------------*
 * �궨��                                       *
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

char main_menu[] = "\r----- ������Ѷ��ӭ��ʹ�� * iEV9000 CMS ϵͳ * -----\r\n\
                \r����������: \r\n\
                \rconfig(c)    : ϵͳ����\r\n\
                \rruninfo(r)   : �鿴ϵͳ������Ϣ\r\n\
                \rversion(v)   : ��ʾϵͳ�汾��\r\n\
                \rdebug(d)     : Զ�̼���ģʽ\r\n\
                \rrundebug(rd) : ������ϢԶ�̼���ģʽ\r\n\
                \rsysshell(ss) : ϵͳShellģʽ\r\n\
                \rtime(t)      : ��ǰʱ��\r\n\
                \rstop         : ��������(ֹͣCMSϵͳ)\r\n\
                \rrestart      : ����ϵͳ(��������CMSϵͳ)\r\n\
                \rlogout       : Telnet�ǳ�\r\n\
                \rexit         : Telnet�˳�\r\n\
                \rhelp(h/?)    : ����\r\n$";

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

char runinfo_menu[] = "\r----- * iEV9000 CMS ϵͳ������Ϣ * -----\r\n\
                \r����������: \r\n\
                \r---------------------------------------------------------------------\r\n\
                \rplatform info(pi)                  : �ϼ�ƽ̨��Ϣ\r\n\
                \r---------------------------------------------------------------------\r\n\
                \rhas used platform thread(hupt)     : �Ѿ�ʹ�õ��ϼ�ƽ̨�߳�\r\n\
                \rno used platform thread(nupt)      : û��ʹ�õ��ϼ�ƽ̨�߳�\r\n\
                \rall platform thread(apt)           : ���е��ϼ�ƽ̨�߳�\r\n\
                \r---------------------------------------------------------------------\r\n\
                \ronline zrv device(ond)             : ����zrv�豸\r\n\
                \roffline zrv device(ofd)            : ����zrv�豸\r\n\
                \rall zrv device(ad)                 : ����zrv�豸\r\n\
                \r---------------------------------------------------------------------\r\n\
                \rhas used device thread(hudt)       : �Ѿ�ʹ�õ��豸�߳�\r\n\
                \rno used device thread(nudt)        : û��ʹ�õ��豸�߳�\r\n\
                \rall device thread(adt)             : ���е��豸�߳�\r\n\
                \r---------------------------------------------------------------------\r\n\
                \rcompress task(ct)                  : ��ǰϵͳ����ѹ������\r\n\
                \r---------------------------------------------------------------------\r\n\
                \rshow system param(ssp)             : ��ʾϵͳȫ�����ò���\r\n\
                \rshow system ip info(ssii)          : ��ʾϵͳIP��ַ��Ϣ\r\n\
                \rshow system thread(sst)            : ��ʾϵͳ�����߳�\r\n\
                \r---------------------------------------------------------------------\r\n\
                \rquit(q)                            : �����ϼ�Ŀ¼\r\n\
                \rhelp(h/?)                          : ����\r\n$";

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

char diagnose_menu[] = "\r----- * iEV9000 CMS ϵͳ���� * -----\r\n\
                \r����������(ע��:������������ϵͳά�����ԣ���������ϵͳ�쳣�������ʹ��): \r\n\
                \rrelease record task        : �ͷ�¼������\r\n\
                \rrelease call task          : �ͷź�������\r\n\
                \rstop poll task             : ֹͣ��Ѳ����\r\n\
                \rstop cruise task           : ֹͣѲ������\r\n\
                \rrelease all ua dialog      : �ͷ�����SIP UA�Ի���Ϣ\r\n\
                \rrelease all uas register   : �ͷ����з����ע����Ϣ\r\n\
                \rrelease all uac register   : �ͷ����пͻ���ע����Ϣ\r\n\
                \rquit(q)                    : �����ϼ�Ŀ¼\r\n\
                \rhelp(h/?)                  : ����\r\n$";

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

char config_menu[] = "\r----- * iEV9000 CMS ϵͳ���� * -----\r\n\
                \r��������������,�����˳�ǰ��s �����������޸ģ�\r\n\
                \rdebug level(dl)      : Debug ��������(1:All 2:Debug 3:Info 4:Warning 5:Error 6:Fatal 7:Off)\r\n\
                \rlog level(ll)        : Log��־��������(1:NORMAL 2:WARNING 3:ERROR)\r\n\
                \rlog2file switch(lfs) : ��־��¼�ļ���������(On/Off)\r\n\
                \rlog2db switch(lds)   : ��־��¼���ݿ⿪������(On/Off)\r\n\
                \rtelnet username(tu)  : Telnet ��¼�û�������(Telnet Username)\r\n\
                \rtelnet password(tp)  : Telnet ��¼��������(Telnet Password)\r\n\
                \rtelnet port(td)      : Telnet ��¼�˿�����(Telnet Port)\r\n\
                \rsave(s)              : �����������޸�(Save Config)\r\n\
                \rquit(q)              : �����ϼ�Ŀ¼\r\n\
                \rprint(p)             : ��ʾ��ǰ������Ϣ\r\n\
                \rhelp(h/?)            : ����\r\n$";

char config_menu_eng[] = "\r----- * iEV9000 CMS system configuration * -----\r\n\
                \rSimple configuration command is as follows, please press s to save the changes before exiting��\r\n\
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
 �� �� ��  : telnet_client_init
 ��������  : telnet�ͻ��˹���ṹ��ʼ��
 �������  : telnet_client_t * sc
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��7��25�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

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
 �� �� ��  : TelnetServerInit
 ��������  : telnet����˳�ʼ��
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��7��25�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

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
 �� �� ��  : TelnetServerFree
 ��������  : telnet������ͷ�
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��7��25�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

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
 �� �� ��  : TelnetServerMain
 ��������  : telnet�����������
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��7��25�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

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

        /* ���÷��ͳ�ʱʱ�� */
        iTimeout.tv_sec = 1; /* 1�볬ʱ */
        iTimeout.tv_usec = 0;
        setsockopt(clntSock, SOL_SOCKET, SO_SNDTIMEO, (char *)&iTimeout, sizeof(struct timeval));

        /* ���÷��ͻ����� */
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

            /* ���³�ʼ�����пͻ�����Ϣ */
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
 �� �� ��  : ScanClientIfExpires
 ��������  : ɨ��ͻ����Ƿ�ʱ
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��7��31�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

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

            char rbuf0[] = "\r��¼��ʱ��Telnet�Զ��ǳ��������µ�¼\r\n";
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
 �� �� ��  : TelnetParseClientData
 ��������  : �����ͻ�������
 �������  : int cpos
             unsigned char* buf
             int len
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��7��25�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

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
                        char rbuf[] = "\r\nϵͳ���������µ�¼!";
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
 �� �� ��  : TelnetPromptUserStateProc
 ��������  : �����û����׶εĴ���
 �������  : int cpos
                            unsigned char* buf
                            int len
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��7��25�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

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
 �� �� ��  : TelnetPromptPasswordStateProc
 ��������  : ��������׶εĴ���
 �������  : int cpos
                            unsigned char* buf
                            int len
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��7��25�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

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
 �� �� ��  : TelnetLoginStateProc
 ��������  : ��¼�ɹ�״̬�µĴ���
 �������  : int cpos
                            unsigned char* buf
                            int len
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��7��25�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

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
                char rbuf[] = "\r��ȷ��Ҫ�˳�Telnet��¼�� ?('y' or 'n'):\r\n$";
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

            //send(sock, (char*)"��δ����\r\n$", sizeof("��δ����\r\n$") - 1, 0);
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
                char rbuf[] = "\r ���������ϢԶ�˲鿴״̬��������ʱ����q �˳�.\r\n$";
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
                char rbuf[] = "\r ����������ϢԶ�˲鿴״̬��������ʱ����q �˳�.\r\n$";
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
                char rbuf[] = "\r ����SIP��Ϣ������ϢԶ�˲鿴״̬��������ʱ����q �˳�.\r\n$";
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
                char rbuf[] = "\r ����ϵͳShellģʽ��������ʱ����q �˳�.\r\n$";
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
            localtime_r(&now, &tp); /* ��ȡ��ǰʱ�� */
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
                char rbuf[] = "\r��ȷ��ҪֹͣCMSϵͳ�� ?('y' or 'n'):\r\n$";
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
                char rbuf[] = "\r��ȷ��Ҫ����CMSϵͳ�� ?('y' or 'n'):\r\n$";
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
                char rbuf[] = "\r��ȷ��Ҫע��Telnet��¼�� ?('y' or 'n'):\r\n$";
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
                char rbuf[] = "\r������������ȷ!\r\n$";
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
 �� �� ��  : TelnetDebugSendStateProc
 ��������  : ������Ϣ���״̬�µĴ���
 �������  : int cpos
                            unsigned char* buf
                            int len
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��7��25�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

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
                char rbuf[] = "\r������q �˳�Զ�̼���״̬!\r\n$";
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
                char rbuf[] = "\r������q �˳�Զ�̼���״̬!\r\n$";
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
                char rbuf[] = "\r������������ȷ,������q �˳�Զ�̼���״̬!\r\n$";
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
 �� �� ��  : TelnetRunTraceSendStateProc
 ��������  : ������Ϣ���״̬�µĴ���
 �������  : int cpos
                            unsigned char* buf
                            int len
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��7��25�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

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
                char rbuf[] = "\r������q �˳�������Ϣ����״̬!\r\n$";
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
                char rbuf[] = "\r������q �˳�������Ϣ����״̬!\r\n$";
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
                char rbuf[] = "\r������������ȷ,������q �˳�������Ϣ����״̬!\r\n$";
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
 �� �� ��  : TelnetSIPDebugSendStateProc
 ��������  : SIP��Ϣ������Ϣ���״̬�µĴ���
 �������  : int cpos
                            unsigned char* buf
                            int len
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��9��4�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

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
                char rbuf[] = "\r������q �˳�Զ��SIP��Ϣ����״̬!\r\n$";
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
                char rbuf[] = "\r������q �˳�Զ��SIP��Ϣ����״̬!\r\n$";
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
                char rbuf[] = "\r������������ȷ,������q �˳�Զ��SIP��Ϣ����״̬!\r\n$";
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
 �� �� ��  : TelnetSystemShellStateProc
 ��������  : ϵͳShellģʽ�µĴ���
 �������  : int cpos
             unsigned char* buf
             int len
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2016��3��8��
    ��    ��   : ���
    �޸�����   : �����ɺ���

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
                char rbuf[] = "\r������q �˳�ϵͳshellģʽ!\r\n$";
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
                char rbuf[] = "\r��֧��TOP������������������������q �˳�ϵͳshellģʽ!\r\n$";
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
                char rbuf[] = "\r��������Ҫִ�е������������q �˳�ϵͳshellģʽ!\r\n$";
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }
        }
        else
        {
            FILE   *stream = NULL;
            char   buf[256] = {0};

            memset(buf, '\0', 256);  //��ʼ��buf,�������д�����뵽�ļ���
            stream = popen(TelnetClients[cpos].cmd, "r");   //����ls ��l���������� ͨ���ܵ���ȡ����r����������FILE* stream

            if (NULL != stream)
            {
                while ((fgets(buf, 256, stream)) != NULL)
                {
                    send(sock, buf, strlen(buf) - 1, 0);
                    send(sock, "\r\n", 2, 0);
                    memset(buf, '\0', 256);  //��ʼ��buf,�������д�����뵽�ļ���
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
                    char rbuf[] = "\r��������Ҫִ�е������������q �˳�ϵͳshellģʽ!\r\n$";
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
 �� �� ��  : TelnetConfigSetStateProc
 ��������  : ����״̬�µĴ���
 �������  : int cpos
                            unsigned char* buf
                            int len
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��7��25�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

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
                sprintf(curbuf, "\r��ǰDebug������:%d\r\n", g_CommonDbgLevel);
                char rbuf[] = "\r������Ҫ�趨��Debug����(1:All 2:Debug 3:Trace 4:Info 5:Warning 6:Error 7:Fatal 8:Off),�س���ȷ������,ֱ�ӻس����޸�\r\n$";
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
                sprintf(curbuf, "\r��ǰLog��־������:%d\r\n", g_SystemLogLevel);
                char rbuf[] = "\r������Ҫ�趨��Log��־����(1:NORMAL 2:WARNING 3:ERROR),�س���ȷ������,ֱ�ӻس����޸�\r\n$";
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
                    sprintf(curbuf, "\r��ǰ��־��¼���ļ���ʶ��:%s\r\n", (char*)"On");
                }
                else
                {
                    sprintf(curbuf, "\r��ǰ��־��¼���ļ���ʶ��:%s\r\n", (char*)"Off");
                }

                char rbuf[] = "\r������־��¼���ļ���ʶ(On/Off),�س���ȷ������,ֱ�ӻس����޸�\r\n$";
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
                    sprintf(curbuf, "\r��ǰ��־��¼�����ݿ��ʶ��:%s\r\n", (char*)"On");
                }
                else
                {
                    sprintf(curbuf, "\r��ǰ��־��¼�����ݿ��ʶ��:%s\r\n", (char*)"Off");
                }

                char rbuf[] = "\r������־��¼�����ݿ��ʶ(On/Off),�س���ȷ������,ֱ�ӻس����޸�\r\n$";
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
                sprintf(curbuf, "\r��ǰTelnet��¼�û�����:%s\r\n", pGblconf->spy_username);
                char rbuf[] = "\r�޸Ĵ˲������ڱ�������֮�󣬽����Զ��˳�Telnet��¼!!!\r\n\
                           \r������Ҫ�趨��Telnet��¼�û���,�س���ȷ������,ֱ�ӻس����޸�\r\n$";
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
                sprintf(curbuf, "\r��ǰTelnet��¼������:%s\r\n", pGblconf->spy_password);
                char rbuf[] = "\r�޸Ĵ˲������ڱ�������֮�󣬽����Զ��˳�Telnet��¼!!!\r\n\
                          \r������Ҫ�趨��Telnet��¼����,�س���ȷ������,ֱ�ӻس����޸�\r\n$";
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
                sprintf(curbuf, "\r��ǰTelnet��¼�˿���:%d\r\n", pGblconf->spy_port);
                char rbuf[] = "\r�޸Ĵ˲������ڱ�������֮�󣬽���ʹ����Telnet�û��˳���¼!!!\r\n\
            \r������Ҫ�趨��Telnet��¼�˿�,�س���ȷ������,ֱ�ӻس����޸�\r\n$";
                TelnetClients[cpos].state = TSCS_TELPORT_SET;
                send(sock, curbuf, strlen(curbuf), 0);
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }
            else
            {
                sprintf(curbuf, "\r��ǰTelnet��¼�˿���:%d\r\n", pGblconf->spy_port);
                char rbuf[] = "\r�޸Ĵ˲������ڱ�������֮�󣬽���ʹ����Telnet�û��˳���¼!!!\r\n\
            \r������Ҫ�趨��Telnet��¼�˿�,�س���ȷ������,ֱ�ӻس����޸�\r\n$";
                TelnetClients[cpos].state = TSCS_TELPORT_SET;
                send(sock, curbuf, strlen(curbuf), 0);
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }
        }
        else if ((!strcmp(TelnetClients[cpos].cmd, COMMOND_SAVE))
                 || (!strcmp(TelnetClients[cpos].cmd, "s")))
        {
            //char rbuf[] = "\rȷ�ϱ��������?('y' or 'n'):\r\n$";
            //TelnetClients[cpos].state = TSCS_CONFIG_SAVE;

            if (1 == g_Language)
            {
                char rbuf[] = "\rNot enabled\r\n$";
                TelnetClients[cpos].state = TSCS_CONFIG_SET;
                send(sock, rbuf, sizeof(rbuf) - 1, 0);
            }
            else
            {
                char rbuf[] = "\r��δ����\r\n$";
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
                sprintf(current_config, "\rDebug���Լ���:%d\r\n\
                            \r��־��¼���ļ�����:%s\r\n\
                            \r��־��¼�����ݿ⿪��:%s\r\n\
                            \rTelnet ��¼�û���:%s\r\n\
                            \rTelnet ��¼����:%s\r\n\
                            \rTelnet ��¼�˿�:%d\r\n$", g_CommonDbgLevel, strLog2File, strLog2DB, pGblconf->spy_username, pGblconf->spy_password, pGblconf->spy_port);
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
                char rbuf[] = "\r������������ȷ!\r\n$";
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
 �� �� ��  : TelnetSysRunInfoStateProc
 ��������  : ϵͳ������Ϣ�鿴�µĴ���
 �������  : int cpos
                            unsigned char* buf
                            int len
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��7��25�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

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
                char rbuf[] = "\r������������ȷ!\r\n$";
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
 �� �� ��  : TelnetSysCallTaskDetalStateProc
 ��������  : �鿴ϵͳ��������״̬�µĴ���
 �������  : int cpos
                            unsigned char* buf
                            int len
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��7��29�� ����һ
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

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
                        char rbuf[] = "\r������Ĳ�����������������!\r\n$";
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
 �� �� ��  : TelnetSysSIPUAInfoStateProc
 ��������  : �鿴SIPUA�Ự��Ϣ״̬�µĴ���
 �������  : int cpos
                            unsigned char* buf
                            int len
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��7��30�� ���ڶ�
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

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
                        char rbuf[] = "\r������Ĳ�����������������!\r\n$";
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
 �� �� ��  : TelnetSysDiagnoseStateProc
 ��������  : ϵͳ�����Ϣ�鿴�µĴ���
 �������  : int cpos
                            unsigned char* buf
                            int len
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��7��25�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

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
                char rbuf[] = "\r������Ҫ�ͷŵ�¼�������������س���ȷ�����룬����all���ʾ�ͷ����е�¼������\r\n$";
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
                char rbuf[] = "\r������Ҫ�ͷŵĺ��������������س���ȷ�����룬����all���ʾ�ͷ����еĺ�������\r\n$";
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
                char rbuf[] = "\r������Ҫֹͣ����ѲID���س���ȷ�����룬����all���ʾֹͣ���е���Ѳ����\r\n$";
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
                char rbuf[] = "\r������Ҫֹͣ��Ѳ��ID���س���ȷ�����룬����all���ʾֹͣ���е�Ѳ������\r\n$";
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
                char rbuf[] = "\r��ȷ��Ҫ�ͷ����е�SIP UA�Ի���Ϣ�� ?('y' or 'n'):\r\n$";
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
                char rbuf[] = "\r��ȷ��Ҫ�ͷ����еķ����ע����ϢUAS�� ?('y' or 'n'):\r\n$";
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
                char rbuf[] = "\r��ȷ��Ҫ�ͷ����еĿͻ���ע����ϢUAC�� ?('y' or 'n'):\r\n$";
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
                char rbuf[] = "\r������������ȷ!\r\n$";
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
 �� �� ��  : TelnetReleaseRecordTaskStateProc
 ��������  : ɾ��¼������״̬�µĴ���
 �������  : int cpos
                            unsigned char* buf
                            int len
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��7��31�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

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
                    char rbuf[] = "\r��ȷ��Ҫ�ͷ����е�¼�������� ?('y' or 'n'):\r\n$";
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
                        char rbuf[] = "\r��ȷ��Ҫ�ͷŸ���¼�������� ?('y' or 'n'):\r\n$";
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
                        char rbuf[] = "\r������Ĳ�����������������!\r\n$";
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
                char rbuf[] = "\r������Ҫ�ͷŵ�¼�������������س���ȷ�����룬����all���ʾ�ͷ����е�¼������\r\n$";
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
 �� �� ��  : TelnetReleaseRecordTaskConfirmStateProc
 ��������  : �ͷ�����¼������ȷ��״̬�µĴ���
 �������  : int cpos
                            unsigned char* buf
                            int len
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��7��30�� ���ڶ�
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

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
                    char rbuf[] = "\rCMSϵͳ�����ͷ����е�¼������...\r\n";
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
                    char rbuf1[] = "\rCMSϵͳ�ɹ��ͷ����е�¼������!\r\n$";
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
                char rbuf[] = "\rȡ���ͷ�¼������!\r\n$";
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
                char rbuf[] = "\r������y����n \r\n$";
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
 �� �� ��  : TelnetReleaseCallTaskStateProc
 ��������  : ɾ����������״̬�µĴ���
 �������  : int cpos
                            unsigned char* buf
                            int len
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��7��31�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

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
                    char rbuf[] = "\r��ȷ��Ҫ�ͷ����еĺ��������� ?('y' or 'n'):\r\n$";
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
                        char rbuf[] = "\r��ȷ��Ҫ�ͷŸ������������� ?('y' or 'n'):\r\n$";
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
                        char rbuf[] = "\r������Ĳ�����������������!\r\n$";
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
                char rbuf[] = "\r������Ҫ�ͷŵĺ��������������س���ȷ�����룬����all���ʾ�ͷ����еĺ�������\r\n$";
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
 �� �� ��  : TelnetReleaseCallTaskConfirmStateProc
 ��������  : �ͷź�������ȷ��״̬�µĴ���
 �������  : int cpos
                            unsigned char* buf
                            int len
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��7��30�� ���ڶ�
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

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
                    char rbuf[] = "\rCMSϵͳ�����ͷ����еĺ�������...\r\n";
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
                    char rbuf1[] = "\rCMSϵͳ�ɹ��ͷ����еĺ�������!\r\n$";
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
                char rbuf[] = "\rȡ���ͷź�������!\r\n$";
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
                char rbuf[] = "\r������y����n \r\n$";
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
 �� �� ��  : TelnetStopPollTaskStateProc
 ��������  : ֹͣ��Ѳ����״̬�µĴ���
 �������  : int cpos
             unsigned char* buf
             int len
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��8��7�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

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
                    char rbuf[] = "\r��ȷ��Ҫֹͣ���е���Ѳ������ ?('y' or 'n'):\r\n$";
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
                        char rbuf[] = "\r��ȷ��Ҫֹͣ����Ѳ������ ?('y' or 'n'):\r\n$";
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
                        char rbuf[] = "\r������Ĳ�����������������!\r\n$";
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
                char rbuf[] = "\r������Ҫֹͣ����ѲID���س���ȷ�����룬����all���ʾֹͣ���е���Ѳ����\r\n$";
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
 �� �� ��  : TelnetStopPollTaskConfirmStateProc
 ��������  : ֹͣ��Ѳ����ȷ��״̬�µĴ���
 �������  : int cpos
             unsigned char* buf
             int len
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��8��7�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

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
                    char rbuf[] = "\rCMSϵͳ����ֹͣ���е���Ѳ����...\r\n";
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
                    char rbuf1[] = "\rCMSϵͳ�ɹ�ֹͣ���е���Ѳ����!\r\n$";
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
                char rbuf[] = "\rȡ��ֹͣ��Ѳ����!\r\n$";
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
                char rbuf[] = "\r������y����n \r\n$";
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
 �� �� ��  : TelnetStopCruiseTaskStateProc
 ��������  : ֹͣѲ������״̬�µĴ���
 �������  : int cpos
             unsigned char* buf
             int len
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��8��7�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

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
                    char rbuf[] = "\r��ȷ��Ҫֹͣ���е�Ѳ�������� ?('y' or 'n'):\r\n$";
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
                        char rbuf[] = "\r��ȷ��Ҫֹͣ��Ѳ�������� ?('y' or 'n'):\r\n$";
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
                        char rbuf[] = "\r������Ĳ�����������������!\r\n$";
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
                char rbuf[] = "\r������Ҫֹͣ��Ѳ��ID���س���ȷ�����룬����all���ʾֹͣ���е�Ѳ������\r\n$";
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
 �� �� ��  : TelnetStopCruiseTaskConfirmStateProc
 ��������  : ֹͣѲ������ȷ��״̬�µĴ���
 �������  : int cpos
             unsigned char* buf
             int len
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��8��7�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

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
                    char rbuf[] = "\rCMSϵͳ����ֹͣ���е�Ѳ������...\r\n";
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
                    char rbuf1[] = "\rCMSϵͳ�ɹ�ֹͣ���е�Ѳ������!\r\n$";
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
                char rbuf[] = "\rȡ��ֹͣѲ������!\r\n$";
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
                char rbuf[] = "\r������y����n \r\n$";
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
 �� �� ��  : TelnetReleaseAllUADialogInfoConfirmStateProc
 ��������  : �ͷ�����SIPUA�Ự��Ϣȷ��״̬�µĴ���
 �������  : int cpos
             unsigned char* buf
             int len
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��7��30�� ���ڶ�
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

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
                char rbuf[] = "\rCMSϵͳ�����ͷ����е�SIP UA�Ự��Ϣ...\r\n";
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
                char rbuf1[] = "\rCMSϵͳ�ɹ��ͷ����е�SIP UA�Ự��Ϣ!\r\n$";
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
                char rbuf[] = "\rȡ���ͷ����е�SIP UA�Ự��Ϣ!\r\n$";
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
                char rbuf[] = "\r������y����n \r\n$";
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
 �� �� ��  : TelnetReleaseAllUASInfoConfirmStateProc
 ��������  : �ͷ����пͻ���ע����ϢUASȷ��״̬�µĴ���
 �������  : int cpos
             unsigned char* buf
             int len
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��7��30�� ���ڶ�
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

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
                char rbuf[] = "\rCMSϵͳ�����ͷ����е�UAS�����ע����Ϣ...\r\n";
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
                char rbuf1[] = "\rCMSϵͳ�ɹ��ͷ����е�UAS�����ע����Ϣ!\r\n$";
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
                char rbuf[] = "\rȡ���ͷ����е�UAS�����ע����Ϣ!\r\n$";
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
                char rbuf[] = "\r������y����n \r\n$";
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
 �� �� ��  : TelnetReleaseAllUACInfoConfirmStateProc
 ��������  : �ͷ����пͻ���ע����ϢUACȷ��״̬�µĴ���
 �������  : int cpos
             unsigned char* buf
             int len
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��7��30�� ���ڶ�
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

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
                char rbuf[] = "\rCMSϵͳ�����ͷ����е�UAC�ͻ���ע����Ϣ...\r\n";
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
                char rbuf1[] = "\rCMSϵͳ�ɹ��ͷ����е�UAC�ͻ���ע����Ϣ!\r\n$";
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
                char rbuf[] = "\rȡ���ͷ����е�UAC�ͻ���ע����Ϣ!\r\n$";
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
                char rbuf[] = "\r������y����n \r\n$";
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
 �� �� ��  : TelnetDebugLevelSetStateProc
 ��������  : Debug���Եȼ�����״̬�µĴ���
 �������  : int cpos
                            unsigned char* buf
                            int len
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��7��25�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

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
                        char rbuf[] = "\r������Ĳ�������ϵͳ������Ĭ��ֵ!\r\n$";
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
                            char rbuf[] = "\r�ɹ��޸�Debug���Եȼ�!\r\n$";
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
                            char rbuf[] = "\r�˲���δ�޸�!\r\n$";
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
                char rbuf[] = "\r�˲���δ�޸�!\r\n$";
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
 �� �� ��  : TelnetLogLevelSetStateProc
 ��������  : ��־�ȼ�����״̬�µĴ���
 �������  : int cpos
             unsigned char* buf
             int len
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��8��11�� ����һ
    ��    ��   : ���
    �޸�����   : �����ɺ���

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
                        char rbuf[] = "\r������Ĳ�������ϵͳ������Ĭ��ֵ!\r\n$";
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
                            char rbuf[] = "\r�ɹ���Log��־�ȼ�!\r\n$";
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
                            char rbuf[] = "\r�˲���δ�޸�!\r\n$";
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
                char rbuf[] = "\r�˲���δ�޸�!\r\n$";
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
 �� �� ��  : TelnetLog2FileSetStateProc
 ��������  : ��־��¼���ļ���������״̬�µĴ���
 �������  : int cpos
                            unsigned char* buf
                            int len
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��7��25�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

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
                        char rbuf[] = "\r��־�ļ���¼��ʶ�޸ĳɹ�!\r\n$";
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
                        char rbuf[] = "\r�˲���δ�޸�!\r\n$";
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
                        char rbuf[] = "\r��־�ļ���¼��ʶ�޸ĳɹ�!\r\n$";
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
                        char rbuf[] = "\r�˲���δ�޸�!\r\n$";
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
                    char rbuf[] = "\r��������������������!\r\n$";
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
                char rbuf[] = "\r�˲���δ�޸�!\r\n$";
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
 �� �� ��  : TelnetLog2DBSetStateProc
 ��������  : ��־��¼�����ݿ⿪������״̬�µĴ���
 �������  : int cpos
             unsigned char* buf
             int len
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��8��7�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

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
                        char rbuf[] = "\r��־���ݿ��¼��ʶ�޸ĳɹ�!\r\n$";
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
                        char rbuf[] = "\r�˲���δ�޸�!\r\n$";
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
                        char rbuf[] = "\r��־���ݿ��¼��ʶ�޸ĳɹ�!\r\n$";
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
                        char rbuf[] = "\r�˲���δ�޸�!\r\n$";
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
                    char rbuf[] = "\r��������������������!\r\n$";
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
                char rbuf[] = "\r�˲���δ�޸�!\r\n$";
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
 �� �� ��  : TelnetUserNameSetStateProc
 ��������  : ����Telnet�û���״̬�µĴ���
 �������  : int cpos
                            unsigned char* buf
                            int len
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��7��25�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

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
                    char rbuf[] = "\r�˲����Ѿ��޸ģ�������s �����������������֮���Զ�ע��!\r\n$";
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
                    char rbuf[] = "\r�˲����Ѿ��޸ģ�������s �����������������֮���Զ�ע��!\r\n$";
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
                    char rbuf[] = "\r�˲���δ�޸�!\r\n$";
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
                char rbuf[] = "\r�˲���δ�޸�!\r\n$";
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
 �� �� ��  : TelnetUserPasswordSetStateProc
 ��������  : Telnet�û������޸�״̬�Ĵ���
 �������  : int cpos
                            unsigned char* buf
                            int len
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��7��25�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

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
                    char rbuf[] = "\r�˲����Ѿ��޸ģ�������s �����������������֮���Զ�ע��!\r\n$";
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
                    char rbuf[] = "\r�˲����Ѿ��޸ģ�������s �����������������֮���Զ�ע��!\r\n$";
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
                    char rbuf[] = "\r�˲���δ�޸�!\r\n$";
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
                char rbuf[] = "\r�˲���δ�޸�!\r\n$";
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
 �� �� ��  : TelnetPortSetStateProc
 ��������  : Telnet��¼�˿ں��޸�״̬�Ĵ���
 �������  : int cpos
                            unsigned char* buf
                            int len
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��7��25�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

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
                    char rbuf[] = "\r�˲����Ѿ��޸ģ�������s �����������������֮���Զ�ע��!\r\n$";
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
                    char rbuf[] = "\r������Ĳ�������ϵͳ������Ĭ��ֵ2000 �˿�!\r\n$";
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
                    char rbuf[] = "\r�˲���δ�޸�!\r\n$";
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
                char rbuf[] = "\r�˲���δ�޸�!\r\n$";
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
 �� �� ��  : TelnetConfigSaveStateProc
 ��������  : ��������״̬�µĴ���
 �������  : int cpos
                            unsigned char* buf
                            int len
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��7��25�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

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
                    char rbuf[] = "\r��������ɹ�!\r\n$";
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
                    char rbuf[] = "\r��������ʧ��!\r\n$";
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
                char rbuf[] = "\r����δ����!\r\n$";
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
                char rbuf[] = "\r������y ����n \r\n$";
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
 �� �� ��  : TelnetStopSystemConfirmStateProc
 ��������  : ֹͣCMSϵͳȷ��״̬�µĴ���
 �������  : int cpos
                            unsigned char* buf
                            int len
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��7��25�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

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
                char rbuf[] = "\rCMS ϵͳ����ֹͣ...\r\n";
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
                char rbuf[] = "\rȡ��ֹͣCMSϵͳ!\r\n$";
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
                char rbuf[] = "\r������y ����n \r\n$";
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
 �� �� ��  : TelnetRestartSystemConfirmStateProc
 ��������  : ����CMSϵͳȷ��״̬�µĴ���
 �������  : int cpos
                            unsigned char* buf
                            int len
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��7��25�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

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
                char rbuf[] = "\rCMS ϵͳ��������...\r\n";
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
                char rbuf[] = "\rȡ������CMSϵͳ!\r\n$";
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
                char rbuf[] = "\r������y ����n \r\n$";
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
 �� �� ��  : TelnetLogoutConfirmStateProc
 ��������  : Telnetע����¼ȷ��״̬�µĴ���
 �������  : int cpos
                            unsigned char* buf
                            int len
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��7��25�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

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
                char rbuf[] = "\rȡ��ע��Telnet ��¼!\r\n$";
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
                char rbuf[] = "\r������y ����n \r\n$";
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
 �� �� ��  : TelnetExitConfirmStateProc
 ��������  : Telnet�˳���¼ȷ��״̬�µĴ���
 �������  : int cpos
                            unsigned char* buf
                            int len
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��7��25�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

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
                char rbuf[] = "\rȡ���˳�Telnet ��¼!\r\n$";
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
                char rbuf[] = "\r������y ����n \r\n$";
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
 �� �� ��  : telnetauth
 ��������  : telnet��¼��֤
 �������  : char* user
             char* password
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��7��25�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

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
 �� �� ��  : TelnetSend
 ��������  : telnetԶ�˷��ͺ���
 �������  : char* msg
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��7��25�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

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
 �� �� ��  : TelnetRunTraceSend
 ��������  : telnet������ϢԶ�˷��ͺ���
 �������  : char* msg
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��7��25�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

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
 �� �� ��  : TelnetSendSIPMessage
 ��������  : TelnetԶ�˷���SIP��Ϣ
 �������  : const char* fmt
             ...
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��9��4�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

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
