
/***********************************************************************
* Copyright (C) 2013, WISCOM VISION TECHNOLOGY Corporation.
* 
* File Name: 	 PlatTimerProc.h
* Description:  定时处理用到的头文件声明
* Others:        无
* Version:  	   dev-v1.01.01
* Author:        输入作者名字及单位
* Date:  	       2013.05.31
* 
* History 1:  		// 修改历史记录，包括修改日期、修改者、修改版本及修改内容
*                2013.05.31    qyg    dev-v1.01.01   首次创建  
* History 2: …
**********************************************************************/
#ifndef  PLAT_TIMER_PROC_H
#define  PLAT_TIMER_PROC_H

/*----------------------------------------------*
 * 包含头文件                                   *
 *----------------------------------------------*/
  
/*----------------------------------------------*
 * 包含头文件                                   *
 *----------------------------------------------*/
#include <sys/types.h>
#ifdef WIN32
#include <sys/socket.h>
#include <winsock2.h>
#else
#include <netinet/in.h>
#include <unistd.h>

#endif


#include <vector>
#include <string>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>



#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */
#define  TIMER_LEN_1S      2
#define  TIMER_LEN_2S      4
#define  TIMER_LEN_5S      10
#define  TIMER_LEN_10S     20
#define  TIMER_LEN_20S     40
#define  TIMER_LEN_2M      (2*60*2)


/* 函数声明  */

extern void InitTimer();
extern void UnInitTimer();
extern void TimerProc(int signo);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif



