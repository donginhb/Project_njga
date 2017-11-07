/*----------------------------------------------*
 * ����ͷ�ļ�                                   *
 *----------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#ifdef WIN32
#include <windows.h>
#include <winsock.h>
#else
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#endif

#include "libsip.h"

#include "common/gbldef.inc"
#include "common/gblfunc_proc.inc"
#include "common/gblconfig_proc.inc"
#include "common/callback_proc.inc"
#include "common/log_proc.inc"
#include "common/db_proc.h"

#include "user/user_info_mgn.inc"
#include "user/user_reg_proc.inc"
#include "user/user_srv_proc.inc"
#include "user/user_thread_proc.inc"

#include "device/device_info_mgn.inc"
#include "device/device_reg_proc.inc"
#include "device/device_thread_proc.inc"

#include "record/record_srv_proc.inc"

#include "route/route_info_mgn.inc"
#include "route/route_thread_proc.inc"

#include "service/call_func_proc.inc"
#include "service/alarm_proc.inc"

/*----------------------------------------------*
 * �ⲿ����˵��                                 *
 *----------------------------------------------*/
extern gbl_conf_t* pGblconf;              /* ȫ��������Ϣ */
extern DBOper g_DBOper;
extern int cms_run_status;  /* 0:û������,1:�������� */

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

#if DECS("�ϲ�Ӧ�ûص�������")
/*****************************************************************************
 �� �� ��  : uas_register_received_proc
 ��������  : ������յ�ע����Ϣ�Ĵ�����,
             ���������Ҫ����ע����Ϣ
 �������  : char* proxy_id,������ID
             register_id, ע���id
             login_ip, ��¼IP
             login_port, ��¼port
             register_name ע���û���
             reg_info_index,ע��������
             expires,ʱ��
             link_type, ��������
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��10��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int uas_register_received_proc(char* proxy_id, char* register_id, char* login_ip, int login_port, char* register_name, int reg_info_index, int expires, int link_type)
{
    int i = 0;
    char* pTmp = NULL;
    char strDeviceType[4] = {0};
    int iType = 0;
    char strLocalMgwID[36] = {0}; /* ����Mgw���� */
    char strErrorCode[32] = {0};

    if (0 == cms_run_status)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "uas_register_received_proc() exit---: CMS Not Run Normal Error \r\n");
        return -1;
    }

    if (NULL == register_id || NULL == login_ip)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "uas_register_received_proc() exit---: Register ID NULL \r\n");
        return -1;
    }

    DEBUG_TRACE(MODULE_COMMON, LOG_INFO,  "uas_register_received_proc() \
    \r\n In Param: \
    \r\n proxy_id=%s \
    \r\n register_id=%s \
    \r\n login_ip=%s \
    \r\n login_port=%d \
    \r\n register_name=%s \
    \r\n reg_info_index=%d \
    \r\n expires=%d \
    \r\n link_type=%d \
    \r\n", proxy_id, register_id, login_ip, login_port, register_name, reg_info_index, expires, link_type);

    /* ע����Ϣ��Ҫ�ǿͻ��˺������׼�����豸��ע�� */
    /* ��ȡע��id�е��豸���ͣ��Ա������Ӧ����Ϣ���� */
    pTmp = &register_id[10];
    osip_strncpy(strDeviceType, pTmp, 3);
    iType = osip_atoi(strDeviceType);

    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "uas_register_received_proc() DeviceType=%d \r\n", iType);

    if (NULL != proxy_id && '\0' != proxy_id[0] && 0 != sstrcmp(proxy_id, local_cms_id_get()))
    {
        EnSystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_WARNING, "Device registration failed:Device ID=%s, Device IP address =%s, Port number=%d, Cause=%s, Registration server ID=%s, Local config CMS ID=%s", register_id, login_ip, login_port, (char*)"Registration server ID do not match with local config CMS ID", proxy_id, local_cms_id_get());
        SystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_WARNING, "�豸ע��ʧ��:�豸ID=%s, �豸IP��ַ=%s, �˿ں�=%d, ԭ��=%s, ע��ķ�����ID=%s, �������õ�CMS ID=%s", register_id, login_ip, login_port, (char*)"�豸ע�������ID�ͱ������õ�CMS ID��ƥ��", proxy_id, local_cms_id_get());

        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_REG_SERVER_ID_NOT_MATCH_ERROR);
        SIP_UASAnswerToRegister(reg_info_index, 403, strErrorCode);
        //SIP_UASAnswerToRegister(reg_info_index, 403, (char*)"Register Server ID Not Matching With Local CMSID");
        return -1;
    }

    /* ����Ǳ��ص�ý�����أ�����IP��ַ�Ƿ��Ǳ���IP��ַ */
    if (EV9000_DEVICETYPE_MGWSERVER == iType)
    {
        if (pGblconf->center_code[0] != '\0' && pGblconf->trade_code != '\0')
        {
            snprintf(strLocalMgwID, 36, "%s%s%s", pGblconf->center_code, pGblconf->trade_code, (char*)"2091090000");

            if (0 == sstrcmp(register_id, strLocalMgwID))
            {
                if (!IsLocalHost(login_ip))
                {
                    EnSystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_WARNING, "Device registration failed:Device ID=%s,  Device IP address=%s, Port number=%d, Cause=%s", register_id, login_ip, login_port, (char*)"Local media gateway registration IP address do not match with local IP address");
                    SystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_WARNING, "�豸ע��ʧ��:�豸ID=%s, �豸IP��ַ=%s, �˿ں�=%d, ԭ��=%s", register_id, login_ip, login_port, (char*)"���ص�ý������ע���IP��ַ�ͱ���IP��ַ��ƥ��");

                    memset(strErrorCode, 0, 32);
                    snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_REG_SERVER_IP_NOT_MATCH_ERROR);
                    SIP_UASAnswerToRegister(reg_info_index, 403, strErrorCode);
                    //SIP_UASAnswerToRegister(reg_info_index, 403, (char*)"Register Login IP Not Matching With Local IP");
                    return -1;
                }
            }
        }

        /* ���ע���IP�ĵ�ַ�Ƿ��Ǳ�����IP��ַ */
        if (checkLoginIPIsSlaveIP(login_ip))
        {
            EnSystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_WARNING, "Device registration failed:Device ID=%s,  Device IP address=%s, Port number=%d, Cause=%s", register_id, login_ip, login_port, (char*)"Media gateway registration IP address is belong to slave cms");
            SystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_WARNING, "�豸ע��ʧ��:�豸ID=%s, �豸IP��ַ=%s, �˿ں�=%d, ԭ��=%s", register_id, login_ip, login_port, (char*)"ý������ע���IP��ַ�Ǳ��������");

            memset(strErrorCode, 0, 32);
            snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_REG_SERVER_IP_NOT_MATCH_ERROR);
            SIP_UASAnswerToRegister(reg_info_index, 403, strErrorCode);
            //SIP_UASAnswerToRegister(reg_info_index, 403, (char*)"Register Login IP Not Matching With Local IP");
            return -1;
        }
    }

#if 0

    if (sys_show_code_flag_get()) /* ����ģʽ */
    {
        if (iType == EV9000_DEVICETYPE_DECODER && !IsIDMatchLocalCMSID(register_id)) /* ������ע����Ҫ�ж�ID�Ƿ�ƥ�䣬��ֹ�ǹ��������ע�� */
        {
            SystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "�豸ע��ʧ��:�豸ID=%s, �豸IP��ַ=%s, �˿ں�=%d, ԭ��=%s, ����CMS ID=%s", register_id, login_ip, login_port, (char*)"�豸ע��ID�ͱ���CMS ID��ƥ��", local_cms_id_get());
            EnSystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "Device registration failed:Device ID=%s,  Device IP address=%s, Port number=%d, Cause=%s, Local CMS ID=%s", register_id, login_ip, login_port, (char*)"Local CMS ID  do not match with registration id.", local_cms_id_get());
            SIP_UASAnswerToRegister(reg_info_index, 403, (char*)"Register ID Not Matching With Local CMSID");

            memset(strErrorCode, 0, 32);
            snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_REG_ID_NOT_MATCH_ERROR);
            SIP_UASAnswerToRegister(reg_info_index, 403, strErrorCode);
            //SIP_UASAnswerToRegister(reg_info_index, 403, (char*)"Register Login IP Not Matching With Local IP");
            return -1;
        }
    }

#endif

#if 0

    if (iType < 500 && iType != EV9000_DEVICETYPE_SIPSERVER && !IsIDMatchLocalCMSID(register_id))
    {
        (void)SystemFaultAlarm(0, register_id, (char*)"6", (char*)"0x01070201", "�豸ע��ʧ��:�豸ID=%s, �豸IP��ַ=%s, ԭ��=%s, ����CMS ID=%s", register_id, login_ip, (char*)"�豸ע��ID�ͱ���CMS ID��ƥ��", local_cms_id_get());
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "�豸ע��ʧ��:�豸ID=%s, �豸IP��ַ=%s, �˿ں�=%d, ԭ��=%s, ����CMS ID=%s", register_id, login_ip, login_port, (char*)"�豸ע��ID�ͱ���CMS ID��ƥ��", local_cms_id_get());
        SIP_UASAnswerToRegister(reg_info_index, 403, (char*)"Register ID Not Matching With Local CMSID");
        return -1;
    }

    /* ���ע��ˢ��ʱ���Ƿ�������С���ʱ�� */
    if (expires > 0 && expires < MIN_REGISTER_EXPIRE)
    {
        SIP_UASAnswerToRegister4RegExpire(reg_info_index, MIN_REGISTER_EXPIRE);
        return -1;
    }

#endif

    if ((iType >= 300 && iType <= 399) /* �����û� */
        || (iType >= 400 && iType <= 499)) /* �ն��û� */
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "uas_register_received_proc() user_reg_msg_add:register_id=%s, login_ip=%s, login_port=%d, Type=%d  \r\n", register_id, login_ip, login_port, iType);
        //printf_system_time();

        i = user_reg_msg_add(register_id, login_ip, login_port, register_name, expires, reg_info_index);

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "uas_register_received_proc() user_reg_msg_add Error:register_id=%s, login_ip=%s, login_port=%d, Type=%d  \r\n", register_id, login_ip, login_port, iType);
        }

        return i;
    }
    else if ((iType >= 111 && iType <= 130) /* ǰ�����豸 */
             || (iType >= 131 && iType <= 199)) /* ǰ����Χ�豸 */
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "uas_register_received_proc() GBDevice_reg_msg_add:register_id=%s, login_ip=%s, login_port=%d, Type=%d  \r\n", register_id, login_ip, login_port, iType);
        //printf_system_time();

        i = GBDevice_reg_msg_add(register_id, iType, login_ip, login_port, register_name, expires, reg_info_index, link_type);

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "uas_register_received_proc() GBDevice_reg_msg_add Error:register_id=%s, login_ip=%s, login_port=%d, Type=%d  \r\n", register_id, login_ip, login_port, iType);
        }

        return i;
    }
    else if (iType >= 200 && iType <= 299) /* ƽ̨�豸:�¼�CMS  */
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "uas_register_received_proc() GBDevice_reg_msg_add:register_id=%s, login_ip=%s, login_port=%d, Type=%d \r\n", register_id, login_ip, login_port, iType);

        i = GBDevice_reg_msg_add(register_id, iType, login_ip, login_port, register_name, expires, reg_info_index, link_type);

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "uas_register_received_proc() GBDevice_reg_msg_add Error:register_id=%s, login_ip=%s, login_port=%d, Type=%d  \r\n", register_id, login_ip, login_port, iType);
        }

        return i;
    }
    else if (iType >= 500 && iType <= 999) /* ��չ�豸����*/
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "uas_register_received_proc() GBDevice_reg_msg_add:register_id=%s, login_ip=%s, login_port=%d, Type=%d  \r\n", register_id, login_ip, login_port, iType);
        //printf_system_time();

        i = GBDevice_reg_msg_add(register_id, iType, login_ip, login_port, register_name, expires, reg_info_index, link_type);

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "uas_register_received_proc() GBDevice_reg_msg_add Error:register_id=%s, login_ip=%s, login_port=%d, Type=%d  \r\n", register_id, login_ip, login_port, iType);
        }

        return i;
    }

    EnSystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "Device registration failed:Device ID:DeviceID=%s, Device IP address=%s, Cause=%s, Registration device type=%d", register_id, login_ip, (char*)"Registration device type is not supported", iType);
    SystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "�豸ע��ʧ��:�豸ID=%s, �豸IP��ַ=%s, ԭ��=%s, ע����豸����=%d", register_id, login_ip, (char*)"ע���豸���Ͳ�֧��", iType);
    memset(strErrorCode, 0, 32);
    snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_REG_DEVICE_TYPE_NOT_SUPPORT_ERROR);
    SIP_UASAnswerToRegister(reg_info_index, 403, strErrorCode);
    //SIP_UASAnswerToRegister(reg_info_index, 403, (char*)"Register ID Type Not Support");
    return -1;
}

/*****************************************************************************
 �� �� ��  : uas_register_received_timeout_proc
 ��������  : �����û���յ��ͻ���ˢ��ע����Ϣ��ʱ����
                            ���������Ҫ����
 �������  : char* proxy_id,������ID
             char* register_id
             char* login_ip
             int login_port
             int reg_info_index
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��17��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int uas_register_received_timeout_proc(char* proxy_id, char* register_id, char* login_ip, int login_port, int reg_info_index)
{
    int i = 0;
    char* pTmp = NULL;
    char strDeviceType[4] = {0};
    int iType = 0;

    if (0 == cms_run_status)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "uas_register_received_timeout_proc() exit---: CMS Not Run Normal Error \r\n");
        return -1;
    }

    if (NULL == register_id)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "uas_register_received_timeout_proc() exit---: Register ID NULL \r\n");
        return -1;
    }

    DEBUG_TRACE(MODULE_COMMON, LOG_INFO,  "uas_register_received_timeout_proc() \
    \r\n In Param: \
    \r\n proxy_id=%s \
    \r\n register_id=%s \
    \r\n login_ip=%s \
    \r\n login_port=%d \
    \r\n reg_info_index=%d \
    \r\n", proxy_id, register_id, login_ip, login_port, reg_info_index);

    /* ��ȡע��id�е��豸���ͣ��Ա������Ӧ����Ϣ���� */
    pTmp = &register_id[10];
    osip_strncpy(strDeviceType, pTmp, 3);
    iType = osip_atoi(strDeviceType);

    if ((iType >= 300 && iType <= 399) /* �����û� */
        || (iType >= 400 && iType <= 499)) /* �ն��û� */
    {
        i = user_reg_msg_add(register_id, login_ip, login_port, NULL, -1, reg_info_index);
    }
    else if ((iType >= 111 && iType <= 130) /* ǰ�����豸 */
             || (iType >= 131 && iType <= 199)) /* ǰ����Χ�豸 */
    {
        i = GBDevice_reg_msg_add(register_id, iType, login_ip, login_port, NULL, 0, reg_info_index, 0);
    }
    else if (iType >= 200 && iType <= 299) /* ƽ̨�豸:�¼�CMS  */
    {
        i = GBDevice_reg_msg_add(register_id, iType, login_ip, login_port, NULL, 0, reg_info_index, 0);
    }
    else if (iType >= 500 && iType <= 999) /* ��չ�豸����*/
    {
        i = GBDevice_reg_msg_add(register_id, iType, login_ip, login_port, NULL, 0, reg_info_index, 0);
    }
    else
    {
        EnSystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "Device registration time out message failed:DeviceID=%s, Device IP address=%s, Cause=%s, Registration device type=%d", register_id, login_ip, (char*)"Registration device type is not supported", iType);
        SystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "�豸ע�ᳬʱ��Ϣ����ʧ��:�豸ID=%s, �豸IP��ַ=%s, ԭ��=%s, ע����豸����=%d", register_id, login_ip, (char*)"ע���豸���Ͳ�֧��", iType);
        return -1;
    }

    return i;
}

/*****************************************************************************
 �� �� ��  : register_response_received_proc
 ��������  : �ͻ��˷���ע����Ϣ�յ�ע���Ӧ��Ϣ�Ĵ�����
 �������  : int reg_info_index
             int iExpires
             int status_code
             char* reasonphrase
             unsigned int iTime
             int user_data
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��10��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int uac_register_response_received_proc(int reg_info_index, int iExpires, int status_code, char* reasonphrase, unsigned int iTime, int user_data)
{
    int iRet = 0;
    int pos = -1;
    route_info_t* pRouteInfo = NULL;
    int route_tl_pos = 0;

    if (0 == cms_run_status)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "uac_register_response_received_proc() exit---: CMS Not Run Normal Error \r\n");
        return -1;
    }

    DEBUG_TRACE(MODULE_COMMON, LOG_INFO,  "uac_register_response_received_proc() \
    \r\n In Param: \
    \r\n reg_info_index=%d \
    \r\n status_code=%d \
    \r\n", reg_info_index, status_code);

    /* ����reg_info_index ����routeע����Ϣ */
    if (reg_info_index < 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "uac_register_response_received_proc() exit---: Register Info Index Error \r\n");
        return -1;
    }

    pos = route_info_find_by_reg_index(reg_info_index);

    if (pos < 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "uac_register_response_received_proc() exit---: Find Route Info Error:reg_info_index=%d \r\n", reg_info_index);
        return -1;
    }

    pRouteInfo = route_info_get(pos);

    if (NULL == pRouteInfo)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "uac_register_response_received_proc() exit---: Get Route Info Error:reg_info_index=%d \r\n", reg_info_index);
        return -1;
    }

    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "uac_register_response_received_proc() RouteInfo:server_id=%s, server_ip=%s, server_port=%d, link_type=%d, reg_status=%d \r\n", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, pRouteInfo->link_type, pRouteInfo->reg_status);

    if (200 == status_code)
    {
        if (iExpires > 0)
        {
            if (pRouteInfo->reg_status <= 0)
            {
                /* �����ϼ�ƽ̨ҵ�����߳� */
                route_tl_pos = route_srv_proc_thread_find(pRouteInfo);
                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "uac_register_response_received_proc() route_srv_proc_thread_find:route_tl_pos=%d \r\n", route_tl_pos);

                if (route_tl_pos < 0)
                {
                    //���䴦���߳�
                    iRet = route_srv_proc_thread_assign(pRouteInfo);
                    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "uac_register_response_received_proc() route_srv_proc_thread_assign:iRet=%d \r\n", iRet);

                    if (iRet != 0)
                    {
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "�����ϼ�ƽ̨ҵ�����߳�ʧ��:�ϼ�ƽ̨ID=%s, IP��ַ=%s, �˿ں�=%d", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                    }
                }
                else
                {
                    /* �ͷ�һ��֮ǰ��ҵ�����߳� */
                    iRet = route_srv_proc_thread_recycle(pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);

                    //���䴦���߳�
                    iRet = route_srv_proc_thread_assign(pRouteInfo);
                    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "uac_register_response_received_proc() route_srv_proc_thread_assign:iRet=%d \r\n", iRet);

                    if (iRet != 0)
                    {
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "�����ϼ�ƽ̨ҵ�����߳�ʧ��:�ϼ�ƽ̨ID=%s, IP��ַ=%s, �˿ں�=%d", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                    }
                }
            }

            pRouteInfo->reg_status = 1;
            pRouteInfo->reg_info_index = reg_info_index;
            pRouteInfo->expires = iExpires;
            pRouteInfo->min_expires = iExpires;

            if (pGblconf->ntp_server_ip[0] == '\0') /* ��NTP��ʱ��Ͳ����ϼ�Уʱ�� */
            {
                /* ����ϼ���IP��ͬһ̨�������棬�����ֻ���MMS���Ͳ�Уʱ�� */
                if (!IsLocalHost(pRouteInfo->server_ip))
                {
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Superior platform national standard registration return time system school: Superior CMS ID=%s, IP address=%s, port number=%d, time=%d", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, iTime);
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�ƽ̨����ע�᷵��ʱ��ϵͳУʱ:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, ʱ��=%d", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, iTime);
                    /* ����ϵͳʱ�� */
                    set_system_time(iTime);
                    system("hwclock -wu");

                    iRet = SendKeepAliveMessageToRouteCMS(pRouteInfo);

                    if (0 != iRet)
                    {
                        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "uac_register_response_received_proc() SendKeepAliveMessageToRouteCMS Error:server_ip=%s, server_port=%d, iRet=%d \r\n", pRouteInfo->server_ip, pRouteInfo->server_port, iRet);
                    }
                    else
                    {
                        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "uac_register_response_received_proc() SendKeepAliveMessageToRouteCMS OK:server_ip=%s, server_port=%d, iRet=%d \r\n", pRouteInfo->server_ip, pRouteInfo->server_port, iRet);
                    }
                }
            }
        }
        else if (iExpires == 0)
        {
            pRouteInfo->reg_status = 0;
            pRouteInfo->reg_info_index = -1;
            pRouteInfo->expires = 0;
            pRouteInfo->min_expires = 0;

            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Superior platform register remove return: Superior CMS ID=%s, IP address=%s, port number=%d", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�ƽ̨ע��ע�᷵��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);

            /* �Ƴ��ϼ������ĵ�λ��Ϣ */
            iRet = RemoveGBLogicDeviceLockInfoByRouteInfo(pRouteInfo);

            if (0 != iRet)
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "uac_register_response_received_proc() RemoveGBLogicDeviceLockInfoByRouteInfo Error:server_ip=%s, server_port=%d, iRet=%d \r\n", pRouteInfo->server_ip, pRouteInfo->server_port, iRet);
            }
            else
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "uac_register_response_received_proc() RemoveGBLogicDeviceLockInfoByRouteInfo OK:server_ip=%s, server_port=%d, iRet=%d \r\n", pRouteInfo->server_ip, pRouteInfo->server_port, iRet);
            }

            iRet = StopAllServiceTaskByCallerIPAndPort(pRouteInfo->server_ip, pRouteInfo->server_port);

            if (0 != iRet)
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "uac_register_response_received_proc() StopAllServiceTaskByCallerIPAndPort Error:server_ip=%s, server_port=%d, iRet=%d \r\n", pRouteInfo->server_ip, pRouteInfo->server_port, iRet);
            }
            else
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "uac_register_response_received_proc() StopAllServiceTaskByCallerIPAndPort OK:server_ip=%s, server_port=%d, iRet=%d \r\n", pRouteInfo->server_ip, pRouteInfo->server_port, iRet);
            }

            /* �������ϼ�CMS������ */
            iRet = StopAllServiceTaskByCalleeIPAndPort(pRouteInfo->server_ip, pRouteInfo->server_port);

            if (0 != iRet)
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "uac_register_response_received_proc() StopAllServiceTaskByCalleeIPAndPort Error:server_ip=%s, server_port=%d, iRet=%d \r\n", pRouteInfo->server_ip, pRouteInfo->server_port, iRet);
            }
            else
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "uac_register_response_received_proc() StopAllServiceTaskByCalleeIPAndPort OK:server_ip=%s, server_port=%d, iRet=%d \r\n", pRouteInfo->server_ip, pRouteInfo->server_port, iRet);
            }

            /* ����ҵ�����߳� */
            iRet = route_srv_proc_thread_recycle(pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);

            if (0 != iRet)
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "uac_register_response_received_proc() route_srv_proc_thread_recycle Error:server_id=%s, server_ip=%s, server_port=%d, iRet=%d \r\n", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, iRet);
            }
            else
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "uac_register_response_received_proc() route_srv_proc_thread_recycle OK:server_id=%s, server_ip=%s, server_port=%d, iRet=%d \r\n", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, iRet);
            }
        }
    }
    else if (status_code >= 300 && status_code < 400)
    {
        pRouteInfo->reg_status = 0;
        pRouteInfo->reg_info_index = -1;
        pRouteInfo->expires = 0;
        pRouteInfo->min_expires = 0;

        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Superior platform register failed: Superior CMS ID=%s, IP address=%s, port number=%d, status code=%d", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, status_code);
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "�ϼ�ƽ̨ע�᷵��ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d��������=%d", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, status_code);

        /* �Ƴ��ϼ������ĵ�λ��Ϣ */
        iRet = RemoveGBLogicDeviceLockInfoByRouteInfo(pRouteInfo);

        if (0 != iRet)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "uac_register_response_received_proc() RemoveGBLogicDeviceLockInfoByRouteInfo Error:server_ip=%s, server_port=%d, iRet=%d \r\n", pRouteInfo->server_ip, pRouteInfo->server_port, iRet);
        }
        else
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "uac_register_response_received_proc() RemoveGBLogicDeviceLockInfoByRouteInfo OK:server_ip=%s, server_port=%d, iRet=%d \r\n", pRouteInfo->server_ip, pRouteInfo->server_port, iRet);
        }

        iRet = StopAllServiceTaskByCallerIPAndPort(pRouteInfo->server_ip, pRouteInfo->server_port);

        if (0 != iRet)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "uac_register_response_received_proc() StopAllServiceTaskByCallerIPAndPort Error:server_ip=%s, server_port=%d, iRet=%d \r\n", pRouteInfo->server_ip, pRouteInfo->server_port, iRet);
        }
        else
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "uac_register_response_received_proc() StopAllServiceTaskByCallerIPAndPort OK:server_ip=%s, server_port=%d, iRet=%d \r\n", pRouteInfo->server_ip, pRouteInfo->server_port, iRet);
        }

        /* �������ϼ�CMS������ */
        iRet = StopAllServiceTaskByCalleeIPAndPort(pRouteInfo->server_ip, pRouteInfo->server_port);

        if (0 != iRet)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "uac_register_response_received_proc() StopAllServiceTaskByCalleeIPAndPort Error:server_ip=%s, server_port=%d, iRet=%d \r\n", pRouteInfo->server_ip, pRouteInfo->server_port, iRet);
        }
        else
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "uac_register_response_received_proc() StopAllServiceTaskByCalleeIPAndPort OK:server_ip=%s, server_port=%d, iRet=%d \r\n", pRouteInfo->server_ip, pRouteInfo->server_port, iRet);
        }

        /* ����ҵ�����߳� */
        iRet = route_srv_proc_thread_recycle(pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);

        if (0 != iRet)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "uac_register_response_received_proc() route_srv_proc_thread_recycle Error:server_id=%s, server_ip=%s, server_port=%d, iRet=%d \r\n", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, iRet);
        }
        else
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "uac_register_response_received_proc() route_srv_proc_thread_recycle OK:server_id=%s, server_ip=%s, server_port=%d, iRet=%d \r\n", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, iRet);
        }
    }
    else if (status_code >= 400 && status_code != 401 && status_code != 407 && status_code != 423)
    {
        pRouteInfo->reg_status = 0;
        pRouteInfo->reg_info_index = -1;
        pRouteInfo->expires = 0;
        pRouteInfo->min_expires = 0;

        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Superior platform register failed: Superior CMS ID=%s, IP address=%s, port number=%d, status code=%d", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, status_code);
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "�ϼ�ƽ̨ע�᷵��ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d��������=%d", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, status_code);

        /* �Ƴ��ϼ������ĵ�λ��Ϣ */
        iRet = RemoveGBLogicDeviceLockInfoByRouteInfo(pRouteInfo);

        if (0 != iRet)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "uac_register_response_received_proc() RemoveGBLogicDeviceLockInfoByRouteInfo Error:server_ip=%s, server_port=%d, iRet=%d \r\n", pRouteInfo->server_ip, pRouteInfo->server_port, iRet);
        }
        else
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "uac_register_response_received_proc() RemoveGBLogicDeviceLockInfoByRouteInfo OK:server_ip=%s, server_port=%d, iRet=%d \r\n", pRouteInfo->server_ip, pRouteInfo->server_port, iRet);
        }

        iRet = StopAllServiceTaskByCallerIPAndPort(pRouteInfo->server_ip, pRouteInfo->server_port);

        if (0 != iRet)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "uac_register_response_received_proc() StopAllServiceTaskByCallerIPAndPort Error:server_ip=%s, server_port=%d, iRet=%d \r\n", pRouteInfo->server_ip, pRouteInfo->server_port, iRet);
        }
        else
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "uac_register_response_received_proc() StopAllServiceTaskByCallerIPAndPort OK:server_ip=%s, server_port=%d, iRet=%d \r\n", pRouteInfo->server_ip, pRouteInfo->server_port, iRet);
        }

        /* �������ϼ�CMS������ */
        iRet = StopAllServiceTaskByCalleeIPAndPort(pRouteInfo->server_ip, pRouteInfo->server_port);

        if (0 != iRet)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "uac_register_response_received_proc() StopAllServiceTaskByCalleeIPAndPort Error:server_ip=%s, server_port=%d, iRet=%d \r\n", pRouteInfo->server_ip, pRouteInfo->server_port, iRet);
        }
        else
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "uac_register_response_received_proc() StopAllServiceTaskByCalleeIPAndPort OK:server_ip=%s, server_port=%d, iRet=%d \r\n", pRouteInfo->server_ip, pRouteInfo->server_port, iRet);
        }

        /* ����ҵ�����߳� */
        iRet = route_srv_proc_thread_recycle(pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);

        if (0 != iRet)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "uac_register_response_received_proc() route_srv_proc_thread_recycle Error:server_id=%s, server_ip=%s, server_port=%d, iRet=%d \r\n", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, iRet);
        }
        else
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "uac_register_response_received_proc() route_srv_proc_thread_recycle OK:server_id=%s, server_ip=%s, server_port=%d, iRet=%d \r\n", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, iRet);
        }
    }

    return 0;
}

/*****************************************************************************
 �� �� ��  : invite_received_proc
 ��������  : �յ�INVITE��Ϣ�Ĵ�����
 �������  : char* caller_id
             char* callee_id
             char* call_id
             int dialog_index
             char* msg_body
             int msg_len
             int user_data
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��10��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int invite_received_proc(char* caller_id, char* callee_id, char* call_id, int dialog_index, char* msg_body, int msg_len, int user_data)
{
    int i = 0;
    int pos = -1;
    user_srv_proc_tl_t* user_srv_thread = NULL;
    device_srv_proc_tl_t* device_srv_thread = NULL;
    route_srv_proc_tl_t* route_srv_thread = NULL;
    GBDevice_info_t* pGBDeviceInfo = NULL;
    route_info_t* pRouteInfo = NULL;
    char* caller_ip = NULL;
    int caller_port = 0;
    char strErrorCode[32] = {0};

    if (0 == cms_run_status)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "invite_received_proc() exit---: CMS Not Run Normal Error \r\n");
        return -1;
    }

    DEBUG_TRACE(MODULE_COMMON, LOG_INFO,  "invite_received_proc() \
    \r\n In Param: \
    \r\n call_id=%s \
    \r\n caller_id=%s \
    \r\n callee_id=%s \
    \r\n dialog_index=%d \
    \r\n", call_id, caller_id, callee_id, dialog_index);

    /* Ŀǰ��INVITE��Ϣ�����ڿͻ����Լ��ϼ�CMS */

    /* ��ȡ��Ϣ��Դ��ip�Ͷ˿ں� */
    caller_ip = SIP_GetDialogFromHost(dialog_index);

    if (NULL == caller_ip)
    {
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_INVITE_CALLER_IP_ERROR);
        SIP_AnswerToInvite(dialog_index, 503, strErrorCode);
        //SIP_AnswerToInvite(dialog_index, 503, (char*)"Get Caller IP Error");
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "invite_received_proc() exit---: Get Caller IP Error \r\n");
        return -1;
    }

    caller_port = SIP_GetDialogFromPort(dialog_index);

    if (caller_port <= 0)
    {
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_INVITE_CALLER_PORT_ERROR);
        SIP_AnswerToInvite(dialog_index, 503, strErrorCode);
        //SIP_AnswerToInvite(dialog_index, 503, (char*)"Get Caller Port Error");
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "invite_received_proc() exit---: Get Caller Port Error \r\n");
        return -1;
    }

    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "invite_received_proc() SIP_GetDialogFromHost:caller_ip=%s,caller_port=%d \r\n", caller_ip, caller_port);

    /* �����û������̶߳��� */
    user_srv_thread = get_user_srv_proc_thread(caller_id, caller_ip, caller_port);

    if (NULL != user_srv_thread)
    {
        if (NULL != user_srv_thread->pUserSrvMsgQueue)
        {
            /* ��ӵ��û�ҵ����Ϣ���� */
            i = user_srv_msg_add(user_srv_thread, MSG_INVITE, caller_id, callee_id, 0, NULL, dialog_index, msg_body, msg_len, -1);
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "invite_received_proc() user_srv_msg_add:i=%d \r\n", i);
            return 0;
        }
    }
    else
    {
#if 1
        /* �����豸�����̶߳��� */
        device_srv_thread = get_device_srv_proc_thread(caller_id, caller_ip, caller_port);

        if (NULL != device_srv_thread)
        {
            /* ��ӵ��豸�߳�ҵ����Ϣ���� */
            i = device_srv_msg_add_for_appoint(device_srv_thread, MSG_INVITE, caller_id, callee_id, 0, NULL, dialog_index, msg_body, msg_len, -1);
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "invite_received_proc() device_srv_msg_add_for_appoint:i=%d \r\n", i);
            return 0;
        }
        else
#endif
        {
            /* ��������ǰ���豸*/
            pGBDeviceInfo = GBDevice_info_find_by_id_ip_and_port(caller_id, caller_ip, caller_port);

            if (NULL != pGBDeviceInfo)
            {
                /* ��ӵ��豸ҵ����Ϣ���� */
                i = device_srv_msg_add(pGBDeviceInfo, MSG_INVITE, caller_id, callee_id, 0, NULL, dialog_index, msg_body, msg_len, -1);
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "invite_received_proc() device_srv_msg_add 1:i=%d \r\n", i);
                return 0;
            }
            else
            {
                pGBDeviceInfo = GBDevice_info_find_by_ip_and_port(caller_ip, caller_port);

                if (NULL != pGBDeviceInfo)
                {
                    /* ��ӵ��豸ҵ����Ϣ���� */
                    i = device_srv_msg_add(pGBDeviceInfo, MSG_INVITE, caller_id, callee_id, 0, NULL, dialog_index, msg_body, msg_len, -1);
                    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "invite_received_proc() device_srv_msg_add 2:i=%d \r\n", i);
                    return 0;
                }
                else
                {
                    /* ����������Կͻ��˺�ǰ���豸,����Ƿ������ϼ�CMS */
#if 1
                    /* �����ϼ�ƽ̨�����̶߳��� */
                    route_srv_thread = get_route_srv_proc_thread(caller_id, caller_ip, caller_port);

                    if (NULL != route_srv_thread)
                    {
                        /* ��ӵ��ϼ�ƽ̨�߳�ҵ����Ϣ���� */
                        i = route_srv_msg_add_for_appoint(route_srv_thread, MSG_INVITE, caller_id, callee_id, 0, NULL, dialog_index, msg_body, msg_len, -1);
                        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "invite_received_proc() route_srv_msg_add_for_appoint:i=%d \r\n", i);
                        return 0;
                    }
                    else
#endif
                    {
                        pos = route_info_find_by_host_and_port(caller_ip, caller_port);

                        if (pos >= 0)
                        {
                            pRouteInfo = route_info_get(pos);

                            if (NULL != pRouteInfo)
                            {
                                /* ��ӵ�����·��ҵ����Ϣ���� */
                                i = route_srv_msg_add(pRouteInfo, MSG_INVITE, caller_id, callee_id, 0, NULL, dialog_index, msg_body, msg_len, -1);
                                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "invite_received_proc() route_srv_msg_add:i=%d \r\n", i);
                                return 0;
                            }
                        }
                    }
                }
            }
        }
    }

    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Invite Message Proc failed:caller_id=%s, callee_id=%s, dialog_index=%d, Cause=%s", caller_id, callee_id, dialog_index, (char*)"No Matching Thread Found To Put");
    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Invite��Ϣ����ʧ��:caller_id=%s, caller_ip=%s, caller_port=%d, callee_id=%s, dialog_index=%d, ԭ��=%s", caller_id, caller_ip, caller_port, callee_id, dialog_index, (char*)"�Ҳ�����Ӧ�Ĵ����߳�");

    memset(strErrorCode, 0, 32);
    snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_NO_DEAL_THREAD_ERROR);
    SIP_AnswerToInvite(dialog_index, 503, strErrorCode);
    //SIP_AnswerToInvite(dialog_index, 503, NULL);
    DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "invite_received_proc() exit---: No Matching Thread Found To Put \r\n");
    return -1;
}

/*****************************************************************************
 �� �� ��  : invite_response_received_proc
 ��������  : �յ�INVITE��Ӧ��Ϣ�Ĵ�����
 �������  : char* caller_id
             char* callee_id
             char* call_id
             int dialog_index
             int status_code
             char* reasonphrase
             char* msg_body
             int msg_len
             int user_data
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��10��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int invite_response_received_proc(char* caller_id, char* callee_id, char* call_id, int dialog_index, int status_code, char* reasonphrase, char* msg_body, int msg_len, int user_data)
{
    int i = 0;
    int pos = -1;
    int cr_pos = -1;
    cr_t* pCrData = NULL;
    user_srv_proc_tl_t* user_srv_thread = NULL;
    device_srv_proc_tl_t* device_srv_thread = NULL;
    route_srv_proc_tl_t* route_srv_thread = NULL;
    GBDevice_info_t* pGBDeviceInfo = NULL;
    route_info_t* pRouteInfo = NULL;
    char* caller_ip = NULL;
    int caller_port = 0;

    if (0 == cms_run_status)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "invite_response_received_proc() exit---: CMS Not Run Normal Error \r\n");
        return -1;
    }

    DEBUG_TRACE(MODULE_COMMON, LOG_INFO,  "invite_response_received_proc() \
    \r\n In Param: \
    \r\n call_id=%s \
    \r\n caller_id=%s \
    \r\n callee_id=%s \
    \r\n dialog_index=%d \
    \r\n status_code=%d \
    \r\n reasonphrase=%s \
    \r\n", call_id, caller_id, callee_id, dialog_index, status_code, reasonphrase);

    /* INVITE��Ӧ��Ϣ���Ա��в� */

    /* 1������callee index ���Һ��м�¼��Ϣ */
    cr_pos = call_record_find_by_callee_index(dialog_index);
    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "invite_response_received_proc() call_record_find_by_callee_index:cr_pos=%d \r\n", cr_pos);

    if (cr_pos >= 0)
    {
        pCrData = call_record_get(cr_pos);

        if (NULL == pCrData)
        {
            if (200 == status_code)
            {
                i = SIP_SendBye(dialog_index);
                DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "invite_response_received_proc() SIP_SendBye:dialog_index=%d, i=%d \r\n", dialog_index, i);
            }

            i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
            i = call_record_remove(cr_pos);
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "invite_response_received_proc() exit---: Get Call Record Error:call_record_remove, cr_pos=%d \r\n", cr_pos);
            return -1;
        }

        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "invite_response_received_proc() pCrData:caller_ua_index=%d,callee_ua_index=%d,call_type=%d \r\n", pCrData->caller_ua_index, pCrData->callee_ua_index, pCrData->call_type);
        /* CMS���������DC���� */
        if (CALL_TYPE_DC == pCrData->call_type)
        {
            /* ��ӵ�DC ҵ����Ϣ���� */
            i = device_srv_msg_add(NULL, MSG_INVITE_RESPONSE, caller_id, callee_id, status_code, reasonphrase, dialog_index, msg_body, msg_len, cr_pos);
            DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "invite_response_received_proc() device_srv_msg_add:caller_id=%s, callee_id=%s, dialog_index=%d, cr_pos=%d, status_code=%d, i=%d \r\n", caller_id, callee_id, dialog_index, cr_pos, status_code, i);
            return 0;
        }
        else if (CALL_TYPE_RECORD == pCrData->call_type)
        {
            /* ��ӵ�¼��ҵ����Ϣ���� */
            i = record_srv_msg_add(MSG_INVITE_RESPONSE, caller_id, callee_id, status_code, reasonphrase, dialog_index, msg_body, msg_len, cr_pos);
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "¼��ҵ��, �յ�INVITE��Ӧ��Ϣ, ��ӵ�¼����Ϣ�������:�߼��豸ID=%s, IP��ַ=%s, callee_ua_index=%d, dialog_index=%d, cr_pos=%d", pCrData->callee_id, pCrData->callee_ip, pCrData->callee_ua_index, dialog_index, cr_pos);
            DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "invite_response_received_proc() record_srv_msg_add:caller_id=%s, callee_id=%s, dialog_index=%d, cr_pos=%d, status_code=%d, i=%d \r\n", caller_id, callee_id, dialog_index, cr_pos, status_code, i);
            return 0;
        }
        else if ((CALL_TYPE_REALTIME == pCrData->call_type)
                 || (CALL_TYPE_RECORD_PLAY == pCrData->call_type)
                 || (CALL_TYPE_DOWNLOAD == pCrData->call_type)
                 || (CALL_TYPE_AUDIO == pCrData->call_type))
        {
            /* ��ȡ��Ϣ��Դ��ip�Ͷ˿ں� */
            caller_ip = SIP_GetDialogFromHost(pCrData->caller_ua_index);

            if (NULL == caller_ip)
            {
                if (200 == status_code)
                {
                    i = SIP_SendBye(dialog_index);
                    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "invite_response_received_proc() SIP_SendBye:dialog_index=%d, i=%d \r\n", dialog_index, i);
                }

                i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
                i = call_record_remove(cr_pos);
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "invite_response_received_proc() exit---: Get Caller IP Error:call_record_remove, cr_pos=%d \r\n", cr_pos);
                return -1;
            }

            caller_port = SIP_GetDialogFromPort(pCrData->caller_ua_index);

            if (caller_port <= 0)
            {
                if (200 == status_code)
                {
                    i = SIP_SendBye(dialog_index);
                    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "invite_response_received_proc() SIP_SendBye:dialog_index=%d, i=%d \r\n", dialog_index, i);
                }

                i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
                i = call_record_remove(cr_pos);
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "invite_response_received_proc() exit---: Get Caller Port Error:call_record_remove, cr_pos=%d \r\n", cr_pos);
                return -1;
            }

            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "invite_response_received_proc() SIP_GetDialogFromHost:caller_ip=%s,caller_port=%d \r\n", caller_ip, caller_port);

            /* 1.1�������û������̶߳��� */
            user_srv_thread = get_user_srv_proc_thread(pCrData->caller_id, caller_ip, caller_port);

            if (NULL != user_srv_thread)
            {
                if (NULL != user_srv_thread->pUserSrvMsgQueue)
                {
                    /* ��ӵ��û�ҵ����Ϣ���� */
                    i = user_srv_msg_add(user_srv_thread, MSG_INVITE_RESPONSE, caller_id, callee_id, status_code, reasonphrase, dialog_index, msg_body, msg_len, cr_pos);
                    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "invite_response_received_proc() user_srv_msg_add:i=%d \r\n", i);
                    return 0;
                }
            }

            /* 1.2 ����Ƿ�����ǰ���豸*/
#if 1
            /* �����豸�����̶߳��� */
            device_srv_thread = get_device_srv_proc_thread(pCrData->caller_id, caller_ip, caller_port);

            if (NULL != device_srv_thread)
            {
                /* ��ӵ��豸�߳�ҵ����Ϣ���� */
                i = device_srv_msg_add_for_appoint(device_srv_thread, MSG_INVITE_RESPONSE, caller_id, callee_id, status_code, reasonphrase, dialog_index, msg_body, msg_len, cr_pos);
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "invite_response_received_proc() device_srv_msg_add_for_appoint:i=%d \r\n", i);
                return 0;
            }
            else
#endif
            {
                pGBDeviceInfo = GBDevice_info_find_by_id_ip_and_port(pCrData->caller_id, caller_ip, caller_port);

                if (NULL != pGBDeviceInfo)
                {
                    /* ��ӵ��豸ҵ����Ϣ���� */
                    i = device_srv_msg_add(pGBDeviceInfo, MSG_INVITE_RESPONSE, caller_id, callee_id, status_code, reasonphrase, dialog_index, msg_body, msg_len, cr_pos);
                    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "invite_response_received_proc() device_srv_msg_add 1:i=%d \r\n", i);
                    return 0;
                }
                else
                {
                    pGBDeviceInfo = GBDevice_info_find_by_ip_and_port(caller_ip, caller_port);

                    if (NULL != pGBDeviceInfo)
                    {
                        /* ��ӵ��豸ҵ����Ϣ���� */
                        i = device_srv_msg_add(pGBDeviceInfo, MSG_INVITE_RESPONSE, caller_id, callee_id, status_code, reasonphrase, dialog_index, msg_body, msg_len, cr_pos);
                        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "invite_response_received_proc() device_srv_msg_add 2:i=%d \r\n", i);
                        return 0;
                    }
                }
            }

            /* 1.3������Ƿ������ϼ�CMS */
#if 1
            /* �����ϼ�ƽ̨�����̶߳��� */
            route_srv_thread = get_route_srv_proc_thread(pCrData->caller_id, caller_ip, caller_port);

            if (NULL != route_srv_thread)
            {
                /* ��ӵ��ϼ�ƽ̨�߳�ҵ����Ϣ���� */
                i = route_srv_msg_add_for_appoint(route_srv_thread, MSG_INVITE_RESPONSE, caller_id, callee_id, status_code, reasonphrase, dialog_index, msg_body, msg_len, cr_pos);
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "invite_response_received_proc() route_srv_msg_add_for_appoint:i=%d \r\n", i);
                return 0;
            }
            else
#endif
            {
                pos = route_info_find_by_host_and_port(caller_ip, caller_port);
                //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "invite_response_received_proc() route_info_find_by_host_and_port:pos=%d \r\n", pos);

                if (pos >= 0)
                {
                    pRouteInfo = route_info_get(pos);

                    if (NULL != pRouteInfo)
                    {
                        /* ��ӵ�����·��ҵ����Ϣ���� */
                        i = route_srv_msg_add(pRouteInfo, MSG_INVITE_RESPONSE, caller_id, callee_id, status_code, reasonphrase, dialog_index, msg_body, msg_len, cr_pos);
                        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "invite_response_received_proc() route_srv_msg_add:i=%d \r\n", i);
                        return 0;
                    }
                }
            }
        }

        if (200 == status_code)
        {
            i = SIP_SendBye(dialog_index);
            DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "invite_response_received_proc() SIP_SendBye:dialog_index=%d, i=%d \r\n", dialog_index, i);
        }

        i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
        i = call_record_remove(cr_pos);
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "invite_response_received_proc() exit---: No Matching Thread Found To Put:call_record_remove, cr_pos=%d \r\n", cr_pos);
        return -1;
    }

    /* 2������caller index ���Һ��м�¼��Ϣ */
    cr_pos = call_record_find_by_caller_index(dialog_index);
    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "invite_response_received_proc() call_record_find_by_caller_index:cr_pos=%d \r\n", cr_pos);

    if (cr_pos >= 0)
    {
        pCrData = call_record_get(cr_pos);

        if (NULL == pCrData)
        {
            if (200 == status_code)
            {
                i = SIP_SendBye(dialog_index);
                DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "invite_response_received_proc() SIP_SendBye:dialog_index=%d, i=%d \r\n", dialog_index, i);
            }

            i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
            i = call_record_remove(cr_pos);
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "invite_response_received_proc() exit---: Get Call Record Error:call_record_remove, cr_pos=%d \r\n", cr_pos);
            return -1;
        }

        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "invite_response_received_proc() pCrData:caller_ua_index=%d,callee_ua_index=%d,call_type=%d \r\n", pCrData->caller_ua_index, pCrData->callee_ua_index, pCrData->call_type);
        /* CMS���������DC���� */
        if (CALL_TYPE_DC == pCrData->call_type)
        {
            /* ��ӵ�DC ҵ����Ϣ���� */
            i = device_srv_msg_add(NULL, MSG_INVITE_RESPONSE, caller_id, callee_id, status_code, reasonphrase, dialog_index, msg_body, msg_len, cr_pos);
            DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "invite_response_received_proc() device_srv_msg_add:caller_id=%s, callee_id=%s, dialog_index=%d, cr_pos=%d, status_code=%d, i=%d \r\n", caller_id, callee_id, dialog_index, cr_pos, status_code, i);
            return 0;
        }

        else if (CALL_TYPE_RECORD == pCrData->call_type)
        {
            /* ��ӵ�¼��ҵ����Ϣ���� */
            i = record_srv_msg_add(MSG_INVITE_RESPONSE, caller_id, callee_id, status_code, reasonphrase, dialog_index, msg_body, msg_len, cr_pos);
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "¼��ҵ��, �յ�INVITE��Ӧ��Ϣ, ��ӵ�¼����Ϣ�������:�߼��豸ID=%s, IP��ַ=%s, caller_ua_index=%d, dialog_index=%d, cr_pos=%d", pCrData->callee_id, pCrData->callee_ip, pCrData->caller_ua_index, dialog_index, cr_pos);
            DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "invite_response_received_proc() record_srv_msg_add:caller_id=%s, callee_id=%s, dialog_index=%d, cr_pos=%d, status_code=%d, i=%d \r\n", caller_id, callee_id, dialog_index, cr_pos, status_code, i);
            return 0;
        }
        else if ((CALL_TYPE_REALTIME == pCrData->call_type)
                 || (CALL_TYPE_RECORD_PLAY == pCrData->call_type)
                 || (CALL_TYPE_DOWNLOAD == pCrData->call_type)
                 || (CALL_TYPE_AUDIO == pCrData->call_type))
        {
            /* ��ȡ��Ϣ��Դ��ip�Ͷ˿ں� */
            caller_ip = SIP_GetDialogFromHost(pCrData->caller_ua_index);

            if (NULL == caller_ip)
            {
                if (200 == status_code)
                {
                    i = SIP_SendBye(dialog_index);
                    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "invite_response_received_proc() SIP_SendBye:dialog_index=%d, i=%d \r\n", dialog_index, i);
                }

                i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
                i = call_record_remove(cr_pos);
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "invite_response_received_proc() exit---: Get User IP Error \r\n");
                return -1;
            }

            caller_port = SIP_GetDialogFromPort(pCrData->caller_ua_index);

            if (caller_port <= 0)
            {
                if (200 == status_code)
                {
                    i = SIP_SendBye(dialog_index);
                    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "invite_response_received_proc() SIP_SendBye:dialog_index=%d, i=%d \r\n", dialog_index, i);
                }

                i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
                i = call_record_remove(cr_pos);
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "invite_response_received_proc() exit---: Get User Port Error:call_record_remove, cr_pos=%d \r\n", cr_pos);
                return -1;
            }

            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "invite_response_received_proc() SIP_GetDialogFromHost:caller_ip=%s,caller_port=%d \r\n", caller_ip, caller_port);

            /* 2.1�������û������̶߳��� */
            user_srv_thread = get_user_srv_proc_thread(pCrData->caller_id, caller_ip, caller_port);

            if (NULL != user_srv_thread)
            {
                if (NULL != user_srv_thread->pUserSrvMsgQueue)
                {
                    /* ��ӵ��û�ҵ����Ϣ���� */
                    i = user_srv_msg_add(user_srv_thread, MSG_INVITE_RESPONSE, caller_id, callee_id, status_code, reasonphrase, dialog_index, msg_body, msg_len, cr_pos);
                    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "invite_response_received_proc() user_srv_msg_add:i=%d \r\n", i);
                    return 0;
                }
            }

            /* 2.2 ����Ƿ�����ǰ���豸*/
#if 1
            /* �����豸�����̶߳��� */
            device_srv_thread = get_device_srv_proc_thread(pCrData->caller_id, caller_ip, caller_port);

            if (NULL != device_srv_thread)
            {
                /* ��ӵ��豸�߳�ҵ����Ϣ���� */
                i = device_srv_msg_add_for_appoint(device_srv_thread, MSG_INVITE_RESPONSE, caller_id, callee_id, status_code, reasonphrase, dialog_index, msg_body, msg_len, cr_pos);
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "invite_response_received_proc() device_srv_msg_add_for_appoint:i=%d \r\n", i);
                return 0;
            }
            else
#endif
            {
                pGBDeviceInfo = GBDevice_info_find_by_id_ip_and_port(pCrData->caller_id, caller_ip, caller_port);

                if (NULL != pGBDeviceInfo)
                {
                    /* ��ӵ��豸ҵ����Ϣ���� */
                    i = device_srv_msg_add(pGBDeviceInfo, MSG_INVITE_RESPONSE, caller_id, callee_id, status_code, reasonphrase, dialog_index, msg_body, msg_len, cr_pos);
                    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "invite_response_received_proc() device_srv_msg_add 1:i=%d \r\n", i);
                    return 0;
                }
                else
                {
                    pGBDeviceInfo = GBDevice_info_find_by_ip_and_port(caller_ip, caller_port);

                    if (NULL != pGBDeviceInfo)
                    {
                        /* ��ӵ��豸ҵ����Ϣ���� */
                        i = device_srv_msg_add(pGBDeviceInfo, MSG_INVITE_RESPONSE, caller_id, callee_id, status_code, reasonphrase, dialog_index, msg_body, msg_len, cr_pos);
                        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "invite_response_received_proc() device_srv_msg_add 2:i=%d \r\n", i);
                        return 0;
                    }
                }
            }

            /* 2.3������Ƿ������ϼ�CMS */
#if 1
            /* �����ϼ�ƽ̨�����̶߳��� */
            route_srv_thread = get_route_srv_proc_thread(pCrData->caller_id, caller_ip, caller_port);

            if (NULL != route_srv_thread)
            {
                /* ��ӵ��ϼ�ƽ̨�߳�ҵ����Ϣ���� */
                i = route_srv_msg_add_for_appoint(route_srv_thread, MSG_INVITE_RESPONSE, caller_id, callee_id, status_code, reasonphrase, dialog_index, msg_body, msg_len, cr_pos);
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "invite_response_received_proc() route_srv_msg_add_for_appoint:i=%d \r\n", i);
                return 0;
            }
            else
#endif
            {
                pos = route_info_find_by_host_and_port(caller_ip, caller_port);
                //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "invite_response_received_proc() route_info_find_by_host_and_port:pos=%d \r\n", pos);

                if (pos >= 0)
                {
                    pRouteInfo = route_info_get(pos);

                    if (NULL != pRouteInfo)
                    {
                        /* ��ӵ�����·��ҵ����Ϣ���� */
                        i = route_srv_msg_add(pRouteInfo, MSG_INVITE_RESPONSE, caller_id, callee_id, status_code, reasonphrase, dialog_index, msg_body, msg_len, cr_pos);
                        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "invite_response_received_proc() route_srv_msg_add:i=%d \r\n", i);
                        return 0;
                    }
                }
            }
        }

        if (200 == status_code)
        {
            i = SIP_SendBye(dialog_index);
            DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "invite_response_received_proc() SIP_SendBye:dialog_index=%d, i=%d \r\n", dialog_index, i);
        }

        i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
        i = call_record_remove(cr_pos);
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "invite_response_received_proc() exit---: No Matching Thread Found To Put:call_record_remove, cr_pos=%d \r\n", cr_pos);
        return -1;
    }

    if (200 == status_code)
    {
        i = SIP_SendBye(dialog_index);
        DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "invite_response_received_proc() SIP_SendBye:dialog_index=%d, i=%d \r\n", dialog_index, i);
    }

    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Invite Response Message Proc failed:caller_id=%s, callee_id=%s, dialog_index=%d, Cause=%s", caller_id, callee_id, dialog_index, (char*)"No Matching Thread Found To Put");
    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Invite��Ӧ��Ϣ����ʧ��:caller_id=%s, caller_ip=%s, caller_port=%d, callee_id=%s, dialog_index=%d, ԭ��=%s", caller_id, caller_ip, caller_port, callee_id, dialog_index, (char*)"�Ҳ�����Ӧ�Ĵ����߳�");

    DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "invite_response_received_proc() exit---: No Matching Thread Found To Put \r\n");
    return -1;
}

/*****************************************************************************
 �� �� ��  : cancel_received_proc
 ��������  : �յ�Cancel��Ϣ�Ĵ�����
 �������  : char* caller_id
             char* callee_id
             char* call_id
             int dialog_index
             int user_data
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��8��22�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int cancel_received_proc(char* caller_id, char* callee_id, char* call_id, int dialog_index, int user_data)
{
    int i = 0;
    int pos = -1;
    int cr_pos = -1;
    cr_t* pCrData = NULL;
    user_srv_proc_tl_t* user_srv_thread = NULL;
    device_srv_proc_tl_t* device_srv_thread = NULL;
    route_srv_proc_tl_t* route_srv_thread = NULL;
    GBDevice_info_t* pGBDeviceInfo = NULL;
    route_info_t* pRouteInfo = NULL;
    char* caller_ip = NULL;
    int caller_port = 0;

    if (0 == cms_run_status)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "cancel_received_proc() exit---: CMS Not Run Normal Error \r\n");
        return -1;
    }

    DEBUG_TRACE(MODULE_COMMON, LOG_INFO,  "cancel_received_proc() \
    \r\n In Param: \
    \r\n call_id=%s \
    \r\n caller_id=%s \
    \r\n callee_id=%s \
    \r\n dialog_index=%d \
    \r\n", call_id, caller_id, callee_id, dialog_index);

    /* Cancel ��Ϣ�������в� */

    /* 1������callee index ���Һ��м�¼��Ϣ */
    cr_pos = call_record_find_by_callee_index(dialog_index);
    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "cancel_received_proc() call_record_find_by_callee_index:cr_pos=%d \r\n", cr_pos);

    if (cr_pos >= 0)
    {
        pCrData = call_record_get(cr_pos);

        if (NULL == pCrData)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "cancel_received_proc() exit---: Get Call Record Error:call_record_remove, cr_pos=%d \r\n", cr_pos);
            return -1;
        }

        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "cancel_received_proc() pCrData:caller_ua_index=%d,callee_ua_index=%d,call_type=%d \r\n", pCrData->caller_ua_index, pCrData->callee_ua_index, pCrData->call_type);
        /* CMS���������DC���� */
        if (CALL_TYPE_DC == pCrData->call_type)
        {
            /* ��ӵ�DC ҵ����Ϣ���� */
            i = device_srv_msg_add(NULL, MSG_CANCEL, caller_id, callee_id, 0, NULL, dialog_index, NULL, 0, cr_pos);
            DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "cancel_received_proc() device_srv_msg_add:caller_id=%s, callee_id=%s, dialog_index=%d, cr_pos=%d, i=%d \r\n", caller_id, callee_id, dialog_index, cr_pos, i);
            return 0;
        }
        else if (CALL_TYPE_RECORD == pCrData->call_type)
        {
            /* ��ӵ�¼��ҵ����Ϣ���� */
            i = record_srv_msg_add(MSG_CANCEL, caller_id, callee_id, 0, NULL, dialog_index, NULL, 0, cr_pos);
            DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "cancel_received_proc() record_srv_msg_add:caller_id=%s, callee_id=%s, dialog_index=%d, cr_pos=%d, i=%d \r\n", caller_id, callee_id, dialog_index, cr_pos, i);
            return 0;
        }
        else if ((CALL_TYPE_REALTIME == pCrData->call_type)
                 || (CALL_TYPE_RECORD_PLAY == pCrData->call_type)
                 || (CALL_TYPE_DOWNLOAD == pCrData->call_type)
                 || (CALL_TYPE_AUDIO == pCrData->call_type))
        {
            /* ��ȡ��Ϣ��Դ��ip�Ͷ˿ں� */
            caller_ip = SIP_GetDialogFromHost(pCrData->caller_ua_index);

            if (NULL == caller_ip)
            {
                i = call_record_remove(cr_pos);
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "cancel_received_proc() exit---: Get User IP Error:call_record_remove, cr_pos=%d \r\n", cr_pos);
                return -1;
            }

            caller_port = SIP_GetDialogFromPort(pCrData->caller_ua_index);

            if (caller_port <= 0)
            {
                i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
                i = call_record_remove(cr_pos);
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "cancel_received_proc() exit---: Get User Port Error:call_record_remove, cr_pos=%d \r\n", cr_pos);
                return -1;
            }

            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "cancel_received_proc() SIP_GetDialogFromHost:caller_ip=%s,caller_port=%d \r\n", caller_ip, caller_port);

            /* 1.1�������û������̶߳��� */
            user_srv_thread = get_user_srv_proc_thread(pCrData->caller_id, caller_ip, caller_port);

            if (NULL != user_srv_thread)
            {
                if (NULL != user_srv_thread->pUserSrvMsgQueue)
                {
                    /* ��ӵ��û�ҵ����Ϣ���� */
                    i = user_srv_msg_add(user_srv_thread, MSG_CANCEL, caller_id, callee_id, 0, NULL, dialog_index, NULL, 0, cr_pos);
                    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "cancel_received_proc() user_srv_msg_add:i=%d \r\n", i);
                    return 0;
                }
            }

            /* 1.2 ����Ƿ�����ǰ���豸*/
#if 1
            /* �����豸�����̶߳��� */
            device_srv_thread = get_device_srv_proc_thread(pCrData->caller_id, caller_ip, caller_port);

            if (NULL != device_srv_thread)
            {
                /* ��ӵ��豸�߳�ҵ����Ϣ���� */
                i = device_srv_msg_add_for_appoint(device_srv_thread, MSG_CANCEL, caller_id, callee_id, 0, NULL, dialog_index, NULL, 0, cr_pos);
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "cancel_received_proc() device_srv_msg_add_for_appoint:i=%d \r\n", i);
                return 0;
            }
            else
#endif
            {
                pGBDeviceInfo = GBDevice_info_find_by_id_ip_and_port(pCrData->caller_id, caller_ip, caller_port);

                if (NULL != pGBDeviceInfo)
                {
                    /* ��ӵ��豸ҵ����Ϣ���� */
                    i = device_srv_msg_add(pGBDeviceInfo, MSG_CANCEL, caller_id, callee_id, 0, NULL, dialog_index, NULL, 0, cr_pos);
                    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "cancel_received_proc() device_srv_msg_add 1:i=%d \r\n", i);
                    return 0;
                }
                else
                {
                    pGBDeviceInfo = GBDevice_info_find_by_ip_and_port(caller_ip, caller_port);

                    if (NULL != pGBDeviceInfo)
                    {
                        /* ��ӵ��豸ҵ����Ϣ���� */
                        i = device_srv_msg_add(pGBDeviceInfo, MSG_CANCEL, caller_id, callee_id, 0, NULL, dialog_index, NULL, 0, cr_pos);
                        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "cancel_received_proc() device_srv_msg_add 2:i=%d \r\n", i);
                        return 0;
                    }
                }
            }

            /* 1.3������Ƿ������ϼ�CMS */
#if 1
            /* �����ϻ�ƽ̨�����̶߳��� */
            route_srv_thread = get_route_srv_proc_thread(pCrData->caller_id, caller_ip, caller_port);

            if (NULL != route_srv_thread)
            {
                /* ��ӵ��ϼ�ƽ̨�߳�ҵ����Ϣ���� */
                i = route_srv_msg_add_for_appoint(route_srv_thread, MSG_CANCEL, caller_id, callee_id, 0, NULL, dialog_index, NULL, 0, cr_pos);
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "cancel_received_proc() route_srv_msg_add_for_appoint:i=%d \r\n", i);
                return 0;
            }
            else
#endif
            {
                pos = route_info_find_by_host_and_port(caller_ip, caller_port);
                //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "cancel_received_proc() route_info_find_by_host_and_port:pos=%d \r\n", pos);

                if (pos >= 0)
                {
                    pRouteInfo = route_info_get(pos);

                    if (NULL != pRouteInfo)
                    {
                        /* ��ӵ�����·��ҵ����Ϣ���� */
                        i = route_srv_msg_add(pRouteInfo, MSG_CANCEL, caller_id, callee_id, 0, NULL, dialog_index, NULL, 0, cr_pos);
                        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "cancel_received_proc() route_srv_msg_add:i=%d \r\n", i);
                        return 0;
                    }
                }
            }
        }

        i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
        i = call_record_remove(cr_pos);
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "cancel_received_proc() exit---: No Matching Thread Found To Put:call_record_remove, cr_pos=%d \r\n", cr_pos);
        return -1;
    }

    /* 2������caller index ���Һ��м�¼��Ϣ */
    cr_pos = call_record_find_by_caller_index(dialog_index);
    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "cancel_received_proc() call_record_find_by_caller_index:cr_pos=%d \r\n", cr_pos);

    if (cr_pos >= 0)
    {
        pCrData = call_record_get(cr_pos);

        if (NULL == pCrData)
        {
            i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
            i = call_record_remove(cr_pos);
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "cancel_received_proc() exit---: Get Call Record Error:call_record_remove, cr_pos=%d \r\n", cr_pos);
            return -1;
        }

        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "cancel_received_proc() pCrData:caller_ua_index=%d,callee_ua_index=%d,call_type=%d \r\n", pCrData->caller_ua_index, pCrData->callee_ua_index, pCrData->call_type);
        /* CMS���������DC���� */
        if (CALL_TYPE_DC == pCrData->call_type)
        {
            /* ��ӵ�DC ҵ����Ϣ���� */
            i = device_srv_msg_add(NULL, MSG_CANCEL, caller_id, callee_id, 0, NULL, dialog_index, NULL, 0, cr_pos);
            DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "cancel_received_proc() device_srv_msg_add:caller_id=%s, callee_id=%s, dialog_index=%d, cr_pos=%d, i=%d \r\n", caller_id, callee_id, dialog_index, cr_pos, i);
            return 0;
        }
        else if (CALL_TYPE_RECORD == pCrData->call_type)
        {
            /* ��ӵ�¼��ҵ����Ϣ���� */
            i = record_srv_msg_add(MSG_CANCEL, caller_id, callee_id, 0, NULL, dialog_index, NULL, 0, cr_pos);
            DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "cancel_received_proc() record_srv_msg_add:caller_id=%s, callee_id=%s, dialog_index=%d, cr_pos=%d, i=%d \r\n", caller_id, callee_id, dialog_index, cr_pos, i);
            return 0;
        }
        else if ((CALL_TYPE_REALTIME == pCrData->call_type)
                 || (CALL_TYPE_RECORD_PLAY == pCrData->call_type)
                 || (CALL_TYPE_DOWNLOAD == pCrData->call_type)
                 || (CALL_TYPE_AUDIO == pCrData->call_type))
        {
            /* ��ȡ��Ϣ��Դ��ip�Ͷ˿ں� */
            caller_ip = SIP_GetDialogFromHost(pCrData->caller_ua_index);

            if (NULL == caller_ip)
            {
                i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
                i = call_record_remove(cr_pos);
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "cancel_received_proc() exit---: Get User IP Error:call_record_remove, cr_pos=%d \r\n", cr_pos);
                return -1;
            }

            caller_port = SIP_GetDialogFromPort(pCrData->caller_ua_index);

            if (caller_port <= 0)
            {
                i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
                i = call_record_remove(cr_pos);
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "cancel_received_proc() exit---: Get User Port Error:call_record_remove, cr_pos=%d \r\n", cr_pos);
                return -1;
            }

            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "cancel_received_proc() SIP_GetDialogFromHost:caller_ip=%s,caller_port=%d \r\n", caller_ip, caller_port);

            /* 2.1�������û������̶߳��� */
            user_srv_thread = get_user_srv_proc_thread(pCrData->caller_id, caller_ip, caller_port);

            if (NULL != user_srv_thread)
            {
                if (NULL != user_srv_thread->pUserSrvMsgQueue)
                {
                    /* ��ӵ��û�ҵ����Ϣ���� */
                    i = user_srv_msg_add(user_srv_thread, MSG_CANCEL, caller_id, callee_id, 0, NULL, dialog_index, NULL, 0, cr_pos);
                    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "cancel_received_proc() user_srv_msg_add:i=%d \r\n", i);
                    return 0;
                }
            }

            /* 2.2 ����Ƿ�����ǰ���豸*/
#if 1
            /* �����豸�����̶߳��� */
            device_srv_thread = get_device_srv_proc_thread(pCrData->caller_id, caller_ip, caller_port);

            if (NULL != device_srv_thread)
            {
                /* ��ӵ��豸�߳�ҵ����Ϣ���� */
                i = device_srv_msg_add_for_appoint(device_srv_thread, MSG_CANCEL, caller_id, callee_id, 0, NULL, dialog_index, NULL, 0, cr_pos);
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "cancel_received_proc() device_srv_msg_add_for_appoint:i=%d \r\n", i);
                return 0;
            }
            else
#endif
            {
                pGBDeviceInfo = GBDevice_info_find_by_id_ip_and_port(pCrData->caller_id, caller_ip, caller_port);

                if (NULL != pGBDeviceInfo)
                {
                    /* ��ӵ��豸ҵ����Ϣ���� */
                    i = device_srv_msg_add(pGBDeviceInfo, MSG_CANCEL, caller_id, callee_id, 0, NULL, dialog_index, NULL, 0, cr_pos);
                    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "cancel_received_proc() device_srv_msg_add 1:i=%d \r\n", i);
                    return 0;
                }
                else
                {
                    pGBDeviceInfo = GBDevice_info_find_by_ip_and_port(caller_ip, caller_port);

                    if (NULL != pGBDeviceInfo)
                    {
                        /* ��ӵ��豸ҵ����Ϣ���� */
                        i = device_srv_msg_add(pGBDeviceInfo, MSG_CANCEL, caller_id, callee_id, 0, NULL, dialog_index, NULL, 0, cr_pos);
                        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "cancel_received_proc() device_srv_msg_add 2:i=%d \r\n", i);
                        return 0;
                    }
                }
            }

            /* 2.3������Ƿ������ϼ�CMS */
#if 1
            /* �����ϻ�ƽ̨�����̶߳��� */
            route_srv_thread = get_route_srv_proc_thread(pCrData->caller_id, caller_ip, caller_port);

            if (NULL != route_srv_thread)
            {
                /* ��ӵ��ϼ�ƽ̨�߳�ҵ����Ϣ���� */
                i = route_srv_msg_add_for_appoint(route_srv_thread, MSG_CANCEL, caller_id, callee_id, 0, NULL, dialog_index, NULL, 0, cr_pos);
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "cancel_received_proc() route_srv_msg_add_for_appoint:i=%d \r\n", i);
                return 0;
            }
            else
#endif
            {
                pos = route_info_find_by_host_and_port(caller_ip, caller_port);
                //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "cancel_received_proc() route_info_find_by_host_and_port:pos=%d \r\n", pos);

                if (pos >= 0)
                {
                    pRouteInfo = route_info_get(pos);

                    if (NULL != pRouteInfo)
                    {
                        /* ��ӵ�����·��ҵ����Ϣ���� */
                        i = route_srv_msg_add(pRouteInfo, MSG_CANCEL, caller_id, callee_id, 0, NULL, dialog_index, NULL, 0, cr_pos);
                        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "cancel_received_proc() route_srv_msg_add:i=%d \r\n", i);
                        return 0;
                    }
                }
            }
        }

        i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
        i = call_record_remove(cr_pos);
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "cancel_received_proc() exit---: No Matching Thread Found To Put:call_record_remove, cr_pos=%d \r\n", cr_pos);
        return -1;
    }

    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Cancel Message Proc failed:caller_id=%s, callee_id=%s, dialog_index=%d, Cause=%s", caller_id, callee_id, dialog_index, (char*)"No Matching Thread Found To Put");
    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "ȡ����Ϣ����ʧ��:caller_id=%s, caller_ip=%s, caller_port=%d, callee_id=%s, dialog_index=%d, ԭ��=%s", caller_id, caller_ip, caller_port, callee_id, dialog_index, (char*)"�Ҳ�����Ӧ�Ĵ����߳�");

    DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "cancel_received_proc() exit---: No Matching Thread Found To Put \r\n");
    return -1;
}

/*****************************************************************************
 �� �� ��  : ack_received_proc
 ��������  : �յ�Ack��Ϣ�Ĵ�����
 �������  : char* caller_id
             char* caller_host,
             int dialog_index
             int user_data
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��19��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int ack_received_proc(char* caller_id, char* callee_id, char* call_id, int dialog_index, int user_data)
{
    return 0; /* Э��ջ�����Ѿ���������Ҫ�ϲ��ٴ��� */

    int i = 0;
    int pos = -1;
    int cr_pos = -1;
    cr_t* pCrData = NULL;
    user_srv_proc_tl_t* user_srv_thread = NULL;
    device_srv_proc_tl_t* device_srv_thread = NULL;
    route_srv_proc_tl_t* route_srv_thread = NULL;
    GBDevice_info_t* pGBDeviceInfo = NULL;
    route_info_t* pRouteInfo = NULL;
    char* caller_ip = NULL;
    int caller_port = 0;

    if (0 == cms_run_status)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "ack_received_proc() exit---: CMS Not Run Normal Error \r\n");
        return -1;
    }

    DEBUG_TRACE(MODULE_COMMON, LOG_INFO,  "ack_received_proc() \
    \r\n In Param: \
    \r\n call_id=%s \
    \r\n caller_id=%s \
    \r\n callee_id=%s \
    \r\n dialog_index=%d \
    \r\n", call_id, caller_id, callee_id, dialog_index);

    /* ACK��Ϣ�������в� */

    /* 1������callee index ���Һ��м�¼��Ϣ */
    cr_pos = call_record_find_by_callee_index(dialog_index);
    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "ack_received_proc() call_record_find_by_callee_index:cr_pos=%d \r\n", cr_pos);

    if (cr_pos >= 0)
    {
        pCrData = call_record_get(cr_pos);

        if (NULL == pCrData)
        {
            i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
            i = call_record_remove(cr_pos);
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "ack_received_proc() exit---: Get Call Record Error:call_record_remove, cr_pos=%d \r\n", cr_pos);
            return -1;
        }

        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "ack_received_proc() pCrData:caller_ua_index=%d,callee_ua_index=%d,call_type=%d \r\n", pCrData->caller_ua_index, pCrData->callee_ua_index, pCrData->call_type);
        /* CMS���������DC���� */
        if (CALL_TYPE_DC == pCrData->call_type)
        {
            /* ��ӵ�DC ҵ����Ϣ���� */
            i = device_srv_msg_add(NULL, MSG_ACK, caller_id, callee_id, 0, NULL, dialog_index, NULL, 0, cr_pos);
            DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "ack_received_proc() device_srv_msg_add:caller_id=%s, callee_id=%s, dialog_index=%d, cr_pos=%d, i=%d \r\n", caller_id, callee_id, dialog_index, cr_pos, i);
            return 0;
        }
        else if (CALL_TYPE_RECORD == pCrData->call_type)
        {
            /* ��ӵ�¼��ҵ����Ϣ���� */
            i = record_srv_msg_add(MSG_ACK, caller_id, callee_id, 0, NULL, dialog_index, NULL, 0, cr_pos);
            DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "ack_received_proc() record_srv_msg_add:caller_id=%s, callee_id=%s, dialog_index=%d, cr_pos=%d, i=%d \r\n", caller_id, callee_id, dialog_index, cr_pos, i);
            return 0;
        }
        else if ((CALL_TYPE_REALTIME == pCrData->call_type)
                 || (CALL_TYPE_RECORD_PLAY == pCrData->call_type)
                 || (CALL_TYPE_DOWNLOAD == pCrData->call_type)
                 || (CALL_TYPE_AUDIO == pCrData->call_type))
        {
            /* ��ȡ��Ϣ��Դ��ip�Ͷ˿ں� */
            caller_ip = SIP_GetDialogFromHost(pCrData->caller_ua_index);

            if (NULL == caller_ip)
            {
                i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
                i = call_record_remove(cr_pos);
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "ack_received_proc() exit---: Get User IP Error:call_record_remove, cr_pos=%d \r\n", cr_pos);
                return -1;
            }

            caller_port = SIP_GetDialogFromPort(pCrData->caller_ua_index);

            if (caller_port <= 0)
            {
                i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
                i = call_record_remove(cr_pos);
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "ack_received_proc() exit---: Get User Port Error:call_record_remove, cr_pos=%d \r\n", cr_pos);
                return -1;
            }

            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "ack_received_proc() SIP_GetDialogFromHost:caller_ip=%s,caller_port=%d \r\n", caller_ip, caller_port);

            /* 1.1�������û������̶߳��� */
            user_srv_thread = get_user_srv_proc_thread(pCrData->caller_id, caller_ip, caller_port);

            if (NULL != user_srv_thread)
            {
                if (NULL != user_srv_thread->pUserSrvMsgQueue)
                {
                    /* ��ӵ��û�ҵ����Ϣ���� */
                    i = user_srv_msg_add(user_srv_thread, MSG_ACK, caller_id, callee_id, 0, NULL, dialog_index, NULL, 0, cr_pos);
                    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "ack_received_proc() user_srv_msg_add:i=%d \r\n", i);
                    return 0;
                }
            }

            /* 1.2 ����Ƿ�����ǰ���豸*/
#if 1
            /* �����豸�����̶߳��� */
            device_srv_thread = get_device_srv_proc_thread(pCrData->caller_id, caller_ip, caller_port);

            if (NULL != device_srv_thread)
            {
                /* ��ӵ��豸�߳�ҵ����Ϣ���� */
                i = device_srv_msg_add_for_appoint(device_srv_thread, MSG_ACK, caller_id, callee_id, 0, NULL, dialog_index, NULL, 0, cr_pos);
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "ack_received_proc() device_srv_msg_add_for_appoint:i=%d \r\n", i);
                return 0;
            }
            else
#endif
            {
                pGBDeviceInfo = GBDevice_info_find_by_id_ip_and_port(pCrData->caller_id, caller_ip, caller_port);

                if (NULL != pGBDeviceInfo)
                {
                    /* ��ӵ��豸ҵ����Ϣ���� */
                    i = device_srv_msg_add(pGBDeviceInfo, MSG_ACK, caller_id, callee_id, 0, NULL, dialog_index, NULL, 0, cr_pos);
                    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "ack_received_proc() device_srv_msg_add 1:i=%d \r\n", i);
                    return 0;
                }
                else
                {
                    pGBDeviceInfo = GBDevice_info_find_by_ip_and_port(caller_ip, caller_port);

                    if (NULL != pGBDeviceInfo)
                    {
                        /* ��ӵ��豸ҵ����Ϣ���� */
                        i = device_srv_msg_add(pGBDeviceInfo, MSG_ACK, caller_id, callee_id, 0, NULL, dialog_index, NULL, 0, cr_pos);
                        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "ack_received_proc() device_srv_msg_add 2:i=%d \r\n", i);
                        return 0;
                    }
                }
            }

            /* 1.3������Ƿ������ϼ�CMS */
#if 1
            /* �����ϻ�ƽ̨�����̶߳��� */
            route_srv_thread = get_route_srv_proc_thread(pCrData->caller_id, caller_ip, caller_port);

            if (NULL != route_srv_thread)
            {
                /* ��ӵ��ϼ�ƽ̨�߳�ҵ����Ϣ���� */
                i = route_srv_msg_add_for_appoint(route_srv_thread, MSG_ACK, caller_id, callee_id, 0, NULL, dialog_index, NULL, 0, cr_pos);
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "ack_received_proc() route_srv_msg_add_for_appoint:i=%d \r\n", i);
                return 0;
            }
            else
#endif
            {
                pos = route_info_find_by_host_and_port(caller_ip, caller_port);
                //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "ack_received_proc() route_info_find_by_host_and_port:pos=%d \r\n", pos);

                if (pos >= 0)
                {
                    pRouteInfo = route_info_get(pos);

                    if (NULL != pRouteInfo)
                    {
                        /* ��ӵ�����·��ҵ����Ϣ���� */
                        i = route_srv_msg_add(pRouteInfo, MSG_ACK, caller_id, callee_id, 0, NULL, dialog_index, NULL, 0, cr_pos);
                        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "ack_received_proc() route_srv_msg_add:i=%d \r\n", i);
                        return 0;
                    }
                }
            }
        }

        i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
        i = call_record_remove(cr_pos);
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "ack_received_proc() exit---: No Matching Thread Found To Put:call_record_remove, cr_pos=%d \r\n", cr_pos);
        return -1;
    }

    /* 2������caller index ���Һ��м�¼��Ϣ */
    cr_pos = call_record_find_by_caller_index(dialog_index); /* ���з����Bye ��Ϣ */
    DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "ack_received_proc() call_record_find_by_caller_index:cr_pos=%d \r\n", cr_pos);

    if (cr_pos >= 0)
    {
        pCrData = call_record_get(cr_pos);

        if (NULL == pCrData)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "ack_received_proc() exit---: Get Call Record Error:call_record_remove, cr_pos=%d \r\n", cr_pos);
            return -1;
        }

        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "ack_received_proc() pCrData:caller_ua_index=%d,callee_ua_index=%d,call_type=%d \r\n", pCrData->caller_ua_index, pCrData->callee_ua_index, pCrData->call_type);
        /* CMS���������DC���� */
        if (CALL_TYPE_DC == pCrData->call_type)
        {
            /* ��ӵ�DC ҵ����Ϣ���� */
            i = device_srv_msg_add(NULL, MSG_ACK, caller_id, callee_id, 0, NULL, dialog_index, NULL, 0, cr_pos);
            DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "ack_received_proc() device_srv_msg_add:caller_id=%s, callee_id=%s, dialog_index=%d, cr_pos=%d, i=%d \r\n", caller_id, callee_id, dialog_index, cr_pos, i);
            return 0;
        }
        else if (CALL_TYPE_RECORD == pCrData->call_type)
        {
            /* ��ӵ�¼��ҵ����Ϣ���� */
            i = record_srv_msg_add(MSG_ACK, caller_id, callee_id, 0, NULL, dialog_index, NULL, 0, cr_pos);
            DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "ack_received_proc() record_srv_msg_add:caller_id=%s, callee_id=%s, dialog_index=%d, cr_pos=%d, i=%d \r\n", caller_id, callee_id, dialog_index, cr_pos, i);
            return 0;
        }
        else if ((CALL_TYPE_REALTIME == pCrData->call_type)
                 || (CALL_TYPE_RECORD_PLAY == pCrData->call_type)
                 || (CALL_TYPE_DOWNLOAD == pCrData->call_type)
                 || (CALL_TYPE_AUDIO == pCrData->call_type))
        {
            /* ��ȡ��Ϣ��Դ��ip�Ͷ˿ں� */
            caller_ip = SIP_GetDialogFromHost(pCrData->caller_ua_index);

            if (NULL == caller_ip)
            {
                i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
                i = call_record_remove(cr_pos);
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "ack_received_proc() exit---: Get User IP Error:call_record_remove, cr_pos=%d \r\n", cr_pos);
                return -1;
            }

            caller_port = SIP_GetDialogFromPort(pCrData->caller_ua_index);

            if (caller_port <= 0)
            {
                i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
                i = call_record_remove(cr_pos);
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "ack_received_proc() exit---: Get User Port Error:call_record_remove, cr_pos=%d \r\n", cr_pos);
                return -1;
            }

            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "ack_received_proc() SIP_GetDialogFromHost:caller_ip=%s,caller_port=%d \r\n", caller_ip, caller_port);

            /* 2.1�������û������̶߳��� */
            user_srv_thread = get_user_srv_proc_thread(pCrData->caller_id, caller_ip, caller_port);

            if (NULL != user_srv_thread)
            {
                if (NULL != user_srv_thread->pUserSrvMsgQueue)
                {
                    /* ��ӵ��û�ҵ����Ϣ���� */
                    i = user_srv_msg_add(user_srv_thread, MSG_ACK, caller_id, callee_id, 0, NULL, dialog_index, NULL, 0, cr_pos);
                    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "ack_received_proc() user_srv_msg_add:i=%d \r\n", i);
                    return 0;
                }
            }

            /* 2.2 ����Ƿ�����ǰ���豸*/
#if 1
            /* �����豸�����̶߳��� */
            device_srv_thread = get_device_srv_proc_thread(pCrData->caller_id, caller_ip, caller_port);

            if (NULL != device_srv_thread)
            {
                /* ��ӵ��豸�߳�ҵ����Ϣ���� */
                i = device_srv_msg_add_for_appoint(device_srv_thread, MSG_ACK, caller_id, callee_id, 0, NULL, dialog_index, NULL, 0, cr_pos);
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "ack_received_proc() device_srv_msg_add_for_appoint:i=%d \r\n", i);
                return 0;
            }
            else
#endif
            {
                pGBDeviceInfo = GBDevice_info_find_by_id_ip_and_port(pCrData->caller_id, caller_ip, caller_port);

                if (NULL != pGBDeviceInfo)
                {
                    /* ��ӵ��豸ҵ����Ϣ���� */
                    i = device_srv_msg_add(pGBDeviceInfo, MSG_ACK, caller_id, callee_id, 0, NULL, dialog_index, NULL, 0, cr_pos);
                    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "ack_received_proc() device_srv_msg_add 1:i=%d \r\n", i);
                    return 0;
                }
                else
                {
                    pGBDeviceInfo = GBDevice_info_find_by_ip_and_port(caller_ip, caller_port);

                    if (NULL != pGBDeviceInfo)
                    {
                        /* ��ӵ��豸ҵ����Ϣ���� */
                        i = device_srv_msg_add(pGBDeviceInfo, MSG_ACK, caller_id, callee_id, 0, NULL, dialog_index, NULL, 0, cr_pos);
                        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "ack_received_proc() device_srv_msg_add 2:i=%d \r\n", i);
                        return 0;
                    }
                }
            }

            /* 2.3������Ƿ������ϼ�CMS */
#if 1
            /* �����ϻ�ƽ̨�����̶߳��� */
            route_srv_thread = get_route_srv_proc_thread(pCrData->caller_id, caller_ip, caller_port);

            if (NULL != route_srv_thread)
            {
                /* ��ӵ��ϼ�ƽ̨�߳�ҵ����Ϣ���� */
                i = route_srv_msg_add_for_appoint(route_srv_thread, MSG_ACK, caller_id, callee_id, 0, NULL, dialog_index, NULL, 0, cr_pos);
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "ack_received_proc() route_srv_msg_add_for_appoint:i=%d \r\n", i);
                return 0;
            }
            else
#endif
            {
                pos = route_info_find_by_host_and_port(caller_ip, caller_port);
                //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "ack_received_proc() route_info_find_by_host_and_port:pos=%d \r\n", pos);

                if (pos >= 0)
                {
                    pRouteInfo = route_info_get(pos);

                    if (NULL != pRouteInfo)
                    {
                        /* ��ӵ�����·��ҵ����Ϣ���� */
                        i = route_srv_msg_add(pRouteInfo, MSG_ACK, caller_id, callee_id, 0, NULL, dialog_index, NULL, 0, cr_pos);
                        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "ack_received_proc() route_srv_msg_add:i=%d \r\n", i);
                        return 0;
                    }
                }
            }
        }

        i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
        i = call_record_remove(cr_pos);
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "ack_received_proc() exit---: No Matching Thread Found To Put:call_record_remove, cr_pos=%d \r\n", cr_pos);
        return -1;
    }

    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Ack Message Proc failed:caller_id=%s, callee_id=%s, dialog_index=%d, Cause=%s", caller_id, callee_id, dialog_index, (char*)"No Matching Thread Found To Put");
    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Ack��Ϣ����ʧ��:caller_id=%s, caller_ip=%s, caller_port=%d, callee_id=%s, dialog_index=%d, ԭ��=%s", caller_id, caller_ip, caller_port, callee_id, dialog_index, (char*)"�Ҳ�����Ӧ�Ĵ����߳�");

    DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "ack_received_proc() exit---: No Matching Thread Found To Put \r\n");
    return -1;
}

/*****************************************************************************
 �� �� ��  : bye_received_proc
 ��������  : �յ�Bye��Ϣ�Ĵ�����
 �������  : char* caller_id
             char* caller_host,
             int dialog_index
             int user_data
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��10��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int bye_received_proc(char* caller_id, char* callee_id, char* call_id, int dialog_index, int user_data)
{
    int i = 0;
    int pos = -1;
    int cr_pos = -1;
    cr_t* pCrData = NULL;
    user_srv_proc_tl_t* user_srv_thread = NULL;
    device_srv_proc_tl_t* device_srv_thread = NULL;
    route_srv_proc_tl_t* route_srv_thread = NULL;
    GBDevice_info_t* pGBDeviceInfo = NULL;
    route_info_t* pRouteInfo = NULL;
    char* caller_ip = NULL;
    int caller_port = 0;

    if (0 == cms_run_status)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "bye_received_proc() exit---: CMS Not Run Normal Error \r\n");
        return -1;
    }

    DEBUG_TRACE(MODULE_COMMON, LOG_INFO,  "bye_received_proc() \
    \r\n In Param: \
    \r\n call_id=%s \
    \r\n caller_id=%s \
    \r\n callee_id=%s \
    \r\n dialog_index=%d \
    \r\n", call_id, caller_id, callee_id, dialog_index);

    /* Bye��Ϣ���������з��͵�Ҳ�����Ǳ��з��͵� */

    /* 1������callee index ���Һ��м�¼��Ϣ */
    cr_pos = call_record_find_by_callee_index(dialog_index); /* ���з����Bye ��Ϣ */
    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "bye_received_proc() call_record_find_by_callee_index:cr_pos=%d \r\n", cr_pos);

    if (cr_pos >= 0)
    {
        pCrData = call_record_get(cr_pos);

        if (NULL == pCrData)
        {
            SIP_AnswerToBye(dialog_index, 503, (char*)"Get Call Record Error");
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "bye_received_proc() exit---: Get Call Record Error:call_record_remove, cr_pos=%d \r\n", cr_pos);
            return -1;
        }

        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "bye_received_proc() pCrData:caller_ua_index=%d,callee_ua_index=%d,call_type=%d \r\n", pCrData->caller_ua_index, pCrData->callee_ua_index, pCrData->call_type);
        /* CMS���������DC���� */
        if (CALL_TYPE_DC == pCrData->call_type)
        {
            /* ��ӵ�DC ҵ����Ϣ���� */
            i = device_srv_msg_add(NULL, MSG_BYE, caller_id, callee_id, 0, NULL, dialog_index, NULL, 0, cr_pos);
            DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "bye_received_proc() device_srv_msg_add:caller_id=%s, callee_id=%s, dialog_index=%d, cr_pos=%d, i=%d \r\n", caller_id, callee_id, dialog_index, cr_pos, i);
            return 0;
        }
        else if (CALL_TYPE_RECORD == pCrData->call_type)
        {
            /* ��ӵ�¼��ҵ����Ϣ���� */
            i = record_srv_msg_add(MSG_BYE, caller_id, callee_id, 0, NULL, dialog_index, NULL, 0, cr_pos);
            DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "bye_received_proc() record_srv_msg_add:caller_id=%s, callee_id=%s, dialog_index=%d, cr_pos=%d, i=%d \r\n", caller_id, callee_id, dialog_index, cr_pos, i);
            return 0;
        }
        else if ((CALL_TYPE_REALTIME == pCrData->call_type)
                 || (CALL_TYPE_RECORD_PLAY == pCrData->call_type)
                 || (CALL_TYPE_DOWNLOAD == pCrData->call_type)
                 || (CALL_TYPE_AUDIO == pCrData->call_type))
        {
            /* ��ȡ��Ϣ��Դ��ip�Ͷ˿ں� */
            caller_ip = SIP_GetDialogFromHost(pCrData->caller_ua_index);

            if (NULL == caller_ip)
            {
                SIP_AnswerToBye(dialog_index, 503, (char*)"Get User IP Error");
                i = call_record_remove(cr_pos);
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "bye_received_proc() exit---: Get User IP Error:call_record_remove, cr_pos=%d \r\n", cr_pos);
                return -1;
            }

            caller_port = SIP_GetDialogFromPort(pCrData->caller_ua_index);

            if (caller_port <= 0)
            {
                SIP_AnswerToBye(dialog_index, 503, (char*)"Get User Port Error");
                i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
                i = call_record_remove(cr_pos);
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "bye_received_proc() exit---: Get User Port Error:call_record_remove, cr_pos=%d \r\n", cr_pos);
                return -1;
            }

            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "bye_received_proc() SIP_GetDialogFromHost:caller_ip=%s,caller_port=%d \r\n", caller_ip, caller_port);

            /* 1.1�������û������̶߳��� */
            user_srv_thread = get_user_srv_proc_thread(pCrData->caller_id, caller_ip, caller_port);

            if (NULL != user_srv_thread)
            {
                if (NULL != user_srv_thread->pUserSrvMsgQueue)
                {
                    /* ��ӵ��û�ҵ����Ϣ���� */
                    i = user_srv_msg_add(user_srv_thread, MSG_BYE, caller_id, callee_id, 0, NULL, dialog_index, NULL, 0, cr_pos);
                    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "bye_received_proc() user_srv_msg_add:i=%d \r\n", i);
                    return 0;
                }
            }

            /* 1.2 ����Ƿ�����ǰ���豸*/
#if 1
            /* �����豸�����̶߳��� */
            device_srv_thread = get_device_srv_proc_thread(pCrData->caller_id, caller_ip, caller_port);

            if (NULL != device_srv_thread)
            {
                /* ��ӵ��豸�߳�ҵ����Ϣ���� */
                i = device_srv_msg_add_for_appoint(device_srv_thread, MSG_BYE, caller_id, callee_id, 0, NULL, dialog_index, NULL, 0, cr_pos);
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "bye_received_proc() device_srv_msg_add_for_appoint:i=%d \r\n", i);
                return 0;
            }
            else
#endif
            {
                pGBDeviceInfo = GBDevice_info_find_by_id_ip_and_port(pCrData->caller_id, caller_ip, caller_port);

                if (NULL != pGBDeviceInfo)
                {
                    /* ��ӵ��豸ҵ����Ϣ���� */
                    i = device_srv_msg_add(pGBDeviceInfo, MSG_BYE, caller_id, callee_id, 0, NULL, dialog_index, NULL, 0, cr_pos);
                    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "bye_received_proc() device_srv_msg_add 1:i=%d \r\n", i);
                    return 0;
                }
                else
                {
                    pGBDeviceInfo = GBDevice_info_find_by_ip_and_port(caller_ip, caller_port);

                    if (NULL != pGBDeviceInfo)
                    {
                        /* ��ӵ��豸ҵ����Ϣ���� */
                        i = device_srv_msg_add(pGBDeviceInfo, MSG_BYE, caller_id, callee_id, 0, NULL, dialog_index, NULL, 0, cr_pos);
                        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "bye_received_proc() device_srv_msg_add 2:i=%d \r\n", i);
                        return 0;
                    }
                }
            }

            /* 1.3������Ƿ������ϼ�CMS */
#if 1
            /* �����ϻ�ƽ̨�����̶߳��� */
            route_srv_thread = get_route_srv_proc_thread(pCrData->caller_id, caller_ip, caller_port);

            if (NULL != route_srv_thread)
            {
                /* ��ӵ��ϼ�ƽ̨�߳�ҵ����Ϣ���� */
                i = route_srv_msg_add_for_appoint(route_srv_thread, MSG_BYE, caller_id, callee_id, 0, NULL, dialog_index, NULL, 0, cr_pos);
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "bye_received_proc() route_srv_msg_add_for_appoint:i=%d \r\n", i);
                return 0;
            }
            else
#endif
            {
                pos = route_info_find_by_host_and_port(caller_ip, caller_port);
                //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "bye_received_proc() route_info_find_by_host_and_port:pos=%d \r\n", pos);

                if (pos >= 0)
                {
                    pRouteInfo = route_info_get(pos);

                    if (NULL != pRouteInfo)
                    {
                        /* ��ӵ�����·��ҵ����Ϣ���� */
                        i = route_srv_msg_add(pRouteInfo, MSG_BYE, caller_id, callee_id, 0, NULL, dialog_index, NULL, 0, cr_pos);
                        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "bye_received_proc() route_srv_msg_add:i=%d \r\n", i);
                        return 0;
                    }
                }
            }
        }

        SIP_AnswerToBye(dialog_index, 481, NULL);
        i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
        i = call_record_remove(cr_pos);
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "bye_received_proc() exit---: No Matching Thread Found To Put:call_record_remove, cr_pos=%d \r\n", cr_pos);
        return -1;
    }

    /* 2������caller index ���Һ��м�¼��Ϣ */
    cr_pos = call_record_find_by_caller_index(dialog_index); /* ���з����Bye ��Ϣ */
    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "bye_received_proc() call_record_find_by_caller_index:cr_pos=%d \r\n", cr_pos);

    if (cr_pos >= 0)
    {
        pCrData = call_record_get(cr_pos);

        if (NULL == pCrData)
        {
            SIP_AnswerToBye(dialog_index, 503, (char*)"Get Call Record Error");
            i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
            i = call_record_remove(cr_pos);
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "bye_received_proc() exit---: Get Call Record Error:call_record_remove, cr_pos=%d \r\n", cr_pos);
            return -1;
        }

        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "bye_received_proc() pCrData:caller_ua_index=%d,callee_ua_index=%d,call_type=%d \r\n", pCrData->caller_ua_index, pCrData->callee_ua_index, pCrData->call_type);
        /* CMS���������DC���� */
        if (CALL_TYPE_DC == pCrData->call_type)
        {
            /* ��ӵ�DC ҵ����Ϣ���� */
            i = device_srv_msg_add(NULL, MSG_BYE, caller_id, callee_id, 0, NULL, dialog_index, NULL, 0, cr_pos);
            DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "bye_received_proc() device_srv_msg_add:caller_id=%s, callee_id=%s, dialog_index=%d, cr_pos=%d, i=%d \r\n", caller_id, callee_id, dialog_index, cr_pos, i);
            return 0;
        }
        else if (CALL_TYPE_RECORD == pCrData->call_type)
        {
            /* ��ӵ�¼��ҵ����Ϣ���� */
            i = record_srv_msg_add(MSG_BYE, caller_id, callee_id, 0, NULL, dialog_index, NULL, 0, cr_pos);
            DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "bye_received_proc() record_srv_msg_add:caller_id=%s, callee_id=%s, dialog_index=%d, cr_pos=%d, i=%d \r\n", caller_id, callee_id, dialog_index, cr_pos, i);
            return 0;
        }
        else if ((CALL_TYPE_REALTIME == pCrData->call_type)
                 || (CALL_TYPE_RECORD_PLAY == pCrData->call_type)
                 || (CALL_TYPE_DOWNLOAD == pCrData->call_type)
                 || (CALL_TYPE_AUDIO == pCrData->call_type))
        {
            /* ��ȡ��Ϣ��Դ��ip�Ͷ˿ں� */
            caller_ip = SIP_GetDialogFromHost(pCrData->caller_ua_index);

            if (NULL == caller_ip)
            {
                SIP_AnswerToBye(dialog_index, 503, (char*)"Get User IP Error");
                i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
                i = call_record_remove(cr_pos);
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "bye_received_proc() exit---: Get User IP Error:call_record_remove, cr_pos=%d \r\n", cr_pos);
                return -1;
            }

            caller_port = SIP_GetDialogFromPort(pCrData->caller_ua_index);

            if (caller_port <= 0)
            {
                SIP_AnswerToBye(dialog_index, 503, (char*)"Get User Port Error");
                i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
                i = call_record_remove(cr_pos);
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "bye_received_proc() exit---: Get User Port Error:call_record_remove, cr_pos=%d \r\n", cr_pos);
                return -1;
            }

            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "bye_received_proc() SIP_GetDialogFromHost:caller_ip=%s,caller_port=%d \r\n", caller_ip, caller_port);

            /* 2.1�������û������̶߳��� */
            user_srv_thread = get_user_srv_proc_thread(pCrData->caller_id, caller_ip, caller_port);

            if (NULL != user_srv_thread)
            {
                if (NULL != user_srv_thread->pUserSrvMsgQueue)
                {
                    /* ��ӵ��û�ҵ����Ϣ���� */
                    i = user_srv_msg_add(user_srv_thread, MSG_BYE, caller_id, callee_id, 0, NULL, dialog_index, NULL, 0, cr_pos);
                    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "bye_received_proc() user_srv_msg_add:i=%d \r\n", i);
                    return 0;
                }
            }

            /* 2.2 ����Ƿ�����ǰ���豸*/
#if 1
            /* �����豸�����̶߳��� */
            device_srv_thread = get_device_srv_proc_thread(pCrData->caller_id, caller_ip, caller_port);

            if (NULL != device_srv_thread)
            {
                /* ��ӵ��豸�߳�ҵ����Ϣ���� */
                i = device_srv_msg_add_for_appoint(device_srv_thread, MSG_BYE, caller_id, callee_id, 0, NULL, dialog_index, NULL, 0, cr_pos);
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "bye_received_proc() device_srv_msg_add_for_appoint:i=%d \r\n", i);
                return 0;
            }
            else
#endif
            {
                pGBDeviceInfo = GBDevice_info_find_by_id_ip_and_port(pCrData->caller_id, caller_ip, caller_port);

                if (NULL != pGBDeviceInfo)
                {
                    /* ��ӵ��豸ҵ����Ϣ���� */
                    i = device_srv_msg_add(pGBDeviceInfo, MSG_BYE, caller_id, callee_id, 0, NULL, dialog_index, NULL, 0, cr_pos);
                    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "bye_received_proc() device_srv_msg_add 1:i=%d \r\n", i);
                    return 0;
                }
                else
                {
                    pGBDeviceInfo = GBDevice_info_find_by_ip_and_port(caller_ip, caller_port);

                    if (NULL != pGBDeviceInfo)
                    {
                        /* ��ӵ��豸ҵ����Ϣ���� */
                        i = device_srv_msg_add(pGBDeviceInfo, MSG_BYE, caller_id, callee_id, 0, NULL, dialog_index, NULL, 0, cr_pos);
                        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "bye_received_proc() device_srv_msg_add 2:i=%d \r\n", i);
                        return 0;
                    }
                }
            }

            /* 2.3������Ƿ������ϼ�CMS */
#if 1
            /* �����ϻ�ƽ̨�����̶߳��� */
            route_srv_thread = get_route_srv_proc_thread(pCrData->caller_id, caller_ip, caller_port);

            if (NULL != route_srv_thread)
            {
                /* ��ӵ��ϼ�ƽ̨�߳�ҵ����Ϣ���� */
                i = route_srv_msg_add_for_appoint(route_srv_thread, MSG_BYE, caller_id, callee_id, 0, NULL, dialog_index, NULL, 0, cr_pos);
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "bye_received_proc() route_srv_msg_add_for_appoint:i=%d \r\n", i);
                return 0;
            }
            else
#endif
            {
                pos = route_info_find_by_host_and_port(caller_ip, caller_port);
                //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "bye_received_proc() route_info_find_by_host_and_port:pos=%d \r\n", pos);

                if (pos >= 0)
                {
                    pRouteInfo = route_info_get(pos);

                    if (NULL != pRouteInfo)
                    {
                        /* ��ӵ�����·��ҵ����Ϣ���� */
                        i = route_srv_msg_add(pRouteInfo, MSG_BYE, caller_id, callee_id, 0, NULL, dialog_index, NULL, 0, cr_pos);
                        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "bye_received_proc() route_srv_msg_add:i=%d \r\n", i);
                        return 0;
                    }
                }
            }
        }

        SIP_AnswerToBye(dialog_index, 481, NULL);
        i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
        i = call_record_remove(cr_pos);
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "bye_received_proc() exit---: No Matching Thread Found To Put:call_record_remove, cr_pos=%d \r\n", cr_pos);
        return -1;
    }

    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Bye Message Proc failed:caller_id=%s, callee_id=%s, dialog_index=%d, Cause=%s", caller_id, callee_id, dialog_index, (char*)"No Matching Thread Found To Put");
    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Bye��Ϣ����ʧ��:caller_id=%s, caller_ip=%s, caller_port=%d, callee_id=%s, dialog_index=%d, ԭ��=%s", caller_id, caller_ip, caller_port, callee_id, dialog_index, (char*)"�Ҳ�����Ӧ�Ĵ����߳�");

    SIP_AnswerToBye(dialog_index, 481, NULL);
    DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "bye_received_proc() exit---: No Matching Thread Found To Put \r\n");
    return -1;
}

/*****************************************************************************
 �� �� ��  : bye_response_received_proc
 ��������  : �յ�Bye��Ӧ��Ϣ�Ĵ�����
 �������  : char* caller_id
             char* caller_host
             int dialog_index
             int status_code
             int user_data
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��10��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int bye_response_received_proc(char* caller_id, char* callee_id, char* call_id, int dialog_index, int status_code, int user_data)
{
    int i = 0;
    int pos = -1;
    int cr_pos = -1;
    cr_t* pCrData = NULL;
    user_srv_proc_tl_t* user_srv_thread = NULL;
    device_srv_proc_tl_t* device_srv_thread = NULL;
    route_srv_proc_tl_t* route_srv_thread = NULL;
    GBDevice_info_t* pGBDeviceInfo = NULL;
    route_info_t* pRouteInfo = NULL;
    char* caller_ip = NULL;
    int caller_port = 0;

    if (0 == cms_run_status)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "bye_response_received_proc() exit---: CMS Not Run Normal Error \r\n");
        return -1;
    }

    DEBUG_TRACE(MODULE_COMMON, LOG_INFO,  "bye_response_received_proc() \
    \r\n In Param: \
    \r\n call_id=%s \
    \r\n caller_id=%s \
    \r\n callee_id=%s \
    \r\n dialog_index=%d \
    \r\n status_code=%d \
    \r\n", call_id, caller_id, callee_id, dialog_index, status_code);

    /* Bye��Ӧ��Ϣ���������з��͵�Ҳ�����Ǳ��з��͵� */

    /* 1������callee index ���Һ��м�¼��Ϣ */
    cr_pos = call_record_find_by_callee_index(dialog_index); /* ���з����Bye ��Ϣ */
    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "bye_response_received_proc() call_record_find_by_callee_index:cr_pos=%d \r\n", cr_pos);

    if (cr_pos >= 0)
    {
        pCrData = call_record_get(cr_pos);

        if (NULL == pCrData)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "bye_response_received_proc() exit---: Get Call Record Error:call_record_remove, cr_pos=%d \r\n", cr_pos);
            return -1;
        }

        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "bye_response_received_proc() pCrData:caller_ua_index=%d,callee_ua_index=%d,call_type=%d \r\n", pCrData->caller_ua_index, pCrData->callee_ua_index, pCrData->call_type);
        /* CMS���������DC���� */
        if (CALL_TYPE_DC == pCrData->call_type)
        {
            /* ��ӵ�DC ҵ����Ϣ���� */
            i = device_srv_msg_add(NULL, MSG_BYE_RESPONSE, caller_id, callee_id, status_code, NULL, dialog_index, NULL, 0, cr_pos);
            DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "bye_response_received_proc() device_srv_msg_add:caller_id=%s, callee_id=%s, dialog_index=%d, status_code=%d, cr_pos=%d, i=%d \r\n", caller_id, callee_id, dialog_index, status_code, cr_pos, i);
            return 0;
        }
        else if (CALL_TYPE_RECORD == pCrData->call_type)
        {
            /* ��ӵ�¼��ҵ����Ϣ���� */
            i = record_srv_msg_add(MSG_BYE_RESPONSE, caller_id, callee_id, status_code, NULL, dialog_index, NULL, 0, cr_pos);
            DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "bye_response_received_proc() record_srv_msg_add:caller_id=%s, callee_id=%s, dialog_index=%d, status_code=%d, cr_pos=%d, i=%d \r\n", caller_id, callee_id, dialog_index, status_code, cr_pos, i);
            return 0;
        }
        else if ((CALL_TYPE_REALTIME == pCrData->call_type)
                 || (CALL_TYPE_RECORD_PLAY == pCrData->call_type)
                 || (CALL_TYPE_DOWNLOAD == pCrData->call_type)
                 || (CALL_TYPE_AUDIO == pCrData->call_type))
        {
            /* ��ȡ��Ϣ��Դ��ip�Ͷ˿ں� */
            caller_ip = SIP_GetDialogFromHost(pCrData->caller_ua_index);

            if (NULL == caller_ip)
            {
                i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
                i = call_record_remove(cr_pos);
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "bye_response_received_proc() exit---: Get User IP Error:call_record_remove, cr_pos=%d \r\n", cr_pos);
                return -1;
            }

            caller_port = SIP_GetDialogFromPort(pCrData->caller_ua_index);

            if (caller_port <= 0)
            {
                i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
                i = call_record_remove(cr_pos);
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "bye_response_received_proc() exit---: Get User Port Error:call_record_remove, cr_pos=%d \r\n", cr_pos);
                return -1;
            }

            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "bye_response_received_proc() SIP_GetDialogFromHost:caller_ip=%s,caller_port=%d \r\n", caller_ip, caller_port);

            /* 1.1�������û������̶߳��� */
            user_srv_thread = get_user_srv_proc_thread(pCrData->caller_id, caller_ip, caller_port);

            if (NULL != user_srv_thread)
            {
                if (NULL != user_srv_thread->pUserSrvMsgQueue)
                {
                    /* ��ӵ��û�ҵ����Ϣ���� */
                    i = user_srv_msg_add(user_srv_thread, MSG_BYE_RESPONSE, caller_id, callee_id, status_code, NULL, dialog_index, NULL, 0, cr_pos);
                    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "bye_response_received_proc() user_srv_msg_add:i=%d \r\n", i);
                    return 0;
                }
            }

            /* 1.2 ����Ƿ�����ǰ���豸*/
#if 1
            /* �����豸�����̶߳��� */
            device_srv_thread = get_device_srv_proc_thread(pCrData->caller_id, caller_ip, caller_port);

            if (NULL != device_srv_thread)
            {
                /* ��ӵ��豸�߳�ҵ����Ϣ���� */
                i = device_srv_msg_add_for_appoint(device_srv_thread, MSG_BYE_RESPONSE, caller_id, callee_id, status_code, NULL, dialog_index, NULL, 0, cr_pos);
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "bye_response_received_proc() device_srv_msg_add_for_appoint:i=%d \r\n", i);
                return 0;
            }
            else
#endif
            {
                pGBDeviceInfo = GBDevice_info_find_by_id_ip_and_port(pCrData->caller_id, caller_ip, caller_port);

                if (NULL != pGBDeviceInfo)
                {
                    /* ��ӵ��豸ҵ����Ϣ���� */
                    i = device_srv_msg_add(pGBDeviceInfo, MSG_BYE_RESPONSE, caller_id, callee_id, status_code, NULL, dialog_index, NULL, 0, cr_pos);
                    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "bye_response_received_proc() device_srv_msg_add 1:i=%d \r\n", i);
                    return 0;
                }
                else
                {
                    pGBDeviceInfo = GBDevice_info_find_by_ip_and_port(caller_ip, caller_port);

                    if (NULL != pGBDeviceInfo)
                    {
                        /* ��ӵ��豸ҵ����Ϣ���� */
                        i = device_srv_msg_add(pGBDeviceInfo, MSG_BYE_RESPONSE, caller_id, callee_id, status_code, NULL, dialog_index, NULL, 0, cr_pos);
                        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "bye_response_received_proc() device_srv_msg_add 2:i=%d \r\n", i);
                        return 0;
                    }
                }
            }

            /* 1.3������Ƿ������ϼ�CMS */
#if 1
            /* �����ϻ�ƽ̨�����̶߳��� */
            route_srv_thread = get_route_srv_proc_thread(pCrData->caller_id, caller_ip, caller_port);

            if (NULL != route_srv_thread)
            {
                /* ��ӵ��ϼ�ƽ̨�߳�ҵ����Ϣ���� */
                i = route_srv_msg_add_for_appoint(route_srv_thread, MSG_BYE_RESPONSE, caller_id, callee_id, status_code, NULL, dialog_index, NULL, 0, cr_pos);
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "bye_response_received_proc() route_srv_msg_add_for_appoint:i=%d \r\n", i);
                return 0;
            }
            else
#endif
            {
                pos = route_info_find_by_host_and_port(caller_ip, caller_port);
                //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "bye_response_received_proc() route_info_find_by_host_and_port:pos=%d \r\n", pos);

                if (pos >= 0)
                {
                    pRouteInfo = route_info_get(pos);

                    if (NULL != pRouteInfo)
                    {
                        /* ��ӵ�����·��ҵ����Ϣ���� */
                        i = route_srv_msg_add(pRouteInfo, MSG_BYE_RESPONSE, caller_id, callee_id, status_code, NULL, dialog_index, NULL, 0, cr_pos);
                        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "bye_response_received_proc() route_srv_msg_add:i=%d \r\n", i);
                        return 0;
                    }
                }
            }
        }

        i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
        i = call_record_remove(cr_pos);
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "bye_response_received_proc() exit---: No Matching Thread Found To Put:call_record_remove, cr_pos=%d \r\n", cr_pos);
        return -1;
    }

    /* 2������caller index ���Һ��м�¼��Ϣ */
    cr_pos = call_record_find_by_caller_index(dialog_index); /* ���з����Bye ��Ϣ */
    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "bye_response_received_proc() call_record_find_by_caller_index:cr_pos=%d \r\n", cr_pos);

    if (cr_pos >= 0)
    {
        pCrData = call_record_get(cr_pos);

        if (NULL == pCrData)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "bye_response_received_proc() exit---: Get Call Record Error:call_record_remove, cr_pos=%d \r\n", cr_pos);
            return -1;
        }

        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "bye_response_received_proc() pCrData:caller_ua_index=%d,callee_ua_index=%d,call_type=%d \r\n", pCrData->caller_ua_index, pCrData->callee_ua_index, pCrData->call_type);
        /* CMS���������DC���� */
        if (CALL_TYPE_DC == pCrData->call_type)
        {
            /* ��ӵ�DC ҵ����Ϣ���� */
            i = device_srv_msg_add(NULL, MSG_BYE_RESPONSE, caller_id, callee_id, status_code, NULL, dialog_index, NULL, 0, cr_pos);
            DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "bye_response_received_proc() device_srv_msg_add:caller_id=%s, callee_id=%s, dialog_index=%d, status_code=%d, cr_pos=%d, i=%d \r\n", caller_id, callee_id, dialog_index, status_code, cr_pos, i);
            return 0;
        }
        else if (CALL_TYPE_RECORD == pCrData->call_type)
        {
            /* ��ӵ�¼��ҵ����Ϣ���� */
            i = record_srv_msg_add(MSG_BYE_RESPONSE, caller_id, callee_id, status_code, NULL, dialog_index, NULL, 0, cr_pos);
            DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "bye_response_received_proc() record_srv_msg_add:caller_id=%s, callee_id=%s, dialog_index=%d, status_code=%d, cr_pos=%d, i=%d \r\n", caller_id, callee_id, dialog_index, status_code, cr_pos, i);
            return 0;
        }
        else if ((CALL_TYPE_REALTIME == pCrData->call_type)
                 || (CALL_TYPE_RECORD_PLAY == pCrData->call_type)
                 || (CALL_TYPE_DOWNLOAD == pCrData->call_type)
                 || (CALL_TYPE_AUDIO == pCrData->call_type))
        {
            /* ��ȡ��Ϣ��Դ��ip�Ͷ˿ں� */
            caller_ip = SIP_GetDialogFromHost(pCrData->caller_ua_index);

            if (NULL == caller_ip)
            {
                i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
                i = call_record_remove(cr_pos);
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "bye_response_received_proc() exit---: Get User IP Error:call_record_remove, cr_pos=%d \r\n", cr_pos);
                return -1;
            }

            caller_port = SIP_GetDialogFromPort(pCrData->caller_ua_index);

            if (caller_port <= 0)
            {
                i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
                i = call_record_remove(cr_pos);
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "bye_response_received_proc() exit---: Get User Port Error:call_record_remove, cr_pos=%d \r\n", cr_pos);
                return -1;
            }

            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "bye_response_received_proc() SIP_GetDialogFromHost:caller_ip=%s,caller_port=%d \r\n", caller_ip, caller_port);

            /* 2.1�������û������̶߳��� */
            user_srv_thread = get_user_srv_proc_thread(pCrData->caller_id, caller_ip, caller_port);

            if (NULL != user_srv_thread)
            {
                if (NULL != user_srv_thread->pUserSrvMsgQueue)
                {
                    /* ��ӵ��û�ҵ����Ϣ���� */
                    i = user_srv_msg_add(user_srv_thread, MSG_BYE_RESPONSE, caller_id, callee_id, status_code, NULL, dialog_index, NULL, 0, cr_pos);
                    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "bye_response_received_proc() user_srv_msg_add:i=%d \r\n", i);
                    return 0;
                }
            }

            /* 2.2 ����Ƿ�����ǰ���豸*/
#if 1
            /* �����豸�����̶߳��� */
            device_srv_thread = get_device_srv_proc_thread(pCrData->caller_id, caller_ip, caller_port);

            if (NULL != device_srv_thread)
            {
                /* ��ӵ��豸�߳�ҵ����Ϣ���� */
                i = device_srv_msg_add_for_appoint(device_srv_thread, MSG_BYE_RESPONSE, caller_id, callee_id, status_code, NULL, dialog_index, NULL, 0, cr_pos);
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "bye_response_received_proc() device_srv_msg_add_for_appoint:i=%d \r\n", i);
                return 0;
            }
            else
#endif
            {
                pGBDeviceInfo = GBDevice_info_find_by_id_ip_and_port(pCrData->caller_id, caller_ip, caller_port);

                if (NULL != pGBDeviceInfo)
                {
                    /* ��ӵ��豸ҵ����Ϣ���� */
                    i = device_srv_msg_add(pGBDeviceInfo, MSG_BYE_RESPONSE, caller_id, callee_id, status_code, NULL, dialog_index, NULL, 0, cr_pos);
                    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "bye_response_received_proc() device_srv_msg_add 1:i=%d \r\n", i);
                    return 0;
                }
                else
                {
                    pGBDeviceInfo = GBDevice_info_find_by_ip_and_port(caller_ip, caller_port);

                    if (NULL != pGBDeviceInfo)
                    {
                        /* ��ӵ��豸ҵ����Ϣ���� */
                        i = device_srv_msg_add(pGBDeviceInfo, MSG_BYE_RESPONSE, caller_id, callee_id, status_code, NULL, dialog_index, NULL, 0, cr_pos);
                        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "bye_response_received_proc() device_srv_msg_add 2:i=%d \r\n", i);
                        return 0;
                    }
                }
            }

            /* 2.3������Ƿ������ϼ�CMS */
#if 1
            /* �����ϻ�ƽ̨�����̶߳��� */
            route_srv_thread = get_route_srv_proc_thread(pCrData->caller_id, caller_ip, caller_port);

            if (NULL != route_srv_thread)
            {
                /* ��ӵ��ϼ�ƽ̨�߳�ҵ����Ϣ���� */
                i = route_srv_msg_add_for_appoint(route_srv_thread, MSG_BYE_RESPONSE, caller_id, callee_id, status_code, NULL, dialog_index, NULL, 0, cr_pos);
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "bye_response_received_proc() route_srv_msg_add_for_appoint:i=%d \r\n", i);
                return 0;
            }
            else
#endif
            {
                pos = route_info_find_by_host_and_port(caller_ip, caller_port);
                //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "bye_response_received_proc() route_info_find_by_host_and_port:pos=%d \r\n", pos);

                if (pos >= 0)
                {
                    pRouteInfo = route_info_get(pos);

                    if (NULL != pRouteInfo)
                    {
                        /* ��ӵ�����·��ҵ����Ϣ���� */
                        i = route_srv_msg_add(pRouteInfo, MSG_BYE_RESPONSE, caller_id, callee_id, status_code, NULL, dialog_index, NULL, 0, cr_pos);
                        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "bye_response_received_proc() route_srv_msg_add:i=%d \r\n", i);
                        return 0;
                    }
                }
            }
        }

        i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
        i = call_record_remove(cr_pos);
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "bye_response_received_proc() exit---: No Matching Thread Found To Put:call_record_remove, cr_pos=%d \r\n", cr_pos);
        return -1;
    }

    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Bye Response Message Proc failed:caller_id=%s, callee_id=%s, dialog_index=%d, Cause=%s", caller_id, callee_id, dialog_index, (char*)"No Matching Thread Found To Put");
    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Bye��Ӧ��Ϣ����ʧ��:caller_id=%s, caller_ip=%s, caller_port=%d, callee_id=%s, dialog_index=%d, ԭ��=%s", caller_id, caller_ip, caller_port, callee_id, dialog_index, (char*)"�Ҳ�����Ӧ�Ĵ����߳�");

    DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "bye_response_received_proc() exit---: No Matching Thread Found To Put \r\n");
    return -1;
}

/*****************************************************************************
 �� �� ��  : message_received_proc
 ��������  : �յ�message��Ϣ�Ĵ�����
 �������  : char* caller_id
             char* caller_ip
             int caller_port
             char* callee_id
             char* caller_ip
             int caller_port,
             char* call_id
             int dialog_index
             char* msg_body
             int msg_len
             int user_data
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��10��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int message_received_proc(char* caller_id, char* caller_ip, int caller_port, char* callee_id, char* callee_ip, int callee_port, char* call_id, int dialog_index, char* msg_body, int msg_len, int user_data)
{
    int i = 0;
    int pos = -1;
    user_srv_proc_tl_t* user_srv_thread = NULL;
    device_srv_proc_tl_t* device_srv_thread = NULL;
    route_srv_proc_tl_t* route_srv_thread = NULL;
    GBDevice_info_t* pGBDeviceInfo = NULL;
    route_info_t* pRouteInfo = NULL;

    if (0 == cms_run_status)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "message_received_proc() exit---: CMS Not Run Normal Error \r\n");
        return -1;
    }

    DEBUG_TRACE(MODULE_COMMON, LOG_INFO,  "message_received_proc() \
    \r\n In Param: \
    \r\n call_id=%s \
    \r\n caller_id=%s \
    \r\n caller_ip=%s \
    \r\n caller_port=%d \
    \r\n callee_id=%s \
    \r\n callee_ip=%s \
    \r\n callee_port=%d \
    \r\n dialog_index=%d \
    \r\n ", call_id, caller_id, caller_ip, caller_port, callee_id, callee_ip, callee_port, dialog_index);

    if ((0 == sstrcmp(caller_id, (char*)"wiscomCallerID"))
        && (0 == sstrcmp(callee_id, (char*)"wiscomCalleeID"))) /* ��ѯ������ID���û�ID */
    {
        //printf("\r\n\r\n ******************** User Get ServerID And UserID Begin ******************* \r\n");
        i = message_get_service_id_proc(caller_id, caller_ip, caller_port, callee_id, callee_ip, callee_port, msg_body, msg_len);
        //printf("\r\n ******************** User Get ServerID And UserID End ******************* \r\n\r\n");
        return i;
    }

    /* �����û������̶߳��� */
    user_srv_thread = get_user_srv_proc_thread(caller_id, caller_ip, caller_port);

    if (NULL != user_srv_thread)
    {
        if (NULL != user_srv_thread->pUserSrvMsgQueue)
        {
            /* ��ӵ��û�ҵ����Ϣ���� */
            i = user_srv_msg_add(user_srv_thread, MSG_MESSAGE, caller_id, callee_id, 0, NULL, dialog_index, msg_body, msg_len, -1);
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "message_received_proc() user_srv_msg_add:i=%d \r\n", i);
            return 0;
        }
    }
    else
    {
        /* �����������豸�ϱ���Ϣ����Ϣ�����ԣ��Ȳ��������豸 */
#if 1
        /* �����豸�����̶߳��� */
        device_srv_thread = get_device_srv_proc_thread(caller_id, caller_ip, caller_port);

        if (NULL != device_srv_thread)
        {
            /* ��ӵ��豸�߳�ҵ����Ϣ���� */
            i = device_srv_msg_add_for_appoint(device_srv_thread, MSG_MESSAGE, caller_id, callee_id, 0, NULL, dialog_index, msg_body, msg_len, -1);
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "message_received_proc() device_srv_msg_add_for_appoint:i=%d \r\n", i);
            return 0;
        }
        else
#endif
        {
            pGBDeviceInfo = GBDevice_info_find_by_id_ip_and_port(caller_id, caller_ip, caller_port);

            if (NULL != pGBDeviceInfo)
            {
                /* �鿴�豸��ע��״̬ */
                if (pGBDeviceInfo->reg_status > 0)
                {
                    /* ��ӵ��豸ҵ����Ϣ���� */
                    i = device_srv_msg_add(pGBDeviceInfo, MSG_MESSAGE, caller_id, callee_id, 0, NULL, dialog_index, msg_body, msg_len, -1);
                    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "message_received_proc() device_message_srv_msg_add 1:i=%d \r\n", i);
                    return 0;
                }
            }
            else
            {
                pGBDeviceInfo = GBDevice_info_find_by_ip_and_port(caller_ip, caller_port);

                if (NULL != pGBDeviceInfo)
                {
                    /* �鿴�豸��ע��״̬ */
                    if (pGBDeviceInfo->reg_status > 0)
                    {
                        /* ��ӵ��豸ҵ����Ϣ���� */
                        i = device_srv_msg_add(pGBDeviceInfo, MSG_MESSAGE, caller_id, callee_id, 0, NULL, dialog_index, msg_body, msg_len, -1);
                        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "message_received_proc() device_message_srv_msg_add 2:i=%d \r\n", i);
                        return 0;
                    }
                }
                else
                {
                    /* ����������Ա������������,����Ƿ�����route */
#if 1
                    /* �����ϻ�ƽ̨�����̶߳��� */
                    route_srv_thread = get_route_srv_proc_thread(caller_id, caller_ip, caller_port);

                    if (NULL != route_srv_thread)
                    {
                        /* ��ӵ��ϼ�ƽ̨�߳�ҵ����Ϣ���� */
                        i = route_srv_msg_add_for_appoint(route_srv_thread, MSG_MESSAGE, caller_id, callee_id, 0, NULL, dialog_index, msg_body, msg_len, -1);
                        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "message_received_proc() route_srv_msg_add_for_appoint:i=%d \r\n", i);
                        return 0;
                    }
                    else
#endif
                    {
                        pos = route_info_find_by_host_and_port(caller_ip, caller_port);

                        if (pos >= 0)
                        {
                            pRouteInfo = route_info_get(pos);

                            if (NULL != pRouteInfo)
                            {
                                /* ��ӵ�����·��ҵ����Ϣ���� */
                                i = route_srv_msg_add(pRouteInfo, MSG_MESSAGE, caller_id, callee_id, 0, NULL, dialog_index, msg_body, msg_len, -1);
                                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "message_received_proc() route_srv_msg_add:i=%d \r\n", i);
                                return 0;
                            }
                        }
                    }
                }
            }
        }
    }

    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Message Proc failed:caller_id=%s, caller_ip=%s, caller_port=%d, Cause=%s", caller_id, caller_ip, caller_port, (char*)"No Matching Thread Found To Put");
    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Message��Ϣ����ʧ��:caller_id=%s, caller_ip=%s, caller_port=%d, callee_id=%s, ԭ��=%s", caller_id, caller_ip, caller_port, callee_id, (char*)"�Ҳ�����Ӧ�Ĵ����߳�");

    DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "message_received_proc() exit---: No Matching Thread Found To Put \r\n");
    return -1;
}

/*****************************************************************************
 �� �� ��  : message_response_received_proc
 ��������  : �յ�message��Ӧ��Ϣ�Ĵ�����
 �������  : char* caller_id
             char* callee_id
             char* call_id
             int status_code
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��10��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int message_response_received_proc(char* caller_id, char* callee_id, char* call_id, int status_code, int user_data)
{
    int iRet = 0;
    int pos = -1;
    route_info_t* pRouteInfo = NULL;
    char strCallID[MAX_128CHAR_STRING_LEN + 4] = {0};

    if (0 == cms_run_status)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "message_response_received_proc() exit---: CMS Not Run Normal Error \r\n");
        return -1;
    }

    DEBUG_TRACE(MODULE_COMMON, LOG_INFO,  "message_response_received_proc() \
    \r\n In Param: \
    \r\n call_id=%s \
    \r\n caller_id=%s \
    \r\n callee_id=%s \
    \r\n status_code=%d \
    \r\n ", call_id, caller_id, callee_id, status_code);

    /* ������������Ϣ(����״̬�ϱ����ص���Ϣ����һ���Ǳ�����Ϣ)���صĴ������ԣ���ʱ���Σ���Ҫ�����ж����� */
    if (status_code != 200)
    {
        pos = route_info_find(callee_id);

        if (pos < 0)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "message_response_received_proc() exit---: Find Route Info Error:callee_id=%s \r\n", callee_id);
            return -1;
        }

        pRouteInfo = route_info_get(pos);

        if (NULL == pRouteInfo)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "message_response_received_proc() exit---: Get Route Info Error:pos=%d \r\n", pos);
            return -1;
        }

        DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "message_response_received_proc() RouteID=%s \r\n", pRouteInfo->server_id);

        /* ƥ��callID,���Ƿ��Ǳ�����Ϣ */
        snprintf(strCallID, MAX_128CHAR_STRING_LEN + 4, "%u", pRouteInfo->keep_alive_sn);

        if (0 == sstrcmp(strCallID, call_id))/* ����Ǳ�����Ϣ */
        {
            pRouteInfo->failed_keep_alive_count++; /* ����ʧ�ܼ��� */

            if (pRouteInfo->failed_keep_alive_count >= local_failed_keep_alive_count_get())
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "�ϼ�·����ʧ�ܴ����ﵽ%d��, ����ע�����ϼ�ע����Ϣ: �ϼ�·��ID=%s, �ϼ�·��IP=%s, �ϼ�·��Port=%d", pRouteInfo->failed_keep_alive_count, pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);

                pRouteInfo->reg_status = 0;
                pRouteInfo->reg_info_index = -1;
                pRouteInfo->expires = 0;
                pRouteInfo->min_expires = 0;
                pRouteInfo->reg_interval = 0;
                pRouteInfo->keep_alive_count = local_keep_alive_interval_get();
                pRouteInfo->failed_keep_alive_count = 0;

                /* �Ƴ��ϼ������ĵ�λ��Ϣ */
                iRet = RemoveGBLogicDeviceLockInfoByRouteInfo(pRouteInfo);

                if (0 != iRet)
                {
                    DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "message_response_received_proc() RemoveGBLogicDeviceLockInfoByRouteInfo Error:server_ip=%s, server_port=%d, iRet=%d \r\n", pRouteInfo->server_ip, pRouteInfo->server_port, iRet);
                }
                else
                {
                    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "message_response_received_proc() RemoveGBLogicDeviceLockInfoByRouteInfo OK:server_ip=%s, server_port=%d, iRet=%d \r\n", pRouteInfo->server_ip, pRouteInfo->server_port, iRet);
                }

                iRet = StopAllServiceTaskByCallerIPAndPort(pRouteInfo->server_ip, pRouteInfo->server_port);

                if (0 != iRet)
                {
                    DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "message_response_received_proc() StopAllServiceTaskByCallerIPAndPort Error:server_ip=%s, server_port=%d, iRet=%d \r\n", pRouteInfo->server_ip, pRouteInfo->server_port, iRet);
                }
                else
                {
                    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "message_response_received_proc() StopAllServiceTaskByCallerIPAndPort OK:server_ip=%s, server_port=%d, iRet=%d \r\n", pRouteInfo->server_ip, pRouteInfo->server_port, iRet);
                }

                /* �������ϼ�CMS������ */
                iRet = StopAllServiceTaskByCalleeIPAndPort(pRouteInfo->server_ip, pRouteInfo->server_port);

                if (0 != iRet)
                {
                    DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "message_response_received_proc() StopAllServiceTaskByCalleeIPAndPort Error:server_ip=%s, server_port=%d, iRet=%d \r\n", pRouteInfo->server_ip, pRouteInfo->server_port, iRet);
                }
                else
                {
                    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "message_response_received_proc() StopAllServiceTaskByCalleeIPAndPort OK:server_ip=%s, server_port=%d, iRet=%d \r\n", pRouteInfo->server_ip, pRouteInfo->server_port, iRet);
                }

                /* ����ҵ�����߳� */
                iRet = route_srv_proc_thread_recycle(pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);

                if (0 != iRet)
                {
                    DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "message_response_received_proc() route_srv_proc_thread_recycle Error:server_id=%s, server_ip=%s, server_port=%d, iRet=%d \r\n", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, iRet);
                }
                else
                {
                    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "message_response_received_proc() route_srv_proc_thread_recycle OK:server_id=%s, server_ip=%s, server_port=%d, iRet=%d \r\n", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, iRet);
                }
            }
            else
            {
                pRouteInfo->keep_alive_count = local_keep_alive_interval_get();
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "�ϼ�·����ʧ�ܴ���=%d��: �ϼ�·��ID=%s, �ϼ�·��IP=%s, �ϼ�·��Port=%d", pRouteInfo->failed_keep_alive_count, pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
            }
        }
    }

    return 0;
}

/*****************************************************************************
 �� �� ��  : subscribe_received_proc
 ��������  : �յ�Subscribe��Ϣ�Ĵ�����
 �������  : char* caller_id
             char* caller_ip
             int caller_port
             char* callee_id
             char* caller_ip
             int caller_port,
             char* call_id
             char* event_type �¼�����
             char* id_param �¼�����ID
             int subscribe_expires ��ʱʱ��
             char* msg_body
             int msg_len
             int user_data
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��10��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int subscribe_received_proc(char* caller_id, char* caller_ip, int caller_port, char* callee_id, char* callee_ip, int callee_port, char* call_id, char* event_type, char* id_param, int subscribe_expires, char* msg_body, int msg_len, int user_data)
{
    int i = 0;
    int pos = -1;
    route_srv_proc_tl_t* route_srv_thread = NULL;
    route_info_t* pRouteInfo = NULL;
    int event_id = 0;

    if (0 == cms_run_status)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "subscribe_received_proc() exit---: CMS Not Run Normal Error \r\n");
        return -1;
    }

    DEBUG_TRACE(MODULE_COMMON, LOG_INFO,  "subscribe_received_proc() \
    \r\n In Param: \
    \r\n call_id=%s \
    \r\n caller_id=%s \
    \r\n caller_ip=%s \
    \r\n caller_port=%d \
    \r\n callee_id=%s \
    \r\n callee_ip=%s \
    \r\n callee_port=%d \
    \r\n event_type=%s \
    \r\n id_param=%s \
    \r\n subscribe_expires=%d \
    \r\n ", call_id, caller_id, caller_ip, caller_port, callee_id, callee_ip, callee_port, event_type, id_param, subscribe_expires);

    /* Ŀǰ��֧��Ŀ¼������Ϣ */
    if (0 != sstrcmp(event_type, (char*)"Catalog"))
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "subscribe_received_proc() exit---: No Support Event Type:event_type=%s \r\n", event_type);
        return -1;
    }

    if (NULL == id_param)
    {
        event_id = osip_atoi(id_param);
    }

    /* ĿǰĿ¼������Ϣ�����������ϼ�ƽ̨ */
    /* �����ϼ���ƽ̨�����̶߳��� */
    route_srv_thread = get_route_srv_proc_thread(caller_id, caller_ip, caller_port);

    if (NULL != route_srv_thread)
    {
        /* ��ӵ��ϼ�ƽ̨�߳�ҵ����Ϣ���� */
        i = route_srv_msg_add_for_appoint(route_srv_thread, MSG_SUBSCRIBE, caller_id, callee_id, event_id, NULL, subscribe_expires, msg_body, msg_len, -1);
        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "subscribe_received_proc() route_srv_msg_add_for_appoint:i=%d \r\n", i);
        return 0;
    }
    else
    {
        pos = route_info_find_by_host_and_port(caller_ip, caller_port);

        if (pos >= 0)
        {
            pRouteInfo = route_info_get(pos);

            if (NULL != pRouteInfo)
            {
                /* ��ӵ�����·��ҵ����Ϣ���� */
                i = route_srv_msg_add(pRouteInfo, MSG_SUBSCRIBE, caller_id, callee_id, event_id, NULL, subscribe_expires, msg_body, msg_len, -1);
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "subscribe_received_proc() route_srv_msg_add:i=%d \r\n", i);
                return 0;
            }
        }
    }

    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Subscribe Message Proc failed:caller_id=%s, caller_ip=%s, caller_port=%d, Cause=%s", caller_id, caller_ip, caller_port, (char*)"No Matching Thread Found To Put");
    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Subscribe��Ϣ����ʧ��:caller_id=%s, caller_ip=%s, caller_port=%d, callee_id=%s, ԭ��=%s", caller_id, caller_ip, caller_port, callee_id, (char*)"�Ҳ�����Ӧ�Ĵ����߳�");

    DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "subscribe_received_proc() exit---: No Matching Thread Found To Put \r\n");
    return -1;
}

/*****************************************************************************
 �� �� ��  : subscribe_response_received_proc
 ��������  : �յ�Subscribe��Ӧ��Ϣ�Ĵ�����
 �������  : char* caller_id
             char* callee_id
             char* call_id
             int expires
             int status_code
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��10��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int subscribe_response_received_proc(char* caller_id, char* callee_id, char* call_id, int expires, int status_code, int user_data)
{
    GBDevice_info_t* pGBDeviceInfo = NULL;
    char strCallID[MAX_128CHAR_STRING_LEN + 4] = {0};

    if (0 == cms_run_status)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "subscribe_response_received_proc() exit---: CMS Not Run Normal Error \r\n");
        return -1;
    }

    DEBUG_TRACE(MODULE_COMMON, LOG_INFO,  "subscribe_response_received_proc() \
    \r\n In Param: \
    \r\n call_id=%s \
    \r\n caller_id=%s \
    \r\n callee_id=%s \
    \r\n expires=%d \
    \r\n status_code=%d \
    \r\n ", call_id, caller_id, callee_id, expires, status_code);

    pGBDeviceInfo = GBDevice_info_find(callee_id);

    if (NULL == pGBDeviceInfo)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "subscribe_response_received_proc() exit---: Find GBDevice Info Error \r\n");
        return -1;
    }

    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "subscribe_response_received_proc() DeviceID=%s \r\n", pGBDeviceInfo->device_id);

    /* ƥ��callID,���Ƿ��Ƕ�����Ϣ */
    snprintf(strCallID, MAX_128CHAR_STRING_LEN + 4, "%u", pGBDeviceInfo->catalog_subscribe_event_id);

    if (0 != sstrcmp(strCallID, call_id))
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "subscribe_response_received_proc() exit---: Call ID Not Equal: event_id=%d \r\n", pGBDeviceInfo->catalog_subscribe_event_id);
        return -1;
    }

    if (status_code == 200)
    {
        if (expires <= 0)
        {
            pGBDeviceInfo->catalog_subscribe_flag = 0;
            pGBDeviceInfo->catalog_subscribe_interval = local_register_retry_interval_get();
            pGBDeviceInfo->catalog_subscribe_expires = 0;
            pGBDeviceInfo->catalog_subscribe_expires_count = 0;
            pGBDeviceInfo->catalog_subscribe_event_id = 0;
        }
        else
        {
            pGBDeviceInfo->catalog_subscribe_flag = 1;
            pGBDeviceInfo->catalog_subscribe_interval = local_register_retry_interval_get();
            pGBDeviceInfo->catalog_subscribe_expires = expires;
            pGBDeviceInfo->catalog_subscribe_expires_count = expires;
        }
    }
    else
    {
        pGBDeviceInfo->catalog_subscribe_flag = 0;
        pGBDeviceInfo->catalog_subscribe_interval = local_register_retry_interval_get();
        pGBDeviceInfo->catalog_subscribe_expires = 0;
        pGBDeviceInfo->catalog_subscribe_expires_count = 0;
        pGBDeviceInfo->catalog_subscribe_event_id = 0;
    }

    return 0;
}

int subscribe_within_dialog_received_proc(char* caller_id, char* caller_ip, int caller_port, char* callee_id, char* call_id, int dialog_index, int subscribe_expires, char* msg_body, int msg_len)
{
    int i = 0;
    int pos = -1;
    route_srv_proc_tl_t* route_srv_thread = NULL;
    route_info_t* pRouteInfo = NULL;

    if (0 == cms_run_status)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "subscribe_within_dialog_received_proc() exit---: CMS Not Run Normal Error \r\n");
        return -1;
    }

    DEBUG_TRACE(MODULE_COMMON, LOG_INFO,  "subscribe_within_dialog_received_proc() \
    \r\n In Param: \
    \r\n caller_id=%s \
    \r\n caller_ip=%s \
    \r\n caller_port=%d \
    \r\n callee_id=%s \
    \r\n call_id=%s \
    \r\n dialog_index=%d \
    \r\n subscribe_expires=%d \
    \r\n", caller_id, caller_ip, caller_port, callee_id, call_id, dialog_index, subscribe_expires);

    if (subscribe_expires <= 0) /* ȥ���� */
    {
        pos = route_info_find_by_host_and_port(caller_ip, caller_port);

        if (pos >= 0)
        {
            pRouteInfo = route_info_get(pos);

            if (NULL != pRouteInfo)
            {
                if (pRouteInfo->catalog_subscribe_dialog_index == dialog_index)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "�ϼ�CMS������Ŀ¼������Ϣ:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �յ��ϼ�CMS��ע��������Ϣ", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);

                    /* ����״̬ */
                    pRouteInfo->catalog_subscribe_flag = 0;
                    pRouteInfo->catalog_subscribe_expires = 0;
                    pRouteInfo->catalog_subscribe_expires_count = 0;
                    pRouteInfo->catalog_subscribe_dialog_index = -1;
                    return 0;
                }
                else
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Subscribe��Ϣ����ʧ��:caller_id=%s, caller_ip=%s, caller_port=%d, callee_id=%s, ԭ��=%s, catalog_subscribe_dialog_index=%d, dialog_index=%d", caller_id, caller_ip, caller_port, callee_id, (char*)"Ŀ¼���ĵ�SUBSCRIBE�Ự������ƥ��", pRouteInfo->catalog_subscribe_dialog_index, dialog_index);
                    return -1;
                }
            }
        }
    }
    else
    {
        /* ĿǰĿ¼������Ϣ�����������ϼ�ƽ̨ */
        /* �����ϼ�ƽ̨�����̶߳��� */
        route_srv_thread = get_route_srv_proc_thread(caller_id, caller_ip, caller_port);

        if (NULL != route_srv_thread)
        {
            /* ��ӵ��ϼ�ƽ̨�߳�ҵ����Ϣ���� */
            i = route_srv_msg_add_for_appoint(route_srv_thread, MSG_SUBSCRIBE, caller_id, callee_id, subscribe_expires, NULL, dialog_index, msg_body, msg_len, -1);
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "subscribe_within_dialog_received_proc() route_srv_msg_add_for_appoint:i=%d \r\n", i);
            return 0;
        }
        else
        {
            pos = route_info_find_by_host_and_port(caller_ip, caller_port);

            if (pos >= 0)
            {
                pRouteInfo = route_info_get(pos);

                if (NULL != pRouteInfo)
                {
                    /* ��ӵ�����·��ҵ����Ϣ���� */
                    i = route_srv_msg_add(pRouteInfo, MSG_SUBSCRIBE, caller_id, callee_id, subscribe_expires, NULL, dialog_index, msg_body, msg_len, -1);
                    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "subscribe_within_dialog_received_proc() route_srv_msg_add:i=%d \r\n", i);
                    return 0;
                }
            }
        }
    }

    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Subscribe Message Proc failed:caller_id=%s, caller_ip=%s, caller_port=%d, Cause=%s", caller_id, caller_ip, caller_port, (char*)"No Matching Thread Found To Put");
    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Subscribe��Ϣ����ʧ��:caller_id=%s, caller_ip=%s, caller_port=%d, callee_id=%s, ԭ��=%s", caller_id, caller_ip, caller_port, callee_id, (char*)"�Ҳ�����Ӧ�Ĵ����߳�");

    DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "subscribe_within_dialog_received_proc() exit---: No Matching Thread Found To Put \r\n");
    return -1;
}

int subscribe_within_dialog_response_received_proc(char* caller_id, char* callee_id, char* call_id, int dialog_index, int expires, int status_code)
{
    GBDevice_info_t* pGBDeviceInfo = NULL;
    char strCallID[MAX_128CHAR_STRING_LEN + 4] = {0};

    if (0 == cms_run_status)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "subscribe_within_dialog_response_received_proc() exit---: CMS Not Run Normal Error \r\n");
        return -1;
    }

    DEBUG_TRACE(MODULE_COMMON, LOG_INFO,  "subscribe_within_dialog_response_received_proc() \
    \r\n In Param: \
    \r\n call_id=%s \
    \r\n caller_id=%s \
    \r\n callee_id=%s \
    \r\n dialog_index=%d \
    \r\n expires=%d \
    \r\n status_code=%d \
    \r\n ", call_id, caller_id, callee_id, dialog_index, expires, status_code);

    pGBDeviceInfo = GBDevice_info_find(callee_id);

    if (NULL == pGBDeviceInfo)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "subscribe_within_dialog_response_received_proc() exit---: Find GBDevice Info Error \r\n");
        return -1;
    }

    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "subscribe_within_dialog_response_received_proc() DeviceID=%s \r\n", pGBDeviceInfo->device_id);

    /* ƥ��callID,���Ƿ��Ƕ�����Ϣ */
    snprintf(strCallID, MAX_128CHAR_STRING_LEN + 4, "%u", pGBDeviceInfo->catalog_subscribe_event_id);

    if (0 != sstrcmp(strCallID, call_id))
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "subscribe_within_dialog_response_received_proc() exit---: Call ID Not Equal: event_id=%d \r\n", pGBDeviceInfo->catalog_subscribe_event_id);
        return -1;
    }

    if (pGBDeviceInfo->catalog_subscribe_dialog_index != dialog_index)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "subscribe_within_dialog_response_received_proc() exit---: dialog_index Not Equal: catalog_subscribe_dialog_index=%d, dialog_index=%d \r\n", pGBDeviceInfo->catalog_subscribe_dialog_index, dialog_index);
        return -1;
    }

    if (status_code == 200)
    {
        if (expires <= 0)
        {
            pGBDeviceInfo->catalog_subscribe_flag = 0;
            pGBDeviceInfo->catalog_subscribe_interval = local_register_retry_interval_get();
            pGBDeviceInfo->catalog_subscribe_expires = 0;
            pGBDeviceInfo->catalog_subscribe_expires_count = 0;
            pGBDeviceInfo->catalog_subscribe_event_id = 0;
            pGBDeviceInfo->catalog_subscribe_dialog_index = -1;
        }
        else
        {
            pGBDeviceInfo->catalog_subscribe_flag = 1;
            pGBDeviceInfo->catalog_subscribe_interval = local_register_retry_interval_get();
            pGBDeviceInfo->catalog_subscribe_expires = expires;
            pGBDeviceInfo->catalog_subscribe_expires_count = expires;
        }
    }
    else
    {
        pGBDeviceInfo->catalog_subscribe_flag = 0;
        pGBDeviceInfo->catalog_subscribe_interval = local_register_retry_interval_get();
        pGBDeviceInfo->catalog_subscribe_expires = 0;
        pGBDeviceInfo->catalog_subscribe_expires_count = 0;
        pGBDeviceInfo->catalog_subscribe_event_id = 0;
        pGBDeviceInfo->catalog_subscribe_dialog_index = -1;
    }

    return 0;
}

/*****************************************************************************
 �� �� ��  : notify_received_proc
 ��������  : �յ�notify��Ϣ�Ĵ�����
 �������  : char* caller_id
             char* caller_ip
             int caller_port
             char* callee_id
             char* caller_ip
             int caller_port,
             char* call_id
             char* msg_body
             int msg_len
             int user_data
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��10��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int notify_received_proc(char* caller_id, char* caller_ip, int caller_port, char* callee_id, char* callee_ip, int callee_port, char* call_id, char* msg_body, int msg_len, int user_data)
{
    int i = 0;
    int pos = -1;
    user_srv_proc_tl_t* user_srv_thread = NULL;
    device_srv_proc_tl_t* device_srv_thread = NULL;
    route_srv_proc_tl_t* route_srv_thread = NULL;
    GBDevice_info_t* pGBDeviceInfo = NULL;
    route_info_t* pRouteInfo = NULL;

    if (0 == cms_run_status)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "notify_received_proc() exit---: CMS Not Run Normal Error \r\n");
        return -1;
    }

    DEBUG_TRACE(MODULE_COMMON, LOG_INFO,  "notify_received_proc() \
    \r\n In Param: \
    \r\n call_id=%s \
    \r\n caller_id=%s \
    \r\n caller_ip=%s \
    \r\n caller_port=%d \
    \r\n callee_id=%s \
    \r\n callee_ip=%s \
    \r\n callee_port=%d \
    \r\n ", call_id, caller_id, caller_ip, caller_port, callee_id, callee_ip, callee_port);

    /* �����û������̶߳��� */
    user_srv_thread = get_user_srv_proc_thread(caller_id, caller_ip, caller_port);

    if (NULL != user_srv_thread)
    {
        if (NULL != user_srv_thread->pUserSrvMsgQueue)
        {
            /* ��ӵ��û�ҵ����Ϣ���� */
            i = user_srv_msg_add(user_srv_thread, MSG_NOTIFY, caller_id, callee_id, 0, NULL, -1, msg_body, msg_len, -1);
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "notify_received_proc() user_srv_msg_add:i=%d \r\n", i);
            return 0;
        }
    }
    else
    {
        /* �����������豸�ϱ���Ϣ����Ϣ�����ԣ��Ȳ��������豸 */
#if 1
        /* �����豸�����̶߳��� */
        device_srv_thread = get_device_srv_proc_thread(caller_id, caller_ip, caller_port);

        if (NULL != device_srv_thread)
        {
            /* ��ӵ��豸�߳�ҵ����Ϣ���� */
            i = device_srv_msg_add_for_appoint(device_srv_thread, MSG_NOTIFY, caller_id, callee_id, 0, NULL, -1, msg_body, msg_len, -1);
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "notify_received_proc() device_srv_msg_add_for_appoint:i=%d \r\n", i);
            return 0;
        }
        else
#endif
        {
            pGBDeviceInfo = GBDevice_info_find_by_id_ip_and_port(caller_id, caller_ip, caller_port);

            if (NULL != pGBDeviceInfo)
            {
                /* �鿴�豸��ע��״̬ */
                if (pGBDeviceInfo->reg_status > 0)
                {
                    /* ��ӵ��豸ҵ����Ϣ���� */
                    i = device_srv_msg_add(pGBDeviceInfo, MSG_NOTIFY, caller_id, callee_id, 0, NULL, -1, msg_body, msg_len, -1);
                    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "notify_received_proc() device_srv_msg_add 1:i=%d \r\n", i);
                    return 0;
                }
            }
            else
            {
                pGBDeviceInfo = GBDevice_info_find_by_ip_and_port(caller_ip, caller_port);

                if (NULL != pGBDeviceInfo)
                {
                    /* �鿴�豸��ע��״̬ */
                    if (pGBDeviceInfo->reg_status > 0)
                    {
                        /* ��ӵ��豸ҵ����Ϣ���� */
                        i = device_srv_msg_add(pGBDeviceInfo, MSG_NOTIFY, caller_id, callee_id, 0, NULL, -1, msg_body, msg_len, -1);
                        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "notify_received_proc() device_srv_msg_add 2:i=%d \r\n", i);
                        return 0;
                    }
                }
                else
                {
                    /* ����������Ա������������,����Ƿ�����route */
#if 1
                    /* �����ϻ�ƽ̨�����̶߳��� */
                    route_srv_thread = get_route_srv_proc_thread(caller_id, caller_ip, caller_port);

                    if (NULL != route_srv_thread)
                    {
                        /* ��ӵ��ϼ�ƽ̨�߳�ҵ����Ϣ���� */
                        i = route_srv_msg_add_for_appoint(route_srv_thread, MSG_NOTIFY, caller_id, callee_id, 0, NULL, -1, msg_body, msg_len, -1);
                        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "notify_received_proc() route_srv_msg_add_for_appoint:i=%d \r\n", i);
                        return 0;
                    }
                    else
#endif
                    {
                        pos = route_info_find_by_host_and_port(caller_ip, caller_port);

                        if (pos >= 0)
                        {
                            pRouteInfo = route_info_get(pos);

                            if (NULL != pRouteInfo)
                            {
                                /* ��ӵ�����·��ҵ����Ϣ���� */
                                i = route_srv_msg_add(pRouteInfo, MSG_NOTIFY, caller_id, callee_id, 0, NULL, -1, msg_body, msg_len, -1);
                                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "notify_received_proc() route_srv_msg_add:i=%d \r\n", i);
                                return 0;
                            }
                        }
                    }
                }
            }
        }
    }

    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Notify Message Proc failed:caller_id=%s, caller_ip=%s, caller_port=%d, Cause=%s", caller_id, caller_ip, caller_port, (char*)"No Matching Thread Found To Put");
    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Notify��Ϣ����ʧ��:caller_id=%s, caller_ip=%s, caller_port=%d, callee_id=%s, ԭ��=%s", caller_id, caller_ip, caller_port, callee_id, (char*)"�Ҳ�����Ӧ�Ĵ����߳�");

    DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "notify_received_proc() exit---: No Matching Thread Found To Put \r\n");
    return -1;
}

/*****************************************************************************
 �� �� ��  : notify_response_received_proc
 ��������  : �յ�Notify��Ӧ��Ϣ�Ĵ�����
 �������  : char* caller_id
             char* callee_id
             char* call_id
             int status_code
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��10��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int notify_response_received_proc(char* caller_id, char* callee_id, char* call_id, int status_code, int user_data)
{
    if (0 == cms_run_status)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "notify_response_received_proc() exit---: CMS Not Run Normal Error \r\n");
        return -1;
    }

    DEBUG_TRACE(MODULE_COMMON, LOG_INFO,  "notify_response_received_proc() \
    \r\n In Param: \
    \r\n call_id=%s \
    \r\n caller_id=%s \
    \r\n callee_id=%s \
    \r\n status_code=%d \
    \r\n ", call_id, caller_id, callee_id, status_code);

    if (status_code != 200)
    {
        // TODO:���·��Ͷ�����Ϣ
    }

    return 0;
}

/*****************************************************************************
 �� �� ��  : info_received_proc
 ��������  : �յ�info��Ϣ�Ĵ�����
 �������  : char* caller_id
             char* caller_ip
             int caller_port
             char* callee_id
             char* call_id
             int dialog_index
             char* msg_body
             int msg_len
             int user_data
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��10��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int info_received_proc(char* caller_id, char* caller_ip, int caller_port, char* callee_id, char* call_id, int dialog_index, char* msg_body, int msg_len, int user_data)
{
    int i = 0;
    int pos = -1;
    user_srv_proc_tl_t* user_srv_thread = NULL;
    device_srv_proc_tl_t* device_srv_thread = NULL;
    route_srv_proc_tl_t* route_srv_thread = NULL;
    GBDevice_info_t* pGBDeviceInfo = NULL;
    route_info_t* pRouteInfo = NULL;

    if (0 == cms_run_status)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "info_received_proc() exit---: CMS Not Run Normal Error \r\n");
        return -1;
    }

    DEBUG_TRACE(MODULE_COMMON, LOG_INFO,  "info_received_proc() \
    \r\n In Param: \
    \r\n call_id=%s \
    \r\n caller_id=%s \
    \r\n caller_ip=%s \
    \r\n caller_port=%d \
    \r\n callee_id=%s \
    \r\n dialog_index=%d \
    \r\n", call_id, caller_id, caller_ip, caller_port, callee_id, dialog_index);

    /* �����û������̶߳��� */
    user_srv_thread = get_user_srv_proc_thread(caller_id, caller_ip, caller_port);

    if (NULL != user_srv_thread)
    {
        if (NULL != user_srv_thread->pUserSrvMsgQueue)
        {
            /* ��ӵ��û�ҵ����Ϣ���� */
            i = user_srv_msg_add(user_srv_thread, MSG_INFO, caller_id, callee_id, 0, NULL, dialog_index, msg_body, msg_len, -1);
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "info_received_proc() user_srv_msg_add:i=%d \r\n", i);
            return 0;
        }
    }
    else
    {
        /* ��������ǰ���豸*/
#if 1
        /* �����豸�����̶߳��� */
        device_srv_thread = get_device_srv_proc_thread(caller_id, caller_ip, caller_port);

        if (NULL != device_srv_thread)
        {
            /* ��ӵ��豸�߳�ҵ����Ϣ���� */
            i = device_srv_msg_add_for_appoint(device_srv_thread, MSG_INFO, caller_id, callee_id, 0, NULL, dialog_index, msg_body, msg_len, -1);
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "info_received_proc() device_srv_msg_add_for_appoint:i=%d \r\n", i);
            return 0;
        }
        else
#endif
        {
            pGBDeviceInfo = GBDevice_info_find_by_id_ip_and_port(caller_id, caller_ip, caller_port);

            if (NULL != pGBDeviceInfo)
            {
                /* �鿴�豸��ע��״̬ */
                if (pGBDeviceInfo->reg_status > 0)
                {
                    /* ��ӵ��豸ҵ����Ϣ���� */
                    i = device_srv_msg_add(pGBDeviceInfo, MSG_INFO, caller_id, callee_id, 0, NULL, dialog_index, msg_body, msg_len, -1);
                    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "info_received_proc() device_srv_msg_add 1:i=%d \r\n", i);
                    return 0;
                }
            }
            else
            {
                pGBDeviceInfo = GBDevice_info_find_by_ip_and_port(caller_ip, caller_port);

                if (NULL != pGBDeviceInfo)
                {
                    /* ��ӵ��豸ҵ����Ϣ���� */
                    i = device_srv_msg_add(pGBDeviceInfo, MSG_INFO, caller_id, callee_id, 0, NULL, dialog_index, msg_body, msg_len, -1);
                    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "info_received_proc() device_srv_msg_add 2:i=%d \r\n", i);
                    return 0;
                }
                else
                {
                    /* ����������Ա������������,����Ƿ�����route */
#if 1
                    /* �����ϻ�ƽ̨�����̶߳��� */
                    route_srv_thread = get_route_srv_proc_thread(caller_id, caller_ip, caller_port);

                    if (NULL != route_srv_thread)
                    {
                        /* ��ӵ��ϼ�ƽ̨�߳�ҵ����Ϣ���� */
                        i = route_srv_msg_add_for_appoint(route_srv_thread, MSG_INFO, caller_id, callee_id, 0, NULL, dialog_index, msg_body, msg_len, -1);
                        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "info_received_proc() route_srv_msg_add_for_appoint:i=%d \r\n", i);
                        return 0;
                    }
                    else
#endif
                    {
                        pos = route_info_find_by_host_and_port(caller_ip, caller_port);

                        if (pos >= 0)
                        {
                            pRouteInfo = route_info_get(pos);

                            if (NULL != pRouteInfo)
                            {
                                /* ��ӵ�����·��ҵ����Ϣ���� */
                                i = route_srv_msg_add(pRouteInfo, MSG_INFO, caller_id, callee_id, 0, NULL, dialog_index, msg_body, msg_len, -1);
                                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "info_received_proc() route_srv_msg_add:i=%d \r\n", i);
                                return 0;
                            }
                        }
                    }
                }
            }
        }
    }

    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Info Message Proc failed:caller_id=%s, caller_ip=%s, caller_port=%d, dialog_index=%d, Cause=%s", caller_id, caller_ip, caller_port, dialog_index, (char*)"No Matching Thread Found To Put");
    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Info��Ϣ����ʧ��:caller_id=%s, caller_ip=%s, caller_port=%d, callee_id=%s, dialog_index=%d, ԭ��=%s", caller_id, caller_ip, caller_port, callee_id, dialog_index, (char*)"�Ҳ�����Ӧ�Ĵ����߳�");

    DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "info_received_proc() exit---: No Matching Thread Found To Put \r\n");
    return -1;
}

/*****************************************************************************
 �� �� ��  : info_response_received_proc
 ��������  : �յ�info��Ӧ��Ϣ�Ĵ�����
 �������  : char* caller_id
             char* callee_id
             char* call_id
             int status_code
             int user_data
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��10��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int info_response_received_proc(char* caller_id, char* callee_id, char* call_id, int status_code, int user_data)
{
    if (0 == cms_run_status)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "info_response_received_proc() exit---: CMS Not Run Normal Error \r\n");
        return -1;
    }

    return 0;
}

/*****************************************************************************
 �� �� ��  : ua_session_expires_proc
 ��������  : UA �Ự��ʱ�Ĵ�����
 �������  : int dialog_index
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��9��3�� ���ڶ�
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int ua_session_expires_proc(int dialog_index)
{
    int iRet = 0;

    if (0 == cms_run_status)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "ua_session_expires_proc() exit---: CMS Not Run Normal Error \r\n");
        return -1;
    }

    DEBUG_TRACE(MODULE_COMMON, LOG_INFO,  "ua_session_expires_proc() \
    \r\n In Param: \
    \r\n dialog_index=%d \
    \r\n", dialog_index);

    //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "ua_session_expires_proc() Enter------------: dialog_index=%d \r\n", dialog_index);
    iRet = StopAllServiceTaskByCallerUAIndex(dialog_index);
    iRet = StopAllServiceTaskByCalleeUAIndex(dialog_index);
    DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "ua_session_expires_proc() Exit------------: dialog_index=%d, iRet=%d \r\n", dialog_index, iRet);

    return 0;
}

/*****************************************************************************
 �� �� ��  : dbg_printf_proc
 ��������  : ���Դ�ӡ����
 �������  : int iLevel
             const char* FILENAME
             const char* FUNCTIONNAME
             int CODELINE
             const char* fmt
             ...
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��10��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void dbg_printf_proc(int iLevel, const char* FILENAME, const char* FUNCTIONNAME, int CODELINE, const char* fmt)
{
    if (0 == cms_run_status)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "dbg_printf_proc() exit---: CMS Not Run Normal Error \r\n");
        return;
    }

    DebugTrace(MODULE_SIPSTACK, iLevel, FILENAME, FUNCTIONNAME, CODELINE, fmt);
    return;
}

/*****************************************************************************
 �� �� ��  : sip_message_trace_proc
 ��������  : SIP��Ϣ���Ը���
 �������  : int type:
             0,��ȷ��
             1:���ʹ����
             2:���ս��������
             3:������Ϣ�����
             4:���մ�����������
             int iDirect:
             0:���͵�
             1:���յ�
             char* ipaddr
             int port
             char* msg
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��9��4�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void sip_message_trace_proc(int type, int iDirect, char* ipaddr, int port, char* msg)
{
    if (0 == cms_run_status)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "sip_message_trace_proc() exit---: CMS Not Run Normal Error \r\n");
        return;
    }

    DebugSIPMessage(type, iDirect, ipaddr, port, msg);
    return;
}

/*****************************************************************************
 �� �� ��  : SetCallbackProcFunc
 ��������  : �����ϲ�ص�������
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��10��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void SetCallbackProcFunc()
{
    app_set_uas_register_received_cb(&uas_register_received_proc);
    app_set_uas_register_received_timeout_cb(&uas_register_received_timeout_proc);
    app_set_uac_register_response_received_cb(&uac_register_response_received_proc, 0);
    app_set_invite_received_cb(&invite_received_proc, 0);
    app_set_invite_response_received_cb(&invite_response_received_proc, 0);
    app_set_cancel_received_cb(&cancel_received_proc, 0);
    app_set_ack_received_cb(&ack_received_proc, 0);
    app_set_bye_received_cb(&bye_received_proc, 0);
    app_set_bye_response_received_cb(&bye_response_received_proc, 0);
    app_set_message_received_cb(&message_received_proc, 0);
    app_set_message_response_received_cb(&message_response_received_proc, 0);
    app_set_subscribe_received_cb(&subscribe_received_proc, 0);
    app_set_subscribe_response_received_cb(&subscribe_response_received_proc, 0);
    app_set_subscribe_within_dialog_received_cb(&subscribe_within_dialog_received_proc);
    app_set_subscribe_within_dialog_response_received_cb(&subscribe_within_dialog_response_received_proc);
    app_set_notify_received_cb(&notify_received_proc, 0);
    app_set_notify_response_received_cb(&notify_response_received_proc, 0);
    app_set_info_received_cb(&info_received_proc, 0);
    app_set_info_response_received_cb(&info_response_received_proc, 0);
    app_set_ua_session_expires_cb(&ua_session_expires_proc);
    app_set_dbg_printf_cb(&dbg_printf_proc);
    app_set_sip_message_trace_cb(&sip_message_trace_proc);
    return;
}

/*****************************************************************************
 �� �� ��  : message_get_service_id_proc
 ��������  : ��ȡ������ID���û�ID
 �������  : char* caller_id
             char* caller_ip
             int caller_port
             char* callee_id
             char* callee_ip
             int callee_port
             char* msg_body
             int msg_body_len
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��8��26�� ����һ
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int message_get_service_id_proc(char* caller_id, char* caller_ip, int caller_port, char* callee_id, char* callee_ip, int callee_port, char* msg_body, int msg_body_len)
{
    int i = 0;
    int iRet = 0;
    char strSN[32] = {0};
    char strServerIP[MAX_IP_LEN] = {0};
    char strUserName[36] = {0};
    char strServerID[36] = {0};

    xml_type_t xml_type = XML_TYPE_NULL;
    CPacket inPacket;
    vector<string> NodeName_Vector;
    CPacket outPacket;
    DOMElement* AccNode = NULL;
    char strErrorCode[32] = {0};

    if ((NULL == caller_id) || (NULL == caller_ip) || (caller_port <= 0))
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "message_get_service_id_proc() exit---: User Info Error \r\n");
        return -1;
    }

    if ((NULL == callee_id) || (NULL == callee_ip) || (callee_port <= 0))
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "message_get_service_id_proc() exit---: Callee Info Error \r\n");
        return -1;
    }

    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "message_get_service_id_proc() Enter---: caller_id=%s, caller_ip=%s, caller_port=%d \r\n", caller_id, caller_ip, caller_port);
    //printf("\r\n ********** message_get_service_id_proc() Enter---: caller_id=%s,user_ip=%s,user_port=%d \r\n", caller_id, pUserInfo->login_ip, pUserInfo->login_port);
    //printf_system_time();
    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�豸�����û���¼ǰ��ʼ��ȡ������ID����:����IP��ַ=%s, �˿ں�=%d, ������IP=%s", caller_ip, caller_port, callee_ip);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Before the user log in,start to get server ID and user ID:Caller IP=%s, Port=%d, Server IP=%s", caller_ip, caller_port, callee_ip);
    //����XML
    iRet = inPacket.BuiltTree(msg_body, msg_body_len);//����DOM���ṹ.

    if (iRet < 0)
    {
        /* �ظ���Ӧ,�齨��Ϣ */
        outPacket.SetRootTag("Response");

        AccNode = outPacket.CreateElement((char*)"CmdType");
        outPacket.SetElementValue(AccNode, (char*)"GetServerID");

        AccNode = outPacket.CreateElement((char*)"SN");
        outPacket.SetElementValue(AccNode, strSN);

        AccNode = outPacket.CreateElement((char*)"Result");
        outPacket.SetElementValue(AccNode, (char*)"Error");

        AccNode = outPacket.CreateElement((char*)"ErrCode");
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_XML_BUILD_TREE_ERROR);
        outPacket.SetElementValue(AccNode, strErrorCode);

        AccNode = outPacket.CreateElement((char*)"Reason");
        outPacket.SetElementValue(AccNode, (char*)"XML Build Tree Error");

        i = SIP_SendMessage(NULL, caller_id, callee_id, callee_ip, callee_port, caller_ip, caller_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

        if (i != 0)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "message_get_service_id_proc() SIP_SendMessage Error:user_id=%s, user_ip=%s, user_port=%d \r\n", caller_id, caller_ip, caller_port);
        }
        else
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "message_get_service_id_proc() SIP_SendMessage OK:user_id=%s, user_ip=%s, user_port=%d \r\n", caller_id, caller_ip, caller_port);
        }

        SystemLog(EV9000_CMS_GET_SERVERID_ERROR, EV9000_LOG_LEVEL_ERROR, "�豸�����û���¼ǰ��ʼ��ȡ������ID����ʧ��:����IP��ַ=%s, �˿ں�=%d, ԭ��=%s", caller_ip, caller_port, (char*)"XML����ʧ��");
        EnSystemLog(EV9000_CMS_GET_SERVERID_ERROR, EV9000_LOG_LEVEL_ERROR, "Before the user log in,start to get server ID and user ID failed:user IP:%s , port=%d , reason= %s", caller_ip, caller_port, (char*)"XML parsing failed.");
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR,  "message_get_service_id_proc() exit---: XML Build Tree Error \r\nmsg=%s \r\n", msg_body);
        return iRet;
    }

    NodeName_Vector.clear();
    DOMDocument* pDOMDocument = inPacket.GetDOMDocument();
    DOMElement* pDOMElement = pDOMDocument->get_root();
    pDOMElement->ClearNodeNumber();

    if (pDOMElement->GetNodeName(NodeName_Vector) <= 0)
    {
        /* �ظ���Ӧ,�齨��Ϣ */
        outPacket.SetRootTag("Response");

        AccNode = outPacket.CreateElement((char*)"CmdType");
        outPacket.SetElementValue(AccNode, (char*)"GetServerID");

        AccNode = outPacket.CreateElement((char*)"SN");
        outPacket.SetElementValue(AccNode, strSN);

        AccNode = outPacket.CreateElement((char*)"Result");
        outPacket.SetElementValue(AccNode, (char*)"Error");

        AccNode = outPacket.CreateElement((char*)"ErrCode");
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_XML_GET_NODE_ERROR);
        outPacket.SetElementValue(AccNode, strErrorCode);

        AccNode = outPacket.CreateElement((char*)"Reason");
        outPacket.SetElementValue(AccNode, (char*)"Get Node Name Error");

        i = SIP_SendMessage(NULL, caller_id, callee_id, callee_ip, callee_port, caller_ip, caller_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

        if (i != 0)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "message_get_service_id_proc() SIP_SendMessage Error:user_id=%s, user_ip=%s, user_port=%d \r\n", caller_id, caller_ip, caller_port);
        }
        else
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "message_get_service_id_proc() SIP_SendMessage OK:user_id=%s, user_ip=%s, user_port=%d \r\n", caller_id, caller_ip, caller_port);
        }

        SystemLog(EV9000_CMS_GET_SERVERID_ERROR, EV9000_LOG_LEVEL_ERROR, "�豸�����û���¼ǰ��ʼ��ȡ������ID����ʧ��:����IP��ַ=%s, �˿ں�=%d, ԭ��=%s", caller_ip, caller_port, (char*)"XML����ʧ��");
        EnSystemLog(EV9000_CMS_GET_SERVERID_ERROR, EV9000_LOG_LEVEL_ERROR, "Before the user log in,start to get server ID and user ID failed:user IP:%s , port=%d , reason= %s", caller_ip, caller_port, (char*)"XML parsing failed.");
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "message_get_service_id_proc() exit---: Get Node Name Error \r\n");
        return -1;
    }

    /* ������xml����Ϣ���� */
    xml_type = get_xml_type_from_xml_body(NodeName_Vector, inPacket);

    if (XML_QUERY_SERVERID == xml_type) /* ��ѯ���� */
    {
        /* ȡ������*/
        inPacket.GetElementValue((char*)"SN", strSN);
        inPacket.GetElementValue((char*)"ServerIP", strServerIP);
        inPacket.GetElementValue((char*)"UserName", strUserName);

        DEBUG_TRACE(MODULE_COMMON, LOG_INFO,  "message_get_service_id_proc() \
    \r\n XML Para: \
    \r\n SN=%s, ServerIP=%s, UserName=%s \r\n", strSN, strServerIP, strUserName);

        if (pGblconf->board_id[0] == '\0' || pGblconf->center_code[0] == '\0' || pGblconf->trade_code[0] == '\0')
        {
            /* �ظ���Ӧ,�齨��Ϣ */
            outPacket.SetRootTag("Response");

            AccNode = outPacket.CreateElement((char*)"CmdType");
            outPacket.SetElementValue(AccNode, (char*)"GetServerID");

            AccNode = outPacket.CreateElement((char*)"SN");
            outPacket.SetElementValue(AccNode, strSN);

            AccNode = outPacket.CreateElement((char*)"Result");
            outPacket.SetElementValue(AccNode, (char*)"Error");

            AccNode = outPacket.CreateElement((char*)"ErrCode");
            memset(strErrorCode, 0, 32);
            snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_LOCAL_CMS_ID_ERROR);
            outPacket.SetElementValue(AccNode, strErrorCode);

            AccNode = outPacket.CreateElement((char*)"Reason");
            outPacket.SetElementValue(AccNode, (char*)"CMS ID Not Config Or Not Right");

            i = SIP_SendMessage(NULL, caller_id, callee_id, callee_ip, callee_port, caller_ip, caller_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

            if (i != 0)
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "message_get_service_id_proc() SIP_SendMessage Error:user_id=%s, user_ip=%s, user_port=%d \r\n", caller_id, caller_ip, caller_port);
            }
            else
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "message_get_service_id_proc() SIP_SendMessage OK:user_id=%s, user_ip=%s, user_port=%d \r\n", caller_id, caller_ip, caller_port);
            }

            SystemLog(EV9000_CMS_GET_SERVERID_ERROR, EV9000_LOG_LEVEL_ERROR, "�豸�����û���¼ǰ��ʼ��ȡ������ID����ʧ��:����IP��ַ=%s, �˿ں�=%d, ԭ��=%s", caller_ip, caller_port, (char*)"������IDû�����û����ô���");
            EnSystemLog(EV9000_CMS_GET_SERVERID_ERROR, EV9000_LOG_LEVEL_ERROR, "Before the user log in,start to get server ID and user ID failed:user IP=%s, port=%d , reason= %s", caller_ip, caller_port, (char*)"Server ID is not configured or configuration errors.");
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "message_get_service_id_proc() exit---: CMS ID Not Config Or Not Right \r\n");
            return -1;
        }

        if (strUserName[0] != '\0')
        {
            i = user_get_service_id_proc(caller_id, caller_ip, caller_port, callee_id, callee_ip, callee_port, strSN, strServerIP, strUserName, &g_DBOper);
        }
        else /* �豸����Ҫ��ȡ�����ID */
        {
            i = device_get_service_id_proc(caller_id, caller_ip, caller_port, callee_id, callee_ip, callee_port, strSN, strServerIP, &g_DBOper);
        }
    }
    else if (XML_RESPONSE_QUERY_SERVERID == xml_type) /* ��ѯ��Ӧ���� */
    {
        /* ȡ������*/
        inPacket.GetElementValue((char*)"SN", strSN);
        inPacket.GetElementValue((char*)"ServerID", strServerID);

        if (!sys_show_code_flag_get()) /* �ǹ���ģʽ */
        {
            i = route_get_service_id_response_proc(caller_id, caller_ip, caller_port, callee_id, callee_ip, callee_port, strSN, strServerID, &g_DBOper);
        }
        else
        {
            SystemLog(EV9000_CMS_GET_SERVERID_ERROR, EV9000_LOG_LEVEL_ERROR, "ϵͳ���ڹ���ģʽ״̬, ���Ի�ȡ�ϼ�CMS ID������Ϣ���д���:�ϼ�IP��ַ=%s, �˿ں�=%d, ���صķ�����ID=%s", caller_ip, caller_port, strServerID);
            EnSystemLog(EV9000_CMS_GET_SERVERID_ERROR, EV9000_LOG_LEVEL_ERROR, " The system is in GB mode state, does not process the message returned by superior CMS: superior IP address=%s, port=%d, return service ID=%s", caller_ip, caller_port, strServerID);
        }
    }
    else
    {
        /* �ظ���Ӧ,�齨��Ϣ */
        outPacket.SetRootTag("Response");

        AccNode = outPacket.CreateElement((char*)"CmdType");
        outPacket.SetElementValue(AccNode, (char*)"GetServerID");

        AccNode = outPacket.CreateElement((char*)"SN");
        outPacket.SetElementValue(AccNode, strSN);

        AccNode = outPacket.CreateElement((char*)"Result");
        outPacket.SetElementValue(AccNode, (char*)"Error");

        AccNode = outPacket.CreateElement((char*)"ErrCode");
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_XML_GET_MSG_TYPE_ERROR);
        outPacket.SetElementValue(AccNode, strErrorCode);

        AccNode = outPacket.CreateElement((char*)"Reason");
        outPacket.SetElementValue(AccNode, (char*)"Get XML Type Error");

        i = SIP_SendMessage(NULL, caller_id, callee_id, callee_ip, callee_port, caller_ip, caller_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

        if (i != 0)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "message_get_service_id_proc() SIP_SendMessage Error:user_id=%s, user_ip=%s, user_port=%d \r\n", caller_id, caller_ip, caller_port);
        }
        else
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "message_get_service_id_proc() SIP_SendMessage OK:user_id=%s, user_ip=%s, user_port=%d \r\n", caller_id, caller_ip, caller_port);
        }

        SystemLog(EV9000_CMS_GET_SERVERID_ERROR, EV9000_LOG_LEVEL_ERROR, "�豸�����û���¼ǰ��ʼ��ȡ������ID����ʧ��:����IP��ַ=%s, �˿ں�=%d, ԭ��=%s", caller_ip, caller_port, (char*)"XML��������ʧ��");
        EnSystemLog(EV9000_CMS_GET_SERVERID_ERROR, EV9000_LOG_LEVEL_ERROR, "Before the user log in,start to get server ID and user ID failed:user IP:%s , port=%d , reason= %s", caller_ip, caller_port, (char*)"XML command type failed.");
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "message_get_service_id_proc() exit---: Message Type Error \r\n");
        return -1;
    }

    return i;
}
#endif
