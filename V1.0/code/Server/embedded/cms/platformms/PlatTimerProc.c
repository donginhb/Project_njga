
/***********************************************************************
* Copyright (C) 2013, WISCOM VISION TECHNOLOGY Corporation.
*
* File Name:      TimerProc.c
* Description:  Cms ���ù����Ӻ���ʵ��
* Others:        ��
* Version:         dev-v1.01.01
* Author:        �����������ּ���λ
* Date:            2013.05.31
*
* History 1:        // �޸���ʷ��¼�������޸����ڡ��޸��ߡ��޸İ汾���޸�����
*                2013.05.31    qyg    dev-v1.01.01   �״δ���
* History 2: ��
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
/*  ����ģ��  */
#include "common/gblconfig_proc.inc"

/*  ȫ�ֱ��� */
static unsigned int g_dwSipTimerCnt = 0;
static sem_t        SemLedAndDog;

/* �ⲿ���� */
extern void cms_time_count(int sig);

/*****************************************************************************
 �� �� ��  : InitTimer
 ��������  : ��ʱ����ʼ��
 �������  : ��
 �������  : ��
 �� �� ֵ      : ��
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��10��
    ��    ��   : qyg
    �޸�����   : �����ɺ���

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

    /*  ��ʱ����������  */
    g_dwSipTimerCnt = 0;

    setitimer(ITIMER_REAL, &timer, NULL);

    return ;

}

void UnInitTimer()
{
    /*  ��ʱ����������  */
    g_dwSipTimerCnt = 0;

    return;
}


/*****************************************************************************
 �� �� ��  : TimerProc
 ��������  : ��ʱ��������
 �������  : signo     �ź�����
 �������  : ��
 �� �� ֵ      : ��
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��10��
    ��    ��   : qyg
    �޸�����   : �����ɺ���

*****************************************************************************/
void TimerProc(int signo)
{
    switch (signo)
    {
        case SIGALRM:

            /* ����ι���͵�Ƶ��ź��� */
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
