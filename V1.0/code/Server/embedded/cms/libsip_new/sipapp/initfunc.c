/******************************************************************************

                  版权所有 (C), 2001-2013, 金智视讯技术有限公司

 ******************************************************************************
  文 件 名   : initfunc.c
  版 本 号   : 初稿
  作    者   : yanghaifeng
  生成日期   : 2013年4月1日
  最近修改   :
  功能描述   : 初始化
  函数列表   :
              SIP_Free
              SIP_Init
              sip_stack_free
              sip_stack_init
  修改历史   :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 创建文件

******************************************************************************/

/*----------------------------------------------*
 * 包含头文件                                   *
 *----------------------------------------------*/
#ifdef WIN32
#include <winsock2.h>
#include <windows.h>
#include <sys/types.h>
#else
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#endif

#include <osip2/internal.h>
#include <osip2/osip.h>
#include <osipparser2/osip_const.h>
#include <osip2/osip_fifo.h>
#include <osipparser2/osip_list.h>
#include <osipparser2/osip_uri.h>
#include <osipparser2/osip_port.h>

#include "gbltype.h"

#include "initfunc.inc"
#include "timerproc.inc"
#include "udp_tl.inc"
#include "sipua.inc"
#include "registrar.inc"

#include "callback.inc"
#include "sip_event.inc"
#include "csdbg.inc"
#include "sipmsg.inc"
#include "garbage.inc"

#include "include/libsip.h"

/*----------------------------------------------*
 * 外部变量说明                                 *
 *----------------------------------------------*/
extern garbage_t * g_recv_garbage;               /* sip: recv garbage */
extern garbage_t * g_recv_register_garbage;      /* sip: recv register garbage */
extern garbage_t * g_recv_msg_garbage;           /* sip: recv msg garbage */
extern garbage_t * g_send_garbage;               /* sip: send garbage */
extern garbage_t * g_send_msg_garbage;           /* sip: send msg garbage */


/*----------------------------------------------*
 * 外部函数原型说明                             *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 内部函数原型说明                             *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 全局变量                                     *
 *----------------------------------------------*/
osip_t* g_recv_cell = NULL;           /* 接收消息的SIP协议栈,非Message消息专用 */
osip_t* g_recv_register_cell = NULL;  /* 接收消息的SIP协议栈,Register消息专用 */
osip_t* g_recv_message_cell = NULL;   /* 接收消息的SIP协议栈,Message消息专用 */
osip_t* g_send_cell = NULL;           /* 发送消息的SIP协议栈,非Message消息专用 */
osip_t* g_send_message_cell = NULL;   /* 发送消息的SIP协议栈,Message消息专用 */

int g_SIPInitCount = 0;

/*----------------------------------------------*
 * 模块级变量                                   *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 常量定义                                     *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 宏定义                                       *
 *----------------------------------------------*/

/*****************************************************************************
 函 数 名  : sip_stack_cell_init_for_recv
 功能描述  : 接收消息协议栈初始化
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年12月11日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int sip_stack_cell_init_for_recv()
{
    int i = 0;

    i = osip_init(&g_recv_cell);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_FATAL, "sip_stack_cell_init_for_recv() exit---: Cell Init Error \r\n");
        return -1;
    }

    osip_set_kill_transaction_callback(g_recv_cell, OSIP_ICT_KILL_TRANSACTION, &cs_cb_kill_ict_transaction_for_rcv);
    osip_set_kill_transaction_callback(g_recv_cell, OSIP_IST_KILL_TRANSACTION, &cs_cb_kill_ist_transaction_for_rcv);
    osip_set_kill_transaction_callback(g_recv_cell, OSIP_NICT_KILL_TRANSACTION, &cs_cb_kill_nict_transaction_for_rcv);
    osip_set_kill_transaction_callback(g_recv_cell, OSIP_NIST_KILL_TRANSACTION, &cs_cb_kill_nist_transaction_for_rcv);

    i = sip_callback_init(g_recv_cell);

    if (i != 0)
    {
        osip_release(g_recv_cell);
        g_recv_cell = NULL;
        SIP_DEBUG_TRACE(LOG_FATAL, "sip_stack_cell_init_for_recv() exit---: SIP Callback Init Error \r\n");
        return -1;
    }

    if (create_garbage(&g_recv_garbage) != 0) //创建garbage
    {
        osip_release(g_recv_cell);
        g_recv_cell = NULL;
        SIP_DEBUG_TRACE(LOG_FATAL, "sip_stack_cell_init_for_recv() exit---: Create Grrbage Error \r\n");
        return -1;
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : sip_stack_cell_init_for_recv_register
 功能描述  : 接收Register消息初始化
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年12月11日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int sip_stack_cell_init_for_recv_register()
{
    int i = 0;

    i = osip_init(&g_recv_register_cell);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_FATAL, "sip_stack_cell_init_for_recv_register() exit---: Cell Init Error \r\n");
        return -1;
    }

    osip_set_kill_transaction_callback(g_recv_register_cell, OSIP_ICT_KILL_TRANSACTION, &cs_cb_kill_ict_transaction_for_rcv_register);
    osip_set_kill_transaction_callback(g_recv_register_cell, OSIP_IST_KILL_TRANSACTION, &cs_cb_kill_ist_transaction_for_rcv_register);
    osip_set_kill_transaction_callback(g_recv_register_cell, OSIP_NICT_KILL_TRANSACTION, &cs_cb_kill_nict_transaction_for_rcv_register);
    osip_set_kill_transaction_callback(g_recv_register_cell, OSIP_NIST_KILL_TRANSACTION, &cs_cb_kill_nist_transaction_for_rcv_register);

    i = sip_callback_init(g_recv_register_cell);

    if (i != 0)
    {
        osip_release(g_recv_register_cell);
        g_recv_register_cell = NULL;
        SIP_DEBUG_TRACE(LOG_FATAL, "sip_stack_cell_init_for_recv_register() exit---: SIP Callback Init Error \r\n");
        return -1;
    }

    if (create_garbage(&g_recv_register_garbage) != 0) //创建garbage
    {
        osip_release(g_recv_register_cell);
        g_recv_register_cell = NULL;
        SIP_DEBUG_TRACE(LOG_FATAL, "sip_stack_cell_init_for_recv_register() exit---: Create Grrbage Error \r\n");
        return -1;
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : sip_stack_cell_init_for_recv_msg
 功能描述  : 接收Message消息初始化
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年12月11日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int sip_stack_cell_init_for_recv_msg()
{
    int i = 0;

    i = osip_init(&g_recv_message_cell);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_FATAL, "sip_stack_cell_init_for_recv_msg() exit---: Cell Init Error \r\n");
        return -1;
    }

    osip_set_kill_transaction_callback(g_recv_message_cell, OSIP_ICT_KILL_TRANSACTION, &cs_cb_kill_ict_transaction_for_rcv_msg);
    osip_set_kill_transaction_callback(g_recv_message_cell, OSIP_IST_KILL_TRANSACTION, &cs_cb_kill_ist_transaction_for_rcv_msg);
    osip_set_kill_transaction_callback(g_recv_message_cell, OSIP_NICT_KILL_TRANSACTION, &cs_cb_kill_nict_transaction_for_rcv_msg);
    osip_set_kill_transaction_callback(g_recv_message_cell, OSIP_NIST_KILL_TRANSACTION, &cs_cb_kill_nist_transaction_for_rcv_msg);

    i = sip_callback_init(g_recv_message_cell);

    if (i != 0)
    {
        osip_release(g_recv_message_cell);
        g_recv_message_cell = NULL;
        SIP_DEBUG_TRACE(LOG_FATAL, "sip_stack_cell_init_for_recv_msg() exit---: SIP Callback Init Error \r\n");
        return -1;
    }

    if (create_garbage(&g_recv_msg_garbage) != 0) //创建garbage
    {
        osip_release(g_recv_message_cell);
        g_recv_message_cell = NULL;
        SIP_DEBUG_TRACE(LOG_FATAL, "sip_stack_cell_init_for_recv_msg() exit---: Create Grrbage Error \r\n");
        return -1;
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : sip_stack_cell_init_for_send
 功能描述  : 发送消息协议栈初始化
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年12月11日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int sip_stack_cell_init_for_send()
{
    int i = 0;

    i = osip_init(&g_send_cell);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_FATAL, "sip_stack_cell_init_for_send() exit---: Cell Init Error \r\n");
        return -1;
    }

    osip_set_kill_transaction_callback(g_send_cell, OSIP_ICT_KILL_TRANSACTION, &cs_cb_kill_ict_transaction_for_send);
    osip_set_kill_transaction_callback(g_send_cell, OSIP_IST_KILL_TRANSACTION, &cs_cb_kill_ist_transaction_for_send);
    osip_set_kill_transaction_callback(g_send_cell, OSIP_NICT_KILL_TRANSACTION, &cs_cb_kill_nict_transaction_for_send);
    osip_set_kill_transaction_callback(g_send_cell, OSIP_NIST_KILL_TRANSACTION, &cs_cb_kill_nist_transaction_for_send);

    i = sip_callback_init(g_send_cell);

    if (i != 0)
    {
        osip_release(g_send_cell);
        g_send_cell = NULL;
        SIP_DEBUG_TRACE(LOG_FATAL, "sip_stack_cell_init_for_send() exit---: SIP Callback Init Error \r\n");
        return -1;
    }

    if (create_garbage(&g_send_garbage) != 0) //创建garbage
    {
        osip_release(g_send_cell);
        g_send_cell = NULL;
        SIP_DEBUG_TRACE(LOG_FATAL, "sip_stack_cell_init_for_send() exit---: Create Grrbage Error \r\n");
        return -1;
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : sip_stack_cell_init_for_send_msg
 功能描述  : 发送Message消息协议栈初始化
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年12月11日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int sip_stack_cell_init_for_send_msg()
{
    int i = 0;

    i = osip_init(&g_send_message_cell);

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_FATAL, "sip_stack_cell_init_for_send_msg() exit---: Cell Init Error \r\n");
        return -1;
    }

    osip_set_kill_transaction_callback(g_send_message_cell, OSIP_ICT_KILL_TRANSACTION, &cs_cb_kill_ict_transaction_for_send_msg);
    osip_set_kill_transaction_callback(g_send_message_cell, OSIP_IST_KILL_TRANSACTION, &cs_cb_kill_ist_transaction_for_send_msg);
    osip_set_kill_transaction_callback(g_send_message_cell, OSIP_NICT_KILL_TRANSACTION, &cs_cb_kill_nict_transaction_for_send_msg);
    osip_set_kill_transaction_callback(g_send_message_cell, OSIP_NIST_KILL_TRANSACTION, &cs_cb_kill_nist_transaction_for_send_msg);

    i = sip_callback_init(g_send_message_cell);

    if (i != 0)
    {
        osip_release(g_send_message_cell);
        g_send_message_cell = NULL;
        SIP_DEBUG_TRACE(LOG_FATAL, "sip_stack_cell_init_for_send_msg() exit---: SIP Callback Init Error \r\n");
        return -1;
    }

    if (create_garbage(&g_send_msg_garbage) != 0) //创建garbage
    {
        osip_release(g_send_message_cell);
        g_send_message_cell = NULL;
        SIP_DEBUG_TRACE(LOG_FATAL, "sip_stack_cell_init_for_send_msg() exit---: Create Grrbage Error \r\n");
        return -1;
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : sip_stack_init
 功能描述  : SIP协议栈初始化
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int sip_stack_init()
{
    int i = 0;

#if 0 /* 协议栈日志开关 */
    TRACE_INITIALIZE(TRACE_LEVEL7, 0);
    TRACE_ENABLE_LEVEL(TRACE_LEVEL0);
    TRACE_ENABLE_LEVEL(TRACE_LEVEL1);
    TRACE_ENABLE_LEVEL(TRACE_LEVEL2);
    TRACE_ENABLE_LEVEL(TRACE_LEVEL3);
    TRACE_ENABLE_LEVEL(TRACE_LEVEL4);
    TRACE_ENABLE_LEVEL(TRACE_LEVEL5);
    TRACE_ENABLE_LEVEL(TRACE_LEVEL6);
    TRACE_ENABLE_LEVEL(TRACE_LEVEL7);
#endif

    i = sip_stack_cell_init_for_recv();

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_FATAL, "sip_stack_init() exit---: sip_stack_cell_init_for_recv Error \r\n");
        return -1;
    }

    i = sip_stack_cell_init_for_recv_register();

    if (i != 0)
    {
        sip_stack_cell_free_for_recv();
        SIP_DEBUG_TRACE(LOG_FATAL, "sip_stack_init() exit---: sip_stack_cell_init_for_recv_register Error \r\n");
        return -1;
    }

    i = sip_stack_cell_init_for_recv_msg();

    if (i != 0)
    {
        sip_stack_cell_free_for_recv();
        sip_stack_cell_free_for_recv_register();
        SIP_DEBUG_TRACE(LOG_FATAL, "sip_stack_init() exit---: sip_stack_cell_init_for_recv_msg Error \r\n");
        return -1;
    }

    i = sip_stack_cell_init_for_send();

    if (i != 0)
    {
        sip_stack_cell_free_for_recv();
        sip_stack_cell_free_for_recv_register();
        sip_stack_cell_free_for_recv_msg();
        SIP_DEBUG_TRACE(LOG_FATAL, "sip_stack_init() exit---: sip_stack_cell_init_for_send Error \r\n");
        return -1;
    }

    i = sip_stack_cell_init_for_send_msg();

    if (i != 0)
    {
        sip_stack_cell_free_for_recv();
        sip_stack_cell_free_for_recv_register();
        sip_stack_cell_free_for_recv_msg();
        sip_stack_cell_free_for_send();
        SIP_DEBUG_TRACE(LOG_FATAL, "sip_stack_init() exit---: sip_stack_cell_init_for_send_msg Error \r\n");
        return -1;
    }

    srand(time(NULL));
    init_random_number();

    sdp_local_config();

    return 0;
}

/*****************************************************************************
 函 数 名  : sip_stack_free
 功能描述  : SIP协议栈释放
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int sip_stack_free()
{
    sip_stack_cell_free_for_recv();

    sip_stack_cell_free_for_recv_register();

    sip_stack_cell_free_for_recv_msg();

    sip_stack_cell_free_for_send();

    sip_stack_cell_free_for_send_msg();

    sdp_config_free();

    return 0;
}

/*****************************************************************************
 函 数 名  : sip_stack_cell_free_for_recv
 功能描述  : 接收消息协议栈释放
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年12月11日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int sip_stack_cell_free_for_recv()
{
    if (g_recv_cell == NULL)
    {
        SIP_DEBUG_TRACE(LOG_FATAL, "sip_stack_cell_free_for_recv() exit---: Param Error \r\n");
        return -1;
    }

    ThrowAll2Garbage(g_recv_cell, g_recv_garbage);
    clean_garbage(g_recv_garbage);
    destroy_garbage(g_recv_garbage);
    g_recv_garbage = NULL;

    osip_release(g_recv_cell);
    g_recv_cell = NULL;

    return 0;
}

/*****************************************************************************
 函 数 名  : sip_stack_cell_free_for_recv_register
 功能描述  : 接收Register消息协议栈释放
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年12月11日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int sip_stack_cell_free_for_recv_register()
{
    if (g_recv_register_cell == NULL)
    {
        SIP_DEBUG_TRACE(LOG_FATAL, "sip_stack_cell_free_for_recv_register() exit---: Param Error \r\n");
        return -1;
    }

    ThrowAll2Garbage(g_recv_register_cell, g_recv_register_garbage);
    clean_garbage(g_recv_register_garbage);
    destroy_garbage(g_recv_register_garbage);
    g_recv_register_garbage = NULL;

    osip_release(g_recv_register_cell);
    g_recv_register_cell = NULL;

    return 0;
}

/*****************************************************************************
 函 数 名  : sip_stack_cell_free_for_recv_msg
 功能描述  : 接收Message消息协议栈释放
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年12月11日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int sip_stack_cell_free_for_recv_msg()
{
    if (g_recv_message_cell == NULL)
    {
        SIP_DEBUG_TRACE(LOG_FATAL, "sip_stack_cell_free_for_recv_msg() exit---: Param Error \r\n");
        return -1;
    }

    ThrowAll2Garbage(g_recv_message_cell, g_recv_msg_garbage);
    clean_garbage(g_recv_msg_garbage);
    destroy_garbage(g_recv_msg_garbage);
    g_recv_msg_garbage = NULL;

    osip_release(g_recv_message_cell);
    g_recv_message_cell = NULL;

    return 0;
}

/*****************************************************************************
 函 数 名  : sip_stack_cell_free_for_send
 功能描述  : 发送消息协议栈释放
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年12月11日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int sip_stack_cell_free_for_send()
{
    if (g_send_cell == NULL)
    {
        SIP_DEBUG_TRACE(LOG_FATAL, "sip_stack_cell_free_for_recv() exit---: Param Error \r\n");
        return -1;
    }

    ThrowAll2Garbage(g_send_cell, g_send_garbage);
    clean_garbage(g_send_garbage);
    destroy_garbage(g_send_garbage);
    g_send_garbage = NULL;

    osip_release(g_send_cell);
    g_send_cell = NULL;

    return 0;
}

/*****************************************************************************
 函 数 名  : sip_stack_cell_free_for_send_msg
 功能描述  : 发送Message消息协议栈释放
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年12月11日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int sip_stack_cell_free_for_send_msg()
{
    if (g_send_message_cell == NULL)
    {
        SIP_DEBUG_TRACE(LOG_FATAL, "sip_stack_cell_free_for_send_msg() exit---: Param Error \r\n");
        return -1;
    }

    ThrowAll2Garbage(g_send_message_cell, g_send_msg_garbage);
    clean_garbage(g_send_msg_garbage);
    destroy_garbage(g_send_msg_garbage);
    g_send_msg_garbage = NULL;

    osip_release(g_send_message_cell);
    g_send_message_cell = NULL;

    sdp_config_free();

    return 0;
}

#if DECS("对外接口")
/*****************************************************************************
 函 数 名  : SIP_Init
 功能描述  : SIP模块初始化函数
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int SIP_Init()
{
    int i = 0;

    g_SIPInitCount++;

    if (g_SIPInitCount > 1)
    {
        g_SIPInitCount--;
        return 0;
    }

    //定时器初始化
    i = ua_timer_list_init();

    if (i != 0)
    {
        SIP_DEBUG_TRACE(LOG_FATAL, "SIP_Init() exit---: UA Timer List Init Error \r\n");
        return EV9000_SIPSTACK_UA_TIMER_INIT_ERROR;
    }

    i = cs_sip_timer_creat();

    if (i != 0)
    {
        ua_timer_list_free();
        SIP_DEBUG_TRACE(LOG_FATAL, "SIP_Init() exit---: CS Sip Timer Creat Error \r\n");
        return EV9000_SIPSTACK_SIP_TIMER_INIT_ERROR;
    }

    //数据表初始化
    /* sip UAC注册信息初始化 */
    i = uac_reginfo_list_init();

    if (i != 0)
    {
        ua_timer_list_free();
        cs_sip_timer_destroy();
        SIP_DEBUG_TRACE(LOG_FATAL, "SIP_Init() exit---: UAC Register Info List Init Error \r\n");
        return EV9000_SIPSTACK_UAC_TIMER_INIT_ERROR;
    }

    /* sip UAS注册信息初始化 */
    i = uas_reginfo_list_init();

    if (i != 0)
    {
        uac_reginfo_list_free();
        ua_timer_list_free();
        cs_sip_timer_destroy();
        SIP_DEBUG_TRACE(LOG_FATAL, "SIP_Init() exit---: UAS Register Info List Init Error \r\n");
        return EV9000_SIPSTACK_UAS_TIMER_INIT_ERROR;
    }

    /* sip会话信息初始化 */
    i = ua_dialog_list_init();

    if (i != 0)
    {
        uac_reginfo_list_free();
        uas_reginfo_list_free();
        ua_timer_list_free();
        cs_sip_timer_destroy();
        SIP_DEBUG_TRACE(LOG_FATAL, "SIP_Init() exit---: UA Dialog List Init Error \r\n");
        return EV9000_SIPSTACK_UA_INIT_ERROR;
    }

    /* sip message消息队列初始化 */
    i = sip_message_list_init();

    if (i != 0)
    {
        ua_dialog_list_free();
        uac_reginfo_list_free();
        uas_reginfo_list_free();
        ua_timer_list_free();
        cs_sip_timer_destroy();
        SIP_DEBUG_TRACE(LOG_FATAL, "SIP_Init() exit---: SIP Message List Init Error \r\n");
        return EV9000_SIPSTACK_SIP_MESSAGE_INIT_ERROR;
    }

    /* 应用层钩子函数结构初始化 */
    i = app_callback_init();

    if (i != 0)
    {
        sip_message_list_free();
        ua_dialog_list_free();
        uac_reginfo_list_free();
        uas_reginfo_list_free();
        ua_timer_list_free();
        cs_sip_timer_destroy();
        SIP_DEBUG_TRACE(LOG_FATAL, "SIP_Init() exit---: APP Callback Init Error \r\n");
        return EV9000_SIPSTACK_CALL_BACK_INIT_ERROR;
    }

    /*协议栈初始化*/
    i = sip_stack_init();

    if (i != 0)
    {
        app_callback_free();
        sip_message_list_free();
        ua_dialog_list_free();
        uac_reginfo_list_free();
        uas_reginfo_list_free();
        ua_timer_list_free();
        cs_sip_timer_destroy();
        SIP_DEBUG_TRACE(LOG_FATAL, "SIP_Init() exit---: SIP Stack Init Error \r\n");
        return EV9000_SIPSTACK_SIPSTACK_INIT_ERROR;
    }

    /* 主接收线程创建*/
    i = sip_run_thread_start();

    if (i != 0)
    {
        sip_stack_free();
        app_callback_free();
        sip_message_list_free();
        ua_dialog_list_free();
        uac_reginfo_list_free();
        uas_reginfo_list_free();
        ua_timer_list_free();
        cs_sip_timer_destroy();
        SIP_DEBUG_TRACE(LOG_FATAL, "SIP_Init() exit---: SIP Run Thread Start Error \r\n");
        return EV9000_SIPSTACK_RUN_THREAD_INIT_ERROR;
    }

    /* sip接收线程队列创建*/
    i = udp_list_init();

    if (i != 0)
    {
        sip_run_thread_stop();
        sip_stack_free();
        app_callback_free();
        sip_message_list_free();
        ua_dialog_list_free();
        uac_reginfo_list_free();
        uas_reginfo_list_free();
        ua_timer_list_free();
        cs_sip_timer_destroy();
        SIP_DEBUG_TRACE(LOG_FATAL, "SIP_Init() exit---: UDP List Init Error \r\n");
        return EV9000_SIPSTACK_UDP_LIST_INIT_ERROR;
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : SIP_Free
 功能描述  : SIP模块释放函数
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void SIP_Free()
{
    g_SIPInitCount--;

    if (g_SIPInitCount < 0)
    {
        g_SIPInitCount++;
        return;
    }

    sip_run_thread_stop();
    SIP_DEBUG_TRACE(LOG_TRACE, "SIP_Free() sip_run_thread_stop OK\r\n");

    app_callback_free();
    SIP_DEBUG_TRACE(LOG_TRACE, "SIP_Free() app_callback_free OK\r\n");

    sip_message_list_free();
    SIP_DEBUG_TRACE(LOG_TRACE, "SIP_Free() sip_message_list_free OK\r\n");

    ua_dialog_list_free();
    SIP_DEBUG_TRACE(LOG_TRACE, "SIP_Free() ua_dialog_list_free OK\r\n");

    uac_reginfo_list_free();
    SIP_DEBUG_TRACE(LOG_TRACE, "SIP_Free() uac_reginfo_list_free OK\r\n");

    uas_reginfo_list_free();
    SIP_DEBUG_TRACE(LOG_TRACE, "SIP_Free() uas_reginfo_list_free OK\r\n");

    ua_timer_list_free();
    SIP_DEBUG_TRACE(LOG_TRACE, "SIP_Free() ua_timer_list_free OK\r\n");

    cs_sip_timer_destroy();
    SIP_DEBUG_TRACE(LOG_TRACE, "SIP_Free() cs_sip_timer_destroy OK\r\n");

    udp_list_free();
    SIP_DEBUG_TRACE(LOG_TRACE, "SIP_Free() udp_list_free OK\r\n");

    sip_stack_free();
    SIP_DEBUG_TRACE(LOG_TRACE, "SIP_Free() sip_stack_free OK\r\n");

    return;
}
#endif
