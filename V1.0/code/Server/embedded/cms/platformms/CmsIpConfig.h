
/***********************************************************************
* Copyright (C) 2013, WISCOM VISION TECHNOLOGY Corporation.
*
* File Name:     CmsIPConfig.h
* Description:  版本加载相关处理控制流程
* Others:        无
* Version:         dev-v1.01.01
* Author:        输入作者名字及单位
* Date:            2013.05.31
*
* History 1:        // 修改历史记录，包括修改日期、修改者、修改版本及修改内容
*                2013.05.31    qyg    dev-v1.01.01   首次创建
* History 2: …
**********************************************************************/
#ifndef CMS_IP_CONFIG_H
#define CMS_IP_CONFIG_H

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

using namespace std;

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

#define  CMS_CONFIG_FILE    "/config/cmscof.cfg"
#define  CMS_WEB_CONFIG_FILE   "/config/webapp.cfg"

#define  CMS_CONF_LEN          512
#define  CMS_IP_STR_LEN        16

#define  CMS_DEVICE_IP         "cms_device_ip"
#define  CMS_DEVICE_MASK       "cms_device_mask"
#define  CMS_DEVICE_GATEWAY    "cms_device_gateway"

#define  CMS_VIDEO_IP          "cms_video_ip"
#define  CMS_VIDEO_MASK        "cms_video_mask"
#define  CMS_VIDEO_GATEWAY     "cms_vido_gateway"

#define  CMS_SLAVE_DEVICE_IP         "cms_slave_device_ip"
#define  CMS_SLAVE_DEVICE_MASK       "cms_slave_device_mask"
#define  CMS_SLAVE_DEVICE_GATEWAY    "cms_slave_device_gateway"

#define  CMS_SLAVE_VIDEO_IP          "cms_slave_video_ip"
#define  CMS_SLAVE_VIDEO_MASK        "cms_slave_video_mask"
#define  CMS_SLAVE_VIDEO_GATEWAY     "cms_slave_vido_gateway"

#define  CMS_COMMON_DEVICE_IP          "cms_common_video_ip"
#define  CMS_COMMON_DEVICE_MASK        "cms_common_video_mask"
#define  CMS_COMMON_DEVICE_GATEWAY     "cms_common_vido_gateway"

#define  CMS_COMMON_VIDEO_IP          "cms_common_video_ip"
#define  CMS_COMMON_VIDEO_MASK        "cms_common_video_mask"
#define  CMS_COMMON_VIDEO_GATEWAY     "cms_common_vido_gateway"

    typedef struct
    {
        unsigned int  dwWebConfigFalg;
        IP_ADDR_T    tDevIP;
        IP_ADDR_T    tVidoIP;
        IP_ADDR_T    tDbIP;
        IP_ADDR_T    tAlarmIP;
        IP_ADDR_T    tNtpIP;
        char         Cmsid[24];

    } WEB_CONF_ATTR;

    extern int WebConf_set(BOARD_NET_ATTR* conf, char* pname, char* pvalue);
    extern int Cms_ReadWebConf(BOARD_NET_ATTR* conf);


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif



