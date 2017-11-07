
/***********************************************************************
* Copyright (C) 2013, WISCOM VISION TECHNOLOGY Corporation.
*
* File Name:      TimerProc.c
* Description:  Cms 配置管理子函数实现
* Others:        无
* Version:         dev-v1.01.01
* Author:        输入作者名字及单位
* Date:            2013.05.31
*
* History 1:        // 修改历史记录，包括修改日期、修改者、修改版本及修改内容
*                2013.05.31    qyg    dev-v1.01.01   首次创建
* History 2: …
**********************************************************************/
#include <stdio.h>
#ifdef WIN32
#include <winsock.h>
#include <sys/types.h>
#else
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/time.h>
#endif

#include "../../../common/include/EV9000_CommonManage.h"
#include "platformms/PlatTimerProc.h"
#include "platformms/BoardInit.h"
/*  其他模块  */
#include "common/gblconfig_proc.inc"

/*  全局变量 */
static unsigned int g_dwSipTimerCnt = 0;
static sem_t        SemLedAndDog;

/* 外部引用 */
extern void cms_time_count(int sig);

/*****************************************************************************
 函 数 名  : InitTimer
 功能描述  : 定时器初始化
 输入参数  : 无
 输出参数  : 无
 返 回 值      : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月10日
    作    者   : qyg
    修改内容   : 新生成函数

*****************************************************************************/
void InitTimer()
{

#if 1
    struct itimerval timer;
    timer.it_value.tv_sec   = 0;
    timer.it_value.tv_usec  = 500000;
    timer.it_interval       = timer.it_value;

    signal(SIGALRM, TimerProc);
#endif

    /*  定时器计数清零  */
    g_dwSipTimerCnt = 0;

    setitimer(ITIMER_REAL, &timer, NULL);

    return ;

}

void UnInitTimer()
{
    /*  定时器计数清零  */
    g_dwSipTimerCnt = 0;

    return;
}


/*****************************************************************************
 函 数 名  : TimerProc
 功能描述  : 定时处理流程
 输入参数  : signo     信号类型
 输出参数  : 无
 返 回 值      : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月10日
    作    者   : qyg
    修改内容   : 新生成函数

*****************************************************************************/
void TimerProc(int signo)
{
    switch (signo)
    {
        case SIGALRM:

            /* 触发喂狗和点灯的信号量 */
            sem_post(&SemLedAndDog);

            if (g_dwSipTimerCnt++  >=  TIMER_LEN_1S)
            {
                cms_time_count(signo);
                g_dwSipTimerCnt = 0;

            }

            break;

        default:
            break;
    }

}
