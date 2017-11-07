
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
#include "common/gblfunc_proc.inc"
#include "common/log_proc.inc"

#include "user/user_reg_proc.inc"
#include "device/device_reg_proc.inc"
#include "user/user_thread_proc.inc"
#include "user/user_srv_proc.inc"

/*----------------------------------------------*
 * �ⲿ����˵��                                 *
 *----------------------------------------------*/
extern gbl_conf_t* pGblconf;              /* ȫ��������Ϣ */

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
user_reg_msg_queue g_UserRegMsgQueue;   /* �û�ע����Ϣ���� */
user_reg_msg_queue g_UserUnRegMsgQueue; /* �û�ע����Ϣ���� */
#ifdef MULTI_THR
osip_mutex_t* g_UserRegMsgQueueLock = NULL;
#endif

/*----------------------------------------------*
 * ��������                                     *
 *----------------------------------------------*/

/*----------------------------------------------*
 * �궨��                                       *
 *----------------------------------------------*/

#if DECS("�û�ע����Ϣ����")
/*****************************************************************************
 �� �� ��  : user_reg_msg_init
 ��������  : �û�ע����Ϣ�ṹ��ʼ��
 �������  : user_reg_msg_t ** user_reg_msg
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��11��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int user_reg_msg_init(user_reg_msg_t** user_reg_msg)
{
    *user_reg_msg = (user_reg_msg_t*)osip_malloc(sizeof(user_reg_msg_t));

    if (*user_reg_msg == NULL)
    {
        DEBUG_TRACE(MODULE_USER, LOG_DEBUG, "user_reg_msg_init() exit---: *user_reg_msg Smalloc Error \r\n");
        return -1;
    }

    (*user_reg_msg)->register_id[0] = '\0';
    (*user_reg_msg)->login_ip[0] = '\0';
    (*user_reg_msg)->login_port = 0;
    (*user_reg_msg)->register_name[0] = '\0';
    (*user_reg_msg)->expires = 0;
    (*user_reg_msg)->reg_info_index = -1;

    return 0;
}

/*****************************************************************************
 �� �� ��  : user_reg_msg_free
 ��������  : �û�ע����Ϣ�ṹ�ͷ�
 �������  : user_reg_msg_t * user_reg_msg
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��11��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void user_reg_msg_free(user_reg_msg_t* user_reg_msg)
{
    if (user_reg_msg == NULL)
    {
        DEBUG_TRACE(MODULE_USER, LOG_DEBUG, "user_reg_msg_free() exit---: Param Error \r\n");
        return;
    }

    memset(user_reg_msg->register_id, 0, MAX_ID_LEN + 4);
    memset(user_reg_msg->login_ip, 0, MAX_IP_LEN);
    user_reg_msg->login_port = 0;
    memset(user_reg_msg->register_name, 0, MAX_128CHAR_STRING_LEN + 4);
    user_reg_msg->expires = 0;
    user_reg_msg->reg_info_index = -1;

    return;
}

/*****************************************************************************
 �� �� ��  : user_reg_msg_list_init
 ��������  : �û�ע����Ϣ���г�ʼ��
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
int user_reg_msg_list_init()
{
    g_UserRegMsgQueue.clear();
    g_UserUnRegMsgQueue.clear();

#ifdef MULTI_THR
    /* init smutex */
    g_UserRegMsgQueueLock = (osip_mutex_t*)osip_mutex_init();

    if (NULL == g_UserRegMsgQueueLock)
    {
        DEBUG_TRACE(MODULE_USER, LOG_DEBUG, "user_reg_msg_list_init() exit---: User Register Message List Lock Init Error \r\n");
        return -1;
    }

#endif
    return 0;
}

/*****************************************************************************
 �� �� ��  : user_reg_msg_list_free
 ��������  : �û�ע����Ϣ�����ͷ�
 �������  : user_reg_msg_list_t * user_reg_msg_list
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��11��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void user_reg_msg_list_free()
{
    user_reg_msg_t* pUserRegMsg = NULL;

    while (!g_UserRegMsgQueue.empty())
    {
        pUserRegMsg = (user_reg_msg_t*) g_UserRegMsgQueue.front();
        g_UserRegMsgQueue.pop_front();

        if (NULL != pUserRegMsg)
        {
            user_reg_msg_free(pUserRegMsg);
            osip_free(pUserRegMsg);
            pUserRegMsg = NULL;
        }
    }

    g_UserRegMsgQueue.clear();

    while (!g_UserUnRegMsgQueue.empty())
    {
        pUserRegMsg = (user_reg_msg_t*) g_UserUnRegMsgQueue.front();
        g_UserUnRegMsgQueue.pop_front();

        if (NULL != pUserRegMsg)
        {
            user_reg_msg_free(pUserRegMsg);
            osip_free(pUserRegMsg);
            pUserRegMsg = NULL;
        }
    }

    g_UserUnRegMsgQueue.clear();

#ifdef MULTI_THR

    if (NULL != g_UserRegMsgQueueLock)
    {
        osip_mutex_destroy((struct osip_mutex*)g_UserRegMsgQueueLock);
        g_UserRegMsgQueueLock = NULL;
    }

#endif

    return;
}

/*****************************************************************************
 �� �� ��  : user_reg_msg_list_clean
 ��������  : �û�ע����Ϣ�������
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
void user_reg_msg_list_clean()
{
    user_reg_msg_t* pUserRegMsg = NULL;

    while (!g_UserRegMsgQueue.empty())
    {
        pUserRegMsg = (user_reg_msg_t*) g_UserRegMsgQueue.front();
        g_UserRegMsgQueue.pop_front();

        if (NULL != pUserRegMsg)
        {
            user_reg_msg_free(pUserRegMsg);
            osip_free(pUserRegMsg);
            pUserRegMsg = NULL;
        }
    }

    g_UserRegMsgQueue.clear();

    return;
}

/*****************************************************************************
 �� �� ��  : user_unreg_msg_list_clean
 ��������  : �û�ע����Ϣ�������
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
void user_unreg_msg_list_clean()
{
    user_reg_msg_t* pUserRegMsg = NULL;

    while (!g_UserUnRegMsgQueue.empty())
    {
        pUserRegMsg = (user_reg_msg_t*) g_UserUnRegMsgQueue.front();
        g_UserUnRegMsgQueue.pop_front();

        if (NULL != pUserRegMsg)
        {
            user_reg_msg_free(pUserRegMsg);
            osip_free(pUserRegMsg);
            pUserRegMsg = NULL;
        }
    }

    g_UserUnRegMsgQueue.clear();

    return;
}

/*****************************************************************************
 �� �� ��  : user_reg_msg_add
 ��������  : ����û�ע����Ϣ��������
 �������  : char* user_id
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
int user_reg_msg_add(char* user_id, char* login_ip, int login_port, char* register_name, int expires, int reg_info_index)
{
    user_reg_msg_t* pUserRegMsg = NULL;
    int iRet = 0;

    if (user_id == NULL || login_ip == NULL)
    {
        DEBUG_TRACE(MODULE_USER, LOG_DEBUG, "user_reg_msg_add() exit---: Param Error \r\n");
        return -1;
    }

    iRet = user_reg_msg_init(&pUserRegMsg);

    if (iRet != 0)
    {
        DEBUG_TRACE(MODULE_USER, LOG_ERROR, "user_reg_msg_add() exit---: Message Init Error \r\n");
        return -1;
    }

    if ('\0' != user_id[0])
    {
        osip_strncpy(pUserRegMsg->register_id, user_id, MAX_ID_LEN);
    }

    if ('\0' != login_ip[0])
    {
        osip_strncpy(pUserRegMsg->login_ip, login_ip, MAX_IP_LEN);
    }

    pUserRegMsg->login_port = login_port;

    if (NULL != register_name && '\0' != register_name[0])
    {
        osip_strncpy(pUserRegMsg->register_name, register_name, MAX_128CHAR_STRING_LEN);
    }
    else
    {
        osip_strncpy(pUserRegMsg->register_name, user_id, MAX_128CHAR_STRING_LEN);
    }

    pUserRegMsg->expires = expires;
    pUserRegMsg->reg_info_index = reg_info_index;

#if 0
    CMS_GBL_SMUTEX_LOCK((struct osip_mutex*)g_UserRegMsgQueueLock);
#endif

    if (expires <= 0)
    {
        g_UserUnRegMsgQueue.push_back(pUserRegMsg);
    }
    else
    {
        g_UserRegMsgQueue.push_back(pUserRegMsg);
    }

#if 0
    CMS_GBL_SMUTEX_UNLOCK((struct osip_mutex*)g_UserRegMsgQueueLock);
#endif

    return 0;
}

/*****************************************************************************
 �� �� ��  : scan_user_reg_msg_list
 ��������  : ɨ���û�ע����Ϣ����
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��5��23�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void scan_user_reg_msg_list(DBOper* pUser_Reg_dboper)
{
    int iRet = 0;
    user_reg_msg_t* pUserRegMsg = NULL;

#if 0
    CMS_GBL_SMUTEX_LOCK((struct osip_mutex*)g_UserRegMsgQueueLock);
#endif

    while (!g_UserRegMsgQueue.empty())
    {
        pUserRegMsg = (user_reg_msg_t*) g_UserRegMsgQueue.front();
        g_UserRegMsgQueue.pop_front();

        if (NULL != pUserRegMsg)
        {
            break;
        }
    }

#if 0
    CMS_GBL_SMUTEX_UNLOCK((struct osip_mutex*)g_UserRegMsgQueueLock);
#endif

    if (NULL != pUserRegMsg)
    {
        DEBUG_TRACE(MODULE_USER, LOG_INFO,  "scan_user_reg_msg_list() \
        \r\n In Param: \
        \r\n register_id=%s \
        \r\n login_ip=%s \
        \r\n login_port=%d \
        \r\n register_name=%s \
        \r\n reg_info_index=%d \
        \r\n expires=%d \
        \r\n ", pUserRegMsg->register_id, pUserRegMsg->login_ip, pUserRegMsg->login_port, pUserRegMsg->register_name, pUserRegMsg->reg_info_index, pUserRegMsg->expires);

        iRet = user_reg_msg_proc(pUserRegMsg, pUser_Reg_dboper); //chenyu
        user_reg_msg_free(pUserRegMsg);
        osip_free(pUserRegMsg);
        pUserRegMsg = NULL;
    }

    return;
}

/*****************************************************************************
 �� �� ��  : scan_user_unreg_msg_list
 ��������  : ɨ���û�ע����Ϣ����
 �������  : DBOper* pUser_Reg_dboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2017��3��7��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
void scan_user_unreg_msg_list(DBOper* pUser_Reg_dboper)
{
    int iRet = 0;
    user_reg_msg_t* pUserRegMsg = NULL;

#if 0
    CMS_GBL_SMUTEX_LOCK((struct osip_mutex*)g_UserRegMsgQueueLock);
#endif

    while (!g_UserUnRegMsgQueue.empty())
    {
        pUserRegMsg = (user_reg_msg_t*) g_UserUnRegMsgQueue.front();
        g_UserUnRegMsgQueue.pop_front();

        if (NULL != pUserRegMsg)
        {
            break;
        }
    }

#if 0
    CMS_GBL_SMUTEX_UNLOCK((struct osip_mutex*)g_UserRegMsgQueueLock);
#endif

    if (NULL != pUserRegMsg)
    {
        DEBUG_TRACE(MODULE_USER, LOG_INFO,  "scan_user_unreg_msg_list() \
        \r\n In Param: \
        \r\n register_id=%s \
        \r\n login_ip=%s \
        \r\n login_port=%d \
        \r\n register_name=%s \
        \r\n reg_info_index=%d \
        \r\n expires=%d \
        \r\n ", pUserRegMsg->register_id, pUserRegMsg->login_ip, pUserRegMsg->login_port, pUserRegMsg->register_name, pUserRegMsg->reg_info_index, pUserRegMsg->expires);

        iRet = user_unreg_msg_proc(pUserRegMsg, pUser_Reg_dboper); //chenyu
        user_reg_msg_free(pUserRegMsg);
        osip_free(pUserRegMsg);
        pUserRegMsg = NULL;
    }

    return;
}
#endif

/*****************************************************************************
 �� �� ��  : GetUserCfg
 ��������  : ��ȡ���ݿ��û�������Ϣ
 �������  : DBOper* pdboper
             string strUserID
             user_cfg_t& user_cfg
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��11��26�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int GetUserCfg(DBOper* pdboper, string strUserID, user_cfg_t& user_cfg)
{
    string strSQL = "";
    int record_count = 0;

    //printf("\r\n GetUserCfg() Enter--- \r\n");

    strSQL = "select * from UserConfig WHERE UserID like '";
    strSQL += strUserID + "';";
    record_count = pdboper->DB_Select(strSQL.c_str(), 1);

    if (record_count < 0)
    {
        DEBUG_TRACE(MODULE_USER, LOG_ERROR, "GetUserCfg() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
        DEBUG_TRACE(MODULE_USER, LOG_ERROR, "GetUserCfg() ErrorMsg=%s\r\n", pdboper->GetLastDbErrorMsg());
        return -1;
    }
    else if (record_count == 0)
    {
        DEBUG_TRACE(MODULE_USER, LOG_WARN, "GetUserCfg() exit---: No Record Count \r\n");
        return 0;
    }

    //���û�
    int tmp_ivalue = 0;
    unsigned int tmp_uvalue = 0;
    string tmp_svalue = "";

    /* �û����� */
    tmp_uvalue = 0;
    pdboper->GetFieldValue("ID", tmp_uvalue);

    user_cfg.id = tmp_uvalue;
    //DEBUG_TRACE(MODULE_USER, LOG_INFO, "GetUserCfg() user_cfg.id:%u", user_cfg.id);

    /* �û�ͳһ���id */
    tmp_svalue.clear();
    pdboper->GetFieldValue("UserID", tmp_svalue);

    if (!tmp_svalue.empty())
    {
        user_cfg.user_id = tmp_svalue;
        //DEBUG_TRACE(MODULE_USER, LOG_INFO, "GetUserCfg() user_cfg.user_id:%s", (char*)user_cfg.user_id.c_str());
    }

    /* �û����� */
    tmp_svalue.clear();
    pdboper->GetFieldValue("UserName", tmp_svalue);

    if (!tmp_svalue.empty())
    {
        user_cfg.user_name = tmp_svalue;
        //DEBUG_TRACE(MODULE_USER, LOG_INFO, "GetUserCfg() user_cfg.user_name:%s", (char*)user_cfg.user_name.c_str());
    }

    /* �Ƿ�����*/
    tmp_ivalue = 0;
    pdboper->GetFieldValue("Enable", tmp_ivalue);

    user_cfg.enable = tmp_ivalue;
    //DEBUG_TRACE(MODULE_USER, LOG_INFO, "GetUserCfg() user_cfg.enable:%d", user_cfg.enable);

    /* �û�ע���˻� */
    tmp_svalue.clear();
    pdboper->GetFieldValue("UserName", tmp_svalue);

    if (!tmp_svalue.empty())
    {
        user_cfg.register_account = tmp_svalue;
        //DEBUG_TRACE(MODULE_USER, LOG_INFO, "GetUserCfg() user_cfg.register_account:%s", (char*)user_cfg.register_account.c_str());
    }

    /* �û�ע������ */
    tmp_svalue.clear();
    pdboper->GetFieldValue("Password", tmp_svalue);

    if (!tmp_svalue.empty())
    {
        user_cfg.register_password = tmp_svalue;
        //DEBUG_TRACE(MODULE_USER, LOG_INFO, "GetUserCfg() user_cfg.register_password:%s", (char*)user_cfg.register_password.c_str());
    }

    /* �û��ȼ�*/
    tmp_uvalue = 0;
    pdboper->GetFieldValue("Level", tmp_uvalue);

    user_cfg.user_level = tmp_uvalue;
    //DEBUG_TRACE(MODULE_USER, LOG_INFO, "GetUserCfg() pUserInfo->user_level:%d", user_cfg.user_level);

    /* �û�Ȩ��*/
    tmp_ivalue = 0;
    pdboper->GetFieldValue("Permission", tmp_ivalue);

    user_cfg.user_permission = tmp_ivalue;

    //printf("\r\n GetUserCfg() Exit--- \r\n");
    //DEBUG_TRACE(MODULE_USER, LOG_INFO, "GetUserCfg() user_cfg.user_permission:%d \r\n", user_cfg.user_permission);
    return record_count;
}

/*****************************************************************************
 �� �� ��  : UserRefresh
 ��������  : �û�ˢ�´���
 �������  : user_info_t* pUserInfo
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
int UserRefresh(user_info_t* pUserInfo, DBOper* pdboper)
{
    pUserInfo->auth_count = 0;
    pUserInfo->reg_status = 1;
    return 0;
}

/*****************************************************************************
 �� �� ��  : UserUnReg
 ��������  : �û�ȥע�ᴦ��
 �������  : user_info_t* pUserInfo
             DBOper* pdboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��10��21�� ���ڶ�
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int UserUnReg(user_info_t* pUserInfo, DBOper* pdboper)
{
    string strUserID = "";
    int i = 0;

    if (NULL == pUserInfo || NULL == pdboper)
    {
        DEBUG_TRACE(MODULE_USER, LOG_DEBUG, "UserUnReg() exit---: Param Error \r\n");
        return -1;
    }

    strUserID = pUserInfo->user_id;

    DEBUG_TRACE(MODULE_USER, LOG_TRACE, "UserUnReg() Enter---:user_id=%s, login_ip=%s, login_port=%d\r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
    printf("UserUnReg() Enter---:user_id=%s, login_ip=%s, login_port=%d\r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);

    SIP_UASAnswerToRegister(pUserInfo->reg_info_index, 200, NULL);

    UserLog(EV9000_USER_LOG_LOGIN, EV9000_LOG_LEVEL_WARNING, pUserInfo, "�û�ע����¼");
    EnUserLog(EV9000_USER_LOG_LOGIN, EV9000_LOG_LEVEL_WARNING, pUserInfo, "User log off.");
    DEBUG_TRACE(MODULE_USER, LOG_INFO, "UserUnReg() REMOVE:user_id=%s, login_ip=%s, login_port=%d, reg_info_index=%d \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, pUserInfo->reg_info_index);

    //i = shdb_user_operate_cmd_proc(pUserInfo, EV9000_SHDB_DVR_SYSTEM_STOP);

    /* �Ƴ��û������ĵ�λ��Ϣ */
    i = RemoveGBLogicDeviceLockInfoByUserInfo(pUserInfo);
    printf("UserUnReg() RemoveGBLogicDeviceLockInfoByUserInfo() Exit--- \r\n");

    if (0 != i)
    {
        DEBUG_TRACE(MODULE_USER, LOG_ERROR, "UserUnReg() RemoveGBLogicDeviceLockInfoByUserInfo Error:user_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, i);
    }
    else
    {
        DEBUG_TRACE(MODULE_USER, LOG_TRACE, "UserUnReg() RemoveGBLogicDeviceLockInfoByUserInfo OK:user_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, i);
    }

    /* �����û���ҵ��ֹͣ����ҵ�� */
    i = StopAllServiceTaskByUserInfo(pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
    printf("UserUnReg() StopAllServiceTaskByUserInfo() Exit--- \r\n");

    if (0 != i)
    {
        DEBUG_TRACE(MODULE_USER, LOG_ERROR, "UserUnReg() StopAllServiceTaskByUserInfo Error:user_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, i);
    }
    else
    {
        DEBUG_TRACE(MODULE_USER, LOG_TRACE, "UserUnReg() StopAllServiceTaskByUserInfo OK:user_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, i);
    }

    /* ֹͣ�û�ҵ�����߳� */
    i = user_srv_proc_thread_recycle(pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
    printf("UserUnReg() user_srv_proc_thread_recycle() Exit--- \r\n");

    if (0 != i)
    {
        DEBUG_TRACE(MODULE_USER, LOG_ERROR, "UserUnReg() user_srv_proc_thread_recycle Error:user_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, i);
    }
    else
    {
        DEBUG_TRACE(MODULE_USER, LOG_TRACE, "UserUnReg() user_srv_proc_thread_recycle OK:user_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, i);
    }

    /* �����ݿ���ɾ�������û���Ϣ */
    i = DeleteUserRegInfoFromDB(pUserInfo->user_index, pUserInfo->login_ip, pUserInfo->login_port, pdboper);
    printf("UserUnReg() DeleteUserRegInfoFromDB() Exit--- \r\n");

    if (i < 0)
    {
        DEBUG_TRACE(MODULE_USER, LOG_ERROR, "UserUnReg() DeleteUserRegInfoFromDB Error:UserID=%s, UserIndex=%u, login_ip=%s, login_port=%d, i=%d \r\n", pUserInfo->user_id, pUserInfo->user_index, pUserInfo->login_ip, pUserInfo->login_port, i);
    }
    else
    {
        DEBUG_TRACE(MODULE_USER, LOG_TRACE, "UserUnReg() DeleteUserRegInfoFromDB OK:UserID=%s, UserIndex=%u, login_ip=%s, login_port=%d, i=%d \r\n", pUserInfo->user_id, pUserInfo->user_index, pUserInfo->login_ip, pUserInfo->login_port, i);
    }

    i = NotifyOnlineUserToAllClientUser(pUserInfo, 2, pdboper);
    printf("UserUnReg() NotifyOnlineUserToAllClientUser() Exit--- \r\n");

    if (i < 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "����֪ͨ�����û��仯��Ϣ�������û�ʧ��, �Ƴ��������û�:�û�ID=%s, IP��ַ=%s, �˿ں�=%d\r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Send notification online users change to online user failure, remove online users: user ID = % s, IP address = % s, port = %d\r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
        DEBUG_TRACE(MODULE_USER, LOG_ERROR, "UserUnReg() NotifyOnlineUserToAllClientUser Error:UserID=%s, UserIndex=%u, login_ip=%s, login_port=%d, i=%d \r\n", pUserInfo->user_id, pUserInfo->user_index, pUserInfo->login_ip, pUserInfo->login_port, i);
    }
    else if (i > 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "����֪ͨ�����û��仯��Ϣ�������û��ɹ�, �Ƴ��������û�:�û�ID=%s, IP��ַ=%s, �˿ں�=%d\r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Send notification online users change to online user failure success, remove online users: user ID = % s, IP address = % s, port = %d\r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
        DEBUG_TRACE(MODULE_USER, LOG_TRACE, "UserUnReg() NotifyOnlineUserToAllClientUser OK:UserID=%s, UserIndex=%u, login_ip=%s, login_port=%d, i=%d \r\n", pUserInfo->user_id, pUserInfo->user_index, pUserInfo->login_ip, pUserInfo->login_port, i);
    }

    /* ɾ���ڴ�ڵ�͸������ݿ���Ϣ */
    i = user_info_remove(pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
    printf("UserUnReg() user_info_remove() Exit--- \r\n");

    if (0 != i)
    {
        DEBUG_TRACE(MODULE_USER, LOG_ERROR, "UserUnReg() user_info_remove Error:UserID=%s, i=%d \r\n", strUserID.c_str(), i);
    }
    else
    {
        DEBUG_TRACE(MODULE_USER, LOG_TRACE, "UserUnReg() user_info_remove OK:UserID=%s, i=%d \r\n", strUserID.c_str(), i);
    }

    i = UpdateUserRegInfo2DB2(strUserID, pdboper);   //�������ݿ�״̬
    printf("UserUnReg() UpdateUserRegInfo2DB2() Exit--- \r\n");

    if (i < 0)
    {
        DEBUG_TRACE(MODULE_USER, LOG_ERROR, "UserUnReg() UpdateUserRegInfo2DB2 Error:UserID=%s, i=%d \r\n", strUserID.c_str(), i);
    }
    else
    {
        DEBUG_TRACE(MODULE_USER, LOG_TRACE, "UserUnReg() UpdateUserRegInfo2DB2 OK:UserID=%s, i=%d \r\n", strUserID.c_str(), i);
    }

    /* �ͷŵ�û�õ�TCP���� */
    free_unused_user_tcp_connect();
    printf("UserUnReg() free_unused_user_tcp_connect() Exit--- \r\n");

    DEBUG_TRACE(MODULE_USER, LOG_TRACE, "UserUnReg() Exit--- \r\n");
    printf("UserUnReg() Exit--- \r\n");

    return 0;
}

/*****************************************************************************
 �� �� ��  : UserUnRegAbnormal
 ��������  : �û��쳣ע��
 �������  : user_info_t* pUserInfo
             DBOper* pdboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2016��3��17��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int UserUnRegAbnormal(user_info_t* pUserInfo, DBOper* pdboper)
{
    string strUserID = "";
    int i = 0;

    if (NULL == pUserInfo || NULL == pdboper)
    {
        DEBUG_TRACE(MODULE_USER, LOG_DEBUG, "UserUnRegAbnormal() exit---: Param Error \r\n");
        return -1;
    }

    strUserID = pUserInfo->user_id;

    DEBUG_TRACE(MODULE_USER, LOG_TRACE, "UserUnRegAbnormal() Enter---:user_id=%s, login_ip=%s, login_port=%d\r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
    printf("UserUnRegAbnormal() Enter---:user_id=%s, login_ip=%s, login_port=%d\r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);

    SIP_UASAnswerToRegister(pUserInfo->reg_info_index, 200, NULL);

    UserLog(EV9000_USER_LOG_LOGIN, EV9000_LOG_LEVEL_WARNING, pUserInfo, "�û�ע����¼");
    EnUserLog(EV9000_USER_LOG_LOGIN, EV9000_LOG_LEVEL_WARNING, pUserInfo, "User log off.");
    DEBUG_TRACE(MODULE_USER, LOG_INFO, "UserUnRegAbnormal() REMOVE:user_id=%s, login_ip=%s, login_port=%d, reg_info_index=%d \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, pUserInfo->reg_info_index);

    //i = shdb_user_operate_cmd_proc(pUserInfo, EV9000_SHDB_DVR_ABNORMAL_STOP);

    /* �Ƴ��û������ĵ�λ��Ϣ */
    i = RemoveGBLogicDeviceLockInfoByUserInfo(pUserInfo);
    printf("UserUnRegAbnormal() RemoveGBLogicDeviceLockInfoByUserInfo() Exit--- \r\n");

    if (0 != i)
    {
        DEBUG_TRACE(MODULE_USER, LOG_ERROR, "UserUnRegAbnormal() RemoveGBLogicDeviceLockInfoByUserInfo Error:user_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, i);
    }
    else
    {
        DEBUG_TRACE(MODULE_USER, LOG_TRACE, "UserUnRegAbnormal() RemoveGBLogicDeviceLockInfoByUserInfo OK:user_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, i);
    }

    /* �����û���ҵ��ֹͣ����ҵ�� */
    i = StopAllServiceTaskByUserInfo(pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
    printf("UserUnRegAbnormal() StopAllServiceTaskByUserInfo() Exit--- \r\n");

    if (0 != i)
    {
        DEBUG_TRACE(MODULE_USER, LOG_ERROR, "UserUnRegAbnormal() StopAllServiceTaskByUserInfo Error:user_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, i);
    }
    else
    {
        DEBUG_TRACE(MODULE_USER, LOG_TRACE, "UserUnRegAbnormal() StopAllServiceTaskByUserInfo OK:user_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, i);
    }

    /* ֹͣ�û�ҵ�����߳� */
    i = user_srv_proc_thread_recycle(pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
    printf("UserUnRegAbnormal() user_srv_proc_thread_recycle() Exit--- \r\n");

    if (0 != i)
    {
        DEBUG_TRACE(MODULE_USER, LOG_ERROR, "UserUnRegAbnormal() user_srv_proc_thread_recycle Error:user_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, i);
    }
    else
    {
        DEBUG_TRACE(MODULE_USER, LOG_TRACE, "UserUnRegAbnormal() user_srv_proc_thread_recycle OK:user_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, i);
    }

    /* �����ݿ���ɾ�������û���Ϣ */
    i = DeleteUserRegInfoFromDB(pUserInfo->user_index, pUserInfo->login_ip, pUserInfo->login_port, pdboper);
    printf("UserUnRegAbnormal() DeleteUserRegInfoFromDB() Exit--- \r\n");

    if (i < 0)
    {
        DEBUG_TRACE(MODULE_USER, LOG_ERROR, "UserUnRegAbnormal() DeleteUserRegInfoFromDB Error:UserID=%s, UserIndex=%u, login_ip=%s, login_port=%d, i=%d \r\n", pUserInfo->user_id, pUserInfo->user_index, pUserInfo->login_ip, pUserInfo->login_port, i);
    }
    else
    {
        DEBUG_TRACE(MODULE_USER, LOG_TRACE, "UserUnRegAbnormal() DeleteUserRegInfoFromDB OK:UserID=%s, UserIndex=%u, login_ip=%s, login_port=%d, i=%d \r\n", pUserInfo->user_id, pUserInfo->user_index, pUserInfo->login_ip, pUserInfo->login_port, i);
    }

    i = NotifyOnlineUserToAllClientUser(pUserInfo, 2, pdboper);
    printf("UserUnRegAbnormal() NotifyOnlineUserToAllClientUser() Exit--- \r\n");

    if (i < 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "����֪ͨ�����û��仯��Ϣ�������û�ʧ��, �Ƴ��������û�:�û�ID=%s, IP��ַ=%s, �˿ں�=%d\r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Send notification online users change to online user failure, remove online users: user ID = % s, IP address = % s, port = %d\r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
        DEBUG_TRACE(MODULE_USER, LOG_ERROR, "UserUnRegAbnormal() NotifyOnlineUserToAllClientUser Error:UserID=%s, UserIndex=%u, login_ip=%s, login_port=%d, i=%d \r\n", pUserInfo->user_id, pUserInfo->user_index, pUserInfo->login_ip, pUserInfo->login_port, i);
    }
    else if (i > 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "����֪ͨ�����û��仯��Ϣ�������û��ɹ�, �Ƴ��������û�:�û�ID=%s, IP��ַ=%s, �˿ں�=%d\r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Send notification online users change to online user failure, remove online users: user ID = % s, IP address = % s, port = %d\r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
        DEBUG_TRACE(MODULE_USER, LOG_TRACE, "UserUnRegAbnormal() NotifyOnlineUserToAllClientUser OK:UserID=%s, UserIndex=%u, login_ip=%s, login_port=%d, i=%d \r\n", pUserInfo->user_id, pUserInfo->user_index, pUserInfo->login_ip, pUserInfo->login_port, i);
    }

    /* ɾ���ڴ�ڵ�͸������ݿ���Ϣ */
    i = user_info_remove(pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
    printf("UserUnRegAbnormal() user_info_remove() Exit--- \r\n");

    if (0 != i)
    {
        DEBUG_TRACE(MODULE_USER, LOG_ERROR, "UserUnRegAbnormal() user_info_remove Error:UserID=%s, i=%d \r\n", strUserID.c_str(), i);
    }
    else
    {
        DEBUG_TRACE(MODULE_USER, LOG_TRACE, "UserUnRegAbnormal() user_info_remove OK:UserID=%s, i=%d \r\n", strUserID.c_str(), i);
    }

    i = UpdateUserRegInfo2DB2(strUserID, pdboper);   //�������ݿ�״̬
    printf("UserUnRegAbnormal() UpdateUserRegInfo2DB2() Exit--- \r\n");

    if (i < 0)
    {
        DEBUG_TRACE(MODULE_USER, LOG_ERROR, "UserUnRegAbnormal() UpdateUserRegInfo2DB2 Error:UserID=%s, i=%d \r\n", strUserID.c_str(), i);
    }
    else
    {
        DEBUG_TRACE(MODULE_USER, LOG_TRACE, "UserUnRegAbnormal() UpdateUserRegInfo2DB2 OK:UserID=%s, i=%d \r\n", strUserID.c_str(), i);
    }

    /* �ͷŵ�û�õ�TCP���� */
    free_unused_user_tcp_connect();
    printf("UserUnRegAbnormal() free_unused_user_tcp_connect() Exit--- \r\n");

    DEBUG_TRACE(MODULE_USER, LOG_TRACE, "UserUnRegAbnormal() Exit--- \r\n");
    printf("UserUnRegAbnormal() Exit--- \r\n");

    return 0;
}

/*****************************************************************************
 �� �� ��  : UserReg
 ��������  : �û�ע�ᴦ��
 �������  : user_info_t* pUserInfo
             user_cfg_t& user_cfg
             DBOper* pdboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��10��21�� ���ڶ�
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int UserReg(user_info_t* pUserInfo, user_cfg_t& user_cfg, DBOper* pdboper)
{
    int reg_info_index = pUserInfo->reg_info_index;
    int user_tl_pos = 0;
    int i = 0;
    char* realm = NULL;
    int iAuth = 0;
    osip_authorization_t* pAuthorization = NULL;
    char strRegisterDomain[128] = {0};
    char* server_ip = NULL;
    int server_port;
    char* pcRegServerEthName = NULL;
    char strErrorCode[32] = {0};
    int iIsRefreshReg = 0; /* �Ƿ���ˢ��ע�� */
    char* call_id = NULL;

    DEBUG_TRACE(MODULE_USER, LOG_TRACE, "UserReg() Enter---\r\n");
    printf("UserReg() Enter---\r\n");

    /* ����Ƿ�����֤��Ϣ */
    /* get authorization*/
    pAuthorization = SIP_UASGetRegisterAuthorization(reg_info_index);

    if (pAuthorization && pAuthorization->realm)
    {
        DEBUG_TRACE(MODULE_USER, LOG_TRACE, "UserReg() Check Realm: Begin---\r\n");

        realm = osip_getcopy_unquoted_string(pAuthorization->realm);
        i = IsLocalAuthRealm(realm);

        DEBUG_TRACE(MODULE_USER, LOG_TRACE, "UserReg() Check Realm: End--- i=%d\r\n", i);

        if (0 == i)
        {
            memset(strErrorCode, 0, 32);
            snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_REG_AUTH_REALM_NOT_LOCAL_ERROR);
            SIP_UASAnswerToRegister(reg_info_index, 403, strErrorCode);
            //SIP_UASAnswerToRegister(reg_info_index, 403, (char*)"Auth Realm Is Not Local");
            DEBUG_TRACE(MODULE_USER, LOG_ERROR, "UserReg() exit---: Auth Realm Is Not Local:realm=%s \r\n", realm);
            SystemLog(EV9000_CMS_USER_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "�û���¼ʧ��:�û�ID=%s, IP��ַ=%s, �˿ں�=%d, ԭ��=%s%s", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, (char*)"��֤���Ǳ�CMS,��֤��=", realm);
            EnSystemLog(EV9000_CMS_USER_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "User login failed:User ID=%s, User IP=%s, Port=%d, reason=%s%s", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, (char*)"Authentication domain is not the current CMS, authentication domain =", realm);
            osip_free(realm);
            realm = NULL;
            return -1;
        }
        else
        {
            osip_free(realm);
            realm = NULL;

            DEBUG_TRACE(MODULE_USER, LOG_TRACE, "UserReg() SIP Auth: Begin--- :register_account=%s, register_password=%s \r\n", (char*)user_cfg.register_account.c_str(), (char*)user_cfg.register_password.c_str());

            iAuth = SIP_Auth(pAuthorization, (char*)user_cfg.register_account.c_str(), (char*)user_cfg.register_password.c_str(), (char*)"REGISTER");

            DEBUG_TRACE(MODULE_USER, LOG_TRACE, "UserReg() SIP Auth: End--- iAuth=%d\r\n", iAuth);

            if (0 == iAuth) /*��֤ʧ��*/
            {
                pUserInfo->auth_count++;

                if (pUserInfo->auth_count > 0)
                {
                    pUserInfo->auth_count = 0;
                    memset(strErrorCode, 0, 32);
                    snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_REG_AUTH_FAILD_ERROR);
                    SIP_UASAnswerToRegister(reg_info_index, 403, strErrorCode);
                    //SIP_UASAnswerToRegister(reg_info_index, 403, (char*)"AUTH FAIL");
                    DEBUG_TRACE(MODULE_USER, LOG_ERROR, "UserReg() exit---: AUTH FAIL \r\n");
                    SystemLog(EV9000_CMS_USER_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "�û���¼ʧ��:�û�ID=%s, IP��ַ=%s, �˿ں�=%d, ԭ��=%s", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, (char*)"�û���֤ʧ��");
                    EnSystemLog(EV9000_CMS_USER_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "User login failed:User ID=%s, User IP=%s, Port=%d, reason=%s", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, (char*)"User authenticate failed.");
                    return -1;
                }
                else
                {
                    snprintf(strRegisterDomain, 128, "%s.spvmn.cn", pGblconf->register_region);
                    SIP_UASAnswerToRegister4Auth(reg_info_index, strRegisterDomain);
                    DEBUG_TRACE(MODULE_USER, LOG_WARN, "UserReg() exit---: NEED AUTH \r\n");
                    //SystemLog(EV9000_CMS_USER_REG_FAILED, EV9000_LOG_LEVEL_WARNING, "�û���¼ʧ��:�û�ID=%s, IP��ַ=%s, �˿ں�=%d, ԭ��=%s", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, (char*)"�û���Ҫ��֤");
                    //EnSystemLog(EV9000_CMS_USER_REG_FAILED, EV9000_LOG_LEVEL_WARNING, "User login failed:User ID=%s, User IP=%s, Port=%d, reason=%s", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, (char*)"User need to authenticate.");
                    return 0;
                }
            }
            else      //auth success
            {
                pUserInfo->auth_count = 0;

                /* ��ȡע��ķ�����IP��ַ���˿ںţ�ȷ���Ǵ��ĸ���ע��� */
                server_ip = SIP_GetUASServerIP(pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
                server_port = SIP_GetUASServerPort(pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
                pcRegServerEthName = get_ip_eth_name(server_ip);

                DEBUG_TRACE(MODULE_USER, LOG_TRACE, "UserReg(): user_id=%s, login_ip=%s, login_port=%d, server_ip=%s, server_port=%d, RegServerEthName=%s \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, server_ip, server_port, pcRegServerEthName);
                printf("UserReg(): user_id=%s, login_ip=%s, login_port=%d, server_ip=%s, server_port=%d, RegServerEthName=%s \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, server_ip, server_port, pcRegServerEthName);

                if (NULL == server_ip)
                {
                    memset(strErrorCode, 0, 32);
                    snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_REG_GET_SERVER_IP_ERROR);
                    SIP_UASAnswerToRegister(reg_info_index, 403, strErrorCode);
                    //SIP_UASAnswerToRegister(reg_info_index, 503, (char*)"Get Register Server IP Error");
                    DEBUG_TRACE(MODULE_USER, LOG_ERROR, "UserReg() exit---: Get Register Server IP Error \r\n");
                    SystemLog(EV9000_CMS_USER_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "�û���¼ʧ��:�û�ID=%s, IP��ַ=%s, �˿ں�=%d, ԭ��=%s", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, (char*)"��ȡ�û�ע��ķ�����IP��ַʧ��");
                    EnSystemLog(EV9000_CMS_USER_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "User login failed:User ID=%s, User IP=%s, Port=%d, reason=%s", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, (char*)"Get user register server IP address failed.");
                    return -1;
                }

                if (server_port <= 0)
                {
                    memset(strErrorCode, 0, 32);
                    snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_REG_GET_SERVER_PORT_ERROR);
                    SIP_UASAnswerToRegister(reg_info_index, 403, strErrorCode);
                    //SIP_UASAnswerToRegister(reg_info_index, 503, (char*)"Get Register Server Port Error");
                    DEBUG_TRACE(MODULE_USER, LOG_ERROR, "UserReg() exit---: Get Register Server Port Error \r\n");
                    SystemLog(EV9000_CMS_USER_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "�û���¼ʧ��:�û�ID=%s, IP��ַ=%s, �˿ں�=%d, ԭ��=%s", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, (char*)"��ȡ�û�ע��ķ������˿ں�ʧ��");
                    EnSystemLog(EV9000_CMS_USER_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "User login failed:User ID=%s, User IP=%s, Port=%d, reason=%s", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, (char*)"Get user register server port failed.");
                    return -1;
                }

                if (NULL == pcRegServerEthName)
                {
                    memset(strErrorCode, 0, 32);
                    snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_REG_GET_SERVER_IP_ETHNAME_ERROR);
                    SIP_UASAnswerToRegister(reg_info_index, 403, strErrorCode);
                    //SIP_UASAnswerToRegister(reg_info_index, 503, (char*)"Get Register Server EthName Error");
                    DEBUG_TRACE(MODULE_USER, LOG_ERROR, "UserReg() exit---: Get Register Server EthName Error \r\n");
                    SystemLog(EV9000_CMS_USER_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "�û���¼ʧ��:�û�ID=%s, IP��ַ=%s, �˿ں�=%d, ԭ��=%s, ������IP=%s", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, (char*)"��ȡ�û�ע��ķ�������������ʧ��", server_ip);
                    EnSystemLog(EV9000_CMS_USER_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "User login failed:User ID=%s, User IP=%s, Port=%d, reason=%s, server IP=%s", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, (char*)"Get user register server network port name failed.", server_ip);
                    return -1;
                }

                /* ȷ���Ƿ���ˢ��ע�� */
                call_id = SIP_GetUASCallID(pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);

                if (NULL == call_id)
                {
                    memset(strErrorCode, 0, 32);
                    snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_REG_CALLID_ERROR);
                    SIP_UASAnswerToRegister(reg_info_index, 403, strErrorCode);
                    //SIP_UASAnswerToRegister(reg_info_index, 503, (char*)"Get Register CallID Error");
                    DEBUG_TRACE(MODULE_USER, LOG_TRACE, "UserReg() exit---: Get Register CallID Error:server_ip=%s, server_port=%d, RegServerEthName=%s \r\n", server_ip, server_port, pcRegServerEthName);
                    SystemLog(EV9000_CMS_USER_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "�û���¼ʧ��:�û�ID=%s, IP��ַ=%s, �˿ں�=%d, ԭ��=%s", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, (char*)"��ȡ�豸ע���CallID�ֶ���Ϣʧ��");
                    EnSystemLog(EV9000_CMS_USER_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "User login failed:User ID=%s, IP address=%s, port number=%d, cause=%s", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, (char*)"Access information of call id for device registration failed");

                    return -1;
                }

                if (pUserInfo->strCallID[0] == '\0')
                {
                    osip_strncpy(pUserInfo->strCallID, call_id, MAX_128CHAR_STRING_LEN);
                    iIsRefreshReg = 0;
                }
                else
                {
                    if (0 != sstrcmp(pUserInfo->strCallID, call_id))
                    {
                        memset(pUserInfo->strCallID, 0, MAX_128CHAR_STRING_LEN + 4);
                        osip_strncpy(pUserInfo->strCallID, call_id, MAX_128CHAR_STRING_LEN);
                        iIsRefreshReg = 0;
                    }
                    else
                    {
                        iIsRefreshReg = 1; /* ˢ��ע�� */
                    }
                }

                DEBUG_TRACE(MODULE_USER, LOG_INFO, "UserReg() user_id=%s, login_ip=%s, login_port=%d: old callid=%s, new callid=%s, IsRefreshReg=%d \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, pUserInfo->strCallID, call_id, iIsRefreshReg);
                printf("UserReg() user_id=%s, login_ip=%s, login_port=%d: old callid=%s, new callid=%s, IsRefreshReg=%d \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, pUserInfo->strCallID, call_id, iIsRefreshReg);

                /* ȷ��ע�������IP��ַ */
                if (pUserInfo->strRegServerIP[0] == '\0')
                {
                    osip_strncpy(pUserInfo->strRegServerIP, server_ip, MAX_IP_LEN);
                }
                else
                {
                    if (0 != sstrcmp(pUserInfo->strRegServerIP, server_ip))
                    {
                        memset(pUserInfo->strRegServerIP, 0, MAX_IP_LEN);
                        osip_strncpy(pUserInfo->strRegServerIP, server_ip, MAX_IP_LEN);
                    }
                }

                /* ȷ��ע��������˿ں� */
                if (pUserInfo->iRegServerPort <= 0)
                {
                    pUserInfo->iRegServerPort = server_port;
                }
                else
                {
                    if (pUserInfo->iRegServerPort != server_port)
                    {
                        pUserInfo->iRegServerPort = server_port;
                    }
                }

                /* ȷ��IP��ַ�������� */
                if (pUserInfo->strRegServerEthName[0] == '\0')
                {
                    osip_strncpy(pUserInfo->strRegServerEthName, pcRegServerEthName, MAX_IP_LEN);
                }
                else
                {
                    if (0 != sstrcmp(pUserInfo->strRegServerEthName, pcRegServerEthName))
                    {
                        memset(pUserInfo->strRegServerEthName, 0, MAX_IP_LEN);
                        osip_strncpy(pUserInfo->strRegServerEthName, pcRegServerEthName, MAX_IP_LEN);
                    }
                }

                if (pUserInfo->reg_status <= 0)
                {
                    /* �����û�ҵ�����߳� */
                    user_tl_pos = user_srv_proc_thread_find(pUserInfo);
                    DEBUG_TRACE(MODULE_USER, LOG_TRACE, "UserReg() user_srv_proc_thread_find:user_tl_pos=%d \r\n", user_tl_pos);
                    printf("UserReg() user_srv_proc_thread_find:user_tl_pos=%d \r\n", user_tl_pos);

                    if (user_tl_pos < 0)
                    {
                        //���䴦���߳�
                        i = user_srv_proc_thread_assign(pUserInfo);
                        DEBUG_TRACE(MODULE_USER, LOG_TRACE, "UserReg() user_srv_proc_thread_assign:i=%d \r\n", i);
                        printf("UserReg() user_srv_proc_thread_assign:i=%d \r\n", i);

                        if (i != 0)
                        {
                            memset(strErrorCode, 0, 32);
                            snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_USER_REG_ASSIGN_THREAD_ERROR);
                            SIP_UASAnswerToRegister(reg_info_index, 403, strErrorCode);
                            //SIP_UASAnswerToRegister(reg_info_index, 503, (char*)"User Srv Thread Start Error");
                            DEBUG_TRACE(MODULE_USER, LOG_ERROR, "UserReg() exit---: User Srv Thread Start Error \r\n");
                            SystemLog(EV9000_CMS_USER_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "�û���¼ʧ��:�û�ID=%s, IP��ַ=%s, �˿ں�=%d, ԭ��=%s", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, (char*)"�����û�ҵ�����߳�ʧ��");
                            EnSystemLog(EV9000_CMS_USER_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "User login failed:User ID=%s, User IP=%s, Port=%d, reason=%s", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, (char*)"Assign user business processing threads fail.");
                            return -1;
                        }
                    }
                    else
                    {
                        /* �Ƴ��û������ĵ�λ��Ϣ */
                        i = RemoveGBLogicDeviceLockInfoByUserInfo(pUserInfo);

                        if (0 != i)
                        {
                            DEBUG_TRACE(MODULE_USER, LOG_ERROR, "UserReg() RemoveGBLogicDeviceLockInfoByUserInfo Error:user_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, i);
                        }
                        else
                        {
                            DEBUG_TRACE(MODULE_USER, LOG_TRACE, "UserReg() RemoveGBLogicDeviceLockInfoByUserInfo OK:user_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, i);
                        }

                        /* �����û���ҵ��ֹͣ����ҵ�� */
                        i = StopAllServiceTaskByUserInfo(pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);

                        if (0 != i)
                        {
                            DEBUG_TRACE(MODULE_USER, LOG_ERROR, "UserReg() StopAllServiceTaskByUserInfo Error:user_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, i);
                        }
                        else
                        {
                            DEBUG_TRACE(MODULE_USER, LOG_TRACE, "UserReg() StopAllServiceTaskByUserInfo OK:user_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, i);
                        }

                        /* �ͷ�һ��֮ǰ���û�ҵ�����߳� */
                        i = user_srv_proc_thread_recycle(pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);

                        if (0 != i)
                        {
                            DEBUG_TRACE(MODULE_USER, LOG_ERROR, "UserReg() user_srv_proc_thread_recycle Error:user_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, i);
                        }
                        else
                        {
                            DEBUG_TRACE(MODULE_USER, LOG_TRACE, "UserReg() user_srv_proc_thread_recycle OK:user_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, i);
                        }

                        //���䴦���߳�
                        i = user_srv_proc_thread_assign(pUserInfo);
                        DEBUG_TRACE(MODULE_USER, LOG_TRACE, "UserReg() user_srv_proc_thread_assign:i=%d \r\n", i);

                        if (i != 0)
                        {
                            memset(strErrorCode, 0, 32);
                            snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_USER_REG_ASSIGN_THREAD_ERROR);
                            SIP_UASAnswerToRegister(reg_info_index, 403, strErrorCode);
                            //SIP_UASAnswerToRegister(reg_info_index, 503, (char*)"User Srv Thread Start Error");
                            DEBUG_TRACE(MODULE_USER, LOG_ERROR, "UserReg() exit---: User Srv Thread Start Error \r\n");
                            SystemLog(EV9000_CMS_USER_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "�û���¼ʧ��:�û�ID=%s, IP��ַ=%s, �˿ں�=%d, ԭ��=%s", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, (char*)"�����û�ҵ�����߳�ʧ��");
                            EnSystemLog(EV9000_CMS_USER_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "User login failed:User ID=%s, User IP=%s, Port=%d, reason=%s", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, (char*)"Assign user business processing threads fail.");
                            return -1;
                        }
                    }

                    pUserInfo->reg_status = 1;

                    i = UpdateUserRegInfo2DB(pUserInfo->user_id, pUserInfo->reg_status, pdboper);

                    if (0 != i)
                    {
                        DEBUG_TRACE(MODULE_USER, LOG_ERROR, "UserReg() UpdateUserRegInfo2DB Error:user_id=%s, pUserInfo->reg_status=%d, i=%d \r\n", pUserInfo->user_id, pUserInfo->reg_status, i);
                    }
                    else
                    {
                        DEBUG_TRACE(MODULE_USER, LOG_TRACE, "UserReg() UpdateUserRegInfo2DB OK:user_id=%s, pUserInfo->reg_status=%d, i=%d \r\n", pUserInfo->user_id, pUserInfo->reg_status, i);
                    }

                    /* ��ӵ������û����ݱ� */
                    i = AddUserRegInfo2DB(pUserInfo->user_index, pUserInfo->login_ip, pUserInfo->login_port, pdboper);

                    if (i < 0)
                    {
                        DEBUG_TRACE(MODULE_USER, LOG_ERROR, "UserReg() AddUserRegInfo2DB Error:UserID=%s, UserIndex=%u, login_ip=%s, login_port=%d, i=%d \r\n", pUserInfo->user_id, pUserInfo->user_index, pUserInfo->login_ip, pUserInfo->login_port, i);
                    }
                    else
                    {
                        DEBUG_TRACE(MODULE_USER, LOG_TRACE, "UserReg() AddUserRegInfo2DB OK:UserID=%s, UserIndex=%u, login_ip=%s, login_port=%d, i=%d \r\n", pUserInfo->user_id, pUserInfo->user_index, pUserInfo->login_ip, pUserInfo->login_port, i);
                    }

                    i = NotifyOnlineUserToAllClientUser(pUserInfo, 1, pdboper);

                    if (i < 0)
                    {
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "����֪ͨ�����û��仯��Ϣ�������û�ʧ��, ���ӵ������û�:�û�ID=%s, IP��ַ=%s, �˿ں�=%d\r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
                        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Send notification online users change to online users failed, increase the online user: user ID = % s, IP address = % s, port = %d\r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
                        DEBUG_TRACE(MODULE_USER, LOG_ERROR, "UserReg() NotifyOnlineUserToAllClientUser Error:UserID=%s, UserIndex=%u, login_ip=%s, login_port=%d, i=%d \r\n", pUserInfo->user_id, pUserInfo->user_index, pUserInfo->login_ip, pUserInfo->login_port, i);
                    }
                    else if (i > 0)
                    {
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "����֪ͨ�����û��仯��Ϣ�������û��ɹ�, ���ӵ������û�:�û�ID=%s, IP��ַ=%s, �˿ں�=%d\r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
                        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Send notification online users change to online users successfully, increase the online user: user ID = % s, IP address = % s, port = %d\r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
                        DEBUG_TRACE(MODULE_USER, LOG_TRACE, "UserReg() NotifyOnlineUserToAllClientUser OK:UserID=%s, UserIndex=%u, login_ip=%s, login_port=%d, i=%d \r\n", pUserInfo->user_id, pUserInfo->user_index, pUserInfo->login_ip, pUserInfo->login_port, i);
                    }

                    UserLog(EV9000_USER_LOG_LOGIN, EV9000_LOG_LEVEL_NORMAL, pUserInfo, "�û���¼�ɹ�");
                    EnUserLog(EV9000_USER_LOG_LOGIN, EV9000_LOG_LEVEL_NORMAL, pUserInfo, "User login successfully.");
                    DEBUG_TRACE(MODULE_USER, LOG_INFO, "UserReg() ADD:user_id=%s, login_ip=%s, login_port=%d, reg_info_index=%d \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, pUserInfo->reg_info_index);
                    printf("UserReg() ADD:user_id=%s, login_ip=%s, login_port=%d, reg_info_index=%d \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, pUserInfo->reg_info_index);

                    //i = shdb_user_operate_cmd_proc(pUserInfo, EV9000_SHDB_DVR_SYSTEM_START);
                }
                else
                {
                    if (iIsRefreshReg)
                    {
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�û�ע��ˢ��:�û�ID=%s, IP��ַ=%s, �˿ں�=%d", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
                        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "User registration refresh: user ID = % s, = % s IP address, port number = % d", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
                        //DebugRunTrace("�û�ע��ˢ��:�û�ID=%s, IP��ַ=%s, �˿ں�=%d\r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
                        DEBUG_TRACE(MODULE_USER, LOG_TRACE, "UserReg() REFRESH:user_id=%s, login_ip=%s, login_port=%d, reg_info_index=%d \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, pUserInfo->reg_info_index);
                        printf("UserReg() REFRESH:user_id=%s, login_ip=%s, login_port=%d, reg_info_index=%d \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, pUserInfo->reg_info_index);
                    }
                    else
                    {
                        user_tl_pos = user_srv_proc_thread_find(pUserInfo);
                        DEBUG_TRACE(MODULE_USER, LOG_TRACE, "UserReg() user_srv_proc_thread_find:user_tl_pos=%d \r\n", user_tl_pos);
                        printf("UserReg() user_srv_proc_thread_find:user_tl_pos=%d \r\n", user_tl_pos);

                        if (user_tl_pos < 0)
                        {
                            //���䴦���߳�
                            i = user_srv_proc_thread_assign(pUserInfo);
                            DEBUG_TRACE(MODULE_USER, LOG_TRACE, "UserReg() user_srv_proc_thread_assign:i=%d \r\n", i);
                            printf("UserReg() user_srv_proc_thread_assign:i=%d \r\n", i);

                            if (i != 0)
                            {
                                memset(strErrorCode, 0, 32);
                                snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_USER_REG_ASSIGN_THREAD_ERROR);
                                SIP_UASAnswerToRegister(reg_info_index, 403, strErrorCode);
                                //SIP_UASAnswerToRegister(reg_info_index, 503, (char*)"User Srv Thread Start Error");
                                DEBUG_TRACE(MODULE_USER, LOG_ERROR, "UserReg() exit---: User Srv Thread Start Error \r\n");
                                SystemLog(EV9000_CMS_USER_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "�û���¼ʧ��:�û�ID=%s, IP��ַ=%s, �˿ں�=%d, ԭ��=%s", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, (char*)"�����û�ҵ�����߳�ʧ��");
                                EnSystemLog(EV9000_CMS_USER_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "User login failed:User ID=%s, User IP=%s, Port=%d, reason=%s", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, (char*)"Assign user business processing threads fail.");
                                return -1;
                            }
                        }
                        else
                        {
                            /* �Ƴ��û������ĵ�λ��Ϣ */
                            i = RemoveGBLogicDeviceLockInfoByUserInfo(pUserInfo);

                            if (0 != i)
                            {
                                DEBUG_TRACE(MODULE_USER, LOG_ERROR, "UserReg() RemoveGBLogicDeviceLockInfoByUserInfo Error:user_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, i);
                            }
                            else
                            {
                                DEBUG_TRACE(MODULE_USER, LOG_TRACE, "UserReg() RemoveGBLogicDeviceLockInfoByUserInfo OK:user_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, i);
                            }

                            /* �����û���ҵ��ֹͣ����ҵ�� */
                            i = StopAllServiceTaskByUserInfo(pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);

                            if (0 != i)
                            {
                                DEBUG_TRACE(MODULE_USER, LOG_ERROR, "UserReg() StopAllServiceTaskByUserInfo Error:user_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, i);
                            }
                            else
                            {
                                DEBUG_TRACE(MODULE_USER, LOG_TRACE, "UserReg() StopAllServiceTaskByUserInfo OK:user_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, i);
                            }

                            /* �ͷ�һ��֮ǰ���û�ҵ�����߳� */
                            i = user_srv_proc_thread_recycle(pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);

                            if (0 != i)
                            {
                                DEBUG_TRACE(MODULE_USER, LOG_ERROR, "UserReg() user_srv_proc_thread_recycle Error:user_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, i);
                            }
                            else
                            {
                                DEBUG_TRACE(MODULE_USER, LOG_TRACE, "UserReg() user_srv_proc_thread_recycle OK:user_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, i);
                            }

                            //���䴦���߳�
                            i = user_srv_proc_thread_assign(pUserInfo);
                            DEBUG_TRACE(MODULE_USER, LOG_TRACE, "UserReg() user_srv_proc_thread_assign:i=%d \r\n", i);
                            printf("UserReg() user_srv_proc_thread_assign:i=%d \r\n", i);

                            if (i != 0)
                            {
                                memset(strErrorCode, 0, 32);
                                snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_USER_REG_ASSIGN_THREAD_ERROR);
                                SIP_UASAnswerToRegister(reg_info_index, 403, strErrorCode);
                                //SIP_UASAnswerToRegister(reg_info_index, 503, (char*)"User Srv Thread Start Error");
                                DEBUG_TRACE(MODULE_USER, LOG_ERROR, "UserReg() exit---: User Srv Thread Start Error \r\n");
                                SystemLog(EV9000_CMS_USER_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "�û���¼ʧ��:�û�ID=%s, IP��ַ=%s, �˿ں�=%d, ԭ��=%s", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, (char*)"�����û�ҵ�����߳�ʧ��");
                                EnSystemLog(EV9000_CMS_USER_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "User login failed:User ID=%s, User IP=%s, Port=%d, reason=%s", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, (char*)"Assign user business processing threads fail.");
                                return -1;
                            }
                        }

                        UserLog(EV9000_USER_LOG_LOGIN, EV9000_LOG_LEVEL_WARNING, pUserInfo, "�û��쳣���ߺ����µ�¼�ɹ�");
                        EnUserLog(EV9000_USER_LOG_LOGIN, EV9000_LOG_LEVEL_WARNING, pUserInfo, "User re login successfully");
                    }
                }

                SIP_UASAnswerToRegister(reg_info_index, 200, NULL);

                printf("UserReg() ADD AGAIN:user_id=%s, login_ip=%s, login_port=%d, reg_info_index=%d \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, pUserInfo->reg_info_index);
                return 0;
            }
        }
    }
    else
    {
        pUserInfo->auth_count++;

        if (pUserInfo->auth_count > 3)
        {
            pUserInfo->auth_count = 0;
            memset(strErrorCode, 0, 32);
            snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_REG_AUTH_FAILD_ERROR);
            SIP_UASAnswerToRegister(reg_info_index, 403, strErrorCode);
            //SIP_UASAnswerToRegister(reg_info_index, 403, NULL);
            DEBUG_TRACE(MODULE_USER, LOG_ERROR, "UserReg() exit---: AUTH FAIL \r\n");
            SystemLog(EV9000_CMS_USER_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "�û���¼ʧ��:�û�ID=%s, IP��ַ=%s, �˿ں�=%d, ԭ��=%s", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, (char*)"�û���֤ʧ��");
            EnSystemLog(EV9000_CMS_USER_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "User login failed:User ID=%s, User IP=%s, Port=%d, reason=%s", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, (char*)"User authenticate failed");
            return -1;
        }
        else
        {
            snprintf(strRegisterDomain, 128, "%s.spvmn.cn", pGblconf->register_region);
            SIP_UASAnswerToRegister4Auth(reg_info_index, strRegisterDomain);
            DEBUG_TRACE(MODULE_USER, LOG_WARN, "UserReg() exit---: NEED AUTH \r\n");
            //SystemLog(EV9000_CMS_USER_REG_FAILED, EV9000_LOG_LEVEL_WARNING, "�û���¼ʧ��:�û�ID=%s, IP��ַ=%s, �˿ں�=%d, ԭ��=%s", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, (char*)"�û���Ҫ��֤");
            //EnSystemLog(EV9000_CMS_USER_REG_FAILED, EV9000_LOG_LEVEL_WARNING, "User login failed:User ID=%s, User IP=%s, Port=%d, reason=%s", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, (char*)"User need to authenticate.");
            return 0;
        }
    }

    memset(strErrorCode, 0, 32);
    snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_SYSTEM_ERROR);
    SIP_UASAnswerToRegister(reg_info_index, 500, strErrorCode);
    //SIP_UASAnswerToRegister(reg_info_index, 500, (char*)"Server Error");
    DEBUG_TRACE(MODULE_USER, LOG_ERROR, "UserReg() exit---: Forbidden \r\n");
    SystemLog(EV9000_CMS_USER_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "�û���¼ʧ��:�û�ID=%s, IP��ַ=%s, �˿ں�=%d, ԭ��=%s", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, (char*)"δ֪ԭ��");
    EnSystemLog(EV9000_CMS_USER_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "User login failed:User ID=%s, User IP=%s, Port=%d, reason=%s", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, (char*)"Unknown reason.");
    return -1;
}

/*****************************************************************************
 �� �� ��  : user_reg_msg_proc
 ��������  : �û�ע����Ϣ����
 �������  : user_reg_msg_t* pMsg
             DBOper* pdboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��10��21�� ���ڶ�
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int user_reg_msg_proc(user_reg_msg_t* pMsg, DBOper* pdboper)
{
    int i = 0;
    int user_pos = -1;
    string strUserID = "";
    int reg_info_index = 0;
    user_info_t* pUserInfo = NULL;
    int iExpires = pMsg->expires;
    user_cfg_t user_cfg;
    char strErrorCode[32] = {0};

    DEBUG_TRACE(MODULE_USER, LOG_TRACE, "user_reg_msg_proc() Enter---: register_id=%s,login_ip=%s,login_port=%d \r\n", pMsg->register_id, pMsg->login_ip, pMsg->login_port);
    printf("\r\nuser_reg_msg_proc() Enter---: register_id=%s,login_ip=%s,login_port=%d \r\n", pMsg->register_id, pMsg->login_ip, pMsg->login_port);

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�û���¼����:�û�ID=%s, IP��ַ=%s, �˿ں�=%d", pMsg->register_id, pMsg->login_ip, pMsg->login_port);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "User login: user ID = % s, = % s IP address, port number = %d", pMsg->register_id, pMsg->login_ip, pMsg->login_port);

    /*��֤����*/
    if (!pMsg || pMsg->reg_info_index < 0)
    {
        DEBUG_TRACE(MODULE_USER, LOG_ERROR, "user_reg_msg_proc() exit---: msg /reg_info_index error \r\n");
        return -1;
    }

    reg_info_index = pMsg->reg_info_index;

    if ('\0' == pMsg->register_id[0] || '\0' == pMsg->login_ip[0] || pMsg->login_port <= 0)
    {
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_REG_MSG_ERROR);
        SIP_UASAnswerToRegister(reg_info_index, 403, strErrorCode);
        //SIP_UASAnswerToRegister(reg_info_index, 503, (char*)"msg /User ID/login_ip login_port error");
        DEBUG_TRACE(MODULE_USER, LOG_ERROR, "user_reg_msg_proc() exit---: msg /User ID/login_ip login_port error \r\n");
        SystemLog(EV9000_CMS_USER_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "�û���¼ʧ��:�û�ID=%s, IP��ַ=%s, �˿ں�=%d, ԭ��=%s", pMsg->register_id, pMsg->login_ip, pMsg->login_port, (char*)"�û�ID/�û�IP��/�û��˿ںŲ��Ϸ�");
        EnSystemLog(EV9000_CMS_USER_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "User login failed:User ID=%s, User IP=%s, Port=%d, reason=%s", pMsg->register_id, pMsg->login_ip, pMsg->login_port, (char*)"User ID / user IP / user port number is not legitimate.");
        return -1;
    }

    printf("user_reg_msg_proc() GetUserCfg Enter---: register_id=%s,login_ip=%s,login_port=%d \r\n", pMsg->register_id, pMsg->login_ip, pMsg->login_port);

    /* �����û�*/
    if (GetUserCfg(pdboper, pMsg->register_id, user_cfg) <= 0) //�Ƿ��û����ѯ��Ϣʧ��
    {
        //i = RegisterSetNotFoundGBDevice2GBPhyDeviceTmpDB(pMsg->register_id, pMsg->register_name, pMsg->login_ip);

        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_USER_GET_USER_INFO_ERROR);
        SIP_UASAnswerToRegister(reg_info_index, 403, strErrorCode);
        //SIP_UASAnswerToRegister(reg_info_index, 404, (char*)"Find User Info Error");

        /* �Ƴ��û������ĵ�λ��Ϣ */
        i = RemoveGBLogicDeviceLockInfoByUserInfo(pUserInfo);

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_USER, LOG_ERROR, "user_reg_msg_proc() RemoveGBLogicDeviceLockInfoByUserInfo Error:user_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_USER, LOG_TRACE, "user_reg_msg_proc() RemoveGBLogicDeviceLockInfoByUserInfo OK:user_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, i);
        }

        /* �Ƴ��û�ҵ�� */
        i = StopAllServiceTaskByUserInfo(pMsg->register_id, pMsg->login_ip, pMsg->login_port);

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_USER, LOG_ERROR, "user_reg_msg_proc() StopAllServiceTaskByUserInfo Error:user_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pMsg->register_id, pMsg->login_ip, pMsg->login_port, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_USER, LOG_TRACE, "user_reg_msg_proc() StopAllServiceTaskByUserInfo OK:user_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pMsg->register_id, pMsg->login_ip, pMsg->login_port, i);
        }

        /* �����û�ҵ���߳� */
        i = user_srv_proc_thread_recycle(pMsg->register_id, pMsg->login_ip, pMsg->login_port);

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_USER, LOG_ERROR, "user_reg_msg_proc() user_srv_proc_thread_recycle Error:user_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pMsg->register_id, pMsg->login_ip, pMsg->login_port, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_USER, LOG_TRACE, "user_reg_msg_proc() user_srv_proc_thread_recycle OK:user_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pMsg->register_id, pMsg->login_ip, pMsg->login_port, i);
        }

        /* �����ݿ���ɾ�������û���Ϣ */
        user_pos = user_info_find(pMsg->register_id, pMsg->login_ip, pMsg->login_port);

        if (user_pos >= 0)
        {
            pUserInfo = user_info_get(user_pos);

            if (NULL != pUserInfo)
            {
                i = DeleteUserRegInfoFromDB(pUserInfo->user_index, pMsg->login_ip, pMsg->login_port, pdboper);

                if (i < 0)
                {
                    DEBUG_TRACE(MODULE_USER, LOG_ERROR, "user_reg_msg_proc() DeleteUserRegInfoFromDB Error:UserID=%s, UserIndex=%u, login_ip=%s, login_port=%d, i=%d \r\n", pMsg->register_id, pUserInfo->user_index, pMsg->login_ip, pMsg->login_port, i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_USER, LOG_TRACE, "user_reg_msg_proc() DeleteUserRegInfoFromDB OK:UserID=%s, UserIndex=%u, login_ip=%s, login_port=%d, i=%d \r\n", pMsg->register_id, pUserInfo->user_index, pMsg->login_ip, pMsg->login_port, i);
                }

                i = NotifyOnlineUserToAllClientUser(pUserInfo, 2, pdboper);

                if (i < 0)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "����֪ͨ�����û��仯��Ϣ�������û�ʧ��, �Ƴ��������û�:�û�ID=%s, IP��ַ=%s, �˿ں�=%d\r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Users send notification that online users change  to  online user failed, remove online users: user ID = % s, IP address = % s, port = % d\r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
                    DEBUG_TRACE(MODULE_USER, LOG_ERROR, "user_reg_msg_proc() NotifyOnlineUserToAllClientUser Error:UserID=%s, UserIndex=%u, login_ip=%s, login_port=%d, i=%d \r\n", pUserInfo->user_id, pUserInfo->user_index, pUserInfo->login_ip, pUserInfo->login_port, i);
                }
                else if (i > 0)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "����֪ͨ�����û��仯��Ϣ�������û��ɹ�, �Ƴ��������û�:�û�ID=%s, IP��ַ=%s, �˿ں�=%d\r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Users send notification that online users change  to  online user successfully, remove online users: user ID = % s, IP address = % s, port = % d\r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
                    DEBUG_TRACE(MODULE_USER, LOG_TRACE, "user_reg_msg_proc() NotifyOnlineUserToAllClientUser OK:UserID=%s, UserIndex=%u, login_ip=%s, login_port=%d, i=%d \r\n", pUserInfo->user_id, pUserInfo->user_index, pUserInfo->login_ip, pUserInfo->login_port, i);
                }
            }

            i = user_info_remove(pMsg->register_id, pMsg->login_ip, pMsg->login_port);

            if (0 != i)
            {
                DEBUG_TRACE(MODULE_USER, LOG_ERROR, "user_reg_msg_proc() user_info_remove Error:UserID=%s, login_ip=%s, login_port=%d, i=%d \r\n", pMsg->register_id, pMsg->login_ip, pMsg->login_port, i);
            }
            else
            {
                DEBUG_TRACE(MODULE_USER, LOG_TRACE, "user_reg_msg_proc() user_info_remove OK:UserID=%s, login_ip=%s, login_port=%d, i=%d \r\n", pMsg->register_id, pMsg->login_ip, pMsg->login_port, i);
            }

            /* �ͷŵ�û�õ�TCP���� */
            free_unused_user_tcp_connect();
        }

        DEBUG_TRACE(MODULE_USER, LOG_ERROR, "user_reg_msg_proc() exit---: Find User Info Error \r\n");
        SystemLog(EV9000_CMS_USER_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "�û���¼ʧ��:�û�ID=%s, IP��ַ=%s, �˿ں�=%d, ԭ��=%s", pMsg->register_id, pMsg->login_ip, pMsg->login_port, (char*)"���ݿ���û���ҵ����û�");
        EnSystemLog(EV9000_CMS_USER_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "User login failed:User ID=%s, User IP=%s, Port=%d, reason=%s", pMsg->register_id, pMsg->login_ip, pMsg->login_port, (char*)"The user was not found in the database.");
        return -1;
    }

    printf("user_reg_msg_proc() GetUserCfg Exit---: register_id=%s,login_ip=%s,login_port=%d \r\n", pMsg->register_id, pMsg->login_ip, pMsg->login_port);

    /* �û��Ƿ����� */
    if (0 == user_cfg.enable)
    {
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_USER_NOT_ENABLE_ERROR);
        SIP_UASAnswerToRegister(reg_info_index, 403, strErrorCode);
        //SIP_UASAnswerToRegister(reg_info_index, 403, (char*)"User Not Enable");

        /* �Ƴ��û������ĵ�λ��Ϣ */
        i = RemoveGBLogicDeviceLockInfoByUserInfo(pUserInfo);

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_USER, LOG_ERROR, "user_reg_msg_proc() RemoveGBLogicDeviceLockInfoByUserInfo Error:user_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_USER, LOG_TRACE, "user_reg_msg_proc() RemoveGBLogicDeviceLockInfoByUserInfo OK:user_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, i);
        }

        /* �Ƴ��û�ҵ�� */
        i = StopAllServiceTaskByUserInfo(pMsg->register_id, pMsg->login_ip, pMsg->login_port);

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_USER, LOG_ERROR, "user_reg_msg_proc() StopAllServiceTaskByUserInfo Error:user_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pMsg->register_id, pMsg->login_ip, pMsg->login_port, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_USER, LOG_TRACE, "user_reg_msg_proc() StopAllServiceTaskByUserInfo OK:user_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pMsg->register_id, pMsg->login_ip, pMsg->login_port, i);
        }

        /* �����û�ҵ���߳� */
        i = user_srv_proc_thread_recycle(pMsg->register_id, pMsg->login_ip, pMsg->login_port);

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_USER, LOG_ERROR, "user_reg_msg_proc() user_srv_proc_thread_recycle Error:user_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pMsg->register_id, pMsg->login_ip, pMsg->login_port, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_USER, LOG_TRACE, "user_reg_msg_proc() user_srv_proc_thread_recycle OK:user_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pMsg->register_id, pMsg->login_ip, pMsg->login_port, i);
        }

        /* �����ݿ���ɾ�������û���Ϣ */
        user_pos = user_info_find(pMsg->register_id, pMsg->login_ip, pMsg->login_port);

        if (user_pos >= 0)
        {
            pUserInfo = user_info_get(user_pos);

            if (NULL != pUserInfo)
            {
                i = DeleteUserRegInfoFromDB(pUserInfo->user_index, pMsg->login_ip, pMsg->login_port, pdboper);

                if (i < 0)
                {
                    DEBUG_TRACE(MODULE_USER, LOG_ERROR, "user_reg_msg_proc() DeleteUserRegInfoFromDB Error:UserID=%s, UserIndex=%u, login_ip=%s, login_port=%d, i=%d \r\n", pMsg->register_id, pUserInfo->user_index, pMsg->login_ip, pMsg->login_port, i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_USER, LOG_TRACE, "user_reg_msg_proc() DeleteUserRegInfoFromDB OK:UserID=%s, UserIndex=%u, login_ip=%s, login_port=%d, i=%d \r\n", pMsg->register_id, pUserInfo->user_index, pMsg->login_ip, pMsg->login_port, i);
                }

                i = NotifyOnlineUserToAllClientUser(pUserInfo, 2, pdboper);

                if (i < 0)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "����֪ͨ�����û��仯��Ϣ�������û�ʧ��, �Ƴ��������û�:�û�ID=%s, IP��ַ=%s, �˿ں�=%d\r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Users send notification that online users change to online user failed, remove online users: user ID = % s, IP address = % s, port = % d\r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
                    DEBUG_TRACE(MODULE_USER, LOG_ERROR, "user_reg_msg_proc() NotifyOnlineUserToAllClientUser Error:UserID=%s, UserIndex=%u, login_ip=%s, login_port=%d, i=%d \r\n", pUserInfo->user_id, pUserInfo->user_index, pUserInfo->login_ip, pUserInfo->login_port, i);
                }
                else if (i > 0)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "����֪ͨ�����û��仯��Ϣ�������û��ɹ�, �Ƴ��������û�:�û�ID=%s, IP��ַ=%s, �˿ں�=%d\r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Users send notification that online users change to online user successfully, remove online users: user ID = % s, IP address = % s, port = % d\r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
                    DEBUG_TRACE(MODULE_USER, LOG_TRACE, "user_reg_msg_proc() NotifyOnlineUserToAllClientUser OK:UserID=%s, UserIndex=%u, login_ip=%s, login_port=%d, i=%d \r\n", pUserInfo->user_id, pUserInfo->user_index, pUserInfo->login_ip, pUserInfo->login_port, i);
                }
            }

            i = user_info_remove(pMsg->register_id, pMsg->login_ip, pMsg->login_port);

            if (0 != i)
            {
                DEBUG_TRACE(MODULE_USER, LOG_ERROR, "user_reg_msg_proc() user_info_remove Error:UserID=%s, login_ip=%s, login_port=%d, i=%d \r\n", pMsg->register_id, pMsg->login_ip, pMsg->login_port, i);
            }
            else
            {
                DEBUG_TRACE(MODULE_USER, LOG_TRACE, "user_reg_msg_proc() user_info_remove OK:UserID=%s, login_ip=%s, login_port=%d, i=%d \r\n", pMsg->register_id, pMsg->login_ip, pMsg->login_port, i);
            }

            /* �ͷŵ�û�õ�TCP���� */
            free_unused_user_tcp_connect();
        }

        DEBUG_TRACE(MODULE_USER, LOG_ERROR, "user_reg_msg_proc() exit---: User Not Enable \r\n");
        SystemLog(EV9000_CMS_USER_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "�û���¼ʧ��:�û�ID=%s, IP��ַ=%s, �˿ں�=%d, ԭ��=%s", pMsg->register_id, pMsg->login_ip, pMsg->login_port, (char*)"�û��Ѿ�������");
        EnSystemLog(EV9000_CMS_USER_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "User login failed:User ID=%s, User IP=%s, Port=%d, reason=%s", pMsg->register_id, pMsg->login_ip, pMsg->login_port, (char*)"User has been disabled.");
        return -1;
    }

    user_pos = user_info_find(pMsg->register_id, pMsg->login_ip, pMsg->login_port);

    DEBUG_TRACE(MODULE_USER, LOG_TRACE, "user_reg_msg_proc() user_info_find:register_id=%s,login_ip=%s,login_port=%d,user_pos=%d \r\n", pMsg->register_id, pMsg->login_ip, pMsg->login_port, user_pos);
    printf("user_reg_msg_proc() user_info_find:register_id=%s,login_ip=%s,login_port=%d,user_pos=%d \r\n", pMsg->register_id, pMsg->login_ip, pMsg->login_port, user_pos);

    if (user_pos < 0)   //�µ�¼�û������ڴ��¼
    {
        user_info_t* pNewUserInfo = new user_info_t();

        if (pNewUserInfo == NULL)
        {
            memset(strErrorCode, 0, 32);
            snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_USER_FIND_USER_INFO_ERROR);
            SIP_UASAnswerToRegister(reg_info_index, 403, strErrorCode);
            //SIP_UASAnswerToRegister(reg_info_index, 503, (char*)"Create User Error Error");
            DEBUG_TRACE(MODULE_USER, LOG_ERROR, "user_reg_msg_proc() new user_info_t() fail  \r\n");
            SystemLog(EV9000_CMS_USER_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "�û���¼ʧ��:�û�ID=%s, IP��ַ=%s, �˿ں�=%d, ԭ��=%s", pMsg->register_id, pMsg->login_ip, pMsg->login_port, (char*)"����û���Ϣʧ��");
            EnSystemLog(EV9000_CMS_USER_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "User login failed:User ID=%s, User IP=%s, Port=%d, reason=%s", pMsg->register_id, pMsg->login_ip, pMsg->login_port, (char*)"Add user information failed.");
            return -1;
        }

        pNewUserInfo->user_index = user_cfg.id;
        pNewUserInfo->user_level = user_cfg.user_level;
        osip_strncpy(pNewUserInfo->user_id, pMsg->register_id, MAX_ID_LEN);
        osip_strncpy(pNewUserInfo->user_name, (char*)user_cfg.user_name.c_str(), MAX_32CHAR_STRING_LEN);
        osip_strncpy(pNewUserInfo->login_ip, pMsg->login_ip, MAX_IP_LEN);
        pNewUserInfo->login_port = pMsg->login_port;
        pNewUserInfo->reg_info_index = reg_info_index;
        pNewUserInfo->alarm_info_send_flag = 0;
        pNewUserInfo->tvwall_status_send_flag = 0;
        pNewUserInfo->tcp_sock = -1;
        pNewUserInfo->tcp_keep_alive_sock = -1;

        DEBUG_TRACE(MODULE_USER, LOG_TRACE, "user_reg_msg_proc() user_info_add:user_id=%s,login_ip=%s,login_port=%d,reg_info_index=%d  \r\n", pNewUserInfo->user_id, pNewUserInfo->login_ip, pNewUserInfo->login_port, pNewUserInfo->reg_info_index);

        user_pos = user_info_add(pNewUserInfo);

        if (user_pos < 0)
        {
            delete  pNewUserInfo;
            memset(strErrorCode, 0, 32);
            snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_USER_CREAT_USER_INFO_ERROR);
            SIP_UASAnswerToRegister(reg_info_index, 403, strErrorCode);
            //SIP_UASAnswerToRegister(reg_info_index, 503, (char*)"add User Info Error");
            DEBUG_TRACE(MODULE_USER, LOG_ERROR, "user_reg_msg_proc() exit---: add User Info Error \r\n");
            SystemLog(EV9000_CMS_USER_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "�û���¼ʧ��:�û�ID=%s, IP��ַ=%s, �˿ں�=%d, ԭ��=%s", pMsg->register_id, pMsg->login_ip, pMsg->login_port, (char*)"����û���Ϣʧ��");
            EnSystemLog(EV9000_CMS_USER_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "User login failed:User ID=%s, User IP=%s, Port=%d, reason=%s", pMsg->register_id, pMsg->login_ip, pMsg->login_port, (char*)"Add user information failed.");
            return -1;
        }
    }

    /* ��ȡ�û���Ϣ */
    pUserInfo = user_info_get(user_pos);

    if (NULL == pUserInfo)
    {
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_USER_GET_USER_INFO_ERROR);
        SIP_UASAnswerToRegister(reg_info_index, 403, strErrorCode);
        //SIP_UASAnswerToRegister(reg_info_index, 503, (char*)"Get User Info Error");
        DEBUG_TRACE(MODULE_USER, LOG_ERROR, "user_reg_msg_proc() exit---: Get User Info Error \r\n");
        SystemLog(EV9000_CMS_USER_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "�û���¼ʧ��:�û�ID=%s, IP��ַ=%s, �˿ں�=%d, ԭ��=%s", pMsg->register_id, pMsg->login_ip, pMsg->login_port, (char*)"��ȡ�û���Ϣʧ��");
        EnSystemLog(EV9000_CMS_USER_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "User login failed:User ID=%s, User IP=%s, Port=%d, reason=%s", pMsg->register_id, pMsg->login_ip, pMsg->login_port, (char*)"Failed to get user information.");
        return -1;
    }

    if (pUserInfo->user_level != user_cfg.user_level)
    {
        pUserInfo->user_level = user_cfg.user_level;
    }

    if (pUserInfo->reg_info_index != reg_info_index)
    {
        pUserInfo->reg_info_index = reg_info_index;
    }

    /* ��ȡ��ʱʱ�� */
    DEBUG_TRACE(MODULE_USER, LOG_TRACE, "user_reg_msg_proc() UserInfo:register_id=%s, iExpires=%d, reg_status=%d, reg_info_index=%d \r\n", pUserInfo->user_id, iExpires, pUserInfo->reg_status, pUserInfo->reg_info_index);
    printf("user_reg_msg_proc() UserInfo:register_id=%s, iExpires=%d, reg_status=%d, reg_info_index=%d \r\n", pUserInfo->user_id, iExpires, pUserInfo->reg_status, pUserInfo->reg_info_index);

    if (iExpires > 0)
    {
        i = UserReg(pUserInfo, user_cfg, pdboper);

        if (i != 0)
        {
            strUserID = pUserInfo->user_id;

            /* �Ƴ��û������ĵ�λ��Ϣ */
            i = RemoveGBLogicDeviceLockInfoByUserInfo(pUserInfo);

            if (0 != i)
            {
                DEBUG_TRACE(MODULE_USER, LOG_ERROR, "user_reg_msg_proc() RemoveGBLogicDeviceLockInfoByUserInfo Error:user_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, i);
            }
            else
            {
                DEBUG_TRACE(MODULE_USER, LOG_TRACE, "user_reg_msg_proc() RemoveGBLogicDeviceLockInfoByUserInfo OK:user_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, i);
            }

            /* �����û���ҵ��ֹͣ����ҵ�� */
            i = StopAllServiceTaskByUserInfo(pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);

            if (0 != i)
            {
                DEBUG_TRACE(MODULE_USER, LOG_ERROR, "user_reg_msg_proc() StopAllServiceTaskByUserInfo Error:user_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, i);
            }
            else
            {
                DEBUG_TRACE(MODULE_USER, LOG_TRACE, "user_reg_msg_proc() StopAllServiceTaskByUserInfo OK:user_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, i);
            }

            /* ֹͣ�û�ҵ�����߳� */
            i = user_srv_proc_thread_recycle(pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);

            if (0 != i)
            {
                DEBUG_TRACE(MODULE_USER, LOG_ERROR, "user_reg_msg_proc() user_srv_proc_thread_recycle Error:user_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, i);
            }
            else
            {
                DEBUG_TRACE(MODULE_USER, LOG_TRACE, "user_reg_msg_proc() user_srv_proc_thread_recycle OK:user_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, i);
            }

            /* �����ݿ���ɾ�������û���Ϣ */
            i = DeleteUserRegInfoFromDB(pUserInfo->user_index, pMsg->login_ip, pMsg->login_port, pdboper);

            if (i < 0)
            {
                DEBUG_TRACE(MODULE_USER, LOG_ERROR, "user_reg_msg_proc() DeleteUserRegInfoFromDB Error:UserID=%s, UserIndex=%u, login_ip=%s, login_port=%d, i=%d \r\n", pMsg->register_id, pUserInfo->user_index, pMsg->login_ip, pMsg->login_port, i);
            }
            else
            {
                DEBUG_TRACE(MODULE_USER, LOG_TRACE, "user_reg_msg_proc() DeleteUserRegInfoFromDB OK:UserID=%s, UserIndex=%u, login_ip=%s, login_port=%d, i=%d \r\n", pMsg->register_id, pUserInfo->user_index, pMsg->login_ip, pMsg->login_port, i);
            }

            i = NotifyOnlineUserToAllClientUser(pUserInfo, 2, pdboper);

            if (i < 0)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "����֪ͨ�����û��仯��Ϣ�������û�ʧ��, �Ƴ��������û�:�û�ID=%s, IP��ַ=%s, �˿ں�=%d\r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Users send notification that online users change	to	online user failed, remove online users: user ID = % s, IP address = % s, port = % d\r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
                DEBUG_TRACE(MODULE_USER, LOG_ERROR, "user_reg_msg_proc() NotifyOnlineUserToAllClientUser Error:UserID=%s, UserIndex=%u, login_ip=%s, login_port=%d, i=%d \r\n", pUserInfo->user_id, pUserInfo->user_index, pUserInfo->login_ip, pUserInfo->login_port, i);
            }
            else if (i > 0)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "����֪ͨ�����û��仯��Ϣ�������û��ɹ�, �Ƴ��������û�:�û�ID=%s, IP��ַ=%s, �˿ں�=%d\r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Users send notification that online users change	to	online user successfully, remove online users: user ID = % s, IP address = % s, port = % d\r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
                DEBUG_TRACE(MODULE_USER, LOG_TRACE, "user_reg_msg_proc() NotifyOnlineUserToAllClientUser OK:UserID=%s, UserIndex=%u, login_ip=%s, login_port=%d, i=%d \r\n", pUserInfo->user_id, pUserInfo->user_index, pUserInfo->login_ip, pUserInfo->login_port, i);
            }

            i = user_info_remove(pMsg->register_id, pMsg->login_ip, pMsg->login_port);

            if (0 != i)
            {
                DEBUG_TRACE(MODULE_USER, LOG_ERROR, "user_reg_msg_proc() user_info_remove Error:UserID=%s, login_ip=%s, login_port=%d, i=%d \r\n", pMsg->register_id, pMsg->login_ip, pMsg->login_port, i);
            }
            else
            {
                DEBUG_TRACE(MODULE_USER, LOG_TRACE, "user_reg_msg_proc() user_info_remove OK:UserID=%s, login_ip=%s, login_port=%d, i=%d \r\n", pMsg->register_id, pMsg->login_ip, pMsg->login_port, i);
            }

            i = UpdateUserRegInfo2DB2(strUserID, pdboper);   //�������ݿ�״̬

            if (i < 0)
            {
                DEBUG_TRACE(MODULE_USER, LOG_ERROR, "user_reg_msg_proc() UpdateUserRegInfo2DB2 Error:UserID=%s, i=%d \r\n", strUserID.c_str(), i);
            }
            else
            {
                DEBUG_TRACE(MODULE_USER, LOG_TRACE, "user_reg_msg_proc() UpdateUserRegInfo2DB2 OK:UserID=%s, i=%d \r\n", strUserID.c_str(), i);
            }

            /* �ͷŵ�û�õ�TCP���� */
            free_unused_user_tcp_connect();
        }

        return i;
    }

    memset(strErrorCode, 0, 32);
    snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_SYSTEM_ERROR);
    SIP_UASAnswerToRegister(reg_info_index, 500, strErrorCode);
    //SIP_UASAnswerToRegister(reg_info_index, 500, (char*)"Server Error");
    DEBUG_TRACE(MODULE_USER, LOG_ERROR, "user_reg_msg_proc() exit---: Server Error \r\n");
    printf("user_reg_msg_proc() exit---: Server Error \r\n");
    return -1;
}

/*****************************************************************************
 �� �� ��  : user_unreg_msg_proc
 ��������  : �û�ע����Ϣ����
 �������  : user_reg_msg_t* pMsg
             DBOper* pdboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2017��3��7��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int user_unreg_msg_proc(user_reg_msg_t* pMsg, DBOper* pdboper)
{
    int i = 0;
    int user_pos = -1;
    int reg_info_index = 0;
    user_info_t* pUserInfo = NULL;
    int iExpires = pMsg->expires;
    char strErrorCode[32] = {0};

    DEBUG_TRACE(MODULE_USER, LOG_TRACE, "user_unreg_msg_proc() Enter---: register_id=%s,login_ip=%s,login_port=%d \r\n", pMsg->register_id, pMsg->login_ip, pMsg->login_port);
    printf("\r\nuser_unreg_msg_proc() Enter---: register_id=%s,login_ip=%s,login_port=%d \r\n", pMsg->register_id, pMsg->login_ip, pMsg->login_port);

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�û�ע����¼����:�û�ID=%s, IP��ַ=%s, �˿ں�=%d", pMsg->register_id, pMsg->login_ip, pMsg->login_port);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "User login: user ID = % s, = % s IP address, port number = %d", pMsg->register_id, pMsg->login_ip, pMsg->login_port);

    /*��֤����*/
    if (!pMsg || pMsg->reg_info_index < 0)
    {
        DEBUG_TRACE(MODULE_USER, LOG_ERROR, "user_unreg_msg_proc() exit---: msg /reg_info_index error \r\n");
        return -1;
    }

    reg_info_index = pMsg->reg_info_index;

    if ('\0' == pMsg->register_id[0] || '\0' == pMsg->login_ip[0] || pMsg->login_port <= 0)
    {
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_REG_MSG_ERROR);
        SIP_UASAnswerToRegister(reg_info_index, 403, strErrorCode);
        //SIP_UASAnswerToRegister(reg_info_index, 503, (char*)"msg /User ID/login_ip login_port error");
        DEBUG_TRACE(MODULE_USER, LOG_ERROR, "user_unreg_msg_proc() exit---: msg /User ID/login_ip login_port error \r\n");
        SystemLog(EV9000_CMS_USER_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "�û�ע����¼ʧ��:�û�ID=%s, IP��ַ=%s, �˿ں�=%d, ԭ��=%s", pMsg->register_id, pMsg->login_ip, pMsg->login_port, (char*)"�û�ID/�û�IP��/�û��˿ںŲ��Ϸ�");
        EnSystemLog(EV9000_CMS_USER_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "User login failed:User ID=%s, User IP=%s, Port=%d, reason=%s", pMsg->register_id, pMsg->login_ip, pMsg->login_port, (char*)"User ID / user IP / user port number is not legitimate.");
        return -1;
    }

    user_pos = user_info_find(pMsg->register_id, pMsg->login_ip, pMsg->login_port);

    DEBUG_TRACE(MODULE_USER, LOG_TRACE, "user_unreg_msg_proc() user_pos=%d \r\n", user_pos);
    printf("user_unreg_msg_proc() user_info_find:register_id=%s,login_ip=%s,login_port=%d,user_pos=%d \r\n", pMsg->register_id, pMsg->login_ip, pMsg->login_port, user_pos);

    if (user_pos < 0)   //�µ�¼�û������ڴ��¼
    {
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_USER_FIND_USER_INFO_ERROR);
        SIP_UASAnswerToRegister(reg_info_index, 403, strErrorCode);
        //SIP_UASAnswerToRegister(reg_info_index, 503, (char*)"add User Info Error");
        DEBUG_TRACE(MODULE_USER, LOG_TRACE, "user_unreg_msg_proc() exit---: Find User Info Error \r\n");
        SystemLog(EV9000_CMS_USER_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "�û�ע����¼ʧ��:�û�ID=%s, IP��ַ=%s, �˿ں�=%d, ԭ��=%s", pMsg->register_id, pMsg->login_ip, pMsg->login_port, (char*)"�����û���Ϣʧ��");
        EnSystemLog(EV9000_CMS_USER_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "User login failed:User ID=%s, User IP=%s, Port=%d, reason=%s", pMsg->register_id, pMsg->login_ip, pMsg->login_port, (char*)"Add user information failed.");
        return -1;
    }

    /* ��ȡ�û���Ϣ */
    pUserInfo = user_info_get(user_pos);

    if (NULL == pUserInfo)
    {
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_USER_GET_USER_INFO_ERROR);
        SIP_UASAnswerToRegister(reg_info_index, 403, strErrorCode);
        //SIP_UASAnswerToRegister(reg_info_index, 503, (char*)"Get User Info Error");
        DEBUG_TRACE(MODULE_USER, LOG_TRACE, "user_unreg_msg_proc() exit---: Get User Info Error \r\n");
        SystemLog(EV9000_CMS_USER_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "�û�ע����¼ʧ��:�û�ID=%s, IP��ַ=%s, �˿ں�=%d, ԭ��=%s", pMsg->register_id, pMsg->login_ip, pMsg->login_port, (char*)"��ȡ�û���Ϣʧ��");
        EnSystemLog(EV9000_CMS_USER_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "User login failed:User ID=%s, User IP=%s, Port=%d, reason=%s", pMsg->register_id, pMsg->login_ip, pMsg->login_port, (char*)"Failed to get user information.");
        return -1;
    }

    pUserInfo->reg_status = 3;

    /* ��ȡ��ʱʱ�� */
    DEBUG_TRACE(MODULE_USER, LOG_TRACE, "user_unreg_msg_proc() UserInfo:register_id=%s, iExpires=%d, reg_status=%d, reg_info_index=%d \r\n", pUserInfo->user_id, iExpires, pUserInfo->reg_status, pUserInfo->reg_info_index);
    printf("user_unreg_msg_proc() UserInfo:register_id=%s, iExpires=%d, reg_status=%d, reg_info_index=%d \r\n", pUserInfo->user_id, iExpires, pUserInfo->reg_status, pUserInfo->reg_info_index);

    if (iExpires < 0)             //�쳣
    {
        return UserUnRegAbnormal(pUserInfo, pdboper);
    }
    else if (0 == iExpires)   //ע��
    {
        return UserUnReg(pUserInfo, pdboper);
    }

    memset(strErrorCode, 0, 32);
    snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_SYSTEM_ERROR);
    SIP_UASAnswerToRegister(reg_info_index, 500, strErrorCode);
    //SIP_UASAnswerToRegister(reg_info_index, 500, (char*)"Server Error");
    DEBUG_TRACE(MODULE_USER, LOG_ERROR, "user_unreg_msg_proc() exit---: Server Error \r\n");
    printf("user_unreg_msg_proc() exit---: Server Error \r\n");
    return -1;
}

