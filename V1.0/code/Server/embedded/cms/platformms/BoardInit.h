
/***********************************************************************
* Copyright (C) 2013, WISCOM VISION TECHNOLOGY Corporation.
*
* File Name:     BoardInit.h
* Description:  单板控制的函数
* Others:        无
* Version:         dev-v1.01.01
* Author:        输入作者名字及单位
* Date:            2013.05.31
*
* History 1:        // 修改历史记录，包括修改日期、修改者、修改版本及修改内容
*                2013.05.31    qyg    dev-v1.01.01   首次创建
* History 2: …
**********************************************************************/
#ifndef BOARD_INIT_H
#define BOARD_INIT_H

/*----------------------------------------------*
 * 包含头文件                                   *
 *----------------------------------------------*/

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
#include <semaphore.h>
#include <pthread.h>
#endif


#include <vector>
#include <string>
#include "../../../common/include/EV9000_CommonManage.h"

using namespace std;

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */
#define CMS_MASTER_USED      0x00
#define CMS_SLAVE_USED       0x01
#define CMS_UNKNOWN_USED     0x02

#define EV9000LX             "9U"
#define EV9000MX             "4U"
#define EV9000SX             0x03

#define MAX_CMDLINE_LEN            512
#define IP_STR_LEN                 16


#define    ETH0   "eth0"
#define    ETH1   "eth1"
#define    ETH2   "eht2"
#define    ETH3   "eth3"
#define    ETH4   "eth4"
#define    ETH5   "eth5"
#define    ETH6   "eth6"
#define    ETH7   "eth7"
#define    ETH8   "eth8"
#define    ETH9   "eth9"

#define  CMS_MAST_COMMUNICATION_IP      "202.108.33.0"     /* 最后一位由槽位号决定 */
#define  CMS_MAST_COMMUNICATION_MASK    "255.0.0.0"
#define  CMS_MAST_COMMUNICATION_GATEWAY  0
#define  CMS_INVALID_IP                  0
#define  CMS_NULL_IP                     0xffffffff

#define  CMS_SLAVE_COMMUNICATION_IP      "202.108.33.0"     /* 最后一位由槽位号决定 */
#define  CMS_SLAVE_COMMUNICATION_MASK    "255.0.0.0"
#define  CMS_SLAVE_COMMUNICATION_GATEWAY 0

#define CMS_DEFAULT_VIDEO_IP          "192.168.0.0" /* 最后一位用槽位号 */
#define CMS_DEFAULT_VIDEO_NETMASK     "255.255.255.0"
#define CMS_DEFAULT_VIDEO_GATEWAY     "192.168.0.1"

#define  CMS_SLAVE_COMMUNICATION_PORT    21458

    typedef struct
    {
        unsigned int dwCMSUsedStatus;
        unsigned int dwCMSErrorFlag;
        unsigned int dwPlaceID;
        unsigned int dwBoardType;
        IP_ADDR_T    tSelfMSConnetIP;
        IP_ADDR_T    tMatchConnetIP;
        IP_ADDR_T    tDefaultDevIP;
        IP_ADDR_T    tDefaultVidIP;

    } MS_SWITCH_ATTR;

    static const char g_EthType[][8] = {"eth0", "eth1", "eth2", "eth3", "eth4", "eht5", "eth6", "eth7", "eth8", "eth9"};

    extern BOARD_NET_ATTR  g_BoardNetConfig;
    extern MS_SWITCH_ATTR  gt_MSSwitchAttr;

    extern int Board_Init();
    extern void Board_UnInit();
    extern void Glb_BoardInit();
    extern void UnGlb_BoardInit();
    extern void BoardReboot();
    extern int BoardEthIPConfig(BOARD_NET_ATTR* pNetConf);
    extern int ConfigEth(const char* pethtype, IP_ADDR_T* pIPAddr, int iDefaultGateWayFlag);


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif



