
/***********************************************************************
* Copyright (C) 2013, WISCOM VISION TECHNOLOGY Corporation.
*
* File Name:     BoardInit.c
* Description :  单板初始化以及主备竞争
* Version:       dev-v1.01.01
* Author:        输入作者名字及单位
* Date:          2013.05.31
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
#endif

#include "../../../common/include/EV9000_CommonManage.h"
#include "platformms/PlatTimerProc.h"
#include "platformms/BoardInit.h"
#include "common/log_proc.inc"
#include "common/gblconfig_proc.inc"
#include "platformms/CmsIpConfig.h"
#include "CPing.h"

/* 全局变量定义  */
BOARD_NET_ATTR  g_BoardNetConfig;
MS_SWITCH_ATTR  gt_MSSwitchAttr;

/*****************************************************************************
 函 数 名  : BoardInit
 功能描述  : 单板从配置文件中读取相应的IP地址
 输入参数  : 无
 输出参数  : 无
 返 回 值  :  0：   成功
              非零： 失败
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月o10日
    作    者   : qyg
    修改内容   : 新生成函数

*****************************************************************************/
int Board_Init()
{
    int iRet = 0;

    /*  1、初始化定时器 */
    InitTimer();

    /* 2、初始化全局变量 */
    Glb_BoardInit();

    /* 3、加载配置文件参数 */
    printf("\r\n BoardInit() Cms_ReadWebConf:Start\n");
    iRet =  Cms_ReadWebConf(&g_BoardNetConfig);
    printf(" BoardInit() Cms_ReadWebConf End:iRet=%d\n", iRet);
    return iRet;
}

void  Board_UnInit()
{
    UnInitTimer();
    UnGlb_BoardInit();
}

/*  单板重启 */
void BoardReboot()
{
    /* 设置系统正常退出标识 */
    change_conf_to_config_file((char*)"sysexitflag", (char*)"0");
    system((char*)"reboot");
}

void Glb_BoardInit()
{
    unsigned int dwTmpIP = 0;

    memset(&g_BoardNetConfig, 0, sizeof(BOARD_NET_ATTR));
    memset(&gt_MSSwitchAttr, 0, sizeof(MS_SWITCH_ATTR));
    gt_MSSwitchAttr.dwCMSUsedStatus = CMS_SLAVE_USED ;

    gt_MSSwitchAttr.dwPlaceID = 0;
    gt_MSSwitchAttr.dwBoardType = LOGIC_BOARD_CMS;

    gt_MSSwitchAttr.tDefaultDevIP.dwIPAddr =  0xFFFFFFFF;
    gt_MSSwitchAttr.tDefaultDevIP.dwIPMask = 0xFFFFFFFF;
    gt_MSSwitchAttr.tDefaultDevIP.dwGetway = 0xFFFFFFFF;

    dwTmpIP = inet_addr(CMS_DEFAULT_VIDEO_IP);
    gt_MSSwitchAttr.tDefaultVidIP.dwIPAddr  = (dwTmpIP  + gt_MSSwitchAttr.dwPlaceID) ;

    gt_MSSwitchAttr.tDefaultVidIP.dwIPMask =    inet_addr(CMS_DEFAULT_VIDEO_NETMASK);
    gt_MSSwitchAttr.tDefaultVidIP.dwGetway =     inet_addr(CMS_DEFAULT_VIDEO_GATEWAY);

    return;
}

void UnGlb_BoardInit()
{
    memset(&g_BoardNetConfig, 0, sizeof(BOARD_NET_ATTR));
    memset(&gt_MSSwitchAttr, 0, sizeof(MS_SWITCH_ATTR));
    gt_MSSwitchAttr.dwCMSUsedStatus = CMS_SLAVE_USED ;

    return;
}

/*****************************************************************************
 函 数 名  : GetMatchUsedStatus
 功能描述  : 配置单板的IP地址
 输入参数  : unsigned int dwServerIP,
              short int wServerPort
             int iDefaultGateWayFlag
 输出参数  : 无
 返 回 值  :  CMS_SLAVE_USED

 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月o10日
    作    者   : qyg
    修改内容   : 新生成函数


*****************************************************************************/
int ConfigEth(const char* pethtype, IP_ADDR_T* pIPAddr, int iDefaultGateWayFlag)
{
    int iRet = 0;
    char  s8Cmd[MAX_CMDLINE_LEN];
    char  s8Ip[IP_STR_LEN];
    char  s8Netmask[IP_STR_LEN];
    char  s8Gateway[IP_STR_LEN];
    char  s8TmpIP[IP_STR_LEN];
    unsigned int udTmpIP = 0;

    if ((0 == pIPAddr->dwIPAddr) || (0xffffffff == pIPAddr->dwIPAddr))
    {
        printf(" ConfigEth() eth=%s,config ERROR ,IPAddr=%d \n", pethtype, pIPAddr->dwIPAddr);
        return -1;
    }

    strcpy(s8Ip, inet_ntoa(*(struct in_addr*)&pIPAddr->dwIPAddr));
    strcpy(s8Netmask, inet_ntoa(*(struct in_addr*)&pIPAddr->dwIPMask));
    printf(" ConfigEth() eth=%s: IP=%s, mask=%s\n", pethtype, s8Ip, s8Netmask);

    snprintf(s8Cmd, MAX_CMDLINE_LEN, "/sbin/ifconfig %s up", pethtype);
    system(s8Cmd);

    snprintf(s8Cmd, MAX_CMDLINE_LEN, "/sbin/ifconfig %s %s netmask %s", pethtype, s8Ip, s8Netmask);
    system(s8Cmd);

    if (CMS_INVALID_IP != pIPAddr->dwGetway && CMS_NULL_IP != pIPAddr->dwGetway)
    {
        if (iDefaultGateWayFlag)
        {
            strcpy(s8Gateway, inet_ntoa(*(struct in_addr*)&pIPAddr->dwGetway));
            snprintf(s8Cmd, MAX_CMDLINE_LEN, "route add default gw %s dev %s",
                     s8Gateway, pethtype);
            system(s8Cmd);
        }
        else
        {
            /* IP与netmask相与 */
            udTmpIP = (pIPAddr->dwIPAddr & pIPAddr->dwIPMask);
            strcpy(s8TmpIP, inet_ntoa(*(struct in_addr*)&udTmpIP));
            /* 网关 */
            strcpy(s8Gateway, inet_ntoa(*(struct in_addr*)&pIPAddr->dwGetway));

            snprintf(s8Cmd, MAX_CMDLINE_LEN, "route add -net %s netmask %s gw %s dev %s",
                     s8TmpIP, s8Netmask, s8Gateway, pethtype);
            printf(" ConfigEth() route=%s\n", s8Cmd);
            system(s8Cmd);
        }

        printf(" ConfigEth() eth=%s: gateway=%s\n", pethtype, s8Gateway);
    }

    return iRet;
}
