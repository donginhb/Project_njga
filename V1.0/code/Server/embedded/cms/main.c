#ifdef WIN32
#include <windows.h>
#include <winsock.h>
#include <sys/types.h>
#else
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#endif
#include <pthread.h>
#include <sched.h>
#include <signal.h>
#include <math.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/utsname.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <net/ethernet.h>
#include <netinet/ip.h>
#include <netinet/in.h>
#include <unistd.h>
#include <dirent.h>

#include "libsip.h"

#include "common/gbldef.inc"
#include "common/ppl_getopt.inc"
#include "common/init_proc.inc"
#include "common/callback_proc.inc"
#include "common/gblconfig_proc.inc"
#include "common/gblfunc_proc.inc"
#include "common/db_proc.h"
#include "common/log_proc.inc"
#include "common/common_thread_proc.inc"
#include "common/systeminfo_proc.inc"


#include "config/telnetd.inc"

#include "platformms/BoardInit.h"
#include "platformms/CPing.h"
#include "device/device_info_mgn.inc"
#include "route/route_info_mgn.inc"
#include "device/device_thread_proc.inc"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "dead_lock_stub.h"

int cms_run_status = 0;     /* 0:û������,1:�������� */

int stop_all = 0;
int restart = 0;

extern gbl_conf_t* pGblconf;                  /* ȫ��������Ϣ */
extern char log_file_time[20];

void start_EV9000MediaService()
{
    int i = 0;
    int iRet = 0;
    int pid_t[128] = {0};
    int pid = 0;

    /* ��ȡ���̵�pid */
    iRet = find_pid_by_name(((char*)"EV9000MediaService"), pid_t);

    if (!iRet)
    {
        for (i = 0; pid_t[i] != 0; i++)
        {
            pid = pid_t[i];

            break;
        }
    }

    if (pid == 0)
    {
        system((char*)"/app/EV9000MediaService > /dev/null &");
    }

}

/*****************************************************************************
 �� �� ��  : check_sys_has_2_disk_partion
 ��������  : ���ϵͳ�Ƿ���ڵڶ���Ӳ�̷���
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2016��11��28��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int check_sys_has_2_disk_partion()
{
    FILE *fstream = NULL;
    char buff[1024] = {0};
    memset(buff, 0, sizeof(buff));

    if (NULL == (fstream = popen("blkid | grep sda2", "r")))
    {
        printf("check_sys_has_2_disk_partion(): execute command failed: %s \r\n", strerror(errno));
        return 0;
    }

    if (NULL != fgets(buff, sizeof(buff), fstream))
    {
        if (buff[0] != '\0')
        {
            printf("check_sys_has_2_disk_partion(): shell return=%s \r\n", buff);
            pclose(fstream);
            return 1;
        }
        else
        {
            printf("check_sys_has_2_disk_partion(): shell return NULL \r\n");
        }
    }
    else
    {
        printf("check_sys_has_2_disk_partion(): fgets Error \r\n");
    }

    pclose(fstream);
    return 0;
}

/*****************************************************************************
 �� �� ��  : check_log_dir_is_exist
 ��������  : ���ϵͳĿ¼�����Ƿ����
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2016��11��28��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int check_log_dir_is_exist()
{
    int i = 0;
    DIR* dir = NULL;

    /* ���Ŀ¼�Ƿ���� */
    dir = opendir(LOGFILE_DIR);

    if (NULL == dir)
    {
        i = mkdir(LOGFILE_DIR, 0775);

        if (i != 0)
        {
            printf("check_log_dir_is_exist() mkdir /data/log Error \r\n");
            return 0;
        }
    }

    printf("check_log_dir_is_exist() mkdir /data/log Exist \r\n");
    closedir(dir);
    dir = NULL;
    return 1;
}

/*****************************************************************************
 �� �� ��  : check_Master_Live
 ��������  : ��������Ƿ���
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2016��8��2��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int check_Master_Live()
{
    int i = 0;
    CPing tmpcPingClass((char*)"10.168.0.101", 0);

    for (i = 1; i <= 5; i++)
    {
        if (0 != tmpcPingClass.ping(5))
        {
            if (tmpcPingClass.GetTimeOut() <= 2)
            {
                printf("check_Master_Live() ��������:10.168.0.101 ����, ��%d��Ping \r\n", i);
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "����������:10.168.0.101 ����, ��%d��Ping \r\n", i);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "The host network detection: 10.168.0.101 is normal \r\n");
                return 1;
            }
            else
            {
                printf("check_Master_Live() ��������:10.168.0.101 �쳣, ��%d��Ping \r\n", i);
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "����������:10.168.0.101 �쳣, ��%d��Ping \r\n", i);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "The host network detection: 10.168.0.101 anomalies \r\n");
                osip_usleep(1000000);
            }
        }
        else
        {
            printf("check_Master_Live() ��������:10.168.0.101 Ping ʧ��, ��%d��Ping \r\n", i);
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "����������:10.168.0.101 Ping ʧ��, ��%d��Ping \r\n", i);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "The host network detection: 10.168.0.101 Ping fails, abnormal \r\n");
            osip_usleep(1000000);
        }
    }

    return 0;
}

/*****************************************************************************
 �� �� ��  : check_Master_Live1
 ��������  : ��������Ƿ���
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2016��11��18��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int check_Master_Live1()
{
    int i = 0;
    char s8Ip[IP_STR_LEN] = {0};

    osip_strncpy(s8Ip, inet_ntoa(*(struct in_addr*)&gt_MSSwitchAttr.tMatchConnetIP.dwIPAddr), IP_STR_LEN);

    printf("check_Master_Live1() ��������IP��ַ=%s \r\n", s8Ip);

    CPing tmpcPingClass(s8Ip, 0);

    for (i = 1; i <= 5; i++)
    {
        if (0 != tmpcPingClass.ping(5))
        {
            if (tmpcPingClass.GetTimeOut() <= 2)
            {
                printf("check_Master_Live1() ��������:%s ����, ��%d��Ping \r\n", s8Ip, i);
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "����������:%s ����, ��%d��Ping \r\n", s8Ip, i);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "The host network detection:%s is normal \r\n", s8Ip);
                return 1;
            }
            else
            {
                printf("check_Master_Live1() ��������:%s �쳣, ��%d��Ping \r\n", s8Ip, i);
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "����������:10.168.0.101 �쳣, ��%d��Ping \r\n", s8Ip, i);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "The host network detection:%s anomalies \r\n", s8Ip);
                osip_usleep(1000000);
            }
        }
        else
        {
            printf("check_Master_Live1() ��������:%s Ping ʧ��, ��%d��Ping \r\n", s8Ip, i);
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "����������:%s Ping ʧ��, �쳣, ��%d��Ping \r\n", s8Ip, i);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "The host network detection:%s Ping fails, abnormal \r\n", s8Ip);
            osip_usleep(1000000);
        }
    }

    return 0;
}

/*****************************************************************************
 �� �� ��  : check_Slave_Live
 ��������  : ��ⱸ���Ƿ���
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2016��8��2��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int check_Slave_Live()
{
    CPing tmpcPingClass((char*)"10.168.0.102", 0);

    if (0 != tmpcPingClass.ping(5))
    {
        if (tmpcPingClass.GetTimeOut() <= 2)
        {
            printf("check_Slave_Live() ��������:10.168.0.102 ���� \r\n");
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "����������:10.168.0.102 ���� \r\n");
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Stand-by network detection: 10.168.0.102 is normal\r\n");
            return 1;
        }
        else
        {
            printf("check_Slave_Live() ��������:10.168.0.102 �쳣 \r\n");
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "����������:10.168.0.102 �쳣 \r\n");
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Stand-by network detection: 10.168.0.102 anomalies \r\n");
            return 0;
        }
    }
    else
    {
        printf("check_Slave_Live() ��������:10.168.0.102 Ping ʧ�� \r\n");
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "����������:10.168.0.102 Ping ʧ��, �쳣 \r\n");
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Stand-by network detection: 10.168.0.102 Ping fails, an exception\r\n");
        return 0;
    }
}

/*****************************************************************************
 �� �� ��  : check_Slave_Live1
 ��������  : ��ⱸ���Ƿ���
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2016��11��18��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int check_Slave_Live1()
{
    char s8Ip[IP_STR_LEN] = {0};

    osip_strncpy(s8Ip, inet_ntoa(*(struct in_addr*)&gt_MSSwitchAttr.tMatchConnetIP.dwIPAddr), IP_STR_LEN);

    printf("check_Slave_Live1() ��������IP��ַ=%s \r\n", s8Ip);

    CPing tmpcPingClass(s8Ip, 0);

    if (0 != tmpcPingClass.ping(5))
    {
        if (tmpcPingClass.GetTimeOut() <= 2)
        {
            printf("check_Slave_Live1() ��������:%s ���� \r\n", s8Ip);
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "����������:%s ���� \r\n", s8Ip);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Stand-by network detection:%s is normal\r\n", s8Ip);
            return 1;
        }
        else
        {
            printf("check_Slave_Live1() ��������:%s �쳣 \r\n", s8Ip);
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "����������:%s �쳣 \r\n", s8Ip);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Stand-by network detection:%s anomalies \r\n", s8Ip);
            return 0;
        }
    }
    else
    {
        printf("check_Slave_Live1() ��������:%s Ping ʧ�� \r\n", s8Ip);
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "����������:%s Ping ʧ��, �쳣 \r\n", s8Ip);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Stand-by network detection:%s Ping fails, an exception\r\n", s8Ip);
        return 0;
    }
}

/*****************************************************************************
 �� �� ��  : check_Common_IP_Live
 ��������  : ��⹫��IP��ַ�Ƿ�����ʹ��
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2016��8��2��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int check_Common_IP_Live()
{
    char tmps8Ip[16] = {0};

    if (g_BoardNetConfig.tCmsCommonVideoIP.tNetIP.dwIPAddr == g_BoardNetConfig.tCmsVideoIP.tNetIP.dwIPAddr)
    {
        printf("check_Common_IP_Live() ����IP��ַ�ͱ�����ַ������ͬ, �����ж� \r\n");
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "����IP��ַ���:����IP��ַ�ͱ�����ַ������ͬ, �����ж� \r\n");
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Public IP address detection: the public IP address is the same as the address of the machine configuration, without judgment\r\n");
        return 0;
    }

    memset(tmps8Ip, 0, sizeof(tmps8Ip));
    osip_strncpy(tmps8Ip, inet_ntoa(*(struct in_addr*)&g_BoardNetConfig.tCmsCommonVideoIP.tNetIP.dwIPAddr), 16);

    if (IsIPAddrIsLocalIP((char*)g_EthType[g_BoardNetConfig.tCmsCommonVideoIP.dwEth], tmps8Ip))
    {
        printf("check_Common_IP_Live() ����IP��ַ�ͱ�����ַ��ͬ, �����ж� \r\n");
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "����IP��ַ���:����IP��ַ�ͱ�����ַ������ͬ, �����ж� \r\n");
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Public IP address detection: the public IP address is the same as the address of the machine configuration, without judgment\r\n");
        return 0;
    }

    CPing  tmpcPingClass(tmps8Ip, 0);

    if (0 != tmpcPingClass.ping(5))
    {
        if (tmpcPingClass.GetTimeOut() <= 2)
        {
            printf("check_Common_IP_Live() ����IP��ַ=%s, �������� \r\n", tmps8Ip);
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "����IP��ַ���:����IP��ַ=%s, �������� \r\n", tmps8Ip);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Public IP address detection: = % s public IP address, network is normal\r\n");
            return 1;
        }
        else
        {
            printf("check_Common_IP_Live() ����IP��ַ=%s, �����쳣 \r\n", tmps8Ip);
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "����IP��ַ���:����IP��ַ=%s, �����쳣 \r\n", tmps8Ip);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Public IP address detection: = % s public IP address, network anomalies \r\n");
            return 0;
        }
    }
    else
    {
        printf("check_Common_IP_Live() ����IP��ַ=%s, Ping ʧ�� \r\n", tmps8Ip);
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "����IP��ַ���:����IP��ַ=%s, Ping ʧ��, �����쳣 \r\n", tmps8Ip);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Public IP address detection: the public IP address = % s, Ping failure, network anomalies\r\n");
        return 0;
    }

    return 1;
}

/*****************************************************************************
 �� �� ��  : set_video_eth_ip
 ��������  : ������Ƶ������ip��ַ
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2016��8��2��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
void set_video_eth_ip()
{
    char  s8Ip[IP_STR_LEN] = {0};
    char  s8Netmask[IP_STR_LEN] = {0};
    char  s8Gateway[IP_STR_LEN] = {0};
    char strSetAddr[256] = {0};

    osip_strncpy(s8Ip, inet_ntoa(*(struct in_addr*)&g_BoardNetConfig.tCmsVideoIP.tNetIP.dwIPAddr), IP_STR_LEN);
    osip_strncpy(s8Netmask, inet_ntoa(*(struct in_addr*)&g_BoardNetConfig.tCmsVideoIP.tNetIP.dwIPMask), IP_STR_LEN);
    osip_strncpy(s8Gateway, inet_ntoa(*(struct in_addr*)&g_BoardNetConfig.tCmsVideoIP.tNetIP.dwGetway), IP_STR_LEN);

    snprintf(strSetAddr, 256, "KingStoreConsole --show-result --set-addr --device=%s --ip=%s --netmask=%s --gateway=%s", g_EthType[g_BoardNetConfig.tCmsVideoIP.dwEth], s8Ip, s8Netmask, s8Gateway);

    printf("\n set_video_eth_ip(): strSetAddr = %s\n", strSetAddr);

    system(strSetAddr); /* �������������Ҫ��������Ч */

    ConfigEth(g_EthType[g_BoardNetConfig.tCmsVideoIP.dwEth], &g_BoardNetConfig.tCmsVideoIP.tNetIP, 1);


    return ;
}

/*****************************************************************************
 �� �� ��  : set_common_ip
 ��������  : ���������������������IP��ַ
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2016��8��2��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
void set_common_ip()
{
    int iRet = 0;

    char  s8Ip[IP_STR_LEN] = {0};
    char  s8Netmask[IP_STR_LEN] = {0};
    char  s8Gateway[IP_STR_LEN] = {0};
    char strSetAddr[256] = {0};

    osip_strncpy(s8Ip, inet_ntoa(*(struct in_addr*)&g_BoardNetConfig.tCmsCommonVideoIP.tNetIP.dwIPAddr), IP_STR_LEN);
    osip_strncpy(s8Netmask, inet_ntoa(*(struct in_addr*)&g_BoardNetConfig.tCmsCommonVideoIP.tNetIP.dwIPMask), IP_STR_LEN);
    osip_strncpy(s8Gateway, inet_ntoa(*(struct in_addr*)&g_BoardNetConfig.tCmsCommonVideoIP.tNetIP.dwGetway), IP_STR_LEN);

    snprintf(strSetAddr, 256, "KingStoreConsole --show-result --set-addr --device=%s --ip=%s --netmask=%s --gateway=%s", g_EthType[g_BoardNetConfig.tCmsCommonVideoIP.dwEth], s8Ip, s8Netmask, s8Gateway);

    printf("\n set_common_ip(): strSetAddr = %s\n", strSetAddr);

    system(strSetAddr); /* �������������Ҫ��������Ч */

    ConfigEth(g_EthType[g_BoardNetConfig.tCmsCommonVideoIP.dwEth], &g_BoardNetConfig.tCmsCommonVideoIP.tNetIP, 1);

    return ;
}

void usage(int code)
{
    printf("\n\
usage:\n\
    \n\
    cms [-v]\n\
    \n\
    [-h]                  print this help.\n\
    [-v]                  print cms version info.\n\n");
    exit(code);

    return;
}

int process_opt(int argc, char** argv)
{
    char c;
    ppl_getopt_t* opt = NULL;
    ppl_status_t  rv;
    const char*   optarg = NULL;

    /**/
    if (argc > 1 && strlen(argv[1]) == 1 && 0 == strncmp(argv[1], "-", 2))
    {
        usage(0);
    }

    if (argc > 1 && strlen(argv[1]) >= 2 && 0 == strncmp(argv[1], "--", 2))
    {
        usage(0);
    }

    ppl_getopt_init(&opt, argc, argv);

#define __APP_BASEARGS "vVh?"

    while ((rv = ppl_getopt(opt, __APP_BASEARGS, &c, &optarg)) == PPL_SUCCESS)
    {
        switch (c)
        {
            case 'f':
                //sclrspace(optarg);
                //snprintf(maincfg.config_file, 255, optarg);
                break;

            case 'k':
                //sclrspace(optarg);
                //snprintf(maincfg.key_file, 255, optarg);
                break;

            case 'c':
                //sclrspace(optarg);
                //snprintf(maincfg.console2gics_file, 255, optarg);
                break;

            case 'd':
                //sclrspace(optarg);
                /*i = strlen(optarg);
                if (optarg[i-1]!='/')
                {
                    snprintf(maincfg.db_dir, 255, "%s/", optarg);
                }
                else
                {
                    snprintf(maincfg.db_dir, 255, optarg);
                }*/
                break;

            case 'l':
                //sclrspace(optarg);
                /*i = strlen(optarg);
                if (optarg[i-1]!='/')
                {
                    snprintf(maincfg.log_dir, 255,  "%s/", optarg);
                }
                else
                {
                    snprintf(maincfg.log_dir, 255, optarg);
                }*/
                break;

            case 'v':
            case 'V':
                printf("cms version:     %s\n", SYS_VERSION);
                printf("build: %s\n", server_built);
                free((void*)(opt->argv));
                free((void*) opt);
                exit(0);

            case 'h':
            case '?':
                printf("cms version:     %s\n", SYS_VERSION);
                printf("build: %s\n", server_built);
                usage(0);
                break;

            default:
                usage(1);
        }
    }

    free((void*)(opt->argv));
    free((void*) opt);

    if (rv != PPL_EOF)
    {
        usage(1);
    }

    return 0;
}

void onsignal(int sig)
{
    switch (sig)
    {
        case SIGINT:
            stop_all = 1;
            break;

        case SIGTRAP:
            //printf("\r\n SIGTRAP\n");
            break;

        case SIGPIPE:
            //printf("\r\n SIGPIPE\n");
            break;

        case 32:
            //printf("\r\n SIG32\n");
            break;

        case 33:
            //printf("\r\n SIG33\n");
            break;
    }

    return;
}

void cms_time_count(int sig)
{
    SIP_1sTimerNotify();
    return;
}

int set_SignalProc()
{
    signal(SIGINT,  &onsignal);
    signal(SIGTRAP, &onsignal);
    signal(SIGPIPE, &onsignal);
    signal(33, SIG_IGN);

#if 0
    signal(SIGALRM, cms_time_count);

    struct itimerval val;
    val.it_interval.tv_sec = 1;
    val.it_interval.tv_usec = 0;
    val.it_value.tv_sec = 1;
    val.it_value.tv_usec = 0;

    setitimer(ITIMER_REAL, &val, NULL);
#endif
    return 0;
}

void init_globals()
{
    restart = 0;
    stop_all = 0;

    return;
}

void do_restart()
{
    restart = 1;
    stop_all = 1;

    return;
}

char* get_sysinfo()
{
    static char ss[128] = {0};
    struct utsname info;

    if (uname(&info) < 0) /* error */
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "get_sysinfo() exit---:1 \r\n");
        return NULL;
    }

    snprintf(ss, 127,  "%s-%s-%s-%s", info.sysname, info.release, info.machine, info.version);
    return ss;
}

char* get_versioninfo()
{
    static char ss[128] = {0};
    snprintf(ss, 127,  "%s Build:%s", SYS_VERSION, server_built);
    return ss;
}

int main_loop()
{
    int iRet = 0;
    int send_flag = 0;
    static int check_interval = 0;
    time_t ntptime = time(NULL);
    time_t db_err_time = time(NULL);
    time_t check_disk_time = time(NULL);
    time_t send_time = 0;

    while (!stop_all)
    {
        /* NTPУʱ���� */
        if (NULL != pGblconf && pGblconf->ntp_server_ip[0] != '\0')
        {
            if (time(NULL) - ntptime > 1800)
            {
                //memset(strNTPSys, 0, 128);
                //snprintf(strNTPSys, 128, "ntpdate %s", pGblconf->ntp_server_ip);
                //system(strNTPSys);
                //system("hwclock -wu");
                system((char*)"killall ntpd; killall -9 ntpd");
                ntptime = time(NULL);
            }
        }

#if 0

        /* �Դ�ý�����س����� */
        if (time(NULL) - Med_time > 60)
        {
            start_EV9000MediaService();
            Med_time = time(NULL);
        }

#endif

        /* ��������µ����ݿ�ͬ�� */
        if (g_BoardNetConfig.dwMSFlag == 1) /* �������� */
        {
            if (g_BoardNetConfig.st_MSFlag == 2) /* �����豸 */
            {
                /* ������豸�Ƿ������ˣ�����֮�󣬻������������豸 */
                if (check_Master_Live())
                {
                    /* �������̣��Զ���Ϊ���� */
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "������⵽�����������ߴ��Զ��лر���");
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Live for machine testing to the host comes back online, automatic cut back to the standby machine");
                    osip_usleep(30000000); /* ��ʱ30��ִ�� */

                    system((char*)"killall cms; killall -9 cms");
                }
            }
        }

        /* ɾ�����ݿ������־ */
        if (time(NULL) - db_err_time > 3600)
        {
            system((char*)"rm -rf /data/db/*.err");
            db_err_time = time(NULL);
        }

        osip_usleep(1000000);
    }

    return restart;
}

// ***BEGIN***  д�����ļ� wangqichao 2013/9/2 add
int WriteInitFile()
{
    char initstr[1000] = {0};
    unsigned int nLen = 0;

    FILE* fd;

    if ((fd = fopen("/config/DataBus-HA.cfg", "w+")) == NULL)
    {
        return -1;
    }

    fseek(fd, 0, SEEK_END);


    memset(initstr, 0, 1000);
    sprintf(initstr, "starter:tcp -h 127.0.0.1 -p 21900\n");
    nLen = strlen(initstr);
    fwrite(initstr, sizeof(char), nLen, fd);

    memset(initstr, 0, 1000);
    sprintf(initstr, "/data/TMPEV9000DB\n");
    nLen = strlen(initstr);
    fwrite(initstr, sizeof(char), nLen, fd);


    memset(initstr, 0, 1000);
    sprintf(initstr, "starter:tcp -h %s -p 21900\n", inet_ntoa(*(struct in_addr*) & (gt_MSSwitchAttr.tMatchConnetIP.dwIPAddr)));
    nLen = strlen(initstr);
    fwrite(initstr, sizeof(char), nLen, fd);


    memset(initstr, 0, 1000);
    sprintf(initstr, "/data/EV9000DB\n");
    nLen = strlen(initstr);
    fwrite(initstr, sizeof(char), nLen, fd);

    fclose(fd);

    fd = NULL;

    if ((fd = fopen("/config/DataBus.cfg", "w+")) == NULL)
    {
        return -1;
    }

    fseek(fd, 0, SEEK_END);


    memset(initstr, 0, 1000);
    sprintf(initstr, "starter:tcp -h 127.0.0.1 -p 21900\n");
    nLen = strlen(initstr);
    fwrite(initstr, sizeof(char), nLen, fd);

    memset(initstr, 0, 1000);
    sprintf(initstr, "/data/TMPEV9000DB\n");
    nLen = strlen(initstr);
    fwrite(initstr, sizeof(char), nLen, fd);

    memset(initstr, 0, 1000);
    sprintf(initstr, "/app/filelog_HA\n");
    nLen = strlen(initstr);
    fwrite(initstr, sizeof(char), nLen, fd);

    fclose(fd);
    return 0;

}
// ***END***  д�����ļ� wangqichao 2013/9/2 add

// ***BEGIN***  EV9000-CMS-�ж��Ƿ���ʵ�������� wangqichao 2013/6/3 add
/*****************************************************************************
 Prototype    : WriteLock
 Description  : �ж��ļ��Ƿ�д����
 Input        : int fd
 Output       : None
 Return Value : int
 Calls        :
 Called By    :

  History        :
  1.Date         : 2013/6/19
    Author       : wangqichao
    Modification : Created function

*****************************************************************************/
int WriteLock(int fd)
{
    //��ʼ���ļ��ṹ��
    /* File record locking structure used by fcntl() */
    struct flock flk;

    flk.l_type   = F_WRLCK;  /* F_RDLCK, F_WRLCK, or F_UNLCK */
    flk.l_whence = SEEK_SET; /* SEEK_SET, SEEK_CUR, SEEK_END */
    flk.l_start  = 0;        /* starting offset relative to l_whence */
    flk.l_len = 0;        /* len == 0 means "until end of file" */
    flk.l_pid = getpid();    /* Process ID of the process holding the
                               lock, returned with F_GETLK */

    return fcntl(fd, F_SETLK, &flk);
}

/*****************************************************************************
 Prototype    : CheckSingletonRunning
 Description  : ����߳��Ƿ�������
 Input        : None
 Output       : None
 Return Value : bool
 Calls        :
 Called By    :

  History        :
  1.Date         : 2013/6/19
    Author       : wangqichao
    Modification : Created function

*****************************************************************************/
bool CheckSingletonRunning()
{
    //��ʼ���ֲ�����
    char strPIDFileName[200] = {0};
    int fd = -1;
    char szInstanceName[200] = "cms";
    char wordir[200] = "/config";

    //getcwd(wordir, 200);


#define FILE_MODE S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP

    sprintf(strPIDFileName, "%s/.%s_pidfile",
            wordir, szInstanceName);

    if ((fd = open(strPIDFileName, O_RDWR | O_CREAT, FILE_MODE)) < 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "CheckSingletonRunning() exit---: Open File Error \r\n");
        return false;
    }

    //�ж��ļ��ǲ�����д��������г����Ѿ����У��������û������
    if (WriteLock(fd) < 0)
    {
        if ((errno == EAGAIN)
            || (errno == EACCES))
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_FATAL, "CheckSingletonRunning() [%s] Another instance running...  Exit now!!!\n", szInstanceName);
        }
        else
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_FATAL, "CheckSingletonRunning() [%s] Can't lock the file %s.\n", szInstanceName, strPIDFileName);
        }

        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "CheckSingletonRunning() exit---: Write Lock Error \r\n");
        return false;
    }

    return true;
}
// ***END***  EV9000-CMS-�ж��Ƿ���ʵ�������� wangqichao 2013/6/3 add
int CmsHaKeep(int HAFlag)
{
    if (0 == HAFlag)
    {
        printf("\r\n HAFlag = 0 \r\n");
        return 0;
    }

    while (gt_MSSwitchAttr.dwCMSUsedStatus)
    {
        osip_usleep(5000);
    }

    time_t  pHA_time = time(NULL);

    while (time(NULL) - pHA_time < 5)
    {
        osip_usleep(1000);
    }

    return 1;
}

int main(int argc, char* argv[])
{
    int iRet = 0;
    time_t now = time(NULL);
    time_t MS_Bakup1 = 0;
    time_t MS_Bakup2 = 0;
    struct tm tp = {0};
    char strCmd[256] = {0};

    process_opt(argc, argv);

main_begin:
    printf("\r\n ############################################## \
    \r\n #        CMS Begin Start: Init Data          # \
    \r\n ############################################## \r\n");

    // *) ��Ӹ��У� ��ʾ����������⹦��
    //DeadLockGraphic::getInstance().start_check();

#if 0

    /* ��Ȩ�ļ���� */
    iRet = checklicence();

    if (iRet == -1)
    {
        printf("Please Register the iEV9000 RX !!!\n");
        return -1;
    }
    else if (iRet == 1)
    {
        printf("iEV9000 RX License File out of date, Please Register it!!!\n");
        return -1;
    }

#endif

    localtime_r(&now, &tp);
    memset(log_file_time, 0, 20);
    strftime(log_file_time, 20, "%Y_%m_%d_%H_%M_%S", &tp);

    system((char*)"chmod -R 777 /config");
    system((char*)"chmod -R 777 /data/log");

    /* ɾ�����ݿ��ϵͳ������־ */
    system((char*)"rm -rf /data/db/*.err");

    /*  ��־�ļ���ʼ��*/
    printf("\r\n main() Log2FileInit() Begin--- \r\n");
    iRet = Log2FileInit();
    printf(" main() Log2FileInit() End--- iRet=%d \r\n", iRet);

    /* ��־��¼���ļ��ļ����г�ʼ��*/
    printf("\r\n main() trace_log2file_list_init() Begin--- \r\n");
    iRet = trace_log2file_list_init();
    printf(" main() trace_log2file_list_init() End--- iRet=%d \r\n", iRet);

    /* sip��Ϣ��¼���ļ����г�ʼ��*/
    printf("\r\n main() sip_msg_log2file_list_init() Begin--- \r\n");
    iRet = sip_msg_log2file_list_init();
    printf(" main() sip_msg_log2file_list_init() End--- iRet=%d \r\n", iRet);

    /* ϵͳ��־��¼�����ݿ���г�ʼ��*/
    printf("\r\n main() system_log2db_list_init() Begin--- \r\n");
    iRet = system_log2db_list_init();
    printf(" main() system_log2db_list_init() End--- iRet=%d \r\n", iRet);

    /*  ��־��¼���ļ������߳� */
    printf("\r\n main() log2file_proc_thread_start() Begin--- \r\n");
    iRet = log2file_proc_thread_start();
    printf(" main() log2file_proc_thread_start() End--- iRet=%d \r\n", iRet);

    /*  �����ʼ�� */
    printf("\r\n main() Board_Init() Begin--- \r\n");
    iRet = Board_Init();
    printf(" main() Board_Init() End--- iRet=%d \r\n", iRet);

    if (0 != iRet)
    {
        Board_UnInit();
        log2file_proc_thread_stop();
        system_log2db_list_free();
        trace_log2file_list_free();
        sip_msg_log2file_list_free();
        Log2FileFree();
        return -1;
    }

    /* ����״̬��� */
    printf("\r\n ����״̬���, ��ʼ--- \r\n");

    printf("\r\n �������ñ�ʶ:dwMSFlag=%d, ��������ʶ:st_MSFlag=%d \r\n", g_BoardNetConfig.dwMSFlag, g_BoardNetConfig.st_MSFlag);

    if (g_BoardNetConfig.dwMSFlag == 1)  /* �������õ������ */
    {
        set_video_eth_ip(); /* ��ԭ�������ڹ���ip��ַ���û���Ƶ����ַ */

        if (g_BoardNetConfig.st_MSFlag == 2) /* ��������������Ƿ��� */
        {
            printf("\r\n ��ǰ�豸Ϊ��������������Ƿ��� \r\n");

            /* ������������Ƿ��ܹ�pingͨ,����������������������ping 5�� */
            while (check_Master_Live())
            {
                printf("\r\n ��ǰ�豸Ϊ��������⵽����״̬���� \r\n");

                /* ����������ʱ��ִ��һ�����ݿ�ͬ��,����ÿСʱͬ��һ�� */
                if (0 == MS_Bakup1 || time(NULL) - MS_Bakup1 > 3600)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "ִ���������ݿⱸ��, ��ʼ---");
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "The main standby database backup, began to -- -- -");

                    memset(strCmd, 0, 256);
                    snprintf(strCmd, 256, "/app/BakeupDB.sh %s %s", (char*)"10.168.0.101", (char*)"10.168.0.102");
                    printf("\r\n EV9000DB���ݿⱸ������:%s \r\n", strCmd);
                    system(strCmd);

                    memset(strCmd, 0, 256);
                    snprintf(strCmd, 256, "/app/BakeupDB_mobile.sh %s %s", (char*)"10.168.0.101", (char*)"10.168.0.102");
                    printf("\r\n EV9000DB_MOBILE���ݿⱸ������:%s \r\n", strCmd);
                    system(strCmd);

                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "ִ���������ݿⱸ��, ����---");
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "The main standby database backup, end -");
                    MS_Bakup1 = time(NULL);
                }

                osip_usleep(1000000);
            }

            /* �ٴ�pingһ�¹���IP��ַ */
            while (check_Common_IP_Live())
            {
                printf("\r\n ��ǰ�豸Ϊ��������⵽����IP��ַ״̬���� \r\n");

                /* ����������ʱ��ִ��һ�����ݿ�ͬ��,����ÿСʱͬ��һ�� */
                if (0 == MS_Bakup1 || time(NULL) - MS_Bakup1 > 3600)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "ִ���������ݿⱸ��, ��ʼ---");
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "The main standby database backup, began to -- -- -");

                    memset(strCmd, 0, 256);
                    snprintf(strCmd, 256, "/app/BakeupDB.sh %s %s", (char*)"10.168.0.101", (char*)"10.168.0.102");
                    printf("\r\n EV9000DB���ݿⱸ������:%s \r\n", strCmd);
                    system(strCmd);

                    memset(strCmd, 0, 256);
                    snprintf(strCmd, 256, "/app/BakeupDB_mobile.sh %s %s", (char*)"10.168.0.101", (char*)"10.168.0.102");
                    printf("\r\n EV9000DB_MOBILE���ݿⱸ������:%s \r\n", strCmd);
                    system(strCmd);

                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "ִ���������ݿⱸ��, ����---");
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "The main standby database backup, end -");
                    MS_Bakup1 = time(NULL);
                }

                osip_usleep(1000000);
            }

            printf("\r\n ��ǰ�豸Ϊ��������⵽����״̬�쳣�����Զ��л�������״̬ \r\n");
        }
        else if (g_BoardNetConfig.st_MSFlag == 1) /* ���� */
        {
            printf("\r\n ��ǰ�豸Ϊ��������⹫��IP��ַ�Ƿ��� \r\n"); /* ��������CMS����֮�󣬹���IP��ַ��û�ж�ʧ */

            /* ��⵱ǰ����IP��ַ�Ƿ����pingͨ���������pingͨ��˵��������� */
            while (check_Common_IP_Live())
            {
                printf("\r\n ��ǰ�豸Ϊ��������⵽����������������״̬ \r\n");

                /* ����������ʱ��ִ��һ�����ݿ�ͬ��,����ÿСʱͬ��һ�� */
                if (0 == MS_Bakup2 || time(NULL) - MS_Bakup2 > 3600)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "ִ���������ݿⱸ��, ��ʼ---");
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "The main standby database backup, began to -- -- - ");

                    memset(strCmd, 0, 256);
                    snprintf(strCmd, 256, "/app/BakeupDB.sh %s %s", (char*)"10.168.0.102", (char*)"10.168.0.101");
                    printf("\r\n EV9000DB���ݿⱸ������:%s \r\n", strCmd);
                    system(strCmd);

                    memset(strCmd, 0, 256);
                    snprintf(strCmd, 256, "/app/BakeupDB_mobile.sh %s %s", (char*)"10.168.0.102", (char*)"10.168.0.101");
                    printf("\r\n EV9000DB_MOBILE���ݿⱸ������:%s \r\n", strCmd);
                    system(strCmd);

                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "ִ���������ݿⱸ��, ����---");
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "The main standby database backup, end -");
                    MS_Bakup2 = time(NULL);
                }

                osip_usleep(1000000);
            }

            printf("\r\n ��ǰ�豸Ϊ��������⵽������û�й���������״̬���Զ������������� \r\n");
        }

        set_common_ip(); /* ���ù���IP��ַ */

        /* ����һ��MMS��ý������EV9000MediaService���̣���ֹMMS���������ȡ������IP��ַ����ȷ */
        system((char*)"killall mms; killall -9 mms");
        system((char*)"killall EV9000MediaService; killall -9 EV9000MediaService");
    }

    printf(" ����״̬���, ����--- \r\n");

    /* ����ȫ�ֲ��� */
    printf("\r\n main() gbl_conf_load() Begin--- \r\n");
    iRet = gbl_conf_load();
    printf(" main() gbl_conf_load() End--- iRet=%d \r\n", iRet);

    DBThreadNew();

    printf("\r\n main() db_connect() Begin--- \r\n");
    iRet = db_connect();
    printf(" main() db_connect() End--- iRet=%d \r\n", iRet);

    while (0 != iRet)
    {
        iRet = db_connect();
        printf(" main() db_connect:End--- iRet=%d \r\n", iRet);

        osip_usleep(5000000);
    }

    //�������ݿ��Զ�ɾ���¼�
    printf("\r\n main() SetMySQLEventOn() Begin--- \r\n");
    iRet = SetMySQLEventOn();
    printf(" main() SetMySQLEventOn:End--- iRet=%d \r\n", iRet);

    /*  ��־��¼�����ݿ⴦���߳� */
    printf("\r\n main() log2db_proc_thread_start() Begin--- \r\n");
    iRet = log2db_proc_thread_start();
    printf(" main() log2db_proc_thread_start:End--- iRet=%d \r\n", iRet);

    /* �źŴ������� */
    printf("\r\n main() set_SignalProc() Begin--- \r\n");
    iRet = set_SignalProc();
    printf(" main() set_SignalProc() End--- iRet=%d \r\n", iRet);

    if (0 != iRet)
    {
        Board_UnInit();
        gbl_conf_free();
        log2db_proc_thread_stop();
        log2file_proc_thread_stop();
        system_log2db_list_free();
        trace_log2file_list_free();
        sip_msg_log2file_list_free();
        Log2FileFree();
        return -1;
    }

    /* ����ģ�龲̬���ݳ�ʼ�� */
    printf("\r\n main() InitModuleStaticListData() Begin--- \r\n");
    iRet = InitModuleStaticListData();
    printf(" main() InitModuleStaticListData() End--- iRet=%d \r\n", iRet);

    if (0 != iRet)
    {
        Board_UnInit();
        gbl_conf_free();
        log2db_proc_thread_stop();
        log2file_proc_thread_stop();
        system_log2db_list_free();
        trace_log2file_list_free();
        sip_msg_log2file_list_free();
        Log2FileFree();
        return -1;
    }

    /* ��ʼ���豸���ע��״̬*/
    printf("\r\n main() InitAllZRVDeviceRegStatus() Begin--- \r\n");
    InitAllZRVDeviceRegStatus();
    printf(" main() InitAllZRVDeviceRegStatus() End--- \r\n");

    /* ����ģ�鴦���߳� */
    printf("\r\n main() CreateModuleProcThread() Begin--- \r\n");
    iRet = CreateModuleProcThread();
    printf(" main() CreateModuleProcThread() End--- iRet=%d \r\n", iRet);

    if (0 != iRet)
    {
        Board_UnInit();
        gbl_conf_free();
        log2db_proc_thread_stop();
        log2file_proc_thread_stop();
        system_log2db_list_free();
        trace_log2file_list_free();
        sip_msg_log2file_list_free();
        Log2FileFree();
        FreeModuleStaticListData();
        return -1;
    }
    
    /* �����ݿ�������� */
    printf("\r\n main() DowndataFromDatabase() Begin--- \r\n");
    iRet = DowndataFromDatabase();
    printf(" main() DowndataFromDatabase() End--- iRet=%d \r\n", iRet);

    /*Telnet ����˳�ʼ�� */
    printf("\r\n main() TelnetServerInit() Begin--- \r\n");
    iRet = TelnetServerInit();
    printf(" main() TelnetServerInit() End--- iRet=%d \r\n", iRet);

    if (0 != iRet)
    {
        DestroyModuleProcThread();
        Board_UnInit();
        gbl_conf_free();
        log2db_proc_thread_stop();
        log2file_proc_thread_stop();
        system_log2db_list_free();
        trace_log2file_list_free();
        sip_msg_log2file_list_free();
        Log2FileFree();
        FreeModuleStaticListData();
        return -1;
    }

    /* �û�TCP��ȡ���ݷ���˳�ʼ�� */
    printf("\r\n main() ZRVDeviceLoginServerInit() Begin--- \r\n");
    iRet = ZRVDeviceLoginServerInit();
    printf(" main() ZRVDeviceLoginServerInit() End--- iRet=%d \r\n", iRet);

    if (0 != iRet)
    {
        TelnetServerFree();
        DestroyModuleProcThread();
        Board_UnInit();
        gbl_conf_free();
        log2db_proc_thread_stop();
        log2file_proc_thread_stop();
        system_log2db_list_free();
        trace_log2file_list_free();
        sip_msg_log2file_list_free();
        Log2FileFree();
        FreeModuleStaticListData();
        return -1;
    }

    printf("\r\n ############################################## \
    \r\n #      CMS Init Data OK: Enter Main Loop     # \
    \r\n ############################################## \r\n");

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "CMS ��ʼ����");
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "CMS start running");

    cms_run_status = 1;

#if 0
    printf("\r\n main() SendNotifyRestartMessageToAllSubCMS() Begin--- \r\n");
    iRet = SendNotifyRestartMessageToAllSubCMS();
    printf("\r\n main() SendNotifyRestartMessageToAllSubCMS() End--- iRet=%d \r\n", iRet);
#endif

    /* ��ѭ��*/
    main_loop();

    cms_run_status = 0;

    /* ����ϵͳ�����˳���ʶ */
    change_conf_to_config_file((char*)"sysexitflag", (char*)"0");

    printf("\r\n ############################################## \
    \r\n #          CMS Begin Exit: Free Data         # \
    \r\n ############################################## \r\n");

    /* ����ģ�鴦���߳� */
    printf("\r\n main() DestroyModuleProcThread() Begin--- \r\n");
    DestroyModuleProcThread();
    printf("\r\n main() DestroyModuleProcThread:END \r\n");

    /* ����ģ�龲̬�����ͷ� */
    printf("\r\n main() FreeModuleStaticListData() Begin--- \r\n");
    FreeModuleStaticListData();
    printf(" main() FreeModuleStaticListData:END \r\n");

    /* Telnet����������ͷ� */
    printf("\r\n main() TelnetServerFree() Begin--- \r\n");
    TelnetServerFree();
    printf("\r\n main() TelnetServerFree:END \r\n");

    printf("\r\n main() ZRVDeviceLoginServerFree() Begin--- \r\n");
    ZRVDeviceLoginServerFree();
    printf("\r\n main() ZRVDeviceLoginServerFree:END \r\n");

    /* ȫ�����������ͷ� */
    printf("\r\n main() gbl_conf_free() Begin--- \r\n");
    gbl_conf_free();
    printf("\r\n main() gbl_conf_free:END \r\n");

    /* ���巴��ʼ�� */
    printf("\r\n main() Board_UnInit() Begin--- \r\n");
    Board_UnInit();
    printf("\r\n main() Board_UnInit:END \r\n");

    printf("\r\n main() log2db_proc_thread_stop() Begin--- \r\n");
    log2db_proc_thread_stop();
    printf("\r\n main() log2db_proc_thread_stop:END \r\n");

    printf("\r\n main() log2file_proc_thread_stop() Begin--- \r\n");
    log2file_proc_thread_stop();
    printf("\r\n main() log2file_proc_thread_stop:END \r\n");

    printf("\r\n main() system_log2db_list_free() Begin--- \r\n");
    system_log2db_list_free();
    printf("\r\n main() system_log2db_list_free:END \r\n");

    printf("\r\n main() trace_log2file_list_free() Begin--- \r\n");
    trace_log2file_list_free();
    printf("\r\n main() trace_log2file_list_free:END \r\n");

    printf("\r\n main() sip_msg_log2file_list_free() Begin--- \r\n");
    sip_msg_log2file_list_free();
    printf("\r\n main() sip_msg_log2file_list_free:END \r\n");

    DBThreadDelete();
    printf("\r\n main() DBThreadDelete():END \r\n");

    /*  ��־�ļ��ͷ�*/
    printf("\r\n main() Log2FileFree() Begin--- \r\n");
    Log2FileFree();
    printf(" main() Log2FileFree:END \r\n");

    if (restart)
    {
        printf("\r\n main() Restart \r\n");
        init_globals();
        goto main_begin;
    }

    ClosedDB();
    printf("\r\n ############################################## \
    \r\n #           CMS Free Data OK: Exit           # \
    \r\n ############################################## \r\n");

    return (0);  /* end */
}

