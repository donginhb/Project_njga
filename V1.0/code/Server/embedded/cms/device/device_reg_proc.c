
/*----------------------------------------------*
 * ����ͷ�ļ�                                   *
 *----------------------------------------------*/
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

#include "common/gbldef.inc"
#include "common/db_proc.h"
#include "common/gblfunc_proc.inc"
#include "common/log_proc.inc"

#include "device/device_reg_proc.inc"
#include "device/device_thread_proc.inc"

#include "user/user_srv_proc.inc"
#include "service/alarm_proc.inc"

/*----------------------------------------------*
 * �ⲿ����˵��                                 *
 *----------------------------------------------*/
extern gbl_conf_t* pGblconf;              /* ȫ��������Ϣ */
extern BOARD_NET_ATTR  g_BoardNetConfig;
extern unsigned int g_RegistrationLimit;  /* ע���豸������ */

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
GBDevice_reg_msg_queue g_GBDeviceRegMsgQueue;   /* ��׼�����豸ע����Ϣ���� */
GBDevice_reg_msg_queue g_GBDeviceUnRegMsgQueue; /* ��׼�����豸ע����Ϣ���� */
#ifdef MULTI_THR
osip_mutex_t* g_GBDeviceRegMsgQueueLock = NULL;
#endif

/*----------------------------------------------*
 * ��������                                     *
 *----------------------------------------------*/

/*----------------------------------------------*
 * �궨��                                       *
 *----------------------------------------------*/

#if DECS("��׼�����豸ע����Ϣ����")
/*****************************************************************************
 �� �� ��  : GBDevice_reg_msg_init
 ��������  : ��׼�����豸ע����Ϣ�ṹ��ʼ��
 �������  : GBDevice_reg_msg_t ** GBDevice_reg_msg
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��11��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int GBDevice_reg_msg_init(GBDevice_reg_msg_t** GBDevice_reg_msg)
{
    *GBDevice_reg_msg = (GBDevice_reg_msg_t*)osip_malloc(sizeof(GBDevice_reg_msg_t));

    if (*GBDevice_reg_msg == NULL)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "GBDevice_reg_msg_init() exit---: *GBDevice_reg_msg Smalloc Error \r\n");
        return -1;
    }

    (*GBDevice_reg_msg)->register_id[0] = '\0';
    (*GBDevice_reg_msg)->device_type = 0;
    (*GBDevice_reg_msg)->login_ip[0] = '\0';
    (*GBDevice_reg_msg)->login_port = 0;
    (*GBDevice_reg_msg)->register_name[0] = '\0';
    (*GBDevice_reg_msg)->expires = 0;
    (*GBDevice_reg_msg)->reg_info_index = -1;

    return 0;
}

/*****************************************************************************
 �� �� ��  : GBDevice_reg_msg_free
 ��������  : ��׼�����豸ע����Ϣ�ṹ�ͷ�
 �������  : GBDevice_reg_msg_t * GBDevice_reg_msg
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��11��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void GBDevice_reg_msg_free(GBDevice_reg_msg_t* GBDevice_reg_msg)
{
    if (GBDevice_reg_msg == NULL)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "GBDevice_reg_msg_free() exit---: Param Error \r\n");
        return;
    }

    memset(GBDevice_reg_msg->register_id, 0, MAX_ID_LEN + 4);
    GBDevice_reg_msg->device_type = 0;
    memset(GBDevice_reg_msg->login_ip, 0, MAX_IP_LEN);
    GBDevice_reg_msg->login_port = 0;
    memset(GBDevice_reg_msg->register_name, 0, MAX_128CHAR_STRING_LEN + 4);
    GBDevice_reg_msg->expires = 0;
    GBDevice_reg_msg->reg_info_index = -1;
    return;
}

/*****************************************************************************
 �� �� ��  : GBDevice_reg_msg_list_init
 ��������  : ��׼�����豸ע����Ϣ���г�ʼ��
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��11��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int GBDevice_reg_msg_list_init()
{
    g_GBDeviceRegMsgQueue.clear();
    g_GBDeviceUnRegMsgQueue.clear();

#ifdef MULTI_THR
    /* init smutex */
    g_GBDeviceRegMsgQueueLock = (osip_mutex_t*)osip_mutex_init();

    if (NULL == g_GBDeviceRegMsgQueueLock)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "GBDevice_reg_msg_list_init() exit---: GBDevice Register Message List Lock Init Error \r\n");
        return -1;
    }

#endif
    return 0;
}

/*****************************************************************************
 �� �� ��  : GBDevice_reg_msg_list_free
 ��������  : ��׼�����豸ע����Ϣ�����ͷ�
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��11��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void GBDevice_reg_msg_list_free()
{
    GBDevice_reg_msg_t* pGBDeviceRegMsg = NULL;

    while (!g_GBDeviceRegMsgQueue.empty())
    {
        pGBDeviceRegMsg = (GBDevice_reg_msg_t*) g_GBDeviceRegMsgQueue.front();
        g_GBDeviceRegMsgQueue.pop_front();

        if (NULL != pGBDeviceRegMsg)
        {
            GBDevice_reg_msg_free(pGBDeviceRegMsg);
            osip_free(pGBDeviceRegMsg);
            pGBDeviceRegMsg = NULL;
        }
    }

    g_GBDeviceRegMsgQueue.clear();

    while (!g_GBDeviceUnRegMsgQueue.empty())
    {
        pGBDeviceRegMsg = (GBDevice_reg_msg_t*) g_GBDeviceUnRegMsgQueue.front();
        g_GBDeviceUnRegMsgQueue.pop_front();

        if (NULL != pGBDeviceRegMsg)
        {
            GBDevice_reg_msg_free(pGBDeviceRegMsg);
            osip_free(pGBDeviceRegMsg);
            pGBDeviceRegMsg = NULL;
        }
    }

    g_GBDeviceUnRegMsgQueue.clear();

#ifdef MULTI_THR

    if (NULL != g_GBDeviceRegMsgQueueLock)
    {
        osip_mutex_destroy((struct osip_mutex*)g_GBDeviceRegMsgQueueLock);
        g_GBDeviceRegMsgQueueLock = NULL;
    }

#endif

    return;
}

/*****************************************************************************
 �� �� ��  : GBDevice_reg_msg_list_clean
 ��������  : ��׼�����豸ע����Ϣ�������
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2017��3��8��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
void GBDevice_reg_msg_list_clean()
{
    GBDevice_reg_msg_t* pGBDeviceRegMsg = NULL;

    while (!g_GBDeviceRegMsgQueue.empty())
    {
        pGBDeviceRegMsg = (GBDevice_reg_msg_t*) g_GBDeviceRegMsgQueue.front();
        g_GBDeviceRegMsgQueue.pop_front();

        if (NULL != pGBDeviceRegMsg)
        {
            GBDevice_reg_msg_free(pGBDeviceRegMsg);
            osip_free(pGBDeviceRegMsg);
            pGBDeviceRegMsg = NULL;
        }
    }

    g_GBDeviceRegMsgQueue.clear();

    return;
}

/*****************************************************************************
 �� �� ��  : GBDevice_unreg_msg_list_clean
 ��������  : ��׼�����豸ע����Ϣ�������
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2017��3��16��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
void GBDevice_unreg_msg_list_clean()
{
    GBDevice_reg_msg_t* pGBDeviceRegMsg = NULL;

    while (!g_GBDeviceUnRegMsgQueue.empty())
    {
        pGBDeviceRegMsg = (GBDevice_reg_msg_t*) g_GBDeviceUnRegMsgQueue.front();
        g_GBDeviceUnRegMsgQueue.pop_front();

        if (NULL != pGBDeviceRegMsg)
        {
            GBDevice_reg_msg_free(pGBDeviceRegMsg);
            osip_free(pGBDeviceRegMsg);
            pGBDeviceRegMsg = NULL;
        }
    }

    g_GBDeviceUnRegMsgQueue.clear();

    return;
}

/*****************************************************************************
 �� �� ��  : GBDevice_reg_msg_add
 ��������  : ��ӱ�׼�����豸ע����Ϣ��������
 �������  : char* device_id
                            int device_type
                            char* login_ip
                            int login_port
                            char* register_name
                            int expires
                            int reg_info_index
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��11��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int GBDevice_reg_msg_add(char* device_id, int device_type, char* login_ip, int login_port, char* register_name, int expires, int reg_info_index, int link_type)
{
    GBDevice_reg_msg_t* pGBDeviceRegMsg = NULL;
    int iRet = 0;

    if (device_id == NULL)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "GBDevice_reg_msg_add() exit---: Param Error \r\n");
        return -1;
    }

    iRet = GBDevice_reg_msg_init(&pGBDeviceRegMsg);

    if (iRet != 0)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GBDevice_reg_msg_add() exit---: Message Init Error \r\n");
        return -1;
    }

    if ('\0' != device_id[0])
    {
        osip_strncpy(pGBDeviceRegMsg->register_id, device_id, MAX_ID_LEN);
    }

    pGBDeviceRegMsg->device_type = device_type;

    if ('\0' != login_ip[0])
    {
        osip_strncpy(pGBDeviceRegMsg->login_ip, login_ip, MAX_IP_LEN);
    }

    pGBDeviceRegMsg->login_port = login_port;

    if (NULL != register_name && '\0' != register_name[0])
    {
        osip_strncpy(pGBDeviceRegMsg->register_name, register_name, MAX_128CHAR_STRING_LEN);
    }
    else
    {
        osip_strncpy(pGBDeviceRegMsg->register_name, device_id, MAX_128CHAR_STRING_LEN);
    }

    pGBDeviceRegMsg->expires = expires;
    pGBDeviceRegMsg->reg_info_index = reg_info_index;
    pGBDeviceRegMsg->link_type = link_type;

#if 0
    CMS_GBL_SMUTEX_LOCK((struct osip_mutex*)g_GBDeviceRegMsgQueueLock);
#endif

    if (expires <= 0)
    {
        g_GBDeviceUnRegMsgQueue.push_back(pGBDeviceRegMsg);
    }
    else
    {
        g_GBDeviceRegMsgQueue.push_back(pGBDeviceRegMsg);
    }


#if 0
    CMS_GBL_SMUTEX_UNLOCK((struct osip_mutex*)g_GBDeviceRegMsgQueueLock);
#endif

    return 0;
}

/*****************************************************************************
 �� �� ��  : scan_GBDevice_reg_msg_list
 ��������  : ɨ���豸ע����Ϣ����
 �������  : DBOper* pDevice_Reg_dboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��5��23�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void scan_GBDevice_reg_msg_list(DBOper* pDevice_Reg_dboper)
{
    int iRet = 0;
    GBDevice_reg_msg_t* pGBDeviceRegMsg = NULL;

#if 0
    CMS_GBL_SMUTEX_LOCK((struct osip_mutex*)g_GBDeviceRegMsgQueueLock);
#endif

    while (!g_GBDeviceRegMsgQueue.empty())
    {
        pGBDeviceRegMsg = (GBDevice_reg_msg_t*) g_GBDeviceRegMsgQueue.front();
        g_GBDeviceRegMsgQueue.pop_front();

        if (NULL != pGBDeviceRegMsg)
        {
            break;
        }
    }

#if 0
    CMS_GBL_SMUTEX_UNLOCK((struct osip_mutex*)g_GBDeviceRegMsgQueueLock);
#endif

    if (NULL != pGBDeviceRegMsg)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_INFO,  "scan_GBDevice_reg_msg_list() \
    \r\n In Param: \
    \r\n register_id=%s \
    \r\n login_ip=%s \
    \r\n login_port=%d \
    \r\n register_name=%s \
    \r\n reg_info_index=%d \
    \r\n expires=%d \
    \r\n ", pGBDeviceRegMsg->register_id, pGBDeviceRegMsg->login_ip, pGBDeviceRegMsg->login_port, pGBDeviceRegMsg->register_name, pGBDeviceRegMsg->reg_info_index, pGBDeviceRegMsg->expires);

        iRet = GBDevice_reg_msg_proc(pGBDeviceRegMsg, pDevice_Reg_dboper);
        GBDevice_reg_msg_free(pGBDeviceRegMsg);
        osip_free(pGBDeviceRegMsg);
        pGBDeviceRegMsg = NULL;
    }

    return;
}

/*****************************************************************************
 �� �� ��  : scan_GBDevice_unreg_msg_list
 ��������  : ɨ���豸ע����Ϣ����
 �������  : DBOper* pDevice_Reg_dboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2017��3��16��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
void scan_GBDevice_unreg_msg_list(DBOper* pDevice_Reg_dboper)
{
    int iRet = 0;
    GBDevice_reg_msg_t* pGBDeviceRegMsg = NULL;

#if 0
    CMS_GBL_SMUTEX_LOCK((struct osip_mutex*)g_GBDeviceRegMsgQueueLock);
#endif

    while (!g_GBDeviceUnRegMsgQueue.empty())
    {
        pGBDeviceRegMsg = (GBDevice_reg_msg_t*) g_GBDeviceUnRegMsgQueue.front();
        g_GBDeviceUnRegMsgQueue.pop_front();

        if (NULL != pGBDeviceRegMsg)
        {
            break;
        }
    }

#if 0
    CMS_GBL_SMUTEX_UNLOCK((struct osip_mutex*)g_GBDeviceRegMsgQueueLock);
#endif

    if (NULL != pGBDeviceRegMsg)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_INFO,  "scan_GBDevice_unreg_msg_list() \
    \r\n In Param: \
    \r\n register_id=%s \
    \r\n login_ip=%s \
    \r\n login_port=%d \
    \r\n register_name=%s \
    \r\n reg_info_index=%d \
    \r\n expires=%d \
    \r\n ", pGBDeviceRegMsg->register_id, pGBDeviceRegMsg->login_ip, pGBDeviceRegMsg->login_port, pGBDeviceRegMsg->register_name, pGBDeviceRegMsg->reg_info_index, pGBDeviceRegMsg->expires);

        iRet = GBDevice_unreg_msg_proc(pGBDeviceRegMsg, pDevice_Reg_dboper);
        GBDevice_reg_msg_free(pGBDeviceRegMsg);
        osip_free(pGBDeviceRegMsg);
        pGBDeviceRegMsg = NULL;
    }

    return;
}
#endif

/*****************************************************************************
 �� �� ��  : GetDevCfg
 ��������  : ��ȡ���ݿ��豸����
 �������  : DBOper* pdboper
             string strDevID
             GBDevice_cfg_t& GBDevice_cfg
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��11��26�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int GetDevCfg(DBOper* pdboper, string strDevID, GBDevice_cfg_t& GBDevice_cfg)
{
    string strSQL = "";
    int record_count = 0;

    if (NULL == pdboper)
    {
        return -1;
    }

    strSQL.clear();
    strSQL = "select * from GBPhyDeviceConfig WHERE DeviceID like '";
    strSQL += strDevID + "';";
    record_count = pdboper->DB_Select(strSQL.c_str(), 1);

    if (record_count < 0)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GetDevCfg() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GetDevCfg() ErrorMsg=%s\r\n", pdboper->GetLastDbErrorMsg());
        return -1;
    }
    else if (record_count == 0)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_WARN, "GetDevCfg() exit---: No Record Count \r\n");
        return 0;
    }

    int tmp_ivalue = 0;
    string tmp_svalue = "";

    /* �豸����*/
    tmp_ivalue = 0;
    pdboper->GetFieldValue("ID", tmp_ivalue);

    GBDevice_cfg.id = tmp_ivalue;
    //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "GetDevCfg() GBDevice_cfg.id:%d", GBDevice_cfg.id);


    /* �豸ͳһ��� */
    tmp_svalue.clear();
    pdboper->GetFieldValue("DeviceID", tmp_svalue);

    if (!tmp_svalue.empty())
    {
        GBDevice_cfg.device_id = tmp_svalue;
        //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "GetDevCfg() GBDevice_cfg.device_id:%s", (char*)GBDevice_cfg.device_id.c_str());
    }

    /* �Ƿ�����*/
    tmp_ivalue = 0;
    pdboper->GetFieldValue("Enable", tmp_ivalue);

    GBDevice_cfg.enable = tmp_ivalue;
    //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "GetDevCfg() GBDevice_cfg.enable:%d", GBDevice_cfg.enable);


    /* �豸����(ǰ���������NVR������CMS��TSU) */
    tmp_ivalue = 0;
    pdboper->GetFieldValue("DeviceType", tmp_ivalue);

    GBDevice_cfg.device_type = tmp_ivalue;
    //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "GetDevCfg() GBDevice_cfg.device_type:%d", GBDevice_cfg.device_type);


    /* ע���˺� */
    tmp_svalue.clear();
    pdboper->GetFieldValue("UserName", tmp_svalue);

    if (!tmp_svalue.empty())
    {
        GBDevice_cfg.register_account = tmp_svalue;
        //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "GetDevCfg() GBDevice_cfg.register_account:%s", (char*)GBDevice_cfg.register_account.c_str());
    }

    /* ע������ */
    tmp_svalue.clear();
    pdboper->GetFieldValue("Password", tmp_svalue);

    if (!tmp_svalue.empty())
    {
        GBDevice_cfg.register_password = tmp_svalue;
        //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "GetDevCfg() GBDevice_cfg.register_password:%s", (char*)GBDevice_cfg.register_password.c_str());
    }

    /* �豸ip��ַ */
    tmp_svalue.clear();
    pdboper->GetFieldValue("DeviceIP", tmp_svalue);

    if (!tmp_svalue.empty())
    {
        GBDevice_cfg.device_ip = tmp_svalue;
        //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "GetDevCfg() GBDevice_cfg.device_ip:%s", (char*)GBDevice_cfg.device_ip.c_str());
    }

    /* �豸��Ƶ����ͨ���� */
    tmp_ivalue = 0;
    pdboper->GetFieldValue("MaxCamera", tmp_ivalue);

    GBDevice_cfg.device_max_camera = tmp_ivalue;
    //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "GetDevCfg() GBDevice_cfg.device_max_camera:%d", GBDevice_cfg.device_max_camera);


    /* �豸��������ͨ���� */
    tmp_ivalue = 0;
    pdboper->GetFieldValue("MaxAlarm", tmp_ivalue);

    GBDevice_cfg.device_max_alarm = tmp_ivalue;
    //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "GetDevCfg() GBDevice_cfg.device_max_alarm:%d", GBDevice_cfg.device_max_alarm);


    /* �豸������ */
    tmp_svalue.clear();
    pdboper->GetFieldValue("Manufacturer", tmp_svalue);

    if (!tmp_svalue.empty())
    {
        GBDevice_cfg.device_manufacturer = tmp_svalue;
        //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "GetDevCfg() GBDevice_cfg.device_manufacturer:%s", (char*)GBDevice_cfg.device_manufacturer.c_str());
    }

    /* �豸�ͺ� */
    tmp_svalue.clear();
    pdboper->GetFieldValue("Model", tmp_svalue);

    if (!tmp_svalue.empty())
    {
        GBDevice_cfg.device_model = tmp_svalue;
        //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "GetDevCfg() GBDevice_cfg.device_model:%s", (char*)GBDevice_cfg.device_model.c_str());
    }

    /* �豸�汾 */
    tmp_svalue.clear();
    pdboper->GetFieldValue("Firmware", tmp_svalue);

    if (!tmp_svalue.empty())
    {
        GBDevice_cfg.device_firmware = tmp_svalue;
        //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "GetDevCfg() GBDevice_cfg.device_firmware:%s \r\n", (char*)GBDevice_cfg.device_firmware.c_str());
    }

    /* ��������:0:���¼���1��ͬ����Ĭ��0 */
    tmp_ivalue = 0;
    pdboper->GetFieldValue("LinkType", tmp_ivalue);

    GBDevice_cfg.link_type = tmp_ivalue;
    //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "GetDevCfg() GBDevice_cfg.link_type:%d", GBDevice_cfg.link_type);

    /* CMS ID */
    tmp_svalue.clear();
    pdboper->GetFieldValue("CMSID", tmp_svalue);

    if (!tmp_svalue.empty())
    {
        GBDevice_cfg.cms_id = tmp_svalue;
        //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "GetDevCfg() GBDevice_cfg.cms_id:%s \r\n", (char*)GBDevice_cfg.cms_id.c_str());
    }

    /* ���䷽ʽ */
    tmp_ivalue = 0;
    pdboper->GetFieldValue("TransferProtocol", tmp_ivalue);

    if (tmp_ivalue == 1)
    {
        GBDevice_cfg.trans_protocol = 1;
    }
    else
    {
        GBDevice_cfg.trans_protocol = 0;
    }

    //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "GetDevCfg() GBDevice_cfg.trans_protocol:%d", GBDevice_cfg.trans_protocol);

    return record_count;
}

/*****************************************************************************
 �� �� ��  : DevRefresh
 ��������  : �豸ˢ�´���
 �������  : GBDevice_info_t* pGBDeviceInfo
             DBOper* pdboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��11��26�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int DevRefresh(GBDevice_info_t* pGBDeviceInfo, DBOper* pdboper)
{
    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "GBDevice_reg_msg_proc() REFRESH \r\n");
    pGBDeviceInfo->auth_count = 0;   //?
    pGBDeviceInfo->reg_status = 1;
    /* ������Ӧ��Ϣ*/
    SIP_UASAnswerToRegister(pGBDeviceInfo->reg_info_index, 200, NULL);
    return 0;
}

/*****************************************************************************
 �� �� ��  : DevUnReg
 ��������  : �豸ע������
 �������  : GBDevice_info_t* pGBDeviceInfo
             DBOper* pdboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��11��26�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int DevUnReg(GBDevice_info_t* pGBDeviceInfo, DBOper* pdboper)
{
    int i = 0;
    unsigned int uType = EV9000_ALARM_ACCESS_DEVICE_ERROR;

    if (NULL == pGBDeviceInfo || NULL == pdboper)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "DevUnReg() exit---: Param Error \r\n");
        return -1;
    }

    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevUnReg() Enter---\r\n");
    printf("DevUnReg() Enter---\r\n");

    /* ������Ӧ��Ϣ*/
    SIP_UASAnswerToRegister(pGBDeviceInfo->reg_info_index, 200, NULL);

    pGBDeviceInfo->auth_count = 0;
    pGBDeviceInfo->reg_status = 0;
    UpdateGBDeviceRegStatus2DB(pGBDeviceInfo, pdboper);

    SystemLog(EV9000_CMS_DEVICE_OFFLINE, EV9000_LOG_LEVEL_ERROR, "�豸����:�豸ID=%s, IP��ַ=%s", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip);
    EnSystemLog(EV9000_CMS_DEVICE_OFFLINE, EV9000_LOG_LEVEL_ERROR, "DevUnReg:ID=%s, IP=%s", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip);
    DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "DevUnReg() REMOVE:device_id=%s, login_ip=%s, login_port=%d, reg_info_index=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, pGBDeviceInfo->reg_info_index);
    printf("DevUnReg() REMOVE:device_id=%s, login_ip=%s, login_port=%d, reg_info_index=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, pGBDeviceInfo->reg_info_index);

    /* �豸���ϱ��� */
    if (EV9000_DEVICETYPE_DECODER == pGBDeviceInfo->device_type)
    {
        uType = EV9000_ALARM_DEC_TSU_ERROR;
    }
    else if (EV9000_DEVICETYPE_VIDEODIAGNOSIS == pGBDeviceInfo->device_type)
    {
        uType = EV9000_ALARM_DIAGNOSE_ERROR;
    }
    else if (EV9000_DEVICETYPE_INTELLIGENTANALYSIS == pGBDeviceInfo->device_type)
    {
        uType = EV9000_ALARM_INTELLIGENT_ERROR;
    }
    else if (EV9000_DEVICETYPE_SIPSERVER == pGBDeviceInfo->device_type)
    {
        uType = EV9000_ALARM_CMS_ERROR;
    }

    i = SendGBPhyDeviceOffLineAlarmToAllClientUser(uType, pGBDeviceInfo->id);
    printf("DevUnReg() SendGBPhyDeviceOffLineAlarmToAllClientUser() Exit--- \r\n");

    if (0 != i)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "DevUnReg() SendGBPhyDeviceOffLineAlarmToAllClientUser Error:DeviceID=%s, iRet=%d \r\n", pGBDeviceInfo->device_id, i);
    }
    else
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevUnReg() SendGBPhyDeviceOffLineAlarmToAllClientUser OK:DeviceID=%s, iRet=%d \r\n", pGBDeviceInfo->device_id, i);
    }

    //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "DevUnReg() Enter------------: DeviceID=%s \r\n", pGBDeviceInfo->device_id);
    /* ���ҵ�λ��ҵ�񣬲�ֹͣ����ҵ�� */
    if (EV9000_DEVICETYPE_DECODER == pGBDeviceInfo->device_type) /* �������������в���Ϣֹͣҵ�� */
    {
        i = StopAllServiceTaskByCallerIPAndPort(pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        printf("DevUnReg() StopAllServiceTaskByCallerIPAndPort() Exit--- \r\n");

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "DevUnReg() StopAllServiceTaskByCallerIPAndPort Error:device_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevUnReg() StopAllServiceTaskByCallerIPAndPort OK:device_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, i);
        }
    }
    else if (EV9000_DEVICETYPE_INTELLIGENTANALYSIS == pGBDeviceInfo->device_type) /* IVS�������С����в���Ϣֹͣҵ�� */
    {
        i = StopAllServiceTaskByCallerIPAndPort(pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port); /* �����������Ƶ�� */
        printf("DevUnReg() StopAllServiceTaskByCallerIPAndPort() Exit--- \r\n");

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "DevUnReg() StopAllServiceTaskByCallerIPAndPort Error:device_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevUnReg() StopAllServiceTaskByCallerIPAndPort OK:device_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, i);
        }

        i = StopAllServiceTaskByCalleeIPAndPort(pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port); /* �������͵����ܷ���Ƶ�� */
        printf("DevUnReg() StopAllServiceTaskByCalleeIPAndPort() Exit--- \r\n");

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "DevUnReg() StopAllServiceTaskByCalleeIPAndPort Error:device_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevUnReg() StopAllServiceTaskByCalleeIPAndPort OK:device_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, i);
        }
    }
    else if (EV9000_DEVICETYPE_SIPSERVER == pGBDeviceInfo->device_type) /* �¼�CMS */
    {
        i = StopAllServiceTaskByCalleeIPAndPort(pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        printf("DevUnReg() StopAllServiceTaskByCalleeIPAndPort() Exit--- \r\n");

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "DevUnReg() StopAllServiceTaskByCalleeIPAndPort Error:device_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevUnReg() StopAllServiceTaskByCalleeIPAndPort OK:device_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, i);
        }
    }
    else
    {
        i = StopAllServiceTaskByCalleeIPAndPort(pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        printf("DevUnReg() StopAllServiceTaskByCalleeIPAndPort() Exit--- \r\n");

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "DevUnReg() StopAllServiceTaskByCalleeIPAndPort Error:device_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevUnReg() StopAllServiceTaskByCalleeIPAndPort OK:device_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, i);
        }
    }

    if (EV9000_DEVICETYPE_INTELLIGENTANALYSIS == pGBDeviceInfo->device_type)
    {
        /* ֪ͨ�ͻ��ˣ��߼��豸���ܷ������� */
        i = SendAllGBLogicDeviceIntelligentStatusOffAlarmToAllClientUser(pGBDeviceInfo->id);
        printf("DevUnReg() SendAllGBLogicDeviceIntelligentStatusOffAlarmToAllClientUser() Exit--- \r\n");

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "DevUnReg() SendAllGBLogicDeviceIntelligentStatusOffAlarmToAllClientUser Error:device_id=%s, i=%d \r\n", pGBDeviceInfo->device_id, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevUnReg() SendAllGBLogicDeviceIntelligentStatusOffAlarmToAllClientUser OK:device_id=%s, i=%d \r\n", pGBDeviceInfo->device_id, i);
        }
    }
    else
    {
        /* ֪ͨ�ͻ��ˣ��߼��豸���� */
        i = SendAllGBLogicDeviceStatusOffAlarmToAllClientUser(pGBDeviceInfo->id);
        printf("DevUnReg() SendAllGBLogicDeviceStatusOffAlarmToAllClientUser() Exit--- \r\n");

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "DevUnReg() SendAllGBLogicDeviceStatusOffAlarmToAllClientUser Error:device_id=%s, i=%d \r\n", pGBDeviceInfo->device_id, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevUnReg() SendAllGBLogicDeviceStatusOffAlarmToAllClientUser OK:device_id=%s, i=%d \r\n", pGBDeviceInfo->device_id, i);
        }
    }

    if (EV9000_DEVICETYPE_INTELLIGENTANALYSIS == pGBDeviceInfo->device_type)
    {
        /* �������ܷ����豸״̬��Ϣ���ͻ��� */
        i = SendAllIntelligentGBLogicDeviceStatusProc(pGBDeviceInfo->id, 0, pdboper);
        printf("DevUnReg() SendAllIntelligentGBLogicDeviceStatusProc() Exit--- \r\n");

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "DevUnReg() SendAllIntelligentGBLogicDeviceStatusProc Error:device_id=%s, i=%d \r\n", pGBDeviceInfo->device_id, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevUnReg() SendAllIntelligentGBLogicDeviceStatusProc OK:device_id=%s, i=%d \r\n", pGBDeviceInfo->device_id, i);
        }
    }
    else
    {
        /* �����豸״̬��Ϣ���ͻ��� */
        i = SendAllGBLogicDeviceStatusProc(pGBDeviceInfo->id, 0, 1, pdboper);
        printf("DevUnReg() SendAllGBLogicDeviceStatusProc() Exit--- \r\n");

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "DevUnReg() SendAllGBLogicDeviceStatusProc Error:device_id=%s, i=%d \r\n", pGBDeviceInfo->device_id, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevUnReg() SendAllGBLogicDeviceStatusProc OK:device_id=%s, i=%d \r\n", pGBDeviceInfo->device_id, i);
        }
    }

    if (EV9000_DEVICETYPE_INTELLIGENTANALYSIS == pGBDeviceInfo->device_type)
    {
        //�����߼��豸�����ܷ���״̬
        i = SetGBLogicDeviceIntelligentStatus(pGBDeviceInfo->id, INTELLIGENT_STATUS_NULL);
        printf("DevUnReg() SetGBLogicDeviceIntelligentStatus() Exit--- \r\n");

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "DevUnReg() SetGBLogicDeviceIntelligentStatus Error:device_id=%s, i=%d \r\n", pGBDeviceInfo->device_id, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevUnReg() SetGBLogicDeviceIntelligentStatus OK:device_id=%s, i=%d \r\n", pGBDeviceInfo->device_id, i);
        }
    }
    else
    {
        //�����߼��豸״̬
        i = SetGBLogicDeviceStatus(pGBDeviceInfo->id, 0, pdboper);
        printf("DevUnReg() SetGBLogicDeviceStatus() Exit--- \r\n");

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "DevUnReg() SetGBLogicDeviceStatus Error:device_id=%s, i=%d \r\n", pGBDeviceInfo->device_id, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevUnReg() SetGBLogicDeviceStatus OK:device_id=%s, i=%d \r\n", pGBDeviceInfo->device_id, i);
        }
    }

    /* �����豸ҵ���߳� */
    if (EV9000_DEVICETYPE_SIPSERVER == pGBDeviceInfo->device_type
        || EV9000_DEVICETYPE_MGWSERVER == pGBDeviceInfo->device_type)
    {
        i = device_srv_proc_thread_recycle(pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        printf("DevUnReg() device_srv_proc_thread_recycle() Exit--- \r\n");

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "DevUnReg() device_srv_proc_thread_recycle Error:device_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevUnReg() device_srv_proc_thread_recycle OK:device_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, i);
        }
    }

    /* ��������˽ṹ��������豸���ͣ���ô��Ҫ���������豸���״̬�����ϱ����ϼ�CMS */
    if (EV9000_DEVICETYPE_DECODER == pGBDeviceInfo->device_type
        || EV9000_DEVICETYPE_SIPSERVER == pGBDeviceInfo->device_type
        || EV9000_DEVICETYPE_MGWSERVER == pGBDeviceInfo->device_type)
    {
        /* �������˽ṹ��״̬ */
        i = UpdateTopologyPhyDeviceStatus2DB(pGBDeviceInfo->device_id, (char*)"0", pdboper);
        printf("DevUnReg() UpdateTopologyPhyDeviceStatus2DB() Exit--- \r\n");
    }

    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevUnReg() Exit---\r\n");
    printf("DevUnReg() Exit---\r\n");

    return 0;
}

/*****************************************************************************
 �� �� ��  : DevReg
 ��������  : �豸ע�ᴦ��
 �������  : GBDevice_info_t* pGBDeviceInfo
             GBDevice_cfg_t& GBDevice_cfg
             DBOper* pdboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��11��26�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int DevReg(GBDevice_info_t* pGBDeviceInfo, GBDevice_cfg_t& GBDevice_cfg, DBOper* pdboper)
{
    int i = 0;
    int device_tl_pos = 0;
    int reg_info_index = pGBDeviceInfo->reg_info_index;
    osip_authorization_t* pAuthorization = NULL;
    char* realm = NULL;
    int iAuth = 0;
    int iIsRefreshReg = 0; /* �Ƿ���ˢ��ע�� */
    char strRegisterDomain[128] = {0};
    char* call_id = NULL;
    char* server_ip = NULL;
    int server_port = 0;
    char* pcRegServerEthName = NULL;
    char strEthName[MAX_IP_LEN] = {0};
    char strDeviceType[16] = {0};
    char strStatus[16] = {0};
    char strErrorCode[32] = {0};

    if (NULL == pGBDeviceInfo || NULL == pdboper)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "DevReg() exit---: Param Error \r\n");
        return -1;
    }

    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() Enter---\r\n");
    printf("DevReg() Enter---\r\n");

    if (is_need_auth())
    {
        /* ����Ƿ�����֤��Ϣ */
        /* get authorization*/
        pAuthorization = SIP_UASGetRegisterAuthorization(reg_info_index);

        if (pAuthorization  && pAuthorization->realm)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() Check Realm: Begin---\r\n");

            realm = osip_getcopy_unquoted_string(pAuthorization->realm);
            i = IsLocalAuthRealm(realm);

            DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() Check Realm: End--- i=%d\r\n", i);

            if (0 == i)
            {
                memset(strErrorCode, 0, 32);
                snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_REG_AUTH_REALM_NOT_LOCAL_ERROR);
                SIP_UASAnswerToRegister(reg_info_index, 403, strErrorCode);
                //SIP_UASAnswerToRegister(reg_info_index, 403, (char*)"Auth Realm Is Not Local");
                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() exit---: Auth Realm Is Not Local:realm=%s \r\n", realm);
                SystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "�豸ע��ʧ��:�豸ID=%s, IP��ַ=%s, �˿ں�=%d, ԭ��=%s%s", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)"�豸��֤���Ǳ�CMS,��֤��=", realm);
                EnSystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "Device registration failed:device ID=%s, IP address=%s, port number=%d, cause=%s%s", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)"The device authentication domain is not the CMS, the authentication domain=", realm);

                osip_free(realm);
                realm = NULL;
                return -1;
            }
            else
            {
                osip_free(realm);
                realm = NULL;

                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() SIP Auth: Begin--- :register_account=%s, register_password=%s \r\n", (char*)GBDevice_cfg.register_account.c_str(), (char*)GBDevice_cfg.register_password.c_str());

                iAuth = SIP_Auth(pAuthorization, (char*)GBDevice_cfg.register_account.c_str(), (char*)GBDevice_cfg.register_password.c_str(), (char*)"REGISTER");

                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() SIP Auth: End--- iAuth=%d\r\n", iAuth);

                if (0 == iAuth) /*��֤ʧ��*/
                {
                    pGBDeviceInfo->auth_count++;

                    if (pGBDeviceInfo->auth_count > 0)
                    {
                        pGBDeviceInfo->auth_count = 0;
                        memset(strErrorCode, 0, 32);
                        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_REG_AUTH_FAILD_ERROR);
                        SIP_UASAnswerToRegister(reg_info_index, 403, strErrorCode);
                        //SIP_UASAnswerToRegister(reg_info_index, 403, NULL);
                        DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() exit---: AUTH FAIL \r\n");
                        SystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "�豸ע��ʧ��:�豸ID=%s, IP��ַ=%s, �˿ں�=%d, ԭ��=%s", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)"�豸��֤ʧ��");
                        EnSystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "Device registration failed:device ID=%s, IP address=%s, port number=%d, cause=%s", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)"Device authentication failed");
                        return -1;
                    }
                    else
                    {
                        snprintf(strRegisterDomain, 128, "%s.spvmn.cn", pGblconf->register_region);
                        SIP_UASAnswerToRegister4Auth(reg_info_index, strRegisterDomain);
                        DEBUG_TRACE(MODULE_DEVICE, LOG_WARN, "DevReg() exit---: NEED AUTH \r\n");
                        //SystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_WARNING, "�豸ע��ʧ��:�豸ID=%s, IP��ַ=%s, �˿ں�=%d, ԭ��=%s", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)"�豸��Ҫ��֤");
                        //EnSystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_WARNING, "Device registration failed:device ID=%s, IP address=%s, port number=%d, cause=%s", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)"Device need authentication");
                        return -1;
                    }
                }
                else
                {
                    /* ��ȡע��ķ�����IP��ַ���˿ںţ�ȷ���Ǵ��ĸ���ע��� */
                    server_ip = SIP_GetUASServerIP(pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);

                    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() device_id=%s, login_ip=%s, login_port=%d, server_ip=%s \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, server_ip);
                    printf("DevReg() device_id=%s, login_ip=%s, login_port=%d, server_ip=%s \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, server_ip);

                    if ((NULL == server_ip)
                        || (NULL != server_ip && 0 == sstrcmp(server_ip, local_cms_id_get()))
                        || (NULL != server_ip && 0 == sstrcmp(server_ip, pGblconf->register_region)))
                    {
                        memset(strEthName, 0, MAX_IP_LEN);
                        snprintf(strEthName, MAX_IP_LEN, "%s", pGblconf->default_eth_name);
                        
                        server_ip = local_ip_get(strEthName);
                        server_port = local_port_get(strEthName);
                        pcRegServerEthName = get_ip_eth_name(server_ip);
                    }
                    else
                    {
                        server_port = SIP_GetUASServerPort(pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                        pcRegServerEthName = get_ip_eth_name(server_ip);
                    }

                    printf("DevReg() device_id=%s, login_ip=%s, login_port=%d, server_port=%d, RegServerEthName=%s \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, server_port, pcRegServerEthName);

                    if (NULL == server_ip)
                    {
                        pGBDeviceInfo->auth_count = 0;
                        memset(strErrorCode, 0, 32);
                        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_REG_GET_SERVER_IP_ERROR);
                        SIP_UASAnswerToRegister(reg_info_index, 403, strErrorCode);
                        //SIP_UASAnswerToRegister(reg_info_index, 503, (char*)"Get Register Server IP Error");
                        DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() exit---: Get Register Server Info Error:server_ip=%s, server_port=%d, RegServerEthName=%s \r\n", server_ip, server_port, pcRegServerEthName);
                        SystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "�豸ע��ʧ��:�豸ID=%s, IP��ַ=%s, �˿ں�=%d, ԭ��=%s", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)"��ȡ�豸ע��ķ�����IP��ַ��Ϣʧ��");
                        EnSystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "Device registration failed:device ID=%s, IP address=%s, port number=%d, cause=%s", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)"Access information of server IP address for device registration failed");

                        return -1;
                    }

                    if (server_port <= 0)
                    {
                        pGBDeviceInfo->auth_count = 0;
                        memset(strErrorCode, 0, 32);
                        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_REG_GET_SERVER_PORT_ERROR);
                        SIP_UASAnswerToRegister(reg_info_index, 403, strErrorCode);
                        //SIP_UASAnswerToRegister(reg_info_index, 503, (char*)"Get Register Server Port Error");
                        DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() exit---: Get Register Server Info Error:server_ip=%s, server_port=%d, RegServerEthName=%s \r\n", server_ip, server_port, pcRegServerEthName);
                        SystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "�豸ע��ʧ��:�豸ID=%s, IP��ַ=%s, �˿ں�=%d, ԭ��=%s", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)"��ȡ�豸ע��ķ�����IP��ַ��Ϣʧ��");
                        EnSystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "Device registration failed:device ID=%s, IP address=%s, port number=%d, cause=%s", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)"Access information of server IP address for device registration failed");

                        return -1;
                    }

                    if (NULL == pcRegServerEthName)
                    {
                        pGBDeviceInfo->auth_count = 0;
                        memset(strErrorCode, 0, 32);
                        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_REG_GET_SERVER_IP_ETHNAME_ERROR);
                        SIP_UASAnswerToRegister(reg_info_index, 403, strErrorCode);
                        //SIP_UASAnswerToRegister(reg_info_index, 503, (char*)"Get Register Server EthName Error");
                        DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() exit---: Get Register Server Info Error:server_ip=%s, server_port=%d, RegServerEthName=%s \r\n", server_ip, server_port, pcRegServerEthName);
                        SystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "�豸ע��ʧ��:�豸ID=%s, IP��ַ=%s, �˿ں�=%d, ԭ��=%s", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)"��ȡ�豸ע��ķ�����IP��ַ��Ϣʧ��");
                        EnSystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "Device registration failed:device ID=%s, IP address=%s, port number=%d, cause=%s", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)"Access information of server IP address for device registration failed");

                        return -1;
                    }

                    /* ȷ���Ƿ���ˢ��ע�� */
                    call_id = SIP_GetUASCallID(pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);

                    if (NULL == call_id)
                    {
                        pGBDeviceInfo->auth_count = 0;
                        memset(strErrorCode, 0, 32);
                        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_REG_CALLID_ERROR);
                        SIP_UASAnswerToRegister(reg_info_index, 403, strErrorCode);
                        //SIP_UASAnswerToRegister(reg_info_index, 503, (char*)"Get Register CallID Error");
                        DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() exit---: Get Register CallID Error:server_ip=%s, server_port=%d, RegServerEthName=%s \r\n", server_ip, server_port, pcRegServerEthName);
                        SystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "�豸ע��ʧ��:�豸ID=%s, IP��ַ=%s, �˿ں�=%d, ԭ��=%s", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)"��ȡ�豸ע���CallID�ֶ���Ϣʧ��");
                        EnSystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "Device registration failed:device ID=%s, IP address=%s, port number=%d, cause=%s", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)"Access information of call id for device registration failed");

                        return -1;
                    }

                    if ('\0' == pGBDeviceInfo->call_id[0])
                    {
                        osip_strncpy(pGBDeviceInfo->call_id, call_id, MAX_128CHAR_STRING_LEN);
                        iIsRefreshReg = 0;
                    }
                    else
                    {
                        if (0 != sstrcmp(pGBDeviceInfo->call_id, call_id))
                        {
                            memset(pGBDeviceInfo->call_id, 0, MAX_128CHAR_STRING_LEN + 4);
                            osip_strncpy(pGBDeviceInfo->call_id, call_id, MAX_128CHAR_STRING_LEN);
                            iIsRefreshReg = 0;
                        }
                        else
                        {
                            iIsRefreshReg = 1; /* ˢ��ע�� */
                        }
                    }

                    DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "DevReg() device_id=%s, login_ip=%s, login_port=%d: old callid=%s, new callid=%s, IsRefreshReg=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, pGBDeviceInfo->call_id, call_id, iIsRefreshReg);
                    printf("DevReg() device_id=%s, login_ip=%s, login_port=%d: old callid=%s, new callid=%s, IsRefreshReg=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, pGBDeviceInfo->call_id, call_id, iIsRefreshReg);

                    /* ȷ��������IP��ַ */
                    if (pGBDeviceInfo->strRegServerIP[0] == '\0')
                    {
                        osip_strncpy(pGBDeviceInfo->strRegServerIP, server_ip, MAX_IP_LEN);
                    }
                    else
                    {
                        if (0 != sstrcmp(pGBDeviceInfo->strRegServerIP, server_ip))
                        {
                            memset(pGBDeviceInfo->strRegServerIP, 0, MAX_IP_LEN);
                            osip_strncpy(pGBDeviceInfo->strRegServerIP, server_ip, MAX_IP_LEN);
                        }
                    }

                    /* ȷ��ע��������˿ں� */
                    if (pGBDeviceInfo->iRegServerPort <= 0)
                    {
                        pGBDeviceInfo->iRegServerPort = server_port;
                    }
                    else
                    {
                        if (pGBDeviceInfo->iRegServerPort != server_port)
                        {
                            pGBDeviceInfo->iRegServerPort = server_port;
                        }
                    }

                    /* ȷ��IP��ַ�������� */
                    if (pGBDeviceInfo->strRegServerEthName[0] == '\0')
                    {
                        osip_strncpy(pGBDeviceInfo->strRegServerEthName, pcRegServerEthName, MAX_IP_LEN);
                    }
                    else
                    {
                        if (0 != sstrcmp(pGBDeviceInfo->strRegServerEthName, pcRegServerEthName))
                        {
                            memset(pGBDeviceInfo->strRegServerEthName, 0, MAX_IP_LEN);
                            osip_strncpy(pGBDeviceInfo->strRegServerEthName, pcRegServerEthName, MAX_IP_LEN);
                        }
                    }

                    pGBDeviceInfo->auth_count = 0;
                    SIP_UASAnswerToRegister(reg_info_index, 200, NULL);

                    if (pGBDeviceInfo->reg_status <= 0)
                    {
                        /* ý�����غ��¼�ƽ̨�����䵥���Ĵ����߳� */
                        if (pGBDeviceInfo->device_type == EV9000_DEVICETYPE_SIPSERVER
                            || pGBDeviceInfo->device_type == EV9000_DEVICETYPE_MGWSERVER
                            || pGBDeviceInfo->device_type == EV9000_DEVICETYPE_VIDEODIAGNOSIS
                            || pGBDeviceInfo->device_type == EV9000_DEVICETYPE_INTELLIGENTANALYSIS
                            || pGBDeviceInfo->device_type == EV9000_DEVICETYPE_DECODER)
                        {
                            /* �����豸ҵ�����߳� */
                            device_tl_pos = device_srv_proc_thread_find(pGBDeviceInfo);
                            DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() device_srv_proc_thread_find:device_tl_pos=%d \r\n", device_tl_pos);
                            printf("DevReg() device_srv_proc_thread_find:device_tl_pos=%d \r\n", device_tl_pos);

                            if (device_tl_pos < 0)
                            {
                                //���䴦���߳�
                                i = device_srv_proc_thread_assign(pGBDeviceInfo);
                                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() device_srv_proc_thread_assign:i=%d \r\n", i);
                                printf("DevReg() device_srv_proc_thread_assign:i=%d \r\n", i);

                                if (i != 0)
                                {
                                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "�����豸ҵ�����߳�ʧ��:�豸ID=%s, IP��ַ=%s, �˿ں�=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                                }
                            }
                            else
                            {
                                /* �ͷ�һ��֮ǰ���û�ҵ�����߳� */
                                i = device_srv_proc_thread_recycle(pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);

                                //���䴦���߳�
                                i = device_srv_proc_thread_assign(pGBDeviceInfo);
                                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() device_srv_proc_thread_assign:i=%d \r\n", i);
                                printf("DevReg() device_srv_proc_thread_assign:i=%d \r\n", i);

                                if (i != 0)
                                {
                                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "�����豸ҵ�����߳�ʧ��:�豸ID=%s, IP��ַ=%s, �˿ں�=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                                }
                            }
                        }

                        /*�����ڴ�*/
                        pGBDeviceInfo->reg_status = 1;
                        pGBDeviceInfo->catalog_subscribe_flag = 0;                                       /* Ŀ¼���ı�ʶ:0:û�ж��ģ�1:�Ѷ��� */
                        pGBDeviceInfo->catalog_subscribe_interval = local_register_retry_interval_get(); /* Ŀ¼����δ�ɹ������Լ��ʱ�� */
                        pGBDeviceInfo->catalog_subscribe_expires = 0;                                    /* Ŀ¼���ĳ�ʱʱ��*/
                        pGBDeviceInfo->catalog_subscribe_expires_count = 0;                              /* Ŀ¼����ʱ����� */
                        pGBDeviceInfo->catalog_subscribe_event_id = 0;                                   /* Ŀ¼�����¼����� */
                        pGBDeviceInfo->last_keep_alive_time = 0;
                        pGBDeviceInfo->keep_alive_expires = 0;
                        pGBDeviceInfo->keep_alive_expires_count = 0;
                        pGBDeviceInfo->keep_alive_count = 0;
                        pGBDeviceInfo->iGetCataLogStatus = 0;
                        pGBDeviceInfo->iGetLogicDeviceStatusCount = 0;
                        pGBDeviceInfo->iLastGetCataLogTime = 0;

                        UpdateGBDeviceRegStatus2DB(pGBDeviceInfo, pdboper);

                        /* �����߼��豸����Ϣ,�����߼��豸��ǰ���Ѿ�ɾ����״̬����ͨ����ȡ�����ϱ��ĵ� */
                        if (pGBDeviceInfo->device_type == EV9000_DEVICETYPE_IPC
                            || pGBDeviceInfo->device_type == EV9000_DEVICETYPE_DECODER
                            || (pGBDeviceInfo->device_type == EV9000_DEVICETYPE_SIPSERVER && pGBDeviceInfo->three_party_flag == 1))
                        {
                            i = SetGBLogicDeviceStatus(pGBDeviceInfo->id, 1, pdboper);

                            if (0 != i)
                            {
                                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "DevReg() SetGBLogicDeviceStatus Error:i=%d \r\n", i);
                            }
                            else
                            {
                                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() SetGBLogicDeviceStatus OK:i=%d \r\n", i);
                            }

                            /* �����豸״̬��Ϣ���ͻ��� */
                            i = SendAllGBLogicDeviceStatusProc(pGBDeviceInfo->id, 1, 1, pdboper);

                            if (0 != i)
                            {
                                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "DevReg() SendAllGBLogicDeviceStatusProc Error:i=%d \r\n", i);
                            }
                            else
                            {
                                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() SendAllGBLogicDeviceStatusProc OK:i=%d \r\n", i);
                            }
                        }

                        /* ���ͻ�ȡ�豸״̬��Ϣ */
                        //i = SendQueryDeviceStatusMessage(pGBDeviceInfo);

                        /* ���ͻ�ȡ�豸��Ϣ��Ϣ */
                        i = SendQueryDeviceInfoMessage(pGBDeviceInfo);

                        if (0 != i)
                        {
                            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "DevReg() SendQueryDeviceInfoMessage Error:i=%d \r\n", i);
                        }
                        else
                        {
                            DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() SendQueryDeviceInfoMessage OK:i=%d \r\n", i);
                        }

                        if (pGBDeviceInfo->device_type == EV9000_DEVICETYPE_SIPSERVER)  /* ƽ̨ */
                        {
                            /* ��ȡ�¼�CMS ���ݿ�IP ��ַ */
                            i = SendQuerySubCMSDBIPMessage(pGBDeviceInfo);

                            if (0 != i)
                            {
                                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "DevReg() SendQuerySubCMSDBIPMessage Error:i=%d \r\n", i);
                            }
                            else
                            {
                                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() SendQuerySubCMSDBIPMessage OK:i=%d \r\n", i);
                            }

                            /* ���ͱ���CMS������������¼�CMS */
                            i = SendNotifyRestartMessageToSubCMS(pGBDeviceInfo);

                            if (0 != i)
                            {
                                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "DevReg() SendNotifyRestartMessageToSubCMS Error:i=%d \r\n", i);
                            }
                            else
                            {
                                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() SendNotifyRestartMessageToSubCMS OK:i=%d \r\n", i);
                            }

                            /* ���͵�λ���¼�ƽ̨ */
                            i = SendNotifyCatalogMessageToSubCMS(pGBDeviceInfo->device_id);

                            if (0 != i)
                            {
                                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "DevReg() SendNotifyCatalogMessageToSubCMS Error:device_id=%s, device_ip=%s, device_port=%d, iRet=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, i);
                            }
                            else
                            {
                                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() SendNotifyCatalogMessageToSubCMS OK:device_id=%s, device_ip=%s, device_port=%d, iRet=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, i);
                            }

                            /* ��ȡ�¼�CMS ���������豸���ñ� */
                            i = SendQuerySubCMSTopologyPhyDeviceConfigMessage(pGBDeviceInfo);

                            if (0 != i)
                            {
                                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "DevReg() SendQuerySubCMSTopologyPhyDeviceConfigMessage Error:i=%d \r\n", i);
                            }
                            else
                            {
                                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() SendQuerySubCMSTopologyPhyDeviceConfigMessage OK:i=%d \r\n", i);
                            }

                            /* ���Ͳ�ѯ�豸Ŀ¼��Ϣ����Ϣ */
                            i = SendQueryDeviceCatalogMessage(pGBDeviceInfo);

                            if (0 != i)
                            {
                                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "DevReg() SendQueryDeviceCatalogMessage Error:i=%d \r\n", i);
                            }
                            else
                            {
                                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() SendQueryDeviceCatalogMessage OK:i=%d \r\n", i);
                            }

                            /*������¼�CMS, ���Ҳ���ͬ�������������ȡ�߼��豸������Ϣ���߼��豸�����ϵ��Ϣ */
                            if (pGBDeviceInfo->link_type == 0)
                            {
                                i = SendQueryDeviceGroupInfoMessage(pGBDeviceInfo);

                                if (0 != i)
                                {
                                    DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "DevReg() SendQueryDeviceGroupInfoMessage Error:DeviceID=%s, i=%d \r\n", pGBDeviceInfo->device_id, i);
                                }
                                else
                                {
                                    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() SendQueryDeviceGroupInfoMessage OK:DeviceID=%s, i=%d \r\n", pGBDeviceInfo->device_id, i);
                                }

                                i = SendQueryDeviceGroupMapInfoMessage(pGBDeviceInfo);

                                if (0 != i)
                                {
                                    DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "DevReg() SendQueryDeviceGroupMapInfoMessage Error:DeviceID=%s, i=%d \r\n", pGBDeviceInfo->device_id, i);
                                }
                                else
                                {
                                    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() SendQueryDeviceGroupMapInfoMessage OK:DeviceID=%s, i=%d \r\n", pGBDeviceInfo->device_id, i);
                                }
                            }
                        }

                        /* ��������˽ṹ��������豸���ͣ���ô��Ҫ��ӵ����˽ṹ����Ϣ�����ϱ����ϼ�CMS */
                        if (EV9000_DEVICETYPE_DECODER == pGBDeviceInfo->device_type
                            || EV9000_DEVICETYPE_SIPSERVER == pGBDeviceInfo->device_type
                            || EV9000_DEVICETYPE_MGWSERVER == pGBDeviceInfo->device_type)
                        {
                            /* ������˽ṹ����Ϣ */
                            snprintf(strDeviceType, 16, "%d", pGBDeviceInfo->device_type);
                            snprintf(strStatus, 16, "%d", pGBDeviceInfo->reg_status);
                            i = AddTopologyPhyDeviceInfo2DB(pGBDeviceInfo->device_id, (char*)"", strDeviceType, pGBDeviceInfo->login_ip, strStatus, local_cms_id_get(), (char*)"1", pdboper);
                        }

                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "�豸����:�豸ID=%s, IP��ַ=%s", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip);
                        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Device online:device ID=%s, IP address=%s", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip);

                        DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "DevReg() ADD:device_id=%s, login_ip=%s, login_port=%d, reg_info_index=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, pGBDeviceInfo->reg_info_index);
                    }
                    else
                    {
                        if (iIsRefreshReg)
                        {
                            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�豸ע��ˢ��:�豸ID=%s, IP��ַ=%s, �˿ں�=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Equipment registered refresh:ID=%s, IP=%s, port=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                        }
                        else
                        {
                            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "�豸ע���CallID�����仯, �������豸���ߺ�����ע������:�豸ID=%s, IP��ַ=%s", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip);
                            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Device re online:device ID=%s, IP address=%s", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip);

                            /* ���ͻ�ȡ�豸��Ϣ��Ϣ */
                            i = SendQueryDeviceInfoMessage(pGBDeviceInfo);

                            if (0 != i)
                            {
                                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "DevReg() SendQueryDeviceInfoMessage Error:i=%d \r\n", i);
                            }
                            else
                            {
                                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() SendQueryDeviceInfoMessage OK:i=%d \r\n", i);
                            }
                        }
                    }

                    /* ��ȡCatalog */
                    if (pGBDeviceInfo->device_type == EV9000_DEVICETYPE_SIPSERVER
                        && pGBDeviceInfo->three_party_flag == 1) /* ������ƽ̨ */
                    {
                        /* ���û�л�ȡ���߼�ͨ���������·��ͻ�ȡ��Ϣ */
                        if (iIsRefreshReg && 0 == pGBDeviceInfo->iGetCataLogStatus)
                        {
                            /* ���Ͳ�ѯ�豸Ŀ¼��Ϣ����Ϣ */
                            i = SendQueryDeviceCatalogMessage(pGBDeviceInfo);

                            if (0 != i)
                            {
                                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "DevReg() SendQueryDeviceCatalogMessage Error:i=%d \r\n", i);
                            }
                            else
                            {
                                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() SendQueryDeviceCatalogMessage OK:i=%d \r\n", i);
                            }

                            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "û�м������¼�������ƽ̨�豸Catalog, �ٴη��Ͳ�ѯCatalog��Ϣ:�¼�������ƽ̨�豸ID=%s, IP��ַ=%s, �˿ں�=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "No Catalog response message from front device, send query catalog message again:front-end device ID=%s, IP address=%s, Port number=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                        }
                    }
                    else if (pGBDeviceInfo->device_type == EV9000_DEVICETYPE_SIPSERVER
                             && pGBDeviceInfo->three_party_flag == 0) /* �Լ���ƽ̨ */
                    {
                        /* ���û�л�ȡ���߼�ͨ���������·��ͻ�ȡ��Ϣ */
                        if (iIsRefreshReg && 0 == pGBDeviceInfo->iGetCataLogStatus)
                        {
                            /* ���Ͳ�ѯ�豸Ŀ¼��Ϣ����Ϣ */
                            i = SendQueryDeviceCatalogMessage(pGBDeviceInfo);

                            if (0 != i)
                            {
                                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "DevReg() SendQueryDeviceCatalogMessage Error:i=%d \r\n", i);
                            }
                            else
                            {
                                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() SendQueryDeviceCatalogMessage OK:i=%d \r\n", i);
                            }

                            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "û�м������¼�CMS�豸Catalog, �ٴη��Ͳ�ѯCatalog��Ϣ:�¼�CMS ID=%s, IP��ַ=%s, �˿ں�=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "No Catalog response message from front device, send query catalog message again:front-end device ID=%s, IP address=%s, Port number=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                        }
                    }
                    else if (EV9000_DEVICETYPE_DECODER == pGBDeviceInfo->device_type) /* ������ */
                    {
                        /* ����������߻���û�л�ȡ���߼�ͨ���������·��ͻ�ȡ��Ϣ */
                        if (!iIsRefreshReg || 0 == pGBDeviceInfo->iGetCataLogStatus)
                        {
                            /* ���Ͳ�ѯ�豸Ŀ¼��Ϣ����Ϣ */
                            i = SendQueryDeviceCatalogMessage(pGBDeviceInfo);

                            if (0 != i)
                            {
                                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "DevReg() SendQueryDeviceCatalogMessage Error:i=%d \r\n", i);
                            }
                            else
                            {
                                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() SendQueryDeviceCatalogMessage OK:i=%d \r\n", i);
                            }

                            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "û�м�����ǰ�˽������豸��Catalog, �ٴη��Ͳ�ѯCatalog��Ϣ:ǰ�˽������豸ID=%s, IP��ַ=%s, �˿ں�=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "No Catalog response message from front device, send query catalog message again:front-end device ID=%s, IP address=%s, Port number=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                        }
                    }
                    else if (EV9000_DEVICETYPE_MGWSERVER == pGBDeviceInfo->device_type) /* �������� */
                    {
                        /* ����������߻���û�л�ȡ���߼�ͨ���������·��ͻ�ȡ��Ϣ */
                        if (!iIsRefreshReg || 0 == pGBDeviceInfo->iGetCataLogStatus)
                        {
                            /* ���Ͳ�ѯ�豸Ŀ¼��Ϣ����Ϣ */
                            i = SendQueryDeviceCatalogMessage(pGBDeviceInfo);

                            if (0 != i)
                            {
                                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "DevReg() SendQueryDeviceCatalogMessage Error:i=%d \r\n", i);
                            }
                            else
                            {
                                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() SendQueryDeviceCatalogMessage OK:i=%d \r\n", i);
                            }

                            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "û�м�����ǰ��ý�������豸��Catalog, �ٴη��Ͳ�ѯCatalog��Ϣ:ǰ��ý�������豸ID=%s, IP��ַ=%s, �˿ں�=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "No Catalog response message from front device, send query catalog message again:front-end device ID=%s, IP address=%s, Port number=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                        }
                    }
                    else if (EV9000_DEVICETYPE_ALARMSERVER == pGBDeviceInfo->device_type) /* ���������� */
                    {
                        /* ����������߻���û�л�ȡ���߼�ͨ���������·��ͻ�ȡ��Ϣ */
                        if (!iIsRefreshReg || 0 == pGBDeviceInfo->iGetCataLogStatus)
                        {
                            /* ���Ͳ�ѯ�豸Ŀ¼��Ϣ����Ϣ */
                            i = SendQueryDeviceCatalogMessage(pGBDeviceInfo);

                            if (0 != i)
                            {
                                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "DevReg() SendQueryDeviceCatalogMessage Error:i=%d \r\n", i);
                            }
                            else
                            {
                                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() SendQueryDeviceCatalogMessage OK:i=%d \r\n", i);
                            }

                            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "û�м�����ǰ�˱�����������Catalog, �ٴη��Ͳ�ѯCatalog��Ϣ:ǰ�˱���������ID=%s, IP��ַ=%s, �˿ں�=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "No Catalog response message from front device, send query catalog message again:front-end device ID=%s, IP address=%s, Port number=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                        }
                    }
                    else if (EV9000_DEVICETYPE_SIPSERVER != pGBDeviceInfo->device_type
                             && EV9000_DEVICETYPE_VIDEODIAGNOSIS != pGBDeviceInfo->device_type
                             && EV9000_DEVICETYPE_DECODER != pGBDeviceInfo->device_type
                             && EV9000_DEVICETYPE_MGWSERVER != pGBDeviceInfo->device_type
                             && EV9000_DEVICETYPE_ALARMSERVER != pGBDeviceInfo->device_type) /* �����豸 */
                    {
                        /* ����������߻���û�л�ȡ���߼�ͨ���������·��ͻ�ȡ��Ϣ */
                        if (!iIsRefreshReg || 0 == pGBDeviceInfo->iGetCataLogStatus)
                        {
                            /* ���Ͳ�ѯ�豸Ŀ¼��Ϣ����Ϣ */
                            i = SendQueryDeviceCatalogMessage(pGBDeviceInfo);

                            if (0 != i)
                            {
                                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "DevReg() SendQueryDeviceCatalogMessage Error:i=%d \r\n", i);
                            }
                            else
                            {
                                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() SendQueryDeviceCatalogMessage OK:i=%d \r\n", i);
                            }

                            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "û�м�����ǰ���豸��Catalog, �ٴη��Ͳ�ѯCatalog��Ϣ:ǰ���豸ID=%s, IP��ַ=%s, �˿ں�=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                        }
                    }

                    printf("DevReg() Exit---\r\n");
                    return 0;
                }
            }
        }
        else
        {
            pGBDeviceInfo->auth_count++;

            if (pGBDeviceInfo->auth_count > 3)
            {
                pGBDeviceInfo->auth_count = 0;
                memset(strErrorCode, 0, 32);
                snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_REG_AUTH_FAILD_ERROR);
                SIP_UASAnswerToRegister(reg_info_index, 403, strErrorCode);
                //SIP_UASAnswerToRegister(reg_info_index, 403, NULL);
                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "DevReg() exit---: AUTH FAIL \r\n");
                SystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "�豸ע��ʧ��:�豸ID=%s, IP��ַ=%s, �˿ں�=%d, ԭ��=%s", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)"�豸��֤ʧ��");
                EnSystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "Device registration failed:device ID=%s, IP address=%s, port number=%d, cause=%s", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)"Device authentication failed");
                return -1;
            }
            else
            {
                snprintf(strRegisterDomain, 128, "%s.spvmn.cn", pGblconf->register_region);
                SIP_UASAnswerToRegister4Auth(reg_info_index, strRegisterDomain);
                DEBUG_TRACE(MODULE_DEVICE, LOG_WARN, "DevReg() exit---: NEED AUTH \r\n");
                //SystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_WARNING, "�豸ע��ʧ��:�豸ID=%s, IP��ַ=%s, �˿ں�=%d, ԭ��=%s", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)"�豸��Ҫ��֤");
                //EnSystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_WARNING, "Device registration failed:device ID=%s, IP address=%s, port number=%d, cause=%s", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)"Device need authentication");
                return -1;
            }
        }
    }
    else
    {
        /* ��ȡע��ķ�����IP��ַ���˿ںţ�ȷ���Ǵ��ĸ���ע��� */
        server_ip = SIP_GetUASServerIP(pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);

        DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() device_id=%s, login_ip=%s, login_port=%d, server_ip=%s \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, server_ip);
        printf("DevReg() device_id=%s, login_ip=%s, login_port=%d, server_ip=%s \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, server_ip);

        if ((NULL == server_ip)
            || (NULL != server_ip && 0 == sstrcmp(server_ip, local_cms_id_get()))
            || (NULL != server_ip && 0 == sstrcmp(server_ip, pGblconf->register_region)))
        {
            memset(strEthName, 0, MAX_IP_LEN);
            snprintf(strEthName, MAX_IP_LEN, "%s", pGblconf->default_eth_name);

            server_ip = local_ip_get(strEthName);
            server_port = local_port_get(strEthName);
            pcRegServerEthName = get_ip_eth_name(server_ip);
        }
        else
        {
            server_port = SIP_GetUASServerPort(pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
            pcRegServerEthName = get_ip_eth_name(server_ip);
        }

        DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() device_id=%s, login_ip=%s, login_port=%d, server_port=%d, RegServerEthName=%s \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, server_port, pcRegServerEthName);
        printf("DevReg() device_id=%s, login_ip=%s, login_port=%d, server_port=%d, RegServerEthName=%s \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, server_port, pcRegServerEthName);

        if (NULL == server_ip)
        {
            pGBDeviceInfo->auth_count = 0;
            memset(strErrorCode, 0, 32);
            snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_REG_GET_SERVER_IP_ERROR);
            SIP_UASAnswerToRegister(reg_info_index, 403, strErrorCode);
            //SIP_UASAnswerToRegister(reg_info_index, 503, (char*)"Get Register Server IP Error");
            DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() exit---: Get Register Server Info Error:server_ip=%s, server_port=%d, RegServerEthName=%s \r\n", server_ip, server_port, pcRegServerEthName);
            SystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "�豸ע��ʧ��:�豸ID=%s, IP��ַ=%s, �˿ں�=%d, ԭ��=%s", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)"��ȡ�豸ע��ķ�����IP��ַ��Ϣʧ��");
            EnSystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "Device registration failed:device ID=%s, IP address=%s, port number=%d, cause=%s", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)"Access information of server IP address for device registration failed");

            return -1;
        }

        if (server_port <= 0)
        {
            pGBDeviceInfo->auth_count = 0;
            memset(strErrorCode, 0, 32);
            snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_REG_GET_SERVER_PORT_ERROR);
            SIP_UASAnswerToRegister(reg_info_index, 403, strErrorCode);
            //SIP_UASAnswerToRegister(reg_info_index, 503, (char*)"Get Register Server Port Error");
            DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() exit---: Get Register Server Info Error:server_ip=%s, server_port=%d, RegServerEthName=%s \r\n", server_ip, server_port, pcRegServerEthName);
            SystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "�豸ע��ʧ��:�豸ID=%s, IP��ַ=%s, �˿ں�=%d, ԭ��=%s", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)"��ȡ�豸ע��ķ�����IP��ַ��Ϣʧ��");
            EnSystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "Device registration failed:device ID=%s, IP address=%s, port number=%d, cause=%s", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)"Access information of server IP address for device registration failed");

            return -1;
        }

        if (NULL == pcRegServerEthName)
        {
            pGBDeviceInfo->auth_count = 0;
            memset(strErrorCode, 0, 32);
            snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_REG_GET_SERVER_IP_ETHNAME_ERROR);
            SIP_UASAnswerToRegister(reg_info_index, 403, strErrorCode);
            //SIP_UASAnswerToRegister(reg_info_index, 503, (char*)"Get Register Server EthName Error");
            DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() exit---: Get Register Server Info Error:server_ip=%s, server_port=%d, RegServerEthName=%s \r\n", server_ip, server_port, pcRegServerEthName);
            SystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "�豸ע��ʧ��:�豸ID=%s, IP��ַ=%s, �˿ں�=%d, ԭ��=%s", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)"��ȡ�豸ע��ķ�����IP��ַ��Ϣʧ��");
            EnSystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "Device registration failed:device ID=%s, IP address=%s, port number=%d, cause=%s", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)"Access information of server IP address for device registration failed");

            return -1;
        }

        /* ȷ���Ƿ���ˢ��ע�� */
        call_id = SIP_GetUASCallID(pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);

        if (NULL == call_id)
        {
            pGBDeviceInfo->auth_count = 0;
            memset(strErrorCode, 0, 32);
            snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_REG_CALLID_ERROR);
            SIP_UASAnswerToRegister(reg_info_index, 403, strErrorCode);
            //SIP_UASAnswerToRegister(reg_info_index, 503, (char*)"Get Register CallID Error");
            DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() exit---: Get Register CallID Error:server_ip=%s, server_port=%d, RegServerEthName=%s \r\n", server_ip, server_port, pcRegServerEthName);
            SystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "�豸ע��ʧ��:�豸ID=%s, IP��ַ=%s, �˿ں�=%d, ԭ��=%s", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)"��ȡ�豸ע���CallID�ֶ���Ϣʧ��");
            EnSystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "Device registration failed:device ID=%s, IP address=%s, port number=%d, cause=%s", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)"Access information of call id for device registration failed");

            return -1;
        }

        if ('\0' == pGBDeviceInfo->call_id[0])
        {
            osip_strncpy(pGBDeviceInfo->call_id, call_id, MAX_128CHAR_STRING_LEN);
            iIsRefreshReg = 0;
        }
        else
        {
            if (0 != sstrcmp(pGBDeviceInfo->call_id, call_id))
            {
                memset(pGBDeviceInfo->call_id, 0, MAX_128CHAR_STRING_LEN + 4);
                osip_strncpy(pGBDeviceInfo->call_id, call_id, MAX_128CHAR_STRING_LEN);
                iIsRefreshReg = 0;
            }
            else
            {
                iIsRefreshReg = 1; /* ˢ��ע�� */
            }
        }

        DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "DevReg() device_id=%s, login_ip=%s, login_port=%d: old callid=%s, new callid=%s, IsRefreshReg=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, pGBDeviceInfo->call_id, call_id, iIsRefreshReg);
        printf("DevReg() device_id=%s, login_ip=%s, login_port=%d: old callid=%s, new callid=%s, IsRefreshReg=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, pGBDeviceInfo->call_id, call_id, iIsRefreshReg);

        /* ȷ��������IP��ַ */
        if (pGBDeviceInfo->strRegServerIP[0] == '\0')
        {
            osip_strncpy(pGBDeviceInfo->strRegServerIP, server_ip, MAX_IP_LEN);
        }
        else
        {
            if (0 != sstrcmp(pGBDeviceInfo->strRegServerIP, server_ip))
            {
                memset(pGBDeviceInfo->strRegServerIP, 0, MAX_IP_LEN);
                osip_strncpy(pGBDeviceInfo->strRegServerIP, server_ip, MAX_IP_LEN);
            }
        }

        /* ȷ��ע��������˿ں� */
        if (pGBDeviceInfo->iRegServerPort <= 0)
        {
            pGBDeviceInfo->iRegServerPort = server_port;
        }
        else
        {
            if (pGBDeviceInfo->iRegServerPort != server_port)
            {
                pGBDeviceInfo->iRegServerPort = server_port;
            }
        }

        /* ȷ��IP��ַ�������� */
        if (pGBDeviceInfo->strRegServerEthName[0] == '\0')
        {
            osip_strncpy(pGBDeviceInfo->strRegServerEthName, pcRegServerEthName, MAX_IP_LEN);
        }
        else
        {
            if (0 != sstrcmp(pGBDeviceInfo->strRegServerEthName, pcRegServerEthName))
            {
                memset(pGBDeviceInfo->strRegServerEthName, 0, MAX_IP_LEN);
                osip_strncpy(pGBDeviceInfo->strRegServerEthName, pcRegServerEthName, MAX_IP_LEN);
            }
        }

        pGBDeviceInfo->auth_count = 0;
        SIP_UASAnswerToRegister(reg_info_index, 200, NULL);

        if (pGBDeviceInfo->reg_status <= 0)
        {
            /* ý�����غ��¼�ƽ̨�����䵥���Ĵ����߳� */
            if (pGBDeviceInfo->device_type == EV9000_DEVICETYPE_SIPSERVER
                || pGBDeviceInfo->device_type == EV9000_DEVICETYPE_MGWSERVER
                || pGBDeviceInfo->device_type == EV9000_DEVICETYPE_VIDEODIAGNOSIS
                || pGBDeviceInfo->device_type == EV9000_DEVICETYPE_INTELLIGENTANALYSIS
                || pGBDeviceInfo->device_type == EV9000_DEVICETYPE_DECODER)
            {
                /* �����豸ҵ�����߳� */
                device_tl_pos = device_srv_proc_thread_find(pGBDeviceInfo);
                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() device_srv_proc_thread_find:device_tl_pos=%d \r\n", device_tl_pos);
                printf("DevReg() device_srv_proc_thread_find:device_tl_pos=%d \r\n", device_tl_pos);

                if (device_tl_pos < 0)
                {
                    //���䴦���߳�
                    i = device_srv_proc_thread_assign(pGBDeviceInfo);
                    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() device_srv_proc_thread_assign:i=%d \r\n", i);
                    printf("DevReg() device_srv_proc_thread_assign:i=%d \r\n", i);

                    if (i != 0)
                    {
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "�����豸ҵ�����߳�ʧ��:�豸ID=%s, IP��ַ=%s, �˿ں�=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    }
                }
                else
                {
                    /* �ͷ�һ��֮ǰ���û�ҵ�����߳� */
                    i = device_srv_proc_thread_recycle(pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);

                    //���䴦���߳�
                    i = device_srv_proc_thread_assign(pGBDeviceInfo);
                    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() device_srv_proc_thread_assign:i=%d \r\n", i);
                    printf("DevReg() device_srv_proc_thread_assign:i=%d \r\n", i);

                    if (i != 0)
                    {
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "�����豸ҵ�����߳�ʧ��:�豸ID=%s, IP��ַ=%s, �˿ں�=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    }
                }
            }

            /*�����ڴ�*/
            pGBDeviceInfo->reg_status = 1;
            pGBDeviceInfo->catalog_subscribe_flag = 0;                                       /* Ŀ¼���ı�ʶ:0:û�ж��ģ�1:�Ѷ��� */
            pGBDeviceInfo->catalog_subscribe_interval = local_register_retry_interval_get(); /* Ŀ¼����δ�ɹ������Լ��ʱ�� */
            pGBDeviceInfo->catalog_subscribe_expires = 0;                                    /* Ŀ¼���ĳ�ʱʱ��*/
            pGBDeviceInfo->catalog_subscribe_expires_count = 0;                              /* Ŀ¼����ʱ����� */
            pGBDeviceInfo->catalog_subscribe_event_id = 0;                                   /* Ŀ¼�����¼����� */
            pGBDeviceInfo->last_keep_alive_time = 0;
            pGBDeviceInfo->keep_alive_expires = 0;
            pGBDeviceInfo->keep_alive_expires_count = 0;
            pGBDeviceInfo->keep_alive_count = 0;
            pGBDeviceInfo->iGetCataLogStatus = 0;
            pGBDeviceInfo->iGetLogicDeviceStatusCount = 0;
            pGBDeviceInfo->iLastGetCataLogTime = 0;

            UpdateGBDeviceRegStatus2DB(pGBDeviceInfo, pdboper);

            /* �����߼��豸����Ϣ,�����߼��豸��ǰ���Ѿ�ɾ����״̬����ͨ����ȡ�����ϱ��ĵ� */
            if (pGBDeviceInfo->device_type == EV9000_DEVICETYPE_IPC
                || pGBDeviceInfo->device_type == EV9000_DEVICETYPE_DECODER
                || (pGBDeviceInfo->device_type == EV9000_DEVICETYPE_SIPSERVER && pGBDeviceInfo->three_party_flag == 1))
            {
                i = SetGBLogicDeviceStatus(pGBDeviceInfo->id, 1, pdboper);

                if (0 != i)
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "DevReg() SetGBLogicDeviceStatus Error:i=%d \r\n", i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() SetGBLogicDeviceStatus OK:i=%d \r\n", i);
                }

                /* �����豸״̬��Ϣ���ͻ��� */
                i = SendAllGBLogicDeviceStatusProc(pGBDeviceInfo->id, 1, 1, pdboper);

                if (0 != i)
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "DevReg() SendAllGBLogicDeviceStatusProc Error:i=%d \r\n", i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() SendAllGBLogicDeviceStatusProc OK:i=%d \r\n", i);
                }
            }

            /* ���ͻ�ȡ�豸״̬��Ϣ */
            //i = SendQueryDeviceStatusMessage(pGBDeviceInfo);

            /* ���ͻ�ȡ�豸��Ϣ��Ϣ */
            i = SendQueryDeviceInfoMessage(pGBDeviceInfo);

            if (0 != i)
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "DevReg() SendQueryDeviceInfoMessage Error:i=%d \r\n", i);
            }
            else
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() SendQueryDeviceInfoMessage OK:i=%d \r\n", i);
            }

            if (pGBDeviceInfo->device_type == EV9000_DEVICETYPE_SIPSERVER) /* ƽ̨ */
            {
                /* ��ȡ�¼�CMS ���ݿ�IP ��ַ */
                i = SendQuerySubCMSDBIPMessage(pGBDeviceInfo);

                if (0 != i)
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "DevReg() SendQuerySubCMSDBIPMessage Error:i=%d \r\n", i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() SendQuerySubCMSDBIPMessage OK:i=%d \r\n", i);
                }

                /* ���ͱ���CMS������������¼�CMS */
                i = SendNotifyRestartMessageToSubCMS(pGBDeviceInfo);

                if (0 != i)
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "DevReg() SendNotifyRestartMessageToSubCMS Error:i=%d \r\n", i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() SendNotifyRestartMessageToSubCMS OK:i=%d \r\n", i);
                }

                /* ���͵�λ���¼�ƽ̨ */
                i = SendNotifyCatalogMessageToSubCMS(pGBDeviceInfo->device_id);

                if (0 != i)
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "DevReg() SendNotifyCatalogMessageToSubCMS Error:device_id=%s, device_ip=%s, device_port=%d, iRet=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() SendNotifyCatalogMessageToSubCMS OK:device_id=%s, device_ip=%s, device_port=%d, iRet=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, i);
                }

                /* ��ȡ�¼�CMS ���������豸���ñ� */
                i = SendQuerySubCMSTopologyPhyDeviceConfigMessage(pGBDeviceInfo);

                if (0 != i)
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "DevReg() SendQuerySubCMSTopologyPhyDeviceConfigMessage Error:i=%d \r\n", i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() SendQuerySubCMSTopologyPhyDeviceConfigMessage OK:i=%d \r\n", i);
                }

                /* ���Ͳ�ѯ�豸Ŀ¼��Ϣ����Ϣ */
                i = SendQueryDeviceCatalogMessage(pGBDeviceInfo);

                if (0 != i)
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "DevReg() SendQueryDeviceCatalogMessage Error:i=%d \r\n", i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() SendQueryDeviceCatalogMessage OK:i=%d \r\n", i);
                }

                /*������¼�CMS, ���Ҳ���ͬ�������������ȡ�߼��豸������Ϣ���߼��豸�����ϵ��Ϣ */
                if (pGBDeviceInfo->link_type == 0)
                {
                    i = SendQueryDeviceGroupInfoMessage(pGBDeviceInfo);

                    if (0 != i)
                    {
                        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "DevReg() SendQueryDeviceGroupInfoMessage Error:DeviceID=%s, i=%d \r\n", pGBDeviceInfo->device_id, i);
                    }
                    else
                    {
                        DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() SendQueryDeviceGroupInfoMessage OK:DeviceID=%s, i=%d \r\n", pGBDeviceInfo->device_id, i);
                    }

                    i = SendQueryDeviceGroupMapInfoMessage(pGBDeviceInfo);

                    if (0 != i)
                    {
                        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "DevReg() SendQueryDeviceGroupMapInfoMessage Error:DeviceID=%s, i=%d \r\n", pGBDeviceInfo->device_id, i);
                    }
                    else
                    {
                        DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() SendQueryDeviceGroupMapInfoMessage OK:DeviceID=%s, i=%d \r\n", pGBDeviceInfo->device_id, i);
                    }
                }
            }

            /* ��������˽ṹ��������豸���ͣ���ô��Ҫ��ӵ����˽ṹ����Ϣ�����ϱ����ϼ�CMS */
            if (EV9000_DEVICETYPE_DECODER == pGBDeviceInfo->device_type
                || EV9000_DEVICETYPE_SIPSERVER == pGBDeviceInfo->device_type
                || EV9000_DEVICETYPE_MGWSERVER == pGBDeviceInfo->device_type)
            {
                /* ������˽ṹ����Ϣ */
                snprintf(strDeviceType, 16, "%d", pGBDeviceInfo->device_type);
                snprintf(strStatus, 16, "%d", pGBDeviceInfo->reg_status);
                i = AddTopologyPhyDeviceInfo2DB(pGBDeviceInfo->device_id, (char*)"", strDeviceType, pGBDeviceInfo->login_ip, strStatus, local_cms_id_get(), (char*)"1", pdboper);
            }

            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "�豸����:�豸ID=%s, IP��ַ=%s", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Device online:device ID=%s, IP address=%s", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip);

            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "DevReg() ADD:device_id=%s, login_ip=%s, login_port=%d, reg_info_index=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, pGBDeviceInfo->reg_info_index);
            printf("DevReg() ADD:device_id=%s, login_ip=%s, login_port=%d, reg_info_index=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, pGBDeviceInfo->reg_info_index);
        }
        else
        {
            if (iIsRefreshReg)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�豸ע��ˢ��:�豸ID=%s, IP��ַ=%s, �˿ں�=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Equipment registered refresh:ID=%s, IP=%s, port=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
            }
            else
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "�豸ע���CallID�����仯, �������豸���ߺ�����ע������:�豸ID=%s, IP��ַ=%s", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Device re online:device ID=%s, IP address=%s", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip);

                /* ���ͻ�ȡ�豸��Ϣ��Ϣ */
                i = SendQueryDeviceInfoMessage(pGBDeviceInfo);

                if (0 != i)
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "DevReg() SendQueryDeviceInfoMessage Error:i=%d \r\n", i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() SendQueryDeviceInfoMessage OK:i=%d \r\n", i);
                }
            }
        }

        /* ��ȡCatalog */
        if (pGBDeviceInfo->device_type == EV9000_DEVICETYPE_SIPSERVER
            && pGBDeviceInfo->three_party_flag == 1) /* ������ƽ̨ */
        {
            /* ���û�л�ȡ���߼�ͨ���������·��ͻ�ȡ��Ϣ */
            if (iIsRefreshReg && 0 == pGBDeviceInfo->iGetCataLogStatus)
            {
                /* ���Ͳ�ѯ�豸Ŀ¼��Ϣ����Ϣ */
                i = SendQueryDeviceCatalogMessage(pGBDeviceInfo);

                if (0 != i)
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "DevReg() SendQueryDeviceCatalogMessage Error:i=%d \r\n", i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() SendQueryDeviceCatalogMessage OK:i=%d \r\n", i);
                }

                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "û�м������¼�������ƽ̨�豸Catalog, �ٴη��Ͳ�ѯCatalog��Ϣ:�¼�������ƽ̨�豸ID=%s, IP��ַ=%s, �˿ں�=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "No Catalog response message from front device, send query catalog message again:front-end device ID=%s, IP address=%s, Port number=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
            }
        }
        else if (pGBDeviceInfo->device_type == EV9000_DEVICETYPE_SIPSERVER
                 && pGBDeviceInfo->three_party_flag == 0) /* �Լ���ƽ̨ */
        {
            /* ���û�л�ȡ���߼�ͨ���������·��ͻ�ȡ��Ϣ */
            if (iIsRefreshReg && 0 == pGBDeviceInfo->iGetCataLogStatus)
            {
                /* ���Ͳ�ѯ�豸Ŀ¼��Ϣ����Ϣ */
                i = SendQueryDeviceCatalogMessage(pGBDeviceInfo);

                if (0 != i)
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "DevReg() SendQueryDeviceCatalogMessage Error:i=%d \r\n", i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() SendQueryDeviceCatalogMessage OK:i=%d \r\n", i);
                }

                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "û�м������¼�CMS�豸Catalog, �ٴη��Ͳ�ѯCatalog��Ϣ:�¼�CMS ID=%s, IP��ַ=%s, �˿ں�=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "No Catalog response message from front device, send query catalog message again:front-end device ID=%s, IP address=%s, Port number=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
            }
        }
        else if (EV9000_DEVICETYPE_DECODER == pGBDeviceInfo->device_type) /* ������ */
        {
            /* ����������߻���û�л�ȡ���߼�ͨ���������·��ͻ�ȡ��Ϣ */
            if (!iIsRefreshReg || 0 == pGBDeviceInfo->iGetCataLogStatus)
            {
                /* ���Ͳ�ѯ�豸Ŀ¼��Ϣ����Ϣ */
                i = SendQueryDeviceCatalogMessage(pGBDeviceInfo);

                if (0 != i)
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "DevReg() SendQueryDeviceCatalogMessage Error:i=%d \r\n", i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() SendQueryDeviceCatalogMessage OK:i=%d \r\n", i);
                }

                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "û�м�����ǰ�˽������豸��Catalog, �ٴη��Ͳ�ѯCatalog��Ϣ:ǰ�˽������豸ID=%s, IP��ַ=%s, �˿ں�=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "No Catalog response message from front device, send query catalog message again:front-end device ID=%s, IP address=%s, Port number=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
            }
        }
        else if (EV9000_DEVICETYPE_MGWSERVER == pGBDeviceInfo->device_type) /* �������� */
        {
            /* ����������߻���û�л�ȡ���߼�ͨ���������·��ͻ�ȡ��Ϣ */
            if (!iIsRefreshReg || 0 == pGBDeviceInfo->iGetCataLogStatus)
            {
                /* ���Ͳ�ѯ�豸Ŀ¼��Ϣ����Ϣ */
                i = SendQueryDeviceCatalogMessage(pGBDeviceInfo);

                if (0 != i)
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "DevReg() SendQueryDeviceCatalogMessage Error:i=%d \r\n", i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() SendQueryDeviceCatalogMessage OK:i=%d \r\n", i);
                }

                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "û�м�����ǰ��ý�������豸��Catalog, �ٴη��Ͳ�ѯCatalog��Ϣ:ǰ��ý�������豸ID=%s, IP��ַ=%s, �˿ں�=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "No Catalog response message from front device, send query catalog message again:front-end device ID=%s, IP address=%s, Port number=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
            }
        }
        else if (EV9000_DEVICETYPE_ALARMSERVER == pGBDeviceInfo->device_type) /* ���������� */
        {
            /* ����������߻���û�л�ȡ���߼�ͨ���������·��ͻ�ȡ��Ϣ */
            if (!iIsRefreshReg || 0 == pGBDeviceInfo->iGetCataLogStatus)
            {
                /* ���Ͳ�ѯ�豸Ŀ¼��Ϣ����Ϣ */
                i = SendQueryDeviceCatalogMessage(pGBDeviceInfo);

                if (0 != i)
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "DevReg() SendQueryDeviceCatalogMessage Error:i=%d \r\n", i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() SendQueryDeviceCatalogMessage OK:i=%d \r\n", i);
                }

                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "û�м�����ǰ�˱�����������Catalog, �ٴη��Ͳ�ѯCatalog��Ϣ:ǰ��ý�������豸ID=%s, IP��ַ=%s, �˿ں�=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "No Catalog response message from front device, send query catalog message again:front-end device ID=%s, IP address=%s, Port number=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
            }
        }
        else if (EV9000_DEVICETYPE_CAMERA == pGBDeviceInfo->device_type
                 || EV9000_DEVICETYPE_IPC == pGBDeviceInfo->device_type) /* IPC */
        {
            /* ����������ߣ������·��ͻ�ȡ��Ϣ */
            if (!iIsRefreshReg)
            {
                /* ���Ͳ�ѯ�豸Ŀ¼��Ϣ����Ϣ */
                i = SendQueryDeviceCatalogMessage(pGBDeviceInfo);

                if (0 != i)
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "DevReg() SendQueryDeviceCatalogMessage Error:i=%d \r\n", i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() SendQueryDeviceCatalogMessage OK:i=%d \r\n", i);
                }

                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "û�м�����ǰ��ý�������豸��Catalog, �ٴη��Ͳ�ѯCatalog��Ϣ:ǰ��ý�������豸ID=%s, IP��ַ=%s, �˿ں�=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "No Catalog response message from front device, send query catalog message again:front-end device ID=%s, IP address=%s, Port number=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
            }
        }
        else if (EV9000_DEVICETYPE_SIPSERVER != pGBDeviceInfo->device_type
                 && EV9000_DEVICETYPE_VIDEODIAGNOSIS != pGBDeviceInfo->device_type
                 && EV9000_DEVICETYPE_DECODER != pGBDeviceInfo->device_type
                 && EV9000_DEVICETYPE_MGWSERVER != pGBDeviceInfo->device_type
                 && EV9000_DEVICETYPE_ALARMSERVER != pGBDeviceInfo->device_type
                 && EV9000_DEVICETYPE_CAMERA != pGBDeviceInfo->device_type
                 && EV9000_DEVICETYPE_IPC != pGBDeviceInfo->device_type) /* �����豸 */
        {
            /* ���Ͳ�ѯ�豸Ŀ¼��Ϣ����Ϣ */
            i = SendQueryDeviceCatalogMessage(pGBDeviceInfo);

            if (0 != i)
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "DevReg() SendQueryDeviceCatalogMessage Error:i=%d \r\n", i);
            }
            else
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() SendQueryDeviceCatalogMessage OK:i=%d \r\n", i);
            }
        }

        printf("DevReg() Exit---\r\n");
        return 0;
    }

    memset(strErrorCode, 0, 32);
    snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_SYSTEM_ERROR);
    SIP_UASAnswerToRegister(reg_info_index, 500, strErrorCode);
    //SIP_UASAnswerToRegister(reg_info_index, 500, (char*)"Server Error");
    DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "DevReg() exit---: Forbidden \r\n");
    SystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "�豸ע��ʧ��:�豸ID=%s, IP��ַ=%s, �˿ں�=%d, ԭ��=%s", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)"δ֪ԭ��");
    EnSystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "Device registration failed:device ID=%s, IP address=%s, port number=%d, cause=%s", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)"Unknown reason");

    return -1;
}

/*****************************************************************************
 �� �� ��  : GBDevice_reg_msg_proc
 ��������  : �豸ע����Ϣ����
 �������  : GBDevice_reg_msg_t* pMsg
             DBOper* pdboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��11��26�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int GBDevice_reg_msg_proc(GBDevice_reg_msg_t* pMsg, DBOper* pdboper)
{
    char strErrorCode[32] = {0};

    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "GBDevice_reg_msg_proc() Enter---: register_id=%s,login_ip=%s,login_port=%d \r\n", pMsg->register_id, pMsg->login_ip, pMsg->login_port);
    printf("\r\nGBDevice_reg_msg_proc() Enter---: register_id=%s,login_ip=%s,login_port=%d \r\n", pMsg->register_id, pMsg->login_ip, pMsg->login_port);

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�豸ע�ᴦ��:�豸ID=%s, �豸IP��ַ=%s, �˿ں�=%d", pMsg->register_id, pMsg->login_ip, pMsg->login_port);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Device registration processing:ID=%s, IP=%s, port=%d", pMsg->register_id, pMsg->login_ip, pMsg->login_port);

    /*��֤����*/
    if (!pMsg || pMsg->reg_info_index < 0)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "GBDevice_reg_msg_proc() exit---: pMsg/ reg_info_indext Error \r\n");
        return -1;
    }

    int reg_info_index = pMsg->reg_info_index;

    if (!(pMsg->login_ip) || !(pMsg->register_id) | (pMsg->login_port <= 0))
    {
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_REG_MSG_ERROR);
        SIP_UASAnswerToRegister(reg_info_index, 403, strErrorCode);
        //SIP_UASAnswerToRegister(reg_info_index, 403, (char*)"register_id/ IP /port Error");
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GBDevice_reg_msg_proc() exit---: register_id/ IP /port  Error \r\n");
        SystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "�豸ע��ʧ��:�豸ID=%s, �豸IP��ַ=%s, �˿ں�=%d, ԭ��=%s", pMsg->register_id, pMsg->login_ip, pMsg->login_port, (char*)"�豸ID/�豸IP��/�豸�˿ںŲ��Ϸ�");
        EnSystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "Device registration failed:device ID=%s, IP address=%s, port number=%d, cause=%s", pMsg->register_id, pMsg->login_ip, pMsg->login_port, (char*)"device IP device ID/ the port number is not valid");

        return -1;
    }

    int i = 0;
    GBDevice_info_t* pGBDeviceInfo = NULL;
    int iExpires = pMsg->expires;
    bool bIsNewDev = false;   //��ע���豸
    GBDevice_cfg_t GBDevice_cfg;

    if (!is_need_auth()) /* �������Ҫ��֤�����õǼǣ�ֱ����� */
    {
        /* ��ȡ�����豸��Ϣ */
        if (GetDevCfg(pdboper, pMsg->register_id, GBDevice_cfg) <= 0) //��ȡ��Ϣʧ��
        {

#if 0

            if (iExpires > 0 && checkNumberOfGBDeviceInfo() > g_RegistrationLimit)
            {
                SIP_UASAnswerToRegister(reg_info_index, 403, "Number of Registration Limited");
                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "GBDevice_reg_msg_proc() exit---: Number of Registration Limited \r\n");
                SystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "�豸ע��ʧ��:�豸ID=%s, IP��ַ=%s, �˿ں�=%d, ԭ��=%s", pMsg->register_id, pMsg->login_ip, pMsg->login_port, (char*)"�豸ע�����ﵽ����");
                return -1;
            }

#endif
            /* �������豸��Ϣֱ��д�������豸�� */
            i = WriteGBPhyDeviceInfoToDB(pMsg->register_id, pMsg->register_name, pMsg->login_ip, pMsg->link_type, pdboper);

            /* �ٴλ�ȡ�����豸��Ϣ */
            if (GetDevCfg(pdboper, pMsg->register_id, GBDevice_cfg) <= 0) //��ȡ��Ϣʧ��
            {
                i = RegisterSetNotFoundGBDevice2GBPhyDeviceTmpDB(pMsg->register_id, pMsg->register_name, pMsg->login_ip, pMsg->link_type, pdboper);

                memset(strErrorCode, 0, 32);
                snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_DEVICE_GET_DEVICE_INFO_ERROR);
                SIP_UASAnswerToRegister(reg_info_index, 403, strErrorCode);
                //SIP_UASAnswerToRegister(reg_info_index, 404, (char*)"Find GBDevice Info Error");
                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GBDevice_reg_msg_proc() exit---: Find GBDevice Info Error \r\n");
                SystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "�豸ע��ʧ��:�豸ID=%s, �豸IP��ַ=%s, �˿ں�=%d, ԭ��=%s", pMsg->register_id, pMsg->login_ip, pMsg->login_port, (char*)"���ݿ���û���ҵ����豸");
                EnSystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "Device registration failed:device ID=%s, IP address=%s, port number=%d, cause=%s", pMsg->register_id, pMsg->login_ip, pMsg->login_port, (char*)"The device was not found in the database.");

                return -1;
            }
        }
        else
        {
            /* �������CMS ID�����ϣ������һ�����ݿ� */
            if (0 != sstrcmp(GBDevice_cfg.cms_id.c_str(), local_cms_id_get()))
            {
                UpdateGBDeviceCMSID2DB(pMsg->register_id, pdboper);
            }
        }
    }
    else if (IsLocalAuthRealm(pMsg->login_ip)) /* ����Ǻ�CMSͬһ��IP�������豸�����õǼǣ�ֱ����� */
    {
        if (GetDevCfg(pdboper, pMsg->register_id, GBDevice_cfg) <= 0) //��ȡ��Ϣʧ��
        {
            /* �������豸��Ϣֱ��д�������豸�� */
            i = WriteGBPhyDeviceInfoToDB(pMsg->register_id, pMsg->register_name, pMsg->login_ip, pMsg->link_type, pdboper);

            /* �ٴλ�ȡ�����豸��Ϣ */
            if (GetDevCfg(pdboper, pMsg->register_id, GBDevice_cfg) <= 0) //��ȡ��Ϣʧ��
            {
                i = RegisterSetNotFoundGBDevice2GBPhyDeviceTmpDB(pMsg->register_id, pMsg->register_name, pMsg->login_ip, pMsg->link_type, pdboper);

                memset(strErrorCode, 0, 32);
                snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_DEVICE_GET_DEVICE_INFO_ERROR);
                SIP_UASAnswerToRegister(reg_info_index, 403, strErrorCode);
                //SIP_UASAnswerToRegister(reg_info_index, 404, (char*)"Find GBDevice Info Error");
                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GBDevice_reg_msg_proc() exit---: Find GBDevice Info Error \r\n");
                SystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "�豸ע��ʧ��:�豸ID=%s, �豸IP��ַ=%s, �˿ں�=%d, ԭ��=%s", pMsg->register_id, pMsg->login_ip, pMsg->login_port, (char*)"���ݿ���û���ҵ����豸");
                EnSystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "Device registration failed:device ID=%s, IP address=%s, port number=%d, cause=%s", pMsg->register_id, pMsg->login_ip, pMsg->login_port, (char*)"The device was not found in the database.");

                return -1;
            }
        }
        else/* ������Ǻ�CMSͬһ��IP�������豸������Ҫ�ȵǼǣ������ */
        {
            /* �������CMS ID�����ϣ������һ�����ݿ� */
            if (0 != sstrcmp(GBDevice_cfg.cms_id.c_str(), local_cms_id_get()))
            {
                UpdateGBDeviceCMSID2DB(pMsg->register_id, pdboper);
            }
        }
    }
    else
    {
        if (GetDevCfg(pdboper, pMsg->register_id, GBDevice_cfg) <= 0) //��ȡ��Ϣʧ��
        {
            i = RegisterSetNotFoundGBDevice2GBPhyDeviceTmpDB(pMsg->register_id, pMsg->register_name, pMsg->login_ip, pMsg->link_type, pdboper);

            memset(strErrorCode, 0, 32);
            snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_DEVICE_GET_DEVICE_INFO_ERROR);
            SIP_UASAnswerToRegister(reg_info_index, 403, strErrorCode);
            //SIP_UASAnswerToRegister(reg_info_index, 404, (char*)"Find GBDevice Info Error");
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GBDevice_reg_msg_proc() exit---: Find GBDevice Info Error \r\n");
            SystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "�豸ע��, ���ȵǼ��豸:�豸ID=%s, �豸IP��ַ=%s, �˿ں�=%d", pMsg->register_id, pMsg->login_ip, pMsg->login_port);
            EnSystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "Device registration, please register device first:device ID=%s, device IP address =%s, port number=%d", pMsg->register_id, pMsg->login_ip, pMsg->login_port);

            return -1;
        }
        else
        {
            /* �������CMS ID�����ϣ������һ�����ݿ� */
            if (0 != sstrcmp(GBDevice_cfg.cms_id.c_str(), local_cms_id_get()))
            {
                UpdateGBDeviceCMSID2DB(pMsg->register_id, pdboper);
            }
        }
    }

    /* ����豸�Ƿ����� */
    if (0 == GBDevice_cfg.enable)/*�豸û������*/
    {
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_DEVICE_NOT_ENABLE_ERROR);
        SIP_UASAnswerToRegister(reg_info_index, 403, strErrorCode);
        //SIP_UASAnswerToRegister(reg_info_index, 403, (char*)"GBDevice Not Enable");

        /* д����ʱ�� */
        i = RegisterSetNotFoundGBDevice2GBPhyDeviceTmpDB(pMsg->register_id, pMsg->register_name, pMsg->login_ip, pMsg->link_type, pdboper);

        SystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "�豸δ����:�豸ID=%s, IP��ַ=%s", pMsg->register_id, pMsg->login_ip);
        EnSystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "The equipment is not enabled:ID=%s, IP=%s", pMsg->register_id, pMsg->login_ip);

        //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "GBDevice_reg_msg_proc() StopAllServiceTaskByGBDeviceIndex() Enter------------: DeviceID=%s \r\n", pGBDeviceInfo->device_id);

        pGBDeviceInfo = GBDevice_info_find(pMsg->register_id);

        if (NULL != pGBDeviceInfo)
        {
            /* ��������˽ṹ��������豸���ͣ���ô��Ҫɾ�����˽ṹ����Ϣ�����ϱ����ϼ�CMS */
            if (EV9000_DEVICETYPE_DECODER == pGBDeviceInfo->device_type
                || EV9000_DEVICETYPE_SIPSERVER == pGBDeviceInfo->device_type
                || EV9000_DEVICETYPE_MGWSERVER == pGBDeviceInfo->device_type)
            {
                /* ɾ�����˽ṹ����Ϣ */
                i = DeleteTopologyPhyDeviceInfoFromDB(pGBDeviceInfo->device_id, pdboper);
            }

            /* ���ҵ�λ��ҵ�񣬲�ֹͣ����ҵ�� */
            if (EV9000_DEVICETYPE_DECODER == pGBDeviceInfo->device_type) /* �������������в���Ϣֹͣҵ�� */
            {
                i = StopAllServiceTaskByCallerIPAndPort(pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);

                if (0 != i)
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GBDevice_reg_msg_proc() StopAllServiceTaskByCallerIPAndPort Error:device_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "GBDevice_reg_msg_proc() StopAllServiceTaskByCallerIPAndPort OK:device_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, i);
                }
            }
            else if (EV9000_DEVICETYPE_INTELLIGENTANALYSIS == pGBDeviceInfo->device_type) /* IVS�������С����в���Ϣֹͣҵ�� */
            {
                i = StopAllServiceTaskByCallerIPAndPort(pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port); /* �����������Ƶ�� */

                if (0 != i)
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GBDevice_reg_msg_proc() StopAllServiceTaskByCallerIPAndPort Error:device_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "GBDevice_reg_msg_proc() StopAllServiceTaskByCallerIPAndPort OK:device_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, i);
                }

                i = StopAllServiceTaskByCalleeIPAndPort(pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port); /* �������͵����ܷ���Ƶ�� */

                if (0 != i)
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GBDevice_reg_msg_proc() StopAllServiceTaskByCalleeIPAndPort Error:device_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "GBDevice_reg_msg_proc() StopAllServiceTaskByCalleeIPAndPort OK:device_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, i);
                }
            }
            else if (EV9000_DEVICETYPE_SIPSERVER == pGBDeviceInfo->device_type) /* �¼�CMS */
            {
                i = StopAllServiceTaskByCalleeIPAndPort(pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);

                if (0 != i)
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GBDevice_reg_msg_proc() StopAllServiceTaskByCalleeIPAndPort Error:device_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "GBDevice_reg_msg_proc() StopAllServiceTaskByCalleeIPAndPort OK:device_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, i);
                }
            }
            else
            {
                i = StopAllServiceTaskByCalleeIPAndPort(pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);

                if (0 != i)
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GBDevice_reg_msg_proc() StopAllServiceTaskByCalleeIPAndPort Error:device_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "GBDevice_reg_msg_proc() StopAllServiceTaskByCalleeIPAndPort OK:device_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, i);
                }
            }

            if (EV9000_DEVICETYPE_INTELLIGENTANALYSIS == pGBDeviceInfo->device_type)
            {
                /* ֪ͨ�ͻ��ˣ��߼��豸���ܷ������� */
                i = SendAllGBLogicDeviceIntelligentStatusOffAlarmToAllClientUser(pGBDeviceInfo->id);

                if (0 != i)
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GBDevice_reg_msg_proc() SendAllGBLogicDeviceIntelligentStatusOffAlarmToAllClientUser Error:device_id=%s, i=%d \r\n", pGBDeviceInfo->device_id, i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "GBDevice_reg_msg_proc() SendAllGBLogicDeviceIntelligentStatusOffAlarmToAllClientUser OK:device_id=%s, i=%d \r\n", pGBDeviceInfo->device_id, i);
                }
            }
            else
            {
                /* ֪ͨ�ͻ��ˣ��߼��豸���� */
                i = SendAllGBLogicDeviceStatusOffAlarmToAllClientUser(pGBDeviceInfo->id);

                if (0 != i)
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GBDevice_reg_msg_proc() SendAllGBLogicDeviceStatusOffAlarmToAllClientUser Error:device_id=%s, i=%d \r\n", pGBDeviceInfo->device_id, i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "GBDevice_reg_msg_proc() SendAllGBLogicDeviceStatusOffAlarmToAllClientUser OK:device_id=%s, i=%d \r\n", pGBDeviceInfo->device_id, i);
                }
            }

            if (EV9000_DEVICETYPE_INTELLIGENTANALYSIS == pGBDeviceInfo->device_type)
            {
                /* �������ܷ����豸״̬��Ϣ���ͻ��� */
                i = SendAllIntelligentGBLogicDeviceStatusProc(pGBDeviceInfo->id, 0, pdboper);

                if (0 != i)
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GBDevice_reg_msg_proc() SendAllIntelligentGBLogicDeviceStatusProc Error:device_id=%s, i=%d \r\n", pGBDeviceInfo->device_id, i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "GBDevice_reg_msg_proc() SendAllIntelligentGBLogicDeviceStatusProc OK:device_id=%s, i=%d \r\n", pGBDeviceInfo->device_id, i);
                }
            }
            else
            {
                /* �����豸״̬��Ϣ���ͻ��� */
                i = SendAllGBLogicDeviceStatusProc(pGBDeviceInfo->id, 0, 0, pdboper);

                if (0 != i)
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GBDevice_reg_msg_proc() SendAllGBLogicDeviceStatusProc Error:device_id=%s, i=%d \r\n", pGBDeviceInfo->device_id, i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "GBDevice_reg_msg_proc() SendAllGBLogicDeviceStatusProc OK:device_id=%s, i=%d \r\n", pGBDeviceInfo->device_id, i);
                }
            }

            /* �����豸ҵ���߳� */
            if (EV9000_DEVICETYPE_SIPSERVER == pGBDeviceInfo->device_type
                || EV9000_DEVICETYPE_MGWSERVER == pGBDeviceInfo->device_type)
            {
                i = device_srv_proc_thread_recycle(pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);

                if (0 != i)
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GBDevice_reg_msg_proc device_srv_proc_thread_recycle Error:device_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "GBDevice_reg_msg_proc() device_srv_proc_thread_recycle OK:device_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, i);
                }
            }

            /* �Ƴ������豸 */
            GBDevice_info_remove(pMsg->register_id);

            /* �Ƴ��߼��豸 */
            if (EV9000_DEVICETYPE_INTELLIGENTANALYSIS == pGBDeviceInfo->device_type)
            {
                //�����߼��豸�����ܷ���״̬
                i = SetGBLogicDeviceIntelligentStatus(pGBDeviceInfo->id, INTELLIGENT_STATUS_NULL);

                if (0 != i)
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GBDevice_reg_msg_proc() SetGBLogicDeviceIntelligentStatus Error:device_id=%s, i=%d \r\n", pGBDeviceInfo->device_id, i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "GBDevice_reg_msg_proc() SetGBLogicDeviceIntelligentStatus OK:device_id=%s, i=%d \r\n", pGBDeviceInfo->device_id, i);
                }
            }
            else
            {
                //�Ƴ������Ӧ���߼��豸
                i = RemoveGBLogicDevice(pGBDeviceInfo->id);

                if (0 != i)
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GBDevice_reg_msg_proc() RemoveGBLogicDevice Error:device_id=%s, i=%d \r\n", pGBDeviceInfo->device_id, i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "GBDevice_reg_msg_proc() RemoveGBLogicDevice OK:device_id=%s, i=%d \r\n", pGBDeviceInfo->device_id, i);
                }
            }

            GBDevice_info_free(pGBDeviceInfo);
            delete pGBDeviceInfo;
            pGBDeviceInfo = NULL;
        }

        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GBDevice_reg_msg_proc() exit---: GBDevice Not Enable:register_id=%s, register_name=%s, login_ip=%s \r\n", pMsg->register_id, pMsg->register_name, pMsg->login_ip);
        SystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "�豸ע��ʧ��:�豸ID=%s, �豸IP��ַ=%s, �˿ں�=%d, ԭ��=%s", pMsg->register_id, pMsg->login_ip, pMsg->login_port, (char*)"�豸�Ѿ�������");
        EnSystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "Device registration failed:device ID=%s, IP address=%s, port number=%d, cause=%s", pMsg->register_id, pMsg->login_ip, pMsg->login_port, (char*)"Device has been disabled");
        return -1;
    }

#if 0
    if (iExpires > 0 && checkNumberOfGBDeviceInfo() > g_RegistrationLimit)
    {
        SIP_UASAnswerToRegister(reg_info_index, 403, "Number of Registration Limited");
        DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "GBDevice_reg_msg_proc() exit---: Number of Registration Limited \r\n");
        SystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "�豸ע��ʧ��:�豸ID=%s, IP��ַ=%s, �˿ں�=%d, ԭ��=%s", pMsg->register_id, pMsg->login_ip, pMsg->login_port, (char*)"�豸ע�����ﵽ����");
        return -1;
    }
#endif

    /* ����Ǳ����ϱ����豸���Ͳ����IP��ַ�Ƿ��б仯�� */
    if (GBDevice_cfg.device_ip.length() > 0 && !IsLocalHost(pMsg->login_ip))
    {
        /* �鿴�豸�ĵ�¼ip��ַ�Ƿ��б仯 */
        if (0 != sstrcmp(GBDevice_cfg.device_ip.c_str(), pMsg->login_ip))
        {
#if 0
            /* д����ʱ�� */
            i = RegisterSetIPConflictGBDevice2GBPhyDeviceTmpDB(pMsg->register_id, pMsg->register_name, pMsg->login_ip, pMsg->link_type, pdboper);

            memset(strErrorCode, 0, 32);
            snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_DEVICE_REG_IP_CONFLICT_ERROR);
            SIP_UASAnswerToRegister(reg_info_index, 403, strErrorCode);
            //SIP_UASAnswerToRegister(reg_info_index, 403, (char*)"Login IP Conflict");
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GBDevice_reg_msg_proc() exit---: Login IP Conflict \r\n");
            SystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "�豸ע��ʧ��:�豸ID=%s, �豸IP��ַ=%s, �˿ں�=%d, ԭ��=%s%s", pMsg->register_id, pMsg->login_ip, pMsg->login_port, (char*)"�豸IP��ַ��֮ǰע��ĳ�ͻ,ԭע��IP=", GBDevice_cfg.device_ip.c_str());
            EnSystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "Device registration failed:device ID=%s, IP address=%s, port number=%d, cause=%s%s", pMsg->register_id, pMsg->login_ip, pMsg->login_port, (char*)"device IP address is conflict with one registered before,old registration IP=", GBDevice_cfg.device_ip.c_str());
            return -1;
#endif
            /* ��Ӧ������Ӧ�ã�IP��ַ���ܱ仯 */
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "�豸ע��IP��ַ�����仯, ���ܴ���ID�ظ�����:�豸ID=%s, �豸ԭ��ע��IP��ַ=%s, �µ�ע��IP��ַ=%s", pMsg->register_id, GBDevice_cfg.device_ip.c_str(), pMsg->login_ip);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Equipment registered IP address changes, there may be repeat configuration ID:ID=%s, old IP=%s, new IP=%s", pMsg->register_id, GBDevice_cfg.device_ip.c_str(), pMsg->login_ip);
        }
    }

    if (!GBDevice_info_find(pMsg->register_id))   //��ע���豸�����ڴ��¼
    {
        GBDevice_info_t* pNewGBDeviceInfo = NULL;

        i = GBDevice_info_init(&pNewGBDeviceInfo);

        if (i != 0 || pNewGBDeviceInfo == NULL)
        {
            memset(strErrorCode, 0, 32);
            snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_DEVICE_CREAT_DEVICE_INFO_ERROR);
            SIP_UASAnswerToRegister(reg_info_index, 403, strErrorCode);
            //SIP_UASAnswerToRegister(reg_info_index, 503, (char*)"Create Device Error");
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GBDevice_reg_msg_proc new GBDevice_info_t() fail  \r\n");
            SystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "�豸ע��ʧ��:�豸ID=%s, �豸IP��ַ=%s, �˿ں�=%d, ԭ��=%s", pMsg->register_id, pMsg->login_ip, pMsg->login_port, (char*)"����豸��Ϣʧ��");
            EnSystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "Device registration failed:device ID=%s, IP address=%s, port number=%d, cause=%s", pMsg->register_id, pMsg->login_ip, pMsg->login_port, (char*)"Add device information failed.");
            return -1;
        }

        pNewGBDeviceInfo->id = GBDevice_cfg.id;
        osip_strncpy(pNewGBDeviceInfo->device_id, pMsg->register_id, MAX_ID_LEN);
        pNewGBDeviceInfo->device_type = pMsg->device_type;
        pNewGBDeviceInfo->link_type = GBDevice_cfg.link_type;
        osip_strncpy(pNewGBDeviceInfo->login_ip, pMsg->login_ip, MAX_IP_LEN);
        pNewGBDeviceInfo->login_port = pMsg->login_port;
        //pNewGBDeviceInfo->reg_info_index = reg_info_index;
        bIsNewDev = true;   //��dev��ʶ

        /* ƽ̨�豸��Ĭ���ǵ���������ֹDeviceInfo��Ϣ����Ӧ */
        if (EV9000_DEVICETYPE_SIPSERVER == pNewGBDeviceInfo->device_type)
        {
            pNewGBDeviceInfo->three_party_flag = 1;
        }

        if (0 != GBDevice_info_add(pNewGBDeviceInfo))  /* ��ӵ�����*/
        {
            GBDevice_info_free(pNewGBDeviceInfo);
            delete pNewGBDeviceInfo;
            pNewGBDeviceInfo = NULL;

            memset(strErrorCode, 0, 32);
            snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_DEVICE_CREAT_DEVICE_INFO_ERROR);
            SIP_UASAnswerToRegister(reg_info_index, 403, strErrorCode);
            //SIP_UASAnswerToRegister(reg_info_index, 503, (char*)"add Dev Info fail");
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GBDevice_reg_msg_proc add Dev Info Error fail  \r\n");
            SystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "�豸ע��ʧ��:�豸ID=%s, �豸IP��ַ=%s, �˿ں�=%d, ԭ��=%s", pMsg->register_id, pMsg->login_ip, pMsg->login_port, (char*)"����豸��Ϣʧ��");
            EnSystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "Device registration failed:device ID=%s, IP address=%s, port number=%d, cause=%s", pMsg->register_id, pMsg->login_ip, pMsg->login_port, (char*)"Add device information failed.");

            return -1;
        }
    }

    /* ��ȡ�豸��Ϣ */
    pGBDeviceInfo = GBDevice_info_find(pMsg->register_id);

    if (NULL == pGBDeviceInfo)
    {
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_DEVICE_FIND_DEVICE_INFO_ERROR);
        SIP_UASAnswerToRegister(reg_info_index, 403, strErrorCode);
        //SIP_UASAnswerToRegister(reg_info_index, 503, (char*)"Get GBDevice Info Error");
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GBDevice_reg_msg_proc() exit---: Get GBDevice Info Error \r\n");
        SystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "�豸ע��ʧ��:�豸ID=%s, �豸IP��ַ=%s, �˿ں�=%d, ԭ��=%s", pMsg->register_id, pMsg->login_ip, pMsg->login_port, (char*)"��ȡ�豸��Ϣʧ��");
        EnSystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "Device registration failed:device ID=%s, IP address=%s, port number=%d, cause=%s", pMsg->register_id, pMsg->login_ip, pMsg->login_port, (char*)"Access device information failed.");

        return -1;
    }

    /* ע��IP��ַ */
    if (0 != sstrcmp(pGBDeviceInfo->login_ip, pMsg->login_ip))
    {
        memset(pGBDeviceInfo->login_ip, 0, MAX_IP_LEN);
        osip_strncpy(pGBDeviceInfo->login_ip, pMsg->login_ip, MAX_IP_LEN);
    }

    /* ע��˿ں� */
    pGBDeviceInfo->login_port = pMsg->login_port;

    /* ע���������¸�ֵ */
    if (pGBDeviceInfo->reg_info_index != reg_info_index)
    {
        pGBDeviceInfo->reg_info_index = reg_info_index;
    }

    /* �豸�������¸�ֵ */
    if (pGBDeviceInfo->id != GBDevice_cfg.id)
    {
        pGBDeviceInfo->id = GBDevice_cfg.id;
    }

    if ((EV9000_DEVICETYPE_SIPSERVER == pGBDeviceInfo->device_type && 0 == pGBDeviceInfo->three_party_flag)
        || EV9000_DEVICETYPE_DECODER == pGBDeviceInfo->device_type
        || EV9000_DEVICETYPE_MGWSERVER == pGBDeviceInfo->device_type)
    {

    }
    else
    {
        /* ���䷽ʽ���¸�ֵ */
        if (pGBDeviceInfo->trans_protocol != GBDevice_cfg.trans_protocol)
        {
            pGBDeviceInfo->trans_protocol = GBDevice_cfg.trans_protocol;
        }
    }

    /* �豸�������¸�ֵ */
    if (pGBDeviceInfo->device_type != pMsg->device_type)
    {
        pGBDeviceInfo->device_type = pMsg->device_type;
    }

    /* �����������¸�ֵ */
    if (pGBDeviceInfo->link_type != pMsg->link_type)
    {
        pGBDeviceInfo->link_type = pMsg->link_type;

        if (pGBDeviceInfo->device_type == EV9000_DEVICETYPE_SIPSERVER)
        {
            UpdateGBDeviceLinkType2DB(pGBDeviceInfo, pdboper);
        }
    }

    /* ��ȡ��ʱʱ�� */
    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "GBDevice_reg_msg_proc() DeviceInfo:register_id=%s, iExpires=%d, reg_status=%d, reg_info_index=%d \r\n", pGBDeviceInfo->device_id, iExpires, pGBDeviceInfo->reg_status, pGBDeviceInfo->reg_info_index);
    printf("GBDevice_reg_msg_proc() DeviceInfo:register_id=%s, iExpires=%d, reg_status=%d, reg_info_index=%d \r\n", pGBDeviceInfo->device_id, iExpires, pGBDeviceInfo->reg_status, pGBDeviceInfo->reg_info_index);

    if (iExpires > 0)       //���߲���
    {
        i = DevReg(pGBDeviceInfo, GBDevice_cfg, pdboper);

        if (i != 0)
        {
            pGBDeviceInfo->reg_info_index = -1;
            pGBDeviceInfo->reg_status = 0;
            //memset(pGBDeviceInfo->login_ip, 0, 16);
            //pGBDeviceInfo->login_port = 0;
        }

        return i;
    }

    memset(strErrorCode, 0, 32);
    snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_SYSTEM_ERROR);
    SIP_UASAnswerToRegister(reg_info_index, 500, strErrorCode);
    //SIP_UASAnswerToRegister(reg_info_index, 500, (char*)"Server Error");
    DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GBDevice_reg_msg_proc() exit---: Server Error \r\n");
    return -1;
}

/*****************************************************************************
 �� �� ��  : GBDevice_unreg_msg_proc
 ��������  : �豸ע����Ϣ����
 �������  : GBDevice_reg_msg_t* pMsg
             DBOper* pdboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2017��3��16��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int GBDevice_unreg_msg_proc(GBDevice_reg_msg_t* pMsg, DBOper* pdboper)
{
    int i = 0;
    int reg_info_index = 0;
    GBDevice_info_t* pGBDeviceInfo = NULL;
    int iExpires = pMsg->expires;
    char strErrorCode[32] = {0};

    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "GBDevice_unreg_msg_proc() Enter---: register_id=%s,login_ip=%s,login_port=%d \r\n", pMsg->register_id, pMsg->login_ip, pMsg->login_port);
    printf("\r\nGBDevice_unreg_msg_proc() Enter---: register_id=%s,login_ip=%s,login_port=%d \r\n", pMsg->register_id, pMsg->login_ip, pMsg->login_port);

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�豸ע�ᴦ��:�豸ID=%s, �豸IP��ַ=%s, �˿ں�=%d", pMsg->register_id, pMsg->login_ip, pMsg->login_port);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Device registration processing:ID=%s, IP=%s, port=%d", pMsg->register_id, pMsg->login_ip, pMsg->login_port);

    /*��֤����*/
    if (!pMsg || pMsg->reg_info_index < 0)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "GBDevice_unreg_msg_proc() exit---: pMsg/ reg_info_indext Error \r\n");
        return -1;
    }

    reg_info_index = pMsg->reg_info_index;

    if (!(pMsg->login_ip) || !(pMsg->register_id) | (pMsg->login_port <= 0))
    {
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_REG_MSG_ERROR);
        SIP_UASAnswerToRegister(reg_info_index, 403, strErrorCode);
        //SIP_UASAnswerToRegister(reg_info_index, 403, (char*)"register_id/ IP /port Error");
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GBDevice_unreg_msg_proc() exit---: register_id/ IP /port  Error \r\n");
        SystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "�豸ע��ʧ��:�豸ID=%s, �豸IP��ַ=%s, �˿ں�=%d, ԭ��=%s", pMsg->register_id, pMsg->login_ip, pMsg->login_port, (char*)"�豸ID/�豸IP��/�豸�˿ںŲ��Ϸ�");
        EnSystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "Device registration failed:device ID=%s, IP address=%s, port number=%d, cause=%s", pMsg->register_id, pMsg->login_ip, pMsg->login_port, (char*)"device IP device ID/ the port number is not valid");

        return -1;
    }

    /* ��ȡ�豸��Ϣ */
    pGBDeviceInfo = GBDevice_info_find(pMsg->register_id);

    if (NULL == pGBDeviceInfo)
    {
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_DEVICE_FIND_DEVICE_INFO_ERROR);
        SIP_UASAnswerToRegister(reg_info_index, 403, strErrorCode);
        //SIP_UASAnswerToRegister(reg_info_index, 503, (char*)"Get GBDevice Info Error");
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GBDevice_unreg_msg_proc() exit---: Get GBDevice Info Error \r\n");
        SystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "�豸ע��ʧ��:�豸ID=%s, �豸IP��ַ=%s, �˿ں�=%d, ԭ��=%s", pMsg->register_id, pMsg->login_ip, pMsg->login_port, (char*)"��ȡ�豸��Ϣʧ��");
        EnSystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "Device registration failed:device ID=%s, IP address=%s, port number=%d, cause=%s", pMsg->register_id, pMsg->login_ip, pMsg->login_port, (char*)"Access device information failed.");
        return -1;
    }

    /* ��ȡ��ʱʱ�� */
    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "GBDevice_unreg_msg_proc() DeviceInfo:register_id=%s, iExpires=%d, reg_status=%d, reg_info_index=%d \r\n", pGBDeviceInfo->device_id, iExpires, pGBDeviceInfo->reg_status, pGBDeviceInfo->reg_info_index);
    printf("GBDevice_unreg_msg_proc() DeviceInfo:register_id=%s, iExpires=%d, reg_status=%d, reg_info_index=%d \r\n", pGBDeviceInfo->device_id, iExpires, pGBDeviceInfo->reg_status, pGBDeviceInfo->reg_info_index);

    if (iExpires <= 0)   //ע��  ���߲���
    {
        i = DevUnReg(pGBDeviceInfo, pdboper);
        pGBDeviceInfo->reg_info_index = -1;
        pGBDeviceInfo->reg_status = 0;
        //memset(pGBDeviceInfo->login_ip, 0, 16);
        //pGBDeviceInfo->login_port = 0;
        return i;
    }

    memset(strErrorCode, 0, 32);
    snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_SYSTEM_ERROR);
    SIP_UASAnswerToRegister(reg_info_index, 500, strErrorCode);
    //SIP_UASAnswerToRegister(reg_info_index, 500, (char*)"Server Error");
    DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GBDevice_unreg_msg_proc() exit---: Server Error \r\n");
    return -1;
}

/*****************************************************************************
 �� �� ��  : RegisterSetNotFoundGBDevice2GBPhyDeviceTmpDB
 ��������  : ��û���ҵ����豸ע����Ϣд���׼�����豸��ʱ��
 �������  : char* device_id
             char* pUserName
             char* pIPAddr
             int link_type
             DBOper* pdboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��6��8�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int RegisterSetNotFoundGBDevice2GBPhyDeviceTmpDB(char* device_id, char* pUserName, char* pIPAddr, int link_type, DBOper* pdboper)
{
    int iRet = 0;

    if (NULL == device_id)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "RegisterSetNotFoundGBDevice2GBPhyDeviceTmpDB() exit---: Param Error \r\n");
        return -1;
    }

    iRet = WriteErrorInfo2GBPhyDeviceTmpDB(device_id, pUserName, pIPAddr, DEVICE_ERROR_REASON_NOT_FOUND, link_type, pdboper);

    if (0 != iRet)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "RegisterSetNotFoundGBDevice2GBPhyDeviceTmpDB() WriteErrorInfo2GBPhyDeviceTmpDB Error:iRet=%d \r\n", iRet);
    }
    else
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "RegisterSetNotFoundGBDevice2GBPhyDeviceTmpDB() WriteErrorInfo2GBPhyDeviceTmpDB OK:iRet=%d \r\n", iRet);
    }

    return iRet;
}

/*****************************************************************************
 �� �� ��  : RegisterSetIPConflictGBDevice2GBPhyDeviceTmpDB
 ��������  : ����ַ��ͻ���豸ע����Ϣд���׼�����豸��ʱ��
 �������  : char* device_id
             char* pUserName
             char* pIPAddr
             int link_type
             DBOper* pdboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��6��27�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int RegisterSetIPConflictGBDevice2GBPhyDeviceTmpDB(char* device_id, char* pUserName, char* pIPAddr, int link_type, DBOper* pdboper)
{
    int iRet = 0;

    if (NULL == device_id)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "RegisterSetIPConflictGBDevice2GBPhyDeviceTmpDB() exit---: Param Error \r\n");
        return -1;
    }

    iRet = WriteErrorInfo2GBPhyDeviceTmpDB(device_id, pUserName, pIPAddr, DEVICE_ERROR_REASON_IP_CONFLICT, link_type, pdboper);
    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "RegisterSetIPConflictGBDevice2GBPhyDeviceTmpDB() WriteErrorInfo2GBPhyDeviceTmpDB:device_id=%s, iRet=%d \r\n", device_id, iRet);
    return iRet;
}

/*****************************************************************************
 �� �� ��  : WriteErrorInfo2GBPhyDeviceTmpDB
 ��������  : ��������豸��Ϣд���׼�����豸��ʱ��
 �������  : char* device_id
             char* user_name
             char* ip_addr
             device_error_reason_type_t eReason
             int link_type
             DBOper* pdboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��6��8�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int WriteErrorInfo2GBPhyDeviceTmpDB(char* device_id, char* user_name, char* ip_addr, device_error_reason_type_t eReason, int link_type, DBOper* pdboper)
{
    int iRet = 0;
    int record_count = 0;
    string strQuerySQL = "";
    string strInsertSQL = "";
    string strUpdateSQL = "";
    char strReason[16] = {0};
    char strLinkType[16] = {0};
    char strDeviceType[4] = {0};
    char* pTmp = NULL;

    if (NULL == device_id || NULL == pdboper)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "WriteErrorInfo2GBPhyDeviceTmpDB() exit---: Param Error \r\n");
        return -1;
    }

    pTmp = &device_id[10];
    osip_strncpy(strDeviceType, pTmp, 3);

    snprintf(strReason, 16, "%d", eReason);

    snprintf(strLinkType, 16, "%d", link_type);

    /* 1����ѯSQL��� */
    strQuerySQL.clear();
    strQuerySQL = "select * from GBPhyDeviceTempConfig WHERE DeviceID like '";
    strQuerySQL += device_id;
    strQuerySQL += "'";

    /* 2������SQL��� */
    strInsertSQL.clear();
    strInsertSQL = "insert into GBPhyDeviceTempConfig (DeviceID,CMSID,UserName,DeviceIP,Enable,DeviceType,LinkType) values (";

    strInsertSQL += "'";
    strInsertSQL += device_id;
    strInsertSQL += "'";

    strInsertSQL += ",";

    strInsertSQL += "'";
    strInsertSQL += local_cms_id_get();
    strInsertSQL += "'";

    strInsertSQL += ",";

    strInsertSQL += "'";

    if (NULL != user_name)
    {
        strInsertSQL += user_name;
    }

    strInsertSQL += "'";

    strInsertSQL += ",";

    strInsertSQL += "'";

    if (NULL != ip_addr && ip_addr[0] != '\0')
    {
        strInsertSQL += ip_addr;
    }

    strInsertSQL += "'";

    strInsertSQL += ",";

    strInsertSQL += "1";

    strInsertSQL += ",";

    strInsertSQL += strDeviceType;

    strInsertSQL += ",";

    strInsertSQL += strLinkType;

    strInsertSQL += ")";

    /* 3������SQL */
    strUpdateSQL.clear();
    strUpdateSQL = "UPDATE GBPhyDeviceTempConfig SET ";

    /*  CMSID */
    strUpdateSQL += "CMSID = ";
    strUpdateSQL += "'";
    strUpdateSQL += local_cms_id_get();
    strUpdateSQL += "'";
    strUpdateSQL += ",";

    /*  UserName */
    strUpdateSQL += "UserName = ";
    strUpdateSQL += "'";
    strUpdateSQL += user_name;
    strUpdateSQL += "'";
    strUpdateSQL += ",";


    /*  DeviceIP  */
    strUpdateSQL += "DeviceIP = " ;
    strUpdateSQL += "'";
    strUpdateSQL += ip_addr;
    strUpdateSQL += "'";

    strUpdateSQL += " WHERE DeviceID like '";
    strUpdateSQL += device_id;
    strUpdateSQL += "'";

    /* ��ѯ���ݿ� */
    record_count = pdboper->DB_Select(strQuerySQL.c_str(), 1);

    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "WriteErrorInfo2GBPhyDeviceTmpDB() DB Select:strSQL=%s,record_count=%d \r\n", strQuerySQL.c_str(), record_count);

    if (record_count <= 0)
    {
        iRet = pdboper->DB_Insert("", "", strInsertSQL.c_str(), 1);

        if (iRet < 0)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "WriteErrorInfo2GBPhyDeviceTmpDB() DB Oper Error:strInsertSQL=%s, iRet=%d \r\n", strInsertSQL.c_str(), iRet);
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "WriteErrorInfo2GBPhyDeviceTmpDB() ErrorMsg=%s\r\n", pdboper->GetLastDbErrorMsg());
        }
    }
    else
    {
        iRet = pdboper->DB_Update(strUpdateSQL.c_str(), 1);

        if (iRet < 0)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "WriteErrorInfo2GBPhyDeviceTmpDB() DB Oper Error:strUpdateSQL=%s, iRet=%d \r\n", strUpdateSQL.c_str(), iRet);
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "WriteErrorInfo2GBPhyDeviceTmpDB() ErrorMsg=%s\r\n", pdboper->GetLastDbErrorMsg());
        }
    }

    return 0;
}

/*****************************************************************************
 �� �� ��  : WriteGBPhyDeviceInfoToDB
 ��������  : �������豸��Ϣд���׼�����豸��
 �������  : char* device_id
             char* user_name
             char* ip_addr
             int link_type
             DBOper* pdboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��11��13�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int WriteGBPhyDeviceInfoToDB(char* device_id, char* user_name, char* ip_addr, int link_type, DBOper* pdboper)
{
    int iRet = 0;
    int record_count = 0;
    string strQuerySQL = "";
    string strInsertSQL = "";
    string strUpdateSQL = "";
    char strLinkType[16] = {0};
    char strDeviceType[4] = {0};
    char* pTmp = NULL;

    if (NULL == device_id || NULL == pdboper)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "WriteGBPhyDeviceInfoToDB() exit---: Param Error \r\n");
        return -1;
    }

    pTmp = &device_id[10];
    osip_strncpy(strDeviceType, pTmp, 3);

    snprintf(strLinkType, 16, "%d", link_type);

    /* 1����ѯSQL��� */
    strQuerySQL.clear();
    strQuerySQL = "select * from GBPhyDeviceConfig WHERE DeviceID like '";
    strQuerySQL += device_id;
    strQuerySQL += "'";

    /* 2������SQL��� */
    strInsertSQL.clear();
    strInsertSQL = "insert into GBPhyDeviceConfig (DeviceID,CMSID,UserName,DeviceIP,Enable,DeviceType,LinkType) values (";

    strInsertSQL += "'";
    strInsertSQL += device_id;
    strInsertSQL += "'";

    strInsertSQL += ",";

    strInsertSQL += "'";
    strInsertSQL += local_cms_id_get();
    strInsertSQL += "'";

    strInsertSQL += ",";

    strInsertSQL += "'";

    if (NULL != user_name)
    {
        strInsertSQL += user_name;
    }

    strInsertSQL += "'";

    strInsertSQL += ",";

    strInsertSQL += "'";

    if (NULL != ip_addr && ip_addr[0] != '\0')
    {
        strInsertSQL += ip_addr;
    }

    strInsertSQL += "'";

    strInsertSQL += ",";

    strInsertSQL += "1";

    strInsertSQL += ",";

    strInsertSQL += strDeviceType;

    strInsertSQL += ",";

    strInsertSQL += strLinkType;

    strInsertSQL += ")";

    /* 3������SQL */
    strUpdateSQL.clear();
    strUpdateSQL = "UPDATE GBPhyDeviceConfig SET ";

    /*  CMSID */
    strUpdateSQL += "CMSID = ";
    strUpdateSQL += "'";
    strUpdateSQL += local_cms_id_get();
    strUpdateSQL += "'";
    strUpdateSQL += ",";

    /*  UserName */
    strUpdateSQL += "UserName = ";
    strUpdateSQL += "'";
    strUpdateSQL += user_name;
    strUpdateSQL += "'";
    strUpdateSQL += ",";


    /*  DeviceIP  */
    strUpdateSQL += "DeviceIP = " ;
    strUpdateSQL += "'";
    strUpdateSQL += ip_addr;
    strUpdateSQL += "'";

    strUpdateSQL += " WHERE DeviceID like '";
    strUpdateSQL += device_id;
    strUpdateSQL += "'";

    /* ��ѯ���ݿ� */
    record_count = pdboper->DB_Select(strQuerySQL.c_str(), 1);

    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "WriteGBPhyDeviceInfoToDB() DB Select:strSQL=%s,record_count=%d \r\n", strQuerySQL.c_str(), record_count);

    if (record_count <= 0)
    {
        iRet = pdboper->DB_Insert("", "", strInsertSQL.c_str(), 1);

        if (iRet < 0)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "WriteGBPhyDeviceInfoToDB() DB Oper Error:strInsertSQL=%s, iRet=%d \r\n", strInsertSQL.c_str(), iRet);
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "WriteGBPhyDeviceInfoToDB() ErrorMsg=%s\r\n", pdboper->GetLastDbErrorMsg());
        }
    }
    else
    {
        iRet = pdboper->DB_Update(strUpdateSQL.c_str(), 1);

        if (iRet < 0)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "WriteGBPhyDeviceInfoToDB() DB Oper Error:strUpdateSQL=%s, iRet=%d \r\n", strUpdateSQL.c_str(), iRet);
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "WriteGBPhyDeviceInfoToDB() ErrorMsg=%s\r\n", pdboper->GetLastDbErrorMsg());
        }
    }

    return 0;
}
