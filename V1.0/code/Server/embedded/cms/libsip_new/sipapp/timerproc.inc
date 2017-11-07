/******************************************************************************

                  版权所有 (C), 2001-2013, 金智视讯技术有限公司

 ******************************************************************************
  文 件 名   : timerproc.h
  版 本 号   : 初稿
  作    者   : yanghaifeng
  生成日期   : 2013年4月1日
  最近修改   :
  功能描述   : timerproc.c 的头文件
  函数列表   :
  修改历史   :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 创建文件

******************************************************************************/
#ifndef __TIMERPROC_H__
#define __TIMERPROC_H__

/*----------------------------------------------*
 * 包含头文件                                   *
 *----------------------------------------------*/
#include <vector>


using namespace std;

//modified by chenyu 130522
#ifdef WIN32

#ifdef EV9000SIP_EXPORTS
#define EV9000SIP_API    __declspec(dllexport)
#else
#define EV9000SIP_API    __declspec(dllimport)
#endif

#else
#define EV9000SIP_API    extern
#endif //end WIN32


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

    /*----------------------------------------------*
     * 常量定义                                     *
     *----------------------------------------------*/

    /*----------------------------------------------*
     * 宏定义                                       *
     *----------------------------------------------*/
    /* 服务端定时器*/
    typedef enum  _cs_timer_type
    {
        IN_REG_EXPIRE,  /* 接收注册超时时间 */
        OUT_REG_TIMER,   /* 向外注册超时时间 */
        UA_SESSION_EXPIRE,    /* UA 会话超时时间 */
        UAC_SEND_UPDATE,      /* UAC 定时发送UPDATE 消息 */
    }
    cs_timer_type;

    typedef struct  _cs_timer_id_t
    {
        cs_timer_type type;
        int          count;
        int          pos;
        void*        timer;
    } cs_timer_id_t;

    typedef struct _cs_timer_list_t
    {
        osip_list_t* timer_list;

        /* the lock */
#ifdef MULTI_THR
        osip_mutex_t*  lock;
#endif

        int     scan_1s_flag;
        int     scan_30s_flag;
        int     scan_10m_flag;
    } cs_timer_list_t;

    /* 客户端定时器*/
    typedef struct _ixt_t1   /* 2XX and ACK retransmit timer */
    {
        osip_message_t* msg2xx;    /* copy of string to retransmit */
        osip_message_t* ack;       /* useless for ist */
        char* dest;
        int port;
        int sock;

        time_t start;    /*for UAS:send the first 200 for INVITE time,
                             for UAC:received the first 200 for INVITE time*/
        int interval;    /*default: 64*T1 */
        time_t retransmit_start;  /* retransmit start time */
        int retransmit_interval;  /*   -1: not set the time, so not need retransmit */
    } ixt_t1;

    typedef struct _cit_t    /* CANCEL INVITE , but no final response for INVITE timer */
    {
        time_t start;       /*send the CANCEL time*/
        int interval;       /*default: 64*T1 */
    } cit_t;

    typedef struct _itt_t    /* invite timeout timer */
    {
        time_t start;       /* send INVITE time*/
        int interval;       /* INVITE expires value */
    } itt_t;

    typedef enum _ua_timer_type
    {
        UA_ACK2XX_RETRANSMIT,
        UA_INVITE_TIMEOUT,
        UA_CANCEL_TIMEOUT
    } ua_timer_type;

    typedef struct _ua_timer_id_t
    {
        ua_timer_type   type;
        int index;
        osip_transaction_t* tr;
        osip_message_t* sip;
    } ua_timer_id_t;

    typedef struct _ua_timer_t
    {
        ua_timer_id_t* id;
        void*        timer;   /*any type.*/
    } ua_timer_t;

    typedef struct _ua_timer_list_t
    {
        osip_list_t* tl;  /*list of ua_timer_t struct */
#ifdef MULTI_THR
        osip_mutex_t* lock;
#endif
    } ua_timer_list_t;

    extern int  cit_do(cit_t* cit);
    extern void cit_free(cit_t* cit);
    extern int cit_init(cit_t** cit, osip_transaction_t* tr);
    extern void cs_scan_timer_list(cs_timer_list_t* timer_link);
    extern void cs_scan_timer_event();
    extern void cs_scan_10m();
    extern void cs_scan_1s();
    extern void cs_scan_30s();
    extern cs_timer_id_t* cs_timer_find(cs_timer_type type, int pos, void* timer);
    extern void cs_timer_id_free(cs_timer_id_t* node);
    extern int  cs_timer_id_init(cs_timer_id_t** node);
    extern int  cs_timer_list_add(cs_timer_list_t* link, cs_timer_id_t* node);
    extern int cs_timer_list_clean(cs_timer_list_t* timer_link);
    extern void cs_timer_list_free(cs_timer_list_t* timer_link);
    extern int cs_timer_list_init(cs_timer_list_t** timer_link);
    extern int cs_timer_list_lock(cs_timer_list_t* timer_link);
    extern int cs_timer_list_unlock(cs_timer_list_t* timer_link);
    extern int  cs_timer_remove(cs_timer_type type, int pos, void* timer);
    extern int  cs_timer_use(cs_timer_type type, int pos, void* timer);
#ifndef WIN32   //modified by chenyu 130522
    extern void cs_time_count();
#endif
    extern int itt_do(itt_t* itt);
    extern void itt_free(itt_t* itt);
    extern int itt_init(itt_t** itt, int expires);
    extern int ixt_do(ixt_t1* ixt);
    extern void ixt_free(ixt_t1* ixt);
    extern int ixt_init(ixt_t1** ixt, osip_message_t* sip, int out_socket);
    extern int ixt_init_as_2xx(ixt_t1** ixt, osip_message_t* sip, int out_socket);
    extern int ixt_init_as_ack(ixt_t1** ixt, osip_message_t* sip, int out_socket);
    extern int cs_sip_timer_creat();
    extern void cs_sip_timer_destroy();
    extern void ua_scan_timer_list();
    extern ua_timer_t* ua_timer_find(ua_timer_type type, int index, osip_transaction_t* tr, osip_message_t* sip);
    extern int ua_timer_find_pos(ua_timer_id_t* id);
    extern void ua_timer_free(ua_timer_t* timer);
    extern void ua_timer_id_free(ua_timer_id_t* id);
    extern int ua_timer_id_init(ua_timer_id_t** id, ua_timer_type type);
    extern int ua_timer_id_match(ua_timer_id_t* id1, ua_timer_id_t* id2);
    extern int  ua_timer_init(ua_timer_t** timer);
    extern void ua_timer_list_free();
    extern int  ua_timer_list_init();
    extern int ua_timer_list_lock();
    extern int ua_timer_list_unlock();
    extern int ua_timer_remove(ua_timer_type type, int index, osip_transaction_t* tr, osip_message_t* sip);
    extern ua_timer_t* ua_timer_use(ua_timer_type type, int index, osip_transaction_t* tr, osip_message_t* sip);
    EV9000SIP_API void SIP_1sTimerNotify();

    extern int uas_register_expire_proc(int pos);
    extern int uac_register_expire_proc(int pos);
    extern int ua_ssesion_expire_proc(int pos);
    extern int ua_send_update_proc(int pos);

#ifdef ENABLE_DEBUG_LOCK
#define CS_TIMER_SMUTEX_LOCK(s) osip_debug_mutex_lock(s, __FILE__, __LINE__, __func__)
#define CS_TIMER_SMUTEX_UNLOCK(s) osip_debug_mutex_unlock(s, __FILE__, __LINE__, __func__)

#define UA_TIMER_SMUTEX_LOCK() osip_debug_mutex_lock((struct osip_mutex*)g_UaTimerList->lock, __FILE__, __LINE__, __func__)
#define UA_TIMER_SMUTEX_UNLOCK() osip_debug_mutex_unlock((struct osip_mutex*)g_UaTimerList->lock, __FILE__, __LINE__, __func__)
#else
#define CS_TIMER_SMUTEX_LOCK(s) osip_mutex_lock(s)
#define CS_TIMER_SMUTEX_UNLOCK(s) osip_mutex_unlock(s)

#define UA_TIMER_SMUTEX_LOCK() osip_mutex_lock((struct osip_mutex*)g_UaTimerList->lock)
#define UA_TIMER_SMUTEX_UNLOCK() osip_mutex_unlock((struct osip_mutex*)g_UaTimerList->lock)
#endif

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif /* __TIMERPROC_H__ */
