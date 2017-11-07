/******************************************************************************

                  版权所有 (C), 2001-2013, 金智视讯技术有限公司

 ******************************************************************************
  文 件 名   : gblconfig_proc.c
  版 本 号   : 初稿
  作    者   : yanghaifeng
  生成日期   : 2013年6月19日 星期三
  最近修改   :
  功能描述   : 全局配置信息处理
  函数列表   :
              gbl_conf_free
              gbl_conf_init
              gbl_conf_load
              gbl_conf_reload
              load_localnet_config
              load_system_config
              local_addr_add
              local_addr_find_by_type
              local_addr_free
              local_addr_get
              local_addr_init
              local_addr_remove
              local_cms_id_get
  修改历史   :
  1.日    期   : 2013年6月19日 星期三
    作    者   : yanghaifeng
    修改内容   : 创建文件

******************************************************************************/

/*----------------------------------------------*
 * 包含头文件                                   *
 *----------------------------------------------*/
#include <stdio.h>
#include <string.h>

#ifdef WIN32
#include <winsock.h>
#include <sys/types.h>
#else
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <errno.h>

#include <asm/types.h>
#include <netinet/ether.h>
#include <net/if.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#endif

#include <time.h>
#include <math.h>

#include <osipparser2/osip_port.h>
#include <osipparser2/osip_md5.h>
#include "sipapp/sipauth.inc"

#include "common/gblfunc_proc.inc"
#include "common/gblconfig_proc.inc"
#include "common/log_proc.inc"
#include "common/systeminfo_proc.inc"

#include "device/device_srv_proc.inc"
#include "route/route_info_mgn.inc"
#include "route/platform_thread_proc.inc"

#include "config/telnetd.inc"
#include "common/db_proc.h"

/*----------------------------------------------*
 * 外部变量说明                                 *
 *----------------------------------------------*/
extern char g_strDBThreadPara[4][100];
extern char g_StrCon[2][100];
extern char g_StrConLog[2][100];

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
extern int g_SystemLogLevel;    /* 系统日志等级 */
extern int g_SystemLog2DBLevel; /* 系统日志记录到数据库的等级 */
extern int g_IsLog2File;
extern int g_IsLog2DB;
extern int g_IsLogSIPMsg;
extern BOARD_NET_ATTR  g_BoardNetConfig;  /* 单板IP 地址配置 */
extern DBOper g_DBOper;
extern int cms_run_status;  /* 0:没有运行,1:正常运行 */

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
gbl_conf_t* pGblconf = NULL;              /* 全局配置信息 */
char old_spy_username[32];                /* telnet 登录用户名 */
char old_spy_password[32];                /* telnet 登录密码 */
int old_spy_port;                         /* telnet 服务端口*/
int g_IsPay = 1;                          /* 是否付费，默认1 */
int g_IsSubscribe = 1;                    /* 是否发送订阅，默认1 */
unsigned int g_RegistrationLimit = 0;     /* 注册设备限制数 */
int g_AlarmMsgSendToUserFlag = 1;         /* 报警消息是否发送给用户,默认发送 */

int g_AlarmMsgSendToRouteFlag = 0;        /* 报警消息是否发送给上级路由，默认不发送 */

int g_LocalMediaTransferFlag = 1;         /* 下级媒体流是否经过本地TSU转发,默认转发 */
int g_DECMediaTransferFlag = 1;           /* 下级媒体切电视墙是否经过本地TSU转发,默认转发 */
int g_RouteMediaTransferFlag = 1;         /* 上级第三方平台是否有媒体转发功能,默认有 */
int g_MMSEnableFlag = 0;                  /* SX、RX是否启用MMS功能，默认不启用 */


#ifdef LANG_EN
int g_Language = 1;                       /* 语言，0:中文；1:英文，默认英文 */
#else
int g_Language = 0;                       /* 语言，0:中文；1:英文，默认中文 */
#endif

int g_AnalysisSubGroupFlag = 0;           /* 是否解析下级分组，0:不解析；1:解析行政区域，2:解析分组，默认不解析 */
int g_LogQueryBufferSize = 0;             /* 日志缓存队列大小，默认0 */

#ifdef MULTI_THR
osip_mutex_t* g_GblconfGroupLock = NULL;   /* 全局的本地分组信息锁 */
osip_mutex_t* g_GblconfGroupMapLock = NULL;/* 全局的本地分组关系信息锁 */
#endif

/*----------------------------------------------*
 * 模块级变量                                   *
 *----------------------------------------------*/
time_t Cfg_lmt;     /* 当前配置文件修改时间 */
time_t Key_lmt;     /* 当前授权文件修改时间 */
time_t Key_Tmp_lmt; /* 当前临时授权文件修改时间 */
int db_GroupInfo_reload_mark = 0;      /* 分组信息数据库更新标识:0:不需要更新，1:需要更新数据库 */
int db_GroupMapInfo_reload_mark = 0;   /* 分组关系信息数据库更新标识:0:不需要更新，1:需要更新数据库 */

/*----------------------------------------------*
 * 常量定义                                     *
 *----------------------------------------------*/
#define  CMS_CONF_LEN          512
#define  CMS_IP_STR_LEN        16

/*----------------------------------------------*
 * 宏定义                                       *
 *----------------------------------------------*/
#define  CMS_CONFIG_FILE            "/config/cmscof.cfg"
#define  CMS_CONFIG_TMP_FILE        "/config/cmscof_tmp.cfg"
#define  CMS_KEY_FILENAME           "/config/key.dat"
#define  CMS_KEY_TMP_FILENAME       "/config/key_tmp.dat"

#define  NTP_CONFIG_FILE            "/config/ntp.conf"
#define  NTPDATE_SH_FILE            "/config/ntpdate.sh"
#define  CMS_WEB_CONFIG_FILE        "/config/webapp.cfg"
#define  CMS_WEB_CONFIG_TMP_FILE    "/config/webapp_tmp.cfg"

#define CONF_BOARD_ID ("cmsid")                                     /* 单板编号 */

#define CONF_DB_SERVER_IP ("dbip")                                  /* 数据库IP地址 */
#define CONF_SDB_SERVER_IP ("sdbip")                                /* 从数据库IP地址 */
#define CONF_ALARM_SERVER_IP ("alarmip")                            /* 告警服务器IP地址 */
#define CONF_NTP_SERVER_IP ("ntpip")                                /* NTP服务器IP地址 */

#define CONF_VIDEO_NET_IP ("vidip")                                 /* 视频网IP地址 */
#define CONF_VIDEO_NET_MASK ("vidmask")                             /* 视频网IP地址掩码 */
#define CONF_VIDEO_NET_GATEWAY ("vidgateway")                       /* 视频网IP地址网关 */
#define CONF_VIDEO_NET_ETH ("videth")                               /* 视频网IP地址网口 */
#define CONF_VIDEO_NET_PORT ("vidport")                             /* 视频网端口号 */
#define CONF_VIDEO_NET_FLAG ("vidflag")                             /* 视频网是否启用 */

#define CONF_DEVICE_NET_IP ("devip")                                /* 设备网IP地址 */
#define CONF_DEVICE_NET_MASK ("devmask")                            /* 设备网IP地址掩码 */
#define CONF_DEVICE_NET_GATEWAY ("devgateway")                      /* 设备网IP地址网关 */
#define CONF_DEVICE_NET_ETH ("deveth")                              /* 设备网IP地址网口 */
#define CONF_DEVICE_NET_PORT ("devport")                            /* 设备网端口号 */
#define CONF_DEVICE_NET_FLAG ("devflag")                            /* 设备网是否启用 */

#define CONF_VIDEO_COMMON_NET_IP ("cvidip")                         /* 视频网公共IP地址 */
#define CONF_VIDEO_SLAVE_NET_IP ("svidip")                          /* 视频网备机IP地址 */

#define CONF_DEVICE_COMMON_NET_IP ("cdevip")                        /* 设备网公共IP地址*/
#define CONF_DEVICE_SLAVE_NET_IP ("sdevip")                         /* 设备网备机IP地址 */

#define CONF_MEENABLE_FLAG ("msenable")                             /* 主备是否启用 */

#define CONF_AUTH_FLAG ("authenable")                               /* 认证标识 */
#define CONF_PRESET_BACK_TIME ("presetbacktime")                    /* 预置位自动归位时间 */
#define CONF_DEVICE_UNLOCK_TIME ("deviceunLocktime")                /* 点位自动解锁时间 */

#define CONF_REGISTER_INTERVAL ("registerinterval")                 /* 没有注册成功情况下重新注册间隔时间 */
#define CONF_REGISTER_EXPIRE ("registerexpire")                     /* 注册超时时间 */
#define CONF_SESSION_EXPIRE ("sessionexpire")                       /* 会话超时时间 */
#define CONF_SUBSCRIBE_EXPIRE ("subscribeexpire")                   /* 订阅超时时间 */
#define CONF_KEEP_ALIVE_INTERVAL ("keepaliveinterval")              /* 保活间隔时间 */
#define CONF_FAILED_KEEP_ALIVE_COUNT ("failkeepalivecount")         /* 保活失败次数 */
#define CONF_ALARM_DURATION ("alarmduration")                       /* 告警延时时间 */
#define CONF_KEEPALIVE_EXPIRE ("keepaliveexpire")                   /* 保活超时时间 */

#define CONF_CMS_NAME    ("cmsname")                                /* cms别名 */

#define CONF_SPY_USERNAME    ("telnetusername")                     /* telnet 登录用户名 */
#define CONF_SPY_PASSWORD    ("telnetpassword")                     /* telnet 登录密码 */
#define CONF_SPY_PORT        ("telnetport")                         /* telnet 端口号 */

#define CONF_DBG_LOG_LEVEL ("dbgloglevel")                          /* 调试日志打印等级 */
#define CONF_RUN_LOG_LEVEL ("runloglevel")                          /* 运行日志打印等级 */
#define CONF_RUN_LOG_2DB_LEVEL ("runlog2dblevel")                   /* 运行日志记录到数据库等级 */
#define CONF_LOG2FILE_FLAG ("log2file")                             /* 日志记录到文件开关 */
#define CONF_LOG2DB_FLAG ("log2db")                                 /* 日志记录到数据库开关 */
#define CONF_LOGSIPMSG_FLAG ("logsipmsg")                           /* 是否记录SIP Message开关 */
#define CONF_IS_SUBSCRIBE ("issubscribe")                           /* 是否订阅 */

#define CONF_ALARM_SEND_TO_USER_FLAG ("alarmsendtouserflag")        /* 报警消息发送给用户标识 */
#define CONF_ALARM_SEND_TO_ROUTE_FLAG ("alarmsendtorouteflag")      /* 报警消息发送给上级路由标识 */

#define CONF_MEDIA_TRANSFER_FLAG ("mediatransferflag")              /* 下级媒体是否经过本地媒体服务器转发 */
#define CONF_DEC_MEDIA_TRANSFER_FLAG ("decmediatransferflag")       /* 下级媒体切电视墙是否经过本地媒体服务器转发 */
#define CONF_ROUTE_MEDIA_TRANS_FLAG ("routemediatransflag")         /* 上级第三方平台是否支持媒体转发，默认支持 */

#define CONF_IS_PAY ("ispay")                                       /* 是否付费 */
#define CONF_LANGUAGE ("language")                                  /* 系统语言 */
#define CONF_MMS_ENABLE_FLAG ("mmsenableflag")                      /* mms是否启用，默认启用 */
#define CONF_ANALYSIS_SUBGROUP_FLAG ("analysissubgroupflag")        /* 下级分组信息解析标识，默认不解析 */
#define CONF_LOG_BUFFER_SIZE ("logquerybuffersize")                 /* 日志缓存队列大小，默认10000 */

#define CONF_SHDB_AGENTID ("shdbagentid")                           /* 上海地标监管平台注册ID */
#define CONF_SHDB_SEVVERIP ("shdbserverip")                         /* 上海地标监管平台服务器IP */
#define CONF_SHDB_PREX_SEC ("shdbprexsec")                          /* 上海地标报警图片上传前N秒 */
#define CONF_SHDB_NEXT_SEC ("shdbnextsec")                          /* 上海地标报警图片上传后M秒 */
#define CONF_SHDB_INTERVAL_SEC ("shdbintervalsec")                  /* 上海地标报警图片上传间隔P秒 */
#define CONF_SYSTEM_EXIT_FLAG ("sysexitflag")                       /* 系统退出标识，0，正常退出，1异常退出 */
#define CONF_SHOW_CODE ("ShowCode")                                 /* 系统模式，1，国标模式，0非国标模式 */


/*****************************************************************************
 函 数 名  : ip_pair_init
 功能描述  : 本地IP地址结构初始化
 输入参数  : ip_pair_t** ip_pair
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年8月12日 星期二
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int ip_pair_init(ip_pair_t** ip_pair)
{
    *ip_pair = (ip_pair_t*)smalloc(sizeof(ip_pair_t));

    if (*ip_pair == NULL)
    {
        return -1;
    }

    (*ip_pair)->eth_name[0] = '\0';
    (*ip_pair)->ip_type = IP_ADDR_NULL;
    (*ip_pair)->local_ip[0] = '\0';
    (*ip_pair)->local_port = 5060;

    return 0;
}

/*****************************************************************************
 函 数 名  : ip_pair_free
 功能描述  : 本地IP地址结构释放
 输入参数  : ip_pair_t* ip_pair
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年8月12日 星期二
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
void ip_pair_free(ip_pair_t* ip_pair)
{
    if (ip_pair == NULL)
    {
        return;
    }

    memset(ip_pair->eth_name, 0, MAX_IP_LEN);
    ip_pair->ip_type = IP_ADDR_NULL;
    memset(ip_pair->local_ip, 0, MAX_IP_LEN);
    ip_pair->local_port = 5060;

    osip_free(ip_pair);
    ip_pair = NULL;

    return;
}

/*****************************************************************************
 函 数 名  : ip_pair_add
 功能描述  : 本地IP地址添加
 输入参数  : char* eth_name
             ip_addr_type_t ip_type
             char* local_ip
             int local_port
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年8月12日 星期二
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int ip_pair_add(char* eth_name, ip_addr_type_t ip_type, char* local_ip, int local_port)
{
    ip_pair_t* pIPaddr = NULL;
    int i = 0;

    if (pGblconf == NULL || pGblconf->pLocalIPAddrList == NULL)
    {
        return -1;
    }

    if (NULL == eth_name || NULL == local_ip || local_port <= 0)
    {
        return -1;
    }

    i = ip_pair_init(&pIPaddr);

    if (i != 0)
    {
        return -1;
    }

    memset(pIPaddr->eth_name, 0, MAX_IP_LEN);
    osip_strncpy(pIPaddr->eth_name, eth_name, MAX_IP_LEN);

    pIPaddr->ip_type = ip_type;

    memset(pIPaddr->local_ip, 0, MAX_IP_LEN);
    osip_strncpy(pIPaddr->local_ip, local_ip, MAX_IP_LEN);

    pIPaddr->local_port = local_port;

    i = osip_list_add(pGblconf->pLocalIPAddrList, pIPaddr, -1); /* add to list tail */

    if (i == -1)
    {
        ip_pair_free(pIPaddr);
        pIPaddr = NULL;
        return -1;
    }

    return i - 1;
}

/*****************************************************************************
 函 数 名  : ip_pair_clone
 功能描述  : IP地址拷贝
 输入参数  : const ip_pair_t* uparam
             ip_pair_t** dest
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年8月12日 星期二
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int ip_pair_clone(const ip_pair_t* uparam, ip_pair_t** dest)
{
    int i;
    ip_pair_t* up = NULL;

    *dest = NULL;

    if (uparam == NULL)
    {
        return -1;
    }

    i = ip_pair_init(&up);

    if (i != 0)                   /* allocation failed */
    {
        return i;
    }

    memset(up->eth_name, 0, MAX_IP_LEN);
    osip_strncpy(up->eth_name, uparam->eth_name, MAX_IP_LEN);

    up->ip_type = uparam->ip_type;

    memset(up->local_ip, 0, MAX_IP_LEN);
    osip_strncpy(up->local_ip, uparam->local_ip, MAX_IP_LEN);

    up->local_port = uparam->local_port;

    *dest = up;

    return 0;
}

#if DECS("全局逻辑设备分组")
/*****************************************************************************
 函 数 名  : primary_group_init
 功能描述  : 分组信息结构初始化
 输入参数  : primary_group_t** primary_group
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年9月2日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int primary_group_init(primary_group_t** primary_group)
{
    *primary_group = (primary_group_t*)smalloc(sizeof(primary_group_t));

    if (*primary_group == NULL)
    {
        return -1;
    }

    (*primary_group)->group_id[0] = '\0';
    (*primary_group)->group_name[0] = '\0';
    (*primary_group)->civil_code[0] = '\0';
    (*primary_group)->group_code[0] = '\0';
    (*primary_group)->parent_code[0] = '\0';
    (*primary_group)->iNeedToUpLoad = 0;
    (*primary_group)->del_mark = 0;

    return 0;
}

/*****************************************************************************
 函 数 名  : primary_group_free
 功能描述  : 分组结构信息释放
 输入参数  : primary_group_t* primary_group
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年9月2日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
void primary_group_free(primary_group_t* primary_group)
{
    if (primary_group == NULL)
    {
        return;
    }

    memset(primary_group->group_id, 0, 36);
    memset(primary_group->group_name, 0, 68);
    memset(primary_group->civil_code, 0, 12);
    memset(primary_group->group_code, 0, 24);
    memset(primary_group->parent_code, 0, 24);
    primary_group->iNeedToUpLoad = 0;
    primary_group->del_mark = 0;

    osip_free(primary_group);
    primary_group = NULL;

    return;
}

/*****************************************************************************
 函 数 名  : get_primary_group
 功能描述  : 根据GroupID获取分组信息
 输入参数  : char* group_id
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年9月2日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
primary_group_t* get_primary_group(char* group_id)
{
    primary_group_t* pPrimaryGroup = NULL;
    PrimaryGroup_Iterator Itr;

    if (NULL == group_id || pGblconf == NULL)
    {
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "get_primary_group() exit---: Param Error \r\n");
        return NULL;
    }

    PRIMARY_GROUP_SMUTEX_LOCK();

    if (pGblconf->PrimaryGroupList.size() <= 0)
    {
        PRIMARY_GROUP_SMUTEX_UNLOCK();
        return NULL;
    }

    Itr = pGblconf->PrimaryGroupList.find(group_id);

    if (Itr == pGblconf->PrimaryGroupList.end())
    {
        PRIMARY_GROUP_SMUTEX_UNLOCK();
        return NULL;
    }
    else
    {
        pPrimaryGroup = Itr->second;
        PRIMARY_GROUP_SMUTEX_UNLOCK();
        return pPrimaryGroup;
    }

    PRIMARY_GROUP_SMUTEX_UNLOCK();
    return NULL;
}

/*****************************************************************************
 函 数 名  : primary_group_add
 功能描述  : 添加全局分组信息到队列
 输入参数  : char* group_id
             char* group_name
             char* civil_code
             char* group_code
             char* parent_code
             int sort_id
             char* parent_id
             int iNeedToUpLoad
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年9月2日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int primary_group_add(char* group_id, char* group_name, char* civil_code, char* group_code, char* parent_code, int sort_id, char* parent_id, int iNeedToUpLoad)
{
    int i = 0;
    int change_flag = 0;
    primary_group_t* pPrimaryGroup = NULL;
    char strOldGroupCode[24] = {0};
    char strOldGroupName[68] = {0};
    char strOldCivilCode[12] = {0};
    char strOldParentCode[24] = {0};
    char strOldParentID[36] = {0};
    int iOldSortID = 0;
    int iOldNeedToUpLoad = 0;

    if (pGblconf == NULL)
    {
        return -1;
    }

    if (NULL == group_id || NULL == group_name || NULL == civil_code || NULL == group_code)
    {
        return -1;
    }

    pPrimaryGroup = get_primary_group(group_id);

    if (NULL == pPrimaryGroup)
    {
        i = primary_group_init(&pPrimaryGroup);

        if (i != 0)
        {
            return -1;
        }

        /* 分组ID */
        memset(pPrimaryGroup->group_id, 0, 36);
        osip_strncpy(pPrimaryGroup->group_id, group_id, 32);

        /* 分组名称 */
        memset(pPrimaryGroup->group_name, 0, 68);

        if (NULL != group_name)
        {
            osip_strncpy(pPrimaryGroup->group_name, group_name, 64);
        }

        /* 分组父ID */
        memset(pPrimaryGroup->parent_id, 0, 36);

        if (NULL != parent_id)
        {
            osip_strncpy(pPrimaryGroup->parent_id, parent_id, 32);
        }

        /* 所属的行政区域 */
        memset(pPrimaryGroup->civil_code, 0, 12);

        if (NULL != civil_code)
        {
            osip_strncpy(pPrimaryGroup->civil_code, civil_code, 8);
        }

        /* 虚拟组织编码 */
        memset(pPrimaryGroup->group_code, 0, 24);

        if (NULL != group_code)
        {
            osip_strncpy(pPrimaryGroup->group_code, group_code, 20);
        }

        /* 虚拟组织父编码 */
        memset(pPrimaryGroup->parent_code, 0, 24);

        if (NULL != parent_code)
        {
            osip_strncpy(pPrimaryGroup->parent_code, parent_code, 20);
        }

        /* 排序ID */
        pPrimaryGroup->iSortID = sort_id;

        /* 是否需要上传 */
        pPrimaryGroup->iNeedToUpLoad = iNeedToUpLoad;

        PRIMARY_GROUP_SMUTEX_LOCK();
        pGblconf->PrimaryGroupList[group_id] = pPrimaryGroup;
        PRIMARY_GROUP_SMUTEX_UNLOCK();

        //printf("\r\nprimary_group_add() add:group_id=%s, group_name=%s, group_code=%s, i=%d", pPrimaryGroup->group_id, pPrimaryGroup->group_name, pPrimaryGroup->group_code, i);
        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "primary_group_add() osip_list_add:group_id=%s, group_name=%s, civil_code=%s, group_code=%s, parent_code=%s, i=%d \r\n", pPrimaryGroup->group_id, pPrimaryGroup->group_name, pPrimaryGroup->civil_code, pPrimaryGroup->parent_code, pPrimaryGroup->group_code, i);

        if (i == -1)
        {
            primary_group_free(pPrimaryGroup);
            pPrimaryGroup = NULL;
            return -1;
        }

        if (1 == cms_run_status)
        {
            /* 发送增加通知给第三方平台 */
            i = SendNotifyGroupCatalogTo3PartyRouteCMS(pPrimaryGroup->group_code, pPrimaryGroup->group_name, pPrimaryGroup->parent_code, 0);

            if (i > 0)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "发送逻辑设备分组增加通知给上级第三方平台:逻辑设备分组ID=%s, 分组名称=%s", pPrimaryGroup->group_code, pPrimaryGroup->group_name);
            }

            /* 发送增加通知给自己平台 */
            i = SendNotifyGroupCatalogToOwnerRouteCMS(pPrimaryGroup->group_id, pPrimaryGroup->group_name, pPrimaryGroup->parent_id, pPrimaryGroup->iSortID, 0);

            if (i > 0)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "发送逻辑设备分组增加通知给上级平台:逻辑设备分组ID=%s, 分组名称=%s", pPrimaryGroup->group_code, pPrimaryGroup->group_name);
            }
        }

        return i - 1;
    }
    else
    {
        pPrimaryGroup->del_mark = 0;

        /* 更新内存 */
        /* 分组名称 */
        memset(strOldGroupName, 0, 68);
        osip_strncpy(strOldGroupName, pPrimaryGroup->group_name, 64);

        memset(pPrimaryGroup->group_name, 0, 68);

        if (NULL != group_name)
        {
            osip_strncpy(pPrimaryGroup->group_name, group_name, 64);
        }

        /* 分组父ID */
        memset(strOldParentID, 0, 36);
        osip_strncpy(strOldParentID, pPrimaryGroup->parent_id, 32);

        memset(pPrimaryGroup->parent_id, 0, 36);

        if (NULL != parent_id)
        {
            osip_strncpy(pPrimaryGroup->parent_id, parent_id, 32);
        }

        /* 所属的行政区域 */
        memset(strOldCivilCode, 0, 12);
        osip_strncpy(strOldCivilCode, pPrimaryGroup->civil_code, 8);

        memset(pPrimaryGroup->civil_code, 0, 12);

        if (NULL != civil_code)
        {
            osip_strncpy(pPrimaryGroup->civil_code, civil_code, 8);
        }

        /* 虚拟组织编码 */
        memset(strOldGroupCode, 0, 24);
        osip_strncpy(strOldGroupCode, pPrimaryGroup->group_code, 20);

        memset(pPrimaryGroup->group_code, 0, 24);

        if (NULL != group_code)
        {
            osip_strncpy(pPrimaryGroup->group_code, group_code, 20);
        }

        /* 虚拟组织父编码 */
        memset(strOldParentCode, 0, 24);
        osip_strncpy(strOldParentCode, pPrimaryGroup->parent_code, 20);

        memset(pPrimaryGroup->parent_code, 0, 24);

        if (NULL != parent_code)
        {
            osip_strncpy(pPrimaryGroup->parent_code, parent_code, 20);
        }

        /* 排序ID */
        iOldSortID = pPrimaryGroup->iSortID;
        pPrimaryGroup->iSortID = sort_id;

        /* 是否需要上传 */
        iOldNeedToUpLoad = pPrimaryGroup->iNeedToUpLoad;
        pPrimaryGroup->iNeedToUpLoad = iNeedToUpLoad;

        if (pPrimaryGroup->iNeedToUpLoad != iOldNeedToUpLoad)
        {
            if (0 == pPrimaryGroup->iNeedToUpLoad && 1 == iOldNeedToUpLoad) /* 删除 */
            {
                if (1 == cms_run_status)
                {
                    /* 删除 */
                    i = SendNotifyGroupCatalogTo3PartyRouteCMS(pPrimaryGroup->group_code, pPrimaryGroup->group_name, pPrimaryGroup->parent_code, 1);

                    if (i > 0)
                    {
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "发送逻辑设备分组删除通知给上级第三方平台:逻辑设备分组ID=%s, 分组名称=%s", pPrimaryGroup->group_code, pPrimaryGroup->group_name);
                    }
                }
            }
            else if (1 == pPrimaryGroup->iNeedToUpLoad && 0 == iOldNeedToUpLoad) /* 增加 */
            {
                if (1 == cms_run_status)
                {
                    /* 增加 */
                    i = SendNotifyGroupCatalogTo3PartyRouteCMS(pPrimaryGroup->group_code, pPrimaryGroup->group_name, pPrimaryGroup->parent_code, 0);

                    if (i > 0)
                    {
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "发送逻辑设备分组增加通知给上级第三方平台:逻辑设备分组ID=%s, 分组名称=%s", pPrimaryGroup->group_code, pPrimaryGroup->group_name);
                    }
                }
            }
        }
        else
        {
            if (1 == pPrimaryGroup->iNeedToUpLoad && 1 == iOldNeedToUpLoad)
            {
                if (0 != sstrcmp(pPrimaryGroup->group_code, strOldGroupCode))
                {
                    change_flag = 1;
                }
                else if (0 != sstrcmp(pPrimaryGroup->group_name, strOldGroupName)
                         || 0 != sstrcmp(pPrimaryGroup->civil_code, strOldCivilCode)
                         || 0 != sstrcmp(pPrimaryGroup->parent_code, strOldParentCode))
                {
                    change_flag = 2;
                }

                if (1 == cms_run_status)
                {
                    /* 发送通知给第三方平台 */
                    if (1 == change_flag)
                    {
                        /* 先删除，后增加 */
                        i |= SendNotifyGroupCatalogTo3PartyRouteCMS(strOldGroupCode, pPrimaryGroup->group_name, pPrimaryGroup->parent_code, 1);

                        i |= SendNotifyGroupCatalogTo3PartyRouteCMS(pPrimaryGroup->group_code, pPrimaryGroup->group_name, pPrimaryGroup->parent_code, 0);

                        if (i > 0)
                        {
                            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "发送逻辑设备分组变化通知给上级第三方平台:逻辑设备分组ID=%s, 分组名称=%s", pPrimaryGroup->group_code, pPrimaryGroup->group_name);
                        }
                    }
                    else if (2 == change_flag)
                    {
                        /* 修改 */
                        i = SendNotifyGroupCatalogTo3PartyRouteCMS(pPrimaryGroup->group_code, pPrimaryGroup->group_name, pPrimaryGroup->parent_code, 2);

                        if (i > 0)
                        {
                            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "发送逻辑设备分组修改通知给上级第三方平台:逻辑设备分组ID=%s, 分组名称=%s", pPrimaryGroup->group_code, pPrimaryGroup->group_name);
                        }
                    }
                }

                change_flag = 0;
            }
        }

        /* 判断是否需要发送给本级的上级平台 */
        if (0 != sstrcmp(pPrimaryGroup->group_name, strOldGroupName)
            || 0 != sstrcmp(pPrimaryGroup->parent_id, strOldParentID)
            || pPrimaryGroup->iSortID != iOldSortID)
        {
            if (1 == cms_run_status)
            {
                /* 发送通知给自己平台 */
                i = SendNotifyGroupCatalogToOwnerRouteCMS(pPrimaryGroup->group_id, pPrimaryGroup->group_name, pPrimaryGroup->parent_id, pPrimaryGroup->iSortID, 2);

                if (i > 0)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "发送逻辑设备分组修改通知给上级平台:逻辑设备分组ID=%s, 分组名称=%s", pPrimaryGroup->group_code, pPrimaryGroup->group_name);
                }
            }
        }

        return 0;
    }
}

/*****************************************************************************
 函 数 名  : primary_group_info_list_lock
 功能描述  : 全局设备分组信息队列锁定
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月11日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int primary_group_info_list_lock()
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_GblconfGroupLock == NULL)
    {
        //DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "primary_group_info_list_lock() exit---: Param Error \r\n");
        return -1;
    }

    iRet = osip_mutex_lock((struct osip_mutex*)g_GblconfGroupLock);

#endif

    return iRet;
}

/*****************************************************************************
 函 数 名  : primary_group_info_list_unlock
 功能描述  : 全局设备分组信息队列解除锁定
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月11日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int primary_group_info_list_unlock()
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_GblconfGroupLock == NULL)
    {
        //DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "primary_group_info_list_unlock() exit---: Param Error \r\n");
        return -1;
    }

    iRet = osip_mutex_unlock((struct osip_mutex*)g_GblconfGroupLock);

#endif

    return iRet;
}

/*****************************************************************************
 函 数 名  : debug_GBDevice_info_list_lock
 功能描述  : 全局设备分组信息队列锁定
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月11日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int debug_primary_group_info_list_lock(const char* file, int line, const char* func)
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_GblconfGroupLock == NULL)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "debug_primary_group_info_list_lock() exit---: Param Error \r\n");
        return -1;
    }

    iRet = osip_debug_mutex_lock((struct osip_mutex*)g_GblconfGroupLock, file, line, func);

#endif

    return iRet;
}

/*****************************************************************************
 函 数 名  : debug_GBDevice_info_list_unlock
 功能描述  : 全局设备分组信息队列解除锁定
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月11日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int debug_primary_group_info_list_unlock(const char* file, int line, const char* func)
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_GblconfGroupLock == NULL)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "debug_primary_group_info_list_unlock() exit---: Param Error \r\n");
        return -1;
    }

    iRet = osip_debug_mutex_unlock((struct osip_mutex*)g_GblconfGroupLock, file,  line, func);

#endif

    return iRet;
}

/*****************************************************************************
 函 数 名  : SetGblLogicDeviceGroupDelFlag
 功能描述  : 设置全局分组信息的删除标识
 输入参数  : int iDelFlag
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年9月6日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int SetGblLogicDeviceGroupDelFlag(int iDelFlag)
{
    primary_group_t* pPrimaryGroup = NULL;
    PrimaryGroup_Iterator Itr;

    if (pGblconf == NULL)
    {
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "SetGblLogicDeviceGroupDelFlag() exit---: Param Error \r\n");
        return -1;
    }

    PRIMARY_GROUP_SMUTEX_LOCK();

    if (pGblconf->PrimaryGroupList.size() <= 0)
    {
        PRIMARY_GROUP_SMUTEX_UNLOCK();
        return NULL;
    }

    for (Itr = pGblconf->PrimaryGroupList.begin(); Itr != pGblconf->PrimaryGroupList.end(); Itr++)
    {
        pPrimaryGroup = Itr->second;

        if (NULL == pPrimaryGroup || pPrimaryGroup->group_id[0] == '\0' || pPrimaryGroup->group_code[0] == '\0')
        {
            continue;
        }

        pPrimaryGroup->del_mark = iDelFlag;
    }

    PRIMARY_GROUP_SMUTEX_UNLOCK();
    return 0;
}

/*****************************************************************************
 函 数 名  : DelGblLogicDeviceGroupInfoByDelMark
 功能描述  : 根据删除标识删除全局分组信息
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年9月6日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int DelGblLogicDeviceGroupInfoByDelMark()
{
    int i = 0;
    int index = 0;
    primary_group_t* pPrimaryGroup = NULL;
    vector<string> GroupIDVector;
    PrimaryGroup_Iterator Itr;

    if (pGblconf == NULL)
    {
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "DelGblLogicDeviceGroupInfoByDelMark() exit---: Param Error \r\n");
        return -1;
    }

    GroupIDVector.clear();

    PRIMARY_GROUP_SMUTEX_LOCK();

    if (pGblconf->PrimaryGroupList.size() <= 0)
    {
        PRIMARY_GROUP_SMUTEX_UNLOCK();
        return NULL;
    }

    for (Itr = pGblconf->PrimaryGroupList.begin(); Itr != pGblconf->PrimaryGroupList.end(); Itr++)
    {
        pPrimaryGroup = Itr->second;

        if (NULL == pPrimaryGroup || pPrimaryGroup->group_id[0] == '\0' || pPrimaryGroup->group_code[0] == '\0')
        {
            continue;
        }

        if (1 == pPrimaryGroup->del_mark) /* 删除 */
        {
            GroupIDVector.push_back(pPrimaryGroup->group_id);
        }
    }

    PRIMARY_GROUP_SMUTEX_UNLOCK();

    if (GroupIDVector.size() > 0)
    {
        for (index = 0; index < (int)GroupIDVector.size(); index++)
        {
            pPrimaryGroup = get_primary_group((char*)GroupIDVector[index].c_str());

            if (NULL != pPrimaryGroup)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "根据删除标识, 逻辑设备分组信息成功:逻辑设备分组ID=%s, 分组名称=%s", pPrimaryGroup->group_id, pPrimaryGroup->group_name);

                /* 发送通知给上级平台 */
                i = SendNotifyGroupCatalogToOwnerRouteCMS(pPrimaryGroup->group_id, pPrimaryGroup->group_name, pPrimaryGroup->parent_id, 0, 1);

                if (i > 0)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "发送逻辑设备分组删除通知给上级平台:逻辑设备分组ID=%s", pPrimaryGroup->group_id);
                }

                /* 发送通知给第三方平台 */
                i = SendNotifyGroupCatalogTo3PartyRouteCMS(pPrimaryGroup->group_code, pPrimaryGroup->group_name, pPrimaryGroup->parent_code, 1);

                if (i > 0)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "发送逻辑设备分组删除通知给上级第三方平台:逻辑设备分组ID=%s", pPrimaryGroup->group_code);
                }

                PRIMARY_GROUP_SMUTEX_LOCK();
                pGblconf->PrimaryGroupList.erase(pPrimaryGroup->group_id);
                primary_group_free(pPrimaryGroup);
                pPrimaryGroup = NULL;
                PRIMARY_GROUP_SMUTEX_UNLOCK();
            }
        }
    }

    GroupIDVector.clear();

    return 0;
}

/*****************************************************************************
 函 数 名  : GetGBLogicDeviceCivilCode
 功能描述  : 根据逻辑点位的索引，获取所在分组的编码
 输入参数  : unsigned int device_index
             DBOper* pDbOper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年9月6日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
char* GetGBLogicDeviceCivilCode(unsigned int device_index, DBOper* pDbOper)
{
    string strSQL = "";
    string strGroupID = "";
    int record_count = 0;
    char strDeviceIndex[32] = {0};
    primary_group_t* pPrimaryGroup = NULL;

    if (device_index <= 0 || NULL == pDbOper)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "GetGBLogicDeviceCivilCode() exit---: Param Error \r\n");
        return NULL;
    }

    snprintf(strDeviceIndex, 32, "%u", device_index);

    strSQL.clear();
    strSQL.clear();
    strSQL = "select * from LogicDeviceMapGroupConfig WHERE DeviceIndex = ";
    strSQL += strDeviceIndex;

    record_count = pDbOper->DB_Select(strSQL.c_str(), 1);

    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "GetGBLogicDeviceCivilCode() DB Select:record_count=%d,strSQL=%s \r\n", record_count, strSQL.c_str());

    if (record_count < 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "GetGBLogicDeviceCivilCode() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "GetGBLogicDeviceCivilCode() ErrorMsg=%s\r\n", pDbOper->GetLastDbErrorMsg());
        return NULL;
    }
    else if (record_count == 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "GetGBLogicDeviceCivilCode() DB No Record:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
        return NULL;
    }

    /* 组编号 */
    strGroupID.clear();
    pDbOper->GetFieldValue("GroupID", strGroupID);

    if (strGroupID.empty())
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "GetGBLogicDeviceCivilCode() GroupID Empty\r\n");
        return NULL;
    }

    pPrimaryGroup = get_primary_group((char*)strGroupID.c_str());

    if (NULL == pPrimaryGroup)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "GetGBLogicDeviceCivilCode() Get Primary Group Error: GroupID=%s \r\n", (char*)strGroupID.c_str());
        return NULL;
    }

    if (pPrimaryGroup->group_code[0] == '\0')
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "GetGBLogicDeviceCivilCode() Get Group Code Error: GroupID=%s \r\n", (char*)strGroupID.c_str());
        return NULL;
    }

    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "GetGBLogicDeviceCivilCode() Get Group Code: device_index=%u, group_code=%s \r\n", device_index, pPrimaryGroup->group_code);
    return pPrimaryGroup->group_code;
}

/*****************************************************************************
 函 数 名  : GetPrimaryGroupInfoByGBLogicDeviceIndex
 功能描述  : 根据设备索引获取分组信息
 输入参数  : unsigned int device_index
             DBOper* pDbOper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年9月9日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
primary_group_t* GetPrimaryGroupInfoByGBLogicDeviceIndex(unsigned int device_index, DBOper* pDbOper)
{
    string strSQL = "";
    string strGroupID = "";
    int record_count = 0;
    char strDeviceIndex[32] = {0};
    primary_group_t* pPrimaryGroup = NULL;

    if (device_index <= 0 || NULL == pDbOper)
    {
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "GetPrimaryGroupInfoByGBLogicDeviceIndex() exit---: Param Error \r\n");
        return NULL;
    }

    snprintf(strDeviceIndex, 32, "%u", device_index);

    strSQL.clear();
    strSQL.clear();
    strSQL = "select * from LogicDeviceMapGroupConfig WHERE DeviceIndex = ";
    strSQL += strDeviceIndex;

    record_count = pDbOper->DB_Select(strSQL.c_str(), 1);

    //DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "GetPrimaryGroupInfoByGBLogicDeviceIndex() DB Select:record_count=%d,strSQL=%s \r\n", record_count, strSQL.c_str());

    if (record_count < 0)
    {
        //DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "GetPrimaryGroupInfoByGBLogicDeviceIndex() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
        //DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "GetPrimaryGroupInfoByGBLogicDeviceIndex() ErrorMsg=%s\r\n", pDbOper->GetLastDbErrorMsg());
        return NULL;
    }
    else if (record_count == 0)
    {
        //DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "GetPrimaryGroupInfoByGBLogicDeviceIndex() DB No Record:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
        return NULL;
    }

    /* 组编号 */
    strGroupID.clear();
    pDbOper->GetFieldValue("GroupID", strGroupID);

    if (strGroupID.empty())
    {
        //DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "GetPrimaryGroupInfoByGBLogicDeviceIndex() GroupID Empty\r\n");
        return NULL;
    }

    pPrimaryGroup = get_primary_group((char*)strGroupID.c_str());

    if (NULL == pPrimaryGroup)
    {
        //DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "GetPrimaryGroupInfoByGBLogicDeviceIndex() Get Primary Group Error: GroupID=%s \r\n", (char*)strGroupID.c_str());
        return NULL;
    }

    return pPrimaryGroup;
}

/*****************************************************************************
 函 数 名  : AddGblLogicDeviceGroupToVectorForRoute
 功能描述  : 添加全局分组信息到队列
 输入参数  : vector<string>& GroupIDVector
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年9月6日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int AddGblLogicDeviceGroupToVectorForRoute(vector<string>& GroupIDVector)
{
    primary_group_t* pPrimaryGroup = NULL;
    PrimaryGroup_Iterator Itr;

    if (pGblconf == NULL)
    {
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "SetGblLogicDeviceGroupDelFlag() exit---: Param Error \r\n");
        return -1;
    }

    PRIMARY_GROUP_SMUTEX_LOCK();

    if (pGblconf->PrimaryGroupList.size() <= 0)
    {
        PRIMARY_GROUP_SMUTEX_UNLOCK();
        return 0;
    }

    for (Itr = pGblconf->PrimaryGroupList.begin(); Itr != pGblconf->PrimaryGroupList.end(); Itr++)
    {
        pPrimaryGroup = Itr->second;

        if (NULL == pPrimaryGroup || pPrimaryGroup->group_id[0] == '\0' || pPrimaryGroup->group_code[0] == '\0')
        {
            continue;
        }

        if (0 == pPrimaryGroup->iNeedToUpLoad)
        {
            continue;
        }

        GroupIDVector.push_back(pPrimaryGroup->group_id);
    }

    PRIMARY_GROUP_SMUTEX_UNLOCK();

    return 0;
}

/*****************************************************************************
 函 数 名  : GBLogicDeviceGroupInfoConfig_db_refresh_proc
 功能描述  : 设置逻辑设备分组配置信息数据库更新操作标识
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年10月18日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int GBLogicDeviceGroupInfoConfig_db_refresh_proc()
{
    if (1 == db_GroupInfo_reload_mark) /* 正在执行 */
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "逻辑设备分组数据库信息正在同步");
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Logic Device Group Info database information are synchronized");
        return 0;
    }

    db_GroupInfo_reload_mark = 1;
    return 0;
}

/*****************************************************************************
 函 数 名  : check_GBLogicDeviceGroupInfoConfig_need_to_reload
 功能描述  : 检查是否需要同步逻辑设备分组信息
 输入参数  : DBOper* pDboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年10月18日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
void check_GBLogicDeviceGroupInfoConfig_need_to_reload(DBOper* pDboper)
{
    /* 检查是否需要更新数据库标识 */
    if (!db_GroupInfo_reload_mark)
    {
        return;
    }

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "同步逻辑设备分组配置数据库信息: 开始---");
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Synchronization GBLogic Device Group info database information: begain---");
    printf("\r\n\r\ncheck_GBLogicDeviceGroupInfoConfig_need_to_reload() :begain---, time=%d \r\n", time(NULL));

    /* 设置全局分组信息的删除标识 */
    SetGblLogicDeviceGroupDelFlag(1);

    /* 将数据库中的变化数据同步到内存 */
    load_device_group_cfg_from_db(pDboper);

    /* 删除多余的全局分组信息 */
    DelGblLogicDeviceGroupInfoByDelMark();
    printf("check_GBLogicDeviceGroupInfoConfig_need_to_reload() :end---, time=%d \r\n", time(NULL));

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "同步逻辑设备分组配置数据库信息: 结束---");
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Synchronization GBLogic Device Group info database information: end---");
    db_GroupInfo_reload_mark = 0;

    return;
}
#endif

#if DECS("全局逻辑设备分组关系")
/*****************************************************************************
 函 数 名  : device_group_map_init
 功能描述  : 逻辑设备分组关系信息结构初始化
 输入参数  : device_group_map_t** device_group_map
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年8月18日 星期一
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int device_group_map_init(device_group_map_t** device_group_map)
{
    *device_group_map = (device_group_map_t*)smalloc(sizeof(device_group_map_t));

    if (*device_group_map == NULL)
    {
        return -1;
    }

    (*device_group_map)->group_id[0] = '\0';
    (*device_group_map)->device_index = 0;
    (*device_group_map)->iSortID = 0;
    (*device_group_map)->del_mark = 0;

    return 0;
}

/*****************************************************************************
 函 数 名  : device_group_map_free
 功能描述  : 逻辑设备分关系信息结构释放
 输入参数  : device_group_map_t* device_group_map
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年8月18日 星期一
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
void device_group_map_free(device_group_map_t* device_group_map)
{
    if (device_group_map == NULL)
    {
        return;
    }

    memset(device_group_map->group_id, 0, 36);
    device_group_map->device_index = 0;
    device_group_map->iSortID = 0;
    device_group_map->del_mark = 0;

    osip_free(device_group_map);
    device_group_map = NULL;

    return;
}
/*****************************************************************************
 函 数 名  : get_device_map_group
 功能描述  : 根据GroupID和点位索引获取分组关系信息
 输入参数  : unsigned int device_index
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年9月2日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
device_group_map_t* get_device_map_group(unsigned int device_index)
{
    device_group_map_t* pDeviceGroupMap = NULL;
    DeviceGroupMap_Iterator Itr;

    if (device_index <= 0 || pGblconf == NULL)
    {
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "get_primary_group() exit---: Param Error \r\n");
        return NULL;
    }

    DEVICE_MAP_GROUP_MAP_SMUTEX_LOCK();

    if (pGblconf->DeviceGroupMapList.size() <= 0)
    {
        DEVICE_MAP_GROUP_MAP_SMUTEX_UNLOCK();
        return NULL;
    }

    Itr = pGblconf->DeviceGroupMapList.find(device_index);

    if (Itr == pGblconf->DeviceGroupMapList.end())
    {
        DEVICE_MAP_GROUP_MAP_SMUTEX_UNLOCK();
        return NULL;
    }
    else
    {
        pDeviceGroupMap = Itr->second;
        DEVICE_MAP_GROUP_MAP_SMUTEX_UNLOCK();
        return pDeviceGroupMap;
    }

    DEVICE_MAP_GROUP_MAP_SMUTEX_UNLOCK();
    return NULL;

}
/*****************************************************************************
 函 数 名  : device_map_group_add
 功能描述  : 添加全局分组关系信息到队列
 输入参数  : char* group_id
             unsigned int device_index
             int sort_id
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年9月2日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int device_map_group_add(char* group_id, unsigned int device_index, int sort_id)
{
    int i = 0;
    int iRet = 0;
    device_group_map_t* pDeviceGroupMap = NULL;
    int iOldSortID = 0;

    if (pGblconf == NULL)
    {
        return -1;
    }

    if (NULL == group_id || device_index <= 0)
    {
        return -1;
    }

    pDeviceGroupMap = get_device_map_group(device_index);

    if (NULL == pDeviceGroupMap)
    {
        i = device_group_map_init(&pDeviceGroupMap);

        if (i != 0)
        {
            return -1;
        }

        memset(pDeviceGroupMap->group_id, 0, 32 + 4);
        osip_strncpy(pDeviceGroupMap->group_id, group_id, 32);

        pDeviceGroupMap->device_index = device_index;

        pDeviceGroupMap->iSortID = sort_id;

        pDeviceGroupMap->del_mark = 0;

        DEVICE_MAP_GROUP_MAP_SMUTEX_LOCK();
        pGblconf->DeviceGroupMapList[device_index] = pDeviceGroupMap;
        DEVICE_MAP_GROUP_MAP_SMUTEX_UNLOCK();

        if (1 == cms_run_status)
        {
            /* 发送增加通知给自己平台 */
            iRet = SendNotifyGroupMapToOwnerRouteCMS(pDeviceGroupMap->group_id, pDeviceGroupMap->device_index, pDeviceGroupMap->iSortID, 0);

            if (iRet > 0)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "发送逻辑设备分组关系增加通知给上级平台:逻辑设备分组ID=%s, 点位索引=%u", pDeviceGroupMap->group_id, pDeviceGroupMap->device_index);
            }
        }

        return 0;
    }
    else
    {
        pDeviceGroupMap->del_mark = 0;

        /* 更新内存 */
        /* 排序ID */
        iOldSortID = pDeviceGroupMap->iSortID;
        pDeviceGroupMap->iSortID = sort_id;

        /* 判断是否需要发送给本级的上级平台 */
        if (pDeviceGroupMap->iSortID != iOldSortID)
        {
            /* 发送通知给自己平台 */
            iRet = SendNotifyGroupMapToOwnerRouteCMS(pDeviceGroupMap->group_id, pDeviceGroupMap->device_index, pDeviceGroupMap->iSortID, 2);

            if (iRet > 0)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "发送逻辑设备分组关系修改通知给上级平台:逻辑设备分组ID=%s, 点位索引=%u", pDeviceGroupMap->group_id, pDeviceGroupMap->device_index);
            }
        }

        return 0;
    }
}

/*****************************************************************************
 函 数 名  : device_map_group_info_list_lock
 功能描述  : 全局设备分组关系信息队列锁定
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月11日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int device_map_group_info_list_lock()
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_GblconfGroupMapLock == NULL)
    {
        //DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "device_map_group_info_list_lock() exit---: Param Error \r\n");
        return -1;
    }

    iRet = osip_mutex_lock((struct osip_mutex*)g_GblconfGroupMapLock);

#endif

    return iRet;
}

/*****************************************************************************
 函 数 名  : device_map_group_info_list_unlock
 功能描述  : 全局设备分组关系信息队列解除锁定
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月11日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int device_map_group_info_list_unlock()
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_GblconfGroupMapLock == NULL)
    {
        //DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "device_map_group_info_list_unlock() exit---: Param Error \r\n");
        return -1;
    }

    iRet = osip_mutex_unlock((struct osip_mutex*)g_GblconfGroupMapLock);

#endif

    return iRet;
}

/*****************************************************************************
 函 数 名  : debug_device_map_group_info_list_lock
 功能描述  : 全局设备分组关系信息队列锁定
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月11日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int debug_device_map_group_info_list_lock(const char* file, int line, const char* func)
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_GblconfGroupMapLock == NULL)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "debug_device_map_group_info_list_lock() exit---: Param Error \r\n");
        return -1;
    }

    iRet = osip_debug_mutex_lock((struct osip_mutex*)g_GblconfGroupMapLock, file, line, func);

#endif

    return iRet;
}

/*****************************************************************************
 函 数 名  : debug_device_map_group_info_list_unlock
 功能描述  : 全局设备分组关系信息队列解除锁定
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月11日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int debug_device_map_group_info_list_unlock(const char* file, int line, const char* func)
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_GblconfGroupMapLock == NULL)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "debug_device_map_group_info_list_unlock() exit---: Param Error \r\n");
        return -1;
    }

    iRet = osip_debug_mutex_unlock((struct osip_mutex*)g_GblconfGroupMapLock, file,  line, func);

#endif

    return iRet;
}

/*****************************************************************************
 函 数 名  : SetGblLogicDeviceGroupMapDelFlag
 功能描述  : 设置全局分组关系信息的删除标识
 输入参数  : int iDelFlag
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年9月6日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int SetGblLogicDeviceGroupMapDelFlag(int iDelFlag)
{
    device_group_map_t* pDeviceGroupMap = NULL;
    DeviceGroupMap_Iterator Itr;

    if (pGblconf == NULL)
    {
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "SetGblLogicDeviceGroupDelFlag() exit---: Param Error \r\n");
        return -1;
    }

    DEVICE_MAP_GROUP_MAP_SMUTEX_LOCK();

    if (pGblconf->DeviceGroupMapList.size() <= 0)
    {
        DEVICE_MAP_GROUP_MAP_SMUTEX_UNLOCK();
        return NULL;
    }

    for (Itr = pGblconf->DeviceGroupMapList.begin(); Itr != pGblconf->DeviceGroupMapList.end(); Itr++)
    {
        pDeviceGroupMap = Itr->second;

        if (NULL == pDeviceGroupMap || pDeviceGroupMap->group_id[0] == '\0' || pDeviceGroupMap->device_index <= 0)
        {
            continue;
        }

        pDeviceGroupMap->del_mark = iDelFlag;
    }

    DEVICE_MAP_GROUP_MAP_SMUTEX_UNLOCK();

    return 0;
}

/*****************************************************************************
 函 数 名  : DelGblLogicDeviceGroupMapInfoByDelMark
 功能描述  : 根据删除标识删除全局分组关系信息
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年9月6日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int DelGblLogicDeviceGroupMapInfoByDelMark()
{
    int i = 0;
    int index = 0;
    device_group_map_t* pDeviceGroupMap = NULL;
    vector<unsigned int> DeviceIndexVector;
    DeviceGroupMap_Iterator Itr;

    if (pGblconf == NULL)
    {
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "DelGblLogicDeviceGroupInfoByDelMark() exit---: Param Error \r\n");
        return -1;
    }

    DeviceIndexVector.clear();

    DEVICE_MAP_GROUP_MAP_SMUTEX_LOCK();

    if (pGblconf->DeviceGroupMapList.size() <= 0)
    {
        DEVICE_MAP_GROUP_MAP_SMUTEX_UNLOCK();
        return NULL;
    }

    for (Itr = pGblconf->DeviceGroupMapList.begin(); Itr != pGblconf->DeviceGroupMapList.end(); Itr++)
    {
        pDeviceGroupMap = Itr->second;

        if (NULL == pDeviceGroupMap || pDeviceGroupMap->group_id[0] == '\0' || pDeviceGroupMap->device_index <= 0)
        {
            continue;
        }

        if (1 == pDeviceGroupMap->del_mark) /* 删除 */
        {
            DeviceIndexVector.push_back(pDeviceGroupMap->device_index);
        }
    }

    DEVICE_MAP_GROUP_MAP_SMUTEX_UNLOCK();

    if (DeviceIndexVector.size() > 0)
    {
        for (index = 0; index < (int)DeviceIndexVector.size(); index++)
        {
            pDeviceGroupMap = get_device_map_group(DeviceIndexVector[index]);

            if (NULL != pDeviceGroupMap)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "根据删除标识, 逻辑设备分组关系信息成功:逻辑设备分组ID=%s, 逻辑设备索引=%u", pDeviceGroupMap->group_id, pDeviceGroupMap->device_index);

                /* 发送通知给上级平台 */
                i = SendNotifyGroupMapToOwnerRouteCMS(pDeviceGroupMap->group_id, pDeviceGroupMap->device_index, 0, 1);

                if (i > 0)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "发送逻辑设备分组关系修改通知给上级平台:逻辑设备分组ID=%s, 逻辑设备索引=%u", pDeviceGroupMap->group_id, pDeviceGroupMap->device_index);
                }

                DEVICE_MAP_GROUP_MAP_SMUTEX_LOCK();
                pGblconf->DeviceGroupMapList.erase(pDeviceGroupMap->device_index);
                device_group_map_free(pDeviceGroupMap);
                pDeviceGroupMap = NULL;
                DEVICE_MAP_GROUP_MAP_SMUTEX_UNLOCK();
            }
        }
    }

    DeviceIndexVector.clear();

    return 0;
}

/*****************************************************************************
 函 数 名  : GBLogicDeviceGroupMapInfoConfig_db_refresh_proc
 功能描述  : 设置逻辑设备分组关系配置信息数据库更新操作标识
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年10月18日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int GBLogicDeviceGroupMapInfoConfig_db_refresh_proc()
{
    if (1 == db_GroupMapInfo_reload_mark) /* 正在执行 */
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "逻辑设备分组关系数据库信息正在同步");
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Logic Device Group Map Info database information are synchronized");
        return 0;
    }

    db_GroupMapInfo_reload_mark = 1;
    return 0;
}

/*****************************************************************************
 函 数 名  : check_GBLogicDeviceGroupMapInfoConfig_need_to_reload
 功能描述  : 检查是否需要同步逻辑设备分组关系信息
 输入参数  : DBOper* pDboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年10月18日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
void check_GBLogicDeviceGroupMapInfoConfig_need_to_reload(DBOper* pDboper)
{
    /* 检查是否需要更新数据库标识 */
    if (!db_GroupMapInfo_reload_mark)
    {
        return;
    }

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "同步逻辑设备分组关系配置数据库信息: 开始---");
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Synchronization GBLogic Device Group Map info database information: begain---");
    printf("\r\n\r\ncheck_GBLogicDeviceGroupMapInfoConfig_need_to_reload() :begain---, time=%d \r\n", time(NULL));

    /* 设置全局分组信息的删除标识 */
    SetGblLogicDeviceGroupMapDelFlag(1);

    /* 将数据库中的变化数据同步到内存 */
    //printf("\r\n\r\n check_GBLogicDeviceGroupMapInfoConfig_need_to_reload() load_device_group_map_cfg_from_db: begain---, time=%d", time(NULL));
    load_device_group_map_cfg_from_db(pDboper);
    //printf("\r\n check_GBLogicDeviceGroupMapInfoConfig_need_to_reload() load_device_group_map_cfg_from_db: end---, time=%d", time(NULL));

    /* 添加逻辑设备的行政区域信息 */
    AddCivilCodeToGBLogicDeviceInfo(pDboper);

    /* 删除多余的全局分组关系信息 */
    //printf("\r\n check_GBLogicDeviceGroupMapInfoConfig_need_to_reload() DelGblLogicDeviceGroupMapInfoByDelMark: begain---");
    DelGblLogicDeviceGroupMapInfoByDelMark();
    //printf("\r\n check_GBLogicDeviceGroupMapInfoConfig_need_to_reload() DelGblLogicDeviceGroupMapInfoByDelMark: end--- \r\n\r\n");
    printf("check_GBLogicDeviceGroupMapInfoConfig_need_to_reload() :end---, time=%d \r\n", time(NULL));

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "同步逻辑设备分组关系配置数据库信息: 结束---");
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Synchronization GBLogic Device Group Map info database information: end---");
    db_GroupMapInfo_reload_mark = 0;

    return;
}
#endif

/*****************************************************************************
 函 数 名  : gbl_conf_init
 功能描述  : 全局配置变量初始化
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年6月19日 星期三
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int gbl_conf_init()
{
#ifdef MULTI_THR
    /* init smutex */
    g_GblconfGroupLock = (osip_mutex_t*)osip_mutex_init();

    if (NULL == g_GblconfGroupLock)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "sip_msg_log2file_list_init() exit---: g_GblconfGroupLock Init Error \r\n");
        return -1;
    }

    g_GblconfGroupMapLock = (osip_mutex_t*)osip_mutex_init();

    if (NULL == g_GblconfGroupMapLock)
    {
        if (NULL != g_GblconfGroupLock)
        {
            osip_mutex_destroy((struct osip_mutex*)g_GblconfGroupLock);
            g_GblconfGroupLock = NULL;
        }

        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "sip_msg_log2file_list_init() exit---: g_GblconfGroupMapLock Init Error \r\n");
        return -1;
    }

#endif

    pGblconf = new gbl_conf_t;

    if (pGblconf == NULL)
    {
#ifdef MULTI_THR

        if (NULL != g_GblconfGroupLock)
        {
            osip_mutex_destroy((struct osip_mutex*)g_GblconfGroupLock);
            g_GblconfGroupLock = NULL;
        }

        if (NULL != g_GblconfGroupMapLock)
        {
            osip_mutex_destroy((struct osip_mutex*)g_GblconfGroupMapLock);
            g_GblconfGroupMapLock = NULL;
        }

#endif

        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "gbl_conf_init() exit---: pGblconf Smalloc Error \r\n");
        return -1;
    }

    pGblconf->board_index = 0;

    pGblconf->board_id[0] = '\0';
    pGblconf->mms_id[0] = '\0';
    //osip_strncpy(pGblconf->board_id, (char*)"32011500002000000001", 20);

    pGblconf->center_code[0] = '\0';

    pGblconf->province_code[0] = '\0';                 /* 省级编码 */

    pGblconf->city_code[0] = '\0';                     /* 市级编码 */

    pGblconf->region_code[0] = '\0';                   /* 区县编码 */

    pGblconf->civil_code[0] = '\0';

    pGblconf->trade_code[0] = '\0';
    pGblconf->type_code[0] = '\0';
    pGblconf->serial_number[0] = '\0';

    pGblconf->register_region[0] = '\0';

    pGblconf->alarm_server_ip[0] = '\0';
    pGblconf->ntp_server_ip[0] = '\0';

    pGblconf->db_server_ip[0] = '\0';
    memcpy(pGblconf->db_server_ip, "127.0.0.1", MAX_IP_LEN);

    pGblconf->spy_username[0] = '\0';
    memcpy(pGblconf->spy_username, "root", MAX_128CHAR_STRING_LEN);

    pGblconf->spy_password[0] = '\0';
    memcpy(pGblconf->spy_password, "zbitcloud", MAX_128CHAR_STRING_LEN);

    pGblconf->default_eth_name[0] = '\0';
    pGblconf->cms_name[0] = '\0';

    pGblconf->iAuthFlag = 0;

    pGblconf->spy_port = 2000;

    pGblconf->uPresetBackTime = DEFAULT_PRESET_BACK_TIME;
    pGblconf->uDeviceUnLockTime = DEFAULT_DEVICE_UNLOCK_TIME;

    pGblconf->register_retry_interval = MIN_REGISTER_RETRY_INTERVAL;
    pGblconf->registry_cleanuprate = MIN_REGISTER_EXPIRE;
    pGblconf->session_expires = MIN_SESSION_EXPIRE;
    pGblconf->subscribe_expires = MIN_SUBSCRIBE_EXPIRE;
    pGblconf->keep_alive_interval = MIN_KEEP_ALIVE_INTERVAL;
    pGblconf->failed_keep_alive_count = DEFAULT_FAILED_KEEP_ALIVE_COUNT;
    pGblconf->keep_alive_expires = DEFAULT_DEVICE_KEEP_ALIVE_EXPIRES;
    pGblconf->alarm_duration = DEFAULT_ALARM_DURATION;

    pGblconf->shdb_agent_id[0] = '\0';                                  /* 上海地标监管平台注册ID */
    pGblconf->shdb_server_ip[0] = '\0';                                 /* 上海地标监管平台服务器IP */
    pGblconf->shdb_prex_seconds = DEFAULT_SHDB_DURATION_SECONDS;        /* 上海地标报警图片上传前N秒, 默认60秒 */
    pGblconf->shdb_next_seconds = DEFAULT_SHDB_DURATION_SECONDS;        /* 上海地标报警图片上传后M秒，默认60秒 */
    pGblconf->shdb_interval_seconds = DEFAULT_SHDB_INTERVAL_SECONDS;    /* 上海地标报警图片上传间隔P秒，默认3秒 */
    pGblconf->sys_exit_flag = 1;                                        /* 系统退出标识，0，正常退出；1，异常退出 */
    pGblconf->showcode = 1;                                             /* ShowCode = 0 非国标版本，ShowCode = 1和无配置 为国标版本*//* */

    /* 本地基层分组队列初始化 */
    pGblconf->PrimaryGroupList.clear();

    /* 本地分组关系队列初始化 */
    pGblconf->DeviceGroupMapList.clear();

    /* IP地址队列初始化 */
    pGblconf->pLocalIPAddrList = (osip_list_t*)osip_malloc(sizeof(osip_list_t));

    if (NULL == pGblconf->pLocalIPAddrList)
    {
#ifdef MULTI_THR

        if (NULL != g_GblconfGroupMapLock)
        {
            osip_mutex_destroy((struct osip_mutex*)g_GblconfGroupMapLock);
            g_GblconfGroupMapLock = NULL;
        }

        if (NULL != g_GblconfGroupLock)
        {
            osip_mutex_destroy((struct osip_mutex*)g_GblconfGroupLock);
            g_GblconfGroupLock = NULL;
        }

#endif

        osip_free(pGblconf);
        pGblconf = NULL;
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "gbl_conf_init() exit---: Local IP Addr List Init Error \r\n");
        return -1;
    }

    osip_list_init(pGblconf->pLocalIPAddrList);

    return 0;
}

/*****************************************************************************
 函 数 名  : gbl_conf_free
 功能描述  : 全局配置变量释放
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年6月19日 星期三
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void gbl_conf_free()
{
    PrimaryGroup_Iterator PrimaryGroupItr;
    DeviceGroupMap_Iterator MapGroupItr;
    device_group_map_t* pDeviceGroupMap = NULL;
    primary_group_t* pPrimaryGroup = NULL;

#ifdef MULTI_THR

    if (NULL != g_GblconfGroupLock)
    {
        osip_mutex_destroy((struct osip_mutex*)g_GblconfGroupLock);
        g_GblconfGroupLock = NULL;
    }

    if (NULL != g_GblconfGroupMapLock)
    {
        osip_mutex_destroy((struct osip_mutex*)g_GblconfGroupMapLock);
        g_GblconfGroupMapLock = NULL;
    }

#endif

    if (pGblconf == NULL)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "gbl_conf_free() exit---: Param Error \r\n");
        return;
    }

    pGblconf->board_index = 0;

    memset(pGblconf->board_id, 0, MAX_ID_LEN + 4);
    memset(pGblconf->mms_id, 0, MAX_ID_LEN + 4);

    pGblconf->center_code[0] = '\0';

    pGblconf->province_code[0] = '\0';                 /* 省级编码 */

    pGblconf->city_code[0] = '\0';                     /* 市级编码 */

    pGblconf->region_code[0] = '\0';                   /* 区县编码 */

    pGblconf->civil_code[0] = '\0';

    pGblconf->trade_code[0] = '\0';
    pGblconf->type_code[0] = '\0';
    pGblconf->serial_number[0] = '\0';

    pGblconf->register_region[0] = '\0';

    pGblconf->alarm_server_ip[0] = '\0';
    pGblconf->ntp_server_ip[0] = '\0';

    pGblconf->db_server_ip[0] = '\0';
    memcpy(pGblconf->db_server_ip, "127.0.0.1", MAX_IP_LEN);

    pGblconf->spy_username[0] = '\0';
    memcpy(pGblconf->spy_username, "root", MAX_128CHAR_STRING_LEN);

    pGblconf->spy_password[0] = '\0';
    memcpy(pGblconf->spy_password, "zbitcloud", MAX_128CHAR_STRING_LEN);

    pGblconf->default_eth_name[0] = '\0';
    pGblconf->cms_name[0] = '\0';

    pGblconf->iAuthFlag = 0;

    pGblconf->spy_port = 2000;

    pGblconf->uPresetBackTime = DEFAULT_PRESET_BACK_TIME;
    pGblconf->uDeviceUnLockTime = DEFAULT_DEVICE_UNLOCK_TIME;

    pGblconf->register_retry_interval = MIN_REGISTER_RETRY_INTERVAL;
    pGblconf->registry_cleanuprate = MIN_REGISTER_EXPIRE;
    pGblconf->session_expires = MIN_SESSION_EXPIRE;
    pGblconf->subscribe_expires = MIN_SUBSCRIBE_EXPIRE;
    pGblconf->keep_alive_interval = MIN_KEEP_ALIVE_INTERVAL;
    pGblconf->failed_keep_alive_count = DEFAULT_FAILED_KEEP_ALIVE_COUNT;
    pGblconf->keep_alive_expires = DEFAULT_DEVICE_KEEP_ALIVE_EXPIRES;
    pGblconf->alarm_duration = DEFAULT_ALARM_DURATION;

    pGblconf->shdb_agent_id[0] = '\0';                                  /* 上海地标监管平台注册ID */
    pGblconf->shdb_server_ip[0] = '\0';                                 /* 上海地标监管平台服务器IP */
    pGblconf->shdb_prex_seconds = DEFAULT_SHDB_DURATION_SECONDS;        /* 上海地标报警图片上传前N秒, 默认60秒 */
    pGblconf->shdb_next_seconds = DEFAULT_SHDB_DURATION_SECONDS;        /* 上海地标报警图片上传后M秒，默认60秒 */
    pGblconf->shdb_interval_seconds = DEFAULT_SHDB_INTERVAL_SECONDS;    /* 上海地标报警图片上传间隔P秒，默认3秒 */
    pGblconf->sys_exit_flag = 1;                                        /* 系统退出标识，0，正常退出；1，异常退出 */
    pGblconf->showcode = 1;                                             /* ShowCode = 0 非国标版本，ShowCode = 1和无配置 为国标版本*//* */

    for (PrimaryGroupItr = pGblconf->PrimaryGroupList.begin(); PrimaryGroupItr != pGblconf->PrimaryGroupList.end(); PrimaryGroupItr++)
    {
        pPrimaryGroup = PrimaryGroupItr->second;

        if (NULL != pPrimaryGroup)
        {
            primary_group_free(pPrimaryGroup);
            pPrimaryGroup = NULL;
        }
    }

    pGblconf->PrimaryGroupList.clear();

    for (MapGroupItr = pGblconf->DeviceGroupMapList.begin(); MapGroupItr != pGblconf->DeviceGroupMapList.end(); MapGroupItr++)
    {
        pDeviceGroupMap = MapGroupItr->second;

        if (NULL != pDeviceGroupMap)
        {
            device_group_map_free(pDeviceGroupMap);
            pDeviceGroupMap = NULL;
        }
    }

    pGblconf->DeviceGroupMapList.clear();

    if (NULL != pGblconf->pLocalIPAddrList)
    {
        osip_list_special_free(pGblconf->pLocalIPAddrList, (void (*)(void*))&ip_pair_free);
        osip_free(pGblconf->pLocalIPAddrList);
        pGblconf->pLocalIPAddrList = NULL;
    }

    delete pGblconf;
    pGblconf = NULL;
    return;
}

/*****************************************************************************
 函 数 名  : local_ip_get
 功能描述  : 获取本地ip地址
 输入参数  : char* eth_name
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年8月14日 星期三
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
char* local_ip_get(char* eth_name)
{
    int i = 0;
    ip_pair_t* pIPaddr = NULL;

    if (NULL == eth_name || pGblconf == NULL || NULL == pGblconf->pLocalIPAddrList)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "local_ip_get() exit---: Param Error \r\n");
        return NULL;
    }

    if (osip_list_size(pGblconf->pLocalIPAddrList) <= 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "local_ip_get() exit---: Local IP Addr List NULL \r\n");
        return NULL;
    }

    for (i = 0; i < osip_list_size(pGblconf->pLocalIPAddrList); i++)
    {
        pIPaddr = (ip_pair_t*)osip_list_get(pGblconf->pLocalIPAddrList, i);

        if (NULL == pIPaddr || pIPaddr->eth_name[0] == '\0')
        {
            continue;
        }

        if (0 == sstrcmp(eth_name, pIPaddr->eth_name))
        {
            return pIPaddr->local_ip;
        }
    }

    return NULL;
}

/*****************************************************************************
 函 数 名  : local_port_get
 功能描述  : 获取本地端口号
 输入参数  : char* eth_name
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年8月14日 星期三
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int local_port_get(char* eth_name)
{
    int i = 0;
    ip_pair_t* pIPaddr = NULL;

    if (NULL == eth_name || pGblconf == NULL || NULL == pGblconf->pLocalIPAddrList)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "local_port_get() exit---: Param Error \r\n");
        return -1;
    }

    if (osip_list_size(pGblconf->pLocalIPAddrList) <= 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "local_port_get() exit---: Local IP Addr List NULL \r\n");
        return -1;
    }

    for (i = 0; i < osip_list_size(pGblconf->pLocalIPAddrList); i++)
    {
        pIPaddr = (ip_pair_t*)osip_list_get(pGblconf->pLocalIPAddrList, i);

        if (NULL == pIPaddr || pIPaddr->eth_name[0] == '\0')
        {
            continue;
        }

        if (0 == sstrcmp(eth_name, pIPaddr->eth_name))
        {
            return pIPaddr->local_port;
        }
    }

    return -1;
}

/*****************************************************************************
 函 数 名  : get_local_ip_type
 功能描述  : 获取本地ip地址类型
 输入参数  : char* eth_name
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年6月9日 星期二
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
ip_addr_type_t get_local_ip_type(char* eth_name)
{
    int i = 0;
    ip_pair_t* pIPaddr = NULL;

    if (NULL == eth_name || pGblconf == NULL || NULL == pGblconf->pLocalIPAddrList)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "get_local_ip_type() exit---: Param Error \r\n");
        return IP_ADDR_NULL;
    }

    if (osip_list_size(pGblconf->pLocalIPAddrList) <= 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "get_local_ip_type() exit---: Local IP Addr List NULL \r\n");
        return IP_ADDR_NULL;
    }

    for (i = 0; i < osip_list_size(pGblconf->pLocalIPAddrList); i++)
    {
        pIPaddr = (ip_pair_t*)osip_list_get(pGblconf->pLocalIPAddrList, i);

        if (NULL == pIPaddr || pIPaddr->eth_name[0] == '\0')
        {
            continue;
        }

        if (0 == sstrcmp(eth_name, pIPaddr->eth_name))
        {
            return pIPaddr->ip_type;
        }
    }

    return IP_ADDR_NULL;
}

/*****************************************************************************
 函 数 名  : get_ip_eth_name
 功能描述  : 根据服务器的IP地址获取服务器的IP地址的网卡名称
 输入参数  : char* server_ip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年7月25日 星期五
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
char* get_ip_eth_name(char* server_ip)
{
    int i = 0;
    ip_pair_t* pIPaddr = NULL;

    if (NULL == server_ip || pGblconf == NULL || NULL == pGblconf->pLocalIPAddrList)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "get_ip_eth_name() exit---: Param Error \r\n");
        return NULL;
    }

    if (osip_list_size(pGblconf->pLocalIPAddrList) <= 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "get_ip_eth_name() exit---: Local IP Addr List NULL \r\n");
        return NULL;
    }

    for (i = 0; i < osip_list_size(pGblconf->pLocalIPAddrList); i++)
    {
        pIPaddr = (ip_pair_t*)osip_list_get(pGblconf->pLocalIPAddrList, i);

        if (NULL == pIPaddr || pIPaddr->local_ip[0] == '\0')
        {
            continue;
        }

        if (0 == sstrcmp(server_ip, pIPaddr->local_ip))
        {
            return pIPaddr->eth_name;
        }
    }

    return NULL;
}

/*****************************************************************************
 函 数 名  : ntp_server_ip_get
 功能描述  : 获取NTP服务器的IP地址
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年10月31日
    作    者   : 用户路由信息清理
    修改内容   : 新生成函数

*****************************************************************************/
char* ntp_server_ip_get()
{
    if (pGblconf == NULL)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "ntp_server_ip_get() exit---: Param Error \r\n");
        return NULL;
    }

    return pGblconf->ntp_server_ip;
}

/*****************************************************************************
 函 数 名  : local_cms_id_get
 功能描述  : 本地ID获取
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年6月19日 星期三
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
char* local_cms_id_get()
{
    if (pGblconf == NULL)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "local_cms_id_get() exit---: Param Error \r\n");
        return NULL;
    }

    return pGblconf->board_id;
}

/*****************************************************************************
 函 数 名  : local_mms_id_get
 功能描述  : 本地MMS ID获取
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年8月12日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
char* local_mms_id_get()
{
    if (pGblconf == NULL)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "local_mms_id_get() exit---: Param Error \r\n");
        return NULL;
    }

    return pGblconf->mms_id;
}

/*****************************************************************************
 函 数 名  : local_cms_name_get
 功能描述  : 本地CMS别名获取
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年11月28日 星期五
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
char* local_cms_name_get()
{
    if (pGblconf == NULL)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "local_cms_name_get() exit---: Param Error \r\n");
        return NULL;
    }

    return pGblconf->cms_name;
}

/*****************************************************************************
 函 数 名  : local_cms_index_get
 功能描述  : 获取本地cms单板的数据库索引
 输入参数  : 无
 输出参数  : 无
 返 回 值  : unsigned
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年4月8日 星期二
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
unsigned int local_cms_index_get()
{
    if (pGblconf == NULL)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "local_cms_index_get() exit---: Param Error \r\n");
        return 0;
    }

    return pGblconf->board_index;
}

/*****************************************************************************
 函 数 名  : local_session_expires_get
 功能描述  : 获取会话超时时间
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年9月2日 星期一
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int local_session_expires_get()
{
    if (pGblconf == NULL)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "local_session_expires_get() exit---: Param Error \r\n");
        return -1;
    }

    return pGblconf->session_expires;
}

/*****************************************************************************
 函 数 名  : local_subscribe_expires_get
 功能描述  : 获取订阅超时时间
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年6月16日 星期二
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int local_subscribe_expires_get()
{
    if (pGblconf == NULL)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "local_subscribe_expires_get() exit---: Param Error \r\n");
        return -1;
    }

    return pGblconf->subscribe_expires;
}

/*****************************************************************************
 函 数 名  : local_keep_alive_expires_get
 功能描述  : 获取保活超时时间设置
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年7月7日 星期二
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int local_keep_alive_expires_get()
{
    if (pGblconf == NULL)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "local_keep_alive_expires_get() exit---: Param Error \r\n");
        return -1;
    }

    return pGblconf->keep_alive_expires;
}

/*****************************************************************************
 函 数 名  : local_preset_back_time_get
 功能描述  : 获取本地配置的预置位自动归位时间
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年11月18日 星期二
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int local_preset_back_time_get()
{
    if (pGblconf == NULL)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "local_preset_back_time_get() exit---: Param Error \r\n");
        return -1;
    }

    return pGblconf->uPresetBackTime;
}

/*****************************************************************************
 函 数 名  : local_device_unlock_time_get
 功能描述  : 获取本地配置的点位自动解锁时间
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年11月18日 星期二
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int local_device_unlock_time_get()
{
    if (pGblconf == NULL)
    {
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "local_device_unlock_time_get() exit---: Param Error \r\n");
        return -1;
    }

    return pGblconf->uDeviceUnLockTime;
}

/*****************************************************************************
 函 数 名  : local_register_retry_interval_get
 功能描述  : 获取重新发起注册超时时间
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年7月15日 星期二
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int local_register_retry_interval_get()
{
    if (pGblconf == NULL)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "local_register_retry_interval_get() exit---: Param Error \r\n");
        return -1;
    }

    return pGblconf->register_retry_interval;
}

/*****************************************************************************
 函 数 名  : local_registry_cleanuprate_get
 功能描述  : 获取注册刷新时间
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年7月15日 星期二
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int local_registry_cleanuprate_get()
{
    if (pGblconf == NULL)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "local_registry_cleanuprate_get() exit---: Param Error \r\n");
        return -1;
    }

    return pGblconf->registry_cleanuprate;
}

/*****************************************************************************
 函 数 名  : local_keep_alive_interval_get
 功能描述  : 获取保活间隔时间
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年7月15日 星期二
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int local_keep_alive_interval_get()
{
    if (pGblconf == NULL)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "local_keep_alive_interval_get() exit---: Param Error \r\n");
        return -1;
    }

    return pGblconf->keep_alive_interval;
}

/*****************************************************************************
 函 数 名  : local_failed_keep_alive_count_get
 功能描述  : 获取保活失败次数
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年3月5日 星期四
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int local_failed_keep_alive_count_get()
{
    if (pGblconf == NULL)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "local_failed_keep_alive_count_get() exit---: Param Error \r\n");
        return -1;
    }

    return pGblconf->failed_keep_alive_count;
}

/*****************************************************************************
 函 数 名  : local_alarm_duration_get
 功能描述  : 获取告警延时时间
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年1月21日 星期三
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int local_alarm_duration_get()
{
    if (pGblconf == NULL)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "local_alarm_duration_get() exit---: Param Error \r\n");
        return -1;
    }

    return pGblconf->alarm_duration;
}

/*****************************************************************************
 函 数 名  : local_civil_code_get
 功能描述  : 本地行政编码获取
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年9月6日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
char* local_civil_code_get()
{
    if (pGblconf == NULL)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "local_civil_code_get() exit---: Param Error \r\n");
        return NULL;
    }

    return pGblconf->civil_code;
}

/*****************************************************************************
 函 数 名  : default_eth_name_get
 功能描述  : 获取默认的网口名称
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年8月15日 星期五
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
char* default_eth_name_get()
{
    if (pGblconf == NULL)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "default_eth_name_get() exit---: Param Error \r\n");
        return NULL;
    }

    return pGblconf->default_eth_name;
}

/*****************************************************************************
 函 数 名  : shdb_agent_id_get
 功能描述  : 上海地标本地AgentID获取
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年3月14日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
char* shdb_agent_id_get()
{
    if (pGblconf == NULL)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "shdb_agent_id_get() exit---: Param Error \r\n");
        return NULL;
    }

    return pGblconf->shdb_agent_id;
}

/*****************************************************************************
 函 数 名  : shdb_server_ip_get
 功能描述  : 上海地标监管平台服务器IP地址获取
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年3月14日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
char* shdb_server_ip_get()
{
    if (pGblconf == NULL)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "shdb_server_ip_get() exit---: Param Error \r\n");
        return NULL;
    }

    return pGblconf->shdb_server_ip;
}

/*****************************************************************************
 函 数 名  : shdb_prex_sec_get
 功能描述  : 上海地标报警图像提前上传时长获取
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年3月14日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int shdb_prex_sec_get()
{
    if (pGblconf == NULL)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "shdb_prex_sec_get() exit---: Param Error \r\n");
        return -1;
    }

    return pGblconf->shdb_prex_seconds;
}

/*****************************************************************************
 函 数 名  : shdb_next_sec_get
 功能描述  : 上海地标报警图像上传延后时长获取
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年3月14日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int shdb_next_sec_get()
{
    if (pGblconf == NULL)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "shdb_next_sec_get() exit---: Param Error \r\n");
        return -1;
    }

    return pGblconf->shdb_next_seconds;
}

/*****************************************************************************
 函 数 名  : shdb_interval_sec_get
 功能描述  : 上海地标报警图像上传间隔时长获取
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年3月14日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int shdb_interval_sec_get()
{
    if (pGblconf == NULL)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "shdb_interval_sec_get() exit---: Param Error \r\n");
        return -1;
    }

    return pGblconf->shdb_interval_seconds;
}

/*****************************************************************************
 函 数 名  : sys_exit_flag_get
 功能描述  : 获取系统退出标识
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年3月15日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int sys_exit_flag_get()
{
    if (pGblconf == NULL)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "sys_exit_flag_get() exit---: Param Error \r\n");
        return -1;
    }

    return pGblconf->sys_exit_flag;
}

/*****************************************************************************
 函 数 名  : sys_show_code_flag_get
 功能描述  : 获取系统国标模式
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年9月9日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int sys_show_code_flag_get()
{
    if (pGblconf == NULL)
    {
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "sys_show_code_flag_get() exit---: Param Error \r\n");
        return -1;
    }

    return pGblconf->showcode;
}

/*****************************************************************************
 函 数 名  : gbl_conf_load
 功能描述  : 全局配置信息加载
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年6月19日 星期三
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int gbl_conf_load()
{
    int i = 0;
    int default_pos = 0;
    ip_pair_t* pIPaddr = NULL;
    char video_net_ip[MAX_IP_LEN] = {0};
    int video_net_port = 0;
    char device_net_ip[MAX_IP_LEN] = {0};
    int device_net_port = 0;
    char strEthName[MAX_IP_LEN] = {0};

    /* 增加删除路由信息 */
    //route add -net 192.168.0.0 netmask 255.255.0.0 gw 192.168.1.1 dev eth1
    //route del -net 192.168.0.0 netmask 255.255.0.0 gw 192.168.1.1 dev eth1

    struct in_addr ip_addr = {0};

    if (pGblconf == NULL)
    {
        i = gbl_conf_init();

        if (i != 0)
        {
            printf(" gbl_conf_load() exit---: Gbl Conf Init Error \r\n");
            return -1;
        }
    }
    else
    {
        (void)gbl_conf_free();

        i = gbl_conf_init();

        if (i != 0)
        {
            printf(" gbl_conf_load() exit---: Gbl Conf Init Error \r\n");
            return -1;
        }
    }

    /* 加载配置文件的配置 */
    i = load_file_config();

    if (i != 0)
    {
        printf(" gbl_conf_load() exit---: Load File Config Error \r\n");
        //return -1;//可能没有配置文件，下面的参数用默认值，不能返回
    }

    if (g_BoardNetConfig.tCmsUsingVideoIP.tNetIP.dwPort > 0 && g_BoardNetConfig.tCmsUsingVideoIP.tNetIP.dwPort != 0xFFFFFFFF)
    {
        video_net_port = g_BoardNetConfig.tCmsUsingVideoIP.tNetIP.dwPort;
    }
    else
    {
        video_net_port = 5060;
    }

    /* 本地IP地址 */
    i = GetSystemIPAddr(video_net_port);

    if (i != 0)
    {
        printf(" gbl_conf_load() exit---: GetSystemIPAddr Error \r\n");
        return -1;
    }

    if (osip_list_size(pGblconf->pLocalIPAddrList) <= 0)
    {
        printf(" gbl_conf_load() exit---: Local IP Addr List NULL \r\n");
        return -1;
    }

    /* 数据库IP地址 */
    if (g_BoardNetConfig.tCmsDBIP.tNetIP.dwIPAddr > 0 && g_BoardNetConfig.tCmsDBIP.tNetIP.dwIPAddr != 0xFFFFFFFF)
    {
        //ipaddr_long2str(pGblconf->db_server_ip , g_BoardNetConfig.tCmsDBIP.tNetIP.dwIPAddr);
        memset(&ip_addr, 0, sizeof(struct in_addr));
        ip_addr.s_addr = g_BoardNetConfig.tCmsDBIP.tNetIP.dwIPAddr;
        osip_strncpy(pGblconf->db_server_ip, inet_ntoa(ip_addr), MAX_IP_LEN);
    }

    /* 获取最后一个IP地址的网口信息，设为默认值,因为添加网口的时候是从后往前添加 */
    default_pos = osip_list_size(pGblconf->pLocalIPAddrList) - 1;

    if (NULL != pGblconf->pLocalIPAddrList)
    {
        pIPaddr = (ip_pair_t*)osip_list_get(pGblconf->pLocalIPAddrList, default_pos);

        if (NULL != pIPaddr)
        {
            if (pIPaddr->eth_name[0] != '\0')
            {
                osip_strncpy(pGblconf->default_eth_name, pIPaddr->eth_name, MAX_IP_LEN);
            }
            else
            {
                osip_strncpy(pGblconf->default_eth_name, (char*)"eth0", MAX_IP_LEN);
            }
        }
        else
        {
            osip_strncpy(pGblconf->default_eth_name, (char*)"eth0", MAX_IP_LEN);
        }
    }
    else
    {
        osip_strncpy(pGblconf->default_eth_name, (char*)"eth0", MAX_IP_LEN);
    }

    snprintf(g_strDBThreadPara[0], 100, "%s;root;zbitcloud;EV9000DB;3306", pGblconf->db_server_ip);
    snprintf(g_strDBThreadPara[1], 100, "%s;root;zbitcloud;EV9000TSU;3306", pGblconf->db_server_ip);
    snprintf(g_strDBThreadPara[2], 100, "%s;root;zbitcloud;EV9000LOG;3306", pGblconf->db_server_ip);
    snprintf(g_strDBThreadPara[3], 100, "%s;root;zbitcloud;EV9000DB_MY;3306", pGblconf->db_server_ip);

    snprintf(g_StrCon[0], 100, "%s;root;zbitcloud;EV9000DB;3306", pGblconf->db_server_ip);
    memset(g_StrCon[1], 0, 100);

    snprintf(g_StrConLog[0], 100, "%s;root;zbitcloud;EV9000LOG;3306", pGblconf->db_server_ip);
    memset(g_StrConLog[1], 0, 100);

    printf("\r\n CMS全局参数: \r\n");
    printf(" gbl_conf_load() CMS编码:CMSID=%s \r\n", pGblconf->board_id);
    printf(" gbl_conf_load() 对应的MMS编码:MMS ID=%s \r\n", pGblconf->mms_id);
    printf(" gbl_conf_load() CMS别名:cms_name=%s \r\n", pGblconf->cms_name);
    printf(" gbl_conf_load() 中心编码:center_code=%s \r\n", pGblconf->center_code);
    printf(" gbl_conf_load() 省级编码:province_code=%s \r\n", pGblconf->province_code);
    printf(" gbl_conf_load() 市级编码:city_code=%s \r\n", pGblconf->city_code);
    printf(" gbl_conf_load() 区县编码:region_code=%s \r\n", pGblconf->region_code);
    printf(" gbl_conf_load() 本CMS的行政区域编码:civil_code=%s \r\n", pGblconf->civil_code);

    printf(" gbl_conf_load() 行业编码:trade_code=%s \r\n", pGblconf->trade_code);
    printf(" gbl_conf_load() 类型编码:type_code=%s \r\n", pGblconf->type_code);
    printf(" gbl_conf_load() 序号:serial_number=%s \r\n", pGblconf->serial_number);
    printf(" gbl_conf_load() 注册域:register_region=%s \r\n", pGblconf->register_region);

    printf(" gbl_conf_load() 数据库IP地址:db_server_ip=%s \r\n", pGblconf->db_server_ip);
    printf(" gbl_conf_load() 告警服务器IP地址:alarm_server_ip=%s \r\n", pGblconf->alarm_server_ip);
    printf(" gbl_conf_load() NTP服务器IP地址:ntp_server_ip=%s \r\n", pGblconf->ntp_server_ip);

    printf(" gbl_conf_load() 默认网口名称:default_eth_name=%s \r\n", pGblconf->default_eth_name);
    printf(" gbl_conf_load() 设备认证标识:AuthFlag=%d \r\n", pGblconf->iAuthFlag);
    printf(" gbl_conf_load() 预置位自动归位时长:PresetBackTime=%u \r\n", pGblconf->uPresetBackTime);
    printf(" gbl_conf_load() 点位自动解锁时长:DeviceUnLockTime=%u \r\n", pGblconf->uDeviceUnLockTime);
    printf(" gbl_conf_load() 日志是否记录到数据库:IsLog2DB=%d \r\n", g_IsLog2DB);
    printf(" gbl_conf_load() 日志是否记录到文件:IsLog2File=%d \r\n", g_IsLog2File);
    printf(" gbl_conf_load() 是否记录SIP消息日志:g_IsLogSIPMsg=%d \r\n", g_IsLogSIPMsg);
    printf(" gbl_conf_load() 系统调试日志等级:g_DbgLevel=%d \r\n", g_CommonDbgLevel);
    printf(" gbl_conf_load() 系统运行日志等级:g_SystemLogLevel=%d \r\n", g_SystemLogLevel);
    printf(" gbl_conf_load() 系统运行日志记录到数据库等级:g_SystemLog2DBLevel=%d \r\n", g_SystemLog2DBLevel);
    printf(" gbl_conf_load() 是否发送订阅消息:IsSubscribe=%d \r\n", g_IsSubscribe);
    printf(" gbl_conf_load() 报警消息是否发送给用户:AlarmMsgSendToUserFlag=%d \r\n", g_AlarmMsgSendToUserFlag);
    printf(" gbl_conf_load() 报警消息是否发送给上级路由:AlarmMsgSendToRouteFlag=%d \r\n", g_AlarmMsgSendToRouteFlag);
    printf(" gbl_conf_load() 下级媒体流是否经过本地TSU转发:LocalMediaTransferFlag=%d \r\n", g_LocalMediaTransferFlag);
    printf(" gbl_conf_load() 下级媒体切电视墙是否经过本地TSU转发:DECMediaTransferFlag=%d \r\n", g_DECMediaTransferFlag);
    printf(" gbl_conf_load() 上级第三方平台是否有媒体转发功能:RouteMediaTransferFlag=%d \r\n", g_RouteMediaTransferFlag);
    printf(" gbl_conf_load() 是否付费:IsPay=%d \r\n", g_IsPay);
    printf(" gbl_conf_load() 主备启用标识:MSFlag=%d \r\n", g_BoardNetConfig.dwMSFlag);
    printf(" gbl_conf_load() 系统使用的语言:Language=%d \r\n", g_Language);
    printf(" gbl_conf_load() SX、RX是否启用MMS功能:MMSEnableFlag=%d \r\n", g_MMSEnableFlag);
    printf(" gbl_conf_load() 下级平台分组信息解析标识:AnalysisSubGroupFlag=%d \r\n", g_AnalysisSubGroupFlag);
    printf(" gbl_conf_load() 日志缓存队列大小:LogQueryBufferSize=%d \r\n", g_LogQueryBufferSize);

    printf(" gbl_conf_load() 上海地标监管平台注册AgentID:shdb_agent_id=%s \r\n", pGblconf->shdb_agent_id);
    printf(" gbl_conf_load() 上海地标监管平台注册服务器IP:shdb_server_ip=%s \r\n", pGblconf->shdb_server_ip);
    printf(" gbl_conf_load() 上海地标监管报警图片上传提前时长:shdb_prex_seconds=%d \r\n", pGblconf->shdb_prex_seconds);
    printf(" gbl_conf_load() 上海地标监管报警图片上传延后时长:shdb_next_seconds=%d \r\n", pGblconf->shdb_next_seconds);
    printf(" gbl_conf_load() 上海地标监管报警图片上传间隔时长:shdb_interval_seconds=%d \r\n", pGblconf->shdb_interval_seconds);
    printf(" gbl_conf_load() 系统上次退出标识:sys_exit_flag=%d \r\n", pGblconf->sys_exit_flag);
    printf(" gbl_conf_load() 是否为国标:showcode=%d \r\n", pGblconf->showcode);

    printf("\r\n 数据库连接参数: \r\n");
    printf(" gbl_conf_load() g_strDBThreadPara[0]=%s \r\n", g_strDBThreadPara[0]);
    printf(" gbl_conf_load() g_strDBThreadPara[1]=%s \r\n", g_strDBThreadPara[1]);
    printf(" gbl_conf_load() g_strDBThreadPara[2]=%s \r\n", g_strDBThreadPara[2]);
    printf(" gbl_conf_load() g_strDBThreadPara[3]=%s \r\n", g_strDBThreadPara[3]);
    printf(" gbl_conf_load() g_StrCon[0]=%s \r\n", g_StrCon[0]);
    printf(" gbl_conf_load() g_StrCon[1]=%s \r\n", g_StrCon[1]);
    printf(" gbl_conf_load() g_StrConLog[0]=%s \r\n", g_StrConLog[0]);
    printf(" gbl_conf_load() g_StrConLog[1]=%s \r\n", g_StrConLog[1]);

    return 0;
}

/*****************************************************************************
 函 数 名  : gbl_conf_reload
 功能描述  : 全局配置信息重新加载
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年6月19日 星期三
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int gbl_conf_reload()
{
    int i = 0;
    char strDeviceType[16] = {0};

    if (pGblconf == NULL)
    {
        i = gbl_conf_init();

        if (i != 0)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "gbl_conf_reload() exit---: Gbl Conf Init Error \r\n");
            return -1;
        }
    }

    /* 加载配置文件的配置 */
    i = reload_file_config();

    if (i < 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "gbl_conf_reload() exit---: Load File Config Error \r\n");
        return -1;
    }
    else if (i > 0)
    {
        /* 添加拓扑结构表信息 */
        snprintf(strDeviceType, 16, "%u", EV9000_DEVICETYPE_SIPSERVER);
        i = AddTopologyPhyDeviceInfo2DB(pGblconf->board_id, pGblconf->cms_name, strDeviceType, local_ip_get(pGblconf->default_eth_name), (char*)"1", local_cms_id_get(), (char*)"0", &g_DBOper);
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : conf_reset
 功能描述  : 重新设置配置信息到内存
 输入参数  : gbl_conf_t* conf
             char* pname
             char* pvalue
             int iRefresh
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年7月22日 星期一
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int conf_reset(gbl_conf_t* conf, char* pname, char* pvalue, int iRefresh)
{
    int iTmp = 0;
    FILE* fp = NULL;
    char str[200] = {0};
    char province_code[4] = {0};                 /* 省级编码 */
    char city_code[4] = {0};                     /* 市级编码 */
    char region_code[4] = {0};                   /* 区县编码 */
    char primary_code[4] = {0};                  /* 底层编码 */
    char tmp1[20] = {0};
    char tmp2[2] = {0};
    int tmp = 0;

    if (conf == NULL || pname == NULL || pvalue == NULL)
    {
        return -1;
    }

    if (strcasecmp(pname, CONF_BOARD_ID) == 0) /* CMSID */
    {
        if (0 != sstrcmp(conf->board_id, pvalue))
        {
            memset(conf->board_id, 0, MAX_ID_LEN + 4);
            osip_strncpy(conf->board_id, pvalue, MAX_ID_LEN);

            if (iRefresh)
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, " conf_reset(): board_id=%s \r\n", conf->board_id);
            }
            else
            {
                printf(" conf_reset() board_id=%s \r\n", conf->board_id);
            }

            if (conf->board_id[0] != '\0')
            {
                memset(conf->center_code, 0, 12);

                memset(conf->province_code, 0, 4);
                memset(conf->city_code, 0, 8);
                memset(conf->region_code, 0, 12);
                memset(conf->civil_code, 0, 12);

                memset(conf->trade_code, 0, 4);
                memset(conf->type_code, 0, 4);
                memset(conf->serial_number, 0, 8);
                memset(conf->register_region, 0, 12);

                osip_strncpy(conf->center_code, conf->board_id, 8);

                /* 获取行政区域编码 */
                memset(province_code, 0, 4);
                memset(city_code, 0, 4);
                memset(region_code, 0, 4);
                memset(primary_code, 0, 4);
                osip_strncpy(province_code, conf->board_id, 2);
                osip_strncpy(city_code, &conf->board_id[2], 2);
                osip_strncpy(region_code, &conf->board_id[4], 2);
                osip_strncpy(primary_code, &conf->board_id[6], 2);

                /* 确定行政区域编码 */
                if (0 != sstrcmp(province_code, (char*)"00")
                    && 0 == sstrcmp(city_code, (char*)"00")
                    && 0 == sstrcmp(region_code, (char*)"00")
                    && 0 == sstrcmp(primary_code, (char*)"00"))
                {
                    osip_strncpy(conf->province_code, conf->board_id, 2);
                    osip_strncpy(conf->city_code, conf->board_id, 4);
                    osip_strncpy(conf->region_code, conf->board_id, 6);
                    osip_strncpy(conf->civil_code, conf->board_id, 2);
                }
                else if (0 != sstrcmp(city_code, (char*)"00")
                         && 0 == sstrcmp(region_code, (char*)"00")
                         && 0 == sstrcmp(primary_code, (char*)"00"))
                {
                    osip_strncpy(conf->province_code, conf->board_id, 2);
                    osip_strncpy(conf->city_code, conf->board_id, 4);
                    osip_strncpy(conf->region_code, conf->board_id, 6);
                    osip_strncpy(conf->civil_code, conf->board_id, 4);
                }
                else if (0 != sstrcmp(region_code, (char*)"00")
                         && 0 == sstrcmp(primary_code, (char*)"00"))
                {
                    osip_strncpy(conf->province_code, conf->board_id, 2);
                    osip_strncpy(conf->city_code, conf->board_id, 4);
                    osip_strncpy(conf->region_code, conf->board_id, 6);
                    osip_strncpy(conf->civil_code, conf->board_id, 6);
                }
                else if (0 != sstrcmp(primary_code, (char*)"00"))
                {
                    osip_strncpy(conf->province_code, conf->board_id, 2);
                    osip_strncpy(conf->city_code, conf->board_id, 4);
                    osip_strncpy(conf->region_code, conf->board_id, 6);
                    osip_strncpy(conf->civil_code, conf->board_id, 8);
                }

                osip_strncpy(conf->trade_code, &conf->board_id[8], 2);
                osip_strncpy(conf->type_code, &conf->board_id[10], 3);
                osip_strncpy(conf->serial_number, &conf->board_id[13], 7);
                osip_strncpy(conf->register_region, conf->board_id, 10);

                osip_strncpy(tmp1, conf->board_id, 19);
                osip_strncpy(tmp2, &conf->board_id[19], 1);
                tmp = osip_atoi(tmp2);
                tmp = tmp + 1; /* 可能是9，用16进制 */
                snprintf(conf->mms_id, MAX_ID_LEN + 4, "%s%x", tmp1, tmp);

                if (iRefresh)
                {

                }
            }
        }
    }
    else if (strcasecmp(pname, CONF_ALARM_SERVER_IP) == 0) /* 告警服务器IP */
    {
        if (0 != sstrcmp(conf->alarm_server_ip, pvalue))
        {
            memset(conf->alarm_server_ip, 0, MAX_IP_LEN);
            osip_strncpy(conf->alarm_server_ip, pvalue, MAX_IP_LEN);

            if (iRefresh)
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, " conf_reset() alarm_server_ip=%s \r\n", conf->alarm_server_ip);
            }
            else
            {
                printf(" conf_reset() alarm_server_ip=%s \r\n", conf->alarm_server_ip);
            }
        }
    }
    else if (strcasecmp(pname, CONF_NTP_SERVER_IP) == 0) /* NTP服务器IP */
    {
        if (0 != sstrcmp(conf->ntp_server_ip, pvalue))
        {
            memset(conf->ntp_server_ip, 0, MAX_IP_LEN);
            osip_strncpy(conf->ntp_server_ip, pvalue, MAX_IP_LEN);

            if (iRefresh)
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, " conf_reset() ntp_server_ip=%s \r\n", conf->ntp_server_ip);
            }
            else
            {
                printf(" conf_reset() ntp_server_ip=%s \r\n", conf->ntp_server_ip);
            }

            /* 写入NTP配置文件 */
            fp = fopen(NTP_CONFIG_FILE, "w+");

            if (NULL == fp)
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "I CAN NOT create ntp config file,please check if you have privilege!!!\n");
                return -1;
            }

            fputs((char*)"driftfile /var/lib/ntp/ntp.drift\n", fp);
            fputs((char*)"statistics loopstats peerstats clockstats\n", fp);
            fputs((char*)"filegen loopstats file loopstats type day enable\n", fp);
            fputs((char*)"filegen peerstats file peerstats type day enable\n", fp);
            fputs((char*)"filegen clockstats file clockstats type day enable\n", fp);
            fputs((char*)"restrict -4 default kod notrap nomodify nopeer noquery\n", fp);
            fputs((char*)"restrict -6 default kod notrap nomodify nopeer noquery\n", fp);
            fputs((char*)"restrict 127.0.0.1\n", fp);
            fputs((char*)"restrict ::1\n", fp);
            fputs((char*)"minpool 8\n", fp);
            fputs((char*)"maxpool 8\n", fp);

            snprintf(str, 200, "server %s iburst\n", conf->ntp_server_ip);
            fputs(str, fp);

            fclose(fp);

            /* 写入NTP脚本文件 */
            fp = fopen(NTPDATE_SH_FILE, "w+");

            if (NULL == fp)
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "I CAN NOT create ntp date sh file,please check if you have privilege!!!\n");
                return -1;
            }

            snprintf(str, 200, "ntpdate %s", conf->ntp_server_ip);
            fputs(str, fp);

            fclose(fp);

            /* 改变文件属性 */
            snprintf(str, 200, "chmod 777 %s", NTPDATE_SH_FILE);
            system(str);

            /* 重启NTPD进程 */
            system((char*)"killall ntpd; killall -9 ntpd");
        }
    }
    else if (strcasecmp(pname, CONF_DBG_LOG_LEVEL) == 0) /* dbg调试打印日志等级 */
    {
        iTmp = osip_atoi(pvalue);

        if (iTmp != g_CommonDbgLevel)
        {
            if (iTmp >= 1 && iTmp <= 8)
            {
                g_CommonDbgLevel = iTmp;
                g_SIPStackDbgLevel = iTmp;
                g_UserDbgLevel = iTmp;
                g_DeviceDbgLevel = iTmp;
                g_RouteDbgLevel = iTmp;
                g_RecordDbgLevel = iTmp;
                g_ResourceDbgLevel = iTmp;
                g_CruiseDbgLevel = iTmp;
                g_PlanDbgLevel = iTmp;
                g_PollDbgLevel = iTmp;
            }
            else
            {
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

            if (iRefresh)
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "conf_reset() g_DbgLevel=%d \r\n", g_CommonDbgLevel);
            }
            else
            {
                printf(" conf_reset() g_DbgLevel=%d \r\n", g_CommonDbgLevel);
            }
        }
    }
    else if (strcasecmp(pname, CONF_RUN_LOG_LEVEL) == 0) /* 运行打印日志等级 */
    {
        iTmp = osip_atoi(pvalue);

        if (iTmp != g_SystemLogLevel)
        {
            if (iTmp >= 1 && iTmp <= 4)
            {
                g_SystemLogLevel = iTmp;
            }
            else
            {
                g_SystemLogLevel = EV9000_LOG_LEVEL_ERROR;
            }

            if (iRefresh)
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "conf_reset() g_SystemLogLevel=%d \r\n", g_SystemLogLevel);
            }
            else
            {
                printf(" conf_reset() g_SystemLogLevel=%d \r\n", g_SystemLogLevel);
            }
        }
    }
    else if (strcasecmp(pname, CONF_RUN_LOG_2DB_LEVEL) == 0) /* 运行日志记录到数据库等级 */
    {
        iTmp = osip_atoi(pvalue);

        if (iTmp != g_SystemLog2DBLevel)
        {
            if (iTmp >= 1 && iTmp <= 4)
            {
                g_SystemLog2DBLevel = iTmp;
            }
            else
            {
                g_SystemLog2DBLevel = EV9000_LOG_LEVEL_ERROR;
            }

            if (iRefresh)
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "conf_reset() g_SystemLog2DBLevel=%d \r\n", g_SystemLog2DBLevel);
            }
            else
            {
                printf(" conf_reset() g_SystemLog2DBLevel=%d \r\n", g_SystemLog2DBLevel);
            }
        }
    }
    else if (strcasecmp(pname, CONF_LOG2FILE_FLAG) == 0) /* 日志记录到文件开关 */
    {
        iTmp = osip_atoi(pvalue);

        if (iTmp != g_IsLog2File)
        {
            if (iTmp > 0)
            {
                g_IsLog2File = 1;
            }
            else
            {
                g_IsLog2File = 0;
            }

            if (iRefresh)
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "conf_reset() g_IsLog2File=%d \r\n", g_IsLog2File);
            }
            else
            {
                printf(" conf_reset() g_IsLog2File=%d \r\n", g_IsLog2File);
            }
        }
    }
    else if (strcasecmp(pname, CONF_LOG2DB_FLAG) == 0) /* 日志记录到数据库开关 */
    {
        iTmp = osip_atoi(pvalue);

        if (iTmp != g_IsLog2DB)
        {
            if (iTmp > 0)
            {
                g_IsLog2DB = 1;
            }
            else
            {
                g_IsLog2DB = 0;
            }

            if (iRefresh)
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "conf_reset() g_IsLog2DB=%d \r\n", g_IsLog2DB);
            }
            else
            {
                printf(" conf_reset() g_IsLog2DB=%d \r\n", g_IsLog2DB);
            }
        }
    }
    else if (strcasecmp(pname, CONF_LOGSIPMSG_FLAG) == 0) /* 是否记录SIP Message开关 */
    {
        iTmp = osip_atoi(pvalue);

        if (iTmp != g_IsLogSIPMsg)
        {
            if (iTmp > 0)
            {
                g_IsLogSIPMsg = 1;
            }
            else
            {
                g_IsLogSIPMsg = 0;
            }

            if (iRefresh)
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "conf_reset() g_IsLogSIPMsg=%d \r\n", g_IsLogSIPMsg);
            }
            else
            {
                printf(" conf_reset() g_IsLogSIPMsg=%d \r\n", g_IsLogSIPMsg);
            }
        }
    }
    else if (strcasecmp(pname, CONF_IS_PAY) == 0) /* 是否付费 */
    {
        iTmp = osip_atoi(pvalue);

        if (iTmp != g_IsPay)
        {
            if (iTmp >= 0)
            {
                g_IsPay = iTmp;
            }
            else
            {
                g_IsPay = 1;
            }

            if (iRefresh)
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "conf_reset() g_IsPay=%d \r\n", g_IsPay);
            }
            else
            {
                printf(" conf_reset() g_IsPay=%d \r\n", g_IsPay);
            }
        }
    }
    else if (strcasecmp(pname, CONF_LANGUAGE) == 0) /* 系统语言 */
    {
        iTmp = osip_atoi(pvalue);

        if (iTmp != g_Language)
        {
            if (iTmp >= 0)
            {
                g_Language = iTmp;
            }
            else
            {
#ifdef LANG_EN
                g_Language = 1;              /* 语言，0:中文；1:英文，默认英文 */
#else
                g_Language = 0;              /* 语言，0:中文；1:英文，默认中文 */
#endif
            }

            if (iRefresh)
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "conf_reset() g_Language=%d \r\n", g_Language);
            }
            else
            {
                printf(" conf_reset() g_Language=%d \r\n", g_Language);
            }
        }
    }
    else if (strcasecmp(pname, CONF_IS_SUBSCRIBE) == 0) /* 是否订阅 */
    {
        iTmp = osip_atoi(pvalue);

        if (iTmp != g_IsSubscribe)
        {
            if (iTmp >= 0)
            {
                g_IsSubscribe = iTmp;
            }
            else
            {
                g_IsSubscribe = 1;
            }

            if (iRefresh)
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "conf_reset() g_IsSubscribe=%d \r\n", g_IsSubscribe);
            }
            else
            {
                printf(" conf_reset() g_IsSubscribe=%d \r\n", g_IsSubscribe);
            }
        }
    }
    else if (strcasecmp(pname, CONF_ALARM_SEND_TO_USER_FLAG) == 0) /* 报警消息发送给用户标识 */
    {
        iTmp = osip_atoi(pvalue);

        if (iTmp != g_AlarmMsgSendToUserFlag)
        {
            if (iTmp >= 0)
            {
                g_AlarmMsgSendToUserFlag = iTmp;
            }
            else
            {
                g_AlarmMsgSendToUserFlag = 0;
            }

            if (iRefresh)
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "conf_reset() g_AlarmMsgSendToUserFlag=%d \r\n", g_AlarmMsgSendToUserFlag);
            }
            else
            {
                printf(" conf_reset() g_AlarmMsgSendToUserFlag=%d \r\n", g_AlarmMsgSendToUserFlag);
            }
        }
    }
    else if (strcasecmp(pname, CONF_ALARM_SEND_TO_ROUTE_FLAG) == 0) /* 报警消息发送给上级路由标识 */
    {
        iTmp = osip_atoi(pvalue);

        if (iTmp != g_AlarmMsgSendToRouteFlag)
        {
            if (iTmp >= 0)
            {
                g_AlarmMsgSendToRouteFlag = iTmp;
            }
            else
            {
                g_AlarmMsgSendToRouteFlag = 0;
            }

            if (iRefresh)
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "conf_reset() g_AlarmMsgSendToRouteFlag=%d \r\n", g_AlarmMsgSendToRouteFlag);
            }
            else
            {
                printf(" conf_reset() g_AlarmMsgSendToRouteFlag=%d \r\n", g_AlarmMsgSendToRouteFlag);
            }
        }
    }
    else if (strcasecmp(pname, CONF_MEDIA_TRANSFER_FLAG) == 0) /* 下级媒体是否经过本地媒体服务器转发 */
    {
        iTmp = osip_atoi(pvalue);

        if (iTmp != g_LocalMediaTransferFlag)
        {
            if (iTmp >= 0)
            {
                g_LocalMediaTransferFlag = iTmp;
            }
            else
            {
                g_LocalMediaTransferFlag = 1;
            }

            if (iRefresh)
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "conf_reset() g_LocalMediaTransferFlag=%d \r\n", g_LocalMediaTransferFlag);
            }
            else
            {
                printf(" conf_reset() g_LocalMediaTransferFlag=%d \r\n", g_LocalMediaTransferFlag);
            }
        }
    }
    else if (strcasecmp(pname, CONF_DEC_MEDIA_TRANSFER_FLAG) == 0) /* 下级媒体切电视墙是否经过本地媒体服务器转发 */
    {
        iTmp = osip_atoi(pvalue);

        if (iTmp != g_DECMediaTransferFlag)
        {
            if (iTmp >= 0)
            {
                g_DECMediaTransferFlag = iTmp;
            }
            else
            {
                g_DECMediaTransferFlag = 1;
            }

            if (iRefresh)
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "conf_reset() g_DECMediaTransferFlag=%d \r\n", g_DECMediaTransferFlag);
            }
            else
            {
                printf(" conf_reset() g_DECMediaTransferFlag=%d \r\n", g_DECMediaTransferFlag);
            }
        }
    }
    else if (strcasecmp(pname, CONF_ROUTE_MEDIA_TRANS_FLAG) == 0) /* 上级第三方平台是否支持媒体转发 */
    {
        iTmp = osip_atoi(pvalue);

        if (iTmp != g_RouteMediaTransferFlag)
        {
            if (iTmp >= 0)
            {
                g_RouteMediaTransferFlag = iTmp;
            }
            else
            {
                g_RouteMediaTransferFlag = 1;
            }

            if (iRefresh)
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "conf_reset() g_RouteMediaTransferFlag=%d \r\n", g_RouteMediaTransferFlag);
            }
            else
            {
                printf(" conf_reset() g_RouteMediaTransferFlag=%d \r\n", g_RouteMediaTransferFlag);
            }
        }
    }
    else if (strcasecmp(pname, CONF_MMS_ENABLE_FLAG) == 0) /* mms功能是否启用 */
    {
        iTmp = osip_atoi(pvalue);

        if (iTmp != g_MMSEnableFlag)
        {
            if (iTmp >= 0)
            {
                g_MMSEnableFlag = iTmp;
            }
            else
            {
                g_MMSEnableFlag = 1;
            }

            if (iRefresh)
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "conf_reset() g_MMSEnableFlag=%d \r\n", g_MMSEnableFlag);
            }
            else
            {
                printf(" conf_reset() g_MMSEnableFlag=%d \r\n", g_MMSEnableFlag);
            }

            if (iRefresh)
            {

            }
        }
    }
    else if (strcasecmp(pname, CONF_ANALYSIS_SUBGROUP_FLAG) == 0) /* 下级分组信息解析标识，默认不解析 */
    {
        iTmp = osip_atoi(pvalue);

        if (iTmp != g_AnalysisSubGroupFlag)
        {
            if (iTmp >= 0)
            {
                g_AnalysisSubGroupFlag = iTmp;
            }
            else
            {
                g_AnalysisSubGroupFlag = 0;
            }

            if (iRefresh)
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "conf_reset() g_AnalysisSubGroupFlag=%d \r\n", g_AnalysisSubGroupFlag);
            }
            else
            {
                printf(" conf_reset() g_AnalysisSubGroupFlag=%d \r\n", g_AnalysisSubGroupFlag);
            }
        }
    }
    else if (strcasecmp(pname, CONF_LOG_BUFFER_SIZE) == 0) /* 日志缓存队列大小 */
    {
        iTmp = osip_atoi(pvalue);

        if (iTmp != g_LogQueryBufferSize)
        {
            if (iTmp >= 0)
            {
                g_LogQueryBufferSize = iTmp;
            }
            else
            {
                g_LogQueryBufferSize = 0;
            }

            if (iRefresh)
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "conf_reset() g_LogQueryBufferSize=%d \r\n", g_LogQueryBufferSize);
            }
            else
            {
                printf(" conf_reset() g_LogQueryBufferSize=%d \r\n", g_LogQueryBufferSize);
            }
        }
    }
    else if (strcasecmp(pname, CONF_AUTH_FLAG) == 0) /* 认证标识 */
    {
        iTmp = osip_atoi(pvalue);

        if (iTmp != conf->iAuthFlag)
        {
            if (iTmp > 0)
            {
                conf->iAuthFlag = 1;
            }
            else
            {
                conf->iAuthFlag = 0;
            }

            if (iRefresh)
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "conf_reset() iAuthFlag=%d \r\n", conf->iAuthFlag);
            }
            else
            {
                printf(" conf_reset() iAuthFlag=%d \r\n", conf->iAuthFlag);
            }
        }
    }
    else if (strcasecmp(pname, CONF_PRESET_BACK_TIME) == 0) /* 预置位自动归位时长 */
    {
        iTmp = osip_atoi(pvalue);

        if (iTmp != conf->uPresetBackTime)
        {
            if (iTmp > 0)
            {
                conf->uPresetBackTime = iTmp;
            }
            else
            {
                conf->uPresetBackTime = DEFAULT_PRESET_BACK_TIME;
            }

            if (iRefresh)
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "conf_reset() uPresetBackTime=%d \r\n", conf->uPresetBackTime);
            }
            else
            {
                printf(" conf_reset() uPresetBackTime=%d \r\n", conf->uPresetBackTime);
            }
        }
    }
    else if (strcasecmp(pname, CONF_DEVICE_UNLOCK_TIME) == 0) /* 点位自动解锁时长 */
    {
        iTmp = osip_atoi(pvalue);

        if (iTmp != conf->uDeviceUnLockTime)
        {
            if (iTmp > 0)
            {
                conf->uDeviceUnLockTime = iTmp;
            }
            else
            {
                conf->uDeviceUnLockTime = DEFAULT_DEVICE_UNLOCK_TIME;
            }

            if (iRefresh)
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "conf_reset() uDeviceUnLockTime=%d \r\n", conf->uDeviceUnLockTime);
            }
            else
            {
                printf(" conf_reset() uDeviceUnLockTime=%d \r\n", conf->uDeviceUnLockTime);
            }
        }
    }
    else if (strcasecmp(pname, CONF_REGISTER_INTERVAL) == 0) /* 没有注册成功情况下重新注册间隔时间 */
    {
        iTmp = osip_atoi(pvalue);

        if (iTmp != conf->register_retry_interval)
        {
            if (iTmp > 0)
            {
                conf->register_retry_interval = iTmp;
            }
            else
            {
                conf->register_retry_interval = MIN_REGISTER_RETRY_INTERVAL;
            }

            if (iRefresh)
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "conf_reset() register_retry_interval=%d \r\n", conf->register_retry_interval);
            }
            else
            {
                printf(" conf_reset() register_retry_interval=%d \r\n", conf->register_retry_interval);
            }
        }
    }
    else if (strcasecmp(pname, CONF_REGISTER_EXPIRE) == 0) /* 注册超时时间 */
    {
        iTmp = osip_atoi(pvalue);

        if (iTmp != conf->registry_cleanuprate)
        {
            if (iTmp > 0)
            {
                conf->registry_cleanuprate = iTmp;
            }
            else
            {
                conf->registry_cleanuprate = MIN_REGISTER_EXPIRE;
            }

            if (iRefresh)
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "conf_reset() registry_cleanuprate=%d \r\n", conf->registry_cleanuprate);
            }
            else
            {
                printf(" conf_reset() registry_cleanuprate=%d \r\n", conf->registry_cleanuprate);
            }
        }
    }
    else if (strcasecmp(pname, CONF_SESSION_EXPIRE) == 0) /* 会话超时时间 */
    {
        iTmp = osip_atoi(pvalue);

        if (iTmp != conf->session_expires)
        {
            if (iTmp > 0)
            {
                conf->session_expires = iTmp;
            }
            else
            {
                conf->session_expires = MIN_SESSION_EXPIRE;
            }

            if (iRefresh)
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "conf_reset() session_expires=%d \r\n", conf->session_expires);
            }
            else
            {
                printf(" conf_reset() session_expires=%d \r\n", conf->session_expires);
            }
        }
    }
    else if (strcasecmp(pname, CONF_SUBSCRIBE_EXPIRE) == 0) /* 订阅超时时间 */
    {
        iTmp = osip_atoi(pvalue);

        if (iTmp != conf->subscribe_expires)
        {
            if (iTmp > 0)
            {
                conf->subscribe_expires = iTmp;
            }
            else
            {
                conf->subscribe_expires = MIN_SUBSCRIBE_EXPIRE;
            }

            if (iRefresh)
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "conf_reset() subscribe_expires=%d \r\n", conf->subscribe_expires);
            }
            else
            {
                printf(" conf_reset() subscribe_expires=%d \r\n", conf->subscribe_expires);
            }
        }
    }
    else if (strcasecmp(pname, CONF_KEEPALIVE_EXPIRE) == 0) /* 保活超时时间 */
    {
        iTmp = osip_atoi(pvalue);

        if (iTmp != conf->keep_alive_expires)
        {
            if (iTmp > 0)
            {
                conf->keep_alive_expires = iTmp;
            }
            else
            {
                conf->keep_alive_expires = DEFAULT_DEVICE_KEEP_ALIVE_EXPIRES;
            }

            if (iRefresh)
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "conf_reset() keep_alive_expires=%d \r\n", conf->keep_alive_expires);
            }
            else
            {
                printf(" conf_reset() keep_alive_expires=%d \r\n", conf->keep_alive_expires);
            }
        }
    }
    else if (strcasecmp(pname, CONF_KEEP_ALIVE_INTERVAL) == 0) /* 保活间隔时间 */
    {
        iTmp = osip_atoi(pvalue);

        if (iTmp != conf->keep_alive_interval)
        {
            if (iTmp > 0)
            {
                conf->keep_alive_interval = iTmp;
            }
            else
            {
                conf->keep_alive_interval = MIN_KEEP_ALIVE_INTERVAL;
            }

            if (iRefresh)
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "conf_reset() keep_alive_expires=%d \r\n", conf->keep_alive_interval);
            }
            else
            {
                printf(" conf_reset() keep_alive_expires=%d \r\n", conf->keep_alive_interval);
            }
        }
    }
    else if (strcasecmp(pname, CONF_FAILED_KEEP_ALIVE_COUNT) == 0) /* 保活失败次数 */
    {
        iTmp = osip_atoi(pvalue);

        if (iTmp != conf->failed_keep_alive_count)
        {
            if (iTmp > 0)
            {
                conf->failed_keep_alive_count = iTmp;
            }
            else
            {
                conf->failed_keep_alive_count = DEFAULT_FAILED_KEEP_ALIVE_COUNT;
            }

            if (iRefresh)
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "conf_reset() failed_keep_alive_count=%d \r\n", conf->failed_keep_alive_count);
            }
            else
            {
                printf(" conf_reset() failed_keep_alive_count=%d \r\n", conf->failed_keep_alive_count);
            }
        }
    }
    else if (strcasecmp(pname, CONF_ALARM_DURATION) == 0) /* 告警延时时间 */
    {
        iTmp = osip_atoi(pvalue);

        if (iTmp != conf->alarm_duration)
        {
            if (iTmp > 0)
            {
                conf->alarm_duration = iTmp;
            }
            else
            {
                conf->alarm_duration = DEFAULT_ALARM_DURATION;
            }

            if (iRefresh)
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "conf_reset() alarm_duration=%d \r\n", conf->alarm_duration);
            }
            else
            {
                printf(" conf_reset() alarm_duration=%d \r\n", conf->alarm_duration);
            }
        }
    }
    else if (strcasecmp(pname, CONF_CMS_NAME) == 0) /* CMS别名 */
    {
        if (0 != sstrcmp(conf->cms_name, pvalue))
        {
            memset(conf->cms_name, 0, MAX_128CHAR_STRING_LEN + 4);
            osip_strncpy(conf->cms_name, pvalue, MAX_128CHAR_STRING_LEN);

            if (iRefresh)
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "conf_reset() cms_name=%s \r\n", conf->cms_name);
            }
            else
            {
                printf(" conf_reset() cms_name=%s \r\n", conf->cms_name);
            }

            return 1;
        }
    }
    else if (strcasecmp(pname, CONF_SHDB_AGENTID) == 0) /* 上海地标监管平台注册ID */
    {
        if (0 != sstrcmp(conf->shdb_agent_id, pvalue))
        {
            memset(conf->shdb_agent_id, 0, MAX_16CHAR_STRING_LEN + 4);
            osip_strncpy(conf->shdb_agent_id, pvalue, MAX_16CHAR_STRING_LEN);

            if (iRefresh)
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "conf_reset() shdb_agent_id=%s \r\n", conf->shdb_agent_id);
            }
            else
            {
                printf(" conf_reset() shdb_agent_id=%s \r\n", conf->shdb_agent_id);
            }
        }
    }
    else if (strcasecmp(pname, CONF_SHDB_SEVVERIP) == 0) /* 上海地标监管平台服务器IP */
    {
        if (0 != sstrcmp(conf->shdb_server_ip, pvalue))
        {
            memset(conf->shdb_server_ip, 0, MAX_IP_LEN);
            osip_strncpy(conf->shdb_server_ip, pvalue, MAX_IP_LEN);

            if (iRefresh)
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "conf_reset() shdb_server_ip=%s \r\n", conf->shdb_server_ip);
            }
            else
            {
                printf(" conf_reset() shdb_server_ip=%s \r\n", conf->shdb_server_ip);
            }
        }
    }
    else if (strcasecmp(pname, CONF_SHDB_PREX_SEC) == 0) /* 上海地标报警图片上传前N秒 */
    {
        iTmp = osip_atoi(pvalue);

        if (iTmp != conf->shdb_prex_seconds)
        {
            if (iTmp > 0)
            {
                conf->shdb_prex_seconds = iTmp;
            }
            else
            {
                conf->shdb_prex_seconds = DEFAULT_SHDB_DURATION_SECONDS;
            }

            if (iRefresh)
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "conf_reset() shdb_prex_seconds=%d \r\n", conf->shdb_prex_seconds);
            }
            else
            {
                printf(" conf_reset() shdb_prex_seconds=%d \r\n", conf->shdb_prex_seconds);
            }
        }
    }
    else if (strcasecmp(pname, CONF_SHDB_NEXT_SEC) == 0) /* 上海地标报警图片上传后M秒 */
    {
        iTmp = osip_atoi(pvalue);

        if (iTmp != conf->shdb_next_seconds)
        {
            if (iTmp > 0)
            {
                conf->shdb_next_seconds = iTmp;
            }
            else
            {
                conf->shdb_next_seconds = DEFAULT_SHDB_DURATION_SECONDS;
            }

            if (iRefresh)
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "conf_reset() shdb_next_seconds=%d \r\n", conf->shdb_next_seconds);
            }
            else
            {
                printf(" conf_reset() shdb_next_seconds=%d \r\n", conf->shdb_next_seconds);
            }
        }
    }
    else if (strcasecmp(pname, CONF_SHDB_INTERVAL_SEC) == 0) /* 上海地标报警图片上传间隔P秒 */
    {
        iTmp = osip_atoi(pvalue);

        if (iTmp != conf->shdb_interval_seconds)
        {
            if (iTmp > 0)
            {
                conf->shdb_interval_seconds = iTmp;
            }
            else
            {
                conf->shdb_interval_seconds = DEFAULT_SHDB_INTERVAL_SECONDS;
            }

            if (iRefresh)
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "conf_reset() shdb_interval_seconds=%d \r\n", conf->shdb_interval_seconds);
            }
            else
            {
                printf(" conf_reset() shdb_interval_seconds=%d \r\n", conf->shdb_interval_seconds);
            }
        }
    }
    else if (strcasecmp(pname, CONF_SYSTEM_EXIT_FLAG) == 0) /* 系统上次退出标识 */
    {
        iTmp = osip_atoi(pvalue);

        if (iTmp != conf->sys_exit_flag)
        {
            if (iTmp >= 0)
            {
                conf->sys_exit_flag = iTmp;
            }
            else
            {
                conf->sys_exit_flag = 1;
            }

            if (iRefresh)
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "conf_reset(): sys_exit_flag=%d \r\n", conf->sys_exit_flag);
            }
            else
            {
                printf(" conf_reset() sys_exit_flag=%d \r\n", conf->sys_exit_flag);
            }
        }
    }
    else if (strcasecmp(pname, CONF_SHOW_CODE) == 0) /* 是否为国标标准 */
    {
        if (0 == sstrcmp("hide", pvalue))
        {
            iTmp = 0; /*非国标*/
        }
        else
        {
            iTmp = 1;/*国标*/
        }

        if (iTmp != conf->showcode)
        {
            conf->showcode = iTmp;

            if (iRefresh)
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "conf_reset(): showcode=%d \r\n", conf->showcode);
            }
            else
            {
                printf(" conf_reset() showcode=%d \r\n", conf->showcode);
            }
        }
    }
    else
    {
        return -2;
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : load_file_config
 功能描述  : 加载配置文件选项
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年7月22日 星期一
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int load_file_config()
{
    int iRet = 0;
    FILE* rcfp = NULL;
    struct stat statbuf;
    char str[256] = {0};
    char cmdStr[256] = {0};
    char* pname = NULL, *pvalue = NULL, *tmp = NULL;

    /* 看是否要创建默认的web配置文件 */
    rcfp = fopen(CMS_WEB_CONFIG_FILE, "r");

    if (NULL == rcfp)
    {
        rcfp = fopen(CMS_WEB_CONFIG_FILE, "w+");

        if (NULL != rcfp)
        {
            fputs((char*)"[role]\n", rcfp);
            fputs((char*)"cms=1\n", rcfp);
            fputs((char*)"[web]\n", rcfp);
            fputs((char*)"LogoName = iEV9000\n", rcfp);
            fclose(rcfp);
            rcfp = NULL;

            snprintf(cmdStr, 200, "chmod 777 %s", CMS_WEB_CONFIG_FILE);
            system(cmdStr);
        }
        else
        {
            printf("I CAN NOT create default web config file,please check if you have privilege!!!\n");
        }
    }
    else
    {
        fclose(rcfp);
        rcfp = NULL;
        change_conf_to_web_config_file((char*)"LogoName", (char*)"iEV9000");
    }

    /* 看是否要创建默认的cms配置文件 */
    rcfp = fopen(CMS_CONFIG_FILE, "r");

    if (NULL == rcfp)
    {
        iRet = write_default_config_file();

        if (0 != iRet)
        {
            printf(" load_file_config() exit---: Wirte Default Config File Error \r\n");
            return -1;
        }

        rcfp = fopen(CMS_CONFIG_FILE, "r");

        if (NULL == rcfp)
        {
            printf(" load_file_config() exit---: Open Config File Error \r\n");
            return -1;
        }

        //printf("load_file_config() exit---: Open Config File Error \r\n");
        //return -1;
    }

    while (NULL != fgets(str, sizeof(str), rcfp))
    {
        if ((strlen(str) == 1) || (0 == strncmp(str, "#", 1)))
        {
            continue;
        }

        tmp = strchr(str, 61); /*find '=' */

        if (tmp != NULL)
        {
            if (tmp - str <= 0 || str + strlen(str) - tmp <= 0)
            {
                printf(" load_file_config malloc len error \r\n");
                continue;
            }

            pname = (char*) osip_malloc(tmp - str + 1);
            pvalue = (char*) osip_malloc(str + strlen(str) - tmp);
            osip_strncpy(pname, str, tmp - str);
            osip_strncpy(pvalue, tmp + 1, str + strlen(str) - tmp - 2);

            sclrspace(pname);
            sclrspace(pvalue);

            iRet = conf_reset(pGblconf, pname, pvalue, 0);
            printf(" load_file_config() pname=%s,pvalue=%s,i=%d \r\n", pname, pvalue, iRet);

            osip_free(pname);
            osip_free(pvalue);
        }
    }

    fclose(rcfp);
    rcfp = NULL;

    /* get file stat */
    if (!stat(CMS_CONFIG_FILE, &statbuf))
    {
        Cfg_lmt = statbuf.st_mtime;
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : reload_file_config
 功能描述  : 重新加载配置文件选项
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年7月22日 星期一
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int reload_file_config()
{
    int iRet = 0;
    int iNameFlag = 0;
    FILE* rcfp = NULL;
    struct stat statbuf;
    char str[256] = {0};
    char* pname = NULL, *pvalue = NULL, *tmp = NULL;

    rcfp = fopen(CMS_CONFIG_FILE, "r");

    if (NULL == rcfp)
    {
        iRet = write_default_config_file();

        if (0 != iRet)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "reload_file_config() exit---: Wirte Default Config File Error \r\n");
            return -1;
        }

        rcfp = fopen(CMS_CONFIG_FILE, "r");

        if (NULL == rcfp)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "reload_file_config() exit---: Open Config File Error \r\n");
            return -1;
        }

        //DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "reload_file_config() exit---: Open Config File Error \r\n");
        //return -1;
    }

    while (NULL != fgets(str, sizeof(str), rcfp))
    {
        if ((strlen(str) == 1) || (0 == strncmp(str, "#", 1)))
        {
            continue;
        }

        tmp = strchr(str, 61); /*find '=' */

        if (tmp != NULL)
        {
            if (tmp - str <= 0 || str + strlen(str) - tmp <= 0)
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "\n reload_file_config malloc len error \r\n");
                continue;
            }

            pname = (char*) osip_malloc(tmp - str + 1);
            pvalue = (char*) osip_malloc(str + strlen(str) - tmp);
            osip_strncpy(pname, str, tmp - str);
            osip_strncpy(pvalue, tmp + 1, str + strlen(str) - tmp - 2);

            sclrspace(pname);
            sclrspace(pvalue);

            iRet = conf_reset(pGblconf, pname, pvalue, 1);

            if (iRet >= 0)
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "reload_file_config() pname=%s,pvalue=%s\r\n", pname, pvalue);

                if (iRet > 0)
                {
                    iNameFlag = 1;
                }
            }
            else if (iRet == -1)
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "reload_file_config() pname=%s,pvalue=%s\r\n", pname, pvalue);
            }

            osip_free(pname);
            osip_free(pvalue);
        }
    }

    fclose(rcfp);

    /* get file stat */
    if (!stat(CMS_CONFIG_FILE, &statbuf))
    {
        Cfg_lmt = statbuf.st_mtime;
    }

    return iNameFlag;
}

/*****************************************************************************
 函 数 名  : refresh_config_file
 功能描述  : 刷新配置文件
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年7月22日 星期一
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void refresh_config_file()
{
    //int iRet = 0;
    struct stat statbuf;

    if (stat(CMS_CONFIG_FILE, &statbuf) != -1)
    {
        if (statbuf.st_mtime > Cfg_lmt)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "refresh_config_file() Refresh Config data \r\n");
            gbl_conf_reload();
        }
    }

#if 0

    if (stat(CMS_KEY_FILENAME, &statbuf) != -1)
    {
        if (statbuf.st_mtime > Key_lmt)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "refresh_config_file() Key Config data \r\n");

            iRet = checklicence();

            if (iRet == -1)
            {
                printf("Please Register the iEV9000 RX !!!\n");
                //do_restart();
                //return;
                system((char*)"killall cms");
            }
            else if (iRet == 1)
            {
                printf("iEV9000 RX License File out of date, Please Register it!!!\n");
                //do_restart();
                //return;
                system((char*)"killall cms");
            }
        }
    }

    if (stat(CMS_KEY_TMP_FILENAME, &statbuf) != -1)
    {
        if (statbuf.st_mtime > Key_Tmp_lmt)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "refresh_config_file() Key Tmp Config data \r\n");

            iRet = checklicence();

            if (iRet == -1)
            {
                printf("Please Register the iEV9000 RX !!!\n");
                //do_restart();
                //return;
                system((char*)"killall cms");
            }
            else if (iRet == 1)
            {
                printf("iEV9000 RX License File out of date, Please Register it!!!\n");
                //do_restart();
                //return;
                system((char*)"killall cms");
            }
        }
    }

#endif
    return;
}

/*****************************************************************************
 函 数 名  : write_default_config_file
 功能描述  : 生成默认的配置文件
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年8月30日 星期五
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int write_default_config_file()
{
    int i = 0;
    FILE* fp = NULL;
    DIR* dir = NULL;
    char str[201] = {0};
    char cmsStr[201] = {0};

    /* 检查目录是否存在 */
    dir = opendir("/config");

    if (NULL == dir)
    {
        i = mkdir("/config", 0775);

        if (i != 0)
        {
            printf(" write_default_config_file() mkdir /config Error \r\n");
            return -1;
        }
    }

    closedir(dir);
    dir = NULL;

    fp = fopen(CMS_CONFIG_FILE, "w+");

    if (NULL == fp)
    {
        printf("write_default_config_file() I CAN NOT create default config file,please check if you have privilege!!!\n");
        return -1;
    }

    fputs((char*)"[cms]\n", fp);

    /* CMSID */
    snprintf(str, 200, "%s = \n", CONF_BOARD_ID);
    fputs(str, fp);

    /* CMS 别名 */
    snprintf(str, 200, "%s = \n", CONF_CMS_NAME);
    fputs(str, fp);

    /* 数据库IP地址 */
    snprintf(str, 200, "%s = %s\n", CONF_DB_SERVER_IP, (char*)"127.0.0.1");
    fputs(str, fp);

    /* 从数据库IP地址 */
    snprintf(str, 200, "%s = \n", CONF_SDB_SERVER_IP);
    fputs(str, fp);

    /* 告警服务器IP地址 */
    snprintf(str, 200, "%s = \n", CONF_ALARM_SERVER_IP);
    fputs(str, fp);

    /* NTP服务器IP地址 */
    snprintf(str, 200, "%s = \n", CONF_NTP_SERVER_IP);
    fputs(str, fp);

    /* 认证标识 */
    snprintf(str, 200, "%s = %u\n", CONF_AUTH_FLAG, pGblconf->iAuthFlag);
    fputs(str, fp);

    /* 预置位自动归位时长 */
    snprintf(str, 200, "%s = %u\n", CONF_PRESET_BACK_TIME, pGblconf->uPresetBackTime);
    fputs(str, fp);

    /* 点位自动解锁时长 */
    snprintf(str, 200, "%s = %u\n", CONF_DEVICE_UNLOCK_TIME, pGblconf->uDeviceUnLockTime);
    fputs(str, fp);

    /* 没有注册成功情况下重新注册间隔时间 */
    snprintf(str, 200, "%s = %u\n", CONF_REGISTER_INTERVAL, pGblconf->register_retry_interval);
    fputs(str, fp);

    /* 注册超时时间 */
    snprintf(str, 200, "%s = %u\n", CONF_REGISTER_EXPIRE, pGblconf->registry_cleanuprate);
    fputs(str, fp);

    /* 会话超时时间 */
    snprintf(str, 200, "%s = %u\n", CONF_SESSION_EXPIRE, pGblconf->session_expires);
    fputs(str, fp);

    /* 订阅超时时间 */
    snprintf(str, 200, "%s = %u\n", CONF_SUBSCRIBE_EXPIRE, pGblconf->subscribe_expires);
    fputs(str, fp);

    /* 保活间隔时间 */
    snprintf(str, 200, "%s = %u\n", CONF_KEEP_ALIVE_INTERVAL, pGblconf->keep_alive_interval);
    fputs(str, fp);

    /* 保活失败次数 */
    snprintf(str, 200, "%s = %u\n", CONF_FAILED_KEEP_ALIVE_COUNT, pGblconf->failed_keep_alive_count);
    fputs(str, fp);

    /* 保活超时时间 */
    snprintf(str, 200, "%s = %u\n", CONF_KEEPALIVE_EXPIRE, pGblconf->keep_alive_expires);
    fputs(str, fp);

    /* 告警延时时间 */
    snprintf(str, 200, "%s = %u\n", CONF_ALARM_DURATION, pGblconf->alarm_duration);
    fputs(str, fp);

    /* 调试日志打印等级 */
    snprintf(str, 200, "%s = %d\n", CONF_DBG_LOG_LEVEL, g_CommonDbgLevel);
    fputs(str, fp);

    /* 运行日志打印等级 */
    snprintf(str, 200, "%s = %d\n", CONF_RUN_LOG_LEVEL, g_SystemLogLevel);
    fputs(str, fp);

    /* 运行日志记录到数据库等级 */
    snprintf(str, 200, "%s = %d\n", CONF_RUN_LOG_2DB_LEVEL, g_SystemLog2DBLevel);
    fputs(str, fp);

    /* 日志记录到文件开关 */
    snprintf(str, 200, "%s = %d\n", CONF_LOG2FILE_FLAG, g_IsLog2File);
    fputs(str, fp);

    /* 日志记录到数据库开关 */
    snprintf(str, 200, "%s = %d\n", CONF_LOG2DB_FLAG, g_IsLog2DB);
    fputs(str, fp);

    /* 是否记录SIP Message消息 */
    snprintf(str, 200, "%s = %d\n", CONF_LOGSIPMSG_FLAG, g_IsLogSIPMsg);
    fputs(str, fp);

    /* 是否发送订阅 */
    snprintf(str, 200, "%s = %d\n", CONF_IS_SUBSCRIBE, g_IsSubscribe);
    fputs(str, fp);

    /* 报警消息发送给用户标识 */
    snprintf(str, 200, "%s = %d\n", CONF_ALARM_SEND_TO_USER_FLAG, g_AlarmMsgSendToUserFlag);
    fputs(str, fp);

    /* 报警消息发送给上级路由标识 */
    snprintf(str, 200, "%s = %d\n", CONF_ALARM_SEND_TO_ROUTE_FLAG, g_AlarmMsgSendToRouteFlag);
    fputs(str, fp);

    /* 下级媒体是否经过本地媒体服务器转发 */
    snprintf(str, 200, "%s = %d\n", CONF_MEDIA_TRANSFER_FLAG, g_LocalMediaTransferFlag);
    fputs(str, fp);

    /* 下级媒体切电视墙是否经过本地媒体服务器转发 */
    snprintf(str, 200, "%s = %d\n", CONF_DEC_MEDIA_TRANSFER_FLAG, g_DECMediaTransferFlag);
    fputs(str, fp);

    /* 上级第三方平台是否支持媒体转发 */
    snprintf(str, 200, "%s = %d\n", CONF_ROUTE_MEDIA_TRANS_FLAG, g_RouteMediaTransferFlag);
    fputs(str, fp);

    /* mms是否启用 */
    snprintf(str, 200, "%s = %d\n", CONF_MMS_ENABLE_FLAG, g_MMSEnableFlag);
    fputs(str, fp);

    /* 下级分组信息解析标识 */
    snprintf(str, 200, "%s = %d\n", CONF_ANALYSIS_SUBGROUP_FLAG, g_AnalysisSubGroupFlag);
    fputs(str, fp);

    /* 日志缓存 */
    snprintf(str, 200, "%s = %d\n", CONF_LOG_BUFFER_SIZE, g_LogQueryBufferSize);
    fputs(str, fp);

    /* 是否付费 */
    snprintf(str, 200, "%s = %d\n", CONF_IS_PAY, g_IsPay);
    fputs(str, fp);

    /* 系统语言 */
    snprintf(str, 200, "%s = %d\n", CONF_LANGUAGE, g_Language);
    fputs(str, fp);

    fclose(fp);
    fp = NULL;

    snprintf(cmsStr, 200, "chmod 777 %s", CMS_CONFIG_FILE);
    system(cmsStr);

    return 0;
}

/*****************************************************************************
 函 数 名  : write_default_web_config_file
 功能描述  : 写默认的web配置
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年12月28日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int write_default_web_config_file()
{
    FILE* rcfp = NULL;
    char cmdStr[201] = {0};

    /* 看是否要创建默认的web配置文件 */
    rcfp = fopen(CMS_WEB_CONFIG_FILE, "r");

    if (NULL == rcfp)
    {
        rcfp = fopen(CMS_WEB_CONFIG_FILE, "w+");

        if (NULL != rcfp)
        {
            fputs((char*)"[role]\n", rcfp);
            fputs((char*)"cms=1\n", rcfp);
            fputs((char*)"[web]\n", rcfp);
            fputs((char*)"LogoName = iEV9000\n", rcfp);
            fclose(rcfp);
            rcfp = NULL;

            snprintf(cmdStr, 200, "chmod 777 %s", CMS_WEB_CONFIG_FILE);
            system(cmdStr);
        }
        else
        {
            printf("write_default_web_config_file() I CAN NOT create default web config file,please check if you have privilege!!!\n");
        }
    }
    else
    {
        fclose(rcfp);
        rcfp = NULL;
        change_conf_to_web_config_file((char*)"LogoName", (char*)"iEV9000");
    }

    /* 第二遍检查，看是否要创建默认的web配置文件 */
    rcfp = fopen(CMS_WEB_CONFIG_FILE, "r");

    if (NULL == rcfp)
    {
        rcfp = fopen(CMS_WEB_CONFIG_FILE, "w+");

        if (NULL != rcfp)
        {
            fputs((char*)"[role]\n", rcfp);
            fputs((char*)"cms=1\n", rcfp);
            fputs((char*)"[web]\n", rcfp);
            fputs((char*)"LogoName = iEV9000\n", rcfp);
            fclose(rcfp);
            rcfp = NULL;

            snprintf(cmdStr, 200, "chmod 777 %s", CMS_WEB_CONFIG_FILE);
            system(cmdStr);
        }
        else
        {
            printf("write_default_web_config_file() I CAN NOT create default web config file,please check if you have privilege!!!\n");
        }
    }
    else
    {
        fclose(rcfp);
        rcfp = NULL;

        return 0;
    }

    /* 第三遍检查，看是否要创建默认的web配置文件 */
    rcfp = fopen(CMS_WEB_CONFIG_FILE, "r");

    if (NULL == rcfp)
    {
        rcfp = fopen(CMS_WEB_CONFIG_FILE, "w+");

        if (NULL != rcfp)
        {
            fputs((char*)"[role]\n", rcfp);
            fputs((char*)"cms=1\n", rcfp);
            fputs((char*)"[web]\n", rcfp);
            fputs((char*)"LogoName = iEV9000\n", rcfp);
            fclose(rcfp);
            rcfp = NULL;

            snprintf(cmdStr, 200, "chmod 777 %s", CMS_WEB_CONFIG_FILE);
            system(cmdStr);
        }
        else
        {
            printf("write_default_web_config_file() I CAN NOT create default web config file,please check if you have privilege!!!\n");
        }
    }
    else
    {
        fclose(rcfp);
        rcfp = NULL;

        return 0;
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : change_conf_to_config_file
 功能描述  : 将修改设置到文件
 输入参数  : char* pcName
             char* pcValue
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年11月19日 星期三
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int change_conf_to_config_file(char* pcName, char* pcValue)
{
    int iRet = 0;
    FILE* conf_fp = NULL;
    FILE* conf_tmp_fp = NULL;

    char str[256] = {0};
    char strConfig[256] = {0};
    char strCommon[256] = {0};

    char* pname = NULL, *tmp = NULL;

    int iFlag = 0;

    if (NULL == pcName || NULL == pcValue)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "change_conf_to_config_file() exit---: Param Error \r\n");
        return -1;
    }

    conf_fp = fopen(CMS_CONFIG_FILE, "r");

    if (NULL == conf_fp)
    {
        iRet = write_default_config_file();

        if (0 != iRet)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "change_conf_to_config_file() exit---: Wirte Default Config File Error \r\n");
            return -1;
        }

        conf_fp = fopen(CMS_CONFIG_FILE, "r");

        if (NULL == conf_fp)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "change_conf_to_config_file() exit---: Open Config File Error \r\n");
            return -1;
        }

        //DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "change_conf_to_config_file() exit---: Open Config File Error \r\n");
        //return -1;
    }

    conf_tmp_fp = fopen(CMS_CONFIG_TMP_FILE, "w+");

    if (NULL == conf_tmp_fp)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "change_conf_to_config_file() exit---: Open Tmp Config File Error \r\n");
        fclose(conf_fp);
        conf_fp = NULL;
        return -1;
    }

    while (NULL != fgets(str, sizeof(str), conf_fp))
    {
        if (0 == strncmp(str, " ", 1))
        {
            continue;
        }

        if ((strlen(str) == 1) || (0 == strncmp(str, "#", 1)) || (0 == strncmp(str, "[cms]", 5)))
        {
            fputs(str, conf_tmp_fp);
            continue;
        }

        tmp = strchr(str, 61); /*find '=' */

        if (tmp != NULL)
        {
            if (tmp - str <= 0 || str + strlen(str) - tmp <= 0)
            {
                fputs(str, conf_tmp_fp);
                continue;
            }

            pname = (char*) osip_malloc(tmp - str + 1);
            osip_strncpy(pname, str, tmp - str);

            sclrspace(pname);

            if (0 != sstrcmp(pname, pcName)) /* 不是需要更新的字段 */
            {
                //printf("change_conf_to_config_file: str=%s\r\n", str);
                fputs(str, conf_tmp_fp);
                osip_free(pname);
                continue;
            }

            iFlag = 1; /* 有需要更新的字段 */

            if (pcValue[0] == '\0')
            {
                snprintf(strConfig, 255, "%s =\n", pcName);
            }
            else
            {
                snprintf(strConfig, 255, "%s = %s\n", pcName, pcValue);
            }

            //printf("change_conf_to_config_file: strConfig=%s\r\n", strConfig);
            fputs(strConfig, conf_tmp_fp);

            osip_free(pname);
        }
    }

    if (!iFlag)
    {
        if (pcValue[0] == '\0')
        {
            snprintf(strConfig, 255, "%s =\n", pcName);
        }
        else
        {
            snprintf(strConfig, 255, "%s = %s\n", pcName, pcValue);
        }

        //printf("change_conf_to_config_file: strConfig=%s\r\n", strConfig);
        fputs(strConfig, conf_tmp_fp);
    }

    fclose(conf_fp);
    conf_fp = NULL;
    fclose(conf_tmp_fp);
    conf_tmp_fp = NULL;

    /* 删除原来老的文件 */
    memset(strCommon, 0, 256);
    snprintf(strCommon, 255, "rm -rf %s\n", CMS_CONFIG_FILE);
    system(strCommon);

    /* 将tmp文件改为新的配置文件 */
    memset(strCommon, 0, 256);
    snprintf(strCommon, 255, "mv -f %s %s\n", CMS_CONFIG_TMP_FILE, CMS_CONFIG_FILE);
    system(strCommon);

    /* 将配置文件权限改为777 */
    memset(strCommon, 0, 256);
    snprintf(strCommon, 255, "chmod 777 %s", CMS_CONFIG_FILE);
    system(strCommon);

    return 0;
}

/*****************************************************************************
 函 数 名  : change_conf_to_web_config_file
 功能描述  : 修改Web配置文件
 输入参数  : char* pcName
             char* pcValue
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年5月23日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int change_conf_to_web_config_file(char* pcName, char* pcValue)
{
    FILE* conf_fp = NULL;
    FILE* conf_tmp_fp = NULL;

    char str[256] = {0};
    char strConfig[256] = {0};
    char strCommon[256] = {0};

    char* pname = NULL, *tmp = NULL;

    int iFlag = 0;

    if (NULL == pcName || NULL == pcValue)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "change_conf_to_web_config_file() exit---: Param Error \r\n");
        return -1;
    }

    conf_fp = fopen(CMS_WEB_CONFIG_FILE, "r");

    if (NULL == conf_fp)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "change_conf_to_web_config_file() exit---: Open Config File Error \r\n");
        return -1;
    }

    conf_tmp_fp = fopen(CMS_WEB_CONFIG_TMP_FILE, "w+");

    if (NULL == conf_tmp_fp)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "change_conf_to_web_config_file() exit---: Open Tmp Config File Error \r\n");
        fclose(conf_fp);
        conf_fp = NULL;
        return -1;
    }

    while (NULL != fgets(str, sizeof(str), conf_fp))
    {
        if (0 == strncmp(str, " ", 1))
        {
            continue;
        }

        if ((strlen(str) == 1) || (0 == strncmp(str, "#", 1)))
        {
            fputs(str, conf_tmp_fp);
            continue;
        }

        if ((0 == strncmp(pcName, (char*)"LogoName", 8)))
        {
            /* 如果找到[cms]，还没有找到LogoName */
            if ((0 == strncmp(str, "[cms]", 5)))
            {
                if (!iFlag)
                {
                    if (pcValue[0] == '\0')
                    {
                        snprintf(strConfig, 255, "%s =\n", pcName);
                    }
                    else
                    {
                        snprintf(strConfig, 255, "%s = %s\n", pcName, pcValue);
                    }

                    //printf("change_conf_to_config_file: strConfig=%s\r\n", strConfig);
                    fputs(strConfig, conf_tmp_fp);
                }

                fputs(str, conf_tmp_fp);
                continue;
            }
        }

        tmp = strchr(str, 61); /*find '=' */

        if (tmp != NULL)
        {
            if (tmp - str <= 0 || str + strlen(str) - tmp <= 0)
            {
                fputs(str, conf_tmp_fp);
                continue;
            }

            pname = (char*) osip_malloc(tmp - str + 1);
            osip_strncpy(pname, str, tmp - str);

            sclrspace(pname);

            if (0 != sstrcmp(pname, pcName)) /* 不是需要更新的字段 */
            {
                //printf("change_conf_to_config_file: str=%s\r\n", str);
                fputs(str, conf_tmp_fp);
                osip_free(pname);
                continue;
            }

            iFlag = 1; /* 有需要更新的字段 */

            if (pcValue[0] == '\0')
            {
                snprintf(strConfig, 255, "%s =\n", pcName);
            }
            else
            {
                snprintf(strConfig, 255, "%s = %s\n", pcName, pcValue);
            }

            //printf("change_conf_to_config_file: strConfig=%s\r\n", strConfig);
            fputs(strConfig, conf_tmp_fp);
            osip_free(pname);
        }
        else
        {
            fputs(str, conf_tmp_fp);
            continue;
        }
    }

    if ((0 != strncmp(pcName, (char*)"LogoName", 8)))
    {
        if (!iFlag)
        {
            if (pcValue[0] == '\0')
            {
                snprintf(strConfig, 255, "%s =\n", pcName);
            }
            else
            {
                snprintf(strConfig, 255, "%s = %s\n", pcName, pcValue);
            }

            //printf("change_conf_to_config_file: strConfig=%s\r\n", strConfig);
            fputs(strConfig, conf_tmp_fp);
        }
    }

    fclose(conf_fp);
    conf_fp = NULL;
    fclose(conf_tmp_fp);
    conf_tmp_fp = NULL;

    /* 删除原来老的文件 */
    memset(strCommon, 0, 256);
    snprintf(strCommon, 255, "rm -rf %s\n", CMS_WEB_CONFIG_FILE);
    system(strCommon);

    /* 将tmp文件改为新的配置文件 */
    memset(strCommon, 0, 256);
    snprintf(strCommon, 255, "mv -f %s %s\n", CMS_WEB_CONFIG_TMP_FILE, CMS_WEB_CONFIG_FILE);
    system(strCommon);

    /* 将配置文件权限改为777 */
    memset(strCommon, 0, 256);
    snprintf(strCommon, 255, "chmod 777 %s", CMS_WEB_CONFIG_FILE);
    system(strCommon);

    return 0;
}

/*****************************************************************************
 函 数 名  : is_need_auth
 功能描述  : 是否需要认证
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年7月21日 星期一
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int is_need_auth()
{
    return pGblconf->iAuthFlag;
}

/*****************************************************************************
 函 数 名  : GetRegPasswd
 功能描述  : 将用户名换算成15位注册码
 输入参数  : char *name
 输出参数  : 无
 返 回 值  : char
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年8月17日 星期一
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
char *GetRegPasswd(char *name)
{
    long Num1 = 0, Num2 = 0, Num3 = 0, Num4 = 0;
    static char sn[32] = {0};
    int i = 0, len = 0;

    if (name == NULL)
    {
        return NULL;
    }

    Num1 = 0;
    Num2 = 0;
    Num3 = 0;
    Num4 = 0;

    len = (int)strlen(name);

    if (len != 0)
    {
        for (i = 1; i <= len; i++)
        {
            //第一步算法
            Num1 = ((long)(Num1 + ((int)(name[i - 1]) * i * i) * (i * (int)sqrt((double)name[i - 1]) + 1))) % 100000;
            //第二步算法
            Num2 = (Num2 * i + ((long)(pow((double)name[i - 1], 2) * i))) % 100000;
            //第三步算法
            Num3 = (Num2 + (long)sqrt((double)Num1)) % 100000;
            //第四步算法
            Num4 = (Num3 + (long)sqrt((double)Num2)) % 100000;
        }

        //以下把四个算法结果分别生成5个字符，共有20个
        for (i = 0; i < 5; i++)
        { sn[i] = (int)(Num1 + 31 + i * i * i) % 128; }

        for (i = 5; i < 10; i++)
        { sn[i] = (int)(Num2 + 31 + i * i * i) % 128; }

        for (i = 10; i < 15; i++)
        { sn[i] = (int)(Num3 + 31 + i * i * i) % 128; }

        for (i = 15; i < 20; i++)
        { sn[i] = (int)(Num4 + 31 + i * i * i) % 128; }

        sn[20] = 0;
        //以下循环把所有生成的字符转换为0---9，A---Z，a----z

        for (i = 0; i < 20; i++)
        {
            while ((sn[i] < '0' || sn[i] > '9') && (sn[i] < 'A' || sn[i] > 'Z') && (sn[i] < 'a' || sn[i] > 'z'))
            {
                sn[i] = (sn[i] + 31 + 7 * i) % 128;
            }
        }
    }
    else
    {
        return NULL;
    }

    //printf("sn:%s\n",sn);
    return sn;
}

/*****************************************************************************
 函 数 名  : checkExpireDay
 功能描述  : 检查有效期
 输入参数  : char *ExpireDay
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年8月17日 星期一
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int checkExpireDay(char *ExpireDay)
{
    char year[5] = {0}, month[3] = {0}, day[3] = {0};
    int d_year = 0, d_month = 0, d_day = 0;

    time_t now = time(NULL);
    struct tm *tp;

    tp = localtime(&now);

    if ((ExpireDay == NULL) || (strlen(ExpireDay) != 8))
    {
        return -1;
    }

    osip_strncpy(year, ExpireDay, 4);
    osip_strncpy(month, ExpireDay + 4, 2);
    osip_strncpy(day, ExpireDay + 6, 2);

    year[4] = '\0';
    month[2] = '\0';
    day[2] = '\0';

    d_year = atoi(year) - 1900;
    d_month = atoi(month) - 1;
    d_day = atoi(day);

    if (tp != NULL)
    {
        if (tp->tm_year > d_year)
        {
            return -1;
        }
        else if ((tp->tm_year == d_year) && (tp->tm_mon > d_month))
        {
            return -1;
        }
        else if ((tp->tm_year == d_year) && (tp->tm_mon == d_month) && (tp->tm_mday > d_day))
        {
            return -1;
        }
    }
    else
    {
        return -1;
    }

    return 0;

}

/*****************************************************************************
 函 数 名  : checklicence
 功能描述  : 检查授权文件
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年8月17日 星期一
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int checklicence()
{
    int i = 0, result = 0xaa;
    FILE *fp = NULL;
    struct stat statbuf;
    char str[256] = {0};
    char *pname = NULL, *pvalue = NULL, *tmp = NULL;
    char Key[256] = {0}, ExpireDay[256] = {0};

    osip_MD5_CTX Md5Ctx;
    HASH HA1;
    OUT HASHHEX SessionKey;

    char *passwd1 = (char*)"http://www.wisvision.com.cn/025-52763366";
    char *passwd2 = NULL;

    fp = fopen(CMS_KEY_FILENAME, "r");

    if (NULL == fp)
    {
        /* 检查临时文件 */
        i = checkTmpLicence();
        printf("checklicence() exit---: checkTmpLicence:i=%d \r\n", i);
        return i;
    }

    /* 获取授权文件 */
    while (NULL != fgets(str, sizeof(str), fp))
    {
        if ((strlen(str) == 1) || (0 == strncmp(str, "#", 1)))
        {
            continue;
        }

        tmp = strchr(str, 61); /*find '=' */

        if (tmp != NULL)
        {
            pname = (char *) osip_malloc(tmp - str + 1);
            pvalue = (char *) osip_malloc(str + strlen(str) - tmp);
            osip_strncpy(pname, str, tmp - str);
            osip_strncpy(pvalue, tmp + 1, str + strlen(str) - tmp - 2);

            sclrspace(pname);

            if (0 == sstrcmp(pname, (char*)"Key"))
            {
                osip_strncpy(Key, pvalue, 255);
                printf("checklicence() Key=%s\r\n", Key);
            }
            else if (0 == sstrcmp(pname, (char*)"ExpireDay"))
            {
                osip_strncpy(ExpireDay, pvalue, 255);
                printf("checklicence() ExpireDay=%s\r\n", ExpireDay);
            }

            osip_free(pname);
            osip_free(pvalue);
        }
    }

    fclose(fp);

    /* get file stat */
    if (!stat(CMS_KEY_FILENAME, &statbuf))
    {
        Key_lmt = statbuf.st_mtime;
    }

    /* 判断文件内容是否有正确 */
    if ((Key[0] == '\0') || (ExpireDay[0] == '\0'))
    {
        printf("checklicence() exit---: Licence key file not correct, Please check it!!!\n");
        return -1;
    }

    /* 检查有效期 */
    if (checkExpireDay(ExpireDay) == -1)
    {
        printf("checklicence() exit---: checkExpireDay Error \r\n");
        return 1;
    }

    passwd2 = GetRegPasswd(passwd1);

    if (passwd2 == NULL)
    {
        printf("checklicence() exit---: Get passwd2 Error \r\n");
        return -1;
    }

    /* MD5 Auth */
    osip_MD5Init(&Md5Ctx);
    osip_MD5Update(&Md5Ctx, (unsigned char*)ExpireDay, strlen(ExpireDay));
    osip_MD5Update(&Md5Ctx, (unsigned char*)":", 1);
    osip_MD5Update(&Md5Ctx, (unsigned char*)passwd2, strlen(passwd2));
    osip_MD5Final((unsigned char *)HA1, &Md5Ctx);
    CvtHex(HA1, SessionKey);

    if ((char *)SessionKey == NULL)
    {
        printf("checklicence() exit---: Get SessionKey Error \r\n");
        return -1;
    }

    result = strncmp((char *)SessionKey, Key, 32);

    if (result == 0)
    {
        return 0;
    }

    return -1;
}

#if 0
/*****************************************************************************
 函 数 名  : checklicence
 功能描述  : 检查授权文件
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年8月17日 星期一
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int checklicence()
{
    int i = 0, result = 0xaa;
    int iTmp = 0;
    int fd = 0;
    FILE *fp = NULL;
    struct stat statbuf;
    char str[256] = {0};
    char *pname = NULL, *pvalue = NULL, *tmp = NULL;
    char Key[256] = {0}, ExpireDay[256] = {0}, RegistrationLimit[256] = {0};
    int interfaceNum = 0;
    struct ifreq buf[16];
    struct ifconf ifc;
    struct ifreq ifrcopy;

    osip_MD5_CTX Md5Ctx;
    HASH HA1;
    OUT HASHHEX SessionKey;

    char mac[18] = {0};
    char *passwd1 = "http://www.wisvision.com.cn/025-52763366";
    char *passwd2 = NULL;

    fp = fopen(CMS_KEY_FILENAME, "r");

    if (NULL == fp)
    {
        /* 检查临时文件 */
        i = checkTmpLicence();
        printf("checklicence() exit---: checkTmpLicence:i=%d \r\n", i);
        return i;
    }

    /* 获取授权文件 */
    while (NULL != fgets(str, sizeof(str), fp))
    {
        if ((strlen(str) == 1) || (0 == strncmp(str, "#", 1)))
        {
            continue;
        }

        tmp = strchr(str, 61); /*find '=' */

        if (tmp != NULL)
        {
            pname = (char *) osip_malloc(tmp - str + 1);
            pvalue = (char *) osip_malloc(str + strlen(str) - tmp);
            osip_strncpy(pname, str, tmp - str);
            osip_strncpy(pvalue, tmp + 1, str + strlen(str) - tmp - 2);

            sclrspace(pname);

            if (0 == sstrcmp(pname, (char*)"Key"))
            {
                osip_strncpy(Key, pvalue, 255);
                printf("checklicence() Key=%s\r\n", Key);
            }

#if 0
            else if (0 == sstrcmp(pname, (char*)"LicensedTo"))
            {
                osip_strncpy(LicensedTo, pvalue, 255);
                printf("checklicence() LicensedTo=%s\r\n", LicensedTo);
            }

#endif
            else if (0 == sstrcmp(pname, (char*)"ExpireDay"))
            {
                osip_strncpy(ExpireDay, pvalue, 255);
                printf("checklicence() ExpireDay=%s\r\n", ExpireDay);
            }
            else if (0 == sstrcmp(pname, (char*)"RegistrationLimit"))
            {
                osip_strncpy(RegistrationLimit, pvalue, 255);

                iTmp = osip_atoi(pvalue);

                if (iTmp > 0)
                {
                    g_RegistrationLimit = iTmp;
                }

                printf("checklicence() RegistrationLimit=%u\r\n", g_RegistrationLimit);
            }

            osip_free(pname);
            osip_free(pvalue);
        }
    }

    fclose(fp);

    /* get file stat */
    if (!stat(CMS_KEY_FILENAME, &statbuf))
    {
        Key_lmt = statbuf.st_mtime;
    }

    if ((Key[0] == '\0') || (ExpireDay[0] == '\0') || (RegistrationLimit[0] == '\0'))
    {
        printf("checklicence() exit---: Licence key file not correct ,please check it!!!\n");
        return -1;
    }

    if (checkExpireDay(ExpireDay) == -1)
    {
        printf("checklicence() exit---: checkExpireDay Error \r\n");
        return 1;
    }

    passwd2 = GetRegPasswd(passwd1);

    if (passwd2 == NULL)
    {
        printf("checklicence() exit---: Get passwd2 Error \r\n");
        return -1;
    }

    /* 获取本机的MAC地址 */
    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        printf("checklicence() exit---: Socket Error \r\n");
        close(fd);
        return -1;
    }

    ifc.ifc_len = sizeof(buf);
    ifc.ifc_buf = (caddr_t)buf;

    if (!ioctl(fd, SIOCGIFCONF, (char*)&ifc))
    {
        interfaceNum = ifc.ifc_len / sizeof(struct ifreq);

        /* 循环添加每个网卡的信息 */
        while (interfaceNum-- > 0)
        {
            /* 网卡名称*/
            if (0 != strncmp(buf[interfaceNum].ifr_name, (char*)"eth", 3)
                && 0 != strncmp(buf[interfaceNum].ifr_name, (char*)"bond", 4)  /* 绑定网口的支持 */
                && 0 != strncmp(buf[interfaceNum].ifr_name, (char*)"mgmt", 4)) /* 管理网口的支持 */
            {
                continue;
            }

            //ignore the interface that not up or not runing
            ifrcopy = buf[interfaceNum];

            if (ioctl(fd, SIOCGIFFLAGS, &ifrcopy))
            {
                printf("checklicence() exit---: ioctl: %s [%s:%d]\n", strerror(errno), __FILE__, __LINE__);
                close(fd);
                return -1;
            }

            if (!ioctl(fd, SIOCGIFHWADDR, (char*)(&buf[interfaceNum])))
            {
                memset(mac, 0, sizeof(mac));
                snprintf(mac, sizeof(mac), "%02x:%02x:%02x:%02x:%02x:%02x",
                         (unsigned char)buf[interfaceNum].ifr_hwaddr.sa_data[0],
                         (unsigned char)buf[interfaceNum].ifr_hwaddr.sa_data[1],
                         (unsigned char)buf[interfaceNum].ifr_hwaddr.sa_data[2],
                         (unsigned char)buf[interfaceNum].ifr_hwaddr.sa_data[3],
                         (unsigned char)buf[interfaceNum].ifr_hwaddr.sa_data[4],
                         (unsigned char)buf[interfaceNum].ifr_hwaddr.sa_data[5]);

                if (mac[0] == '\0')
                {
                    continue;
                }

                printf("checklicence() InterFace name: %s, MAC=%s \r\n", buf[interfaceNum].ifr_name, mac);

                /* MD5 Auth */
                osip_MD5Init(&Md5Ctx);
                osip_MD5Update(&Md5Ctx, (unsigned char*)ExpireDay, strlen(ExpireDay));
                osip_MD5Update(&Md5Ctx, (unsigned char*)":", 1);
                osip_MD5Update(&Md5Ctx, (unsigned char*)RegistrationLimit, strlen(RegistrationLimit));
                osip_MD5Update(&Md5Ctx, (unsigned char*)":", 1);
                osip_MD5Update(&Md5Ctx, (unsigned char*)mac, strlen(mac));
                osip_MD5Update(&Md5Ctx, (unsigned char*)":", 1);
                osip_MD5Update(&Md5Ctx, (unsigned char*)passwd2, strlen(passwd2));
                osip_MD5Update(&Md5Ctx, (unsigned char*)":", 1);
                //stolowercase(LicensedTo);
                //osip_MD5Update(&Md5Ctx, (unsigned char*)LicensedTo, strlen(LicensedTo));
                osip_MD5Final((unsigned char *)HA1, &Md5Ctx);
                CvtHex(HA1, SessionKey);

                if ((char *)SessionKey == NULL)
                {
                    return -1;
                }

                result = strncmp((char *)SessionKey, Key, 32);

                if (result == 0)
                {
                    return 0;
                }
            }
            else
            {
                printf("checklicence() ioctl: %s [%s:%d]\n", strerror(errno), __FILE__, __LINE__);
                close(fd);
                return -1;
            }
        }
    }
    else
    {
        printf("checklicence() exit---: ioctl: %s [%s:%d]\n", strerror(errno), __FILE__, __LINE__);
        close(fd);
        return -1;
    }

    close(fd);
    return -1;
}
#endif

/*****************************************************************************
 函 数 名  : checkTmpLicence
 功能描述  : 检查临时文件
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年10月16日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int checkTmpLicence()
{
    int i = 0, result = 0xaa;
    FILE *fp = NULL;
    struct stat statbuf;
    char str[256] = {0};
    char *pname = NULL, *pvalue = NULL, *tmp = NULL;
    char Key[256] = {0}, ExpireDay[256] = {0};

    osip_MD5_CTX Md5Ctx;
    HASH HA1;
    OUT HASHHEX SessionKey;

    char *passwd1 = (char*)"http://www.wisvision.com.cn/025-52763366";
    char *passwd2 = NULL;

    fp = fopen(CMS_KEY_TMP_FILENAME, "r");

    if (NULL == fp)
    {
        /* 新增临时文件 */
        i = createNewTmpLicence();

        if (0 != i)
        {
            printf("checkTmpLicence() exit---: createNewTmpLicence Error \r\n");
            return -1;
        }
        else
        {
            /* 再次打开 */
            fp = fopen(CMS_KEY_TMP_FILENAME, "r");

            if (NULL == fp)
            {
                printf("checkTmpLicence() exit---: Open Tmp Key File Error \r\n");
                return -1;
            }
        }
    }

    /* 获取授权文件 */
    while (NULL != fgets(str, sizeof(str), fp))
    {
        if ((strlen(str) == 1) || (0 == strncmp(str, "#", 1)))
        {
            continue;
        }

        tmp = strchr(str, 61); /*find '=' */

        if (tmp != NULL)
        {
            pname = (char *) osip_malloc(tmp - str + 1);
            pvalue = (char *) osip_malloc(str + strlen(str) - tmp);
            osip_strncpy(pname, str, tmp - str);
            osip_strncpy(pvalue, tmp + 1, str + strlen(str) - tmp - 2);

            sclrspace(pname);

            if (0 == sstrcmp(pname, (char*)"Key"))
            {
                osip_strncpy(Key, pvalue, 255);
                printf("checkTmpLicence() Key=%s\r\n", Key);
            }
            else if (0 == sstrcmp(pname, (char*)"ExpireDay"))
            {
                osip_strncpy(ExpireDay, pvalue, 255);
                printf("checkTmpLicence() ExpireDay=%s\r\n", ExpireDay);
            }

            osip_free(pname);
            osip_free(pvalue);
        }
    }

    fclose(fp);

    /* get file stat */
    if (!stat(CMS_KEY_TMP_FILENAME, &statbuf))
    {
        Key_Tmp_lmt = statbuf.st_mtime;
    }

    /* 判断文件内容是否有正确 */
    if ((Key[0] == '\0') || (ExpireDay[0] == '\0'))
    {
        printf("checkTmpLicence() exit---: Licence key file not correct, Please check!\n");
        return -1;
    }

    /* 检查有效期 */
    if (checkExpireDay(ExpireDay) == -1)
    {
        printf("checkTmpLicence() exit---: checkExpireDay Error \r\n");
        return 1;
    }

    passwd2 = GetRegPasswd(passwd1);

    if (passwd2 == NULL)
    {
        printf("checkTmpLicence() exit---: Get passwd2 Error \r\n");
        return -1;
    }

    /* MD5 Auth */
    osip_MD5Init(&Md5Ctx);
    osip_MD5Update(&Md5Ctx, (unsigned char*)ExpireDay, strlen(ExpireDay));
    osip_MD5Update(&Md5Ctx, (unsigned char*)":", 1);
    osip_MD5Update(&Md5Ctx, (unsigned char*)passwd2, strlen(passwd2));
    osip_MD5Final((unsigned char *)HA1, &Md5Ctx);
    CvtHex(HA1, SessionKey);

    if ((char *)SessionKey == NULL)
    {
        printf("checkTmpLicence() exit---: Get SessionKey Error \r\n");
        return -1;
    }

    result = strncmp((char *)SessionKey, Key, 32);

    if (result == 0)
    {
        return 0;
    }

    return -1;
}

/*****************************************************************************
 函 数 名  : checkLoginIPIsSlaveIP
 功能描述  : 主备启用的情况下，检测注册IP地址是否是备机的IP地址
 输入参数  : char* host
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年9月28日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int checkLoginIPIsSlaveIP(char* host)
{
    /* 主备启用的情况，看下是否是备机媒体网关注册来的 */
    if (g_BoardNetConfig.dwMSFlag == 1)  /* 主备启用的情况下 */
    {
        char s8Ip[CMS_IP_STR_LEN] = {0};
        osip_strncpy(s8Ip, inet_ntoa(*(struct in_addr*)&g_BoardNetConfig.tCmsSVideoIP.tNetIP.dwIPAddr), CMS_IP_STR_LEN);

        if (0 == sstrcmp(host, s8Ip))
        {
            return 1;
        }
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : createNewTmpLicence
 功能描述  : 生成临时授权文件，默认30天后过期
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年10月16日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int createNewTmpLicence()
{
    FILE *fp = NULL;
    char str[256] = {0};
    char ExpireDay[32] = {0};

    osip_MD5_CTX Md5Ctx;
    HASH HA1;
    OUT HASHHEX SessionKey;

    time_t now = time(NULL);
    struct tm local_time = { 0 };

    char *passwd1 = (char*)"http://www.wisvision.com.cn/025-52763366";
    char *passwd2 = NULL;

    /* 新增临时文件 */
    fp = fopen(CMS_KEY_TMP_FILENAME, "w+");

    if (NULL != fp)
    {
        /* 生成字符串 */
        passwd2 = GetRegPasswd(passwd1);

        if (passwd2 == NULL)
        {
            printf("createNewTmpLicence() exit---: Get passwd2 Error \r\n");
            return -1;
        }

        /* 生成日期,默认30天:30*24*3600 = 2592000 */
        now = now + 2592000;
        localtime_r(&now, &local_time);
        strftime(ExpireDay, sizeof(ExpireDay), "%Y%m%d", &local_time);

        /* MD5 Auth */
        osip_MD5Init(&Md5Ctx);
        osip_MD5Update(&Md5Ctx, (unsigned char*)ExpireDay, strlen(ExpireDay));
        osip_MD5Update(&Md5Ctx, (unsigned char*)":", 1);
        osip_MD5Update(&Md5Ctx, (unsigned char*)passwd2, strlen(passwd2));
        osip_MD5Final((unsigned char *)HA1, &Md5Ctx);
        CvtHex(HA1, SessionKey);

        if ((char *)SessionKey == NULL)
        {
            printf("createNewTmpLicence() exit---: Get SessionKey Error \r\n");
            return -1;
        }

        /* 写入文件 */
        snprintf(str, 200, "Key=%s\n", (char *)SessionKey);
        fputs(str, fp);
        snprintf(str, 200, "ExpireDay=%s\n", ExpireDay);
        fputs(str, fp);

        fclose(fp);
        fp = NULL;

        /* 将文件属性置为可读性 */
        snprintf(str, 200, "chmod 777 %s", CMS_KEY_TMP_FILENAME);
        system(str);

        return 0;
    }

    printf("createNewTmpLicence() exit---: Wirte Tmp licence key file Error \r\n");
    return -1;
}

/*****************************************************************************
 函 数 名  : show_system_gbl_param
 功能描述  : 显示系统全局配置参数
 输入参数  : int sock
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年9月7日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
void show_system_gbl_param(int sock)
{
    char strLine[] = "\r------------------------------------------------------------------------------------------------\r\n";
    char rbuf[256] = {0};

    if (sock > 0)
    {
        send(sock, strLine, strlen(strLine), 0);
    }

    snprintf(rbuf, 256, "\rCMS编码                                :CMS ID=%s \r\n", pGblconf->board_id);

    if (sock > 0)
    {
        send(sock, rbuf, strlen(rbuf), 0);
    }

    snprintf(rbuf, 256, "\r对应的MMS编码                          :MMS ID=%s \r\n", pGblconf->mms_id);

    if (sock > 0)
    {
        send(sock, rbuf, strlen(rbuf), 0);
    }

    snprintf(rbuf, 256, "\rCMS别名                                :CMS Name=%s \r\n", pGblconf->cms_name);

    if (sock > 0)
    {
        send(sock, rbuf, strlen(rbuf), 0);
    }

    snprintf(rbuf, 256, "\r中心编码                               :center_code=%s \r\n", pGblconf->center_code);

    if (sock > 0)
    {
        send(sock, rbuf, strlen(rbuf), 0);
    }

    snprintf(rbuf, 256, "\r省级编码                               :province_code=%s \r\n", pGblconf->province_code);

    if (sock > 0)
    {
        send(sock, rbuf, strlen(rbuf), 0);
    }

    snprintf(rbuf, 256, "\r市级编码                               :city_code=%s \r\n", pGblconf->city_code);

    if (sock > 0)
    {
        send(sock, rbuf, strlen(rbuf), 0);
    }

    snprintf(rbuf, 256, "\r区县编码                               :region_code=%s \r\n", pGblconf->region_code);

    if (sock > 0)
    {
        send(sock, rbuf, strlen(rbuf), 0);
    }

    snprintf(rbuf, 256, "\r本CMS的行政区域编码                    :civil_code=%s \r\n", pGblconf->civil_code);

    if (sock > 0)
    {
        send(sock, rbuf, strlen(rbuf), 0);
    }

    snprintf(rbuf, 256, "\r行业编码                               :trade_code=%s \r\n", pGblconf->trade_code);

    if (sock > 0)
    {
        send(sock, rbuf, strlen(rbuf), 0);
    }

    snprintf(rbuf, 256, "\r类型编码                               :type_code=%s \r\n", pGblconf->type_code);

    if (sock > 0)
    {
        send(sock, rbuf, strlen(rbuf), 0);
    }

    snprintf(rbuf, 256, "\r序号                                   :serial_number=%s \r\n", pGblconf->serial_number);

    if (sock > 0)
    {
        send(sock, rbuf, strlen(rbuf), 0);
    }

    snprintf(rbuf, 256, "\r注册域                                 :register_region=%s \r\n", pGblconf->register_region);

    if (sock > 0)
    {
        send(sock, rbuf, strlen(rbuf), 0);
    }

    snprintf(rbuf, 256, "\r数据库IP地址                           :db_server_ip=%s \r\n", pGblconf->db_server_ip);

    if (sock > 0)
    {
        send(sock, rbuf, strlen(rbuf), 0);
    }

    snprintf(rbuf, 256, "\r告警服务器IP地址                       :alarm_server_ip=%s \r\n", pGblconf->alarm_server_ip);

    if (sock > 0)
    {
        send(sock, rbuf, strlen(rbuf), 0);
    }

    snprintf(rbuf, 256, "\rNTP服务器IP地址                        :ntp_server_ip=%s \r\n", pGblconf->ntp_server_ip);

    if (sock > 0)
    {
        send(sock, rbuf, strlen(rbuf), 0);
    }

    snprintf(rbuf, 256, "\r默认网口名称                           :default_eth_name=%s \r\n", pGblconf->default_eth_name);

    if (sock > 0)
    {
        send(sock, rbuf, strlen(rbuf), 0);
    }

    snprintf(rbuf, 256, "\r设备认证标识                           :AuthFlag=%d \r\n", pGblconf->iAuthFlag);

    if (sock > 0)
    {
        send(sock, rbuf, strlen(rbuf), 0);
    }

    snprintf(rbuf, 256, "\r预置位自动归位时长                     :PresetBackTime=%u \r\n", pGblconf->uPresetBackTime);

    if (sock > 0)
    {
        send(sock, rbuf, strlen(rbuf), 0);
    }

    snprintf(rbuf, 256, "\r点位自动解锁时长                       :DeviceUnLockTime=%u \r\n", pGblconf->uDeviceUnLockTime);

    if (sock > 0)
    {
        send(sock, rbuf, strlen(rbuf), 0);
    }

    snprintf(rbuf, 256, "\r未注册成功，重新注册时间间隔(默认60秒) :register_retry_interval=%u \r\n", pGblconf->register_retry_interval);

    if (sock > 0)
    {
        send(sock, rbuf, strlen(rbuf), 0);
    }

    snprintf(rbuf, 256, "\r注册刷新周期(默认180秒)                :registry_cleanuprate=%u \r\n", pGblconf->registry_cleanuprate);

    if (sock > 0)
    {
        send(sock, rbuf, strlen(rbuf), 0);
    }

    snprintf(rbuf, 256, "\r会话超时时间(默认300秒)                :session_expires=%u \r\n", pGblconf->session_expires);

    if (sock > 0)
    {
        send(sock, rbuf, strlen(rbuf), 0);
    }

    snprintf(rbuf, 256, "\r订阅超时时间(默认3600秒)               :subscribe_expires=%u \r\n", pGblconf->subscribe_expires);

    if (sock > 0)
    {
        send(sock, rbuf, strlen(rbuf), 0);
    }

    snprintf(rbuf, 256, "\r保活间隔时间(默认60秒)                 :keep_alive_interval=%u \r\n", pGblconf->keep_alive_interval);

    if (sock > 0)
    {
        send(sock, rbuf, strlen(rbuf), 0);
    }

    snprintf(rbuf, 256, "\r保活失败次数(默认3次)                  :failed_keep_alive_count=%u \r\n", pGblconf->failed_keep_alive_count);

    if (sock > 0)
    {
        send(sock, rbuf, strlen(rbuf), 0);
    }

    snprintf(rbuf, 256, "\r保活超时时间(默认3分钟)                :keep_alive_expires=%u \r\n", pGblconf->keep_alive_expires);

    if (sock > 0)
    {
        send(sock, rbuf, strlen(rbuf), 0);
    }

    snprintf(rbuf, 256, "\r告警延时时间(默认30秒)                 :alarm_duration=%u \r\n", pGblconf->alarm_duration);

    if (sock > 0)
    {
        send(sock, rbuf, strlen(rbuf), 0);
    }

    snprintf(rbuf, 256, "\r日志是否记录到数据库                   :IsLog2DB=%d \r\n", g_IsLog2DB);

    if (sock > 0)
    {
        send(sock, rbuf, strlen(rbuf), 0);
    }

    snprintf(rbuf, 256, "\r日志是否记录到文件                     :IsLog2File=%d \r\n", g_IsLog2File);

    if (sock > 0)
    {
        send(sock, rbuf, strlen(rbuf), 0);
    }

    snprintf(rbuf, 256, "\r是否记录SIP Message日志                :IsLogSIPMsg=%d \r\n", g_IsLogSIPMsg);

    if (sock > 0)
    {
        send(sock, rbuf, strlen(rbuf), 0);
    }

    snprintf(rbuf, 256, "\r系统调试日志等级                       :DbgLevel=%d \r\n", g_CommonDbgLevel);

    if (sock > 0)
    {
        send(sock, rbuf, strlen(rbuf), 0);
    }

    snprintf(rbuf, 256, "\r系统运行日志等级                       :SystemLogLevel=%d \r\n", g_SystemLogLevel);

    if (sock > 0)
    {
        send(sock, rbuf, strlen(rbuf), 0);
    }

    snprintf(rbuf, 256, "\r系统运行日志记录到数据库等级           :SystemLog2DBLevel=%d \r\n", g_SystemLog2DBLevel);

    if (sock > 0)
    {
        send(sock, rbuf, strlen(rbuf), 0);
    }

    snprintf(rbuf, 256, "\r是否发送订阅消息                       :IsSubscribe=%d \r\n", g_IsSubscribe);

    if (sock > 0)
    {
        send(sock, rbuf, strlen(rbuf), 0);
    }

    snprintf(rbuf, 256, "\r报警消息是否发送给用户                 :AlarmMsgSendToUserFlag=%d \r\n", g_AlarmMsgSendToUserFlag);

    if (sock > 0)
    {
        send(sock, rbuf, strlen(rbuf), 0);
    }

    snprintf(rbuf, 256, "\r报警消息是否发送给上级路由             :AlarmMsgSendToRouteFlag=%d \r\n", g_AlarmMsgSendToRouteFlag);

    if (sock > 0)
    {
        send(sock, rbuf, strlen(rbuf), 0);
    }

    snprintf(rbuf, 256, "\r下级媒体流是否经过本地TSU转发          :LocalMediaTransferFlag=%d \r\n", g_LocalMediaTransferFlag);

    if (sock > 0)
    {
        send(sock, rbuf, strlen(rbuf), 0);
    }

    snprintf(rbuf, 256, "\r下级媒体切电视墙是否经过本地TSU转发    :DECMediaTransferFlag=%d \r\n", g_DECMediaTransferFlag);

    if (sock > 0)
    {
        send(sock, rbuf, strlen(rbuf), 0);
    }

    snprintf(rbuf, 256, "\r上级第三方平台是否有媒体转发功能       :RouteMediaTransferFlag=%d \r\n", g_RouteMediaTransferFlag);

    if (sock > 0)
    {
        send(sock, rbuf, strlen(rbuf), 0);
    }

    snprintf(rbuf, 256, "\rSX、RX是否启用MMS功能                  :MMSEnableFlag=%d \r\n", g_MMSEnableFlag);

    if (sock > 0)
    {
        send(sock, rbuf, strlen(rbuf), 0);
    }

    snprintf(rbuf, 256, "\r下级平台分组信息解析标识               :AnalysisSubGroupFlag=%d \r\n", g_AnalysisSubGroupFlag);

    if (sock > 0)
    {
        send(sock, rbuf, strlen(rbuf), 0);
    }

    snprintf(rbuf, 256, "\r日志缓存队列大小                       :LogQueryBufferSize=%d \r\n", g_LogQueryBufferSize);

    if (sock > 0)
    {
        send(sock, rbuf, strlen(rbuf), 0);
    }

    snprintf(rbuf, 256, "\r主备启用标识                           :MSFlag=%d \r\n", g_BoardNetConfig.dwMSFlag);

    if (sock > 0)
    {
        send(sock, rbuf, strlen(rbuf), 0);
    }

    snprintf(rbuf, 256, "\r系统当前使用的语言                     :Language=%d \r\n", g_Language);

    if (sock > 0)
    {
        send(sock, rbuf, strlen(rbuf), 0);
    }

    snprintf(rbuf, 256, "\r系统国标模式标识                       :show code=%d \r\n", pGblconf->showcode);

    if (sock > 0)
    {
        send(sock, rbuf, strlen(rbuf), 0);
    }

    if (sock > 0)
    {
        send(sock, strLine, strlen(strLine), 0);
    }

    return;
}

/*****************************************************************************
 函 数 名  : show_system_ip_info
 功能描述  : 显示本地ip地址信息
 输入参数  : int sock
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年9月7日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
void show_system_ip_info(int sock)
{
    int i = 0;
    char strLine[] = "\r--------------------------------------------\r\n";
    char strHead[] = "\rEthName    IP Type  IP Addr         Port    \r\n";
    char rbuf[256] = {0};
    ip_pair_t* pIPaddr = NULL;

    if (sock > 0)
    {
        send(sock, strLine, strlen(strLine), 0);
        send(sock, strHead, strlen(strHead), 0);
    }

    if (pGblconf == NULL || NULL == pGblconf->pLocalIPAddrList)
    {
        return;
    }

    if (osip_list_size(pGblconf->pLocalIPAddrList) <= 0)
    {
        return;
    }

    for (i = 0; i < osip_list_size(pGblconf->pLocalIPAddrList); i++)
    {
        pIPaddr = (ip_pair_t*)osip_list_get(pGblconf->pLocalIPAddrList, i);

        if (NULL == pIPaddr || pIPaddr->eth_name[0] == '\0')
        {
            continue;
        }

        snprintf(rbuf, 256, "\r%-10s %-8d %-15s %-8d\r\n", pIPaddr->eth_name, pIPaddr->ip_type, pIPaddr->local_ip, pIPaddr->local_port);

        if (sock > 0)
        {
            send(sock, rbuf, strlen(rbuf), 0);
        }
    }

    if (sock > 0)
    {
        send(sock, strLine, strlen(strLine), 0);
    }

    return;
}

/*****************************************************************************
 函 数 名  : show_system_civil_info
 功能描述  : 显示系统使用的行政区域编码
 输入参数  : int sock
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年9月7日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
void show_system_civil_info(int sock)
{
    char strLine[] = "\r-------------------------------------------------------------------------------------------------------------------------------------------------------------\r\n";
    char strHead[] = "\rGroupID                          GroupName                        ParentID                         CivilCode GroupCode            ParentCode           Upload\r\n";
    char rbuf[256] = {0};
    primary_group_t* pPrimaryGroup = NULL;
    PrimaryGroup_Iterator Itr;

    if (sock > 0)
    {
        send(sock, strLine, strlen(strLine), 0);
        send(sock, strHead, strlen(strHead), 0);
    }

    if (pGblconf == NULL)
    {
        return;
    }

    if (pGblconf->PrimaryGroupList.size() <= 0)
    {
        return;
    }

    for (Itr = pGblconf->PrimaryGroupList.begin(); Itr != pGblconf->PrimaryGroupList.end(); Itr++)
    {
        pPrimaryGroup = Itr->second;

        if (NULL == pPrimaryGroup || pPrimaryGroup->group_code[0] == '\0')
        {
            continue;
        }

        snprintf(rbuf, 256, "\r%-32s %-32s %-32s %-9s %-20s %-20s %-12d\r\n", pPrimaryGroup->group_id, pPrimaryGroup->group_name, pPrimaryGroup->parent_id, pPrimaryGroup->civil_code, pPrimaryGroup->group_code, pPrimaryGroup->parent_code, pPrimaryGroup->iNeedToUpLoad);

        if (sock > 0)
        {
            send(sock, rbuf, strlen(rbuf), 0);
        }
    }

    if (sock > 0)
    {
        send(sock, strLine, strlen(strLine), 0);
    }

    return;
}

/*****************************************************************************
 函 数 名  : show_system_group_map_info
 功能描述  : 显示系统使用的逻辑设备分组关系
 输入参数  : int sock
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年11月16日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
void show_system_group_map_info(int sock)
{
    char strLine[] = "\r------------------------------------------------------------------\r\n";
    char strHead[] = "\rGroupID                          DeviceIndex                      \r\n";
    char rbuf[256] = {0};
    device_group_map_t* pDeviceGroupMap = NULL;
    DeviceGroupMap_Iterator Itr;

    if (sock > 0)
    {
        send(sock, strLine, strlen(strLine), 0);
        send(sock, strHead, strlen(strHead), 0);
    }

    if (pGblconf == NULL)
    {
        return;
    }

    if (pGblconf->DeviceGroupMapList.size() <= 0)
    {
        return;
    }

    for (Itr = pGblconf->DeviceGroupMapList.begin(); Itr != pGblconf->DeviceGroupMapList.end(); Itr++)
    {
        pDeviceGroupMap = Itr->second;

        if (NULL == pDeviceGroupMap || pDeviceGroupMap->group_id[0] == '\0' || pDeviceGroupMap->device_index <= 0)
        {
            continue;
        }

        snprintf(rbuf, 256, "\r%-32s %-32u\r\n", pDeviceGroupMap->group_id, pDeviceGroupMap->device_index);

        if (sock > 0)
        {
            send(sock, rbuf, strlen(rbuf), 0);
        }
    }

    if (sock > 0)
    {
        send(sock, strLine, strlen(strLine), 0);
    }

    return;
}

