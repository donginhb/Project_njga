/******************************************************************************

                  版权所有 (C), 2001-2013, 金智视讯技术有限公司

 ******************************************************************************
  文 件 名   : systeminfo_proc.h
  版 本 号   : 初稿
  作    者   : yanghaifeng
  生成日期   : 2013年4月16日
  最近修改   :
  功能描述   : systeminfo_proc.c 的头文件
  函数列表   :
  修改历史   :
  1.日    期   : 2014年4月16日
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

#ifndef __SYSINFO_PROC_H__
#define __SYSINFO_PROC_H__

#include "common/gbldef.inc"


#include "libxml/Packet.h"

#include <vector>
#include <string>
using namespace std;

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

#define VMRSS_LINE 15//VMRSS所在行
#define PROCESS_ITEM 14//进程CPU时间开始的项数

    //cpu 硬件信息
    typedef struct cpuinfo
    {
        char system_type[128];
        char cpu_model[128];
        char BogoMIPS[32];
    } cpu_info;

    //CPU 状态
    typedef struct cpustatus
    {
        long total;
        float user;
        float nice;
        float system;
        float idle;
    } cpu_status;

    typedef struct          //声明一个occupy的结构体
    {
        unsigned int user;  //从系统启动开始累计到当前时刻，处于用户态的运行时间，不包含 nice值为负进程。
        unsigned int nice;  //从系统启动开始累计到当前时刻，nice值为负的进程所占用的CPU时间
        unsigned int system;//从系统启动开始累计到当前时刻，处于核心态的运行时间
        unsigned int idle;  //从系统启动开始累计到当前时刻，除IO等待时间以外的其它等待时间iowait (12256) 从系统启动开始累计到当前时刻，IO等待时间(since 2.5.41)
    } total_cpu_occupy_t;

    typedef struct
    {
        pid_t pid;//pid号
        unsigned int utime;   //该任务在用户态运行的时间，单位为jiffies
        unsigned int stime;   //该任务在核心态运行的时间，单位为jiffies
        unsigned int cutime;  //所有已死线程在用户态运行的时间，单位为jiffies
        unsigned int cstime;  //所有已死在核心态运行的时间，单位为jiffies
    } process_cpu_occupy_t;

    //memory
    typedef struct meminfo
    {
        char total[20];
        char free[20];
    } mem_info;

    //process memory
    typedef struct processmeminfo
    {
        char mem[20];  /* 物理内存 */
        char vmem[20]; /* 虚拟内存 */
    } process_mem_info;

    extern int AddCPUInfoToSysInfoXML(CPacket * pOutPacket);
    extern int AddDeviceInfoToBoardSearchXML(CPacket * pOutPacket);
    extern int AddFanInfoToSysInfoXML(CPacket * pOutPacket);
    extern int AddMemoryInfoToSysInfoXML(CPacket * pOutPacket);
    extern int AddNetWorkInfoToBoardSearchXML(CPacket * pOutPacket, DOMElement * ItemAccNode);
    extern int AddNetWorkInfoToSysInfoXML(CPacket * pOutPacket);
    extern int AddSystemDateTimeInfoToSysInfoXML(CPacket * pOutPacket);
    extern int BoardSearchCmdProc(char * pcSerialNumber, std::string & outBuff);
    extern int BoardSearchProc(char * pcInBuff, int iInLen, std::string & outBuff);
    extern int BoardSearchRebootCMSProc();
    extern int BoardSearchRebootDBProc();
    extern int BoardSearchRebootKeyBoardProc();
    extern int BoardSearchRebootMediaServiceProc();
    extern int BoardSearchRestoreFactorySettingsProc();
    extern int BoardSearchSetIPAddrProc(CPacket & inPacket, std::string & outBuff);
    extern int CreateXMLHeadResponseForSystemRunState(CPacket** pOutPacket, char * strCmdType, char * pcSerialNumber);
    extern int find_pid_by_name(char* ProcName, int* foundpid);
    extern int GeneratingErrorResponseForSystemRunState(char * strCmdType, char * pcSerialNumber, int iErrorCode, char * strErrorReason, std::string & outBuff);
    extern int GetDeviceInfoProc(char* pcSerialNumber, std::string& outBuff);
    extern int GetEthNameByMac(char* pcMac, char* pEthName);
    extern int getLocalNetInfo(void);
    extern int GetSystemInfoProc(char* pcSerialNumber, std::string& outBuff);
    extern int GetSystemIPAddr(int port);
    extern int GetSystemRunStateProc(const std::string& inBuff, std::string& outBuff);
    extern int GetTaskInfoProc(char* pcSerialNumber, std::string& outBuff);
    extern int GetTSUInfoProc(char* pcSerialNumber, std::string& outBuff);
    extern int GetUserInfoProc(char* pcSerialNumber, std::string& outBuff);
    extern int get_cpu_info(cpu_info* cpuinfo);
    extern unsigned int get_cpu_process_occupy(const pid_t p);
    extern void get_cpu_status(cpu_status* cpu_stat, int process_num);
    extern unsigned int get_cpu_total_occupy();
    extern void get_eth_gateway(char* eth_name, char* gateway_addr);
    extern const char* get_items(const char* buffer, int ie);
    extern void get_mem_info(mem_info* minfo);
    extern float get_pcpu(pid_t p);
    extern int get_phy_mem(const pid_t p);
    extern float get_pmem(pid_t p);
    extern int get_progress_cpu_usage(char* ProcName, float* pfcpu);
    extern int get_progress_memory_usage(char* ProcName, process_mem_info* proc_mem);
    extern int get_total_mem();
    extern int IsIPAddrIsLocalIP(char* strEthName, char* strIPAddr);
    extern int SendSystemDiskErrorToAllClientUser();
    extern int SetIPAddrProc(CPacket& inPacket, std::string& outBuff);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif /* __GBLFUNC_PROC_H__ */
