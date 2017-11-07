/******************************************************************************

                  版权所有 (C), 2001-2013, 金智视讯技术有限公司

 ******************************************************************************
  文 件 名   : gblfunc.c
  版 本 号   : 初稿
  作    者   : yanghaifeng
  生成日期   : 2013年4月1日
  最近修改   :
  功能描述   : 公共处理函数
  函数列表   :
              bit2netmask
              datetime
              digital_string
              get_exe_name
              get_ipaddr
              get_sysinfo
              get_versioninfo
              good_host
              good_ipaddr
              host_match
              inet_type
              ipaddr2str
              ipstr2long
              ip_addr4_free
              ip_addr4_init
              is_host_ip
              is_private_ip
              partial_ipaddr
              satod
              sstrcmp
  修改历史   :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 创建文件

******************************************************************************/

/*----------------------------------------------*
 * 包含头文件                                   *
 *----------------------------------------------*/
#ifdef WIN32
#include <winsock2.h>
#include <windows.h>
#include <sys/types.h>
#include <stdio.h>
#include "time.h"
#else
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/stat.h>
#endif

#include "gblfunc.inc"
#include "csdbg.inc"

/* added by chenyu 130522 */
#ifdef WIN32
#define vsnprintf _vsnprintf
#define snprintf  _snprintf
#endif

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

double satod(char* number)
{
#if defined(__linux) || defined(HAVE_STRTOL)
    double i;

    if (NULL == number)
    {
        return -1;
    }

    i = strtod(number, (char**) NULL);
    return i;
#endif

    return atof(number);
}

int datetime(char* s, size_t smax, time_t* tp)
{
    struct tm t = { 0 };

    if (NULL == tp)
    {
        return -1;
    }

#ifdef WIN32
    time_t utc_time = time(NULL);
    struct tm* local_time_tmp = NULL;
    local_time_tmp = localtime(&utc_time);
    t = *local_time_tmp;                      //待修改
#else
    localtime_r(tp, &t);
#endif

    strftime(s, smax, "%Y-%m-%d %H:%M:%S", &t);

    return 0;
}

int sstrcmp(const char* s1, const char* s2)
{
    if (NULL == s1 || NULL == s2)
    {
        return -1;
    }

    int len1 = 0, len2 = 0;
    len1 = strlen(s1);
    len2 = strlen(s2);

    if (len1 >= len2)
    {
        return strncmp(s1, s2, len1);
    }
    else
    {
        return strncmp(s1, s2, len2);
    }
}

void ipaddr2str(char* buffer, unsigned long ipaddr)
{
    int     addr_byte[4] = {0};
    int     i = 0;
    unsigned long   xbyte;

    for (i = 0; i < 4; i++)
    {
        xbyte = ipaddr >> (i * 8);
        xbyte = xbyte & (unsigned long)0x000000FF;
        addr_byte[i] = xbyte;
    }

    snprintf(buffer, 16, "%u.%u.%u.%u", addr_byte[3], addr_byte[2],
             addr_byte[1], addr_byte[0]);
    return;
}

int  is_private_ip(char* ipaddr)
{
    if (ipaddr == NULL)
    {
        return 0;
    }

    if (0 != strncmp(ipaddr, "192.168", 7)
        && 0 != strncmp(ipaddr, "10.", 3)
        && 0 != strncmp(ipaddr, "172.16.", 7)
        && 0 != strncmp(ipaddr, "172.17.", 7)
        && 0 != strncmp(ipaddr, "172.18.", 7)
        && 0 != strncmp(ipaddr, "172.19.", 7)
        && 0 != strncmp(ipaddr, "172.20.", 7)
        && 0 != strncmp(ipaddr, "172.21.", 7)
        && 0 != strncmp(ipaddr, "172.22.", 7)
        && 0 != strncmp(ipaddr, "172.23.", 7)
        && 0 != strncmp(ipaddr, "172.24.", 7)
        && 0 != strncmp(ipaddr, "172.25.", 7)
        && 0 != strncmp(ipaddr, "172.26.", 7)
        && 0 != strncmp(ipaddr, "172.27.", 7)
        && 0 != strncmp(ipaddr, "172.28.", 7)
        && 0 != strncmp(ipaddr, "172.29.", 7)
        && 0 != strncmp(ipaddr, "172.30.", 7)
        && 0 != strncmp(ipaddr, "172.31.", 7)
        && 0 != strncmp(ipaddr, "169.254", 7))
    {
        return 0;  /* PUBLIC */
    }

    return 1;  /* PRIVATE */
}

int good_ipaddr(char* addr)
{
    int dot_count;
    int digit_count;

    if (addr == NULL)
    {
        return -1;
    }

    dot_count = 0;
    digit_count = 0;

    while (*addr != '\0' && *addr != ' ')
    {
        if (*addr == '.')
        {
            dot_count++;
            digit_count = 0;
        }
        else if (!isdigit(*addr))
        {
            dot_count = 5;
        }
        else
        {
            digit_count++;

            if (digit_count > 3)
            {
                dot_count = 5;
            }
        }

        addr++;
    }

    if (dot_count != 3)
    {
        return(-1);
    }
    else
    {
        return(0);
    }
}

int good_host(char* addr)
{
    int dot_count;
    int sect_count;

    if (addr == NULL)
    {
        return -1;
    }

    dot_count = 0;
    sect_count = 0;

    while (*addr != '\0' && *addr != ' ')
    {
        if (*addr == '.')
        {
            dot_count++;

            if (dot_count != 1 && sect_count == 0)
            {
                dot_count = 0;
                break;
            }

            sect_count = 0;
        }
        else if (!isprint(*addr))
        {
            dot_count = 0;
            break;
        }
        else
        {
            sect_count++;
        }

        addr++;
    }

    if (dot_count == 0)
    {
        return(-1);
    }

    if (sect_count == 0)
    {
        return(-1);
    }

    return(0);
}

unsigned int get_ipaddr(char* host)
{
    struct hostent*  hp = NULL;

    if (good_ipaddr(host) == 0)
    {
        return(ntohl(inet_addr(host)));
    }
    else if ((hp = gethostbyname(host)) == (struct hostent*)NULL)
    {
        return((unsigned int)0);
    }

    return(ntohl(*(unsigned int*)hp->h_addr));
    //return((*(UINT4 *)hp->h_addr));
}

int inet_type(char* host)
{
    if (host == NULL)
    {
        return -1;
    }

    if (!good_ipaddr(host))
    {
        /* ip address */
        if (is_private_ip(host))
        {
            return 0;
        }

        return 1;
    }
    else if (!good_host(host))
    {
        /* url */
        unsigned long ipaddr = 0;
        char ipstr[16] = {0};
        ipaddr = get_ipaddr(host);
        ipaddr2str(ipstr, ipaddr);

        if (is_private_ip(ipstr))
        {
            return 0;
        }

        return 1;
    }

    return -1;
}

int is_host_ip(char* host, char* ipaddr)
{
    int pos;
    struct hostent*  hp = NULL;
    unsigned long    ip;

    if (host == NULL || ipaddr == NULL)
    {
        return 0;
    }

    if (good_ipaddr(ipaddr) != 0)
    {
        return 0;
    }

    if (good_ipaddr(host) == 0)
    {
        if (sstrcmp(host, ipaddr) == 0)
        {
            return 1;
        }
    }

    if (good_host(host) != 0)
    {
        return 0;
    }

    ip = inet_addr(ipaddr);

    if ((hp = gethostbyname(host)) == (struct hostent*)NULL)
    {
        return 0;
    }

    pos = 0;

    while (hp->h_addr_list[pos] != NULL)
    {
        if ((*(unsigned long*)hp->h_addr_list[pos]) == ip)
        {
            return 1;
        }

        pos++;
    }

    return 0;
}

int host_match(char* host1, char* host2)
{
    int host1_type = 0; /* 0:not legal. 1: ip address 2: url */
    int host2_type = 0;

    if (host1 == NULL || host2 == NULL)
    {
        return -1;
    }

    if (good_ipaddr(host1))
    {
        if (good_host(host1))
        {
            host1_type = 0;
        }
        else
        {
            host1_type = 2;
        }
    }
    else
    {
        host1_type = 1;
    }

    if (good_ipaddr(host2))
    {
        if (good_host(host2))
        {
            host2_type = 0;
        }
        else
        {
            host2_type = 2;
        }
    }
    else
    {
        host2_type = 1;
    }

    if (!host1_type || !host2_type)
    {
        return -1;
    }

    if (host1_type == 2 && host2_type == 1)
    {
        if (is_host_ip(host1, host2))
        {
            return 0;
        }
    }
    else if (host1_type == 1 && host2_type == 1)
    {
        if (is_host_ip(host2, host1))
        {
            return 0;
        }
    }
    else
    {
        /* only compare string */
        return sstrcmp(host1, host2);
    }

    return -1;
}

int printf_system_time1()
{
    time_t utc_time;
    struct tm local_time = { 0 };
    char str_date[12] = {0};
    char str_time[12] = {0};
    char str_head[32] = {0};
#ifndef WIN32
    utc_time = time(NULL);
    localtime_r(&utc_time, &local_time);

    strftime(str_date, sizeof(str_date), "%Y-%m-%d", &local_time);
    strftime(str_time, sizeof(str_time), "%H:%M:%S", &local_time);
    snprintf(str_head, 32, "%sT%s", str_date, str_time);
    printf(" time=%s \r\n", str_head);
#endif
    return 0;
}

unsigned int analysis_time1(char* strTime)
{
    char* tmp = NULL;

    char pcDate[11] = {0};
    char pcTime[9] = {0};

    char pcYear[5] = {0};
    char pcMonthAndDay[6] = {0};
    char pcMonth[3] = {0};
    char pcDay[3] = {0};

    char pcHour[3] = {0};
    char pcMinuteAndSecond[6] = {0};
    char pcMinute[3] = {0};
    char pcSecond[3] = {0};

    struct tm stm;
    time_t timep;

    if (NULL == strTime)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "analysis_time() exit---: Param Error \r\n");
        return -1;
    }

    tmp = strchr(strTime, 'T');

    if (tmp != NULL)
    {
        osip_strncpy(pcDate, strTime, tmp - strTime);
        osip_strncpy(pcTime, tmp + 1, strTime + strlen(strTime) - tmp - 1);
    }
    else
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "analysis_time() exit---: Analysis T Error \r\n");
        return -1;
    }

    /* 解析日期 */
    tmp = strchr(pcDate, '-');

    if (tmp != NULL)
    {
        osip_strncpy(pcYear, pcDate, tmp - pcDate);
        osip_strncpy(pcMonthAndDay, tmp + 1, pcDate + strlen(pcDate) - tmp - 1);
    }
    else
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "analysis_time() exit---: Analysis Date Error \r\n");
        return -1;
    }

    tmp = strchr(pcMonthAndDay, '-');

    if (tmp != NULL)
    {
        osip_strncpy(pcMonth, pcMonthAndDay, tmp - pcMonthAndDay);
        osip_strncpy(pcDay, tmp + 1, pcMonthAndDay + strlen(pcMonthAndDay) - tmp - 1);
    }
    else
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "analysis_time() exit---: Analysis Date Error \r\n");
        return -1;
    }

    /* 解析时间 */
    tmp = strchr(pcTime, ':');

    if (tmp != NULL)
    {
        osip_strncpy(pcHour, pcTime, tmp - pcTime);
        osip_strncpy(pcMinuteAndSecond, tmp + 1, pcTime + strlen(pcTime) - tmp - 1);
    }
    else
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "analysis_time() exit---: Analysis Time Error \r\n");
        return -1;
    }

    tmp = strchr(pcMinuteAndSecond, ':');

    if (tmp != NULL)
    {
        osip_strncpy(pcMinute, pcMinuteAndSecond, tmp - pcMinuteAndSecond);
        osip_strncpy(pcSecond, tmp + 1, pcMinuteAndSecond + strlen(pcMinuteAndSecond) - tmp - 1);
    }
    else
    {
        SIP_DEBUG_TRACE(LOG_ERROR, "analysis_time() exit---: Analysis Time Error \r\n");
        return -1;
    }

    /* 转换成time_t */
    stm.tm_year = osip_atoi(pcYear) - 1900;
    stm.tm_mon = osip_atoi(pcMonth) - 1;
    stm.tm_mday = osip_atoi(pcDay);

    stm.tm_hour = osip_atoi(pcHour);
    stm.tm_min = osip_atoi(pcMinute);
    stm.tm_sec = osip_atoi(pcSecond);

    timep = mktime(&stm);

    return timep;
}