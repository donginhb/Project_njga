/******************************************************************************

                  ��Ȩ���� (C), 2001-2013, ������Ѷ�������޹�˾

 ******************************************************************************
  �� �� ��   : initfunc.c
  �� �� ��   : ����
  ��    ��   : yanghaifeng
  ��������   : 2013��4��1��
  ����޸�   :
  ��������   : ��ʼ��
  �����б�   :
              SIP_Free
              SIP_Init
              sip_stack_free
              sip_stack_init
  �޸���ʷ   :
  1.��    ��   : 2013��4��1��
    ��    ��   : yanghaifeng
    �޸�����   : �����ļ�

******************************************************************************/

/*----------------------------------------------*
 * ����ͷ�ļ�                                   *
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
 * �ⲿ����˵��                                 *
 *----------------------------------------------*/
extern DBOper g_DBOper;

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
 �� �� ��  : InitModuleStaticListData
 ��������  : ��ʼ����̬���ݶ���
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��6��4�� ���ڶ�
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int InitModuleStaticListData()
{
    int i = 0;

    /* ��׼�����豸������� */
    i = ZRVDevice_info_list_init();

    if (i != 0)
    {
        printf(" InitModuleStaticListData() exit---: ZRVDevice_info_list_init Error \r\n");
        return -1;
    }

    /* TSU�������񷵻ؽ����Ϣ���� */
    i = tsu_creat_task_result_msg_list_init();

    if (i != 0)
    {
        ZRVDevice_info_list_free();
        printf(" InitModuleStaticListData() exit---: tsu_creat_task_result_msg_list_init Error \r\n");
        return -1;
    }

    /* ·����Ϣ������� */
    i = platform_info_list_init();

    if (i != 0)
    {
        ZRVDevice_info_list_free();
        tsu_creat_task_result_msg_list_free();
        printf(" InitModuleStaticListData() exit---: platform_info_list_init Error \r\n");
        return -1;
    }

    /* �̴߳�����г�ʼ�� */
    i = thread_proc_list_init();

    if (i != 0)
    {
        ZRVDevice_info_list_free();
        tsu_creat_task_result_msg_list_free();
        platform_info_list_free();
        printf(" InitModuleStaticListData() exit---: thread_proc_list_init Error \r\n");
        return -1;
    }

    /* �ϼ�ƽ̨ҵ�����̶߳��г�ʼ�� */
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

    /* �ϼ�ƽ̨ҵ�����̶߳��г�ʼ�� */
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
 �� �� ��  : FreeModuleStaticListData
 ��������  : �ͷž�̬���ݶ���
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��6��4�� ���ڶ�
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

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
 �� �� ��  : CreateModuleProcThread
 ��������  : ��������ģ�鴦���߳�
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��6��4�� ���ڶ�
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int CreateModuleProcThread()
{
    int i = 0;

    /* �̼߳���߳� */
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

    /* �ϼ�ƽ̨ҵ�����߳� */
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
 �� �� ��  : DestroyModuleProcThread
 ��������  : ���ٸ���ģ�鴦���߳�
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��6��4�� ���ڶ�
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

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
 �� �� ��  : DowndataFromDatabase
 ��������  : �����ݿ��������
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��6��4�� ���ڶ�
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int DowndataFromDatabase()
{
    int ret = 0;

    /*���������豸��Ϣ*/
    ret = set_db_data_to_ZRVDevice_info_list();

    if (ret != 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "DowndataFromDatabase() exit---: set_db_data_to_ZRVDevice_info_list Error \r\n");
    }

    /*���ػ���·����Ϣ*/
    ret = set_db_data_to_platform_info_list();

    if (ret != 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "DowndataFromDatabase() exit---: set_db_data_to_platform_info_list Error \r\n");
    }

    //ret = add_compress_task_for_test((char*)"192.168.0.100", &g_DBOper);

    return ret;
}

/*****************************************************************************
 �� �� ��  : StopAllService
 ��������  : ֹͣ����ҵ��
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��7��24�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void StopAllService()
{
    StopAllServiceTaskWhenExit();
}
