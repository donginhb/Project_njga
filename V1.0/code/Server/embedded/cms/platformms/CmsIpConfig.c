
/***********************************************************************
* Copyright (C) 2013, WISCOM VISION TECHNOLOGY Corporation.
*
* File Name:       CmsIpconfig.c
* Description : Cms的各种IP的配置文件的读取和存储
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
#endif

#include "../../../common/include/EV9000_CommonManage.h"
#include "platformms/CmsIpConfig.h"
#include "platformms/BoardInit.h"
#include "common/gblfunc_proc.inc"
#include "common/gblconfig_proc.inc"
#include "../libsip_new/sipstack/include/osipparser2/osip_port.h"

/* 全局变量定义  */

//extern BOARD_NET_ATTR  g_BoardNetConfig;
WEB_CONF_ATTR   g_tWebConf;

/*****************************************************************************
 函 数 名  : Cms_ReadWebConf
 功能描述  : 读取web界面的配置文件
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

int Cms_ReadWebConf(BOARD_NET_ATTR* conf)
{
    FILE* rcfp = NULL;
    char str[256] = {0};
    char* pname = NULL, *pvalue = NULL, *tmp = NULL;
    int iRet = 0;
    unsigned int  dwTempValue = 0;

    rcfp = fopen(CMS_CONFIG_FILE, "r");

    if (NULL == rcfp)
    {
        iRet = gbl_conf_load();

        if (0 != iRet)
        {
            printf(" Cms_ReadWebConf() exit---: gbl_conf_load Error \r\n");
            return -1;
        }

        rcfp = fopen(CMS_CONFIG_FILE, "r");

        if (NULL == rcfp)
        {
            printf(" Cms_ReadWebConf(): exit -1 \n");
            return -1;
        }
    }

    while (NULL != fgets(str, sizeof(str), rcfp))
    {
        if ((strlen(str) == 1) || (0 == strncmp(str, "#", 1)))
        {
            continue;
        }

        tmp = strchr(str, 61); /*find '=' */

        if (tmp != NULL)
        {
            if (tmp - str <= 0 || str + strlen(str) - tmp <= 0)
            {
                printf(" Cms_ReadWebConf() malloc len error \n");
                continue;
            }

            pname = (char*) osip_malloc(tmp - str + 1);
            pvalue = (char*) osip_malloc(str + strlen(str) - tmp);
            osip_strncpy(pname, str, tmp - str);
            osip_strncpy(pvalue, tmp + 1, str + strlen(str) - tmp - 2);

            sclrspace(pname);
            //stolowercase(pname);
            sclrspace(pvalue);

            iRet = WebConf_set(conf, pname, pvalue);
            printf(" Cms_ReadWebConf() WebConf_set:pname=%s, pvalue=%s, iRet=%d\n", pname, pvalue, iRet);
            //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "load_file_config() pname=%s,pvalue=%s,i=%d \r\n", pname, pvalue, iRet);

            osip_free(pname);
            pname = NULL;
            osip_free(pvalue);
            pvalue = NULL;
        }

    }

    /*  更新公共网的信息  */
    dwTempValue = conf->tCmsCommonDeviceIP.tNetIP.dwIPAddr;
    memcpy((void*) & (conf->tCmsCommonDeviceIP), (void*) & (conf->tCmsDeviceIp), sizeof(ETH_ATTR));
    conf->tCmsCommonDeviceIP.tNetIP.dwIPAddr = dwTempValue;

    dwTempValue = conf->tCmsCommonVideoIP.tNetIP.dwIPAddr;
    memcpy((void*) & (conf->tCmsCommonVideoIP), (void*) & (conf->tCmsVideoIP), sizeof(ETH_ATTR));
    conf->tCmsCommonVideoIP.tNetIP.dwIPAddr  = dwTempValue;

    fclose(rcfp);

    if (CMS_NULL_IP == conf->tCmsVideoIP.tNetIP.dwIPAddr) /* 默认配置文件里面没有视频网IP地址信息 */
    {
        return -1;
    }

    return iRet;

}

int WebConf_set(BOARD_NET_ATTR* conf, char* pname, char* pvalue)
{
    int iRet = 0;

    if (conf == NULL || pname == NULL || pvalue == NULL)
    {
        return -1;
    }

    if (sstrcmp(pname, "cmsid") == 0)
    {
        memcpy(conf->ucCmsID, pvalue, strlen(pvalue));
    }
    else if (sstrcmp(pname, "dbip") == 0)
    {
        conf->tCmsDBIP.tNetIP.dwIPAddr = (unsigned int)(inet_addr(pvalue));
    }
    else if (sstrcmp(pname, "sdbip") == 0)
    {
        conf->tCmsSDBIP.tNetIP.dwIPAddr = (unsigned int)(inet_addr(pvalue));
    }
    else if (sstrcmp(pname, "alarmip") == 0)
    {
        conf->tCmsAlarmIP.tNetIP.dwIPAddr = (unsigned int)(inet_addr(pvalue));
    }
    else if (sstrcmp(pname, "ntpip") == 0)
    {
        conf->tCmsNtpIP.tNetIP.dwIPAddr = (unsigned int)(inet_addr(pvalue));
    }
    /*  设备网 */
    else if (sstrcmp(pname, "deveth") == 0)
    {
        conf->tCmsDeviceIp.dwEth = (unsigned int)atoi(pvalue);
    }
    else if (sstrcmp(pname, "devip") == 0)
    {
        conf->tCmsDeviceIp.tNetIP.dwIPAddr = (inet_addr(pvalue));
    }
    else if (sstrcmp(pname, "devmask") == 0)
    {
        conf->tCmsDeviceIp.tNetIP.dwIPMask = (inet_addr(pvalue));
    }
    else if (sstrcmp(pname, "devgateway") == 0)
    {
        conf->tCmsDeviceIp.tNetIP.dwGetway = (inet_addr(pvalue));
    }
    else if (sstrcmp(pname, "devhost") == 0)
    {
        memcpy(conf->tCmsDeviceIp.tNetIP.ucHostname, pvalue, strlen(pvalue));
    }
    else if (sstrcmp(pname, "devport") == 0)
    {
        conf->tCmsDeviceIp.tNetIP.dwPort = (unsigned int)atoi(pvalue);
    }
    else if (sstrcmp(pname, "devflag") == 0)
    {
        conf->tCmsDeviceIp.UsedFlag = (unsigned int)atoi(pvalue);
    }
    /* 视频网 */
    else if (sstrcmp(pname, "videth") == 0)
    {
        conf->tCmsVideoIP.dwEth = (unsigned int)atoi(pvalue);
    }
    else if (sstrcmp(pname, "vidip") == 0)
    {
        conf->tCmsVideoIP.tNetIP.dwIPAddr = (inet_addr(pvalue));
    }
    else if (sstrcmp(pname, "vidmask") == 0)
    {
        conf->tCmsVideoIP.tNetIP.dwIPMask = (inet_addr(pvalue));
    }
    else if (sstrcmp(pname, "vidgateway") == 0)
    {
        conf->tCmsVideoIP.tNetIP.dwGetway = (inet_addr(pvalue));
    }
    else if (sstrcmp(pname, "vidhost") == 0)
    {
        memcpy(conf->tCmsVideoIP.tNetIP.ucHostname, pvalue, strlen(pvalue));
    }
    else if (sstrcmp(pname, "vidflag") == 0)
    {
        conf->tCmsVideoIP.UsedFlag = (unsigned int)atoi(pvalue);
    }
    else if (sstrcmp(pname, "vidport") == 0)
    {
        conf->tCmsVideoIP.tNetIP.dwPort = (unsigned int)atoi(pvalue);
    }
    /* 公共视频网 */
    else if (sstrcmp(pname, "cvidip") == 0)
    {
        conf->tCmsCommonVideoIP.tNetIP.dwIPAddr = (inet_addr(pvalue));
    }
    /*  公共设备网 */
    else if (sstrcmp(pname, "cdevip") == 0)
    {
        conf->tCmsCommonDeviceIP.tNetIP.dwIPAddr = (inet_addr(pvalue));
    }
    /* 备机的ip */
    else if (sstrcmp(pname, "sdevip") == 0)
    {
        conf->tCmsSDeviceIP.tNetIP.dwIPAddr = (inet_addr(pvalue));
    }
    else if (sstrcmp(pname, "svidip") == 0)
    {
        conf->tCmsSVideoIP.tNetIP.dwIPAddr = (inet_addr(pvalue));
    }
    else if (sstrcmp(pname, "msenable") == 0)
    {
        conf->dwMSFlag = (unsigned int)atoi(pvalue);
    }
    else if (sstrcmp(pname, "WMSFlag") == 0)
    {
        conf->st_MSFlag = (unsigned int)atoi(pvalue);
    }

    return iRet;
}
