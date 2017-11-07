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
 * 外部变量说明                                 *
 *----------------------------------------------*/
extern gbl_conf_t* pGblconf;              /* 全局配置信息 */
extern User_Info_MAP g_UserInfoMap;                       /* 用户信息队列 */
extern GBDevice_Info_MAP g_GBDeviceInfoMap;              /* 标准物理设备信息队列 */
extern GBLogicDevice_Info_MAP g_GBLogicDeviceInfoMap;    /* 标准逻辑设备信息队列 */
extern TSU_Resource_Info_MAP g_TSUResourceInfoMap;       /* TSU 资源信息队列 */
extern CR_Data_MAP g_CallRecordMap;                       /* 呼叫链接数据队列 */
extern MS_SWITCH_ATTR  gt_MSSwitchAttr;
extern BOARD_NET_ATTR  g_BoardNetConfig;
extern int cms_run_status;  /* 0:没有运行,1:正常运行 */
extern DBOper g_DBOper;
extern int g_GetChannelFlag;

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
/*****************************************************************************
 函 数 名  : get_cpu_info
 功能描述  : 获取CPU的参数信息
 输入参数  : cpu_info* cpuinfo
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年4月16日 星期三
    作    者   : 杨海锋
    修改内容   : 新生成函数

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
 函 数 名  : get_cpu_status
 功能描述  : 获取CPU的状态信息
 输入参数  : cpu_status* cpu_stat
                             int process_num
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年4月16日 星期三
    作    者   : 杨海锋
    修改内容   : 新生成函数

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
 函 数 名  : get_mem_info
 功能描述  : 获取系统内存信息
 输入参数  : mem_info* minfo
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年4月16日 星期三
    作    者   : 杨海锋
    修改内容   : 新生成函数

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

        if (strlen(total) > 0 && strlen(free) > 0) /* 已经全部找到，跳出 */
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
 函 数 名  : find_pid_by_name
 功能描述  : 根据进程名称获取进程ID
 输入参数  : char* ProcName
                            int* foundpid
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年4月15日 星期二
    作    者   : 杨海锋
    修改内容   : 新生成函数

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
 函 数 名  : get_progress_cpu_usage
 功能描述  :  获取当前进程的cpu使用率
 输入参数  : char* ProcName
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年4月16日 星期三
    作    者   : 杨海锋
    修改内容   : 新生成函数

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

    /* 获取进程的pid */
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
 函 数 名  : get_progress_memory_usage
 功能描述  : 获取当前进程内存和虚拟内存使用量
 输入参数  : char* ProcName
             process_mem_info* proc_mem
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年4月16日 星期三
    作    者   : 杨海锋
    修改内容   : 新生成函数

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

    /* 获取进程的pid */
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
        if (0 == strncmp("VmRSS", temp, 5)) //物理内存
        {
            strcpy(mem, temp);
        }
        else if (0 == strncmp("VmData", temp, 6)) //虚拟内存
        {
            strcpy(vmem, temp);
        }

        if (strlen(mem) > 0 && strlen(vmem) > 0) /* 已经全部找到，跳出 */
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
 函 数 名  : get_phy_mem
 功能描述  : 获取进程的物理内存
 输入参数  : const pid_t p
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年4月16日 星期三
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int get_phy_mem(const pid_t p)
{
    int i = 0;
    FILE* fd = NULL;         //定义文件指针fd
    char line_buff[256] = {0};  //读取行的缓冲区
    char name[32] = {0};//存放项目名称
    char file[64] = {0};//文件名
    int vmrss = 0;//存放内存峰值大小

    sprintf(file, "/proc/%d/status", p); //文件中第11行包含着

    //fprintf(stderr, "current pid:%d\n", p);
    fd = fopen(file, "r");  //以R读的方式打开文件再赋给指针fd

    if (NULL == fd)
    {
        printf("Open %s failed!\n", file);
        return -1;
    }

    //获取vmrss:实际物理内存占用

    for (i = 0; i < VMRSS_LINE - 1; i++)
    {
        fgets(line_buff, sizeof(line_buff), fd);
    }//读到第15行

    fgets(line_buff, sizeof(line_buff), fd); //读取VmRSS这一行的数据,VmRSS在第15行
    sscanf(line_buff, "%s %d", name, &vmrss);
    //fprintf(stderr, "====%s：%d====\n", name, vmrss);
    fclose(fd);     //关闭文件fd
    return vmrss;
}

/*****************************************************************************
 函 数 名  : get_total_mem
 功能描述  : 获取系统总的物理内存
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年4月16日 星期三
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int get_total_mem()
{
    FILE* fd = NULL;         //定义文件指针fd
    char line_buff[256] = {0};  //读取行的缓冲区
    char name[32] = {0};//存放项目名称
    int memtotal = 0;//存放内存峰值大小
    const char* file = "/proc/meminfo";//文件名

    fd = fopen(file, "r");  //以R读的方式打开文件再赋给指针fd

    if (NULL == fd)
    {
        printf("Open %s failed!\n", file);
        return -1;
    }

    //获取memtotal:总内存占用大小
    fgets(line_buff, sizeof(line_buff), fd); //读取memtotal这一行的数据,memtotal在第1行
    sscanf(line_buff, "%s %d", name, &memtotal);
    //fprintf(stderr, "====%s：%d====\n", name, memtotal);
    fclose(fd);     //关闭文件fd
    return memtotal;
}

/*****************************************************************************
 函 数 名  : get_pmem
 功能描述  : 获取进程的内存占用百分比
 输入参数  : pid_t p
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年4月16日 星期三
    作    者   : 杨海锋
    修改内容   : 新生成函数

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
 函 数 名  : get_cpu_process_occupy
 功能描述  : 获取进程的CPU信息
 输入参数  : const pid_t p
 输出参数  : 无
 返 回 值  : unsigned
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年4月16日 星期三
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
unsigned int get_cpu_process_occupy(const pid_t p)
{
    char file[64] = {0};//文件名
    process_cpu_occupy_t t;
    FILE* fd = NULL;         //定义文件指针fd
    char line_buff[1024] = {0};  //读取行的缓冲区
    const char* q = NULL;

    sprintf(file, "/proc/%d/stat", p); //文件中第11行包含着

    //fprintf(stderr, "current pid:%d\n", p);
    fd = fopen(file, "r");  //以R读的方式打开文件再赋给指针fd

    if (NULL == fd)
    {
        printf("Open %s failed!\n", file);
        return 0;
    }

    fgets(line_buff, sizeof(line_buff), fd);  //从fd文件中读取长度为buff的字符串再存到起始地址为buff这个空间里

    sscanf(line_buff, "%u", &t.pid); //取得第一项
    q = get_items((const char*)line_buff, PROCESS_ITEM); //取得从第14项开始的起始指针
    sscanf(q, "%u %u %u %u", &t.utime, &t.stime, &t.cutime, &t.cstime); //格式化第14,15,16,17项

    //fprintf(stderr, "====pid%u:%u %u %u %u====\n", t.pid, t.utime, t.stime, t.cutime, t.cstime);
    fclose(fd);     //关闭文件fd
    return (t.utime + t.stime + t.cutime + t.cstime);
}

/*****************************************************************************
 函 数 名  : get_cpu_total_occupy
 功能描述  : 获取总的CPU信息
 输入参数  : 无
 输出参数  : 无
 返 回 值  : unsigned
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年4月16日 星期三
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
unsigned int get_cpu_total_occupy()
{
    FILE* fd;         //定义文件指针fd
    char buff[1024] = {0};  //定义局部变量buff数组为char类型大小为1024
    total_cpu_occupy_t t;
    char name[16] = {0};//暂时用来存放字符串

    fd = fopen("/proc/stat", "r");  //以R读的方式打开stat文件再赋给指针fd

    if (NULL == fd)
    {
        printf("Open /proc/stat failed!\n");
        return 0;
    }

    fgets(buff, sizeof(buff), fd);  //从fd文件中读取长度为buff的字符串再存到起始地址为buff这个空间里
    /*下面是将buff的字符串根据参数format后转换为数据的结果存入相应的结构体参数 */
    sscanf(buff, "%s %u %u %u %u", name, &t.user, &t.nice, &t.system, &t.idle);

    //fprintf(stderr, "====%s:%u %u %u %u====\n", name, t.user, t.nice, t.system, t.idle);
    fclose(fd);     //关闭文件fd
    return (t.user + t.nice + t.system + t.idle);
}

/*****************************************************************************
 函 数 名  : get_pcpu
 功能描述  : 获取进程的cpu占用率
 输入参数  : pid_t p
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年4月16日 星期三
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
float get_pcpu(pid_t p)
{
    float pcpu = 0.0;
    unsigned int totalcputime1 = 0, totalcputime2 = 0;
    unsigned int procputime1 = 0, procputime2 = 0;
    totalcputime1 = get_cpu_total_occupy();
    procputime1 = get_cpu_process_occupy(p);
    usleep(500000);//延迟500毫秒
    totalcputime2 = get_cpu_total_occupy();
    procputime2 = get_cpu_process_occupy(p);
    pcpu = 100.0 * (procputime2 - procputime1) / (totalcputime2 - totalcputime1);
    //fprintf(stderr, "====pcpu:%.6f\n====", pcpu);
    return pcpu;
}

const char* get_items(const char* buffer, int ie)
{
    char* p = NULL;//指向缓冲区
    int len = 0;
    int count = 0;//统计空格数

    if (NULL == buffer)
    {
        return NULL;
    }

    p = (char*)buffer;//指向缓冲区
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
 函 数 名  : getLocalNetInfo
 功能描述  : 获取本地网络信息
 输入参数  : void
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年4月18日 星期五
    作    者   : 杨海锋
    修改内容   : 新生成函数

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

            /* 网关地址 */
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
 函 数 名  : get_eth_gateway
 功能描述  : 获取网卡的网关
 输入参数  : char* eth_name
                            char* gateway_addr
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年4月17日 星期四
    作    者   : 杨海锋
    修改内容   : 新生成函数

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
 函 数 名  : GetSystemInfoProc
 功能描述  : 获取系统信息处理函数
 输入参数  : char* pcSerialNumber
                            std::string& outBuff
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年4月17日 星期四
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int GetSystemInfoProc(char* pcSerialNumber, std::string& outBuff)
{
    int i = 0;

    CPacket* pOutPacket = NULL;
    DOMDocument* pDOMDocument = NULL;
    DOMElement* pDOMElement = NULL;

    /* 创建XML的头部 */
    i |= CreateXMLHeadResponseForSystemRunState(&pOutPacket, (char*)"GetSystemInfo", pcSerialNumber);

    if (i != 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "GetSystemInfoProc() exit---: Create XML Packet Error \r\n");
        return -1;
    }

    pDOMDocument = pOutPacket->GetDOMDocument();
    pDOMElement = pDOMDocument->get_root();

    /* 添加时间信息 */
    pOutPacket->SetCurrentElement(pDOMElement);
    i |= AddSystemDateTimeInfoToSysInfoXML(pOutPacket);

    /* 添加CPU 信息 */
    pOutPacket->SetCurrentElement(pDOMElement);
    i |= AddCPUInfoToSysInfoXML(pOutPacket);

    /* 添加内存信息 */
    pOutPacket->SetCurrentElement(pDOMElement);
    i |= AddMemoryInfoToSysInfoXML(pOutPacket);

    /* 添加风扇信息 */
    pOutPacket->SetCurrentElement(pDOMElement);
    i |= AddFanInfoToSysInfoXML(pOutPacket);

    /* 添加网络信息 */
    pOutPacket->SetCurrentElement(pDOMElement);
    i |= AddNetWorkInfoToSysInfoXML(pOutPacket);

    outBuff = pOutPacket->GetXml(NULL);

    delete pOutPacket;

    return i;
}

/*****************************************************************************
 函 数 名  : GetUserInfoProc
 功能描述  : 获取用户信息处理函数
 输入参数  : char* pcSerialNumber
                            std::string& outBuff
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年4月18日 星期五
    作    者   : 杨海锋
    修改内容   : 新生成函数

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

    /* 创建XML的头部 */
    i |= CreateXMLHeadResponseForSystemRunState(&pOutPacket, (char*)"GetCMS_UserInfo", pcSerialNumber);

    if (i != 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "GetUserInfoProc() exit---: Create XML Packet Error \r\n");
        return -1;
    }

    pDOMDocument = pOutPacket->GetDOMDocument();
    pDOMElement = pDOMDocument->get_root();

    pOutPacket->SetCurrentElement(pDOMElement);

    /* 填写XML数据*/
    ItemAccNode = pOutPacket->CreateElement((char*)"userinfo");
    pOutPacket->SetCurrentElement(ItemAccNode);

    USER_INFO_SMUTEX_LOCK();

    user_count = g_UserInfoMap.size();

    /* 用户总个数 */
    ItemInfoAccNode = pOutPacket->CreateElement((char*)"count");
    sprintf(strUserCount, "%d", user_count);
    pOutPacket->SetElementValue(ItemInfoAccNode, strUserCount);

    /* 循环添加用户信息 */
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

        /* 用户ID */
        UserItemInfoAccNode = pOutPacket->CreateElement((char*)"userid");
        pOutPacket->SetElementValue(UserItemInfoAccNode, pUserInfo->user_id);

        /* IP 地址*/
        UserItemInfoAccNode = pOutPacket->CreateElement((char*)"ipaddress");
        pOutPacket->SetElementValue(UserItemInfoAccNode, pUserInfo->login_ip);

        /* 端口 */
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
 函 数 名  : GetDeviceInfoProc
 功能描述  : 获取设备信息处理函数
 输入参数  : char* pcSerialNumber
                            std::string& outBuff
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年4月18日 星期五
    作    者   : 杨海锋
    修改内容   : 新生成函数

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

    /* 创建XML的头部 */
    i |= CreateXMLHeadResponseForSystemRunState(&pOutPacket, (char*)"GetCMS_DeviceInfo", pcSerialNumber);

    if (i != 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "GetDeviceInfoProc() exit---: Create XML Packet Error \r\n");
        return -1;
    }

    pDOMDocument = pOutPacket->GetDOMDocument();
    pDOMElement = pDOMDocument->get_root();

    pOutPacket->SetCurrentElement(pDOMElement);

    /* 填写XML数据*/
    ItemAccNode = pOutPacket->CreateElement((char*)"deviceinfo");
    pOutPacket->SetCurrentElement(ItemAccNode);

    device_count = g_GBDeviceInfoMap.size();

    /* 物理设备总个数 */
    ItemInfoAccNode = pOutPacket->CreateElement((char*)"count");
    sprintf(strDeviceCount, "%d", device_count);
    pOutPacket->SetElementValue(ItemInfoAccNode, strDeviceCount);

    /* 循环添加物理设备信息 */
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

        /* 物理设备ID */
        DeviceItemInfoAccNode = pOutPacket->CreateElement((char*)"deviceid");
        pOutPacket->SetElementValue(DeviceItemInfoAccNode, pGBDeviceInfo->device_id);

        /* IP 地址*/
        DeviceItemInfoAccNode = pOutPacket->CreateElement((char*)"ipaddress");
        pOutPacket->SetElementValue(DeviceItemInfoAccNode, pGBDeviceInfo->login_ip);

        /* 端口 */
        DeviceItemInfoAccNode = pOutPacket->CreateElement((char*)"port");
        sprintf(strLoginPort, "%d", pGBDeviceInfo->login_port);
        pOutPacket->SetElementValue(DeviceItemInfoAccNode, strLoginPort);

        /* 状态*/
        DeviceItemInfoAccNode = pOutPacket->CreateElement((char*)"status");

        if (pGBDeviceInfo->reg_status == 0)
        {
            pOutPacket->SetElementValue(DeviceItemInfoAccNode, (char*)"OffLine");
        }
        else
        {
            pOutPacket->SetElementValue(DeviceItemInfoAccNode, (char*)"OnLine");
        }

        /* 添加下面的逻辑设备 */
        ItemAccNode1 = pOutPacket->CreateElement((char*)"logicdevicelist");
        pOutPacket->SetCurrentElement(ItemAccNode1);

        /* 逻辑设备总个数 */
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

        /* 循环添加逻辑设备信息 */
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

            /* 逻辑设备ID */
            LogicDeviceItemInfoAccNode = pOutPacket->CreateElement((char*)"deviceid");
            pOutPacket->SetElementValue(LogicDeviceItemInfoAccNode, pGBLogicDeviceInfo->device_id);

            /* 状态*/
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
 函 数 名  : GetTSUInfoProc
 功能描述  : 获取TSU信息处理函数
 输入参数  : char* pcSerialNumber
                            std::string& outBuff
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年4月18日 星期五
    作    者   : 杨海锋
    修改内容   : 新生成函数

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

    /* 创建XML的头部 */
    i |= CreateXMLHeadResponseForSystemRunState(&pOutPacket, (char*)"GetCMS_TSUInfo", pcSerialNumber);

    if (i != 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "GetTSUInfoProc() exit---: Create XML Packet Error \r\n");
        return -1;
    }

    pDOMDocument = pOutPacket->GetDOMDocument();
    pDOMElement = pDOMDocument->get_root();

    pOutPacket->SetCurrentElement(pDOMElement);

    /* 填写XML数据*/
    ItemAccNode = pOutPacket->CreateElement((char*)"tsuinfo");
    pOutPacket->SetCurrentElement(ItemAccNode);

    tsu_count = g_TSUResourceInfoMap.size();

    /* TSU 总个数 */
    ItemInfoAccNode = pOutPacket->CreateElement((char*)"count");
    sprintf(strTSUCount, "%d", tsu_count);
    pOutPacket->SetElementValue(ItemInfoAccNode, strTSUCount);

    /* 循环添加TSU 信息 */
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

        /* 视频网IP 地址*/
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

        /* 设备网IP 地址*/
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

        /* 状态*/
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
 函 数 名  : GetTaskInfoProc
 功能描述  : 获取任务信息处理函数
 输入参数  : char* pcSerialNumber
                            std::string& outBuff
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年4月18日 星期五
    作    者   : 杨海锋
    修改内容   : 新生成函数

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

    /* 创建XML的头部 */
    i |= CreateXMLHeadResponseForSystemRunState(&pOutPacket, (char*)"GetCMS_TaskInfo", pcSerialNumber);

    if (i != 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "GetTaskInfoProc() exit---: Create XML Packet Error \r\n");
        return -1;
    }

    pDOMDocument = pOutPacket->GetDOMDocument();
    pDOMElement = pDOMDocument->get_root();

    pOutPacket->SetCurrentElement(pDOMElement);

    /* 填写XML数据*/
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

    /* TSU 总个数 */
    ItemInfoAccNode = pOutPacket->CreateElement((char*)"count");
    sprintf(strTaskCount, "%d", task_count);
    pOutPacket->SetElementValue(ItemInfoAccNode, strTaskCount);

    /* 循环添加任务信息 */
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

        /* 类型 */
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

        /* 索引编号*/
        TaskItemInfoAccNode = pOutPacket->CreateElement((char*)"index");
        sprintf(strIndex, "%d", Itr->first);
        pOutPacket->SetElementValue(TaskItemInfoAccNode, strIndex);

        /* 呼叫者ID */
        TaskItemInfoAccNode = pOutPacket->CreateElement((char*)"callerid");
        pOutPacket->SetElementValue(TaskItemInfoAccNode, pCrData->caller_id);

        /* 呼叫者IP */
        TaskItemInfoAccNode = pOutPacket->CreateElement((char*)"callerip");
        pOutPacket->SetElementValue(TaskItemInfoAccNode, pCrData->caller_ip);

        /* 呼叫者端口 */
        TaskItemInfoAccNode = pOutPacket->CreateElement((char*)"callerport");
        sprintf(strCallerSDPPort, "%d", pCrData->caller_sdp_port);
        pOutPacket->SetElementValue(TaskItemInfoAccNode, strCallerSDPPort);

        /* 被叫者ID */
        TaskItemInfoAccNode = pOutPacket->CreateElement((char*)"calleeid");
        pOutPacket->SetElementValue(TaskItemInfoAccNode, pCrData->callee_id);

        /* 被叫者IP */
        TaskItemInfoAccNode = pOutPacket->CreateElement((char*)"calleeip");
        pOutPacket->SetElementValue(TaskItemInfoAccNode, pCrData->callee_ip);

        /* 被叫者端口 */
        TaskItemInfoAccNode = pOutPacket->CreateElement((char*)"calleeport");
        sprintf(strCalleeSDPPort, "%d", pCrData->callee_sdp_port);
        pOutPacket->SetElementValue(TaskItemInfoAccNode, strCalleeSDPPort);

        /* TSU IP */
        TaskItemInfoAccNode = pOutPacket->CreateElement((char*)"tsuip");
        pOutPacket->SetElementValue(TaskItemInfoAccNode, pCrData->tsu_ip);

        /* TSU 接收端口 */
        TaskItemInfoAccNode = pOutPacket->CreateElement((char*)"tsuport");
        sprintf(strTSUPort, "%d", pCrData->tsu_recv_port);
        pOutPacket->SetElementValue(TaskItemInfoAccNode, strTSUPort);

        /* TSU 发送端口 */
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
 函 数 名  : SetIPAddrProc
 功能描述  : 从ICE接口过来的设置IP地址的处理
 输入参数  : CPacket& inPacket
                            std::string& outBuff
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年4月26日 星期六
    作    者   : 杨海锋
    修改内容   : 新生成函数

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

    /* 创建XML的头部 */
    i = CreateXMLHeadResponseForSystemRunState(&pOutPacket, (char*)"SetIPAddr", strSN);

    if (i != 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "SetIPAddrProc() exit---: Create XML Packet Error \r\n");
        return -1;
    }

    outBuff = pOutPacket->GetXml(NULL);

    delete pOutPacket;

    /* 重启单板 */
    //BoardReboot();

    return i;
}

/*****************************************************************************
 函 数 名  : GetEthNameByMac
 功能描述  : 通过Mac地址获取网卡名称
 输入参数  : char* pcMac
             char** pEthName
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年7月15日 星期二
    作    者   : 杨海锋
    修改内容   : 新生成函数

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

        /* 循环添加每个网卡的信息 */
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
 函 数 名  : GetSystemIPAddr
 功能描述  : 获取系统IP地址
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年7月24日 星期四
    作    者   : 杨海锋
    修改内容   : 新生成函数

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

        /* 循环添加每个网卡的信息 */
        while (interfaceNum-- > 0)
        {
            /* 网卡名称*/
            printf("GetSystemIPAddr() InterFace name: %s\r\n", buf[interfaceNum].ifr_name);

            if (0 != strncmp(buf[interfaceNum].ifr_name, (char*)"eth", 3)
                && 0 != strncmp(buf[interfaceNum].ifr_name, (char*)"em", 2)   /* 服务器网口支持 */
                && 0 != strncmp(buf[interfaceNum].ifr_name, (char*)"ens", 3)   /* 虚拟机网口的支持 */
                && 0 != strncmp(buf[interfaceNum].ifr_name, (char*)"bond", 4)  /* 绑定网口的支持 */
                && 0 != strncmp(buf[interfaceNum].ifr_name, (char*)"wlan", 4)  /* 无线网口的支持 */
                && 0 != strncmp(buf[interfaceNum].ifr_name, (char*)"mgmt", 4)) /* 管理网口的支持 */
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

                /* IP  地址*/
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
 函 数 名  : IsIPAddrIsLocalIP
 功能描述  : IP地址是否是本地ip地址
 输入参数  : char* strEthName
             char* strIPAddr
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年8月4日
    作    者   : 杨海锋
    修改内容   : 新生成函数

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

        /* 循环添加每个网卡的信息 */
        while (interfaceNum-- > 0)
        {
            /* 网卡名称*/
            printf("IsIPAddrIsLocalIP() InterFace name: %s\r\n", buf[interfaceNum].ifr_name);

            if (0 != strncmp(buf[interfaceNum].ifr_name, (char*)"eth", 3)
                && 0 != strncmp(buf[interfaceNum].ifr_name, (char*)"bond", 4)  /* 绑定网口的支持 */
                && 0 != strncmp(buf[interfaceNum].ifr_name, (char*)"wlan", 4)  /* 无线网口的支持 */
                && 0 != strncmp(buf[interfaceNum].ifr_name, (char*)"mgmt", 4)) /* 管理网口的支持 */
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
 函 数 名  : GetSystemRunStateProc
 功能描述  : ICE接口获取系统运行信息的处理
 输入参数  : std::string& inBuff
                            std::string& outBuff
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年4月17日 星期四
    作    者   : 杨海锋
    修改内容   : 新生成函数

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

    //解析XML
    iRet = inPacket.BuiltTree(inBuff.c_str(), inBuff.length());//生成DOM树结构.

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

    /* 解析出xml的消息类型 */
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
        case XML_GET_SYSTEMINFO : /* 获取系统信息 */
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "GetSystemRunStateProc() GetSystemInfoProc \r\n");
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Web通过ICE获取系统信息消息处理");
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Web access system info message processing through ICE");
            i = GetSystemInfoProc(strSN, outBuff);
            break;

        case XML_GET_USERINFO :   /* 获取用户信息 */
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "GetSystemRunStateProc() GetUserInfoProc \r\n");
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Web通过ICE获取用户信息消息处理");
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Web access user info message processing through ICE");
            i = GetUserInfoProc(strSN, outBuff);
            break;

        case XML_GET_DEVICEINFO : /* 获取设备信息 */
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "GetSystemRunStateProc() GetDeviceInfoProc \r\n");
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Web通过ICE获取设备信息消息处理");
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Web access device info message processing through ICE");
            i = GetDeviceInfoProc(strSN, outBuff);
            break;

        case XML_GET_TSUINFO :    /* 获取TSU 信息 */
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "GetSystemRunStateProc() GetTSUInfoProc \r\n");
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Web通过ICE获取TSU信息消息处理");
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Web access TSU info message processing through ICE");
            i = GetTSUInfoProc(strSN, outBuff);
            break;

        case XML_GET_TASKINFO :   /* 获取任务信息 */
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "GetSystemRunStateProc() GetTaskInfoProc \r\n");
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Web通过ICE获取任务信息消息处理");
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Web access task info message processing through ICE");
            i = GetTaskInfoProc(strSN, outBuff);
            break;

        case XML_SET_IPADDR :   /* 设置IP地址 */
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "GetSystemRunStateProc() SetIPAddrProc \r\n");
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Web通过ICE设置单板IP地址消息处理");
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Web set board IP address message processing through ICE");
            i = SetIPAddrProc(inPacket, outBuff);
            break;

        case XML_NOTIFY_REBOOT : /* 重启单板 */
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "GetSystemRunStateProc() BoardReboot \r\n");
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Web通过ICE通知重启单板消息处理");
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Web notify restarting board message processing through ICE");
            BoardReboot();
            break;

        case XML_NOTIFY_SHUTDOWN : /* 关闭单板 */
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "GetSystemRunStateProc() TurnOnffBoard \r\n");
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Web通过ICE通知关闭单板消息处理");
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Web notify closing board message processing through ICE");
            //TurnOnffBoard();
            break;

        case XML_NOTIFY_SENDCATALOG_TO_ROUTE_CMS : /* 向上级CMS发送目录 */
            inPacket.GetElementValue((char*)"routeid", strRouteID);

            if (strRouteID[0] != '\0')
            {
                iRet = SendNotifyCatalogMessageToRouteCMS(osip_atoi(strRouteID));
            }

            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "GetSystemRunStateProc() SendCatalog To Route CMS:RouteID=%s, iRet=%d \r\n", strRouteID, iRet);
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Web通过ICE通知发送目录消息到上级CMS的处理:RouteID=%s, iRet=%d \r\n", strRouteID, iRet);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Web notify send directory message To Route CMS processing through ICE:RouteID=%s, iRet=%d \r\n", strRouteID, iRet);

            break;

        case XML_NOTIFY_SENDCATALOG_TO_SUB_CMS : /* 向下级CMS发送目录 */
            inPacket.GetElementValue((char*)"cmsid", strCMSID);

            if (strCMSID[0] != '\0')
            {
                iRet = SendNotifyCatalogMessageToSubCMS(strCMSID);
            }

            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "GetSystemRunStateProc() SendCatalog To Sub CMS:CMSID=%s, iRet=%d\r\n", strCMSID, iRet);
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Web通过ICE通知发送目录消息到下级CMS的处理:CMSID=%s, iRet=%d\r\n", strCMSID, iRet);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Web notify send Catalog message To Sub CMS processing through ICE:CMSID=%s, iRet=%d\r\n", strCMSID, iRet);

            break;

        case XML_NOTIFY_REBOOTKEYBOARD : /* 通知重启键盘服务程序 */
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "GetSystemRunStateProc() : restart KeyBoardCtrl\r\n");
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Web通过ICE通知重启键盘服务进程KeyBoardCtrl处理");
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Web notify restart KeyBoardCtrl process processing through ICE");
            i = BoardSearchRebootKeyBoardProc();
            break;

        case XML_NOTIFY_REBOOTMEDIASERVICE : /* 通知重启自带媒体网关程序 */
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "GetSystemRunStateProc() : restart EV9000MediaService\r\n");
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Web通过ICE通知重启自带媒体网关进程EV9000MediaService处理");
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Web notify restart EV9000MediaService process processing through ICE");
            i = BoardSearchRebootMediaServiceProc();
            break;

        case XML_NOTIFY_RESTORE_FACTORY : /* 通知恢复出厂设置 */
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "GetSystemRunStateProc() : Restore factory settings\r\n");
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Web通过ICE通知恢复出厂设置处理");
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Web notify restore factory settings processing through ICE");
            i = BoardSearchRestoreFactorySettingsProc();
            break;

        case XML_NOTIFY_EXECUTE_CMD : /* 通知执行命令 */
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "GetSystemRunStateProc() : Execute cmd settings\r\n");
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Web通过ICE通知执行命令处理");
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Web notify execute cmd processing through ICE");
            inPacket.GetElementValue((char*)"CMD", strCMD);

            if (strCMD[0] != '\0')
            {
                strReturnBuff.clear();
                memset(buf, '\0', 256);  //初始化buf,以免后面写如乱码到文件中
                stream = popen(strCMD, "r");   //将“ls －l”命令的输出 通过管道读取（“r”参数）到FILE* stream

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

                        memset(buf, '\0', 256);  //初始化buf,以免后面写如乱码到文件中
                    }

                    pclose(stream);
                    stream = NULL;

                    outBuff = strReturnBuff;
                }
            }

            break;

        case XML_NOTIFY_REFRESH_DBCONFIG : /* Web通知刷新数据库配置 */
            inPacket.GetElementValue((char*)"tablename", strDBName);

            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "GetSystemRunStateProc() : Refresh DB Config\r\n");
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Web通过ICE通知刷新数据库配置:tablename=%s", strDBName);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Web notify refresh db config through ICE");

            if (strDBName[0] != '\0')
            {
                iRet = WebNotifyDBRefreshProc(strDBName);
            }

            break;

        case XML_NOTIFY_SYNC_DATABASE : /* Web通知同步数据库配置 */

            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "GetSystemRunStateProc() : Sync DB Config\r\n");
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Web通过ICE通知主备同步数据库配置");
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Web notify sync db config through ICE");

            iRet = WebNotifyDBSyncProc();
            break;

        case XML_NOTIFY_ENTER_SYSCONFIG : /* Web通知CMS进入配置 */
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "GetSystemRunStateProc() : Enter System settings\r\n");
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Web通过ICE通知进入系统设置处理");
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Web notify enter system settings processing through ICE");
            inPacket.GetElementValue((char*)"username", strUserName);

            if (strUserName[0] != '\0')
            {
                //iRet = shdb_user_operate_cmd_proc2(strUserName, EV9000_SHDB_DVR_PARAM_CONFIG);
            }

            break;

        case XML_NOTIFY_SAVE_SYSCONFIG : /* Web通知CMS保存配置 */
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "GetSystemRunStateProc() : Save System settings\r\n");
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Web通过ICE通知保存系统设置处理");
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Web notify save system settings processing through ICE");
            inPacket.GetElementValue((char*)"username", strUserName);

            if (strUserName[0] != '\0')
            {
                //iRet = shdb_user_operate_cmd_proc2(strUserName, EV9000_SHDB_DVR_PARAM_COMMIT);
            }

            break;

        case XML_NOTIFY_GET_VMS_CHANNEL : /* Web通知获取VMS的通道信息 */
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "GetSystemRunStateProc() : Get VMS Channel Info \r\n");
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Web通过ICE通知获取VMS通道信息处理");
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Web notify get VMS channel info processing through ICE");

            if (0 == g_GetChannelFlag)
            {
                g_GetChannelFlag = 1;
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Web通过ICE通知获取VMS通道信息处理成功!");
            }
            else
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Web通过ICE通知获取VMS通道信息处理失败,上次获取过程还没有结束!");
            }

            break;

        case XML_NOTIFY_GET_DEVICE_INFO : /* Web通知获取前端设备信息 */
            inPacket.GetElementValue((char*)"DeviceID", strDeviceID);
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "GetSystemRunStateProc() : Get Device Info:DeviceID=%s \r\n", strDeviceID);
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Web通过ICE通知获取前端物理设备信息:DeviceID=%s", strDeviceID);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Web notify get device info through ICE:DeviceID=%s", strDeviceID);

            if (strDeviceID[0] != '\0')
            {
                pGBDeviceInfo = GBDevice_info_find(strDeviceID);

                if (NULL != pGBDeviceInfo)
                {
                    iRet = SendQueryDeviceInfoMessage(pGBDeviceInfo);

                    if (0 != iRet)
                    {
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Web通过ICE通知获取前端物理设备信息失败:原因=发送消息到前端物理设备失败:DeviceID=%s \r\n", strDeviceID);
                    }
                }
                else
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Web通过ICE通知获取前端物理设备信息失败:原因=获取物理设备信息失败:DeviceID=%s \r\n", strDeviceID);
                }
            }
            else
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Web通过ICE通知获取前端物理设备信息失败:原因=DeviceID为空");
            }

            break;

        case XML_NOTIFY_GET_DEVICE_STATUS : /* Web通知获取前端设备状态 */
            inPacket.GetElementValue((char*)"DeviceID", strDeviceID);
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "GetSystemRunStateProc() : Get Device Status:DeviceID=%s \r\n", strDeviceID);
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Web通过ICE通知获取前端物理设备状态:DeviceID=%s", strDeviceID);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Web notify get device status through ICE:DeviceID=%s", strDeviceID);

            if (strDeviceID[0] != '\0')
            {
                pGBDeviceInfo = GBDevice_info_find(strDeviceID);

                if (NULL != pGBDeviceInfo)
                {
                    iRet = SendQueryDeviceStatusMessage(pGBDeviceInfo);

                    if (0 != iRet)
                    {
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Web通过ICE通知获取前端物理设备状态失败:原因=发送消息到前端物理设备失败:DeviceID=%s \r\n", strDeviceID);
                    }
                }
                else
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Web通过ICE通知获取前端物理设备状态失败:原因=获取物理设备信息失败:DeviceID=%s \r\n", strDeviceID);
                }
            }
            else
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Web通过ICE通知获取前端物理设备状态失败:原因=DeviceID为空");
            }

            break;

        case XML_NOTIFY_GET_DEVICE_CATALOG : /* Web通知获取前端设备目录 */
            inPacket.GetElementValue((char*)"DeviceID", strDeviceID);
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "GetSystemRunStateProc() : Get Device Catalog:DeviceID=%s \r\n", strDeviceID);
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Web通过ICE通知获取前端物理设备目录:DeviceID=%s", strDeviceID);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Web notify get device catalog through ICE:DeviceID=%s", strDeviceID);

            if (strDeviceID[0] != '\0')
            {
                pGBDeviceInfo = GBDevice_info_find(strDeviceID);

                if (NULL != pGBDeviceInfo)
                {
                    if (pGBDeviceInfo->device_type == EV9000_DEVICETYPE_SIPSERVER
                        && pGBDeviceInfo->link_type == 0
                        && pGBDeviceInfo->three_party_flag == 0) /* 非第三方平台 */
                    {
                        /*如果是下级CMS, 并且不是同级互联，则还需获取逻辑设备分组信息和逻辑设备分组关系信息 */
                        iRet = SendQueryDeviceGroupInfoMessage(pGBDeviceInfo);

                        if (0 != iRet)
                        {
                            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Web通过ICE通知获取下级平台分组信息失败:原因=发送消息到前端物理设备失败:DeviceID=%s \r\n", strDeviceID);
                        }

                        iRet = SendQueryDeviceGroupMapInfoMessage(pGBDeviceInfo);

                        if (0 != iRet)
                        {
                            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Web通过ICE通知获取下级平台分组关系信息失败:原因=发送消息到前端物理设备失败:DeviceID=%s \r\n", strDeviceID);
                        }
                    }

                    iRet = SendQueryDeviceCatalogMessage(pGBDeviceInfo);

                    if (0 != iRet)
                    {
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Web通过ICE通知获取前端物理设备目录失败:原因=发送消息到前端物理设备失败:DeviceID=%s \r\n", strDeviceID);
                    }
                }
                else
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Web通过ICE通知获取前端物理设备目录失败:原因=获取物理设备信息失败:DeviceID=%s \r\n", strDeviceID);
                }
            }
            else
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Web通过ICE通知获取前端物理设备目录失败:原因=DeviceID为空");
            }

            break;

        case XML_NOTIFY_DEVICE_TELEBOOT : /* Web通知前端设备远程重启 */
            inPacket.GetElementValue((char*)"DeviceID", strDeviceID);
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "GetSystemRunStateProc() : Notify Device teleboot:DeviceID=%s \r\n", strDeviceID);
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Web通过ICE通知前端物理设备远程重启:DeviceID=%s", strDeviceID);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Web notify device teleboot through ICE:DeviceID=%s", strDeviceID);

            if (strDeviceID[0] != '\0')
            {
                pGBDeviceInfo = GBDevice_info_find(strDeviceID);

                if (NULL != pGBDeviceInfo)
                {
                    iRet = SendDeviceTeleBootMessage(pGBDeviceInfo);

                    if (0 != iRet)
                    {
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Web通过ICE通知前端物理设备远程重启失败:原因=发送消息到前端物理设备失败:DeviceID=%s \r\n", strDeviceID);
                    }
                }
                else
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Web通过ICE通知前端物理设备远程重启失败:原因=获取物理设备信息失败:DeviceID=%s \r\n", strDeviceID);
                }
            }
            else
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Web通过ICE通知前端物理设备远程重启失败:原因=DeviceID为空");
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
 函 数 名  : GeneratingErrorResponseForSystemRunState
 功能描述  : ICE接口获取系统运行信息时候生成错误应答函数
 输入参数  : char* strCmdType
                            char* pcSerialNumber
                            int iErrorCode
                            char* strErrorReason
                            std::string& outBuff
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年4月17日 星期四
    作    者   : 杨海锋
    修改内容   : 新生成函数

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

    /* 回复响应,组建消息 */
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
 函 数 名  : CreateXMLHeadResponseForSystemRunState
 功能描述  : 创建ICE接口获取系统运行信息的应答消息XML头部
 输入参数  : CPacket** pOutPacket
                            char* strCmdType
                            char* pcSerialNumber
                            DOMElement** ListItemNode
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年4月17日 星期四
    作    者   : 杨海锋
    修改内容   : 新生成函数

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

    /* 回复响应,组建消息 */
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
 函 数 名  : AddSystemDateTimeInfoToSysInfoXML
 功能描述  : 添加系统时间到ICE接口获取系统运行信息的应答消息
 输入参数  : CPacket* pOutPacket
                            DOMElement* ListItemNode
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年4月17日 星期四
    作    者   : 杨海锋
    修改内容   : 新生成函数

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

    /* 填写XML数据*/
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
 函 数 名  : AddCPUInfoToSysInfoXML
 功能描述  : 添加CPU信息到ICE接口获取系统运行信息的应答消息
 输入参数  : CPacket* pOutPacket
                            DOMElement* ListItemNode
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年4月17日 星期四
    作    者   : 杨海锋
    修改内容   : 新生成函数

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

    /* 获取CPU的信息 */
    memset(&cpu_inf, 0, sizeof(cpu_info));
    process_num = get_cpu_info(&cpu_inf);
    //printf("\r\n CPU Process Total Number: %d \r\n", process_num);
    //printf("System Type: %s \r\n", cpu_inf.system_type);
    //printf("CPU Model: %s \r\n", cpu_inf.cpu_model);
    //printf("CPU BogoMIPS: %s \r\n", cpu_inf.BogoMIPS);

    //计算cpu使用率
    memset(&cpu_stat, 0, sizeof(cpu_status));
    get_cpu_status(&cpu_stat, -1);
    //printf("user\t nice\t system\t idle\n");
    //printf("%4.2f\t %4.2f\t %4.2f\t %4.2f\n", cpu_stat.user, cpu_stat.nice, cpu_stat.system, cpu_stat.idle);

    /* 填写XML数据*/
    ItemAccNode = pOutPacket->CreateElement((char*)"cpu");
    pOutPacket->SetCurrentElement(ItemAccNode);

    /* 系统类型 */
    ItemInfoAccNode = pOutPacket->CreateElement((char*)"system_type");
    pOutPacket->SetElementValue(ItemInfoAccNode, cpu_inf.system_type);

    /* BogoMIPS */
    ItemInfoAccNode = pOutPacket->CreateElement((char*)"BogoMIPS");
    pOutPacket->SetElementValue(ItemInfoAccNode, cpu_inf.BogoMIPS);

    /* 使用率 */
    ItemInfoAccNode = pOutPacket->CreateElement((char*)"use");
    sprintf(strCPUUse, "%4.2f", cpu_stat.user + cpu_stat.nice + cpu_stat.system);
    pOutPacket->SetElementValue(ItemInfoAccNode, strCPUUse);

    /* 核的个数 */
    ItemInfoAccNode = pOutPacket->CreateElement((char*)"cores");
    sprintf(strProcessNum, "%d", process_num);
    pOutPacket->SetElementValue(ItemInfoAccNode, strProcessNum);

    /* 温度 */
    ItemInfoAccNode = pOutPacket->CreateElement((char*)"temperature");

    sprintf(strCPUTemp, "%4.2f", fShelfCpuTemp);
    pOutPacket->SetElementValue(ItemInfoAccNode, strCPUTemp);

    /* 循环添加每个核的信息 */
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

        /* 核序列号*/
        CoreItemInfoAccNode = pOutPacket->CreateElement((char*)"processor");
        memset(strProcessCount, 0, 16);
        sprintf(strProcessCount, "%d", i);
        pOutPacket->SetElementValue(CoreItemInfoAccNode, strProcessCount);

        /* 核型号*/
        CoreItemInfoAccNode = pOutPacket->CreateElement((char*)"cpu_modle");
        pOutPacket->SetElementValue(CoreItemInfoAccNode, cpu_inf.cpu_model);

        /* BogoMIPS */
        CoreItemInfoAccNode = pOutPacket->CreateElement((char*)"BogoMIPS");
        pOutPacket->SetElementValue(CoreItemInfoAccNode, cpu_inf.BogoMIPS);

        /* 使用率 */
        CoreItemInfoAccNode = pOutPacket->CreateElement((char*)"use");
        sprintf(strCPUUse, "%4.2f", cpu_stat.user + cpu_stat.nice + cpu_stat.system);
        pOutPacket->SetElementValue(CoreItemInfoAccNode, strCPUUse);
    }

    /* 各个应用软件信息 */
    /* 1、获取CMS 的CPU 占用率 */
    i = get_progress_cpu_usage((char*)"cms", &cms_cpu_p);
    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "AddSystemDateTimeInfoToSysInfoXML() cms_cpu_p=%.6f \r\n", cms_cpu_p);

    pOutPacket->SetCurrentElement(ItemAccNode);
    SoftItemAccNode = pOutPacket->CreateElement((char*)"software");

    pOutPacket->SetCurrentElement(SoftItemAccNode);

    /* 名称*/
    SoftItemInfoAccNode = pOutPacket->CreateElement((char*)"name");
    pOutPacket->SetElementValue(SoftItemInfoAccNode, (char*)"CMS");

    /* 占用率*/
    SoftItemInfoAccNode = pOutPacket->CreateElement((char*)"use");
    sprintf(strCMSCPUP, "%.6f", cms_cpu_p);
    pOutPacket->SetElementValue(SoftItemInfoAccNode, strCMSCPUP);

    /* 2、获取MySQL 的CPU 占用率 */
    i = get_progress_cpu_usage((char*)"mysqld_safe", &mysql_cpu_p);
    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "AddSystemDateTimeInfoToSysInfoXML() mysqld=%.6f \r\n", mysql_cpu_p);

    pOutPacket->SetCurrentElement(ItemAccNode);
    SoftItemAccNode = pOutPacket->CreateElement((char*)"software");

    pOutPacket->SetCurrentElement(SoftItemAccNode);

    /* 名称*/
    SoftItemInfoAccNode = pOutPacket->CreateElement((char*)"name");
    pOutPacket->SetElementValue(SoftItemInfoAccNode, (char*)"MySQL");

    /* 占用率*/
    SoftItemInfoAccNode = pOutPacket->CreateElement((char*)"use");
    sprintf(strMySQLCPUP, "%.6f", mysql_cpu_p);
    pOutPacket->SetElementValue(SoftItemInfoAccNode, strMySQLCPUP);

    /* 3、获取Onvif 的CPU 占用率 */
    i = get_progress_cpu_usage((char*)"EV9000MediaService", &onvif_cpu_p);
    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "AddSystemDateTimeInfoToSysInfoXML() onvif_cpu_p=%.6f \r\n", onvif_cpu_p);

    pOutPacket->SetCurrentElement(ItemAccNode);
    SoftItemAccNode = pOutPacket->CreateElement((char*)"software");

    pOutPacket->SetCurrentElement(SoftItemAccNode);

    /* 名称*/
    SoftItemInfoAccNode = pOutPacket->CreateElement((char*)"name");
    pOutPacket->SetElementValue(SoftItemInfoAccNode, (char*)"OnvifProxy");

    /* 占用率*/
    SoftItemInfoAccNode = pOutPacket->CreateElement((char*)"use");
    sprintf(strONVIFCPUP, "%.6f", onvif_cpu_p);
    pOutPacket->SetElementValue(SoftItemInfoAccNode, strONVIFCPUP);

    return 0;
}

/*****************************************************************************
 函 数 名  : AddMemoryInfoToSysInfoXML
 功能描述  : 添加内存信息到ICE接口获取系统运行信息的应答消息
 输入参数  : CPacket* pOutPacket
                            DOMElement* ListItemNode
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年4月17日 星期四
    作    者   : 杨海锋
    修改内容   : 新生成函数

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

    //获取内存
    memset(&mem_inf, 0, sizeof(mem_info));
    get_mem_info(&mem_inf);
    //printf("total=\t%s\n", mem_inf.total);
    //printf("free=\t%s\n", mem_inf.free);

    /* 填写XML数据*/
    ItemAccNode = pOutPacket->CreateElement((char*)"memory");
    pOutPacket->SetCurrentElement(ItemAccNode);

    /* 总大小 */
    ItemInfoAccNode = pOutPacket->CreateElement((char*)"size");
    pOutPacket->SetElementValue(ItemInfoAccNode, mem_inf.total);

    /* 可用空间 */
    ItemInfoAccNode = pOutPacket->CreateElement((char*)"used");
    total = strtoul(mem_inf.total, NULL, 10);
    free = strtoul(mem_inf.free, NULL, 10);
    used = total - free;
    sprintf(strUsedMem, "%ld", used);
    pOutPacket->SetElementValue(ItemInfoAccNode, strUsedMem);

    /* 各个应用软件信息 */
    /* 1、获取CMS 的内存使用情况 */
    memset(&proc_mem, 0, sizeof(process_mem_info));
    i = get_progress_memory_usage((char*)"cms", &proc_mem);
    //printf("CMS Phy Mem=\t%s\n, V Mem=\t%s\n", proc_mem.mem, proc_mem.vmem);

    pOutPacket->SetCurrentElement(ItemAccNode);
    SoftItemAccNode = pOutPacket->CreateElement((char*)"software");

    pOutPacket->SetCurrentElement(SoftItemAccNode);

    /* 名称*/
    SoftItemInfoAccNode = pOutPacket->CreateElement((char*)"name");
    pOutPacket->SetElementValue(SoftItemInfoAccNode, (char*)"CMS");

    /* 已使用*/
    SoftItemInfoAccNode = pOutPacket->CreateElement((char*)"used");
    pOutPacket->SetElementValue(SoftItemInfoAccNode, proc_mem.mem);

    /* 2、获取MySQL 的CPU 占用率 */
    memset(&proc_mem, 0, sizeof(process_mem_info));
    i = get_progress_memory_usage((char*)"mysqld_safe", &proc_mem);
    //printf("Phy Mem=\t%s\n", proc_mem.mem);
    //printf("V Mem=\t%s\n", proc_mem.vmem);

    pOutPacket->SetCurrentElement(ItemAccNode);
    SoftItemAccNode = pOutPacket->CreateElement((char*)"software");

    pOutPacket->SetCurrentElement(SoftItemAccNode);

    /* 名称*/
    SoftItemInfoAccNode = pOutPacket->CreateElement((char*)"name");
    pOutPacket->SetElementValue(SoftItemInfoAccNode, (char*)"MySQL");

    /* 已使用*/
    SoftItemInfoAccNode = pOutPacket->CreateElement((char*)"used");
    pOutPacket->SetElementValue(SoftItemInfoAccNode, proc_mem.mem);

    /* 3、获取Onvif 的CPU 占用率 */
    memset(&proc_mem, 0, sizeof(process_mem_info));
    i = get_progress_memory_usage((char*)"OnvifProxy", &proc_mem);
    //printf("Phy Mem=\t%s\n", proc_mem.mem);
    //printf("V Mem=\t%s\n", proc_mem.vmem);

    pOutPacket->SetCurrentElement(ItemAccNode);
    SoftItemAccNode = pOutPacket->CreateElement((char*)"software");

    pOutPacket->SetCurrentElement(SoftItemAccNode);

    /* 名称*/
    SoftItemInfoAccNode = pOutPacket->CreateElement((char*)"name");
    pOutPacket->SetElementValue(SoftItemInfoAccNode, (char*)"OnvifProxy");

    /* 已使用*/
    SoftItemInfoAccNode = pOutPacket->CreateElement((char*)"used");
    pOutPacket->SetElementValue(SoftItemInfoAccNode, proc_mem.mem);

    return 0;
}

/*****************************************************************************
 函 数 名  : AddFanInfoToSysInfoXML
 功能描述  :  添加风扇信息到ICE接口获取系统运行信息的应答消息
 输入参数  : CPacket* pOutPacket
                            DOMElement* ListItemNode
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年4月17日 星期四
    作    者   : 杨海锋
    修改内容   : 新生成函数

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

    /* 填写XML数据*/
    ItemAccNode = pOutPacket->CreateElement((char*)"fans");
    pOutPacket->SetCurrentElement(ItemAccNode);

    return 0;
}

/*****************************************************************************
 函 数 名  : AddNetWorkInfoToSysInfoXML
 功能描述  :  添加网络信息到ICE接口获取系统运行信息的应答消息
 输入参数  : CPacket* pOutPacket
                            DOMElement* ListItemNode
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年4月17日 星期四
    作    者   : 杨海锋
    修改内容   : 新生成函数

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

    /* 填写XML数据*/
    ItemAccNode = pOutPacket->CreateElement((char*)"network");
    pOutPacket->SetCurrentElement(ItemAccNode);

    ifc.ifc_len = sizeof(buf);
    ifc.ifc_buf = (caddr_t)buf;

    if (!ioctl(fd, SIOCGIFCONF, (char*)&ifc))
    {
        interfaceNum = ifc.ifc_len / sizeof(struct ifreq);
        //printf("interface num = %d\n", interfaceNum);

        /* 循环添加每个网卡的信息 */
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

            /* 网卡名称*/
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

                /* MAC 地址*/
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

                /* IP  地址*/
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

                /* broadAddr  地址*/
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

                /* mask  地址*/
                InterfaceItemInfoAccNode = pOutPacket->CreateElement((char*)"mask");
                pOutPacket->SetElementValue(InterfaceItemInfoAccNode, subnetMask);
            }
            else
            {
                printf("ioctl: %s [%s:%d]\n", strerror(errno), __FILE__, __LINE__);
                close(fd);
                return -1;
            }

            /* 网关地址 */
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
 函 数 名  : BoardSearchProc
 功能描述  : 单板搜索处理
 输入参数  : char* pcInBuff
                            int iInLen
                            std::string& outBuff
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年6月18日 星期三
    作    者   : 杨海锋
    修改内容   : 新生成函数

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

    //解析XML
    iRet = inPacket.BuiltTree(pcInBuff, iInLen);//生成DOM树结构.

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

    /* 解析出xml的消息类型 */
    xml_type = get_xml_type_from_xml_body(NodeName_Vector, inPacket);
    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "BoardSearchProc() get_xml_type_from_xml_body:xml_type=%d \r\n", xml_type);
    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "接收到单板搜索命令消息, 命令类型=%d", xml_type);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Received board search command message, command type=%d", xml_type);

    inPacket.GetElementValue((char*)"serial_number", strSN);

    switch (xml_type)
    {
        case XML_SEARCH_BOARD : /* 单板搜索 */
            i = BoardSearchCmdProc(strSN, outBuff);
            break;

        case XML_SET_IPADDR :   /* 设置IP 地址 */
            i = BoardSearchSetIPAddrProc(inPacket, outBuff);
            break;

        case XML_NOTIFY_REBOOTCMS : /* 重启CMS */
            i = BoardSearchRebootCMSProc();
            break;

        case XML_NOTIFY_REBOOTDB : /* 重启数据库 */
            i = BoardSearchRebootDBProc();
            break;

        case XML_NOTIFY_REBOOTKEYBOARD : /* 通知重启键盘服务程序 */
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "BoardSearchProc() : restart KeyBoardCtrl\r\n");
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "单板搜索命令接收到重启键盘服务进程KeyBoardCtrl处理");
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Board search command restart KeyBoardCtrl process");
            i = BoardSearchRebootKeyBoardProc();
            break;

        case XML_NOTIFY_REBOOTMEDIASERVICE : /* 通知重启自带媒体网关程序 */
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "BoardSearchProc() : restart EV9000MediaService\r\n");
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "单板搜索命令接收到重启自带媒体网关进程EV9000MediaService处理");
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Board search command restart EV9000MediaService process");
            i = BoardSearchRebootMediaServiceProc();
            break;

        case XML_NOTIFY_LOCATION : /* 单板定位 */
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "BoardSearchProc() BoardLocation \r\n");
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "单板搜索命令单板定位处理");
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Board search command board locate process");
            //BoardLocation();
            break;

        case XML_NOTIFY_REBOOT : /* 重启单板 */
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "BoardSearchProc() BoardReboot \r\n");
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "单板搜索命令重启单板处理");
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Board search command restart board process");
            BoardReboot();
            break;

        case XML_NOTIFY_SHUTDOWN : /* 关闭单板 */
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "BoardSearchProc() TurnOnffBoard \r\n");
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "单板搜索命令关闭单板处理");
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Board search command close board process");
            //TurnOnffBoard();
            break;

        default:
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "单板搜索命令, 未定义的消息命令类型=%d", xml_type);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Board search command, undefined message command type=%d", xml_type);
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "BoardSearchProc() exit---: Not Support Message Type:%d \r\n", xml_type);
            return -1;
    }

    //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "BoardSearchProc() OutBuffer=%s \r\n", outBuff.c_str());

    return i;
}

/*****************************************************************************
 函 数 名  : AddNetWorkInfoToBoardSearchXML
 功能描述  : 添加网络信息到单板搜索的XML消息
 输入参数  : CPacket* pOutPacket
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年7月8日 星期二
    作    者   : 杨海锋
    修改内容   : 新生成函数

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

        /* 循环添加每个网卡的信息 */
        while (interfaceNum-- > 0)
        {
            if (0 == sstrcmp(buf[interfaceNum].ifr_name, (char*)"lo"))
            {
                continue;
            }

            pOutPacket->SetCurrentElement(ItemAccNode);

            /* 填写XML数据*/
            InterfaceItemAccNode = pOutPacket->CreateElement((char*)"ethernet");
            pOutPacket->SetCurrentElement(InterfaceItemAccNode);

            //printf("\ndevice name: %s\n", buf[interfaceNum].ifr_name);

            /* 网卡名称*/
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

                /* MAC 地址*/
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

                /* IP  地址*/
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

                /* broadAddr  地址*/
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

                /* mask  地址*/
                InterfaceItemInfoAccNode = pOutPacket->CreateElement((char*)"mask");
                pOutPacket->SetElementValue(InterfaceItemInfoAccNode, subnetMask);
            }
            else
            {
                printf("ioctl: %s [%s:%d]\n", strerror(errno), __FILE__, __LINE__);
                close(fd);
                return -1;
            }

            /* 网关地址 */
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
 函 数 名  : AddDeviceInfoToBoardSearchXML
 功能描述  : 添加设备信息到单板搜索响应XML消息中
 输入参数  : CPacket* pOutPacket
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年7月8日 星期二
    作    者   : 杨海锋
    修改内容   : 新生成函数

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

    /* 填写XML数据*/
    ItemAccNode = pOutPacket->CreateElement((char*)"deviceinfo");
    pOutPacket->SetCurrentElement(ItemAccNode);

    ItemInfoAccNode = pOutPacket->CreateElement((char*)"type");
    pOutPacket->SetElementValue(ItemInfoAccNode, (char*)"CMS");

    ItemInfoAccNode = pOutPacket->CreateElement((char*)"status");
    pOutPacket->SetElementValue(ItemInfoAccNode, (char*)"OnLine");

    ItemInfoAccNode = pOutPacket->CreateElement((char*)"slotid");
    pOutPacket->SetElementValue(ItemInfoAccNode, (char*)"0");

    /* 增加版本信息 */
    ItemInfoAccNode = pOutPacket->CreateElement((char*)"appVersion");
    //GetAppVerInfoForRoute(strCMSVer);
    pOutPacket->SetElementValue(ItemInfoAccNode, strCMSVer);

    /* 添加网络信息 */
    i = AddNetWorkInfoToBoardSearchXML(pOutPacket, ItemAccNode);

    return i;
}

/*****************************************************************************
 函 数 名  : BoardSearchSetIPAddrProc
 功能描述  : 单板搜索后的IP地址设置处理
 输入参数  : CPacket& inPacket
             std::string& outBuff
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年7月8日 星期二
    作    者   : 杨海锋
    修改内容   : 新生成函数

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

    /* 获取所有的网口信息数据 */
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

    /* 创建XML的头部 */
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
        /* 重启单板 */
        BoardReboot();
    }

    return i;
}

/*****************************************************************************
 函 数 名  : BoardSearchCmdProc
 功能描述  : 单板搜索命令处理
 输入参数  : char* pcSerialNumber
             std::string& outBuff
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年7月8日 星期二
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int BoardSearchCmdProc(char * pcSerialNumber, std::string & outBuff)
{
    int i = 0;

    CPacket* pOutPacket = NULL;
    DOMDocument* pDOMDocument = NULL;
    DOMElement* pDOMElement = NULL;

    /* 创建XML的头部 */
    i |= CreateXMLHeadResponseForSystemRunState(&pOutPacket, (char*)"EV9000SearchBoard", pcSerialNumber);

    if (i != 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "BoardSearchCmdProc() exit---: Create XML Packet Error \r\n");
        return -1;
    }

    pDOMDocument = pOutPacket->GetDOMDocument();
    pDOMElement = pDOMDocument->get_root();

    /* 添加设备信息 */
    pOutPacket->SetCurrentElement(pDOMElement);
    i |= AddDeviceInfoToBoardSearchXML(pOutPacket);

    outBuff = pOutPacket->GetXml(NULL, "UTF-8");

    delete pOutPacket;

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "单板搜索命令应答处理，CMS ID=%s, CMS IP=%s", local_cms_id_get(), local_ip_get(default_eth_name_get()));
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Board search command response process，CMS ID=%s, CMS IP=%s", local_cms_id_get(), local_ip_get(default_eth_name_get()));

    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "BoardSearchCmdProc() exit---: outBuff=%s \r\n", outBuff.c_str());

    return i;
}

/*****************************************************************************
 函 数 名  : BoardSearchRebootCMSProc
 功能描述  : 单板搜索接口重启CMS进程处理
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年7月8日 星期二
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int BoardSearchRebootCMSProc()
{
    int i = 0;
    int iRet = 0;
    int pid_t[128] = {0};
    int pid = 0;

    /* 获取进程的pid */
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
    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "单板搜索命令重启CMS处理，pid=%d", pid);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Board search command restart CMS process，pid=%d", pid);
    osip_usleep(10000000);

    //sprintf(strCmd, "kill %d", pid);
    //system(strCmd);
    system((char*)"killall cms; killall -9 cms");

    return 0;
}

/*****************************************************************************
 函 数 名  : BoardSearchRebootDBProc
 功能描述  : 单板搜索接口重启数据库进程处理
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年7月8日 星期二
    作    者   : 杨海锋
    修改内容   : 新生成函数

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

    /* 获取进程的pid */
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
    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "单板搜索命令重启mysql处理");
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Board search command restart mysql process");

    system("/bin/sh /etc/stop_mysql &");
    sleep(1);
    system("/bin/sh /etc/start_mysql &");

    return 0;
}

/*****************************************************************************
 函 数 名  : BoardSearchRebootKeyBoardProc
 功能描述  : 重启KeyBoardCtrl进程
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年11月25日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int BoardSearchRebootKeyBoardProc()
{
    system((char*)"killall KeyBoardCtrl; killall -9 KeyBoardCtrl");

    return 0;
}

/*****************************************************************************
 函 数 名  : BoardSearchRebootMediaServiceProc
 功能描述  : 重启EV9000MediaService进程
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年11月25日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int BoardSearchRebootMediaServiceProc()
{
    system((char*)"killall EV9000MediaService; killall -9 EV9000MediaService");

    return 0;
}

/*****************************************************************************
 函 数 名  : BoardSearchRestoreFactorySettingsProc
 功能描述  : 恢复出厂设置
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年12月15日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int BoardSearchRestoreFactorySettingsProc()
{
    /* 删除数据库 */
    system("/app/delete_DB.sh");

    /* 删除配置文件 */
    system("rm -rf /config/*.cfg");
    system("rm -rf /config/*.conf");
    system("rm -rf /config/config");

    /* 删除日志文件 */
    system("rm -rf /data/log/*");

    /* 删除手机数据库 */
    system("/app/delete_DB_mobile.sh");

    /* 写个默认的web配置文件，防止web启动的时候，CMS还没有起来，NPE版本变为视讯版本 */
    write_default_web_config_file();

    /* 自动重启 */
    BoardReboot();

    return 0;
}

/*****************************************************************************
 函 数 名  : SendSystemDiskErrorToAllClientUser
 功能描述  : 发送系统磁盘故障给在线用户
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年11月28日
    作    者   : 杨海锋
    修改内容   : 新生成函数

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

    /* 发送报警数据给客户端用户 */
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
    outPacket.SetElementValue(AccNode, (char*)"系统硬盘第二个分区故障,不能读写, 将影响系统日志和录像记录的操作, 请从Web尝试格式化操作进行恢复, 或者联系厂家技术支持");

    i = SendMessageToOnlineUser((char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length(), 0);

    if (i > 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "发送系统磁盘故障消息给用户成功");
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "SendSystemDiskErrorToAllClientUser() SendMessageToOnlineUser OK");
    }
    else if (i < 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "发送系统磁盘故障消息给用户失败");
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "SendSystemDiskErrorToAllClientUser() SendMessageToOnlineUser Error");
    }

    return i;
}
