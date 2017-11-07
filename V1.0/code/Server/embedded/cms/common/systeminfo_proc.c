#ifdef WIN32
#include <windows.h>
#include <winsock.h>
#include <sys/types.h>
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
#include <sys/types.h>
#include <dirent.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <errno.h>

#include <asm/types.h>
#include <netinet/ether.h>
#include <net/if.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>

#endif

#include "common/gbldef.inc"
#include "common/systeminfo_proc.inc"
#include "common/gblconfig_proc.inc"
#include "common/gblfunc_proc.inc"
#include "common/log_proc.inc"
#include "platformms/BoardInit.h"

#include "device/device_info_mgn.inc"
#include "user/user_info_mgn.inc"
#include "resource/resource_info_mgn.inc"
#include "service/call_func_proc.inc"
#include "route/route_info_mgn.inc"
#include "common/db_proc.h"
#include "common/common_thread_proc.inc"

/*----------------------------------------------*
 * �ⲿ����˵��                                 *
 *----------------------------------------------*/
extern gbl_conf_t* pGblconf;              /* ȫ��������Ϣ */
extern User_Info_MAP g_UserInfoMap;                       /* �û���Ϣ���� */
extern GBDevice_Info_MAP g_GBDeviceInfoMap;              /* ��׼�����豸��Ϣ���� */
extern GBLogicDevice_Info_MAP g_GBLogicDeviceInfoMap;    /* ��׼�߼��豸��Ϣ���� */
extern TSU_Resource_Info_MAP g_TSUResourceInfoMap;       /* TSU ��Դ��Ϣ���� */
extern CR_Data_MAP g_CallRecordMap;                       /* �����������ݶ��� */
extern MS_SWITCH_ATTR  gt_MSSwitchAttr;
extern BOARD_NET_ATTR  g_BoardNetConfig;
extern int cms_run_status;  /* 0:û������,1:�������� */
extern DBOper g_DBOper;
extern int g_GetChannelFlag;

/*----------------------------------------------*
 * �ⲿ����ԭ��˵��                             *
 *----------------------------------------------*/

/*----------------------------------------------*
 * �ڲ�����ԭ��˵��                             *
 *----------------------------------------------*/

/*----------------------------------------------*
 * ȫ�ֱ���                                     *
 *----------------------------------------------*/

/*----------------------------------------------*
 * ģ�鼶����                                   *
 *----------------------------------------------*/

/*----------------------------------------------*
 * ��������                                     *
 *----------------------------------------------*/

/*----------------------------------------------*
 * �궨��                                       *
 *----------------------------------------------*/
/*****************************************************************************
 �� �� ��  : get_cpu_info
 ��������  : ��ȡCPU�Ĳ�����Ϣ
 �������  : cpu_info* cpuinfo
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��4��16�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int get_cpu_info(cpu_info* cpuinfo)
{
    FILE* fp = NULL;
    char text[201] = {0};
    int process_num = 0;
    char* tmp = NULL;
    char* tmpValue = NULL;
    int len = 0;

    fp = fopen("/proc/cpuinfo", "r");

    if (NULL == fp)
    {
        printf("Open proc cpu info failed!\n");
        return 0;
    }

    while (fgets(text, 200, fp))
    {
        if (0 == strncmp("system type", text, 11))
        {
            if (cpuinfo->system_type[0] == '\0')
            {
                tmp = strchr(text, ':'); /*find ':' */

                if (tmp != NULL)
                {
                    tmpValue = tmp + 2;
                    strcpy(cpuinfo->system_type, tmpValue);

                    len = strlen(cpuinfo->system_type);

                    if (cpuinfo->system_type[len - 1] == '\n')
                    {
                        cpuinfo->system_type[len - 1] = '\0';
                    }
                }
            }
        }
        else if (0 == strncmp("processor", text, 9))
        {
            process_num++;
        }
        else if (0 == strncmp("cpu model", text, 9))
        {
            if (cpuinfo->cpu_model[0] == '\0')
            {
                tmp = strchr(text, ':'); /*find ':' */

                if (tmp != NULL)
                {
                    tmpValue = tmp + 2;
                    strcpy(cpuinfo->cpu_model, tmpValue);

                    len = strlen(cpuinfo->cpu_model);

                    if (cpuinfo->cpu_model[len - 1] == '\n')
                    {
                        cpuinfo->cpu_model[len - 1] = '\0';
                    }
                }
            }
        }
        else if (0 == strncmp("BogoMIPS", text, 8))
        {
            if (cpuinfo->BogoMIPS[0] == '\0')
            {
                tmp = strchr(text, ':'); /*find ':' */

                if (tmp != NULL)
                {
                    tmpValue = tmp + 2;
                    strcpy(cpuinfo->BogoMIPS, tmpValue);

                    len = strlen(cpuinfo->BogoMIPS);

                    if (cpuinfo->BogoMIPS[len - 1] == '\n')
                    {
                        cpuinfo->BogoMIPS[len - 1] = '\0';
                    }
                }
            }
        }
    }

    fclose(fp);

    return process_num;
}

/*****************************************************************************
 �� �� ��  : get_cpu_status
 ��������  : ��ȡCPU��״̬��Ϣ
 �������  : cpu_status* cpu_stat
                             int process_num
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��4��16�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
void get_cpu_status(cpu_status* cpu_stat, int process_num)
{
    unsigned int total = 0;
    float user = 0.0;
    float nice = 0.0;
    float system = 0.0;
    float idle = 0.0;
    char cpu[21] = {0};
    char text[201] = {0};
    char strCPU[10] = {0};

    FILE* fp = NULL;

    fp = fopen("/proc/stat", "r");

    if (NULL == fp)
    {
        printf("Open proc cpu status failed!\n");
        return;
    }

    while (fgets(text, 200, fp))
    {
        if (process_num < 0)
        {
            if (0 == strncmp((char*)"cpu ", text, 4))
            {
                sscanf(text, "%s %f %f %f %f", cpu, &user, &nice, &system, &idle);
            }
        }
        else
        {
            sprintf(strCPU, "cpu%d", process_num);

            if (0 == strncmp(strCPU, text, strlen(strCPU)))
            {
                sscanf(text, "%s %f %f %f %f", cpu, &user, &nice, &system, &idle);
            }
        }
    }

    fclose(fp);

    total = (user + nice + system + idle);
    user = (user / total) * 100;
    nice = (nice / total) * 100;
    system = (system / total) * 100;
    idle = (idle / total) * 100;

    cpu_stat->total = total;
    cpu_stat->user = user;
    cpu_stat->nice = nice;
    cpu_stat->system = system;
    cpu_stat->idle = idle;

    return;
}

/*****************************************************************************
 �� �� ��  : get_mem_info
 ��������  : ��ȡϵͳ�ڴ���Ϣ
 �������  : mem_info* minfo
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��4��16�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
void get_mem_info(mem_info* minfo)
{
    FILE* fp = NULL;
    unsigned int i = 0, j = 0;
    char total[81] = {0};
    char free[81] = {0};
    char temp[81] = {0};

    fp = fopen("/proc/meminfo", "r");

    if (NULL == fp)
    {
        printf("Open proc mem info failed!\n");
        return;
    }

    while (fgets(temp, 80, fp))
    {
        if (0 == strncmp("MemTotal", temp, 8)) //total info
        {
            strcpy(total, temp);
        }
        else if (0 == strncmp("MemFree", temp, 7)) //free info
        {
            strcpy(free, temp);
        }

        if (strlen(total) > 0 && strlen(free) > 0) /* �Ѿ�ȫ���ҵ������� */
        {
            break;
        }
    }

    if (strlen(total) > 0)
    {
        for (i = 0, j = 0; i < strlen(total); i++)
        {
            if (isdigit(total[i]))
            {
                minfo->total[j++] = total[i];
            }
        }

        minfo->total[j] = '\0';
    }

    if (strlen(free) > 0)
    {
        for (i = 0, j = 0; i < strlen(free); i++)
        {
            if (isdigit(free[i]))
            {
                minfo->free[j++] = free[i];
            }
        }

        minfo->free[j] = '\0';
    }

    fclose(fp);

    return;
}

/*****************************************************************************
 �� �� ��  : find_pid_by_name
 ��������  : ���ݽ������ƻ�ȡ����ID
 �������  : char* ProcName
                            int* foundpid
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��4��15�� ���ڶ�
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int find_pid_by_name(char* ProcName, int* foundpid)
{
    DIR*             dir;
    struct dirent*   d;
    int             pid, i;
    char*            s;
    int pnlen;

    i = 0;
    foundpid[0] = 0;
    pnlen = strlen(ProcName);

    /* Open the /proc directory. */
    dir = opendir("/proc");

    if (!dir)
    {
        printf("cannot open /proc");
        return -1;
    }

    /* Walk through the directory. */
    while ((d = readdir(dir)) != NULL)
    {

        char exe [PATH_MAX + 1];
        char path[PATH_MAX + 1];
        int len;
        int namelen;

        /* See if this is a process */
        if ((pid = atoi(d->d_name)) == 0)
        {
            continue;
        }

        snprintf(exe, sizeof(exe), "/proc/%s/exe", d->d_name);

        if ((len = readlink(exe, path, PATH_MAX)) < 0)
        {
            continue;
        }

        path[len] = '\0';

        /* Find ProcName */
        s = strrchr(path, '/');

        if (s == NULL)
        {
            continue;
        }

        s++;

        /* we don't need small name len */
        namelen = strlen(s);

        if (namelen < pnlen)
        {
            continue;
        }

        if (!strncmp(ProcName, s, pnlen))
        {
            /* to avoid subname like search proc tao but proc taolinke matched */
            if (s[pnlen] == ' ' || s[pnlen] == '\0')
            {
                foundpid[i] = pid;
                i++;
            }
        }
    }

    foundpid[i] = 0;
    closedir(dir);

    return  0;

}

/*****************************************************************************
 �� �� ��  : get_progress_cpu_usage
 ��������  :  ��ȡ��ǰ���̵�cpuʹ����
 �������  : char* ProcName
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��4��16�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int get_progress_cpu_usage(char* ProcName, float* pfcpu)
{
    int i = 0;
    int iRet = 0;
    int pid_t[128] = {0};
    int pid = 0;
    float pcpu = 0.0;

    if (NULL == ProcName)
    {
        return -1;
    }

    /* ��ȡ���̵�pid */
    iRet = find_pid_by_name(ProcName, pid_t);

    if (!iRet)
    {
        for (i = 0; pid_t[i] != 0; i++)
        {
            pid = pid_t[i];
            break;
        }
    }
    else
    {
        return iRet;
    }

    pcpu = get_pcpu(pid);

    *pfcpu = pcpu;
    //printf("\r\n ********** Porcess Name=%s: pid=%d, pcpu=%f ********** \r\n", ProcName, pid, pcpu);

    return 0;
}

/*****************************************************************************
 �� �� ��  : get_progress_memory_usage
 ��������  : ��ȡ��ǰ�����ڴ�������ڴ�ʹ����
 �������  : char* ProcName
             process_mem_info* proc_mem
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��4��16�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int get_progress_memory_usage(char* ProcName, process_mem_info* proc_mem)
{
    unsigned int i = 0, j = 0;
    int iRet = 0;
    int pid_t[128] = {0};
    int pid = 0;
    char buf[128] = {0};
    FILE* fp = NULL;
    char mem[81] = {0};
    char vmem[81] = {0};
    char temp[81] = {0};

    if (NULL == ProcName || NULL == proc_mem)
    {
        return -1;
    }

    /* ��ȡ���̵�pid */
    iRet = find_pid_by_name(ProcName, pid_t);

    if (!iRet)
    {
        for (i = 0; pid_t[i] != 0; i++)
        {
            pid = pid_t[i];
            //printf("\r\n ********** Porcess Name=%s: pid=%d ********** \r\n", ProcName, pid_t[i]);
            break;
        }
    }
    else
    {
        return iRet;
    }

    sprintf(buf, "/proc/%d/status", pid);


    fp = fopen(buf, "r");

    if (NULL == fp)
    {
        printf("Open %s failed!\n", buf);
        return -1;
    }

    while (fgets(temp, 80, fp))
    {
        if (0 == strncmp("VmRSS", temp, 5)) //�����ڴ�
        {
            strcpy(mem, temp);
        }
        else if (0 == strncmp("VmData", temp, 6)) //�����ڴ�
        {
            strcpy(vmem, temp);
        }

        if (strlen(mem) > 0 && strlen(vmem) > 0) /* �Ѿ�ȫ���ҵ������� */
        {
            break;
        }
    }

    if (strlen(mem) > 0)
    {
        for (i = 0, j = 0; i < strlen(mem); i++)
        {
            if (isdigit(mem[i]))
            {
                proc_mem->mem[j++] = mem[i];
            }
        }

        proc_mem->mem[j] = '\0';
    }

    if (strlen(vmem) > 0)
    {
        for (i = 0, j = 0; i < strlen(vmem); i++)
        {
            if (isdigit(vmem[i]))
            {
                proc_mem->vmem[j++] = vmem[i];
            }
        }

        proc_mem->vmem[j] = '\0';
    }

    fclose(fp);

    return 0;
}

/*****************************************************************************
 �� �� ��  : get_phy_mem
 ��������  : ��ȡ���̵������ڴ�
 �������  : const pid_t p
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��4��16�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int get_phy_mem(const pid_t p)
{
    int i = 0;
    FILE* fd = NULL;         //�����ļ�ָ��fd
    char line_buff[256] = {0};  //��ȡ�еĻ�����
    char name[32] = {0};//�����Ŀ����
    char file[64] = {0};//�ļ���
    int vmrss = 0;//����ڴ��ֵ��С

    sprintf(file, "/proc/%d/status", p); //�ļ��е�11�а�����

    //fprintf(stderr, "current pid:%d\n", p);
    fd = fopen(file, "r");  //��R���ķ�ʽ���ļ��ٸ���ָ��fd

    if (NULL == fd)
    {
        printf("Open %s failed!\n", file);
        return -1;
    }

    //��ȡvmrss:ʵ�������ڴ�ռ��

    for (i = 0; i < VMRSS_LINE - 1; i++)
    {
        fgets(line_buff, sizeof(line_buff), fd);
    }//������15��

    fgets(line_buff, sizeof(line_buff), fd); //��ȡVmRSS��һ�е�����,VmRSS�ڵ�15��
    sscanf(line_buff, "%s %d", name, &vmrss);
    //fprintf(stderr, "====%s��%d====\n", name, vmrss);
    fclose(fd);     //�ر��ļ�fd
    return vmrss;
}

/*****************************************************************************
 �� �� ��  : get_total_mem
 ��������  : ��ȡϵͳ�ܵ������ڴ�
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��4��16�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int get_total_mem()
{
    FILE* fd = NULL;         //�����ļ�ָ��fd
    char line_buff[256] = {0};  //��ȡ�еĻ�����
    char name[32] = {0};//�����Ŀ����
    int memtotal = 0;//����ڴ��ֵ��С
    const char* file = "/proc/meminfo";//�ļ���

    fd = fopen(file, "r");  //��R���ķ�ʽ���ļ��ٸ���ָ��fd

    if (NULL == fd)
    {
        printf("Open %s failed!\n", file);
        return -1;
    }

    //��ȡmemtotal:���ڴ�ռ�ô�С
    fgets(line_buff, sizeof(line_buff), fd); //��ȡmemtotal��һ�е�����,memtotal�ڵ�1��
    sscanf(line_buff, "%s %d", name, &memtotal);
    //fprintf(stderr, "====%s��%d====\n", name, memtotal);
    fclose(fd);     //�ر��ļ�fd
    return memtotal;
}

/*****************************************************************************
 �� �� ��  : get_pmem
 ��������  : ��ȡ���̵��ڴ�ռ�ðٷֱ�
 �������  : pid_t p
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��4��16�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
float get_pmem(pid_t p)
{
    int phy = get_phy_mem(p);
    int total = get_total_mem();
    float occupy = (phy * 1.0) / (total * 1.0);
    //fprintf(stderr, "====process mem occupy:%.6f\n====", occupy);
    return occupy;
}

/*****************************************************************************
 �� �� ��  : get_cpu_process_occupy
 ��������  : ��ȡ���̵�CPU��Ϣ
 �������  : const pid_t p
 �������  : ��
 �� �� ֵ  : unsigned
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��4��16�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
unsigned int get_cpu_process_occupy(const pid_t p)
{
    char file[64] = {0};//�ļ���
    process_cpu_occupy_t t;
    FILE* fd = NULL;         //�����ļ�ָ��fd
    char line_buff[1024] = {0};  //��ȡ�еĻ�����
    const char* q = NULL;

    sprintf(file, "/proc/%d/stat", p); //�ļ��е�11�а�����

    //fprintf(stderr, "current pid:%d\n", p);
    fd = fopen(file, "r");  //��R���ķ�ʽ���ļ��ٸ���ָ��fd

    if (NULL == fd)
    {
        printf("Open %s failed!\n", file);
        return 0;
    }

    fgets(line_buff, sizeof(line_buff), fd);  //��fd�ļ��ж�ȡ����Ϊbuff���ַ����ٴ浽��ʼ��ַΪbuff����ռ���

    sscanf(line_buff, "%u", &t.pid); //ȡ�õ�һ��
    q = get_items((const char*)line_buff, PROCESS_ITEM); //ȡ�ôӵ�14�ʼ����ʼָ��
    sscanf(q, "%u %u %u %u", &t.utime, &t.stime, &t.cutime, &t.cstime); //��ʽ����14,15,16,17��

    //fprintf(stderr, "====pid%u:%u %u %u %u====\n", t.pid, t.utime, t.stime, t.cutime, t.cstime);
    fclose(fd);     //�ر��ļ�fd
    return (t.utime + t.stime + t.cutime + t.cstime);
}

/*****************************************************************************
 �� �� ��  : get_cpu_total_occupy
 ��������  : ��ȡ�ܵ�CPU��Ϣ
 �������  : ��
 �������  : ��
 �� �� ֵ  : unsigned
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��4��16�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
unsigned int get_cpu_total_occupy()
{
    FILE* fd;         //�����ļ�ָ��fd
    char buff[1024] = {0};  //����ֲ�����buff����Ϊchar���ʹ�СΪ1024
    total_cpu_occupy_t t;
    char name[16] = {0};//��ʱ��������ַ���

    fd = fopen("/proc/stat", "r");  //��R���ķ�ʽ��stat�ļ��ٸ���ָ��fd

    if (NULL == fd)
    {
        printf("Open /proc/stat failed!\n");
        return 0;
    }

    fgets(buff, sizeof(buff), fd);  //��fd�ļ��ж�ȡ����Ϊbuff���ַ����ٴ浽��ʼ��ַΪbuff����ռ���
    /*�����ǽ�buff���ַ������ݲ���format��ת��Ϊ���ݵĽ��������Ӧ�Ľṹ����� */
    sscanf(buff, "%s %u %u %u %u", name, &t.user, &t.nice, &t.system, &t.idle);

    //fprintf(stderr, "====%s:%u %u %u %u====\n", name, t.user, t.nice, t.system, t.idle);
    fclose(fd);     //�ر��ļ�fd
    return (t.user + t.nice + t.system + t.idle);
}

/*****************************************************************************
 �� �� ��  : get_pcpu
 ��������  : ��ȡ���̵�cpuռ����
 �������  : pid_t p
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��4��16�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
float get_pcpu(pid_t p)
{
    float pcpu = 0.0;
    unsigned int totalcputime1 = 0, totalcputime2 = 0;
    unsigned int procputime1 = 0, procputime2 = 0;
    totalcputime1 = get_cpu_total_occupy();
    procputime1 = get_cpu_process_occupy(p);
    usleep(500000);//�ӳ�500����
    totalcputime2 = get_cpu_total_occupy();
    procputime2 = get_cpu_process_occupy(p);
    pcpu = 100.0 * (procputime2 - procputime1) / (totalcputime2 - totalcputime1);
    //fprintf(stderr, "====pcpu:%.6f\n====", pcpu);
    return pcpu;
}

const char* get_items(const char* buffer, int ie)
{
    char* p = NULL;//ָ�򻺳���
    int len = 0;
    int count = 0;//ͳ�ƿո���

    if (NULL == buffer)
    {
        return NULL;
    }

    p = (char*)buffer;//ָ�򻺳���
    len = strlen(buffer);

    if (1 == ie || ie < 1)
    {
        return p;
    }

    int i;

    for (i = 0; i < len; i++)
    {
        if (' ' == *p)
        {
            count++;

            if (count == ie - 1)
            {
                p++;
                break;
            }
        }

        p++;
    }

    return p;
}

/*****************************************************************************
 �� �� ��  : getLocalNetInfo
 ��������  : ��ȡ����������Ϣ
 �������  : void
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��4��18�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int getLocalNetInfo(void)
{
    int fd;
    int interfaceNum = 0;
    struct ifreq buf[16];
    struct ifconf ifc;
    struct ifreq ifrcopy;
    char mac[16] = {0};
    char ip[32] = {0};
    char broadAddr[32] = {0};
    char subnetMask[32] = {0};
    char gateway[32] = {0};

    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("socket");
        close(fd);
        return -1;
    }

    ifc.ifc_len = sizeof(buf);
    ifc.ifc_buf = (caddr_t)buf;

    if (!ioctl(fd, SIOCGIFCONF, (char*)&ifc))
    {
        interfaceNum = ifc.ifc_len / sizeof(struct ifreq);
        printf("interface num = %d\n", interfaceNum);

        while (interfaceNum-- > 0)
        {
            printf("\ndevice name: %s\n", buf[interfaceNum].ifr_name);
            //ignore the interface that not up or not runing
            ifrcopy = buf[interfaceNum];

            if (ioctl(fd, SIOCGIFFLAGS, &ifrcopy))
            {
                printf("ioctl: %s [%s:%d]\n", strerror(errno), __FILE__, __LINE__);
                close(fd);
                return -1;
            }

            //get the mac of this interface
            if (!ioctl(fd, SIOCGIFHWADDR, (char*)(&buf[interfaceNum])))
            {
                memset(mac, 0, sizeof(mac));
                snprintf(mac, sizeof(mac), "%02x%02x%02x%02x%02x%02x",
                         (unsigned char)buf[interfaceNum].ifr_hwaddr.sa_data[0],
                         (unsigned char)buf[interfaceNum].ifr_hwaddr.sa_data[1],
                         (unsigned char)buf[interfaceNum].ifr_hwaddr.sa_data[2],
                         (unsigned char)buf[interfaceNum].ifr_hwaddr.sa_data[3],
                         (unsigned char)buf[interfaceNum].ifr_hwaddr.sa_data[4],
                         (unsigned char)buf[interfaceNum].ifr_hwaddr.sa_data[5]);
                printf("device mac: %s\n", mac);
            }
            else
            {
                printf("ioctl: %s [%s:%d]\n", strerror(errno), __FILE__, __LINE__);
                close(fd);
                return -1;
            }

            //get the IP of this interface
            if (!ioctl(fd, SIOCGIFADDR, (char*)&buf[interfaceNum]))
            {
                snprintf(ip, sizeof(ip), "%s",
                         (char*)inet_ntoa(((struct sockaddr_in*) & (buf[interfaceNum].ifr_addr))->sin_addr));
                printf("device ip: %s\n", ip);
            }
            else
            {
                printf("ioctl: %s [%s:%d]\n", strerror(errno), __FILE__, __LINE__);
                close(fd);
                return -1;
            }

            //get the broad address of this interface
            if (!ioctl(fd, SIOCGIFBRDADDR, &buf[interfaceNum]))
            {
                snprintf(broadAddr, sizeof(broadAddr), "%s",
                         (char*)inet_ntoa(((struct sockaddr_in*) & (buf[interfaceNum].ifr_broadaddr))->sin_addr));
                printf("device broadAddr: %s\n", broadAddr);
            }
            else
            {
                printf("ioctl: %s [%s:%d]\n", strerror(errno), __FILE__, __LINE__);
                close(fd);
                return -1;
            }

            //get the subnet mask of this interface
            if (!ioctl(fd, SIOCGIFNETMASK, &buf[interfaceNum]))
            {
                snprintf(subnetMask, sizeof(subnetMask), "%s",
                         (char*)inet_ntoa(((struct sockaddr_in*) & (buf[interfaceNum].ifr_netmask))->sin_addr));
                printf("device subnetMask: %s\n", subnetMask);
            }
            else
            {
                printf("ioctl: %s [%s:%d]\n", strerror(errno), __FILE__, __LINE__);
                close(fd);
                return -1;
            }

            /* ���ص�ַ */
            memset(gateway, 0, 32);
            get_eth_gateway(buf[interfaceNum].ifr_name, gateway);
            printf("device gateway: %s\n", gateway);
        }
    }
    else
    {
        printf("ioctl: %s [%s:%d]\n", strerror(errno), __FILE__, __LINE__);
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
}

/*****************************************************************************
 �� �� ��  : get_eth_gateway
 ��������  : ��ȡ����������
 �������  : char* eth_name
                            char* gateway_addr
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��4��17�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
void get_eth_gateway(char* eth_name, char* gateway_addr)
{
    unsigned long gateway = 0;
    char* tmp1 = 0;
    char* tmp2 = 0;
    char* tmp3 = 0;
    char* tmp4 = 0;
    char* tmp5 = 0;

    char strGateWay[32] = {0};

    char text[201] = {0};

    FILE* fp = NULL;

    fp = fopen("/proc/net/route", "r");

    if (NULL == fp)
    {
        printf("Open proc net route failed!\n");
        return;
    }

    while (fgets(text, 200, fp))
    {
        if (0 == strncmp(eth_name, text, strlen(eth_name)))
        {
            tmp1 = strchr(text, 9);

            if (tmp1 != NULL)
            {
                tmp2 = tmp1 + 1;

                tmp3 = strchr(tmp2, 9);

                if (tmp3 != NULL)
                {
                    tmp4 = tmp3 + 1;

                    tmp5 = strchr(tmp4, 9);

                    if (tmp5 != NULL)
                    {
                        osip_strncpy(strGateWay, tmp4, tmp5 - tmp4);
                        gateway = strtoul(strGateWay, NULL, 16);
                    }
                }
            }

            //sscanf(text, "%s\t%0x\t%0x\t%4d\t%d\t%d\t%d\t%0x\t%d\t%d\t%d", ethname, &dest, &gateway, &tmp1, &tmp2, &tmp3, &tmp4, mask, &tmp5, &tmp6, &tmp7);

            if (gateway > 0)
            {
                break;
            }
        }
    }

    fclose(fp);

    ipaddr_long2str(gateway_addr, gateway);

    return;
}

/*****************************************************************************
 �� �� ��  : GetSystemInfoProc
 ��������  : ��ȡϵͳ��Ϣ������
 �������  : char* pcSerialNumber
                            std::string& outBuff
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��4��17�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int GetSystemInfoProc(char* pcSerialNumber, std::string& outBuff)
{
    int i = 0;

    CPacket* pOutPacket = NULL;
    DOMDocument* pDOMDocument = NULL;
    DOMElement* pDOMElement = NULL;

    /* ����XML��ͷ�� */
    i |= CreateXMLHeadResponseForSystemRunState(&pOutPacket, (char*)"GetSystemInfo", pcSerialNumber);

    if (i != 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "GetSystemInfoProc() exit---: Create XML Packet Error \r\n");
        return -1;
    }

    pDOMDocument = pOutPacket->GetDOMDocument();
    pDOMElement = pDOMDocument->get_root();

    /* ���ʱ����Ϣ */
    pOutPacket->SetCurrentElement(pDOMElement);
    i |= AddSystemDateTimeInfoToSysInfoXML(pOutPacket);

    /* ���CPU ��Ϣ */
    pOutPacket->SetCurrentElement(pDOMElement);
    i |= AddCPUInfoToSysInfoXML(pOutPacket);

    /* ����ڴ���Ϣ */
    pOutPacket->SetCurrentElement(pDOMElement);
    i |= AddMemoryInfoToSysInfoXML(pOutPacket);

    /* ��ӷ�����Ϣ */
    pOutPacket->SetCurrentElement(pDOMElement);
    i |= AddFanInfoToSysInfoXML(pOutPacket);

    /* ���������Ϣ */
    pOutPacket->SetCurrentElement(pDOMElement);
    i |= AddNetWorkInfoToSysInfoXML(pOutPacket);

    outBuff = pOutPacket->GetXml(NULL);

    delete pOutPacket;

    return i;
}

/*****************************************************************************
 �� �� ��  : GetUserInfoProc
 ��������  : ��ȡ�û���Ϣ������
 �������  : char* pcSerialNumber
                            std::string& outBuff
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��4��18�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int GetUserInfoProc(char* pcSerialNumber, std::string& outBuff)
{
    int i = 0;

    int user_count = 0;
    char strUserCount[32] = {0};
    char strLoginPort[32] = {0};

    CPacket* pOutPacket = NULL;
    DOMDocument* pDOMDocument = NULL;
    DOMElement* pDOMElement = NULL;

    DOMElement* ItemAccNode = NULL;
    DOMElement* ItemInfoAccNode = NULL;

    DOMElement* UserItemAccNode = NULL;
    DOMElement* UserItemInfoAccNode = NULL;

    user_info_t* pUserInfo = NULL;
    User_Info_Iterator UserInfoItr;

    /* ����XML��ͷ�� */
    i |= CreateXMLHeadResponseForSystemRunState(&pOutPacket, (char*)"GetCMS_UserInfo", pcSerialNumber);

    if (i != 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "GetUserInfoProc() exit---: Create XML Packet Error \r\n");
        return -1;
    }

    pDOMDocument = pOutPacket->GetDOMDocument();
    pDOMElement = pDOMDocument->get_root();

    pOutPacket->SetCurrentElement(pDOMElement);

    /* ��дXML����*/
    ItemAccNode = pOutPacket->CreateElement((char*)"userinfo");
    pOutPacket->SetCurrentElement(ItemAccNode);

    USER_INFO_SMUTEX_LOCK();

    user_count = g_UserInfoMap.size();

    /* �û��ܸ��� */
    ItemInfoAccNode = pOutPacket->CreateElement((char*)"count");
    sprintf(strUserCount, "%d", user_count);
    pOutPacket->SetElementValue(ItemInfoAccNode, strUserCount);

    /* ѭ������û���Ϣ */
    for (UserInfoItr = g_UserInfoMap.begin(); UserInfoItr != g_UserInfoMap.end(); UserInfoItr++)
    {
        pUserInfo = UserInfoItr->second;

        if ((NULL == pUserInfo) || (pUserInfo->user_id[0] == '\0') || (pUserInfo->reg_info_index < 0))
        {
            continue;
        }

        pOutPacket->SetCurrentElement(ItemAccNode);

        UserItemAccNode = pOutPacket->CreateElement((char*)"user");
        pOutPacket->SetCurrentElement(UserItemAccNode);

        /* �û�ID */
        UserItemInfoAccNode = pOutPacket->CreateElement((char*)"userid");
        pOutPacket->SetElementValue(UserItemInfoAccNode, pUserInfo->user_id);

        /* IP ��ַ*/
        UserItemInfoAccNode = pOutPacket->CreateElement((char*)"ipaddress");
        pOutPacket->SetElementValue(UserItemInfoAccNode, pUserInfo->login_ip);

        /* �˿� */
        UserItemInfoAccNode = pOutPacket->CreateElement((char*)"port");
        sprintf(strLoginPort, "%d", pUserInfo->login_port);
        pOutPacket->SetElementValue(UserItemInfoAccNode, strLoginPort);
    }

    USER_INFO_SMUTEX_UNLOCK();

    outBuff = pOutPacket->GetXml(NULL);

    delete pOutPacket;

    return i;
}

/*****************************************************************************
 �� �� ��  : GetDeviceInfoProc
 ��������  : ��ȡ�豸��Ϣ������
 �������  : char* pcSerialNumber
                            std::string& outBuff
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��4��18�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int GetDeviceInfoProc(char* pcSerialNumber, std::string& outBuff)
{
    int i = 0;

    int device_count = 0;
    int logicDevice_count = 0;

    char strDeviceCount[32] = {0};
    char strLogicDeviceCount[32] = {0};

    char strLoginPort[32] = {0};

    CPacket* pOutPacket = NULL;
    DOMDocument* pDOMDocument = NULL;
    DOMElement* pDOMElement = NULL;

    DOMElement* ItemAccNode = NULL;
    DOMElement* ItemInfoAccNode = NULL;

    DOMElement* ItemAccNode1 = NULL;
    DOMElement* ItemInfoAccNode1 = NULL;

    DOMElement* DeviceItemAccNode = NULL;
    DOMElement* DeviceItemInfoAccNode = NULL;

    DOMElement* LogicDeviceItemAccNode = NULL;
    DOMElement* LogicDeviceItemInfoAccNode = NULL;

    GBDevice_info_t* pGBDeviceInfo = NULL;
    GBDevice_Info_Iterator GBDeviceInfoItr;

    GBLogicDevice_info_t* pGBLogicDeviceInfo = NULL;
    GBLogicDevice_Info_Iterator GBLogicDeviceInfoItr;

    /* ����XML��ͷ�� */
    i |= CreateXMLHeadResponseForSystemRunState(&pOutPacket, (char*)"GetCMS_DeviceInfo", pcSerialNumber);

    if (i != 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "GetDeviceInfoProc() exit---: Create XML Packet Error \r\n");
        return -1;
    }

    pDOMDocument = pOutPacket->GetDOMDocument();
    pDOMElement = pDOMDocument->get_root();

    pOutPacket->SetCurrentElement(pDOMElement);

    /* ��дXML����*/
    ItemAccNode = pOutPacket->CreateElement((char*)"deviceinfo");
    pOutPacket->SetCurrentElement(ItemAccNode);

    device_count = g_GBDeviceInfoMap.size();

    /* �����豸�ܸ��� */
    ItemInfoAccNode = pOutPacket->CreateElement((char*)"count");
    sprintf(strDeviceCount, "%d", device_count);
    pOutPacket->SetElementValue(ItemInfoAccNode, strDeviceCount);

    /* ѭ����������豸��Ϣ */
    for (GBDeviceInfoItr = g_GBDeviceInfoMap.begin(); GBDeviceInfoItr != g_GBDeviceInfoMap.end(); GBDeviceInfoItr++)
    {
        pGBDeviceInfo = GBDeviceInfoItr->second;

        if (NULL == pGBDeviceInfo)
        {
            continue;
        }

        pOutPacket->SetCurrentElement(ItemAccNode);

        DeviceItemAccNode = pOutPacket->CreateElement((char*)"phydevice");
        pOutPacket->SetCurrentElement(DeviceItemAccNode);

        /* �����豸ID */
        DeviceItemInfoAccNode = pOutPacket->CreateElement((char*)"deviceid");
        pOutPacket->SetElementValue(DeviceItemInfoAccNode, pGBDeviceInfo->device_id);

        /* IP ��ַ*/
        DeviceItemInfoAccNode = pOutPacket->CreateElement((char*)"ipaddress");
        pOutPacket->SetElementValue(DeviceItemInfoAccNode, pGBDeviceInfo->login_ip);

        /* �˿� */
        DeviceItemInfoAccNode = pOutPacket->CreateElement((char*)"port");
        sprintf(strLoginPort, "%d", pGBDeviceInfo->login_port);
        pOutPacket->SetElementValue(DeviceItemInfoAccNode, strLoginPort);

        /* ״̬*/
        DeviceItemInfoAccNode = pOutPacket->CreateElement((char*)"status");

        if (pGBDeviceInfo->reg_status == 0)
        {
            pOutPacket->SetElementValue(DeviceItemInfoAccNode, (char*)"OffLine");
        }
        else
        {
            pOutPacket->SetElementValue(DeviceItemInfoAccNode, (char*)"OnLine");
        }

        /* ���������߼��豸 */
        ItemAccNode1 = pOutPacket->CreateElement((char*)"logicdevicelist");
        pOutPacket->SetCurrentElement(ItemAccNode1);

        /* �߼��豸�ܸ��� */
        logicDevice_count = g_GBLogicDeviceInfoMap.size();
        ItemInfoAccNode1 = pOutPacket->CreateElement((char*)"count");

        if (logicDevice_count <= 0)
        {
            pOutPacket->SetElementValue(ItemInfoAccNode1, (char*)"0");
        }
        else
        {
            logicDevice_count = 0;

            for (GBLogicDeviceInfoItr = g_GBLogicDeviceInfoMap.begin(); GBLogicDeviceInfoItr != g_GBLogicDeviceInfoMap.end(); GBLogicDeviceInfoItr++)
            {
                pGBLogicDeviceInfo = GBLogicDeviceInfoItr->second;

                if (NULL == pGBLogicDeviceInfo)
                {
                    continue;
                }

                if (pGBLogicDeviceInfo->phy_mediaDeviceIndex != pGBDeviceInfo->id)
                {
                    continue;
                }

                logicDevice_count++;
            }

            sprintf(strLogicDeviceCount, "%d", logicDevice_count);
            pOutPacket->SetElementValue(ItemInfoAccNode1, strLogicDeviceCount);
        }

        /* ѭ������߼��豸��Ϣ */
        for (GBLogicDeviceInfoItr = g_GBLogicDeviceInfoMap.begin(); GBLogicDeviceInfoItr != g_GBLogicDeviceInfoMap.end(); GBLogicDeviceInfoItr++)
        {
            pGBLogicDeviceInfo = GBLogicDeviceInfoItr->second;

            if (NULL == pGBLogicDeviceInfo)
            {
                continue;
            }

            if (pGBLogicDeviceInfo->phy_mediaDeviceIndex != pGBDeviceInfo->id)
            {
                continue;
            }

            pOutPacket->SetCurrentElement(ItemAccNode1);

            LogicDeviceItemAccNode = pOutPacket->CreateElement((char*)"logicdevice");
            pOutPacket->SetCurrentElement(LogicDeviceItemAccNode);

            /* �߼��豸ID */
            LogicDeviceItemInfoAccNode = pOutPacket->CreateElement((char*)"deviceid");
            pOutPacket->SetElementValue(LogicDeviceItemInfoAccNode, pGBLogicDeviceInfo->device_id);

            /* ״̬*/
            LogicDeviceItemInfoAccNode = pOutPacket->CreateElement((char*)"status");

            if (pGBLogicDeviceInfo->status == 0)
            {
                pOutPacket->SetElementValue(LogicDeviceItemInfoAccNode, (char*)"OffLine");
            }
            else if (pGBLogicDeviceInfo->status == 1)
            {
                pOutPacket->SetElementValue(LogicDeviceItemInfoAccNode, (char*)"OnLine");
            }
            else if (pGBLogicDeviceInfo->status == 2)
            {
                pOutPacket->SetElementValue(LogicDeviceItemInfoAccNode, (char*)"NoStream");
            }
        }
    }

    outBuff = pOutPacket->GetXml(NULL);

    delete pOutPacket;

    return i;
}

/*****************************************************************************
 �� �� ��  : GetTSUInfoProc
 ��������  : ��ȡTSU��Ϣ������
 �������  : char* pcSerialNumber
                            std::string& outBuff
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��4��18�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int GetTSUInfoProc(char* pcSerialNumber, std::string& outBuff)
{
    int i = 0;

    int tsu_count = 0;
    char strTSUCount[32] = {0};
    char strTSUIndex[32] = {0};

    CPacket* pOutPacket = NULL;
    DOMDocument* pDOMDocument = NULL;
    DOMElement* pDOMElement = NULL;

    DOMElement* ItemAccNode = NULL;
    DOMElement* ItemInfoAccNode = NULL;

    DOMElement* TSUItemAccNode = NULL;
    DOMElement* TSUItemInfoAccNode = NULL;

    tsu_resource_info_t* pTsuResourceInfo = NULL;
    TSU_Resource_Info_Iterator TSUInfoItr;

    char strEthName[MAX_IP_LEN] = {0};
    char* tsu_ip = NULL;

    /* ����XML��ͷ�� */
    i |= CreateXMLHeadResponseForSystemRunState(&pOutPacket, (char*)"GetCMS_TSUInfo", pcSerialNumber);

    if (i != 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "GetTSUInfoProc() exit---: Create XML Packet Error \r\n");
        return -1;
    }

    pDOMDocument = pOutPacket->GetDOMDocument();
    pDOMElement = pDOMDocument->get_root();

    pOutPacket->SetCurrentElement(pDOMElement);

    /* ��дXML����*/
    ItemAccNode = pOutPacket->CreateElement((char*)"tsuinfo");
    pOutPacket->SetCurrentElement(ItemAccNode);

    tsu_count = g_TSUResourceInfoMap.size();

    /* TSU �ܸ��� */
    ItemInfoAccNode = pOutPacket->CreateElement((char*)"count");
    sprintf(strTSUCount, "%d", tsu_count);
    pOutPacket->SetElementValue(ItemInfoAccNode, strTSUCount);

    /* ѭ�����TSU ��Ϣ */
    for (TSUInfoItr = g_TSUResourceInfoMap.begin(); TSUInfoItr != g_TSUResourceInfoMap.end(); TSUInfoItr++)
    {
        pTsuResourceInfo = TSUInfoItr->second;

        pTsuResourceInfo = TSUInfoItr->second;

        if ((NULL == pTsuResourceInfo) || (0 == pTsuResourceInfo->iUsed))
        {
            continue;
        }

        pOutPacket->SetCurrentElement(ItemAccNode);

        TSUItemAccNode = pOutPacket->CreateElement((char*)"tsu");
        pOutPacket->SetCurrentElement(TSUItemAccNode);

        /* TSU Index */
        TSUItemInfoAccNode = pOutPacket->CreateElement((char*)"index");
        sprintf(strTSUIndex, "%d", TSUInfoItr->first);
        pOutPacket->SetElementValue(TSUItemInfoAccNode, strTSUIndex);

        /* TSU ID */
        TSUItemInfoAccNode = pOutPacket->CreateElement((char*)"tsuid");
        pOutPacket->SetElementValue(TSUItemInfoAccNode, pTsuResourceInfo->tsu_device_id);

        /* ��Ƶ��IP ��ַ*/
        TSUItemInfoAccNode = pOutPacket->CreateElement((char*)"videoip");

        tsu_ip = get_tsu_ip(pTsuResourceInfo, default_eth_name_get());

        if (NULL != tsu_ip)
        {
            pOutPacket->SetElementValue(TSUItemInfoAccNode, tsu_ip);
        }
        else
        {
            pOutPacket->SetElementValue(TSUItemInfoAccNode, (char*)"");
        }

        /* �豸��IP ��ַ*/
        TSUItemInfoAccNode = pOutPacket->CreateElement((char*)"deviceip");

        tsu_ip = get_tsu_ip(pTsuResourceInfo, default_eth_name_get());

        if (NULL != tsu_ip)
        {
            pOutPacket->SetElementValue(TSUItemInfoAccNode, tsu_ip);
        }
        else
        {
            pOutPacket->SetElementValue(TSUItemInfoAccNode, (char*)"");
        }

        /* ״̬*/
        TSUItemInfoAccNode = pOutPacket->CreateElement((char*)"status");

        if (pTsuResourceInfo->iStatus == 0)
        {
            pOutPacket->SetElementValue(TSUItemInfoAccNode, (char*)"OffLine");
        }
        else
        {
            pOutPacket->SetElementValue(TSUItemInfoAccNode, (char*)"OnLine");
        }
    }

    outBuff = pOutPacket->GetXml(NULL);

    delete pOutPacket;

    return i;
}

/*****************************************************************************
 �� �� ��  : GetTaskInfoProc
 ��������  : ��ȡ������Ϣ������
 �������  : char* pcSerialNumber
                            std::string& outBuff
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��4��18�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int GetTaskInfoProc(char* pcSerialNumber, std::string& outBuff)
{
    int i = 0;

    int task_count = 0;
    char strTaskCount[32] = {0};
    char strIndex[32] = {0};

    char strCallerSDPPort[32] = {0};
    char strCalleeSDPPort[32] = {0};

    char strTSUPort[32] = {0};

    CPacket* pOutPacket = NULL;
    DOMDocument* pDOMDocument = NULL;
    DOMElement* pDOMElement = NULL;

    DOMElement* ItemAccNode = NULL;
    DOMElement* ItemInfoAccNode = NULL;

    DOMElement* TaskItemAccNode = NULL;
    DOMElement* TaskItemInfoAccNode = NULL;

    cr_t* pCrData = NULL;
    CR_Data_Iterator Itr;

    /* ����XML��ͷ�� */
    i |= CreateXMLHeadResponseForSystemRunState(&pOutPacket, (char*)"GetCMS_TaskInfo", pcSerialNumber);

    if (i != 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "GetTaskInfoProc() exit---: Create XML Packet Error \r\n");
        return -1;
    }

    pDOMDocument = pOutPacket->GetDOMDocument();
    pDOMElement = pDOMDocument->get_root();

    pOutPacket->SetCurrentElement(pDOMElement);

    /* ��дXML����*/
    ItemAccNode = pOutPacket->CreateElement((char*)"taskinfo");
    pOutPacket->SetCurrentElement(ItemAccNode);

    CR_SMUTEX_LOCK();

    for (Itr = g_CallRecordMap.begin(); Itr != g_CallRecordMap.end(); Itr++)
    {
        pCrData = Itr->second;

        if ((NULL == pCrData) || (0 == pCrData->iUsed))
        {
            continue;
        }

        if (CALL_TYPE_NULL == pCrData->call_type
            || '\0' == pCrData->caller_id[0]
            || '\0' == pCrData->caller_ip[0]
            || pCrData->caller_port <= 0
            || '\0' == pCrData->callee_id[0]
            || '\0' == pCrData->callee_ip[0]
            || pCrData->callee_port <= 0)
        {
            continue;
        }

        task_count++;
    }

    /* TSU �ܸ��� */
    ItemInfoAccNode = pOutPacket->CreateElement((char*)"count");
    sprintf(strTaskCount, "%d", task_count);
    pOutPacket->SetElementValue(ItemInfoAccNode, strTaskCount);

    /* ѭ�����������Ϣ */
    for (Itr = g_CallRecordMap.begin(); Itr != g_CallRecordMap.end(); Itr++)
    {
        pCrData = Itr->second;

        if ((NULL == pCrData) || (0 == pCrData->iUsed))
        {
            continue;
        }

        if (CALL_TYPE_NULL == pCrData->call_type
            || '\0' == pCrData->caller_id[0]
            || '\0' == pCrData->caller_ip[0]
            || pCrData->caller_port <= 0
            || '\0' == pCrData->callee_id[0]
            || '\0' == pCrData->callee_ip[0]
            || pCrData->callee_port <= 0)
        {
            continue;
        }

        pOutPacket->SetCurrentElement(ItemAccNode);

        TaskItemAccNode = pOutPacket->CreateElement((char*)"task");
        pOutPacket->SetCurrentElement(TaskItemAccNode);

        /* ���� */
        TaskItemInfoAccNode = pOutPacket->CreateElement((char*)"type");

        if (pCrData->call_type == CALL_TYPE_REALTIME)
        {
            pOutPacket->SetElementValue(TaskItemInfoAccNode, (char*)"realplay");
        }
        else if (pCrData->call_type == CALL_TYPE_RECORD)
        {
            pOutPacket->SetElementValue(TaskItemInfoAccNode, (char*)"record");
        }
        else if (pCrData->call_type == CALL_TYPE_DC)
        {
            pOutPacket->SetElementValue(TaskItemInfoAccNode, (char*)"dc");
        }
        else if (pCrData->call_type == CALL_TYPE_RECORD_PLAY)
        {
            pOutPacket->SetElementValue(TaskItemInfoAccNode, (char*)"record play");
        }
        else if (pCrData->call_type == CALL_TYPE_DOWNLOAD)
        {
            pOutPacket->SetElementValue(TaskItemInfoAccNode, (char*)"download");
        }
        else if (pCrData->call_type == CALL_TYPE_AUDIO)
        {
            pOutPacket->SetElementValue(TaskItemInfoAccNode, (char*)"audio");
        }

        /* �������*/
        TaskItemInfoAccNode = pOutPacket->CreateElement((char*)"index");
        sprintf(strIndex, "%d", Itr->first);
        pOutPacket->SetElementValue(TaskItemInfoAccNode, strIndex);

        /* ������ID */
        TaskItemInfoAccNode = pOutPacket->CreateElement((char*)"callerid");
        pOutPacket->SetElementValue(TaskItemInfoAccNode, pCrData->caller_id);

        /* ������IP */
        TaskItemInfoAccNode = pOutPacket->CreateElement((char*)"callerip");
        pOutPacket->SetElementValue(TaskItemInfoAccNode, pCrData->caller_ip);

        /* �����߶˿� */
        TaskItemInfoAccNode = pOutPacket->CreateElement((char*)"callerport");
        sprintf(strCallerSDPPort, "%d", pCrData->caller_sdp_port);
        pOutPacket->SetElementValue(TaskItemInfoAccNode, strCallerSDPPort);

        /* ������ID */
        TaskItemInfoAccNode = pOutPacket->CreateElement((char*)"calleeid");
        pOutPacket->SetElementValue(TaskItemInfoAccNode, pCrData->callee_id);

        /* ������IP */
        TaskItemInfoAccNode = pOutPacket->CreateElement((char*)"calleeip");
        pOutPacket->SetElementValue(TaskItemInfoAccNode, pCrData->callee_ip);

        /* �����߶˿� */
        TaskItemInfoAccNode = pOutPacket->CreateElement((char*)"calleeport");
        sprintf(strCalleeSDPPort, "%d", pCrData->callee_sdp_port);
        pOutPacket->SetElementValue(TaskItemInfoAccNode, strCalleeSDPPort);

        /* TSU IP */
        TaskItemInfoAccNode = pOutPacket->CreateElement((char*)"tsuip");
        pOutPacket->SetElementValue(TaskItemInfoAccNode, pCrData->tsu_ip);

        /* TSU ���ն˿� */
        TaskItemInfoAccNode = pOutPacket->CreateElement((char*)"tsuport");
        sprintf(strTSUPort, "%d", pCrData->tsu_recv_port);
        pOutPacket->SetElementValue(TaskItemInfoAccNode, strTSUPort);

        /* TSU ���Ͷ˿� */
        TaskItemInfoAccNode = pOutPacket->CreateElement((char*)"tsusendport");
        sprintf(strTSUPort, "%d", pCrData->tsu_send_port);
        pOutPacket->SetElementValue(TaskItemInfoAccNode, strTSUPort);
    }

    CR_SMUTEX_UNLOCK();

    outBuff = pOutPacket->GetXml(NULL);

    delete pOutPacket;

    return i;
}

/*****************************************************************************
 �� �� ��  : SetIPAddrProc
 ��������  : ��ICE�ӿڹ���������IP��ַ�Ĵ���
 �������  : CPacket& inPacket
                            std::string& outBuff
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��4��26�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int SetIPAddrProc(CPacket& inPacket, std::string& outBuff)
{
    int i = 0;

    CPacket* pOutPacket = NULL;

    char strSN[128] = {0};
    char strEthName[MAX_IP_LEN] = {0};
    char strIP[32] = {0};
    char strMask[32] = {0};
    char strGateWay[32] = {0};

    inPacket.GetElementValue((char*)"serial_number", strSN);
    inPacket.GetElementValue((char*)"ethname", strEthName);
    inPacket.GetElementValue((char*)"ip", strIP);
    inPacket.GetElementValue((char*)"mask", strMask);
    inPacket.GetElementValue((char*)"gateway", strGateWay);

    if (strEthName[0] == '\0')
    {
        i = GeneratingErrorResponseForSystemRunState((char*)"SetIPAddr", strSN, 20001, (char*)"Get XML Ethernet Name Error", outBuff);
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "SetIPAddrProc() exit---: Get XML Ethernet Name Error \r\n");
        return -1;
    }

#if 0

    if (strIP[0] == '\0')
    {
        i = GeneratingErrorResponseForSystemRunState((char*)"SetIPAddr", strSN, 20002, (char*)"Get XML IP Error", outBuff);
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "SetIPAddrProc() exit---: Get XML XML IP Error \r\n");
        return -1;
    }

    if (strMask[0] == '\0')
    {
        i = GeneratingErrorResponseForSystemRunState((char*)"SetIPAddr", strSN, 20003, (char*)"Get XML Mask Error", outBuff);
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "SetIPAddrProc() exit---: Get XML XML Mask Error \r\n");
        return -1;
    }

    if (strGateWay[0] == '\0')
    {
        i = GeneratingErrorResponseForSystemRunState((char*)"SetIPAddr", strSN, 20004, (char*)"Get XML GateWay Error", outBuff);
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "SetIPAddrProc() exit---: Get XML XML GateWay Error \r\n");
        return -1;
    }

#endif

    i = GeneratingErrorResponseForSystemRunState((char*)"SetIPAddr", strSN, 20005, (char*)"Not Support Set IP Addr", outBuff);
    DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "SetIPAddrProc() exit---: Not Support Set IP Addr \r\n");
    return -1;

    /* ����XML��ͷ�� */
    i = CreateXMLHeadResponseForSystemRunState(&pOutPacket, (char*)"SetIPAddr", strSN);

    if (i != 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "SetIPAddrProc() exit---: Create XML Packet Error \r\n");
        return -1;
    }

    outBuff = pOutPacket->GetXml(NULL);

    delete pOutPacket;

    /* �������� */
    //BoardReboot();

    return i;
}

/*****************************************************************************
 �� �� ��  : GetEthNameByMac
 ��������  : ͨ��Mac��ַ��ȡ��������
 �������  : char* pcMac
             char** pEthName
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��7��15�� ���ڶ�
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int GetEthNameByMac(char* pcMac, char* pEthName)
{
    int fd = 0;
    int interfaceNum = 0;
    struct ifreq buf[16];
    struct ifconf ifc;
    struct ifreq ifrcopy;
    char mac[16] = {0};

    if (NULL == pcMac || NULL == pEthName)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "GetEthNameByMac() exit---: Param Error \r\n");
        return -1;
    }

    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "GetEthNameByMac() exit---: Socket Error \r\n");
        close(fd);
        return -1;
    }

    ifc.ifc_len = sizeof(buf);
    ifc.ifc_buf = (caddr_t)buf;

    if (!ioctl(fd, SIOCGIFCONF, (char*)&ifc))
    {
        interfaceNum = ifc.ifc_len / sizeof(struct ifreq);
        //printf("interface num = %d\n", interfaceNum);

        /* ѭ�����ÿ����������Ϣ */
        while (interfaceNum-- > 0)
        {
            if (0 == sstrcmp(buf[interfaceNum].ifr_name, (char*)"lo"))
            {
                continue;
            }

            //ignore the interface that not up or not runing
            ifrcopy = buf[interfaceNum];

            if (ioctl(fd, SIOCGIFFLAGS, &ifrcopy))
            {
                printf("ioctl: %s [%s:%d]\n", strerror(errno), __FILE__, __LINE__);
                close(fd);
                return -1;
            }

            //get the mac of this interface
            if (!ioctl(fd, SIOCGIFHWADDR, (char*)(&buf[interfaceNum])))
            {
                memset(mac, 0, sizeof(mac));
                snprintf(mac, sizeof(mac), "%02x%02x%02x%02x%02x%02x",
                         (unsigned char)buf[interfaceNum].ifr_hwaddr.sa_data[0],
                         (unsigned char)buf[interfaceNum].ifr_hwaddr.sa_data[1],
                         (unsigned char)buf[interfaceNum].ifr_hwaddr.sa_data[2],
                         (unsigned char)buf[interfaceNum].ifr_hwaddr.sa_data[3],
                         (unsigned char)buf[interfaceNum].ifr_hwaddr.sa_data[4],
                         (unsigned char)buf[interfaceNum].ifr_hwaddr.sa_data[5]);
            }
            else
            {
                printf("ioctl: %s [%s:%d]\n", strerror(errno), __FILE__, __LINE__);
                close(fd);
                return -1;
            }

            if (0 == sstrcmp(mac, pcMac))
            {
                strcpy(pEthName, buf[interfaceNum].ifr_name);
                close(fd);
                return 0;
            }
        }
    }
    else
    {
        printf("ioctl: %s [%s:%d]\n", strerror(errno), __FILE__, __LINE__);
        close(fd);
        return -1;
    }

    close(fd);
    return -1;
}

/*****************************************************************************
 �� �� ��  : GetSystemIPAddr
 ��������  : ��ȡϵͳIP��ַ
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��7��24�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int GetSystemIPAddr(int port)
{
    int i = 0;
    int fd = 0;
    int interfaceNum = 0;
    struct ifreq buf[16];
    struct ifconf ifc;
    struct ifreq ifrcopy;
    char ip[16] = {0};

    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        printf("GetSystemIPAddr() exit---: Socket Error \r\n");
        close(fd);
        return -1;
    }

    ifc.ifc_len = sizeof(buf);
    ifc.ifc_buf = (caddr_t)buf;

    if (!ioctl(fd, SIOCGIFCONF, (char*)&ifc))
    {
        interfaceNum = ifc.ifc_len / sizeof(struct ifreq);
        //printf("interface num = %d\n", interfaceNum);

        /* ѭ�����ÿ����������Ϣ */
        while (interfaceNum-- > 0)
        {
            /* ��������*/
            printf("GetSystemIPAddr() InterFace name: %s\r\n", buf[interfaceNum].ifr_name);

            if (0 != strncmp(buf[interfaceNum].ifr_name, (char*)"eth", 3)
                && 0 != strncmp(buf[interfaceNum].ifr_name, (char*)"em", 2)   /* ����������֧�� */
                && 0 != strncmp(buf[interfaceNum].ifr_name, (char*)"ens", 3)   /* ��������ڵ�֧�� */
                && 0 != strncmp(buf[interfaceNum].ifr_name, (char*)"bond", 4)  /* �����ڵ�֧�� */
                && 0 != strncmp(buf[interfaceNum].ifr_name, (char*)"wlan", 4)  /* �������ڵ�֧�� */
                && 0 != strncmp(buf[interfaceNum].ifr_name, (char*)"mgmt", 4)) /* �������ڵ�֧�� */
            {
                continue;
            }

            //ignore the interface that not up or not runing
            ifrcopy = buf[interfaceNum];

            if (ioctl(fd, SIOCGIFFLAGS, &ifrcopy))
            {
                printf("GetSystemIPAddr() exit---: ioctl: %s [%s:%d]\n", strerror(errno), __FILE__, __LINE__);
                close(fd);
                return -1;
            }

            //get the IP of this interface
            if (!ioctl(fd, SIOCGIFADDR, (char*)&buf[interfaceNum]))
            {
                memset(ip, 0, 16);

                snprintf(ip, sizeof(ip), "%s",
                         (char*)inet_ntoa(((struct sockaddr_in*) & (buf[interfaceNum].ifr_addr))->sin_addr));
                printf("GetSystemIPAddr() InterFace ip: %s\n", ip);

                /* IP  ��ַ*/
                if (0 == sstrcmp(buf[interfaceNum].ifr_name, (char*)"eth0")
                    || 0 == sstrcmp(buf[interfaceNum].ifr_name, (char*)"bond0"))
                {
                    i = ip_pair_add(buf[interfaceNum].ifr_name, IP_ADDR_VIDEO, ip, port);
                }
                else if (0 == sstrcmp(buf[interfaceNum].ifr_name, (char*)"eth1"))
                {
                    i = ip_pair_add(buf[interfaceNum].ifr_name, IP_ADDR_DEVICE, ip, port);
                }
                else
                {
                    i = ip_pair_add(buf[interfaceNum].ifr_name, IP_ADDR_NULL, ip, port);
                }

                printf("GetSystemIPAddr() ip_pair_add:eth_name=%s, ip=%s, port=%d, i=%d \r\n", buf[interfaceNum].ifr_name, ip, 5060, i);
            }
            else
            {
                printf("GetSystemIPAddr() exit---: ioctl: %s [%s:%d]\n", strerror(errno), __FILE__, __LINE__);
                close(fd);
                return -1;
            }
        }
    }
    else
    {
        printf("GetSystemIPAddr() exit---: ioctl: %s [%s:%d]\n", strerror(errno), __FILE__, __LINE__);
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
}

/*****************************************************************************
 �� �� ��  : IsIPAddrIsLocalIP
 ��������  : IP��ַ�Ƿ��Ǳ���ip��ַ
 �������  : char* strEthName
             char* strIPAddr
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2016��8��4��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int IsIPAddrIsLocalIP(char* strEthName, char* strIPAddr)
{
    int fd = 0;
    int interfaceNum = 0;
    struct ifreq buf[16];
    struct ifconf ifc;
    struct ifreq ifrcopy;
    char ip[MAX_IP_LEN] = {0};

    if (NULL == strEthName || NULL == strIPAddr)
    {
        printf("IsIPAddrIsLocalIP() exit---: EthName Error \r\n");
        return 0;
    }

    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        printf("IsIPAddrIsLocalIP() exit---: Socket Error \r\n");
        close(fd);
        return 0;
    }

    ifc.ifc_len = sizeof(buf);
    ifc.ifc_buf = (caddr_t)buf;

    if (!ioctl(fd, SIOCGIFCONF, (char*)&ifc))
    {
        interfaceNum = ifc.ifc_len / sizeof(struct ifreq);
        //printf("interface num = %d\n", interfaceNum);

        /* ѭ�����ÿ����������Ϣ */
        while (interfaceNum-- > 0)
        {
            /* ��������*/
            printf("IsIPAddrIsLocalIP() InterFace name: %s\r\n", buf[interfaceNum].ifr_name);

            if (0 != strncmp(buf[interfaceNum].ifr_name, (char*)"eth", 3)
                && 0 != strncmp(buf[interfaceNum].ifr_name, (char*)"bond", 4)  /* �����ڵ�֧�� */
                && 0 != strncmp(buf[interfaceNum].ifr_name, (char*)"wlan", 4)  /* �������ڵ�֧�� */
                && 0 != strncmp(buf[interfaceNum].ifr_name, (char*)"mgmt", 4)) /* �������ڵ�֧�� */
            {
                continue;
            }

            //ignore the interface that not up or not runing
            ifrcopy = buf[interfaceNum];

            if (ioctl(fd, SIOCGIFFLAGS, &ifrcopy))
            {
                printf("IsIPAddrIsLocalIP() exit---: ioctl: %s [%s:%d]\n", strerror(errno), __FILE__, __LINE__);
                close(fd);
                return 0;
            }

            //get the IP of this interface
            if (!ioctl(fd, SIOCGIFADDR, (char*)&buf[interfaceNum]))
            {
                memset(ip, 0, MAX_IP_LEN);

                snprintf(ip, sizeof(ip), "%s",
                         (char*)inet_ntoa(((struct sockaddr_in*) & (buf[interfaceNum].ifr_addr))->sin_addr));
                printf("IsIPAddrIsLocalIP() InterFace:eth_name=%s, ip=%s\r\n", buf[interfaceNum].ifr_name, ip);

                if (0 == strcmp(strEthName, buf[interfaceNum].ifr_name))
                {
                    if (0 == strcmp(strIPAddr, ip))
                    {
                        close(fd);
                        return 1;
                    }
                }
            }
            else
            {
                printf("IsIPAddrIsLocalIP() exit---: ioctl: %s [%s:%d]\n", strerror(errno), __FILE__, __LINE__);
                close(fd);
                return 0;
            }
        }
    }
    else
    {
        printf("IsIPAddrIsLocalIP() exit---: ioctl: %s [%s:%d]\n", strerror(errno), __FILE__, __LINE__);
        close(fd);
        return 0;
    }

    close(fd);
    return 0;
}

/*****************************************************************************
 �� �� ��  : GetSystemRunStateProc
 ��������  : ICE�ӿڻ�ȡϵͳ������Ϣ�Ĵ���
 �������  : std::string& inBuff
                            std::string& outBuff
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��4��17�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int GetSystemRunStateProc(const std::string& inBuff, std::string& outBuff)
{
    int i = 0;
    int iRet = 0;
    xml_type_t xml_type = XML_TYPE_NULL;
    CPacket inPacket;
    vector<string> NodeName_Vector;
    char strSN[128] = {0};
    char strRouteID[128] = {0};
    char strCMSID[128] = {0};
    char strDBName[128] = {0};
    char strCMD[128] = {0};
    char strUserName[64] = {0};
    char strDeviceID[32] = {0};
    GBDevice_info_t * pGBDeviceInfo = NULL;
    FILE   *stream = NULL;
    char   buf[256] = {0};
    string strReturnBuff = "";

    if (inBuff.empty())
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "GetSystemRunStateProc() exit---: Param Error \r\n");
        return -1;
    }

    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "GetSystemRunStateProc() InBuffer=%s \r\n", inBuff.c_str());

    //����XML
    iRet = inPacket.BuiltTree(inBuff.c_str(), inBuff.length());//����DOM���ṹ.

    if (iRet < 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR,  "GetSystemRunStateProc() exit---: XML Build Tree Error \r\nmsg=%s \r\n", inBuff.c_str());
        return iRet;
    }

    NodeName_Vector.clear();
    DOMDocument* pDOMDocument = inPacket.GetDOMDocument();
    DOMElement* pDOMElement = pDOMDocument->get_root();
    pDOMElement->ClearNodeNumber();

    if (pDOMElement->GetNodeName(NodeName_Vector) <= 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "GetSystemRunStateProc() exit---: Get Node Name Error \r\n");
        return -1;
    }

    /* ������xml����Ϣ���� */
    xml_type = get_xml_type_from_xml_body(NodeName_Vector, inPacket);
    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "GetSystemRunStateProc() get_xml_type_from_xml_body:xml_type=%d \r\n", xml_type);

    inPacket.GetElementValue((char*)"serial_number", strSN);

#if 0

    if (strSN[0] == '\0')
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "GetSystemRunStateProc() exit---: Get Serial Numbere Error \r\n");
        return -1;
    }

#endif

    if (0 == cms_run_status)
    {
        if (XML_NOTIFY_REBOOT != xml_type && XML_NOTIFY_SHUTDOWN != xml_type && XML_NOTIFY_SYNC_DATABASE != xml_type)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "GetSystemRunStateProc() exit---: CMS Not Run Normal Error \r\n");
            return -1;
        }
    }

    switch (xml_type)
    {
        case XML_GET_SYSTEMINFO : /* ��ȡϵͳ��Ϣ */
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "GetSystemRunStateProc() GetSystemInfoProc \r\n");
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Webͨ��ICE��ȡϵͳ��Ϣ��Ϣ����");
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Web access system info message processing through ICE");
            i = GetSystemInfoProc(strSN, outBuff);
            break;

        case XML_GET_USERINFO :   /* ��ȡ�û���Ϣ */
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "GetSystemRunStateProc() GetUserInfoProc \r\n");
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Webͨ��ICE��ȡ�û���Ϣ��Ϣ����");
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Web access user info message processing through ICE");
            i = GetUserInfoProc(strSN, outBuff);
            break;

        case XML_GET_DEVICEINFO : /* ��ȡ�豸��Ϣ */
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "GetSystemRunStateProc() GetDeviceInfoProc \r\n");
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Webͨ��ICE��ȡ�豸��Ϣ��Ϣ����");
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Web access device info message processing through ICE");
            i = GetDeviceInfoProc(strSN, outBuff);
            break;

        case XML_GET_TSUINFO :    /* ��ȡTSU ��Ϣ */
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "GetSystemRunStateProc() GetTSUInfoProc \r\n");
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Webͨ��ICE��ȡTSU��Ϣ��Ϣ����");
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Web access TSU info message processing through ICE");
            i = GetTSUInfoProc(strSN, outBuff);
            break;

        case XML_GET_TASKINFO :   /* ��ȡ������Ϣ */
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "GetSystemRunStateProc() GetTaskInfoProc \r\n");
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Webͨ��ICE��ȡ������Ϣ��Ϣ����");
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Web access task info message processing through ICE");
            i = GetTaskInfoProc(strSN, outBuff);
            break;

        case XML_SET_IPADDR :   /* ����IP��ַ */
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "GetSystemRunStateProc() SetIPAddrProc \r\n");
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Webͨ��ICE���õ���IP��ַ��Ϣ����");
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Web set board IP address message processing through ICE");
            i = SetIPAddrProc(inPacket, outBuff);
            break;

        case XML_NOTIFY_REBOOT : /* �������� */
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "GetSystemRunStateProc() BoardReboot \r\n");
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Webͨ��ICE֪ͨ����������Ϣ����");
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Web notify restarting board message processing through ICE");
            BoardReboot();
            break;

        case XML_NOTIFY_SHUTDOWN : /* �رյ��� */
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "GetSystemRunStateProc() TurnOnffBoard \r\n");
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Webͨ��ICE֪ͨ�رյ�����Ϣ����");
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Web notify closing board message processing through ICE");
            //TurnOnffBoard();
            break;

        case XML_NOTIFY_SENDCATALOG_TO_ROUTE_CMS : /* ���ϼ�CMS����Ŀ¼ */
            inPacket.GetElementValue((char*)"routeid", strRouteID);

            if (strRouteID[0] != '\0')
            {
                iRet = SendNotifyCatalogMessageToRouteCMS(osip_atoi(strRouteID));
            }

            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "GetSystemRunStateProc() SendCatalog To Route CMS:RouteID=%s, iRet=%d \r\n", strRouteID, iRet);
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Webͨ��ICE֪ͨ����Ŀ¼��Ϣ���ϼ�CMS�Ĵ���:RouteID=%s, iRet=%d \r\n", strRouteID, iRet);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Web notify send directory message To Route CMS processing through ICE:RouteID=%s, iRet=%d \r\n", strRouteID, iRet);

            break;

        case XML_NOTIFY_SENDCATALOG_TO_SUB_CMS : /* ���¼�CMS����Ŀ¼ */
            inPacket.GetElementValue((char*)"cmsid", strCMSID);

            if (strCMSID[0] != '\0')
            {
                iRet = SendNotifyCatalogMessageToSubCMS(strCMSID);
            }

            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "GetSystemRunStateProc() SendCatalog To Sub CMS:CMSID=%s, iRet=%d\r\n", strCMSID, iRet);
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Webͨ��ICE֪ͨ����Ŀ¼��Ϣ���¼�CMS�Ĵ���:CMSID=%s, iRet=%d\r\n", strCMSID, iRet);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Web notify send Catalog message To Sub CMS processing through ICE:CMSID=%s, iRet=%d\r\n", strCMSID, iRet);

            break;

        case XML_NOTIFY_REBOOTKEYBOARD : /* ֪ͨ�������̷������ */
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "GetSystemRunStateProc() : restart KeyBoardCtrl\r\n");
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Webͨ��ICE֪ͨ�������̷������KeyBoardCtrl����");
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Web notify restart KeyBoardCtrl process processing through ICE");
            i = BoardSearchRebootKeyBoardProc();
            break;

        case XML_NOTIFY_REBOOTMEDIASERVICE : /* ֪ͨ�����Դ�ý�����س��� */
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "GetSystemRunStateProc() : restart EV9000MediaService\r\n");
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Webͨ��ICE֪ͨ�����Դ�ý�����ؽ���EV9000MediaService����");
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Web notify restart EV9000MediaService process processing through ICE");
            i = BoardSearchRebootMediaServiceProc();
            break;

        case XML_NOTIFY_RESTORE_FACTORY : /* ֪ͨ�ָ��������� */
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "GetSystemRunStateProc() : Restore factory settings\r\n");
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Webͨ��ICE֪ͨ�ָ��������ô���");
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Web notify restore factory settings processing through ICE");
            i = BoardSearchRestoreFactorySettingsProc();
            break;

        case XML_NOTIFY_EXECUTE_CMD : /* ִ֪ͨ������ */
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "GetSystemRunStateProc() : Execute cmd settings\r\n");
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Webͨ��ICEִ֪ͨ�������");
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Web notify execute cmd processing through ICE");
            inPacket.GetElementValue((char*)"CMD", strCMD);

            if (strCMD[0] != '\0')
            {
                strReturnBuff.clear();
                memset(buf, '\0', 256);  //��ʼ��buf,�������д�����뵽�ļ���
                stream = popen(strCMD, "r");   //����ls ��l���������� ͨ���ܵ���ȡ����r����������FILE* stream

                if (NULL != stream)
                {
                    while ((fgets(buf, 256, stream)) != NULL)
                    {
                        if (strReturnBuff.empty())
                        {
                            strReturnBuff = buf;
                            strReturnBuff += "\r\n";
                        }
                        else
                        {
                            strReturnBuff += buf;
                            strReturnBuff += "\r\n";
                        }

                        memset(buf, '\0', 256);  //��ʼ��buf,�������д�����뵽�ļ���
                    }

                    pclose(stream);
                    stream = NULL;

                    outBuff = strReturnBuff;
                }
            }

            break;

        case XML_NOTIFY_REFRESH_DBCONFIG : /* Web֪ͨˢ�����ݿ����� */
            inPacket.GetElementValue((char*)"tablename", strDBName);

            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "GetSystemRunStateProc() : Refresh DB Config\r\n");
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Webͨ��ICE֪ͨˢ�����ݿ�����:tablename=%s", strDBName);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Web notify refresh db config through ICE");

            if (strDBName[0] != '\0')
            {
                iRet = WebNotifyDBRefreshProc(strDBName);
            }

            break;

        case XML_NOTIFY_SYNC_DATABASE : /* Web֪ͨͬ�����ݿ����� */

            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "GetSystemRunStateProc() : Sync DB Config\r\n");
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Webͨ��ICE֪ͨ����ͬ�����ݿ�����");
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Web notify sync db config through ICE");

            iRet = WebNotifyDBSyncProc();
            break;

        case XML_NOTIFY_ENTER_SYSCONFIG : /* Web֪ͨCMS�������� */
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "GetSystemRunStateProc() : Enter System settings\r\n");
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Webͨ��ICE֪ͨ����ϵͳ���ô���");
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Web notify enter system settings processing through ICE");
            inPacket.GetElementValue((char*)"username", strUserName);

            if (strUserName[0] != '\0')
            {
                //iRet = shdb_user_operate_cmd_proc2(strUserName, EV9000_SHDB_DVR_PARAM_CONFIG);
            }

            break;

        case XML_NOTIFY_SAVE_SYSCONFIG : /* Web֪ͨCMS�������� */
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "GetSystemRunStateProc() : Save System settings\r\n");
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Webͨ��ICE֪ͨ����ϵͳ���ô���");
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Web notify save system settings processing through ICE");
            inPacket.GetElementValue((char*)"username", strUserName);

            if (strUserName[0] != '\0')
            {
                //iRet = shdb_user_operate_cmd_proc2(strUserName, EV9000_SHDB_DVR_PARAM_COMMIT);
            }

            break;

        case XML_NOTIFY_GET_VMS_CHANNEL : /* Web֪ͨ��ȡVMS��ͨ����Ϣ */
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "GetSystemRunStateProc() : Get VMS Channel Info \r\n");
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Webͨ��ICE֪ͨ��ȡVMSͨ����Ϣ����");
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Web notify get VMS channel info processing through ICE");

            if (0 == g_GetChannelFlag)
            {
                g_GetChannelFlag = 1;
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Webͨ��ICE֪ͨ��ȡVMSͨ����Ϣ����ɹ�!");
            }
            else
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Webͨ��ICE֪ͨ��ȡVMSͨ����Ϣ����ʧ��,�ϴλ�ȡ���̻�û�н���!");
            }

            break;

        case XML_NOTIFY_GET_DEVICE_INFO : /* Web֪ͨ��ȡǰ���豸��Ϣ */
            inPacket.GetElementValue((char*)"DeviceID", strDeviceID);
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "GetSystemRunStateProc() : Get Device Info:DeviceID=%s \r\n", strDeviceID);
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Webͨ��ICE֪ͨ��ȡǰ�������豸��Ϣ:DeviceID=%s", strDeviceID);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Web notify get device info through ICE:DeviceID=%s", strDeviceID);

            if (strDeviceID[0] != '\0')
            {
                pGBDeviceInfo = GBDevice_info_find(strDeviceID);

                if (NULL != pGBDeviceInfo)
                {
                    iRet = SendQueryDeviceInfoMessage(pGBDeviceInfo);

                    if (0 != iRet)
                    {
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Webͨ��ICE֪ͨ��ȡǰ�������豸��Ϣʧ��:ԭ��=������Ϣ��ǰ�������豸ʧ��:DeviceID=%s \r\n", strDeviceID);
                    }
                }
                else
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Webͨ��ICE֪ͨ��ȡǰ�������豸��Ϣʧ��:ԭ��=��ȡ�����豸��Ϣʧ��:DeviceID=%s \r\n", strDeviceID);
                }
            }
            else
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Webͨ��ICE֪ͨ��ȡǰ�������豸��Ϣʧ��:ԭ��=DeviceIDΪ��");
            }

            break;

        case XML_NOTIFY_GET_DEVICE_STATUS : /* Web֪ͨ��ȡǰ���豸״̬ */
            inPacket.GetElementValue((char*)"DeviceID", strDeviceID);
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "GetSystemRunStateProc() : Get Device Status:DeviceID=%s \r\n", strDeviceID);
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Webͨ��ICE֪ͨ��ȡǰ�������豸״̬:DeviceID=%s", strDeviceID);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Web notify get device status through ICE:DeviceID=%s", strDeviceID);

            if (strDeviceID[0] != '\0')
            {
                pGBDeviceInfo = GBDevice_info_find(strDeviceID);

                if (NULL != pGBDeviceInfo)
                {
                    iRet = SendQueryDeviceStatusMessage(pGBDeviceInfo);

                    if (0 != iRet)
                    {
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Webͨ��ICE֪ͨ��ȡǰ�������豸״̬ʧ��:ԭ��=������Ϣ��ǰ�������豸ʧ��:DeviceID=%s \r\n", strDeviceID);
                    }
                }
                else
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Webͨ��ICE֪ͨ��ȡǰ�������豸״̬ʧ��:ԭ��=��ȡ�����豸��Ϣʧ��:DeviceID=%s \r\n", strDeviceID);
                }
            }
            else
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Webͨ��ICE֪ͨ��ȡǰ�������豸״̬ʧ��:ԭ��=DeviceIDΪ��");
            }

            break;

        case XML_NOTIFY_GET_DEVICE_CATALOG : /* Web֪ͨ��ȡǰ���豸Ŀ¼ */
            inPacket.GetElementValue((char*)"DeviceID", strDeviceID);
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "GetSystemRunStateProc() : Get Device Catalog:DeviceID=%s \r\n", strDeviceID);
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Webͨ��ICE֪ͨ��ȡǰ�������豸Ŀ¼:DeviceID=%s", strDeviceID);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Web notify get device catalog through ICE:DeviceID=%s", strDeviceID);

            if (strDeviceID[0] != '\0')
            {
                pGBDeviceInfo = GBDevice_info_find(strDeviceID);

                if (NULL != pGBDeviceInfo)
                {
                    if (pGBDeviceInfo->device_type == EV9000_DEVICETYPE_SIPSERVER
                        && pGBDeviceInfo->link_type == 0
                        && pGBDeviceInfo->three_party_flag == 0) /* �ǵ�����ƽ̨ */
                    {
                        /*������¼�CMS, ���Ҳ���ͬ�������������ȡ�߼��豸������Ϣ���߼��豸�����ϵ��Ϣ */
                        iRet = SendQueryDeviceGroupInfoMessage(pGBDeviceInfo);

                        if (0 != iRet)
                        {
                            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Webͨ��ICE֪ͨ��ȡ�¼�ƽ̨������Ϣʧ��:ԭ��=������Ϣ��ǰ�������豸ʧ��:DeviceID=%s \r\n", strDeviceID);
                        }

                        iRet = SendQueryDeviceGroupMapInfoMessage(pGBDeviceInfo);

                        if (0 != iRet)
                        {
                            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Webͨ��ICE֪ͨ��ȡ�¼�ƽ̨�����ϵ��Ϣʧ��:ԭ��=������Ϣ��ǰ�������豸ʧ��:DeviceID=%s \r\n", strDeviceID);
                        }
                    }

                    iRet = SendQueryDeviceCatalogMessage(pGBDeviceInfo);

                    if (0 != iRet)
                    {
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Webͨ��ICE֪ͨ��ȡǰ�������豸Ŀ¼ʧ��:ԭ��=������Ϣ��ǰ�������豸ʧ��:DeviceID=%s \r\n", strDeviceID);
                    }
                }
                else
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Webͨ��ICE֪ͨ��ȡǰ�������豸Ŀ¼ʧ��:ԭ��=��ȡ�����豸��Ϣʧ��:DeviceID=%s \r\n", strDeviceID);
                }
            }
            else
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Webͨ��ICE֪ͨ��ȡǰ�������豸Ŀ¼ʧ��:ԭ��=DeviceIDΪ��");
            }

            break;

        case XML_NOTIFY_DEVICE_TELEBOOT : /* Web֪ͨǰ���豸Զ������ */
            inPacket.GetElementValue((char*)"DeviceID", strDeviceID);
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "GetSystemRunStateProc() : Notify Device teleboot:DeviceID=%s \r\n", strDeviceID);
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Webͨ��ICE֪ͨǰ�������豸Զ������:DeviceID=%s", strDeviceID);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Web notify device teleboot through ICE:DeviceID=%s", strDeviceID);

            if (strDeviceID[0] != '\0')
            {
                pGBDeviceInfo = GBDevice_info_find(strDeviceID);

                if (NULL != pGBDeviceInfo)
                {
                    iRet = SendDeviceTeleBootMessage(pGBDeviceInfo);

                    if (0 != iRet)
                    {
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Webͨ��ICE֪ͨǰ�������豸Զ������ʧ��:ԭ��=������Ϣ��ǰ�������豸ʧ��:DeviceID=%s \r\n", strDeviceID);
                    }
                }
                else
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Webͨ��ICE֪ͨǰ�������豸Զ������ʧ��:ԭ��=��ȡ�����豸��Ϣʧ��:DeviceID=%s \r\n", strDeviceID);
                }
            }
            else
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Webͨ��ICE֪ͨǰ�������豸Զ������ʧ��:ԭ��=DeviceIDΪ��");
            }

            break;

        default:
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "GetSystemRunStateProc() exit---: Not Support Message Type:%d \r\n", xml_type);
            return 0;
    }

    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "GetSystemRunStateProc() OutBuffer=%s \r\n", outBuff.c_str());

    return 0;
}

/*****************************************************************************
 �� �� ��  : GeneratingErrorResponseForSystemRunState
 ��������  : ICE�ӿڻ�ȡϵͳ������Ϣʱ�����ɴ���Ӧ����
 �������  : char* strCmdType
                            char* pcSerialNumber
                            int iErrorCode
                            char* strErrorReason
                            std::string& outBuff
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��4��17�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int GeneratingErrorResponseForSystemRunState(char * strCmdType, char * pcSerialNumber, int iErrorCode, char * strErrorReason, std::string & outBuff)
{
    time_t utc_time;
    struct tm local_time = { 0 };
    char strDateTime[48] = {0};

    CPacket outPacket;
    DOMElement* AccNode = NULL;
    DOMElement* ErrorItemAccNode = NULL;
    char strErrorCode[32] = {0};

    utc_time = time(NULL);
    localtime_r(&utc_time, &local_time);

    strftime(strDateTime, sizeof(strDateTime), "%Y-%m-%d %H:%M:%S", &local_time);

    /* �ظ���Ӧ,�齨��Ϣ */
    outPacket.SetRootTag("response");

    AccNode = outPacket.CreateElement((char*)"cmdtype");
    outPacket.SetElementValue(AccNode, strCmdType);

    AccNode = outPacket.CreateElement((char*)"datetime");
    outPacket.SetElementValue(AccNode, strDateTime);

    AccNode = outPacket.CreateElement((char*)"serial_number");
    outPacket.SetElementValue(AccNode, pcSerialNumber);

    AccNode = outPacket.CreateElement((char*)"responser");
    outPacket.SetElementValue(AccNode, (char*)"CMS");

    AccNode = outPacket.CreateElement((char*)"success");
    outPacket.SetElementValue(AccNode, (char*)"false");

    AccNode = outPacket.CreateElement((char*)"error");
    outPacket.SetCurrentElement(AccNode);

    ErrorItemAccNode = outPacket.CreateElement((char*)"errorcode");
    sprintf(strErrorCode, "%d", iErrorCode);
    outPacket.SetElementValue(ErrorItemAccNode, (char*)"false");

    ErrorItemAccNode = outPacket.CreateElement((char*)"errormessage");
    outPacket.SetElementValue(ErrorItemAccNode, strErrorReason);

    ErrorItemAccNode = outPacket.CreateElement((char*)"errorlogfile");
    outPacket.SetElementValue(ErrorItemAccNode, (char*)"/data/log");

    outBuff = outPacket.GetXml(NULL);

    return 0;
}

/*****************************************************************************
 �� �� ��  : CreateXMLHeadResponseForSystemRunState
 ��������  : ����ICE�ӿڻ�ȡϵͳ������Ϣ��Ӧ����ϢXMLͷ��
 �������  : CPacket** pOutPacket
                            char* strCmdType
                            char* pcSerialNumber
                            DOMElement** ListItemNode
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��4��17�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int CreateXMLHeadResponseForSystemRunState(CPacket** pOutPacket, char * strCmdType, char * pcSerialNumber)
{
    time_t utc_time;
    struct tm local_time = { 0 };
    char strDateTime[48] = {0};

    DOMElement* AccNode = NULL;

    *pOutPacket = new CPacket();

    if (NULL == *pOutPacket)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "CreateXMLHeadResponseForSystemRunState() exit---: Create XML Packet Error \r\n");
        return -1;
    }

    utc_time = time(NULL);
    localtime_r(&utc_time, &local_time);

    strftime(strDateTime, sizeof(strDateTime), "%Y-%m-%d %H:%M:%S", &local_time);

    /* �ظ���Ӧ,�齨��Ϣ */
    (*pOutPacket)->SetRootTag("response");

    AccNode = (*pOutPacket)->CreateElement((char*)"cmdtype");
    (*pOutPacket)->SetElementValue(AccNode, strCmdType);

    AccNode = (*pOutPacket)->CreateElement((char*)"datetime");
    (*pOutPacket)->SetElementValue(AccNode, strDateTime);

    AccNode = (*pOutPacket)->CreateElement((char*)"serial_number");
    (*pOutPacket)->SetElementValue(AccNode, pcSerialNumber);

    AccNode = (*pOutPacket)->CreateElement((char*)"responser");
    (*pOutPacket)->SetElementValue(AccNode, (char*)"CMS");

    AccNode = (*pOutPacket)->CreateElement((char*)"success");
    (*pOutPacket)->SetElementValue(AccNode, (char*)"true");

    return 0;
}

/*****************************************************************************
 �� �� ��  : AddSystemDateTimeInfoToSysInfoXML
 ��������  : ���ϵͳʱ�䵽ICE�ӿڻ�ȡϵͳ������Ϣ��Ӧ����Ϣ
 �������  : CPacket* pOutPacket
                            DOMElement* ListItemNode
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��4��17�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int AddSystemDateTimeInfoToSysInfoXML(CPacket * pOutPacket)
{
    DOMElement* ItemAccNode = NULL;
    DOMElement* ItemInfoAccNode = NULL;

    time_t utc_time;
    struct tm local_time = {0};
    char strDateTime[48] = {0};

    if (NULL == pOutPacket)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "AddSystemDateTimeInfoToSysInfoXML() exit---: Param Error \r\n");
        return -1;
    }

    /* ��дXML����*/
    ItemAccNode = pOutPacket->CreateElement((char*)"systemdatetime");
    pOutPacket->SetCurrentElement(ItemAccNode);

    ItemInfoAccNode = pOutPacket->CreateElement((char*)"utc");
    pOutPacket->SetElementValue(ItemInfoAccNode, (char*)"false");

    ItemInfoAccNode = pOutPacket->CreateElement((char*)"timezone");
    pOutPacket->SetElementValue(ItemInfoAccNode, (char*)"8");

    ItemInfoAccNode = pOutPacket->CreateElement((char*)"now");
    utc_time = time(NULL);
    localtime_r(&utc_time, &local_time);
    strftime(strDateTime, sizeof(strDateTime), "%Y-%m-%d %H:%M:%S", &local_time);
    pOutPacket->SetElementValue(ItemInfoAccNode, strDateTime);

    return 0;
}

/*****************************************************************************
 �� �� ��  : AddCPUInfoToSysInfoXML
 ��������  : ���CPU��Ϣ��ICE�ӿڻ�ȡϵͳ������Ϣ��Ӧ����Ϣ
 �������  : CPacket* pOutPacket
                            DOMElement* ListItemNode
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��4��17�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int AddCPUInfoToSysInfoXML(CPacket * pOutPacket)
{
    int i = 0;
    int process_num = 0;
    char strProcessNum[16] = {0};
    char strCPUUse[32] = {0};
    float fShelfCpuTemp = 0x00;
    char strCPUTemp[32] = {0};
    char strProcessCount[16] = {0};

    float cms_cpu_p = 0.0;
    char strCMSCPUP[32] = {0};
    float onvif_cpu_p = 0.0;
    char strONVIFCPUP[32] = {0};
    float mysql_cpu_p = 0.0;
    char strMySQLCPUP[32] = {0};

    cpu_info cpu_inf;
    cpu_status cpu_stat;

    DOMElement* ItemAccNode = NULL;
    DOMElement* ItemInfoAccNode = NULL;
    DOMElement* CoreItemAccNode = NULL;
    DOMElement* CoreItemInfoAccNode = NULL;
    DOMElement* SoftItemAccNode = NULL;
    DOMElement* SoftItemInfoAccNode = NULL;

    if (NULL == pOutPacket)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "AddCPUInfoToSysInfoXML() exit---: Param Error \r\n");
        return -1;
    }

    /* ��ȡCPU����Ϣ */
    memset(&cpu_inf, 0, sizeof(cpu_info));
    process_num = get_cpu_info(&cpu_inf);
    //printf("\r\n CPU Process Total Number: %d \r\n", process_num);
    //printf("System Type: %s \r\n", cpu_inf.system_type);
    //printf("CPU Model: %s \r\n", cpu_inf.cpu_model);
    //printf("CPU BogoMIPS: %s \r\n", cpu_inf.BogoMIPS);

    //����cpuʹ����
    memset(&cpu_stat, 0, sizeof(cpu_status));
    get_cpu_status(&cpu_stat, -1);
    //printf("user\t nice\t system\t idle\n");
    //printf("%4.2f\t %4.2f\t %4.2f\t %4.2f\n", cpu_stat.user, cpu_stat.nice, cpu_stat.system, cpu_stat.idle);

    /* ��дXML����*/
    ItemAccNode = pOutPacket->CreateElement((char*)"cpu");
    pOutPacket->SetCurrentElement(ItemAccNode);

    /* ϵͳ���� */
    ItemInfoAccNode = pOutPacket->CreateElement((char*)"system_type");
    pOutPacket->SetElementValue(ItemInfoAccNode, cpu_inf.system_type);

    /* BogoMIPS */
    ItemInfoAccNode = pOutPacket->CreateElement((char*)"BogoMIPS");
    pOutPacket->SetElementValue(ItemInfoAccNode, cpu_inf.BogoMIPS);

    /* ʹ���� */
    ItemInfoAccNode = pOutPacket->CreateElement((char*)"use");
    sprintf(strCPUUse, "%4.2f", cpu_stat.user + cpu_stat.nice + cpu_stat.system);
    pOutPacket->SetElementValue(ItemInfoAccNode, strCPUUse);

    /* �˵ĸ��� */
    ItemInfoAccNode = pOutPacket->CreateElement((char*)"cores");
    sprintf(strProcessNum, "%d", process_num);
    pOutPacket->SetElementValue(ItemInfoAccNode, strProcessNum);

    /* �¶� */
    ItemInfoAccNode = pOutPacket->CreateElement((char*)"temperature");

    sprintf(strCPUTemp, "%4.2f", fShelfCpuTemp);
    pOutPacket->SetElementValue(ItemInfoAccNode, strCPUTemp);

    /* ѭ�����ÿ���˵���Ϣ */
    for (i = 0; i < process_num; i++)
    {
        memset(&cpu_stat, 0, sizeof(cpu_status));
        get_cpu_status(&cpu_stat, i);
        //printf("\r\n CPU Process Number: %d \r\n", i);
        //printf("user\t nice\t system\t idle\n");
        //printf("%4.2f\t %4.2f\t %4.2f\t %4.2f\n", cpu_stat.user, cpu_stat.nice, cpu_stat.system, cpu_stat.idle);

        pOutPacket->SetCurrentElement(ItemAccNode);
        CoreItemAccNode = pOutPacket->CreateElement((char*)"core");

        pOutPacket->SetCurrentElement(CoreItemAccNode);

        /* �����к�*/
        CoreItemInfoAccNode = pOutPacket->CreateElement((char*)"processor");
        memset(strProcessCount, 0, 16);
        sprintf(strProcessCount, "%d", i);
        pOutPacket->SetElementValue(CoreItemInfoAccNode, strProcessCount);

        /* ���ͺ�*/
        CoreItemInfoAccNode = pOutPacket->CreateElement((char*)"cpu_modle");
        pOutPacket->SetElementValue(CoreItemInfoAccNode, cpu_inf.cpu_model);

        /* BogoMIPS */
        CoreItemInfoAccNode = pOutPacket->CreateElement((char*)"BogoMIPS");
        pOutPacket->SetElementValue(CoreItemInfoAccNode, cpu_inf.BogoMIPS);

        /* ʹ���� */
        CoreItemInfoAccNode = pOutPacket->CreateElement((char*)"use");
        sprintf(strCPUUse, "%4.2f", cpu_stat.user + cpu_stat.nice + cpu_stat.system);
        pOutPacket->SetElementValue(CoreItemInfoAccNode, strCPUUse);
    }

    /* ����Ӧ�������Ϣ */
    /* 1����ȡCMS ��CPU ռ���� */
    i = get_progress_cpu_usage((char*)"cms", &cms_cpu_p);
    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "AddSystemDateTimeInfoToSysInfoXML() cms_cpu_p=%.6f \r\n", cms_cpu_p);

    pOutPacket->SetCurrentElement(ItemAccNode);
    SoftItemAccNode = pOutPacket->CreateElement((char*)"software");

    pOutPacket->SetCurrentElement(SoftItemAccNode);

    /* ����*/
    SoftItemInfoAccNode = pOutPacket->CreateElement((char*)"name");
    pOutPacket->SetElementValue(SoftItemInfoAccNode, (char*)"CMS");

    /* ռ����*/
    SoftItemInfoAccNode = pOutPacket->CreateElement((char*)"use");
    sprintf(strCMSCPUP, "%.6f", cms_cpu_p);
    pOutPacket->SetElementValue(SoftItemInfoAccNode, strCMSCPUP);

    /* 2����ȡMySQL ��CPU ռ���� */
    i = get_progress_cpu_usage((char*)"mysqld_safe", &mysql_cpu_p);
    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "AddSystemDateTimeInfoToSysInfoXML() mysqld=%.6f \r\n", mysql_cpu_p);

    pOutPacket->SetCurrentElement(ItemAccNode);
    SoftItemAccNode = pOutPacket->CreateElement((char*)"software");

    pOutPacket->SetCurrentElement(SoftItemAccNode);

    /* ����*/
    SoftItemInfoAccNode = pOutPacket->CreateElement((char*)"name");
    pOutPacket->SetElementValue(SoftItemInfoAccNode, (char*)"MySQL");

    /* ռ����*/
    SoftItemInfoAccNode = pOutPacket->CreateElement((char*)"use");
    sprintf(strMySQLCPUP, "%.6f", mysql_cpu_p);
    pOutPacket->SetElementValue(SoftItemInfoAccNode, strMySQLCPUP);

    /* 3����ȡOnvif ��CPU ռ���� */
    i = get_progress_cpu_usage((char*)"EV9000MediaService", &onvif_cpu_p);
    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "AddSystemDateTimeInfoToSysInfoXML() onvif_cpu_p=%.6f \r\n", onvif_cpu_p);

    pOutPacket->SetCurrentElement(ItemAccNode);
    SoftItemAccNode = pOutPacket->CreateElement((char*)"software");

    pOutPacket->SetCurrentElement(SoftItemAccNode);

    /* ����*/
    SoftItemInfoAccNode = pOutPacket->CreateElement((char*)"name");
    pOutPacket->SetElementValue(SoftItemInfoAccNode, (char*)"OnvifProxy");

    /* ռ����*/
    SoftItemInfoAccNode = pOutPacket->CreateElement((char*)"use");
    sprintf(strONVIFCPUP, "%.6f", onvif_cpu_p);
    pOutPacket->SetElementValue(SoftItemInfoAccNode, strONVIFCPUP);

    return 0;
}

/*****************************************************************************
 �� �� ��  : AddMemoryInfoToSysInfoXML
 ��������  : ����ڴ���Ϣ��ICE�ӿڻ�ȡϵͳ������Ϣ��Ӧ����Ϣ
 �������  : CPacket* pOutPacket
                            DOMElement* ListItemNode
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��4��17�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int AddMemoryInfoToSysInfoXML(CPacket * pOutPacket)
{
    int i = 0;
    mem_info mem_inf;
    process_mem_info proc_mem;

    unsigned long total = 0, free = 0, used = 0;
    char strUsedMem[64] = {0};

    DOMElement* ItemAccNode = NULL;
    DOMElement* ItemInfoAccNode = NULL;
    DOMElement* SoftItemAccNode = NULL;
    DOMElement* SoftItemInfoAccNode = NULL;

    if (NULL == pOutPacket)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "AddMemoryInfoToSysInfoXML() exit---: Param Error \r\n");
        return -1;
    }

    //��ȡ�ڴ�
    memset(&mem_inf, 0, sizeof(mem_info));
    get_mem_info(&mem_inf);
    //printf("total=\t%s\n", mem_inf.total);
    //printf("free=\t%s\n", mem_inf.free);

    /* ��дXML����*/
    ItemAccNode = pOutPacket->CreateElement((char*)"memory");
    pOutPacket->SetCurrentElement(ItemAccNode);

    /* �ܴ�С */
    ItemInfoAccNode = pOutPacket->CreateElement((char*)"size");
    pOutPacket->SetElementValue(ItemInfoAccNode, mem_inf.total);

    /* ���ÿռ� */
    ItemInfoAccNode = pOutPacket->CreateElement((char*)"used");
    total = strtoul(mem_inf.total, NULL, 10);
    free = strtoul(mem_inf.free, NULL, 10);
    used = total - free;
    sprintf(strUsedMem, "%ld", used);
    pOutPacket->SetElementValue(ItemInfoAccNode, strUsedMem);

    /* ����Ӧ�������Ϣ */
    /* 1����ȡCMS ���ڴ�ʹ����� */
    memset(&proc_mem, 0, sizeof(process_mem_info));
    i = get_progress_memory_usage((char*)"cms", &proc_mem);
    //printf("CMS Phy Mem=\t%s\n, V Mem=\t%s\n", proc_mem.mem, proc_mem.vmem);

    pOutPacket->SetCurrentElement(ItemAccNode);
    SoftItemAccNode = pOutPacket->CreateElement((char*)"software");

    pOutPacket->SetCurrentElement(SoftItemAccNode);

    /* ����*/
    SoftItemInfoAccNode = pOutPacket->CreateElement((char*)"name");
    pOutPacket->SetElementValue(SoftItemInfoAccNode, (char*)"CMS");

    /* ��ʹ��*/
    SoftItemInfoAccNode = pOutPacket->CreateElement((char*)"used");
    pOutPacket->SetElementValue(SoftItemInfoAccNode, proc_mem.mem);

    /* 2����ȡMySQL ��CPU ռ���� */
    memset(&proc_mem, 0, sizeof(process_mem_info));
    i = get_progress_memory_usage((char*)"mysqld_safe", &proc_mem);
    //printf("Phy Mem=\t%s\n", proc_mem.mem);
    //printf("V Mem=\t%s\n", proc_mem.vmem);

    pOutPacket->SetCurrentElement(ItemAccNode);
    SoftItemAccNode = pOutPacket->CreateElement((char*)"software");

    pOutPacket->SetCurrentElement(SoftItemAccNode);

    /* ����*/
    SoftItemInfoAccNode = pOutPacket->CreateElement((char*)"name");
    pOutPacket->SetElementValue(SoftItemInfoAccNode, (char*)"MySQL");

    /* ��ʹ��*/
    SoftItemInfoAccNode = pOutPacket->CreateElement((char*)"used");
    pOutPacket->SetElementValue(SoftItemInfoAccNode, proc_mem.mem);

    /* 3����ȡOnvif ��CPU ռ���� */
    memset(&proc_mem, 0, sizeof(process_mem_info));
    i = get_progress_memory_usage((char*)"OnvifProxy", &proc_mem);
    //printf("Phy Mem=\t%s\n", proc_mem.mem);
    //printf("V Mem=\t%s\n", proc_mem.vmem);

    pOutPacket->SetCurrentElement(ItemAccNode);
    SoftItemAccNode = pOutPacket->CreateElement((char*)"software");

    pOutPacket->SetCurrentElement(SoftItemAccNode);

    /* ����*/
    SoftItemInfoAccNode = pOutPacket->CreateElement((char*)"name");
    pOutPacket->SetElementValue(SoftItemInfoAccNode, (char*)"OnvifProxy");

    /* ��ʹ��*/
    SoftItemInfoAccNode = pOutPacket->CreateElement((char*)"used");
    pOutPacket->SetElementValue(SoftItemInfoAccNode, proc_mem.mem);

    return 0;
}

/*****************************************************************************
 �� �� ��  : AddFanInfoToSysInfoXML
 ��������  :  ��ӷ�����Ϣ��ICE�ӿڻ�ȡϵͳ������Ϣ��Ӧ����Ϣ
 �������  : CPacket* pOutPacket
                            DOMElement* ListItemNode
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��4��17�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int AddFanInfoToSysInfoXML(CPacket * pOutPacket)
{
    int i = 0;
    char strFanNum[16] = {0};
    char strFanSpeed[16] = {0};

    DOMElement* ItemAccNode = NULL;
    DOMElement* fanItemAccNode = NULL;
    DOMElement* fanItemInfoAccNode = NULL;

    if (NULL == pOutPacket)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "AddFanInfoToSysInfoXML() exit---: Param Error \r\n");
        return -1;
    }

    /* ��дXML����*/
    ItemAccNode = pOutPacket->CreateElement((char*)"fans");
    pOutPacket->SetCurrentElement(ItemAccNode);

    return 0;
}

/*****************************************************************************
 �� �� ��  : AddNetWorkInfoToSysInfoXML
 ��������  :  ���������Ϣ��ICE�ӿڻ�ȡϵͳ������Ϣ��Ӧ����Ϣ
 �������  : CPacket* pOutPacket
                            DOMElement* ListItemNode
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��4��17�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int AddNetWorkInfoToSysInfoXML(CPacket * pOutPacket)
{
    int fd = 0;
    int interfaceNum = 0;
    struct ifreq buf[16];
    struct ifconf ifc;
    struct ifreq ifrcopy;
    char mac[16] = {0};
    char ip[32] = {0};
    char broadAddr[32] = {0};
    char subnetMask[32] = {0};
    char gateway[32] = {0};

    DOMElement* ItemAccNode = NULL;
    DOMElement* InterfaceItemAccNode = NULL;
    DOMElement* InterfaceItemInfoAccNode = NULL;

    if (NULL == pOutPacket)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "AddNetWorkInfoToSysInfoXML() exit---: Param Error \r\n");
        return -1;
    }

    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "AddNetWorkInfoToSysInfoXML() exit---: Socket Error \r\n");
        close(fd);
        return -1;
    }

    /* ��дXML����*/
    ItemAccNode = pOutPacket->CreateElement((char*)"network");
    pOutPacket->SetCurrentElement(ItemAccNode);

    ifc.ifc_len = sizeof(buf);
    ifc.ifc_buf = (caddr_t)buf;

    if (!ioctl(fd, SIOCGIFCONF, (char*)&ifc))
    {
        interfaceNum = ifc.ifc_len / sizeof(struct ifreq);
        //printf("interface num = %d\n", interfaceNum);

        /* ѭ�����ÿ����������Ϣ */
        while (interfaceNum-- > 0)
        {
            if (0 == sstrcmp(buf[interfaceNum].ifr_name, (char*)"lo"))
            {
                continue;
            }

            pOutPacket->SetCurrentElement(ItemAccNode);
            InterfaceItemAccNode = pOutPacket->CreateElement((char*)"ethernet");

            pOutPacket->SetCurrentElement(InterfaceItemAccNode);

            //printf("\ndevice name: %s\n", buf[interfaceNum].ifr_name);

            /* ��������*/
            InterfaceItemInfoAccNode = pOutPacket->CreateElement((char*)"name");
            pOutPacket->SetElementValue(InterfaceItemInfoAccNode, buf[interfaceNum].ifr_name);

            //ignore the interface that not up or not runing
            ifrcopy = buf[interfaceNum];

            if (ioctl(fd, SIOCGIFFLAGS, &ifrcopy))
            {
                printf("ioctl: %s [%s:%d]\n", strerror(errno), __FILE__, __LINE__);
                close(fd);
                return -1;
            }

            //get the mac of this interface
            if (!ioctl(fd, SIOCGIFHWADDR, (char*)(&buf[interfaceNum])))
            {
                memset(mac, 0, sizeof(mac));
                snprintf(mac, sizeof(mac), "%02x%02x%02x%02x%02x%02x",
                         (unsigned char)buf[interfaceNum].ifr_hwaddr.sa_data[0],
                         (unsigned char)buf[interfaceNum].ifr_hwaddr.sa_data[1],
                         (unsigned char)buf[interfaceNum].ifr_hwaddr.sa_data[2],
                         (unsigned char)buf[interfaceNum].ifr_hwaddr.sa_data[3],
                         (unsigned char)buf[interfaceNum].ifr_hwaddr.sa_data[4],
                         (unsigned char)buf[interfaceNum].ifr_hwaddr.sa_data[5]);
                //printf("device mac: %s\n", mac);

                /* MAC ��ַ*/
                InterfaceItemInfoAccNode = pOutPacket->CreateElement((char*)"mac");
                pOutPacket->SetElementValue(InterfaceItemInfoAccNode, mac);
            }
            else
            {
                printf("ioctl: %s [%s:%d]\n", strerror(errno), __FILE__, __LINE__);
                close(fd);
                return -1;
            }

            //get the IP of this interface
            if (!ioctl(fd, SIOCGIFADDR, (char*)&buf[interfaceNum]))
            {
                snprintf(ip, sizeof(ip), "%s",
                         (char*)inet_ntoa(((struct sockaddr_in*) & (buf[interfaceNum].ifr_addr))->sin_addr));
                //printf("device ip: %s\n", ip);

                /* IP  ��ַ*/
                InterfaceItemInfoAccNode = pOutPacket->CreateElement((char*)"addr");
                pOutPacket->SetElementValue(InterfaceItemInfoAccNode, ip);
            }
            else
            {
                printf("ioctl: %s [%s:%d]\n", strerror(errno), __FILE__, __LINE__);
                close(fd);
                return -1;
            }

            //get the broad address of this interface
            if (!ioctl(fd, SIOCGIFBRDADDR, &buf[interfaceNum]))
            {
                snprintf(broadAddr, sizeof(broadAddr), "%s",
                         (char*)inet_ntoa(((struct sockaddr_in*) & (buf[interfaceNum].ifr_broadaddr))->sin_addr));
                //printf("device broadAddr: %s\n", broadAddr);

                /* broadAddr  ��ַ*/
                //InterfaceItemInfoAccNode = pOutPacket->CreateElement((char*)"addr");
                //pOutPacket->SetElementValue(InterfaceItemInfoAccNode, broadAddr);
            }
            else
            {
                printf("ioctl: %s [%s:%d]\n", strerror(errno), __FILE__, __LINE__);
                close(fd);
                return -1;
            }

            //get the subnet mask of this interface
            if (!ioctl(fd, SIOCGIFNETMASK, &buf[interfaceNum]))
            {
                snprintf(subnetMask, sizeof(subnetMask), "%s",
                         (char*)inet_ntoa(((struct sockaddr_in*) & (buf[interfaceNum].ifr_netmask))->sin_addr));
                //printf("device subnetMask: %s\n", subnetMask);

                /* mask  ��ַ*/
                InterfaceItemInfoAccNode = pOutPacket->CreateElement((char*)"mask");
                pOutPacket->SetElementValue(InterfaceItemInfoAccNode, subnetMask);
            }
            else
            {
                printf("ioctl: %s [%s:%d]\n", strerror(errno), __FILE__, __LINE__);
                close(fd);
                return -1;
            }

            /* ���ص�ַ */
            memset(gateway, 0, 32);
            get_eth_gateway(buf[interfaceNum].ifr_name, gateway);
            InterfaceItemInfoAccNode = pOutPacket->CreateElement((char*)"gateway");
            pOutPacket->SetElementValue(InterfaceItemInfoAccNode, gateway);

            /* DHCP */
            InterfaceItemInfoAccNode = pOutPacket->CreateElement((char*)"dhcp");
            pOutPacket->SetElementValue(InterfaceItemInfoAccNode, (char*)"false");

            /* DHCP Server */
            InterfaceItemInfoAccNode = pOutPacket->CreateElement((char*)"dhcpserver");
            pOutPacket->SetElementValue(InterfaceItemInfoAccNode, (char*)"");

            /* DNS 1 */
            InterfaceItemInfoAccNode = pOutPacket->CreateElement((char*)"dns1");
            pOutPacket->SetElementValue(InterfaceItemInfoAccNode, (char*)"");

            /* DNS 2 */
            InterfaceItemInfoAccNode = pOutPacket->CreateElement((char*)"dns2");
            pOutPacket->SetElementValue(InterfaceItemInfoAccNode, (char*)"");

            /* DNS 3 */
            InterfaceItemInfoAccNode = pOutPacket->CreateElement((char*)"dns3");
            pOutPacket->SetElementValue(InterfaceItemInfoAccNode, (char*)"");
        }
    }
    else
    {
        printf("ioctl: %s [%s:%d]\n", strerror(errno), __FILE__, __LINE__);
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
}

/*****************************************************************************
 �� �� ��  : BoardSearchProc
 ��������  : ������������
 �������  : char* pcInBuff
                            int iInLen
                            std::string& outBuff
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��6��18�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int BoardSearchProc(char * pcInBuff, int iInLen, std::string & outBuff)
{
    int i = 0;
    int iRet = 0;
    xml_type_t xml_type = XML_TYPE_NULL;
    CPacket inPacket;
    vector<string> NodeName_Vector;
    char strSN[128] = {0};

    if (0 == cms_run_status)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "BoardSearchProc() exit---: CMS Not Run Normal Error \r\n");
        return -1;
    }

    if (NULL == pcInBuff)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "BoardSearchProc() exit---: Param Error \r\n");
        return -1;
    }

    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "BoardSearchProc() InBuffer=%s \r\n", pcInBuff);

    //����XML
    iRet = inPacket.BuiltTree(pcInBuff, iInLen);//����DOM���ṹ.

    if (iRet < 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR,  "BoardSearchProc() exit---: XML Build Tree Error \r\nmsg=%s \r\n", pcInBuff);
        return iRet;
    }

    NodeName_Vector.clear();
    DOMDocument* pDOMDocument = inPacket.GetDOMDocument();
    DOMElement* pDOMElement = pDOMDocument->get_root();
    pDOMElement->ClearNodeNumber();

    if (pDOMElement->GetNodeName(NodeName_Vector) <= 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "BoardSearchProc() exit---: Get Node Name Error \r\n");
        return -1;
    }

    /* ������xml����Ϣ���� */
    xml_type = get_xml_type_from_xml_body(NodeName_Vector, inPacket);
    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "BoardSearchProc() get_xml_type_from_xml_body:xml_type=%d \r\n", xml_type);
    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "���յ���������������Ϣ, ��������=%d", xml_type);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Received board search command message, command type=%d", xml_type);

    inPacket.GetElementValue((char*)"serial_number", strSN);

    switch (xml_type)
    {
        case XML_SEARCH_BOARD : /* �������� */
            i = BoardSearchCmdProc(strSN, outBuff);
            break;

        case XML_SET_IPADDR :   /* ����IP ��ַ */
            i = BoardSearchSetIPAddrProc(inPacket, outBuff);
            break;

        case XML_NOTIFY_REBOOTCMS : /* ����CMS */
            i = BoardSearchRebootCMSProc();
            break;

        case XML_NOTIFY_REBOOTDB : /* �������ݿ� */
            i = BoardSearchRebootDBProc();
            break;

        case XML_NOTIFY_REBOOTKEYBOARD : /* ֪ͨ�������̷������ */
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "BoardSearchProc() : restart KeyBoardCtrl\r\n");
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "��������������յ��������̷������KeyBoardCtrl����");
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Board search command restart KeyBoardCtrl process");
            i = BoardSearchRebootKeyBoardProc();
            break;

        case XML_NOTIFY_REBOOTMEDIASERVICE : /* ֪ͨ�����Դ�ý�����س��� */
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "BoardSearchProc() : restart EV9000MediaService\r\n");
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "��������������յ������Դ�ý�����ؽ���EV9000MediaService����");
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Board search command restart EV9000MediaService process");
            i = BoardSearchRebootMediaServiceProc();
            break;

        case XML_NOTIFY_LOCATION : /* ���嶨λ */
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "BoardSearchProc() BoardLocation \r\n");
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "������������嶨λ����");
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Board search command board locate process");
            //BoardLocation();
            break;

        case XML_NOTIFY_REBOOT : /* �������� */
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "BoardSearchProc() BoardReboot \r\n");
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�������������������崦��");
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Board search command restart board process");
            BoardReboot();
            break;

        case XML_NOTIFY_SHUTDOWN : /* �رյ��� */
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "BoardSearchProc() TurnOnffBoard \r\n");
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "������������رյ��崦��");
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Board search command close board process");
            //TurnOnffBoard();
            break;

        default:
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "������������, δ�������Ϣ��������=%d", xml_type);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Board search command, undefined message command type=%d", xml_type);
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "BoardSearchProc() exit---: Not Support Message Type:%d \r\n", xml_type);
            return -1;
    }

    //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "BoardSearchProc() OutBuffer=%s \r\n", outBuff.c_str());

    return i;
}

/*****************************************************************************
 �� �� ��  : AddNetWorkInfoToBoardSearchXML
 ��������  : ���������Ϣ������������XML��Ϣ
 �������  : CPacket* pOutPacket
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��7��8�� ���ڶ�
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int AddNetWorkInfoToBoardSearchXML(CPacket * pOutPacket, DOMElement * ItemAccNode)
{
    int fd = 0;
    int interfaceNum = 0;
    struct ifreq buf[16];
    struct ifconf ifc;
    struct ifreq ifrcopy;
    char mac[16] = {0};
    char ip[32] = {0};
    char broadAddr[32] = {0};
    char subnetMask[32] = {0};
    char gateway[32] = {0};

    DOMElement* InterfaceItemAccNode = NULL;
    DOMElement* InterfaceItemInfoAccNode = NULL;

    if (NULL == pOutPacket || NULL == ItemAccNode)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "AddNetWorkInfoToBoardSearchXML() exit---: Param Error \r\n");
        return -1;
    }

    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "AddNetWorkInfoToBoardSearchXML() exit---: Socket Error \r\n");
        close(fd);
        return -1;
    }

    ifc.ifc_len = sizeof(buf);
    ifc.ifc_buf = (caddr_t)buf;

    if (!ioctl(fd, SIOCGIFCONF, (char*)&ifc))
    {
        interfaceNum = ifc.ifc_len / sizeof(struct ifreq);
        //printf("interface num = %d\n", interfaceNum);

        /* ѭ�����ÿ����������Ϣ */
        while (interfaceNum-- > 0)
        {
            if (0 == sstrcmp(buf[interfaceNum].ifr_name, (char*)"lo"))
            {
                continue;
            }

            pOutPacket->SetCurrentElement(ItemAccNode);

            /* ��дXML����*/
            InterfaceItemAccNode = pOutPacket->CreateElement((char*)"ethernet");
            pOutPacket->SetCurrentElement(InterfaceItemAccNode);

            //printf("\ndevice name: %s\n", buf[interfaceNum].ifr_name);

            /* ��������*/
            InterfaceItemInfoAccNode = pOutPacket->CreateElement((char*)"name");
            pOutPacket->SetElementValue(InterfaceItemInfoAccNode, buf[interfaceNum].ifr_name);

            //ignore the interface that not up or not runing
            ifrcopy = buf[interfaceNum];

            if (ioctl(fd, SIOCGIFFLAGS, &ifrcopy))
            {
                printf("ioctl: %s [%s:%d]\n", strerror(errno), __FILE__, __LINE__);
                close(fd);
                return -1;
            }

            //get the mac of this interface
            if (!ioctl(fd, SIOCGIFHWADDR, (char*)(&buf[interfaceNum])))
            {
                memset(mac, 0, sizeof(mac));
                snprintf(mac, sizeof(mac), "%02x%02x%02x%02x%02x%02x",
                         (unsigned char)buf[interfaceNum].ifr_hwaddr.sa_data[0],
                         (unsigned char)buf[interfaceNum].ifr_hwaddr.sa_data[1],
                         (unsigned char)buf[interfaceNum].ifr_hwaddr.sa_data[2],
                         (unsigned char)buf[interfaceNum].ifr_hwaddr.sa_data[3],
                         (unsigned char)buf[interfaceNum].ifr_hwaddr.sa_data[4],
                         (unsigned char)buf[interfaceNum].ifr_hwaddr.sa_data[5]);
                //printf("device mac: %s\n", mac);

                /* MAC ��ַ*/
                InterfaceItemInfoAccNode = pOutPacket->CreateElement((char*)"mac");
                pOutPacket->SetElementValue(InterfaceItemInfoAccNode, mac);
            }
            else
            {
                printf("ioctl: %s [%s:%d]\n", strerror(errno), __FILE__, __LINE__);
                close(fd);
                return -1;
            }

            //get the IP of this interface
            if (!ioctl(fd, SIOCGIFADDR, (char*)&buf[interfaceNum]))
            {
                snprintf(ip, sizeof(ip), "%s",
                         (char*)inet_ntoa(((struct sockaddr_in*) & (buf[interfaceNum].ifr_addr))->sin_addr));
                //printf("device ip: %s\n", ip);

                /* IP  ��ַ*/
                InterfaceItemInfoAccNode = pOutPacket->CreateElement((char*)"addr");
                pOutPacket->SetElementValue(InterfaceItemInfoAccNode, ip);
            }
            else
            {
                printf("ioctl: %s [%s:%d]\n", strerror(errno), __FILE__, __LINE__);
                close(fd);
                return -1;
            }

            //get the broad address of this interface
            if (!ioctl(fd, SIOCGIFBRDADDR, &buf[interfaceNum]))
            {
                snprintf(broadAddr, sizeof(broadAddr), "%s",
                         (char*)inet_ntoa(((struct sockaddr_in*) & (buf[interfaceNum].ifr_broadaddr))->sin_addr));
                //printf("device broadAddr: %s\n", broadAddr);

                /* broadAddr  ��ַ*/
                //InterfaceItemInfoAccNode = pOutPacket->CreateElement((char*)"addr");
                //pOutPacket->SetElementValue(InterfaceItemInfoAccNode, broadAddr);
            }
            else
            {
                printf("ioctl: %s [%s:%d]\n", strerror(errno), __FILE__, __LINE__);
                close(fd);
                return -1;
            }

            //get the subnet mask of this interface
            if (!ioctl(fd, SIOCGIFNETMASK, &buf[interfaceNum]))
            {
                snprintf(subnetMask, sizeof(subnetMask), "%s",
                         (char*)inet_ntoa(((struct sockaddr_in*) & (buf[interfaceNum].ifr_netmask))->sin_addr));
                //printf("device subnetMask: %s\n", subnetMask);

                /* mask  ��ַ*/
                InterfaceItemInfoAccNode = pOutPacket->CreateElement((char*)"mask");
                pOutPacket->SetElementValue(InterfaceItemInfoAccNode, subnetMask);
            }
            else
            {
                printf("ioctl: %s [%s:%d]\n", strerror(errno), __FILE__, __LINE__);
                close(fd);
                return -1;
            }

            /* ���ص�ַ */
            memset(gateway, 0, 32);
            get_eth_gateway(buf[interfaceNum].ifr_name, gateway);
            InterfaceItemInfoAccNode = pOutPacket->CreateElement((char*)"gateway");
            pOutPacket->SetElementValue(InterfaceItemInfoAccNode, gateway);

            /* DNS 1 */
            InterfaceItemInfoAccNode = pOutPacket->CreateElement((char*)"dns1");
            pOutPacket->SetElementValue(InterfaceItemInfoAccNode, (char*)"");
        }
    }
    else
    {
        printf("ioctl: %s [%s:%d]\n", strerror(errno), __FILE__, __LINE__);
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
}

/*****************************************************************************
 �� �� ��  : AddDeviceInfoToBoardSearchXML
 ��������  : ����豸��Ϣ������������ӦXML��Ϣ��
 �������  : CPacket* pOutPacket
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��7��8�� ���ڶ�
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int AddDeviceInfoToBoardSearchXML(CPacket * pOutPacket)
{
    int i = 0;
    DOMElement* ItemAccNode = NULL;
    DOMElement* ItemInfoAccNode = NULL;

    char strSlotID[32] = {0};
    char strCMSVer[256] = {0};

    if (NULL == pOutPacket)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "AddDeviceInfoToBoardSearchXML() exit---: Param Error \r\n");
        return -1;
    }

    /* ��дXML����*/
    ItemAccNode = pOutPacket->CreateElement((char*)"deviceinfo");
    pOutPacket->SetCurrentElement(ItemAccNode);

    ItemInfoAccNode = pOutPacket->CreateElement((char*)"type");
    pOutPacket->SetElementValue(ItemInfoAccNode, (char*)"CMS");

    ItemInfoAccNode = pOutPacket->CreateElement((char*)"status");
    pOutPacket->SetElementValue(ItemInfoAccNode, (char*)"OnLine");

    ItemInfoAccNode = pOutPacket->CreateElement((char*)"slotid");
    pOutPacket->SetElementValue(ItemInfoAccNode, (char*)"0");

    /* ���Ӱ汾��Ϣ */
    ItemInfoAccNode = pOutPacket->CreateElement((char*)"appVersion");
    //GetAppVerInfoForRoute(strCMSVer);
    pOutPacket->SetElementValue(ItemInfoAccNode, strCMSVer);

    /* ���������Ϣ */
    i = AddNetWorkInfoToBoardSearchXML(pOutPacket, ItemAccNode);

    return i;
}

/*****************************************************************************
 �� �� ��  : BoardSearchSetIPAddrProc
 ��������  : �����������IP��ַ���ô���
 �������  : CPacket& inPacket
             std::string& outBuff
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��7��8�� ���ڶ�
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int BoardSearchSetIPAddrProc(CPacket & inPacket, std::string & outBuff)
{
    int i = 0;
    int iIsChange = 0;
    CPacket* pOutPacket = NULL;
    DOMElement* ItemAccNode = NULL;

    char strSN[128] = {0};
    char strEthName[MAX_IP_LEN] = {0};
    char strIP[MAX_IP_LEN] = {0};
    char strMask[MAX_IP_LEN] = {0};
    char strGateWay[MAX_IP_LEN] = {0};
    char strMac[MAX_IP_LEN] = {0};

    inPacket.GetElementValue((char*)"serial_number", strSN);

    /* ��ȡ���е�������Ϣ���� */
    ItemAccNode = inPacket.SearchElement((char*)"ethernet");

    if (!ItemAccNode)
    {
        i = GeneratingErrorResponseForSystemRunState((char*)"EV9000SetBoardIP", strSN, 20001, (char*)"Get XML ethnet Item Node Error", outBuff);
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR,  "BoardSearchSetIPAddrProc() exit---: Get XML ethnet Item Node Error \r\n");
        return -1;
    }

    inPacket.SetCurrentElement(ItemAccNode);

    while (ItemAccNode)
    {
        memset(strEthName, 0, MAX_IP_LEN);
#if 0
        inPacket.GetElementValue((char*)"name", strEthName);
#endif

        memset(strIP, 0, MAX_IP_LEN);
        inPacket.GetElementValue((char*)"addr", strIP);

        memset(strMask, 0, MAX_IP_LEN);
        inPacket.GetElementValue((char*)"mask", strMask);

        memset(strGateWay, 0, MAX_IP_LEN);
        inPacket.GetElementValue((char*)"gateway", strGateWay);

        memset(strMac, 0, MAX_IP_LEN);
        inPacket.GetElementValue((char*)"mac", strMac);

#if 0

        if (strEthName[0] == '\0')
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "BoardSearchSetIPAddrProc() Continue---: Get XML Ethernet Name Error \r\n");
            ItemAccNode = inPacket.SearchNextElement(true);
            continue;
        }

#endif

        if (strMac[0] == '\0')
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "BoardSearchSetIPAddrProc() Continue---: Get XML Ethernet Mac Error \r\n");
            ItemAccNode = inPacket.SearchNextElement(true);
            continue;
        }

        i = GetEthNameByMac(strMac, strEthName);

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "BoardSearchSetIPAddrProc() Continue---: Get XML Ethernet Name Error:strMac=%s \r\n", strMac);
            ItemAccNode = inPacket.SearchNextElement(true);
            continue;
        }

#if 0

        if (strIP[0] == '\0')
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "BoardSearchSetIPAddrProc() Continue---: Get XML XML IP Error \r\n");
            ItemAccNode = inPacket.SearchNextElement(true);
            continue;
        }

        if (strMask[0] == '\0')
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "BoardSearchSetIPAddrProc() Continue---: Get XML XML Mask Error \r\n");
            ItemAccNode = inPacket.SearchNextElement(true);
            continue;
        }

        if (strGateWay[0] == '\0')
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "BoardSearchSetIPAddrProc() Continue---: Get XML XML GateWay Error \r\n");
            ItemAccNode = inPacket.SearchNextElement(true);
            continue;
        }

#endif

        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "BoardSearchSetIPAddrProc() Continue---: Not Support Set IP Addr:EthName=%s \r\n", strEthName);
        ItemAccNode = inPacket.SearchNextElement(true);
        continue;
    }

    /* ����XML��ͷ�� */
    i = CreateXMLHeadResponseForSystemRunState(&pOutPacket, (char*)"EV9000SetBoardIP", strSN);

    if (i != 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "BoardSearchSetIPAddrProc() exit---: Create XML Packet Error \r\n");
        return -1;
    }

    outBuff = pOutPacket->GetXml(NULL);

    delete pOutPacket;

    if (1 == iIsChange)
    {
        /* �������� */
        BoardReboot();
    }

    return i;
}

/*****************************************************************************
 �� �� ��  : BoardSearchCmdProc
 ��������  : �������������
 �������  : char* pcSerialNumber
             std::string& outBuff
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��7��8�� ���ڶ�
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int BoardSearchCmdProc(char * pcSerialNumber, std::string & outBuff)
{
    int i = 0;

    CPacket* pOutPacket = NULL;
    DOMDocument* pDOMDocument = NULL;
    DOMElement* pDOMElement = NULL;

    /* ����XML��ͷ�� */
    i |= CreateXMLHeadResponseForSystemRunState(&pOutPacket, (char*)"EV9000SearchBoard", pcSerialNumber);

    if (i != 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "BoardSearchCmdProc() exit---: Create XML Packet Error \r\n");
        return -1;
    }

    pDOMDocument = pOutPacket->GetDOMDocument();
    pDOMElement = pDOMDocument->get_root();

    /* ����豸��Ϣ */
    pOutPacket->SetCurrentElement(pDOMElement);
    i |= AddDeviceInfoToBoardSearchXML(pOutPacket);

    outBuff = pOutPacket->GetXml(NULL, "UTF-8");

    delete pOutPacket;

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "������������Ӧ����CMS ID=%s, CMS IP=%s", local_cms_id_get(), local_ip_get(default_eth_name_get()));
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Board search command response process��CMS ID=%s, CMS IP=%s", local_cms_id_get(), local_ip_get(default_eth_name_get()));

    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "BoardSearchCmdProc() exit---: outBuff=%s \r\n", outBuff.c_str());

    return i;
}

/*****************************************************************************
 �� �� ��  : BoardSearchRebootCMSProc
 ��������  : ���������ӿ�����CMS���̴���
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��7��8�� ���ڶ�
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int BoardSearchRebootCMSProc()
{
    int i = 0;
    int iRet = 0;
    int pid_t[128] = {0};
    int pid = 0;

    /* ��ȡ���̵�pid */
    iRet = find_pid_by_name((char*)"cms", pid_t);

    if (!iRet)
    {
        for (i = 0; pid_t[i] != 0; i++)
        {
            pid = pid_t[i];
            break;
        }
    }
    else
    {
        return iRet;
    }

    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "BoardSearchRebootCMSProc() : cms, pid=%d \r\n", pid);
    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "����������������CMS����pid=%d", pid);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Board search command restart CMS process��pid=%d", pid);
    osip_usleep(10000000);

    //sprintf(strCmd, "kill %d", pid);
    //system(strCmd);
    system((char*)"killall cms; killall -9 cms");

    return 0;
}

/*****************************************************************************
 �� �� ��  : BoardSearchRebootDBProc
 ��������  : ���������ӿ��������ݿ���̴���
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��7��8�� ���ڶ�
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int BoardSearchRebootDBProc()
{
    return 0;
#if 0
    int i = 0;
    int iRet = 0;
    int pid_t[128] = {0};
    int pid = 0;
    char strCmd[64] = {0};

    /* ��ȡ���̵�pid */
    iRet = find_pid_by_name("start_mysql", pid_t);

    if (!iRet)
    {
        for (i = 0; pid_t[i] != 0; i++)
        {
            pid = pid_t[i];
            break;
        }
    }
    else
    {
        return iRet;
    }

    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "BoardSearchRebootCMSProc() : start_mysql, pid=%d \r\n", pid);

    sprintf(strCmd, "kill %d", pid);
    system(strCmd);
#endif

    //system("/usr/libexec/mysqld --basedir=/usr --datadir=/data/db --user=root --log-error=/data/db/(none).err --pid-file=/data/db/(none).pid");
    //system("/bin/sh /usr/bin/mysqld_safe --user=root --datadir=/data/db");
    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "BoardSearchRebootCMSProc() : restart mysql\r\n");
    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "����������������mysql����");
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Board search command restart mysql process");

    system("/bin/sh /etc/stop_mysql &");
    sleep(1);
    system("/bin/sh /etc/start_mysql &");

    return 0;
}

/*****************************************************************************
 �� �� ��  : BoardSearchRebootKeyBoardProc
 ��������  : ����KeyBoardCtrl����
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��11��25��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int BoardSearchRebootKeyBoardProc()
{
    system((char*)"killall KeyBoardCtrl; killall -9 KeyBoardCtrl");

    return 0;
}

/*****************************************************************************
 �� �� ��  : BoardSearchRebootMediaServiceProc
 ��������  : ����EV9000MediaService����
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��11��25��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int BoardSearchRebootMediaServiceProc()
{
    system((char*)"killall EV9000MediaService; killall -9 EV9000MediaService");

    return 0;
}

/*****************************************************************************
 �� �� ��  : BoardSearchRestoreFactorySettingsProc
 ��������  : �ָ���������
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��12��15��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int BoardSearchRestoreFactorySettingsProc()
{
    /* ɾ�����ݿ� */
    system("/app/delete_DB.sh");

    /* ɾ�������ļ� */
    system("rm -rf /config/*.cfg");
    system("rm -rf /config/*.conf");
    system("rm -rf /config/config");

    /* ɾ����־�ļ� */
    system("rm -rf /data/log/*");

    /* ɾ���ֻ����ݿ� */
    system("/app/delete_DB_mobile.sh");

    /* д��Ĭ�ϵ�web�����ļ�����ֹweb������ʱ��CMS��û��������NPE�汾��Ϊ��Ѷ�汾 */
    write_default_web_config_file();

    /* �Զ����� */
    BoardReboot();

    return 0;
}

/*****************************************************************************
 �� �� ��  : SendSystemDiskErrorToAllClientUser
 ��������  : ����ϵͳ���̹��ϸ������û�
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
int SendSystemDiskErrorToAllClientUser()
{
    int i = 0;
    CPacket outPacket;
    DOMElement* AccNode = NULL;
    time_t utc_time;
    struct tm local_time = { 0 };
    char str_date[12] = {0};
    char str_time[12] = {0};
    char strTime[32] = {0};

    /* ���ͱ������ݸ��ͻ����û� */
    outPacket.SetRootTag("Notify");

    AccNode = outPacket.CreateElement((char*)"CmdType");
    outPacket.SetElementValue(AccNode, (char*)"Alarm");

    AccNode = outPacket.CreateElement((char*)"SN");
    outPacket.SetElementValue(AccNode, (char*)"99999");

    AccNode = outPacket.CreateElement((char*)"DeviceID");
    outPacket.SetElementValue(AccNode, local_cms_id_get());

    AccNode = outPacket.CreateElement((char*)"AlarmPriority");
    outPacket.SetElementValue(AccNode, (char*)"4");

    AccNode = outPacket.CreateElement((char*)"AlarmMethod");
    outPacket.SetElementValue(AccNode, (char*)"6");

    AccNode = outPacket.CreateElement((char*)"AlarmTime");
    utc_time = time(NULL);
    localtime_r(&utc_time, &local_time);
    strftime(str_date, sizeof(str_date), "%Y-%m-%d", &local_time);
    strftime(str_time, sizeof(str_time), "%H:%M:%S", &local_time);
    snprintf(strTime, 32, "%sT%s", str_date, str_time);
    outPacket.SetElementValue(AccNode, strTime);

    AccNode = outPacket.CreateElement((char*)"AlarmDescription");
    outPacket.SetElementValue(AccNode, (char*)"ϵͳӲ�̵ڶ�����������,���ܶ�д, ��Ӱ��ϵͳ��־��¼���¼�Ĳ���, ���Web���Ը�ʽ���������лָ�, ������ϵ���Ҽ���֧��");

    i = SendMessageToOnlineUser((char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length(), 0);

    if (i > 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "����ϵͳ���̹�����Ϣ���û��ɹ�");
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "SendSystemDiskErrorToAllClientUser() SendMessageToOnlineUser OK");
    }
    else if (i < 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "����ϵͳ���̹�����Ϣ���û�ʧ��");
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "SendSystemDiskErrorToAllClientUser() SendMessageToOnlineUser Error");
    }

    return i;
}
