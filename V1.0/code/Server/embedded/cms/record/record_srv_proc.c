
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

#include "common/gblfunc_proc.inc"
#include "common/gblconfig_proc.inc"
#include "common/log_proc.inc"

#include "device/device_info_mgn.inc"
#include "resource/resource_info_mgn.inc"

#include "record/record_srv_proc.inc"
#include "record/record_info_mgn.inc"

#include "service/call_func_proc.inc"

/*----------------------------------------------*
 * �ⲿ����˵��                                 *
 *----------------------------------------------*/
extern CR_Data_MAP g_CallRecordMap;             /* �����������ݶ��� */
extern record_info_list_t* g_RecordInfoList;    /* ¼����Ϣ���� */
extern int current_record_pos;                  /* ��ǰ¼��λ�� */

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
record_srv_msg_queue g_RecordSrvMsgQueue; /* ¼��ҵ����Ϣ���� */
#ifdef MULTI_THR
osip_mutex_t* g_RecordSrvMsgQueueLock = NULL;
#endif

/*----------------------------------------------*
 * ��������                                     *
 *----------------------------------------------*/

/*----------------------------------------------*
 * �궨��                                       *
 *----------------------------------------------*/
#if DECS("¼��ҵ����Ϣ����")
/*****************************************************************************
 �� �� ��  : record_srv_msg_init
 ��������  : ¼��ҵ����Ϣ�ṹ��ʼ��
 �������  : record_srv_msg_t ** record_srv_msg
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��27��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int record_srv_msg_init(record_srv_msg_t** record_srv_msg)
{
    *record_srv_msg = (record_srv_msg_t*)osip_malloc(sizeof(record_srv_msg_t));

    if (*record_srv_msg == NULL)
    {
        DEBUG_TRACE(MODULE_RECORD, LOG_DEBUG, "record_srv_msg_init() exit---: *record_srv_msg Smalloc Error \r\n");
        return -1;
    }

    (*record_srv_msg)->msg_type = MSG_TYPE_NULL;
    (*record_srv_msg)->caller_id[0] = '\0';
    (*record_srv_msg)->callee_id[0] = '\0';
    (*record_srv_msg)->response_code = 0;
    (*record_srv_msg)->reasonphrase[0] = '\0';
    (*record_srv_msg)->ua_dialog_index = -1;
    (*record_srv_msg)->msg_body[0] = '\0';
    (*record_srv_msg)->msg_body_len = 0;
    (*record_srv_msg)->cr_pos = -1;

    return 0;
}

/*****************************************************************************
 �� �� ��  : record_srv_msg_free
 ��������  : ¼��ҵ����Ϣ�ṹ�ͷ�
 �������  : record_srv_msg_t * record_srv_msg
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��27��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void record_srv_msg_free(record_srv_msg_t* record_srv_msg)
{
    if (record_srv_msg == NULL)
    {
        DEBUG_TRACE(MODULE_RECORD, LOG_DEBUG, "record_srv_msg_free() exit---: Param Error \r\n");
        return;
    }

    record_srv_msg->msg_type = MSG_TYPE_NULL;

    memset(record_srv_msg->caller_id, 0, MAX_ID_LEN + 4);
    memset(record_srv_msg->callee_id, 0, MAX_ID_LEN + 4);

    record_srv_msg->response_code = 0;

    memset(record_srv_msg->reasonphrase, 0, MAX_128CHAR_STRING_LEN + 4);

    record_srv_msg->ua_dialog_index = -1;

    memset(record_srv_msg->msg_body, 0, MAX_MSG_BODY_STRING_LEN + 4);

    record_srv_msg->msg_body_len = 0;

    osip_free(record_srv_msg);
    record_srv_msg = NULL;

    return;
}

/*****************************************************************************
 �� �� ��  : record_srv_msg_list_init
 ��������  : ¼��ҵ����Ϣ���г�ʼ��
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��27��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int record_srv_msg_list_init()
{
    g_RecordSrvMsgQueue.clear();

#ifdef MULTI_THR
    /* init smutex */
    g_RecordSrvMsgQueueLock = (osip_mutex_t*)osip_mutex_init();

    if (NULL == g_RecordSrvMsgQueueLock)
    {
        DEBUG_TRACE(MODULE_RECORD, LOG_DEBUG, "record_srv_msg_list_init() exit---: Record Srv Msg List Lock Init Error \r\n");
        return -1;
    }

#endif
    return 0;
}

/*****************************************************************************
 �� �� ��  : record_srv_msg_list_free
 ��������  : ¼��ҵ����Ϣ�����ͷ�
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��27��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void record_srv_msg_list_free()
{
    record_srv_msg_t* pRecordSrvMsg = NULL;

    while (!g_RecordSrvMsgQueue.empty())
    {
        pRecordSrvMsg = (record_srv_msg_t*) g_RecordSrvMsgQueue.front();
        g_RecordSrvMsgQueue.pop_front();

        if (NULL != pRecordSrvMsg)
        {
            record_srv_msg_free(pRecordSrvMsg);
            pRecordSrvMsg = NULL;
        }
    }

    g_RecordSrvMsgQueue.clear();

#ifdef MULTI_THR

    if (NULL != g_RecordSrvMsgQueueLock)
    {
        osip_mutex_destroy((struct osip_mutex*)g_RecordSrvMsgQueueLock);
        g_RecordSrvMsgQueueLock = NULL;
    }

#endif

    return;
}

/*****************************************************************************
 �� �� ��  : record_srv_msg_list_clean
 ��������  : ¼��ҵ����Ϣ�������
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
void record_srv_msg_list_clean()
{
    record_srv_msg_t* pRecordSrvMsg = NULL;

    while (!g_RecordSrvMsgQueue.empty())
    {
        pRecordSrvMsg = (record_srv_msg_t*) g_RecordSrvMsgQueue.front();
        g_RecordSrvMsgQueue.pop_front();

        if (NULL != pRecordSrvMsg)
        {
            record_srv_msg_free(pRecordSrvMsg);
            pRecordSrvMsg = NULL;
        }
    }

    g_RecordSrvMsgQueue.clear();

    return;
}

/*****************************************************************************
 �� �� ��  : record_srv_msg_add
 ��������  : ���¼��ҵ����Ϣ��������
 �������  : msg_type_t msg_type
             char* caller_id
             char* callee_id
             char* call_id
             int response_code
             char* reasonphrase
             int ua_dialog_index
             char* msg_body
             int msg_body_len
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��27��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int record_srv_msg_add(msg_type_t msg_type, char* caller_id, char* callee_id, int response_code, char* reasonphrase, int ua_dialog_index, char* msg_body, int msg_body_len, int cr_pos)
{
    record_srv_msg_t* pRecordSrvMsg = NULL;
    int iRet = 0;

    if (caller_id == NULL || callee_id == NULL)
    {
        DEBUG_TRACE(MODULE_RECORD, LOG_DEBUG, "record_srv_msg_add() exit---: Param Error \r\n");
        return -1;
    }

    iRet = record_srv_msg_init(&pRecordSrvMsg);

    if (iRet != 0)
    {
        DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "record_srv_msg_add() exit---: Record Srv Msg Init Error \r\n");
        return -1;
    }

    pRecordSrvMsg->msg_type = msg_type;

    if (NULL != caller_id)
    {
        osip_strncpy(pRecordSrvMsg->caller_id, caller_id, MAX_ID_LEN);
    }

    if (NULL != callee_id)
    {
        osip_strncpy(pRecordSrvMsg->callee_id, callee_id, MAX_ID_LEN);
    }

    pRecordSrvMsg->response_code = response_code;

    if (NULL != reasonphrase)
    {
        osip_strncpy(pRecordSrvMsg->reasonphrase, reasonphrase, MAX_128CHAR_STRING_LEN);
    }

    pRecordSrvMsg->ua_dialog_index = ua_dialog_index;

    if (NULL != msg_body)
    {
        osip_strncpy(pRecordSrvMsg->msg_body, msg_body, MAX_MSG_BODY_STRING_LEN);
    }

    pRecordSrvMsg->msg_body_len = msg_body_len;
    pRecordSrvMsg->cr_pos = cr_pos;

#if 0
    CMS_GBL_SMUTEX_LOCK((struct osip_mutex*)g_RecordSrvMsgQueueLock);
#endif

    g_RecordSrvMsgQueue.push_back(pRecordSrvMsg);

#if 0
    CMS_GBL_SMUTEX_UNLOCK((struct osip_mutex*)g_RecordSrvMsgQueueLock);
#endif

    return 0;
}

/*****************************************************************************
 �� �� ��  : scan_record_srv_msg_list
 ��������  : ɨ��༶������Ϣ����
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��17��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void scan_record_srv_msg_list()
{
    int iRet = 0;
    record_srv_msg_t* pRecordSrvMsg = NULL;

#if 0
    CMS_GBL_SMUTEX_LOCK((struct osip_mutex*)g_RecordSrvMsgQueueLock);
#endif

    while (!g_RecordSrvMsgQueue.empty())
    {
        pRecordSrvMsg = (record_srv_msg_t*) g_RecordSrvMsgQueue.front();
        g_RecordSrvMsgQueue.pop_front();

        if (NULL != pRecordSrvMsg)
        {
            break;
        }
    }

    if (NULL != pRecordSrvMsg)
    {
        DEBUG_TRACE(MODULE_RECORD, LOG_INFO,  "scan_record_srv_msg_list() \
        \r\n In Param: \
        \r\n msg_type=%d \
        \r\n caller_id=%s \
        \r\n callee_id=%s \
        \r\n response_code=%d \
        \r\n ua_dialog_index=%d \
        \r\n msg_body_len=%d \
        \r\n ", pRecordSrvMsg->msg_type, pRecordSrvMsg->caller_id, pRecordSrvMsg->callee_id, pRecordSrvMsg->response_code, pRecordSrvMsg->ua_dialog_index, pRecordSrvMsg->msg_body_len);

        iRet = record_srv_msg_proc(pRecordSrvMsg);
        DEBUG_TRACE(MODULE_RECORD, LOG_INFO,  "scan_record_srv_msg_list() record_srv_msg_proc end:iRet=%d\r\n", iRet);

        record_srv_msg_free(pRecordSrvMsg);
        pRecordSrvMsg = NULL;
    }

#if 0
    CMS_GBL_SMUTEX_UNLOCK((struct osip_mutex*)g_RecordSrvMsgQueueLock);
#endif

    return;
}
#endif

/*****************************************************************************
 �� �� ��  : record_srv_msg_proc
 ��������  : ¼��ҵ����Ϣ����
 �������  : msg_type_t msg_type
                            char* caller_id
                            char* callee_id
                            char* call_id
                            int response_code
                            int ua_dialog_index
                            char* msg_body
                            int msg_body_len
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��17��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int record_srv_msg_proc(record_srv_msg_t* pRecordSrvMsg)
{
    int i = 0;

    if (NULL == pRecordSrvMsg)
    {
        return -1;
    }

    //DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "record_srv_msg_proc() msg_type=%d  \r\n", msg_type);

    switch (pRecordSrvMsg->msg_type)
    {
        case  MSG_INVITE_RESPONSE:
            i = record_invite_response_msg_proc(pRecordSrvMsg->cr_pos, pRecordSrvMsg->ua_dialog_index, pRecordSrvMsg->response_code, pRecordSrvMsg->msg_body, pRecordSrvMsg->msg_body_len);
            DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "record_srv_msg_proc() record_invite_response_msg_proc:i=%d \r\n", i);
            printf("record_srv_msg_proc() record_invite_response_msg_proc:i=%d \r\n", i);
            break;

        case  MSG_BYE:
            i = record_bye_msg_proc(pRecordSrvMsg->cr_pos, pRecordSrvMsg->ua_dialog_index);
            //DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "record_srv_msg_proc() record_bye_msg_proc:i=%d \r\n", i);
            break;

        case  MSG_BYE_RESPONSE:
            i = record_bye_response_msg_proc(pRecordSrvMsg->cr_pos, pRecordSrvMsg->ua_dialog_index, pRecordSrvMsg->response_code);
            //DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "record_srv_msg_proc() record_bye_response_msg_proc:i=%d \r\n", i);
            break;

        default:
            DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "record_srv_msg_proc() exit---: Not Support Message Type:%d \r\n", pRecordSrvMsg->msg_type);
            return -1;
    }

    return i;
}

/*****************************************************************************
 �� �� ��  : record_invite_response_msg_proc
 ��������  : ¼��INVITE ��Ӧ��Ϣ����
 �������  : char* caller_id
                            char* callee_id
                            char* call_id
                            int ua_dialog_index
                            char* msg_body
                            int msg_body_len
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��17��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int record_invite_response_msg_proc(int cr_pos, int ua_dialog_index, int response_code, char* msg_body, int msg_body_len)
{
    int i = 0;
    int iRet = 0 ;
    int record_info_index = -1;
    sdp_message_t* pRemoteSDP = NULL;
    sdp_param_t stRemoteSDPParam;
    sdp_extend_param_t stRemoteSDPExParam;
    record_info_t* pRecordInfo = NULL;
    cr_t* pCrData = NULL;
    string record_id = "";
    GBLogicDevice_info_t* pCalleeGBLogicDeviceInfo = NULL;

    //printf_system_time();
    printf("record_invite_response_msg_proc() Enter--- :cr_pos=%d, ua_dialog_index=%d, response_code=%d \r\n", cr_pos, ua_dialog_index, response_code);

    if (cr_pos < 0)
    {
        if (200 == response_code)
        {
            i = SIP_SendAck(ua_dialog_index);
            DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "record_invite_response_msg_proc() SIP_SendAck:caller_ua_index=%d, i=%d \r\n", ua_dialog_index, i);

            i = SIP_SendBye(ua_dialog_index);
            DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "record_invite_response_msg_proc() SIP_SendBye:caller_ua_index=%d, i=%d \r\n", ua_dialog_index, i);
        }

        DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "record_invite_response_msg_proc() exit---: Param Error \r\n");
        return -1;
    }

    pCrData = call_record_get(cr_pos);
    //printf_system_time();
    printf("record_invite_response_msg_proc() call_record_get:cr_pos=%d\r\n", cr_pos);

    if (pCrData == NULL)
    {
        if (200 == response_code)
        {
            i = SIP_SendAck(ua_dialog_index);
            DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "record_invite_response_msg_proc() SIP_SendAck:caller_ua_index=%d, i=%d \r\n", ua_dialog_index, i);

            i = SIP_SendBye(ua_dialog_index);
            DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "record_invite_response_msg_proc() SIP_SendBye:caller_ua_index=%d, i=%d \r\n", ua_dialog_index, i);
        }

        DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "record_invite_response_msg_proc() exit---: Get Call Record Data Info Error \r\n");
        return -1;
    }

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "¼��ҵ��, INVITE��Ӧ��Ϣ����:�߼��豸ID=%s, �߼��豸IP��ַ=%s, callee_ua_index=%d, ua_dialog_index=%d, cr_pos=%d", pCrData->callee_id, pCrData->callee_ip, pCrData->callee_ua_index, ua_dialog_index, cr_pos);

    /*  ��ȡ¼����Ϣ�� */
    record_info_index = record_info_find_by_cr_index_for_response(cr_pos);  /*  ¼������� */
    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "¼��ҵ��, INVITE��Ӧ��Ϣ����:���ݺ�����Դ�������Ҷ�Ӧ��¼�������Ϣ, �߼��豸ID=%s, �߼��豸IP��ַ=%s, cr_pos=%d, record_info_index=%d", pCrData->callee_id, pCrData->callee_ip, cr_pos, record_info_index);
    //printf_system_time();
    printf("record_invite_response_msg_proc() record_info_find_by_cr_index_for_response:cr_pos=%d, record_info_index=%d\r\n", cr_pos, record_info_index);

    if (record_info_index < 0)
    {
        if (-2 == record_info_index)
        {
            SystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "¼��ҵ��, INVITE��Ӧ��Ϣ����ʧ��:�߼��豸ID=%s, ԭ��=%s", pCrData->callee_id, (char*)"�����߼��豸¼����Դʧ��:��Ӧ��¼�������Ϣ��ɾ��");
            EnSystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "INVITE response message process failed: logic deviceID=%s, cause=%s", pCrData->callee_id, (char*)"Search video resource of logic device failed");
        }
        else if (-3 == record_info_index)
        {
            SystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "¼��ҵ��, INVITE��Ӧ��Ϣ����ʧ��:�߼��豸ID=%s, ԭ��=%s", pCrData->callee_id, (char*)"�����߼��豸¼����Դʧ��:��Ӧ��¼�������Ϣû������");
            EnSystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "INVITE response message process failed: logic deviceID=%s, cause=%s", pCrData->callee_id, (char*)"Search video resource of logic device failed");
        }
        else if (-4 == record_info_index)
        {
            SystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "¼��ҵ��, INVITE��Ӧ��Ϣ����ʧ��:�߼��豸ID=%s, ԭ��=%s", pCrData->callee_id, (char*)"�����߼��豸¼����Դʧ��:��Ӧ��¼�������Ϣ��TSU����Ϊ��");
            EnSystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "INVITE response message process failed: logic deviceID=%s, cause=%s", pCrData->callee_id, (char*)"Search video resource of logic device failed");
        }
        else if (-5 == record_info_index)
        {
            SystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "¼��ҵ��, INVITE��Ӧ��Ϣ����ʧ��:�߼��豸ID=%s, ԭ��=%s", pCrData->callee_id, (char*)"�����߼��豸¼����Դʧ��:��Ӧ��¼�������Ϣ��״̬�����ڵȴ�������Ӧ");
            EnSystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "INVITE response message process failed: logic deviceID=%s, cause=%s", pCrData->callee_id, (char*)"Search video resource of logic device failed");
        }
        else
        {
            SystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "¼��ҵ��, INVITE��Ӧ��Ϣ����ʧ��:�߼��豸ID=%s, ԭ��=%s", pCrData->callee_id, (char*)"�����߼��豸¼����Դʧ��");
            EnSystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "INVITE response message process failed: logic deviceID=%s, cause=%s", pCrData->callee_id, (char*)"Search video resource of logic device failed");
        }

        /* ��Ӧ��Ϣ������ */
        if (200 == response_code)
        {
            i = SIP_SendAck(ua_dialog_index);
            DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "record_invite_response_msg_proc() SIP_SendAck:caller_ua_index=%d, i=%d \r\n", ua_dialog_index, i);

            i = SIP_SendBye(ua_dialog_index);
            DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "record_invite_response_msg_proc() SIP_SendBye:caller_ua_index=%d, i=%d \r\n", ua_dialog_index, i);
        }

        i = RemoveDeviceRecordInfo(pCrData->callee_id, pCrData->callee_stream_type, pCrData->tsu_resource_index);
        DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "record_invite_response_msg_proc() :callee_id=%s, callee_stream_type=%d, tsu_resource_index=%d, i=%d \r\n", pCrData->callee_id, pCrData->callee_stream_type, pCrData->tsu_resource_index, i);
        //printf_system_time();
        printf("record_invite_response_msg_proc() :callee_id=%s, callee_stream_type=%d, tsu_resource_index=%d, i=%d \r\n", pCrData->callee_id, pCrData->callee_stream_type, pCrData->tsu_resource_index, i);

        i = return_error_for_wait_answer_call_record(pCrData, EV9000_CMS_ERR_RECORD_FIND_RECORD_INFO_ERROR);

        i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
        i = call_record_remove(cr_pos);

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "record_invite_response_msg_proc() call_record_remove Error:cr_pos=%d, i=%d \r\n", cr_pos, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "record_invite_response_msg_proc() call_record_remove OK:cr_pos=%d, i=%d \r\n", cr_pos, i);
        }

        DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "record_invite_response_msg_proc() exit---: record_info_find_by_cr_index_for_response Error \r\n");
        return -1;
    }

    pRecordInfo = record_info_get(record_info_index);
    //printf_system_time();
    printf("record_invite_response_msg_proc() record_info_get:record_info_index=%d\r\n", record_info_index);

    if (NULL == pRecordInfo)
    {
        SystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "¼��ҵ��, INVITE��Ӧ��Ϣ����ʧ��:�߼��豸ID=%s, ԭ��=%s", pCrData->callee_id, (char*)"��ȡ�߼��豸¼����Դʧ��");

        /* ��Ӧ��Ϣ������ */
        if (200 == response_code)
        {
            i = SIP_SendAck(ua_dialog_index);
            DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "record_invite_response_msg_proc() SIP_SendAck:caller_ua_index=%d, i=%d \r\n", ua_dialog_index, i);

            i = SIP_SendBye(ua_dialog_index);
            DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "record_invite_response_msg_proc() SIP_SendBye:caller_ua_index=%d, i=%d \r\n", ua_dialog_index, i);
        }

        i = RemoveDeviceRecordInfo(pCrData->callee_id, pCrData->callee_stream_type, pCrData->tsu_resource_index);
        DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "record_invite_response_msg_proc() :callee_id=%s, callee_stream_type=%d, tsu_resource_index=%d, i=%d \r\n", pCrData->callee_id, pCrData->callee_stream_type, pCrData->tsu_resource_index, i);

        i = return_error_for_wait_answer_call_record(pCrData, EV9000_CMS_ERR_RECORD_GET_RECORD_INFO_ERROR);

        i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
        i = call_record_remove(cr_pos);

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "record_invite_response_msg_proc() call_record_remove Error:cr_pos=%d, i=%d \r\n", cr_pos, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "record_invite_response_msg_proc() call_record_remove OK:cr_pos=%d, i=%d \r\n", cr_pos, i);
        }

        DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "record_invite_response_msg_proc() exit---: record_info_get Error \r\n");
        return -1;
    }

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "¼��ҵ��, INVITE��Ӧ��Ϣ����:¼���������=%u, �߼��豸ID=%s, �߼��豸IP��ַ=%s, record_cr_index=%d", pRecordInfo->uID, pCrData->callee_id, pCrData->callee_ip, pRecordInfo->record_cr_index);

    if (200 == response_code)
    {
        if (NULL == msg_body || msg_body_len == 0)
        {
            SystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "¼��ҵ��, INVITE��Ӧ��Ϣ����ʧ��:�߼��豸ID=%s, ԭ��=%s", pCrData->callee_id, (char*)"200��Ӧ��Ϣ������û��SDP��Ϣ");
            EnSystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "INVITE response message process failed: logic deviceID=%s, cause=%s", pCrData->callee_id, (char*)"200 response mesaage body do not have SDP message");

            /* ��Ӧ��Ϣ������ */
            i = SIP_SendAck(ua_dialog_index);
            DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "record_invite_response_msg_proc() SIP_SendAck:caller_ua_index=%d, i=%d \r\n", ua_dialog_index, i);

            i = SIP_SendBye(ua_dialog_index);
            DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "record_invite_response_msg_proc() SIP_SendBye:caller_ua_index=%d, i=%d \r\n", ua_dialog_index, i);

            i = RemoveDeviceRecordInfo(pCrData->callee_id, pCrData->callee_stream_type, pCrData->tsu_resource_index);
            DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "record_invite_response_msg_proc() :callee_id=%s, callee_stream_type=%d, tsu_resource_index=%d, i=%d \r\n", pCrData->callee_id, pCrData->callee_stream_type, pCrData->tsu_resource_index, i);

            i = return_error_for_wait_answer_call_record(pCrData, EV9000_CMS_ERR_SDP_GET_VIDEO_INFO_ERROR);

            i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
            i = call_record_remove(cr_pos);

            if (0 != i)
            {
                DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "record_invite_response_msg_proc() call_record_remove Error:cr_pos=%d, i=%d \r\n", cr_pos, i);
            }
            else
            {
                DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "record_invite_response_msg_proc() call_record_remove OK:cr_pos=%d, i=%d \r\n", cr_pos, i);
            }

            DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "record_invite_response_msg_proc() NULL == msg_body || msg_body_len == 0  \r\n");
            return -1;
        }

        /* ��ȡ200��Ϣ�еı��е�sdp��Ϣ */
        i = sdp_message_init(&pRemoteSDP);

        if (0 != i)
        {
            SystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "¼��ҵ��, INVITE��Ӧ��Ϣ����ʧ��:�߼��豸ID=%s, ԭ��=%s", pCrData->callee_id, (char*)"SDP��Ϣ��ʼ��ʧ��");
            EnSystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "INVITE response message process failed: logic deviceID=%s, cause=%s", pCrData->callee_id, (char*)"SDP info initialization failed");

            /* ��Ӧ��Ϣ������ */
            i = SIP_SendAck(ua_dialog_index);
            DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "record_invite_response_msg_proc() SIP_SendAck:caller_ua_index=%d, i=%d \r\n", ua_dialog_index, i);

            i = SIP_SendBye(ua_dialog_index);
            DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "record_invite_response_msg_proc() SIP_SendBye:caller_ua_index=%d, i=%d \r\n", ua_dialog_index, i);

            i = RemoveDeviceRecordInfo(pCrData->callee_id, pCrData->callee_stream_type, pCrData->tsu_resource_index);
            DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "record_invite_response_msg_proc() :callee_id=%s, callee_stream_type=%d, tsu_resource_index=%d, i=%d \r\n", pCrData->callee_id, pCrData->callee_stream_type, pCrData->tsu_resource_index, i);

            i = return_error_for_wait_answer_call_record(pCrData, EV9000_CMS_ERR_SDP_MSG_INIT_ERROR);

            i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
            i = call_record_remove(cr_pos);

            if (0 != i)
            {
                DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "record_invite_response_msg_proc() call_record_remove Error:cr_pos=%d, i=%d \r\n", cr_pos, i);
            }
            else
            {
                DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "record_invite_response_msg_proc() call_record_remove OK:cr_pos=%d, i=%d \r\n", cr_pos, i);
            }

            DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "record_invite_response_msg_proc() exit---: Remote SDP Message Init Error \r\n");
            return -1;
        }

        i = sdp_message_parse(pRemoteSDP, msg_body); /*parse body */

        if (0 != i)
        {
            SystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "¼��ҵ��, INVITE��Ӧ��Ϣ����ʧ��:�߼��豸ID=%s, ԭ��=%s", pCrData->callee_id, (char*)"����200��Ӧ��Ϣ������SDP��Ϣʧ��");
            EnSystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "INVITE response message process failed: logic deviceID=%s, cause=%s", pCrData->callee_id, (char*)"Analysis SDP message in 200 response message body failed");

            /* ��Ӧ��Ϣ������ */
            i = SIP_SendAck(ua_dialog_index);
            DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "record_invite_response_msg_proc() SIP_SendAck:caller_ua_index=%d, i=%d \r\n", ua_dialog_index, i);

            i = SIP_SendBye(ua_dialog_index);
            DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "record_invite_response_msg_proc() SIP_SendBye:caller_ua_index=%d, i=%d \r\n", ua_dialog_index, i);

            i = RemoveDeviceRecordInfo(pCrData->callee_id, pCrData->callee_stream_type, pCrData->tsu_resource_index);
            DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "record_invite_response_msg_proc() RemoveDeviceRecordInfo:cr_pos=%d \r\n", cr_pos);

            i = return_error_for_wait_answer_call_record(pCrData, EV9000_CMS_ERR_SDP_MSG_PARSE_ERROR);

            i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
            i = call_record_remove(cr_pos);

            if (0 != i)
            {
                DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "record_invite_response_msg_proc() call_record_remove Error:cr_pos=%d, i=%d \r\n", cr_pos, i);
            }
            else
            {
                DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "record_invite_response_msg_proc() call_record_remove OK:cr_pos=%d, i=%d \r\n", cr_pos, i);
            }

            sdp_message_free(pRemoteSDP);
            pRemoteSDP = NULL;

            DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "record_invite_response_msg_proc() exit---: Remote SDP Message Parse Error \r\n");
            return -1;
        }

        /* ��ȡЭ�̵�SDP��Ϣ */
        memset(&stRemoteSDPParam, 0, sizeof(sdp_param_t));
        memset(&stRemoteSDPExParam, 0, sizeof(sdp_extend_param_t));

        i = SIP_GetSDPInfoEx(pRemoteSDP, &stRemoteSDPParam, &stRemoteSDPExParam);
        //printf_system_time();
        printf("record_invite_response_msg_proc() SIP_GetSDPInfoEx:i=%d\r\n", i);

        if (0 != i)
        {
            SystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "¼��ҵ��, INVITE��Ӧ��Ϣ����ʧ��:�߼��豸ID=%s, ԭ��=%s", pCrData->callee_id, (char*)"��ȡSDP�е���Ϣʧ��");
            EnSystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "INVITE response message process failed: logic deviceID=%s, cause=%s", pCrData->callee_id, (char*)"Access info in SDP failed");

            /* ��Ӧ��Ϣ������ */
            i = SIP_SendAck(ua_dialog_index);
            DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "record_invite_response_msg_proc() SIP_SendAck:caller_ua_index=%d, i=%d \r\n", ua_dialog_index, i);

            i = SIP_SendBye(ua_dialog_index);
            DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "record_invite_response_msg_proc() SIP_SendBye:caller_ua_index=%d, i=%d \r\n", ua_dialog_index, i);

            i = RemoveDeviceRecordInfo(pCrData->callee_id, pCrData->callee_stream_type, pCrData->tsu_resource_index);
            DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "record_invite_response_msg_proc() :callee_id=%s, callee_stream_type=%d, tsu_resource_index=%d, i=%d \r\n", pCrData->callee_id, pCrData->callee_stream_type, pCrData->tsu_resource_index, i);

            i = return_error_for_wait_answer_call_record(pCrData, EV9000_CMS_ERR_SDP_GENERAL_MSG_ERROR);

            i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
            i = call_record_remove(cr_pos);

            if (0 != i)
            {
                DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "record_invite_response_msg_proc() call_record_remove Error:cr_pos=%d, i=%d \r\n", cr_pos, i);
            }
            else
            {
                DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "record_invite_response_msg_proc() call_record_remove OK:cr_pos=%d, i=%d \r\n", cr_pos, i);
            }

            sdp_message_free(pRemoteSDP);
            pRemoteSDP = NULL;

            DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "record_invite_response_msg_proc() exit---: Get SDP Video Info Error \r\n");
            return -1;
        }

        DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "record_invite_response_msg_proc() addr=%s,port=%d,code=%d,flag=%d \r\n", stRemoteSDPParam.sdp_ip, stRemoteSDPParam.video_port, stRemoteSDPParam.video_code_type, stRemoteSDPParam.media_direction);
        //printf_system_time();
        printf("record_invite_response_msg_proc() addr=%s,port=%d,code=%d,flag=%d \r\n", stRemoteSDPParam.sdp_ip, stRemoteSDPParam.video_port, stRemoteSDPParam.video_code_type, stRemoteSDPParam.media_direction);

        /* �ж�ǰ���豸��ONVIF URL */
        if (stRemoteSDPExParam.onvif_url[0] != '\0')
        {
            if (strlen(stRemoteSDPExParam.onvif_url) < 255)
            {
                osip_strncpy(pCrData->callee_onvif_url, stRemoteSDPExParam.onvif_url, strlen(stRemoteSDPExParam.onvif_url));
            }
            else
            {
                osip_strncpy(pCrData->callee_onvif_url, stRemoteSDPExParam.onvif_url, 255);
            }

            DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "record_invite_response_msg_proc() callee_onvif_url=%s \r\n", pCrData->callee_onvif_url);

            /* ���в��Э�����͸�ΪRTSP */
            //pCrData->callee_transfer_type = TRANSFER_PROTOCOL_RTSP;
        }

        /* ��ӱ��е�SDP ��Ϣ */
        osip_strncpy(pCrData->callee_sdp_ip, stRemoteSDPParam.sdp_ip, MAX_IP_LEN);

        pCrData->callee_sdp_port = stRemoteSDPParam.video_port;
        pCrData->tsu_code = stRemoteSDPParam.video_code_type;

        /* ֪ͨTSU��ʼ¼��*/
        iRet = notify_tsu_add_record_task(pCrData, 1, pRecordInfo->record_type, pRecordInfo->record_days, pRecordInfo->bandwidth);
        //printf_system_time();
        printf("record_invite_response_msg_proc() notify_tsu_add_record_task:i=%d\r\n", i);

        if (iRet < 0)
        {
            DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "record_invite_response_msg_proc() notify_tsu_add_record_task Error: TSU IP=%s, iRet=%d \r\n", pCrData->tsu_ip, iRet);
            SystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "¼��ҵ��, INVITE��Ӧ��Ϣ����ʧ��:�߼��豸ID=%s, ԭ��=%s, TSU IP=%s, iRet=%d", pCrData->callee_id, (char*)"֪ͨTSU���¼������ʧ��", pCrData->tsu_ip, iRet);
            EnSystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "INVITE response message process failed: logic deviceID=%s, cause=%s, TSU IP=%s, iRet=%d", pCrData->callee_id, (char*)"Notify TSU to add video task failed", pCrData->tsu_ip, iRet);

            if (iRet == -2) /* TSU ¼��·���ﵽ���� */
            {
                i = SetTSUStatus(pCrData->tsu_resource_index, 2);
                DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "record_invite_response_msg_proc() SetTSUStatus:tsu_resource_index=%d, i=%d \r\n", pCrData->tsu_resource_index, i);
            }
            else if (iRet == -3) /* TSU û�л�ȡ��������������һ�� */
            {
                i = set_tsu_index_id2(pCrData->tsu_resource_index);
                DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "record_invite_response_msg_proc() set_tsu_index_id2:tsu_resource_index=%d, i=%d \r\n", pCrData->tsu_resource_index, i);
            }

            i = SIP_SendAck(ua_dialog_index);
            DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "record_invite_response_msg_proc() SIP_SendAck:caller_ua_index=%d, i=%d \r\n", ua_dialog_index, i);

            i = SIP_SendBye(ua_dialog_index);
            DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "record_invite_response_msg_proc() SIP_SendBye:caller_ua_index=%d, i=%d \r\n", ua_dialog_index, i);

            i = RemoveDeviceRecordInfo(pCrData->callee_id, pCrData->callee_stream_type, pCrData->tsu_resource_index);
            DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "record_invite_response_msg_proc() :callee_id=%s, callee_stream_type=%d, tsu_resource_index=%d, i=%d \r\n", pCrData->callee_id, pCrData->callee_stream_type, pCrData->tsu_resource_index, i);

            i = return_error_for_wait_answer_call_record(pCrData, EV9000_CMS_ERR_TSU_NOTIFY_ADD_TRANSFER_ERROR);

            i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
            i = call_record_remove(cr_pos);

            if (0 != i)
            {
                DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "record_invite_response_msg_proc() call_record_remove Error:cr_pos=%d, i=%d \r\n", cr_pos, i);
            }
            else
            {
                DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "record_invite_response_msg_proc() call_record_remove OK:cr_pos=%d, i=%d \r\n", cr_pos, i);
            }

            if (iRet == -2) /* ��¼����Ϣ��״̬����Ϊû�п��е�TSU */
            {
                pRecordInfo->record_status = RECORD_STATUS_NO_TSU;
            }

            sdp_message_free(pRemoteSDP);
            pRemoteSDP = NULL;
            return -1;
        }
        else
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "¼��ҵ��, INVITE��Ӧ��Ϣ����:�߼��豸ID=%s, ֪ͨTSU���¼������ɹ�, TSU IP=%s, task_id=%s, iRet=%d", pCrData->callee_id, pCrData->tsu_ip, pCrData->task_id, iRet);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Video business, INVITE response message processing: logical device ID = % s, notify the TSU add video mission success, TSU IP = % s, task_id = % s, iRet = % d", pCrData->callee_id, pCrData->tsu_ip, pCrData->task_id, iRet);
        }

        /* �������ȫ��¼����Ҫ������ͣ�����TSU */
        if (1 != pRecordInfo->TimeOfAllWeek)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "¼��ҵ��, INVITE��Ӧ��Ϣ����: ��¼����ʱ���¼��, ֪ͨTSU��ͣ¼��: ��λID=%s, TSU IP��ַ=%s, task_id=%s", pCrData->callee_id, pCrData->tsu_ip, pCrData->task_id);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "INVITE response message process: the video is time slot record video, notify TSU to pause video: point ID=%s, TSU IP address=%s, task_id=%s", pCrData->callee_id, pCrData->tsu_ip, pCrData->task_id);

            i = notify_tsu_pause_record(pCrData->tsu_ip, pCrData->task_id);

            if (0 != i)
            {
                DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "record_invite_response_msg_proc() notify_tsu_pause_record Error: tsu_ip=%s, task_id=%s, i=%d", pCrData->tsu_ip, pCrData->task_id, i);
                pRecordInfo->iTSUPauseStatus = 0;
            }
            else
            {
                DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "record_invite_response_msg_proc() notify_tsu_pause_record OK: tsu_ip=%s, task_id=%s, i=%d", pCrData->tsu_ip, pCrData->task_id, i);
                pRecordInfo->iTSUPauseStatus = 1;
            }

            /* ȥ�������еı�־λ */
            i = RemoveRecordTimeSchedMark(pRecordInfo->uID);
        }
        else
        {
            pRecordInfo->iTSUResumeStatus = 1;
        }

        /* ���������ACK ��Ϣ���� */
        if (ua_dialog_index == pCrData->callee_ua_index)
        {
            i = ack_send_use(cr_pos, -1, ua_dialog_index);
            printf("record_invite_response_msg_proc() ack_send_use:cr_pos=%d, callee_ua_index=%d \r\n", cr_pos, ua_dialog_index);
            DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "record_invite_response_msg_proc() ack_send_use:cr_pos=%d, callee_ua_index=%d \r\n", cr_pos, ua_dialog_index);
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "¼��ҵ��, INVITE��Ӧ��Ϣ����, ��ӵȴ�TSU֪ͨ���񴴽������Ϣ�������¼�, �ȴ�����ACK��Ϣ:cr_pos=%d, callee_ua_index=%d, iRet=%d", cr_pos, ua_dialog_index, i);
        }
        else
        {
            //i = SIP_SendAck(ua_dialog_index);
            DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "record_invite_response_msg_proc() not ack_send_use:cr_pos=%d, caller_ua_index=%d, callee_ua_index=%d, ua_dialog_index=%d \r\n", cr_pos, pCrData->caller_ua_index, pCrData->callee_ua_index, ua_dialog_index);
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "¼��ҵ��, INVITE��Ӧ��Ϣ����ʧ��, ԭ��=CrData��û���ҵ��յ���ua_dialog_index:cr_pos=%d, callee_ua_index=%d, ua_dialog_index=%d", cr_pos, pCrData->callee_ua_index, ua_dialog_index);

            i = SIP_SendAck(ua_dialog_index);
            DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "record_invite_response_msg_proc() SIP_SendAck:caller_ua_index=%d, i=%d \r\n", ua_dialog_index, i);

            i = SIP_SendBye(ua_dialog_index);
            DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "record_invite_response_msg_proc() SIP_SendBye:caller_ua_index=%d, i=%d \r\n", ua_dialog_index, i);

            /* ֪ͨTSUֹͣ��������*/
            i = notify_tsu_delete_record_task(pCrData->tsu_ip, pCrData->task_id);

            if (i < 0)
            {
                DEBUG_TRACE(MODULE_RECORD, LOG_WARN, "record_invite_response_msg_proc() notify_tsu_delete_record_task Error:tsu_ip=%s, task_id=%s, i=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
            }
            else
            {
                DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "record_invite_response_msg_proc() notify_tsu_delete_record_task OK:tsu_ip=%s, task_id=%s, i=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
            }

            i = RemoveDeviceRecordInfo(pCrData->callee_id, pCrData->callee_stream_type, pCrData->tsu_resource_index);
            DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "record_invite_response_msg_proc() :callee_id=%s, callee_stream_type=%d, tsu_resource_index=%d, i=%d \r\n", pCrData->callee_id, pCrData->callee_stream_type, pCrData->tsu_resource_index, i);

            /* ����TSU ״̬ */
            i = SetTSUStatus(pCrData->tsu_resource_index, 1);
            DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "record_invite_response_msg_proc() SetTSUStatus:tsu_resource_index=%d, i=%d \r\n", pCrData->tsu_resource_index, i);

            i = return_error_for_wait_answer_call_record(pCrData, EV9000_CMS_ERR_RECORD_FIND_RECORD_INFO_ERROR);

            i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
            i = call_record_remove(cr_pos);

            if (0 != i)
            {
                DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "record_invite_response_msg_proc() call_record_remove Error:cr_pos=%d, i=%d \r\n", cr_pos, i);
            }
            else
            {
                DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "record_invite_response_msg_proc() call_record_remove OK:cr_pos=%d, i=%d \r\n", cr_pos, i);
            }

            return -1;
        }

        pRecordInfo->record_status = RECORD_STATUS_COMPLETE;
        sdp_message_free(pRemoteSDP);
        pRemoteSDP = NULL;

        /* ֪ͨ�ȴ��ĺ������񣬽�������֪ͨTSUת������ */
        i = resumed_wait_answer_call_record2(pCrData, pRecordInfo->record_type);

        DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "record_invite_response_msg_proc() record_info_index=%d, device_index=%u, stream_type=%d, record_status=%d \r\n", record_info_index, pRecordInfo->stream_type, pRecordInfo->record_status);
        //printf_system_time();
        printf("record_invite_response_msg_proc() record_info_index=%d, device_index=%u, stream_type=%d, record_status=%d \r\n", record_info_index, pRecordInfo->stream_type, pRecordInfo->record_status);

        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "¼��ҵ��, INVITE��Ӧ��Ϣ����ɹ�:�߼��豸ID=%s, IP��ַ=%s, cr_pos=%d", pCrData->callee_id, pCrData->callee_ip, cr_pos);
    }
    else
    {
        SystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "¼��ҵ��, INVITE��Ӧ��Ϣ����ʧ��:�߼��豸ID=%s, ԭ��=%s, ������=%d", pCrData->callee_id, (char*)"��200�Ĵ�����Ӧ��Ϣ", response_code);
        EnSystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "INVITE response message process failed: logic deviceID=%s, cause=%s, error code=%d", pCrData->callee_id, (char*)"non 200 fault response messageϢ", response_code);

        i = RemoveDeviceRecordInfo(pCrData->callee_id, pCrData->callee_stream_type, pCrData->tsu_resource_index);
        DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "record_invite_response_msg_proc() :callee_id=%s, callee_stream_type=%d, tsu_resource_index=%d, i=%d \r\n", pCrData->callee_id, pCrData->callee_stream_type, pCrData->tsu_resource_index, i);

        i = return_error_for_wait_answer_call_record(pCrData, response_code);

        i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
        i = call_record_remove(cr_pos);

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "record_invite_response_msg_proc() call_record_remove Error:cr_pos=%d, i=%d \r\n", cr_pos, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "record_invite_response_msg_proc() call_record_remove OK:cr_pos=%d, i=%d \r\n", cr_pos, i);
        }

        /* �����408�Ĵ��󣬿������粻ͨ�����ʱ�򽫵�λ����Ϊ���� */
        if (408 == response_code)
        {
            pCalleeGBLogicDeviceInfo = GBLogicDevice_info_find(pCrData->callee_id);

            if (NULL != pCalleeGBLogicDeviceInfo)
            {
                pCalleeGBLogicDeviceInfo->status = 3;
                SystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "�߼��豸ID=%s, ����=%s, ������ʱ���ɴ�", pCalleeGBLogicDeviceInfo->device_id, pCalleeGBLogicDeviceInfo->device_name);
                EnSystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "Logical device ID=%s, name =%s, the network is temporarily not reachable", pCalleeGBLogicDeviceInfo->device_id, pCalleeGBLogicDeviceInfo->device_name);

                DEBUG_TRACE(MODULE_RECORD, LOG_WARN, "record_invite_response_msg_proc() device_id=%s, NetWork Unreached \r\n", pCalleeGBLogicDeviceInfo->device_id);
            }
        }

        DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "record_invite_response_msg_proc() response_code=%d \r\n", response_code);
        return -1;
    }

    DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "record_invite_response_msg_proc() End--- :callee_id=%s, callee_ip=%s \r\n", pCrData->callee_id, pCrData->callee_ip);
    //printf_system_time();
    printf("record_invite_response_msg_proc() Exit--- :callee_id=%s, callee_ip=%s \r\n", pCrData->callee_id, pCrData->callee_ip);
    return 0;
}

/*****************************************************************************
 �� �� ��  : record_bye_msg_proc
 ��������  : ¼��BYE ��Ϣ����
 �������  : char* caller_id
                            char* callee_id
                            char* call_id
                            int ua_dialog_index
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��17��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int record_bye_msg_proc(int cr_pos, int ua_dialog_index)
{
    int i = 0;
    cr_t* pCrData = NULL;

    /* ���Һ��м�¼��Ϣ */
    if (cr_pos < 0)
    {
        SIP_AnswerToBye(ua_dialog_index, 481, NULL);
        DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "record_bye_msg_proc() exit---: Find Record Srv Error \r\n");
        return -1;
    }

    pCrData = call_record_get(cr_pos);

    if (NULL == pCrData)
    {
        SIP_AnswerToBye(ua_dialog_index, 481, NULL);
        DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "record_bye_msg_proc() exit---: Get Record Srv Error:cr_pos=%d \r\n", cr_pos);
        return -1;
    }

    DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "record_bye_msg_proc():callee_id=%s, callee_ip=%s \r\n", pCrData->callee_id, pCrData->callee_ip);
    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "¼��ҵ��, �յ�ǰ��BYE��Ϣ����, ¼����Ƶ�ر�:�߼��豸ID=%s, IP��ַ=%s", pCrData->callee_id, pCrData->callee_ip);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Video service, received a front-end BYE message processing, video video off: logic device ID=%s, IP address =%s", pCrData->callee_id, pCrData->callee_ip);

    /* ��Ӧ200 */
    SIP_AnswerToBye(ua_dialog_index, 200, NULL);

    /* ֪ͨTSUֹͣ��������*/
    i = notify_tsu_delete_record_task(pCrData->tsu_ip, pCrData->task_id);

    if (i < 0)
    {
        DEBUG_TRACE(MODULE_RECORD, LOG_WARN, "record_bye_msg_proc() notify_tsu_delete_record_task Error:tsu_ip=%s, task_id=%s, i=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
    }
    else
    {
        DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "record_bye_msg_proc() notify_tsu_delete_record_task OK:tsu_ip=%s, task_id=%s, i=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
    }

    /* ����Bye ����������ʹ��¼��ҵ������в��û�*/
    i = send_bye_to_all_other_caller_by_callee_id_and_streamtype(pCrData->callee_id, pCrData->callee_stream_type, cr_pos);
    DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "record_bye_msg_proc() send_bye_to_all_other_caller_by_callee_id_and_streamtype:callee_id=%s, callee_stream_type=%d, i=%d \r\n", pCrData->callee_id, pCrData->callee_stream_type, i);

    i = RemoveDeviceRecordInfo(pCrData->callee_id, pCrData->callee_stream_type, pCrData->tsu_resource_index);
    DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "record_bye_msg_proc() :callee_id=%s, callee_stream_type=%d, tsu_resource_index=%d, i=%d \r\n", pCrData->callee_id, pCrData->callee_stream_type, pCrData->tsu_resource_index, i);

    /* ����TSU ״̬ */
    i = SetTSUStatus(pCrData->tsu_resource_index, 1);
    DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "record_bye_msg_proc() SetTSUStatus:tsu_resource_index=%d, i=%d \r\n", pCrData->tsu_resource_index, i);

    i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
    i = call_record_remove(cr_pos);

    if (0 != i)
    {
        DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "record_bye_msg_proc() call_record_remove Error:cr_pos=%d, i=%d \r\n", cr_pos, i);
    }
    else
    {
        DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "record_bye_msg_proc() call_record_remove OK:cr_pos=%d, i=%d \r\n", cr_pos, i);
    }

    return 0;
}

/*****************************************************************************
 �� �� ��  : record_bye_response_msg_proc
 ��������  : ¼��BYE ��Ӧ��Ϣ����
 �������  : char* caller_id
                            char* callee_id
                            char* call_id
                            int ua_dialog_index
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��17��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int record_bye_response_msg_proc(int cr_pos, int ua_dialog_index, int response_code)
{
    int i = 0;
    cr_t* pCrData = NULL;

    if (cr_pos < 0)
    {
        DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "record_bye_response_msg_proc() exit---: Param Error \r\n");
        return -1;
    }

    pCrData = call_record_get(cr_pos);

    if (NULL == pCrData)
    {
        DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "record_bye_response_msg_proc() exit---: Get Call Record Error:cr_pos=%d \r\n", cr_pos);
        return -1;
    }

    DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "record_bye_response_msg_proc():callee_id=%s, callee_ip=%s \r\n", pCrData->callee_id, pCrData->callee_ip);
    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "¼��ҵ��, �յ�BYE��Ӧ��Ϣ����, ¼����Ƶ�ر�:�߼��豸ID=%s, IP��ַ=%s, cr_pos=%d", pCrData->callee_id, pCrData->callee_ip, cr_pos);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Record business, received the BYE response message processing, video video closed: logical device ID = % s, IP address = % s, cr_pos = % d", pCrData->callee_id, pCrData->callee_ip, cr_pos);

    if (pCrData->callee_ua_index == ua_dialog_index)    /* ���з��͵�Bye */
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "¼��ҵ��, �յ�����BYE��Ӧ��Ϣ����, ¼����Ƶ�ر�:�߼��豸ID=%s, cr_pos=%d", pCrData->callee_id, cr_pos);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Record business, received calling BYE response message processing, video video closed: logical device ID = %s, cr_pos=%d", pCrData->callee_id, cr_pos);

        if (pCrData->caller_ua_index >= 0)
        {
            SIP_AnswerToBye(pCrData->caller_ua_index, response_code, NULL);
        }
    }
    else if (pCrData->caller_ua_index == ua_dialog_index) /* ���з��͵�Bye */
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "¼��ҵ��, �յ�ǰ�˽�BYE��Ӧ��Ϣ����, ¼����Ƶ�ر�:�߼��豸ID=%s, cr_pos=%d", pCrData->callee_id, cr_pos);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Record business, receive front-end call BYE response message processing, video video closed: logical device ID = % s, cr_pos = %d", pCrData->callee_id, cr_pos);

        if (pCrData->callee_ua_index >= 0)
        {
            SIP_AnswerToBye(pCrData->callee_ua_index, response_code, NULL);
        }
    }

    /* �Ƴ����м�¼��Ϣ */
    i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
    i = call_record_remove(cr_pos);

    if (0 != i)
    {
        DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "record_bye_response_msg_proc() call_record_remove Error:cr_pos=%d, i=%d \r\n", cr_pos, i);
    }
    else
    {
        DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "record_bye_response_msg_proc() call_record_remove OK:cr_pos=%d, i=%d \r\n", cr_pos, i);
    }

    return 0;
}

/*****************************************************************************
 �� �� ��  : StartDeviceRecord
 ��������  : �����豸¼������
 �������  : int device_index
                            int storagelife
                            int bandwidth
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��5��29�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int StartDeviceRecord(record_info_t* pProcRecordInfo)
{
    int i = 0;
    int tsu_index = -1;
    int index = 0;
    int cr_pos = -1;
    int record_info_index = -1;
    GBLogicDevice_info_t* pGBLogicDeviceInfo = NULL;
    GBDevice_info_t* pGBDeviceInfo = NULL;
    char* device_ip = NULL;
    int device_port = 0;
    char* sdp_ssrc = NULL;
    sdp_message_t* local_sdp = NULL;
    sdp_param_t sdp_param;
    int recv_port = 0;
    tsu_resource_info_t* pTsuResourceInfo = NULL;
    char* tsu_ip = NULL;
    cr_t* pCrData = NULL;
    int has_record_pos = -1;
    record_info_t* pHasRecordInfo = NULL;
    record_info_t* pRecordInfo = NULL;
    char* sdp_tsu_ip = NULL;

    if (NULL == pProcRecordInfo)
    {
        DEBUG_TRACE(MODULE_RECORD, LOG_DEBUG, "StartDeviceRecord() exit---: Param Error \r\n");
        return -1;
    }

    printf("\r\nStartDeviceRecord() Start---: device_index=%u \r\n", pProcRecordInfo->device_index);

    /* ���������鿴�Ƿ��Ѿ���ǰ�˽����¼���� */
    has_record_pos = has_record_info_find_by_stream_type(pProcRecordInfo->device_index, pProcRecordInfo->stream_type, pProcRecordInfo->record_type);

    if (has_record_pos >= 0) /* ͬһ���������ͣ���ͬ��¼�����ͣ���Ҫ���¶�����ԣ������ٴ�ǰ��ȡ�� */
    {
        pHasRecordInfo = record_info_get(has_record_pos);

        if (NULL != pHasRecordInfo)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "������¼���������Ѿ���¼��:�߼��豸Index=%u, ��������=%d, ¼������=%d, ��¼���¼������=%d", pHasRecordInfo->device_index, pHasRecordInfo->stream_type, pHasRecordInfo->record_type, pHasRecordInfo->record_type);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Start the record stream type has been in the video: logical device Index = % u, code flow type = % d, video type = % d, have video video type = %d", pHasRecordInfo->device_index, pHasRecordInfo->stream_type, pHasRecordInfo->record_type, pHasRecordInfo->record_type);
            DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "StartDeviceRecord() Device Has Recording Proc:device_index=%u,stream_type=%d,record_type=%d, has_record_type=%d  \r\n", pHasRecordInfo->device_index, pHasRecordInfo->stream_type, pHasRecordInfo->record_type, pHasRecordInfo->record_type);

            if (RECORD_STATUS_INIT == pHasRecordInfo->record_status)
            {
                SystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "������¼���������Ѿ���¼��¼��û�гɹ�:�߼��豸Index=%u, ��������=%d, ¼������=%d, ��¼���¼������=%d", pHasRecordInfo->device_index, pHasRecordInfo->stream_type, pHasRecordInfo->record_type, pHasRecordInfo->record_type);
                EnSystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "Video streaming has been started. The video was not successful: the logic device Index=%u, stream type =%d, video type =%d, recorded video type =%d", pHasRecordInfo->device_index, pHasRecordInfo->stream_type, pHasRecordInfo->record_type, pHasRecordInfo->record_type);

                DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "StartDeviceRecord() exit---: Device Has Recording Proc:device_index=%u,stream_type=%d,record_type=%d, has_record_type=%d  \r\n", pHasRecordInfo->device_index, pHasRecordInfo->stream_type, pHasRecordInfo->record_type, pHasRecordInfo->record_type);
                return -1;
            }
            else if (RECORD_STATUS_PROC == pHasRecordInfo->record_status)
            {
                SystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "������¼���������Ѿ���¼��¼�����̻�û����:�߼��豸Index=%u, ��������=%d, ¼������=%d, ��¼���¼������=%d", pHasRecordInfo->device_index, pHasRecordInfo->stream_type, pHasRecordInfo->record_type, pHasRecordInfo->record_type);
                EnSystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "Video streaming has been started, The video is not over yet: the logic device Index=%u,  stream type =%d,  video type =%d, recorded video type =%d", pHasRecordInfo->device_index, pHasRecordInfo->stream_type, pHasRecordInfo->record_type, pHasRecordInfo->record_type);

                DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "StartDeviceRecord() exit---: Device Has Recording Proc:device_index=%u,stream_type=%d,record_type=%d, has_record_type=%d  \r\n", pHasRecordInfo->device_index, pHasRecordInfo->stream_type, pHasRecordInfo->record_type, pHasRecordInfo->record_type);
                return -1;
            }
            else
            {
                pProcRecordInfo->record_cr_index = pHasRecordInfo->record_cr_index;
                pProcRecordInfo->record_status = pHasRecordInfo->record_status;
                pProcRecordInfo->tsu_index = pHasRecordInfo->tsu_index;

                SystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_WARNING, "������¼���������Ѿ���¼��:�߼��豸Index=%u, ��������=%d, ¼������=%d, ��¼���¼������=%d", pProcRecordInfo->device_index, pProcRecordInfo->stream_type, pProcRecordInfo->record_type, pHasRecordInfo->record_type);
                EnSystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_WARNING, "Video streaming has been launched in the video: logic device Index=%u, stream type =%d, video type =%d, recorded video type=%d", pProcRecordInfo->device_index, pProcRecordInfo->stream_type, pProcRecordInfo->record_type, pHasRecordInfo->record_type);

                DEBUG_TRACE(MODULE_RECORD, LOG_WARN, "StartDeviceRecord() exit---: Device Has Recording:device_index=%u,stream_type=%d,record_type=%d, has_record_type=%d  \r\n", pProcRecordInfo->device_index, pProcRecordInfo->stream_type, pProcRecordInfo->record_type, pHasRecordInfo->record_type);
                return pProcRecordInfo->record_cr_index;
            }
        }
    }

    printf("\r\nStartDeviceRecord() has_record_info_find_by_stream_type End--- \r\n");

    /* �����߼��豸��Ϣ */
    pGBLogicDeviceInfo = GBLogicDevice_info_find_by_device_index(pProcRecordInfo->device_index);   /* �߼��豸�� */

    if (NULL == pGBLogicDeviceInfo)
    {
        if (pProcRecordInfo->record_try_count >= 3)
        {
            SystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "����¼��ʧ��:�߼��豸Index=%d��ԭ��=%s", pProcRecordInfo->device_index, (char*)"��ȡ�߼��豸���߼��豸��Ϣʧ��");
            EnSystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "Start video recording failed: logic device ID =%s, cause=%s", pProcRecordInfo->device_index, (char*)"Access logic device info of logic device failed.");
        }

        DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "StartDeviceRecord() exit---: GBLogicDevice_info_find_by_device_index Error:device_index=%u \r\n", pProcRecordInfo->device_index);
        return -1;
    }

    printf("\r\nStartDeviceRecord() GBLogicDevice_info_find_by_device_index End--- \r\n");

#if 0

    /* �ж��߼��豸��¼������ */
    if (1 == pGBLogicDeviceInfo->record_type)
    {
        if (pProcRecordInfo->record_try_count >= 3)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "����¼��ʧ��:�߼��豸Index=%d��ԭ��=%s", pProcRecordInfo->device_index, (char*)"�߼��豸����Ϊǰ��¼��");
        }

        DEBUG_TRACE(MODULE_RECORD, LOG_DEBUG, "StartDeviceRecord() exit---: GBLogicDevice Not Config Record In CMS:device_index=%d \r\n", pProcRecordInfo->device_index);
        return -1;
    }

#endif

    //DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "StartDeviceRecord() device_id=%s \r\n", pGBLogicDeviceInfo->device_id);
    //printf("StartDeviceRecord() device_id=%s \r\n", pGBLogicDeviceInfo->device_id);
    //LogRunTraceToFile("��ʼ����¼��:�߼��豸ID=%s\r\n", pGBLogicDeviceInfo->device_id);

    if (0 == pGBLogicDeviceInfo->status)
    {
        if (pProcRecordInfo->record_try_count >= 3)
        {
            SystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "����¼��ʧ��:�߼��豸ID=%s��ԭ��=%s", pGBLogicDeviceInfo->device_id, (char*)"�߼��豸������");
            EnSystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "Start video recording failed: logic device ID =%s, cause=%s", pGBLogicDeviceInfo->device_id, (char*)"Logic device is not online.");
        }

        DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "StartDeviceRecord() exit---: GBlogic Device Not Online \r\n");
        return -2;
    }
    else if (2 == pGBLogicDeviceInfo->status)
    {
        if (pProcRecordInfo->record_try_count >= 3)
        {
            SystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "����¼��ʧ��:�߼��豸ID=%s��ԭ��=%s", pGBLogicDeviceInfo->device_id, (char*)"�߼��豸û����Ƶ��");
            EnSystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "Start video recording failed: logic device ID =%s, cause=%s", pGBLogicDeviceInfo->device_id, (char*)"Logic device do not have video stream.");
        }

        DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "StartDeviceRecord() exit---: GBlogic Device No Stream \r\n");
        return -3;
    }
    else if (3 == pGBLogicDeviceInfo->status)
    {
        if (pProcRecordInfo->record_try_count >= 3)
        {
            SystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "����¼��ʧ��:�߼��豸ID=%s��ԭ��=%s", pGBLogicDeviceInfo->device_id, (char*)"�߼��豸���粻�ɴ�");
            EnSystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "Start video recording failed: logic device ID =%s, cause=%s", pGBLogicDeviceInfo->device_id, (char*)"Logic device network is not accessble");
        }

        DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "StartDeviceRecord() exit---: GBlogic Device Unreached \r\n");
        return -5;
    }

    if (pProcRecordInfo->stream_type == EV9000_STREAM_TYPE_SLAVE && pGBLogicDeviceInfo->stream_count == 1)
    {
        if (pProcRecordInfo->record_try_count >= 3)
        {
            SystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "����¼��ʧ��:�߼��豸ID=%s��ԭ��=%s", pGBLogicDeviceInfo->device_id, (char*)"�߼��豸��֧�ֶ�����");
            EnSystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "Start video recording failed: logic device ID =%s, cause=%s", pGBLogicDeviceInfo->device_id, (char*)"Logic device do not support mult stream");
        }

        DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "StartDeviceRecord() exit---: GBlogic Device Not Support Muilt Stream \r\n");
        return -6;
    }

    /* ��ȡ�����豸 */
    pGBDeviceInfo = GBDevice_info_get_by_stream_type(pGBLogicDeviceInfo, pProcRecordInfo->stream_type);

    if (NULL == pGBDeviceInfo)
    {
        if (pProcRecordInfo->record_try_count >= 3)
        {
            SystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "����¼��ʧ��:�߼��豸ID=%s��ԭ��=%s, ý��������=%d", pGBLogicDeviceInfo->device_id, (char*)"��ȡ�߼��豸�����豸����", pProcRecordInfo->stream_type);
            EnSystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "Start video recording failed: logic device ID =%s, cause=%s, media straeam type=%d", pGBLogicDeviceInfo->device_id, (char*)"Access logic device physical device failed", pProcRecordInfo->stream_type);
        }

        DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "StartDeviceRecord() exit---: Get Device Port Error \r\n");
        return -1;
    }

    printf("\r\nStartDeviceRecord() GBDevice_info_get_by_stream_type End--- \r\n");

    device_ip = pGBDeviceInfo->login_ip;
    device_port = pGBDeviceInfo->login_port;

    /* ���������Դ*/
    cr_pos = call_record_add();
    DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "StartDeviceRecord() call_record_add:device_id=%s, cr_pos=%d \r\n", pGBLogicDeviceInfo->device_id, cr_pos);

    if (cr_pos < 0)
    {
        SystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "����¼��ʧ��:�߼��豸ID=%s��ԭ��=%s", pGBLogicDeviceInfo->device_id, (char*)"��ȡ���õĺ�����Դʧ��");
        EnSystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "Start video recording failed: logic device ID =%s, cause=%s", pGBLogicDeviceInfo->device_id, (char*)"Access to available call resource failed");

        DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "StartDeviceRecord() exit---: Call Record Add Error \r\n");
        return -1;
    }

    pCrData = call_record_get(cr_pos);

    if (pCrData == NULL)
    {
        SystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "����¼��ʧ��:�߼��豸ID=%s��ԭ��=%s", pGBLogicDeviceInfo->device_id, (char*)"��ȡ���õĺ�����Դʧ��");
        EnSystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "Start video recording failed: logic device ID =%s, cause=%s", pGBLogicDeviceInfo->device_id, (char*)"Access to available call resource failure");

        DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "StartDeviceRecord() exit---: Get Transfer Info Error \r\n");
        return -1;
    }

    printf("\r\nStartDeviceRecord() call_record_add End--- \r\n");

    record_info_index = record_info_find_by_cr_index(cr_pos);  /*  �������cr_pos�Ƿ�ռ���ˣ����ռ���ˣ��ͷŵ� */

    if (record_info_index >= 0)
    {
        pRecordInfo = record_info_get(record_info_index);

        if (NULL != pRecordInfo)
        {
            pRecordInfo->tsu_index = -1;
            pRecordInfo->record_cr_index = -1;
            pRecordInfo->record_status = RECORD_STATUS_INIT;
            pRecordInfo->record_start_time = 0;
            pRecordInfo->record_try_count = 0;
            pRecordInfo->record_retry_interval = 5;
            pRecordInfo->iTSUPauseStatus = 0;
            pRecordInfo->iTSUResumeStatus = 0;
            pRecordInfo->iTSUAlarmRecordStatus = 0;
            //printf("StartDeviceRecord() get_idle_tsu_by_resource_balance_for_record: RecordInfo: device_index=%u, record_type=%d, stream_type=%d tsu_index reset \r\n", pRecordInfo->device_index, pRecordInfo->record_type, pRecordInfo->stream_type);
        }
    }

    printf("\r\nStartDeviceRecord() record_info_find_by_cr_index End--- \r\n");

    /* ���ԭ����ҵ����ֹͣ�� */
    if (pProcRecordInfo->record_cr_index >= 0)
    {
        i = StopDeviceRecord(pProcRecordInfo->record_cr_index);
        DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "StartDeviceRecord() StopDeviceRecord: record_cr_index=%d, i=%d \r\n", pProcRecordInfo->record_cr_index, i);
    }

    pCrData->call_type = CALL_TYPE_RECORD;

    if (pProcRecordInfo->assign_record)
    {
        tsu_index = tsu_resource_info_find_by_index(pProcRecordInfo->assign_tsu_index);
        DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "StartDeviceRecord() tsu_resource_info_find_by_index: assign_tsu_index=%d, tsu_index=%d \r\n", pProcRecordInfo->assign_tsu_index, tsu_index);
    }
    else
    {
        /* ��ȡ���е�TSU��Դ */
        DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "StartDeviceRecord() get_idle_tsu_by_resource_balance_for_record: Begin--- \r\n", tsu_index);
        tsu_index = get_idle_tsu_by_resource_balance_for_record(pProcRecordInfo->record_days, pProcRecordInfo->bandwidth);
        DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "StartDeviceRecord() get_idle_tsu_by_resource_balance_for_record: End--- tsu_index=%d \r\n", tsu_index);
    }

    if (tsu_index < 0)
    {
        if (-2 == tsu_index)
        {
            SystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "����¼��ʧ��:�߼��豸ID=%s��ԭ��=%s", pGBLogicDeviceInfo->device_id, (char*)"���ҿ��е�¼��TSU��Դ����ʧ��,TSU��Դ����Ϊ��");
            EnSystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "Start video recording failed: logic device ID =%s, cause=%s", pGBLogicDeviceInfo->device_id, (char*)"Access to available TSU resource failed");
        }
        else if (-3 == tsu_index)
        {
            SystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "����¼��ʧ��:�߼��豸ID=%s��ԭ��=%s", pGBLogicDeviceInfo->device_id, (char*)"���ҿ��е�¼��TSU��Դ����ʧ��,TSU��Դ��Ϣ����");
            EnSystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "Start video recording failed: logic device ID =%s, cause=%s", pGBLogicDeviceInfo->device_id, (char*)"Access to available TSU resource failed");
        }
        else if (-4 == tsu_index)
        {
            SystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "����¼��ʧ��:�߼��豸ID=%s��ԭ��=%s", pGBLogicDeviceInfo->device_id, (char*)"���ҿ��е�¼��TSU��Դ����ʧ��,TSU��Դ��û������");
            EnSystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "Start video recording failed: logic device ID =%s, cause=%s", pGBLogicDeviceInfo->device_id, (char*)"Access to available TSU resource failed");
        }
        else if (-5 == tsu_index)
        {
            SystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "����¼��ʧ��:�߼��豸ID=%s��ԭ��=%s", pGBLogicDeviceInfo->device_id, (char*)"���ҿ��е�¼��TSU��Դ����ʧ��,TSU��Դ��������");
            EnSystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "Start video recording failed: logic device ID =%s, cause=%s", pGBLogicDeviceInfo->device_id, (char*)"Access to available TSU resource failed");
        }
        else if (-6 == tsu_index)
        {
            SystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "����¼��ʧ��:�߼��豸ID=%s��ԭ��=%s", pGBLogicDeviceInfo->device_id, (char*)"���ҿ��е�¼��TSU��Դ����ʧ��,TSU��Դ������ָ��¼��");
            EnSystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "Start video recording failed: logic device ID =%s, cause=%s", pGBLogicDeviceInfo->device_id, (char*)"Access to available TSU resource failed");
        }
        else if (-7 == tsu_index)
        {
            SystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "����¼��ʧ��:�߼��豸ID=%s��ԭ��=%s", pGBLogicDeviceInfo->device_id, (char*)"���ҿ��е�¼��TSU��Դ����ʧ��,TSU��Դ���Ѿ��ﵽ����");
            EnSystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "Start video recording failed: logic device ID =%s, cause=%s", pGBLogicDeviceInfo->device_id, (char*)"Access to available TSU resource failed");
        }
        else if (-8 == tsu_index)
        {
            SystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "����¼��ʧ��:�߼��豸ID=%s��ԭ��=%s", pGBLogicDeviceInfo->device_id, (char*)"���ҿ��е�¼��TSU��Դ����ʧ��,TSU��Դ��û�й��ش���");
            EnSystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "Start video recording failed: logic device ID =%s, cause=%s", pGBLogicDeviceInfo->device_id, (char*)"Access to available TSU resource failed");
        }
        else if (-9 == tsu_index)
        {
            SystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "����¼��ʧ��:�߼��豸ID=%s��ԭ��=%s", pGBLogicDeviceInfo->device_id, (char*)"���ҿ��е�¼��TSU��Դ����ʧ��,ͨ��ICE��ȡ���е�TSU��Դ״̬ʧ��");
            EnSystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "Start video recording failed: logic device ID =%s, cause=%s", pGBLogicDeviceInfo->device_id, (char*)"Access to available TSU resource failed");
        }
        else if (-10 == tsu_index)
        {
            SystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "����¼��ʧ��:�߼��豸ID=%s��ԭ��=%s", pGBLogicDeviceInfo->device_id, (char*)"���ҿ��е�¼��TSU��Դ����ʧ��,TSU��Դ��û������ָ��¼��");
            EnSystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "Start video recording failed: logic device ID =%s, cause=%s", pGBLogicDeviceInfo->device_id, (char*)"Access to available TSU resource failed");
        }
        else
        {
            SystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "����¼��ʧ��:�߼��豸ID=%s��ԭ��=%s", pGBLogicDeviceInfo->device_id, (char*)"���ҿ��е�¼��TSU��Դ����ʧ��");
            EnSystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "Start video recording failed: logic device ID =%s, cause=%s", pGBLogicDeviceInfo->device_id, (char*)"Access to available TSU resource failed");
        }

        i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
        i = call_record_remove(cr_pos);

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "StartDeviceRecord() call_record_remove Error:cr_pos=%d, i=%d \r\n", cr_pos, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "StartDeviceRecord() call_record_remove OK:cr_pos=%d, i=%d \r\n", cr_pos, i);
        }

        printf("\r\nStartDeviceRecord() exit---: Find Idle TSU Error \r\n");
        return -4;
    }

    printf("\r\nStartDeviceRecord() get_idle_tsu_by_resource_balance_for_record End--- \r\n");

    pProcRecordInfo->tsu_index = tsu_index;

    pTsuResourceInfo = tsu_resource_info_get(tsu_index);

    if (NULL == pTsuResourceInfo)
    {
        SystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "����¼��ʧ��:�߼��豸ID=%s��ԭ��=%s, tsu_index=%d", pGBLogicDeviceInfo->device_id, (char*)"��ȡ¼��TSU��Դʧ��", tsu_index);
        EnSystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "Start video recording failed: logic device ID =%s, cause=%s", pGBLogicDeviceInfo->device_id, (char*)"Access to available TSU resource failed");

        i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
        i = call_record_remove(cr_pos);

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "StartDeviceRecord() call_record_remove Error:cr_pos=%d, i=%d \r\n", cr_pos, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "StartDeviceRecord() call_record_remove OK:cr_pos=%d, i=%d \r\n", cr_pos, i);
        }

        printf("\r\nStartDeviceRecord() exit---: tsu_resource_info_get Error \r\n");
        return -4;
    }

    printf("\r\nStartDeviceRecord() tsu_resource_info_get End--- \r\n");

    if (0 == pTsuResourceInfo->iStatus)
    {
        SystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "����¼��ʧ��:�߼��豸ID=%s��ԭ��=%s, TSU ID=%s", pGBLogicDeviceInfo->device_id, (char*)"TSU������", pTsuResourceInfo->tsu_device_id);
        EnSystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "Start video recording failed: logic device ID =%s, cause=%s, TSU ID=%s", pGBLogicDeviceInfo->device_id, (char*)"TSU not online", pTsuResourceInfo->tsu_device_id);

        i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
        i = call_record_remove(cr_pos);

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "StartDeviceRecord() call_record_remove Error:cr_pos=%d, i=%d \r\n", cr_pos, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "StartDeviceRecord() call_record_remove OK:cr_pos=%d, i=%d \r\n", cr_pos, i);
        }

        printf("\r\nStartDeviceRecord() exit---: TSU Status Error 0 \r\n");
        return -4;
    }

    if (2 == pTsuResourceInfo->iStatus)
    {
        SystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "����¼��ʧ��:�߼��豸ID=%s��ԭ��=%s, TSU ID=%s", pGBLogicDeviceInfo->device_id, (char*)"TSU��Դ�ﵽ����", pTsuResourceInfo->tsu_device_id);
        EnSystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "Start video recording failed: logic device ID =%s, cause=%s, TSU ID=%s", pGBLogicDeviceInfo->device_id, (char*)"TSU resource reach upper limit", pTsuResourceInfo->tsu_device_id);

        i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
        i = call_record_remove(cr_pos);

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "StartDeviceRecord() call_record_remove Error:cr_pos=%d, i=%d \r\n", cr_pos, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "StartDeviceRecord() call_record_remove OK:cr_pos=%d, i=%d \r\n", cr_pos, i);
        }

        printf("\r\nStartDeviceRecord() exit---: TSU Status Error 2 \r\n");
        return -4;
    }

    if (pProcRecordInfo->assign_record && pTsuResourceInfo->iTsuType != 1)
    {
        SystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "����¼��ʧ��:�߼��豸ID=%s��ԭ��=%s", pGBLogicDeviceInfo->device_id, (char*)"ָ��¼���λ��ָ��TSUû������Ϊָ��¼��TSU");
        EnSystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "Start video recording failed: logic device ID =%s, cause=%s", pGBLogicDeviceInfo->device_id, (char*)"The specified TSU is not set to the specified video TSU");

        i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
        i = call_record_remove(cr_pos);

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "StartDeviceRecord() call_record_remove Error:cr_pos=%d, i=%d \r\n", cr_pos, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "StartDeviceRecord() call_record_remove OK:cr_pos=%d, i=%d \r\n", cr_pos, i);
        }

        printf("\r\nStartDeviceRecord() exit---: TSU assign_record \r\n");
        return -4;
    }

    /* ��ȡ��TSUͨ�ŵ�IP��ַ */
    tsu_ip = get_tsu_ip(pTsuResourceInfo, default_eth_name_get());

    if (NULL == tsu_ip)
    {
        SystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "����¼��ʧ��:�߼��豸ID=%s��ԭ��=%s", pGBLogicDeviceInfo->device_id, (char*)"��ȡTSU IP��ַʧ��");
        EnSystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "Start video recording failed: logic device ID =%s, cause=%s", pGBLogicDeviceInfo->device_id, (char*)"Access TSU IP address failed");

        i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
        i = call_record_remove(cr_pos);

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "StartDeviceRecord() call_record_remove Error:cr_pos=%d, i=%d \r\n", cr_pos, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "StartDeviceRecord() call_record_remove OK:cr_pos=%d, i=%d \r\n", cr_pos, i);
        }

        printf("\r\nStartDeviceRecord() exit---: get_tsu_ip Error \r\n");
        return -4;
    }

    printf("\r\nStartDeviceRecord() get_tsu_ip End--- \r\n");

    /* ��ȡTSU ���ն˿ں� */
    recv_port = get_recv_port_by_tsu_resource(tsu_ip);

    if (recv_port <= 0)
    {
        SystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "����¼��ʧ��:�߼��豸ID=%s��ԭ��=%s, tsu_ip=%s", pGBLogicDeviceInfo->device_id, (char*)"��ȡ���õ�TSU�ʽ��ն˿�ʧ��", tsu_ip);
        EnSystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "Start video recording failed: logic device ID =%s, cause=%s, tsu_ip=%s", pGBLogicDeviceInfo->device_id, (char*)"Access to available TSU resource receive port failed", tsu_ip);

        i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
        i = call_record_remove(cr_pos);

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "StartDeviceRecord() call_record_remove Error:cr_pos=%d, i=%d \r\n", cr_pos, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "StartDeviceRecord() call_record_remove OK:cr_pos=%d, i=%d \r\n", cr_pos, i);
        }

        printf("\r\nStartDeviceRecord() exit---: get_recv_port_by_tsu_resource Error \r\n");
        return -4;
    }

    printf("\r\nStartDeviceRecord() get_recv_port_by_tsu_resource End--- \r\n");

    /* ���ת����Ϣ */
    /* TSU��ת����Դ */
    osip_strncpy(pCrData->tsu_device_id, pTsuResourceInfo->tsu_device_id, MAX_ID_LEN);
    pCrData->tsu_resource_index = tsu_index;
    pCrData->tsu_recv_port = recv_port;
    osip_strncpy(pCrData->tsu_ip, tsu_ip, MAX_IP_LEN);

    i = TSUResourceIPAddrListClone(pTsuResourceInfo->pTSUIPAddrList, pCrData);

    if (i != 0)
    {
        DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "StartDeviceRecord() exit---: TSU IP Addr List Clone Error \r\n");
        SystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "����¼��ʧ��:�߼��豸ID=%s��ԭ��=%s", pGBLogicDeviceInfo->device_id, (char*)"��ӱ���TSU IP��ַʧ��");
        EnSystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "Start video recording failed: logic device ID =%s, cause=%s", pGBLogicDeviceInfo->device_id, (char*)"add local TSU IP address failed");

        i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
        i = call_record_remove(cr_pos);

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "StartDeviceRecord() call_record_remove Error:cr_pos=%d, i=%d \r\n", cr_pos, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "StartDeviceRecord() call_record_remove OK:cr_pos=%d, i=%d \r\n", cr_pos, i);
        }

        printf("\r\nStartDeviceRecord() exit---: TSU IP Addr List Clone Error \r\n");
        return -1;
    }

    printf("\r\nStartDeviceRecord() TSUResourceIPAddrListClone End--- \r\n");

    /* ���в� */
    osip_strncpy(pCrData->caller_id, local_cms_id_get(), MAX_ID_LEN);
    osip_strncpy(pCrData->caller_ip, pGBDeviceInfo->strRegServerIP, MAX_IP_LEN);
    pCrData->caller_port = pGBDeviceInfo->iRegServerPort;
    osip_strncpy(pCrData->caller_server_ip_ethname, pGBDeviceInfo->strRegServerEthName, MAX_IP_LEN);
    osip_strncpy(pCrData->caller_server_ip, pGBDeviceInfo->strRegServerIP, MAX_IP_LEN);
    pCrData->caller_server_port = pGBDeviceInfo->iRegServerPort;

    if (1 == pGBDeviceInfo->trans_protocol)
    {
        pCrData->caller_transfer_type = TRANSFER_PROTOCOL_TCP;
    }
    else
    {
        pCrData->caller_transfer_type = TRANSFER_PROTOCOL_UDP; /* Ĭ��UDP */
    }

    /* ���в�SDP ��Ϣ */
    sdp_tsu_ip = get_cr_sdp_tsu_ip(pCrData, pCrData->caller_server_ip_ethname);

    if (NULL == sdp_tsu_ip)
    {
        SystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "����¼��ʧ��:�߼��豸ID=%s��ԭ��=%s, caller_server_ip_ethname=%s", pGBLogicDeviceInfo->device_id, (char*)"��ȡ���в��SDP��Ϣ�е�TSU��IP��ַʧ��", pCrData->caller_server_ip_ethname);
        EnSystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "Start video recording failed: logic device ID =%s, cause=%s, caller_server_ip_ethname=%s", pGBLogicDeviceInfo->device_id, (char*)"Get the calling side TSU IP address in SDP information failed", pCrData->caller_server_ip_ethname);

        i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
        i = call_record_remove(cr_pos);

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "StartDeviceRecord() call_record_remove Error:cr_pos=%d, i=%d \r\n", cr_pos, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "StartDeviceRecord() call_record_remove OK:cr_pos=%d, i=%d \r\n", cr_pos, i);
        }

        DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "StartDeviceRecord() exit---: Get Caller TSU SDP IP Error:caller_server_ip_ethname=%s \r\n", pCrData->caller_server_ip_ethname);
        return -1;
    }

    printf("\r\nStartDeviceRecord() get_cr_sdp_tsu_ip caller_server_ip_ethname End--- \r\n");

    osip_strncpy(pCrData->caller_sdp_ip, sdp_tsu_ip, MAX_IP_LEN);
    pCrData->caller_sdp_port = recv_port;

    /* ���в� */
    osip_strncpy(pCrData->callee_id, pGBLogicDeviceInfo->device_id, MAX_ID_LEN);
    osip_strncpy(pCrData->callee_ip, device_ip, MAX_IP_LEN);
    pCrData->callee_port = device_port;
    osip_strncpy(pCrData->callee_server_ip_ethname, pGBDeviceInfo->strRegServerEthName, MAX_IP_LEN);
    osip_strncpy(pCrData->callee_server_ip, pGBDeviceInfo->strRegServerIP, MAX_IP_LEN);
    pCrData->callee_server_port = pGBDeviceInfo->iRegServerPort;
    pCrData->callee_framerate = pGBLogicDeviceInfo->frame_count;
    pCrData->callee_stream_type = pProcRecordInfo->stream_type;
    pCrData->callee_gb_device_type = pGBDeviceInfo->device_type;

    if (1 == pGBDeviceInfo->trans_protocol)
    {
        pCrData->callee_transfer_type = TRANSFER_PROTOCOL_TCP;
    }
    else
    {
        pCrData->callee_transfer_type = TRANSFER_PROTOCOL_UDP; /* Ĭ��UDP */
    }

    /* �齨����SDP��Ϣ*/
    memset(&sdp_param, 0, sizeof(sdp_param_t));
    osip_strncpy(sdp_param.o_username, local_cms_id_get(), 32);
    osip_strncpy(sdp_param.s_name, (char*)"Play", 32);

    sdp_tsu_ip = get_cr_sdp_tsu_ip(pCrData, pCrData->callee_server_ip_ethname);

    if (NULL == sdp_tsu_ip)
    {
        SystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "����¼��ʧ��:�߼��豸ID=%s��ԭ��=%s, callee_server_ip_ethname=%s", pGBLogicDeviceInfo->device_id, (char*)"��ȡ���в��SDP��Ϣ�е�TSU��IP��ַʧ��", pCrData->callee_server_ip_ethname);
        EnSystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "Start video recording failed: logic device ID =%s, cause=%s, callee_server_ip_ethname=%s", pGBLogicDeviceInfo->device_id, (char*)"Get the called side TSU IP address in SDP information failed", pCrData->callee_server_ip_ethname);

        i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
        i = call_record_remove(cr_pos);

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "StartDeviceRecord() call_record_remove Error:cr_pos=%d, i=%d \r\n", cr_pos, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "StartDeviceRecord() call_record_remove OK:cr_pos=%d, i=%d \r\n", cr_pos, i);
        }

        DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "StartDeviceRecord() exit---: Get Callee TSU SDP IP Error:callee_server_ip_ethname=%s \r\n", pCrData->callee_server_ip_ethname);
        return -1;
    }

    printf("\r\nStartDeviceRecord() get_cr_sdp_tsu_ip callee_server_ip_ethname End--- \r\n");

    osip_strncpy(sdp_param.sdp_ip, sdp_tsu_ip, MAX_IP_LEN);
    sdp_param.video_port = pCrData->tsu_recv_port;
    sdp_param.video_code_type = -1;
    sdp_param.media_direction = MEDIA_DIRECTION_TYPE_RECVONLY;
    sdp_param.stream_type = pCrData->callee_stream_type;

    /* ���䷽ʽ��ֵ */
    if (TRANSFER_PROTOCOL_TCP == pCrData->callee_transfer_type)
    {
        sdp_param.trans_type = 2;
    }
    else
    {
        sdp_param.trans_type = 1;
    }

    i = SIP_BuildSDPInfoEx(&local_sdp, &sdp_param, NULL);

    if (0 != i)
    {
        sdp_message_free(local_sdp);
        local_sdp = NULL;

        SystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "����¼��ʧ��:�߼��豸ID=%s��ԭ��=%s", pGBLogicDeviceInfo->device_id, (char*)"��������SDP��Ϣʧ��");
        EnSystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "Start video recording failed: logic device ID =%s, cause=%s", pGBLogicDeviceInfo->device_id, (char*)"Create local SDP info failed");

        i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
        i = call_record_remove(cr_pos);

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "StartDeviceRecord() call_record_remove Error:cr_pos=%d, i=%d \r\n", cr_pos, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "StartDeviceRecord() call_record_remove OK:cr_pos=%d, i=%d \r\n", cr_pos, i);
        }

        DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "StartDeviceRecord() exit---: SDP Build Offer Error:tsu_port=%d \r\n", pCrData->tsu_recv_port);
        return -1;
    }

    printf("\r\nStartDeviceRecord() SIP_BuildSDPInfoEx End--- \r\n");

#if 1
    sdp_ssrc = sdp_message_y_ssrc_get(local_sdp);

    if (NULL == sdp_ssrc)
    {
        sdp_ssrc = osip_getcopy((char*)"0100000001");
        sdp_message_y_ssrc_set(local_sdp, sdp_ssrc);
    }

#endif

    printf("\r\nStartDeviceRecord() sdp_message_y_ssrc_get End--- \r\n");

    index = SIP_SendInvite(local_cms_id_get(), pGBLogicDeviceInfo->device_id, pCrData->callee_server_ip, pCrData->callee_server_port, device_ip, device_port, NULL, NULL, local_sdp);
    DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "StartDeviceRecord() SIP_SendInvite:device_id=%s, index=%d \r\n", pGBLogicDeviceInfo->device_id, index);

    if (index < 0)
    {
        sdp_message_free(local_sdp);
        local_sdp = NULL;

        SystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "����¼��ʧ��:�߼��豸ID=%s��ԭ��=%s, ua_index=%d", pGBLogicDeviceInfo->device_id, (char*)"����SIP Invite��Ϣʧ��", index);
        EnSystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "Start video recording failed: logic device ID =%s, cause=%s", pGBLogicDeviceInfo->device_id, (char*)"Send SIP Invite message failed");

        i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
        i = call_record_remove(cr_pos);

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "StartDeviceRecord() call_record_remove Error:cr_pos=%d, i=%d \r\n", cr_pos, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "StartDeviceRecord() call_record_remove OK:cr_pos=%d, i=%d \r\n", cr_pos, i);
        }

        printf("\r\nStartDeviceRecord() exit---: SIP_SendInvite Error:index=%d \r\n", index);
        return -1;
    }

    printf("\r\nStartDeviceRecord() SIP_SendInvite End--- \r\n");

    /* ���������Ϣ��¼��ҵ���¼*/
    pCrData->callee_ua_index = index;

    /* ���¼����Ϣ��TSU�� */
    i = AddRecordInfoToTSU(pTsuResourceInfo, pProcRecordInfo);

    printf("\r\nStartDeviceRecord() AddRecordInfoToTSU End--- \r\n");

    pProcRecordInfo->record_status = RECORD_STATUS_PROC;
    pProcRecordInfo->record_start_time = time(NULL);
    pProcRecordInfo->record_cr_index = cr_pos;

    sdp_message_free(local_sdp);
    local_sdp = NULL;

    //DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "StartDeviceRecord() device_id=%s, device_index=%u, stream_type=%d, record_status=%d, cr_pos=%d, caller_ua_index=%d, callee_ua_index=%d \r\n", pGBLogicDeviceInfo->device_id, pProcRecordInfo->device_index, pProcRecordInfo->stream_type, pProcRecordInfo->record_status, pProcRecordInfo->record_cr_index, pCrData->caller_ua_index, pCrData->callee_ua_index);

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "����¼��ɹ�:�߼��豸ID=%s, �߼��豸����=%s, �߼��豸IP��ַ=%s, ¼������=%d, ��������=%d, ��ѡ��TSU IP=%s, callee_ua_index=%d, cr_pos=%d, ¼���������=%u", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, device_ip, pProcRecordInfo->record_type, pProcRecordInfo->stream_type, tsu_ip, index, cr_pos, pProcRecordInfo->uID);

    printf("\r\nStartDeviceRecord() exit OK --- \r\n");
    return cr_pos;
}

/*****************************************************************************
 �� �� ��  : StopDeviceRecord
 ��������  : ֹͣ�豸¼������
 �������  : int record_cr_index
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��5��29�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int StopDeviceRecord(int record_cr_index)
{
    int i = 0;
    cr_t* pCrData = NULL;

    pCrData = call_record_get(record_cr_index);

    if (NULL != pCrData)
    {
        if (pCrData->call_type != CALL_TYPE_RECORD)
        {
            DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "StopDeviceRecord() exit---: call_type Error:record_cr_index=%d, call_type=%d \r\n", record_cr_index, pCrData->call_type);
            return -1;
        }

        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "ֹͣ¼��:�߼��豸ID=%s", pCrData->callee_id);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Stop video: logical device ID = % s", pCrData->callee_id);
        DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "StopDeviceRecord() :device_id=%s, device_ip=%s, record_cr_index=%d \r\n", pCrData->callee_id, pCrData->callee_ip, record_cr_index);
        printf("StopDeviceRecord() :device_id=%s, device_ip=%s, record_cr_index=%d \r\n", pCrData->callee_id, pCrData->callee_ip, record_cr_index);

        i = SIP_SendBye(pCrData->callee_ua_index);
        DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "StopDeviceRecord() SIP_SendBye:callee_ua_index=%d, i=%d \r\n", pCrData->callee_ua_index, i);

        /* ֪ͨTSUֹͣ��������*/
        i = notify_tsu_delete_record_task(pCrData->tsu_ip, pCrData->task_id);

        if (i < 0)
        {
            DEBUG_TRACE(MODULE_RECORD, LOG_WARN, "StopDeviceRecord() notify_tsu_delete_record_task Error:tsu_ip=%s, task_id=%s, i=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "StopDeviceRecord() notify_tsu_delete_record_task OK:tsu_ip=%s, task_id=%s, i=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
        }

        /* ����Bye ����������ʹ��¼��ҵ������в��û�*/
        i = send_bye_to_all_other_caller_by_callee_id_and_streamtype(pCrData->callee_id, pCrData->callee_stream_type, record_cr_index);
        DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "StopDeviceRecord() send_bye_to_all_other_caller_by_callee_id_and_streamtype:callee_id=%s, callee_stream_type=%d, i=%d \r\n", pCrData->callee_id, pCrData->callee_stream_type, i);
        printf("StopDeviceRecord() send_bye_to_all_other_caller_by_callee_id_and_streamtype:callee_id=%s, callee_stream_type=%d, i=%d \r\n", pCrData->callee_id, pCrData->callee_stream_type, i);

        i = RemoveDeviceRecordInfo(pCrData->callee_id, pCrData->callee_stream_type, pCrData->tsu_resource_index);
        DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "StopDeviceRecord() :callee_id=%s, callee_stream_type=%d, tsu_resource_index=%d, i=%d \r\n", pCrData->callee_id, pCrData->callee_stream_type, pCrData->tsu_resource_index, i);
        printf("StopDeviceRecord() :callee_id=%s, callee_stream_type=%d, tsu_resource_index=%d, i=%d \r\n", pCrData->callee_id, pCrData->callee_stream_type, pCrData->tsu_resource_index, i);

        /* ����TSU ״̬ */
        i = SetTSUStatus(pCrData->tsu_resource_index, 1);
        DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "StopDeviceRecord() SetTSUStatus:tsu_resource_index=%d, i=%d \r\n", pCrData->tsu_resource_index, i);

        i = call_record_set_call_status(record_cr_index, CALL_STATUS_WAIT_RELEASE);
        i = call_record_remove(record_cr_index);

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "StopDeviceRecord() call_record_remove Error:cr_pos=%d, i=%d \r\n", record_cr_index, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "StopDeviceRecord() call_record_remove OK:cr_pos=%d, i=%d \r\n", record_cr_index, i);
        }
    }

    return 0;
}

/*****************************************************************************
 �� �� ��  : StopAllRecordTask
 ��������  : ����¼������
 �������  : int sock
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��7��24�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int StopAllRecordTask(int sock)
{
    int i = 0;
    record_info_t* pRecordInfo = NULL;
    needtoproc_recordinfo_queue needtoproc;
    char rbuf[128] = {0};

    if ((NULL == g_RecordInfoList) || (NULL == g_RecordInfoList->pRecordInfoList))
    {
        //DEBUG_TRACE(MODULE_RECORD, LOG_DEBUG, "scan_record_info_list() exit---: Param Error \r\n");
        return -1;
    }

    needtoproc.clear();

    RECORD_INFO_SMUTEX_LOCK();

    if (osip_list_size(g_RecordInfoList->pRecordInfoList) <= 0)
    {
        RECORD_INFO_SMUTEX_UNLOCK();
        return 0;
    }

    for (i = 0; i < osip_list_size(g_RecordInfoList->pRecordInfoList); i++)
    {
        pRecordInfo = (record_info_t*)osip_list_get(g_RecordInfoList->pRecordInfoList, i);

        if (NULL == pRecordInfo)
        {
            continue;
        }

        if (pRecordInfo->record_cr_index >= 0) /* �Ѿ�¼��� */
        {
            needtoproc.push_back(pRecordInfo);
        }
        else if (pRecordInfo->record_status != RECORD_STATUS_INIT)
        {
            needtoproc.push_back(pRecordInfo);
        }
    }

    RECORD_INFO_SMUTEX_UNLOCK();

    /* ������Ҫֹͣ¼��� */
    while (!needtoproc.empty())
    {
        pRecordInfo = (record_info_t*) needtoproc.front();
        needtoproc.pop_front();

        if (NULL != pRecordInfo)
        {
            i = StopDeviceRecord(pRecordInfo->record_cr_index);

            if (sock > 0)
            {
                memset(rbuf, 0, 128);

                if (i == 0)
                {
                    snprintf(rbuf, 128, "\r�ͷ�¼������ɹ�:¼����������=%d\r\n", pRecordInfo->record_cr_index);
                    send(sock, rbuf, sizeof(rbuf) - 1, 0);
                }
                else
                {
                    snprintf(rbuf, 128, "\r�ͷ�¼������ʧ��:¼����������=%d\r\n", pRecordInfo->record_cr_index);
                    send(sock, rbuf, sizeof(rbuf) - 1, 0);
                }
            }

            pRecordInfo->tsu_index = -1;
            pRecordInfo->record_cr_index = -1;
            pRecordInfo->record_status = RECORD_STATUS_INIT;
            pRecordInfo->record_start_time = 0;
            pRecordInfo->record_try_count = 0;
            pRecordInfo->record_retry_interval = 5;
            pRecordInfo->iTSUPauseStatus = 0;
            pRecordInfo->iTSUResumeStatus = 0;
            pRecordInfo->iTSUAlarmRecordStatus = 0;
        }
    }

    needtoproc.clear();

    return 0;
}

/*****************************************************************************
 �� �� ��  : StopAllProcRecordTask
 ��������  : �ͷŵ�û��¼��ɹ���¼������
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2016��8��25��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int StopAllProcRecordTask()
{
    int i = 0;
    record_info_t* pRecordInfo = NULL;
    needtoproc_recordinfo_queue needtoproc;
    time_t time_now;
    GBLogicDevice_info_t* pGBLogicDeviceInfo = NULL;

    if ((NULL == g_RecordInfoList) || (NULL == g_RecordInfoList->pRecordInfoList))
    {
        //DEBUG_TRACE(MODULE_RECORD, LOG_DEBUG, "scan_record_info_list() exit---: Param Error \r\n");
        return -1;
    }

    needtoproc.clear();

    RECORD_INFO_SMUTEX_LOCK();

    if (osip_list_size(g_RecordInfoList->pRecordInfoList) <= 0)
    {
        RECORD_INFO_SMUTEX_UNLOCK();
        return 0;
    }

    for (i = 0; i < osip_list_size(g_RecordInfoList->pRecordInfoList); i++)
    {
        pRecordInfo = (record_info_t*)osip_list_get(g_RecordInfoList->pRecordInfoList, i);

        if (NULL == pRecordInfo)
        {
            continue;
        }

        if (pRecordInfo->record_cr_index >= 0 && pRecordInfo->record_status == RECORD_STATUS_PROC)
        {
            /* ��������ʱ�� */
            time_now = time(NULL);

            if (pRecordInfo->record_start_time > 0 && time_now > pRecordInfo->record_start_time && time_now - pRecordInfo->record_start_time >= 600)
            {
                needtoproc.push_back(pRecordInfo);
                printf("StopAllProcRecordTask() needtoproc:device_index=%u \r\n", pRecordInfo->device_index);
            }
        }
    }

    RECORD_INFO_SMUTEX_UNLOCK();

    /* ������Ҫֹͣ¼��� */
    while (!needtoproc.empty())
    {
        pRecordInfo = (record_info_t*) needtoproc.front();
        needtoproc.pop_front();

        if (NULL != pRecordInfo)
        {
            i = StopDeviceRecord(pRecordInfo->record_cr_index);
            printf("StopAllProcRecordTask() StopDeviceRecord:record_cr_index=%d \r\n", pRecordInfo->record_cr_index);

            pRecordInfo->tsu_index = -1;
            pRecordInfo->record_cr_index = -1;
            pRecordInfo->record_status = RECORD_STATUS_INIT;
            pRecordInfo->record_start_time = 0;
            pRecordInfo->record_try_count = 0;
            pRecordInfo->record_retry_interval = 5;
            pRecordInfo->iTSUPauseStatus = 0;
            pRecordInfo->iTSUResumeStatus = 0;
            pRecordInfo->iTSUAlarmRecordStatus = 0;

            /* �����߼��豸��Ϣ */
            pGBLogicDeviceInfo = GBLogicDevice_info_find_by_device_index(pRecordInfo->device_index);

            if (NULL != pGBLogicDeviceInfo)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "�ͷŵ�û��¼��ɹ���¼������, ¼���λID=%s, ��λ����=%s, ¼������=%d, ��������=%d", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, pRecordInfo->record_type, pRecordInfo->stream_type);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Video, video released no video success point ID = %s, point name = % s, video type = % d, code flow type = % d", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, pRecordInfo->record_type, pRecordInfo->stream_type);
            }
            else
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "�ͷŵ�û��¼��ɹ���¼������, ¼���λ����=%u, ¼������=%d, ��������=%d", pRecordInfo->device_index, pRecordInfo->record_type, pRecordInfo->stream_type);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Video, video released no video success point index = % u, video type = % d, code flow type = % d ", pRecordInfo->device_index, pRecordInfo->record_type, pRecordInfo->stream_type);
            }
        }
    }

    needtoproc.clear();

    return 0;
}

/*****************************************************************************
 �� �� ��  : StopRecordTask
 ��������  : �ͷ�¼������
 �������  : int sock
             int record_cr_index
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��7��31�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int StopRecordTask(int sock, int record_cr_index)
{
    int i = 0;
    int record_info_index = -1;
    char rbuf[128] = {0};
    record_info_t* pRecordInfo = NULL;

    i = StopDeviceRecord(record_cr_index);

    if (sock > 0)
    {
        memset(rbuf, 0, 128);

        if (i == 0)
        {
            snprintf(rbuf, 128, "\r�ͷ�¼������ɹ�:¼����������=%d\r\n$", record_cr_index);
            send(sock, rbuf, sizeof(rbuf) - 1, 0);
        }
        else
        {
            snprintf(rbuf, 128, "\r�ͷ�¼������ʧ��:¼����������=%d\r\n$", record_cr_index);
            send(sock, rbuf, sizeof(rbuf) - 1, 0);
        }
    }

    /* ��ֹ�����ͷ�ʧ�� */
    record_info_index = record_info_find_by_cr_index(record_cr_index);

    if (record_info_index >= 0)
    {
        pRecordInfo = record_info_get(record_info_index);

        if (NULL != pRecordInfo)
        {
            pRecordInfo->tsu_index = -1;
            pRecordInfo->record_cr_index = -1;
            pRecordInfo->record_status = RECORD_STATUS_INIT;
            pRecordInfo->record_start_time = 0;
            pRecordInfo->record_try_count = 0;
            pRecordInfo->record_retry_interval = 5;
            pRecordInfo->iTSUPauseStatus = 0;
            pRecordInfo->iTSUResumeStatus = 0;
            pRecordInfo->iTSUAlarmRecordStatus = 0;
        }
    }

    return 0;
}
