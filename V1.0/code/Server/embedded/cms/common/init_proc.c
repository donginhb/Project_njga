/******************************************************************************

                  版权所有 (C), 2001-2013, 金智视讯技术有限公司

 ******************************************************************************
  文 件 名   : initfunc.c
  版 本 号   : 初稿
  作    者   : yanghaifeng
  生成日期   : 2013年4月1日
  最近修改   :
  功能描述   : 初始化
  函数列表   :
              SIP_Free
              SIP_Init
              sip_stack_free
              sip_stack_init
  修改历史   :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 创建文件

******************************************************************************/

/*----------------------------------------------*
 * 包含头文件                                   *
 *----------------------------------------------*/
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

#include "libsip.h"

#include "common/db_proc.h"
#include "common/init_proc.inc"
#include "common/gblconfig_proc.inc"
#include "common/log_proc.inc"
#include "common/common_thread_proc.inc"

#include "user/user_thread_proc.inc"
#include "user/user_info_mgn.inc"
#include "user/user_reg_proc.inc"
#include "user/user_srv_proc.inc"

#include "device/device_info_mgn.inc"
#include "device/device_reg_proc.inc"
#include "device/device_thread_proc.inc"

#include "resource/resource_info_mgn.inc"

#include "record/record_info_mgn.inc"
#include "record/record_srv_proc.inc"

#include "route/route_info_mgn.inc"
#include "route/route_thread_proc.inc"
#include "route/platform_info_mgn.inc"
#include "route/platform_thread_proc.inc"

#include "service/poll_srv_proc.inc"
#include "service/plan_srv_proc.inc"
#include "service/cruise_srv_proc.inc"
#include "service/call_func_proc.inc"
#include "service/compress_task_proc.inc"
#include "service/alarm_proc.inc"
#include "service/preset_proc.inc"

/*----------------------------------------------*
 * 外部变量说明                                 *
 *----------------------------------------------*/
extern DBOper g_DBOper;

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
 函 数 名  : InitModuleStaticListData
 功能描述  : 初始化静态数据队列
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年6月4日 星期二
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int InitModuleStaticListData()
{
    int i = 0;

    /* 标准物理设备管理队列 */
    i = ZRVDevice_info_list_init();

    if (i != 0)
    {
        printf(" InitModuleStaticListData() exit---: ZRVDevice_info_list_init Error \r\n");
        return -1;
    }

    /* TSU创建任务返回结果消息队列 */
    i = tsu_creat_task_result_msg_list_init();

    if (i != 0)
    {
        ZRVDevice_info_list_free();
        printf(" InitModuleStaticListData() exit---: tsu_creat_task_result_msg_list_init Error \r\n");
        return -1;
    }

    /* 路由信息管理队列 */
    i = platform_info_list_init();

    if (i != 0)
    {
        ZRVDevice_info_list_free();
        tsu_creat_task_result_msg_list_free();
        printf(" InitModuleStaticListData() exit---: platform_info_list_init Error \r\n");
        return -1;
    }

    /* 线程处理队列初始化 */
    i = thread_proc_list_init();

    if (i != 0)
    {
        ZRVDevice_info_list_free();
        tsu_creat_task_result_msg_list_free();
        platform_info_list_free();
        printf(" InitModuleStaticListData() exit---: thread_proc_list_init Error \r\n");
        return -1;
    }

    /* 上级平台业务处理线程队列初始化 */
    i = platform_srv_proc_thread_list_init();

    if (i != 0)
    {
        ZRVDevice_info_list_free();
        tsu_creat_task_result_msg_list_free();
        platform_info_list_free();
        thread_proc_list_free();
        printf(" InitModuleStaticListData() exit---: platform_srv_proc_thread_list_init Error \r\n");
        return -1;
    }

    /* 上级平台业务处理线程队列初始化 */
    i = compress_task_list_init();

    if (i != 0)
    {
        ZRVDevice_info_list_free();
        tsu_creat_task_result_msg_list_free();
        platform_info_list_free();
        thread_proc_list_free();
        platform_srv_proc_thread_list_free();
        printf(" InitModuleStaticListData() exit---: compress_task_list_init Error \r\n");
        return -1;
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : FreeModuleStaticListData
 功能描述  : 释放静态数据队列
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年6月4日 星期二
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void FreeModuleStaticListData()
{
    compress_task_list_free();
    printf(" FreeModuleStaticListData() compress_task_list_free OK \r\n");

    platform_srv_proc_thread_list_free();
    printf(" FreeModuleStaticListData() platform_srv_proc_thread_list_free OK \r\n");

    thread_proc_list_free();
    printf(" FreeModuleStaticListData() thread_proc_list_free OK \r\n");

    platform_info_list_free();
    printf(" FreeModuleStaticListData() platform_info_list_free OK \r\n");

    tsu_creat_task_result_msg_list_free();
    printf(" FreeModuleStaticListData() tsu_creat_task_result_msg_list_free OK \r\n");

    ZRVDevice_info_list_free();
    printf(" FreeModuleStaticListData() ZRVDevice_info_list_free OK \r\n");

    return;
}

/*****************************************************************************
 函 数 名  : CreateModuleProcThread
 功能描述  : 创建各个模块处理线程
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年6月4日 星期二
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int CreateModuleProcThread()
{
    int i = 0;

    /* 线程监控线程 */
    i = thread_monitor_proc_thread_start();

    if (i != 0)
    {
        printf(" CreateModuleProcThread() exit---: thread_monitor_proc_thread_start Error \r\n");
        return -1;
    }

    i = thread_proc_thread_start_all();

    if (i != 0)
    {
        thread_monitor_proc_thread_stop();
        printf(" CreateModuleProcThread() exit---: thread_proc_thread_start_all Error \r\n");
        return -1;
    }

    /* 上级平台业务处理线程 */
    i = platform_srv_proc_thread_start_all();

    if (i != 0)
    {
        thread_proc_thread_stop_all();
        thread_monitor_proc_thread_stop();
        user_srv_proc_thread_stop_all();
        device_srv_proc_thread_stop_all();
        printf(" CreateModuleProcThread() exit---: platform_srv_proc_thread_start_all Error \r\n");
        return -1;
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : DestroyModuleProcThread
 功能描述  : 销毁各个模块处理线程
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年6月4日 星期二
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void DestroyModuleProcThread()
{
    platform_srv_proc_thread_stop_all();
    printf(" DestroyModuleProcThread() platform_srv_proc_thread_stop_all OK \r\n");

    thread_proc_thread_stop_all();
    printf(" DestroyModuleProcThread() thread_proc_thread_stop_all OK \r\n");

    thread_monitor_proc_thread_stop();
    printf(" DestroyModuleProcThread() thread_monitor_proc_thread_stop OK \r\n");

    return;
}

/*****************************************************************************
 函 数 名  : DowndataFromDatabase
 功能描述  : 从数据库加载数据
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年6月4日 星期二
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int DowndataFromDatabase()
{
    int ret = 0;

    /*加载物理设备信息*/
    ret = set_db_data_to_ZRVDevice_info_list();

    if (ret != 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "DowndataFromDatabase() exit---: set_db_data_to_ZRVDevice_info_list Error \r\n");
    }

    /*加载互连路由信息*/
    ret = set_db_data_to_platform_info_list();

    if (ret != 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "DowndataFromDatabase() exit---: set_db_data_to_platform_info_list Error \r\n");
    }

    //ret = add_compress_task_for_test((char*)"192.168.0.100", &g_DBOper);

    return ret;
}

/*****************************************************************************
 函 数 名  : StopAllService
 功能描述  : 停止所有业务
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年7月24日 星期三
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void StopAllService()
{
    StopAllServiceTaskWhenExit();
}
