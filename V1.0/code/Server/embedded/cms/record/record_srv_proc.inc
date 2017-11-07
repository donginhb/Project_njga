#ifndef __RECORD_SRV_PROC_H__
#define __RECORD_SRV_PROC_H__


/*----------------------------------------------*
 * 包含头文件                                   *
 *----------------------------------------------*/
#include <sys/types.h>
#ifdef WIN32
#include <winsock2.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#endif

#include "libsip.h"
#include "libxml/Packet.h"

#include "common/gbldef.inc"
#include "common/DBOper.h"

#include <vector>
#include <string>
#include <queue>

#include "record/record_info_mgn.inc"
#include "resource/resource_info_mgn.inc"

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

    /*----------------------------------------------*
     * 常量定义                                     *
     *----------------------------------------------*/

    /*----------------------------------------------*
     * 宏定义                                       *
     *----------------------------------------------*/
    /* 录像业务消息结构体 */
    typedef struct _record_srv_msg_t
    {
        msg_type_t msg_type;                                /* 消息类型 */
        char caller_id[MAX_ID_LEN + 4];                     /* 主叫用户统一编号 */
        char callee_id[MAX_ID_LEN + 4];                     /* 被叫用户统一编号 */
        int response_code;                                  /* 回应消息码,只有回应消息有 */
        char reasonphrase[MAX_128CHAR_STRING_LEN + 4];      /* 回应的错误信息，只有回应消息有 */
        int ua_dialog_index;                                /* 对应dialog 索引，只有会话类消息有 */
        char msg_body[MAX_MSG_BODY_STRING_LEN + 4];         /* SIP 消息携带的消息体 */
        int msg_body_len;                                   /* SIP 消息携带的消息体长度 */
        int cr_pos;                                         /* 对应的呼叫资源索引信息 */
    } record_srv_msg_t;

    /* 录像业务消息队列 */
    typedef std::deque <record_srv_msg_t*> record_srv_msg_queue;

    extern int record_bye_msg_proc(int cr_pos, int ua_dialog_index);
    extern int record_bye_response_msg_proc(int cr_pos, int ua_dialog_index, int response_code);
    extern int record_invite_response_msg_proc(int cr_pos, int ua_dialog_index, int response_code, char* msg_body, int msg_body_len);
    extern int record_srv_msg_add(msg_type_t msg_type, char* caller_id, char* callee_id, int response_code, char* reasonphrase, int ua_dialog_index, char* msg_body, int msg_body_len, int cr_pos);
    extern void record_srv_msg_free(record_srv_msg_t* record_srv_msg);
    extern int record_srv_msg_init(record_srv_msg_t** record_srv_msg);
    extern void record_srv_msg_list_clean();
    extern void record_srv_msg_list_free();
    extern int record_srv_msg_list_init();
    extern int record_srv_msg_proc(record_srv_msg_t* pRecordSrvMsg);
    extern void scan_record_srv_msg_list();
    extern int StartDeviceRecord(record_info_t* pProcRecordInfo);
    extern int StopAllProcRecordTask();
    extern int StopAllRecordTask(int sock);
    extern int StopDeviceRecord(int record_cr_index);
    extern int StopRecordTask(int sock, int record_cr_index);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif /* __UDP_TL_H__ */
