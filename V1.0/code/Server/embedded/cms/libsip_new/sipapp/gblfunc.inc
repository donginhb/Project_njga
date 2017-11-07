/******************************************************************************

                  版权所有 (C), 2001-2013, 金智视讯技术有限公司

 ******************************************************************************
  文 件 名   : gblfunc.h
  版 本 号   : 初稿
  作    者   : yanghaifeng
  生成日期   : 2013年4月1日
  最近修改   :
  功能描述   : gblfunc.c 的头文件
  函数列表   :
  修改历史   :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 创建文件

******************************************************************************/
#ifndef __GBLFUNC_H__
#define __GBLFUNC_H__
/*----------------------------------------------*
 * 包含头文件                                   *
 *----------------------------------------------*/
#include   <stdlib.h>
#ifdef WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

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
#ifndef   PATH_MAX
#define   PATH_MAX   1024
#endif

    /*----------------------------------------------*
     * 宏定义                                       *
     *----------------------------------------------*/
    extern int datetime(char* s, size_t smax, time_t* tp);
    extern unsigned int get_ipaddr(char* host);
    extern int good_host(char* addr);
    extern int good_ipaddr(char* addr);
    extern int host_match(char* host1, char* host2);
    extern int inet_type(char* host);
    extern void ipaddr2str(char* buffer, unsigned long ipaddr);
    extern int is_host_ip(char* host, char* ipaddr);
    extern int  is_private_ip(char* ipaddr);
    extern double satod(char* number);
    extern int sstrcmp(const char* s1, const char* s2);
    extern int printf_system_time1();
    extern unsigned int analysis_time1(char* strTime);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif /* __GBLFUNC_H__ */
