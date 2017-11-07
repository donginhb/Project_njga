#ifndef __ALARM_PROC_H__
#define __ALARM_PROC_H__


/*----------------------------------------------*
 * ����ͷ�ļ�                                   *
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
#include "common/common_thread_proc.inc"

#include <vector>
#include <string>
using namespace std;

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

    /*----------------------------------------------*
     * �ⲿ����˵��                                 *
     *----------------------------------------------*/

    /*----------------------------------------------*
     * �ⲿ����ԭ��˵��                             *
     *----------------------------------------------*/

    /*----------------------------------------------*
     * �ڲ�����ԭ��˵��                             *
     *----------------------------------------------*/

    /*----------------------------------------------*
     * ȫ�ֱ���                                     *
     *----------------------------------------------*/

    /*----------------------------------------------*
     * ģ�鼶����                                   *
     *----------------------------------------------*/

    /*----------------------------------------------*
     * ��������                                     *
     *----------------------------------------------*/

    /*----------------------------------------------*
     * �궨��                                       *
     *----------------------------------------------*/

    /* ������Ϣ�ṹ�� */
    typedef struct _fault_msg_t
    {
        unsigned int uLogicDeviceIndex;                   /* �߼��豸����*/
        char strLogicDeviceID[MAX_ID_LEN + 4];            /* �߼��豸ͳһ��� */
        unsigned int uFaultType;                          /* �������ͣ�������������ʹ�� */
        char strPriority[32];                             /* ���ϼ��� 1Ϊһ�����飬2Ϊ�������飬3Ϊ�������飬4Ϊ�ļ�����    */
        char strMethod[32];                               /* ���Ϸ�ʽ ȡֵ1Ϊ�绰������2Ϊ�豸������3Ϊ���ű�����4ΪGPS������5Ϊ��Ƶ������6Ϊ�豸���ϱ�����7�������� */
        unsigned int uLogTime;                            /* ����ʱ�� */
        char strDeseription[512 + 4];                     /* ���� */
    } fault_msg_t;

    /* ������Ϣ���� */
    typedef std::deque <fault_msg_t*> fault_msg_queue;

    /* ������Ϣ�ṹ�� */
    typedef struct _alarm_msg_t
    {
        char strSN[32];                                   /* ��Ϣ���к� */
        unsigned int uDeviceIndex;                        /* �豸����*/
        char strDeviceID[MAX_ID_LEN + 4];                 /* �豸ͳһ��� */
        char strPriority[32];                             /* �������� 1Ϊһ�����飬2Ϊ�������飬3Ϊ�������飬4Ϊ�ļ����� */
        char strMethod[32];                               /* ������ʽ ȡֵ1Ϊ�绰������2Ϊ�豸������3Ϊ���ű�����4ΪGPS������5Ϊ��Ƶ������6Ϊ�豸���ϱ�����7�������� */
        char strAlarmStartTime[32];                       /* ������ʼʱ�� */
        char strAlarmEndTime[32];                         /* ��������ʱ�� */
        int iAlarmMsgType;                                /* ������Ϣ����:0,��ʼ������1,�������� */
        int iAlarmLengthOfTime;                           /* Ĭ�ϵı�������ʱ�� */
        char strDeseription[512 + 4];                     /* �������� */
        int iAlarmDeviceType;                             /* �����豸���� */
    } alarm_msg_t;

    /* ������Ϣ���� */
    typedef std::deque <alarm_msg_t*> alarm_msg_queue;

    /* ����ʱ�̲��Խṹ�� */
    typedef struct _alarm_time_sched_t
    {
        char UUID[36];                     /* ���ݿ����� */
        int iBeginTime;                    /* ��ʼʱ�� */
        int iEndTime;                      /* ����ʱ�� */
        int del_mark;                      /* ɾ����ʶ */
    } alarm_time_sched_t;

    /* ����ʱ����Զ��� */
    typedef struct _alarm_deployment_t
    {
        unsigned int LogicDeviceIndex;     /* �߼��豸���� */
        int del_mark;                      /* ɾ����ʶ */

        osip_list_t* pDayOfWeekTimeList;   /* ÿ��ʱ���б� */

    } alarm_deployment_t;

    /* �澯��ʱ������*/
    typedef struct _alarm_deployment_list_t
    {
        osip_list_t* pAlarmDeploymentList;

        /* the lock */
#ifdef MULTI_THR
        osip_mutex_t*  lock;
#endif

    } alarm_deployment_list_t;

    /* �澯����*/
    typedef enum  _alarm_timer_type
    {
        DEVICE_OFFLINE,   /* �豸���� */
        DB_ERROR,         /* ���ݿ����*/
    }
    alarm_timer_type;

    /* �澯��ʱ���ṹ */
    typedef struct  _alarm_timer_t
    {
        alarm_timer_type type;
        int          iAlarmTimers;           /* �������� */
        int          iAlarmInterval;         /* ������� */
        int          iAlarmTimersCount;      /* ������������ */
        int          iAlarmIntervalCount;    /* ����������� */
        int          count;
        int          pos;
    } alarm_timer_t;

    /* �澯��ʱ������*/
    typedef struct _alarm_timer_list_t
    {
        osip_list_t* timer_list;

        /* the lock */
#ifdef MULTI_THR
        osip_mutex_t*  lock;
#endif

    } alarm_timer_list_t;

    /* ��ʱ�澯�ṹ */
    typedef struct  _alarm_duration_t
    {
        unsigned int uDeviceIndex;                        /* �豸����*/
        int iPriority;                                    /* �������� 1Ϊһ�����飬2Ϊ�������飬3Ϊ�������飬4Ϊ�ļ�����    */
        int iMethod;                                      /* ������ʽ ȡֵ1Ϊ�绰������2Ϊ�豸������3Ϊ���ű�����4ΪGPS������5Ϊ��Ƶ������6Ϊ�豸���ϱ�����7�������� */
        char strDeseription[512 + 4];                     /* �������� */
        unsigned int uStartTime;                          /* ������ʼʱ�� */
        int iDurationTime;                                /* ����ʱ�� */
        int iDurationTimeCount;                           /* ���� */
        int iFlag;                                        /* ɾ����ʶ */
    } alarm_duration_t;

    /* ��ʱ�澯����*/
    typedef struct _alarm_duration_list_t
    {
        osip_list_t* duration_list;

        /* the lock */
#ifdef MULTI_THR
        osip_mutex_t*  lock;
#endif

    } alarm_duration_list_t;

    /* ��������ʱ�澯���� */
    typedef std::deque <alarm_duration_t*> needtoproc_alarmduration_queue;

    extern int AlarmDeploymentConfig_db_refresh_proc();
    extern int alarm_deployment_add(unsigned int uLogicDeviceIndex);
    extern void alarm_deployment_free(alarm_deployment_t* alarm_deployment);
    extern alarm_deployment_t* alarm_deployment_get(unsigned int uLogicDeviceIndex);
    extern int alarm_deployment_init(alarm_deployment_t** alarm_deployment);
    extern void alarm_deployment_list_free();
    extern int alarm_deployment_list_init();
    extern int alarm_deployment_list_lock();
    extern int alarm_deployment_list_unlock();
    extern alarm_duration_t* alarm_duration_find(unsigned int uDeviceIndex, int iPriority, int iMethod, char* strDeseription);
    extern void alarm_duration_free(alarm_duration_t* node);
    extern int alarm_duration_init(alarm_duration_t** node);
    extern int alarm_duration_list_clean();
    extern void alarm_duration_list_free();
    extern int alarm_duration_list_init();
    extern int alarm_duration_use(unsigned int uDeviceIndex, int iPriority, int iMethod, char* strDeseription, unsigned int uStartTime, int iAlarmDuration);
    extern int alarm_endtime_proc(unsigned int uDeviceIndex, unsigned int uAlarmEndTime, DBOper* pDboper);
    extern int alarm_msg_add(char* strSN, unsigned int uDeviceIndex, char* strDeviceID, char* strPriority, char* strMethod, char* strAlarmStartTime, char* strAlarmEndTime, int iAlarmMsgType, int iAlarmLengthOfTime, char* strDeseription, int iAlarmDeviceType);
    extern void alarm_msg_free(alarm_msg_t* alarm_msg);
    extern int alarm_msg_init(alarm_msg_t** alarm_msg);
    extern void alarm_msg_list_clean();
    extern void alarm_msg_list_free();
    extern int alarm_msg_list_init();
    extern int alarm_msg_proc(alarm_msg_t* pAlarmMsg, DBOper* pDboper, DBOper* pDbLogoper);
    extern alarm_timer_t* alarm_timer_find(alarm_timer_type type, int pos);
    extern void alarm_timer_free(alarm_timer_t* node);
    extern int alarm_timer_init(alarm_timer_t** node);
    extern int alarm_timer_list_add(alarm_timer_t* node);
    extern int alarm_timer_list_clean();
    extern void alarm_timer_list_free();
    extern int alarm_timer_list_init();
    extern int alarm_timer_remove(alarm_timer_type type, int pos);
    extern int alarm_timer_use(alarm_timer_type type, int pos, int iAlarmTimers, int iAlarmInterval);
    extern int alarm_time_sched_add(osip_list_t* pDayOfWeekTimeList, char* pUUID, int iBeginTime, int iEndTime);
    extern void alarm_time_sched_free(alarm_time_sched_t* alarm_time_sched);
    extern alarm_time_sched_t* alarm_time_sched_get(osip_list_t* pDayOfWeekTimeList, char* pUUID);
    extern int alarm_time_sched_init(alarm_time_sched_t** alarm_time_sched);
    extern void check_AlarmDeploymentConfig_need_to_reload_begin(DBOper* pDboper);
    extern void check_AlarmDeploymentConfig_need_to_reload_end();
    extern int check_alarm_deployment_from_db_to_list(DBOper* pDBOper);
    extern int check_alarm_time_sched_config_from_db_to_list(alarm_deployment_t* pAlarmDeployment, DBOper* pDBOper);
    extern int debug_alarm_deployment_list_lock(const char* file, int line, const char* func);
    extern int debug_alarm_deployment_list_unlock(const char* file, int line, const char* func);
    extern int delete_alarm_deployment_from_list_by_mark();
    extern int EnSystemFaultAlarm(unsigned int uLogicDeviceIndex, char* strLogicDeviceID, unsigned int uFaultType, char* strPriority, char* strMethod, const char* fmt, ...);
    extern int fault_msg_add(unsigned int uLogicDeviceIndex, char* strLogicDeviceID, unsigned int uFaultType, char* strPriority, char* strMethod, char* strDeseription);
    extern void fault_msg_free(fault_msg_t* fault_msg);
    extern int fault_msg_init(fault_msg_t** fault_msg);
    extern int fault_msg_linkage_proc(fault_msg_t* pFaultMsg, DBOper* pDboper);
    extern void fault_msg_list_clean();
    extern void fault_msg_list_free();
    extern int fault_msg_list_init();
    extern int fault_msg_proc(fault_msg_t* pFaultMsg, DBOper* pDboper, DBOper* pLogDboper);
    extern int FindHasPlanLinkageForAlarmProc(alarm_msg_t* pAlarmMsg, int iPlanActionType, DBOper* pDboper);
    extern int FindHasPlanLinkageForAutoEndAlarmProc(unsigned int uDeviceIndex, DBOper* pDboper);
    extern int FindHasPlanLinkageForFaultProc(fault_msg_t* pFaultMsg, int iPlanActionType, DBOper* pDboper);
    extern int find_device_is_in_alarm_deployment(unsigned int uDeviceIndex, int iAlarmTime);
    extern int get_alarm_deployment_logic_device_index_from_db(vector<unsigned int>& LogicDeviceIndexVector, DBOper* pDBOper);
    extern void scan_alarm_deployment_list();
    extern void scan_alarm_duration_list(DBOper* pAlarm_dboper);
    extern void scan_alarm_msg_list(thread_proc_t* run);
    extern void scan_alarm_timer_list(DBOper* pAlarm_dboper);
    extern void scan_fault_msg_list(thread_proc_t* run);
    extern int send_alarm_to_user_proc(CPacket& inPacket);
    extern int send_alarm_to_user_proc2(unsigned int device_index, CPacket& inPacket, DBOper* pDboper);
    extern int set_alarm_deployment_list_del_mark(int del_mark);
    extern void show_alarm_time_deployment(int sock);
    extern int StartPlanActionForAlarm(int iPlanID, alarm_msg_t* pAlarmMsg, DBOper* pDboper);
    extern int StartPlanActionForAutoEndAlarm(int iPlanID, DBOper* pDboper);
    extern int StartPlanActionForFault(int iPlanID, fault_msg_t* pFaultMsg, DBOper* pDboper);
    extern int SystemFaultAlarm(unsigned int uLogicDeviceIndex, char* strLogicDeviceID, unsigned int uFaultType, char* strPriority, char* strMethod, const char* fmt, ...);
    extern int write_alarm_to_db_proc(alarm_msg_t* pAlarmMsg, unsigned int uAlarmStartTime, unsigned int uAlarmEndTime, DBOper* pDboper);
    extern int write_fault_to_db_proc(fault_msg_t* pFaultMsg, DBOper* pDboper);

#ifdef ENABLE_DEBUG_LOCK
#define ALARM_DEPLOYMENT_SMUTEX_LOCK() debug_alarm_deployment_list_lock(__FILE__, __LINE__, __func__)
#define ALARM_DEPLOYMENT_SMUTEX_UNLOCK() debug_alarm_deployment_list_unlock(__FILE__, __LINE__, __func__)
#else

#define ALARM_DEPLOYMENT_SMUTEX_LOCK() alarm_deployment_list_lock()
#define ALARM_DEPLOYMENT_SMUTEX_UNLOCK() alarm_deployment_list_unlock()
#endif

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif /* __UDP_TL_H__ */