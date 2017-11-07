#ifndef __PLAN_SRV_PROC_H__
#define __PLAN_SRV_PROC_H__


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

#include "user/user_info_mgn.inc"

#include <vector>
#include <string>
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

    /* 预案业务结构体 */
    typedef struct _plan_srv__t
    {
        unsigned int plan_id;                           /* 预案id */
        char plan_name[MAX_128CHAR_STRING_LEN + 4];     /* 预案名称 */
        int iScheduledRun;                              /* 是否定时执行 */
        int start_time;                                 /* 开始时间 */
        int status;                                     /* 状态: 0:没有执行，1:已经执行*/
        int del_mark;                                   /* 删除标识 */
    } plan_srv_t;

    /* 预案业务队列 */
    typedef struct _plan_srv_list_t
    {
        osip_list_t* pPlanSrvList;
#ifdef MULTI_THR
        osip_mutex_t* lock;
#endif
    } plan_srv_list_t;

    /* 待处理预案业务队列 */
    typedef std::deque <plan_srv_t*> needtoproc_plansrv_queue;

    /* 待处理预案业务动作队列 */
    typedef std::deque <EV9000_PlanActionConfig*> needtoproc_planaction_queue;

    extern int check_db_data_to_plan_srv_list_for_start(DBOper* pPlan_Srv_dboper);
    extern void check_PlanSrvConfig_need_to_reload_begin(DBOper* pDboper);
    extern void check_PlanSrvConfig_need_to_reload_end();
    extern int debug_plan_srv_list_lock(const char* file, int line, const char* func);
    extern int debug_plan_srv_list_unlock(const char* file, int line, const char* func);
    extern void delete_plan_srv_data();
    extern int get_user_index_from_user_plan_config(char* pcPlanID, vector<unsigned int>& UserIndexVector, DBOper* pDBOper);
    extern int PlanSrvConfig_db_refresh_proc();
    extern void plan_action_free(EV9000_PlanActionConfig* plan_action);
    extern int plan_action_init(EV9000_PlanActionConfig** plan_action);
    extern int plan_srv_add(plan_srv_t* pPlanSrv);
    extern int plan_srv_find(unsigned int id);
    extern void plan_srv_free(plan_srv_t* plan_srv);
    extern plan_srv_t* plan_srv_get(int pos);
    extern int plan_srv_init(plan_srv_t** plan_srv);
    extern void plan_srv_list_free();
    extern int plan_srv_list_init();
    extern int plan_srv_list_lock();
    extern int plan_srv_list_unlock();
    extern int plan_srv_remove(int pos);
    extern void scan_plan_srv_list(DBOper* pPlan_Srv_dboper);
    extern int SendNotifyExecutePlanActionToOnlineUser(user_info_t* pUserInfo, char* pcPlanID, char* pcPlanName, DBOper* pPlan_Srv_dboper);
    extern int set_plan_srv_list_del_mark(int del_mark);
    extern int StartPlanActionByPlanID(user_info_t* pUserInfo, char* pcPlanID, char* pcPlanName, DBOper* pPlan_Srv_dboper);
    extern int StartPlanActionByPlanIDForAlarm(char* pcPlanID, char* pcPlanName, alarm_msg_t* pAlarmMsg, DBOper* pPlan_Srv_dboper);
    extern int StartPlanActionByPlanIDForAutoEndAlarm(char* pcPlanID, char* pcPlanName, alarm_msg_t* pAlarmMsg, DBOper* pPlan_Srv_dboper);
    extern int StartPlanActionByPlanIDForFault(char* pcPlanID, char* pcPlanName, fault_msg_t* pFaultMsg, DBOper* pPlan_Srv_dboper);

#ifdef ENABLE_DEBUG_LOCK
#define PLAN_SMUTEX_LOCK() debug_plan_srv_list_lock(__FILE__, __LINE__, __func__)
#define PLAN_SMUTEX_UNLOCK() debug_plan_srv_list_unlock(__FILE__, __LINE__, __func__)
#else
#define PLAN_SMUTEX_LOCK() plan_srv_list_lock()
#define PLAN_SMUTEX_UNLOCK() plan_srv_list_unlock()
#endif

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif /* __UDP_TL_H__ */
