/******************************************************************************

                  版权所有 (C), 2001-2013, 金智视讯技术有限公司

 ******************************************************************************
  文 件 名   : callback_proc.h
  版 本 号   : 初稿
  作    者   : yanghaifeng
  生成日期   : 2013年4月10日
  最近修改   :
  功能描述   : callback_proc.c 的头文件
  函数列表   :
  修改历史   :
  1.日    期   : 2013年4月10日
    作    者   : yanghaifeng
    修改内容   : 创建文件

******************************************************************************/

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

#ifndef __CALLBACK_PROC_H__
#define __CALLBACK_PROC_H__

#include "libsip.h"
#include "libxml/Packet.h"

#include "common/gbldef.inc"
#include "common/DBOper.h"

#include <vector>
#include <string>
using namespace std;


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

    extern int ack_received_proc(char* caller_id, char* callee_id, char* call_id, int dialog_index, int user_data);
    extern int bye_received_proc(char* caller_id, char* callee_id, char* call_id, int dialog_index, int user_data);
    extern int bye_response_received_proc(char* caller_id, char* callee_id, char* call_id, int dialog_index, int status_code, int user_data);
    extern int cancel_received_proc(char* caller_id, char* callee_id, char* call_id, int dialog_index, int user_data);
    extern void dbg_printf_proc(int iLevel, const char* FILENAME, const char* FUNCTIONNAME, int CODELINE, const char* fmt);
    extern int info_received_proc(char* caller_id, char* caller_ip, int caller_port, char* callee_id, char* call_id, int dialog_index, char* msg_body, int msg_len, int user_data);
    extern int info_response_received_proc(char* caller_id, char* callee_id, char* call_id, int status_code, int user_data);
    extern int invite_received_proc(char* caller_id, char* callee_id, char* call_id, int dialog_index, char* msg_body, int msg_len, int user_data);
    extern int invite_response_received_proc(char* caller_id, char* callee_id, char* call_id, int dialog_index, int status_code, char* reasonphrase, char* msg_body, int msg_len, int user_data);
    extern int message_get_service_id_proc(char* caller_id, char* caller_ip, int caller_port, char* callee_id, char* callee_ip, int callee_port, char* msg_body, int msg_body_len);
    extern int message_received_proc(char* caller_id, char* caller_ip, int caller_port, char* callee_id, char* callee_ip, int callee_port, char* call_id, int dialog_index, char* msg_body, int msg_len, int user_data);
    extern int message_response_received_proc(char* caller_id, char* callee_id, char* call_id, int status_code, int user_data);
    extern int notify_received_proc(char* caller_id, char* caller_ip, int caller_port, char* callee_id, char* callee_ip, int callee_port, char* call_id, char* msg_body, int msg_len, int user_data);
    extern int notify_response_received_proc(char* caller_id, char* callee_id, char* call_id, int status_code, int user_data);
    extern void SetCallbackProcFunc();
    extern void sip_message_trace_proc(int type, int iDirect, char* ipaddr, int port, char* msg);
    extern int subscribe_received_proc(char* caller_id, char* caller_ip, int caller_port, char* callee_id, char* callee_ip, int callee_port, char* call_id, char* event_type, char* id_param, int subscribe_expires, char* msg_body, int msg_len, int user_data);
    extern int subscribe_response_received_proc(char* caller_id, char* callee_id, char* call_id, int expires, int status_code, int user_data);
    extern int subscribe_within_dialog_received_proc(char* caller_id, char* caller_ip, int caller_port, char* callee_id, char* call_id, int dialog_index, int subscribe_expires, char* msg_body, int msg_len);
    extern int subscribe_within_dialog_response_received_proc(char* caller_id, char* callee_id, char* call_id, int dialog_index, int expires, int status_code);
    extern int uac_register_response_received_proc(int reg_info_index, int iExpires, int status_code, char* reasonphrase, unsigned int iTime, int user_data);
    extern int uas_register_received_proc(char* proxy_id, char* register_id, char* login_ip, int login_port, char* register_name, int reg_info_index, int expires, int link_type);
    extern int uas_register_received_timeout_proc(char* proxy_id, char* register_id, char* login_ip, int login_port, int reg_info_index);
    extern int ua_session_expires_proc(int dialog_index);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif /* __CALLBACK_PROC_H__ */
