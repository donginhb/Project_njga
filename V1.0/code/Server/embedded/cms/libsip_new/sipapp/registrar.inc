/******************************************************************************

                  版权所有 (C), 2001-2013, 金智视讯技术有限公司

 ******************************************************************************
  文 件 名   : registrar.h
  版 本 号   : 初稿
  作    者   : yanghaifeng
  生成日期   : 2013年4月1日
  最近修改   :
  功能描述   : registrar.c 的头文件
  函数列表   :
  修改历史   :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 创建文件

******************************************************************************/
#ifndef __REGISTRAR_H__
#define __REGISTRAR_H__

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

/*----------------------------------------------*
 * 包含头文件                                   *
 *----------------------------------------------*/
#include <time.h>

#include <osip2/internal.h>
#include <osip2/osip.h>

#include <vector>
#include <string>
#include <map>
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
#define DEFAULT_REG_CLEARATE 600

    /* 服务端注册信息维护 */
    typedef struct _uas_reg_info_t
    {
        int iUsed;                         /* 是否启用  */
        char register_id[128 + 4];         /* 注册id  */

        char register_account[128 + 4];    /* 认证账户 */
        char register_password[128 + 4];   /* 认证密码 */

        /* dynamic routes (get from REGISTER) */
        time_t login_time;
        time_t last_active_time;
        osip_authorization_t* authorization;

        /* register info */
        int   cseqnum;
        char callid[128 + 4];
        int expires;
        int expires_count;
        double q_param;

        /* url info */
        char from_host[16];         /* get from REGISTER contact header */
        int from_port;              /* get from REGISTER contact header */
        osip_uri_t* contact_url;    /* Contact Url */

        char serverid[128 + 4];      /* 注册的服务器id */
        char serverip[16];          /* 注册的服务端ip地址 */
        int serverport;             /* 注册的服务端端口号*/

        osip_transaction_t* tr;
        osip_message_t* sip;
    } uas_reg_info_t;

    typedef std::map <int, uas_reg_info_t*> UAS_Reg_Info_MAP;
    typedef std::map <int, uas_reg_info_t*>::iterator UAS_Reg_Info_Iterator;

    /* 客户端注册信息维护 */
    typedef struct _uac_reg_info_t
    {
        int iUsed;                    /* 是否启用  */
        char register_id[128 + 4];    /* 注册id  */

        char proxy_id[128 + 4];       /* 注册服务器id  */

        char proxyip[16];             /* 服务器ip */
        int proxyport;                /* 服务器端口*/

        char register_account[128 + 4];       /* 认证账户 */
        char register_password[128 + 4];      /* 认证密码 */

        char register_callid_number[128 + 4]; /* callid for REGISTER */
        int  register_cseq_number;            /* always start registration with 1 */

        int link_type;
        
        int isReg;
        int isReging;
        int expires;
        int min_expires;

        char localip[16];             /* local ip */
        int localport;                /* local port */
    } uac_reg_info_t;

    typedef std::map <int, uac_reg_info_t*> UAC_Reg_Info_MAP;
    typedef std::map <int, uac_reg_info_t*>::iterator UAC_Reg_Info_Iterator;

    /*----------------------------------------------*
     * 宏定义                                       *
     *----------------------------------------------*/

    extern int  build_contact(osip_uri_t* contact_url, double q_param, int expires, char* dest, int dmax);
    extern int is_valid_uas_reg_info_index(int index);
    extern int  register_msg_proc(osip_transaction_t* tr, osip_message_t* reg);
    extern int  register_response_add_contacts(osip_message_t* response, uas_reg_info_t* pUasRegInfo);
    EV9000SIP_API int SIP_UASAnswerToRegister(int reg_info_index, int code, char* reasonphrase);
    EV9000SIP_API int SIP_UASAnswerToRegister4Auth(int reg_info_index,  char* realm);
    EV9000SIP_API int SIP_UASAnswerToRegister4RegExpire(int reg_info_index,  int iMinRegExpire);
    EV9000SIP_API osip_authorization_t* SIP_UASGetRegisterAuthorization(int reg_info_index);
    EV9000SIP_API int SIP_UASUpdateRegisterExpires(int reg_info_index);
    EV9000SIP_API int SIP_UASRemoveRegisterInfo(int reg_info_index);
    extern int uac_reginfo_add(char* user);
    extern int uac_reginfo_find(char* call_id);
    extern int uac_reginfo_find_by_server_and_local_info(char* service_id, char* server_ip, int server_port, char* local_id, char* local_ip, int local_port);
    extern int uac_reginfo_find_by_serverinfo(char* server_id, char* server_ip, int server_port);
    extern int uac_reginfo_find_by_server_host_and_port(char* server_ip, int server_port);
    extern void uac_reginfo_free(uac_reg_info_t* uac_reg_info);
    extern uac_reg_info_t* uac_reginfo_get(int index);
    uac_reg_info_t* uac_reginfo_get2(int index);
    extern int uac_reginfo_init(uac_reg_info_t** uac_reg_info);
    extern void uac_reginfo_list_free();
    extern int uac_reginfo_list_init();
    extern int uac_reginfo_list_lock();
    extern int uac_reginfo_list_unlock();
    extern void uac_reginfo_remove(int pos);
    extern int uas_reginfo_add(char* user, char* orig_host, int orig_port, char* map_host, int map_port);
    extern int uas_reginfo_find(char* user, char* orig_host, int orig_port);
    extern int uas_reginfo_find_by_caller_host_and_port(char* orig_host, int orig_port);
    extern int uas_reginfo_find_by_register_id(char* register_id);
    extern void uas_reginfo_free(uas_reg_info_t* uas_reg_info);
    extern uas_reg_info_t* uas_reginfo_get(int index);
    extern uas_reg_info_t* uas_reginfo_get2(int index);
    extern osip_uri_t* BuildTargetUrl(char* callee, char* remote_ip, int remote_port);
    extern int uas_reginfo_init(uas_reg_info_t** uas_reg_info);
    extern void uas_reginfo_list_free();
    extern int uas_reginfo_list_init();
    extern int uas_reginfo_list_lock();
    extern int uas_reginfo_list_unlock();
    extern void uas_reginfo_remove(int pos);
    extern int is_valid_uac_reg_info_index(int index);
    EV9000SIP_API void SIP_ShowUASRegisterInfo(int sock);
    EV9000SIP_API void SIP_ShowUACRegisterInfo(int sock);
    EV9000SIP_API void SIP_ReleaseAllUASRegisterInfo();
    EV9000SIP_API void SIP_ReleaseAllUACRegisterInfo();
    EV9000SIP_API char* SIP_GetUASServerIP(char* register_id, char* login_ip, int login_port);
    EV9000SIP_API int SIP_GetUASServerPort(char* register_id, char* login_ip, int login_port);
    EV9000SIP_API int SIP_GetUASRegExpires(char* register_id, char* login_ip, int login_port);
    EV9000SIP_API char* SIP_GetUASCallID(char* register_id, char* login_ip, int login_port);

    int debug_uas_reginfo_list_lock(const char* file, int line, const char* func);
    int debug_uas_reginfo_list_unlock(const char* file, int line, const char* func);
    int debug_uac_reginfo_list_lock(const char* file, int line, const char* func);
    int debug_uac_reginfo_list_unlock(const char* file, int line, const char* func);

#ifdef ENABLE_DEBUG_LOCK
#define UAS_SMUTEX_LOCK() debug_uas_reginfo_list_lock(__FILE__, __LINE__, __func__)
#define UAS_SMUTEX_UNLOCK() debug_uas_reginfo_list_unlock(__FILE__, __LINE__, __func__)
#define UAC_SMUTEX_LOCK() debug_uac_reginfo_list_lock(__FILE__, __LINE__, __func__)
#define UAC_SMUTEX_UNLOCK() debug_uac_reginfo_list_unlock(__FILE__, __LINE__, __func__)
#else
#define UAS_SMUTEX_LOCK() uas_reginfo_list_lock()
#define UAS_SMUTEX_UNLOCK() uas_reginfo_list_unlock()
#define UAC_SMUTEX_LOCK() uac_reginfo_list_lock()
#define UAC_SMUTEX_UNLOCK() uac_reginfo_list_unlock()
#endif

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif /* __REGISTRAR_H__ */
