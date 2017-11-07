/******************************************************************************

                  版权所有 (C), 2001-2013, 金智视讯技术有限公司

 ******************************************************************************
  文 件 名   : gblfunc_proc.h
  版 本 号   : 初稿
  作    者   : yanghaifeng
  生成日期   : 2013年4月16日
  最近修改   :
  功能描述   : gblfunc_proc.c 的头文件
  函数列表   :
  修改历史   :
  1.日    期   : 2013年4月16日
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

#ifndef __GBLFUNC_PROC_H__
#define __GBLFUNC_PROC_H__

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

    /* 转换double数据的宏 */
#define ntohd(x) (SwapDoubleEndian(&(x)))

    extern unsigned int analysis_time(char* strTime);
    extern int analysis_time2(char* strTime);
    extern int CMS_CloseSIPTCPConnect(int sockfd);
    extern int CMS_CreateSIPTCPConnect(char* remote_ip, int remote_port);
    extern unsigned int crc32(unsigned char* buf, int len);
    extern int Find_NodeName_for_Vector(char* NodeName, vector<string>& NodeName_vector);
    extern int format_time(int iTime, char* strTime);
    extern unsigned char GetWeekFromDay(unsigned char year, unsigned char month, unsigned char day);
    extern _xml_sub_type_t get_xml_sub_type_from_xml_body(vector<string>& NodeName_vector, CPacket& inPacket);
    extern xml_type_t get_xml_type_from_xml_body(vector<string>& NodeName_vector, CPacket& inPacket);
    extern xml_type_t2 get_xml_type_from_xml_body2(vector<string>& NodeName_vector, CPacket& inPacket);
    extern void ipaddr_long2str(char* buffer, unsigned long ipaddr);
    extern int ip_addr_list_clone(const osip_list_t* src, osip_list_t* dst);
    extern bool isGB2312Code(const string& strIn);
    extern bool isGBKCode(const string& strIn);
    extern int IsIDMatchLocalCMSID(char* strID);
    extern int IsLocalAuthRealm(char* realm);
    extern int IsLocalHost(char* host);
    extern bool IsUTF8String(const char* str, int length);
    extern int printf_system_time();
    extern char *random_uuid(char buf[37]);
    extern int set_system_time(int iTime);
    extern int StartUDPReceive();
    extern int String2Bytes(unsigned char* szSrc, unsigned char* pDst, int nDstMaxLen);
    extern int StringIsAllDigit(char* pcCivilCode);
    extern char* strupr(char* str);
    extern double SwapDoubleEndian(double* pdVal);
    extern int time_config_add(osip_list_t* pDayOfWeekTimeList, unsigned int uID, int iBeginTime, int iEndTime);
    extern void time_config_free(time_config_t* time_config);
    extern time_config_t* time_config_get(osip_list_t* pDayOfWeekTimeList, unsigned int uID);
    extern int time_config_init(time_config_t** time_config);
    extern void Utf8ToGb2312(const char * utf8, int len, char * temp);
    extern int SortCodeTable();
    extern int sstrcmp(const char* s1, const char* s2);
    extern int Setnonblocking(int sock);

#ifdef ENABLE_DEBUG_LOCK
#define CMS_GBL_SMUTEX_LOCK(s) osip_debug_mutex_lock(s, __FILE__, __LINE__, __func__)
#define CMS_GBL_SMUTEX_UNLOCK(s) osip_debug_mutex_unlock(s, __FILE__, __LINE__, __func__)
#else
#define CMS_GBL_SMUTEX_LOCK(s) osip_mutex_lock(s)
#define CMS_GBL_SMUTEX_UNLOCK(s) osip_mutex_unlock(s)
#endif

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif /* __GBLFUNC_PROC_H__ */
