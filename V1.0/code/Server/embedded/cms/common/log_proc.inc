/******************************************************************************

                  版权所有 (C), 2001-2013, 金智视讯技术有限公司

 ******************************************************************************
  文 件 名   : csdbg.h
  版 本 号   : 初稿
  作    者   : yanghaifeng
  生成日期   : 2013年4月1日
  最近修改   :
  功能描述   : csdbg.c 的头文件
  函数列表   :
  修改历史   :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 创建文件

******************************************************************************/
#ifndef __LOG_PROC_H__
#define __LOG_PROC_H__

/*----------------------------------------------*
 * 包含头文件                                   *
 *----------------------------------------------*/
#include "gbldef.inc"
#include "libsip.h"

#include <vector>
#include <string>
#include <queue>

#include "common/DBOper.h"
#include "user/user_info_mgn.inc"

using namespace std;

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

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
    /* SIP消息文件记录结构 */
    typedef struct _sip_msglog_t
    {
        int sndcnt;
        int rcvcnt;
        FILE* logfile;
    } sip_msglog_t;

    /* SIP错误消息文件记录结构 */
    typedef struct _sip_errorlog_t
    {
        int cnt;
        FILE* logfile;
    } sip_errorlog_t;

    /* 系统调试日志文件记录结构 */
    typedef struct _debug_log_t
    {
        FILE* logfile;
    } debug_log_t;

    typedef debug_log_t run_log_t;

    typedef struct _cms_log_t
    {
        sip_msglog_t* sip_msglog;
        sip_errorlog_t* sip_errorlog;
        debug_log_t* debug_log;
        run_log_t* run_log;
    } cms_log_t;

    /* 1、SIP消息日志记录文件 结构体 */
    typedef struct _sip_msg_log2file_t
    {
        int type;
        char msg[MAX_2048CHAR_STRING_LEN + 4];
        char ipaddr[MAX_IP_LEN];
        int port;
    } sip_msg_log2file_t;

    /* SIP消息日志队列 */
    typedef std::deque <sip_msg_log2file_t*> sip_msg_log2file_queue;


    /* 2、日志记录到文件 结构体 */
    typedef struct _trace_log2file_t
    {
        int type;
        char msg[MAX_2048CHAR_STRING_LEN + 4];
    } trace_log2file_t;

    /* 日志记录到文件消息队列 */
    typedef std::deque <trace_log2file_t*> trace_log2file_queue;

    /* 3、系统日志记录到数据库结构体 */
    typedef struct _system_log2db_t
    {
        int iType;                /* 日志类型 */
        int iLevel;               /* 日志级别 */
        int iTime;                /* 日志时间*/
        char msg[MAX_2048CHAR_STRING_LEN + 4];           /* 日志内容 */
    } system_log2db_t;

    /* 系统日志记录到数据库消息队列 */
    typedef std::deque <system_log2db_t*> system_log2db_queue;

    /* 4、用户操作记录到数据库结构体 */
    typedef struct _user_log2db_t
    {
        char pcUserID[MAX_ID_LEN + 4];               /* 用户ID */
        char pcUserIP[MAX_IP_LEN];                   /* 用户IP */
        unsigned int iUserIndex;                     /* 用户索引 */
        int iType;                                   /* 日志类型 */
        int iLevel;                                  /* 日志级别 */
        int iTime;                                   /* 日志时间*/
        char msg[MAX_2048CHAR_STRING_LEN + 4];       /* 日志内容 */
    } user_log2db_t;

    /* 用户操作记录到数据库消息队列 */
    typedef std::deque <user_log2db_t*> user_log2db_queue;

    /*----------------------------------------------*
     * 常量定义                                     *
     *----------------------------------------------*/
    /* 日志级别 */
#define LOG_ALL     1    /* 打印所有日志 */

#define LOG_DEBUG   2    /* 打印调试日志 */
#define LOG_TRACE   3    /* 打印跟踪日志 */
#define LOG_INFO    4    /* 打印一般日志 */
#define LOG_WARN    5    /* 打印告警日志 */
#define LOG_ERROR   6    /* 打印错误日志 */
#define LOG_FATAL   7    /* 打印致命日志 */

#define LOG_OFF     8    /* 关闭日志 */

    /* 模块定义 */
#define MODULE_COMMON           1    /* 公共模块 */
#define MODULE_SIPSTACK         2    /* SIP协议栈模块 */
#define MODULE_USER             3    /* 用户模块 */
#define MODULE_DEVICE           4    /* 设备模块 */
#define MODULE_ROUTE            5    /* 互联路由模块 */
#define MODULE_RECORD           6    /* 录像模块 */
#define MODULE_RESOURCE         7    /* 资源模块 */
#define MODULE_CRUISE_SRV       8    /* 巡航业务模块 */
#define MODULE_PLAN_SRV         9    /* 预案业务模块 */
#define MODULE_POLL_SRV         10   /* 轮巡业务模块 */

    /*----------------------------------------------*
     * 宏定义                                       *
     *----------------------------------------------*/
#define LOGFILE_DIR            ("/data/log/")
#define LOGFILE_SIP_MSG        ("cms_sipmsg")
#define LOGFILE_SIP_ERRORMSG   ("cms_siperror")
#define LOGFILE_RUN            ("cms_run")
#define LOGFILE_DEBUG          ("cms_debug")

#define __LOG4CMS_FUNC__ "X::X()"

    extern int cms_log_init(cms_log_t** log);
    extern void cs_log_free(cms_log_t* log);
    extern void DebugRunTrace(const char* fmt, ...);
    extern void DebugSIPMessage(int type, int iDirect, char* ipaddr, int port, char* msg);
    extern void DebugTrace(int iModule, int iLevel, const char* FILENAME, const char* FUNCTIONNAME, int CODELINE, const char* fmt, ...);
    extern void debug_log_free(debug_log_t* log);
    extern int debug_log_init(debug_log_t** log);
    extern void DeleteOutDateFile(int iYear, int iMonth, int iDay, int iHour);
    extern void EnSystemLog(int iType, int iLevel, const char* fmt, ...);
    extern void EnUserLog(int iType, int iLevel, user_info_t* pUserInfo, const char* fmt, ...);
    extern char* GetLogFilename(int type);
    extern void ListDirFile(char* path, int indent);
    extern void Log2FileFree();
    extern int Log2FileInit();
    extern void LogDebugTraceToFile(char* msg);
    extern void LogRunTraceToFile(char* msg);
    extern void LogSipErrMsgToFile(int type, char* msg, char* ipaddr, int port);
    extern void LogSipMsgToFile(int type, char* msg, char* ipaddr, int port);
    extern void run_log_free(run_log_t* log);
    extern int run_log_init(run_log_t** log);
    extern void scan_sip_msg_log2file_list();
    extern void scan_system_log2db_list(thread_proc_t* run);
    extern void scan_trace_log2file_list();
    extern void scan_user_log2db_list(thread_proc_t* run);
    extern void sip_errorlog_free(sip_errorlog_t* log);
    extern int  sip_errorlog_init(sip_errorlog_t** log);
    extern void sip_msglog_free(sip_msglog_t* log);
    extern int sip_msglog_init(sip_msglog_t** log);
    extern int sip_msg_log2file_add(int type, char* msg, char* ipaddr, int port);
    extern void sip_msg_log2file_free(sip_msg_log2file_t* sip_msg_log2file);
    extern int sip_msg_log2file_init(sip_msg_log2file_t** sip_msg_log2file);
    extern void sip_msg_log2file_list_free();
    extern int sip_msg_log2file_list_init();
    extern void SystemLog(int iType, int iLevel, const char* fmt, ...);
    extern void SystemLogToDB(DBOper* pLog2DB_dboper, system_log2db_t* system_log);
    extern int system_log2db_add(int type, int level, int iTime, char* msg);
    extern void system_log2db_free(system_log2db_t* system_log2db);
    extern int system_log2db_init(system_log2db_t** system_log2db);
    extern void system_log2db_list_clean();
    extern void system_log2db_list_free();
    extern int system_log2db_list_init();
    extern int trace_log2file_add(int type, char* msg);
    extern void trace_log2file_free(trace_log2file_t* trace_log2file);
    extern int trace_log2file_init(trace_log2file_t** trace_log2file);
    extern void trace_log2file_list_free();
    extern int trace_log2file_list_init();
    extern void UserLog(int iType, int iLevel, user_info_t* pUserInfo, const char* fmt, ...);
    extern void UserLogToDB(DBOper* pLog2DB_dboper, user_log2db_t* user_log);
    extern int user_log2db_add(int type, int level, user_info_t* pUserInfo, int iTime, char* msg);
    extern void user_log2db_free(user_log2db_t* user_log2db);
    extern int user_log2db_init(user_log2db_t** user_log2db);
    extern void user_log2db_list_clean();
    extern void user_log2db_list_free();
    extern int user_log2db_list_init();

#define DEBUG_TRACE(Module, level, fmt, args...) \
    DebugTrace(Module, level, __FILE__, __LOG4CMS_FUNC__, __LINE__, fmt, ##args)

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif /* __CSDBG_H__ */
