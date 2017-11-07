
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
#include <string.h>
#include <errno.h>
#endif

#include "common/gblfunc_proc.inc"
#include "common/gblconfig_proc.inc"
#include "common/log_proc.inc"
#include "platformms/BoardInit.h"

#include "route/route_srv_proc.inc"

#include "user/user_srv_proc.inc"

#include "device/device_info_mgn.inc"
#include "device/device_srv_proc.inc"

#include "service/call_func_proc.inc"
#include "service/preset_proc.inc"
#include "service/compress_task_proc.inc"

#include "resource/resource_info_mgn.inc"

#include "record/record_srv_proc.inc"
#include "jwpt_interface/interface_operate.inc"

/*----------------------------------------------*
 * �ⲿ����˵��                                 *
 *----------------------------------------------*/
extern gbl_conf_t* pGblconf;              /* ȫ��������Ϣ */
extern BOARD_NET_ATTR  g_BoardNetConfig;
extern unsigned int g_transfer_xml_sn;    /* ȫ�ֵ�ת��XML��SN */
extern int g_LocalMediaTransferFlag;      /* �¼�ý�����Ƿ񾭹�����TSUת��,Ĭ��ת�� */
extern int g_RouteMediaTransferFlag;      /* �ϼ�������ƽ̨�Ƿ���ý��ת������,Ĭ���� */

extern int db_GBLogicDeviceInfo_reload_mark; /* �߼��豸���ݿ���±�ʶ:0:����Ҫ���£�1:��Ҫ�������ݿ� */
extern int db_GBDeviceInfo_reload_mark;      /* ��׼�����豸���ݿ���±�ʶ:0:����Ҫ���£�1:��Ҫ�������ݿ� */
extern int db_GroupInfo_reload_mark;         /* ������Ϣ���ݿ���±�ʶ:0:����Ҫ���£�1:��Ҫ�������ݿ� */
extern int db_GroupMapInfo_reload_mark;      /* �����ϵ��Ϣ���ݿ���±�ʶ:0:����Ҫ���£�1:��Ҫ�������ݿ� */

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
route_srv_msg_queue g_RouteSrvMsgQueue;  /* ����·��ҵ����Ϣ���� */
#ifdef MULTI_THR
osip_mutex_t* g_RouteSrvMsgQueueLock = NULL;
#endif

route_srv_msg_queue g_RouteMessageSrvMsgQueue;  /* ����·��Messageҵ����Ϣ���� */
#ifdef MULTI_THR
osip_mutex_t* g_RouteMessageSrvMsgQueueLock = NULL;
#endif

/*----------------------------------------------*
 * ��������                                     *
 *----------------------------------------------*/

/*----------------------------------------------*
 * �궨��                                       *
 *----------------------------------------------*/
#define MAXSIZE                    (8 * 4096)

#define SWAPL(x)    ((((x) & 0x000000ff) << 24) | \
             (((x) & 0x0000ff00) <<  8) | \
             (((x) & 0x00ff0000) >>  8) | \
             (((x) & 0xff000000) >> 24))

#if DECS("����·��ҵ����Ϣ����")
/*****************************************************************************
 �� �� ��  : route_srv_msg_init
 ��������  : ����·��ҵ����Ϣ�ṹ��ʼ��
 �������  : route_srv_msg_t ** route_srv_msg
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��11��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int route_srv_msg_init(route_srv_msg_t** route_srv_msg)
{
    *route_srv_msg = (route_srv_msg_t*)osip_malloc(sizeof(route_srv_msg_t));

    if (*route_srv_msg == NULL)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_srv_msg_init() exit---: *route_srv_msg Smalloc Error \r\n");
        return -1;
    }

    (*route_srv_msg)->msg_type = MSG_TYPE_NULL;
    (*route_srv_msg)->pRouteInfo = NULL;
    (*route_srv_msg)->caller_id[0] = '\0';
    (*route_srv_msg)->callee_id[0] = '\0';
    (*route_srv_msg)->response_code = 0;
    (*route_srv_msg)->reasonphrase[0] = '\0';
    (*route_srv_msg)->ua_dialog_index = -1;
    (*route_srv_msg)->msg_body[0] = '\0';
    (*route_srv_msg)->msg_body_len = 0;
    (*route_srv_msg)->cr_pos = -1;

    return 0;
}

/*****************************************************************************
 �� �� ��  : route_srv_msg_free
 ��������  : ����·��ҵ����Ϣ�ṹ�ͷ�
 �������  : route_srv_msg_t * route_srv_msg
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��11��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void route_srv_msg_free(route_srv_msg_t* route_srv_msg)
{
    if (route_srv_msg == NULL)
    {
        //DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_srv_msg_free() exit---: Param Error \r\n");
        return;
    }

    route_srv_msg->msg_type = MSG_TYPE_NULL;
    route_srv_msg->pRouteInfo = NULL;

    memset(route_srv_msg->caller_id, 0, MAX_ID_LEN + 4);
    memset(route_srv_msg->callee_id, 0, MAX_ID_LEN + 4);

    route_srv_msg->response_code = 0;

    memset(route_srv_msg->reasonphrase, 0, MAX_128CHAR_STRING_LEN + 4);

    route_srv_msg->ua_dialog_index = -1;

    memset(route_srv_msg->msg_body, 0, MAX_MSG_BODY_STRING_LEN + 4);

    route_srv_msg->msg_body_len = 0;
    route_srv_msg->cr_pos = -1;

    osip_free(route_srv_msg);
    route_srv_msg = NULL;

    return;
}

/*****************************************************************************
 �� �� ��  : route_srv_msg_list_init
 ��������  : ����·��ҵ����Ϣ���г�ʼ��
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
int route_srv_msg_list_init()
{
    g_RouteSrvMsgQueue.clear();

#ifdef MULTI_THR
    /* init smutex */
    g_RouteSrvMsgQueueLock = (osip_mutex_t*)osip_mutex_init();

    if (NULL == g_RouteSrvMsgQueueLock)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_srv_msg_list_init() exit---: Route Service Message List Lock Init Error \r\n");
        return -1;
    }

#endif
    return 0;
}

/*****************************************************************************
 �� �� ��  : route_srv_msg_list_free
 ��������  : ����·��ҵ����Ϣ�����ͷ�
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
void route_srv_msg_list_free()
{
    route_srv_msg_t* pRouteSrvMsg = NULL;

    while (!g_RouteSrvMsgQueue.empty())
    {
        pRouteSrvMsg = (route_srv_msg_t*) g_RouteSrvMsgQueue.front();
        g_RouteSrvMsgQueue.pop_front();

        if (NULL != pRouteSrvMsg)
        {
            route_srv_msg_free(pRouteSrvMsg);
            pRouteSrvMsg = NULL;
        }
    }

    g_RouteSrvMsgQueue.clear();

#ifdef MULTI_THR

    if (NULL != g_RouteSrvMsgQueueLock)
    {
        osip_mutex_destroy((struct osip_mutex*)g_RouteSrvMsgQueueLock);
        g_RouteSrvMsgQueueLock = NULL;
    }

#endif

    return;
}

/*****************************************************************************
 �� �� ��  : route_srv_msg_list_clean
 ��������  : ����·��ҵ����Ϣ�������
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
void route_srv_msg_list_clean()
{
    route_srv_msg_t* pRouteSrvMsg = NULL;

    while (!g_RouteSrvMsgQueue.empty())
    {
        pRouteSrvMsg = (route_srv_msg_t*) g_RouteSrvMsgQueue.front();
        g_RouteSrvMsgQueue.pop_front();

        if (NULL != pRouteSrvMsg)
        {
            route_srv_msg_free(pRouteSrvMsg);
            pRouteSrvMsg = NULL;
        }
    }

    g_RouteSrvMsgQueue.clear();

    return;
}

/*****************************************************************************
 �� �� ��  : route_srv_msg_add
 ��������  : ��ӻ���·��ҵ����Ϣ��������
 �������  : route_info_t* pRouteInfo
             msg_type_t msg_type
             char* caller_id
             char* callee_id
             int response_code
             char* reasonphrase
             int ua_dialog_index
             char* msg_body
             int msg_body_len
             int cr_pos
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��11��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int route_srv_msg_add(route_info_t* pRouteInfo, msg_type_t msg_type, char* caller_id, char* callee_id, int response_code, char* reasonphrase, int ua_dialog_index, char* msg_body, int msg_body_len, int cr_pos)
{
    route_srv_msg_t* pRouteSrvMsg = NULL;
    int iRet = 0;

    if (caller_id == NULL || pRouteInfo == NULL || callee_id == NULL)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_srv_msg_add() exit---: Param Error \r\n");
        return -1;
    }

    iRet = route_srv_msg_init(&pRouteSrvMsg);

    if (iRet != 0)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_srv_msg_add() exit---: Message Init Error \r\n");
        return -1;
    }

    pRouteSrvMsg->msg_type = msg_type;
    pRouteSrvMsg->pRouteInfo = pRouteInfo;

    if (NULL != caller_id)
    {
        osip_strncpy(pRouteSrvMsg->caller_id, caller_id, MAX_ID_LEN);
    }

    if (NULL != callee_id)
    {
        osip_strncpy(pRouteSrvMsg->callee_id, callee_id, MAX_ID_LEN);
    }

    pRouteSrvMsg->response_code = response_code;

    if (NULL != reasonphrase)
    {
        osip_strncpy(pRouteSrvMsg->reasonphrase, reasonphrase, MAX_128CHAR_STRING_LEN);
    }

    pRouteSrvMsg->ua_dialog_index = ua_dialog_index;

    if (NULL != msg_body)
    {
        osip_strncpy(pRouteSrvMsg->msg_body, msg_body, MAX_MSG_BODY_STRING_LEN);
    }

    pRouteSrvMsg->msg_body_len = msg_body_len;
    pRouteSrvMsg->cr_pos = cr_pos;

#ifdef MULTI_THR
    CMS_GBL_SMUTEX_LOCK((struct osip_mutex*)g_RouteSrvMsgQueueLock);
#endif

    g_RouteSrvMsgQueue.push_back(pRouteSrvMsg);

#ifdef MULTI_THR
    CMS_GBL_SMUTEX_UNLOCK((struct osip_mutex*)g_RouteSrvMsgQueueLock);
#endif

    return 0;
}

/*****************************************************************************
 �� �� ��  : scan_route_srv_msg_list
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
void scan_route_srv_msg_list(DBOper* pRoute_Srv_dboper)
{
    int iRet = 0;
    route_srv_msg_t* pRouteSrvMsg = NULL;

#ifdef MULTI_THR
    CMS_GBL_SMUTEX_LOCK((struct osip_mutex*)g_RouteSrvMsgQueueLock);
#endif

    while (!g_RouteSrvMsgQueue.empty())
    {
        pRouteSrvMsg = (route_srv_msg_t*) g_RouteSrvMsgQueue.front();
        g_RouteSrvMsgQueue.pop_front();

        if (NULL != pRouteSrvMsg)
        {
            break;
        }
    }

#ifdef MULTI_THR
    CMS_GBL_SMUTEX_UNLOCK((struct osip_mutex*)g_RouteSrvMsgQueueLock);
#endif

    if (NULL != pRouteSrvMsg)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_INFO,  "scan_route_srv_msg_list() \
        \r\n In Param: \
        \r\n msg_type=%d \
        \r\n caller_id=%s \
        \r\n callee_id=%s \
        \r\n response_code=%d \
        \r\n ua_dialog_index=%d \
        \r\n msg_body_len=%d \
        \r\n ", pRouteSrvMsg->msg_type, pRouteSrvMsg->caller_id, pRouteSrvMsg->callee_id, pRouteSrvMsg->response_code, pRouteSrvMsg->ua_dialog_index, pRouteSrvMsg->msg_body_len);

        iRet = route_srv_msg_proc(pRouteSrvMsg, pRoute_Srv_dboper);
        route_srv_msg_free(pRouteSrvMsg);
        pRouteSrvMsg = NULL;
    }

    return;
}

/*****************************************************************************
 �� �� ��  : route_message_srv_msg_list_init
 ��������  : ����·��Messageҵ����Ϣ���г�ʼ��
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
int route_message_srv_msg_list_init()
{
    g_RouteMessageSrvMsgQueue.clear();

#ifdef MULTI_THR
    /* init smutex */
    g_RouteMessageSrvMsgQueueLock = (osip_mutex_t*)osip_mutex_init();

    if (NULL == g_RouteMessageSrvMsgQueueLock)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_message_srv_msg_list_init() exit---: Route Service Message List Lock Init Error \r\n");
        return -1;
    }

#endif
    return 0;
}

/*****************************************************************************
 �� �� ��  : route_message_srv_msg_list_free
 ��������  : ����·��Messageҵ����Ϣ�����ͷ�
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
void route_message_srv_msg_list_free()
{
    route_srv_msg_t* pRouteSrvMsg = NULL;

    while (!g_RouteMessageSrvMsgQueue.empty())
    {
        pRouteSrvMsg = (route_srv_msg_t*) g_RouteMessageSrvMsgQueue.front();
        g_RouteMessageSrvMsgQueue.pop_front();

        if (NULL != pRouteSrvMsg)
        {
            route_srv_msg_free(pRouteSrvMsg);
            pRouteSrvMsg = NULL;
        }
    }

    g_RouteMessageSrvMsgQueue.clear();

#ifdef MULTI_THR

    if (NULL != g_RouteMessageSrvMsgQueueLock)
    {
        osip_mutex_destroy((struct osip_mutex*)g_RouteMessageSrvMsgQueueLock);
        g_RouteMessageSrvMsgQueueLock = NULL;
    }

#endif

    return;
}

/*****************************************************************************
 �� �� ��  : route_message_srv_msg_list_clean
 ��������  : ����·��Messageҵ����Ϣ�������
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
void route_message_srv_msg_list_clean()
{
    route_srv_msg_t* pRouteSrvMsg = NULL;

    while (!g_RouteMessageSrvMsgQueue.empty())
    {
        pRouteSrvMsg = (route_srv_msg_t*) g_RouteMessageSrvMsgQueue.front();
        g_RouteMessageSrvMsgQueue.pop_front();

        if (NULL != pRouteSrvMsg)
        {
            route_srv_msg_free(pRouteSrvMsg);
            pRouteSrvMsg = NULL;
        }
    }

    g_RouteMessageSrvMsgQueue.clear();

    return;
}

/*****************************************************************************
 �� �� ��  : route_message_srv_msg_add
 ��������  : ��ӻ���·Message��ҵ����Ϣ��������
 �������  : route_info_t* pRouteInfo
             msg_type_t msg_type
             char* caller_id
             char* callee_id
             int response_code
             int ua_dialog_index
             char* msg_body
             int msg_body_len
             int cr_pos
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��11��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int route_message_srv_msg_add(route_info_t* pRouteInfo, msg_type_t msg_type, char* caller_id, char* callee_id, int response_code, int ua_dialog_index, char* msg_body, int msg_body_len, int cr_pos)
{
    route_srv_msg_t* pRouteSrvMsg = NULL;
    int iRet = 0;

    if (caller_id == NULL || pRouteInfo == NULL || callee_id == NULL)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_message_srv_msg_add() exit---: Param Error \r\n");
        return -1;
    }

    iRet = route_srv_msg_init(&pRouteSrvMsg);

    if (iRet != 0)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_message_srv_msg_add() exit---: Message Init Error \r\n");
        return -1;
    }

    pRouteSrvMsg->msg_type = msg_type;
    pRouteSrvMsg->pRouteInfo = pRouteInfo;

    if (NULL != caller_id)
    {
        osip_strncpy(pRouteSrvMsg->caller_id, caller_id, MAX_ID_LEN);
    }

    if (NULL != callee_id)
    {
        osip_strncpy(pRouteSrvMsg->callee_id, callee_id, MAX_ID_LEN);
    }

    pRouteSrvMsg->response_code = response_code;
    pRouteSrvMsg->ua_dialog_index = ua_dialog_index;

    if (NULL != msg_body)
    {
        osip_strncpy(pRouteSrvMsg->msg_body, msg_body, MAX_MSG_BODY_STRING_LEN);
    }

    pRouteSrvMsg->msg_body_len = msg_body_len;
    pRouteSrvMsg->cr_pos = cr_pos;

#ifdef MULTI_THR
    CMS_GBL_SMUTEX_LOCK((struct osip_mutex*)g_RouteMessageSrvMsgQueueLock);
#endif

    g_RouteMessageSrvMsgQueue.push_back(pRouteSrvMsg);

#ifdef MULTI_THR
    CMS_GBL_SMUTEX_UNLOCK((struct osip_mutex*)g_RouteMessageSrvMsgQueueLock);
#endif

    return 0;
}

/*****************************************************************************
 �� �� ��  : scan_route_message_srv_msg_list
 ��������  : ɨ��༶����Message��Ϣ����
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
void scan_route_message_srv_msg_list(DBOper* pRoute_Srv_dboper)
{
    int iRet = 0;
    route_srv_msg_t* pRouteSrvMsg = NULL;

#ifdef MULTI_THR

    if (NULL != g_RouteMessageSrvMsgQueueLock)
    {
        CMS_GBL_SMUTEX_LOCK((struct osip_mutex*)g_RouteMessageSrvMsgQueueLock);
    }

#endif

    while (!g_RouteMessageSrvMsgQueue.empty())
    {
        pRouteSrvMsg = (route_srv_msg_t*) g_RouteMessageSrvMsgQueue.front();
        g_RouteMessageSrvMsgQueue.pop_front();

        if (NULL != pRouteSrvMsg)
        {
            break;
        }
    }

#ifdef MULTI_THR
    CMS_GBL_SMUTEX_UNLOCK((struct osip_mutex*)g_RouteMessageSrvMsgQueueLock);
#endif

    if (NULL != pRouteSrvMsg)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_INFO,  "scan_route_message_srv_msg_list() \
        \r\n In Param: \
        \r\n msg_type=%d \
        \r\n caller_id=%s \
        \r\n callee_id=%s \
        \r\n response_code=%d \
        \r\n ua_dialog_index=%d \
        \r\n msg_body_len=%d \
        \r\n ", pRouteSrvMsg->msg_type, pRouteSrvMsg->caller_id, pRouteSrvMsg->callee_id, pRouteSrvMsg->response_code, pRouteSrvMsg->ua_dialog_index, pRouteSrvMsg->msg_body_len);

        iRet = route_srv_msg_proc(pRouteSrvMsg, pRoute_Srv_dboper);
        route_srv_msg_free(pRouteSrvMsg);
        pRouteSrvMsg = NULL;
    }

    return;
}
#endif

/*****************************************************************************
 �� �� ��  : route_srv_msg_proc
 ��������  : �༶����ҵ����Ϣ����
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
int route_srv_msg_proc(route_srv_msg_t* pRouteSrvMsg, DBOper* pRoute_Srv_dboper)
{
    int i = 0;

    if (NULL == pRouteSrvMsg)
    {
        return -1;
    }

    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE,  "route_srv_msg_proc() msg_type=%d \r\n ", pRouteSrvMsg->msg_type);

    switch (pRouteSrvMsg->msg_type)
    {
        case MSG_INVITE:
            i = route_invite_msg_proc(pRouteSrvMsg->pRouteInfo, pRouteSrvMsg->caller_id, pRouteSrvMsg->callee_id, pRouteSrvMsg->ua_dialog_index, pRouteSrvMsg->msg_body, pRouteSrvMsg->msg_body_len);
            break;

        case MSG_INVITE_RESPONSE:
            i = route_invite_response_msg_proc(pRouteSrvMsg->cr_pos, pRouteSrvMsg->ua_dialog_index, pRouteSrvMsg->response_code, pRouteSrvMsg->reasonphrase, pRouteSrvMsg->msg_body, pRouteSrvMsg->msg_body_len);
            break;

        case MSG_CANCEL:
            i = route_cancel_msg_proc(pRouteSrvMsg->cr_pos, pRouteSrvMsg->ua_dialog_index);
            break;

        case MSG_ACK:
            i = route_ack_msg_proc(pRouteSrvMsg->cr_pos, pRouteSrvMsg->ua_dialog_index);
            break;

        case MSG_BYE:
            i = route_bye_msg_proc(pRouteSrvMsg->cr_pos, pRouteSrvMsg->ua_dialog_index);
            break;

        case MSG_BYE_RESPONSE:
            i = route_bye_response_msg_proc(pRouteSrvMsg->cr_pos, pRouteSrvMsg->ua_dialog_index, pRouteSrvMsg->response_code);
            break;

        case MSG_INFO:
            i = route_info_msg_proc(pRouteSrvMsg->caller_id, pRouteSrvMsg->callee_id, pRouteSrvMsg->ua_dialog_index, pRouteSrvMsg->msg_body, pRouteSrvMsg->msg_body_len);
            break;

        case MSG_MESSAGE:
            i = route_message_msg_proc(pRouteSrvMsg->pRouteInfo, pRouteSrvMsg->caller_id, pRouteSrvMsg->callee_id, pRouteSrvMsg->msg_body, pRouteSrvMsg->msg_body_len, pRoute_Srv_dboper);
            break;

        case MSG_NOTIFY:
            i = route_notify_msg_proc(pRouteSrvMsg->pRouteInfo, pRouteSrvMsg->caller_id, pRouteSrvMsg->callee_id, pRouteSrvMsg->msg_body, pRouteSrvMsg->msg_body_len, pRoute_Srv_dboper);
            break;

        case MSG_SUBSCRIBE:
            //i = route_subscribe_msg_proc(pRouteSrvMsg->pRouteInfo, pRouteSrvMsg->caller_id, pRouteSrvMsg->callee_id, pRouteSrvMsg->response_code, pRouteSrvMsg->ua_dialog_index, pRouteSrvMsg->msg_body, pRouteSrvMsg->msg_body_len, pRoute_Srv_dboper);
            i = route_subscribe_within_dialog_msg_proc(pRouteSrvMsg->pRouteInfo, pRouteSrvMsg->caller_id, pRouteSrvMsg->callee_id, pRouteSrvMsg->ua_dialog_index, pRouteSrvMsg->response_code, pRouteSrvMsg->msg_body, pRouteSrvMsg->msg_body_len, pRoute_Srv_dboper);
            break;

        default:
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_srv_msg_proc() exit---: Not Support Message Type:%d \r\n", pRouteSrvMsg->msg_type);
            return -1;
    }

    return i;
}

/*****************************************************************************
 �� �� ��  : route_invite_msg_proc
 ��������  : �ϼ�����CMS���͹�����INVITE��Ϣ����
 �������  : route_info_t* pRouteInfo
             char* caller_id
             char* callee_id
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
int route_invite_msg_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, int ua_dialog_index, char* msg_body, int msg_body_len)
{
    int i = 0;
    sdp_message_t* pClientSDP = NULL;
    sdp_param_t stClientSDPParam;
    sdp_extend_param_t stClientSDPExParam;
    GBLogicDevice_info_t* pCalleeGBLogicDeviceInfo = NULL;
    char strErrorCode[32] = {0};

    if (NULL == pRouteInfo)
    {
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_INVITE_CALLER_ROUTE_INFO_ERROR);
        SIP_AnswerToInvite(ua_dialog_index, 503, strErrorCode);
        //SIP_AnswerToInvite(ua_dialog_index, 503, (char*)"Route Info Error");
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_msg_proc() exit---: Route Info Error \r\n");
        return -1;
    }

    if ((NULL == caller_id) || (NULL == callee_id))
    {
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_PARAM_ERROR);
        SIP_AnswerToInvite(ua_dialog_index, 503, strErrorCode);
        //SIP_AnswerToInvite(ua_dialog_index, 503, (char*)"Param Error");
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_msg_proc() exit---: Param Error \r\n");
        return -1;
    }

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS������ʵʱ��Ƶ����:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Superior CMS real-time video request: superior CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);

    if (NULL == msg_body || msg_body_len == 0)
    {
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_INVITE_CALLER_MSG_BODY_ERROR);
        SIP_AnswerToInvite(ua_dialog_index, 503, strErrorCode);
        //SIP_AnswerToInvite(ua_dialog_index, 503, (char*)"Get Message SDP Body Error");
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_msg_proc() exit---: Get Message SDP Body Error \r\n");
        SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS������ʵʱ��Ƶ����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"��ȡ����SDP��Ϣ��ʧ��");
        EnSystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "Real-time monitoring request from superior CMS failed:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, cause=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"Access SDP message body from the requester failed.");
        return -1;
    }

    /* 1�������߼��豸��Ϣ */
    pCalleeGBLogicDeviceInfo = GBLogicDevice_info_find(callee_id);

    if (NULL == pCalleeGBLogicDeviceInfo)
    {
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_INVITE_CALLEE_LOGIC_DEVICE_INFO_ERROR);
        SIP_AnswerToInvite(ua_dialog_index, 503, strErrorCode);
        //SIP_AnswerToInvite(ua_dialog_index, 503, (char*)"Get Callee GBlogicDevice Info Error");
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_msg_proc() exit---: Get Callee GBlogicDevice Info Error:callee_id=%s \r\n", callee_id);
        SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS������ʵʱ��Ƶ����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"��ȡ�߼��豸��Ϣʧ��");
        EnSystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "Real-time monitoring request from superior CMS failed:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, cause=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"Access logical device information failed");
        return -1;
    }

    /* 2����ȡ��Դ�Ŀͻ���sdp��Ϣ���������е�s�ֶΣ��ж�ҵ������ */
    i = sdp_message_init(&pClientSDP);

    if (0 != i)
    {
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_SDP_MSG_INIT_ERROR);
        SIP_AnswerToInvite(ua_dialog_index, 503, strErrorCode);
        //SIP_AnswerToInvite(ua_dialog_index, 503, (char*)"SDP Message Init Error");
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_msg_proc() exit---: SDP Message Init Error \r\n");
        SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS������ʵʱ��Ƶ����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"SDP��Ϣ��ʼ��ʧ��");
        EnSystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "Real-time monitoring request from superior CMS failed:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, cause=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"SDP message initialization failed");
        return -1;
    }

    i = sdp_message_parse(pClientSDP, msg_body);

    if (0 != i)
    {
        sdp_message_free(pClientSDP);
        pClientSDP = NULL;

        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_SDP_MSG_PARSE_ERROR);
        SIP_AnswerToInvite(ua_dialog_index, 503, strErrorCode);
        //SIP_AnswerToInvite(ua_dialog_index, 503, (char*)"SDP Message Parse Error");
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_msg_proc() exit---: SDP Message Parse Error \r\n");
        SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS������ʵʱ��Ƶ����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"SDP��Ϣ����ʧ��");
        EnSystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "Real-time monitoring request from superior CMS failed:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, cause=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"SDP message parsing failed");
        return -1;
    }

    /* 3������sdp�е���Ϣ */
    memset(&stClientSDPParam, 0, sizeof(sdp_param_t));
    memset(&stClientSDPExParam, 0, sizeof(sdp_extend_param_t));

    i = SIP_GetSDPInfoEx(pClientSDP, &stClientSDPParam, &stClientSDPExParam);

    if (i != 0)
    {
        sdp_message_free(pClientSDP);
        pClientSDP = NULL;

        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_SDP_GET_VIDEO_INFO_ERROR);
        SIP_AnswerToInvite(ua_dialog_index, 503, strErrorCode);
        //SIP_AnswerToInvite(ua_dialog_index, 503, (char*)"Get Client SDP Video Info Error");
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_msg_proc() exit---: Get Client SDP Video Info Error \r\n");
        SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS������ʵʱ��Ƶ����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"��ȡSDP��Ϣ�е���Ϣʧ��");
        EnSystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "Real-time monitoring request from superior CMS failed:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, cause=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"Access the information in the SDP message failed");
        return -1;
    }

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS������ʵʱ��Ƶ����:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ����Я������Ƶ����:audio_port=%d, audio_code_type=%d, video_port=%d, video_code_type=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, stClientSDPParam.audio_port, stClientSDPParam.audio_code_type, stClientSDPParam.video_port, stClientSDPParam.video_code_type);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Superior CMS real-time video request: superior CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s, the requester to carry video parameters:audio_port=%d, audio_code_type=%d, video_port=%d, video_code_type=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, stClientSDPParam.audio_port, stClientSDPParam.audio_code_type, stClientSDPParam.video_port, stClientSDPParam.video_code_type);

    /* �ж��������ͣ�����Ƶ��������Ƶ���� */
    if (stClientSDPParam.audio_port > 0
        && stClientSDPParam.video_port <= 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS������ʵʱ��Ƶ����:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, �û����������Ƶ�Խ�ҵ��", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Superior CMS real-time video request: superior CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s, user requests are audio speaker business", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);

        i = route_invite_audio_msg_proc(pRouteInfo, pClientSDP, &stClientSDPParam, pCalleeGBLogicDeviceInfo, caller_id, callee_id, ua_dialog_index);

        if (i != 0)
        {
            sdp_message_free(pClientSDP);
            pClientSDP = NULL;
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_msg_proc() exit---: Audio Invite Proc Error \r\n");
            return -1;
        }
    }
    else
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS������ʵʱ��Ƶ����:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, �û��������ʵʱ��Ƶҵ��", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Superior CMS real-time video request: superior CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s, user requests are real-time video business", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);

        i = route_invite_video_msg_proc(pRouteInfo, pClientSDP, &stClientSDPParam, pCalleeGBLogicDeviceInfo, caller_id, callee_id, ua_dialog_index);

        if (i != 0)
        {
            sdp_message_free(pClientSDP);
            pClientSDP = NULL;
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_msg_proc() exit---: Video Invite Proc Error \r\n");
            return -1;
        }
    }

    sdp_message_free(pClientSDP);
    pClientSDP = NULL;

    return i;
}

/*****************************************************************************
 �� �� ��  : route_invite_video_msg_proc
 ��������  : �ϼ�����CMS���͹�����INVITE��Ƶ��Ϣ����
 �������  : route_info_t* pRouteInfo
             sdp_message_t* pClientSDP
             sdp_param_t* pClientSDPParam
             GBLogicDevice_info_t* pCalleeGBLogicDeviceInfo
             char* caller_id
             char* callee_id
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
int route_invite_video_msg_proc(route_info_t* pRouteInfo, sdp_message_t* pClientSDP, sdp_param_t* pClientSDPParam, GBLogicDevice_info_t* pCalleeGBLogicDeviceInfo, char* caller_id, char* callee_id, int ua_dialog_index)
{
    int i = 0;
    char strErrorCode[32] = {0};

    if (NULL == pRouteInfo)
    {
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_INVITE_CALLER_ROUTE_INFO_ERROR);
        SIP_AnswerToInvite(ua_dialog_index, 503, strErrorCode);
        //SIP_AnswerToInvite(ua_dialog_index, 503, (char*)"Route Info Error");
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_video_msg_proc() exit---: Route Info Error \r\n");
        return -1;
    }

    if (NULL == pClientSDP)
    {
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_INVITE_CALLER_SDP_MSG_ERROR);
        SIP_AnswerToInvite(ua_dialog_index, 503, strErrorCode);
        //SIP_AnswerToInvite(ua_dialog_index, 503, (char*)"Client SDP Info Error");
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_video_msg_proc() exit---: Client SDP Info Error \r\n");
        return -1;
    }

    if (NULL == pClientSDPParam)
    {
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_INVITE_CALLER_SDP_PARAM_ERROR);
        SIP_AnswerToInvite(ua_dialog_index, 503, strErrorCode);
        //SIP_AnswerToInvite(ua_dialog_index, 503, (char*)"Client SDP Param Info Error");
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_video_msg_proc() exit---: Client SDP Param Info Error \r\n");
        return -1;
    }

    if (NULL == pCalleeGBLogicDeviceInfo)
    {
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_INVITE_CALLEE_LOGIC_DEVICE_INFO_ERROR);
        SIP_AnswerToInvite(ua_dialog_index, 503, strErrorCode);
        //SIP_AnswerToInvite(ua_dialog_index, 503, (char*)"Callee GBLogic Device Info Error");
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_video_msg_proc() exit---: Callee GBLogic Device Info Error \r\n");
        return -1;
    }

    /* �����߼��豸����������жϣ�������Ϣ���� */
    if (1 == pCalleeGBLogicDeviceInfo->other_realm)
    {
        i = route_invite_route_video_msg_proc(pRouteInfo, pClientSDP, pClientSDPParam, pCalleeGBLogicDeviceInfo, caller_id, callee_id, ua_dialog_index);
    }
    else
    {
        i = route_invite_sub_video_msg_proc(pRouteInfo, pClientSDP, pClientSDPParam, pCalleeGBLogicDeviceInfo, caller_id, callee_id, ua_dialog_index);
    }

    return i;
}


/*****************************************************************************
 �� �� ��  : route_invite_sub_video_msg_proc
 ��������  : �ϼ�����CMS���͹�����INVITE��Ƶ��Ϣ����
 �������  : route_info_t* pRouteInfo
             sdp_message_t* pClientSDP
             sdp_param_t* pClientSDPParam
             GBLogicDevice_info_t* pCalleeGBLogicDeviceInfo
             char* caller_id
             char* callee_id
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
int route_invite_sub_video_msg_proc(route_info_t* pRouteInfo, sdp_message_t* pClientSDP, sdp_param_t* pClientSDPParam, GBLogicDevice_info_t* pCalleeGBLogicDeviceInfo, char* caller_id, char* callee_id, int ua_dialog_index)
{
    int i = 0;
    int cr_pos = -1;
    cr_t* pCrData = NULL;
    int record_info_pos = -1;
    record_info_t* pRecordInfo = NULL;

    GBDevice_info_t* pCalleeGBDeviceInfo = NULL;
    GBDevice_info_t* pCalleeCmsGBDeviceInfo = NULL;

    //int iAuth = 0;
    //char* realm = NULL;
    //osip_authorization_t* pAuthorization = NULL;

    char* sdp_url = NULL;
    char* sdp_ssrc = NULL;
    char* o_name = NULL;
    char* o_sess_id = NULL;
    char* o_sess_version = NULL;
    char* time_r_repeat = NULL;
    //char strSDPUrl[128] = {0};
    //int iSessionExpires = 0;
    call_type_t eCallType = CALL_TYPE_NULL;
    int stream_type = 0;
    int record_type = 0;
    transfer_protocol_type_t trans_type = TRANSFER_PROTOCOL_NULL;

    char strStartTime[64] = {0};
    char strEndTime[64] = {0};
    char strPlayBackURL[64] = {0};
    int iPlaybackTimeGap = 0;
    char strErrorCode[32] = {0};
    int record_cr_index = -1;

    if (NULL == pRouteInfo)
    {
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_INVITE_CALLER_ROUTE_INFO_ERROR);
        SIP_AnswerToInvite(ua_dialog_index, 503, strErrorCode);
        //SIP_AnswerToInvite(ua_dialog_index, 503, (char*)"Route Info Error");
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_sub_video_msg_proc() exit---: Route Info Error \r\n");
        return -1;
    }

    if (NULL == pClientSDP)
    {
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_INVITE_CALLER_SDP_MSG_ERROR);
        SIP_AnswerToInvite(ua_dialog_index, 503, strErrorCode);
        //SIP_AnswerToInvite(ua_dialog_index, 503, (char*)"Client SDP Info Error");
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_sub_video_msg_proc() exit---: Client SDP Info Error \r\n");
        return -1;
    }

    if (NULL == pClientSDPParam)
    {
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_INVITE_CALLER_SDP_PARAM_ERROR);
        SIP_AnswerToInvite(ua_dialog_index, 503, strErrorCode);
        //SIP_AnswerToInvite(ua_dialog_index, 503, (char*)"Client SDP Param Info Error");
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_sub_video_msg_proc() exit---: Client SDP Param Info Error \r\n");
        return -1;
    }

    if (NULL == pCalleeGBLogicDeviceInfo)
    {
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_INVITE_CALLEE_LOGIC_DEVICE_INFO_ERROR);
        SIP_AnswerToInvite(ua_dialog_index, 503, strErrorCode);
        //SIP_AnswerToInvite(ua_dialog_index, 503, (char*)"Callee GBLogic Device Info Error");
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_sub_video_msg_proc() exit---: Callee GBLogic Device Info Error \r\n");
        return -1;
    }

    /* 1����ȡ�����ͣ����Ҷ�Ӧ�������豸 */
    if (pClientSDPParam->stream_type <= 0)
    {
        stream_type = EV9000_STREAM_TYPE_MASTER;
    }
    else
    {
        stream_type = pClientSDPParam->stream_type;
    }

    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_invite_sub_video_msg_proc() Stream Type=%d \r\n", stream_type);

    if (stream_type == EV9000_STREAM_TYPE_SLAVE && pCalleeGBLogicDeviceInfo->stream_count == 1)
    {
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_INVITE_CALLEE_NOT_SUPPORT_MULTI_STREAM_ERROR);
        SIP_AnswerToInvite(ua_dialog_index, 503, strErrorCode);
        //SIP_AnswerToInvite(ua_dialog_index, 503, (char*)"Callee GBDevice Not Support Multi stream");
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_sub_video_msg_proc() exit---: Callee GBDevice Not Support Multi stream \r\n");
        SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS������ʵʱ��Ƶ����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=%s ", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"�߼��豸��Ϣ��֧�ֶ�����");
        EnSystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "Real-time monitoring request from superior CMS failed:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, cause=%s ", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"Logic device information does not support multi stream");
        return -1;
    }

    /* 2����ȡ�������� */
    if (pClientSDPParam->trans_type <= 0)
    {
        trans_type = TRANSFER_PROTOCOL_UDP;
    }
    else if (pClientSDPParam->trans_type == 2)
    {
        trans_type = TRANSFER_PROTOCOL_TCP;
    }
    else
    {
        trans_type = TRANSFER_PROTOCOL_UDP;
    }

    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_invite_sub_video_msg_proc() Transfer Protocol Type=%d \r\n", trans_type);

    /* ����Ŀ�������豸��Ϣ */
    pCalleeGBDeviceInfo = GBDevice_info_get_by_stream_type(pCalleeGBLogicDeviceInfo, stream_type);

    if (NULL == pCalleeGBDeviceInfo)
    {
        /* ��������ܷ��������������¼�ƽ̨�ĵ�λ�����ʱ����Ҫ�ٲ���һ�������豸 */
        if (EV9000_STREAM_TYPE_INTELLIGENCE == stream_type)
        {
            pCalleeCmsGBDeviceInfo = GBDevice_info_get_by_stream_type(pCalleeGBLogicDeviceInfo, EV9000_STREAM_TYPE_MASTER);

            if (NULL != pCalleeCmsGBDeviceInfo
                && EV9000_DEVICETYPE_SIPSERVER == pCalleeCmsGBDeviceInfo->device_type)
            {
                pCalleeGBDeviceInfo = pCalleeCmsGBDeviceInfo;
                DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_invite_sub_video_msg_proc() CalleeCmsGBDevice: id=%s, ip=%s \r\n", pCalleeCmsGBDeviceInfo->device_id, pCalleeCmsGBDeviceInfo->login_ip);
            }
        }
        else if (EV9000_STREAM_TYPE_SLAVE == stream_type) /* ����Ǹ���������һ�������豸 */
        {
            pCalleeGBDeviceInfo = GBDevice_info_get_by_stream_type(pCalleeGBLogicDeviceInfo, EV9000_STREAM_TYPE_MASTER);
        }
    }

    if (NULL == pCalleeGBDeviceInfo)
    {
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_INVITE_CALLEE_DEVICE_INFO_ERROR);
        SIP_AnswerToInvite(ua_dialog_index, 503, strErrorCode);
        //SIP_AnswerToInvite(ua_dialog_index, 503, (char*)"Get Callee GBDevice Info Error");
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_sub_video_msg_proc() exit---: Get Callee GBDevice Info Error \r\n");
        SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS������ʵʱ��Ƶ����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=%s, ý��������=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"��ȡ�߼��豸��Ӧ�������豸��Ϣʧ��", stream_type);
        EnSystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "Real-time monitoring request from superior CMS failed:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, cause=%s, media stream type=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"Physical device obtain the corresponding logical device information failed", stream_type);
        return -1;
    }

    /* ����ǵ�����ƽ̨��ȥ����չ��SDP��Ϣ */
    if (EV9000_DEVICETYPE_SIPSERVER == pCalleeGBDeviceInfo->device_type
        && 1 == pCalleeGBDeviceInfo->three_party_flag)
    {
        DelSDPMediaAttributeByName(pClientSDP, (char*)"recordtype");
        DelSDPMediaAttributeByName(pClientSDP, (char*)"streamtype");

        /* �޸����� */
        o_name = sdp_message_o_username_get(pClientSDP);

        if (NULL != o_name && 0 != sstrcmp(o_name, local_cms_id_get()))
        {
            osip_free(o_name);
            o_name = NULL;

            o_name = osip_getcopy(local_cms_id_get());
            sdp_message_o_username_set(pClientSDP, o_name);
        }
        else if (NULL == o_name)
        {
            o_name = osip_getcopy(local_cms_id_get());
            sdp_message_o_username_set(pClientSDP, o_name);
        }

        o_sess_id = sdp_message_o_sess_id_get(pClientSDP);

        if (NULL != o_sess_id && 0 != sstrcmp(o_sess_id, (char*)"0"))
        {
            osip_free(o_sess_id);
            o_sess_id = NULL;

            o_sess_id = osip_getcopy((char*)"0");
            sdp_message_o_sess_id_set(pClientSDP, o_sess_id);
        }
        else if (NULL == o_sess_id)
        {
            o_sess_id = osip_getcopy(local_cms_id_get());
            sdp_message_o_sess_id_set(pClientSDP, o_sess_id);
        }

        o_sess_version = sdp_message_o_sess_version_get(pClientSDP);

        if (NULL != o_sess_version && 0 != sstrcmp(o_sess_version, (char*)"0"))
        {
            osip_free(o_sess_version);
            o_sess_version = NULL;

            o_sess_version = osip_getcopy((char*)"0");
            sdp_message_o_sess_version_set(pClientSDP, o_sess_version);
        }
        else if (NULL == o_sess_version)
        {
            o_sess_version = osip_getcopy(local_cms_id_get());
            sdp_message_o_sess_version_set(pClientSDP, o_sess_version);
        }

        time_r_repeat = sdp_message_r_repeat_get(pClientSDP, 0, 0);

        if (NULL != time_r_repeat)
        {
            DelSDPTimeRepeatInfo(pClientSDP);
        }
    }

    if (0 == strncmp(pClientSDPParam->s_name, (char*)"Playback", 8))
    {
        eCallType = CALL_TYPE_RECORD_PLAY;

        sdp_url = sdp_message_u_uri_get(pClientSDP);

        if (NULL == sdp_url)
        {
            snprintf(strPlayBackURL, 64, "%s:%s", callee_id, (char*)"application");
            sdp_url = osip_getcopy(strPlayBackURL);
            sdp_message_u_uri_set(pClientSDP, sdp_url);
        }
        else
        {
            osip_free(sdp_url);
            sdp_url = NULL;

            snprintf(strPlayBackURL, 64, "%s:%s", callee_id, (char*)"application");
            sdp_url = osip_getcopy(strPlayBackURL);
            sdp_message_u_uri_set(pClientSDP, sdp_url);
        }

#if 1
        sdp_ssrc = sdp_message_y_ssrc_get(pClientSDP);

        if (NULL == sdp_ssrc)
        {
            sdp_ssrc = osip_getcopy((char*)"1100000001");
            sdp_message_y_ssrc_set(pClientSDP, sdp_ssrc);
        }

#endif

        /* ȷ��¼������ */
        if (pClientSDPParam->record_type <= 0)
        {
            record_type = EV9000_RECORD_TYPE_NORMAL;
        }
        else
        {
            record_type = pClientSDPParam->record_type;
        }

        i = format_time(pClientSDPParam->start_time, strStartTime);
        i = format_time(pClientSDPParam->end_time, strEndTime);

#if 0

        /* �����ǰ�˵�NVR����DVR¼�����޸Ĳ���ʱ�� */
        if (EV9000_DEVICETYPE_DVR == pCalleeGBDeviceInfo->device_type || EV9000_DEVICETYPE_NVR == pCalleeGBDeviceInfo->device_type)
        {
            if (1 == pCalleeGBLogicDeviceInfo->record_type)
            {
                i = ModifySDPRecordPlayTime(pClientSDP);
                iPlaybackTimeGap = pClientSDPParam->play_time - pClientSDPParam->start_time;
                DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_invite_sub_video_msg_proc() PlaybackTimeGap=%d \r\n", iPlaybackTimeGap);
            }
        }
        else if (EV9000_DEVICETYPE_SIPSERVER == pCalleeGBDeviceInfo->device_type && 1 == pCalleeGBDeviceInfo->three_party_flag) /* ������ƽ̨ */
        {
            i = ModifySDPRecordPlayTime(pClientSDP);
            iPlaybackTimeGap = pClientSDPParam->play_time - pClientSDPParam->start_time;
            DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_invite_sub_video_msg_proc() PlaybackTimeGap=%d \r\n", iPlaybackTimeGap);
        }

#endif
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS��������ʷ��Ƶ�ط�����:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ¼������=%d, ý��������=%d, ���䷽ʽ=%d, �طſ�ʼʱ��=%s, ����ʱ��=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, record_type, stream_type, trans_type, strStartTime, strEndTime);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Historical video playback request from superior CMS:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, cause=%s, media stream type=%d, transmission method=%d, playback start time=%s, end time=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, stream_type, trans_type, strStartTime, strEndTime);
    }
    else if (0 == strncmp(pClientSDPParam->s_name, (char*)"Play", 4))
    {
        eCallType = CALL_TYPE_REALTIME;

#if 1
        sdp_ssrc = sdp_message_y_ssrc_get(pClientSDP);

        if (NULL == sdp_ssrc)
        {
            sdp_ssrc = osip_getcopy((char*)"0100000001");
            sdp_message_y_ssrc_set(pClientSDP, sdp_ssrc);
        }

#endif

        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS������ʵʱ��Ƶ����, �ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ý��������=%d, ���䷽ʽ=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, stream_type, trans_type);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Real-time monitoring request from superior CMS, superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, cause=%s, media stream type=%d, transmission method=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, stream_type, trans_type);
    }
    else if (0 == strncmp(pClientSDPParam->s_name, (char*)"Download", 8))
    {
        eCallType = CALL_TYPE_DOWNLOAD;

        sdp_url = sdp_message_u_uri_get(pClientSDP);

        if (NULL == sdp_url)
        {
            snprintf(strPlayBackURL, 64, "%s:%s", callee_id, (char*)"application");
            sdp_url = osip_getcopy(strPlayBackURL);
            sdp_message_u_uri_set(pClientSDP, sdp_url);
        }
        else
        {
            osip_free(sdp_url);
            sdp_url = NULL;

            snprintf(strPlayBackURL, 64, "%s:%s", callee_id, (char*)"application");
            sdp_url = osip_getcopy(strPlayBackURL);
            sdp_message_u_uri_set(pClientSDP, sdp_url);
        }

#if 1
        sdp_ssrc = sdp_message_y_ssrc_get(pClientSDP);

        if (NULL == sdp_ssrc)
        {
            sdp_ssrc = osip_getcopy((char*)"1100000001");
            sdp_message_y_ssrc_set(pClientSDP, sdp_ssrc);
        }

#endif

        /* ȷ��¼������ */
        if (pClientSDPParam->record_type <= 0)
        {
            record_type = EV9000_RECORD_TYPE_NORMAL;
        }
        else
        {
            record_type = pClientSDPParam->record_type;
        }

        i = format_time(pClientSDPParam->start_time, strStartTime);
        i = format_time(pClientSDPParam->end_time, strEndTime);

#if 0

        /* �����ǰ�˵�NVR����DVR¼�����޸Ĳ���ʱ�� */
        if (EV9000_DEVICETYPE_DVR == pCalleeGBDeviceInfo->device_type || EV9000_DEVICETYPE_NVR == pCalleeGBDeviceInfo->device_type)
        {
            if (1 == pCalleeGBLogicDeviceInfo->record_type)
            {
                i = ModifySDPRecordPlayTime(pClientSDP);
                iPlaybackTimeGap = pClientSDPParam->play_time - pClientSDPParam->start_time;
                DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_invite_sub_video_msg_proc() PlaybackTimeGap=%d \r\n", iPlaybackTimeGap);
            }
        }
        else if (EV9000_DEVICETYPE_SIPSERVER == pCalleeGBDeviceInfo->device_type && 1 == pCalleeGBDeviceInfo->three_party_flag) /* ������ƽ̨ */
        {
            i = ModifySDPRecordPlayTime(pClientSDP);
            iPlaybackTimeGap = pClientSDPParam->play_time - pClientSDPParam->start_time;
            DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_invite_sub_video_msg_proc() PlaybackTimeGap=%d \r\n", iPlaybackTimeGap);
        }

#endif
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS��������ʷ��Ƶ�ļ���������:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ¼������=%d, ý��������=%d, ���䷽ʽ=%d, �طſ�ʼʱ��=%s, ����ʱ��=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, stream_type, trans_type, strStartTime, strEndTime);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Historical video file download request from superior CMS:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, cause=%s, media stream type=%d, transmission method=%d, playback start time=%s, end time=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, stream_type, trans_type, strStartTime, strEndTime);
    }
    else
    {
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_SDP_NOT_SUPPORT_S_TYPE_ERROR);
        SIP_AnswerToInvite(ua_dialog_index, 488, strErrorCode);
        //SIP_AnswerToInvite(ua_dialog_index, 488, (char*)"SDP S Type Not Support");
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_sub_video_msg_proc() exit---: SDP S Type Not Support \r\n");
        SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS������ʵʱ��Ƶ����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=%s, S����=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"��֧�ֵ�S��������", pClientSDPParam->s_name);
        EnSystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "Real-time monitoring request from superior CMS failed:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, cause=%s, S name=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"do not support S name sype", pClientSDPParam->s_name);
        return -1;
    }

    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_invite_sub_video_msg_proc() Call Type=%d \r\n", eCallType);

    /* ʵʱ��Ƶ�鿴��Ҫ�ж��豸����״̬ */
    if (eCallType == CALL_TYPE_REALTIME)
    {
        /* �������豸�Ƿ����� */
        if (0 == pCalleeGBDeviceInfo->reg_status)
        {
            memset(strErrorCode, 0, 32);
            snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_DEVICE_OFFLINE_ERROR);
            SIP_AnswerToInvite(ua_dialog_index, 480, strErrorCode);
            //SIP_AnswerToInvite(ua_dialog_index, 480, NULL);
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_sub_video_msg_proc() exit---: Device Not Online:device_id=%s \r\n", pCalleeGBDeviceInfo->device_id);
            SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS������ʵʱ��Ƶ����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=%s, �����豸ID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"ǰ�������豸������", pCalleeGBDeviceInfo->device_id);
            EnSystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "Real-time monitoring request from superior CMS failed:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, cause=%s, physical deviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"Front-end physical device not online", pCalleeGBDeviceInfo->device_id);
            return -1;
        }

        /* ���߼��豸��λ״̬, ������¼�CMS�ĵ㣬����Ҫ�ж� */
        if (EV9000_DEVICETYPE_SIPSERVER != pCalleeGBDeviceInfo->device_type && 0 == pCalleeGBLogicDeviceInfo->status)
        {
            memset(strErrorCode, 0, 32);
            snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_DEVICE_LOGIC_DEVICE_OFFLINE_ERROR);
            SIP_AnswerToInvite(ua_dialog_index, 480, strErrorCode);
            //SIP_AnswerToInvite(ua_dialog_index, 480, NULL);
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_sub_video_msg_proc() exit---: GBLogic Device Not Online \r\n");
            SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS������ʵʱ��Ƶ����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"ǰ���߼��豸������");
            EnSystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "Real-time monitoring request from superior CMS failed:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, cause=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"Front-end logic device is not online");
            return -1;
        }

#if 0

        if (2 == pCalleeGBLogicDeviceInfo->status)
        {
            SIP_AnswerToInvite(ua_dialog_index, 480, NULL);
            DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_invite_msg_proc() exit---: GBLogic Device No Stream \r\n");
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS������ʵʱ��Ƶ����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"ǰ���߼��豸û������");
            return -1;
        }

#endif

        if (3 == pCalleeGBLogicDeviceInfo->status)
        {
            memset(strErrorCode, 0, 32);
            snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_DEVICE_LOGIC_DEVICE_UNREACHED_ERROR);
            SIP_AnswerToInvite(ua_dialog_index, 503, strErrorCode);
            //SIP_AnswerToInvite(ua_dialog_index, 480, NULL);
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_sub_video_msg_proc() exit---: GBLogic Device NetWork UnReached \r\n");
            SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS������ʵʱ��Ƶ����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"ǰ���߼��豸���粻�ɴ�");
            EnSystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "Real-time monitoring request from superior CMS failed:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, cause=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"Front-end logic device network unaccessable");
            return -1;
        }

        /* ������ƽ̨, �鿴ԭ���Ƿ���ҵ������У���رյ� */
        if (pRouteInfo->three_party_flag && 1 == g_RouteMediaTransferFlag)
        {
            i = StopRouteService(pRouteInfo, pCalleeGBLogicDeviceInfo->device_id, stream_type);

            if (i == -2)
            {
                /* û������ */
            }
            else if (i < 0 && i != -2)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR,  "route_invite_sub_video_msg_proc() StopRouteService Error:DeviceID=%s, stream_type=%d \r\n", pCalleeGBLogicDeviceInfo->device_id, stream_type);
            }
            else
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "�ϼ�CMS������ʵʱ��Ƶ����, �رյ���λԭ�е�ʵʱ��Ƶҵ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ��������=%d, i=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, stream_type, i);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Real-time monitoring request from superior CMS, close old service:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, stream type=%s, i=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, stream_type, i);

                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE,  "route_invite_sub_video_msg_proc() StopRouteService OK:DeviceID=%s, stream_type=%d \r\n", pCalleeGBLogicDeviceInfo->device_id, stream_type);
            }
        }
    }

    /* 3�����������Դ */
    cr_pos = call_record_add();
    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_invite_sub_video_msg_proc() call_record_add:cr_pos=%d \r\n", cr_pos);

    pCrData = call_record_get(cr_pos);

    if (NULL == pCrData)
    {
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_INVITE_GET_IDLE_CR_DATA_ERROR);
        SIP_AnswerToInvite(ua_dialog_index, 503, strErrorCode);
        //SIP_AnswerToInvite(ua_dialog_index, 503, (char*)"Get Call Record Data Error");
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_sub_video_msg_proc() exit---: Get Call Record Data Error \r\n");
        SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS������ʵʱ��Ƶ����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"���������Դʧ��");
        EnSystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "Real-time monitoring request from superior CMS failed:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, cause=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"apply to call resource failed");
        return -1;
    }

    pCrData->call_type = eCallType;

    /* 4����� ��������Ϣ��������Դ��Ϣ */
    /* ���ж���Ϣ */
    osip_strncpy(pCrData->caller_id, caller_id, MAX_ID_LEN);
    osip_strncpy(pCrData->caller_ip, pRouteInfo->server_ip, MAX_IP_LEN);
    pCrData->caller_port = pRouteInfo->server_port;
    pCrData->caller_ua_index = ua_dialog_index;
    osip_strncpy(pCrData->caller_server_ip_ethname, pRouteInfo->strRegLocalEthName, MAX_IP_LEN);
    osip_strncpy(pCrData->caller_server_ip, pRouteInfo->strRegLocalIP, MAX_IP_LEN);
    pCrData->caller_server_port = pRouteInfo->iRegLocalPort;
    pCrData->caller_transfer_type = trans_type;
    pCrData->iPlaybackTimeGap = iPlaybackTimeGap;

    /* ���ж˵�SDP��Ϣ */
    osip_strncpy(pCrData->caller_sdp_ip, pClientSDPParam->sdp_ip, MAX_IP_LEN);
    pCrData->caller_sdp_port = pClientSDPParam->video_port;

#if 0
    printf("\r\n\r\n ************************************************* \r\n");
    printf(" route_invite_sub_video_msg_proc() pCrData->caller_id=%s \r\n", pCrData->caller_id);
    printf(" route_invite_sub_video_msg_proc() pCrData->caller_ip=%s \r\n", pCrData->caller_ip);
    printf(" route_invite_sub_video_msg_proc() pCrData->caller_port=%d \r\n", pCrData->caller_port);
    printf(" route_invite_sub_video_msg_proc() pCrData->caller_sdp_ip=%s \r\n", pCrData->caller_sdp_ip);
    printf(" route_invite_sub_video_msg_proc() pCrData->caller_sdp_port=%d \r\n", pCrData->caller_sdp_port);
    printf(" route_invite_sub_video_msg_proc() pCrData->caller_ua_index=%d \r\n", pCrData->caller_ua_index);
    printf(" ************************************************* \r\n\r\n ");
#endif

    /* ���ж���Ϣ */
    osip_strncpy(pCrData->callee_id, callee_id, MAX_ID_LEN);
    osip_strncpy(pCrData->callee_ip, pCalleeGBDeviceInfo->login_ip, MAX_IP_LEN);
    pCrData->callee_port = pCalleeGBDeviceInfo->login_port;
    osip_strncpy(pCrData->callee_server_ip_ethname, pCalleeGBDeviceInfo->strRegServerEthName, MAX_IP_LEN);
    osip_strncpy(pCrData->callee_server_ip, pCalleeGBDeviceInfo->strRegServerIP, MAX_IP_LEN);
    pCrData->callee_server_port = pCalleeGBDeviceInfo->iRegServerPort;
    pCrData->callee_framerate = pCalleeGBLogicDeviceInfo->frame_count;
    pCrData->callee_stream_type = stream_type;
    pCrData->callee_gb_device_type = pCalleeGBDeviceInfo->device_type;

    if (1 == pCalleeGBDeviceInfo->trans_protocol)
    {
        pCrData->callee_transfer_type = TRANSFER_PROTOCOL_TCP;
    }
    else
    {
        pCrData->callee_transfer_type = TRANSFER_PROTOCOL_UDP; /* Ĭ��UDP */
    }

#if 0
    printf("\r\n\r\n ************************************************* \r\n");
    printf(" route_invite_sub_video_msg_proc() pCrData->callee_id=%s \r\n", pCrData->callee_id);
    printf(" route_invite_sub_video_msg_proc() pCrData->callee_ip=%s \r\n", pCrData->callee_ip);
    printf(" route_invite_sub_video_msg_proc() pCrData->callee_port=%d \r\n", pCrData->callee_port);
    printf(" ************************************************* \r\n\r\n ");
#endif

    /* �����¼��طŻ�������, �ж�¼������ǰ�˻����ڱ���,���¼�ڱ��أ�����Ҫת��ǰ�˴��� */
    if (eCallType == CALL_TYPE_RECORD_PLAY || eCallType == CALL_TYPE_DOWNLOAD)
    {
        /* �鿴�õ�λ�Ƿ�������¼�� */
        if (EV9000_RECORD_TYPE_NORMAL == record_type
            || EV9000_RECORD_TYPE_ALARM == record_type)
        {
            record_info_pos = record_info_find_by_stream_type(pCalleeGBLogicDeviceInfo->id, EV9000_STREAM_TYPE_MASTER);
            DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_invite_sub_video_msg_proc() record_info_find_by_stream_type:device_id=%s, stream_type=%d, record_info_pos=%d \r\n", pCalleeGBLogicDeviceInfo->device_id, EV9000_STREAM_TYPE_MASTER, record_info_pos);
        }
        else if (EV9000_RECORD_TYPE_BACKUP == record_type)
        {
            record_info_pos = record_info_find_by_stream_type(pCalleeGBLogicDeviceInfo->id, EV9000_STREAM_TYPE_SLAVE);
            DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_invite_sub_video_msg_proc() record_info_find_by_stream_type:device_id=%s, stream_type=%d, record_info_pos=%d \r\n", pCalleeGBLogicDeviceInfo->device_id, EV9000_STREAM_TYPE_SLAVE, record_info_pos);
        }
        else if (EV9000_RECORD_TYPE_INTELLIGENCE == record_type)
        {
            record_info_pos = record_info_find_by_stream_type(pCalleeGBLogicDeviceInfo->id, EV9000_STREAM_TYPE_INTELLIGENCE);
            DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_invite_sub_video_msg_proc() record_info_find_by_stream_type:device_id=%s, stream_type=%d, record_info_pos=%d \r\n", pCalleeGBLogicDeviceInfo->device_id, EV9000_STREAM_TYPE_INTELLIGENCE, record_info_pos);
        }
        else
        {
            record_info_pos = record_info_find_by_stream_type(pCalleeGBLogicDeviceInfo->id, EV9000_STREAM_TYPE_MASTER);
            DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_invite_sub_video_msg_proc() record_info_find_by_stream_type:device_id=%s, stream_type=%d, record_info_pos=%d \r\n", pCalleeGBLogicDeviceInfo->device_id, EV9000_STREAM_TYPE_MASTER, record_info_pos);
        }

        if (record_info_pos >= 0)
        {
            pRecordInfo = record_info_get(record_info_pos);

            if (NULL != pRecordInfo)
            {
                if (1 == pRecordInfo->record_enable)
                {
                    i = user_video_sevice_current_cms_proc(pCrData, pClientSDP, eCallType, pClientSDPParam->start_time, pClientSDPParam->end_time, pClientSDPParam->play_time, 0, record_type);

                    if (0 != i)
                    {
                        memset(strErrorCode, 0, 32);
                        snprintf(strErrorCode, 32, "%d", i);
                        SIP_AnswerToInvite(ua_dialog_index, 503, strErrorCode);
                        //SIP_AnswerToInvite(ua_dialog_index, 503, (char*)"Current CMS Proc Error");
                        i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
                        i = call_record_remove(cr_pos);

                        if (0 != i)
                        {
                            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_sub_video_msg_proc() call_record_remove Error:cr_pos=%d, i=%d \r\n", cr_pos, i);
                        }
                        else
                        {
                            DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_invite_sub_video_msg_proc() call_record_remove OK:cr_pos=%d, i=%d \r\n", cr_pos, i);
                        }
                    }

                    return i;
                }
            }
        }
    }
    else
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_invite_sub_video_msg_proc() CalleeGBLogicDeviceInfo Record Type=%d, Status=%d \r\n", pCalleeGBLogicDeviceInfo->record_type, pCalleeGBLogicDeviceInfo->status);

        if (0 == pCalleeGBLogicDeviceInfo->record_type && 2 != pCalleeGBLogicDeviceInfo->status)/* ����¼���Ҳ���ǰ��û��������״̬����¼��¼����Ϣ����Ϊǰ��û����������²�Ӱ��ʵʱ��Ƶ���� */
        {
            /* ���õ�λ�Ƿ�¼���� */
            record_info_pos = record_info_find_by_stream_type(pCalleeGBLogicDeviceInfo->id, stream_type);
            DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_invite_sub_video_msg_proc() record_info_find_by_stream_type:device_id=%s, stream_type=%d, record_info_pos=%d \r\n", pCalleeGBLogicDeviceInfo->device_id, stream_type, record_info_pos);

            if (record_info_pos >= 0)
            {
                pRecordInfo = record_info_get(record_info_pos);

                if (NULL != pRecordInfo)
                {
                    if (1 == pRecordInfo->record_enable)
                    {
                        if (pRecordInfo->record_cr_index < 0 && pRecordInfo->record_status != RECORD_STATUS_NO_TSU)
                        {
                            record_cr_index = StartDeviceRecord(pRecordInfo);

                            if (record_cr_index >= 0)
                            {
                                pRecordInfo->record_retry_interval = 5;
                                pRecordInfo->record_try_count = 0;
                                pRecordInfo->iTSUPauseStatus = 0;
                                pRecordInfo->iTSUResumeStatus = 0;
                                pRecordInfo->iTSUAlarmRecordStatus = 0;

                                i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_ANSWER);
                                SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_WARNING, "�ϼ�CMS������ʵʱ��Ƶ����:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, �ȴ��߼��豸¼��ҵ�����̽���", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);
                                return 0;
                            }
                            else
                            {
                                pRecordInfo->tsu_index = -1;
                                pRecordInfo->iTSUPauseStatus = 0;
                                pRecordInfo->iTSUResumeStatus = 0;
                                pRecordInfo->iTSUAlarmRecordStatus = 0;

                                if (-2 == record_cr_index)
                                {
                                    pRecordInfo->record_status = RECORD_STATUS_OFFLINE;
                                }
                                else if (-3 == record_cr_index)
                                {
                                    pRecordInfo->record_status = RECORD_STATUS_NOSTREAM;
                                }
                                else if (-5 == record_cr_index)
                                {
                                    pRecordInfo->record_status = RECORD_STATUS_NETWORK_ERROR;
                                }
                                else if (-4 == record_cr_index)
                                {
                                    pRecordInfo->record_status = RECORD_STATUS_NO_TSU;
                                }
                                else if (-6 == record_cr_index)
                                {
                                    pRecordInfo->record_status = RECORD_STATUS_NOT_SUPPORT_MULTI_STREAM;
                                }
                                else
                                {
                                    pRecordInfo->record_status = RECORD_STATUS_INIT;
                                }

                                pRecordInfo->record_start_time = 0;

                                pRecordInfo->record_try_count++;

                                if (pRecordInfo->record_try_count >= 3)
                                {
                                    pRecordInfo->record_try_count = 0;
                                    pRecordInfo->record_retry_interval = 5;
                                }

                                memset(strErrorCode, 0, 32);
                                snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_INVITE_CALLEE_RECORD_NOT_START_ERROR);
                                SIP_AnswerToInvite(ua_dialog_index, 503, strErrorCode);

                                i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
                                i = call_record_remove(cr_pos);

                                if (0 != i)
                                {
                                    DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_sub_video_msg_proc() call_record_remove Error:cr_pos=%d, i=%d \r\n", cr_pos, i);
                                }
                                else
                                {
                                    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_invite_sub_video_msg_proc() call_record_remove OK:cr_pos=%d, i=%d \r\n", cr_pos, i);
                                }

                                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_sub_video_msg_proc() exit---: Callee GBDevice Not Start Record \r\n");
                                SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS������ʵʱ��Ƶ����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"�߼��豸������¼�񣬵�������¼��ʧ����");
                                EnSystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "Real-time monitoring request from superior CMS failed:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, cause=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"video is configured for the logic device, but not started");
                                return -1;
                            }
                        }
                        else if (pRecordInfo->record_cr_index >= 0 && pRecordInfo->record_status != RECORD_STATUS_COMPLETE)
                        {
                            i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_ANSWER);
                            SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_WARNING, "�ϼ�CMS������ʵʱ��Ƶ����:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, �ȴ��߼��豸¼��ҵ�����̽���", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);
                            return 0;
                        }
                    }
                }
            }
        }
    }

    /* 5�������豸��������ͬ�Ĵ��� */
    if (pCalleeGBDeviceInfo->device_type == EV9000_DEVICETYPE_SIPSERVER)
    {
        i = user_video_sevice_sub_cms_proc(pCrData, pClientSDP, eCallType);

        if (EV9000_CMS_ERR_INVITE_CALLEE_RECORD_NOT_COMPLETE_ERROR == i)
        {
            i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_ANSWER);
            SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_WARNING, "�ϼ�CMS������ʵʱ��Ƶ����:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, �ȴ��߼��豸¼��ҵ�����̽���", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);
            return 0;
        }
        else if (EV9000_CMS_ERR_INVITE_CALLEE_VIDEO_NOT_COMPLETE_ERROR == i)
        {
            i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_ANSWER);
            SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_WARNING, "�ϼ�CMS������ʵʱ��Ƶ����:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, �ȴ��߼��豸��Ƶҵ�����̽���", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);
            return 0;
        }
        else if (0 != i)
        {
            memset(strErrorCode, 0, 32);
            snprintf(strErrorCode, 32, "%d", i);
            SIP_AnswerToInvite(ua_dialog_index, 503, strErrorCode);
            //SIP_AnswerToInvite(ua_dialog_index, 503, (char*)"Sub CMS Proc Error");
            i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
            i = call_record_remove(cr_pos);

            if (0 != i)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_sub_video_msg_proc() call_record_remove Error:cr_pos=%d, i=%d \r\n", cr_pos, i);
            }
            else
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_invite_sub_video_msg_proc() call_record_remove OK:cr_pos=%d, i=%d \r\n", cr_pos, i);
            }
        }
    }
    else
    {
        i = user_video_sevice_current_cms_proc(pCrData, pClientSDP, eCallType, pClientSDPParam->start_time, pClientSDPParam->end_time, pClientSDPParam->play_time, pCalleeGBLogicDeviceInfo->record_type, record_type);

        if (EV9000_CMS_ERR_INVITE_CALLEE_RECORD_NOT_COMPLETE_ERROR == i)
        {
            i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_ANSWER);
            SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_WARNING, "�ϼ�CMS������ʵʱ��Ƶ����:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, �ȴ��߼��豸¼��ҵ�����̽���", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);
            return 0;
        }
        else if (EV9000_CMS_ERR_INVITE_CALLEE_VIDEO_NOT_COMPLETE_ERROR == i)
        {
            i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_ANSWER);
            SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_WARNING, "�ϼ�CMS������ʵʱ��Ƶ����:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, �ȴ��߼��豸��Ƶҵ�����̽���", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);
            return 0;
        }
        else if (0 != i)
        {
            memset(strErrorCode, 0, 32);
            snprintf(strErrorCode, 32, "%d", i);
            SIP_AnswerToInvite(ua_dialog_index, 503, strErrorCode);
            //SIP_AnswerToInvite(ua_dialog_index, 503, (char*)"Current CMS Proc Error");
            i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
            i = call_record_remove(cr_pos);

            if (0 != i)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_sub_video_msg_proc() call_record_remove Error:cr_pos=%d, i=%d \r\n", cr_pos, i);
            }
            else
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_invite_sub_video_msg_proc() call_record_remove OK:cr_pos=%d, i=%d \r\n", cr_pos, i);
            }
        }
    }

    return i;
}

/*****************************************************************************
 �� �� ��  : route_invite_route_video_msg_proc
 ��������  : �ϼ�����CMS���͹�����INVITE��Ƶ��Ϣ����
 �������  : route_info_t* pRouteInfo
             sdp_message_t* pClientSDP
             sdp_param_t* pClientSDPParam
             GBLogicDevice_info_t* pCalleeGBLogicDeviceInfo
             char* caller_id
             char* callee_id
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
int route_invite_route_video_msg_proc(route_info_t* pRouteInfo, sdp_message_t* pClientSDP, sdp_param_t* pClientSDPParam, GBLogicDevice_info_t* pCalleeGBLogicDeviceInfo, char* caller_id, char* callee_id, int ua_dialog_index)
{
    int i = 0;
    int cr_pos = -1;
    cr_t* pCrData = NULL;
    int iCalleeRoutePos = 0;
    route_info_t* pCalleeRouteInfo = NULL;

    char* sdp_url = NULL;
    char* sdp_ssrc = NULL;
    char* o_name = NULL;
    char* o_sess_id = NULL;
    char* o_sess_version = NULL;
    char* time_r_repeat = NULL;
    call_type_t eCallType = CALL_TYPE_NULL;
    int stream_type = 0;
    int record_type = 0;
    transfer_protocol_type_t trans_type = TRANSFER_PROTOCOL_NULL;

    char strStartTime[64] = {0};
    char strEndTime[64] = {0};
    char strPlayBackURL[64] = {0};
    int iPlaybackTimeGap = 0;
    char strErrorCode[32] = {0};

    if (NULL == pRouteInfo)
    {
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_INVITE_CALLER_ROUTE_INFO_ERROR);
        SIP_AnswerToInvite(ua_dialog_index, 503, strErrorCode);
        //SIP_AnswerToInvite(ua_dialog_index, 503, (char*)"Route Info Error");
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_route_video_msg_proc() exit---: Route Info Error \r\n");
        return -1;
    }

    if (NULL == pClientSDP)
    {
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_INVITE_CALLER_SDP_MSG_ERROR);
        SIP_AnswerToInvite(ua_dialog_index, 503, strErrorCode);
        //SIP_AnswerToInvite(ua_dialog_index, 503, (char*)"Client SDP Info Error");
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_route_video_msg_proc() exit---: Client SDP Info Error \r\n");
        return -1;
    }

    if (NULL == pClientSDPParam)
    {
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_INVITE_CALLER_SDP_PARAM_ERROR);
        SIP_AnswerToInvite(ua_dialog_index, 503, strErrorCode);
        //SIP_AnswerToInvite(ua_dialog_index, 503, (char*)"Client SDP Param Info Error");
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_route_video_msg_proc() exit---: Client SDP Param Info Error \r\n");
        return -1;
    }

    if (NULL == pCalleeGBLogicDeviceInfo)
    {
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_INVITE_CALLEE_LOGIC_DEVICE_INFO_ERROR);
        SIP_AnswerToInvite(ua_dialog_index, 503, strErrorCode);
        //SIP_AnswerToInvite(ua_dialog_index, 503, (char*)"Callee GBLogic Device Info Error");
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_route_video_msg_proc() exit---: Callee GBLogic Device Info Error \r\n");
        return -1;
    }

    /* 1����ȡ�����ͣ����Ҷ�Ӧ�������豸 */
    if (pClientSDPParam->stream_type <= 0)
    {
        stream_type = EV9000_STREAM_TYPE_MASTER;
    }
    else
    {
        stream_type = pClientSDPParam->stream_type;
    }

    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_invite_route_video_msg_proc() Stream Type=%d \r\n", stream_type);

    if (stream_type == EV9000_STREAM_TYPE_SLAVE && pCalleeGBLogicDeviceInfo->stream_count == 1)
    {
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_INVITE_CALLEE_NOT_SUPPORT_MULTI_STREAM_ERROR);
        SIP_AnswerToInvite(ua_dialog_index, 503, strErrorCode);
        //SIP_AnswerToInvite(ua_dialog_index, 503, (char*)"Callee GBDevice Not Support Multi stream");
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_route_video_msg_proc() exit---: Callee GBDevice Not Support Multi stream \r\n");
        SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS������ʵʱ��Ƶ����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=%s ", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"�߼��豸��Ϣ��֧�ֶ�����");
        EnSystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "Real-time monitoring request from superior CMS failed:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, cause=%s ", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"Logic device information does not support multi stream");
        return -1;
    }

    /* 2����ȡ�������� */
    if (pClientSDPParam->trans_type <= 0)
    {
        trans_type = TRANSFER_PROTOCOL_UDP;
    }
    else if (pClientSDPParam->trans_type == 2)
    {
        trans_type = TRANSFER_PROTOCOL_TCP;
    }
    else
    {
        trans_type = TRANSFER_PROTOCOL_UDP;
    }

    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_invite_route_video_msg_proc() Transfer Protocol Type=%d \r\n", trans_type);

    /* ����Ŀ���ϼ�·����Ϣ */
    iCalleeRoutePos = route_info_find(pCalleeGBLogicDeviceInfo->cms_id);

    if (iCalleeRoutePos < 0)
    {
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_INVITE_CALLEE_ROUTE_INFO_ERROR);
        SIP_AnswerToInvite(ua_dialog_index, 503, strErrorCode);
        //SIP_AnswerToInvite(ua_dialog_index, 503, (char*)"Find Callee Route Info Error");
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_route_video_msg_proc() exit---: Find Callee Route Info Error \r\n");
        SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS������ʵʱ��Ƶ����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=%s, ý��������=%d", caller_id,  pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"�����߼��豸��Ӧ���ϼ�·����Ϣʧ��", stream_type);
        EnSystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "Real-time monitoring request from superior CMS failed:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, reason=%s, Media stream type=%d", caller_id,  pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"Find the route information which corresponding to logical device failed.", stream_type);
        return -1;
    }

    pCalleeRouteInfo = route_info_get(iCalleeRoutePos);

    if (NULL == pCalleeRouteInfo)
    {
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_INVITE_CALLEE_ROUTE_INFO_ERROR);
        SIP_AnswerToInvite(ua_dialog_index, 503, strErrorCode);
        //SIP_AnswerToInvite(ua_dialog_index, 503, (char*)"Get Callee Route Info Error");
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_route_video_msg_proc() exit---: Get Callee Route Info Error \r\n");
        SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS������ʵʱ��Ƶ����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=%s, ý��������=%d", caller_id,  pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"��ȡ�߼��豸��Ӧ���ϼ�·����Ϣʧ��", stream_type);
        EnSystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "Real-time monitoring request from superior CMS failed:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, reason=%s, Media stream type=%d", caller_id,  pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"Get the route information which corresponding to logical device failed.", stream_type);
        return -1;
    }

    /* ����ǵ�����ƽ̨��ȥ����չ��SDP��Ϣ */
    if (1 == pCalleeRouteInfo->three_party_flag)
    {
        DelSDPMediaAttributeByName(pClientSDP, (char*)"recordtype");
        DelSDPMediaAttributeByName(pClientSDP, (char*)"streamtype");

        /* �޸����� */
        o_name = sdp_message_o_username_get(pClientSDP);

        if (NULL != o_name && 0 != sstrcmp(o_name, local_cms_id_get()))
        {
            osip_free(o_name);
            o_name = NULL;

            o_name = osip_getcopy(local_cms_id_get());
            sdp_message_o_username_set(pClientSDP, o_name);
        }
        else if (NULL == o_name)
        {
            o_name = osip_getcopy(local_cms_id_get());
            sdp_message_o_username_set(pClientSDP, o_name);
        }

        o_sess_id = sdp_message_o_sess_id_get(pClientSDP);

        if (NULL != o_sess_id && 0 != sstrcmp(o_sess_id, (char*)"0"))
        {
            osip_free(o_sess_id);
            o_sess_id = NULL;

            o_sess_id = osip_getcopy((char*)"0");
            sdp_message_o_sess_id_set(pClientSDP, o_sess_id);
        }
        else if (NULL == o_sess_id)
        {
            o_sess_id = osip_getcopy(local_cms_id_get());
            sdp_message_o_sess_id_set(pClientSDP, o_sess_id);
        }

        o_sess_version = sdp_message_o_sess_version_get(pClientSDP);

        if (NULL != o_sess_version && 0 != sstrcmp(o_sess_version, (char*)"0"))
        {
            osip_free(o_sess_version);
            o_sess_version = NULL;

            o_sess_version = osip_getcopy((char*)"0");
            sdp_message_o_sess_version_set(pClientSDP, o_sess_version);
        }
        else if (NULL == o_sess_version)
        {
            o_sess_version = osip_getcopy(local_cms_id_get());
            sdp_message_o_sess_version_set(pClientSDP, o_sess_version);
        }

        time_r_repeat = sdp_message_r_repeat_get(pClientSDP, 0, 0);

        if (NULL != time_r_repeat)
        {
            DelSDPTimeRepeatInfo(pClientSDP);
        }
    }

    if (0 == strncmp(pClientSDPParam->s_name, (char*)"Playback", 8))
    {
        eCallType = CALL_TYPE_RECORD_PLAY;

        sdp_url = sdp_message_u_uri_get(pClientSDP);

        if (NULL == sdp_url)
        {
            snprintf(strPlayBackURL, 64, "%s:%s", callee_id, (char*)"application");
            sdp_url = osip_getcopy(strPlayBackURL);
            sdp_message_u_uri_set(pClientSDP, sdp_url);
        }
        else
        {
            osip_free(sdp_url);
            sdp_url = NULL;

            snprintf(strPlayBackURL, 64, "%s:%s", callee_id, (char*)"application");
            sdp_url = osip_getcopy(strPlayBackURL);
            sdp_message_u_uri_set(pClientSDP, sdp_url);
        }

#if 1
        sdp_ssrc = sdp_message_y_ssrc_get(pClientSDP);

        if (NULL == sdp_ssrc)
        {
            sdp_ssrc = osip_getcopy((char*)"1100000001");
            sdp_message_y_ssrc_set(pClientSDP, sdp_ssrc);
        }

#endif

        /* ȷ��¼������ */
        if (pClientSDPParam->record_type <= 0)
        {
            record_type = EV9000_RECORD_TYPE_NORMAL;
        }
        else
        {
            record_type = pClientSDPParam->record_type;
        }

        i = format_time(pClientSDPParam->start_time, strStartTime);
        i = format_time(pClientSDPParam->end_time, strEndTime);

#if 0

        /* �����ǰ�˵�NVR����DVR¼�����޸Ĳ���ʱ�� */
        if (EV9000_DEVICETYPE_DVR == pCalleeGBDeviceInfo->device_type || EV9000_DEVICETYPE_NVR == pCalleeGBDeviceInfo->device_type)
        {
            if (1 == pCalleeGBLogicDeviceInfo->record_type)
            {
                i = ModifySDPRecordPlayTime(pClientSDP);
                iPlaybackTimeGap = pClientSDPParam->play_time - pClientSDPParam->start_time;
                DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_invite_route_video_msg_proc() PlaybackTimeGap=%d \r\n", iPlaybackTimeGap);
            }
        }
        else if (EV9000_DEVICETYPE_SIPSERVER == pCalleeGBDeviceInfo->device_type && 1 == pCalleeGBDeviceInfo->three_party_flag) /* ������ƽ̨ */
        {
            i = ModifySDPRecordPlayTime(pClientSDP);
            iPlaybackTimeGap = pClientSDPParam->play_time - pClientSDPParam->start_time;
            DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_invite_route_video_msg_proc() PlaybackTimeGap=%d \r\n", iPlaybackTimeGap);
        }

#endif
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS��������ʷ��Ƶ�ط�����:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ¼������=%d, ý��������=%d, ���䷽ʽ=%d, �طſ�ʼʱ��=%s, ����ʱ��=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, record_type, stream_type, trans_type, strStartTime, strEndTime);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Historical video playback request from superior CMS:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, cause=%s, media stream type=%d, transmission method=%d, playback start time=%s, end time=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, stream_type, trans_type, strStartTime, strEndTime);
    }
    else if (0 == strncmp(pClientSDPParam->s_name, (char*)"Play", 4))
    {
        eCallType = CALL_TYPE_REALTIME;

#if 1
        sdp_ssrc = sdp_message_y_ssrc_get(pClientSDP);

        if (NULL == sdp_ssrc)
        {
            sdp_ssrc = osip_getcopy((char*)"0100000001");
            sdp_message_y_ssrc_set(pClientSDP, sdp_ssrc);
        }

#endif

        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS������ʵʱ��Ƶ����, �ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ý��������=%d, ���䷽ʽ=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, stream_type, trans_type);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Real-time monitoring request from superior CMS, superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, cause=%s, media stream type=%d, transmission method=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, stream_type, trans_type);
    }
    else if (0 == strncmp(pClientSDPParam->s_name, (char*)"Download", 8))
    {
        eCallType = CALL_TYPE_DOWNLOAD;

        sdp_url = sdp_message_u_uri_get(pClientSDP);

        if (NULL == sdp_url)
        {
            snprintf(strPlayBackURL, 64, "%s:%s", callee_id, (char*)"application");
            sdp_url = osip_getcopy(strPlayBackURL);
            sdp_message_u_uri_set(pClientSDP, sdp_url);
        }
        else
        {
            osip_free(sdp_url);
            sdp_url = NULL;

            snprintf(strPlayBackURL, 64, "%s:%s", callee_id, (char*)"application");
            sdp_url = osip_getcopy(strPlayBackURL);
            sdp_message_u_uri_set(pClientSDP, sdp_url);
        }

#if 1
        sdp_ssrc = sdp_message_y_ssrc_get(pClientSDP);

        if (NULL == sdp_ssrc)
        {
            sdp_ssrc = osip_getcopy((char*)"1100000001");
            sdp_message_y_ssrc_set(pClientSDP, sdp_ssrc);
        }

#endif

        /* ȷ��¼������ */
        if (pClientSDPParam->record_type <= 0)
        {
            record_type = EV9000_RECORD_TYPE_NORMAL;
        }
        else
        {
            record_type = pClientSDPParam->record_type;
        }

        i = format_time(pClientSDPParam->start_time, strStartTime);
        i = format_time(pClientSDPParam->end_time, strEndTime);

#if 0

        /* �����ǰ�˵�NVR����DVR¼�����޸Ĳ���ʱ�� */
        if (EV9000_DEVICETYPE_DVR == pCalleeGBDeviceInfo->device_type || EV9000_DEVICETYPE_NVR == pCalleeGBDeviceInfo->device_type)
        {
            if (1 == pCalleeGBLogicDeviceInfo->record_type)
            {
                i = ModifySDPRecordPlayTime(pClientSDP);
                iPlaybackTimeGap = pClientSDPParam->play_time - pClientSDPParam->start_time;
                DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_invite_route_video_msg_proc() PlaybackTimeGap=%d \r\n", iPlaybackTimeGap);
            }
        }
        else if (EV9000_DEVICETYPE_SIPSERVER == pCalleeGBDeviceInfo->device_type && 1 == pCalleeGBDeviceInfo->three_party_flag) /* ������ƽ̨ */
        {
            i = ModifySDPRecordPlayTime(pClientSDP);
            iPlaybackTimeGap = pClientSDPParam->play_time - pClientSDPParam->start_time;
            DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_invite_route_video_msg_proc() PlaybackTimeGap=%d \r\n", iPlaybackTimeGap);
        }

#endif
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS��������ʷ��Ƶ�ļ���������:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ¼������=%d, ý��������=%d, ���䷽ʽ=%d, �طſ�ʼʱ��=%s, ����ʱ��=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, record_type, stream_type, trans_type, strStartTime, strEndTime);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Historical video file download request from superior CMS:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, cause=%s, media stream type=%d, transmission method=%d, playback start time=%s, end time=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, stream_type, trans_type, strStartTime, strEndTime);
    }
    else
    {
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_SDP_NOT_SUPPORT_S_TYPE_ERROR);
        SIP_AnswerToInvite(ua_dialog_index, 488, strErrorCode);
        //SIP_AnswerToInvite(ua_dialog_index, 488, (char*)"SDP S Type Not Support");
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_route_video_msg_proc() exit---: SDP S Type Not Support \r\n");
        SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS������ʵʱ��Ƶ����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=%s, S����=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"��֧�ֵ�S��������", pClientSDPParam->s_name);
        EnSystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "Real-time monitoring request from superior CMS failed:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, cause=%s, S name=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"do not support S name sype", pClientSDPParam->s_name);
        return -1;
    }

    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_invite_route_video_msg_proc() Call Type=%d \r\n", eCallType);

    /* ʵʱ��Ƶ�鿴��Ҫ�ж��豸����״̬ */
    if (eCallType == CALL_TYPE_REALTIME)
    {
        /* �������豸�Ƿ����� */
        if (0 == pCalleeRouteInfo->reg_status)
        {
            memset(strErrorCode, 0, 32);
            snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_DEVICE_OFFLINE_ERROR);
            SIP_AnswerToInvite(ua_dialog_index, 480, strErrorCode);
            //SIP_AnswerToInvite(ua_dialog_index, 480, NULL);
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_route_video_msg_proc() exit---: Device Not Online:device_id=%s \r\n", pCalleeRouteInfo->server_id);
            SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS������ʵʱ��Ƶ����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=%s, �����豸ID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"ǰ�������豸������", pCalleeRouteInfo->server_id);
            EnSystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "Real-time monitoring request from superior CMS failed:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, cause=%s, physical deviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"Front-end physical device not online", pCalleeRouteInfo->server_id);
            return -1;
        }

#if 0

        /* ���߼��豸��λ״̬ */
        if (0 == pCalleeGBLogicDeviceInfo->status)
        {
            memset(strErrorCode, 0, 32);
            snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_DEVICE_LOGIC_DEVICE_OFFLINE_ERROR);
            SIP_AnswerToInvite(ua_dialog_index, 480, strErrorCode);
            //SIP_AnswerToInvite(ua_dialog_index, 480, NULL);
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_route_video_msg_proc() exit---: GBLogic Device Not Online \r\n");
            SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS������ʵʱ��Ƶ����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"ǰ���߼��豸������");
            EnSystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "Real-time monitoring request from superior CMS failed:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, cause=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"Front-end logic device is not online");
            return -1;
        }

        if (2 == pCalleeGBLogicDeviceInfo->status)
        {
            SIP_AnswerToInvite(ua_dialog_index, 480, NULL);
            DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_invite_msg_proc() exit---: GBLogic Device No Stream \r\n");
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS������ʵʱ��Ƶ����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"ǰ���߼��豸û������");
            return -1;
        }

#endif

        if (3 == pCalleeGBLogicDeviceInfo->status)
        {
            memset(strErrorCode, 0, 32);
            snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_DEVICE_LOGIC_DEVICE_UNREACHED_ERROR);
            SIP_AnswerToInvite(ua_dialog_index, 503, strErrorCode);
            //SIP_AnswerToInvite(ua_dialog_index, 480, NULL);
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_route_video_msg_proc() exit---: GBLogic Device NetWork UnReached \r\n");
            SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS������ʵʱ��Ƶ����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"ǰ���߼��豸���粻�ɴ�");
            EnSystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "Real-time monitoring request from superior CMS failed:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, cause=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"Front-end logic device network unaccessable");
            return -1;
        }

        /* ������ƽ̨, �鿴ԭ���Ƿ���ҵ������У���رյ� */
        if (pRouteInfo->three_party_flag && 1 == g_RouteMediaTransferFlag)
        {
            i = StopRouteService(pRouteInfo, pCalleeGBLogicDeviceInfo->device_id, stream_type);

            if (i == -2)
            {
                /* û������ */
            }
            else if (i < 0 && i != -2)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR,  "route_invite_route_video_msg_proc() StopRouteService Error:DeviceID=%s, stream_type=%d \r\n", pCalleeGBLogicDeviceInfo->device_id, stream_type);
            }
            else
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "�ϼ�CMS������ʵʱ��Ƶ����, �رյ���λԭ�е�ʵʱ��Ƶҵ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ��������=%d, i=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, stream_type, i);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Real-time monitoring request from superior CMS, close old service:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, stream type=%s, i=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, stream_type, i);

                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE,  "route_invite_route_video_msg_proc() StopRouteService OK:DeviceID=%s, stream_type=%d \r\n", pCalleeGBLogicDeviceInfo->device_id, stream_type);
            }
        }
    }

    /* 3�����������Դ */
    cr_pos = call_record_add();
    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_invite_route_video_msg_proc() call_record_add:cr_pos=%d \r\n", cr_pos);

    pCrData = call_record_get(cr_pos);

    if (NULL == pCrData)
    {
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_INVITE_GET_IDLE_CR_DATA_ERROR);
        SIP_AnswerToInvite(ua_dialog_index, 503, strErrorCode);
        //SIP_AnswerToInvite(ua_dialog_index, 503, (char*)"Get Call Record Data Error");
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_route_video_msg_proc() exit---: Get Call Record Data Error \r\n");
        SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS������ʵʱ��Ƶ����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"���������Դʧ��");
        EnSystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "Real-time monitoring request from superior CMS failed:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, cause=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"apply to call resource failed");
        return -1;
    }

    pCrData->call_type = eCallType;

    /* 4����� ��������Ϣ��������Դ��Ϣ */
    /* ���ж���Ϣ */
    osip_strncpy(pCrData->caller_id, caller_id, MAX_ID_LEN);
    osip_strncpy(pCrData->caller_ip, pRouteInfo->server_ip, MAX_IP_LEN);
    pCrData->caller_port = pRouteInfo->server_port;
    pCrData->caller_ua_index = ua_dialog_index;
    osip_strncpy(pCrData->caller_server_ip_ethname, pRouteInfo->strRegLocalEthName, MAX_IP_LEN);
    osip_strncpy(pCrData->caller_server_ip, pRouteInfo->strRegLocalIP, MAX_IP_LEN);
    pCrData->caller_server_port = pRouteInfo->iRegLocalPort;
    pCrData->caller_transfer_type = trans_type;
    pCrData->iPlaybackTimeGap = iPlaybackTimeGap;

    /* ���ж˵�SDP��Ϣ */
    osip_strncpy(pCrData->caller_sdp_ip, pClientSDPParam->sdp_ip, MAX_IP_LEN);
    pCrData->caller_sdp_port = pClientSDPParam->video_port;

#if 0
    printf("\r\n\r\n ************************************************* \r\n");
    printf(" route_invite_route_video_msg_proc() pCrData->caller_id=%s \r\n", pCrData->caller_id);
    printf(" route_invite_route_video_msg_proc() pCrData->caller_ip=%s \r\n", pCrData->caller_ip);
    printf(" route_invite_route_video_msg_proc() pCrData->caller_port=%d \r\n", pCrData->caller_port);
    printf(" route_invite_route_video_msg_proc() pCrData->caller_sdp_ip=%s \r\n", pCrData->caller_sdp_ip);
    printf(" route_invite_route_video_msg_proc() pCrData->caller_sdp_port=%d \r\n", pCrData->caller_sdp_port);
    printf(" route_invite_route_video_msg_proc() pCrData->caller_ua_index=%d \r\n", pCrData->caller_ua_index);
    printf(" ************************************************* \r\n\r\n ");
#endif

    /* ���ж���Ϣ */
    osip_strncpy(pCrData->callee_id, callee_id, MAX_ID_LEN);
    osip_strncpy(pCrData->callee_ip, pCalleeRouteInfo->server_ip, MAX_IP_LEN);
    pCrData->callee_port = pCalleeRouteInfo->server_port;
    osip_strncpy(pCrData->callee_server_ip_ethname, pCalleeRouteInfo->strRegLocalEthName, MAX_IP_LEN);
    osip_strncpy(pCrData->callee_server_ip, pCalleeRouteInfo->strRegLocalIP, MAX_IP_LEN);
    pCrData->callee_server_port = pCalleeRouteInfo->iRegLocalPort;
    pCrData->callee_framerate = pCalleeGBLogicDeviceInfo->frame_count;
    pCrData->callee_stream_type = stream_type;
    pCrData->callee_gb_device_type = EV9000_DEVICETYPE_SIPSERVER;

    if (1 == pCalleeRouteInfo->trans_protocol)
    {
        pCrData->callee_transfer_type = TRANSFER_PROTOCOL_TCP;
    }
    else
    {
        pCrData->callee_transfer_type = TRANSFER_PROTOCOL_UDP; /* Ĭ��UDP */
    }

#if 0
    printf("\r\n\r\n ************************************************* \r\n");
    printf(" route_invite_route_video_msg_proc() pCrData->callee_id=%s \r\n", pCrData->callee_id);
    printf(" route_invite_route_video_msg_proc() pCrData->callee_ip=%s \r\n", pCrData->callee_ip);
    printf(" route_invite_route_video_msg_proc() pCrData->callee_port=%d \r\n", pCrData->callee_port);
    printf(" ************************************************* \r\n\r\n ");
#endif

    i = user_video_sevice_sub_cms_proc(pCrData, pClientSDP, eCallType);

    if (EV9000_CMS_ERR_INVITE_CALLEE_RECORD_NOT_COMPLETE_ERROR == i)
    {
        i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_ANSWER);
        SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_WARNING, "�ϼ�CMS������ʵʱ��Ƶ����:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, �ȴ��߼��豸¼��ҵ�����̽���", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);
        return 0;
    }
    else if (EV9000_CMS_ERR_INVITE_CALLEE_VIDEO_NOT_COMPLETE_ERROR == i)
    {
        i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_ANSWER);
        SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_WARNING, "�ϼ�CMS������ʵʱ��Ƶ����:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, �ȴ��߼��豸��Ƶҵ�����̽���", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);
        return 0;
    }
    else if (0 != i)
    {
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", i);
        SIP_AnswerToInvite(ua_dialog_index, 503, strErrorCode);
        //SIP_AnswerToInvite(ua_dialog_index, 503, (char*)"Route CMS Proc Error");
        i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
        i = call_record_remove(cr_pos);

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_route_video_msg_proc() call_record_remove Error:cr_pos=%d, i=%d \r\n", cr_pos, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_invite_route_video_msg_proc() call_record_remove OK:cr_pos=%d, i=%d \r\n", cr_pos, i);
        }
    }

    return i;
}

/*****************************************************************************
 �� �� ��  : route_invite_audio_msg_proc
 ��������  : �ϼ�����CMS���͹�����INVITE��Ƶ�Խ���Ϣ����
 �������  : route_info_t* pRouteInfo
             sdp_message_t* pClientSDP
             sdp_param_t* pClientSDPParam
             GBLogicDevice_info_t* pCalleeGBLogicDeviceInfo
             char* caller_id
             char* callee_id
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
int route_invite_audio_msg_proc(route_info_t* pRouteInfo, sdp_message_t* pClientSDP, sdp_param_t* pClientSDPParam, GBLogicDevice_info_t* pCalleeGBLogicDeviceInfo, char* caller_id, char* callee_id, int ua_dialog_index)
{
    int i = 0;
    int cr_pos = -1;
    cr_t* pCrData = NULL;

    GBDevice_info_t* pCalleeGBDeviceInfo = NULL;

    //int iAuth = 0;
    //char* realm = NULL;
    //osip_authorization_t* pAuthorization = NULL;
    //char* sdp_ssrc = NULL;
    //char strSDPUrl[128] = {0};
    //int iSessionExpires = 0;
    call_type_t eCallType = CALL_TYPE_NULL;
    char strErrorCode[32] = {0};

    if (NULL == pRouteInfo)
    {
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_INVITE_CALLER_ROUTE_INFO_ERROR);
        SIP_AnswerToInvite(ua_dialog_index, 503, strErrorCode);
        //SIP_AnswerToInvite(ua_dialog_index, 503, (char*)"Route Info Error");
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_audio_msg_proc() exit---: Route Info Error \r\n");
        return -1;
    }

    if (NULL == pClientSDP)
    {
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_INVITE_CALLER_SDP_MSG_ERROR);
        SIP_AnswerToInvite(ua_dialog_index, 503, strErrorCode);
        //SIP_AnswerToInvite(ua_dialog_index, 503, (char*)"Client SDP Info Error");
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_audio_msg_proc() exit---: Client SDP Info Error \r\n");
        return -1;
    }

    if (NULL == pClientSDPParam)
    {
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_INVITE_CALLER_SDP_PARAM_ERROR);
        SIP_AnswerToInvite(ua_dialog_index, 503, strErrorCode);
        //SIP_AnswerToInvite(ua_dialog_index, 503, (char*)"Client SDP Param Info Error");
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_audio_msg_proc() exit---: Client SDP Param Info Error \r\n");
        return -1;
    }

    if (NULL == pCalleeGBLogicDeviceInfo)
    {
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_INVITE_CALLEE_LOGIC_DEVICE_INFO_ERROR);
        SIP_AnswerToInvite(ua_dialog_index, 503, strErrorCode);
        //SIP_AnswerToInvite(ua_dialog_index, 503, (char*)"Callee GBLogic Device Info Error");
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_audio_msg_proc() exit---: Callee GBLogic Device Info Error \r\n");
        return -1;
    }

    /* 1�����Ҷ�Ӧ�������豸 */
    pCalleeGBDeviceInfo = GBDevice_info_get_by_stream_type(pCalleeGBLogicDeviceInfo, EV9000_STREAM_TYPE_MASTER);

    if (NULL == pCalleeGBDeviceInfo)
    {
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_INVITE_CALLEE_DEVICE_INFO_ERROR);
        SIP_AnswerToInvite(ua_dialog_index, 503, strErrorCode);
        //SIP_AnswerToInvite(ua_dialog_index, 503, (char*)"Get Callee GBDevice Info Error");
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_audio_msg_proc() exit---: Get Callee GBDevice Info Error \r\n");
        SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS������ʵʱ��Ƶ�Խ�����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"��ȡ�߼��豸��Ӧ�������豸��Ϣʧ��");
        EnSystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "Real time audio intercom request from superior CMS failed:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, cause=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"Access logic device corresponded physical device info failed");
        return -1;
    }

    /* Ŀǰ�����Խ�ֻ֧��ʵʱ�Խ� */
    if (0 == strncmp(pClientSDPParam->s_name, "Play", 4))
    {
        eCallType = CALL_TYPE_AUDIO;
    }
    else
    {
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_SDP_NOT_SUPPORT_S_TYPE_ERROR);
        SIP_AnswerToInvite(ua_dialog_index, 488, strErrorCode);
        //SIP_AnswerToInvite(ua_dialog_index, 488, (char*)"SDP S Type Not Support");
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_audio_msg_proc() exit---: SDP S Type Not Support \r\n");
        SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS������ʵʱ��Ƶ�Խ�����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=%s, S����=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"��֧�ֵ�S��������", pClientSDPParam->s_name);
        EnSystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "Real time audio intercom request from superior CMS failed:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, cause=%s, S name=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"Do not support S name type", pClientSDPParam->s_name);
        return -1;
    }

    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_invite_audio_msg_proc() Call Type=%d \r\n", eCallType);

    /* 2.�������豸�Ƿ����� */
    if (0 == pCalleeGBDeviceInfo->reg_status)
    {
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_DEVICE_OFFLINE_ERROR);
        SIP_AnswerToInvite(ua_dialog_index, 480, strErrorCode);
        //SIP_AnswerToInvite(ua_dialog_index, 480, NULL);
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_audio_msg_proc() exit---: Device Not Online:device_id=%s \r\n", pCalleeGBDeviceInfo->device_id);
        SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS������ʵʱ��Ƶ�Խ�����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=%s, �����豸ID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"ǰ�������豸������", pCalleeGBDeviceInfo->device_id);
        EnSystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "Real time audio intercom request from superior CMS failed:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, cause=%s, physical deviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"Front-end physical device not online", pCalleeGBDeviceInfo->device_id);
        return -1;
    }

    /* ���߼��豸��λ״̬, ������¼�CMS�ĵ㣬����Ҫ�ж� */
    if (EV9000_DEVICETYPE_SIPSERVER != pCalleeGBDeviceInfo->device_type && 0 == pCalleeGBLogicDeviceInfo->status)
    {
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_DEVICE_LOGIC_DEVICE_OFFLINE_ERROR);
        SIP_AnswerToInvite(ua_dialog_index, 480, strErrorCode);
        //SIP_AnswerToInvite(ua_dialog_index, 480, NULL);
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_audio_msg_proc() exit---: GBLogic Device Not Online \r\n");
        SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS������ʵʱ��Ƶ�Խ�����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"ǰ���߼��豸������");
        EnSystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "Real time audio intercom request from superior CMS failed:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, cause=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"Front-end logic device is not online");
        return -1;
    }

    if (3 == pCalleeGBLogicDeviceInfo->status)
    {
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_DEVICE_LOGIC_DEVICE_UNREACHED_ERROR);
        SIP_AnswerToInvite(ua_dialog_index, 503, strErrorCode);
        //SIP_AnswerToInvite(ua_dialog_index, 480, NULL);
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_audio_msg_proc() exit---: GBLogic Device NetWork UnReached \r\n");
        SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS������ʵʱ��Ƶ�Խ�����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"ǰ���߼��豸���粻�ɴ�");
        EnSystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "Real time audio intercom request from superior CMS failed:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, cause=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"Front-end logic device network unaccessable");
        return -1;
    }

    /* 3�����������Դ */
    cr_pos = call_record_add();
    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_invite_audio_msg_proc() call_record_add:cr_pos=%d \r\n", cr_pos);

    pCrData = call_record_get(cr_pos);

    if (NULL == pCrData)
    {
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_INVITE_GET_IDLE_CR_DATA_ERROR);
        SIP_AnswerToInvite(ua_dialog_index, 503, strErrorCode);
        //SIP_AnswerToInvite(ua_dialog_index, 503, (char*)"Get Call Record Data Error");
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_audio_msg_proc() exit---: Get Call Record Data Error \r\n");
        SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS������ʵʱ��Ƶ�Խ�����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"���������Դʧ��");
        EnSystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "Real time audio intercom request from superior CMS failed:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, cause=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"Apply to call resource failed");
        return -1;
    }

    pCrData->call_type = eCallType;

    /* 4����� ��������Ϣ��������Դ��Ϣ */
    /* ���ж���Ϣ */
    osip_strncpy(pCrData->caller_id, caller_id, MAX_ID_LEN);
    osip_strncpy(pCrData->caller_ip, pRouteInfo->server_ip, MAX_IP_LEN);
    pCrData->caller_port = pRouteInfo->server_port;
    pCrData->caller_ua_index = ua_dialog_index;
    osip_strncpy(pCrData->caller_server_ip_ethname, pRouteInfo->strRegLocalEthName, MAX_IP_LEN);
    osip_strncpy(pCrData->caller_server_ip, pRouteInfo->strRegLocalIP, MAX_IP_LEN);
    pCrData->caller_server_port = pRouteInfo->iRegLocalPort;
    pCrData->caller_transfer_type = TRANSFER_PROTOCOL_UDP; /* ��Ĭ��ֵ */

    /* ���ж˵�SDP��Ϣ */
    osip_strncpy(pCrData->caller_sdp_ip, pClientSDPParam->sdp_ip, MAX_IP_LEN);
    pCrData->caller_sdp_port = pClientSDPParam->audio_port;

#if 0
    printf("\r\n\r\n ************************************************* \r\n");
    printf(" route_invite_audio_msg_proc() pCrData->caller_id=%s \r\n", pCrData->caller_id);
    printf(" route_invite_audio_msg_proc() pCrData->caller_ip=%s \r\n", pCrData->caller_ip);
    printf(" route_invite_audio_msg_proc() pCrData->caller_port=%d \r\n", pCrData->caller_port);
    printf(" route_invite_audio_msg_proc() pCrData->caller_sdp_ip=%s \r\n", pCrData->caller_sdp_ip);
    printf(" route_invite_audio_msg_proc() pCrData->caller_sdp_port=%d \r\n", pCrData->caller_sdp_port);
    printf(" route_invite_audio_msg_proc() pCrData->caller_ua_index=%d \r\n", pCrData->caller_ua_index);
    printf(" ************************************************* \r\n\r\n ");
#endif

    /* ���ж���Ϣ */
    osip_strncpy(pCrData->callee_id, callee_id, MAX_ID_LEN);
    osip_strncpy(pCrData->callee_ip, pCalleeGBDeviceInfo->login_ip, MAX_IP_LEN);
    pCrData->callee_port = pCalleeGBDeviceInfo->login_port;
    osip_strncpy(pCrData->callee_server_ip_ethname, pCalleeGBDeviceInfo->strRegServerEthName, MAX_IP_LEN);
    osip_strncpy(pCrData->callee_server_ip, pCalleeGBDeviceInfo->strRegServerIP, MAX_IP_LEN);
    pCrData->callee_server_port = pCalleeGBDeviceInfo->iRegServerPort;
    pCrData->callee_framerate = pCalleeGBLogicDeviceInfo->frame_count;
    pCrData->callee_stream_type = EV9000_STREAM_TYPE_MASTER;
    pCrData->callee_transfer_type = TRANSFER_PROTOCOL_UDP; /* ��Ĭ��ֵ */
    pCrData->callee_gb_device_type = pCalleeGBDeviceInfo->device_type;

#if 0
    printf("\r\n\r\n ************************************************* \r\n");
    printf(" route_invite_audio_msg_proc() pCrData->callee_id=%s \r\n", pCrData->callee_id);
    printf(" route_invite_audio_msg_proc() pCrData->callee_ip=%s \r\n", pCrData->callee_ip);
    printf(" route_invite_audio_msg_proc() pCrData->callee_port=%d \r\n", pCrData->callee_port);
    printf(" ************************************************* \r\n\r\n ");
#endif

    /* 5��ת����Ϣ��ǰ�� */
    i = user_transfer_invite_to_dest_for_audio_proc(pCrData, pClientSDP);

    if (0 != i)
    {
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_INVITE_TRANSFER_MSG_TO_DEST_ERROR);
        SIP_AnswerToInvite(ua_dialog_index, 503, strErrorCode);
        //SIP_AnswerToInvite(ua_dialog_index, 503, (char*)"Transfer Invite Message Proc Error");
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_audio_msg_proc() exit---: Transfer Invite Message Proc Error \r\n");
        i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
        i = call_record_remove(cr_pos);

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_audio_msg_proc() call_record_remove Error:cr_pos=%d, i=%d \r\n", cr_pos, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_invite_audio_msg_proc() call_record_remove OK:cr_pos=%d, i=%d \r\n", cr_pos, i);
        }
    }

    return i;
}

/*****************************************************************************
 �� �� ��  : route_invite_response_msg_proc
 ��������  : �ϼ�����CMS���͹�����INVITE ��Ӧ��Ϣ����
 �������  : int cr_pos
             int ua_dialog_index
             int response_code
             char* reasonphrase
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
int route_invite_response_msg_proc(int cr_pos, int ua_dialog_index, int response_code, char* reasonphrase, char* msg_body, int msg_body_len)
{
    int i = 0;
    cr_t* pCrData = NULL;
    sdp_message_t* pRemoteSDP = NULL;
    sdp_param_t stRemoteSDPParam;
    sdp_extend_param_t stRemoteSDPExParam;
    GBLogicDevice_info_t* pCalleeGBLogicDeviceInfo = NULL;
    char strErrorCode[32] = {0};

    if (cr_pos < 0)
    {
        if (200 == response_code)
        {
            i = SIP_SendAck(ua_dialog_index);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_response_msg_proc() SIP_SendAck:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);

            i = SIP_SendBye(ua_dialog_index);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_response_msg_proc() SIP_SendBye:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);
        }

        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_response_msg_proc() exit---: Param Error \r\n");
        return -1;
    }

    /* invite����Ӧһ���ɱ��з��͹���*/
    pCrData = call_record_get(cr_pos);

    if (NULL == pCrData)
    {
        if (200 == response_code)
        {
            i = SIP_SendAck(ua_dialog_index);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_response_msg_proc() SIP_SendAck:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);

            i = SIP_SendBye(ua_dialog_index);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_response_msg_proc() SIP_SendBye:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);
        }

        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_response_msg_proc() exit---: Get Call Record Error \r\n");
        return -1;
    }

    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_invite_response_msg_proc():caller_id=%s, caller_ip=%s, callee_id=%s, callee_ip=%s \r\n", pCrData->caller_id, pCrData->caller_ip, pCrData->callee_id, pCrData->callee_ip);
    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS������ʵʱ��Ƶ����, INVITE��Ӧ��Ϣ����:�ϼ�CMS ID=%s, IP��ַ=%s, �߼��豸ID=%s, IP��ַ=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->callee_id, pCrData->callee_ip);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Real-time monitoring request from superior CMS, INVITE response message process:superior CMS ID=%s, IPaddress=%s, logic device ID=%s, IP address=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->callee_id, pCrData->callee_ip);

    /* �����߼��豸��Ϣ */
    pCalleeGBLogicDeviceInfo = GBLogicDevice_info_find(pCrData->callee_id);

    if (NULL == pCalleeGBLogicDeviceInfo)
    {
        SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS������ʵʱ��Ƶ����, INVITE��Ӧ��Ϣ����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, (char*)"��ȡǰ���߼��豸��Ϣʧ��");
        EnSystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "Real-time monitoring request from superior CMS, INVITE response message process failed:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, cause=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, (char*)"Access front-end logical device information failed");

        if (200 == response_code)
        {
            /* ��Ӧ��Ϣ������ */
            i = SIP_SendAck(ua_dialog_index);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_response_msg_proc() SIP_SendAck:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);

            i = SIP_SendBye(ua_dialog_index);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_response_msg_proc() SIP_SendBye:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);

            /* ��Ӧ��Ϣ������ */
            memset(strErrorCode, 0, 32);
            snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_INVITE_CALLEE_LOGIC_DEVICE_INFO_ERROR);
            SIP_AnswerToInvite(pCrData->caller_ua_index, 503, strErrorCode);
            //SIP_AnswerToInvite(pCrData->caller_ua_index, 503, (char*)"Get Callee GBlogicDevice Info Error");
        }

        i = return_error_for_wait_answer_call_record(pCrData, EV9000_CMS_ERR_INVITE_CALLEE_LOGIC_DEVICE_INFO_ERROR);

        i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
        i = call_record_remove(cr_pos);

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_response_msg_proc() call_record_remove Error:cr_pos=%d, i=%d \r\n", cr_pos, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_invite_response_msg_proc() call_record_remove OK:cr_pos=%d, i=%d \r\n", cr_pos, i);
        }

        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_response_msg_proc() exit---: Get Callee GBlogicDevice Info Error \r\n");
        return -1;
    }

    /* ������Ӧ������ͬ�Ĵ��� */
    if (200 == response_code)
    {
        if (NULL == msg_body || msg_body_len == 0)
        {
            SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS������ʵʱ��Ƶ����, INVITE��Ӧ��Ϣ����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, (char*)"��Զ˵�SDP��Ϣʧ��");
            EnSystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "Real-time monitoring request from superior CMS, INVITE response message process failed:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, cause=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, (char*)"Access opposite end SDP info failed");

            /* ��Ӧ��Ϣ������ */
            i = SIP_SendAck(ua_dialog_index);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_response_msg_proc() SIP_SendAck:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);

            i = SIP_SendBye(ua_dialog_index);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_response_msg_proc() SIP_SendBye:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);

            /* ��Ӧ��Ϣ������ */
            memset(strErrorCode, 0, 32);
            snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_INVITE_CALLEE_MSG_BODY_ERROR);
            SIP_AnswerToInvite(pCrData->caller_ua_index, 503, strErrorCode);

            i = return_error_for_wait_answer_call_record(pCrData, EV9000_CMS_ERR_INVITE_CALLEE_MSG_BODY_ERROR);

            i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
            i = call_record_remove(cr_pos);

            if (0 != i)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_response_msg_proc() call_record_remove Error:cr_pos=%d, i=%d \r\n", cr_pos, i);
            }
            else
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_invite_response_msg_proc() call_record_remove OK:cr_pos=%d, i=%d \r\n", cr_pos, i);
            }

            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_response_msg_proc() exit---: Get Remote Message SDP Body Error \r\n");
            return -1;
        }

        /* ��ȡ200��Ϣ�еı��е�sdp��Ϣ */
        i = sdp_message_init(&pRemoteSDP);

        if (0 != i)
        {
            SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS������ʵʱ��Ƶ����, INVITE��Ӧ��Ϣ����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, (char*)"SDP��ʼ��ʧ��");
            EnSystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "Real-time monitoring request from superior CMS, INVITE response message process failed:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, cause=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, (char*)"SDP Initialization failed ");

            /* ��Ӧ��Ϣ������ */
            i = SIP_SendAck(ua_dialog_index);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_response_msg_proc() SIP_SendAck:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);

            i = SIP_SendBye(ua_dialog_index);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_response_msg_proc() SIP_SendBye:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);

            /* ��Ӧ��Ϣ������ */
            memset(strErrorCode, 0, 32);
            snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_SDP_MSG_INIT_ERROR);
            SIP_AnswerToInvite(pCrData->caller_ua_index, 503, strErrorCode);

            i = return_error_for_wait_answer_call_record(pCrData, EV9000_CMS_ERR_SDP_MSG_INIT_ERROR);

            i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
            i = call_record_remove(cr_pos);

            if (0 != i)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_response_msg_proc() call_record_remove Error:cr_pos=%d, i=%d \r\n", cr_pos, i);
            }
            else
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_invite_response_msg_proc() call_record_remove OK:cr_pos=%d, i=%d \r\n", cr_pos, i);
            }

            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_response_msg_proc() exit---: Remote SDP Message Init Error \r\n");
            return -1;
        }

        /* ����SDP��Ϣ */
        i = sdp_message_parse(pRemoteSDP, msg_body);

        if (0 != i)
        {
            SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS������ʵʱ��Ƶ����, INVITE��Ӧ��Ϣ����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, (char*)"SDP��Ϣ����ʧ��");
            EnSystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "Real-time monitoring request from superior CMS, INVITE response message process failed:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, cause=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, (char*)"SDP info analysis failed");

            /* ��Ӧ��Ϣ������ */
            i = SIP_SendAck(ua_dialog_index);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_response_msg_proc() SIP_SendAck:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);

            i = SIP_SendBye(ua_dialog_index);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_response_msg_proc() SIP_SendBye:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);

            /* ��Ӧ��Ϣ������ */
            memset(strErrorCode, 0, 32);
            snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_SDP_MSG_PARSE_ERROR);
            SIP_AnswerToInvite(pCrData->caller_ua_index, 503, strErrorCode);

            i = return_error_for_wait_answer_call_record(pCrData, EV9000_CMS_ERR_SDP_MSG_PARSE_ERROR);

            i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
            i = call_record_remove(cr_pos);

            if (0 != i)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_response_msg_proc() call_record_remove Error:cr_pos=%d, i=%d \r\n", cr_pos, i);
            }
            else
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_invite_response_msg_proc() call_record_remove OK:cr_pos=%d, i=%d \r\n", cr_pos, i);
            }

            sdp_message_free(pRemoteSDP);
            pRemoteSDP = NULL;

            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_response_msg_proc() exit---: Remote SDP Message Parse Error \r\n");
            return -1;
        }

        /* ��ȡЭ�̵�SDP��Ϣ */
        memset(&stRemoteSDPParam, 0, sizeof(sdp_param_t));
        memset(&stRemoteSDPExParam, 0, sizeof(sdp_extend_param_t));

        i = SIP_GetSDPInfoEx(pRemoteSDP, &stRemoteSDPParam, &stRemoteSDPExParam);

        if (0 != i)
        {
            SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS������ʵʱ��Ƶ����, INVITE��Ӧ��Ϣ����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, (char*)"��ȡSDP�е���Ϣʧ��");
            EnSystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "Real-time monitoring request from superior CMS, INVITE response message process failed:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, cause=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, (char*)"Access info in SDP failed");

            /* ��Ӧ��Ϣ������ */
            i = SIP_SendAck(ua_dialog_index);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_response_msg_proc() SIP_SendAck:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);

            i = SIP_SendBye(ua_dialog_index);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_response_msg_proc() SIP_SendBye:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);

            /* ��Ӧ��Ϣ������ */
            memset(strErrorCode, 0, 32);
            snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_SDP_GET_VIDEO_INFO_ERROR);
            SIP_AnswerToInvite(pCrData->caller_ua_index, 503, strErrorCode);

            i = return_error_for_wait_answer_call_record(pCrData, EV9000_CMS_ERR_SDP_GET_VIDEO_INFO_ERROR);

            i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
            i = call_record_remove(cr_pos);

            if (0 != i)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_response_msg_proc() call_record_remove Error:cr_pos=%d, i=%d \r\n", cr_pos, i);
            }
            else
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_invite_response_msg_proc() call_record_remove OK:cr_pos=%d, i=%d \r\n", cr_pos, i);
            }

            sdp_message_free(pRemoteSDP);
            pRemoteSDP = NULL;

            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_response_msg_proc() exit---: Get Remote SDP Video Info Error \r\n");
            return -1;
        }

        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_response_msg_proc() addr=%s,port=%d,code=%d,flag=%d \r\n", stRemoteSDPParam.sdp_ip, stRemoteSDPParam.video_port, stRemoteSDPParam.video_code_type, stRemoteSDPParam.media_direction);

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

            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_response_msg_proc() callee_onvif_url=%s \r\n", pCrData->callee_onvif_url);

            /* ���в��Э�����͸�ΪRTSP */
            //pCrData->callee_transfer_type = TRANSFER_PROTOCOL_RTSP;
        }

        /* ��ӱ��е�SDP ��Ϣ */
        osip_strncpy(pCrData->callee_sdp_ip, stRemoteSDPParam.sdp_ip, MAX_IP_LEN);

#if 0
        printf("\r\n\r\n ************************************************* \r\n");
        printf(" route_invite_response_msg_proc() pCrData->callee_sdp_ip=%s \r\n", pCrData->callee_sdp_ip);
        printf(" route_invite_response_msg_proc() pCrData->callee_sdp_port=%d \r\n", pCrData->callee_sdp_port);
        printf(" route_invite_response_msg_proc() pCrData->tsu_code=%d \r\n", pCrData->tsu_code);
        printf(" ************************************************* \r\n\r\n ");
#endif

        if (CALL_TYPE_AUDIO == pCrData->call_type)
        {
            pCrData->callee_sdp_port = stRemoteSDPParam.audio_port;
            pCrData->tsu_code = stRemoteSDPParam.audio_code_type;

            i = route_invite_audio_response_msg_proc(cr_pos, ua_dialog_index, pCrData, pRemoteSDP, pCalleeGBLogicDeviceInfo);
        }
        else
        {
            pCrData->callee_sdp_port = stRemoteSDPParam.video_port;
            pCrData->tsu_code = stRemoteSDPParam.video_code_type;

            i = route_invite_video_response_msg_proc(cr_pos, ua_dialog_index, pCrData, pRemoteSDP, pCalleeGBLogicDeviceInfo);
        }

        if (i != 0)
        {
            i = return_error_for_wait_answer_call_record(pCrData, i);

            i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
            i = call_record_remove(cr_pos);

            if (0 != i)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_response_msg_proc() call_record_remove Error:cr_pos=%d, i=%d \r\n", cr_pos, i);
            }
            else
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_invite_response_msg_proc() call_record_remove OK:cr_pos=%d, i=%d \r\n", cr_pos, i);
            }

            sdp_message_free(pRemoteSDP);
            pRemoteSDP = NULL;

            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_response_msg_proc() exit---: Accept Invite Error \r\n");
            return -1;
        }

        /* ֪ͨ�ȴ��ĺ������񣬽�������֪ͨTSUת������ */
        i = resumed_wait_answer_call_record1(pCrData);

        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS������ʵʱ��Ƶ����, INVITE��Ӧ��Ϣ����ɹ�:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Superior CMS real-time video request, respond the INVITE message processing success: the higher the CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id);
        sdp_message_free(pRemoteSDP);
        pRemoteSDP = NULL;
    }
    else
    {
        SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS������ʵʱ��Ƶ����, INVITE��Ӧ��Ϣ����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=%s, ������=%d, reasonphrase=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, (char*)"��200�Ĵ�����Ӧ��Ϣ", response_code, reasonphrase);
        EnSystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "Real-time monitoring request from superior CMS, INVITE response message process failed:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, cause=%s, error code=%d", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, (char*)"Non 200 fault response message", response_code);

        if (EV9000_DEVICETYPE_SIPSERVER == pCrData->callee_gb_device_type)/* �������¼�CMS���ص��о������ԭ�� */
        {
            if (NULL != reasonphrase && reasonphrase[0] != '\0')
            {
                SIP_AnswerToInvite(pCrData->caller_ua_index, response_code, reasonphrase);
                i = return_error_for_wait_answer_call_record(pCrData, response_code);
            }
            else
            {
                memset(strErrorCode, 0, 32);
                snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_INVITE_FRONT_RETURN_ERROR);
                SIP_AnswerToInvite(pCrData->caller_ua_index, response_code, strErrorCode);
                i = return_error_for_wait_answer_call_record(pCrData, EV9000_CMS_ERR_INVITE_FRONT_RETURN_ERROR);
            }
        }
        else
        {
            memset(strErrorCode, 0, 32);
            snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_INVITE_FRONT_RETURN_ERROR);
            SIP_AnswerToInvite(pCrData->caller_ua_index, response_code, strErrorCode);
            i = return_error_for_wait_answer_call_record(pCrData, EV9000_CMS_ERR_INVITE_FRONT_RETURN_ERROR);
        }
    }

    /* 4������Ǵ����Ӧ���Ƴ�������Ϣ */
    if (response_code >= 400)
    {
        i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
        i = call_record_remove(cr_pos);

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_response_msg_proc() call_record_remove Error:cr_pos=%d, i=%d \r\n", cr_pos, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_invite_response_msg_proc() call_record_remove OK:cr_pos=%d, i=%d \r\n", cr_pos, i);
        }
    }

    return 0;
}

/*****************************************************************************
 �� �� ��  : route_invite_video_response_msg_proc
 ��������  : �ϼ�����CMS���͹�����INVITE ��Ƶ��Ӧ��Ϣ����
 �������  : int cr_pos
             int ua_dialog_index
             cr_t* pCrData
             sdp_message_t* pRemoteSDP
             GBLogicDevice_info_t* pCalleeGBLogicDeviceInfo
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��17��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int route_invite_video_response_msg_proc(int cr_pos, int ua_dialog_index, cr_t* pCrData, sdp_message_t* pRemoteSDP, GBLogicDevice_info_t* pCalleeGBLogicDeviceInfo)
{
    int i = 0;

    /* �����߼��豸����������жϣ�������Ϣ���� */
    if (1 == pCalleeGBLogicDeviceInfo->other_realm)
    {
        i = route_invite_video_route_response_msg_proc(cr_pos, ua_dialog_index, pCrData, pRemoteSDP, pCalleeGBLogicDeviceInfo);
    }
    else
    {
        i = route_invite_video_sub_response_msg_proc(cr_pos, ua_dialog_index, pCrData, pRemoteSDP, pCalleeGBLogicDeviceInfo);
    }

    return i;
}

/*****************************************************************************
 �� �� ��  : route_invite_video_sub_response_msg_proc
 ��������  : �ϼ�����CMS���͹�����INVITE ��Ƶ��Ӧ��Ϣ����
 �������  : int cr_pos
             int ua_dialog_index
             cr_t* pCrData
             sdp_message_t* pRemoteSDP
             GBLogicDevice_info_t* pCalleeGBLogicDeviceInfo
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��17��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int route_invite_video_sub_response_msg_proc(int cr_pos, int ua_dialog_index, cr_t* pCrData, sdp_message_t* pRemoteSDP, GBLogicDevice_info_t* pCalleeGBLogicDeviceInfo)
{
    int i = 0;
    int send_port = 0;
    char* sdp_tsu_ip = NULL;
    char strErrorCode[32] = {0};

    if (!g_LocalMediaTransferFlag
        && EV9000_DEVICETYPE_SIPSERVER == pCrData->callee_gb_device_type)
    {
        /* ����ǿ缶CMS�ĵ�λ���Ҳ���Ҫ����TSUת��������ֱ��ת����Ϣ */
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS������ʵʱ��Ƶ����, INVITE��Ӧ��Ϣ����:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ý��������������ת��, ֱ��ת����Ϣ��ý��������", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Superior CMS real-time video request, respond the INVITE message processing: the higher the CMS, ID = % s = % s IP address and port number = % d, logical device ID = % s, stream without forward at the corresponding level, the forward message directly to the requesting party media flow", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id);
    }
    else
    {
        if (EV9000_DEVICETYPE_SIPSERVER == pCrData->callee_gb_device_type
            && (pCrData->call_type == CALL_TYPE_RECORD_PLAY || pCrData->call_type == CALL_TYPE_DOWNLOAD)) /* �¼�CMS ������ */
        {
            //���Ͷ˿ںŴ��»�ȡ
            /* ��ȡTSU ���Ͷ˿ں� */
            send_port = get_send_port_by_tsu_resource(pCrData->tsu_ip);

            if (send_port <= 0)
            {
                SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS������ʵʱ��Ƶ����, INVITE��Ӧ��Ϣ����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=%s, tsu_ip=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, (char*)"��ȡTSU�ķ��Ͷ˿ں�ʧ��", pCrData->tsu_ip);
                EnSystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "Real-time monitoring request from superior CMS, INVITE response message process failed:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, cause=%s, tsu_ip=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, (char*)"Access TSU send port number failed", pCrData->tsu_ip);

                /* ��Ӧ��Ϣ������ */
                i = SIP_SendAck(ua_dialog_index);
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_video_sub_response_msg_proc() SIP_SendAck:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);

                i = SIP_SendBye(ua_dialog_index);
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_video_sub_response_msg_proc() SIP_SendBye:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);

                /* ��Ӧ��Ϣ������ */
                memset(strErrorCode, 0, 32);
                snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_TSU_GET_SEND_PORT_ERROR);
                SIP_AnswerToInvite(pCrData->caller_ua_index, 503, strErrorCode);
                //SIP_AnswerToInvite(pCrData->caller_ua_index, 503, (char*)"Get TSU Send Port rror");

                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_video_sub_response_msg_proc() exit---: Get TSU Send Port Error:tsu_ip=%s \r\n", pCrData->tsu_ip);
                return EV9000_CMS_ERR_TSU_GET_SEND_PORT_ERROR;
            }

            pCrData->tsu_send_port = send_port;

            /* ֪ͨTSU��ʼת������ */
            i = notify_tsu_add_transfer_for_replay_task(pCrData, 0, pCrData->callee_stream_type);
        }
        else if (pCalleeGBLogicDeviceInfo->record_type == 1 && (pCrData->call_type == CALL_TYPE_RECORD_PLAY || pCrData->call_type == CALL_TYPE_DOWNLOAD)) /* ����ǰ��¼�� */
        {
            //���Ͷ˿ںŴ��»�ȡ
            /* ��ȡTSU ���Ͷ˿ں� */
            send_port = get_send_port_by_tsu_resource(pCrData->tsu_ip);

            if (send_port <= 0)
            {
                SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS������ʵʱ��Ƶ����, INVITE��Ӧ��Ϣ����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=%s, tsu_ip=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, (char*)"��ȡTSU�ķ��Ͷ˿ں�ʧ��", pCrData->tsu_ip);
                EnSystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "Real-time monitoring request from superior CMS, INVITE response message process failed:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, cause=%s, tsu_ip=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, (char*)"Access TSU send port number failed", pCrData->tsu_ip);

                /* ��Ӧ��Ϣ������ */
                i = SIP_SendAck(ua_dialog_index);
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_video_sub_response_msg_proc() SIP_SendAck:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);

                i = SIP_SendBye(ua_dialog_index);
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_video_sub_response_msg_proc() SIP_SendBye:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);

                /* ��Ӧ��Ϣ������ */
                memset(strErrorCode, 0, 32);
                snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_TSU_GET_SEND_PORT_ERROR);
                SIP_AnswerToInvite(pCrData->caller_ua_index, 503, strErrorCode);
                //SIP_AnswerToInvite(pCrData->caller_ua_index, 503, (char*)"Get TSU Send Port rror");

                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_video_sub_response_msg_proc() exit---: Get TSU Send Port Error:tsu_ip=%s \r\n", pCrData->tsu_ip);
                return EV9000_CMS_ERR_TSU_GET_SEND_PORT_ERROR;
            }

            pCrData->tsu_send_port = send_port;

            /* ֪ͨTSU��ʼת������ */
            i = notify_tsu_add_transfer_for_replay_task(pCrData, 0, pCrData->callee_stream_type);
        }
        else
        {
            if (TRANSFER_PROTOCOL_TCP == pCrData->caller_transfer_type) /* TCP������£�ֱ�ӵ���TSUת���ӿڣ�����ֵ��TSU�ķ��Ͷ˿ں� */
            {
                /* ֪ͨTSU��ʼת������ */
                i = notify_tsu_add_transfer_task(pCrData, pCrData->callee_service_type, pCrData->callee_stream_type);

                if (i > 0)
                {
                    pCrData->tsu_send_port = i;
                    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_invite_video_sub_response_msg_proc() tsu_send_port=%d \r\n", pCrData->tsu_send_port);
                }
                else
                {
                    DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_video_sub_response_msg_proc() tsu_send_port=%d \r\n", pCrData->tsu_send_port);
                }
            }
            else
            {
                //���Ͷ˿ںŴ��»�ȡ
                /* ��ȡTSU ���Ͷ˿ں� */
                send_port = get_send_port_by_tsu_resource(pCrData->tsu_ip);

                if (send_port <= 0)
                {
                    SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS������ʵʱ��Ƶ����, INVITE��Ӧ��Ϣ����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=%s, tsu_ip=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, (char*)"��ȡTSU�ķ��Ͷ˿ں�ʧ��", pCrData->tsu_ip);
                    EnSystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "Real-time monitoring request from superior CMS, INVITE response message process failed:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, cause=%s, tsu_ip=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, (char*)"Access TSU send port number failed", pCrData->tsu_ip);

                    /* ��Ӧ��Ϣ������ */
                    i = SIP_SendAck(ua_dialog_index);
                    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_video_sub_response_msg_proc() SIP_SendAck:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);

                    i = SIP_SendBye(ua_dialog_index);
                    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_video_sub_response_msg_proc() SIP_SendBye:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);

                    /* ��Ӧ��Ϣ������ */
                    memset(strErrorCode, 0, 32);
                    snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_TSU_GET_SEND_PORT_ERROR);
                    SIP_AnswerToInvite(pCrData->caller_ua_index, 503, strErrorCode);
                    //SIP_AnswerToInvite(pCrData->caller_ua_index, 503, (char*)"Get TSU Send Port rror");

                    DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_video_sub_response_msg_proc() exit---: Get TSU Send Port Error:tsu_ip=%s \r\n", pCrData->tsu_ip);
                    return EV9000_CMS_ERR_TSU_GET_SEND_PORT_ERROR;
                }

                pCrData->tsu_send_port = send_port;

                /* ֪ͨTSU��ʼת������ */
                i = notify_tsu_add_transfer_task(pCrData, 0, pCrData->callee_stream_type);
            }
        }

        if (i < 0)
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_video_sub_response_msg_proc() exit---: notify_tsu_add_transfer_task Error: TSU IP=%s, i=%d \r\n", pCrData->tsu_ip, i);
            SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS������ʵʱ��Ƶ����, INVITE��Ӧ��Ϣ����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=%s, TSU IP=%s, iRet=%d", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, (char*)"֪ͨTSU���ת������ʧ��", pCrData->tsu_ip, i);
            EnSystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "Real-time monitoring request from superior CMS, INVITE response message process failed:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, cause=%s, TSU IP=%s, iRet=%d", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, (char*)"Notify TSU to add forwarding task failed.", pCrData->tsu_ip, i);

            /* ��Ӧ��Ϣ������ */
            i = SIP_SendAck(ua_dialog_index);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_video_sub_response_msg_proc() SIP_SendAck:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);

            i = SIP_SendBye(ua_dialog_index);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_video_sub_response_msg_proc() SIP_SendBye:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);

            /* ��Ӧ��Ϣ������ */
            i = SIP_SendBye(pCrData->caller_ua_index);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_video_sub_response_msg_proc() SIP_SendBye:caller_ua_index=%d, i=%d \r\n", pCrData->caller_ua_index, i);

            return EV9000_CMS_ERR_TSU_NOTIFY_ADD_TRANSFER_ERROR;
        }
        else
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS������ʵʱ��Ƶ����, INVITE��Ӧ��Ϣ����:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ֪ͨTSU���ת������ɹ�, TSU IP=%s, task_id=%s, iRet=%d", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, pCrData->tsu_ip, pCrData->task_id, i);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Superior CMS real-time video request, respond the INVITE message processing: the higher the CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s, notify the TSU add forward task successfully, TSU IP=%s, task_id=%s, iRet=%d", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, pCrData->tsu_ip, pCrData->task_id, i);
        }

        /* �齨����SDP��Ϣ*/
        sdp_tsu_ip = get_cr_sdp_tsu_ip(pCrData, pCrData->caller_server_ip_ethname);

        if (NULL == sdp_tsu_ip)
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_video_sub_response_msg_proc() exit---: Get Caller TSU SDP IP Error:callee_server_ip_ethname=%s\r\n", pCrData->caller_server_ip_ethname);
            SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS������ʵʱ��Ƶ����, INVITE��Ӧ��Ϣ����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=%s, callee_server_ip_ethname=%s, iRet=%d", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, (char*)"��ȡ���в��SDP��Ϣ�е�TSU��IP��ַʧ��", pCrData->caller_server_ip_ethname, i);
            EnSystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "Real-time monitoring request from superior CMS, INVITE response message process failed: superior CMS ID=%s, IPaddress=%s, port number=%d, logic deviceID=%s, cause=%s, callee_server_ip_ethname=%s, iRet=%d", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, (char*)"Access TSU IP address from caller side SDP.", pCrData->caller_server_ip_ethname, i);

            /* ֪ͨTSUֹͣת������ */
            i = notify_tsu_delete_transfer_task(pCrData->tsu_ip, pCrData->task_id);

            /* ��Ӧ��Ϣ������ */
            i = SIP_SendAck(ua_dialog_index);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_video_sub_response_msg_proc() SIP_SendAck:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);

            i = SIP_SendBye(ua_dialog_index);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_video_sub_response_msg_proc() SIP_SendBye:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);

            /* ��Ӧ��Ϣ������ */
            memset(strErrorCode, 0, 32);
            snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_TSU_GET_CALLER_TSU_IP_ERROR);
            SIP_AnswerToInvite(pCrData->caller_ua_index, 503, strErrorCode);
            //SIP_AnswerToInvite(pCrData->caller_ua_index, 503, (char*)"Modify SDP S Name Error");

            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_video_sub_response_msg_proc() exit---: Get sdp_tsu_ip Error \r\n");

            return EV9000_CMS_ERR_TSU_GET_CALLER_TSU_IP_ERROR;
        }

        /* �޸�SDP�е�S Name */
        i = ModifySDPSName(pRemoteSDP, pCrData->call_type);

        if (i != 0)
        {
            SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS������ʵʱ��Ƶ����, INVITE��Ӧ��Ϣ����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, (char*)"�޸�SDP��S Nameʧ��");
            EnSystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "Real-time monitoring request from superior CMS, INVITE response message process failed:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, cause=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, (char*)"Edit S Name in SDP failed");

            /* ֪ͨTSUֹͣת������ */
            i = notify_tsu_delete_transfer_task(pCrData->tsu_ip, pCrData->task_id);

            /* ��Ӧ��Ϣ������ */
            i = SIP_SendAck(ua_dialog_index);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_video_sub_response_msg_proc() SIP_SendAck:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);

            i = SIP_SendBye(ua_dialog_index);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_video_sub_response_msg_proc() SIP_SendBye:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);

            /* ��Ӧ��Ϣ������ */
            memset(strErrorCode, 0, 32);
            snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_SDP_MODIFY_S_NAME_ERROR);
            SIP_AnswerToInvite(pCrData->caller_ua_index, 503, strErrorCode);
            //SIP_AnswerToInvite(pCrData->caller_ua_index, 503, (char*)"Modify SDP S Name Error");

            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_video_sub_response_msg_proc() exit---: Modify SDP S Name Error \r\n");
            return EV9000_CMS_ERR_SDP_MODIFY_S_NAME_ERROR;
        }

        /* �޸�SDP�е�ip��ַ�Ͷ˿ں�*/
        i = ModifySDPIPAndPort(pRemoteSDP, sdp_tsu_ip, pCrData->tsu_send_port);

        if (i != 0)
        {
            SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS������ʵʱ��Ƶ����, INVITE��Ӧ��Ϣ����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, (char*)"�޸�SDP�е�IP�Ͷ˿ں�ʧ��");
            EnSystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "Real-time monitoring request from superior CMS, INVITE response message process failed:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, cause=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, (char*)"Edit IP and port number in SDP failed");

            /* ֪ͨTSUֹͣת������ */
            i = notify_tsu_delete_transfer_task(pCrData->tsu_ip, pCrData->task_id);

            /* ��Ӧ��Ϣ������ */
            i = SIP_SendAck(ua_dialog_index);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_video_sub_response_msg_proc() SIP_SendAck:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);

            i = SIP_SendBye(ua_dialog_index);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_video_sub_response_msg_proc() SIP_SendBye:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);

            /* ��Ӧ��Ϣ������ */
            memset(strErrorCode, 0, 32);
            snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_SDP_MODIFY_IP_ERROR);
            SIP_AnswerToInvite(pCrData->caller_ua_index, 503, strErrorCode);
            //SIP_AnswerToInvite(pCrData->caller_ua_index, 503, (char*)"Modify SDP IP And Addr Error");

            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_video_sub_response_msg_proc() exit---: Modify SDP IP And Addr Error \r\n");
            return EV9000_CMS_ERR_SDP_MODIFY_IP_ERROR;
        }

        /* �޸�SDP�е�Э�鴫������ */
        if (pCrData->caller_transfer_type != pCrData->callee_transfer_type)
        {
            if (TRANSFER_PROTOCOL_TCP == pCrData->caller_transfer_type)
            {
                i = ModifySDPTransProtocol(pRemoteSDP, 1);
            }
            else
            {
                i = ModifySDPTransProtocol(pRemoteSDP, 0);
            }

            if (i != 0)
            {
                SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS������ʵʱ��Ƶ����, INVITE��Ӧ��Ϣ����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, (char*)"�޸�SDP�е�Э�鴫������ʧ��");
                EnSystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "Real-time monitoring request from superior CMS, INVITE response message process failed:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, cause=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, (char*)"Edit protocal transimission type in SDP failed");

                /* ֪ͨTSUֹͣת������ */
                i = notify_tsu_delete_transfer_task(pCrData->tsu_ip, pCrData->task_id);

                /* ��Ӧ��Ϣ������ */
                i = SIP_SendAck(ua_dialog_index);
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_video_sub_response_msg_proc() SIP_SendAck:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);

                i = SIP_SendBye(ua_dialog_index);
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_video_sub_response_msg_proc() SIP_SendBye:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);

                /* ��Ӧ��Ϣ������ */
                memset(strErrorCode, 0, 32);
                snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_SDP_MODIFY_PROTOCOL_ERROR);
                SIP_AnswerToInvite(pCrData->caller_ua_index, 503, strErrorCode);
                //SIP_AnswerToInvite(pCrData->caller_ua_index, 503, (char*)"Modify SDP Protocol Error");

                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_video_sub_response_msg_proc() exit---: Modify SDP Protocol Error \r\n");
                return EV9000_CMS_ERR_SDP_MODIFY_PROTOCOL_ERROR;
            }
        }

        /* ȥ��SDP�е�ONVIF URL������֮�䲻����RTSP */
        if (NULL != pRemoteSDP->u_uri)
        {
            osip_free(pRemoteSDP->u_uri);
            pRemoteSDP->u_uri = NULL;
        }
    }

    /* �������з��ĺ���*/
    i = SIP_AcceptInvite(pCrData->caller_ua_index, pRemoteSDP);

    if (i != 0)
    {
        SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS������ʵʱ��Ƶ����, INVITE��Ӧ��Ϣ����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, (char*)"���տͻ��˵�INVITE��Ϣʧ��");
        EnSystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "Real-time monitoring request from superior CMS, INVITE response message process failed:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, cause=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, (char*)"Receive INVITE message from client failed");

        if (!g_LocalMediaTransferFlag
            && EV9000_DEVICETYPE_SIPSERVER == pCrData->callee_gb_device_type)
        {
            /* ����ǿ缶CMS�ĵ�λ���Ҳ���Ҫ����TSUת��������ֱ��ת����Ϣ */
        }
        else
        {
            /* ֪ͨTSUֹͣת������ */
            i = notify_tsu_delete_transfer_task(pCrData->tsu_ip, pCrData->task_id);
        }

        /* ��Ӧ��Ϣ������ */
        i = SIP_SendAck(ua_dialog_index);
        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_video_sub_response_msg_proc() SIP_SendAck:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);

        i = SIP_SendBye(ua_dialog_index);
        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_video_sub_response_msg_proc() SIP_SendBye:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);

        /* ��Ӧ��Ϣ������ */
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_INVITE_ACCEPT_ERROR);
        SIP_AnswerToInvite(pCrData->caller_ua_index, 503, strErrorCode);
        //SIP_AnswerToInvite(pCrData->caller_ua_index, 503, (char*)"Accept Invite Error");

        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_video_sub_response_msg_proc() exit---: Accept Invite Error \r\n");
        return EV9000_CMS_ERR_INVITE_ACCEPT_ERROR;
    }

    if (!g_LocalMediaTransferFlag
        && EV9000_DEVICETYPE_SIPSERVER == pCrData->callee_gb_device_type)
    {
        /* ����ǿ缶CMS�ĵ�λ���Ҳ���Ҫ����TSUת��������ֱ��ת����Ϣ */
        i = SIP_SendAck(ua_dialog_index);
        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_video_sub_response_msg_proc() SIP_SendAck:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);
    }
    else
    {
        if (EV9000_DEVICETYPE_SIPSERVER == pCrData->callee_gb_device_type
            && (pCrData->call_type == CALL_TYPE_RECORD_PLAY || pCrData->call_type == CALL_TYPE_DOWNLOAD)) /* �¼�CMS ������ */
        {
            i = SIP_SendAck(ua_dialog_index);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_video_sub_response_msg_proc() SIP_SendAck:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);
        }
        else if (pCalleeGBLogicDeviceInfo->record_type == 1 && (pCrData->call_type == CALL_TYPE_RECORD_PLAY || pCrData->call_type == CALL_TYPE_DOWNLOAD)) /* ����ǰ��¼�� */
        {
            i = SIP_SendAck(ua_dialog_index);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_video_sub_response_msg_proc() SIP_SendAck:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);
        }
        else
        {
            /* ���������ACK ��Ϣ���� */
            if (ua_dialog_index == pCrData->caller_ua_index)
            {
                i = ack_send_use(cr_pos, ua_dialog_index, -1);
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_video_sub_response_msg_proc() ack_send_use:cr_pos=%d, caller_ua_index=%d, i=%d \r\n", cr_pos, ua_dialog_index, i);
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS������ʵʱ��Ƶ����, INVITE��Ӧ��Ϣ����, ��ӵȴ�TSU֪ͨ���񴴽������Ϣ�������¼�, �ȴ�����ACK��Ϣ:cr_pos=%d, caller_ua_index=%d, iRet=%d", cr_pos, ua_dialog_index, i);
            }
            else if (ua_dialog_index == pCrData->callee_ua_index)
            {
                i = ack_send_use(cr_pos, -1, ua_dialog_index);
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_video_sub_response_msg_proc() ack_send_use:cr_pos=%d, callee_ua_index=%d, i=%d \r\n", cr_pos, ua_dialog_index, i);
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS������ʵʱ��Ƶ����, INVITE��Ӧ��Ϣ����, ��ӵȴ�TSU֪ͨ���񴴽������Ϣ�������¼�, �ȴ�����ACK��Ϣ:cr_pos=%d, callee_ua_index=%d, iRet=%d", cr_pos, ua_dialog_index, i);
            }
        }
    }

    return 0;
}

/*****************************************************************************
 �� �� ��  : route_invite_video_route_response_msg_proc
 ��������  : �ϼ�����CMS���͹�����INVITE ��Ƶ��Ӧ��Ϣ����
 �������  : int cr_pos
             int ua_dialog_index
             cr_t* pCrData
             sdp_message_t* pRemoteSDP
             GBLogicDevice_info_t* pCalleeGBLogicDeviceInfo
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��17��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int route_invite_video_route_response_msg_proc(int cr_pos, int ua_dialog_index, cr_t* pCrData, sdp_message_t* pRemoteSDP, GBLogicDevice_info_t* pCalleeGBLogicDeviceInfo)
{
    int i = 0;
    int send_port = 0;
    char* sdp_tsu_ip = NULL;
    char strErrorCode[32] = {0};

    if (!g_LocalMediaTransferFlag)
    {
        /* ����ǿ缶CMS�ĵ�λ���Ҳ���Ҫ����TSUת��������ֱ��ת����Ϣ */
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS������ʵʱ��Ƶ����, INVITE��Ӧ��Ϣ����:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ý��������������ת��, ֱ��ת����Ϣ��ý��������", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Superior CMS real-time video request, respond the INVITE message processing: the higher the CMS, ID = % s = % s IP address and port number = % d, logical device ID = % s, stream without forward at the corresponding level, the forward message directly to the requesting party media flow", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id);
    }
    else
    {
        if (pCrData->call_type == CALL_TYPE_RECORD_PLAY || pCrData->call_type == CALL_TYPE_DOWNLOAD) /* ����ǰ��¼�� */
        {
            //���Ͷ˿ںŴ��»�ȡ
            /* ��ȡTSU ���Ͷ˿ں� */
            send_port = get_send_port_by_tsu_resource(pCrData->tsu_ip);

            if (send_port <= 0)
            {
                SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS������ʵʱ��Ƶ����, INVITE��Ӧ��Ϣ����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=%s, tsu_ip=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, (char*)"��ȡTSU�ķ��Ͷ˿ں�ʧ��", pCrData->tsu_ip);
                EnSystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "Real-time monitoring request from superior CMS, INVITE response message process failed:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, cause=%s, tsu_ip=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, (char*)"Access TSU send port number failed", pCrData->tsu_ip);

                /* ��Ӧ��Ϣ������ */
                i = SIP_SendAck(ua_dialog_index);
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_video_route_response_msg_proc() SIP_SendAck:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);

                i = SIP_SendBye(ua_dialog_index);
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_video_route_response_msg_proc() SIP_SendBye:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);

                /* ��Ӧ��Ϣ������ */
                memset(strErrorCode, 0, 32);
                snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_TSU_GET_SEND_PORT_ERROR);
                SIP_AnswerToInvite(pCrData->caller_ua_index, 503, strErrorCode);
                //SIP_AnswerToInvite(pCrData->caller_ua_index, 503, (char*)"Get TSU Send Port rror");

                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_video_route_response_msg_proc() exit---: Get TSU Send Port Error:tsu_ip=%s \r\n", pCrData->tsu_ip);
                return EV9000_CMS_ERR_TSU_GET_SEND_PORT_ERROR;
            }

            pCrData->tsu_send_port = send_port;

            /* ֪ͨTSU��ʼת������ */
            i = notify_tsu_add_transfer_for_replay_task(pCrData, 0, pCrData->callee_stream_type);
        }
        else
        {
            if (TRANSFER_PROTOCOL_TCP == pCrData->caller_transfer_type) /* TCP������£�ֱ�ӵ���TSUת���ӿڣ�����ֵ��TSU�ķ��Ͷ˿ں� */
            {
                /* ֪ͨTSU��ʼת������ */
                i = notify_tsu_add_transfer_task(pCrData, pCrData->callee_service_type, pCrData->callee_stream_type);

                if (i > 0)
                {
                    pCrData->tsu_send_port = i;
                    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_invite_video_route_response_msg_proc() tsu_send_port=%d \r\n", pCrData->tsu_send_port);
                }
                else
                {
                    DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_video_route_response_msg_proc() tsu_send_port=%d \r\n", pCrData->tsu_send_port);
                }
            }
            else
            {
                //���Ͷ˿ںŴ��»�ȡ
                /* ��ȡTSU ���Ͷ˿ں� */
                send_port = get_send_port_by_tsu_resource(pCrData->tsu_ip);

                if (send_port <= 0)
                {
                    SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS������ʵʱ��Ƶ����, INVITE��Ӧ��Ϣ����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=%s, tsu_ip=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, (char*)"��ȡTSU�ķ��Ͷ˿ں�ʧ��", pCrData->tsu_ip);
                    EnSystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "Real-time monitoring request from superior CMS, INVITE response message process failed:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, cause=%s, tsu_ip=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, (char*)"Access TSU send port number failed", pCrData->tsu_ip);

                    /* ��Ӧ��Ϣ������ */
                    i = SIP_SendAck(ua_dialog_index);
                    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_video_route_response_msg_proc() SIP_SendAck:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);

                    i = SIP_SendBye(ua_dialog_index);
                    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_video_route_response_msg_proc() SIP_SendBye:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);

                    /* ��Ӧ��Ϣ������ */
                    memset(strErrorCode, 0, 32);
                    snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_TSU_GET_SEND_PORT_ERROR);
                    SIP_AnswerToInvite(pCrData->caller_ua_index, 503, strErrorCode);
                    //SIP_AnswerToInvite(pCrData->caller_ua_index, 503, (char*)"Get TSU Send Port rror");

                    DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_video_route_response_msg_proc() exit---: Get TSU Send Port Error:tsu_ip=%s \r\n", pCrData->tsu_ip);
                    return EV9000_CMS_ERR_TSU_GET_SEND_PORT_ERROR;
                }

                pCrData->tsu_send_port = send_port;

                /* ֪ͨTSU��ʼת������ */
                i = notify_tsu_add_transfer_task(pCrData, 0, pCrData->callee_stream_type);
            }
        }

        if (i < 0)
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_video_route_response_msg_proc() exit---: notify_tsu_add_transfer_task Error: TSU IP=%s, i=%d \r\n", pCrData->tsu_ip, i);
            SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS������ʵʱ��Ƶ����, INVITE��Ӧ��Ϣ����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=%s, TSU IP=%s, iRet=%d", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, (char*)"֪ͨTSU���ת������ʧ��", pCrData->tsu_ip, i);
            EnSystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "Real-time monitoring request from superior CMS, INVITE response message process failed:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, cause=%s, TSU IP=%s, iRet=%d", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, (char*)"Notify TSU to add forwarding task failed.", pCrData->tsu_ip, i);

            /* ��Ӧ��Ϣ������ */
            i = SIP_SendAck(ua_dialog_index);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_video_route_response_msg_proc() SIP_SendAck:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);

            i = SIP_SendBye(ua_dialog_index);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_video_route_response_msg_proc() SIP_SendBye:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);

            /* ��Ӧ��Ϣ������ */
            i = SIP_SendBye(pCrData->caller_ua_index);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_video_route_response_msg_proc() SIP_SendBye:caller_ua_index=%d, i=%d \r\n", pCrData->caller_ua_index, i);

            return EV9000_CMS_ERR_TSU_NOTIFY_ADD_TRANSFER_ERROR;
        }
        else
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS������ʵʱ��Ƶ����, INVITE��Ӧ��Ϣ����:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ֪ͨTSU���ת������ɹ�, TSU IP=%s, task_id=%s, iRet=%d", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, pCrData->tsu_ip, pCrData->task_id, i);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Superior CMS real-time video request, respond the INVITE message processing: the higher the CMS, ID = % s = % s IP address and port number = % d, logical device ID = % s, notify the TSU add forward mission success, TSU IP = % s, task_id = % s, iRet = %d", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, pCrData->tsu_ip, pCrData->task_id, i);
        }

        /* �齨����SDP��Ϣ*/
        sdp_tsu_ip = get_cr_sdp_tsu_ip(pCrData, pCrData->caller_server_ip_ethname);

        if (NULL == sdp_tsu_ip)
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_video_route_response_msg_proc() exit---: Get Caller TSU SDP IP Error:callee_server_ip_ethname=%s\r\n", pCrData->caller_server_ip_ethname);
            SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS������ʵʱ��Ƶ����, INVITE��Ӧ��Ϣ����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=%s, callee_server_ip_ethname=%s, iRet=%d", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, (char*)"��ȡ���в��SDP��Ϣ�е�TSU��IP��ַʧ��", pCrData->caller_server_ip_ethname, i);
            EnSystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "Real-time monitoring request from superior CMS, INVITE response message process failed: superior CMS ID=%s, IPaddress=%s, port number=%d, logic deviceID=%s, cause=%s, callee_server_ip_ethname=%s, iRet=%d", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, (char*)"Access TSU IP address from caller side SDP.", pCrData->caller_server_ip_ethname, i);

            /* ֪ͨTSUֹͣת������ */
            i = notify_tsu_delete_transfer_task(pCrData->tsu_ip, pCrData->task_id);

            /* ��Ӧ��Ϣ������ */
            i = SIP_SendAck(ua_dialog_index);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_video_route_response_msg_proc() SIP_SendAck:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);

            i = SIP_SendBye(ua_dialog_index);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_video_route_response_msg_proc() SIP_SendBye:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);

            /* ��Ӧ��Ϣ������ */
            memset(strErrorCode, 0, 32);
            snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_TSU_GET_CALLER_TSU_IP_ERROR);
            SIP_AnswerToInvite(pCrData->caller_ua_index, 503, strErrorCode);
            //SIP_AnswerToInvite(pCrData->caller_ua_index, 503, (char*)"Modify SDP S Name Error");

            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_video_route_response_msg_proc() exit---: Get sdp_tsu_ip Error \r\n");
            return EV9000_CMS_ERR_TSU_GET_CALLER_TSU_IP_ERROR;
        }

        /* �޸�SDP�е�S Name */
        i = ModifySDPSName(pRemoteSDP, pCrData->call_type);

        if (i != 0)
        {
            SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS������ʵʱ��Ƶ����, INVITE��Ӧ��Ϣ����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, (char*)"�޸�SDP��S Nameʧ��");
            EnSystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "Real-time monitoring request from superior CMS, INVITE response message process failed:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, cause=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, (char*)"Edit S Name in SDP failed");

            /* ֪ͨTSUֹͣת������ */
            i = notify_tsu_delete_transfer_task(pCrData->tsu_ip, pCrData->task_id);

            /* ��Ӧ��Ϣ������ */
            i = SIP_SendAck(ua_dialog_index);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_video_route_response_msg_proc() SIP_SendAck:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);

            i = SIP_SendBye(ua_dialog_index);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_video_route_response_msg_proc() SIP_SendBye:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);

            /* ��Ӧ��Ϣ������ */
            memset(strErrorCode, 0, 32);
            snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_SDP_MODIFY_S_NAME_ERROR);
            SIP_AnswerToInvite(pCrData->caller_ua_index, 503, strErrorCode);
            //SIP_AnswerToInvite(pCrData->caller_ua_index, 503, (char*)"Modify SDP S Name Error");

            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_video_route_response_msg_proc() exit---: Modify SDP S Name Error \r\n");
            return EV9000_CMS_ERR_SDP_MODIFY_S_NAME_ERROR;
        }

        /* �޸�SDP�е�ip��ַ�Ͷ˿ں�*/
        i = ModifySDPIPAndPort(pRemoteSDP, sdp_tsu_ip, pCrData->tsu_send_port);

        if (i != 0)
        {
            SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS������ʵʱ��Ƶ����, INVITE��Ӧ��Ϣ����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, (char*)"�޸�SDP�е�IP�Ͷ˿ں�ʧ��");
            EnSystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "Real-time monitoring request from superior CMS, INVITE response message process failed:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, cause=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, (char*)"Edit IP and port number in SDP failed");

            /* ֪ͨTSUֹͣת������ */
            i = notify_tsu_delete_transfer_task(pCrData->tsu_ip, pCrData->task_id);

            /* ��Ӧ��Ϣ������ */
            i = SIP_SendAck(ua_dialog_index);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_video_route_response_msg_proc() SIP_SendAck:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);

            i = SIP_SendBye(ua_dialog_index);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_video_route_response_msg_proc() SIP_SendBye:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);

            /* ��Ӧ��Ϣ������ */
            memset(strErrorCode, 0, 32);
            snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_SDP_MODIFY_IP_ERROR);
            SIP_AnswerToInvite(pCrData->caller_ua_index, 503, strErrorCode);
            //SIP_AnswerToInvite(pCrData->caller_ua_index, 503, (char*)"Modify SDP IP And Addr Error");

            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_video_route_response_msg_proc() exit---: Modify SDP IP And Addr Error \r\n");
            return EV9000_CMS_ERR_SDP_MODIFY_IP_ERROR;
        }

        /* �޸�SDP�е�Э�鴫������ */
        if (pCrData->caller_transfer_type != pCrData->callee_transfer_type)
        {
            if (TRANSFER_PROTOCOL_TCP == pCrData->caller_transfer_type)
            {
                i = ModifySDPTransProtocol(pRemoteSDP, 1);
            }
            else
            {
                i = ModifySDPTransProtocol(pRemoteSDP, 0);
            }

            if (i != 0)
            {
                SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS������ʵʱ��Ƶ����, INVITE��Ӧ��Ϣ����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, (char*)"�޸�SDP�е�Э�鴫������ʧ��");
                EnSystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "Real-time monitoring request from superior CMS, INVITE response message process failed:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, cause=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, (char*)"Edit protocal transimission type in SDP failed");

                /* ֪ͨTSUֹͣת������ */
                i = notify_tsu_delete_transfer_task(pCrData->tsu_ip, pCrData->task_id);

                /* ��Ӧ��Ϣ������ */
                i = SIP_SendAck(ua_dialog_index);
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_video_route_response_msg_proc() SIP_SendAck:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);

                i = SIP_SendBye(ua_dialog_index);
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_video_route_response_msg_proc() SIP_SendBye:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);

                /* ��Ӧ��Ϣ������ */
                memset(strErrorCode, 0, 32);
                snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_SDP_MODIFY_PROTOCOL_ERROR);
                SIP_AnswerToInvite(pCrData->caller_ua_index, 503, strErrorCode);
                //SIP_AnswerToInvite(pCrData->caller_ua_index, 503, (char*)"Modify SDP Protocol Error");

                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_video_route_response_msg_proc() exit---: Modify SDP Protocol Error \r\n");
                return EV9000_CMS_ERR_SDP_MODIFY_PROTOCOL_ERROR;
            }
        }

        /* ȥ��SDP�е�ONVIF URL������֮�䲻����RTSP */
        if (NULL != pRemoteSDP->u_uri)
        {
            osip_free(pRemoteSDP->u_uri);
            pRemoteSDP->u_uri = NULL;
        }
    }

    /* �������з��ĺ���*/
    i = SIP_AcceptInvite(pCrData->caller_ua_index, pRemoteSDP);

    if (i != 0)
    {
        SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS������ʵʱ��Ƶ����, INVITE��Ӧ��Ϣ����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, (char*)"���տͻ��˵�INVITE��Ϣʧ��");
        EnSystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "Real-time monitoring request from superior CMS, INVITE response message process failed:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, cause=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, (char*)"Receive INVITE message from client failed");

        if (!g_LocalMediaTransferFlag)
        {
            /* ����ǿ缶CMS�ĵ�λ���Ҳ���Ҫ����TSUת��������ֱ��ת����Ϣ */
        }
        else
        {
            /* ֪ͨTSUֹͣת������ */
            i = notify_tsu_delete_transfer_task(pCrData->tsu_ip, pCrData->task_id);
        }

        /* ��Ӧ��Ϣ������ */
        i = SIP_SendAck(ua_dialog_index);
        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_video_route_response_msg_proc() SIP_SendAck:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);

        i = SIP_SendBye(ua_dialog_index);
        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_video_route_response_msg_proc() SIP_SendBye:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);

        /* ��Ӧ��Ϣ������ */
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_INVITE_ACCEPT_ERROR);
        SIP_AnswerToInvite(pCrData->caller_ua_index, 503, strErrorCode);
        //SIP_AnswerToInvite(pCrData->caller_ua_index, 503, (char*)"Accept Invite Error");

        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_video_route_response_msg_proc() exit---: Accept Invite Error \r\n");
        return EV9000_CMS_ERR_INVITE_ACCEPT_ERROR;
    }

    if (!g_LocalMediaTransferFlag)
    {
        /* ����ǿ缶CMS�ĵ�λ���Ҳ���Ҫ����TSUת��������ֱ��ת����Ϣ */
        i = SIP_SendAck(ua_dialog_index);
        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_video_route_response_msg_proc() SIP_SendAck:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);
    }
    else
    {
        if (pCrData->call_type == CALL_TYPE_RECORD_PLAY || pCrData->call_type == CALL_TYPE_DOWNLOAD) /* �¼�CMS ������ */
        {
            i = SIP_SendAck(ua_dialog_index);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_video_route_response_msg_proc() SIP_SendAck:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);
        }
        else
        {
            /* ���������ACK ��Ϣ���� */
            if (ua_dialog_index == pCrData->caller_ua_index)
            {
                i = ack_send_use(cr_pos, ua_dialog_index, -1);
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_video_route_response_msg_proc() ack_send_use:cr_pos=%d, caller_ua_index=%d, i=%d \r\n", cr_pos, ua_dialog_index, i);
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS������ʵʱ��Ƶ����, INVITE��Ӧ��Ϣ����, ��ӵȴ�TSU֪ͨ���񴴽������Ϣ�������¼�, �ȴ�����ACK��Ϣ:cr_pos=%d, caller_ua_index=%d, iRet=%d", cr_pos, ua_dialog_index, i);
            }
            else if (ua_dialog_index == pCrData->callee_ua_index)
            {
                i = ack_send_use(cr_pos, -1, ua_dialog_index);
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_video_route_response_msg_proc() ack_send_use:cr_pos=%d, callee_ua_index=%d, i=%d \r\n", cr_pos, ua_dialog_index, i);
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS������ʵʱ��Ƶ����, INVITE��Ӧ��Ϣ����, ��ӵȴ�TSU֪ͨ���񴴽������Ϣ�������¼�, �ȴ�����ACK��Ϣ:cr_pos=%d, callee_ua_index=%d, iRet=%d", cr_pos, ua_dialog_index, i);
            }
        }
    }

    return 0;
}

/*****************************************************************************
 �� �� ��  : route_invite_audio_response_msg_proc
 ��������  : �ϼ�����CMS���͹�����INVITE ��Ƶ��Ӧ��Ϣ����
 �������  : int cr_pos
             int ua_dialog_index
             cr_t* pCrData
             sdp_message_t* pRemoteSDP
             GBLogicDevice_info_t* pCalleeGBLogicDeviceInfo
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��17��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int route_invite_audio_response_msg_proc(int cr_pos, int ua_dialog_index, cr_t* pCrData, sdp_message_t* pRemoteSDP, GBLogicDevice_info_t* pCalleeGBLogicDeviceInfo)
{
    int i = 0;
    int recv_port = 0;
    char* sdp_tsu_ip = NULL;
    char strErrorCode[32] = {0};

    if (!g_LocalMediaTransferFlag
        && EV9000_DEVICETYPE_SIPSERVER == pCrData->callee_gb_device_type)
    {
        /* ����ǿ缶CMS�ĵ�λ���Ҳ���Ҫ����TSUת��������ֱ��ת����Ϣ */
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS������ʵʱ��Ƶ�Խ�����, INVITE��Ӧ��Ϣ����:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ý��������������ת��, ֱ��ת����Ϣ��ý��������", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Superior CMS real-time audio speaker request, respond the INVITE message processing: the higher the CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s, stream without forward at the corresponding level, the forward message directly to the media stream requesting party", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id);

    }
    else
    {
        /* ��ȡTSU ��Ƶ���ն˿ں� */
        recv_port = get_tsu_audio_recv_port(pCrData->tsu_ip);

        if (recv_port <= 0)
        {
            SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS������ʵʱ��Ƶ�Խ�����, INVITE��Ӧ��Ϣ����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=%s, tsu_ip=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, (char*)"��ȡTSU�Ľ��ն˿ں�ʧ��", pCrData->tsu_ip);
            EnSystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "Real time audio intercom request from superior CMS, INVITE response message process failed:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, cause=%s, tsu_ip=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, (char*)"Access receive port number of TSU failed.", pCrData->tsu_ip);

            /* ��Ӧ��Ϣ������ */
            i = SIP_SendAck(ua_dialog_index);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_audio_response_msg_proc() SIP_SendAck:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);

            i = SIP_SendBye(ua_dialog_index);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_audio_response_msg_proc() SIP_SendBye:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);

            /* ��Ӧ��Ϣ������ */
            memset(strErrorCode, 0, 32);
            snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_TSU_GET_RECV_PORT_ERROR);
            SIP_AnswerToInvite(pCrData->caller_ua_index, 503, strErrorCode);
            //SIP_AnswerToInvite(pCrData->caller_ua_index, 503, (char*)"Get TSU Send Port rror");

            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_audio_response_msg_proc() exit---: Get TSU Send Port Error:tsu_ip=%s \r\n", pCrData->tsu_ip);
            return EV9000_CMS_ERR_TSU_GET_RECV_PORT_ERROR;
        }

        pCrData->tsu_recv_port = recv_port;

        /* �齨����SDP��Ϣ*/
        sdp_tsu_ip = get_cr_sdp_tsu_ip(pCrData, pCrData->caller_server_ip_ethname);

        if (NULL == sdp_tsu_ip)
        {
            SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS������ʵʱ��Ƶ�Խ�����, INVITE��Ӧ��Ϣ����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=%s, callee_server_ip_ethname=%s, iRet=%d", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, (char*)"��ȡ���в��SDP��Ϣ�е�TSU��IP��ַʧ��", pCrData->caller_server_ip_ethname, i);
            EnSystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "Real time audio intercom request from superior CMS, INVITE response message process failed:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, cause=%s, callee_server_ip_ethname=%s, iRet=%d", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, (char*)"Access IP address from caller side SDP message", pCrData->caller_server_ip_ethname, i);

            /* ��Ӧ��Ϣ������ */
            i = SIP_SendAck(ua_dialog_index);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_audio_response_msg_proc() SIP_SendAck:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);

            i = SIP_SendBye(ua_dialog_index);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_audio_response_msg_proc() SIP_SendBye:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);

            /* ��Ӧ��Ϣ������ */
            memset(strErrorCode, 0, 32);
            snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_TSU_GET_CALLER_TSU_IP_ERROR);
            SIP_AnswerToInvite(pCrData->caller_ua_index, 503, strErrorCode);
            //SIP_AnswerToInvite(pCrData->caller_ua_index, 503, (char*)"Get Caller TSU SDP IP Error");

            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_audio_response_msg_proc() exit---: Get Caller TSU SDP IP Error \r\n");
            return EV9000_CMS_ERR_TSU_GET_CALLER_TSU_IP_ERROR;
        }

        /* �޸�SDP�е�ip��ַ�Ͷ˿ں�*/
        i = ModifySDPIPAndPort(pRemoteSDP, sdp_tsu_ip, pCrData->tsu_recv_port);

        if (i != 0)
        {
            SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS������ʵʱ��Ƶ�Խ�����, INVITE��Ӧ��Ϣ����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, (char*)"�޸�SDP�е�IP�Ͷ˿ں�ʧ��");
            EnSystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "Real time audio intercom request from superior CMS, INVITE response message process failed:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, cause=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, (char*)"Edit IP and port number in SDP failed");

            /* ��Ӧ��Ϣ������ */
            i = SIP_SendAck(ua_dialog_index);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_audio_response_msg_proc() SIP_SendAck:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);

            i = SIP_SendBye(ua_dialog_index);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_audio_response_msg_proc() SIP_SendBye:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);

            /* ��Ӧ��Ϣ������ */
            memset(strErrorCode, 0, 32);
            snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_SDP_MODIFY_IP_ERROR);
            SIP_AnswerToInvite(pCrData->caller_ua_index, 503, strErrorCode);
            //SIP_AnswerToInvite(pCrData->caller_ua_index, 503, (char*)"Modify SDP Info Error");

            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_audio_response_msg_proc() exit---: Modify SDP Info Error \r\n");
            return EV9000_CMS_ERR_SDP_MODIFY_IP_ERROR;
        }
    }

    /* �������з��ĺ���*/
    i = SIP_AcceptInvite(pCrData->caller_ua_index, pRemoteSDP);

    if (i != 0)
    {
        SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS������ʵʱ��Ƶ�Խ�����, INVITE��Ӧ��Ϣ����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, (char*)"���տͻ��˵�INVITE��Ϣʧ��");
        EnSystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "Real time audio intercom request from superior CMS, INVITE response message process failed:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, cause=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, (char*)"Receive INVITE message from client failed");

        /* ��Ӧ��Ϣ������ */
        i = SIP_SendAck(ua_dialog_index);
        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_audio_response_msg_proc() SIP_SendAck:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);

        i = SIP_SendBye(ua_dialog_index);
        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_audio_response_msg_proc() SIP_SendBye:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);

        /* ��Ӧ��Ϣ������ */
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_INVITE_ACCEPT_ERROR);
        SIP_AnswerToInvite(pCrData->caller_ua_index, 503, strErrorCode);
        //SIP_AnswerToInvite(pCrData->caller_ua_index, 503, (char*)"Accept Invite Error");

        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_audio_response_msg_proc() exit---: Accept Invite Error \r\n");
        return EV9000_CMS_ERR_INVITE_ACCEPT_ERROR;
    }

    if (!g_LocalMediaTransferFlag
        && EV9000_DEVICETYPE_SIPSERVER == pCrData->callee_gb_device_type)
    {
        /* ����ǿ缶CMS�ĵ�λ���Ҳ���Ҫ����TSUת��������ֱ��ת����Ϣ */
    }
    else
    {
        /* ֪ͨTSU��ʼת����Ƶ�� */
        i = notify_tsu_add_audio_transfer_task(pCrData);

        if (i < 0)
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_audio_response_msg_proc() exit---: notify_tsu_add_audio_transfer_task Error: TSU IP=%s, i=%d \r\n", pCrData->tsu_ip, i);
            SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS������ʵʱ��Ƶ�Խ�����, INVITE��Ӧ��Ϣ����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=%s, TSU IP=%s, iRet=%d", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, (char*)"֪ͨTSU���ת������ʧ��", pCrData->tsu_ip, i);
            EnSystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "Real time audio intercom request from superior CMS, INVITE response message process failed:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, cause=%s, TSU IP=%s, iRet=%d", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, (char*)"Notify TSU to add forwarding task failed", pCrData->tsu_ip, i);

            /* ��Ӧ��Ϣ������ */
            i = SIP_SendAck(ua_dialog_index);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_audio_response_msg_proc() SIP_SendAck:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);

            i = SIP_SendBye(ua_dialog_index);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_audio_response_msg_proc() SIP_SendBye:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);

            /* ��Ӧ��Ϣ������ */
            i = SIP_SendBye(pCrData->caller_ua_index);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_audio_response_msg_proc() SIP_SendBye:caller_ua_index=%d, i=%d \r\n", pCrData->caller_ua_index, i);

            return EV9000_CMS_ERR_TSU_NOTIFY_ADD_TRANSFER_ERROR;
        }
    }

    i = SIP_SendAck(ua_dialog_index);
    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_audio_response_msg_proc() SIP_SendAck:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);

    return 0;
}

/*****************************************************************************
 �� �� ��  : route_cancel_msg_proc
 ��������  : �ϼ�����CMS���͹�����Cancel��Ϣ����
 �������  : int cr_pos
             int ua_dialog_index
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��8��29�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int route_cancel_msg_proc(int cr_pos, int ua_dialog_index)
{
    int i = 0;
    cr_t* pCrData = NULL;

    if (cr_pos < 0)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_cancel_msg_proc() exit---: Param Error \r\n");
        return -1;
    }

    /* Cancel��Ϣ�ɿͻ����ڷ���Invite֮��û���յ�����Ӧ��ǰ���� */
    pCrData = call_record_get(cr_pos);

    if (NULL == pCrData)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_cancel_msg_proc() exit---: Get Call Record Error:cr_pos=%d \r\n", cr_pos);
        return -1;
    }

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS������ʵʱ��Ƶ����, �յ�CANCELȡ����Ϣ����, ʵʱ��Ƶ�ر�:�ϼ�CMS ID=%s, IP��ַ=%s, �߼��豸ID=%s, IP��ַ=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->callee_id, pCrData->callee_ip);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Real-time video from superior CMS, receive the user cancel message processing:User ID=%s, User IP=%s,Logic Device ID=%s, IP Address=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->callee_id, pCrData->callee_ip);

    /* 2�������ʵʱ��Ƶҵ�񣬷�������Ҫ��Cancel ��Ϣת�������з� */
    if (pCrData->callee_ua_index >= 0)
    {
        i = SIP_SendCancel(pCrData->callee_ua_index);
        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_cancel_msg_proc() SIP_SendCancel:callee_ua_index=%d, i=%d \r\n", pCrData->callee_ua_index, i);
    }

    /* 3���Ƴ����м�¼��Ϣ */
    i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
    i = call_record_remove(cr_pos);

    if (0 != i)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_cancel_msg_proc() call_record_remove Error:cr_pos=%d, i=%d \r\n", cr_pos, i);
    }
    else
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_cancel_msg_proc() call_record_remove OK:cr_pos=%d, i=%d \r\n", cr_pos, i);
    }

    return i;
}

/*****************************************************************************
 �� �� ��  : route_ack_msg_proc
 ��������  : �ϼ�����CMS���͹�����ACK��Ϣ����
 �������  : int cr_pos
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
int route_ack_msg_proc(int cr_pos, int ua_dialog_index)
{
    int i = 0;
    cr_t* pCrData = NULL;

    if (cr_pos < 0)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_ack_msg_proc() exit---: Param Error \r\n");
        return -1;
    }

    /* Ack��Ϣ���ϼ�CMS �յ�200�󷢳� */
    pCrData = call_record_get(cr_pos);

    if (NULL == pCrData)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_ack_msg_proc() exit---: Get Call Record Error:cr_pos=%d \r\n", cr_pos);
        return -1;
    }

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS������ʵʱ��Ƶ����, �յ�ACK��Ϣ����:�ϼ�CMS ID=%s, IP��ַ=%s, �߼��豸ID=%s, IP��ַ=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->callee_id, pCrData->callee_ip);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Superior CMS real-time video request, receives an ACK message processing: the higher the CMS, ID = % s IP address = % s, logical device ID = % s, IP address = % s", pCrData->caller_id, pCrData->caller_ip, pCrData->callee_id, pCrData->callee_ip);

    /* 2�������ʵʱ��Ƶҵ�񣬷�������Ҫ��ack��Ϣת�������з� */
    /*if (pCrData->callee_ua_index >= 0) //�յ�ǰ����Ӧ��ʱ��ֱ�ӻظ���ACK,��������ط��յ��ͻ��˵�ACK����Ҫ��ת����ǰ��
    {
        i = SIP_SendAck(pCrData->callee_ua_index);
    }*/


    return i;
}

/*****************************************************************************
 �� �� ��  : route_bye_msg_proc
 ��������  : �ϼ�����CMS���͹�����BYE ��Ϣ����
 �������  : int cr_pos
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
int route_bye_msg_proc(int cr_pos, int ua_dialog_index)
{
    int i = 0;
    cr_t* pCrData = NULL;
    GBLogicDevice_info_t* pCalleeGBLogicDeviceInfo = NULL;

    if (cr_pos < 0)
    {
        SIP_AnswerToBye(ua_dialog_index, 403, NULL);
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_bye_msg_proc() exit---: Param Error \r\n");
        return -1;
    }

    pCrData = call_record_get(cr_pos);

    if (NULL == pCrData)
    {
        SIP_AnswerToBye(ua_dialog_index, 481, NULL);
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_bye_msg_proc() exit---: Get Call Record Error:cr_pos=%d \r\n", cr_pos);
        return -1;
    }

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS������ʵʱ��Ƶ, �յ�BYE�ر���Ϣ����:�ϼ�CMS ID=%s, IP��ַ=%s, �߼��豸ID=%s, IP��ַ=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->callee_id, pCrData->callee_ip);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Real-time video from superior CMS, receive close message process from superior CMS:user ID=%s, user IP address=%s, logic deviceID=%s, IPaddress=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->callee_id, pCrData->callee_ip);

    /* �����߼��豸��Ϣ */
    pCalleeGBLogicDeviceInfo = GBLogicDevice_info_find(pCrData->callee_id);

    if (NULL == pCalleeGBLogicDeviceInfo)
    {
        SIP_AnswerToBye(ua_dialog_index, 503, NULL);
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_bye_msg_proc() exit---: Get Callee GBLogic Device Info Error:callee_id=%s \r\n", pCrData->callee_id);
        return -1;
    }

    if (CALL_TYPE_AUDIO == pCrData->call_type)
    {
        i = route_bye_audio_msg_proc(cr_pos, ua_dialog_index, pCrData, pCalleeGBLogicDeviceInfo);
    }
    else
    {
        i = route_bye_video_msg_proc(cr_pos, ua_dialog_index, pCrData, pCalleeGBLogicDeviceInfo);
    }

    return i;
}

/*****************************************************************************
 �� �� ��  : route_bye_video_msg_proc
 ��������  : �ϼ�����CMS���͹�����BYE ��Ƶ��Ϣ����
 �������  : int cr_pos
             int ua_dialog_index
             cr_t* pCrData
             GBLogicDevice_info_t* pCalleeGBLogicDeviceInfo
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��17��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int route_bye_video_msg_proc(int cr_pos, int ua_dialog_index, cr_t* pCrData, GBLogicDevice_info_t* pCalleeGBLogicDeviceInfo)
{
    int i = 0;

    /* �����߼��豸����������жϣ�������Ϣ���� */
    if (1 == pCalleeGBLogicDeviceInfo->other_realm)
    {
        i = route_bye_route_video_msg_proc(cr_pos, ua_dialog_index, pCrData, pCalleeGBLogicDeviceInfo);
    }
    else
    {
        i = route_bye_sub_video_msg_proc(cr_pos, ua_dialog_index, pCrData, pCalleeGBLogicDeviceInfo);
    }

    return i;
}

/*****************************************************************************
 �� �� ��  : route_bye_sub_video_msg_proc
 ��������  : �ϼ�����CMS���͹�����BYE ��Ƶ��Ϣ����
 �������  : int cr_pos
             int ua_dialog_index
             cr_t* pCrData
             GBDevice_info_t* pCalleeGBDeviceInfo
             GBLogicDevice_info_t* pCalleeGBLogicDeviceInfo
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��17��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int route_bye_sub_video_msg_proc(int cr_pos, int ua_dialog_index, cr_t* pCrData, GBLogicDevice_info_t* pCalleeGBLogicDeviceInfo)
{
    int i = 0;
    int other_cr_pos = -1;
    cr_t* pOtherCrData = NULL;
    GBDevice_info_t* pCalleeGBDeviceInfo = NULL;
    GBDevice_info_t* pCalleeCmsGBDeviceInfo = NULL;

    if (NULL == pCrData)
    {
        SIP_AnswerToBye(ua_dialog_index, 481, NULL);
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_bye_sub_video_msg_proc() exit---: Get Call Record Error:cr_pos=%d \r\n", cr_pos);
        return -1;
    }

    if (NULL == pCalleeGBLogicDeviceInfo)
    {
        SIP_AnswerToBye(ua_dialog_index, 503, NULL);
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_bye_sub_video_msg_proc() exit---: Get Callee GBLogic Device Info Error:callee_id=%s \r\n", pCrData->callee_id);
        return -1;
    }

    /* ���Ҷ�Ӧ�������豸 */
    pCalleeGBDeviceInfo = GBDevice_info_get_by_stream_type(pCalleeGBLogicDeviceInfo, pCrData->callee_stream_type);

    if (NULL == pCalleeGBDeviceInfo)
    {
        /* ��������ܷ��������������¼�ƽ̨�ĵ�λ�����ʱ����Ҫ�ٲ���һ�������豸 */
        if (EV9000_STREAM_TYPE_INTELLIGENCE == pCrData->callee_stream_type)
        {
            pCalleeCmsGBDeviceInfo = GBDevice_info_get_by_stream_type(pCalleeGBLogicDeviceInfo, EV9000_STREAM_TYPE_MASTER);

            if (NULL != pCalleeCmsGBDeviceInfo
                && EV9000_DEVICETYPE_SIPSERVER == pCalleeCmsGBDeviceInfo->device_type)
            {
                pCalleeGBDeviceInfo = pCalleeCmsGBDeviceInfo;
                DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_bye_sub_video_msg_proc() CalleeCmsGBDevice: id=%s, ip=%s \r\n", pCalleeCmsGBDeviceInfo->device_id, pCalleeCmsGBDeviceInfo->login_ip);
            }
        }
    }

    if (NULL == pCalleeGBDeviceInfo)
    {
        SIP_AnswerToBye(ua_dialog_index, 503, NULL);
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_bye_sub_video_msg_proc() exit---: Get Callee GBDevice Info Error \r\n");
        return -1;
    }

    /* Bye ��Ϣ���������з��͵�Ҳ�����Ǳ��з��͵�*/
    if (pCrData->callee_ua_index == ua_dialog_index)    /* Դ�˷��͵�Bye */
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS������ʵʱ��Ƶ����, �յ�ǰ��BYE��Ϣ����, ʵʱ��Ƶ�ر�:�ϼ�CMS ID=%s, IP��ַ=%s, �߼��豸ID=%s, IP��ַ=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->callee_id, pCrData->callee_ip);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Superior CMS real-time video request, receive front-end BYE message processing, real-time video closed: the higher the CMS, ID = % s IP address = % s, logical device ID = % s, IP address = % s", pCrData->caller_id, pCrData->caller_ip, pCrData->callee_id, pCrData->callee_ip);

        /* ֱ�ӷ���200 ��Ӧ��Ϣ�����в� */
        SIP_AnswerToBye(ua_dialog_index, 200, NULL);

        /* ֪ͨTSUֹͣ��������*/
        if ((CALL_TYPE_RECORD_PLAY == pCrData->call_type)
            || (CALL_TYPE_DOWNLOAD == pCrData->call_type))
        {
            /* ����TSU�еĻ��滹û�з��꣬���ʱ���ܷ���Bye ���ͻ��� */
            i = notify_tsu_delete_replay_task(pCrData->tsu_ip, pCrData->task_id);

            if (i < 0)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_WARN, "route_bye_sub_video_msg_proc() notify_tsu_delete_replay_task Error:tsu_ip=%s, task_id=%s, i=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
            }
            else
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_bye_sub_video_msg_proc() notify_tsu_delete_replay_task OK:tsu_ip=%s, task_id=%s, i=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
            }

            /* ������¼�¼��鿴����ǰ��¼�������£���Ҫת��Bye�ϼ����߿ͻ��� */
            if (pCalleeGBDeviceInfo->device_type == EV9000_DEVICETYPE_SIPSERVER) /* �¼�CMS ��¼�� */
            {
                /* ����Bye �����в� */
                i = SIP_SendBye(pCrData->caller_ua_index);
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_bye_sub_video_msg_proc() SIP_SendBye To Caller:caller_ua_index=%d, i=%d \r\n", pCrData->caller_ua_index, i);
            }
            else if (pCalleeGBLogicDeviceInfo->record_type == 1) /* ����ǰ��¼�� */
            {
                /* ����Bye �����в� */
                i = SIP_SendBye(pCrData->caller_ua_index);
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_bye_sub_video_msg_proc() SIP_SendBye To Caller:caller_ua_index=%d, i=%d \r\n", pCrData->caller_ua_index, i);
            }
        }
        else
        {
            i = notify_tsu_delete_transfer_task(pCrData->tsu_ip, pCrData->task_id);

            if (i < 0)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_WARN, "route_bye_sub_video_msg_proc() notify_tsu_delete_transfer_task Error:tsu_ip=%s, task_id=%s, i=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
            }
            else
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_bye_sub_video_msg_proc() notify_tsu_delete_transfer_task OK:tsu_ip=%s, task_id=%s, i=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
            }

            /* ����Bye ����������ҵ�����в��û�*/
            i = send_bye_to_all_other_caller_by_callee_id_and_streamtype(pCrData->callee_id, pCrData->callee_stream_type, cr_pos);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_bye_sub_video_msg_proc() send_bye_to_all_other_caller_by_callee_id_and_streamtype:callee_id=%s, callee_stream_type=%d, i=%d \r\n", pCrData->callee_id, pCrData->callee_stream_type, i);

            /* ����Bye �����в� */
            i = SIP_SendBye(pCrData->caller_ua_index);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_bye_sub_video_msg_proc() SIP_SendBye To Caller:caller_ua_index=%d, i=%d \r\n", pCrData->caller_ua_index, i);
        }

        /* �Ƴ����м�¼��Ϣ */
        i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
        i = call_record_remove(cr_pos);

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_bye_sub_video_msg_proc() call_record_remove Error:cr_pos=%d, i=%d \r\n", cr_pos, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_bye_sub_video_msg_proc() call_record_remove OK:cr_pos=%d, i=%d \r\n", cr_pos, i);
        }

        return 0;
    }
    else if (pCrData->caller_ua_index == ua_dialog_index) /* ���з��͵�Bye */
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS������ʵʱ��Ƶ����, �յ�����BYE��Ϣ����, ʵʱ��Ƶ�ر�:�ϼ�CMS ID=%s, IP��ַ=%s, �߼��豸ID=%s, IP��ַ=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->callee_id, pCrData->callee_ip);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Superior CMS real-time video request, received the requester BYE message processing, real-time video closed: the higher the CMS, ID = % s IP address = % s, logical device ID = % s, IP address = % s", pCrData->caller_id, pCrData->caller_ip, pCrData->callee_id, pCrData->callee_ip);

        /* ֪ͨTSUֹͣ��������*/
        if ((CALL_TYPE_RECORD_PLAY == pCrData->call_type)
            || (CALL_TYPE_DOWNLOAD == pCrData->call_type))
        {
            i = notify_tsu_delete_replay_task(pCrData->tsu_ip, pCrData->task_id);

            if (i < 0)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_WARN, "route_bye_sub_video_msg_proc() notify_tsu_delete_replay_task Error:tsu_ip=%s, task_id=%s, i=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
            }
            else
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_bye_sub_video_msg_proc() notify_tsu_delete_replay_task OK:tsu_ip=%s, task_id=%s, i=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
            }

            /* ������¼�¼��鿴����ǰ��¼�������£���Ҫת��Bye���¼�����ǰ�� */
            if (pCalleeGBDeviceInfo->device_type == EV9000_DEVICETYPE_SIPSERVER) /* �¼�CMS ��¼�� */
            {
                /*����Bye �����в� */
                i = SIP_SendBye(pCrData->callee_ua_index);
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_bye_sub_video_msg_proc() SIP_SendBye To Callee:callee_ua_index=%d, i=%d \r\n", pCrData->callee_ua_index, i);
            }
            else if (pCalleeGBLogicDeviceInfo->record_type == 1) /* ����ǰ��¼�� */
            {
                /*����Bye �����в� */
                i = SIP_SendBye(pCrData->callee_ua_index);
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_bye_sub_video_msg_proc() SIP_SendBye To Callee:callee_ua_index=%d, i=%d \r\n", pCrData->callee_ua_index, i);
            }
        }
        else
        {
            i = notify_tsu_delete_transfer_task(pCrData->tsu_ip, pCrData->task_id);

            if (i < 0)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_WARN, "route_bye_sub_video_msg_proc() notify_tsu_delete_transfer_task Error:tsu_ip=%s, task_id=%s, i=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
            }
            else
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_bye_sub_video_msg_proc() notify_tsu_delete_transfer_task OK:tsu_ip=%s, task_id=%s, i=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
            }

            /* ���Ƿ���ǰ������ */
            if (pCrData->callee_ua_index >= 0)
            {
                /* �鿴�Ƿ��������ͻ���ҵ�� */
                other_cr_pos = is_GBLogic_device_has_other_service(pCrData->callee_id, pCrData->callee_stream_type, cr_pos);
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_bye_sub_video_msg_proc() is_GBLogic_device_has_other_service:other_cr_pos=%d \r\n", other_cr_pos);

                if (other_cr_pos < 0) /* û������ҵ�� */
                {
                    /*����Bye �����в� */
                    i = SIP_SendBye(pCrData->callee_ua_index);
                    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_bye_sub_video_msg_proc() SIP_SendBye To Callee:callee_ua_index=%d, i=%d \r\n", pCrData->callee_ua_index, i);
                }
                else
                {
                    pOtherCrData = call_record_get(other_cr_pos);

                    if (NULL != pOtherCrData)
                    {
                        pOtherCrData->callee_ua_index = pCrData->callee_ua_index; /* ��ǰ�˵ĻỰ����������¸�ҵ�� */
                        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_bye_sub_video_msg_proc() callee_ua_index=%d copy from %d to %d \r\n", pOtherCrData->callee_ua_index, cr_pos, other_cr_pos);
                    }
                }
            }
        }

        /* ֱ�ӷ���200 ��Ӧ��Ϣ���в� */
        SIP_AnswerToBye(ua_dialog_index, 200, NULL);

        /* �Ƴ����м�¼��Ϣ */
        i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
        i = call_record_remove(cr_pos);

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_bye_sub_video_msg_proc() call_record_remove Error:cr_pos=%d, i=%d \r\n", cr_pos, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_bye_sub_video_msg_proc() call_record_remove OK:cr_pos=%d, i=%d \r\n", cr_pos, i);
        }

        return 0;
    }

    /* �Ƴ����м�¼��Ϣ */
    i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
    i = call_record_remove(cr_pos);

    if (0 != i)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_bye_sub_video_msg_proc() call_record_remove Error:cr_pos=%d, i=%d \r\n", cr_pos, i);
    }
    else
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_bye_sub_video_msg_proc() call_record_remove OK:cr_pos=%d, i=%d \r\n", cr_pos, i);
    }

    SIP_AnswerToBye(ua_dialog_index, 481, NULL);
    return -1;
}

/*****************************************************************************
 �� �� ��  : route_bye_route_video_msg_proc
 ��������  : �ϼ�����CMS���͹�����BYE ��Ƶ��Ϣ����
 �������  : int cr_pos
             int ua_dialog_index
             cr_t* pCrData
             GBLogicDevice_info_t* pCalleeGBLogicDeviceInfo
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��17��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int route_bye_route_video_msg_proc(int cr_pos, int ua_dialog_index, cr_t* pCrData, GBLogicDevice_info_t* pCalleeGBLogicDeviceInfo)
{
    int i = 0;
    int other_cr_pos = -1;
    cr_t* pOtherCrData = NULL;

    if (NULL == pCrData)
    {
        SIP_AnswerToBye(ua_dialog_index, 481, NULL);
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_bye_route_video_msg_proc() exit---: Get Call Record Error:cr_pos=%d \r\n", cr_pos);
        return -1;
    }

    if (NULL == pCalleeGBLogicDeviceInfo)
    {
        SIP_AnswerToBye(ua_dialog_index, 503, NULL);
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_bye_route_video_msg_proc() exit---: Get Callee GBLogic Device Info Error:callee_id=%s \r\n", pCrData->callee_id);
        return -1;
    }

    /* Bye ��Ϣ���������з��͵�Ҳ�����Ǳ��з��͵�*/
    if (pCrData->callee_ua_index == ua_dialog_index)    /* Դ�˷��͵�Bye */
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS������ʵʱ��Ƶ����, �յ�ǰ��BYE��Ϣ����, ʵʱ��Ƶ�ر�:�ϼ�CMS ID=%s, IP��ַ=%s, �߼��豸ID=%s, IP��ַ=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->callee_id, pCrData->callee_ip);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Superior CMS real-time video request, receive front-end BYE message processing, real-time video closed: the higher the CMS, ID = % s IP address = % s, logical device ID = % s, IP address = % s", pCrData->caller_id, pCrData->caller_ip, pCrData->callee_id, pCrData->callee_ip);

        /* ֱ�ӷ���200 ��Ӧ��Ϣ�����в� */
        SIP_AnswerToBye(ua_dialog_index, 200, NULL);

        /* ֪ͨTSUֹͣ��������*/
        if ((CALL_TYPE_RECORD_PLAY == pCrData->call_type)
            || (CALL_TYPE_DOWNLOAD == pCrData->call_type))
        {
            /* ����TSU�еĻ��滹û�з��꣬���ʱ���ܷ���Bye ���ͻ��� */
            i = notify_tsu_delete_replay_task(pCrData->tsu_ip, pCrData->task_id);

            if (i < 0)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_WARN, "route_bye_route_video_msg_proc() notify_tsu_delete_replay_task Error:tsu_ip=%s, task_id=%s, i=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
            }
            else
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_bye_route_video_msg_proc() notify_tsu_delete_replay_task OK:tsu_ip=%s, task_id=%s, i=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
            }

            /* ����Bye �����в� */
            i = SIP_SendBye(pCrData->caller_ua_index);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_bye_route_video_msg_proc() SIP_SendBye To Caller:caller_ua_index=%d, i=%d \r\n", pCrData->caller_ua_index, i);
        }
        else
        {
            i = notify_tsu_delete_transfer_task(pCrData->tsu_ip, pCrData->task_id);

            if (i < 0)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_WARN, "route_bye_route_video_msg_proc() notify_tsu_delete_transfer_task Error:tsu_ip=%s, task_id=%s, i=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
            }
            else
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_bye_route_video_msg_proc() notify_tsu_delete_transfer_task OK:tsu_ip=%s, task_id=%s, i=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
            }

            /* ����Bye ����������ҵ�����в��û�*/
            i = send_bye_to_all_other_caller_by_callee_id_and_streamtype(pCrData->callee_id, pCrData->callee_stream_type, cr_pos);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_bye_route_video_msg_proc() send_bye_to_all_other_caller_by_callee_id_and_streamtype:callee_id=%s, callee_stream_type=%d, i=%d \r\n", pCrData->callee_id, pCrData->callee_stream_type, i);

            /* ����Bye �����в� */
            i = SIP_SendBye(pCrData->caller_ua_index);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_bye_route_video_msg_proc() SIP_SendBye To Caller:caller_ua_index=%d, i=%d \r\n", pCrData->caller_ua_index, i);
        }

        /* �Ƴ����м�¼��Ϣ */
        i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
        i = call_record_remove(cr_pos);

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_bye_route_video_msg_proc() call_record_remove Error:cr_pos=%d, i=%d \r\n", cr_pos, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_bye_route_video_msg_proc() call_record_remove OK:cr_pos=%d, i=%d \r\n", cr_pos, i);
        }

        return 0;
    }
    else if (pCrData->caller_ua_index == ua_dialog_index) /* ���з��͵�Bye */
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS������ʵʱ��Ƶ����, �յ�����BYE��Ϣ����, ʵʱ��Ƶ�ر�:�ϼ�CMS ID=%s, IP��ַ=%s, �߼��豸ID=%s, IP��ַ=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->callee_id, pCrData->callee_ip);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Superior CMS real-time video request, received the requester BYE message processing, real-time video closed: the higher the CMS, ID = % s IP address = % s, logical device ID = % s, IP address = % s", pCrData->caller_id, pCrData->caller_ip, pCrData->callee_id, pCrData->callee_ip);

        /* ֪ͨTSUֹͣ��������*/
        if ((CALL_TYPE_RECORD_PLAY == pCrData->call_type)
            || (CALL_TYPE_DOWNLOAD == pCrData->call_type))
        {
            i = notify_tsu_delete_replay_task(pCrData->tsu_ip, pCrData->task_id);

            if (i < 0)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_WARN, "route_bye_route_video_msg_proc() notify_tsu_delete_replay_task Error:tsu_ip=%s, task_id=%s, i=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
            }
            else
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_bye_route_video_msg_proc() notify_tsu_delete_replay_task OK:tsu_ip=%s, task_id=%s, i=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
            }

            /*����Bye �����в� */
            i = SIP_SendBye(pCrData->callee_ua_index);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_bye_route_video_msg_proc() SIP_SendBye To Callee:callee_ua_index=%d, i=%d \r\n", pCrData->callee_ua_index, i);
        }
        else
        {
            i = notify_tsu_delete_transfer_task(pCrData->tsu_ip, pCrData->task_id);

            if (i < 0)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_WARN, "route_bye_route_video_msg_proc() notify_tsu_delete_transfer_task Error:tsu_ip=%s, task_id=%s, i=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
            }
            else
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_bye_route_video_msg_proc() notify_tsu_delete_transfer_task OK:tsu_ip=%s, task_id=%s, i=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
            }

            /* ���Ƿ���ǰ������ */
            if (pCrData->callee_ua_index >= 0)
            {
                /* �鿴�Ƿ��������ͻ���ҵ�� */
                other_cr_pos = is_GBLogic_device_has_other_service(pCrData->callee_id, pCrData->callee_stream_type, cr_pos);
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_bye_route_video_msg_proc() is_GBLogic_device_has_other_service:other_cr_pos=%d \r\n", other_cr_pos);

                if (other_cr_pos < 0) /* û������ҵ�� */
                {
                    /*����Bye �����в� */
                    i = SIP_SendBye(pCrData->callee_ua_index);
                    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_bye_route_video_msg_proc() SIP_SendBye To Callee:callee_ua_index=%d, i=%d \r\n", pCrData->callee_ua_index, i);
                }
                else
                {
                    pOtherCrData = call_record_get(other_cr_pos);

                    if (NULL != pOtherCrData)
                    {
                        pOtherCrData->callee_ua_index = pCrData->callee_ua_index; /* ��ǰ�˵ĻỰ����������¸�ҵ�� */
                        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_bye_route_video_msg_proc() callee_ua_index=%d copy from %d to %d \r\n", pOtherCrData->callee_ua_index, cr_pos, other_cr_pos);
                    }
                }
            }
        }

        /* ֱ�ӷ���200 ��Ӧ��Ϣ���в� */
        SIP_AnswerToBye(ua_dialog_index, 200, NULL);

        /* �Ƴ����м�¼��Ϣ */
        i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
        i = call_record_remove(cr_pos);

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_bye_route_video_msg_proc() call_record_remove Error:cr_pos=%d, i=%d \r\n", cr_pos, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_bye_route_video_msg_proc() call_record_remove OK:cr_pos=%d, i=%d \r\n", cr_pos, i);
        }

        return 0;
    }

    /* �Ƴ����м�¼��Ϣ */
    i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
    i = call_record_remove(cr_pos);

    if (0 != i)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_bye_route_video_msg_proc() call_record_remove Error:cr_pos=%d, i=%d \r\n", cr_pos, i);
    }
    else
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_bye_route_video_msg_proc() call_record_remove OK:cr_pos=%d, i=%d \r\n", cr_pos, i);
    }

    SIP_AnswerToBye(ua_dialog_index, 481, NULL);
    return -1;
}

/*****************************************************************************
 �� �� ��  : route_bye_audio_msg_proc
 ��������  : �ϼ�����CMS���͹�����BYE ��Ƶ�Խ���Ϣ����
 �������  : int cr_pos
             int ua_dialog_index
             cr_t* pCrData
             GBLogicDevice_info_t* pCalleeGBLogicDeviceInfo
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��17��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int route_bye_audio_msg_proc(int cr_pos, int ua_dialog_index, cr_t* pCrData, GBLogicDevice_info_t* pCalleeGBLogicDeviceInfo)
{
    int i = 0;

    if (NULL == pCrData)
    {
        SIP_AnswerToBye(ua_dialog_index, 481, NULL);
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_bye_audio_msg_proc() exit---: Get Call Record Error:cr_pos=%d \r\n", cr_pos);
        return -1;
    }

    if (NULL == pCalleeGBLogicDeviceInfo)
    {
        SIP_AnswerToBye(ua_dialog_index, 503, NULL);
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_bye_audio_msg_proc() exit---: Get Callee GBLogic Device Info Error:callee_id=%s \r\n", pCrData->callee_id);
        return -1;
    }

    /* Bye ��Ϣ���������з��͵�Ҳ�����Ǳ��з��͵�*/
    if (pCrData->callee_ua_index == ua_dialog_index)    /* Դ�˷��͵�Bye */
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS������ʵʱ��Ƶ�Խ�����, �յ�ǰ��BYE��Ϣ����, ʵʱ��Ƶ�ر�:�ϼ�CMS ID=%s, IP��ַ=%s, �߼��豸ID=%s, IP��ַ=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->callee_id, pCrData->callee_ip);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Superior CMS real-time audio speaker request, receive front-end BYE message processing, real-time video closed: the higher the CMS, ID = % s IP address = % s, logical device ID = % s, IP address = % s", pCrData->caller_id, pCrData->caller_ip, pCrData->callee_id, pCrData->callee_ip);

        /* ֱ�ӷ���200 ��Ӧ��Ϣ�����в� */
        SIP_AnswerToBye(ua_dialog_index, 200, NULL);

        /* ֪ͨTSUֹͣת����Ƶ��*/
        i = notify_tsu_delete_audio_transfer_task(pCrData->tsu_ip, pCrData->caller_sdp_ip, pCrData->caller_sdp_port);

        if (i < 0)
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_WARN, "route_bye_audio_msg_proc() notify_tsu_delete_audio_transfer_task Error:tsu_ip=%s, receive_ip=%s, receive_port=%d, i=%d \r\n", pCrData->tsu_ip, pCrData->caller_sdp_ip, pCrData->caller_sdp_port, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_bye_audio_msg_proc() notify_tsu_delete_audio_transfer_task OK:tsu_ip=%s, receive_ip=%s, receive_port=%d, i=%d \r\n", pCrData->tsu_ip, pCrData->caller_sdp_ip, pCrData->caller_sdp_port, i);
        }

        /* ����Bye �����в� */
        i = SIP_SendBye(pCrData->caller_ua_index);
        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_bye_audio_msg_proc() SIP_SendBye To Caller:caller_ua_index=%d, i=%d \r\n", pCrData->caller_ua_index, i);

        /* �Ƴ����м�¼��Ϣ */
        i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
        i = call_record_remove(cr_pos);

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_bye_audio_msg_proc() call_record_remove Error:cr_pos=%d, i=%d \r\n", cr_pos, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_bye_audio_msg_proc() call_record_remove OK:cr_pos=%d, i=%d \r\n", cr_pos, i);
        }

        return 0;
    }
    else if (pCrData->caller_ua_index == ua_dialog_index) /* ���з��͵�Bye */
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS������ʵʱ��Ƶ�Խ�����, �յ�����BYE��Ϣ����, ʵʱ��Ƶ�ر�:�ϼ�CMS ID=%s, IP��ַ=%s, �߼��豸ID=%s, IP��ַ=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->callee_id, pCrData->callee_ip);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Superior CMS real-time audio speaker request, received the requester BYE message processing, real-time video closed: the higher the CMS, ID = % s IP address = % s, logical device ID = % s, IP address = % s", pCrData->caller_id, pCrData->caller_ip, pCrData->callee_id, pCrData->callee_ip);

        /* ֱ�ӷ���200 ��Ӧ��Ϣ�����в� */
        SIP_AnswerToBye(ua_dialog_index, 200, NULL);

        /* ֪ͨTSUֹͣת����Ƶ��*/
        i = notify_tsu_delete_audio_transfer_task(pCrData->tsu_ip, pCrData->caller_sdp_ip, pCrData->caller_sdp_port);

        if (i < 0)
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_WARN, "route_bye_audio_msg_proc() notify_tsu_delete_audio_transfer_task Error:tsu_ip=%s, receive_ip=%s, receive_port=%d, i=%d \r\n", pCrData->tsu_ip, pCrData->caller_sdp_ip, pCrData->caller_sdp_port, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_bye_audio_msg_proc() notify_tsu_delete_audio_transfer_task OK:tsu_ip=%s, receive_ip=%s, receive_port=%d, i=%d \r\n", pCrData->tsu_ip, pCrData->caller_sdp_ip, pCrData->caller_sdp_port, i);
        }

        /*����Bye �����в� */
        i = SIP_SendBye(pCrData->callee_ua_index);
        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_bye_audio_msg_proc() SIP_SendBye To Callee:callee_ua_index=%d, i=%d \r\n", pCrData->callee_ua_index, i);

        /* �Ƴ����м�¼��Ϣ */
        i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
        i = call_record_remove(cr_pos);

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_bye_audio_msg_proc() call_record_remove Error:cr_pos=%d, i=%d \r\n", cr_pos, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_bye_audio_msg_proc() call_record_remove OK:cr_pos=%d, i=%d \r\n", cr_pos, i);
        }

        return 0;
    }

    /* �Ƴ����м�¼��Ϣ */
    i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
    i = call_record_remove(cr_pos);

    if (0 != i)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_bye_audio_msg_proc() call_record_remove Error:cr_pos=%d, i=%d \r\n", cr_pos, i);
    }
    else
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_bye_audio_msg_proc() call_record_remove OK:cr_pos=%d, i=%d \r\n", cr_pos, i);
    }

    SIP_AnswerToBye(ua_dialog_index, 481, NULL);
    return -1;
}

/*****************************************************************************
 �� �� ��  : route_bye_response_msg_proc
 ��������  : �ϼ�����CMS���͹�����BYE ��Ӧ��Ϣ����
 �������  : int cr_pos
             int ua_dialog_index
             int response_code
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��17��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int route_bye_response_msg_proc(int cr_pos, int ua_dialog_index, int response_code)
{
    int i = 0;
    cr_t* pCrData = NULL;

    if (cr_pos < 0)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_bye_response_msg_proc() exit---: Param Error \r\n");
        return -1;
    }

    pCrData = call_record_get(cr_pos);

    if (NULL == pCrData)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_bye_response_msg_proc() exit---: Get Call Record Error:cr_pos=%d \r\n", cr_pos);
        return -1;
    }

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS������ʵʱ��Ƶ���� �յ�BYE��Ӧ��Ϣ����:�ϼ�CMS ID=%s, IP��ַ=%s, �߼��豸ID=%s, IP��ַ=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->callee_id, pCrData->callee_ip);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Superior CMS real-time video request received BYE response message handling: the higher the CMS, ID = % s IP address = % s, logical device ID = % s, IP address = % s", pCrData->caller_id, pCrData->caller_ip, pCrData->callee_id, pCrData->callee_ip);

    /* Bye ��Ӧ��Ϣ���������з��͵�Ҳ�����Ǳ��з��͵�*/
    if (pCrData->callee_ua_index == ua_dialog_index)    /* ���з��͵�Bye */
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS������ʵʱ��Ƶ����, �յ�����BYE��Ӧ��Ϣ����, ʵʱ��Ƶ�ر�:�ϼ�CMS ID=%s, IP��ַ=%s, �߼��豸ID=%s, IP��ַ=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->callee_id, pCrData->callee_ip);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Superior CMS real-time video request, received the requester BYE response message processing, real-time video closed: the higher the CMS, ID = % s IP address = % s, logical device ID = % s, IP address = % s", pCrData->caller_id, pCrData->caller_ip, pCrData->callee_id, pCrData->callee_ip);

        /* �����ʵʱ��Ƶҵ�񣬷�������Ҫ��Bye��Ӧ��Ϣת�����в� */
        if (pCrData->caller_ua_index >= 0)
        {
            SIP_AnswerToBye(pCrData->caller_ua_index, response_code, NULL);
        }
    }
    else if (pCrData->caller_ua_index == ua_dialog_index) /* ���з��͵�Bye */
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS������ʵʱ��Ƶ����, �յ�ǰ��BYE��Ϣ����, ʵʱ��Ƶ�ر�:�ϼ�CMS ID=%s, IP��ַ=%s, �߼��豸ID=%s, IP��ַ=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->callee_id, pCrData->callee_ip);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Superior CMS real-time video request, receive front-end BYE message processing, real-time video closed: the higher the CMS, ID = % s IP address = % s, logical device ID = % s, IP address = % s", pCrData->caller_id, pCrData->caller_ip, pCrData->callee_id, pCrData->callee_ip);

        /* �����ʵʱ��Ƶҵ�񣬷�������Ҫ��Bye��Ӧ��Ϣת�����в� */
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
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_bye_response_msg_proc() call_record_remove Error:cr_pos=%d, i=%d \r\n", cr_pos, i);
    }
    else
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_bye_response_msg_proc() call_record_remove OK:cr_pos=%d, i=%d \r\n", cr_pos, i);
    }

    return -1;
}

#if 1
/*****************************************************************************
 �� �� ��  : route_info_msg_proc
 ��������  : �ϼ�����CMS���͹�����Info ��Ϣ����
 �������  : char* caller_id
             char* callee_id
             int dialog_index
             char* msg_body
             int msg_body_len
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��5��20��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int route_info_msg_proc(char* caller_id, char* callee_id, int dialog_index, char* msg_body, int msg_body_len)
{
    int i = 0;
    int cr_pos = -1;
    mansrtsp_t* rtsp = NULL;
    cr_t* pCrData = NULL;
    GBDevice_info_t* pCalleeGBDeviceInfo = NULL;
    GBLogicDevice_info_t* pCalleeGBLogicDeviceInfo = NULL;
    float iScale = 0.0;
    int iLen = 0;
    int iNtp = 0;
    char tmpNtp[32] = {0};

    int iCalleeRoutePos = 0;
    route_info_t* pCalleeRouteInfo = NULL;

    if ((NULL == caller_id) || (NULL == callee_id) || (NULL == msg_body) || dialog_index < 0)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_info_msg_proc() exit---: Param Error \r\n");
        return -1;
    }

    //�������

    /* ��ȡ���м�¼ */
    cr_pos = call_record_find_by_caller_index(dialog_index);

    if (cr_pos < 0)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() exit---: Find Call Record Error:dialog_index=%d \r\n", dialog_index);
        return -1;
    }

    pCrData = call_record_get(cr_pos);

    if (NULL == pCrData)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() exit---: Get Call Record Error:cr_pos=%d \r\n", cr_pos);
        return -1;
    }

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS��ʷ��Ƶ�طſ��ƴ���:����ID=%s, IP��ַ=%s, �߼��豸ID=%s, cr_pos=%d", caller_id, pCrData->caller_ip, callee_id, cr_pos);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Superior CMS history video playback control process: the requester, ID = % s IP address = % s, logical device ID = % s, cr_pos = % d", caller_id, pCrData->caller_ip, callee_id, cr_pos);

    /* �����߼��豸��Ϣ */
    pCalleeGBLogicDeviceInfo = GBLogicDevice_info_find(callee_id);

    if (NULL == pCalleeGBLogicDeviceInfo)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_info_msg_proc() exit---: Get Callee GBlogicDevice Info Error \r\n");
        return -1;
    }

    /* �����߼��豸����������жϣ�������Ϣ���� */
    if (1 == pCalleeGBLogicDeviceInfo->other_realm)
    {
        /* �����ϼ�·����Ϣ */
        iCalleeRoutePos = route_info_find(pCalleeGBLogicDeviceInfo->cms_id);

        if (iCalleeRoutePos >= 0)
        {
            pCalleeRouteInfo = route_info_get(iCalleeRoutePos);

            if (NULL != pCalleeRouteInfo)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS��ʷ��Ƶ�طſ��ƴ���, ת�����ϼ�CMS:����ID=%s, IP��ַ=%s, �߼��豸ID=%s, cr_pos=%d, ת���ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d", pCrData->caller_id, pCrData->caller_ip, pCrData->callee_id, cr_pos, pCalleeRouteInfo->server_id, pCalleeRouteInfo->server_ip, pCalleeRouteInfo->server_port);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Superior CMS video playback control processing history, forwarded to the superior CMS: requester, ID = % s IP address = % s, logical device ID = % s, cr_pos = % d, forwarding the superior CMS, ID = % s = % s IP address, port number = % d", pCrData->caller_id, pCrData->caller_ip, pCrData->callee_id, cr_pos, pCalleeRouteInfo->server_id, pCalleeRouteInfo->server_ip, pCalleeRouteInfo->server_port);

                /* ת����Ϣ��ȥ */
                i = SIP_SendInfoWithinDialog(pCrData->callee_ua_index, msg_body, msg_body_len);

                if (0 != i)
                {
                    DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() SIP_SendInfoWithinDialog Error:callee_ua_index=%d, callee_id=%s, callip=%s, iRet=%d \r\n", pCrData->callee_ua_index, pCrData->callee_id, pCrData->callee_ip, i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_info_msg_proc() SIP_SendInfoWithinDialog OK:callee_ua_index=%d, callee_id=%s, callip=%s, iRet=%d \r\n", pCrData->callee_ua_index, pCrData->callee_id, pCrData->callee_ip, i);
                }
            }
            else
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() route_info_get Error:CalleeRoutePos=%d \r\n", iCalleeRoutePos);
            }
        }
        else
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() route_info_get Error:cms_id=%s \r\n", pCalleeGBLogicDeviceInfo->cms_id);
        }
    }
    else
    {
        /* ���Ҷ�Ӧ�������豸 */
        pCalleeGBDeviceInfo = GBDevice_info_get_by_stream_type(pCalleeGBLogicDeviceInfo, pCrData->callee_stream_type);

        if (NULL != pCalleeGBDeviceInfo)
        {
            if (pCalleeGBDeviceInfo->device_type == EV9000_DEVICETYPE_SIPSERVER
                || pCalleeGBLogicDeviceInfo->record_type == 1) /* �¼�CMS ��ȡǰ��¼��*/
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS��ʷ��Ƶ�طſ��ƴ���, ת����ǰ���豸:����ID=%s, IP��ַ=%s, �߼��豸ID=%s, cr_pos=%d, ת��ǰ���豸ID=%s, IP��ַ=%s, �˿ں�=%d", pCrData->caller_id, pCrData->caller_ip, pCrData->callee_id, cr_pos, pCalleeGBDeviceInfo->device_id, pCalleeGBDeviceInfo->login_ip, pCalleeGBDeviceInfo->login_port);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Superior CMS history video playback control processing, forwarded to the front-end equipment: the requester, ID = % s IP address = % s, logical device ID = % s, cr_pos = % d, forwarding the front-end device ID = % s, = % s IP address, port number = % d", pCrData->caller_id, pCrData->caller_ip, pCrData->callee_id, cr_pos, pCalleeGBDeviceInfo->device_id, pCalleeGBDeviceInfo->login_ip, pCalleeGBDeviceInfo->login_port);

                /* ת����Ϣ��ȥ */
                i = SIP_SendInfoWithinDialog(pCrData->callee_ua_index, msg_body, msg_body_len);

                if (0 != i)
                {
                    DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() SIP_SendInfoWithinDialog Error:callee_ua_index=%d, callee_id=%s, callip=%s, iRet=%d \r\n", pCrData->callee_ua_index, pCrData->callee_id, pCrData->callee_ip, i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_info_msg_proc() SIP_SendInfoWithinDialog OK:callee_ua_index=%d, callee_id=%s, callip=%s, iRet=%d \r\n", pCrData->callee_ua_index, pCrData->callee_id, pCrData->callee_ip, i);
                }
            }
            else
            {
                //����RTSP�е���Ϣ�������TSU
                if (0 == strncmp(msg_body, "PLAY", 4))
                {
                    i = notify_tsu_start_replay(pCrData->tsu_ip, pCrData->task_id);

                    if (0 != i)
                    {
                        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() notify_tsu_start_replay Error:tsu_ip=%s, task_id=%s, iRet=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
                    }
                    else
                    {
                        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_info_msg_proc() notify_tsu_start_replay OK:tsu_ip=%s, task_id=%s, iRet=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
                    }

                    /* ����MANSRTSP ��Ϣ */
                    i = mansrtsp_init(&rtsp);

                    if (i != 0)
                    {
                        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() exit---: Mansrtsp Init Error \r\n");
                        return -1;
                    }

                    i = mansrtsp_parse(rtsp, msg_body);

                    if (i != 0)
                    {
                        mansrtsp_free(rtsp);
                        osip_free(rtsp);
                        rtsp = NULL;

                        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() exit---: Mansrtsp Parse Error \r\n");
                        return -1;
                    }

                    /* ������� */
                    if (NULL != rtsp->scale && NULL != rtsp->scale->number)
                    {
                        iScale = atof(rtsp->scale->number);

                        DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_info_msg_proc() Scale Number=%f \r\n", iScale);
                    }
                    else
                    {
                        DEBUG_TRACE(MODULE_ROUTE, LOG_WARN, "route_info_msg_proc() No Scale Value \r\n");
                    }

                    /* �Ϸ� */
                    if (NULL != rtsp->range && NULL != rtsp->range->start)
                    {
                        DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_info_msg_proc() NPT Start=%s \r\n", rtsp->range->start);

                        if (0 != sstrcmp(rtsp->range->start, "now")
                            && 0 != sstrcmp(rtsp->range->start, "196")
                            && 0 != sstrcmp(rtsp->range->start, "196-")
                            && 0 != sstrcmp(rtsp->range->start, "0-")) /* �ϰ汾�ͻ���������ͣ��ļ�����������Я������196����ֹ��ͷ��ʼ���� */
                        {
                            iLen = strlen(rtsp->range->start);

                            if (iLen > 0)
                            {
                                osip_strncpy(tmpNtp, rtsp->range->start, iLen - 1); /* ȥ������"-" */
                                iNtp = osip_atoi(tmpNtp);
                            }
                            else
                            {
                                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() Get NPT Error \r\n");
                            }
                        }
                        else
                        {
                            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() Range Start Now \r\n");
                        }
                    }
                    else
                    {
                        DEBUG_TRACE(MODULE_ROUTE, LOG_WARN, "route_info_msg_proc() No NPT Value \r\n");
                    }

                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS��ʷ��Ƶ�طſ��ƴ���, PLAY�����:����ID=%s, IP��ַ=%s, �߼��豸ID=%s, cr_pos=%d, Scale=%f, NPT=%d", caller_id, pCrData->caller_ip, callee_id, cr_pos, iScale, iNtp);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Superior CMS history video playback control processing , PLAY command processing: the requester, ID = % s IP address = % s, logical device ID = % s, cr_pos = % d, Scale = % f, NPT = % d", caller_id, pCrData->caller_ip, callee_id, cr_pos, iScale, iNtp);

                    if (iScale > 0 || iNtp > 0)
                    {
                        if (iScale > 0 && iScale != 1.0)
                        {
                            pCrData->iScale = iScale;
                        }

                        if (iScale > 0)
                        {
                            i = notify_set_replay_speed(pCrData->tsu_ip, pCrData->task_id, iScale);

                            if (0 != i)
                            {
                                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() notify_set_replay_speed Error:tsu_ip=%s, task_id=%s, Scale=%f, iRet=%d \r\n", pCrData->tsu_ip, pCrData->task_id, iScale, i);
                            }
                            else
                            {
                                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_info_msg_proc() notify_set_replay_speed OK:tsu_ip=%s, task_id=%s, Scale=%f, iRet=%d \r\n", pCrData->tsu_ip, pCrData->task_id, iScale, i);
                            }
                        }

                        if (iNtp > 0)
                        {
                            i = notify_tsu_seek_replay(pCrData->tsu_ip, pCrData->task_id, iNtp);

                            if (0 != i)
                            {
                                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() notify_tsu_seek_replay Error:tsu_ip=%s, task_id=%s, start=%d, iRet=%d \r\n", pCrData->tsu_ip, pCrData->task_id, iNtp, i);
                            }
                            else
                            {
                                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_info_msg_proc() notify_tsu_seek_replay OK:tsu_ip=%s, task_id=%s, start=%d, iRet=%d \r\n", pCrData->tsu_ip, pCrData->task_id, iNtp, i);
                            }
                        }
                    }

                    mansrtsp_free(rtsp);
                    osip_free(rtsp);
                    rtsp = NULL;

                    return i;
                }
                else if (0 == strncmp(msg_body, "PAUSE", 5))
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS��ʷ��Ƶ�طſ��ƴ���, PAUSE�����:����ID=%s, IP��ַ=%s, �߼��豸ID=%s, cr_pos=%d", caller_id, pCrData->caller_ip, callee_id, cr_pos);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Superior CMS history video playback control processing , PAUSE command processing: the requester, ID = % s IP address = % s, logical device ID = % s, cr_pos = % d", caller_id, pCrData->caller_ip, callee_id, cr_pos);

                    i = notify_tsu_pause_replay(pCrData->tsu_ip, pCrData->task_id);

                    if (0 != i)
                    {
                        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() notify_tsu_pause_replay Error:tsu_ip=%s, task_id=%s, iRet=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
                    }
                    else
                    {
                        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_info_msg_proc() notify_tsu_pause_replay OK:tsu_ip=%s, task_id=%s, iRet=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
                    }

                    return i;
                }
                else if (0 == strncmp(msg_body, "TEARDOWN", 8))
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS��ʷ��Ƶ�طſ��ƴ���, TEARDOWN�����:����ID=%s, IP��ַ=%s, �߼��豸ID=%s, cr_pos=%d", caller_id, pCrData->caller_ip, callee_id, cr_pos);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Superior CMS historical video playback control processing , TEARDOWN command processing: the requester, ID = % s IP address = % s, logical device ID = % s, cr_pos = % d", caller_id, pCrData->caller_ip, callee_id, cr_pos);

                    i = notify_tsu_stop_replay(pCrData->tsu_ip, pCrData->task_id);

                    if (0 != i)
                    {
                        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() notify_tsu_stop_replay Error:tsu_ip=%s, task_id=%s, iRet=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
                    }
                    else
                    {
                        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_info_msg_proc() notify_tsu_stop_replay OK:tsu_ip=%s, task_id=%s, iRet=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
                    }

                    return i;
                }
                else if (0 == strncmp(msg_body, "SEEK", 4))
                {
                    /* ����MANSRTSP ��Ϣ */
                    i = mansrtsp_init(&rtsp);

                    if (i != 0)
                    {
                        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() exit---: Mansrtsp Init Error \r\n");
                        return -1;
                    }

                    i = mansrtsp_parse(rtsp, msg_body);

                    if (i != 0)
                    {
                        mansrtsp_free(rtsp);
                        osip_free(rtsp);
                        rtsp = NULL;

                        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() exit---: Mansrtsp Parse Error \r\n");
                        return -1;
                    }

                    if (NULL != rtsp->range && NULL != rtsp->range->start)
                    {
                        DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_info_msg_proc() NPT Start=%s \r\n", rtsp->range->start);

                        if (0 != sstrcmp(rtsp->range->start, "now"))
                        {
                            iLen = strlen(rtsp->range->start);

                            if (iLen > 0)
                            {
                                osip_strncpy(tmpNtp, rtsp->range->start, iLen - 1); /* ȥ������"-" */
                                iNtp = osip_atoi(tmpNtp);
                            }
                            else
                            {
                                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() Get NPT Error \r\n");
                            }

                            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS��ʷ��Ƶ�طſ��ƴ���, SEEK�����:����ID=%s, IP��ַ=%s, �߼��豸ID=%s, cr_pos=%d, NPT=%d", caller_id, pCrData->caller_ip, callee_id, cr_pos, iNtp);
                            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Superior CMS historical video playback control processing, The SEEK command processing: the requester, ID = % s IP address = % s, logical device ID = % s, cr_pos = % d, NPT = % d", caller_id, pCrData->caller_ip, callee_id, cr_pos, iNtp);

                            if (iNtp > 0)
                            {
                                i = notify_tsu_seek_replay(pCrData->tsu_ip, pCrData->task_id, iNtp);

                                if (0 != i)
                                {
                                    DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() notify_tsu_seek_replay Error:tsu_ip=%s, task_id=%s, start=%d, iRet=%d \r\n", pCrData->tsu_ip, pCrData->task_id, osip_atoi(rtsp->range->start), i);
                                }
                                else
                                {
                                    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_info_msg_proc() notify_tsu_seek_replay OK:tsu_ip=%s, task_id=%s, start=%d, iRet=%d \r\n", pCrData->tsu_ip, pCrData->task_id, osip_atoi(rtsp->range->start), i);
                                }
                            }
                            else
                            {
                                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() NPT Value Error \r\n");
                            }
                        }
                        else
                        {
                            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() Range Start Now \r\n");
                        }
                    }

                    mansrtsp_free(rtsp);
                    osip_free(rtsp);
                    rtsp = NULL;

                    return i;
                }
            }
        }
        else
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() GBDevice_info_get_by_stream_type Error:device_id=%s, callee_stream_type=%d \r\n", pCalleeGBLogicDeviceInfo->device_id, pCrData->callee_stream_type);
        }
    }

    return -1;
}
#endif

#if 0
/*****************************************************************************
 �� �� ��  : route_info_msg_proc
 ��������  : �ϼ�����CMS���͹�����Info ��Ϣ����
 �������  : char* caller_id
             char* callee_id
             int dialog_index
             char* msg_body
             int msg_body_len
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��5��20��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int route_info_msg_proc(char* caller_id, char* callee_id, int dialog_index, char* msg_body, int msg_body_len)
{
    int i = 0;
    int cr_pos = -1;
    mansrtsp_t* rtsp = NULL;
    cr_t* pCrData = NULL;
    GBDevice_info_t* pCalleeGBDeviceInfo = NULL;
    GBLogicDevice_info_t* pCalleeGBLogicDeviceInfo = NULL;
    float iScale = 0.0;
    int iLen = 0;
    int iNtp = 0;
    char tmpBuf[128] = {0};
    char tmpNtp[32] = {0};

    int iCalleeRoutePos = 0;
    route_info_t* pCalleeRouteInfo = NULL;

    if ((NULL == caller_id) || (NULL == callee_id) || (NULL == msg_body) || dialog_index < 0)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_info_msg_proc() exit---: Param Error \r\n");
        return -1;
    }

    //�������

    /* ��ȡ���м�¼ */
    cr_pos = call_record_find_by_caller_index(dialog_index);

    //DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_info_msg_proc() call_record_find_by_caller_index:cr_pos=%d \r\n", cr_pos);

    if (cr_pos < 0)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() exit---: Find Call Record Error:dialog_index=%d \r\n", dialog_index);
        return -1;
    }

    pCrData = call_record_get(cr_pos);

    if (NULL == pCrData)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() exit---: Get Call Record Error:cr_pos=%d \r\n", cr_pos);
        return -1;
    }

    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_info_msg_proc():caller_id=%s, caller_ip=%s, callee_id=%s, callee_ip=%s \r\n", pCrData->caller_id, pCrData->caller_ip, pCrData->callee_id, pCrData->callee_ip);

#if 0

    /* �����߼��豸��Ϣ */
    pCalleeGBLogicDeviceInfo = GBLogicDevice_info_find(callee_id);

    if (NULL == pCalleeGBLogicDeviceInfo)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_info_msg_proc() exit---: Get Callee GBlogicDevice Info Error \r\n");
        return -1;
    }

    /* ���Ҷ�Ӧ�������豸 */
    pCalleeGBDeviceInfo = pCalleeGBLogicDeviceInfo->ptGBDeviceInfo;

    if (NULL == pCalleeGBDeviceInfo)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_info_msg_proc() exit---: Get Callee GBDevice Info Error \r\n");
        return -1;
    }

    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_info_msg_proc() CalleeGBDeviceInfo device_type=%d \r\n", pCalleeGBDeviceInfo->device_type);

    if (pCalleeGBDeviceInfo->device_type == EV9000_DEVICETYPE_SIPSERVER || pCalleeGBLogicDeviceInfo->record_type == 1) /* �¼�CMS ��ȡǰ��¼��*/
    {
        /* ת����Ϣ��ȥ */
        SIP_SendInfoWithinDialog(pCrData->callee_ua_index, msg_body, msg_body_len);

        return 0;
    }
    else
#endif
    {
        // TODO:����RTSP�е���Ϣ�������TSU
        if (0 == strncmp(msg_body, "PLAY", 4))
        {
            i = notify_tsu_start_replay(pCrData->tsu_ip, pCrData->task_id);

            if (0 != i)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() notify_tsu_start_replay Error:tsu_ip=%s, task_id=%s, iRet=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
            }
            else
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_info_msg_proc() notify_tsu_start_replay OK:tsu_ip=%s, task_id=%s, iRet=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
            }

            /* ����MANSRTSP ��Ϣ */
            i = mansrtsp_init(&rtsp);

            if (i != 0)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() exit---: Mansrtsp Init Error \r\n");
                return -1;
            }

            i = mansrtsp_parse(rtsp, msg_body);

            if (i != 0)
            {
                mansrtsp_free(rtsp);
                osip_free(rtsp);
                rtsp = NULL;

                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() exit---: Mansrtsp Parse Error \r\n");
                return -1;
            }

            /* ������� */
            if (NULL != rtsp->scale && NULL != rtsp->scale->number)
            {
                iScale = atof(rtsp->scale->number);

                DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_info_msg_proc() Scale Number=%f \r\n", iScale);
            }
            else
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_WARN, "route_info_msg_proc() No Scale Value \r\n");
            }

            /* �Ϸ� */
            if (NULL != rtsp->range && NULL != rtsp->range->start)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_info_msg_proc() NPT Start=%s \r\n", rtsp->range->start);

                if (0 != sstrcmp(rtsp->range->start, "now")
                    && 0 != sstrcmp(rtsp->range->start, "196")
                    && 0 != sstrcmp(rtsp->range->start, "196-")
                    && 0 != sstrcmp(rtsp->range->start, "0-")) /* �ϰ汾�ͻ���������ͣ��ļ�����������Я������196����ֹ��ͷ��ʼ���� */
                {
                    iLen = strlen(rtsp->range->start);

                    if (iLen > 0)
                    {
                        osip_strncpy(tmpNtp, rtsp->range->start, iLen - 1); /* ȥ������"-" */
                        iNtp = osip_atoi(tmpNtp);
                    }
                    else
                    {
                        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() Get NPT Error \r\n");
                    }
                }
                else
                {
                    DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() Range Start Now \r\n");
                }
            }
            else
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_WARN, "route_info_msg_proc() No NPT Value \r\n");
            }

            DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_info_msg_proc() Scale=%f, NPT=%d \r\n", iScale, iNtp);

            if (iScale > 0 || iNtp > 0)
            {
                if (iScale > 0 && iScale != 1.0)
                {
                    pCrData->iScale = iScale;
                }

                /* �����߼��豸��Ϣ */
                pCalleeGBLogicDeviceInfo = GBLogicDevice_info_find(callee_id);

                if (NULL != pCalleeGBLogicDeviceInfo)
                {
                    /* �����߼��豸����������жϣ�������Ϣ���� */
                    if (1 == pCalleeGBLogicDeviceInfo->other_realm)
                    {
                        /* �����ϼ�·����Ϣ */
                        iCalleeRoutePos = route_info_find(pCalleeGBLogicDeviceInfo->cms_id);

                        if (iCalleeRoutePos >= 0)
                        {
                            pCalleeRouteInfo = route_info_get(iCalleeRoutePos);

                            if (NULL != pCalleeRouteInfo)
                            {
                                if (iNtp > 0)
                                {
                                    i = notify_tsu_seek_replay(pCrData->tsu_ip, pCrData->task_id, iNtp);

                                    if (0 != i)
                                    {
                                        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() notify_tsu_seek_replay Error:tsu_ip=%s, task_id=%s, start=%d, iRet=%d \r\n", pCrData->tsu_ip, pCrData->task_id, iNtp, i);
                                    }
                                    else
                                    {
                                        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_info_msg_proc() notify_tsu_seek_replay OK:tsu_ip=%s, task_id=%s, start=%d, iRet=%d \r\n", pCrData->tsu_ip, pCrData->task_id, iNtp, i);
                                    }

                                    /* �Ϸ�֮ǰ�ȷ���һ�»ָ������NVR����ͣ״̬�£�����Ӧ�Ϸ����� */
                                    if (iScale > 0 && iScale != 1.0)
                                    {
                                        snprintf(tmpBuf, 128, (char*)"PLAY RTSP/1.0\r\nCSeq: %s\r\nScale: %f\r\nRange: npt=now-\r\n", rtsp->cseq->number, pCrData->iScale);
                                    }
                                    else
                                    {
                                        snprintf(tmpBuf, 128, (char*)"PLAY RTSP/1.0\r\nCSeq: %s\r\nRange: npt=now-\r\n", rtsp->cseq->number);
                                    }

                                    i = SIP_SendInfoWithinDialog(pCrData->callee_ua_index, tmpBuf, strlen(tmpBuf));

                                    if (0 != i)
                                    {
                                        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() SIP_SendInfoWithinDialog:callee_ua_index=%d, callee_id=%s, callip=%s, iRet=%d \r\n", pCrData->callee_ua_index, pCrData->callee_id, pCrData->callee_ip, i);
                                    }
                                    else
                                    {
                                        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_info_msg_proc() SIP_SendInfoWithinDialog:callee_ua_index=%d, callee_id=%s, callip=%s, iRet=%d \r\n", pCrData->callee_ua_index, pCrData->callee_id, pCrData->callee_ip, i);
                                    }

                                    /* �޸��Ϸ�ʱ�� */
                                    if (pCrData->iPlaybackTimeGap > 0)
                                    {
                                        if (iScale > 0 && iScale != 1.0)
                                        {
                                            snprintf(tmpBuf, 128, (char*)"PLAY RTSP/1.0\r\nCSeq: %s\r\nScale: %f\r\nRange: npt=%d-\r\n", rtsp->cseq->number, pCrData->iScale, iNtp - pCrData->iPlaybackTimeGap);
                                        }
                                        else
                                        {
                                            snprintf(tmpBuf, 128, (char*)"PLAY RTSP/1.0\r\nCSeq: %s\r\nRange: npt=%d-\r\n", rtsp->cseq->number, iNtp - pCrData->iPlaybackTimeGap);
                                        }

                                        DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_info_msg_proc() SIP_SendInfoWithinDialog:Scale=%f, NPT=%d \r\n", iScale, iNtp - pCrData->iPlaybackTimeGap);

                                        i = SIP_SendInfoWithinDialog(pCrData->callee_ua_index, tmpBuf, strlen(tmpBuf));

                                        if (0 != i)
                                        {
                                            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() SIP_SendInfoWithinDialog:callee_ua_index=%d, callee_id=%s, callip=%s, iRet=%d \r\n", pCrData->callee_ua_index, pCrData->callee_id, pCrData->callee_ip, i);
                                        }
                                        else
                                        {
                                            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_info_msg_proc() SIP_SendInfoWithinDialog:callee_ua_index=%d, callee_id=%s, callip=%s, iRet=%d \r\n", pCrData->callee_ua_index, pCrData->callee_id, pCrData->callee_ip, i);
                                        }
                                    }
                                    else
                                    {
                                        /* ת����Ϣ��ȥ */
                                        i = SIP_SendInfoWithinDialog(pCrData->callee_ua_index, msg_body, msg_body_len);

                                        if (0 != i)
                                        {
                                            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() SIP_SendInfoWithinDialog Error:callee_ua_index=%d, callee_id=%s, callip=%s, iRet=%d \r\n", pCrData->callee_ua_index, pCrData->callee_id, pCrData->callee_ip, i);
                                        }
                                        else
                                        {
                                            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_info_msg_proc() SIP_SendInfoWithinDialog OK:callee_ua_index=%d, callee_id=%s, callip=%s, iRet=%d \r\n", pCrData->callee_ua_index, pCrData->callee_id, pCrData->callee_ip, i);
                                        }
                                    }
                                }
                                else
                                {
                                    /* ת����Ϣ��ȥ */
                                    i = SIP_SendInfoWithinDialog(pCrData->callee_ua_index, msg_body, msg_body_len);

                                    if (0 != i)
                                    {
                                        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() SIP_SendInfoWithinDialog Error:callee_ua_index=%d, callee_id=%s, callip=%s, iRet=%d \r\n", pCrData->callee_ua_index, pCrData->callee_id, pCrData->callee_ip, i);
                                    }
                                    else
                                    {
                                        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_info_msg_proc() SIP_SendInfoWithinDialog OK:callee_ua_index=%d, callee_id=%s, callip=%s, iRet=%d \r\n", pCrData->callee_ua_index, pCrData->callee_id, pCrData->callee_ip, i);
                                    }
                                }
                            }
                            else
                            {
                                if (iScale > 0)
                                {
                                    i = notify_set_replay_speed(pCrData->tsu_ip, pCrData->task_id, iScale);

                                    if (0 != i)
                                    {
                                        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() notify_set_replay_speed Error:tsu_ip=%s, task_id=%s, Scale=%f, iRet=%d \r\n", pCrData->tsu_ip, pCrData->task_id, iScale, i);
                                    }
                                    else
                                    {
                                        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_info_msg_proc() notify_set_replay_speed OK:tsu_ip=%s, task_id=%s, Scale=%f, iRet=%d \r\n", pCrData->tsu_ip, pCrData->task_id, iScale, i);
                                    }
                                }

                                if (iNtp > 0)
                                {
                                    i = notify_tsu_seek_replay(pCrData->tsu_ip, pCrData->task_id, iNtp);

                                    if (0 != i)
                                    {
                                        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() notify_tsu_seek_replay Error:tsu_ip=%s, task_id=%s, start=%d, iRet=%d \r\n", pCrData->tsu_ip, pCrData->task_id, iNtp, i);
                                    }
                                    else
                                    {
                                        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_info_msg_proc() notify_tsu_seek_replay OK:tsu_ip=%s, task_id=%s, start=%d, iRet=%d \r\n", pCrData->tsu_ip, pCrData->task_id, iNtp, i);
                                    }
                                }
                            }
                        }
                        else
                        {
                            if (iScale > 0)
                            {
                                i = notify_set_replay_speed(pCrData->tsu_ip, pCrData->task_id, iScale);

                                if (0 != i)
                                {
                                    DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() notify_set_replay_speed Error:tsu_ip=%s, task_id=%s, Scale=%f, iRet=%d \r\n", pCrData->tsu_ip, pCrData->task_id, iScale, i);
                                }
                                else
                                {
                                    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_info_msg_proc() notify_set_replay_speed OK:tsu_ip=%s, task_id=%s, Scale=%f, iRet=%d \r\n", pCrData->tsu_ip, pCrData->task_id, iScale, i);
                                }
                            }

                            if (iNtp > 0)
                            {
                                i = notify_tsu_seek_replay(pCrData->tsu_ip, pCrData->task_id, iNtp);

                                if (0 != i)
                                {
                                    DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() notify_tsu_seek_replay Error:tsu_ip=%s, task_id=%s, start=%d, iRet=%d \r\n", pCrData->tsu_ip, pCrData->task_id, iNtp, i);
                                }
                                else
                                {
                                    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_info_msg_proc() notify_tsu_seek_replay OK:tsu_ip=%s, task_id=%s, start=%d, iRet=%d \r\n", pCrData->tsu_ip, pCrData->task_id, iNtp, i);
                                }
                            }
                        }
                    }
                    else
                    {
                        /* ���Ҷ�Ӧ�������豸 */
                        pCalleeGBDeviceInfo = GBDevice_info_get_by_stream_type(pCalleeGBLogicDeviceInfo, pCrData->callee_stream_type);

                        if (NULL != pCalleeGBDeviceInfo)
                        {
                            if (pCalleeGBDeviceInfo->device_type == EV9000_DEVICETYPE_SIPSERVER || pCalleeGBLogicDeviceInfo->record_type == 1) /* �¼�CMS����ǰ��¼��*/
                            {
                                if (iNtp > 0)
                                {
                                    i = notify_tsu_seek_replay(pCrData->tsu_ip, pCrData->task_id, iNtp);

                                    if (0 != i)
                                    {
                                        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() notify_tsu_seek_replay Error:tsu_ip=%s, task_id=%s, start=%d, iRet=%d \r\n", pCrData->tsu_ip, pCrData->task_id, iNtp, i);
                                    }
                                    else
                                    {
                                        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_info_msg_proc() notify_tsu_seek_replay OK:tsu_ip=%s, task_id=%s, start=%d, iRet=%d \r\n", pCrData->tsu_ip, pCrData->task_id, iNtp, i);
                                    }

                                    /* �Ϸ�֮ǰ�ȷ���һ�»ָ������NVR����ͣ״̬�£�����Ӧ�Ϸ����� */
                                    if (iScale > 0 && iScale != 1.0)
                                    {
                                        snprintf(tmpBuf, 128, (char*)"PLAY RTSP/1.0\r\nCSeq: %s\r\nScale: %f\r\nRange: npt=now-\r\n", rtsp->cseq->number, pCrData->iScale);
                                    }
                                    else
                                    {
                                        snprintf(tmpBuf, 128, (char*)"PLAY RTSP/1.0\r\nCSeq: %s\r\nRange: npt=now-\r\n", rtsp->cseq->number);
                                    }

                                    i = SIP_SendInfoWithinDialog(pCrData->callee_ua_index, tmpBuf, strlen(tmpBuf));

                                    if (0 != i)
                                    {
                                        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() SIP_SendInfoWithinDialog:callee_ua_index=%d, callee_id=%s, callip=%s, iRet=%d \r\n", pCrData->callee_ua_index, pCrData->callee_id, pCrData->callee_ip, i);
                                    }
                                    else
                                    {
                                        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_info_msg_proc() SIP_SendInfoWithinDialog:callee_ua_index=%d, callee_id=%s, callip=%s, iRet=%d \r\n", pCrData->callee_ua_index, pCrData->callee_id, pCrData->callee_ip, i);
                                    }

                                    /* �޸��Ϸ�ʱ�� */
                                    if (pCrData->iPlaybackTimeGap > 0)
                                    {
                                        if (iScale > 0 && iScale != 1.0)
                                        {
                                            snprintf(tmpBuf, 128, (char*)"PLAY RTSP/1.0\r\nCSeq: %s\r\nScale: %f\r\nRange: npt=%d-\r\n", rtsp->cseq->number, pCrData->iScale, iNtp - pCrData->iPlaybackTimeGap);
                                        }
                                        else
                                        {
                                            snprintf(tmpBuf, 128, (char*)"PLAY RTSP/1.0\r\nCSeq: %s\r\nRange: npt=%d-\r\n", rtsp->cseq->number, iNtp - pCrData->iPlaybackTimeGap);
                                        }

                                        DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_info_msg_proc() SIP_SendInfoWithinDialog:Scale=%f, NPT=%d \r\n", iScale, iNtp - pCrData->iPlaybackTimeGap);

                                        i = SIP_SendInfoWithinDialog(pCrData->callee_ua_index, tmpBuf, strlen(tmpBuf));

                                        if (0 != i)
                                        {
                                            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() SIP_SendInfoWithinDialog:callee_ua_index=%d, callee_id=%s, callip=%s, iRet=%d \r\n", pCrData->callee_ua_index, pCrData->callee_id, pCrData->callee_ip, i);
                                        }
                                        else
                                        {
                                            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_info_msg_proc() SIP_SendInfoWithinDialog:callee_ua_index=%d, callee_id=%s, callip=%s, iRet=%d \r\n", pCrData->callee_ua_index, pCrData->callee_id, pCrData->callee_ip, i);
                                        }
                                    }
                                    else
                                    {
                                        /* ת����Ϣ��ȥ */
                                        i = SIP_SendInfoWithinDialog(pCrData->callee_ua_index, msg_body, msg_body_len);

                                        if (0 != i)
                                        {
                                            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() SIP_SendInfoWithinDialog Error:callee_ua_index=%d, callee_id=%s, callip=%s, iRet=%d \r\n", pCrData->callee_ua_index, pCrData->callee_id, pCrData->callee_ip, i);
                                        }
                                        else
                                        {
                                            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_info_msg_proc() SIP_SendInfoWithinDialog OK:callee_ua_index=%d, callee_id=%s, callip=%s, iRet=%d \r\n", pCrData->callee_ua_index, pCrData->callee_id, pCrData->callee_ip, i);
                                        }
                                    }
                                }
                                else
                                {
                                    /* ת����Ϣ��ȥ */
                                    i = SIP_SendInfoWithinDialog(pCrData->callee_ua_index, msg_body, msg_body_len);

                                    if (0 != i)
                                    {
                                        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() SIP_SendInfoWithinDialog Error:callee_ua_index=%d, callee_id=%s, callip=%s, iRet=%d \r\n", pCrData->callee_ua_index, pCrData->callee_id, pCrData->callee_ip, i);
                                    }
                                    else
                                    {
                                        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_info_msg_proc() SIP_SendInfoWithinDialog OK:callee_ua_index=%d, callee_id=%s, callip=%s, iRet=%d \r\n", pCrData->callee_ua_index, pCrData->callee_id, pCrData->callee_ip, i);
                                    }
                                }
                            }
                            else
                            {
                                if (iScale > 0)
                                {
                                    i = notify_set_replay_speed(pCrData->tsu_ip, pCrData->task_id, iScale);

                                    if (0 != i)
                                    {
                                        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() notify_set_replay_speed Error:tsu_ip=%s, task_id=%s, Scale=%f, iRet=%d \r\n", pCrData->tsu_ip, pCrData->task_id, iScale, i);
                                    }
                                    else
                                    {
                                        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_info_msg_proc() notify_set_replay_speed OK:tsu_ip=%s, task_id=%s, Scale=%f, iRet=%d \r\n", pCrData->tsu_ip, pCrData->task_id, iScale, i);
                                    }
                                }

                                if (iNtp > 0)
                                {
                                    i = notify_tsu_seek_replay(pCrData->tsu_ip, pCrData->task_id, iNtp);

                                    if (0 != i)
                                    {
                                        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() notify_tsu_seek_replay Error:tsu_ip=%s, task_id=%s, start=%d, iRet=%d \r\n", pCrData->tsu_ip, pCrData->task_id, iNtp, i);
                                    }
                                    else
                                    {
                                        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_info_msg_proc() notify_tsu_seek_replay OK:tsu_ip=%s, task_id=%s, start=%d, iRet=%d \r\n", pCrData->tsu_ip, pCrData->task_id, iNtp, i);
                                    }
                                }
                            }
                        }
                        else
                        {
                            if (iScale > 0)
                            {
                                i = notify_set_replay_speed(pCrData->tsu_ip, pCrData->task_id, iScale);

                                if (0 != i)
                                {
                                    DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() notify_set_replay_speed Error:tsu_ip=%s, task_id=%s, Scale=%f, iRet=%d \r\n", pCrData->tsu_ip, pCrData->task_id, iScale, i);
                                }
                                else
                                {
                                    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_info_msg_proc() notify_set_replay_speed OK:tsu_ip=%s, task_id=%s, Scale=%f, iRet=%d \r\n", pCrData->tsu_ip, pCrData->task_id, iScale, i);
                                }
                            }

                            if (iNtp > 0)
                            {
                                i = notify_tsu_seek_replay(pCrData->tsu_ip, pCrData->task_id, iNtp);

                                if (0 != i)
                                {
                                    DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() notify_tsu_seek_replay Error:tsu_ip=%s, task_id=%s, start=%d, iRet=%d \r\n", pCrData->tsu_ip, pCrData->task_id, iNtp, i);
                                }
                                else
                                {
                                    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_info_msg_proc() notify_tsu_seek_replay OK:tsu_ip=%s, task_id=%s, start=%d, iRet=%d \r\n", pCrData->tsu_ip, pCrData->task_id, iNtp, i);
                                }
                            }
                        }
                    }
                }
                else
                {
                    if (iScale > 0)
                    {
                        i = notify_set_replay_speed(pCrData->tsu_ip, pCrData->task_id, iScale);

                        if (0 != i)
                        {
                            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() notify_set_replay_speed Error:tsu_ip=%s, task_id=%s, Scale=%f, iRet=%d \r\n", pCrData->tsu_ip, pCrData->task_id, iScale, i);
                        }
                        else
                        {
                            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_info_msg_proc() notify_set_replay_speed OK:tsu_ip=%s, task_id=%s, Scale=%f, iRet=%d \r\n", pCrData->tsu_ip, pCrData->task_id, iScale, i);
                        }
                    }

                    if (iNtp > 0)
                    {
                        i = notify_tsu_seek_replay(pCrData->tsu_ip, pCrData->task_id, iNtp);

                        if (0 != i)
                        {
                            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() notify_tsu_seek_replay Error:tsu_ip=%s, task_id=%s, start=%d, iRet=%d \r\n", pCrData->tsu_ip, pCrData->task_id, iNtp, i);
                        }
                        else
                        {
                            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_info_msg_proc() notify_tsu_seek_replay OK:tsu_ip=%s, task_id=%s, start=%d, iRet=%d \r\n", pCrData->tsu_ip, pCrData->task_id, iNtp, i);
                        }
                    }
                }
            }

            return i;
        }
        else if (0 == strncmp(msg_body, "PAUSE", 5))
        {
            i = notify_tsu_pause_replay(pCrData->tsu_ip, pCrData->task_id);

            if (0 != i)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() notify_tsu_pause_replay Error:tsu_ip=%s, task_id=%s, iRet=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
            }
            else
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_info_msg_proc() notify_tsu_pause_replay OK:tsu_ip=%s, task_id=%s, iRet=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
            }

            return i;
        }
        else if (0 == strncmp(msg_body, "TEARDOWN", 8))
        {
            i = notify_tsu_stop_replay(pCrData->tsu_ip, pCrData->task_id);

            if (0 != i)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() notify_tsu_stop_replay Error:tsu_ip=%s, task_id=%s, iRet=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
            }
            else
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_info_msg_proc() notify_tsu_stop_replay OK:tsu_ip=%s, task_id=%s, iRet=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
            }

            return i;
        }
        else if (0 == strncmp(msg_body, "SEEK", 4))
        {
            /* Seek ������Ҫͬʱ���͸�ǰ�� */
            /* �����߼��豸��Ϣ */
            pCalleeGBLogicDeviceInfo = GBLogicDevice_info_find(callee_id);

            if (NULL != pCalleeGBLogicDeviceInfo)
            {
                /* �����߼��豸����������жϣ�������Ϣ���� */
                if (1 == pCalleeGBLogicDeviceInfo->other_realm)
                {
                    /* �����ϼ�·����Ϣ */
                    iCalleeRoutePos = route_info_find(pCalleeGBLogicDeviceInfo->cms_id);

                    if (iCalleeRoutePos >= 0)
                    {
                        pCalleeRouteInfo = route_info_get(iCalleeRoutePos);

                        if (NULL != pCalleeRouteInfo)
                        {
                            /* ת����Ϣ��ȥ */
                            i = SIP_SendInfoWithinDialog(pCrData->callee_ua_index, msg_body, msg_body_len);

                            if (0 != i)
                            {
                                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() SIP_SendInfoWithinDialog Error:callee_ua_index=%d, callee_id=%s, callip=%s, iRet=%d \r\n", pCrData->callee_ua_index, pCrData->callee_id, pCrData->callee_ip, i);
                            }
                            else
                            {
                                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_info_msg_proc() SIP_SendInfoWithinDialog OK:callee_ua_index=%d, callee_id=%s, callip=%s, iRet=%d \r\n", pCrData->callee_ua_index, pCrData->callee_id, pCrData->callee_ip, i);
                            }
                        }
                    }
                }
                else
                {
                    /* ���Ҷ�Ӧ�������豸 */
                    pCalleeGBDeviceInfo = GBDevice_info_get_by_stream_type(pCalleeGBLogicDeviceInfo, pCrData->callee_stream_type);

                    if (NULL != pCalleeGBDeviceInfo)
                    {
                        if (pCalleeGBDeviceInfo->device_type == EV9000_DEVICETYPE_SIPSERVER || pCalleeGBLogicDeviceInfo->record_type == 1) /* �¼�CMS ��ȡǰ��¼��*/
                        {
                            /* ת����Ϣ��ȥ */
                            i = SIP_SendInfoWithinDialog(pCrData->callee_ua_index, msg_body, msg_body_len);

                            if (0 != i)
                            {
                                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() SIP_SendInfoWithinDialog Error:callee_ua_index=%d, callee_id=%s, callip=%s, iRet=%d \r\n", pCrData->callee_ua_index, pCrData->callee_id, pCrData->callee_ip, i);
                            }
                            else
                            {
                                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_info_msg_proc() SIP_SendInfoWithinDialog OK:callee_ua_index=%d, callee_id=%s, callip=%s, iRet=%d \r\n", pCrData->callee_ua_index, pCrData->callee_id, pCrData->callee_ip, i);
                            }
                        }
                    }
                }
            }

            /* ����MANSRTSP ��Ϣ */
            i = mansrtsp_init(&rtsp);

            if (i != 0)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() exit---: Mansrtsp Init Error \r\n");
                return -1;
            }

            i = mansrtsp_parse(rtsp, msg_body);

            if (i != 0)
            {
                mansrtsp_free(rtsp);
                osip_free(rtsp);
                rtsp = NULL;

                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() exit---: Mansrtsp Parse Error \r\n");
                return -1;
            }

            if (NULL != rtsp->range && NULL != rtsp->range->start)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_info_msg_proc() NPT Start=%s \r\n", rtsp->range->start);

                if (0 != sstrcmp(rtsp->range->start, "now"))
                {
                    iLen = strlen(rtsp->range->start);

                    if (iLen > 0)
                    {
                        osip_strncpy(tmpNtp, rtsp->range->start, iLen - 1); /* ȥ������"-" */
                        iNtp = osip_atoi(tmpNtp);
                    }
                    else
                    {
                        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() Get NPT Error \r\n");
                    }

                    if (iNtp > 0)
                    {
                        i = notify_tsu_seek_replay(pCrData->tsu_ip, pCrData->task_id, iNtp);

                        if (0 != i)
                        {
                            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() notify_tsu_seek_replay Error:tsu_ip=%s, task_id=%s, start=%d, iRet=%d \r\n", pCrData->tsu_ip, pCrData->task_id, osip_atoi(rtsp->range->start), i);
                        }
                        else
                        {
                            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_info_msg_proc() notify_tsu_seek_replay OK:tsu_ip=%s, task_id=%s, start=%d, iRet=%d \r\n", pCrData->tsu_ip, pCrData->task_id, osip_atoi(rtsp->range->start), i);
                        }
                    }
                    else
                    {
                        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() NPT Value Error \r\n");
                    }
                }
                else
                {
                    DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() Range Start Now \r\n");
                }
            }

            mansrtsp_free(rtsp);
            osip_free(rtsp);
            rtsp = NULL;

            return i;
        }
    }

    return -1;
}
#endif

/*****************************************************************************
 �� �� ��  : route_message_msg_proc
 ��������  :�ϼ�����CMS���͹�����Message��Ϣ����
 �������  : route_info_t* pRouteInfo
             char* caller_id
             char* callee_id
             CPacket& inPacket
             DBOper* pRoute_Srv_dboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��5��20��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int route_message_msg_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, char* msg_body, int msg_body_len, DBOper* pRoute_Srv_dboper)
{
    int i = 0;
    int iRet = 0;
    xml_type_t xml_type = XML_TYPE_NULL;
    CPacket inPacket;
    vector<string> NodeName_Vector;

    if (NULL == pRouteInfo)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_message_msg_proc() exit---: Route Info Error \r\n");
        return -1;
    }

    if ((NULL == caller_id) || (NULL == callee_id) || (NULL == msg_body))
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_message_msg_proc() exit---: Param Error \r\n");
        return -1;
    }

    if (NULL == pRoute_Srv_dboper)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG,  "route_message_msg_proc() exit---: Route Srv DB Oper Error \r\n");
        return -1;
    }

    //����XML
    iRet = inPacket.BuiltTree(msg_body, msg_body_len);//����DOM���ṹ.

    if (iRet < 0)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR,  "route_message_msg_proc() exit---: XML Build Tree Error \r\nmsg=%s \r\n", msg_body);
        return iRet;
    }

    NodeName_Vector.clear();
    DOMDocument* pDOMDocument = inPacket.GetDOMDocument();
    DOMElement* pDOMElement = pDOMDocument->get_root();
    pDOMElement->ClearNodeNumber();

    if (pDOMElement->GetNodeName(NodeName_Vector) <= 0)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_message_msg_proc() exit---: Get Node Name Error \r\n");
        return -1;
    }

    /* ������xml����Ϣ���� */
    xml_type = get_xml_type_from_xml_body(NodeName_Vector, inPacket);
    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_message_msg_proc() get_xml_type_from_xml_body:xml_type=%d \r\n", xml_type);

    switch (xml_type)
    {
        case XML_CONTROL_DEVICECONTROL : /* �豸���� */
            i = route_device_control_proc(pRouteInfo, caller_id, callee_id, inPacket, pRoute_Srv_dboper);
            break;

        case XML_CONTROL_DEVICECONFIG : /* �豸���� */
            i = route_device_config_proc(pRouteInfo, caller_id, callee_id, inPacket);
            break;

        case XML_CONTROL_SETDEVICEVIDEOPARAM :    /* ����ǰ��ͼ����� */
            i = route_set_device_video_param_proc(pRouteInfo, caller_id, callee_id, inPacket);
            break;

        case XML_CONTROL_REQUESTIFRAMDATA :        /* ����I ֡ */
            i = route_request_ifram_data_proc(pRouteInfo, caller_id, callee_id, inPacket);
            break;

        case XML_CONTROL_AUTOZOOMIN :              /* ����Ŵ�*/
            i = route_control_autozoomin_proc(pRouteInfo, caller_id, callee_id, inPacket);
            break;

        case XML_CONTROL_SETDEVICEXYPARAM :
            i = route_set_device_xy_param_proc(pRouteInfo, caller_id, callee_id, inPacket, pRoute_Srv_dboper);
            break;

        case XML_CONTROL_EXECUTEPRESET : /* ִ��Ԥ��λ */
            i = route_execute_preset_proc(pRouteInfo, caller_id, callee_id, inPacket, pRoute_Srv_dboper);
            break;

        case XML_QUERY_CATALOG :
            i = route_query_catalog_proc(pRouteInfo, caller_id, callee_id, inPacket, pRoute_Srv_dboper);
            break;

        case XML_QUERY_DEVICEINFO :
            i = route_query_device_info_proc(pRouteInfo, caller_id, callee_id, inPacket, pRoute_Srv_dboper);
            break;

        case XML_QUERY_DEVICESTATUS :
            i = route_query_device_status_proc(pRouteInfo, caller_id, callee_id, inPacket);
            break;

        case XML_QUERY_DEVICECONFIG : /* �豸���ò�ѯ */
            i = route_query_device_config_proc(pRouteInfo, caller_id, callee_id, inPacket);
            break;

        case XML_QUERY_RECORDINFO :
            i = route_query_record_info_proc(pRouteInfo, caller_id, callee_id, inPacket, pRoute_Srv_dboper);
            break;

        case XML_QUERY_GETPRESET :                    /* ��ȡԤ��λ */
            i = route_query_preset_info_proc(pRouteInfo, caller_id, callee_id, inPacket, pRoute_Srv_dboper);
            break;

        case XML_QUERY_DEVICEGROUP :                /* �߼��豸������Ϣ */
            i = route_query_device_group_config_proc(pRouteInfo, caller_id, callee_id, inPacket, pRoute_Srv_dboper);
            break;

        case XML_QUERY_DEVICEMAPGROUP :             /* �߼��豸����ӳ���ϵ */
            i = route_query_device_map_group_config_proc(pRouteInfo, caller_id, callee_id, inPacket, pRoute_Srv_dboper);
            break;

        case XML_QUERY_DEVICEVIDEOPARAM :        /* ��ȡǰ��ͼ�����*/
            i = route_query_device_video_param_proc(pRouteInfo, caller_id, callee_id, inPacket);
            break;

        case XML_QUERY_DEVICEPRESET :            /* ��ȡǰ��Ԥ��λ*/
            i = route_query_device_preset_proc(pRouteInfo, caller_id, callee_id, inPacket);
            break;

        case XML_QUERY_GETDBIP :                 /* ��ȡ���ݿ�IP��ַ */
            i = route_get_db_ip_proc(pRouteInfo, caller_id, callee_id, inPacket);
            break;

        case XML_QUERY_TOPOLOGYPHYDEVICE :       /* ��ȡ���������豸���ñ� */
            i = route_query_topology_phydevice_config_proc(pRouteInfo, caller_id, callee_id, inPacket, pRoute_Srv_dboper);
            break;

        case XML_RESPONSE_QUERY_GETPRESET:
        case XML_RESPONSE_GETDEVICEPRESET:       /* ��ȡǰ��Ԥ��λ��Ӧ  */
            i = route_preset_info_response_proc(pRouteInfo, caller_id, callee_id, inPacket);
            break;

        case XML_NOTIFY_ALARM :
            i = route_notify_alarm_proc(pRouteInfo, caller_id, callee_id, inPacket, pRoute_Srv_dboper);
            break;

        case XML_NOTIFY_KEEPLIVE :
            i = route_notify_keep_alive_proc(pRouteInfo, caller_id, callee_id, inPacket);
            break;

        case XML_NOTIFY_TV_STATUS :
            i = route_notify_tv_status_proc(pRouteInfo, caller_id, callee_id, inPacket);
            break;

        case XML_NOTIFY_CMS_RESTART :
            i = route_notify_cms_restart_proc(pRouteInfo, caller_id, callee_id, inPacket);
            break;

        case XML_NOTIFY_CATALOG:
            i = route_notify_catalog_proc(pRouteInfo, caller_id, callee_id, inPacket, pRoute_Srv_dboper);
            break;

        case XML_NOTIFY_STATUS :
            i = route_notify_status_proc(pRouteInfo, caller_id, callee_id, inPacket, pRoute_Srv_dboper);
            break;

        case XML_RESPONSE_DEVICECONTROL :
            i = route_device_control_response_proc(pRouteInfo, caller_id, callee_id, inPacket);
            break;

        case XML_RESPONSE_ALARM :
            i = route_notify_alarm_response_proc(pRouteInfo, caller_id, callee_id, inPacket);
            break;

        case XML_RESPONSE_CATALOG :
            i = route_query_catalog_response_proc(pRouteInfo, caller_id, callee_id, inPacket, pRoute_Srv_dboper);
            break;

        case XML_RESPONSE_DEVICEINFO :
            i = route_device_info_response_proc(pRouteInfo, caller_id, callee_id, inPacket);
            break;

        case XML_RESPONSE_DEVICESTATUS:
            i = route_device_status_response_proc(pRouteInfo, caller_id, callee_id, inPacket);
            break;

        case XML_RESPONSE_RECORDINFO:
            i = route_record_info_response_proc(pRouteInfo, caller_id, callee_id, inPacket);
            break;

        default:
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_message_msg_proc() exit---: Not Support Message Type:%d \r\n", xml_type);
            return -1;
    }

    return i;
}

/*****************************************************************************
 �� �� ��  : route_notify_msg_proc
 ��������  : ����·��Notify��Ϣ����
 �������  : route_info_t* pRouteInfo
             char* caller_id
             char* callee_id
             char* msg_body
             int msg_body_len
             DBOper* pRoute_Srv_dboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��10��9��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int route_notify_msg_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, char* msg_body, int msg_body_len, DBOper* pRoute_Srv_dboper)
{
    int i = 0;
    int iRet = 0;
    xml_type_t xml_type = XML_TYPE_NULL;
    CPacket inPacket;
    vector<string> NodeName_Vector;

    if (NULL == pRouteInfo)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "device_notify_msg_proc() exit---: GBDevice Info Error \r\n");
        return -1;
    }

    if ((NULL == caller_id) || (NULL == callee_id) || (NULL == msg_body))
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG,  "device_notify_msg_proc() exit---: Param Error \r\n");
        return -1;
    }

    if (NULL == pRoute_Srv_dboper)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG,  "device_notify_msg_proc() exit---: Device Srv DB Oper Error \r\n");
        return -1;
    }

    //����XML
    iRet = inPacket.BuiltTree(msg_body, msg_body_len);//����DOM���ṹ.

    if (iRet < 0)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR,  "device_notify_msg_proc() exit---: XML Build Tree Error \r\nmsg=%s \r\n", msg_body);
        return iRet;
    }

    NodeName_Vector.clear();
    DOMDocument* pDOMDocument = inPacket.GetDOMDocument();
    DOMElement* pDOMElement = pDOMDocument->get_root();
    pDOMElement->ClearNodeNumber();

    if (pDOMElement->GetNodeName(NodeName_Vector) <= 0)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR,  "device_notify_msg_proc() exit---: Get XML Node Name Error \r\n");
        return -1;
    }

    /* ������xml����Ϣ���� */
    xml_type = get_xml_type_from_xml_body(NodeName_Vector, inPacket);
    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE,  "device_notify_msg_proc() get_xml_type_from_xml_body:xml_type=%d \r\n", xml_type);

    switch (xml_type)
    {
        case XML_NOTIFY_CATALOG:
            i = route_notify_catalog_proc(pRouteInfo, caller_id, callee_id, inPacket, pRoute_Srv_dboper);
            break;

        case XML_NOTIFY_STATUS :
            i = route_notify_status_proc(pRouteInfo, caller_id, callee_id, inPacket, pRoute_Srv_dboper);
            break;

        default:
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR,  "device_notify_msg_proc() exit---: Not Support Message Type:%d \r\n", xml_type);
            return -1;
    }

    return i;
}

/*****************************************************************************
 �� �� ��  : route_subscribe_msg_proc
 ��������  : �ϼ�����·�ɹ����Ķ�����Ϣ����
 �������  : route_info_t* pRouteInfo
             char* caller_id
             char* callee_id
             int event_id
             int subscribe_expires
             char* msg_body
             int msg_body_len
             DBOper* pRoute_Srv_dboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��6��10�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int route_subscribe_msg_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, int event_id, int subscribe_expires, char* msg_body, int msg_body_len, DBOper* pRoute_Srv_dboper)
{
    int i = 0;
    int iRet = 0;
    xml_type_t xml_type = XML_TYPE_NULL;
    CPacket inPacket;
    vector<string> NodeName_Vector;

    if (NULL == pRouteInfo)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_subscribe_msg_proc() exit---: Route Info Error \r\n");
        return -1;
    }

    if ((NULL == caller_id) || (NULL == callee_id) || (NULL == msg_body))
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_subscribe_msg_proc() exit---: Param Error \r\n");
        return -1;
    }

    if (NULL == pRoute_Srv_dboper)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG,  "route_subscribe_msg_proc() exit---: Route Srv DB Oper Error \r\n");
        return -1;
    }

    //����XML
    iRet = inPacket.BuiltTree(msg_body, msg_body_len);//����DOM���ṹ.

    if (iRet < 0)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR,  "route_subscribe_msg_proc() exit---: XML Build Tree Error \r\nmsg=%s \r\n", msg_body);
        return iRet;
    }

    NodeName_Vector.clear();
    DOMDocument* pDOMDocument = inPacket.GetDOMDocument();
    DOMElement* pDOMElement = pDOMDocument->get_root();
    pDOMElement->ClearNodeNumber();

    if (pDOMElement->GetNodeName(NodeName_Vector) <= 0)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_subscribe_msg_proc() exit---: Get Node Name Error \r\n");
        return -1;
    }

    /* ������xml����Ϣ���� */
    xml_type = get_xml_type_from_xml_body(NodeName_Vector, inPacket);
    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_subscribe_msg_proc() get_xml_type_from_xml_body:xml_type=%d \r\n", xml_type);

    switch (xml_type)
    {
        case XML_QUERY_CATALOG :
            i = route_subscribe_query_catalog_proc(pRouteInfo, caller_id, callee_id, event_id, subscribe_expires, inPacket, pRoute_Srv_dboper);
            break;

        default:
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_subscribe_msg_proc() exit---: Not Support Message Type:%d \r\n", xml_type);
            return -1;
    }

    return i;
}

int route_subscribe_within_dialog_msg_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, int dialog_index, int subscribe_expires, char* msg_body, int msg_body_len, DBOper* pRoute_Srv_dboper)
{
    int i = 0;
    int iRet = 0;
    xml_type_t xml_type = XML_TYPE_NULL;
    CPacket inPacket;
    vector<string> NodeName_Vector;

    if (NULL == pRouteInfo)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_subscribe_within_dialog_msg_proc() exit---: Route Info Error \r\n");
        return -1;
    }

    if ((NULL == caller_id) || (NULL == callee_id) || (NULL == msg_body))
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_subscribe_within_dialog_msg_proc() exit---: Param Error \r\n");
        return -1;
    }

    if (NULL == pRoute_Srv_dboper)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG,  "route_subscribe_within_dialog_msg_proc() exit---: Route Srv DB Oper Error \r\n");
        return -1;
    }

    //����XML
    iRet = inPacket.BuiltTree(msg_body, msg_body_len);//����DOM���ṹ.

    if (iRet < 0)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR,  "route_subscribe_within_dialog_msg_proc() exit---: XML Build Tree Error \r\nmsg=%s \r\n", msg_body);
        return iRet;
    }

    NodeName_Vector.clear();
    DOMDocument* pDOMDocument = inPacket.GetDOMDocument();
    DOMElement* pDOMElement = pDOMDocument->get_root();
    pDOMElement->ClearNodeNumber();

    if (pDOMElement->GetNodeName(NodeName_Vector) <= 0)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_subscribe_within_dialog_msg_proc() exit---: Get Node Name Error \r\n");
        return -1;
    }

    /* ������xml����Ϣ���� */
    xml_type = get_xml_type_from_xml_body(NodeName_Vector, inPacket);
    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_subscribe_within_dialog_msg_proc() get_xml_type_from_xml_body:xml_type=%d \r\n", xml_type);

    switch (xml_type)
    {
        case XML_QUERY_CATALOG :
            i = route_subscribe_witin_dialog_query_catalog_proc(pRouteInfo, caller_id, callee_id, dialog_index, subscribe_expires, inPacket, pRoute_Srv_dboper);
            break;

        default:
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_subscribe_within_dialog_msg_proc() exit---: Not Support Message Type:%d \r\n", xml_type);
            return -1;
    }

    return i;
}

/*****************************************************************************
 �� �� ��  : route_device_control_proc
 ��������  : �ϼ�����CMS���͹������豸������Ϣ����
 �������  : route_info_t* pRouteInfo
             char* caller_id
             char* callee_id
             CPacket& inPacket
             DBOper* pRoute_Srv_dboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��5��20��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int route_device_control_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, CPacket& inPacket, DBOper* pRoute_Srv_dboper)
{
    int i = 0;
    int iLen = 0;
    GBDevice_info_t* pGBDeviceInfo = NULL;
    GBLogicDevice_info_t* pGBLogicDeviceInfo = NULL;

    char strSN[32] = {0};
    char strDeviceID[32] = {0};
    char strPTZCmd[64] = {0};      /* ���/��̨�������� */
    char strRecordCmd[32] = {0};   /* ¼��������� */
    char strGuardCmd[32] = {0};    /* ��������/�������� */
    char strLockCmd[32] = {0};     /* ��λ�������� */
    char strTeleBootCmd[32] = {0}; /* Զ�������������� */
    char strAlarmCmd[32] = {0};    /* ������λ���� */

    time_t utc_time;
    struct tm local_time = { 0 };
    char str_date[12] = {0};
    char str_time[12] = {0};
    char strTime[32] = {0};
    char strReason[128] = {0};     /* ԭ�� */

    CPacket outPacket;
    DOMElement* AccNode = NULL;

    int iCalleeRoutePos = 0;
    route_info_t* pCalleeRouteInfo = NULL;

    char strTransferSN[32] = {0};
    DOMElement* AccSnNode = NULL;
    unsigned int old_xml_sn = 0;
    unsigned int transfer_xml_sn = 0;

    unsigned char szPtzCmd[PTZCMD_28181_LEN + 1] = {0};

    if (NULL == pRouteInfo)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_device_control_proc() exit---: Route Info Error \r\n");
        return -1;
    }

    if (NULL == caller_id || NULL == callee_id || NULL == pRoute_Srv_dboper)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_device_control_proc() exit---: Param Error \r\n");
        return -1;
    }

    /* �豸���Ƶ�����ֱ��ת����ǰ���豸����������
          �������̼�9.3.2

          ������������ֶ�:
          <!-- �������ͣ��豸���ƣ���ѡ�� -->
          <element name="CmdType" fixed ="DeviceControl" />
          <!-- �������кţ���ѡ�� -->
          <element name="SN" type="integer" minInclusive value = "1" />
          <!-- Ŀ���豸���루��ѡ�� -->
          <element name="DeviceID" type="tg:deviceIDType" />
          <!-- ���/��̨���������ѡ��������Ӧ���ϸ�¼L�еĹ涨) -->
          <element name=" PTZCmd " type="tg:PTZType" />
          <!-- Զ���������������ѡ�� -->
          <element name="TeleBoot" minOccurs= "0">
          <restriction base="string">
          <enumeration value="Boot"/>
          </restriction>
          </element>
          <!-- ¼����������ѡ�� -->
          <element name=" RecordCmd " type="tg:recordType" minOccurs= "0"/>
          <!-- ��������/���������ѡ�� -->
          <element name=" GuardCmd " type="tg:guardType" minOccurs= "0"/>
          <!-- ������λ�����ѡ�� -->
          <element name="AlarmCmd" minOccurs= "0">
          <restriction base="string">
          <enumeration value="ResetAlarm"/>
          </restriction>
          </element>
          <!-- ��չ��Ϣ���ɶ��� -->
          <element name= "Info" minOccurs= "0" maxOccurs="unbounded">
          <restriction base= "string">
          <maxLength value= "1024" />
          </restriction>
          </element>
      */

    /* ȡ������*/
    inPacket.GetElementValue((char*)"SN", strSN);
    inPacket.GetElementValue((char*)"DeviceID", strDeviceID);
    inPacket.GetElementValue((char*)"PTZCmd", strPTZCmd);
    inPacket.GetElementValue((char*)"RecordCmd", strRecordCmd);
    inPacket.GetElementValue((char*)"GuardCmd", strGuardCmd);
    inPacket.GetElementValue((char*)"LockCmd", strLockCmd);
    inPacket.GetElementValue((char*)"TeleBoot", strTeleBootCmd);
    inPacket.GetElementValue((char*)"AlarmCmd", strAlarmCmd);

    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO,  "route_device_control_proc() \
    \r\n XML Para: \
    \r\n SN=%s, DeviceID=%s, PTZCmd=%s, RecordCmd=%s, GuardCmd=%s, LockCmd=%s, TeleBoot=%s, AlarmCmd=%s \r\n", strSN, strDeviceID, strPTZCmd, strRecordCmd, strGuardCmd, strLockCmd, strTeleBootCmd, strAlarmCmd);

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS�������豸���������:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ����:PTZCmd=%s, RecordCmd=%s, GuardCmd=%s, LockCmd=%s, TeleBoot=%s, AlarmCmd=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, strPTZCmd, strRecordCmd, strGuardCmd, strLockCmd, strTeleBootCmd, strAlarmCmd);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Superior CMS equipment control command processing, superior CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s, command: PTZCmd = % s, RecordCmd = % s, GuardCmd = % s, LockCmd = % s, TeleBoot = % s, AlarmCmd = % s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, strPTZCmd, strRecordCmd, strGuardCmd, strLockCmd, strTeleBootCmd, strAlarmCmd);

    /* �������� */
    if (strTeleBootCmd[0] != '\0')
    {
        if (0 == sstrcmp(strTeleBootCmd, (char*)"Boot"))
        {
            if (0 == sstrcmp(strDeviceID, local_cms_id_get()))
            {
                /* ����ȥע�� */
                i = SIP_SendUnRegister(pRouteInfo->reg_info_index);

                if (i < 0)
                {
                    DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_device_control_proc() SIP_SendUnRegister Error:reg_info_index=%d, iRet=%d \r\n", pRouteInfo->reg_info_index, i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_device_control_proc() SIP_SendUnRegister OK:reg_info_index=%d, iRet=%d \r\n", pRouteInfo->reg_info_index, i);
                }

                osip_usleep(5000000);

                BoardReboot();

                return i;
            }
        }
    }

    /* ��λ���� */
    if (strLockCmd[0] != '\0')
    {
        i = RouteLockDeviceProc(strLockCmd, strDeviceID, pRouteInfo);
    }

    /* �ֶ�¼�� */
    if (strRecordCmd[0] != '\0')
    {
        pGBLogicDeviceInfo = GBLogicDevice_info_find(strDeviceID);

        if (NULL != pGBLogicDeviceInfo)
        {
            pGBDeviceInfo = GBDevice_info_get_by_stream_type(pGBLogicDeviceInfo, EV9000_STREAM_TYPE_MASTER);

            if (EV9000_DEVICETYPE_SIPSERVER != pGBDeviceInfo->device_type)
            {
                if (0 == sstrcmp(strRecordCmd, (char*)"Record")) /* ��ʼ�ֶ�¼�� */
                {
                    if (0 == pGBLogicDeviceInfo->record_type) /* ����¼�� */
                    {
                        /* ����¼�� */
                        i = add_record_info_by_message_cmd(pGBLogicDeviceInfo->id, pRoute_Srv_dboper);

                        if (i != 0)
                        {
                            SystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�������豸���������, �ֶ�����¼��ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"���¼������ʧ��");
                            EnSystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "The equipment control command of superior CMS processing, manual start video failure: the higher the CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s, reason = % s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"���¼������ʧ��");
                            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_device_control_proc() add_record_info_by_message_cmd Error \r\n");
                        }
                        else
                        {
                            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS�������豸���������, �ֶ�����¼��ɹ�:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);
                            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "The equipment control command of superior CMS processing, manual start video success: the higher the CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);
                            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_device_control_proc() add_record_info_by_message_cmd OK \r\n");
                        }

                        /* �齨XML��Ϣ */
                        outPacket.SetRootTag("Response");
                        AccNode = outPacket.CreateElement((char*)"CmdType");
                        outPacket.SetElementValue(AccNode, (char*)"DeviceControl");

                        AccNode = outPacket.CreateElement((char*)"SN");
                        outPacket.SetElementValue(AccNode, strSN);

                        AccNode = outPacket.CreateElement((char*)"DeviceID");
                        outPacket.SetElementValue(AccNode, (char*)strDeviceID);

                        AccNode = outPacket.CreateElement((char*)"Result");
                        outPacket.SetElementValue(AccNode, (char*)"OK");

                        /* ������Ӧ��Ϣ���ϼ�CMS */
                        i = SIP_SendMessage(NULL, local_cms_id_get(), caller_id, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

                        if (i != 0)
                        {
                            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_device_control_proc() SIP_SendMessage Error:server_id=%s, server_ip=%s, server_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                        }
                        else
                        {
                            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_device_control_proc() SIP_SendMessage OK:server_id=%s, server_ip=%s, server_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                        }

                        return i;
                    }
                }
                else if (0 == sstrcmp(strRecordCmd, (char*)"StopRecord")) /* ֹͣ�ֶ�¼�� */
                {
                    if (0 == pGBLogicDeviceInfo->record_type) /* ����¼�� */
                    {
                        /* ֹͣ¼�� */
                        i = del_record_info_by_message_cmd(pGBLogicDeviceInfo->id, pRoute_Srv_dboper);

                        if (i != 0)
                        {
                            SystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�������豸���������, �ֶ�ֹͣ¼��ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"ɾ��¼������ʧ��");
                            EnSystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "The equipment control command of superior CMS processing, Manual stop video failure: the superior CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s, reason = % s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"ɾ��¼������ʧ��");
                            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_device_control_proc() del_record_info_by_message_cmd Error \r\n");
                        }
                        else
                        {
                            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS�������豸���������, �ֶ�ֹͣ¼��ɹ�:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);
                            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "The equipment control command of superior CMS processing, Manual stop video success: the superior CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);
                            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_device_control_proc() del_record_info_by_message_cmd OK \r\n");
                        }

                        /* �齨XML��Ϣ */
                        outPacket.SetRootTag("Response");
                        AccNode = outPacket.CreateElement((char*)"CmdType");
                        outPacket.SetElementValue(AccNode, (char*)"DeviceControl");

                        AccNode = outPacket.CreateElement((char*)"SN");
                        outPacket.SetElementValue(AccNode, strSN);

                        AccNode = outPacket.CreateElement((char*)"DeviceID");
                        outPacket.SetElementValue(AccNode, (char*)strDeviceID);

                        AccNode = outPacket.CreateElement((char*)"Result");
                        outPacket.SetElementValue(AccNode, (char*)"OK");

                        /* ������Ӧ��Ϣ���ϼ�CMS */
                        i = SIP_SendMessage(NULL, local_cms_id_get(), caller_id, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

                        if (i != 0)
                        {
                            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_device_control_proc() SIP_SendMessage Error:server_id=%s, server_ip=%s, server_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                        }
                        else
                        {
                            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_device_control_proc() SIP_SendMessage OK:server_id=%s, server_ip=%s, server_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                        }

                        return i;
                    }
                }
                else
                {
                    SystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�������豸���������, �ֶ�¼��ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=��֧�ֵ�¼������:RecordCmd=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, strRecordCmd);
                    EnSystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "Device control command process from superior CMS, Manual recording failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=do not support video command:RecordCmd=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, strRecordCmd);
                    DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_device_control_proc() Record Cmd Error: RecordCmd=%s \r\n", strRecordCmd);
                }
            }
        }
        else
        {
            SystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�������豸���������, �ֶ�¼��ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=�����߼��豸ʧ��:DeviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, strDeviceID);
            EnSystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "Device control command process from superior CMS, Manual recording failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause= search for logic device failed:DeviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, strDeviceID);
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_device_control_proc() exit---: Find GB LogicDevice Info Error: DeviceID=%s \r\n", strDeviceID);
        }
    }

    /* ���������� */
    if (strGuardCmd[0] != '\0')
    {
        pGBLogicDeviceInfo = GBLogicDevice_info_find(strDeviceID);

        if (NULL != pGBLogicDeviceInfo)
        {
            if (0 == sstrcmp(strGuardCmd, (char*)"SetGuard")) /* ����*/
            {
                pGBLogicDeviceInfo->guard_type = 1;
                DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_device_control_proc() GBLogicDevice=%s, GuardStatus=%d \r\n", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->guard_type);
            }
            else if (0 == sstrcmp(strGuardCmd, (char*)"ResetGuard")) /*  ���� */
            {
                pGBLogicDeviceInfo->guard_type = 0;
                DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_device_control_proc() GBLogicDevice=%s, GuardStatus=%d \r\n", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->guard_type);
            }
            else
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_device_control_proc() Guard Cmd Error: GuardCmd=%s \r\n", strGuardCmd);
            }
        }
        else
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_device_control_proc() exit---: Find GB LogicDevice Info Error: DeviceID=%s \r\n", strDeviceID);
        }

        /* �齨XML��Ϣ */
        outPacket.SetRootTag("Response");
        AccNode = outPacket.CreateElement((char*)"CmdType");
        outPacket.SetElementValue(AccNode, (char*)"DeviceControl");

        AccNode = outPacket.CreateElement((char*)"SN");
        outPacket.SetElementValue(AccNode, strSN);

        AccNode = outPacket.CreateElement((char*)"DeviceID");
        outPacket.SetElementValue(AccNode, (char*)strDeviceID);

        AccNode = outPacket.CreateElement((char*)"Result");
        outPacket.SetElementValue(AccNode, (char*)"OK");

        /* ������Ӧ��Ϣ���ϼ�CMS */
        i = SIP_SendMessage(NULL, local_cms_id_get(), caller_id, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

        if (i != 0)
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_device_control_proc() SIP_SendMessage Error:server_id=%s, server_ip=%s, server_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
        }
        else
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_device_control_proc() SIP_SendMessage OK:server_id=%s, server_ip=%s, server_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
        }
    }

    /* ������λ */
    if (strAlarmCmd[0] != '\0')
    {
        pGBLogicDeviceInfo = GBLogicDevice_info_find(strDeviceID);

        if (NULL != pGBLogicDeviceInfo)
        {
            if (0 == sstrcmp(strAlarmCmd, (char*)"ResetAlarm")) /* ��λ�澯 */
            {
                pGBLogicDeviceInfo->guard_type = 0;
                DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_device_control_proc() GBLogicDevice=%s, GuardStatus=%d \r\n", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->guard_type);
            }
            else
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_device_control_proc() Alarm Cmd Error: AlarmCmd=%s \r\n", strAlarmCmd);
            }
        }
        else
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_device_control_proc() exit---: Find GB LogicDevice Info Error: DeviceID=%s \r\n", strDeviceID);
        }

        /* �齨XML��Ϣ */
        outPacket.SetRootTag("Response");
        AccNode = outPacket.CreateElement((char*)"CmdType");
        outPacket.SetElementValue(AccNode, (char*)"DeviceControl");

        AccNode = outPacket.CreateElement((char*)"SN");
        outPacket.SetElementValue(AccNode, strSN);

        AccNode = outPacket.CreateElement((char*)"DeviceID");
        outPacket.SetElementValue(AccNode, (char*)strDeviceID);

        AccNode = outPacket.CreateElement((char*)"Result");
        outPacket.SetElementValue(AccNode, (char*)"OK");

        /* ������Ӧ��Ϣ���ϼ�CMS */
        i = SIP_SendMessage(NULL, local_cms_id_get(), caller_id, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

        if (i != 0)
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_device_control_proc() SIP_SendMessage Error:server_id=%s, server_ip=%s, server_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
        }
        else
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_device_control_proc() SIP_SendMessage OK:server_id=%s, server_ip=%s, server_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
        }
    }

    /* PTZ��̨�������� */
    if (strPTZCmd[0] != '\0')
    {
        /* ����ǿ��������Ҫ����߼���λ�Ƿ����� */
        pGBLogicDeviceInfo = GBLogicDevice_info_find(strDeviceID);

        if (NULL != pGBLogicDeviceInfo)
        {
            if (LOCK_STATUS_USER_LOCK == pGBLogicDeviceInfo->lock_status)
            {
                if (NULL != pGBLogicDeviceInfo->pLockUserInfo)
                {
                    /* �ϼ������ܱ����û��������� */
                }
                else /* ԭ���û�����ʧЧ */
                {
                    pGBLogicDeviceInfo->lock_status = LOCK_STATUS_OFF;
                    pGBLogicDeviceInfo->pLockUserInfo = NULL;
                    pGBLogicDeviceInfo->pLockRouteInfo = NULL;
                }
            }
            else if (LOCK_STATUS_ROUTE_LOCK == pGBLogicDeviceInfo->lock_status)
            {
                if (NULL != pGBLogicDeviceInfo->pLockRouteInfo)
                {
                    if (0 == pGBLogicDeviceInfo->pLockRouteInfo->reg_status)
                    {
                        pGBLogicDeviceInfo->lock_status = LOCK_STATUS_OFF;
                        pGBLogicDeviceInfo->pLockUserInfo = NULL;
                        pGBLogicDeviceInfo->pLockRouteInfo = NULL;
                    }
                    else
                    {
                        if (pGBLogicDeviceInfo->pLockRouteInfo == pRouteInfo)
                        {
                            /* ��������ʱ�� */
                            i = device_auto_unlock_update(pGBLogicDeviceInfo->id);
                        }
                        else
                        {
                            /* ���ͱ������ݸ��ͻ����û� */
                            outPacket.SetRootTag("Notify");

                            AccNode = outPacket.CreateElement((char*)"CmdType");
                            outPacket.SetElementValue(AccNode, (char*)"Alarm");

                            AccNode = outPacket.CreateElement((char*)"SN");
                            outPacket.SetElementValue(AccNode, strSN);

                            AccNode = outPacket.CreateElement((char*)"DeviceID");
                            outPacket.SetElementValue(AccNode, strDeviceID);

                            AccNode = outPacket.CreateElement((char*)"AlarmPriority");
                            outPacket.SetElementValue(AccNode, (char*)"4");

                            AccNode = outPacket.CreateElement((char*)"AlarmMethod");
                            outPacket.SetElementValue(AccNode, (char*)"5");

                            AccNode = outPacket.CreateElement((char*)"AlarmTime");
                            utc_time = time(NULL);
                            localtime_r(&utc_time, &local_time);
                            strftime(str_date, sizeof(str_date), "%Y-%m-%d", &local_time);
                            strftime(str_time, sizeof(str_time), "%H:%M:%S", &local_time);
                            snprintf(strTime, 32, "%sT%s", str_date, str_time);
                            outPacket.SetElementValue(AccNode, strTime);

                            AccNode = outPacket.CreateElement((char*)"AlarmDescription");
                            snprintf(strReason, 128, "�߼��豸�Ѿ����ϼ�����:�����ϼ�CMD ID=%s, �����ϼ�CMS IP��ַ=%s, �����ϼ�CMS�˿ں�=%d", pGBLogicDeviceInfo->pLockRouteInfo->server_id, pGBLogicDeviceInfo->pLockRouteInfo->server_ip, pGBLogicDeviceInfo->pLockRouteInfo->server_port);
                            outPacket.SetElementValue(AccNode, strReason);

                            /* ������Ӧ��Ϣ���ϼ�CMS */
                            i = SIP_SendMessage(NULL, local_cms_id_get(), caller_id, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

                            if (i != 0)
                            {
                                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_device_control_proc() SIP_SendMessage Error:server_id=%s, server_ip=%s, server_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                            }
                            else
                            {
                                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_device_control_proc() SIP_SendMessage OK:server_id=%s, server_ip=%s, server_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                            }

                            SystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�������豸���������ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=�߼��豸�Ѿ����ϼ�����:�����ϼ�CMD ID=%s, �����ϼ�CMS IP��ַ=%s, �����ϼ�CMS�˿ں�=%d", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, pGBLogicDeviceInfo->pLockRouteInfo->server_id, pGBLogicDeviceInfo->pLockRouteInfo->server_ip, pGBLogicDeviceInfo->pLockRouteInfo->server_port);
                            EnSystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "Device control command process from superior CMS failed:CMS ID=%s, IP=%s, Port=%d, Logic Device ID=%s,  reason=Logical device has been locked", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID);
                            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_device_control_proc() Record Cmd Error: GBLogicDevice Locked: ID=%s \r\n", strDeviceID);
                            return -1;
                        }
                    }
                }
                else /* ԭ���ϼ�ƽ̨����ʧЧ */
                {
                    pGBLogicDeviceInfo->lock_status = LOCK_STATUS_OFF;
                    pGBLogicDeviceInfo->pLockUserInfo = NULL;
                    pGBLogicDeviceInfo->pLockRouteInfo = NULL;
                }
            }
        }
    }

    /* ת����Ϣ��ǰ�� */
    pGBLogicDeviceInfo = GBLogicDevice_info_find(strDeviceID);

    if (NULL != pGBLogicDeviceInfo)
    {
        /* ������ĵ�λ */
        if (1 == pGBLogicDeviceInfo->other_realm)
        {
            /* �����ϼ�·����Ϣ */
            iCalleeRoutePos = route_info_find(pGBLogicDeviceInfo->cms_id);

            if (iCalleeRoutePos < 0)
            {
                SystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�������豸���������ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=û���ҵ���Ӧ���ϼ�·����Ϣ:cms_id=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBLogicDeviceInfo->cms_id);
                EnSystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "Device control command process from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause= corresponding route info not found:cms_id=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBLogicDeviceInfo->cms_id);
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_device_control_proc() exit---: Get LogicDevice's Route Info Error: DeviceID=%s \r\n", callee_id);
                return -1;
            }

            pCalleeRouteInfo = route_info_get(iCalleeRoutePos);

            if (NULL == pCalleeRouteInfo)
            {
                SystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�������豸���������ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=û���ҵ���Ӧ���ϼ�·����Ϣ:cms_id=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBLogicDeviceInfo->cms_id);
                EnSystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "Device control command process from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause= corresponding route info not found:cms_id=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBLogicDeviceInfo->cms_id);
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_device_control_proc() exit---: Get LogicDevice's Route Info Error: DeviceID=%s \r\n", callee_id);
                return -1;
            }

            if (strPTZCmd[0] != '\0') /* ����������Ԥ��λ�����ģ�ǰ���з��� */
            {
                iLen = String2Bytes((unsigned char*)strPTZCmd, szPtzCmd, PTZCMD_28181_LEN);

                if (szPtzCmd[3] == 0x81 || szPtzCmd[3] == 0x83)
                {
                    /* ��ȡ�ϵ�SN�ڵ� */
                    AccSnNode = inPacket.SearchElement((char*)"SN");

                    if (NULL != AccSnNode)
                    {
                        g_transfer_xml_sn++;
                        snprintf(strTransferSN, 32, "%u", g_transfer_xml_sn);
                        inPacket.SetElementValue(AccSnNode, strTransferSN);
                    }

                    i = SIP_SendMessage(NULL, local_cms_id_get(), callee_id, pCalleeRouteInfo->strRegLocalIP, pCalleeRouteInfo->iRegLocalPort, pCalleeRouteInfo->server_ip, pCalleeRouteInfo->server_port, (char*)inPacket.GetXml(NULL).c_str(), inPacket.GetXml(NULL).length());

                    if (i != 0)
                    {
                        SystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�������豸���������ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=%s, ǰ�������豸ID=%s, IP=%s, �˿�=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"ת����Ϣ���ϼ�CMSʧ��", pCalleeRouteInfo->server_id, pCalleeRouteInfo->server_ip, pCalleeRouteInfo->server_port);
                        EnSystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "Device control command process from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=%s, front-end physical device ID=%s, IP=%s, port=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"forward message to superior CMS failed", pCalleeRouteInfo->server_id, pCalleeRouteInfo->server_ip, pCalleeRouteInfo->server_port);
                        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_device_control_proc() SIP_SendMessage Error:server_id=%s, server_ip=%s, server_port=%d \r\n", pCalleeRouteInfo->server_id, pCalleeRouteInfo->server_ip, pCalleeRouteInfo->server_port);
                    }
                    else
                    {
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS�������豸���������ɹ�:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ǰ�������豸ID=%s, IP=%s, �˿�=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pCalleeRouteInfo->server_id, pCalleeRouteInfo->server_ip, pCalleeRouteInfo->server_port);
                        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_device_control_proc() SIP_SendMessage OK:server_id=%s, server_ip=%s, server_port=%d \r\n", pCalleeRouteInfo->server_id, pCalleeRouteInfo->server_ip, pCalleeRouteInfo->server_port);

                        old_xml_sn = strtoul(strSN, NULL, 10);
                        transfer_xml_sn = strtoul(strTransferSN, NULL, 10);
                        i = transfer_xml_msg_add(XML_CONTROL_DEVICECONTROL, old_xml_sn, transfer_xml_sn, pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, strDeviceID);
                        DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_device_control_proc() transfer_xml_msg_add: Type=%d, old_xml_sn=%u, transfer_xml_sn=%u, DeviceID=%s, pos=%d \r\n", XML_CONTROL_DEVICECONTROL, old_xml_sn, transfer_xml_sn, strDeviceID, i);
                    }
                }
                else
                {
                    i = SIP_SendMessage(NULL, local_cms_id_get(), callee_id, pCalleeRouteInfo->strRegLocalIP, pCalleeRouteInfo->iRegLocalPort, pCalleeRouteInfo->server_ip, pCalleeRouteInfo->server_port, (char*)inPacket.GetXml(NULL).c_str(), inPacket.GetXml(NULL).length());

                    if (i != 0)
                    {
                        SystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�������豸���������ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=%s, ǰ�������豸ID=%s, IP=%s, �˿�=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"ת����Ϣ���ϼ�CMSʧ��", pCalleeRouteInfo->server_id, pCalleeRouteInfo->server_ip, pCalleeRouteInfo->server_port);
                        EnSystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "Device control command process from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=%s, front-end physical device ID=%s, IP=%s, port=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"forward message to superior CMS failed", pCalleeRouteInfo->server_id, pCalleeRouteInfo->server_ip, pCalleeRouteInfo->server_port);
                        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_device_control_proc() SIP_SendMessage Error:server_id=%s, server_ip=%s, server_port=%d \r\n", pCalleeRouteInfo->server_id, pCalleeRouteInfo->server_ip, pCalleeRouteInfo->server_port);
                    }
                    else
                    {
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS�������豸���������ɹ�:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ǰ�������豸ID=%s, IP=%s, �˿�=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pCalleeRouteInfo->server_id, pCalleeRouteInfo->server_ip, pCalleeRouteInfo->server_port);
                        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_device_control_proc() SIP_SendMessage OK:server_id=%s, server_ip=%s, server_port=%d \r\n", pCalleeRouteInfo->server_id, pCalleeRouteInfo->server_ip, pCalleeRouteInfo->server_port);
                    }
                }
            }
            else
            {
                i = SIP_SendMessage(NULL, local_cms_id_get(), callee_id, pCalleeRouteInfo->strRegLocalIP, pCalleeRouteInfo->iRegLocalPort, pCalleeRouteInfo->server_ip, pCalleeRouteInfo->server_port, (char*)inPacket.GetXml(NULL).c_str(), inPacket.GetXml(NULL).length());

                if (i != 0)
                {
                    SystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�������豸���������ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=%s, ǰ�������豸ID=%s, IP=%s, �˿�=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"ת����Ϣ���ϼ�CMSʧ��", pCalleeRouteInfo->server_id, pCalleeRouteInfo->server_ip, pCalleeRouteInfo->server_port);
                    EnSystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "Device control command process from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=%s, front-end physical device ID=%s, IP=%s, port=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"forward message to superior CMS failed", pCalleeRouteInfo->server_id, pCalleeRouteInfo->server_ip, pCalleeRouteInfo->server_port);
                    DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_device_control_proc() SIP_SendMessage Error:server_id=%s, server_ip=%s, server_port=%d \r\n", pCalleeRouteInfo->server_id, pCalleeRouteInfo->server_ip, pCalleeRouteInfo->server_port);
                }
                else
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS�������豸���������ɹ�:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ǰ�������豸ID=%s, IP=%s, �˿�=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pCalleeRouteInfo->server_id, pCalleeRouteInfo->server_ip, pCalleeRouteInfo->server_port);
                    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_device_control_proc() SIP_SendMessage OK:server_id=%s, server_ip=%s, server_port=%d \r\n", pCalleeRouteInfo->server_id, pCalleeRouteInfo->server_ip, pCalleeRouteInfo->server_port);
                }
            }
        }
        else
        {
            pGBDeviceInfo = GBDevice_info_get_by_stream_type(pGBLogicDeviceInfo, EV9000_STREAM_TYPE_MASTER);

            if (NULL != pGBDeviceInfo)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_device_control_proc() GBDeviceInfo device_type=%d\r\n", pGBDeviceInfo->device_type);

                if (strPTZCmd[0] != '\0')
                {
                    //�����Ԥ��λ����ж��Ƿ���Ҫ�������ݿ�,�¼�cms����²���Ҫ�ж�
                    if (EV9000_DEVICETYPE_SIPSERVER != pGBDeviceInfo->device_type)
                    {
                        i = preset_record(inPacket, pGBLogicDeviceInfo->id, pRoute_Srv_dboper);

                        if (i < 0)
                        {
                            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_device_control_proc() Preset Record Error:device_id=%s \r\n", pGBDeviceInfo->device_id);

                            /* �齨XML��Ϣ */
                            outPacket.SetRootTag("Response");
                            AccNode = outPacket.CreateElement((char*)"CmdType");
                            outPacket.SetElementValue(AccNode, (char*)"DeviceControl");

                            AccNode = outPacket.CreateElement((char*)"SN");
                            outPacket.SetElementValue(AccNode, strSN);

                            AccNode = outPacket.CreateElement((char*)"DeviceID");
                            outPacket.SetElementValue(AccNode, (char*)strDeviceID);

                            AccNode = outPacket.CreateElement((char*)"Result");
                            outPacket.SetElementValue(AccNode, (char*)"ERROR");

                            /* ������Ӧ��Ϣ���ϼ�CMS */
                            i = SIP_SendMessage(NULL, local_cms_id_get(), caller_id, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

                            if (i != 0)
                            {
                                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_device_control_proc() SIP_SendMessage Error:server_id=%s, server_ip=%s, server_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                            }
                            else
                            {
                                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_device_control_proc() SIP_SendMessage OK:server_id=%s, server_ip=%s, server_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                            }

                            return -1;
                        }
                        else
                        {
                            /* �齨XML��Ϣ */
                            outPacket.SetRootTag("Response");
                            AccNode = outPacket.CreateElement((char*)"CmdType");
                            outPacket.SetElementValue(AccNode, (char*)"DeviceControl");

                            AccNode = outPacket.CreateElement((char*)"SN");
                            outPacket.SetElementValue(AccNode, strSN);

                            AccNode = outPacket.CreateElement((char*)"DeviceID");
                            outPacket.SetElementValue(AccNode, (char*)strDeviceID);

                            AccNode = outPacket.CreateElement((char*)"Result");
                            outPacket.SetElementValue(AccNode, (char*)"OK");

                            /* ������Ӧ��Ϣ���ϼ�CMS */
                            i = SIP_SendMessage(NULL, local_cms_id_get(), caller_id, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

                            if (i != 0)
                            {
                                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_device_control_proc() SIP_SendMessage Error:server_id=%s, server_ip=%s, server_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                            }
                            else
                            {
                                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_device_control_proc() SIP_SendMessage OK:server_id=%s, server_ip=%s, server_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                            }
                        }
                    }
                    else if (EV9000_DEVICETYPE_SIPSERVER == pGBDeviceInfo->device_type
                             && 1 == pGBDeviceInfo->three_party_flag) /* ������ƽ̨���¼���λ */
                    {
                        i = preset_record(inPacket, pGBLogicDeviceInfo->id, pRoute_Srv_dboper);

                        if (i < 0)
                        {
                            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_device_control_proc() Preset Record Error:device_id=%s \r\n", pGBDeviceInfo->device_id);

                            /* �齨XML��Ϣ */
                            outPacket.SetRootTag("Response");
                            AccNode = outPacket.CreateElement((char*)"CmdType");
                            outPacket.SetElementValue(AccNode, (char*)"DeviceControl");

                            AccNode = outPacket.CreateElement((char*)"SN");
                            outPacket.SetElementValue(AccNode, strSN);

                            AccNode = outPacket.CreateElement((char*)"DeviceID");
                            outPacket.SetElementValue(AccNode, (char*)strDeviceID);

                            AccNode = outPacket.CreateElement((char*)"Result");
                            outPacket.SetElementValue(AccNode, (char*)"ERROR");

                            /* ������Ӧ��Ϣ���ϼ�CMS */
                            i = SIP_SendMessage(NULL, local_cms_id_get(), caller_id, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

                            if (i != 0)
                            {
                                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_device_control_proc() SIP_SendMessage Error:server_id=%s, server_ip=%s, server_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                            }
                            else
                            {
                                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_device_control_proc() SIP_SendMessage OK:server_id=%s, server_ip=%s, server_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                            }

                            return -1;
                        }
                        else
                        {
                            /* �齨XML��Ϣ */
                            outPacket.SetRootTag("Response");
                            AccNode = outPacket.CreateElement((char*)"CmdType");
                            outPacket.SetElementValue(AccNode, (char*)"DeviceControl");

                            AccNode = outPacket.CreateElement((char*)"SN");
                            outPacket.SetElementValue(AccNode, strSN);

                            AccNode = outPacket.CreateElement((char*)"DeviceID");
                            outPacket.SetElementValue(AccNode, (char*)strDeviceID);

                            AccNode = outPacket.CreateElement((char*)"Result");
                            outPacket.SetElementValue(AccNode, (char*)"OK");

                            /* ������Ӧ��Ϣ���ϼ�CMS */
                            i = SIP_SendMessage(NULL, local_cms_id_get(), caller_id, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

                            if (i != 0)
                            {
                                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_device_control_proc() SIP_SendMessage Error:server_id=%s, server_ip=%s, server_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                            }
                            else
                            {
                                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_device_control_proc() SIP_SendMessage OK:server_id=%s, server_ip=%s, server_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                            }
                        }
                    }
                }

                if (strLockCmd[0] != '\0') /* ������������ֻ�����¼�ƽ̨ */
                {
                    if (EV9000_DEVICETYPE_SIPSERVER == pGBDeviceInfo->device_type)
                    {
                        i = SIP_SendMessage(NULL, local_cms_id_get(), callee_id, pGBDeviceInfo->strRegServerIP, pGBDeviceInfo->iRegServerPort, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)inPacket.GetXml(NULL).c_str(), inPacket.GetXml(NULL).length());

                        if (i != 0)
                        {
                            SystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�������豸���������ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=%s, ǰ�������豸ID=%s, IP=%s, �˿�=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"ת����Ϣ��ǰ�������豸ʧ��", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                            EnSystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "Device control command process from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=%s, front-end physical device ID=%s, IP=%s, port=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"forward message to frond-end physical device failed", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_device_control_proc() SIP_SendMessage Error:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                        }
                        else
                        {
                            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS�������豸���������ɹ�:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ǰ�������豸ID=%s, IP=%s, �˿�=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Device control command process from superior CMS success:Superior CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s, front end physical device ID = % s, IP = % s, port = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_device_control_proc() SIP_SendMessage OK:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                        }
                    }
                }
                else if (strRecordCmd[0] != '\0') /* ������ֶ�¼�����ǰ���з��� */
                {
                    /* ��ȡ�ϵ�SN�ڵ� */
                    AccSnNode = inPacket.SearchElement((char*)"SN");

                    if (NULL != AccSnNode)
                    {
                        g_transfer_xml_sn++;
                        snprintf(strTransferSN, 32, "%u", g_transfer_xml_sn);
                        inPacket.SetElementValue(AccSnNode, strTransferSN);
                    }

                    i = SIP_SendMessage(NULL, local_cms_id_get(), callee_id, pGBDeviceInfo->strRegServerIP, pGBDeviceInfo->iRegServerPort, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)inPacket.GetXml(NULL).c_str(), inPacket.GetXml(NULL).length());

                    if (i != 0)
                    {
                        SystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�������豸���������ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=%s, ת��ǰ�������豸ID=%s, IP��ַ=%s, �˿�=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"ת����Ϣ��ǰ�������豸ʧ��", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                        EnSystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "Device control command process from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=%s, front-end physical device ID=%s, IP=%s, port=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"forward message to frond-end physical device failed", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_device_control_proc() SIP_SendMessage Error:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    }
                    else
                    {
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS�������豸���������ɹ�,ת����Ϣ��ǰ�������豸:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ת��ǰ�������豸ID=%s, IP��ַ=%s, �˿�=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Device control command process from superior CMS success, forwarding messages to the front end physical device: the higher the CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s, forward front physical device ID = % s, IP address = % s, port = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_device_control_proc() SIP_SendMessage OK:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);

                        old_xml_sn = strtoul(strSN, NULL, 10);
                        transfer_xml_sn = strtoul(strTransferSN, NULL, 10);
                        i = transfer_xml_msg_add(XML_CONTROL_DEVICECONTROL, old_xml_sn, transfer_xml_sn, pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, strDeviceID);
                        DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_device_control_proc() transfer_xml_msg_add: Type=%d, old_xml_sn=%u, transfer_xml_sn=%u, DeviceID=%s, pos=%d \r\n", XML_CONTROL_DEVICECONTROL, old_xml_sn, transfer_xml_sn, strDeviceID, i);
                    }
                }
                else if (strPTZCmd[0] != '\0') /* ����������Ԥ��λ�����ģ�ǰ���з��� */
                {
                    if (EV9000_DEVICETYPE_SIPSERVER == pGBDeviceInfo->device_type
                        && 0 == pGBDeviceInfo->three_party_flag) /* �¼���λ */
                    {
                        iLen = String2Bytes((unsigned char*)strPTZCmd, szPtzCmd, PTZCMD_28181_LEN);

                        if (szPtzCmd[3] == 0x81 || szPtzCmd[3] == 0x83)
                        {
                            /* ��ȡ�ϵ�SN�ڵ� */
                            AccSnNode = inPacket.SearchElement((char*)"SN");

                            if (NULL != AccSnNode)
                            {
                                g_transfer_xml_sn++;
                                snprintf(strTransferSN, 32, "%u", g_transfer_xml_sn);
                                inPacket.SetElementValue(AccSnNode, strTransferSN);
                            }

                            i = SIP_SendMessage(NULL, local_cms_id_get(), callee_id, pGBDeviceInfo->strRegServerIP, pGBDeviceInfo->iRegServerPort, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)inPacket.GetXml(NULL).c_str(), inPacket.GetXml(NULL).length());

                            if (i != 0)
                            {
                                SystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�������豸���������ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=%s, ת��ǰ�������豸ID=%s, IP��ַ=%s, �˿�=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"ת����Ϣ��ǰ�������豸ʧ��", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                                EnSystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "Device control command process from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=%s, front-end physical device ID=%s, IP=%s, port=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"forward message to frond-end physical device failed", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_device_control_proc() SIP_SendMessage Error:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                            }
                            else
                            {
                                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS�������豸���������ɹ�,ת����Ϣ��ǰ�������豸:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ת��ǰ�������豸ID=%s, IP��ַ=%s, �˿�=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Device control command process from superior CMS success, forwarding messages to the front end physical device: the higher the CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s, forward front physical device ID = % s, IP address = % s, port = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_device_control_proc() SIP_SendMessage OK:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);

                                old_xml_sn = strtoul(strSN, NULL, 10);
                                transfer_xml_sn = strtoul(strTransferSN, NULL, 10);
                                i = transfer_xml_msg_add(XML_CONTROL_DEVICECONTROL, old_xml_sn, transfer_xml_sn, pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, strDeviceID);
                                DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_device_control_proc() transfer_xml_msg_add: Type=%d, old_xml_sn=%u, transfer_xml_sn=%u, DeviceID=%s, pos=%d \r\n", XML_CONTROL_DEVICECONTROL, old_xml_sn, transfer_xml_sn, strDeviceID, i);
                            }
                        }
                        else
                        {
                            i = SIP_SendMessage(NULL, local_cms_id_get(), callee_id, pGBDeviceInfo->strRegServerIP, pGBDeviceInfo->iRegServerPort, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)inPacket.GetXml(NULL).c_str(), inPacket.GetXml(NULL).length());

                            if (i != 0)
                            {
                                SystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�������豸���������ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=%s, ǰ�������豸ID=%s, IP��ַ=%s, �˿�=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"ת����Ϣ��ǰ�������豸ʧ��", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                                EnSystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "Device control command process from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=%s, front-end physical device ID=%s, IP=%s, port=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"forward message to frond-end physical device failed", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_device_control_proc() SIP_SendMessage Error:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                            }
                            else
                            {
                                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS�������豸���������ɹ�:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ǰ�������豸ID=%s, IP��ַ=%s, �˿�=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Device control command process from superior CMS success:the higher the CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s, front-end, physical device ID = % s = % s IP address, port = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_device_control_proc() SIP_SendMessage OK:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                            }
                        }
                    }
                    else
                    {
                        i = SIP_SendMessage(NULL, local_cms_id_get(), callee_id, pGBDeviceInfo->strRegServerIP, pGBDeviceInfo->iRegServerPort, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)inPacket.GetXml(NULL).c_str(), inPacket.GetXml(NULL).length());

                        if (i != 0)
                        {
                            SystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�������豸���������ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=%s, ǰ�������豸ID=%s, IP��ַ=%s, �˿�=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"ת����Ϣ��ǰ�������豸ʧ��", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                            EnSystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "Device control command process from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=%s, front-end physical device ID=%s, IP=%s, port=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"forward message to frond-end physical device failed", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_device_control_proc() SIP_SendMessage Error:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                        }
                        else
                        {
                            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS�������豸���������ɹ�:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ǰ�������豸ID=%s, IP��ַ=%s, �˿�=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Device control command process from superior CMS success:the higher the CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s, front-end, physical device ID = % s = % s IP address, port = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_device_control_proc() SIP_SendMessage OK:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                        }
                    }
                }
                else
                {
                    i = SIP_SendMessage(NULL, local_cms_id_get(), callee_id, pGBDeviceInfo->strRegServerIP, pGBDeviceInfo->iRegServerPort, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)inPacket.GetXml(NULL).c_str(), inPacket.GetXml(NULL).length());

                    if (i != 0)
                    {
                        SystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�������豸���������ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=%s, ǰ�������豸ID=%s, IP��ַ=%s, �˿�=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"ת����Ϣ��ǰ�������豸ʧ��", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                        EnSystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "Device control command process from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=%s, front-end physical device ID=%s, IP=%s, port=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"forward message to frond-end physical device failed", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_device_control_proc() SIP_SendMessage Error:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    }
                    else
                    {
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS�������豸���������ɹ�:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ǰ�������豸ID=%s, IP��ַ=%s, �˿�=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Device control command process from superior CMS success:the higher the CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s, front-end, physical device ID = % s = % s IP address, port = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_device_control_proc() SIP_SendMessage OK:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    }
                }
            }
            else
            {
                SystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�������豸���������ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=û���ҵ���Ӧ�������豸��Ϣ:DeviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, strDeviceID);
                EnSystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "Device control command process from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause= corresponding physical device info not found:DeviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, strDeviceID);
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_device_control_proc() exit---: Get LogicDevice's GBDevice Info Error: DeviceID=%s \r\n", callee_id);
                return -1;
            }
        }
    }
    else
    {
        SystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�������豸���������ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=û���ҵ���Ӧ�������豸��Ϣ:DeviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, strDeviceID);
        EnSystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "Device control command process from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=corresponding physical device info not found:DeviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, strDeviceID);
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_device_control_proc() exit---: Find GBDevice Info Error: DeviceID=%s \r\n", callee_id);
        return -1;
    }

    return i;
}

/*****************************************************************************
 �� �� ��  : route_device_config_proc
 ��������  : �ϼ�����CMS���͹������豸���������
 �������  : route_info_t* pRouteInfo
             char* caller_id
             char* callee_id
             CPacket& inPacket
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��7��15�� ���ڶ�
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int route_device_config_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, CPacket& inPacket)
{
    int i = 0;
    char strSN[32] = {0};
    char strDeviceID[32] = {0};
    GBDevice_info_t* pGBDeviceInfo = NULL;
    GBLogicDevice_info_t* pGBLogicDeviceInfo = NULL;
    CPacket outPacket;
    DOMElement* AccNode = NULL;

    if (NULL == pRouteInfo)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_device_config_proc() exit---: Route Info Error \r\n");
        return -1;
    }

    if (NULL == caller_id || NULL == callee_id)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_device_config_proc() exit---: Param Error \r\n");
        return -1;
    }

    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_device_config_proc() Enter---: caller_id=%s, caller_ip=%s, caller_port=%d, callee_id=%s\r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);
    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS������ǰ���豸���������:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Device control configuration process from superior CMS success:the higher the CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s,", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);

    /* �����豸����

          �豸��������ֱ��ת����ǰ���豸����������
          ������������ֶ�:
          <!-- �������ͣ��豸��Ϣ��ѯ����ѡ�� -->
          <element name="CmdType" fixed ="DeviceConfig" />
          <!-- �������кţ���ѡ�� -->
          <element name="SN" type="integer" minInclusive value = "1" />
          <!-- Ŀ���豸���豸���루��ѡ�� -->
          <element name="DeviceID" type="tg:deviceIDType" />
      */

    /* ȡ������*/
    inPacket.GetElementValue((char*)"SN", strSN);
    inPacket.GetElementValue((char*)"DeviceID", strDeviceID);

    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO,  "route_device_config_proc() \
        \r\n XML Para: \
        \r\n SN=%s, DeviceID=%s \r\n", strSN, strDeviceID);

    /* �鿴�����Ƿ��Ǳ�CMS ID */
    if (0 == strncmp(callee_id, local_cms_id_get(), 20))
    {
        // TODO:���ñ���CMS�ļ�������

        /* �ظ���Ӧ,�齨��Ϣ */
        outPacket.SetRootTag("Response");
        AccNode = outPacket.CreateElement((char*)"CmdType");
        outPacket.SetElementValue(AccNode, (char*)"DeviceConfig");

        AccNode = outPacket.CreateElement((char*)"SN");
        outPacket.SetElementValue(AccNode, strSN);

        AccNode = outPacket.CreateElement((char*)"DeviceID");
        outPacket.SetElementValue(AccNode, strDeviceID);

        AccNode = outPacket.CreateElement((char*)"Result");
        outPacket.SetElementValue(AccNode, (char*)"OK");

        /* ������Ӧ��Ϣ ���ϼ�CMS */
        i = SIP_SendMessage(NULL, local_cms_id_get(), caller_id, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

        if (i != 0)
        {
            SystemLog(EV9000_CMS_SET_DEVICE_CONFIG_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�������豸����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, ԭ��=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"����Ӧ����Ϣʧ��");
            EnSystemLog(EV9000_CMS_SET_DEVICE_CONFIG_ERROR, EV9000_LOG_LEVEL_ERROR, "Device configuration from superior CMS :Requester ID=%s, IP address=%s, port number=%d,cause=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"Failed to send reply message");
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_device_status_proc() SIP_SendMessage Error:caller_id=%s, caller_ip=%s, caller_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
        }
        else
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS�������豸���óɹ�:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Device configuration from superior CMS failure:Requester ID=%s, IP address=%s, port number=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_device_status_proc() SIP_SendMessage OK:caller_id=%s, caller_ip=%s, caller_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
        }
    }
    else
    {
        pGBLogicDeviceInfo = GBLogicDevice_info_find(callee_id);

        if (NULL != pGBLogicDeviceInfo)
        {
            pGBDeviceInfo = GBDevice_info_get_by_stream_type(pGBLogicDeviceInfo, EV9000_STREAM_TYPE_MASTER);

            if (NULL != pGBDeviceInfo)
            {
                i = SIP_SendMessage(NULL, local_cms_id_get(), callee_id, pGBDeviceInfo->strRegServerIP, pGBDeviceInfo->iRegServerPort, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)inPacket.GetXml(NULL).c_str(), inPacket.GetXml(NULL).length());

                if (i != 0)
                {
                    SystemLog(EV9000_CMS_SET_DEVICE_CONFIG_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS������ǰ���豸���������ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=%s, ǰ�������豸ID=%s, IP=%s, �˿�=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"ת����Ϣ��ǰ�������豸ʧ��", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    EnSystemLog(EV9000_CMS_SET_DEVICE_CONFIG_ERROR, EV9000_LOG_LEVEL_ERROR, "Front-end device configuration command process from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=%s, front-end physical device ID=%s, IP=%s, port=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"forward message to frond-end physical device failed", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_device_config_proc() SIP_SendMessage Error:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                }
                else
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS������ǰ���豸���������ɹ�:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ǰ�������豸ID=%s, IP=%s, �˿�=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "The front of the equipment configuration command from superior CMS processing success: the higher the CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s, front end physical device ID = % s, IP = % s, port = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_device_config_proc() SIP_SendMessage OK:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                }
            }
            else
            {
                SystemLog(EV9000_CMS_SET_DEVICE_CONFIG_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS������ǰ���豸���������ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=û���ҵ���Ӧ�������豸��Ϣ:DeviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, callee_id);
                EnSystemLog(EV9000_CMS_SET_DEVICE_CONFIG_ERROR, EV9000_LOG_LEVEL_ERROR, "Front-end device configuration command process from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=corresponding physical device info not found:DeviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, callee_id);
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_device_config_proc() exit---: Get LogicDevice's GBDevice Info Error: DeviceID=%s \r\n", callee_id);
                return -1;
            }
        }
        else
        {
            pGBDeviceInfo = GBDevice_info_find(callee_id);

            if (NULL != pGBDeviceInfo)
            {
                i = SIP_SendMessage(NULL, local_cms_id_get(), callee_id, pGBDeviceInfo->strRegServerIP, pGBDeviceInfo->iRegServerPort, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)inPacket.GetXml(NULL).c_str(), inPacket.GetXml(NULL).length());

                if (i != 0)
                {
                    SystemLog(EV9000_CMS_SET_DEVICE_CONFIG_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS������ǰ���豸���������ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=%s, ǰ�������豸ID=%s, IP=%s, �˿�=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"ת����Ϣ��ǰ�������豸ʧ��", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    EnSystemLog(EV9000_CMS_SET_DEVICE_CONFIG_ERROR, EV9000_LOG_LEVEL_ERROR, "Front-end device configuration command process from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=%s, front-end physical device ID=%s, IP=%s, port=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"forward message to frond-end physical device failed", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_device_config_proc() SIP_SendMessage Error:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                }
                else
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS������ǰ���豸���������ɹ�:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ǰ�������豸ID=%s, IP=%s, �˿�=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "The front of the equipment configuration command from superior CMS processing success: the higher the CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s, front end physical device ID = % s, IP = % s, port = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_device_config_proc() SIP_SendMessage OK:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                }
            }
            else
            {
                SystemLog(EV9000_CMS_SET_DEVICE_CONFIG_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS������ǰ���豸���������ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=û���ҵ���Ӧ�������豸��Ϣ:DeviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, callee_id);
                EnSystemLog(EV9000_CMS_SET_DEVICE_CONFIG_ERROR, EV9000_LOG_LEVEL_ERROR, "Front-end device configuration command process from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=corresponding physical device info not found:DeviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, callee_id);
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_device_config_proc() exit---: Find GBDevice Info Error: DeviceID=%s \r\n", callee_id);
                return -1;
            }
        }
    }

    return i;
}

/*****************************************************************************
 �� �� ��  : route_query_device_video_param_proc
 ��������  : �ϼ�����CMS���͹����Ļ�ȡǰ��ͼ�����
 �������  : route_info_t* pRouteInfo
             char* caller_id
             char* callee_id
             CPacket& inPacket
             DBOper* pRoute_Srv_dboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int route_query_device_video_param_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, CPacket& inPacket)
{
    int i = 0;
    GBDevice_info_t* pGBDeviceInfo = NULL;
    GBLogicDevice_info_t* pGBLogicDeviceInfo = NULL;

    char strSN[32] = {0};
    char strTransferSN[32] = {0};
    char strDeviceID[32] = {0};
    DOMElement* AccSnNode = NULL;
    unsigned int old_xml_sn = 0;
    unsigned int transfer_xml_sn = 0;

    if (NULL == pRouteInfo)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_query_device_video_param_proc() exit---: Route Info Error \r\n");
        return -1;
    }

    if (NULL == caller_id || NULL == callee_id)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_query_device_video_param_proc() exit---: Param Error \r\n");
        return -1;
    }

    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_device_video_param_proc() Enter---: caller_id=%s, caller_ip=%s, caller_port=%d, callee_id=%s\r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);
    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS�����Ļ�ȡǰ��ͼ�����:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Access front-end image parameter from superior CMS:Superior CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);

    /* ��ȡǰ��ͼ�����������ֱ��ת����ǰ���豸����������

          ������������ֶ�:

      <?xml version="1.0"?>
      <Query>
      <CmdType>GetDeviceVideoParam</CmdType>
      <SN>43</SN>
      <DeviceID>64010000001110000001</DeviceID>
      </Query>
      */
    inPacket.GetElementValue((char*)"SN", strSN);
    inPacket.GetElementValue((char*)"DeviceID", strDeviceID);

    if (strSN[0] == '\0')
    {
        SystemLog(EV9000_CMS_GET_VIDEO_PARAM_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�����Ļ�ȡǰ��ͼ�����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=��ȡXML��Ϣ�е�SNʧ��", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);
        EnSystemLog(EV9000_CMS_GET_VIDEO_PARAM_ERROR, EV9000_LOG_LEVEL_ERROR, "Access front-end image parameter from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=access SN from XML message failed", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_device_video_param_proc() exit---: Get XML SN Error\r\n");
        return -1;
    }

    if (strDeviceID[0] == '\0')
    {
        SystemLog(EV9000_CMS_GET_VIDEO_PARAM_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�����Ļ�ȡǰ��ͼ�����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=��ȡXML��Ϣ�е�DeviceIDʧ��", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);
        EnSystemLog(EV9000_CMS_GET_VIDEO_PARAM_ERROR, EV9000_LOG_LEVEL_ERROR, "Access front-end image parameter from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=access device ID from XML message failed", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_device_video_param_proc() exit---: Get XML DeviceID Error\r\n");
        return -1;
    }

    pGBLogicDeviceInfo = GBLogicDevice_info_find(callee_id);

    if (NULL != pGBLogicDeviceInfo)
    {
        pGBDeviceInfo = GBDevice_info_get_by_stream_type(pGBLogicDeviceInfo, EV9000_STREAM_TYPE_MASTER);

        if (NULL != pGBDeviceInfo)
        {
            /* ��ȡ�ϵ�SN�ڵ� */
            AccSnNode = inPacket.SearchElement((char*)"SN");

            if (NULL != AccSnNode)
            {
                g_transfer_xml_sn++;
                snprintf(strTransferSN, 32, "%u", g_transfer_xml_sn);
                inPacket.SetElementValue(AccSnNode, strTransferSN);
            }

            i = SIP_SendMessage(NULL, local_cms_id_get(), callee_id, pGBDeviceInfo->strRegServerIP, pGBDeviceInfo->iRegServerPort, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)inPacket.GetXml(NULL).c_str(), inPacket.GetXml(NULL).length());

            if (i != 0)
            {
                SystemLog(EV9000_CMS_GET_VIDEO_PARAM_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�����Ļ�ȡǰ��ͼ�����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=%s, ǰ�������豸ID=%s, IP��ַ=%s, �˿�=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"ת����Ϣ��ǰ�������豸ʧ��", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                EnSystemLog(EV9000_CMS_GET_VIDEO_PARAM_ERROR, EV9000_LOG_LEVEL_ERROR, "Access front-end image parameter from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=%s, front-end physical deviceID=%s, IP=%s, port=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"forward message to frond-end physical device failed", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_device_video_param_proc() SIP_SendMessage Error:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
            }
            else
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS�����Ļ�ȡǰ��ͼ������ɹ�,ת����Ϣ��ǰ�������豸:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ת��ǰ�������豸ID=%s, IP��ַ=%s, �˿�=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Access front-end image parameter from superior CMS  success, forwarding messages to the front end physical device: the higher the CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s, forwarding the front-end, physical device ID = % s = % s IP address, port = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_device_video_param_proc() SIP_SendMessage OK:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);

                old_xml_sn = strtoul(strSN, NULL, 10);
                transfer_xml_sn = strtoul(strTransferSN, NULL, 10);
                i = transfer_xml_msg_add(XML_QUERY_DEVICEVIDEOPARAM, old_xml_sn, transfer_xml_sn, pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, strDeviceID);
                DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_query_device_video_param_proc() transfer_xml_msg_add: Type=%d, old_xml_sn=%u, transfer_xml_sn=%u, DeviceID=%s, pos=%d \r\n", XML_QUERY_DEVICEVIDEOPARAM, old_xml_sn, transfer_xml_sn, strDeviceID, i);
            }
        }
        else
        {
            SystemLog(EV9000_CMS_GET_VIDEO_PARAM_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�����Ļ�ȡǰ��ͼ�����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=û���ҵ���Ӧ�������豸��Ϣ:DeviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, callee_id);
            EnSystemLog(EV9000_CMS_GET_VIDEO_PARAM_ERROR, EV9000_LOG_LEVEL_ERROR, "Access front-end image parameter from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=corresponding physical device info not found:DeviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, callee_id);
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_device_video_param_proc() exit---: Get LogicDevice's GBDevice Info Error: DeviceID=%s \r\n", callee_id);
            return -1;
        }
    }
    else
    {
        pGBDeviceInfo = GBDevice_info_find(callee_id);

        if (NULL != pGBDeviceInfo)
        {
            /* ��ȡ�ϵ�SN�ڵ� */
            AccSnNode = inPacket.SearchElement((char*)"SN");

            if (NULL != AccSnNode)
            {
                g_transfer_xml_sn++;
                snprintf(strTransferSN, 32, "%u", g_transfer_xml_sn);
                inPacket.SetElementValue(AccSnNode, strTransferSN);
            }

            i = SIP_SendMessage(NULL, local_cms_id_get(), callee_id, pGBDeviceInfo->strRegServerIP, pGBDeviceInfo->iRegServerPort, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)inPacket.GetXml(NULL).c_str(), inPacket.GetXml(NULL).length());

            if (i != 0)
            {
                SystemLog(EV9000_CMS_GET_VIDEO_PARAM_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�����Ļ�ȡǰ��ͼ�����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=%s, ת��ǰ�������豸ID=%s, IP��ַ=%s, �˿�=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"ת����Ϣ��ǰ�������豸ʧ��", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                EnSystemLog(EV9000_CMS_GET_VIDEO_PARAM_ERROR, EV9000_LOG_LEVEL_ERROR, "Access front-end image parameter from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=%s, front-end physical deviceID=%s, IP=%s, port=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"forward message to frond-end physical device failed", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_device_video_param_proc() SIP_SendMessage Error:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
            }
            else
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS�����Ļ�ȡǰ��ͼ������ɹ�,ת����Ϣ��ǰ�������豸:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ת��ǰ�������豸ID=%s, IP��ַ=%s, �˿�=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Access front-end image parameter from superior CMS success, forwarding messages to the front end physical device: the higher the CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s, forwarding the front-end, physical device ID = % s = % s IP address, port = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);

                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_device_video_param_proc() SIP_SendMessage OK:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);

                old_xml_sn = strtoul(strSN, NULL, 10);
                transfer_xml_sn = strtoul(strTransferSN, NULL, 10);
                i = transfer_xml_msg_add(XML_QUERY_DEVICEVIDEOPARAM, old_xml_sn, transfer_xml_sn, pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, strDeviceID);
                DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_query_device_video_param_proc() transfer_xml_msg_add: Type=%d, old_xml_sn=%u, transfer_xml_sn=%u, DeviceID=%s, pos=%d \r\n", XML_QUERY_DEVICEVIDEOPARAM, old_xml_sn, transfer_xml_sn, strDeviceID, i);
            }
        }
        else
        {
            SystemLog(EV9000_CMS_GET_VIDEO_PARAM_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�����Ļ�ȡǰ��ͼ�����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=û���ҵ���Ӧ�������豸��Ϣ:DeviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, callee_id);
            EnSystemLog(EV9000_CMS_GET_VIDEO_PARAM_ERROR, EV9000_LOG_LEVEL_ERROR, "Access front-end image parameter from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=corresponding physical device info not found:DeviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, callee_id);
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_device_video_param_proc() exit---: Find GBDevice Info Error: DeviceID=%s \r\n", callee_id);
            return -1;
        }
    }

    return i;
}

/*****************************************************************************
 �� �� ��  : route_set_device_video_param_proc
 ��������  : �ϼ�����CMS���͹���������ǰ��ͼ�����
 �������  : route_info_t* pRouteInfo
             char* caller_id
             char* callee_id
             CPacket& inPacket
             DBOper* pRoute_Srv_dboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��1��28�� ���ڶ�
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int route_set_device_video_param_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, CPacket& inPacket)
{
    int i = 0;
    GBDevice_info_t* pGBDeviceInfo = NULL;
    GBLogicDevice_info_t* pGBLogicDeviceInfo = NULL;

    if (NULL == pRouteInfo)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_set_device_video_param_proc() exit---: Route Info Error \r\n");
        return -1;
    }

    if (NULL == caller_id || NULL == callee_id)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_set_device_video_param_proc() exit---: Param Error \r\n");
        return -1;
    }

    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_set_device_video_param_proc() Enter---: caller_id=%s, caller_ip=%s, caller_port=%d, callee_id=%s\r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);
    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS����������ǰ��ͼ�����:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Access front-end image parameter from superior CMS: the superior CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);

    /* ����ǰ��ͼ�����������ֱ��ת����ǰ���豸����������

          ������������ֶ�:

      <?xml version="1.0"?>
      <Control>
      <CmdType>SetDeviceVideoParam</CmdType>
      <SN>43</SN>
      <DeviceID>64010000001110000001</DeviceID>
      <Brightness>12</ Brightness >        // ����
      <Saturation>22</ Saturation >         // ���Ͷ�
      <Contrast>2</ Contrast>                 // �Աȶ�
      <ColourDegree>2</ ColourDegree >  // ɫ��
      </Control>
      */

    pGBLogicDeviceInfo = GBLogicDevice_info_find(callee_id);

    if (NULL != pGBLogicDeviceInfo)
    {
        pGBDeviceInfo = GBDevice_info_get_by_stream_type(pGBLogicDeviceInfo, EV9000_STREAM_TYPE_MASTER);

        if (NULL != pGBDeviceInfo)
        {
            i = SIP_SendMessage(NULL, local_cms_id_get(), callee_id, pGBDeviceInfo->strRegServerIP, pGBDeviceInfo->iRegServerPort, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)inPacket.GetXml(NULL).c_str(), inPacket.GetXml(NULL).length());

            if (i != 0)
            {
                SystemLog(EV9000_CMS_SET_VIDEO_PARAM_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS����������ǰ��ͼ�����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=%s, ǰ�������豸ID=%s, IP=%s, �˿�=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"ת����Ϣ��ǰ�������豸ʧ��", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                EnSystemLog(EV9000_CMS_SET_VIDEO_PARAM_ERROR, EV9000_LOG_LEVEL_ERROR, "Set front-end image parameter from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=%s, front-end physical deviceID=%s, IP=%s, port=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"forward message to frond-end physical device failed", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_set_device_video_param_proc() SIP_SendMessage Error:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
            }
            else
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS����������ǰ��ͼ������ɹ�:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ǰ�������豸ID=%s, IP=%s, �˿�=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Set front-end image parameter from superior CMS success:Superior CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s, front end physical device ID = % s, IP = % s, port = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_set_device_video_param_proc() SIP_SendMessage OK:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
            }
        }
        else
        {
            SystemLog(EV9000_CMS_SET_VIDEO_PARAM_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS����������ǰ��ͼ�����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=û���ҵ���Ӧ�������豸��Ϣ:DeviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, callee_id);
            EnSystemLog(EV9000_CMS_SET_VIDEO_PARAM_ERROR, EV9000_LOG_LEVEL_ERROR, "Set front-end image parameter from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=corresponding physical device info not found:DeviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, callee_id);
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_set_device_video_param_proc() exit---: Get LogicDevice's GBDevice Info Error: DeviceID=%s \r\n", callee_id);
            return -1;
        }
    }
    else
    {
        pGBDeviceInfo = GBDevice_info_find(callee_id);

        if (NULL != pGBDeviceInfo)
        {
            i = SIP_SendMessage(NULL, local_cms_id_get(), callee_id, pGBDeviceInfo->strRegServerIP, pGBDeviceInfo->iRegServerPort, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)inPacket.GetXml(NULL).c_str(), inPacket.GetXml(NULL).length());

            if (i != 0)
            {
                SystemLog(EV9000_CMS_SET_VIDEO_PARAM_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS����������ǰ��ͼ�����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=%s, ǰ�������豸ID=%s, IP=%s, �˿�=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"ת����Ϣ��ǰ�������豸ʧ��", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                EnSystemLog(EV9000_CMS_SET_VIDEO_PARAM_ERROR, EV9000_LOG_LEVEL_ERROR, "Set front-end image parameter from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=%s, front-end physical deviceID=%s, IP=%s, port=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"forward message to frond-end physical device failed", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_set_device_video_param_proc() SIP_SendMessage Error:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
            }
            else
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS����������ǰ��ͼ������ɹ�:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ǰ�������豸ID=%s, IP=%s, �˿�=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Set front-end image parameter from superior CMS success:Superior CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s, front end physical device ID = % s, IP = % s, port = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_set_device_video_param_proc() SIP_SendMessage OK:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
            }
        }
        else
        {
            SystemLog(EV9000_CMS_SET_VIDEO_PARAM_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS����������ǰ��ͼ�����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=û���ҵ���Ӧ�������豸��Ϣ:DeviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, callee_id);
            EnSystemLog(EV9000_CMS_SET_VIDEO_PARAM_ERROR, EV9000_LOG_LEVEL_ERROR, "Set front-end image parameter from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=corresponding physical device info not found:DeviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, callee_id);
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_set_device_video_param_proc() exit---: Find GB Device Info Error:DeviceID=%s \r\n", callee_id);
            return -1;
        }
    }

    return i;
}

/*****************************************************************************
 �� �� ��  : route_request_ifram_data_proc
 ��������  : �ϼ�����CMS���͹���������I֡
 �������  : route_info_t* pRouteInfo
             char* caller_id
             char* callee_id
             CPacket& inPacket
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��1��28�� ���ڶ�
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int route_request_ifram_data_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, CPacket& inPacket)
{
    int i = 0;
    GBDevice_info_t* pGBDeviceInfo = NULL;
    GBLogicDevice_info_t* pGBLogicDeviceInfo = NULL;

    if (NULL == pRouteInfo)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_request_ifram_data_proc() exit---: Route Info Error \r\n");
        return -1;
    }

    if (NULL == caller_id || NULL == callee_id)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_request_ifram_data_proc() exit---: Param Error \r\n");
        return -1;
    }

    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_request_ifram_data_proc() Enter---: caller_id=%s, caller_ip=%s, caller_port=%d, callee_id=%s\r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);
    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS����������I֡:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Request I frame from superior CMS:Superior CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);

    /* ����I ֡������ֱ��ת����ǰ���豸����������

          ������������ֶ�:

      <?xml version="1.0"?>
      < Control >
      <CmdType>RequestIFrameData</CmdType>
      <SN>43</SN>
      <DeviceID>64010000001110000001</DeviceID>
      </ Control >

      */

    pGBLogicDeviceInfo = GBLogicDevice_info_find(callee_id);

    if (NULL != pGBLogicDeviceInfo)
    {
        pGBDeviceInfo = GBDevice_info_get_by_stream_type(pGBLogicDeviceInfo, EV9000_STREAM_TYPE_MASTER);

        if (NULL != pGBDeviceInfo)
        {
            if (EV9000_DEVICETYPE_MGWSERVER == pGBDeviceInfo->device_type
                || (EV9000_DEVICETYPE_SIPSERVER == pGBDeviceInfo->device_type && 0 == pGBDeviceInfo->three_party_flag))
            {
                i = SIP_SendMessage(NULL, local_cms_id_get(), callee_id, pGBDeviceInfo->strRegServerIP, pGBDeviceInfo->iRegServerPort, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)inPacket.GetXml(NULL).c_str(), inPacket.GetXml(NULL).length());

                if (i != 0)
                {
                    SystemLog(EV9000_CMS_REQUEST_IFRAM_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS����������I֡ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=%s, ǰ�������豸ID=%s, IP=%s, �˿�=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"ת����Ϣ��ǰ�������豸ʧ��", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    EnSystemLog(EV9000_CMS_REQUEST_IFRAM_ERROR, EV9000_LOG_LEVEL_ERROR, "Request I frame from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=%s, front-end physical deviceID=%s, IP=%s, port=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"forward message to frond-end physical device failed", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_request_ifram_data_proc() SIP_SendMessage Error:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                }
                else
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS����������I֡�ɹ�:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ǰ�������豸ID=%s, IP=%s, �˿�=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Request I frame from superior CMS success:Superior CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s, front end physical device ID = % s, IP = % s, port = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_request_ifram_data_proc() SIP_SendMessage OK:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                }
            }
        }
        else
        {
            SystemLog(EV9000_CMS_REQUEST_IFRAM_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS����������I֡ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=û���ҵ���Ӧ�������豸��Ϣ:DeviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, callee_id);
            EnSystemLog(EV9000_CMS_REQUEST_IFRAM_ERROR, EV9000_LOG_LEVEL_ERROR, "Request I frame from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=corresponding physical device info not found:DeviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, callee_id);
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_request_ifram_data_proc() exit---: Get LogicDevice's GBDevice Info Error: DeviceID=%s \r\n", callee_id);
            return -1;
        }
    }
    else
    {
        pGBDeviceInfo = GBDevice_info_find(callee_id);

        if (NULL != pGBDeviceInfo)
        {
            if (EV9000_DEVICETYPE_MGWSERVER == pGBDeviceInfo->device_type
                || (EV9000_DEVICETYPE_SIPSERVER == pGBDeviceInfo->device_type && 0 == pGBDeviceInfo->three_party_flag))
            {
                i = SIP_SendMessage(NULL, local_cms_id_get(), callee_id, pGBDeviceInfo->strRegServerIP, pGBDeviceInfo->iRegServerPort, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)inPacket.GetXml(NULL).c_str(), inPacket.GetXml(NULL).length());

                if (i != 0)
                {
                    SystemLog(EV9000_CMS_REQUEST_IFRAM_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS����������I֡ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=%s, ǰ�������豸ID=%s, IP=%s, �˿�=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"ת����Ϣ��ǰ�������豸ʧ��", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    EnSystemLog(EV9000_CMS_REQUEST_IFRAM_ERROR, EV9000_LOG_LEVEL_ERROR, "Request I frame from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=%s, front-end physical deviceID=%s, IP=%s, port=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"forward message to frond-end physical device failed", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_request_ifram_data_proc() SIP_SendMessage Error:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                }
                else
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS����������I֡�ɹ�:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ǰ�������豸ID=%s, IP=%s, �˿�=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Request I frame from superior CMS success:Superior CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s, front end physical device ID = % s, IP = % s, port = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_request_ifram_data_proc() SIP_SendMessage OK:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                }
            }
        }
        else
        {
            SystemLog(EV9000_CMS_REQUEST_IFRAM_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS����������I֡ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=û���ҵ���Ӧ�������豸��Ϣ:DeviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, callee_id);
            EnSystemLog(EV9000_CMS_REQUEST_IFRAM_ERROR, EV9000_LOG_LEVEL_ERROR, "Request I frame from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=corresponding physical device info not found:DeviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, callee_id);
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_request_ifram_data_proc() exit---: Find GB Device Info Error:DeviceID=%s \r\n", callee_id);
            return -1;
        }
    }

    return i;
}

/*****************************************************************************
 �� �� ��  : route_control_autozoomin_proc
 ��������  : �ϼ�����CMS���͹����ĵ���Ŵ�
 �������  : route_info_t* pRouteInfo
             char* caller_id
             char* callee_id
             CPacket& inPacket
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��1��28�� ���ڶ�
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int route_control_autozoomin_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, CPacket& inPacket)
{
    int i = 0;
    GBDevice_info_t* pGBDeviceInfo = NULL;
    GBLogicDevice_info_t* pGBLogicDeviceInfo = NULL;

    if (NULL == pRouteInfo)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_control_autozoomin_proc() exit---: Route Info Error \r\n");
        return -1;
    }

    if (NULL == caller_id || NULL == callee_id)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_control_autozoomin_proc() exit---: Param Error \r\n");
        return -1;
    }

    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_control_autozoomin_proc() Enter---: caller_id=%s, caller_ip=%s, caller_port=%d, callee_id=%s\r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);
    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS�����ĵ���Ŵ���Ϣ:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Click to enlarge from superior CMS:Superior CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);

    /* ����Ŵ������ֱ��ת����ǰ���豸����������

          ������������ֶ�:

      <?xml version="1.0"?>
      <Control>
      <CmdType> AutoZoomIn</CmdType>
      <SN>43</SN>
      <DeviceID>64010000001110000001</DeviceID>
      <DisplayFrameWide>12</ DisplayFrameWide>      //��ʾ�����
      <DisplayFrameHigh>12</ DisplayFrameHigh >      //��ʾ�����
      <DestMatrixTopLeftX>1.2</ DestMatrixTopLeftX >  //Ŀ��������Ͻ�λ�ã�
      <DestMatrixTopLeftY>1.2</ DestMatrixTopLeftY >  //Ŀ��������Ͻ�λ�ã�
      <DestMatrixWide>22</ DestMatrixWide >             //Ŀ����ο�
      <DestMatrixHigh>13</ DestMatrixHigh >              //Ŀ����θ�
      </ Control >

      */

    pGBLogicDeviceInfo = GBLogicDevice_info_find(callee_id);

    if (NULL != pGBLogicDeviceInfo)
    {
        pGBDeviceInfo = GBDevice_info_get_by_stream_type(pGBLogicDeviceInfo, EV9000_STREAM_TYPE_MASTER);

        if (NULL != pGBDeviceInfo)
        {
            i = SIP_SendMessage(NULL, local_cms_id_get(), callee_id, pGBDeviceInfo->strRegServerIP, pGBDeviceInfo->iRegServerPort, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)inPacket.GetXml(NULL).c_str(), inPacket.GetXml(NULL).length());

            if (i != 0)
            {
                SystemLog(EV9000_CMS_CONTROL_AUTOZOMMIN_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�����ĵ���Ŵ���Ϣ����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=%s, ǰ�������豸ID=%s, IP=%s, �˿�=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"ת����Ϣ��ǰ�������豸ʧ��", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                EnSystemLog(EV9000_CMS_CONTROL_AUTOZOMMIN_ERROR, EV9000_LOG_LEVEL_ERROR, "Click to enlarge from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=%s, front-end physical deviceID=%s, IP=%s, port=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"forward message to frond-end physical device failed", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_control_autozoomin_proc() SIP_SendMessage Error:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
            }
            else
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS�����ĵ���Ŵ���Ϣ����ɹ�:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ǰ�������豸ID=%s, IP=%s, �˿�=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Click to enlarge from superior CMS success: the higher the CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s, front end physical device ID = % s, IP = % s, port = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_control_autozoomin_proc() SIP_SendMessage OK:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
            }
        }
        else
        {
            SystemLog(EV9000_CMS_CONTROL_AUTOZOMMIN_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�����ĵ���Ŵ���Ϣ����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=û���ҵ���Ӧ�������豸��Ϣ:DeviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, callee_id);
            EnSystemLog(EV9000_CMS_CONTROL_AUTOZOMMIN_ERROR, EV9000_LOG_LEVEL_ERROR, "Click to enlarge from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=corresponding physical device info not found:DeviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, callee_id);
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_control_autozoomin_proc() exit---: Get LogicDevice's GBDevice Info Error: DeviceID=%s \r\n", callee_id);
            return -1;
        }
    }
    else
    {
        pGBDeviceInfo = GBDevice_info_find(callee_id);

        if (NULL != pGBDeviceInfo)
        {
            i = SIP_SendMessage(NULL, local_cms_id_get(), callee_id, pGBDeviceInfo->strRegServerIP, pGBDeviceInfo->iRegServerPort, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)inPacket.GetXml(NULL).c_str(), inPacket.GetXml(NULL).length());

            if (i != 0)
            {
                SystemLog(EV9000_CMS_CONTROL_AUTOZOMMIN_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�����ĵ���Ŵ���Ϣ����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=%s, ǰ�������豸ID=%s, IP=%s, �˿�=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"ת����Ϣ��ǰ�������豸ʧ��", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                EnSystemLog(EV9000_CMS_CONTROL_AUTOZOMMIN_ERROR, EV9000_LOG_LEVEL_ERROR, "Click to enlarge from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=%s, front-end physical deviceID=%s, IP=%s, port=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"forward message to frond-end physical device failed", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_control_autozoomin_proc() SIP_SendMessage Error:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
            }
            else
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS�����ĵ���Ŵ���Ϣ����ɹ�:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ǰ�������豸ID=%s, IP=%s, �˿�=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Click to enlarge from superior CMS success: the higher the CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s, front end physical device ID = % s, IP = % s, port = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_control_autozoomin_proc() SIP_SendMessage OK:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
            }
        }
        else
        {
            SystemLog(EV9000_CMS_CONTROL_AUTOZOMMIN_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�����ĵ���Ŵ���Ϣ����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=û���ҵ���Ӧ�������豸��Ϣ:DeviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, callee_id);
            EnSystemLog(EV9000_CMS_CONTROL_AUTOZOMMIN_ERROR, EV9000_LOG_LEVEL_ERROR, "Click to enlarge from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=corresponding physical device info not found:DeviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, callee_id);
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_control_autozoomin_proc() exit---: Find GB Device Info Error:DeviceID=%s \r\n", callee_id);
            return -1;
        }
    }

    return i;
}

/*****************************************************************************
 �� �� ��  : route_set_device_xy_param_proc
 ��������  : �ϼ�����CMS���͹��������õ�λ�ľ�γ����Ϣ����
 �������  : route_info_t* pRouteInfo
             char* caller_id
             char* callee_id
             CPacket& inPacket
             DBOper* pRoute_Srv_dboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��3��31�� ����һ
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int route_set_device_xy_param_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, CPacket& inPacket, DBOper* pRoute_Srv_dboper)
{
    int i = 0;
    char strSN[32] = {0};
    char strDeviceID[32] = {0};
    char strLongitude[64] = {0};
    char strLatitude[64] = {0};
    char strMapLayer[MAX_128CHAR_STRING_LEN + 4] = {0};
    double longitude = 0.0;
    double latitude = 0.0;
    GBDevice_info_t* pGBDeviceInfo = NULL;
    GBLogicDevice_info_t* pGBLogicDeviceInfo = NULL;

    if (NULL == pRouteInfo)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_set_device_xy_param_proc() exit---: Route Info Error \r\n");
        return -1;
    }

    if (NULL == caller_id || NULL == callee_id)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_set_device_xy_param_proc() exit---: Param Error \r\n");
        return -1;
    }

    if (NULL == pRoute_Srv_dboper)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_set_device_xy_param_proc() exit---: Route Srv dboper Error \r\n");
        return -1;
    }

    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_set_device_xy_param_proc() Enter---: caller_id=%s, caller_ip=%s, caller_port=%d, callee_id=%s\r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);
    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS���������õ�λ�ľ�γ����Ϣ:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Set point latitude and longitude from superior CMS :the higher the CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);

    /* ����ǰ�˾�γ�ȵ�����ֱ�����õ����ݿ�

          ������������ֶ�:

      <?xml version="1.0"?>
          <Control>
          <CmdType>SetDeviceXYParam</CmdType>
          <SN>1234</SN>
          <DeviceID>�߼��豸ID</DeviceID>
          <Longitude>����</Longitude>
          <Latitude>γ��</Latitude>
          </Control>
      */

    /* ȡ�� ����*/
    inPacket.GetElementValue((char*)"SN", strSN);
    inPacket.GetElementValue((char*)"DeviceID", strDeviceID);
    inPacket.GetElementValue((char*)"Longitude", strLongitude);
    inPacket.GetElementValue((char*)"Latitude", strLatitude);
    inPacket.GetElementValue((char*)"MapLayer", strMapLayer);

    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO,  "route_set_device_xy_param_proc() \
    \r\n XML Para: \
    \r\n SN=%s \
    \r\n DeviceID=%s \
    \r\n Longitude=%s \
    \r\n Latitude=%s \
    \r\n MapLayer=%s \r\n", strSN, strDeviceID, strLongitude, strLatitude, strMapLayer);

    pGBLogicDeviceInfo = GBLogicDevice_info_find(strDeviceID);

    if (NULL != pGBLogicDeviceInfo)
    {
        pGBDeviceInfo = GBDevice_info_get_by_stream_type(pGBLogicDeviceInfo, EV9000_STREAM_TYPE_MASTER);

        if (NULL != pGBDeviceInfo)
        {
            if (EV9000_DEVICETYPE_SIPSERVER == pGBDeviceInfo->device_type) /* ������¼��ĵ�λ��������ͬ����ʱ�� �������¼���λ�ľ�γ����Ϣ */
            {
                if (pGBDeviceInfo->link_type == 1)
                {
                    i = SIP_SendMessage(NULL, local_cms_id_get(), callee_id, pGBDeviceInfo->strRegServerIP, pGBDeviceInfo->iRegServerPort, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)inPacket.GetXml(NULL).c_str(), inPacket.GetXml(NULL).length());

                    if (i != 0)
                    {
                        SystemLog(EV9000_CMS_SET_XY_PARAM_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS���������õ�λ�ľ�γ����Ϣ����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=%s, �¼� CMS ID=%s, IP=%s, �˿�=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"��λ����ͬ��CMS���¼�CMS, ת����Ϣ��ǰ�������豸ʧ��", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                        EnSystemLog(EV9000_CMS_SET_XY_PARAM_ERROR, EV9000_LOG_LEVEL_ERROR, "Set point latitude and longitude from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=%s, subordinate CMS ID=%s, IP=%s, port=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"point belongs to subordinate CMS among parallel CMS, forward message to frond-end physical device failed", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_set_device_xy_param_proc() SIP_SendMessage Error:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    }
                    else
                    {
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS���������õ�λ�ľ�γ����Ϣ����ɹ�:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, �¼� CMS ID=%s, IP=%s, �˿�=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Set point latitude and longitude from superior CMS  success:Superior CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s, the lower the CMS, ID = % s IP = % s, port = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_set_device_xy_param_proc() SIP_SendMessage OK:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    }

                    return 0;
                }
            }

            /* ���� */
            if (strLongitude[0] != '\0')
            {
                longitude = strtod(strLongitude, (char**) NULL);
            }

            /* γ�� */
            if (strLatitude[0] != '\0')
            {
                latitude = strtod(strLatitude, (char**) NULL);
            }

            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE,  "route_set_device_xy_param_proc() Longitude=%.16lf, Latitude=%.16lf \r\n", longitude, latitude);

            if (pGBLogicDeviceInfo->longitude != longitude
                || pGBLogicDeviceInfo->latitude != latitude
                || 0 != sstrcmp(pGBLogicDeviceInfo->map_layer, strMapLayer)) /* �б仯�͸��� */
            {
                pGBLogicDeviceInfo->longitude = longitude;
                pGBLogicDeviceInfo->latitude = latitude;

                if (0 != sstrcmp(pGBLogicDeviceInfo->map_layer, strMapLayer))
                {
                    memset(pGBLogicDeviceInfo->map_layer, 0, MAX_128CHAR_STRING_LEN + 4);
                    osip_strncpy(pGBLogicDeviceInfo->map_layer, strMapLayer, MAX_128CHAR_STRING_LEN);
                }

                /* ���µ����ݿ� */
                i = UpdateGBLogicDeviceXYParam2DB(pGBLogicDeviceInfo->device_id, longitude, latitude, strMapLayer, pRoute_Srv_dboper);

                if (i < 0)
                {
                    SystemLog(EV9000_CMS_SET_XY_PARAM_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS���������õ�λ�ľ�γ����Ϣ����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ����=%.16lf, γ��=%.16lf, ԭ��=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, longitude, latitude, (char*)"���µ����ݿ�ʧ��");
                    EnSystemLog(EV9000_CMS_SET_XY_PARAM_ERROR, EV9000_LOG_LEVEL_ERROR, "Set point latitude and longitude from superior CMS failed:requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, longitude=%.16lf, latitude=%.16lf, cause=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, longitude, latitude, (char*)"update to database failed");
                    DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_set_device_xy_param_proc() UpdateGBLogicDeviceXYParam2DB Error \r\n");
                }
                else
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS���������õ�λ�ľ�γ����Ϣ����ɹ�:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ����=%.16lf, γ��=%.16lf", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, longitude, latitude);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Set point latitude and longitude from superior CMS  success:Superior CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s, the lower the CMS, ID = % s IP = % s, port = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_set_device_xy_param_proc() UpdateGBLogicDeviceXYParam2DB OK \r\n");
                }
            }
        }
        else
        {
            SystemLog(EV9000_CMS_SET_XY_PARAM_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS���������õ�λ�ľ�γ����Ϣ����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=û���ҵ���Ӧ�������豸��Ϣ:GBLogicDeviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, strDeviceID);
            EnSystemLog(EV9000_CMS_SET_XY_PARAM_ERROR, EV9000_LOG_LEVEL_ERROR, "Set point latitude and longitude from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=corresponding physical device info not found:GBLogicDeviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, strDeviceID);
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_set_device_xy_param_proc() exit---: Get LogicDevice's GBDevice Info Error: DeviceID=%s \r\n", strDeviceID);
            return -1;
        }
    }
    else
    {
        SystemLog(EV9000_CMS_SET_XY_PARAM_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS���������õ�λ�ľ�γ����Ϣ����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=û���ҵ���Ӧ���߼��豸��Ϣ:GBLogicDeviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, strDeviceID);
        EnSystemLog(EV9000_CMS_SET_XY_PARAM_ERROR, EV9000_LOG_LEVEL_ERROR, "Set point latitude and longitude from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=corresponding logic device info not found:GBLogicDeviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, strDeviceID);
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_set_device_xy_param_proc() exit---: Find GBLogicDevice Info Error:GBLogicDeviceID=%s \r\n", strDeviceID);
        return -1;
    }

    return i;
}

/*****************************************************************************
 �� �� ��  : route_execute_preset_proc
 ��������  : �ϼ�CMS������ִ��Ԥ��λ����
 �������  : route_info_t* pRouteInfo
             char* caller_id
             char* callee_id
             CPacket& inPacket
             DBOper* pRoute_Srv_dboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��1��28�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int route_execute_preset_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, CPacket& inPacket, DBOper* pRoute_Srv_dboper)
{
    int i = 0;
    char strSN[32] = {0};
    char strDeviceID[32] = {0};
    char strPresetID[64] = {0};
    int iPresetID = 0;

    if (NULL == pRouteInfo)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_execute_preset_proc() exit---: Route Info Error \r\n");
        return -1;
    }

    if (NULL == caller_id || NULL == callee_id)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_execute_preset_proc() exit---: Param Error \r\n");
        return -1;
    }

    if (NULL == pRoute_Srv_dboper)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_execute_preset_proc() exit---: Route Srv dboper Error \r\n");
        return -1;
    }

    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_execute_preset_proc() Enter---: caller_id=%s, caller_ip=%s, caller_port=%d, callee_id=%s\r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);
    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS������ִ��Ԥ��λ��Ϣ:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "The execution of the preset message of superior CMS: superior, CMS ID = % s = % s IP address, port number = % d, logical device ID = % s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);
    /* ȡ�� ����*/
    inPacket.GetElementValue((char*)"SN", strSN);
    inPacket.GetElementValue((char*)"DeviceID", strDeviceID);
    inPacket.GetElementValue((char*)"PresetID", strPresetID);

    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO,  "route_execute_preset_proc() \
    \r\n XML Para: \
    \r\n SN=%s \
    \r\n DeviceID=%s \
    \r\n PresetID=%s \r\n", strSN, strDeviceID, strPresetID);

    iPresetID = osip_atoi(strPresetID);

    i = ExecuteDevicePresetByPresetID(iPresetID, pRoute_Srv_dboper);

    if (i != 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS������ִ��Ԥ��λ��Ϣ����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, PresetID=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, iPresetID);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "The execution of the preset message of superior CMS processing failure: the higher the CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s, PresetID=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, iPresetID);
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_execute_preset_proc() ExecuteDevicePresetByPresetID Error:PresetID=%d\r\n", iPresetID);
    }
    else
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS������ִ��Ԥ��λ��Ϣ����ɹ�:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, PresetID=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, iPresetID);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "The execution of the preset message of superior CMS processing success: the higher the CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s, PresetID = %", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, iPresetID);
        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_execute_preset_proc() ExecuteDevicePresetByPresetID OK:PresetID=%d\r\n", iPresetID);
    }

    return i;
}

/*****************************************************************************
 �� �� ��  : route_query_device_preset_proc
 ��������  : �ϼ�����CMS���͹����Ļ�ȡǰ��Ԥ��λ
 �������  : route_info_t* pRouteInfo
             char* caller_id
             char* callee_id
             CPacket& inPacket
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��1��28�� ���ڶ�
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int route_query_device_preset_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, CPacket& inPacket)
{
    int i = 0;
    GBDevice_info_t* pGBDeviceInfo = NULL;
    GBLogicDevice_info_t* pGBLogicDeviceInfo = NULL;

    if (NULL == pRouteInfo)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_query_device_preset_proc() exit---: Route Info Error \r\n");
        return -1;
    }

    if (NULL == caller_id || NULL == callee_id)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_query_device_preset_proc() exit---: Param Error \r\n");
        return -1;
    }

    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_device_preset_proc() Enter---: caller_id=%s, caller_ip=%s, caller_port=%d, callee_id=%s\r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);
    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS�����Ļ�ȡǰ��Ԥ��λ��Ϣ:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Access front-end preset from superior CMS::Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);

    /* ��ȡǰ��Ԥ��λ������ֱ��ת����ǰ���豸����������

          ������������ֶ�:

      <Query>
      <CmdType>GetDevicePreset</CmdType>
      <SN>43</SN>
      <DeviceID>64010000001110000001</DeviceID>
      </Query>

      */

    pGBLogicDeviceInfo = GBLogicDevice_info_find(callee_id);

    if (NULL != pGBLogicDeviceInfo)
    {
        pGBDeviceInfo = GBDevice_info_get_by_stream_type(pGBLogicDeviceInfo, EV9000_STREAM_TYPE_MASTER);

        if (NULL != pGBDeviceInfo)
        {
            i = SIP_SendMessage(NULL, local_cms_id_get(), callee_id, pGBDeviceInfo->strRegServerIP, pGBDeviceInfo->iRegServerPort, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)inPacket.GetXml(NULL).c_str(), inPacket.GetXml(NULL).length());

            if (i != 0)
            {
                SystemLog(EV9000_CMS_GET_PRESET_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�����Ļ�ȡǰ��Ԥ��λ��Ϣ����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=%s, ǰ�������豸ID=%s, IP=%s, �˿�=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"ת����Ϣ��ǰ�������豸ʧ��", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                EnSystemLog(EV9000_CMS_GET_PRESET_ERROR, EV9000_LOG_LEVEL_ERROR, "Access front-end preset from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=%s, front-end physical deviceID=%s, IP=%s, port=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"forward message to frond-end physical device failed", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_device_preset_proc() SIP_SendMessage Error:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
            }
            else
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS�����Ļ�ȡǰ��Ԥ��λ��Ϣ����ɹ�:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ǰ�������豸ID=%s, IP=%s, �˿�=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Access front-end preset from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, front-end physical deviceID=%s, IP=%s, port=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_device_preset_proc() SIP_SendMessage OK:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
            }
        }
        else
        {
            SystemLog(EV9000_CMS_GET_PRESET_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�����Ļ�ȡǰ��Ԥ��λ��Ϣ����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=û���ҵ���Ӧ�������豸��Ϣ:DeviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, callee_id);
            EnSystemLog(EV9000_CMS_GET_PRESET_ERROR, EV9000_LOG_LEVEL_ERROR, "Access front-end preset from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=corresponding physical device info not found:DeviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, callee_id);
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_device_preset_proc() exit---: Get LogicDevice's GBDevice Info Error: DeviceID=%s \r\n", callee_id);
            return -1;
        }
    }
    else
    {
        pGBDeviceInfo = GBDevice_info_find(callee_id);

        if (NULL != pGBDeviceInfo)
        {
            i = SIP_SendMessage(NULL, local_cms_id_get(), callee_id, pGBDeviceInfo->strRegServerIP, pGBDeviceInfo->iRegServerPort, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)inPacket.GetXml(NULL).c_str(), inPacket.GetXml(NULL).length());

            if (i != 0)
            {
                SystemLog(EV9000_CMS_GET_PRESET_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�����Ļ�ȡǰ��Ԥ��λ��Ϣ����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=%s, ǰ�������豸ID=%s, IP=%s, �˿�=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"ת����Ϣ��ǰ�������豸ʧ��", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                EnSystemLog(EV9000_CMS_GET_PRESET_ERROR, EV9000_LOG_LEVEL_ERROR, "Access front-end preset from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=%s, front-end physical deviceID=%s, IP=%s, port=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"forward message to frond-end physical device failed", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_device_preset_proc() SIP_SendMessage Error:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
            }
            else
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS�����Ļ�ȡǰ��Ԥ��λ��Ϣ����ɹ�:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ǰ�������豸ID=%s, IP=%s, �˿�=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Access front-end preset from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, front-end physical deviceID=%s, IP=%s, port=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_device_preset_proc() SIP_SendMessage OK:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
            }
        }
        else
        {
            SystemLog(EV9000_CMS_GET_PRESET_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�����Ļ�ȡǰ��Ԥ��λʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=û���ҵ���Ӧ�������豸��Ϣ:DeviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, callee_id);
            EnSystemLog(EV9000_CMS_GET_PRESET_ERROR, EV9000_LOG_LEVEL_ERROR, "Access front-end preset from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=corresponding physical device info not found:DeviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, callee_id);
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_device_preset_proc() exit---: Find GB Device Info Error:DeviceID=%s \r\n", callee_id);
            return -1;
        }
    }

    return i;
}

/*****************************************************************************
 �� �� ��  : route_get_db_ip_proc
 ��������  : �ϼ�����CMS���͹����Ļ�ȡ����CMS���ݿ�IP��ַ
 �������  : route_info_t* pRouteInfo
             char* caller_id
             char* callee_id
             CPacket& inPacket
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��3��10�� ����һ
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int route_get_db_ip_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, CPacket& inPacket)
{
    int iRet = 0;
    char strSN[32] = {0};
    char strDeviceID[32] = {0};
    char strEthName1[MAX_IP_LEN] = {0};
    char strEthName2[MAX_IP_LEN] = {0};
    char* local_ip = NULL;

    if (NULL == pRouteInfo)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_get_db_ip_proc() exit---: Route Info Error \r\n");
        return -1;
    }

    if (NULL == caller_id || NULL == callee_id)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_get_db_ip_proc() exit---: Param Error \r\n");
        return -1;
    }

    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_get_db_ip_proc() Enter---: caller_id=%s, caller_ip=%s, caller_port=%d, callee_id=%s\r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);
    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS�����Ļ�ȡ���ݿ�IP��ַ��Ϣ:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Access database IP address from superior CMS :Superior CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);

    /* ȡ������*/
    inPacket.GetElementValue((char*)"SN", strSN);
    inPacket.GetElementValue((char*)"DeviceID", strDeviceID);

    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO,  "route_get_db_ip_proc() \
    \r\n XML Para: \
    \r\n SN=%s, DeviceID=%s \r\n", strSN, strDeviceID);

    /* �ظ���Ӧ,�齨��Ϣ */
    CPacket outPacket;
    DOMElement* AccNode = NULL;

    outPacket.SetRootTag("Response");

    AccNode = outPacket.CreateElement((char*)"CmdType");
    outPacket.SetElementValue(AccNode, (char*)"GetDBIP");

    AccNode = outPacket.CreateElement((char*)"SN");
    outPacket.SetElementValue(AccNode, strSN);

    AccNode = outPacket.CreateElement((char*)"DeviceID");
    outPacket.SetElementValue(AccNode, strDeviceID);

    AccNode = outPacket.CreateElement((char*)"DBIP");

    if (0 == sstrcmp(pGblconf->db_server_ip, (char*)"127.0.0.1"))
    {
        local_ip = local_ip_get(pRouteInfo->strRegLocalEthName);

        if (NULL != local_ip)
        {
            outPacket.SetElementValue(AccNode, local_ip);
        }
        else
        {
            outPacket.SetElementValue(AccNode, (char*)"");
        }
    }
    else if (pGblconf->db_server_ip[0] == '\0')
    {
        local_ip = local_ip_get(pRouteInfo->strRegLocalEthName);

        if (NULL != local_ip)
        {
            outPacket.SetElementValue(AccNode, local_ip);
        }
        else
        {
            outPacket.SetElementValue(AccNode, (char*)"");
        }
    }
    else
    {
        outPacket.SetElementValue(AccNode, pGblconf->db_server_ip);
    }

    iRet = SIP_SendMessage(NULL, local_cms_id_get(), caller_id, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

    if (iRet != 0)
    {
        SystemLog(EV9000_CMS_GET_DBIP_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�����Ļ�ȡ���ݿ�IP��ַ��Ϣ����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=����Ӧ����Ϣʧ��", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
        EnSystemLog(EV9000_CMS_GET_DBIP_ERROR, EV9000_LOG_LEVEL_ERROR, "Access database IP address from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=send reply message failed", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_get_db_ip_proc() SIP_SendMessage Error:callee_id=%s, caller_ip=%s, caller_port=%d \r\n", callee_id, pRouteInfo->server_ip, pRouteInfo->server_port);
    }
    else
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS�����Ļ�ȡ���ݿ�IP��ַ��Ϣ����ɹ�:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Access database IP address from superior CMS success:Superior CMS, ID = % s = % s IP address, port number = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_get_db_ip_proc() SIP_SendMessage OK:callee_id=%s, caller_ip=%s, caller_port=%d \r\n", callee_id, pRouteInfo->server_ip, pRouteInfo->server_port);
    }

    return iRet;
}

/*****************************************************************************
 �� �� ��  : route_notify_tv_status_proc
 ��������  : �ϼ�����CMS���͹�����֪ͨ����ǽ״̬
 �������  : route_info_t* pRouteInfo
             char* caller_id
             char* callee_id
             CPacket& inPacket
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��1��28�� ���ڶ�
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int route_notify_tv_status_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, CPacket& inPacket)
{
    int i = 0;
    GBDevice_info_t* pGBDeviceInfo = NULL;
    GBLogicDevice_info_t* pGBLogicDeviceInfo = NULL;

    if (NULL == pRouteInfo)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_notify_tv_status_proc() exit---: Route Info Error \r\n");
        return -1;
    }

    if (NULL == caller_id || NULL == callee_id)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_notify_tv_status_proc() exit---: Param Error \r\n");
        return -1;
    }

    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_notify_tv_status_proc() Enter---: caller_id=%s, caller_ip=%s, caller_port=%d, callee_id=%s\r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);
    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS������֪ͨ����ǽ״̬��Ϣ:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Notify TV wall status from superior CMS:Superior CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);

    /* ��ȡǰ��ͼ�����������ֱ��ת����ǰ���豸����������

          ������������ֶ�:

      <?xml version="1.0"?>
      <Notify>
      <CmdType>TVStatus</ CmdType>
      <SN>1234</SN>
      <DecoderChannelID>�������߼�ͨ������</DecoderChannelID>
      <Status></Status>        //  max  normal
      </Notify>

      */

    pGBLogicDeviceInfo = GBLogicDevice_info_find(callee_id);

    if (NULL != pGBLogicDeviceInfo)
    {
        pGBDeviceInfo = GBDevice_info_get_by_stream_type(pGBLogicDeviceInfo, EV9000_STREAM_TYPE_MASTER);

        if (NULL != pGBDeviceInfo)
        {
            i = SIP_SendMessage(NULL, local_cms_id_get(), callee_id, pGBDeviceInfo->strRegServerIP, pGBDeviceInfo->iRegServerPort, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)inPacket.GetXml(NULL).c_str(), inPacket.GetXml(NULL).length());

            if (i != 0)
            {
                SystemLog(EV9000_CMS_NOTIFY_DEC_STATUS_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS������֪ͨ����ǽ״̬��Ϣ����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=%s, ǰ�������豸ID=%s, IP=%s, �˿�=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"ת����Ϣ��ǰ�������豸ʧ��", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                EnSystemLog(EV9000_CMS_NOTIFY_DEC_STATUS_ERROR, EV9000_LOG_LEVEL_ERROR, "Notify TV wall status from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=%s, front-end physical deviceID=%s, IP=%s, port=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"forward message to frond-end physical device failed", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_notify_tv_status_proc() SIP_SendMessage Error:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
            }
            else
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS������֪ͨ����ǽ״̬��Ϣ����ɹ�:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ǰ�������豸ID=%s, IP=%s, �˿�=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Notify TV wall status from superior CMS success:Superior CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s, front end physical device ID = % s, IP = % s, port = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_notify_tv_status_proc() SIP_SendMessage OK:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
            }
        }
        else
        {
            SystemLog(EV9000_CMS_NOTIFY_DEC_STATUS_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS������֪ͨ����ǽ״̬��Ϣ����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=û���ҵ���Ӧ�������豸��Ϣ:DeviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, callee_id);
            EnSystemLog(EV9000_CMS_NOTIFY_DEC_STATUS_ERROR, EV9000_LOG_LEVEL_ERROR, "Notify TV wall status from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=corresponding physical device info not found:DeviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, callee_id);
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_notify_tv_status_proc() exit---: Get LogicDevice's GBDevice Info Error: DeviceID=%s \r\n", callee_id);
            return -1;
        }
    }
    else
    {
        SystemLog(EV9000_CMS_NOTIFY_DEC_STATUS_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS������֪ͨ����ǽ״̬��Ϣ����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=û���ҵ���Ӧ���߼��豸��Ϣ:DeviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, callee_id);
        EnSystemLog(EV9000_CMS_NOTIFY_DEC_STATUS_ERROR, EV9000_LOG_LEVEL_ERROR, "Notify TV wall status from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=orresponding logic device info not found:DeviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, callee_id);
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_notify_tv_status_proc() exit---: Find GBDevice Info Error: DeviceID=%s \r\n", callee_id);
        return -1;
    }

    return i;
}

/*****************************************************************************
 �� �� ��  : route_query_catalog_proc
 ��������  : �ϼ�����CMS���͹����Ĳ�ѯ�豸Ŀ¼
 �������  : route_info_t* pRouteInfo
             char* caller_id
             char* callee_id
             CPacket& inPacket
             DBOper* pRoute_Srv_dboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��5��20��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int route_query_catalog_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, CPacket& inPacket, DBOper* pRoute_Srv_dboper)
{
    int i = 0;
    char strSN[32] = {0};
    char strDeviceID[32] = {0};
    GBDevice_info_t* pGBDeviceInfo = NULL;
    GBLogicDevice_info_t* pGBLogicDeviceInfo = NULL;
    char strTransferSN[32] = {0};
    DOMElement* AccSnNode = NULL;
    unsigned int old_xml_sn = 0;
    unsigned int transfer_xml_sn = 0;

    if (NULL == pRouteInfo)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_query_catalog_proc() exit---: Route Info Error \r\n");
        return -1;
    }

    if (NULL == caller_id || NULL == callee_id || NULL == pRoute_Srv_dboper)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_query_catalog_proc() exit---: Route Srv DB Oper Error \r\n");
        return -1;
    }

    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_catalog_proc() Enter---: caller_id=%s, caller_ip=%s, caller_port=%d, callee_id=%s \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);
    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS�����Ļ�ȡ�߼��豸��Ϣ��Ϣ:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Search for device info from superior CMS:Superior CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);

    /* �����豸��Ϣ��ѯ
          �������̼�9.5.2
          �豸Ŀ¼��ѯ������ֱ��ת����ǰ���豸����������

          ������������ֶ�:
          <!-- �������ͣ��豸Ŀ¼��ѯ����ѡ�� -->
          <element name="CmdType" fixed ="Catalog" />
          <!-- �������кţ���ѡ�� -->
          <element name="SN" type="integer" minInclusive value = "1" />
          <!-- Ŀ���豸���豸���루��ѡ�� -->
          <element name="DeviceID" type="tg:deviceIDType" />
      */
    /* ȡ������*/
    inPacket.GetElementValue((char*)"SN", strSN);
    inPacket.GetElementValue((char*)"DeviceID", strDeviceID);

    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO,  "route_query_catalog_proc() \
            \r\n XML Para: \
            \r\n SN=%s, DeviceID=%s\r\n", strSN, strDeviceID);

    /* �鿴�����Ƿ��Ǳ�CMS ID */
    if (0 == strncmp(callee_id, local_cms_id_get(), 20))
    {
        if (0 == sstrcmp(callee_id, strDeviceID)) /* �����ѯ���豸IDΪ��CMS ID����ʾ��ȡ���豸�б�*/
        {
            i = RouteGetGBDeviceListAndSendCataLogToCMS(pRouteInfo, caller_id, strDeviceID, strSN, pRoute_Srv_dboper);
        }
        else /* ��ȡ�����ĳһ���豸��Ϣ */
        {
            pGBLogicDeviceInfo = GBLogicDevice_info_find(strDeviceID);

            if (NULL != pGBLogicDeviceInfo)
            {
                pGBDeviceInfo = GBDevice_info_get_by_stream_type(pGBLogicDeviceInfo, EV9000_STREAM_TYPE_MASTER);

                if (NULL != pGBDeviceInfo)
                {
                    AccSnNode = inPacket.SearchElement((char*)"SN");

                    if (NULL != AccSnNode)
                    {
                        g_transfer_xml_sn++;
                        snprintf(strTransferSN, 32, "%u", g_transfer_xml_sn);
                        inPacket.SetElementValue(AccSnNode, strTransferSN);
                        //inPacket.SetTextContent();
                    }

                    i = SIP_SendMessage(NULL, local_cms_id_get(), callee_id, pGBDeviceInfo->strRegServerIP, pGBDeviceInfo->iRegServerPort, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)inPacket.GetXml(NULL).c_str(), inPacket.GetXml(NULL).length());

                    if (i != 0)
                    {
                        SystemLog(EV9000_CMS_GET_DEVICE_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�����Ļ�ȡ�߼��豸��Ϣ��Ϣ����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=%s, ǰ�������豸ID=%s, IP��ַ=%s, �˿�=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"ת����Ϣ��ǰ�������豸ʧ��", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                        EnSystemLog(EV9000_CMS_GET_DEVICE_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "Search for device info from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=%s, front-end physical deviceID=%s, IP=%s, port=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"forward message to frond-end physical device failed", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_catalog_proc() SIP_SendMessage Error:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    }
                    else
                    {
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS�����Ļ�ȡ�߼��豸��Ϣ��Ϣ����ɹ�,ת����Ϣ��ǰ�������豸:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ǰ�������豸ID=%s, IP��ַ=%s, �˿�=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Search for device info from superior CMS success,Forwards messages to the front end physical device: the superior CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s, front-end, physical device ID = % s = % s IP address, port = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_catalog_proc() SIP_SendMessage OK:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);

                        old_xml_sn = strtoul(strSN, NULL, 10);
                        transfer_xml_sn = strtoul(strTransferSN, NULL, 10);
                        i = transfer_xml_msg_add(XML_QUERY_CATALOG, old_xml_sn, transfer_xml_sn, pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, strDeviceID);
                        DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_query_catalog_proc() transfer_xml_msg_add: Type=%d, old_xml_sn=%u, transfer_xml_sn=%u, DeviceID=%s, pos=%d \r\n", XML_QUERY_CATALOG, old_xml_sn, transfer_xml_sn, strDeviceID, i);

                    }
                }
            }
            else
            {
                SystemLog(EV9000_CMS_GET_DEVICE_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�����Ļ�ȡ�߼��豸��Ϣ��Ϣ����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=û���ҵ���Ӧ�������豸��Ϣ:DeviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, callee_id);
                EnSystemLog(EV9000_CMS_GET_DEVICE_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "Search for device info from superior CMS failed:Superior CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s, reason = didn't find the corresponding physical device information: DeviceID = % s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, callee_id);
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_catalog_proc() exit---: Find GBDevice Info Error  \r\n");
                return -1;
            }
        }
    }
    else
    {
        pGBLogicDeviceInfo = GBLogicDevice_info_find(callee_id);

        if (NULL != pGBLogicDeviceInfo)
        {
            pGBDeviceInfo = GBDevice_info_get_by_stream_type(pGBLogicDeviceInfo, EV9000_STREAM_TYPE_MASTER);

            if (NULL != pGBDeviceInfo)
            {
                AccSnNode = inPacket.SearchElement((char*)"SN");

                if (NULL != AccSnNode)
                {
                    g_transfer_xml_sn++;
                    snprintf(strTransferSN, 32, "%u", g_transfer_xml_sn);
                    inPacket.SetElementValue(AccSnNode, strTransferSN);
                    //inPacket.SetTextContent();
                }

                i = SIP_SendMessage(NULL, local_cms_id_get(), callee_id, pGBDeviceInfo->strRegServerIP, pGBDeviceInfo->iRegServerPort, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)inPacket.GetXml(NULL).c_str(), inPacket.GetXml(NULL).length());

                if (i != 0)
                {
                    SystemLog(EV9000_CMS_GET_DEVICE_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�����Ļ�ȡ�߼��豸��Ϣ��Ϣ����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=%s, ת��ǰ�������豸ID=%s, IP��ַ=%s, �˿�=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"ת����Ϣ��ǰ�������豸ʧ��", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    EnSystemLog(EV9000_CMS_GET_DEVICE_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "Search for device info from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=%s, front-end physical deviceID=%s, IP=%s, port=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"forward message to frond-end physical device failed", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_catalog_proc() SIP_SendMessage Error:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                }
                else
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS�����Ļ�ȡ�߼��豸��Ϣ��Ϣ����ɹ�,ת����Ϣ��ǰ�������豸:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ת��ǰ�������豸ID=%s, IP��ַ=%s, �˿�=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Search for device info from superior CMS success,Forwarding messages to the front end physical device: the superior CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s, forwarding the front-end, physical device ID = % s = % s IP address, port = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_catalog_proc() SIP_SendMessage OK:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);

                    old_xml_sn = strtoul(strSN, NULL, 10);
                    transfer_xml_sn = strtoul(strTransferSN, NULL, 10);
                    i = transfer_xml_msg_add(XML_QUERY_CATALOG, old_xml_sn, transfer_xml_sn, pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, strDeviceID);
                    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_query_catalog_proc() transfer_xml_msg_add: Type=%d, old_xml_sn=%u, transfer_xml_sn=%u, DeviceID=%s, pos=%d \r\n", XML_QUERY_CATALOG, old_xml_sn, transfer_xml_sn, strDeviceID, i);

                }
            }
            else
            {
                SystemLog(EV9000_CMS_GET_DEVICE_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�����Ļ�ȡ�߼��豸��Ϣ��Ϣ����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=û���ҵ���Ӧ�������豸��Ϣ:DeviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, callee_id);
                EnSystemLog(EV9000_CMS_GET_DEVICE_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "Search for device info from superior CMS failure:Superior CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s, reason = didn't find the corresponding physical device information: DeviceID = % s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, callee_id);
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_catalog_proc() exit---: Get LogicDevice's GBDevice Info Error: DeviceID=%s \r\n", callee_id);
                return -1;
            }
        }
        else
        {
            pGBDeviceInfo = GBDevice_info_find(callee_id);

            if (NULL != pGBDeviceInfo)
            {
                AccSnNode = inPacket.SearchElement((char*)"SN");

                if (NULL != AccSnNode)
                {
                    g_transfer_xml_sn++;
                    snprintf(strTransferSN, 32, "%u", g_transfer_xml_sn);
                    inPacket.SetElementValue(AccSnNode, strTransferSN);
                    //inPacket.SetTextContent();
                }

                if (i != 0)
                {
                    SystemLog(EV9000_CMS_GET_DEVICE_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�����Ļ�ȡ�߼��豸��Ϣ��Ϣ����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=%s, ת��ǰ�������豸ID=%s, IP��ַ=%s, �˿�=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"ת����Ϣ��ǰ�������豸ʧ��", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    EnSystemLog(EV9000_CMS_GET_DEVICE_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "Search for device info from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=%s, front-end physical deviceID=%s, IP=%s, port=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"forward message to frond-end physical device failed", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_catalog_proc() SIP_SendMessage Error:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                }
                else
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS�����Ļ�ȡ�߼��豸��Ϣ��Ϣ����ɹ�,ת����Ϣ��ǰ�������豸:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ת��ǰ�������豸ID=%s, IP��ַ=%s, �˿�=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Search for device info from superior CMS success, forwarding the message to the front end physical device:Superior CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s, forward front physical device ID = % s, IP address = % s, port = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_catalog_proc() SIP_SendMessage OK:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);

                    old_xml_sn = strtoul(strSN, NULL, 10);
                    transfer_xml_sn = strtoul(strTransferSN, NULL, 10);
                    i = transfer_xml_msg_add(XML_QUERY_CATALOG, old_xml_sn, transfer_xml_sn, pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, strDeviceID);
                    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_query_catalog_proc() transfer_xml_msg_add: Type=%d, old_xml_sn=%u, transfer_xml_sn=%u, DeviceID=%s, pos=%d \r\n", XML_QUERY_CATALOG, old_xml_sn, transfer_xml_sn, strDeviceID, i);

                }
            }
            else
            {
                SystemLog(EV9000_CMS_GET_DEVICE_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�����Ļ�ȡ�߼��豸��Ϣ��Ϣ����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=û���ҵ���Ӧ�������豸��Ϣ:DeviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, callee_id);
                EnSystemLog(EV9000_CMS_GET_DEVICE_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "Search for device info from superior CMS failed:Superior CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s, reason = didn't find the corresponding physical device information: DeviceID = % s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, callee_id);
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_catalog_proc() exit---: Find GBDevice Info Error: DeviceID=%s \r\n", callee_id);
                return -1;
            }
        }
    }

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS�����Ļ�ȡ�߼��豸��Ϣ��Ϣ����ɹ�:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Search for device info from superior CMS  success:Superior CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);
    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_catalog_proc() Exit---\r\n");

    return i;
}

/*****************************************************************************
 �� �� ��  : route_query_device_info_proc
 ��������  : �ϼ�����CMS���͹����Ĳ�ѯ�豸��Ϣ����
 �������  : route_info_t* pRouteInfo
             char* caller_id
             char* callee_id
             CPacket& inPacket
             DBOper* pRoute_Srv_dboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��5��20��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int route_query_device_info_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, CPacket& inPacket, DBOper* pRoute_Srv_dboper)
{
    int i = 0;
    char strSN[32] = {0};
    char strDeviceID[32] = {0};
    GBDevice_info_t* pGBDeviceInfo = NULL;
    GBLogicDevice_info_t* pGBLogicDeviceInfo = NULL;
    char strTransferSN[32] = {0};
    DOMElement* AccSnNode = NULL;
    unsigned int old_xml_sn = 0;
    unsigned int transfer_xml_sn = 0;

    if (NULL == pRouteInfo)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_query_device_info_proc() exit---: Route Info Error \r\n");
        return -1;
    }

    if (NULL == caller_id || NULL == callee_id || NULL == pRoute_Srv_dboper)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_query_device_info_proc() exit---: Route Srv DB Oper Error \r\n");
        return -1;
    }

    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_device_info_proc() Enter---: caller_id=%s, caller_ip=%s, caller_port=%d, callee_id=%s\r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);
    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS�����Ĳ�ѯ�豸��Ϣ��Ϣ����:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Search for device info from superior CMS:the higher the CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);

    /* �����豸��Ϣ��ѯ
          �������̼�9.5.2
          �豸��Ϣ��ѯ������ֱ��ת����ǰ���豸����������

          ������������ֶ�:
          <!-- �������ͣ��豸��Ϣ��ѯ����ѡ�� -->
          <element name="CmdType" fixed ="DeviceInfo" />
          <!-- �������кţ���ѡ�� -->
          <element name="SN" type="integer" minInclusive value = "1" />
          <!-- Ŀ���豸���豸���루��ѡ�� -->
          <element name="DeviceID" type="tg:deviceIDType" />
      */
    /* ȡ������*/
    inPacket.GetElementValue((char*)"SN", strSN);
    inPacket.GetElementValue((char*)"DeviceID", strDeviceID);

    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO,  "route_query_device_info_proc() \
            \r\n XML Para: \
            \r\n SN=%s, DeviceID=%s \r\n", strSN, strDeviceID);

    /* �鿴�����Ƿ��Ǳ�CMS ID */
    if (0 == strncmp(callee_id, local_cms_id_get(), 20))
    {
        i = SendDeviceInfoMessageToRouteCMS(caller_id, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, strSN, strDeviceID, pRouteInfo->three_party_flag, pRouteInfo->trans_protocol);

        if (i != 0)
        {
            SystemLog(EV9000_CMS_GET_DEVICE_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�����Ĳ�ѯ�豸��Ϣ��Ϣ����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, ԭ��=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"����Ӧ����Ϣʧ��");
            EnSystemLog(EV9000_CMS_GET_DEVICE_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "Search for device info from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, cause=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"send reply message failed");
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_device_info_proc() SendDeviceInfoMessageToRouteCMS Error\r\n");
        }
        else
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS�����Ĳ�ѯ�豸��Ϣ��Ϣ����ɹ�:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Search for device info from superior CMS success:Superior CMS, ID = % s = % s IP address, port number = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_device_info_proc() SendDeviceInfoMessageToRouteCMS OK\r\n");
        }
    }
    else
    {
        pGBLogicDeviceInfo = GBLogicDevice_info_find(callee_id);

        if (NULL != pGBLogicDeviceInfo)
        {
            pGBDeviceInfo = GBDevice_info_get_by_stream_type(pGBLogicDeviceInfo, EV9000_STREAM_TYPE_MASTER);

            if (NULL != pGBDeviceInfo)
            {
                AccSnNode = inPacket.SearchElement((char*)"SN");

                if (NULL != AccSnNode)
                {
                    g_transfer_xml_sn++;
                    snprintf(strTransferSN, 32, "%u", g_transfer_xml_sn);
                    inPacket.SetElementValue(AccSnNode, strTransferSN);
                    //inPacket.SetTextContent();
                }

                i = SIP_SendMessage(NULL, local_cms_id_get(), callee_id, pGBDeviceInfo->strRegServerIP, pGBDeviceInfo->iRegServerPort, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)inPacket.GetXml(NULL).c_str(), inPacket.GetXml(NULL).length());

                if (i != 0)
                {
                    SystemLog(EV9000_CMS_GET_DEVICE_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�����Ĳ�ѯ�豸��Ϣ��Ϣ����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=%s, ת��ǰ�������豸ID=%s, IP��ַ=%s, �˿�=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"ת����Ϣ��ǰ�������豸ʧ��", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    EnSystemLog(EV9000_CMS_GET_DEVICE_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "Search for device info from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=%s, front-end physical deviceID=%s, IP=%s, port=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"forward message to frond-end physical device failed", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_device_info_proc() SIP_SendMessage Error:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                }
                else
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS�����Ĳ�ѯ�豸��Ϣ��Ϣ����ɹ�,ת����Ϣ��ǰ�������豸:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ת��ǰ�������豸ID=%s, IP��ַ=%s, �˿�=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Search for device info from superior CMS success, forwarding the message to the front end physical device:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ת��ǰ�������豸ID=%s, IP��ַ=%s, �˿�=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_device_info_proc() SIP_SendMessage OK:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);

                    old_xml_sn = strtoul(strSN, NULL, 10);
                    transfer_xml_sn = strtoul(strTransferSN, NULL, 10);
                    i = transfer_xml_msg_add(XML_QUERY_DEVICEINFO, old_xml_sn, transfer_xml_sn, pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, strDeviceID);
                    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_query_device_info_proc() transfer_xml_msg_add: Type=%d, old_xml_sn=%u, transfer_xml_sn=%u, DeviceID=%s, pos=%d \r\n", XML_QUERY_DEVICEINFO, old_xml_sn, transfer_xml_sn, strDeviceID, i);

                }
            }
            else
            {
                SystemLog(EV9000_CMS_GET_DEVICE_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�����Ĳ�ѯ�豸��Ϣ��Ϣ����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=û���ҵ���Ӧ�������豸��Ϣ:DeviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, callee_id);
                EnSystemLog(EV9000_CMS_GET_DEVICE_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "Search for device info from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=corresponding physical device info not found:DeviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, callee_id);
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_device_info_proc() exit---: Get LogicDevice's GBDevice Info Error: DeviceID=%s \r\n", callee_id);
                return -1;
            }
        }
        else
        {
            pGBDeviceInfo = GBDevice_info_find(callee_id);

            if (NULL != pGBDeviceInfo)
            {
                AccSnNode = inPacket.SearchElement((char*)"SN");

                if (NULL != AccSnNode)
                {
                    g_transfer_xml_sn++;
                    snprintf(strTransferSN, 32, "%u", g_transfer_xml_sn);
                    inPacket.SetElementValue(AccSnNode, strTransferSN);
                    //inPacket.SetTextContent();
                }

                i = SIP_SendMessage(NULL, local_cms_id_get(), callee_id, pGBDeviceInfo->strRegServerIP, pGBDeviceInfo->iRegServerPort, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)inPacket.GetXml(NULL).c_str(), inPacket.GetXml(NULL).length());

                if (i != 0)
                {
                    SystemLog(EV9000_CMS_GET_DEVICE_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�����Ĳ�ѯ�豸��Ϣ��Ϣ����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=%s, ת��ǰ�������豸ID=%s, IP��ַ=%s, �˿�=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"ת����Ϣ��ǰ�������豸ʧ��", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    EnSystemLog(EV9000_CMS_GET_DEVICE_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "Search for device info from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=%s, front-end physical deviceID=%s, IP=%s, port=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"forward message to frond-end physical device failed", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_device_info_proc() SIP_SendMessage Error:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                }
                else
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS�����Ĳ�ѯ�豸��Ϣ��Ϣ����ɹ�,ת����Ϣ��ǰ�������豸:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ת��ǰ�������豸ID=%s, IP��ַ=%s, �˿�=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Search for device info from superior CMS success, forwarding the message to the front end physical device:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ת��ǰ�������豸ID=%s, IP��ַ=%s, �˿�=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_device_info_proc() SIP_SendMessage OK:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);

                    old_xml_sn = strtoul(strSN, NULL, 10);
                    transfer_xml_sn = strtoul(strTransferSN, NULL, 10);
                    i = transfer_xml_msg_add(XML_QUERY_DEVICEINFO, old_xml_sn, transfer_xml_sn, pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, strDeviceID);
                    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_query_device_info_proc() transfer_xml_msg_add: Type=%d, old_xml_sn=%u, transfer_xml_sn=%u, DeviceID=%s, pos=%d \r\n", XML_QUERY_DEVICEINFO, old_xml_sn, transfer_xml_sn, strDeviceID, i);

                }
            }
            else
            {
                SystemLog(EV9000_CMS_GET_DEVICE_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�����Ĳ�ѯ�豸��Ϣ��Ϣ����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=û���ҵ���Ӧ�������豸��Ϣ:DeviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, callee_id);
                EnSystemLog(EV9000_CMS_GET_DEVICE_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "Search for device info from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=corresponding physical device info not found:DeviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, callee_id);
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_device_info_proc() exit---: Find GBDevice Info Error: DeviceID=%s \r\n", callee_id);
                return -1;
            }
        }
    }

    return i;
}

/*****************************************************************************
 �� �� ��  : route_query_device_status_proc
 ��������  : �ϼ�����CMS���͹����Ĳ�ѯ�豸״̬����
 �������  : route_info_t* pRouteInfo
             char* caller_id
             char* callee_id
             CPacket& inPacket
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��5��20��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int route_query_device_status_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, CPacket& inPacket)
{
    int i = 0;
    char strSN[32] = {0};
    char strDeviceID[32] = {0};
    GBLogicDevice_info_t* pGBLogicDeviceInfo = NULL;
    GBDevice_info_t* pGBDeviceInfo = NULL;
    CPacket outPacket;
    DOMElement* AccNode = NULL;
    string strStatus = "";

    if (NULL == pRouteInfo)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_query_device_status_proc() exit---: Route Info Error \r\n");
        return -1;
    }

    if (NULL == caller_id || NULL == callee_id)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_query_device_status_proc() exit---: Param Error \r\n");
        return -1;
    }

    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_device_status_proc() Enter---: caller_id=%s, caller_ip=%s, caller_port=%d, callee_id=%s\r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);
    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS�����Ĳ�ѯ�豸״̬��Ϣ:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Search for device status from superior CMS:Superior CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);


    /* �����豸��Ϣ��ѯ
          �������̼�9.5.2
          �豸״̬��ѯ������ֱ��ת����ǰ���豸����������

          ������������ֶ�:
          <!-- �������ͣ��豸״̬��ѯ����ѡ�� -->
          <element name="CmdType" fixed ="DeviceStatus" />
          <!-- �������кţ���ѡ�� -->
          <element name="SN" type="integer" minInclusive value = "1" />
          <!-- Ŀ���豸���豸���루��ѡ�� -->
          <element name="DeviceID" type="tg:deviceIDType" />
      */

    /* ȡ������*/
    inPacket.GetElementValue((char*)"SN", strSN);
    inPacket.GetElementValue((char*)"DeviceID", strDeviceID);

    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO,  "route_query_device_status_proc() \
        \r\n XML Para: \
        \r\n SN=%s, DeviceID=%s \r\n", strSN, strDeviceID);

    /* �鿴�����Ƿ��Ǳ�CMS ID */
    if (0 == strncmp(callee_id, local_cms_id_get(), 20))
    {
        /* �ظ���Ӧ,�齨��Ϣ */
        outPacket.SetRootTag("Response");
        AccNode = outPacket.CreateElement((char*)"CmdType");
        outPacket.SetElementValue(AccNode, (char*)"DeviceStatus");

        AccNode = outPacket.CreateElement((char*)"SN");
        outPacket.SetElementValue(AccNode, strSN);

        AccNode = outPacket.CreateElement((char*)"DeviceID");
        outPacket.SetElementValue(AccNode, strDeviceID);

        AccNode = outPacket.CreateElement((char*)"Result");
        outPacket.SetElementValue(AccNode, (char*)"OK");

        AccNode = outPacket.CreateElement((char*)"Online");
        outPacket.SetElementValue(AccNode, (char*)"ONLINE");

        AccNode = outPacket.CreateElement((char*)"Status");
        outPacket.SetElementValue(AccNode, (char*)"OK");

        /* ������Ӧ��Ϣ ���ϼ�CMS */
        i = SIP_SendMessage(NULL, local_cms_id_get(), caller_id, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

        if (i != 0)
        {
            SystemLog(EV9000_CMS_GET_DEVICE_STATUS_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�����Ĳ�ѯ�豸״̬��Ϣ����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, ԭ��=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"����Ӧ����Ϣʧ��");
            EnSystemLog(EV9000_CMS_GET_DEVICE_STATUS_ERROR, EV9000_LOG_LEVEL_ERROR, "Search for device status from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, cause=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"send reply message failed");
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_device_status_proc() SIP_SendMessage Error:caller_id=%s, caller_ip=%s, caller_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
        }
        else
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS�����Ĳ�ѯ�豸״̬��Ϣ����ɹ�:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Search for device status from superior CMS success:Superior CMS, ID = % s = % s IP address, port number = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_device_status_proc() SIP_SendMessage OK:caller_id=%s, caller_ip=%s, caller_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
        }
    }
    else
    {
        strStatus = "OFFLINE";

        pGBLogicDeviceInfo = GBLogicDevice_info_find(strDeviceID);  /* �����߼��豸���� */

        if (NULL != pGBLogicDeviceInfo)
        {
            if (pRouteInfo->three_party_flag) /* ������ƽ̨ */
            {
                if (1 == pGBLogicDeviceInfo->status)
                {
                    strStatus = "ON";
                }
                else if (2 == pGBLogicDeviceInfo->status)
                {
                    strStatus = "VLOST";
                }
            }
            else
            {
                if (1 == pGBLogicDeviceInfo->status)
                {
                    if (INTELLIGENT_STATUS_ON == pGBLogicDeviceInfo->intelligent_status)
                    {
                        strStatus = "INTELLIGENT";
                    }
                    else if (ALARM_STATUS_CLOSE == pGBLogicDeviceInfo->alarm_status)
                    {
                        strStatus = "CLOSE";
                    }
                    else if (ALARM_STATUS_APART == pGBLogicDeviceInfo->alarm_status)
                    {
                        strStatus = "APART";
                    }
                    else
                    {
                        strStatus = "ONLINE";
                    }
                }
                else if (2 == pGBLogicDeviceInfo->status)
                {
                    strStatus = "NOVIDEO";
                }
            }

            /* �ظ���Ӧ,�齨��Ϣ */
            outPacket.SetRootTag("Response");
            AccNode = outPacket.CreateElement((char*)"CmdType");
            outPacket.SetElementValue(AccNode, (char*)"DeviceStatus");

            AccNode = outPacket.CreateElement((char*)"SN");
            outPacket.SetElementValue(AccNode, strSN);

            AccNode = outPacket.CreateElement((char*)"DeviceID");
            outPacket.SetElementValue(AccNode, strDeviceID);

            AccNode = outPacket.CreateElement((char*)"Result");
            outPacket.SetElementValue(AccNode, (char*)"OK");

            AccNode = outPacket.CreateElement((char*)"Online");
            outPacket.SetElementValue(AccNode, (char*)strStatus.c_str());

            AccNode = outPacket.CreateElement((char*)"Status");
            outPacket.SetElementValue(AccNode, (char*)"OK");

            /* ������Ӧ��Ϣ ���ϼ�CMS */
            i = SIP_SendMessage(NULL, local_cms_id_get(), caller_id, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

            if (i != 0)
            {
                SystemLog(EV9000_CMS_GET_DEVICE_STATUS_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�����Ĳ�ѯ�豸״̬��Ϣ����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, ԭ��=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"����Ӧ����Ϣʧ��");
                EnSystemLog(EV9000_CMS_GET_DEVICE_STATUS_ERROR, EV9000_LOG_LEVEL_ERROR, "Search for device status from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d,cause=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"send reply message failed");
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_device_status_proc() SIP_SendMessage Error:caller_id=%s, caller_ip=%s, caller_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
            }
            else
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS�����Ĳ�ѯ�豸״̬��Ϣ����ɹ�:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Search for device status from superior CMS success:Superior CMS, ID = % s = % s IP address, port number = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_device_status_proc() SIP_SendMessage OK:caller_id=%s, caller_ip=%s, caller_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
            }
        }
        else
        {
            pGBDeviceInfo = GBDevice_info_find(callee_id);

            if (NULL != pGBDeviceInfo)
            {
                i = SIP_SendMessage(NULL, local_cms_id_get(), callee_id, pGBDeviceInfo->strRegServerIP, pGBDeviceInfo->iRegServerPort, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)inPacket.GetXml(NULL).c_str(), inPacket.GetXml(NULL).length());

                if (i != 0)
                {
                    SystemLog(EV9000_CMS_GET_DEVICE_STATUS_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�����Ĳ�ѯ�豸״̬��Ϣ����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=%s, ǰ�������豸ID=%s, IP=%s, �˿�=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"ת����Ϣ��ǰ�������豸ʧ��", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    EnSystemLog(EV9000_CMS_GET_DEVICE_STATUS_ERROR, EV9000_LOG_LEVEL_ERROR, "Search for device status from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=%s, front-end physical deviceID=%s, IP=%s, port=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"forward message to frond-end physical device failed", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_device_status_proc() SIP_SendMessage Error:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                }
                else
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS�����Ĳ�ѯ�豸״̬��Ϣ����ɹ�:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ǰ�������豸ID=%s, IP=%s, �˿�=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Search for device status from superior CMS success:Superior CMS, ID = % s = % s IP address, port number = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_device_status_proc() SIP_SendMessage OK:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                }
            }
            else
            {
                SystemLog(EV9000_CMS_GET_DEVICE_STATUS_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�����Ĳ�ѯ�豸״̬��Ϣ����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=û���ҵ���Ӧ�������豸��Ϣ:DeviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, callee_id);
                EnSystemLog(EV9000_CMS_GET_DEVICE_STATUS_ERROR, EV9000_LOG_LEVEL_ERROR, "Search for device status from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=corresponding physical device info not found:DeviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, callee_id);
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_device_status_proc() exit---: Find GBDevice Info Error: DeviceID=%s \r\n", callee_id);
                return -1;
            }
        }
    }

    return i;
}

/*****************************************************************************
 �� �� ��  : route_query_device_config_proc
 ��������  : �ϼ�����CMS���͹����Ĳ�ѯ�豸���ô���
 �������  : route_info_t* pRouteInfo
             char* caller_id
             char* callee_id
             CPacket& inPacket
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��7��15�� ���ڶ�
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int route_query_device_config_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, CPacket& inPacket)
{
    int i = 0;
    char strSN[32] = {0};
    char strDeviceID[32] = {0};
    char strConfigType[32] = {0};
    GBDevice_info_t* pGBDeviceInfo = NULL;
    GBLogicDevice_info_t* pGBLogicDeviceInfo = NULL;
    CPacket outPacket;
    DOMElement* AccNode = NULL;
    DOMElement* ListAccNode = NULL;
    DOMElement* ItemAccNode = NULL;
    DOMElement* BasicParamNode = NULL;
    char strServerPort[32] = {0};
    char strExpire[32] = {0};
    char strHeartBeatInterval[32] = {0};

    char strTransferSN[32] = {0};
    DOMElement* AccSnNode = NULL;
    unsigned int old_xml_sn = 0;
    unsigned int transfer_xml_sn = 0;

    if (NULL == pRouteInfo)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_query_device_config_proc() exit---: Route Info Error \r\n");
        return -1;
    }

    if (NULL == caller_id || NULL == callee_id)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_query_device_config_proc() exit---: Param Error \r\n");
        return -1;
    }

    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_device_config_proc() Enter---: caller_id=%s, caller_ip=%s, caller_port=%d, callee_id=%s\r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);
    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS�����Ĳ�ѯ�豸������Ϣ:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Search for device config from superior CMS:Superior CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);

    /* �����豸���ò�ѯ

          �豸���ò�ѯ������ֱ��ת����ǰ���豸����������
          ������������ֶ�:
          <!-- �������ͣ��豸��Ϣ��ѯ����ѡ�� -->
          <element name="CmdType" fixed ="ConfigDownload" />
          <!-- �������кţ���ѡ�� -->
          <element name="SN" type="integer" minInclusive value = "1" />
          <!-- Ŀ���豸���豸���루��ѡ�� -->
          <element name="DeviceID" type="tg:deviceIDType" />
      */

    /* ȡ������*/
    inPacket.GetElementValue((char*)"SN", strSN);
    inPacket.GetElementValue((char*)"DeviceID", strDeviceID);
    inPacket.GetElementValue((char*)"ConfigType", strConfigType);

    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO,  "route_query_device_config_proc() \
        \r\n XML Para: \
        \r\n SN=%s, DeviceID=%s, ConfigType=%s \r\n", strSN, strDeviceID, strConfigType);

    if (strSN[0] == '\0')
    {
        SystemLog(EV9000_CMS_GET_DEVICE_CONFIG_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�����Ĳ�ѯ�豸������Ϣ����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=��ȡXML��Ϣ�е�SNʧ��", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);
        EnSystemLog(EV9000_CMS_GET_DEVICE_CONFIG_ERROR, EV9000_LOG_LEVEL_ERROR, "Search for device config from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=access SN from XML failed", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_device_config_proc() exit---: Get XML SN Error\r\n");
        return -1;
    }

    if (strDeviceID[0] == '\0')
    {
        SystemLog(EV9000_CMS_GET_DEVICE_CONFIG_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�����Ĳ�ѯ�豸������Ϣ����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=��ȡXML��Ϣ�е�DeviceIDʧ��", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);
        EnSystemLog(EV9000_CMS_GET_DEVICE_CONFIG_ERROR, EV9000_LOG_LEVEL_ERROR, "Search for device config from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=access device ID SN from XML failed", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_device_config_proc() exit---: Get XML DeviceID Error\r\n");
        return -1;
    }

    /* �鿴�����Ƿ��Ǳ�CMS ID */
    if (0 == strncmp(callee_id, local_cms_id_get(), 20))
    {
        /* �ظ���Ӧ,�齨��Ϣ */
        if (0 == sstrcmp(strConfigType, (char*)"VideoParamConfig"))
        {
            /* �ظ���Ӧ,�齨��Ϣ */
            outPacket.SetRootTag("Response");
            AccNode = outPacket.CreateElement((char*)"CmdType");
            outPacket.SetElementValue(AccNode, (char*)"ConfigDownload");

            AccNode = outPacket.CreateElement((char*)"SN");
            outPacket.SetElementValue(AccNode, strSN);

            AccNode = outPacket.CreateElement((char*)"DeviceID");
            outPacket.SetElementValue(AccNode, strDeviceID);

            AccNode = outPacket.CreateElement((char*)"Result");
            outPacket.SetElementValue(AccNode, (char*)"OK");

            ListAccNode = outPacket.CreateElement((char*)"VideoParamConfig");
            outPacket.SetElementAttr(ListAccNode, (char*)"Num", (char*)"1");

            outPacket.SetCurrentElement(ListAccNode);
            ItemAccNode = outPacket.CreateElement((char*)"Item");
            outPacket.SetCurrentElement(ItemAccNode);

            AccNode = outPacket.CreateElement((char*)"StreamName");
            outPacket.SetElementValue(AccNode, (char*)"Stream1");

            AccNode = outPacket.CreateElement((char*)"VideoFormat");
            outPacket.SetElementValue(AccNode, (char*)"2");

            AccNode = outPacket.CreateElement((char*)"Resolution");
            outPacket.SetElementValue(AccNode, (char*)"0");

            AccNode = outPacket.CreateElement((char*)"FrameRate");
            outPacket.SetElementValue(AccNode, (char*)"25");

            AccNode = outPacket.CreateElement((char*)"BitRateType");
            outPacket.SetElementValue(AccNode, (char*)"1");

            AccNode = outPacket.CreateElement((char*)"VideoBitRate");
            outPacket.SetElementValue(AccNode, (char*)"2048");

            /* ������Ӧ��Ϣ ���ϼ�CMS */
            i = SIP_SendMessage(NULL, local_cms_id_get(), caller_id, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

            if (i != 0)
            {
                SystemLog(EV9000_CMS_GET_DEVICE_CONFIG_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�����Ĳ�ѯ�豸������Ϣ����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, ԭ��=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"����Ӧ����Ϣʧ��");
                EnSystemLog(EV9000_CMS_GET_DEVICE_CONFIG_ERROR, EV9000_LOG_LEVEL_ERROR, "Search for device config from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, cause=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"send reply message failed");
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_device_status_proc() SIP_SendMessage Error:caller_id=%s, caller_ip=%s, caller_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
            }
            else
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS�����Ĳ�ѯ�豸������Ϣ����ɹ�:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Search for device config from superior CMS success:Superior CMS, ID = % s = % s IP address, port number = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_device_status_proc() SIP_SendMessage OK:caller_id=%s, caller_ip=%s, caller_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
            }
        }
        else if (0 == sstrcmp(strConfigType, (char*)"BasicParam"))
        {
            outPacket.SetRootTag("Response");
            AccNode = outPacket.CreateElement((char*)"CmdType");
            outPacket.SetElementValue(AccNode, (char*)"ConfigDownload");

            AccNode = outPacket.CreateElement((char*)"SN");
            outPacket.SetElementValue(AccNode, strSN);

            AccNode = outPacket.CreateElement((char*)"DeviceID");
            outPacket.SetElementValue(AccNode, strDeviceID);

            AccNode = outPacket.CreateElement((char*)"Result");
            outPacket.SetElementValue(AccNode, (char*)"OK");

            BasicParamNode = outPacket.CreateElement((char*)"BasicParam");
            outPacket.SetCurrentElement(BasicParamNode);

            /* Name */
            AccNode = outPacket.CreateElement((char*)"Name");
            outPacket.SetElementValue(AccNode, local_cms_name_get());

            /* ID */
            AccNode = outPacket.CreateElement((char*)"DeviceID");
            outPacket.SetElementValue(AccNode, local_cms_id_get());

            /* SIP Server ID */
            AccNode = outPacket.CreateElement((char*)"SIPServerID");
            outPacket.SetElementValue(AccNode, pRouteInfo->server_id);

            /* SIP Server IP */
            AccNode = outPacket.CreateElement((char*)"SIPServerIP");
            outPacket.SetElementValue(AccNode, pRouteInfo->server_ip);

            /* SIP Server Port */
            AccNode = outPacket.CreateElement((char*)"SIPServerPort");
            snprintf(strServerPort, 32, "%d", pRouteInfo->server_port);
            outPacket.SetElementValue(AccNode, strServerPort);

            /* DomainName */
            AccNode = outPacket.CreateElement((char*)"DomainName");
            outPacket.SetElementValue(AccNode, pRouteInfo->server_host);

            /* Expriration */
            AccNode = outPacket.CreateElement((char*)"Expriration");
            snprintf(strExpire, 32, "%d", pRouteInfo->min_expires);
            outPacket.SetElementValue(AccNode, strExpire);

            /* Password */
            AccNode = outPacket.CreateElement((char*)"Password");
            outPacket.SetElementValue(AccNode, pRouteInfo->register_password);

            /* HeartBeatInterval, �������ʱ�� */
            AccNode = outPacket.CreateElement((char*)"HeartBeatInterval");
            snprintf(strHeartBeatInterval, 32, "%d", local_keep_alive_interval_get());
            outPacket.SetElementValue(AccNode, strHeartBeatInterval);

            /* HeartBeatCount, ������ʱ���� */
            AccNode = outPacket.CreateElement((char*)"HeartBeatCount");
            outPacket.SetElementValue(AccNode, (char*)"3");

            /* ������Ӧ��Ϣ ���ϼ�CMS */
            i = SIP_SendMessage(NULL, local_cms_id_get(), caller_id, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

            if (i != 0)
            {
                SystemLog(EV9000_CMS_GET_DEVICE_CONFIG_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�����Ĳ�ѯ�豸������Ϣ����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, ԭ��=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"����Ӧ����Ϣʧ��");
                EnSystemLog(EV9000_CMS_GET_DEVICE_CONFIG_ERROR, EV9000_LOG_LEVEL_ERROR, "Search for device config from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, cause=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"send reply message failed");
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_device_status_proc() SIP_SendMessage Error:caller_id=%s, caller_ip=%s, caller_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
            }
            else
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS�����Ĳ�ѯ�豸������Ϣ����ɹ�:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Search for device config from superior CMS success:Superior CMS, ID = % s = % s IP address, port number = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_device_status_proc() SIP_SendMessage OK:caller_id=%s, caller_ip=%s, caller_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
            }
        }
    }
    else
    {
        pGBLogicDeviceInfo = GBLogicDevice_info_find(callee_id);

        if (NULL != pGBLogicDeviceInfo)
        {
            pGBDeviceInfo = GBDevice_info_get_by_stream_type(pGBLogicDeviceInfo, EV9000_STREAM_TYPE_MASTER);

            if (NULL != pGBDeviceInfo)
            {
                /* ��ȡ�ϵ�SN�ڵ� */
                AccSnNode = inPacket.SearchElement((char*)"SN");

                if (NULL != AccSnNode)
                {
                    g_transfer_xml_sn++;
                    snprintf(strTransferSN, 32, "%u", g_transfer_xml_sn);
                    inPacket.SetElementValue(AccSnNode, strTransferSN);
                }

                i = SIP_SendMessage(NULL, local_cms_id_get(), callee_id, pGBDeviceInfo->strRegServerIP, pGBDeviceInfo->iRegServerPort, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)inPacket.GetXml(NULL).c_str(), inPacket.GetXml(NULL).length());

                if (i != 0)
                {
                    SystemLog(EV9000_CMS_GET_DEVICE_CONFIG_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�����Ĳ�ѯ�豸������Ϣ����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=%s, ת��ǰ�������豸ID=%s, IP��ַ=%s, �˿�=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"ת����Ϣ��ǰ�������豸ʧ��", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    EnSystemLog(EV9000_CMS_GET_DEVICE_CONFIG_ERROR, EV9000_LOG_LEVEL_ERROR, "Search for device config from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=%s, front-end physical deviceID=%s, IP=%s, port=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"forward message to frond-end physical device failed", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_device_config_proc() SIP_SendMessage Error:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                }
                else
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS�����Ĳ�ѯ�豸������Ϣ����ɹ�,ת����Ϣ��ǰ�������豸:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ת��ǰ�������豸ID=%s, IP��ַ=%s, �˿�=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Search for device config from superior CMS success,Forwarding messages to the front end physical devices:Superior CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s, forward front physical device ID = % s, IP address = % s, port = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_device_config_proc() SIP_SendMessage OK:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);

                    old_xml_sn = strtoul(strSN, NULL, 10);
                    transfer_xml_sn = strtoul(strTransferSN, NULL, 10);
                    i = transfer_xml_msg_add(XML_QUERY_DEVICECONFIG, old_xml_sn, transfer_xml_sn, pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, strDeviceID);
                    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_query_device_config_proc() transfer_xml_msg_add: Type=%d, old_xml_sn=%u, transfer_xml_sn=%u, DeviceID=%s, pos=%d \r\n", XML_QUERY_DEVICECONFIG, old_xml_sn, transfer_xml_sn, strDeviceID, i);
                }
            }
            else
            {
                SystemLog(EV9000_CMS_GET_DEVICE_CONFIG_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�����Ĳ�ѯ�豸������Ϣ����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=û���ҵ���Ӧ�������豸��Ϣ:DeviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, callee_id);
                EnSystemLog(EV9000_CMS_GET_DEVICE_CONFIG_ERROR, EV9000_LOG_LEVEL_ERROR, "Search for device config from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=corresponding physical device info not found:DeviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, callee_id);
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_device_config_proc() exit---: Get LogicDevice's GBDevice Info Error: DeviceID=%s \r\n", callee_id);
                return -1;
            }
        }
        else
        {
            pGBDeviceInfo = GBDevice_info_find(callee_id);

            if (NULL != pGBDeviceInfo)
            {
                /* ��ȡ�ϵ�SN�ڵ� */
                AccSnNode = inPacket.SearchElement((char*)"SN");

                if (NULL != AccSnNode)
                {
                    g_transfer_xml_sn++;
                    snprintf(strTransferSN, 32, "%u", g_transfer_xml_sn);
                    inPacket.SetElementValue(AccSnNode, strTransferSN);
                }

                i = SIP_SendMessage(NULL, local_cms_id_get(), callee_id, pGBDeviceInfo->strRegServerIP, pGBDeviceInfo->iRegServerPort, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)inPacket.GetXml(NULL).c_str(), inPacket.GetXml(NULL).length());

                if (i != 0)
                {
                    SystemLog(EV9000_CMS_GET_DEVICE_CONFIG_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�����Ĳ�ѯ�豸������Ϣ����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=%s, ת��ǰ�������豸ID=%s, IP��ַ=%s, �˿�=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"ת����Ϣ��ǰ�������豸ʧ��", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    EnSystemLog(EV9000_CMS_GET_DEVICE_CONFIG_ERROR, EV9000_LOG_LEVEL_ERROR, "Search for device config from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=%s, front-end physical deviceID=%s, IP=%s, port=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"forward message to frond-end physical device failed", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_device_config_proc() SIP_SendMessage Error:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                }
                else
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS�����Ĳ�ѯ�豸������Ϣ����ɹ�,ת����Ϣ��ǰ�������豸:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ת��ǰ�������豸ID=%s, IP��ַ=%s, �˿�=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Search for device config from superior CMS success,Forwarding messages to the front end physical devices:Superior CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s, forward front physical device ID = % s, IP address = % s, port = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_device_config_proc() SIP_SendMessage OK:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);

                    old_xml_sn = strtoul(strSN, NULL, 10);
                    transfer_xml_sn = strtoul(strTransferSN, NULL, 10);
                    i = transfer_xml_msg_add(XML_QUERY_DEVICECONFIG, old_xml_sn, transfer_xml_sn, pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, strDeviceID);
                    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_query_device_config_proc() transfer_xml_msg_add: Type=%d, old_xml_sn=%u, transfer_xml_sn=%u, DeviceID=%s, pos=%d \r\n", XML_QUERY_DEVICECONFIG, old_xml_sn, transfer_xml_sn, strDeviceID, i);
                }
            }
            else
            {
                SystemLog(EV9000_CMS_GET_DEVICE_CONFIG_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�����Ĳ�ѯ�豸������Ϣ����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=û���ҵ���Ӧ�������豸��Ϣ:DeviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, callee_id);
                EnSystemLog(EV9000_CMS_GET_DEVICE_CONFIG_ERROR, EV9000_LOG_LEVEL_ERROR, "Search for device config from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=corresponding physical device info not found:DeviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, callee_id);
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_device_config_proc() exit---: Find GBDevice Info Error: DeviceID=%s \r\n", callee_id);
                return -1;
            }
        }
    }

    return i;
}

/*****************************************************************************
 �� �� ��  : route_query_record_info_proc
 ��������  : �ϼ�����CMS���͹����Ĳ�ѯ�豸��Ƶ�ļ�����
 �������  : route_info_t* pRouteInfo
             char* caller_id
             char* callee_id
             CPacket& inPacket
             DBOper* pRoute_Srv_dboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��5��20��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int route_query_record_info_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, CPacket& inPacket, DBOper* pRoute_Srv_dboper)
{
    /* �豸����Ƶ�ļ�����
          �������̼�9.7.2
      */
    int i = 0;
    int iRet = 0;
    int index = 0;
    int tsu_index = -1;
    int record_count = 0;
    int query_count = 0;
    char strSN[32] = {0};
    char strDeviceID[32] = {0};
    char strRecordType[32] = {0};
    char strStartTime[32] = {0};
    char strEndTime[32] = {0};
    char strFilePath[32] = {0};
    char strAddress[32] = {0};
    char strSecrecy[32] = {0};
    char str3PartFlag[16] = {0};
    char strRecorderID[32] = {0};
    char strErrorCode[32] = {0};

    GBDevice_info_t* pGBDeviceInfo = NULL;
    GBLogicDevice_info_t* pGBLogicDeviceInfo = NULL;
    tsu_resource_info_t* pTsuResourceInfo = NULL;
    VideoRecordList stVideoRecordList;
    unsigned int iStartTime = 0;
    unsigned int iEndTime = 0;
    int record_type = 0;

    DOMElement* RecordListAccNode = NULL;
    //DOMElement* AccSnNode = NULL;
    char strTransferSN[32] = {0};
    unsigned int old_xml_sn = 0;
    unsigned int transfer_xml_sn = 0;

    int iCalleeRoutePos = 0;
    route_info_t* pCalleeRouteInfo = NULL;

    int record_info_pos = -1;
    record_info_t* pRecordInfo = NULL;

    int iRecordCRPos = -1;
    cr_t* pRecordCrData = NULL;

    if (NULL == pRouteInfo)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_query_record_info_proc() exit---: Route Info Error \r\n");
        return -1;
    }

    if (NULL == caller_id || NULL == callee_id || NULL == pRoute_Srv_dboper)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_query_record_info_proc() exit---: Route Srv DB Oper Error \r\n");
        return -1;
    }

    /* ȡ�ò�ѯ��������*/
    inPacket.GetElementValue((char*)"SN", strSN);
    inPacket.GetElementValue((char*)"DeviceID", strDeviceID);
    inPacket.GetElementValue((char*)"StartTime", strStartTime);
    inPacket.GetElementValue((char*)"EndTime", strEndTime);
    inPacket.GetElementValue((char*)"FilePath", strFilePath);
    inPacket.GetElementValue((char*)"Address", strAddress);
    inPacket.GetElementValue((char*)"Secrecy", strSecrecy);
    inPacket.GetElementValue((char*)"Type", strRecordType);
    inPacket.GetElementValue((char*)"ThreeParty", str3PartFlag);
    inPacket.GetElementValue((char*)"RecorderID", strRecorderID);

    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO,  "route_query_record_info_proc() \
        \r\n XML Param: \
        \r\n SN=%s \
        \r\n DeviceID=%s \
        \r\n RecordType=%s \
        \r\n StartTime=%s \
        \r\n EndTime=%s \
        \r\n", strSN, strDeviceID, strRecordType, strStartTime, strEndTime);

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS�����Ĳ�ѯ¼���¼��Ϣ:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, ��ѯ���߼��豸ID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Search for video record info from superior CMS :CMS ID=%s, IP address=%s, port number=%d, logic deviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID);

    /* 1����ȡ¼���λ��Ϣ */
    pGBLogicDeviceInfo = GBLogicDevice_info_find(strDeviceID);

    if (NULL == pGBLogicDeviceInfo)
    {
        /* �ظ���Ӧ,�齨��Ϣ */
        CPacket outPacket;
        DOMElement* AccNode = NULL;

        outPacket.SetRootTag("Response");

        AccNode = outPacket.CreateElement((char*)"CmdType");
        outPacket.SetElementValue(AccNode, (char*)"RecordInfo");

        AccNode = outPacket.CreateElement((char*)"SN");
        outPacket.SetElementValue(AccNode, strSN);

        AccNode = outPacket.CreateElement((char*)"DeviceID");
        outPacket.SetElementValue(AccNode, strDeviceID);

        AccNode = outPacket.CreateElement((char*)"Name");
        outPacket.SetElementValue(AccNode, (char*)" ");

        AccNode = outPacket.CreateElement((char*)"SumNum");
        outPacket.SetElementValue(AccNode, (char*)"-1");

        AccNode = outPacket.CreateElement((char*)"ErrCode");
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_DEVICE_FIND_LOGIC_DEVICE_INFO_ERROR);
        outPacket.SetElementValue(AccNode, strErrorCode);

        AccNode = outPacket.CreateElement((char*)"Reason");
        outPacket.SetElementValue(AccNode, (char*)"��ȡ�߼��豸��Ϣʧ��");

        RecordListAccNode = outPacket.CreateElement((char*)"RecordList");
        outPacket.SetElementAttr(RecordListAccNode, (char*)"Num", (char*)"-1");

        /* ������Ӧ��Ϣ ���ϼ�CMS */
        i = SIP_SendMessage(NULL, local_cms_id_get(), caller_id, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

        if (i != 0)
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_record_info_proc() SIP_SendMessage Error:caller_id=%s, caller_ip=%s, caller_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
        }
        else
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_record_info_proc() SIP_SendMessage OK:caller_id=%s, caller_ip=%s, caller_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
        }

        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_record_info_proc() exit---: Get GBLogic Device Info Error \r\n");
        SystemLog(EV9000_CMS_QUERY_RECORD_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�����Ĳ�ѯ¼���¼��Ϣʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, ��ѯ���߼��豸ID=%s, ԭ��=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, (char*)"��ȡ�߼��豸��Ϣʧ��");
        EnSystemLog(EV9000_CMS_QUERY_RECORD_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "Search for video record info from superior CMS failed:CMS ID=%s, IP address=%s, port number=%d, cause=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"Access logical device information failed");
        return -1;
    }

    /* ���Ƿ���������ĵ�λ */
    if (1 == pGBLogicDeviceInfo->other_realm)
    {
        /* �����ϼ�·����Ϣ */
        iCalleeRoutePos = route_info_find(pGBLogicDeviceInfo->cms_id);

        if (iCalleeRoutePos < 0)
        {
            /* �ظ���Ӧ,�齨��Ϣ */
            CPacket outPacket;
            DOMElement* AccNode = NULL;

            outPacket.SetRootTag("Response");

            AccNode = outPacket.CreateElement((char*)"CmdType");
            outPacket.SetElementValue(AccNode, (char*)"RecordInfo");

            AccNode = outPacket.CreateElement((char*)"SN");
            outPacket.SetElementValue(AccNode, strSN);

            AccNode = outPacket.CreateElement((char*)"DeviceID");
            outPacket.SetElementValue(AccNode, strDeviceID);

            AccNode = outPacket.CreateElement((char*)"Name");
            outPacket.SetElementValue(AccNode, (char*)" ");

            AccNode = outPacket.CreateElement((char*)"SumNum");
            outPacket.SetElementValue(AccNode, (char*)"-1");

            AccNode = outPacket.CreateElement((char*)"ErrCode");
            memset(strErrorCode, 0, 32);
            snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_ROUTE_FIND_ROUTE_INFO_ERROR);
            outPacket.SetElementValue(AccNode, strErrorCode);

            AccNode = outPacket.CreateElement((char*)"Reason");
            outPacket.SetElementValue(AccNode, (char*)"��ȡ�ϼ�·����Ϣʧ��");

            RecordListAccNode = outPacket.CreateElement((char*)"RecordList");
            outPacket.SetElementAttr(RecordListAccNode, (char*)"Num", (char*)"-1");

            /* ������Ӧ��Ϣ ���ϼ�CMS */
            i = SIP_SendMessage(NULL, local_cms_id_get(), caller_id, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

            if (i != 0)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_record_info_proc() SIP_SendMessage Error:caller_id=%s, caller_ip=%s, caller_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
            }
            else
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_record_info_proc() SIP_SendMessage OK:caller_id=%s, caller_ip=%s, caller_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
            }

            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_record_info_proc() exit---: Get GBLogic Device Info Error \r\n");
            SystemLog(EV9000_CMS_QUERY_RECORD_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�����Ĳ�ѯ¼���¼��Ϣʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, ��ѯ���߼��豸ID=%s, ԭ��=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, (char*)"��ȡ�ϼ�·����Ϣʧ��");
            EnSystemLog(EV9000_CMS_QUERY_RECORD_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "Search for video record info from superior CMS failed:CMS ID=%s, IP address=%s, port number=%d, cause=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"Get Route information failed");
            return -1;
        }

        pCalleeRouteInfo = route_info_get(iCalleeRoutePos);

        if (NULL == pCalleeRouteInfo)
        {
            /* �ظ���Ӧ,�齨��Ϣ */
            CPacket outPacket;
            DOMElement* AccNode = NULL;

            outPacket.SetRootTag("Response");

            AccNode = outPacket.CreateElement((char*)"CmdType");
            outPacket.SetElementValue(AccNode, (char*)"RecordInfo");

            AccNode = outPacket.CreateElement((char*)"SN");
            outPacket.SetElementValue(AccNode, strSN);

            AccNode = outPacket.CreateElement((char*)"DeviceID");
            outPacket.SetElementValue(AccNode, strDeviceID);

            AccNode = outPacket.CreateElement((char*)"Name");
            outPacket.SetElementValue(AccNode, (char*)" ");

            AccNode = outPacket.CreateElement((char*)"SumNum");
            outPacket.SetElementValue(AccNode, (char*)"-1");

            AccNode = outPacket.CreateElement((char*)"ErrCode");
            memset(strErrorCode, 0, 32);
            snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_ROUTE_GET_ROUTE_INFO_ERROR);
            outPacket.SetElementValue(AccNode, strErrorCode);

            AccNode = outPacket.CreateElement((char*)"Reason");
            outPacket.SetElementValue(AccNode, (char*)"��ȡ�ϼ�·����Ϣʧ��");

            RecordListAccNode = outPacket.CreateElement((char*)"RecordList");
            outPacket.SetElementAttr(RecordListAccNode, (char*)"Num", (char*)"-1");

            /* ������Ӧ��Ϣ ���ϼ�CMS */
            i = SIP_SendMessage(NULL, local_cms_id_get(), caller_id, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

            if (i != 0)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_record_info_proc() SIP_SendMessage Error:caller_id=%s, caller_ip=%s, caller_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
            }
            else
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_record_info_proc() SIP_SendMessage OK:caller_id=%s, caller_ip=%s, caller_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
            }

            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_record_info_proc() exit---: Get GBLogic Device Info Error \r\n");
            SystemLog(EV9000_CMS_QUERY_RECORD_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�����Ĳ�ѯ¼���¼��Ϣʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, ��ѯ���߼��豸ID=%s, ԭ��=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, (char*)"��ȡ�ϼ�·����Ϣʧ��");
            EnSystemLog(EV9000_CMS_QUERY_RECORD_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "Search for video record info from superior CMS failed:CMS ID=%s, IP address=%s, port number=%d, cause=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"Get route information failed");
            return -1;
        }

        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS�����Ĳ�ѯ¼���¼��Ϣ���ϼ�CMS�еĵ�λ:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, ��ѯ���߼��豸ID=%s, ���ڵ��ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, pCalleeRouteInfo->server_id, pCalleeRouteInfo->server_ip, pCalleeRouteInfo->server_port);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Superior CMS to query video information is the higher level in the CMS: superior CMS, ID = % s = % s IP address, port number = % d, query logic device ID = % s, the superior CMS, ID = % s = % s IP address, port number = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, pCalleeRouteInfo->server_id, pCalleeRouteInfo->server_ip, pCalleeRouteInfo->server_port);

        /* �����齨��Ϣ */
        CPacket outPacket;
        DOMElement* AccNode = NULL;

        outPacket.SetRootTag("Query");

        AccNode = outPacket.CreateElement((char*)"CmdType");
        outPacket.SetElementValue(AccNode, (char*)"RecordInfo");

        AccNode = outPacket.CreateElement((char*)"SN");
        g_transfer_xml_sn++;
        snprintf(strTransferSN, 32, "%u", g_transfer_xml_sn);
        outPacket.SetElementValue(AccNode, strTransferSN);

        AccNode = outPacket.CreateElement((char*)"DeviceID");
        outPacket.SetElementValue(AccNode, strDeviceID);

        AccNode = outPacket.CreateElement((char*)"StartTime");
        outPacket.SetElementValue(AccNode, strStartTime);

        AccNode = outPacket.CreateElement((char*)"EndTime");
        outPacket.SetElementValue(AccNode, strEndTime);

        AccNode = outPacket.CreateElement((char*)"Type");

        if (strRecordType[0] != 0)
        {
            outPacket.SetElementValue(AccNode, strRecordType);
        }
        else
        {
            outPacket.SetElementValue(AccNode, (char*)"all");
        }

        /* ������ƽ̨��ѯ¼���������ת���ϼ�ȥ���ϼ�ƽ̨���Լ��Ļ�����Ҫ���ϵ�������ʶ */
        if (1 == pRouteInfo->three_party_flag && 0 == pCalleeRouteInfo->three_party_flag)
        {
            AccNode = outPacket.CreateElement((char*)"ThreeParty");
            outPacket.SetElementValue(AccNode, (char*)"YES");
        }

        AccNode = outPacket.CreateElement((char*)"RecorderID");
        outPacket.SetElementValue(AccNode, pCalleeRouteInfo->server_id);

        /* ����ѯ¼���������Ϣת����ȥ */
        i = SIP_SendMessage(NULL, local_cms_id_get(), pCalleeRouteInfo->server_id, pCalleeRouteInfo->strRegLocalIP, pCalleeRouteInfo->iRegLocalPort, pCalleeRouteInfo->server_ip, pCalleeRouteInfo->server_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

        if (i != 0)
        {
            SystemLog(EV9000_CMS_QUERY_RECORD_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�����Ĳ�ѯ¼���¼��Ϣʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, ԭ��=%s, ת���ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, ��ѯ���߼��豸ID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, (char*)"ת����ѯ��Ϣ���ϼ�CMSʧ��", pCalleeRouteInfo->server_id, pCalleeRouteInfo->server_ip, pCalleeRouteInfo->server_port);
            EnSystemLog(EV9000_CMS_QUERY_RECORD_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "Search for video record info from superior CMS , forward search info to superior CMS failed:superior CMS ID=%s, IP address=%s, port number=%d, superior CMS ID=%s, IPaddress =%s, port number=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, pCalleeRouteInfo->server_id, pCalleeRouteInfo->server_ip, pCalleeRouteInfo->server_port);
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_record_info_proc() SIP_SendMessage Error:device_id=%s, device_ip=%s, device_port=%d \r\n", pCalleeRouteInfo->server_id, pCalleeRouteInfo->server_ip, pCalleeRouteInfo->server_port);
        }
        else
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS�����Ĳ�ѯ¼���¼��Ϣ�ɹ�,ת����ѯ��Ϣ���ϼ�CMS:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, ת���ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, ��ѯ���߼��豸ID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, pCalleeRouteInfo->server_id, pCalleeRouteInfo->server_ip, pCalleeRouteInfo->server_port);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Search for video record info from superior success,Forwarding query message to the superior the CMS:Superior CMS, ID = % s = % s IP address, port number = % d, forwarding the superior CMS, ID = % s = % s IP address, port number = % d, query logic device ID = % s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, pCalleeRouteInfo->server_id, pCalleeRouteInfo->server_ip, pCalleeRouteInfo->server_port);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_record_info_proc() SIP_SendMessage OK:server_id=%s, server_ip=%s, server_port=%d \r\n", pCalleeRouteInfo->server_id, pCalleeRouteInfo->server_ip, pCalleeRouteInfo->server_port);

            old_xml_sn = strtoul(strSN, NULL, 10);
            transfer_xml_sn = strtoul(strTransferSN, NULL, 10);
            i = transfer_xml_msg_add(XML_QUERY_RECORDINFO, old_xml_sn, transfer_xml_sn, pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, strDeviceID);
            DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_query_record_info_proc() transfer_xml_msg_add: Type=%d, old_xml_sn=%u, transfer_xml_sn=%u, DeviceID=%s, pos=%d \r\n", XML_QUERY_RECORDINFO, old_xml_sn, transfer_xml_sn, strDeviceID, i);
        }

        return i;
    }

    /* ȷ��¼������ */
    if (strRecordType[0] != 0)
    {
        record_type = osip_atoi(strRecordType);
    }
    else
    {
        record_type = EV9000_RECORD_TYPE_NORMAL;
    }

    if (record_type <= 0)
    {
        record_type = EV9000_RECORD_TYPE_NORMAL;
    }

    /* ��ȡ�߼��豸�����������豸 */
    if (EV9000_RECORD_TYPE_NORMAL == record_type
        || EV9000_RECORD_TYPE_BACKUP == record_type
        || EV9000_RECORD_TYPE_ALARM == record_type)
    {
        pGBDeviceInfo = GBDevice_info_get_by_stream_type(pGBLogicDeviceInfo, EV9000_STREAM_TYPE_MASTER);
    }
    else
    {
        pGBDeviceInfo = GBDevice_info_get_by_stream_type(pGBLogicDeviceInfo, EV9000_STREAM_TYPE_INTELLIGENCE);
    }

    if (NULL == pGBDeviceInfo)
    {
        /* �ظ���Ӧ,�齨��Ϣ */
        CPacket outPacket;
        DOMElement* AccNode = NULL;

        outPacket.SetRootTag("Response");

        AccNode = outPacket.CreateElement((char*)"CmdType");
        outPacket.SetElementValue(AccNode, (char*)"RecordInfo");

        AccNode = outPacket.CreateElement((char*)"SN");
        outPacket.SetElementValue(AccNode, strSN);

        AccNode = outPacket.CreateElement((char*)"DeviceID");
        outPacket.SetElementValue(AccNode, strDeviceID);

        AccNode = outPacket.CreateElement((char*)"Name");
        outPacket.SetElementValue(AccNode, pGBLogicDeviceInfo->device_name);

        AccNode = outPacket.CreateElement((char*)"SumNum");
        outPacket.SetElementValue(AccNode, (char*)"-1");

        AccNode = outPacket.CreateElement((char*)"ErrCode");
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_DEVICE_GET_DEVICE_INFO_ERROR);
        outPacket.SetElementValue(AccNode, strErrorCode);

        AccNode = outPacket.CreateElement((char*)"Reason");
        outPacket.SetElementValue(AccNode, (char*)"��ȡ�����豸��Ϣʧ��");

        RecordListAccNode = outPacket.CreateElement((char*)"RecordList");
        outPacket.SetElementAttr(RecordListAccNode, (char*)"Num", (char*)"-1");

        /* ������Ӧ��Ϣ ���ϼ�CMS */
        i = SIP_SendMessage(NULL, local_cms_id_get(), caller_id, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

        if (i != 0)
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_record_info_proc() SIP_SendMessage Error:caller_id=%s, caller_ip=%s, caller_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
        }
        else
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_record_info_proc() SIP_SendMessage OK:caller_id=%s, caller_ip=%s, caller_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
        }

        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_record_info_proc() exit---: Get GBLogic Device Info Error \r\n");
        SystemLog(EV9000_CMS_QUERY_RECORD_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�����Ĳ�ѯ¼���¼��Ϣʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, ��ѯ���߼��豸ID=%s, ԭ��=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, (char*)"��ȡ�����豸��Ϣʧ��");
        EnSystemLog(EV9000_CMS_QUERY_RECORD_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "Search for video record info from superior CMS failed:CMS ID=%s, IP address=%s, port number=%d, cause=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"Access physical device info failed");
        return -1;
    }

    /* �鿴�õ�λ�����Ƿ�������¼����������ˣ����ڱ��ز���¼���¼ */
    if (EV9000_RECORD_TYPE_NORMAL == record_type
        || EV9000_RECORD_TYPE_ALARM == record_type)
    {
        record_info_pos = record_info_find_by_stream_type(pGBLogicDeviceInfo->id, EV9000_STREAM_TYPE_MASTER);
        DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_query_record_info_proc() record_info_find_by_stream_type:device_id=%s, stream_type=%d, record_info_pos=%d \r\n", pGBLogicDeviceInfo->device_id, EV9000_STREAM_TYPE_MASTER, record_info_pos);
    }
    else if (EV9000_RECORD_TYPE_BACKUP == record_type)
    {
        record_info_pos = record_info_find_by_stream_type(pGBLogicDeviceInfo->id, EV9000_STREAM_TYPE_SLAVE);
        DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_query_record_info_proc() record_info_find_by_stream_type:device_id=%s, stream_type=%d, record_info_pos=%d \r\n", pGBLogicDeviceInfo->device_id, EV9000_STREAM_TYPE_SLAVE, record_info_pos);
    }
    else if (EV9000_RECORD_TYPE_INTELLIGENCE == record_type)
    {
        record_info_pos = record_info_find_by_stream_type(pGBLogicDeviceInfo->id, EV9000_STREAM_TYPE_INTELLIGENCE);
        DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_query_record_info_proc() record_info_find_by_stream_type:device_id=%s, stream_type=%d, record_info_pos=%d \r\n", pGBLogicDeviceInfo->device_id, EV9000_STREAM_TYPE_INTELLIGENCE, record_info_pos);
    }
    else
    {
        record_info_pos = record_info_find_by_stream_type(pGBLogicDeviceInfo->id, EV9000_STREAM_TYPE_MASTER);
        DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_query_record_info_proc() record_info_find_by_stream_type:device_id=%s, stream_type=%d, record_info_pos=%d \r\n", pGBLogicDeviceInfo->device_id, EV9000_STREAM_TYPE_MASTER, record_info_pos);
    }

    if (record_info_pos >= 0)
    {
        pRecordInfo = record_info_get(record_info_pos);

        if (NULL != pRecordInfo)
        {
            if (1 == pRecordInfo->record_enable)
            {
                goto normal_record;
            }
        }
    }

    if (pGBDeviceInfo->device_type == EV9000_DEVICETYPE_SIPSERVER) /* �¼�cms ��¼��  */
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS�����Ĳ�ѯ¼���¼��Ϣ���¼�CMS�еĵ�λ:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, ��ѯ���߼��豸ID=%s, ���ڵ��¼�CMS ID=%s, IP��ַ=%s, �˿ں�=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Superior CMS of query video information is lower in the CMS point: the superior CMS, ID = % s = % s IP address, port number = % d, query logic device ID = % s, the lower the CMS, ID = % s = % s IP address, port number = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);

        /* �����齨��Ϣ */
        CPacket outPacket;
        DOMElement* AccNode = NULL;

        outPacket.SetRootTag("Query");

        AccNode = outPacket.CreateElement((char*)"CmdType");
        outPacket.SetElementValue(AccNode, (char*)"RecordInfo");

        AccNode = outPacket.CreateElement((char*)"SN");
        g_transfer_xml_sn++;
        snprintf(strTransferSN, 32, "%u", g_transfer_xml_sn);
        outPacket.SetElementValue(AccNode, strTransferSN);

        AccNode = outPacket.CreateElement((char*)"DeviceID");
        outPacket.SetElementValue(AccNode, strDeviceID);

        AccNode = outPacket.CreateElement((char*)"StartTime");
        outPacket.SetElementValue(AccNode, strStartTime);

        AccNode = outPacket.CreateElement((char*)"EndTime");
        outPacket.SetElementValue(AccNode, strEndTime);

        AccNode = outPacket.CreateElement((char*)"Type");

        if (1 == pGBDeviceInfo->three_party_flag)
        {
            outPacket.SetElementValue(AccNode, (char*)"all");
        }
        else
        {
            if (strRecordType[0] != 0)
            {
                outPacket.SetElementValue(AccNode, strRecordType);
            }
            else
            {
                outPacket.SetElementValue(AccNode, (char*)"all");
            }
        }

        /* ������ƽ̨��ѯ¼���������ת���¼�ȥ���¼����Լ���ƽ̨�Ļ�����Ҫ���ϵ�������ʶ */
        if (1 == pRouteInfo->three_party_flag && 0 == pGBDeviceInfo->three_party_flag)
        {
            AccNode = outPacket.CreateElement((char*)"ThreeParty");
            outPacket.SetElementValue(AccNode, (char*)"YES");
        }

        AccNode = outPacket.CreateElement((char*)"RecorderID");
        outPacket.SetElementValue(AccNode, pGBDeviceInfo->device_id);

        /* ����ѯ¼���������Ϣת����ȥ */
        i = SIP_SendMessage(NULL, local_cms_id_get(), strDeviceID, pGBDeviceInfo->strRegServerIP, pGBDeviceInfo->iRegServerPort, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

        if (i != 0)
        {
            SystemLog(EV9000_CMS_QUERY_RECORD_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�����Ĳ�ѯ¼���¼��Ϣʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, ��ѯ���߼��豸ID=%s, ԭ��=%s, ת���¼�CMS ID=%s, IP��ַ=%s, �˿ں�=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, (char*)"ת����ѯ��Ϣ���¼�CMSʧ��", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
            EnSystemLog(EV9000_CMS_QUERY_RECORD_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "Search for video record info from superior CMS , forward search info to subordinate CMS failed:superior CMS ID=%s, IP address=%s, port number=%d, subordinate CMS ID=%s, IPaddress =%s, port number=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_record_info_proc() SIP_SendMessage Error:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        }
        else
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS�����Ĳ�ѯ¼���¼��Ϣ�ɹ�,ת����ѯ��Ϣ���¼�CMS:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, ��ѯ���߼��豸ID=%s, ת���¼�CMS ID=%s, IP��ַ=%s, �˿ں�=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Search for video record info from superior CMS , forward search info to subordinate CMS sucessfully:Superior CMS, ID = % s = % s IP address, port number = % d, query logic device ID = % s, forwarding the lower CMS, ID = % s = % s IP address, port number = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_record_info_proc() SIP_SendMessage OK:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);

            old_xml_sn = strtoul(strSN, NULL, 10);
            transfer_xml_sn = strtoul(strTransferSN, NULL, 10);
            i = transfer_xml_msg_add(XML_QUERY_RECORDINFO, old_xml_sn, transfer_xml_sn, pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, strDeviceID);
            DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_query_record_info_proc() transfer_xml_msg_add: Type=%d, old_xml_sn=%u, transfer_xml_sn=%u, DeviceID=%s, pos=%d \r\n", XML_QUERY_RECORDINFO, old_xml_sn, transfer_xml_sn, strDeviceID, i);
        }

        return i;
    }
    else if (1 == pGBLogicDeviceInfo->record_type) /* ǰ��¼�������£�ֱ��ת����ǰ�˴��� */
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS�����Ĳ�ѯ¼���¼��Ϣ�ĵ�λ����ǰ��¼��,�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, ��ѯ���߼��豸ID=%s, ���ڵ�ǰ���豸ID=%s, IP��ַ=%s, �˿ں�=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Search for video record info from superior CMS is on the front end video, the higher the CMS, ID = % s = % s IP address and port number = % d, query logic device ID = % s, where the front-end device ID = % s, = % s IP address and port number = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);

        /* �����齨��Ϣ */
        CPacket outPacket;
        DOMElement* AccNode = NULL;

        outPacket.SetRootTag("Query");

        AccNode = outPacket.CreateElement((char*)"CmdType");
        outPacket.SetElementValue(AccNode, (char*)"RecordInfo");

        AccNode = outPacket.CreateElement((char*)"SN");
        g_transfer_xml_sn++;
        snprintf(strTransferSN, 32, "%u", g_transfer_xml_sn);
        outPacket.SetElementValue(AccNode, strTransferSN);

        AccNode = outPacket.CreateElement((char*)"DeviceID");
        outPacket.SetElementValue(AccNode, strDeviceID);

        AccNode = outPacket.CreateElement((char*)"StartTime");
        outPacket.SetElementValue(AccNode, strStartTime);

        AccNode = outPacket.CreateElement((char*)"EndTime");
        outPacket.SetElementValue(AccNode, strEndTime);

        AccNode = outPacket.CreateElement((char*)"Type");

        /* �����NVR����DVR,��Ҫ��Ϊall */
        if (EV9000_DEVICETYPE_IPC == pGBDeviceInfo->device_type
            || EV9000_DEVICETYPE_DVR == pGBDeviceInfo->device_type
            || EV9000_DEVICETYPE_NVR == pGBDeviceInfo->device_type)
        {
            outPacket.SetElementValue(AccNode, (char*)"all");
        }
        else
        {
            if (strRecordType[0] != 0)
            {
                outPacket.SetElementValue(AccNode, strRecordType);
            }
            else
            {
                outPacket.SetElementValue(AccNode, (char*)"all");
            }
        }

        /* ������ƽ̨��ѯ¼���������ת��ǰ��ý������ȥ��ǰ��ý���������Լ��Ļ�����Ҫ���ϵ�������ʶ */
        if (EV9000_DEVICETYPE_MGWSERVER == pGBDeviceInfo->device_type)
        {
            if (1 == pRouteInfo->three_party_flag && 0 == pGBDeviceInfo->three_party_flag)
            {
                AccNode = outPacket.CreateElement((char*)"ThreeParty");
                outPacket.SetElementValue(AccNode, (char*)"YES");
            }
        }

        AccNode = outPacket.CreateElement((char*)"RecorderID");
        outPacket.SetElementValue(AccNode, pGBDeviceInfo->device_id);

        /* ����ѯ¼���������Ϣת����ȥ */
        i = SIP_SendMessage(NULL, local_cms_id_get(), strDeviceID, pGBDeviceInfo->strRegServerIP, pGBDeviceInfo->iRegServerPort, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

        if (i != 0)
        {
            SystemLog(EV9000_CMS_QUERY_RECORD_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�����Ĳ�ѯ¼���¼��Ϣʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, ��ѯ���߼��豸ID=%s, ԭ��=%s, ǰ���豸ID=%s, IP��ַ=%s, �˿ں�=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, (char*)"ת����ѯ��Ϣ��ǰ���豸ʧ��", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
            EnSystemLog(EV9000_CMS_QUERY_RECORD_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "Search for video record info from superior CMS , forward search info to front-end device failed:superior CMS ID=%s, IP address=%s, port number=%d, front-end numberID=%s, IP address=%s, port number=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_record_info_proc() SIP_SendMessage Error:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        }
        else
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS�����Ĳ�ѯ¼���¼��Ϣ�ɹ�,ת����ѯ��Ϣ��ǰ���豸:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, ��ѯ���߼��豸ID=%s, ǰ���豸ID=%s, IP��ַ=%s, �˿ں�=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Search for video record info from superior CMS , forward search info to front-end device successfully:Superior CMS, ID = % s = % s IP address, port number = % d, query logic device ID = % s, front-end device ID = % s, IP address = % s, port = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_record_info_proc() SIP_SendMessage OK:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);

            old_xml_sn = strtoul(strSN, NULL, 10);
            transfer_xml_sn = strtoul(strTransferSN, NULL, 10);
            i = transfer_xml_msg_add(XML_QUERY_RECORDINFO, old_xml_sn, transfer_xml_sn, pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, strDeviceID);
            DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_query_record_info_proc() transfer_xml_msg_add: Type=%d, old_xml_sn=%u, transfer_xml_sn=%u, DeviceID=%s, pos=%d \r\n", XML_QUERY_RECORDINFO, old_xml_sn, transfer_xml_sn, strDeviceID, i);
        }

        return i;
    }

normal_record:
    {
        /* 2������¼��ط�TSU ��Դ */
        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_record_info_proc() get_idle_tsu_by_resource_balance_for_replay: Begin--- \r\n", tsu_index);
        tsu_index = get_idle_tsu_by_resource_balance_for_replay();
        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_record_info_proc() get_idle_tsu_by_resource_balance_for_replay: End--- tsu_index=%d \r\n", tsu_index);

        if (tsu_index < 0)
        {
            /* �ظ���Ӧ,�齨��Ϣ */
            CPacket outPacket;
            DOMElement* AccNode = NULL;

            outPacket.SetRootTag("Response");

            AccNode = outPacket.CreateElement((char*)"CmdType");
            outPacket.SetElementValue(AccNode, (char*)"RecordInfo");

            AccNode = outPacket.CreateElement((char*)"SN");
            outPacket.SetElementValue(AccNode, strSN);

            AccNode = outPacket.CreateElement((char*)"DeviceID");
            outPacket.SetElementValue(AccNode, strDeviceID);

            AccNode = outPacket.CreateElement((char*)"Name");
            outPacket.SetElementValue(AccNode, pGBLogicDeviceInfo->device_name);

            AccNode = outPacket.CreateElement((char*)"SumNum");
            outPacket.SetElementValue(AccNode, (char*)"-1");

            AccNode = outPacket.CreateElement((char*)"ErrCode");
            memset(strErrorCode, 0, 32);
            snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_TSU_GET_IDLE_TSU_INDEX_ERROR);
            outPacket.SetElementValue(AccNode, strErrorCode);

            AccNode = outPacket.CreateElement((char*)"Reason");
            outPacket.SetElementValue(AccNode, (char*)"��ȡ���õ�TSU����ʧ��");

            RecordListAccNode = outPacket.CreateElement((char*)"RecordList");
            outPacket.SetElementAttr(RecordListAccNode, (char*)"Num", (char*)"-1");

            /* ������Ӧ��Ϣ ���ϼ�CMS */
            i = SIP_SendMessage(NULL, local_cms_id_get(), caller_id, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

            if (i != 0)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_record_info_proc() SIP_SendMessage Error:caller_id=%s, caller_ip=%s, caller_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
            }
            else
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_record_info_proc() SIP_SendMessage OK:caller_id=%s, caller_ip=%s, caller_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
            }

            if (-2 == tsu_index)
            {
                SystemLog(EV9000_CMS_QUERY_RECORD_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�����Ĳ�ѯ¼���¼��Ϣʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, ��ѯ���߼��豸ID=%s, ԭ��=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, (char*)"���ҿ��õ�¼��ط�TSU����ʧ��,TSU��Դ����Ϊ��");
                EnSystemLog(EV9000_CMS_QUERY_RECORD_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "Search for video record info from superior CMS failed:CMS ID=%s, IP address=%s, port number=%d, cause=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"access available TSU index failed");
            }
            else if (-3 == tsu_index)
            {
                SystemLog(EV9000_CMS_QUERY_RECORD_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�����Ĳ�ѯ¼���¼��Ϣʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, ��ѯ���߼��豸ID=%s, ԭ��=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, (char*)"���ҿ��õ�¼��ط�TSU����ʧ��,TSU��Դ��Ϣ����");
                EnSystemLog(EV9000_CMS_QUERY_RECORD_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "Search for video record info from superior CMS failed:CMS ID=%s, IP address=%s, port number=%d, cause=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"access available TSU index failed");
            }
            else if (-4 == tsu_index)
            {
                SystemLog(EV9000_CMS_QUERY_RECORD_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�����Ĳ�ѯ¼���¼��Ϣʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, ��ѯ���߼��豸ID=%s, ԭ��=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, (char*)"���ҿ��õ�¼��ط�TSU����ʧ��,TSU��Դ��û������");
                EnSystemLog(EV9000_CMS_QUERY_RECORD_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "Search for video record info from superior CMS failed:CMS ID=%s, IP address=%s, port number=%d, cause=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"access available TSU index failed");
            }
            else if (-5 == tsu_index)
            {
                SystemLog(EV9000_CMS_QUERY_RECORD_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�����Ĳ�ѯ¼���¼��Ϣʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, ��ѯ���߼��豸ID=%s, ԭ��=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, (char*)"���ҿ��õ�¼��ط�TSU����ʧ��,TSU��Դ��������");
                EnSystemLog(EV9000_CMS_QUERY_RECORD_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "Search for video record info from superior CMS failed:CMS ID=%s, IP address=%s, port number=%d, cause=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"access available TSU index failed");
            }
            else if (-9 == tsu_index)
            {
                SystemLog(EV9000_CMS_QUERY_RECORD_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�����Ĳ�ѯ¼���¼��Ϣʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, ��ѯ���߼��豸ID=%s, ԭ��=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, (char*)"���ҿ��õ�¼��ط�TSU����ʧ��,ͨ��ICE��ȡ���е�TSU��Դ״̬ʧ��");
                EnSystemLog(EV9000_CMS_QUERY_RECORD_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "Search for video record info from superior CMS failed:CMS ID=%s, IP address=%s, port number=%d, cause=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"access available TSU index failed");
            }
            else
            {
                SystemLog(EV9000_CMS_QUERY_RECORD_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�����Ĳ�ѯ¼���¼��Ϣʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, ��ѯ���߼��豸ID=%s, ԭ��=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, (char*)"���ҿ��õ�¼��ط�TSU����ʧ��");
                EnSystemLog(EV9000_CMS_QUERY_RECORD_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "Search for video record info from superior CMS failed:CMS ID=%s, IP address=%s, port number=%d, cause=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"access available TSU index failed");
            }

            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_record_info_proc() exit---: Get Idle TSU Index Error \r\n");
            return -1;
        }

        pTsuResourceInfo = tsu_resource_info_get(tsu_index);

        if (NULL == pTsuResourceInfo)
        {
            /* �ظ���Ӧ,�齨��Ϣ */
            CPacket outPacket;
            DOMElement* AccNode = NULL;

            outPacket.SetRootTag("Response");

            AccNode = outPacket.CreateElement((char*)"CmdType");
            outPacket.SetElementValue(AccNode, (char*)"RecordInfo");

            AccNode = outPacket.CreateElement((char*)"SN");
            outPacket.SetElementValue(AccNode, strSN);

            AccNode = outPacket.CreateElement((char*)"DeviceID");
            outPacket.SetElementValue(AccNode, strDeviceID);

            AccNode = outPacket.CreateElement((char*)"Name");
            outPacket.SetElementValue(AccNode, pGBLogicDeviceInfo->device_name);

            AccNode = outPacket.CreateElement((char*)"SumNum");
            outPacket.SetElementValue(AccNode, (char*)"-1");

            AccNode = outPacket.CreateElement((char*)"ErrCode");
            memset(strErrorCode, 0, 32);
            snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_TSU_GET_TSU_INFO_ERROR);
            outPacket.SetElementValue(AccNode, strErrorCode);

            AccNode = outPacket.CreateElement((char*)"Reason");
            outPacket.SetElementValue(AccNode, (char*)"��ȡ���õ�TSU��Ϣʧ��");

            RecordListAccNode = outPacket.CreateElement((char*)"RecordList");
            outPacket.SetElementAttr(RecordListAccNode, (char*)"Num", (char*)"-1");

            /* ������Ӧ��Ϣ ���ϼ�CMS */
            i = SIP_SendMessage(NULL, local_cms_id_get(), caller_id, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

            if (i != 0)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_record_info_proc() SIP_SendMessage Error:caller_id=%s, caller_ip=%s, caller_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
            }
            else
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_record_info_proc() SIP_SendMessage OK:caller_id=%s, caller_ip=%s, caller_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
            }

            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_record_info_proc() exit---: Get TSU Resource Info Error \r\n");
            SystemLog(EV9000_CMS_QUERY_RECORD_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�����Ĳ�ѯ¼���¼��Ϣʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, ��ѯ���߼��豸ID=%s, ԭ��=%s, tsu_index=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, (char*)"��ȡ¼��طŵ�TSU��Ϣʧ��", tsu_index);
            EnSystemLog(EV9000_CMS_QUERY_RECORD_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "Search for video record info from superior CMS failed:CMS ID=%s, IPaddress=%s, port number=%d, cause=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"access available TSU info failed");
            return -1;
        }

        /* 3��֪ͨTSU, ��ѯ¼���б� */
        iStartTime = analysis_time(strStartTime);
        iEndTime = analysis_time(strEndTime);

        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_record_info_proc() iStartTime=%d,iEndTime=%d \r\n", iStartTime, iEndTime);

        if ((iStartTime <= 0) || (iEndTime <= 0))
        {
            /* �ظ���Ӧ,�齨��Ϣ */
            CPacket outPacket;
            DOMElement* AccNode = NULL;

            outPacket.SetRootTag("Response");

            AccNode = outPacket.CreateElement((char*)"CmdType");
            outPacket.SetElementValue(AccNode, (char*)"RecordInfo");

            AccNode = outPacket.CreateElement((char*)"SN");
            outPacket.SetElementValue(AccNode, strSN);

            AccNode = outPacket.CreateElement((char*)"DeviceID");
            outPacket.SetElementValue(AccNode, strDeviceID);

            AccNode = outPacket.CreateElement((char*)"Name");
            outPacket.SetElementValue(AccNode, pGBLogicDeviceInfo->device_name);

            AccNode = outPacket.CreateElement((char*)"SumNum");
            outPacket.SetElementValue(AccNode, (char*)"-1");

            AccNode = outPacket.CreateElement((char*)"ErrCode");
            memset(strErrorCode, 0, 32);
            snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_RECORD_TIME_INFO_ERROR);
            outPacket.SetElementValue(AccNode, strErrorCode);

            AccNode = outPacket.CreateElement((char*)"Reason");
            outPacket.SetElementValue(AccNode, (char*)"¼��ʱ����Ϣ����");

            RecordListAccNode = outPacket.CreateElement((char*)"RecordList");
            outPacket.SetElementAttr(RecordListAccNode, (char*)"Num", (char*)"-1");

            /* ������Ӧ��Ϣ ���ϼ�CMS */
            i = SIP_SendMessage(NULL, local_cms_id_get(), caller_id, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

            if (i != 0)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_record_info_proc() SIP_SendMessage Error:caller_id=%s, caller_ip=%s, caller_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
            }
            else
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_record_info_proc() SIP_SendMessage OK:caller_id=%s, caller_ip=%s, caller_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
            }

            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_record_info_proc() exit---: Get Time Error \r\n");
            SystemLog(EV9000_CMS_QUERY_RECORD_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�����Ĳ�ѯ¼���¼��Ϣʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, ��ѯ���߼��豸ID=%s, ԭ��=%s,��ʼʱ��=%s, ����ʱ��=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, (char*)"¼��ʱ����Ϣ����", strStartTime, strEndTime);
            EnSystemLog(EV9000_CMS_QUERY_RECORD_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "Search for video record info from superior CMS failed:CMS ID=%s, IPaddress=%s, port number=%d, cause=%s,start time=%s, end time=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"record time incorrect", strStartTime, strEndTime);
            return -1;
        }

        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS�����Ĳ�ѯ¼���¼��Ϣ, ��ʼ֪ͨTSU��ѯ��¼:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, ��ѯ���߼��豸ID=%s, TSU ID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, pTsuResourceInfo->tsu_device_id);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Search for video record info from superior CMS , start to notify TSU search record:CMS ID=%s, IPaddress=%s, port number=%d, TSU ID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, pTsuResourceInfo->tsu_device_id);

        /* ���Ҹõ�λ��¼������֪ͨTSU���½���ʱ�� */
        if (EV9000_RECORD_TYPE_NORMAL == record_type
            || EV9000_RECORD_TYPE_ALARM == record_type)
        {
            iRecordCRPos = record_call_record_find_by_calleeid_and_streamtype(strDeviceID, EV9000_STREAM_TYPE_MASTER);
        }
        else if (EV9000_RECORD_TYPE_BACKUP == record_type)
        {
            iRecordCRPos = record_call_record_find_by_calleeid_and_streamtype(strDeviceID, EV9000_STREAM_TYPE_SLAVE);
        }
        else if (EV9000_RECORD_TYPE_INTELLIGENCE == record_type)
        {
            iRecordCRPos = record_call_record_find_by_calleeid_and_streamtype(strDeviceID, EV9000_STREAM_TYPE_INTELLIGENCE);
        }

        DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_query_record_info_proc():record_call_record_find_by_calleeid_and_streamtype:callee_id=%s, RecordCRPos=%d\r\n", strDeviceID, iRecordCRPos);

        if (iRecordCRPos >= 0)
        {
            pRecordCrData = call_record_get(iRecordCRPos);

            if (NULL != pRecordCrData)
            {
                i = notify_tsu_update_mysql_record_stoptime(pRecordCrData->tsu_ip, pRecordCrData->task_id);
                DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_query_record_info_proc():notify_tsu_update_mysql_record_stoptime:tsu_ip=%s, task_id=%s, i=%d \r\n", pRecordCrData->tsu_ip, pRecordCrData->task_id, i);
            }
            else
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_record_info_proc() exit---: call_record_get Error: RecordCRPos=%d \r\n", iRecordCRPos);
            }
        }
        else
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_record_info_proc() exit---: record_call_record_find_by_calleeid_and_streamtype Error: callee_id=%s \r\n", strDeviceID);
        }

        /* ֪ͨTSU��ѯ¼�� */
        iRet = notify_tsu_query_replay_list(pTsuResourceInfo, strDeviceID, 1, record_type, iStartTime, iEndTime, stVideoRecordList);

        if (iRet < 0)
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_record_info_proc() notify_tsu_query_replay_list Error:i=%d \r\n", iRet);
        }
        else
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_record_info_proc() notify_tsu_query_replay_list OK:i=%d \r\n", iRet);
        }

        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS�����Ĳ�ѯ¼���¼��Ϣ, TSU��ѯ��¼����:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, ��ѯ���߼��豸ID=%s, TSU ID=%s, i=%d, ��¼��=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, pTsuResourceInfo->tsu_device_id, i, stVideoRecordList.size());
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Search for video record info from superior CMS , TSU search record result:CMS ID=%s, IPaddress=%s, port number=%d, TSU ID=%s, i=%d, record number=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, pTsuResourceInfo->tsu_device_id, i, stVideoRecordList.size());

        if (iRet < 0)
        {
            /* �ظ���Ӧ,�齨��Ϣ */
            CPacket outPacket;
            DOMElement* AccNode = NULL;

            outPacket.SetRootTag("Response");

            AccNode = outPacket.CreateElement((char*)"CmdType");
            outPacket.SetElementValue(AccNode, (char*)"RecordInfo");

            AccNode = outPacket.CreateElement((char*)"SN");
            outPacket.SetElementValue(AccNode, strSN);

            AccNode = outPacket.CreateElement((char*)"DeviceID");
            outPacket.SetElementValue(AccNode, strDeviceID);

            AccNode = outPacket.CreateElement((char*)"Name");
            outPacket.SetElementValue(AccNode, pGBLogicDeviceInfo->device_name);

            AccNode = outPacket.CreateElement((char*)"SumNum");
            outPacket.SetElementValue(AccNode, (char*)"-1");

            AccNode = outPacket.CreateElement((char*)"ErrCode");
            memset(strErrorCode, 0, 32);

            if (-1 == iRet)
            {
                snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_TSU_ICE_ERROR);
            }
            else
            {
                snprintf(strErrorCode, 32, "%d", iRet);
            }

            outPacket.SetElementValue(AccNode, strErrorCode);

            AccNode = outPacket.CreateElement((char*)"Reason");
            outPacket.SetElementValue(AccNode, (char*)"TSU��ѯ¼���¼����ʧ��,ICE�쳣");

            RecordListAccNode = outPacket.CreateElement((char*)"RecordList");
            outPacket.SetElementAttr(RecordListAccNode, (char*)"Num", (char*)"-1");

            /* ������Ӧ��Ϣ ���ϼ�CMS */
            i = SIP_SendMessage(NULL, local_cms_id_get(), caller_id, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

            if (i != 0)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_record_info_proc() SIP_SendMessage Error:caller_id=%s, caller_ip=%s, caller_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
            }
            else
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_record_info_proc() SIP_SendMessage OK:caller_id=%s, caller_ip=%s, caller_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
            }

            if (-1 == iRet)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_record_info_proc() exit---: Notify TSU Query Replay List Error \r\n");
                SystemLog(EV9000_CMS_QUERY_RECORD_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�����Ĳ�ѯ¼���¼��Ϣʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, ��ѯ���߼��豸ID=%s, ԭ��=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, (char*)"TSU��ѯ¼���¼����ʧ��, ICE�쳣");
                EnSystemLog(EV9000_CMS_QUERY_RECORD_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "Search for video record info from superior CMS failed:CMS ID=%s, IP address=%s, port number=%d, cause=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"TSU search for video record return failed, ICE abnormal");
            }
            else
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_record_info_proc() exit---: Notify TSU Query Replay List Error \r\n");
                SystemLog(EV9000_CMS_QUERY_RECORD_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�����Ĳ�ѯ¼���¼��Ϣʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, ��ѯ���߼��豸ID=%s, ԭ��=%s, iRet=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, (char*)"TSU��ѯ¼���¼����ʧ��", iRet);
                EnSystemLog(EV9000_CMS_QUERY_RECORD_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "Search for video record info from superior CMS failed:CMS ID=%s, IP address=%s, port number=%d, cause=%s, iRet=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"TSUTSU search for video record return failed", iRet);
            }

            return -1;
        }

        record_count = stVideoRecordList.size();
        DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_query_record_info_proc() record_count=%d \r\n", record_count);

        if (record_count == 0)
        {
            /* �ظ���Ӧ,�齨��Ϣ */
            CPacket outPacket;
            DOMElement* AccNode = NULL;

            outPacket.SetRootTag("Response");

            AccNode = outPacket.CreateElement((char*)"CmdType");
            outPacket.SetElementValue(AccNode, (char*)"RecordInfo");

            AccNode = outPacket.CreateElement((char*)"SN");
            outPacket.SetElementValue(AccNode, strSN);

            AccNode = outPacket.CreateElement((char*)"DeviceID");
            outPacket.SetElementValue(AccNode, strDeviceID);

            AccNode = outPacket.CreateElement((char*)"Name");
            outPacket.SetElementValue(AccNode, pGBLogicDeviceInfo->device_name);

            AccNode = outPacket.CreateElement((char*)"SumNum");
            outPacket.SetElementValue(AccNode, (char*)"0");

            RecordListAccNode = outPacket.CreateElement((char*)"RecordList");
            outPacket.SetElementAttr(RecordListAccNode, (char*)"Num", (char*)"0");

            /* ������Ӧ��Ϣ ���ϼ�CMS */
            i = SIP_SendMessage(NULL, local_cms_id_get(), caller_id, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

            if (i != 0)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_record_info_proc() SIP_SendMessage Error:caller_id=%s, caller_ip=%s, caller_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
            }
            else
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_record_info_proc() SIP_SendMessage OK:caller_id=%s, caller_ip=%s, caller_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
            }

            SystemLog(EV9000_CMS_QUERY_RECORD_INFO_ERROR, EV9000_LOG_LEVEL_WARNING, "�ϼ�CMS�����Ĳ�ѯ¼���¼��Ϣʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, ��ѯ���߼��豸ID=%s, ԭ��=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, (char*)"û�в�ѯ��¼���¼");
            EnSystemLog(EV9000_CMS_QUERY_RECORD_INFO_ERROR, EV9000_LOG_LEVEL_WARNING, "Search for video record info from superior CMS failed:CMS ID=%s, IP address=%s, port number=%d, cause=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"Video record not found.");
            DEBUG_TRACE(MODULE_ROUTE, LOG_WARN, "route_query_record_info_proc() exit---: No Record Count \r\n");
            return i;
        }

        /* 4��ѭ��������������ȡ¼���ļ��б���Ϣ������xml�� */
        CPacket* pOutPacket = NULL;

        if ((str3PartFlag[0] != 0 && 0 == sstrcmp(str3PartFlag, (char*)"YES"))
            || (1 == pRouteInfo->three_party_flag)) /* ������ƽ̨,�������������� */
        {
            for (index = 0; index < record_count; index++)
            {
                /* �����¼������10����Ҫ�ִη��� */
                query_count++;

                /* ����XMLͷ�� */
                i = CreateRecordInfoQueryResponseXMLHeadForRoute(&pOutPacket, query_count, record_count, strSN, strDeviceID, pGBLogicDeviceInfo->device_name, &RecordListAccNode);

                /* ����Item ֵ */
                i = AddRecordInfoToXMLItemForRoute(pOutPacket, RecordListAccNode, stVideoRecordList[index], strDeviceID, pGBLogicDeviceInfo->device_name);

                if ((query_count % MAX_ROUTE_RECORD_INFO_COUT_SEND == 0) || (query_count == record_count))
                {
                    if (NULL != pOutPacket)
                    {
                        /* ������Ӧ��Ϣ ���ϼ�CMS */
                        i |= SIP_SendMessage(NULL, local_cms_id_get(), caller_id, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)pOutPacket->GetXml(NULL).c_str(), pOutPacket->GetXml(NULL).length());

                        if (i != 0)
                        {
                            SystemLog(EV9000_CMS_QUERY_RECORD_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�����Ĳ�ѯ¼���¼��Ϣ, ����Message��Ϣ���ϼ�CMSʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                            EnSystemLog(EV9000_CMS_QUERY_RECORD_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "Superior CMS video query information, Message sending messages to the superior CMS failure: the higher the CMS, ID = % s = % s IP address, port number = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_record_info_proc() SIP_SendMessage Error:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                        }
                        else
                        {
                            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS�����Ĳ�ѯ¼���¼��Ϣ, ����Message��Ϣ���ϼ�CMS�ɹ�:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Superior CMS video query information, Message sending messages to the superior CMS success: the higher the CMS, ID = % s = % s IP address, port number = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_record_info_proc() SIP_SendMessage OK:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                        }

                        delete pOutPacket;
                        pOutPacket = NULL;
                    }
                }
            }
        }
        else /* ������Լ�ƽ̨�����շ��͸��û����������� */
        {
            for (index = 0; index < record_count; index++)
            {
                /* �����¼������10����Ҫ�ִη��� */
                query_count++;

                /* ����XMLͷ�� */
                i = CreateRecordInfoQueryResponseXMLHead(&pOutPacket, query_count, record_count, strSN, strDeviceID, pGBLogicDeviceInfo->device_name, &RecordListAccNode);

                /* ����Item ֵ */
                i = AddRecordInfoToXMLItem(pOutPacket, RecordListAccNode, stVideoRecordList[index], strDeviceID, pGBLogicDeviceInfo->device_name);

                if ((query_count % MAX_USER_RECORD_INFO_COUT_SEND == 0) || (query_count == record_count))
                {
                    if (NULL != pOutPacket)
                    {
                        /* ������Ӧ��Ϣ ���ϼ�CMS */
                        i |= SIP_SendMessage(NULL, local_cms_id_get(), caller_id, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)pOutPacket->GetXml(NULL).c_str(), pOutPacket->GetXml(NULL).length());

                        if (i != 0)
                        {
                            SystemLog(EV9000_CMS_QUERY_RECORD_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�����Ĳ�ѯ¼���¼��Ϣ, ����Message��Ϣ���ϼ�CMSʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                            EnSystemLog(EV9000_CMS_QUERY_RECORD_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "Superior CMS video query information, Message sending messages to the superior CMS failure: the higher the CMS, ID = % s = % s IP address, port number = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_record_info_proc() SIP_SendMessage Error:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                        }
                        else
                        {
                            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS�����Ĳ�ѯ¼���¼��Ϣ, ����Message��Ϣ���ϼ�CMS�ɹ�:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Superior CMS video query information, Message sending messages to the superior CMS success: the higher the CMS, ID = % s = % s IP address, port number = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_record_info_proc() SIP_SendMessage OK:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                        }

                        delete pOutPacket;
                        pOutPacket = NULL;
                    }
                }
            }
        }
    }

    if (i == 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS�����Ĳ�ѯ¼���¼��Ϣ�ɹ�:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, ��ѯ���߼��豸ID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Superior CMS video query information of success: the superior CMS, ID = % s = % s IP address, port number = % d, query logic device ID = % s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID);
    }
    else
    {
        SystemLog(EV9000_CMS_QUERY_RECORD_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�����Ĳ�ѯ¼���¼��Ϣ�ɹ�:�ϼ�ID=%s, IP��ַ=%s, �˿ں�=%d, ԭ��=%s, ��ѯ���߼��豸ID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, (char*)"����SIP��Ӧ��Ϣʧ��");
        EnSystemLog(EV9000_CMS_QUERY_RECORD_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "Superior CMS video query information failure: superior, ID = % s = % s IP address, port number = % d, reason = % s, query logic device ID = % s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, (char*)"SIP response message sent failure");
    }

    return 0;
}

/*****************************************************************************
 �� �� ��  : route_query_preset_info_proc
 ��������  : �ϼ�����CMS���͹����Ĳ�ѯ�豸Ԥ��λ��Ϣ����
 �������  : route_info_t* pRouteInfo
             char* caller_id
             char* callee_id
             CPacket& inPacket
             DBOper* pRoute_Srv_dboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��3��15�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int route_query_preset_info_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, CPacket& inPacket, DBOper* pRoute_Srv_dboper)
{
    int i = 0;
    int record_count = 0; /* ��¼�� */
    int send_count = 0;   /* ���͵Ĵ��� */
    int query_count = 0;  /* ��ѯ����ͳ�� */

    char strSN[32] = {0};
    char strDeviceID[32] = {0};
    GBLogicDevice_info_t* pGBLogicDeviceInfo = NULL;
    GBDevice_info_t* pGBDeviceInfo = NULL;
    int while_count = 0;
    char strTransferSN[32] = {0};
    DOMElement* AccSnNode = NULL;
    unsigned int old_xml_sn = 0;
    unsigned int transfer_xml_sn = 0;

    int iCalleeRoutePos = 0;
    route_info_t* pCalleeRouteInfo = NULL;

    if (NULL == pRouteInfo)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_query_preset_info_proc() exit---: Route Info Error \r\n");
        return -1;
    }

    if (NULL == caller_id || NULL == callee_id || NULL == pRoute_Srv_dboper)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_query_preset_info_proc() exit---: para Error \r\n");
        return -1;
    }

    /* ȡ�ò�ѯ��������*/
    inPacket.GetElementValue((char*)"SN", strSN);
    inPacket.GetElementValue((char*)"DeviceID", strDeviceID);

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS�����Ĳ�ѯԤ��λ��Ϣ:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, ��ѯ���߼��豸ID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Search for preset info from superior CMS :CMS ID=%s, IP address=%s, port=%d, logic deviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID);

    /* 1����ȡ¼���λ��Ϣ */
    pGBLogicDeviceInfo = GBLogicDevice_info_find(strDeviceID);

    if (NULL == pGBLogicDeviceInfo)
    {
        /* �ظ���Ӧ,�齨��Ϣ */
        CPacket* pOutPacket = NULL;
        DOMElement* AccNode = NULL;
        DOMElement* ListAccNode = NULL;

        //DOMElement* ItemAccNode = NULL;
        if (!pOutPacket)
        {
            pOutPacket = new CPacket();
        }

        pOutPacket->SetRootTag("Response");
        AccNode = pOutPacket->CreateElement((char*)"CmdType");
        pOutPacket->SetElementValue(AccNode, (char*)"PresetConfig");

        AccNode = pOutPacket->CreateElement((char*)"SN");
        pOutPacket->SetElementValue(AccNode, strSN);

        AccNode = pOutPacket->CreateElement((char*)"DeviceID");
        pOutPacket->SetElementValue(AccNode, strDeviceID);

        AccNode = pOutPacket->CreateElement((char*)"SumNum");
        pOutPacket->SetElementValue(AccNode, (char*)"0");

        ListAccNode = pOutPacket->CreateElement((char*)"PresetConfigList");
        pOutPacket->SetElementAttr(ListAccNode, (char*)"Num", (char*)"0");

        /* ������Ӧ��Ϣ ���ϼ�CMS */
        i = SIP_SendMessage(NULL, local_cms_id_get(), caller_id, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)pOutPacket->GetXml(NULL).c_str(), pOutPacket->GetXml(NULL).length());

        if (i != 0)
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_preset_info_proc() SIP_SendMessage Error:caller_id=%s, caller_ip=%s, caller_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
        }
        else
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_preset_info_proc() SIP_SendMessage OK:caller_id=%s, caller_ip=%s, caller_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
        }

        if (pOutPacket)
        {
            delete pOutPacket;
        }

        pOutPacket = NULL;
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_preset_info_proc() exit---: Get GBLogic Device Info Error \r\n");
        SystemLog(EV9000_CMS_GET_PRESET_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�����Ĳ�ѯԤ��λ��Ϣʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, ��ѯ���߼��豸ID=%s, ԭ��=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, (char*)"��ȡ�߼��豸��Ϣʧ��");
        EnSystemLog(EV9000_CMS_GET_PRESET_ERROR, EV9000_LOG_LEVEL_ERROR, "Search for preset info from superior CMS failed:CMS ID=%s, IP address=%s, port=%d, cause=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"Access logical device information failed");
        return -1;
    }

    /* �����߼��豸����������жϣ�������Ϣ���� */
    if (1 == pGBLogicDeviceInfo->other_realm)
    {
        /* �����ϼ�·����Ϣ */
        iCalleeRoutePos = route_info_find(pGBLogicDeviceInfo->cms_id);

        if (iCalleeRoutePos < 0)
        {
            /* �ظ���Ӧ,�齨��Ϣ */
            CPacket* pOutPacket = NULL;
            DOMElement* AccNode = NULL;
            DOMElement* ListAccNode = NULL;

            //DOMElement* ItemAccNode = NULL;
            if (!pOutPacket)
            {
                pOutPacket = new CPacket();
            }

            pOutPacket->SetRootTag("Response");

            AccNode = pOutPacket->CreateElement((char*)"CmdType");
            pOutPacket->SetElementValue(AccNode, (char*)"PresetConfig");

            AccNode = pOutPacket->CreateElement((char*)"SN");
            pOutPacket->SetElementValue(AccNode, strSN);

            AccNode = pOutPacket->CreateElement((char*)"DeviceID");
            pOutPacket->SetElementValue(AccNode, strDeviceID);

            AccNode = pOutPacket->CreateElement((char*)"SumNum");
            pOutPacket->SetElementValue(AccNode, (char*)"0");

            ListAccNode = pOutPacket->CreateElement((char*)"PresetConfigList");
            pOutPacket->SetElementAttr(ListAccNode, (char*)"Num", (char*)"0");

            /* ������Ӧ��Ϣ */
            i = SIP_SendMessage(NULL, local_cms_id_get(), caller_id, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)pOutPacket->GetXml(NULL).c_str(), pOutPacket->GetXml(NULL).length());

            if (i != 0)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_preset_info_proc() SIP_SendMessage Error:CMS ID=%s, IP address=%s, port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
            }
            else
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_preset_info_proc() SIP_SendMessage OK:CMS ID=%s, IP address=%s, port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
            }

            if (pOutPacket)
            {
                delete pOutPacket;
            }

            pOutPacket = NULL;
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_preset_info_proc() exit---: Find Callee Route Info Error \r\n");
            SystemLog(EV9000_CMS_GET_PRESET_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�����Ĳ�ѯԤ��λ��Ϣʧ��:CMD ID=%s, IP��ַ=%s, �˿ں�=%d, ��ѯ���߼��豸ID=%s, ԭ��=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, (char*)"�����߼��豸��Ӧ���ϼ�·����Ϣʧ��");
            EnSystemLog(EV9000_CMS_GET_PRESET_ERROR, EV9000_LOG_LEVEL_ERROR, "Search for preset info from superior CMS failed:CMS ID=%s, IP address=%s, port=%d, reason=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"Find the route information which corresponding to logical device failed.");
            return -1;
        }

        pCalleeRouteInfo = route_info_get(iCalleeRoutePos);

        if (NULL == pCalleeRouteInfo)
        {
            /* �ظ���Ӧ,�齨��Ϣ */
            CPacket* pOutPacket = NULL;
            DOMElement* AccNode = NULL;
            DOMElement* ListAccNode = NULL;

            //DOMElement* ItemAccNode = NULL;
            if (!pOutPacket)
            {
                pOutPacket = new CPacket();
            }

            pOutPacket->SetRootTag("Response");

            AccNode = pOutPacket->CreateElement((char*)"CmdType");
            pOutPacket->SetElementValue(AccNode, (char*)"PresetConfig");

            AccNode = pOutPacket->CreateElement((char*)"SN");
            pOutPacket->SetElementValue(AccNode, strSN);

            AccNode = pOutPacket->CreateElement((char*)"DeviceID");
            pOutPacket->SetElementValue(AccNode, strDeviceID);

            AccNode = pOutPacket->CreateElement((char*)"SumNum");
            pOutPacket->SetElementValue(AccNode, (char*)"0");

            ListAccNode = pOutPacket->CreateElement((char*)"PresetConfigList");
            pOutPacket->SetElementAttr(ListAccNode, (char*)"Num", (char*)"0");

            /* ������Ӧ��Ϣ */
            i = SIP_SendMessage(NULL, local_cms_id_get(), caller_id, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)pOutPacket->GetXml(NULL).c_str(), pOutPacket->GetXml(NULL).length());

            if (i != 0)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_preset_info_proc() SIP_SendMessage Error:CMS ID=%s, IP address=%s, port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
            }
            else
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_preset_info_proc() SIP_SendMessage OK:CMS ID=%s, IP address=%s, port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
            }

            if (pOutPacket)
            {
                delete pOutPacket;
            }

            pOutPacket = NULL;
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_preset_info_proc() exit---: Get Callee Route Info Error \r\n");
            SystemLog(EV9000_CMS_GET_PRESET_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�����Ĳ�ѯԤ��λ��Ϣʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, ��ѯ���߼��豸ID=%s, ԭ��=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, (char*)"��ȡ�߼��豸��Ӧ���ϼ�·����Ϣʧ��");
            EnSystemLog(EV9000_CMS_GET_PRESET_ERROR, EV9000_LOG_LEVEL_ERROR, "Search for preset info from superior CMS failed:CMS ID=%s, IP address=%s, port=%d, reason=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"Get the route information which corresponding to logical device failed.");
            return -1;
        }

        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS�����Ĳ�ѯԤ��λ��Ϣ���ϼ�CMS�еĵ�λ:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, ��ѯ���߼��豸ID=%s, ���ڵ��ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, pCalleeRouteInfo->server_id, pCalleeRouteInfo->server_ip, pCalleeRouteInfo->server_port);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Search for preset info from superior CMS is the higher level in the CMS: superior CMS, ID = % s = % s IP address, port number = % d, query logic device ID = % s, the superior CMS, ID = % s = % s IP address, port number = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, pCalleeRouteInfo->server_id, pCalleeRouteInfo->server_ip, pCalleeRouteInfo->server_port);

        AccSnNode = inPacket.SearchElement((char*)"SN");

        if (NULL != AccSnNode)
        {
            g_transfer_xml_sn++;
            snprintf(strTransferSN, 32, "%u", g_transfer_xml_sn);
            inPacket.SetElementValue(AccSnNode, strTransferSN);
            //inPacket.SetTextContent();
        }

        /* ����ѯ¼���������Ϣת����ȥ */
        i = SIP_SendMessage(NULL, local_cms_id_get(), pCalleeRouteInfo->server_id, pCalleeRouteInfo->strRegLocalIP, pCalleeRouteInfo->iRegLocalPort, pCalleeRouteInfo->server_ip, pCalleeRouteInfo->server_port, (char*)inPacket.GetXml(NULL).c_str(), inPacket.GetXml(NULL).length());

        if (i != 0)
        {
            SystemLog(EV9000_CMS_GET_PRESET_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�����Ĳ�ѯԤ��λ��Ϣʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, ԭ��=%s, ת���ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, (char*)"ת����ѯ��Ϣ���ϼ�CMSʧ��", pCalleeRouteInfo->server_id, pCalleeRouteInfo->server_ip, pCalleeRouteInfo->server_port);
            EnSystemLog(EV9000_CMS_GET_PRESET_ERROR, EV9000_LOG_LEVEL_ERROR, "Search for preset info from superior CMS, forwards the query message to the superior CMS failure:CMS ID=%s, IP address=%s, port=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_preset_info_proc() SIP_SendMessage Error:CMS ID=%s, IP address=%s, port=%d \r\n", pCalleeRouteInfo->server_id, pCalleeRouteInfo->server_ip, pCalleeRouteInfo->server_port);
        }
        else
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS�����Ĳ�ѯԤ��λ��Ϣ, ת����ѯ��Ϣ���ϼ�CMS�ɹ�:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, ת���ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, pCalleeRouteInfo->server_id, pCalleeRouteInfo->server_ip, pCalleeRouteInfo->server_port);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Search for preset info from superior CMS, forwards the query message to the superior CMS success:Superior CMS, ID = % s = % s IP address, port number = % d, forward the superior CMS, ID = % s = % s IP address, port number = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, pCalleeRouteInfo->server_id, pCalleeRouteInfo->server_ip, pCalleeRouteInfo->server_port);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_preset_info_proc() SIP_SendMessage OK:CMS ID=%s, IP address=%s, port=%d \r\n", pCalleeRouteInfo->server_id, pCalleeRouteInfo->server_ip, pCalleeRouteInfo->server_port);

            old_xml_sn = strtoul(strSN, NULL, 10);
            transfer_xml_sn = strtoul(strTransferSN, NULL, 10);
            i = transfer_xml_msg_add(XML_QUERY_GETPRESET, old_xml_sn, transfer_xml_sn, pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, strDeviceID);
            DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_query_preset_info_proc() transfer_xml_msg_add: Type=%d, old_xml_sn=%u, transfer_xml_sn=%u, DeviceID=%s, pos=%d \r\n", XML_QUERY_GETPRESET, old_xml_sn, transfer_xml_sn, strDeviceID, i);
        }
    }
    else
    {
        /* ��ȡ�߼��豸�����������豸 */
        pGBDeviceInfo = GBDevice_info_get_by_stream_type(pGBLogicDeviceInfo, EV9000_STREAM_TYPE_MASTER);

        if (NULL != pGBDeviceInfo
            && EV9000_DEVICETYPE_SIPSERVER == pGBDeviceInfo->device_type
            && 0 == pGBDeviceInfo->three_party_flag) /* �ǵ�����ƽ̨���¼�cms �ĵ�λ */
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS�����Ĳ�ѯԤ��λ��Ϣ���¼�CMS�еĵ�λ:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, ��ѯ���߼��豸ID=%s, ���ڵ��¼�CMS ID=%s, IP��ַ=%s, �˿ں�=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Search for preset info from superior CMS is the lower level in the CMS:Superior CMS, ID = % s = % s IP address, port number = % d, query logic device ID = % s, the lower the CMS, ID = % s = % s IP address, port number = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);

            AccSnNode = inPacket.SearchElement((char*)"SN");

            if (NULL != AccSnNode)
            {
                g_transfer_xml_sn++;
                snprintf(strTransferSN, 32, "%u", g_transfer_xml_sn);
                inPacket.SetElementValue(AccSnNode, strTransferSN);
                //inPacket.SetTextContent();
            }

            /* ����ѯ¼���������Ϣת����ȥ */
            i = SIP_SendMessage(NULL, local_cms_id_get(), pGBDeviceInfo->device_id, pGBDeviceInfo->strRegServerIP, pGBDeviceInfo->iRegServerPort, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)inPacket.GetXml(NULL).c_str(), inPacket.GetXml(NULL).length());

            if (i != 0)
            {
                SystemLog(EV9000_CMS_GET_PRESET_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�����Ĳ�ѯԤ��λ��Ϣʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, ��ѯ���߼��豸ID=%s, ԭ��=%s, ת���¼�CMS ID=%s, IP��ַ=%s, �˿ں�=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, (char*)"ת����ѯ��Ϣ���¼�CMSʧ��", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                EnSystemLog(EV9000_CMS_GET_PRESET_ERROR, EV9000_LOG_LEVEL_ERROR, "Search for preset info from superior CMS ,forward search info to subordinate CMS failed:CMS ID=%s, IP address=%s, port=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_preset_info_proc() SIP_SendMessage Error:CMS ID=%s, IP address=%s, port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
            }
            else
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS�����Ĳ�ѯԤ��λ��Ϣ�ɹ�,ת����ѯ��Ϣ���¼�CMS:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, ��ѯ���߼��豸ID=%s, ת���¼�CMS ID=%s, IP��ַ=%s, �˿ں�=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Search for preset info from superior CMS success,forward search info to subordinate CMS:Superior CMS, ID = % s = % s IP address, port number = % d, query logic device ID = % s, forwarding the lower CMS, ID = % s = % s IP address, port number = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_preset_info_proc() SIP_SendMessage OK:CMS ID=%s, IP address=%s, port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);

                old_xml_sn = strtoul(strSN, NULL, 10);
                transfer_xml_sn = strtoul(strTransferSN, NULL, 10);
                i = transfer_xml_msg_add(XML_QUERY_GETPRESET, old_xml_sn, transfer_xml_sn, pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, strDeviceID);
                DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_query_preset_info_proc() transfer_xml_msg_add: Type=%d, old_xml_sn=%u, transfer_xml_sn=%u, DeviceID=%s, pos=%d \r\n", XML_QUERY_GETPRESET, old_xml_sn, transfer_xml_sn, strDeviceID, i);
            }
        }
        else
        {
            //query
            char szSql[100] = {0};
            char strSumNum[32] = {0};
            snprintf(szSql, 100, "select * from PresetConfig WHERE DeviceIndex = %u order by PresetID asc;", pGBLogicDeviceInfo->id);
            record_count  = pRoute_Srv_dboper->DB_Select(szSql, 1);
            snprintf(strSumNum, 32, "%d", record_count);

            if (record_count < 0)
            {
                /* �ظ���Ӧ,�齨��Ϣ */
                CPacket* pOutPacket = NULL;
                DOMElement* AccNode = NULL;
                DOMElement* ListAccNode = NULL;

                //DOMElement* ItemAccNode = NULL;
                if (!pOutPacket)
                {
                    pOutPacket = new CPacket();
                }

                pOutPacket->SetRootTag("Response");

                AccNode = pOutPacket->CreateElement((char*)"CmdType");
                pOutPacket->SetElementValue(AccNode, (char*)"PresetConfig");

                AccNode = pOutPacket->CreateElement((char*)"SN");
                pOutPacket->SetElementValue(AccNode, strSN);

                AccNode = pOutPacket->CreateElement((char*)"DeviceID");
                pOutPacket->SetElementValue(AccNode, strDeviceID);

                AccNode = pOutPacket->CreateElement((char*)"SumNum");
                pOutPacket->SetElementValue(AccNode, (char*)"0");

                ListAccNode = pOutPacket->CreateElement((char*)"PresetConfigList");
                pOutPacket->SetElementAttr(ListAccNode, (char*)"Num", (char*)"0");

                /* ������Ӧ��Ϣ ���ϼ�CMS */
                i = SIP_SendMessage(NULL, local_cms_id_get(), caller_id, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)pOutPacket->GetXml(NULL).c_str(), pOutPacket->GetXml(NULL).length());

                if (i != 0)
                {
                    SystemLog(EV9000_CMS_GET_PRESET_ERROR, EV9000_LOG_LEVEL_ERROR, "����Message��Ϣ��Ŀ�ĵ�ʧ��:Ŀ��ID=%s, IP��ַ=%s, �˿ں�=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                    EnSystemLog(EV9000_CMS_GET_PRESET_ERROR, EV9000_LOG_LEVEL_ERROR, "Sending messages to a destination failure: objective, ID = % s = % s IP address, port number = % d ", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                    DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_preset_info_proc() SIP_SendMessage Error:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                }
                else
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "����Message��Ϣ��Ŀ�ĵسɹ�:Ŀ��ID=%s, IP��ַ=%s, �˿ں�=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Sending messages to a destination success: objective, ID = % s = % s IP address, port number = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_preset_info_proc() SIP_SendMessage OK:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                }

                if (pOutPacket)
                {
                    delete pOutPacket;
                }

                pOutPacket = NULL;
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_preset_info_proc() Error DB Oper Error:strSQL=%s, record_count=%d \r\n", szSql, record_count);
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_preset_info_proc() ErrorMsg=%s\r\n", pRoute_Srv_dboper->GetLastDbErrorMsg());
                SystemLog(EV9000_CMS_GET_PRESET_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�����Ĳ�ѯԤ��λ��Ϣʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, ��ѯ���߼��豸ID=%s, ԭ��=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, (char*)"��ѯ���ݿ�ʧ��");
                EnSystemLog(EV9000_CMS_GET_PRESET_ERROR, EV9000_LOG_LEVEL_ERROR, "Search for preset info from superior CMS failed:CMS ID=%s, IP address=%s, port=%d, cause=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"Search in database failed");
                return -1;
            }
            else if (record_count == 0)
            {
                /* �ظ���Ӧ,�齨��Ϣ */
                CPacket* pOutPacket = NULL;
                DOMElement* AccNode = NULL;
                DOMElement* ListAccNode = NULL;

                //DOMElement* ItemAccNode = NULL;
                if (!pOutPacket)
                {
                    pOutPacket = new CPacket();
                }

                pOutPacket->SetRootTag("Response");
                AccNode = pOutPacket->CreateElement((char*)"CmdType");
                pOutPacket->SetElementValue(AccNode, (char*)"PresetConfig");

                AccNode = pOutPacket->CreateElement((char*)"SN");
                pOutPacket->SetElementValue(AccNode, strSN);

                AccNode = pOutPacket->CreateElement((char*)"DeviceID");
                pOutPacket->SetElementValue(AccNode, strDeviceID);

                AccNode = pOutPacket->CreateElement((char*)"SumNum");
                pOutPacket->SetElementValue(AccNode, (char*)"0");

                ListAccNode = pOutPacket->CreateElement((char*)"PresetConfigList");
                pOutPacket->SetElementAttr(ListAccNode, (char*)"Num", (char*)"0");

                /* ������Ӧ��Ϣ ���ϼ�CMS */
                i = SIP_SendMessage(NULL, local_cms_id_get(), caller_id, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)pOutPacket->GetXml(NULL).c_str(), pOutPacket->GetXml(NULL).length());

                if (i != 0)
                {
                    SystemLog(EV9000_CMS_GET_PRESET_ERROR, EV9000_LOG_LEVEL_ERROR, "����Message��Ϣ��Ŀ�ĵ�ʧ��:Ŀ��ID=%s, IP��ַ=%s, �˿ں�=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                    EnSystemLog(EV9000_CMS_GET_PRESET_ERROR, EV9000_LOG_LEVEL_ERROR, "Sending messages to a destination failure: objective, ID = % s = % s IP address, port number = % d ", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                    DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_preset_info_proc() SIP_SendMessage Error:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                }
                else
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "����Message��Ϣ��Ŀ�ĵسɹ�:Ŀ��ID=%s, IP��ַ=%s, �˿ں�=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Sending messages to a destination success: objective, ID = % s = % s IP address, port number = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_preset_info_proc() SIP_SendMessage OK:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                }

                if (pOutPacket)
                {
                    delete pOutPacket;
                }

                pOutPacket = NULL;
                DEBUG_TRACE(MODULE_ROUTE, LOG_WARN, "route_query_preset_info_proc() exit---: No Record Count \r\n");
                SystemLog(EV9000_CMS_GET_PRESET_ERROR, EV9000_LOG_LEVEL_WARNING, "�ϼ�CMS�����Ĳ�ѯԤ��λ��Ϣʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, ��ѯ���߼��豸ID=%s, ԭ��=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, (char*)"û�в�ѯ�����ݿ��¼");
                EnSystemLog(EV9000_CMS_GET_PRESET_ERROR, EV9000_LOG_LEVEL_WARNING, "Search for preset info from superior CMS failed:CMS ID=%s, IP address=%s, port=%d, cause=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"Database record not found");
                return i;
            }

            CPacket* pOutPacket = NULL;
            DOMElement* ListAccNode = NULL;

            /* ѭ���������ݿ⣬���������XML ���͸��ͻ��� */
            do
            {
                int             nID = 0;                                      //��¼���
                unsigned int    nDeviceIndex = 0;                            //�豸ID
                int             nPresetID = 0;                               //Ԥ��λ���
                string          strPresetName = "";                          //Ԥ��λ����
                int             nResved1 = 0;                                //����1
                string          strResved2 = "";

                while_count++;

                if (while_count % 10000 == 0)
                {
                    DEBUG_TRACE(MODULE_ROUTE, LOG_WARN, "route_query_preset_info_proc() While Count=%d \r\n", while_count);
                }

                query_count++;

                /* ����XMLͷ�� */
                i = CreatePresetConfigResponseXMLHead(&pOutPacket, query_count, record_count, strSN, strDeviceID, &ListAccNode);

                /* ���� */
                pRoute_Srv_dboper->GetFieldValue("ID", nID);

                /* ��λ���� */
                pRoute_Srv_dboper->GetFieldValue("DeviceIndex", nDeviceIndex);

                /* Ԥ��λID */
                pRoute_Srv_dboper->GetFieldValue("PresetID", nPresetID);

                /* Ԥ��λ���� */
                strPresetName.clear();
                pRoute_Srv_dboper->GetFieldValue("PresetName", strPresetName);

                /* Ԥ��1 */
                pRoute_Srv_dboper->GetFieldValue("Resved1", nResved1);

                /* Ԥ��2 */
                strResved2.clear();
                pRoute_Srv_dboper->GetFieldValue("ParentID", strResved2);

                /* ����Item ֵ */
                i = AddPresetConfigToXMLItem(pOutPacket, ListAccNode, nID, nDeviceIndex, nPresetID, strPresetName, nResved1, strResved2);

                if ((query_count % MAX_DEVICE_PRESET_COUT_SEND == 0) || (query_count == record_count))
                {
                    if (NULL != pOutPacket)
                    {
                        send_count++;
                        /* ���ͳ�ȥ */
                        i |= SIP_SendMessage(NULL, local_cms_id_get(), caller_id, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)pOutPacket->GetXml(NULL).c_str(), pOutPacket->GetXml(NULL).length());

                        if (i != 0)
                        {
                            SystemLog(EV9000_CMS_GET_PRESET_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�����Ĳ�ѯԤ��λ��Ϣ, ����Message��Ϣ���ϼ�CMSʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                            EnSystemLog(EV9000_CMS_GET_PRESET_ERROR, EV9000_LOG_LEVEL_ERROR, "Query preset information from superior CMS, sending messages to the superior CMS failure: the higher the CMS, ID = % s = % s IP address, port number = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_preset_info_proc() SIP_SendMessage Error:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                        }
                        else
                        {
                            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS�����Ĳ�ѯԤ��λ��Ϣ, ����Message��Ϣ���ϼ�CMS�ɹ�:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Query preset information from superior CMS, sending messages to the superior CMS success: the higher the CMS, ID = % s = % s IP address, port number = % d ", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_preset_info_proc() SIP_SendMessage OK:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                        }

                        delete pOutPacket;
                        pOutPacket = NULL;
                    }
                }
            }
            while (pRoute_Srv_dboper->MoveNext() >= 0);

            if (i == 0)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS�����Ĳ�ѯԤ��λ��Ϣ�ɹ�:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, ��¼��=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, record_count);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Access logic device group information success:front-end ID=%s, IP=%s, port=%d, record count=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, record_count);
            }
            else
            {
                SystemLog(EV9000_CMS_GET_PRESET_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�����Ĳ�ѯԤ��λ��Ϣʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, ԭ��=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"����SIP��Ӧ��Ϣʧ��");
                EnSystemLog(EV9000_CMS_GET_PRESET_ERROR, EV9000_LOG_LEVEL_ERROR, "Access logic device group information failure:front-end ID=%s, IP=%s, port=%d, cause=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"SIP response message sent failure");
            }
        }
    }

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS�����Ĳ�ѯԤ��λ��Ϣ�ɹ�:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, ��ѯ���߼��豸ID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Query preset information from superior CMS, sending messages to the superior CMS success: the higher the CMS, ID = % s = % s IP address, port number = % d ", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
    return 0;
}

/*****************************************************************************
 �� �� ��  : route_query_device_group_config_proc
 ��������  : �ϼ�����CMS���͹����Ļ�ȡ�߼��豸�������ñ�
 �������  : route_info_t* pRouteInfo
             char* caller_id
             char* callee_id
             CPacket& inPacket
             DBOper* pRoute_Srv_dboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��2��12�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int route_query_device_group_config_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, CPacket& inPacket, DBOper* pRoute_Srv_dboper)
{
    /* �ϼ�CMS ��ȡ�߼��豸�������ñ�
      */
    int i = 0;
    int record_count = 0; /* ��¼�� */
    int send_count = 0;   /* ���͵Ĵ��� */
    int query_count = 0;  /* ��ѯ����ͳ�� */

    char strSN[32] = {0};
    char strDeviceID[32] = {0};

    DOMElement* ListAccNode = NULL;

    string strSQL = "";
    int while_count = 0;

    if (NULL == pRouteInfo)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_query_device_group_config_proc() exit---: Route Info Error \r\n");
        return -1;
    }

    if (NULL == caller_id || NULL == callee_id || NULL == pRoute_Srv_dboper)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_query_device_group_config_proc() exit---: Route Srv DB Oper Error \r\n");
        return -1;
    }

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS�����Ļ�ȡ�߼��豸����������Ϣ:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Access device group config info from superior CMS:Superior CMS, ID = % s = % s IP address, port number = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);

    /* ������ǻ�ȡ��CMS���򷵻� */
    if (0 != strncmp(callee_id, local_cms_id_get(), 20))
    {
        SystemLog(EV9000_CMS_GET_DEIVCE_GROUP_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�����Ļ�ȡ�߼��豸����������Ϣ����ʧ��:����=%s, IP��ַ=%s, �˿ں�=%d, ԭ��=%s:%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"��ѯ��ID���Ǳ�CMS��ID", callee_id);
        EnSystemLog(EV9000_CMS_GET_DEIVCE_GROUP_ERROR, EV9000_LOG_LEVEL_ERROR, "Access device group config info from superior CMS failed:IP address=%s,IP address=%s, port number=%d, cause=%s:%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"ID searched does not belong to this CMS", callee_id);
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_device_group_config_proc() exit---: DeviceID Not Belong To Mine CMSID:callee_id=%s \r\n", callee_id);
        return -1;
    }

    /* ȡ������*/
    inPacket.GetElementValue((char*)"SN", strSN);
    inPacket.GetElementValue((char*)"DeviceID", strDeviceID);

    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO,  "route_query_device_group_config_proc() \
        \r\n XML Para: \
        \r\n SN=%s, DeviceID=%s \r\n", strSN, strDeviceID);

    /* �����ѯ���豸ID���Ǳ�CMS ID*/
    if (0 != sstrcmp(callee_id, strDeviceID))
    {
        SystemLog(EV9000_CMS_GET_DEIVCE_GROUP_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�����Ļ�ȡ�߼��豸����������Ϣ����ʧ��:�ϼ�CMS ID=%s, IP address=%s, port number=%d, cause=%s:%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"��ѯ��ID���Ǳ�CMS��ID", strDeviceID);
        EnSystemLog(EV9000_CMS_GET_DEIVCE_GROUP_ERROR, EV9000_LOG_LEVEL_ERROR, "Access device group config info from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, cause=%s:%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"ID searched does not belong to this CMS", strDeviceID);
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_device_group_config_proc() exit---: DeviceID Not Belong To Mine CMSID:DeviceID=%s \r\n", strDeviceID);
        return -1;
    }

    /* ���ݲ�ѯ�������������ݿ⣬�ҵ���Ӧ���߼��豸����������Ϣ */
    strSQL.clear();
    strSQL = "select * from LogicDeviceGroupConfig WHERE OtherRealm = 0 order by GroupID asc";
    record_count = pRoute_Srv_dboper->DB_Select(strSQL.c_str(), 1);

    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_device_group_config_proc() record_count=%d \r\n", record_count);

    if (record_count <= 0)
    {
        SystemLog(EV9000_CMS_GET_DEIVCE_MAP_GROUP_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�����Ļ�ȡ�߼��豸����������Ϣ����:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, ��ѯ���ķ�����������=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, record_count);
    }
    else
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS�����Ļ�ȡ�߼��豸����������Ϣ����:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, ��ѯ���ķ�����������=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, record_count);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Access device group config info from superior CMS:Superior CMS, ID = % s = % s IP address, port number = % d, total number of queries to the grouping configuration = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, record_count);
    }

    if (record_count < 0)
    {
        SystemLog(EV9000_CMS_GET_DEIVCE_GROUP_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�����Ļ�ȡ�߼��豸����������Ϣ����, ��ѯ���ݿ�ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, ԭ��=%s:%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"��ѯ���ݿ�ʧ��", pRoute_Srv_dboper->GetLastDbErrorMsg());
        EnSystemLog(EV9000_CMS_GET_DEIVCE_GROUP_ERROR, EV9000_LOG_LEVEL_ERROR, "Access device group config info from superior CMS search in database failed:Requester ID=%s, IP address=%s, port number=%d, cause=%s:%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"searcch in database failed", pRoute_Srv_dboper->GetLastDbErrorMsg());
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_device_group_config_proc() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_device_group_config_proc() ErrorMsg=%s\r\n", pRoute_Srv_dboper->GetLastDbErrorMsg());
        return -1;
    }

    if (record_count == 0)
    {
        /* �ظ���Ӧ,�齨��Ϣ */
        CPacket outPacket;
        DOMElement* AccNode = NULL;

        outPacket.SetRootTag("Response");

        AccNode = outPacket.CreateElement((char*)"CmdType");
        outPacket.SetElementValue(AccNode, (char*)"LogicDeviceGroupConfig");

        AccNode = outPacket.CreateElement((char*)"SN");
        outPacket.SetElementValue(AccNode, strSN);

        AccNode = outPacket.CreateElement((char*)"DeviceID");
        outPacket.SetElementValue(AccNode, strDeviceID);

        AccNode = outPacket.CreateElement((char*)"Result");
        outPacket.SetElementValue(AccNode, (char*)"OK");

        AccNode = outPacket.CreateElement((char*)"SumNum");
        outPacket.SetElementValue(AccNode, (char*)"0");

        ListAccNode = outPacket.CreateElement((char*)"LogicDeviceGroupConfigList");
        outPacket.SetElementAttr(ListAccNode, (char*)"Num", (char*)"0");

        /* ������Ӧ��Ϣ���ϼ�CMS */
        i = SIP_SendMessage(NULL, local_cms_id_get(), caller_id, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

        if (i != 0)
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_device_group_config_proc() SIP_SendMessage Error:caller_id=%s, caller_ip=%s, caller_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
        }
        else
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_device_group_config_proc() SIP_SendMessage OK:caller_id=%s, caller_ip=%s, caller_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
        }

        SystemLog(EV9000_CMS_GET_DEIVCE_GROUP_ERROR, EV9000_LOG_LEVEL_WARNING, "�ϼ�CMS�����Ļ�ȡ�߼��豸����������Ϣ����, ��ѯ���ݿ�ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, ԭ��=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"δ��ѯ�����ݿ��¼");
        EnSystemLog(EV9000_CMS_GET_DEIVCE_GROUP_ERROR, EV9000_LOG_LEVEL_WARNING, "Access device group config info from superior CMS search in database failed:Requester ID=%s, IP address=%s, port number=%d,cause=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"Database record not found");
        DEBUG_TRACE(MODULE_ROUTE, LOG_WARN, "route_query_device_group_config_proc() exit---: No Record Count \r\n");
        return i;

    }

    CPacket* pOutPacket = NULL;

    /* ѭ���������ݿ⣬���������XML ���͸��ϼ�CMS */
    do
    {
        int id = 0;
        string strName = "";
        string strGroupID = "";
        string strCMSID = "";
        int SortID = 0;
        string strParentID = "";

        while_count++;

        if (while_count % 10000 == 0)
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_WARN, "route_query_device_group_config_proc() While Count=%d \r\n", while_count);
        }

        query_count++;

        /* ����XMLͷ�� */
        i = CreateDeviceGroupConfigResponseXMLHead(&pOutPacket, query_count, record_count, strSN, strDeviceID, &ListAccNode);

        /* ���� */
        pRoute_Srv_dboper->GetFieldValue("ID", id);

        /* ���� */
        strGroupID.clear();
        pRoute_Srv_dboper->GetFieldValue("GroupID", strGroupID);

        /* CMS��� */
        /*
           strCMSID.clear();
           pRoute_Srv_dboper->GetFieldValue("CMSID", strCMSID);
           */

        /* ������ */
        strName.clear();
        pRoute_Srv_dboper->GetFieldValue("Name", strName);

        /* ͬһ���ڵ����������� */
        pRoute_Srv_dboper->GetFieldValue("SortID", SortID);

        /* ���ڵ��� */
        strParentID.clear();
        pRoute_Srv_dboper->GetFieldValue("ParentID", strParentID);

        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS�����Ļ�ȡ�߼��豸����������Ϣ����, ��ѯ���ķ�����Ϣ:����=%s, ������=%s", strGroupID.c_str(), strName.c_str());
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Access device group config info from superior CMS, Query to the grouping of information: the group number = % s, group name = % s", strGroupID.c_str(), strName.c_str());

        /* ����Item ֵ */
        i = AddDeviceGroupConfigToXMLItem(pOutPacket, ListAccNode, id, strGroupID, strCMSID, strName, SortID, strParentID);

        if ((query_count % MAX_DEVICE_GROUP_COUT_SEND == 0) || (query_count == record_count))
        {
            if (NULL != pOutPacket)
            {
                send_count++;
                /* ������Ӧ��Ϣ ���ϼ�CMS */
                i |= SIP_SendMessage(NULL, local_cms_id_get(), caller_id, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)pOutPacket->GetXml(NULL).c_str(), pOutPacket->GetXml(NULL).length());

                if (i != 0)
                {
                    SystemLog(EV9000_CMS_GET_DEIVCE_GROUP_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�����Ļ�ȡ�߼��豸����������Ϣ����, ����Message��Ϣ���ϼ�CMSʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                    EnSystemLog(EV9000_CMS_GET_DEIVCE_GROUP_ERROR, EV9000_LOG_LEVEL_ERROR, "Access device group config info from superior CMS, Send Message Message to superiors CMS failure: the superior CMS, ID = % s = % s IP address, port number = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                    DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_device_group_config_proc() SIP_SendMessage Error:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                }
                else
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS�����Ļ�ȡ�߼��豸����������Ϣ����, ����Message��Ϣ���ϼ�CMS�ɹ�:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Access device group config info from superior CMS, Send Message Message to superiors CMS success: the superior CMS, ID = % s = % s IP address, port number = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_device_group_config_proc() SIP_SendMessage OK:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                }

                delete pOutPacket;
                pOutPacket = NULL;
            }
        }
    }
    while (pRoute_Srv_dboper->MoveNext() >= 0);

    if (i == 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS�����Ļ�ȡ�߼��豸����������Ϣ����ɹ�:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, ��¼��=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, record_count);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Access device group config info from superior CMS, Send Message Message to superiors CMS success: the superior CMS, ID = % s = % s IP address, port number = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
    }
    else
    {
        SystemLog(EV9000_CMS_GET_DEIVCE_GROUP_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�����Ļ�ȡ�߼��豸����������Ϣ����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, ԭ��=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"����SIP��Ӧ��Ϣʧ��");
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Access device group config info from superior CMS, Send Message Message to superiors CMS success: the superior CMS, ID = % s = % s IP address, port number = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
    }

    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_device_group_config_proc Exit--- \r\n");

    return 0;
}

/*****************************************************************************
 �� �� ��  : route_query_device_map_group_config_proc
 ��������  : �ϼ�����CMS���͹����Ļ�ȡ�߼��豸�����ϵ���ñ�
 �������  : route_info_t* pRouteInfo
             char* caller_id
             char* callee_id
             CPacket& inPacket
             DBOper* pRoute_Srv_dboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��2��12�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int route_query_device_map_group_config_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, CPacket& inPacket, DBOper* pRoute_Srv_dboper)
{
    /* �ϼ�CMS�����Ļ�ȡ�߼��豸�����ϵ���ñ�
      */
    int i = 0;
    int record_count = 0; /* ��¼�� */
    int send_count = 0;   /* ���͵Ĵ��� */
    int query_count = 0;  /* ��ѯ����ͳ�� */

    char strSN[32] = {0};
    char strDeviceID[32] = {0};

    DOMElement* ListAccNode = NULL;

    string strSQL = "";
    int while_count = 0;

    if (NULL == pRouteInfo)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_query_device_map_group_config_proc() exit---: Route Info Error \r\n");
        return -1;
    }

    if (NULL == caller_id || NULL == callee_id || NULL == pRoute_Srv_dboper)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_query_device_map_group_config_proc() exit---: User Srv DB Oper Error \r\n");
        return -1;
    }

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS�����Ļ�ȡ�߼��豸�����ϵ������Ϣ:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Access device group relationship config info from superior CMS:Superior CMS, ID = % s = % s IP address, port number = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);

    /* ������ǻ�ȡ��CMS���򷵻� */
    if (0 != strncmp(callee_id, local_cms_id_get(), 20))
    {
        SystemLog(EV9000_CMS_GET_DEIVCE_MAP_GROUP_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�����Ļ�ȡ�߼��豸�����ϵ������Ϣ����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, ԭ��=%s:%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"��ѯ��ID���Ǳ�CMS��ID", callee_id);
        EnSystemLog(EV9000_CMS_GET_DEIVCE_MAP_GROUP_ERROR, EV9000_LOG_LEVEL_ERROR, "Access device group relationship config info from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, cause=%s:%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"ID searched does not belong to this CMS", callee_id);
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_device_map_group_config_proc() exit---: DeviceID Not Belong To Mine CMSID \r\n");
        return -1;
    }

    /* ȡ������*/
    inPacket.GetElementValue((char*)"SN", strSN);
    inPacket.GetElementValue((char*)"DeviceID", strDeviceID);

    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO,  "route_query_device_map_group_config_proc() \
            \r\n XML Para: \
            \r\n SN=%s, DeviceID=%s \r\n", strSN, strDeviceID);

    /* �����ѯ���豸ID���Ǳ�CMS ID*/
    if (0 != sstrcmp(callee_id, strDeviceID))
    {
        SystemLog(EV9000_CMS_GET_DEIVCE_MAP_GROUP_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�����Ļ�ȡ�߼��豸�����ϵ������Ϣ����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, ԭ��=%s:%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"��ѯ��ID���Ǳ�CMS��ID", strDeviceID);
        EnSystemLog(EV9000_CMS_GET_DEIVCE_MAP_GROUP_ERROR, EV9000_LOG_LEVEL_ERROR, "Access device group relationship config info from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, cause=%s:%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"ID searched does not belong to this CMS", strDeviceID);
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_record_info_proc() exit---: DeviceID Not Belong To Mine CMSID \r\n");
        return -1;
    }

    /* ���ݲ�ѯ�������������ݿ⣬�ҵ���Ӧ���߼��豸�����ϵ���ù�ϵ��Ϣ*/
    strSQL.clear();
    strSQL = "SELECT G.ID, G.GroupID, G.DeviceIndex, G.SortID FROM LogicDeviceMapGroupConfig AS G, GBLogicDeviceConfig AS GD WHERE G.DeviceIndex = GD.ID AND GD.OtherRealm=0";
    record_count = pRoute_Srv_dboper->DB_Select(strSQL.c_str(), 1);

    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_device_map_group_config_proc() record_count=%d \r\n", record_count);

    if (record_count <= 0)
    {
        SystemLog(EV9000_CMS_GET_DEIVCE_MAP_GROUP_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�����Ļ�ȡ�߼��豸�����ϵ������Ϣ����:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, ��ѯ���ķ����ϵ��������=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, record_count);
        EnSystemLog(EV9000_CMS_GET_DEIVCE_MAP_GROUP_ERROR, EV9000_LOG_LEVEL_ERROR, "Access device group relationship config info from superior CMS:Superior CMS, ID = % s = % s IP address, port number = % d, total number of queries to grouping relationship configuration = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, record_count);
    }
    else
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS�����Ļ�ȡ�߼��豸�����ϵ������Ϣ����:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, ��ѯ���ķ����ϵ��������=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, record_count);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Access device group relationship config info from superior CMS:Superior CMS, ID = % s = % s IP address, port number = % d, total number of queries to grouping relationship configuration = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, record_count);
    }

    if (record_count < 0)
    {
        SystemLog(EV9000_CMS_GET_DEIVCE_MAP_GROUP_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�����Ļ�ȡ�߼��豸�������ù�ϵ��Ϣ����, ��ʼ��ѯ���ݿ�ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, ԭ��=%s:%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"��ѯ���ݿ�ʧ��", pRoute_Srv_dboper->GetLastDbErrorMsg());
        EnSystemLog(EV9000_CMS_GET_DEIVCE_MAP_GROUP_ERROR, EV9000_LOG_LEVEL_ERROR, "Access device group relationship config info from superior CMS start to search in database failed:Requester ID=%s, IP address=%s, port number=%d, cause=%s:%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"searcch in database failed", pRoute_Srv_dboper->GetLastDbErrorMsg());
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_device_map_group_config_proc() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_device_map_group_config_proc() ErrorMsg=%s\r\n", pRoute_Srv_dboper->GetLastDbErrorMsg());
        return -1;
    }

    if (record_count == 0)
    {
        /* �ظ���Ӧ,�齨��Ϣ */
        CPacket outPacket;
        DOMElement* AccNode = NULL;

        outPacket.SetRootTag("Response");

        AccNode = outPacket.CreateElement((char*)"CmdType");
        outPacket.SetElementValue(AccNode, (char*)"LogicDeviceMapGroupConfig");

        AccNode = outPacket.CreateElement((char*)"SN");
        outPacket.SetElementValue(AccNode, strSN);

        AccNode = outPacket.CreateElement((char*)"DeviceID");
        outPacket.SetElementValue(AccNode, strDeviceID);

        AccNode = outPacket.CreateElement((char*)"Result");
        outPacket.SetElementValue(AccNode, (char*)"OK");

        AccNode = outPacket.CreateElement((char*)"SumNum");
        outPacket.SetElementValue(AccNode, (char*)"0");

        ListAccNode = outPacket.CreateElement((char*)"LogicDeviceMapGroupConfigList");
        outPacket.SetElementAttr(ListAccNode, (char*)"Num", (char*)"0");

        /* ������Ӧ��Ϣ ���ϼ�CMS */
        i = SIP_SendMessage(NULL, local_cms_id_get(), caller_id, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

        if (i != 0)
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_device_map_group_config_proc() SIP_SendMessage Error:caller_id=%s, caller_ip=%s, caller_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
        }
        else
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_device_map_group_config_proc() SIP_SendMessage OK:caller_id=%s, caller_ip=%s, caller_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
        }

        SystemLog(EV9000_CMS_GET_DEIVCE_MAP_GROUP_ERROR, EV9000_LOG_LEVEL_WARNING, "�ϼ�CMS�����Ļ�ȡ�߼��豸�������ù�ϵ��Ϣ����, ��ѯ���ݿ�ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, ԭ��=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"δ��ѯ�����ݿ��¼");
        EnSystemLog(EV9000_CMS_GET_DEIVCE_MAP_GROUP_ERROR, EV9000_LOG_LEVEL_WARNING, "Access device group config info from superior CMS search in database failed:Requester ID=%s, IP address=%s, port number=%d,cause=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"Database record not found");
        DEBUG_TRACE(MODULE_ROUTE, LOG_WARN, "route_query_device_map_group_config_proc() exit---: No Record Count \r\n");
        return i;
    }

    CPacket* pOutPacket = NULL;

    /* ѭ���������ݿ⣬���������XML ���͸��ϼ�CMS */
    do
    {
        int id = 0;
        string strGroupID = "";
        unsigned int DeviceIndex = 0;
        string strCMSID = "";
        int SortID = 0;

        while_count++;

        if (while_count % 10000 == 0)
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_WARN, "route_query_device_map_group_config_proc() While Count=%d \r\n", while_count);
        }

        query_count++;

        /* ����XMLͷ�� */
        i = CreateDeviceMapGroupConfigResponseXMLHead(&pOutPacket, query_count, record_count, strSN, strDeviceID, &ListAccNode);

        /* ���� */
        pRoute_Srv_dboper->GetFieldValue("ID", id);

        /* ��λ���� */
        strGroupID.clear();
        pRoute_Srv_dboper->GetFieldValue("GroupID", strGroupID);

        /* �߼��豸���� */
        pRoute_Srv_dboper->GetFieldValue("DeviceIndex", DeviceIndex);

        /* CMS ��� */
        /*
           strCMSID.clear();
           pRoute_Srv_dboper->GetFieldValue("CMSID", strCMSID);
           */

        /* ������ */
        pRoute_Srv_dboper->GetFieldValue("SortID", SortID);

        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS�����Ļ�ȡ�߼��豸�������ù�ϵ��Ϣ����, ��ѯ���ķ����ϵ��Ϣ:����=%s, �߼��豸����=%u", strGroupID.c_str(), DeviceIndex);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Access logical device group configuration from superior CMS  processing,Query to the grouping of relationship information: group number = % s, logical device index = % u", strGroupID.c_str(), DeviceIndex);

        /* ����Item ֵ */
        i = AddDeviceMapGroupConfigToXMLItem(pOutPacket, ListAccNode, id, strGroupID, DeviceIndex, strCMSID, SortID);

        if ((query_count % MAX_DEVICE_MAP_GROUP_COUT_SEND == 0) || (query_count == record_count))
        {
            if (NULL != pOutPacket)
            {
                send_count++;
                /* ������Ӧ��Ϣ ���ϼ�CMS */
                i |= SIP_SendMessage(NULL, local_cms_id_get(), caller_id, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)pOutPacket->GetXml(NULL).c_str(), pOutPacket->GetXml(NULL).length());

                if (i != 0)
                {
                    SystemLog(EV9000_CMS_GET_DEIVCE_MAP_GROUP_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�����Ļ�ȡ�߼��豸�������ù�ϵ��Ϣ����, ����Message��Ϣ���ϼ�CMSʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                    EnSystemLog(EV9000_CMS_GET_DEIVCE_MAP_GROUP_ERROR, EV9000_LOG_LEVEL_ERROR, "Access logical device group configuration from superior CMS  processing, Message sending messages to the superior CMS failure: the higher the CMS, ID = % s = % s IP address and port number = % d" , caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                    DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_device_group_config_proc() SIP_SendMessage Error:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                }
                else
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS�����Ļ�ȡ�߼��豸�������ù�ϵ��Ϣ����, ����Message��Ϣ���ϼ�CMS�ɹ�:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Access logical device group configuration from superior CMS  processing, Message sending messages to the superior CMS success: the higher the CMS, ID = % s = % s IP address and port number = % d" , caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_device_group_config_proc() SIP_SendMessage OK:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                }

                delete pOutPacket;
                pOutPacket = NULL;
            }
        }
    }
    while (pRoute_Srv_dboper->MoveNext() >= 0);

    if (i == 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS�����Ļ�ȡ�߼��豸�����ϵ������Ϣ����ɹ�:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, ��¼��=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, record_count);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Access logical device group configuration from superior CMS  processing, Message sending messages to the superior CMS success: the higher the CMS, ID = % s = % s IP address and port number = % d" , caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
    }
    else
    {
        SystemLog(EV9000_CMS_GET_DEIVCE_MAP_GROUP_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�����Ļ�ȡ�߼��豸�����ϵ������Ϣ����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, ԭ��=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"����SIP��Ӧ��Ϣʧ��");
        EnSystemLog(EV9000_CMS_GET_DEIVCE_MAP_GROUP_ERROR, EV9000_LOG_LEVEL_ERROR, "Access logical device group configuration from superior CMS  processing, Message sending messages to the superior CMS failure: the higher the CMS, ID = % s = % s IP address and port number = % d" , caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
    }

    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_device_map_group_config_proc Exit--- \r\n");

    return 0;
}

/*****************************************************************************
 �� �� ��  : route_query_topology_phydevice_config_proc
 ��������  : �ϼ�����CMS���͹����Ļ�ȡ���������豸���ñ�
 �������  : route_info_t* pRouteInfo
             char* caller_id
             char* callee_id
             CPacket& inPacket
             DBOper* pRoute_Srv_dboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��11��27�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int route_query_topology_phydevice_config_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, CPacket& inPacket, DBOper* pRoute_Srv_dboper)
{
    /* �ϼ�CMS ��ȡ���������豸���ñ�
      */
    int i = 0;
    int iRet = 0;
    int record_count = 0; /* ��¼�� */
    int send_count = 0;   /* ���͵Ĵ��� */
    int query_count = 0;  /* ��ѯ����ͳ�� */
    char strDeviceType[16] = {0};

    char strSN[32] = {0};
    char strDeviceID[32] = {0};

    DOMElement* ListAccNode = NULL;

    string strSQL = "";
    int while_count = 0;
    char* local_ip = NULL;

    if (NULL == pRouteInfo)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_query_topology_phydevice_config_proc() exit---: Route Info Error \r\n");
        return -1;
    }

    if (NULL == caller_id || NULL == callee_id || NULL == pRoute_Srv_dboper)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_query_topology_phydevice_config_proc() exit---: Route Srv DB Oper Error \r\n");
        return -1;
    }

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS�����Ļ�ȡ���������豸������Ϣ:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Access topology physical device config info from superior CMS:Superior CMS, ID = % s = % s IP address, port number = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);

    /* ������ǻ�ȡ��CMS���򷵻� */
    if (0 != strncmp(callee_id, local_cms_id_get(), 20))
    {
        SystemLog(EV9000_CMS_GET_TOP_DEVICE_CONFIG_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�����Ļ�ȡ���������豸������Ϣʧ��:����=%s, IP��ַ=%s, �˿ں�=%d, ԭ��=%s:%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"��ѯ��ID���Ǳ�CMS��ID", callee_id);
        EnSystemLog(EV9000_CMS_GET_TOP_DEVICE_CONFIG_ERROR, EV9000_LOG_LEVEL_ERROR, "Access topology physical device config info from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, cause=%s:%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"ID searched does not belong to this CMS", callee_id);
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_topology_phydevice_config_proc() exit---: DeviceID Not Belong To Mine CMSID:callee_id=%s \r\n", callee_id);
        return -1;
    }

    /* ȡ������*/
    inPacket.GetElementValue((char*)"SN", strSN);
    inPacket.GetElementValue((char*)"DeviceID", strDeviceID);

    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO,  "route_query_topology_phydevice_config_proc() \
        \r\n XML Para: \
        \r\n SN=%s, DeviceID=%s \r\n", strSN, strDeviceID);

    /* �����ѯ���豸ID���Ǳ�CMS ID*/
    if (0 != sstrcmp(callee_id, strDeviceID))
    {
        SystemLog(EV9000_CMS_GET_TOP_DEVICE_CONFIG_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�����Ļ�ȡ���������豸������Ϣ����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, ԭ��=%s:%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"��ѯ��ID���Ǳ�CMS��ID", strDeviceID);
        EnSystemLog(EV9000_CMS_GET_TOP_DEVICE_CONFIG_ERROR, EV9000_LOG_LEVEL_ERROR, "Access topology physical device config info from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, cause=%s:%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"ID searched does not belong to this CMS", strDeviceID);
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_topology_phydevice_config_proc() exit---: DeviceID Not Belong To Mine CMSID:DeviceID=%s \r\n", strDeviceID);
        return -1;
    }

    /* ������˽ṹ����Ϣ */
    snprintf(strDeviceType, 16, "%u", EV9000_DEVICETYPE_SIPSERVER);
    local_ip = local_ip_get(default_eth_name_get());
    iRet = AddTopologyPhyDeviceInfo2DB(local_cms_id_get(), local_cms_name_get(), strDeviceType, local_ip, (char*)"1", local_cms_id_get(), (char*)"0", pRoute_Srv_dboper);

    /* ���ݲ�ѯ�������������ݿ⣬�ҵ���Ӧ���߼��豸����������Ϣ */
    strSQL.clear();
    strSQL = "select * from TopologyPhyDeviceConfig order by DeviceID asc";
    record_count = pRoute_Srv_dboper->DB_Select(strSQL.c_str(), 1);

    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_topology_phydevice_config_proc() record_count=%d \r\n", record_count);

    if (record_count < 0)
    {
        SystemLog(EV9000_CMS_GET_TOP_DEVICE_CONFIG_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�����Ļ�ȡ���������豸������Ϣ����, ��ѯ���ݿ�ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, ԭ��=%s:%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"��ѯ���ݿ�ʧ��", pRoute_Srv_dboper->GetLastDbErrorMsg());
        EnSystemLog(EV9000_CMS_GET_TOP_DEVICE_CONFIG_ERROR, EV9000_LOG_LEVEL_ERROR, "Access topology physical device config info from superior CMS search in database failed:Requester ID=%s, IPaddress=%s, port number=%d, cause=%s:%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"searcch in database failed", pRoute_Srv_dboper->GetLastDbErrorMsg());
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_topology_phydevice_config_proc() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_topology_phydevice_config_proc() ErrorMsg=%s\r\n", pRoute_Srv_dboper->GetLastDbErrorMsg());
        return -1;
    }

    if (record_count == 0)
    {
        /* �ظ���Ӧ,�齨��Ϣ */
        CPacket outPacket;
        DOMElement* AccNode = NULL;

        outPacket.SetRootTag("Response");

        AccNode = outPacket.CreateElement((char*)"CmdType");
        outPacket.SetElementValue(AccNode, (char*)"TopologyPhyDeviceConfig");

        AccNode = outPacket.CreateElement((char*)"SN");
        outPacket.SetElementValue(AccNode, strSN);

        AccNode = outPacket.CreateElement((char*)"DeviceID");
        outPacket.SetElementValue(AccNode, strDeviceID);

        AccNode = outPacket.CreateElement((char*)"Result");
        outPacket.SetElementValue(AccNode, (char*)"OK");

        AccNode = outPacket.CreateElement((char*)"SumNum");
        outPacket.SetElementValue(AccNode, (char*)"0");

        ListAccNode = outPacket.CreateElement((char*)"TopologyPhyDeviceConfigList");
        outPacket.SetElementAttr(ListAccNode, (char*)"Num", (char*)"0");

        /* ������Ӧ��Ϣ ���ϼ�CMS */
        i = SIP_SendMessage(NULL, local_cms_id_get(), caller_id, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

        if (i != 0)
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_topology_phydevice_config_proc() SIP_SendMessage Error:caller_id=%s, caller_ip=%s, caller_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
        }
        else
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_topology_phydevice_config_proc() SIP_SendMessage OK:caller_id=%s, caller_ip=%s, caller_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
        }

        SystemLog(EV9000_CMS_GET_TOP_DEVICE_CONFIG_ERROR, EV9000_LOG_LEVEL_WARNING, "�ϼ�CMS�����Ļ�ȡ���������豸������Ϣ����, ��ѯ���ݿ�ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, ԭ��=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"δ��ѯ�����ݿ��¼");
        EnSystemLog(EV9000_CMS_GET_TOP_DEVICE_CONFIG_ERROR, EV9000_LOG_LEVEL_WARNING, "Access topology physical device config info from superior CMS search in database failed:Requester ID=%s, IPaddress=%s, port number=%d, cause=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"Database record not found");
        DEBUG_TRACE(MODULE_ROUTE, LOG_WARN, "route_query_topology_phydevice_config_proc() exit---: No Record Count \r\n");
        return i;
    }

    CPacket* pOutPacket = NULL;

    /* ѭ���������ݿ⣬���������XML ���͸��ϼ�CMS */
    do
    {
        string strItemDeviceID = "";
        string strDeviceName = "";
        int iDeviceType = 0;
        string strDeviceIP = "";
        int iStatus = 0;
        string strCMSID = "";
        int iLinkType = 0;

        while_count++;

        if (while_count % 10000 == 0)
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_WARN, "route_query_topology_phydevice_config_proc() While Count=%d \r\n", while_count);
        }

        query_count++;

        /* ����XMLͷ�� */
        i = CreateTopologyPhyDeviceConfigResponseXMLHead(&pOutPacket, query_count, record_count, strSN, strDeviceID, &ListAccNode);

        /* �豸ID */
        strItemDeviceID.clear();
        pRoute_Srv_dboper->GetFieldValue("DeviceID", strItemDeviceID);

        /* �豸���� */
        strDeviceName.clear();
        pRoute_Srv_dboper->GetFieldValue("DeviceName", strDeviceName);

        /* �豸���� */
        pRoute_Srv_dboper->GetFieldValue("DeviceType", iDeviceType);

        /* �豸IP */
        strDeviceIP.clear();
        pRoute_Srv_dboper->GetFieldValue("DeviceIP", strDeviceIP);

        /* �豸״̬ */
        pRoute_Srv_dboper->GetFieldValue("Status", iStatus);

        /* ����CMSID */
        strCMSID.clear();
        pRoute_Srv_dboper->GetFieldValue("CMSID", strCMSID);

        /* �Ƿ�ͬ�� */
        pRoute_Srv_dboper->GetFieldValue("LinkType", iLinkType);

        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS�����Ļ�ȡ���������豸������Ϣ����, ��ѯ���ķ�����Ϣ:�豸ID=%s, �豸IP=%s", strItemDeviceID.c_str(), strDeviceIP.c_str());
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, " Access topology physical device configuration from Superior CMS, query to the grouping of information: device ID = % s, equipment IP = % s ", strItemDeviceID.c_str(), strDeviceIP.c_str());

        /* ����Item ֵ */
        i = AddTopologyPhyDeviceConfigToXMLItem(pOutPacket, ListAccNode, strItemDeviceID, strDeviceName, iDeviceType, strDeviceIP, iStatus, strCMSID, iLinkType);

        if ((query_count % MAX_TOPOLOGY_DEVICE_CONFIG_COUT_SEND == 0) || (query_count == record_count))
        {
            if (NULL != pOutPacket)
            {
                send_count++;
                /* ������Ӧ��Ϣ ���ϼ�CMS */
                i |= SIP_SendMessage(NULL, local_cms_id_get(), caller_id, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)pOutPacket->GetXml(NULL).c_str(), pOutPacket->GetXml(NULL).length());

                if (i != 0)
                {
                    SystemLog(EV9000_CMS_GET_TOP_DEVICE_CONFIG_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�����Ļ�ȡ���������豸������Ϣ����, ����Message��Ϣ���ϼ�CMSʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                    EnSystemLog(EV9000_CMS_GET_TOP_DEVICE_CONFIG_ERROR, EV9000_LOG_LEVEL_ERROR, "Access topology physical device configuration from Superior CMS, Send Message Message to superiors CMS failure: the superior CMS, ID = % s = % s IP address, port number = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                    DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_topology_phydevice_config_proc() SIP_SendMessage Error:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                }
                else
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS�����Ļ�ȡ���������豸������Ϣ����, ����Message��Ϣ���ϼ�CMS�ɹ�:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Access topology physical device configuration from Superior CMS, Send Message Message to superiors CMS success: the superior CMS, ID = % s = % s IP address, port number = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_topology_phydevice_config_proc() SIP_SendMessage OK:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                }

                delete pOutPacket;
                pOutPacket = NULL;
            }
        }
    }
    while (pRoute_Srv_dboper->MoveNext() >= 0);

    if (i == 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS�����Ļ�ȡ���������豸������Ϣ����ɹ�:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, ��¼��=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, record_count);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Access topology physical device configuration from Superior CMS, Send Message Message to superiors CMS success: the superior CMS, ID = % s = % s IP address, port number = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
    }
    else
    {
        SystemLog(EV9000_CMS_GET_TOP_DEVICE_CONFIG_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�����Ļ�ȡ���������豸������Ϣ����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, ԭ��=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"����SIP��Ӧ��Ϣʧ��");
        EnSystemLog(EV9000_CMS_GET_TOP_DEVICE_CONFIG_ERROR, EV9000_LOG_LEVEL_ERROR, "Access topology physical device configuration from Superior CMS, Send Message Message to superiors CMS failure: the superior CMS, ID = % s = % s IP address, port number = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
    }

    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_topology_phydevice_config_proc Exit--- \r\n");

    return 0;
}

/*****************************************************************************
 �� �� ��  : route_preset_info_response_proc
 ��������  : �ϼ�����CMS������Ԥ��λ��ѯ�������
 �������  : route_info_t* pRouteInfo
             char* caller_id
             char* callee_id
             CPacket& inPacket
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��11��26��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int route_preset_info_response_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, CPacket& inPacket)
{
    /* �豸Ԥ��λ��Ϣ, �������¼�CMS���ص����ݣ���Ҫת�����û�
      */
    int i = 0;
    int xml_pos = -1;
    char strSN[32] = {0};
    char strDeviceID[32] = {0};
    char strSumNum[16] = {0};
    string strPresetConfigListNum = "";
    int iSumNum = 0;
    int iPresetConfigListNum = 0;
    unsigned int transfer_xml_sn = 0;

    if (NULL == pRouteInfo)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_preset_info_response_proc() exit---: GBDevice Info Error \r\n");
        return -1;
    }

    if (NULL == caller_id || NULL == callee_id)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_preset_info_response_proc() exit---: Param Error \r\n");
        return -1;
    }

    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_preset_info_response_proc() caller_id=%s, callee_id=%s\r\n", caller_id, callee_id);
    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMSԤ��λ��Ϣ��ѯ��Ӧ��Ϣ:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Superior CMS preset info search response message:Superior CMS, ID = % s = % s IP address, port number = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);

    /* ȡ������*/
    inPacket.GetElementValue((char*)"SN", strSN);
    inPacket.GetElementValue((char*)"DeviceID", strDeviceID);
    inPacket.GetElementValue((char*)"SumNum", strSumNum);
    inPacket.GetElementAttr((char*)"PresetConfigList", (char*)"Num", strPresetConfigListNum);

    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO,  "route_preset_info_response_proc() \
    \r\n XML Para: \
    \r\n SN=%s \
    \r\n DeviceID=%s \
    \r\n SumNum=%s \
    \r\n PresetConfigList Num=%s \r\n ", strSN, strDeviceID, strSumNum, (char*)strPresetConfigListNum.c_str());

    if (strSumNum[0] == '\0')
    {
        SystemLog(EV9000_CMS_GET_PRESET_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMSԤ��λ��Ϣ��ѯ��Ӧ��Ϣ����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, ԭ��=û�л�ȡ��ǰ���ϱ���Ԥ��λ����", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
        EnSystemLog(EV9000_CMS_GET_PRESET_ERROR, EV9000_LOG_LEVEL_ERROR, "Superior CMS preset info search response message process failed:front-end device ID=%s, cause= did not get total preset number from front-end", caller_id);
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR,  "route_preset_info_response_proc() exit---: Get Sun Num Error \r\n");
        return -1;
    }

    iSumNum = osip_atoi(strSumNum);
    iPresetConfigListNum = osip_atoi((char*)strPresetConfigListNum.c_str());
    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE,  "route_preset_info_response_proc() SumNum=%d, RecordListNum=%d \r\n", iSumNum, iPresetConfigListNum);
    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMSԤ��λ��Ϣ��ѯ��Ӧ��Ϣ:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �ϱ���Ԥ��λ����=%d, �����ϱ���Ԥ��λ����=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, iSumNum, iPresetConfigListNum);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Superior CMS preset info search response message:front-end device ID=%s, total preset number reported=%d, number of presets reported this time=%d", caller_id, iSumNum, iPresetConfigListNum);

    /* �����Ƿ����û���ѯ�����ϼ���ѯ�� */
    transfer_xml_sn = strtoul(strSN, NULL, 10);
    xml_pos = transfer_xml_msg_find(XML_QUERY_GETPRESET, strDeviceID, transfer_xml_sn);

    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_preset_info_response_proc() transfer_xml_msg_find:Type=%d, DeviceID=%s, transfer_xml_sn=%d, xml_pos=%d \r\n", XML_QUERY_GETPRESET, strDeviceID, transfer_xml_sn, xml_pos);

    if (xml_pos >= 0)
    {
        i = transfer_xml_message_to_dest(xml_pos, iSumNum, iPresetConfigListNum, inPacket);

        if (i != 0)
        {
            SystemLog(EV9000_CMS_GET_PRESET_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMSԤ��λ��Ϣ��ѯ��Ӧ��Ϣ����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, ԭ��=����XML��SNת����Ŀ�ĵ�ʧ��, xml_pos=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, xml_pos);
            EnSystemLog(EV9000_CMS_GET_PRESET_ERROR, EV9000_LOG_LEVEL_ERROR, "Superior CMS preset info search response message process failed:front-end device ID=%s, cause=forward to destination accorrding to XML SN failed", caller_id);
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_preset_info_response_proc() transfer_xml_message_to_dest Error:device_id=%s\r\n", caller_id);
        }
        else
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "ǰ���豸Ԥ��λ��Ϣ��ѯ��Ӧ��Ϣ����ɹ�:ǰ���豸ID=%s, IP��ַ=%s, �˿ں�=%d,����XML��SNת����Ŀ�ĵ�, xml_pos=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, xml_pos);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "A front-end equipment preset information query response message processing success: the front-end device ID = % s, = % s IP address, port number = % d, according to the XML SN forwarded to destination, xml_pos = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, xml_pos);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_preset_info_response_proc() transfer_xml_message_to_dest OK:device_id=%s\r\n", caller_id);
        }
    }
    else
    {
        SystemLog(EV9000_CMS_GET_PRESET_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMSԤ��λ��Ϣ��ѯ��Ӧ��Ϣ����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, ԭ��=����XML��SN����Ŀ�ĵ�ʧ��, transfer_xml_sn=%d, strDeviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, transfer_xml_sn, strDeviceID);
        EnSystemLog(EV9000_CMS_GET_PRESET_ERROR, EV9000_LOG_LEVEL_ERROR, "Superior CMS preset info search response message process failed:front-end device ID=%s, cause=find destination accorrding to XML SN failed", caller_id);
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_preset_info_response_proc() transfer_xml_message_to_dest Error:device_id=%s\r\n", caller_id);
    }

    return 0;
}

/*****************************************************************************
 �� �� ��  : route_notify_alarm_proc
 ��������  : �ϼ�����CMS���͹����ı����¼�֪ͨ��Ϣ����
 �������  : route_info_t* pRouteInfo
             char* caller_id
             char* callee_id
             CPacket& inPacket
             DBOper* pRoute_Srv_dboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��5��20��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int route_notify_alarm_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, CPacket& inPacket, DBOper* pRoute_Srv_dboper)
{
    int i = 0;
    char strSN[32] = {0};
    char strDeviceID[32] = {0};
    char strAlarmPriority[32] = {0};
    char strAlarmTime[32] = {0};
    char strAlarmMethod[32] = {0};
    CPacket outPacket;
    DOMElement* AccNode = NULL;

    if (NULL == pRouteInfo)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_notify_alarm_proc() exit---: Route Info Error \r\n");
        return -1;
    }

    if (NULL == caller_id || NULL == callee_id || NULL == pRoute_Srv_dboper)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_notify_alarm_proc() exit---: Param Error \r\n");
        return -1;
    }

    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_notify_alarm_proc() Enter---: caller_id=%s, caller_ip=%s, caller_port=%d, callee_id=%s\r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);

    /* �����¼�֪ͨ�ͷַ�
          �������̼�9.4.2

          ������������ֶ�:
          <!-- �������ͣ�����֪ͨ����ѡ�� -->
          <element name="CmdType" fixed = "Alarm" />
          <!-- �������кţ���ѡ�� -->
          <element name="SN" type="integer" minInclusive value = "1" />
          <!-- �����豸���루��ѡ��-->
          <element name="DeviceID" type="tg:deviceIDType" />
          <!-- �������𣨱�ѡ����1Ϊһ�����飬2Ϊ�������飬3Ϊ�������飬4Ϊ�ļ�����-->
          <element name="AlarmPriority" type="string" />
          <!-- ������ʽ����ѡ����ȡֵ1Ϊ�绰������2Ϊ�豸������3Ϊ���ű�����4ΪGPS������5Ϊ��Ƶ������6
          Ϊ�豸���ϱ�����7��������-->
          <element name= "AlarmMethod" type= "string" />
          <!--����ʱ�䣨��ѡ��-->
          <element name= "AlarmTime" type="dateTime" />
          <!-- ��γ����Ϣ��ѡ -->
          <element name="Longitude" type="double" minOccurs= "0"/>
          <element name="Latitude" type="double" minOccurs= "0"/>
          <!-- ��չ��Ϣ���ɶ��� -->
          <element name= "Info" minOccurs= "0" maxOccurs="unbounded">
          <restriction base= "string">
          <maxLength value= "1024" />
          </restriction>
          </element>
      */

    /* ȡ�ñ������� */
    inPacket.GetElementValue((char*)"SN", strSN);/* �������к� */
    inPacket.GetElementValue((char*)"DeviceID", strDeviceID);/* �����豸���� */
    inPacket.GetElementValue((char*)"AlarmPriority", strAlarmPriority);/* ��������:1Ϊ һ�����飬2Ϊ�������飬3Ϊ�������飬4Ϊ�ļ����� */
    inPacket.GetElementValue((char*)"AlarmTime", strAlarmTime);/* ����ʱ�� */
    inPacket.GetElementValue((char*)"AlarmMethod", strAlarmMethod);/* ������ʽ: ȡֵ1Ϊ�绰������2Ϊ�豸������3Ϊ���ű�����4ΪGPS������5Ϊ��Ƶ������6Ϊ�豸���ϱ�����7�������� */

    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO,  "route_notify_alarm_proc() \
        \r\n XML Para: \
        \r\n SN=%s \
        \r\n DeviceID=%s \
        \r\n AlarmPriority=%s \
        \r\n AlarmTime=%s \
        \r\n AlarmMethod=%s \r\n", strSN, strDeviceID, strAlarmPriority, strAlarmTime, strAlarmMethod);

    /* �ظ���Ӧ */
    outPacket.SetRootTag("Response");
    AccNode = outPacket.CreateElement((char*)"CmdType");
    outPacket.SetElementValue(AccNode, (char*)"Alarm");
    AccNode = outPacket.CreateElement((char*)"SN");
    outPacket.SetElementValue(AccNode, strSN);
    AccNode = outPacket.CreateElement((char*)"DeviceID");
    outPacket.SetElementValue(AccNode, strDeviceID);
    AccNode = outPacket.CreateElement((char*)"Result");
    outPacket.SetElementValue(AccNode, (char*)"OK");

    /* ��Ӧ��Ϣ */
    i = SIP_SendMessage(NULL, local_cms_id_get(), caller_id, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)inPacket.GetXml(NULL).c_str(), inPacket.GetXml(NULL).length());

    if (i != 0)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_notify_alarm_proc() SIP_SendMessage Error:caller_id=%s, caller_ip=%s, caller_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
    }
    else
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_notify_alarm_proc() SIP_SendMessage OK:caller_id=%s, caller_ip=%s, caller_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
    }

    return i;
}

/*****************************************************************************
 �� �� ��  : route_notify_keep_alive_proc
 ��������  : �ϼ�����CMS���͹����ı�����Ϣ����
 �������  : route_info_t* pRouteInfo
             char* caller_id
             char* callee_id
             CPacket& inPacket
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��5��20��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int route_notify_keep_alive_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, CPacket& inPacket)
{
    /* �豸״̬��Ϣ������Ϣ
          �������̼�9.6.2
          Ŀǰû�д�·�ɹ����ĸ澯��Ҫ����

          ������������ֶ�:
          <!-- �������ͣ��豸״̬��Ϣ���ͣ���ѡ�� -->
          < element name="CmdType" fixed ="Keepalive" />
          <!-- �������кţ���ѡ�� -->
          <element name="SN" type="integer" minInclusive value = "1" />
          <!-- Դ�豸���豸���루��ѡ�� -->
          <element name="DeviceID" type="tg:deviceIDType" />
          <!-- �Ƿ�������������ѡ�� -->
          <element name="Status" type="tg:resultType" />
     */

    return 0;
}

/*****************************************************************************
 �� �� ��  : route_notify_cms_restart_proc
 ��������  : �ϼ�����CMS���͹�������������
 �������  : route_info_t* pRouteInfo
             char* caller_id
             char* callee_id
             CPacket& inPacket
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��7��5�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int route_notify_cms_restart_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, CPacket& inPacket)
{
    int iRet = 0;
    char strSN[32] = {0};
    char strCMSID[32] = {0};

    if (NULL == pRouteInfo)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_notify_cms_restart_proc() exit---: Route Info Error \r\n");
        return -1;
    }

    /* ȡ�� ����*/
    inPacket.GetElementValue((char*)"SN", strSN);
    inPacket.GetElementValue((char*)"CMSID", strCMSID);

    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO,  "route_notify_cms_restart_proc() \
    \r\n XML Para: \
    \r\n SN=%s \
    \r\n CMSID=%s \r\n", strSN, strCMSID);

    iRet = StopAllServiceTaskByCallerIPAndPort(pRouteInfo->server_ip, pRouteInfo->server_port);

    if (0 != iRet)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_notify_cms_restart_proc() StopAllServiceTaskByCallerIPAndPort Error:server_ip=%s, server_port=%d, iRet=%d \r\n", pRouteInfo->server_ip, pRouteInfo->server_port, iRet);
    }
    else
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_notify_cms_restart_proc() StopAllServiceTaskByCallerIPAndPort OK:server_ip=%s, server_port=%d, iRet=%d \r\n", pRouteInfo->server_ip, pRouteInfo->server_port, iRet);
    }

    /* �������ϼ�CMS������ */
    iRet = StopAllServiceTaskByCalleeIPAndPort(pRouteInfo->server_ip, pRouteInfo->server_port);

    if (0 != iRet)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_notify_cms_restart_proc() StopAllServiceTaskByCalleeIPAndPort Error:server_ip=%s, server_port=%d, iRet=%d \r\n", pRouteInfo->server_ip, pRouteInfo->server_port, iRet);
    }
    else
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_notify_cms_restart_proc() StopAllServiceTaskByCalleeIPAndPort OK:server_ip=%s, server_port=%d, iRet=%d \r\n", pRouteInfo->server_ip, pRouteInfo->server_port, iRet);
    }

    return iRet;
}

/*****************************************************************************
 �� �� ��  : route_notify_catalog_proc
 ��������  : �ϼ������ĵ�λ�仯֪ͨ��Ϣ����
 �������  : route_info_t* pRouteInfo
             char* caller_id
             char* callee_id
             CPacket& inPacket
             DBOper* pRoute_Srv_dboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��10��9��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int route_notify_catalog_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, CPacket& inPacket, DBOper* pRoute_Srv_dboper)
{
    int i = 0;
    int logic_device_pos = -1;
    char strSN[32] = {0};
    char strDeviceIndex[64] = {0};
    char strDeviceID[32] = {0};
    char strEvent[32] = {0};
    char strGBLogicDeviceID[32] = {0};
    char strSumNum[16] = {0};
    string strDeviceListNum = "";
    int iSumNum = 0;
    int iDeviceListNum = 0;
    int iItemNumCount = 0;
    char strName[MAX_128CHAR_STRING_LEN + 4] = {0};
    char strEnable[64] = {0};
    char strAlarmDeviceSubType[64] = {0};
    char strCtrlEnable[64] = {0};
    char strMicEnable[64] = {0};
    char strFrameCount[64] = {0};
    char strStreamCount[64] = {0};
    char strAlarmLengthOfTime[64] = {0};
    char strManufacturer[MAX_128CHAR_STRING_LEN + 4] = {0};
    char strModel[MAX_128CHAR_STRING_LEN + 4] = {0};
    char strOwner[MAX_128CHAR_STRING_LEN + 4] = {0};
    char strCivilCode[MAX_128CHAR_STRING_LEN + 4] = {0};
    char strBlock[MAX_128CHAR_STRING_LEN + 4] = {0};
    char strAddress[MAX_128CHAR_STRING_LEN + 4] = {0};
    char strParental[16] = {0};
    char strParentID[MAX_128CHAR_STRING_LEN + 4] = {0};
    char strSafetyWay[16] = {0};
    char strRegisterWay[16] = {0};
    char strCertNum[MAX_128CHAR_STRING_LEN + 4] = {0};
    char strCertifiable[16] = {0};
    char strErrCode[16] = {0};
    char strEndTime[MAX_128CHAR_STRING_LEN + 4] = {0};
    char strSecrecy[16] = {0};
    char strIPAddress[MAX_IP_LEN] = {0};
    char strPort[16] = {0};
    char strPassword[MAX_128CHAR_STRING_LEN + 4] = {0};
    char strStatus[16] = {0};
    char strLongitude[64] = {0};
    char strLatitude[64] = {0};
    char strMapLayer[MAX_128CHAR_STRING_LEN + 4] = {0};
    char strCMSID[MAX_ID_LEN + 4] = {0};
    char strAlarmPriority[32] = {0};/*2016.10.10 add for RCU*/
    char strGuard[32] = {0};/*2016.10.10 add for RCU*/
    char strValue[256] = {0};/*2016.10.10 add for RCU*/
    char strUnit[32] = {0};/*2016.10.10 add for RCU*/
    char strPTZType[64] = {0};
    GBLogicDevice_info_t* pOldGBLogicDeviceInfo = NULL;
    GBLogicDevice_info_t* pNewGBLogicDeviceInfo = NULL;
    DOMElement* ItemAccNode = NULL;
    char strDeviceType[4] = {0};
    int iPTZType = 0;
    char* pTmp = NULL;
    //char* tmp_civil_code;

    if (NULL == pRouteInfo)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_notify_catalog_proc() exit---: GBDevice Info Error \r\n");
        return -1;
    }

    if (NULL == caller_id || NULL == callee_id || NULL == pRoute_Srv_dboper)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_notify_catalog_proc() exit---: Param Error \r\n");
        return -1;
    }

    /* �����豸��Ϣ��ѯ��Ӧ��Ϣֱ��ת������������
          �������̼�9.5.2

          ������������ֶ�:
          <!-- �������ͣ��豸Ŀ¼��ѯ����ѡ�� -->
          <element name="CmdType" fixed ="Catalog" />
          <!-- �������кţ���ѡ�� -->
          <element name="SN" type="integer" minInclusive value = "1" />
          <!-- Ŀ���豸���豸���루��ѡ�� -->
          <element name="DeviceID" type="tg:deviceIDType" />
          <!-- �豸Ŀ¼���б�,num��ʾĿ¼����� -->
          <element name="DeviceList">
          <attribute name="Num" type="integer"/>
          <choice minOccurs= "0" maxOccurs= " unbounded " >
          <element name="Item" type="tg:itemType"/>
          </choice>
          </element>
          <!-- ��չ��Ϣ���ɶ��� -->
          <element name= "Info" minOccurs= "0" maxOccurs="unbounded">
          <restriction base= "string">
          <maxLength value= "1024" />
          </restriction>
          </element>
      */

    /* �߼��豸��Ϣһ�����豸ע��ɹ�֮��CMS��������Ĳ�ѯ */
    /* ȡ������*/
    inPacket.GetElementValue((char*)"SN", strSN);
    inPacket.GetElementValue((char*)"DeviceID", strDeviceID);
    inPacket.GetElementValue((char*)"SumNum", strSumNum);
    inPacket.GetElementAttr((char*)"DeviceList", (char*)"Num", strDeviceListNum);

    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO,  "route_notify_catalog_proc() \
    \r\n XML Para: \
    \r\n SN=%s \
    \r\n DeviceID=%s \
    \r\n SumNum=%s \
    \r\n DeviceList Num=%s \r\n ", strSN, strDeviceID, strSumNum, (char*)strDeviceListNum.c_str());

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS���͵�λ��Ϣ:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Superior CMS push point info :Superior CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID);

    if (strSumNum[0] == '\0')
    {
        SystemLog(EV9000_CMS_NOTIFY_CATALOG_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS���͵�λ��Ϣ����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, ԭ��=û�л�ȡ��ǰ���ϱ����߼�ͨ��Ŀ¼����", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
        EnSystemLog(EV9000_CMS_NOTIFY_CATALOG_ERROR, EV9000_LOG_LEVEL_ERROR, "Superior CMS push point info process failed:Superior CMS ID=%s, IP address=%s, port number=%d, cause=did not find totoal logic channel number reported by front-end.", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR,  "route_notify_catalog_proc() exit---: Get Sun Num Error \r\n");
        return -1;
    }

    if (0 != sstrcmp(strDeviceID, pRouteInfo->server_id))
    {
        SystemLog(EV9000_CMS_NOTIFY_CATALOG_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS���͵�λ��Ϣ����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, ԭ��=ǰ���豸��XML������豸ID����, XML�豸ID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID);
        EnSystemLog(EV9000_CMS_NOTIFY_CATALOG_ERROR, EV9000_LOG_LEVEL_ERROR, "Superior CMS push point info process failed:Superior CMS ID=%s, IP address=%s, port number=%d, cause=device ID in XML of front-end device incorrect, XML deviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID);
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR,  "route_query_catalog_response_proc() exit---: DeviceID Error:DeviceID=%s \r\n", strDeviceID);
        return -1;
    }

    /* ���豸��Ϣд���׼�߼��豸�� */
    iSumNum = osip_atoi(strSumNum);
    iDeviceListNum = osip_atoi((char*)strDeviceListNum.c_str());
    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE,  "route_notify_catalog_proc() SumNum=%d, DeviceListNum=%d \r\n", iSumNum, iDeviceListNum);
    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS���͵�λ��Ϣ����:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d:�ϱ����߼��豸����=%d, �����ϱ����߼��豸����=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, iSumNum, iDeviceListNum);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Superior CMS push point info process failed:superior CMS ID=%s, IPaddress=%s, port number=%d:total number of logic device reported=%d, number of logic device reported this time=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, iSumNum, iDeviceListNum);

    /* �鿴�����Ƿ��Ǳ�CMS ID */
    if (0 == strncmp(callee_id, local_cms_id_get(), 20))
    {
        if (iSumNum > 0)
        {
            if (iDeviceListNum <= 0)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR,  "route_notify_catalog_proc() exit---: DeviceListNum Error \r\n");
                return -1;
            }

            /* ��ȡ���е�Item ���� */
            ItemAccNode = inPacket.SearchElement((char*)"Item");

            if (!ItemAccNode)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR,  "route_notify_catalog_proc() exit---: Get Item Node Error \r\n");
                return -1;
            }

            inPacket.SetCurrentElement(ItemAccNode);

            while (ItemAccNode)
            {
                iItemNumCount++;

                if (iItemNumCount > iDeviceListNum)
                {
                    break;
                }

                memset(strDeviceIndex, 0, 64);
                inPacket.GetElementValue((char*)"ID", strDeviceIndex);

                memset(strGBLogicDeviceID, 0, 32);
                inPacket.GetElementValue((char*)"DeviceID", strGBLogicDeviceID);

                memset(strEvent, 0, 32);
                inPacket.GetElementValue((char*)"Event", strEvent);

                memset(strName, 0, MAX_128CHAR_STRING_LEN + 4);
                inPacket.GetElementValue((char*)"Name", strName);

                memset(strEnable, 0, 64);
                inPacket.GetElementValue((char*)"Enable", strEnable);

                memset(strCtrlEnable, 0, 64);
                inPacket.GetElementValue((char*)"CtrlEnable", strCtrlEnable);

                memset(strAlarmDeviceSubType, 0, 64);
                inPacket.GetElementValue((char*)"ChlType", strAlarmDeviceSubType);

                memset(strMicEnable, 0, 64);
                inPacket.GetElementValue((char*)"MicEnable", strMicEnable);

                memset(strFrameCount, 0, 64);
                inPacket.GetElementValue((char*)"FrameCount", strFrameCount);

                memset(strStreamCount, 0, 64);
                inPacket.GetElementValue((char*)"StreamCount", strStreamCount);

                memset(strAlarmLengthOfTime, 0, 64);
                inPacket.GetElementValue((char*)"AlarmLengthOfTime", strAlarmLengthOfTime);

                memset(strManufacturer, 0, MAX_128CHAR_STRING_LEN + 4);
                inPacket.GetElementValue((char*)"Manufacturer", strManufacturer);

                memset(strModel, 0, MAX_128CHAR_STRING_LEN + 4);
                inPacket.GetElementValue((char*)"Model", strModel);

                memset(strOwner, 0, MAX_128CHAR_STRING_LEN + 4);
                inPacket.GetElementValue((char*)"Owner", strOwner);

                memset(strCivilCode, 0, MAX_128CHAR_STRING_LEN + 4);
                inPacket.GetElementValue((char*)"CivilCode", strCivilCode);

                memset(strBlock, 0, MAX_128CHAR_STRING_LEN + 4);
                inPacket.GetElementValue((char*)"Block", strBlock);

                memset(strAddress, 0, MAX_128CHAR_STRING_LEN + 4);
                inPacket.GetElementValue((char*)"Address", strAddress);

                memset(strParental, 0, 16);
                inPacket.GetElementValue((char*)"Parental", strParental);

                memset(strParentID, 0, MAX_128CHAR_STRING_LEN + 4);
                inPacket.GetElementValue((char*)"ParentID", strParentID);

                memset(strSafetyWay, 0, 16);
                inPacket.GetElementValue((char*)"SafetyWay", strSafetyWay);

                memset(strRegisterWay, 0, 16);
                inPacket.GetElementValue((char*)"RegisterWay", strRegisterWay);

                memset(strCertNum, 0, MAX_128CHAR_STRING_LEN + 4);
                inPacket.GetElementValue((char*)"CertNum", strCertNum);

                memset(strCertifiable, 0, 16);
                inPacket.GetElementValue((char*)"Certifiable", strCertifiable);

                memset(strErrCode, 0, 16);
                inPacket.GetElementValue((char*)"ErrCode", strErrCode);

                memset(strEndTime, 0, MAX_128CHAR_STRING_LEN + 4);
                inPacket.GetElementValue((char*)"EndTime", strEndTime);

                memset(strSecrecy, 0, 16);
                inPacket.GetElementValue((char*)"Secrecy", strSecrecy);

                memset(strIPAddress, 0, 16);
                inPacket.GetElementValue((char*)"IPAddress", strIPAddress);

                memset(strPort, 0, 16);
                inPacket.GetElementValue((char*)"Port", strPort);

                memset(strPassword, 0, MAX_128CHAR_STRING_LEN + 4);
                inPacket.GetElementValue((char*)"Password", strPassword);

                memset(strStatus, 0, 16);
                inPacket.GetElementValue((char*)"Status", strStatus);

                memset(strLongitude, 0, 64);
                inPacket.GetElementValue((char*)"Longitude", strLongitude);

                memset(strLatitude, 0, 64);
                inPacket.GetElementValue((char*)"Latitude", strLatitude);

                memset(strMapLayer, 0, MAX_128CHAR_STRING_LEN + 4);
                inPacket.GetElementValue((char*)"MapLayer", strMapLayer);

                memset(strCMSID, 0, 24);
                inPacket.GetElementValue((char*)"CMSID", strCMSID);

#if 1 /*2016.10.10 add for RCU*/
                memset(strValue, 0, 256);
                inPacket.GetElementValue((char*)"Value", strValue);

                memset(strUnit, 0, 32);
                inPacket.GetElementValue((char*)"Unit", strUnit);

                memset(strGuard, 0, 32);
                inPacket.GetElementValue((char*)"Guard", strGuard);

                memset(strAlarmPriority, 0, 32);
                inPacket.GetElementValue((char*)"AlarmPriority", strAlarmPriority);
#endif/*2016.10.10 add for RCU*/

                DEBUG_TRACE(MODULE_ROUTE, LOG_INFO,  "route_notify_catalog_proc() ItemCount=%d, GBLogicDeviceID=%s, Name=%s, Event=%s, Status=%s \r\n", iItemNumCount, strGBLogicDeviceID, strName, strEvent, strStatus);
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS���͵�λ��Ϣ:�ϱ����߼��豸ID=%s, �߼���λ����=%s, �¼�=%s, ״̬=%s", strGBLogicDeviceID, strName, strEvent, strStatus);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Superior CMS push point info:Reported logic device ID = % s, logic point name = % s, event = % s, state = % s", strGBLogicDeviceID, strName, strEvent, strStatus);

                if (strGBLogicDeviceID[0] == '\0')
                {
                    DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR,  "route_notify_catalog_proc() exit---: GBLogic Device ID Error \r\n");
                    ItemAccNode = inPacket.SearchNextElement(true);
                    continue;
                }

                if (20 != strlen(strGBLogicDeviceID))
                {
                    SystemLog(EV9000_CMS_NOTIFY_CATALOG_ERROR, EV9000_LOG_LEVEL_WARNING, "�ϼ�CMS���͵�λ��Ϣ:�ϱ����߼��豸ID=%s, �߼���λ����=%s, �߼��豸ID���Ȳ��Ϸ�, �߼��豸ID����=%d", strGBLogicDeviceID, strName, strlen(strGBLogicDeviceID));
                    EnSystemLog(EV9000_CMS_NOTIFY_CATALOG_ERROR, EV9000_LOG_LEVEL_WARNING, "Superior CMS push point info:Logic device ID reported=%s, logic point name=%s, logice device ID length is not valid, logic device ID length=%d", strGBLogicDeviceID, strName, strlen(strGBLogicDeviceID));
                    DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR,  "route_notify_catalog_proc() exit---: GBLogic Device ID=%s, Length=%d Error \r\n", strGBLogicDeviceID, strlen(strGBLogicDeviceID));
                    ItemAccNode = inPacket.SearchNextElement(true);
                    continue;
                }

                if (sstrcmp(strEvent, "ADD") == 0 || sstrcmp(strEvent, "UPDATE") == 0)
                {
                    /* ���Ҿɵ��߼��豸,���ɵ��߼��豸�Ƿ��Ǳ������߼��豸 */
                    pOldGBLogicDeviceInfo = GBLogicDevice_info_find(strGBLogicDeviceID);

                    if (NULL != pOldGBLogicDeviceInfo)
                    {
                        /* ����·��ĵ�λ���ڱ���CMS�п����ҵ�������Ҳ�Ƿ�������ĵ�λ���򲻴���,Ĭ�ϻ��Ǳ���ĵ�λ */
                        if (0 == pOldGBLogicDeviceInfo->other_realm)
                        {
                            SystemLog(EV9000_CMS_NOTIFY_CATALOG_ERROR, EV9000_LOG_LEVEL_WARNING, "�ϼ�CMS���͵�λ��Ϣ:�ϱ����߼��豸ID=%s, �߼���λ����=%s, ���߼��豸�Ǳ���CMS���߼��豸", strGBLogicDeviceID, strName);
                            EnSystemLog(EV9000_CMS_NOTIFY_CATALOG_ERROR, EV9000_LOG_LEVEL_WARNING, "Superior CMS push point info:Logic device ID reported=%s, logic point name=%s, this logic device is parallel CMS logic device", strGBLogicDeviceID, strName);
                            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR,  "route_notify_catalog_proc() exit---: GBLogic Device ID=%s, is Local Realm LogicDevice \r\n", strGBLogicDeviceID);
                            ItemAccNode = inPacket.SearchNextElement(true);
                            continue;
                        }
                    }

                    /* ����Ϣд���µĽṹ */
                    i = GBLogicDevice_info_init(&pNewGBLogicDeviceInfo);

                    if (i != 0)
                    {
                        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR,  "route_notify_catalog_proc() exit---: GBLogicDevice Info Init Error \r\n");
                        ItemAccNode = inPacket.SearchNextElement(true);
                        continue;
                    }

                    /* ��λͳһ��� */
                    if (strGBLogicDeviceID[0] != '\0')
                    {
                        osip_strncpy(pNewGBLogicDeviceInfo->device_id, strGBLogicDeviceID, MAX_ID_LEN);
                    }

                    /* �߼��豸���� */
                    if (strDeviceIndex[0] != '\0')
                    {
                        pNewGBLogicDeviceInfo->id = strtoul(strDeviceIndex, NULL, 10);
                    }
                    else
                    {
                        if ('\0' != pNewGBLogicDeviceInfo->device_id[0])
                        {
                            pNewGBLogicDeviceInfo->id = crc32((unsigned char*)pNewGBLogicDeviceInfo->device_id, MAX_ID_LEN);
                        }
                    }

                    /* ������CMSͳһ��ţ�������¼�CMS�ϱ��ģ�����ڸ����ݣ�����Ǿ��������豸�����CMSID */
                    if (strCMSID[0] != '\0')
                    {
                        osip_strncpy(pNewGBLogicDeviceInfo->cms_id, strCMSID, MAX_ID_LEN);
                    }
                    else
                    {
                        osip_strncpy(pNewGBLogicDeviceInfo->cms_id, local_cms_id_get(), MAX_ID_LEN);
                    }

                    /* ��λ���� */
                    if (strName[0] != '\0')
                    {
                        osip_strncpy(pNewGBLogicDeviceInfo->device_name, strName, MAX_128CHAR_STRING_LEN);
                    }

                    /* �豸���� */
                    if ('\0' != pNewGBLogicDeviceInfo->device_id[0])
                    {
                        pTmp = &pNewGBLogicDeviceInfo->device_id[10];
                        osip_strncpy(strDeviceType, pTmp, 3);
                        pNewGBLogicDeviceInfo->device_type = osip_atoi(strDeviceType);
                    }
                    else
                    {
                        pNewGBLogicDeviceInfo->device_type = EV9000_DEVICETYPE_IPC;
                    }

                    /* �Ƿ����� */
                    if (strEnable[0] != '\0')
                    {
                        pNewGBLogicDeviceInfo->enable = osip_atoi(strEnable);
                    }
                    else
                    {
                        pNewGBLogicDeviceInfo->enable = 1;
                    }

                    /* �Ƿ�ɿ� */
                    if (strPTZType[0] != '\0') /* ����������չЭ��, 1-�����2-����3-�̶�ǹ���� 4-ң��ǹ�� */
                    {
                        iPTZType = osip_atoi(strPTZType);

                        pNewGBLogicDeviceInfo->ctrl_enable = iPTZType;
                    }
                    else
                    {
                        if (strCtrlEnable[0] != '\0')
                        {
                            if (0 == sstrcmp((char*)"Disable", strCtrlEnable))
                            {
                                pNewGBLogicDeviceInfo->ctrl_enable = 0;
                            }
                            else if (0 == sstrcmp((char*)"Enable", strCtrlEnable))
                            {
                                pNewGBLogicDeviceInfo->ctrl_enable = 1;
                            }
                            else
                            {
                                pNewGBLogicDeviceInfo->ctrl_enable = 0;
                            }
                        }
                        else
                        {
                            pNewGBLogicDeviceInfo->ctrl_enable = 0;
                        }
                    }

                    /* �Ƿ�֧�ֶԽ� */
                    if (strMicEnable[0] != '\0')
                    {
                        if (0 == sstrcmp((char*)"Disable", strMicEnable))
                        {
                            pNewGBLogicDeviceInfo->mic_enable = 0;
                        }
                        else if (0 == sstrcmp((char*)"Enable", strMicEnable))
                        {
                            pNewGBLogicDeviceInfo->mic_enable = 1;
                        }
                        else
                        {
                            pNewGBLogicDeviceInfo->mic_enable = 0;
                        }
                    }
                    else
                    {
                        pNewGBLogicDeviceInfo->mic_enable = 0;
                    }

                    /* ֡�� */
                    if (strFrameCount[0] != '\0')
                    {
                        pNewGBLogicDeviceInfo->frame_count = osip_atoi(strFrameCount);
                    }
                    else
                    {
                        pNewGBLogicDeviceInfo->frame_count = 25;
                    }

                    /* �Ƿ�֧�ֶ����� */
                    if (strStreamCount[0] != '\0')
                    {
                        pNewGBLogicDeviceInfo->stream_count = osip_atoi(strStreamCount);
                    }
                    else
                    {
                        if (pNewGBLogicDeviceInfo->device_type == EV9000_DEVICETYPE_CAMERA
                            || pNewGBLogicDeviceInfo->device_type == EV9000_DEVICETYPE_IPC
                            || pNewGBLogicDeviceInfo->device_type == EV9000_DEVICETYPE_SCREEN
                            || pNewGBLogicDeviceInfo->device_type == EV9000_DEVICETYPE_CODER)
                        {
                            pNewGBLogicDeviceInfo->stream_count = 1;
                        }
                        else
                        {
                            pNewGBLogicDeviceInfo->stream_count = 0;
                        }
                    }

                    /* ֡�� */
                    if (strAlarmLengthOfTime[0] != '\0')
                    {
                        pNewGBLogicDeviceInfo->alarm_duration = osip_atoi(strAlarmLengthOfTime);
                    }
                    else
                    {
                        pNewGBLogicDeviceInfo->alarm_duration = 0;
                    }

                    /* �Ƿ����������� */
                    pNewGBLogicDeviceInfo->other_realm = 1;

                    /* �豸������ */
                    if (strManufacturer[0] != '\0')
                    {
                        osip_strncpy(pNewGBLogicDeviceInfo->manufacturer, strManufacturer, MAX_128CHAR_STRING_LEN);
                    }

                    /* �豸�ͺ� */
                    if (strModel[0] != '\0')
                    {
                        osip_strncpy(pNewGBLogicDeviceInfo->model, strModel, MAX_128CHAR_STRING_LEN);
                    }

                    /* �豸���� */
                    if (strOwner[0] != '\0')
                    {
                        osip_strncpy(pNewGBLogicDeviceInfo->owner, strOwner, MAX_128CHAR_STRING_LEN);
                    }

                    /* ���� */
                    if (strBlock[0] != '\0')
                    {
                        osip_strncpy(pNewGBLogicDeviceInfo->block, strBlock, MAX_128CHAR_STRING_LEN);
                    }

                    /* ��װ��ַ */
                    if (strAddress[0] != '\0')
                    {
                        osip_strncpy(pNewGBLogicDeviceInfo->address, strAddress, MAX_128CHAR_STRING_LEN);
                    }

                    /* �Ƿ������豸 */
                    if (strParental[0] != '\0')
                    {
                        pNewGBLogicDeviceInfo->parental = osip_atoi(strParental);
                    }

                    /* ���豸/����/ϵͳID */
                    if (strParentID[0] != '\0')
                    {
                        osip_strncpy(pNewGBLogicDeviceInfo->parentID, strParentID, MAX_128CHAR_STRING_LEN);
                    }

                    /* ���ȫģʽ*/
                    if (strSafetyWay[0] != '\0')
                    {
                        pNewGBLogicDeviceInfo->safety_way = osip_atoi(strSafetyWay);
                    }

                    /* ע�᷽ʽ */
                    if (strRegisterWay[0] != '\0')
                    {
                        pNewGBLogicDeviceInfo->register_way = osip_atoi(strRegisterWay);
                    }

                    /* ֤�����к�*/
                    if (strCertNum[0] != '\0')
                    {
                        osip_strncpy(pNewGBLogicDeviceInfo->cert_num, strCertNum, MAX_128CHAR_STRING_LEN);
                    }

                    /* ֤����Ч��ʶ */
                    if (strCertifiable[0] != '\0')
                    {
                        pNewGBLogicDeviceInfo->certifiable = osip_atoi(strCertifiable);
                    }

                    /* ��Чԭ���� */
                    if (strErrCode[0] != '\0')
                    {
                        pNewGBLogicDeviceInfo->error_code = osip_atoi(strErrCode);
                    }

                    /* ֤����ֹ��Ч��*/
                    if (strEndTime[0] != '\0')
                    {
                        osip_strncpy(pNewGBLogicDeviceInfo->end_time, strEndTime, MAX_128CHAR_STRING_LEN);
                    }

                    /* �������� */
                    if (strSecrecy[0] != '\0')
                    {
                        pNewGBLogicDeviceInfo->secrecy = osip_atoi(strSecrecy);
                    }

                    /* IP��ַ*/
                    if (strIPAddress[0] != '\0')
                    {
                        osip_strncpy(pNewGBLogicDeviceInfo->ip_address, strIPAddress, MAX_IP_LEN);
                    }
                    else
                    {
                        osip_strncpy(pNewGBLogicDeviceInfo->ip_address, pRouteInfo->server_ip, MAX_IP_LEN);
                    }

                    /* �˿ں�*/
                    if (strPort[0] != '\0')
                    {
                        pNewGBLogicDeviceInfo->port = osip_atoi(strPort);
                    }
                    else
                    {
                        pNewGBLogicDeviceInfo->port = pRouteInfo->server_port;
                    }

                    /* ����*/
                    if (strPassword[0] != '\0')
                    {
                        osip_strncpy(pNewGBLogicDeviceInfo->password, strPassword, MAX_128CHAR_STRING_LEN);
                    }

                    /* ��λ״̬ */
                    if (strStatus[0] != '\0' && (0 == sstrcmp(strStatus, (char*)"ON") || 0 == sstrcmp(strStatus, (char*)"ONLINE")))
                    {
                        pNewGBLogicDeviceInfo->status = 1;
                    }
                    else if (strStatus[0] != '\0' && (0 == sstrcmp(strStatus, (char*)"OFF") || 0 == sstrcmp(strStatus, (char*)"OFFLINE")))
                    {
                        pNewGBLogicDeviceInfo->status = 0;
                        pNewGBLogicDeviceInfo->intelligent_status = INTELLIGENT_STATUS_NULL;
                        pNewGBLogicDeviceInfo->alarm_status = ALARM_STATUS_NULL;
                    }
                    else if (strStatus[0] != '\0' && 0 == sstrcmp(strStatus, (char*)"NOVIDEO"))
                    {
                        pNewGBLogicDeviceInfo->status = 2;
                    }
                    else if (strStatus[0] != '\0' && 0 == sstrcmp(strStatus, (char*)"VLOST"))
                    {
                        pNewGBLogicDeviceInfo->status = 2;
                    }
                    else if (strStatus[0] != '\0' && 0 == sstrcmp(strStatus, (char*)"INTELLIGENT"))
                    {
                        pNewGBLogicDeviceInfo->status = 1;
                        pNewGBLogicDeviceInfo->intelligent_status = INTELLIGENT_STATUS_ON;
                    }
                    else if (strStatus[0] != '\0' && 0 == sstrcmp(strStatus, (char*)"CLOSE"))
                    {
                        pNewGBLogicDeviceInfo->status = 1;
                        pNewGBLogicDeviceInfo->alarm_status = ALARM_STATUS_CLOSE;
                    }
                    else if (strStatus[0] != '\0' && 0 == sstrcmp(strStatus, (char*)"APART"))
                    {
                        pNewGBLogicDeviceInfo->status = 1;
                        pNewGBLogicDeviceInfo->alarm_status = ALARM_STATUS_APART;
                    }
                    else
                    {
                        pNewGBLogicDeviceInfo->status = 0;
                    }

                    /* ���� */
                    if (strLongitude[0] != '\0')
                    {
                        pNewGBLogicDeviceInfo->longitude = strtod(strLongitude, (char**) NULL);
                    }

                    /* γ�� */
                    if (strLatitude[0] != '\0')
                    {
                        pNewGBLogicDeviceInfo->latitude = strtod(strLatitude, (char**) NULL);
                    }

                    /* ����ͼ�� */
                    if (strMapLayer[0] != '\0')
                    {
                        osip_strncpy(pNewGBLogicDeviceInfo->map_layer, strMapLayer, MAX_128CHAR_STRING_LEN);
                    }

                    /* �����豸������ */
                    if (strAlarmDeviceSubType[0] != '\0')
                    {
                        pNewGBLogicDeviceInfo->alarm_device_sub_type = osip_atoi(strAlarmDeviceSubType);
                    }
                    else
                    {
                        pNewGBLogicDeviceInfo->alarm_device_sub_type = 0;
                    }

#if 1 /*2016.10.10 add for RCU*/

                    /* Guard*/
                    if (strGuard[0] != '\0')
                    {
                        pNewGBLogicDeviceInfo->guard_type = osip_atoi(strGuard);
                    }

                    /* �������� */
                    if (strAlarmPriority[0] != '\0')
                    {
                        pNewGBLogicDeviceInfo->AlarmPriority = osip_atoi(strAlarmPriority);
                    }

                    /* ��λ */
                    if (strUnit[0] != '\0')
                    {
                        osip_strncpy(pNewGBLogicDeviceInfo->Unit, strUnit, 32);
                    }

                    /* Value */
                    if (strValue[0] != '\0')
                    {
                        osip_strncpy(pNewGBLogicDeviceInfo->Value, strValue, 256);
                    }

#endif /*2016.10.10 add for RCU*/

                    /* ���ݾɵ��߼��豸�ж� */
                    if (NULL != pOldGBLogicDeviceInfo)
                    {
                        i = GBLogicDeviceCatalogInfoProcForRoute(pNewGBLogicDeviceInfo, pOldGBLogicDeviceInfo, pRoute_Srv_dboper);

                        if (i < 0)
                        {
                            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_notify_catalog_proc() GBLogicDeviceCatalogInfoProcForRoute Error:i=%d \r\n", i);
                        }
                        else
                        {
                            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE,  "route_notify_catalog_proc() GBLogicDeviceCatalogInfoProcForRoute OK:iRet=%d \r\n", i);
                        }

                        GBLogicDevice_info_free(pNewGBLogicDeviceInfo);
                        osip_free(pNewGBLogicDeviceInfo);
                        pNewGBLogicDeviceInfo = NULL;
                    }
                    else
                    {
                        /* ����߼��豸��Ϣ */
                        logic_device_pos = GBLogicDevice_info_add(pNewGBLogicDeviceInfo);

                        //DEBUG_TRACE(MODULE_ROUTE, LOG_INFO,  "route_query_catalog_response_proc() GBLogicDevice_info_add:logic_device_pos=%d \r\n", logic_device_pos);

                        if (logic_device_pos < 0)
                        {
                            GBLogicDevice_info_free(pNewGBLogicDeviceInfo);
                            osip_free(pNewGBLogicDeviceInfo);
                            pNewGBLogicDeviceInfo = NULL;
                            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR,  "route_notify_catalog_proc() exit---: GBLogicDevice Info Add Error \r\n");
                            ItemAccNode = inPacket.SearchNextElement(true);
                            continue;
                        }

                        /* ���������Ϣ���¼�CMS  */
                        i = SendNotifyCatalogToSubCMS(pNewGBLogicDeviceInfo, 0, pRoute_Srv_dboper);

                        if (i < 0)
                        {
                            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR,  "route_notify_catalog_proc() SendNotifyCatalogToSubCMS Error:iRet=%d \r\n", i);
                        }
                        else if (i > 0)
                        {
                            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE,  "route_notify_catalog_proc() SendNotifyCatalogToSubCMS OK:iRet=%d \r\n", i);
                        }

                        /* �������ݿ� */
                        i = AddGBLogicDeviceInfo2DBForRoute(strGBLogicDeviceID, pRoute_Srv_dboper);

                        if (i < 0)
                        {
                            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_notify_catalog_proc() AddGBLogicDeviceInfo2DBForRoute ERROR:device_id=%s, iRet=%d \r\n", strGBLogicDeviceID, i);
                        }
                        else
                        {
                            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_notify_catalog_proc() AddGBLogicDeviceInfo2DBForRoute OK:device_id=%s, iRet=%d \r\n", strGBLogicDeviceID, i);
                        }
                    }
                }
                else if (sstrcmp(strEvent, "DEL") == 0)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS���͵�λɾ����Ϣ:�ϱ����߼��豸ID=%s, �߼���λ����=%s", strGBLogicDeviceID, strName);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Superior CMS push point delete message:Logic device ID reported=%s, logic point name=%s", strGBLogicDeviceID, strName);
                    DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR,  "route_notify_catalog_proc() : DeviceID=%s, DeviceName=%s, Notify Event=DEL \r\n", strGBLogicDeviceID, strName);

                    /* ���Ҿɵ��߼��豸 */
                    pOldGBLogicDeviceInfo = GBLogicDevice_info_find(strGBLogicDeviceID);

                    if (NULL == pOldGBLogicDeviceInfo)
                    {
                        SystemLog(EV9000_CMS_NOTIFY_CATALOG_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS���͵�λɾ����Ϣ:�ϱ����߼��豸ID=%s, �߼���λ����=%s, û���ҵ���Ӧ���߼��豸��Ϣ", strGBLogicDeviceID, strName);
                        EnSystemLog(EV9000_CMS_NOTIFY_CATALOG_ERROR, EV9000_LOG_LEVEL_ERROR, "Superior CMS push point delete message:Logic device ID reported=%s, logic point name=%s, Corresponding logic device not found", strGBLogicDeviceID, strName);
                        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR,  "route_notify_catalog_proc() exit---: Find GBLogicDevice Info Error \r\n");
                        ItemAccNode = inPacket.SearchNextElement(true);
                        continue;
                    }

                    if (0 == pOldGBLogicDeviceInfo->enable)
                    {
                        SystemLog(EV9000_CMS_NOTIFY_CATALOG_ERROR, EV9000_LOG_LEVEL_WARNING, "�ϼ�CMS���͵�λɾ����Ϣ:�ϱ����߼��豸ID=%s, �߼���λ����=%s, �ϱ��ĵ�λ�Ѿ�������", strGBLogicDeviceID, strName);
                        EnSystemLog(EV9000_CMS_NOTIFY_CATALOG_ERROR, EV9000_LOG_LEVEL_WARNING, "Superior CMS push point delete message:Logic device ID reported=%s, logic point name=%s, point reported is disabled", strGBLogicDeviceID, strName);
                        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR,  "route_notify_catalog_proc() exit---: Find GBLogicDevice Info Error \r\n");
                        ItemAccNode = inPacket.SearchNextElement(true);
                        continue;
                    }

                    pOldGBLogicDeviceInfo->enable = 0;
                    pOldGBLogicDeviceInfo->status = 0;

                    /* �����豸״̬��Ϣ���ͻ��� */
                    i = SendDeviceStatusToAllClientUser(pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->status, pRoute_Srv_dboper);

                    if (i < 0)
                    {
                        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR,  "route_notify_catalog_proc() SendDeviceStatusToAllClientUser Error:iRet=%d \r\n", i);
                    }
                    else if (i > 0)
                    {
                        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE,  "route_notify_catalog_proc() SendDeviceStatusToAllClientUser OK:iRet=%d \r\n", i);
                    }

                    /* ����ɾ����Ϣ���¼�CMS  */
                    i = SendNotifyCatalogToSubCMS(pOldGBLogicDeviceInfo, 1, pRoute_Srv_dboper);

                    if (i < 0)
                    {
                        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_notify_catalog_proc() SendNotifyCatalogToSubCMS ERROR:device_id=%s, status=%d, iRet=%d \r\n", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->status, i);
                    }
                    else if (i > 0)
                    {
                        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_notify_catalog_proc() SendNotifyCatalogToSubCMS OK:device_id=%s, status=%d, iRet=%d \r\n", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->status, i);
                    }

                    /* ���͸澯��Ϣ���ͻ��� */
                    i = SendDeviceOffLineAlarmToAllClientUser(pOldGBLogicDeviceInfo->device_id);

                    if (0 != i)
                    {
                        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_notify_catalog_proc() SendDeviceOffLineAlarmToAllClientUser Error:device_id=%s, iRet=%d \r\n", pOldGBLogicDeviceInfo->device_id, i);
                    }
                    else
                    {
                        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_notify_catalog_proc() SendDeviceOffLineAlarmToAllClientUser OK:device_id=%s, iRet=%d \r\n", pOldGBLogicDeviceInfo->device_id, i);
                    }

                    /* ֹͣ�������� */
                    i = StopAllServiceTaskByLogicDeviceID(pOldGBLogicDeviceInfo->device_id);

                    if (0 != i)
                    {
                        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_notify_catalog_proc() StopAllServiceTaskByLogicDeviceID ERROR:device_id=%s, iRet=%d \r\n", pOldGBLogicDeviceInfo->device_id, i);
                    }
                    else
                    {
                        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE,  "route_notify_catalog_proc() StopAllServiceTaskByLogicDeviceID OK:device_id=%s, iRet=%d \r\n", pOldGBLogicDeviceInfo->device_id, i);
                    }

                    /* ֹͣ��Ƶ�Խ�ҵ�� */
                    i = StopAudioServiceTaskByLogicDeviceID(pOldGBLogicDeviceInfo->device_id);

                    if (0 != i)
                    {
                        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_notify_catalog_proc() StopAudioServiceTaskByLogicDeviceID Error:DeviceID=%s, iRet=%d \r\n", pOldGBLogicDeviceInfo->device_id, i);
                    }
                    else
                    {
                        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_notify_catalog_proc() StopAudioServiceTaskByLogicDeviceID OK:DeviceID=%s, iRet=%d \r\n", pOldGBLogicDeviceInfo->device_id, i);
                    }

                    /* �������ݿ� */
                    i = AddGBLogicDeviceInfo2DBForRoute(strGBLogicDeviceID, pRoute_Srv_dboper);

                    if (i < 0)
                    {
                        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_notify_catalog_proc() AddGBLogicDeviceInfo2DBForRoute ERROR:device_id=%s, iRet=%d \r\n", strGBLogicDeviceID, i);
                    }
                    else
                    {
                        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE,  "route_notify_catalog_proc() AddGBLogicDeviceInfo2DBForRoute OK:device_id=%s, iRet=%d \r\n", strGBLogicDeviceID, i);
                    }
                }
                else if (sstrcmp(strEvent, "ON") == 0)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS���͵�λ������Ϣ:�ϱ����߼��豸ID=%s, �߼���λ����=%s", strGBLogicDeviceID, strName);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Superior CMS push point online message:Logic device ID reported=%s, logic point name=%s", strGBLogicDeviceID, strName);
                    DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR,  "route_notify_catalog_proc() : DeviceID=%s, DeviceName=%s, Notify Event=OFF \r\n", strGBLogicDeviceID, strName);

                    /* ���Ҿɵ��߼��豸 */
                    pOldGBLogicDeviceInfo = GBLogicDevice_info_find(strGBLogicDeviceID);

                    if (NULL == pOldGBLogicDeviceInfo)
                    {
                        SystemLog(EV9000_CMS_NOTIFY_CATALOG_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS���͵�λ������Ϣ:�ϱ����߼��豸ID=%s, �߼���λ����=%s, û���ҵ���Ӧ���߼��豸��Ϣ", strGBLogicDeviceID, strName);
                        EnSystemLog(EV9000_CMS_NOTIFY_CATALOG_ERROR, EV9000_LOG_LEVEL_ERROR, "Superior CMS push point online message:Logic device ID reported=%s, logic point name=%s, Corresponding logic device not found", strGBLogicDeviceID, strName);
                        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR,  "route_notify_catalog_proc() exit---: Find GBLogicDevice Info Error \r\n");
                        ItemAccNode = inPacket.SearchNextElement(true);
                        continue;
                    }

                    if (0 == pOldGBLogicDeviceInfo->enable)
                    {
                        pOldGBLogicDeviceInfo->enable = 1;
                        pOldGBLogicDeviceInfo->status = 1;
                    }

                    if (pOldGBLogicDeviceInfo->status == 0)
                    {
                        pOldGBLogicDeviceInfo->status = 1;

                        /* �����豸״̬��Ϣ���ͻ��� */
                        i = SendDeviceStatusToAllClientUser(pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->status, pRoute_Srv_dboper);

                        if (i < 0)
                        {
                            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR,  "route_notify_catalog_proc() SendDeviceStatusToAllClientUser Error:iRet=%d \r\n", i);
                        }
                        else if (i > 0)
                        {
                            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE,  "route_notify_catalog_proc() SendDeviceStatusToAllClientUser OK:iRet=%d \r\n", i);
                        }

                        /* �����豸״̬��Ϣ���¼�CMS  */
                        i = SendDeviceStatusToSubCMS(pOldGBLogicDeviceInfo, pOldGBLogicDeviceInfo->status, pRoute_Srv_dboper);

                        if (i < 0)
                        {
                            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_notify_catalog_proc() SendDeviceStatusToSubCMS ERROR:device_id=%s, status=%d, iRet=%d \r\n", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->status, i);
                        }
                        else if (i > 0)
                        {
                            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_notify_catalog_proc() SendDeviceStatusToSubCMS OK:device_id=%s, status=%d, iRet=%d \r\n", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->status, i);
                        }

                        /* �������ݿ� */
                        i = AddGBLogicDeviceInfo2DBForRoute(strGBLogicDeviceID, pRoute_Srv_dboper);

                        if (i < 0)
                        {
                            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_notify_catalog_proc() AddGBLogicDeviceInfo2DBForRoute ERROR:device_id=%s, iRet=%d \r\n", strGBLogicDeviceID, i);
                        }
                        else
                        {
                            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_notify_catalog_proc() AddGBLogicDeviceInfo2DBForRoute OK:device_id=%s, iRet=%d \r\n", strGBLogicDeviceID, i);
                        }
                    }
                }
                else if (sstrcmp(strEvent, "OFF") == 0)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS���͵�λ������Ϣ:�ϱ����߼��豸ID=%s, �߼���λ����=%s", strGBLogicDeviceID, strName);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Superior CMS push point offline message:Logic device ID reported=%s, logic point name=%s", strGBLogicDeviceID, strName);
                    DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR,  "route_notify_catalog_proc() : DeviceID=%s, DeviceName=%s, Notify Event=OFF \r\n", strGBLogicDeviceID, strName);

                    /* ���Ҿɵ��߼��豸 */
                    pOldGBLogicDeviceInfo = GBLogicDevice_info_find(strGBLogicDeviceID);

                    if (NULL == pOldGBLogicDeviceInfo)
                    {
                        SystemLog(EV9000_CMS_NOTIFY_CATALOG_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS���͵�λ������Ϣ:�ϱ����߼��豸ID=%s, �߼���λ����=%s, û���ҵ���Ӧ���߼��豸��Ϣ", strGBLogicDeviceID, strName);
                        EnSystemLog(EV9000_CMS_NOTIFY_CATALOG_ERROR, EV9000_LOG_LEVEL_ERROR, "Superior CMS push point offline message:Logic device ID reported=%s, logic point name=%s, Corresponding logic device not found", strGBLogicDeviceID, strName);
                        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR,  "route_notify_catalog_proc() exit---: Find GBLogicDevice Info Error \r\n");
                        ItemAccNode = inPacket.SearchNextElement(true);
                        continue;
                    }

                    if (0 == pOldGBLogicDeviceInfo->enable)
                    {
                        SystemLog(EV9000_CMS_NOTIFY_CATALOG_ERROR, EV9000_LOG_LEVEL_WARNING, "�ϼ�CMS���͵�λ������Ϣ:�ϱ����߼��豸ID=%s, �߼���λ����=%s, �ϱ��ĵ�λ�Ѿ�������", strGBLogicDeviceID, strName);
                        EnSystemLog(EV9000_CMS_NOTIFY_CATALOG_ERROR, EV9000_LOG_LEVEL_WARNING, "Superior CMS push point offline message:Logic device ID reported=%s, logic point name=%s, point reported is disabled", strGBLogicDeviceID, strName);
                        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR,  "route_notify_catalog_proc() exit---: Find GBLogicDevice Info Error \r\n");
                        ItemAccNode = inPacket.SearchNextElement(true);
                        continue;
                    }

                    if (pOldGBLogicDeviceInfo->status == 1)
                    {
                        pOldGBLogicDeviceInfo->status = 0;

                        /* �����豸״̬��Ϣ���ͻ��� */
                        i = SendDeviceStatusToAllClientUser(pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->status, pRoute_Srv_dboper);

                        if (i < 0)
                        {
                            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR,  "route_notify_catalog_proc() SendDeviceStatusToAllClientUser Error:iRet=%d \r\n", i);
                        }
                        else if (i > 0)
                        {
                            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE,  "route_notify_catalog_proc() SendDeviceStatusToAllClientUser OK:iRet=%d \r\n", i);
                        }

                        /* �����豸״̬��Ϣ���¼�CMS  */
                        i = SendDeviceStatusToSubCMS(pOldGBLogicDeviceInfo, pOldGBLogicDeviceInfo->status, pRoute_Srv_dboper);

                        if (i < 0)
                        {
                            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_notify_catalog_proc() SendDeviceStatusToSubCMS ERROR:device_id=%s, status=%d, iRet=%d \r\n", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->status, i);
                        }
                        else if (i > 0)
                        {
                            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_notify_catalog_proc() SendDeviceStatusToSubCMS OK:device_id=%s, status=%d, iRet=%d \r\n", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->status, i);
                        }

                        /* ���͸澯��Ϣ���ͻ��� */
                        i = SendDeviceOffLineAlarmToAllClientUser(pOldGBLogicDeviceInfo->device_id);

                        if (0 != i)
                        {
                            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_notify_catalog_proc() SendDeviceOffLineAlarmToAllClientUser Error:device_id=%s, iRet=%d \r\n", pOldGBLogicDeviceInfo->device_id, i);
                        }
                        else
                        {
                            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_notify_catalog_proc() SendDeviceOffLineAlarmToAllClientUser OK:device_id=%s, iRet=%d \r\n", pOldGBLogicDeviceInfo->device_id, i);
                        }

                        /* ֹͣ�������� */
                        i = StopAllServiceTaskByLogicDeviceID(pOldGBLogicDeviceInfo->device_id);

                        if (0 != i)
                        {
                            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_notify_catalog_proc() StopAllServiceTaskByLogicDeviceID ERROR:device_id=%s, iRet=%d \r\n", pOldGBLogicDeviceInfo->device_id, i);
                        }
                        else
                        {
                            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_notify_catalog_proc() StopAllServiceTaskByLogicDeviceID OK:device_id=%s, iRet=%d \r\n", pOldGBLogicDeviceInfo->device_id, i);
                        }

                        /* ֹͣ��Ƶ�Խ�ҵ�� */
                        i = StopAudioServiceTaskByLogicDeviceID(pOldGBLogicDeviceInfo->device_id);

                        if (0 != i)
                        {
                            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_notify_catalog_proc() StopAudioServiceTaskByLogicDeviceID Error:DeviceID=%s, iRet=%d \r\n", pOldGBLogicDeviceInfo->device_id, i);
                        }
                        else
                        {
                            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_notify_catalog_proc() StopAudioServiceTaskByLogicDeviceID OK:DeviceID=%s, iRet=%d \r\n", pOldGBLogicDeviceInfo->device_id, i);
                        }

                        /* �������ݿ� */
                        i = AddGBLogicDeviceInfo2DBForRoute(strGBLogicDeviceID, pRoute_Srv_dboper);

                        if (i < 0)
                        {
                            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_notify_catalog_proc() AddGBLogicDeviceInfo2DBForRoute ERROR:device_id=%s, iRet=%d \r\n", strGBLogicDeviceID, i);
                        }
                        else
                        {
                            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_notify_catalog_proc() AddGBLogicDeviceInfo2DBForRoute OK:device_id=%s, iRet=%d \r\n", strGBLogicDeviceID, i);
                        }
                    }
                }
                else
                {
                    DEBUG_TRACE(MODULE_ROUTE, LOG_WARN, "route_notify_catalog_proc() Event Error:Event=%s \r\n", strEvent);
                }

                ItemAccNode = inPacket.SearchNextElement(true);
            }
        }
    }

    return i;
}

/*****************************************************************************
 �� �� ��  : route_notify_status_proc
 ��������  : �յ��ϼ��������豸״̬֪ͨ��Ϣ����
 �������  : route_info_t* pRouteInfo
             char* caller_id
             char* callee_id
             CPacket& inPacket
             DBOper* pRoute_Srv_dboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��10��12��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int route_notify_status_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, CPacket& inPacket, DBOper* pRoute_Srv_dboper)
{
    int i = 0;
    char strSN[32] = {0};
    char strDeviceID[32] = {0};
    char strStatus[32] = {0};
    int iOldDeviceStatus = 0;
    intelligent_status_t iOldIntelligentDeviceStatus = INTELLIGENT_STATUS_NULL;
    alarm_status_t iOldAlarmDeviceStatus = ALARM_STATUS_NULL;
    GBLogicDevice_info_t* pGBLogicDeviceInfo = NULL;

    if (NULL == pRouteInfo)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_notify_status_proc() exit---: Route Info Error \r\n");
        return -1;
    }

    if (NULL == caller_id || NULL == callee_id || NULL == pRoute_Srv_dboper)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_notify_status_proc() exit---: Param Error \r\n");
        return -1;
    }

    /* �豸״̬��Ϣ������Ϣ*/
    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS֪ͨ�豸״̬��Ϣ:�ϼ�CMS ID=%, IP��ַ=%s, �˿ں�=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Superior CMS notify device status message:Superior CMS, ID = % = % s IP address, port number = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);

    /* �鿴�����Ƿ��Ǳ�CMS ID */
    if (0 != strncmp(callee_id, local_cms_id_get(), 20))
    {
        SystemLog(EV9000_CMS_NOTIFY_STATUS_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS֪ͨ�豸״̬��Ϣ����ʧ��:�ϼ�CMS ID=%, IP��ַ=%s, �˿ں�=%d, ԭ��=CMS ID�����ڱ�CMS: CMS ID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);
        EnSystemLog(EV9000_CMS_NOTIFY_STATUS_ERROR, EV9000_LOG_LEVEL_ERROR, "Superior CMS notify device status message process failed:superior CMS ID=%, cause=CMS ID does not belong to localCMS: CMS ID=%s", caller_id, callee_id);
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR,  "route_notify_status_proc() exit---: Not Belong To Mine:callee_id=%s \r\n", callee_id);
        return -1;
    }

    /* ȡ������*/
    inPacket.GetElementValue((char*)"SN", strSN);
    inPacket.GetElementValue((char*)"DeviceID", strDeviceID);
    inPacket.GetElementValue((char*)"Status", strStatus);

    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO,  "route_notify_status_proc() \
    \r\n XML Para: \
    \r\n SN=%s, DeviceID=%s, Status=%s \r\n", strSN, strDeviceID, strStatus);

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS֪ͨ�豸״̬��Ϣ����:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ״̬=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, strStatus);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Superior CMS notify device status message process:Superior CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s, state = % s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, strStatus);

    pGBLogicDeviceInfo = GBLogicDevice_info_find(strDeviceID);

    if (NULL == pGBLogicDeviceInfo)
    {
        SystemLog(EV9000_CMS_NOTIFY_STATUS_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS֪ͨ�豸״̬��Ϣ����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, ԭ��=��ȡ�߼��豸��Ϣʧ��: DeviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID);
        EnSystemLog(EV9000_CMS_NOTIFY_STATUS_ERROR, EV9000_LOG_LEVEL_ERROR, "Superior CMS notify device status message process failed:superior CMS ID=%s, cause=access logice device info failed: DeviceID=%s", caller_id, strDeviceID);
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR,  "route_notify_status_proc() exit---: Find GBLogic Device Info Error:DeviceID=%s \r\n", strDeviceID);
        return -1;
    }

    iOldDeviceStatus = pGBLogicDeviceInfo->status;
    iOldIntelligentDeviceStatus = pGBLogicDeviceInfo->intelligent_status;
    iOldAlarmDeviceStatus = pGBLogicDeviceInfo->alarm_status;

    if (0 == sstrcmp(strStatus, (char*)"ON") || sstrcmp(strStatus, (char*)"ONLINE") == 0)
    {
        pGBLogicDeviceInfo->status = 1;
    }
    else if (0 == sstrcmp(strStatus, (char*)"OFF") || sstrcmp(strStatus, (char*)"OFFLINE") == 0)
    {
        pGBLogicDeviceInfo->status = 0;
        pGBLogicDeviceInfo->intelligent_status = INTELLIGENT_STATUS_NULL;
        pGBLogicDeviceInfo->alarm_status = ALARM_STATUS_NULL;
    }
    else if (sstrcmp(strStatus, (char*)"NOVIDEO") == 0)
    {
        pGBLogicDeviceInfo->status = 2;
    }
    else if (sstrcmp(strStatus, (char*)"VLOST") == 0)
    {
        pGBLogicDeviceInfo->status = 2;
    }
    else if (sstrcmp(strStatus, (char*)"INTELLIGENT") == 0)
    {
        pGBLogicDeviceInfo->status = 1;
        pGBLogicDeviceInfo->intelligent_status = INTELLIGENT_STATUS_ON;
    }
    else if (sstrcmp(strStatus, (char*)"CLOSE") == 0)
    {
        pGBLogicDeviceInfo->status = 1;
        pGBLogicDeviceInfo->alarm_status = ALARM_STATUS_CLOSE;
    }
    else if (sstrcmp(strStatus, (char*)"APART") == 0)
    {
        pGBLogicDeviceInfo->status = 1;
        pGBLogicDeviceInfo->alarm_status = ALARM_STATUS_APART;
    }

    /* �鿴״̬�Ƿ��б仯 */
    if (pGBLogicDeviceInfo->status != iOldDeviceStatus
        || iOldIntelligentDeviceStatus != pGBLogicDeviceInfo->intelligent_status
        || iOldAlarmDeviceStatus != pGBLogicDeviceInfo->alarm_status)
    {
        if (pGBLogicDeviceInfo->status != iOldDeviceStatus)
        {
            /* �������ݿ� */
            i = UpdateGBLogicDeviceRegStatus2DB(pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->status, pRoute_Srv_dboper);

            if (i < 0)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_notify_status_proc() UpdateGBLogicDeviceRegStatus2DB Error:device_id=%s, status=%d, i=%d \r\n", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->status, i);
            }
            else
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_notify_status_proc() UpdateGBLogicDeviceRegStatus2DB OK:device_id=%s, status=%d, i=%d \r\n", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->status, i);
            }
        }

        /* �����豸״̬��Ϣ���ͻ��� */
        if (1 == pGBLogicDeviceInfo->status && INTELLIGENT_STATUS_ON == pGBLogicDeviceInfo->intelligent_status)
        {
            i = SendDeviceStatusToAllClientUser(pGBLogicDeviceInfo->device_id, 4, pRoute_Srv_dboper);

            if (i < 0)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_notify_status_proc() SendDeviceStatusToAllClientUser Error:device_id=%s, status=%d, i=%d \r\n", pGBLogicDeviceInfo->device_id, 4, i);
            }
            else if (i > 0)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_notify_status_proc() SendDeviceStatusToAllClientUser OK:device_id=%s, status=%d, i=%d \r\n", pGBLogicDeviceInfo->device_id, 4, i);
            }

            /* �����豸״̬��Ϣ���¼�CMS  */
            i = SendDeviceStatusToSubCMS(pGBLogicDeviceInfo, 4, pRoute_Srv_dboper);

            if (i < 0)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_notify_status_proc() SendDeviceStatusToSubCMS ERROR:device_id=%s, status=%d, iRet=%d \r\n", pGBLogicDeviceInfo->device_id, 4, i);
            }
            else if (i > 0)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_notify_status_proc() SendDeviceStatusToSubCMS OK:device_id=%s, status=%d, iRet=%d \r\n", pGBLogicDeviceInfo->device_id, 4, i);
            }
        }
        else if (1 == pGBLogicDeviceInfo->status && ALARM_STATUS_CLOSE == pGBLogicDeviceInfo->alarm_status)
        {
            i = SendDeviceStatusToAllClientUser(pGBLogicDeviceInfo->device_id, 5, pRoute_Srv_dboper);

            if (i < 0)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_notify_status_proc() SendDeviceStatusToAllClientUser Error:device_id=%s, status=%d, i=%d \r\n", pGBLogicDeviceInfo->device_id, 5, i);
            }
            else if (i > 0)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_notify_status_proc() SendDeviceStatusToAllClientUser OK:device_id=%s, status=%d, i=%d \r\n", pGBLogicDeviceInfo->device_id, 5, i);
            }

            /* �����豸״̬��Ϣ���¼�CMS  */
            i = SendDeviceStatusToSubCMS(pGBLogicDeviceInfo, 5, pRoute_Srv_dboper);

            if (i < 0)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_notify_status_proc() SendDeviceStatusToSubCMS ERROR:device_id=%s, status=%d, iRet=%d \r\n", pGBLogicDeviceInfo->device_id, 4, i);
            }
            else if (i > 0)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_notify_status_proc() SendDeviceStatusToSubCMS OK:device_id=%s, status=%d, iRet=%d \r\n", pGBLogicDeviceInfo->device_id, 4, i);
            }
        }
        else if (1 == pGBLogicDeviceInfo->status && ALARM_STATUS_APART == pGBLogicDeviceInfo->alarm_status)
        {
            i = SendDeviceStatusToAllClientUser(pGBLogicDeviceInfo->device_id, 6, pRoute_Srv_dboper);

            if (i < 0)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_notify_status_proc() SendDeviceStatusToAllClientUser Error:device_id=%s, status=%d, i=%d \r\n", pGBLogicDeviceInfo->device_id, 6, i);
            }
            else if (i > 0)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_notify_status_proc() SendDeviceStatusToAllClientUser OK:device_id=%s, status=%d, i=%d \r\n", pGBLogicDeviceInfo->device_id, 6, i);
            }

            /* �����豸״̬��Ϣ���¼�CMS  */
            i = SendDeviceStatusToSubCMS(pGBLogicDeviceInfo, 6, pRoute_Srv_dboper);

            if (i < 0)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_notify_status_proc() SendDeviceStatusToSubCMS ERROR:device_id=%s, status=%d, iRet=%d \r\n", pGBLogicDeviceInfo->device_id, 4, i);
            }
            else if (i > 0)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_notify_status_proc() SendDeviceStatusToSubCMS OK:device_id=%s, status=%d, iRet=%d \r\n", pGBLogicDeviceInfo->device_id, 4, i);
            }
        }
        else
        {
            i = SendDeviceStatusToAllClientUser(pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->status, pRoute_Srv_dboper);

            if (i < 0)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_notify_status_proc() SendDeviceStatusToAllClientUser Error:device_id=%s, status=%d, i=%d \r\n", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->status, i);
            }
            else if (i > 0)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_notify_status_proc() SendDeviceStatusToAllClientUser OK:device_id=%s, status=%d, i=%d \r\n", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->status, i);
            }

            /* �����豸״̬��Ϣ���¼�CMS  */
            i = SendDeviceStatusToSubCMS(pGBLogicDeviceInfo, pGBLogicDeviceInfo->status, pRoute_Srv_dboper);

            if (i < 0)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_notify_status_proc() SendDeviceStatusToSubCMS ERROR:device_id=%s, status=%d, iRet=%d \r\n", pGBLogicDeviceInfo->device_id, 4, i);
            }
            else if (i > 0)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_notify_status_proc() SendDeviceStatusToSubCMS OK:device_id=%s, status=%d, iRet=%d \r\n", pGBLogicDeviceInfo->device_id, 4, i);
            }
        }

        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE,  "route_notify_status_proc() UpdateGBLogicDeviceRegStatus2DB:device_id=%s,status=%d\r\n", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->status);
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "�ϼ�CMS֪ͨ�豸״̬��Ϣ:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ��״̬=%d, ��״̬=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, pGBLogicDeviceInfo->device_id, iOldDeviceStatus, pGBLogicDeviceInfo->status);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Superior CMS notify device status message :logice device ID=%s, old state=%d, new state=%d", pGBLogicDeviceInfo->device_id, iOldDeviceStatus, pGBLogicDeviceInfo->status);
    }

    if (iOldDeviceStatus == 1 && (pGBLogicDeviceInfo->status == 0 || pGBLogicDeviceInfo->status == 2))
    {
        if (iOldDeviceStatus == 1 && pGBLogicDeviceInfo->status == 0)
        {
            /* ���͸澯��Ϣ���ͻ��� */
            i = SendDeviceOffLineAlarmToAllClientUser(pGBLogicDeviceInfo->device_id);

            if (0 != i)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_notify_status_proc() SendDeviceOffLineAlarmToAllClientUser Error:device_id=%s, iRet=%d \r\n", pGBLogicDeviceInfo->device_id, i);
            }
            else
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_notify_status_proc() SendDeviceOffLineAlarmToAllClientUser OK:device_id=%s, iRet=%d \r\n", pGBLogicDeviceInfo->device_id, i);
            }
        }
        else if (iOldDeviceStatus == 1 && pGBLogicDeviceInfo->status == 2)
        {
            /* ���͸澯��Ϣ���ͻ��� */
            i = SendDeviceNoStreamAlarmToAllClientUser(pGBLogicDeviceInfo->device_id);

            if (0 != i)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_notify_status_proc() SendDeviceNoStreamAlarmToAllClientUser Error:device_id=%s, iRet=%d \r\n", pGBLogicDeviceInfo->device_id, i);
            }
            else
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_notify_status_proc() SendDeviceNoStreamAlarmToAllClientUser OK:device_id=%s, iRet=%d \r\n", pGBLogicDeviceInfo->device_id, i);
            }
        }

        i = StopAllServiceTaskByLogicDeviceID(pGBLogicDeviceInfo->device_id);

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_notify_status_proc() StopAllServiceTaskByLogicDeviceID Error:device_id=%s, iRet=%d \r\n", pGBLogicDeviceInfo->device_id, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_notify_status_proc() StopAllServiceTaskByLogicDeviceID OK:device_id=%s, iRet=%d \r\n", pGBLogicDeviceInfo->device_id, i);
        }

        /* ֹͣ��Ƶ�Խ�ҵ�� */
        i = StopAudioServiceTaskByLogicDeviceID(pGBLogicDeviceInfo->device_id);

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_notify_status_proc() StopAudioServiceTaskByLogicDeviceID Error:DeviceID=%s, iRet=%d \r\n", pGBLogicDeviceInfo->device_id, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_notify_status_proc() StopAudioServiceTaskByLogicDeviceID OK:DeviceID=%s, iRet=%d \r\n", pGBLogicDeviceInfo->device_id, i);
        }
    }
    else if (INTELLIGENT_STATUS_ON == iOldIntelligentDeviceStatus && INTELLIGENT_STATUS_NULL == pGBLogicDeviceInfo->intelligent_status)
    {
        /* ���͸澯��Ϣ���ͻ��� */
        i = SendIntelligentDeviceOffLineAlarmToAllClientUser(pGBLogicDeviceInfo->device_id);

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_notify_status_proc() SendIntelligentDeviceOffLineAlarmToAllClientUser Error:device_id=%s, iRet=%d \r\n", pGBLogicDeviceInfo->device_id, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_notify_status_proc() SendIntelligentDeviceOffLineAlarmToAllClientUser OK:device_id=%s, iRet=%d \r\n", pGBLogicDeviceInfo->device_id, i);
        }

        i = StopAllServiceTaskByLogicDeviceIDAndStreamType(pGBLogicDeviceInfo->device_id, EV9000_STREAM_TYPE_INTELLIGENCE);

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_notify_status_proc() StopAllServiceTaskByLogicDeviceIDAndStreamType Error:device_id=%s, iRet=%d \r\n", pGBLogicDeviceInfo->device_id, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_notify_status_proc() StopAllServiceTaskByLogicDeviceIDAndStreamType OK:device_id=%s, iRet=%d \r\n", pGBLogicDeviceInfo->device_id, i);
        }
    }

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "ǰ���豸֪ͨ�豸״̬��Ϣ����ɹ�:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Front-end equipment inform status message processing success: the superior CMS, ID = % s = % s IP address, port number = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
    return 0;
}

/*****************************************************************************
 �� �� ��  : route_device_control_response_proc
 ��������  : �ϼ�����CMS���͹������豸������Ӧ����
 �������  : route_info_t* pRouteInfo
             char* caller_id
             char* callee_id
             CPacket& inPacket
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��5��20��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int route_device_control_response_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, CPacket& inPacket)
{
    int i = 0;
    int xml_pos = -1;
    char strSN[32] = {0};
    char strDeviceID[32] = {0};
    unsigned int transfer_xml_sn = 0;

    if (NULL == pRouteInfo)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_device_control_response_proc() exit---: GBDevice Info Error \r\n");
        return -1;
    }

    if (NULL == caller_id || NULL == callee_id)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_device_control_response_proc() exit---: Param Error \r\n");
        return -1;
    }

    /* �豸���Ƶ���Ӧ����ֱ��ת������������
          �������̼�9.3.2

          ������������ֶ�:
          <!-- �������ͣ��豸���ƣ���ѡ�� -->
          <element name="CmdType" fixed ="DeviceControl" />
          <!-- �������кţ���ѡ�� -->
          <element name="SN" type="integer" minInclusive value = "1" />
          <!-- Ŀ���豸���루��ѡ�� -->
          <element name="DeviceID" type="tg:deviceIDType" />
          <!-- ִ�н����־����ѡ�� -->
          <element name="Result" type="tg:resultType" />
      */

    inPacket.GetElementValue((char*)"SN", strSN);
    inPacket.GetElementValue((char*)"DeviceID", strDeviceID);

    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO,  "route_device_control_response_proc() \
    \r\n XML Para: \
    \r\n SN=%s \
    \r\n DeviceID=%s \r\n ", strSN, strDeviceID);

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS�豸������Ӧ��Ϣ:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Front-end equipment control response message: front-end equipment, ID = % s = % s IP address, port number = % d, logical device ID = % s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID);

    if (strSN[0] == '\0')
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_device_control_response_proc() exit---: Get XML SN Error\r\n");
        return -1;
    }

    if (strDeviceID[0] == '\0')
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "route_device_control_response_proc() exit---: Get XML DeviceID Error\r\n");
        return -1;
    }

    /* �����Ƿ����û���ѯ�����ϼ���ѯ�� */
    transfer_xml_sn = strtoul(strSN, NULL, 10);
    xml_pos = transfer_xml_msg_find(XML_CONTROL_DEVICECONTROL, strDeviceID, transfer_xml_sn);

    DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "route_device_control_response_proc() transfer_xml_msg_find:Type=%d, DeviceID=%s, transfer_xml_sn=%d, xml_pos=%d \r\n", XML_CONTROL_DEVICECONTROL, strDeviceID, transfer_xml_sn, xml_pos);

    if (xml_pos >= 0)
    {
        i = transfer_xml_message_to_dest2(xml_pos, inPacket);

        if (i != 0)
        {
            SystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS������Ӧ��Ϣ����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, ԭ��=����XML��SNת����Ŀ�ĵ�ʧ��, xml_pos=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, xml_pos);
            EnSystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "The front-end device control response message processing failed: the front-end device ID=%s, the reason = SN XML forwarding to the destination failed", strDeviceID);
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "route_device_control_response_proc() transfer_xml_message_to_dest Error:device_id=%s\r\n", caller_id);
        }
        else
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS������Ӧ��Ϣ����ɹ�:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d,����XML��SNת����Ŀ�ĵ�, xml_pos=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, xml_pos);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Front-end equipment control response message processing success: the front-end device ID = % s, = % s IP address, port number = % d, according to the XML SN forwarded to destination, xml_pos = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, xml_pos);
            DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "route_device_control_response_proc() transfer_xml_message_to_dest OK:device_id=%s\r\n", caller_id);
        }
    }
    else
    {
        SystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS������Ӧ��Ϣ����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, ԭ��=����XML��SN����Ŀ�ĵ�ʧ��, transfer_xml_sn=%d, strDeviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, transfer_xml_sn, strDeviceID);
        EnSystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "The front-end device control response message processing failed: the front-end device ID=%s, the reason = SN XML forwarding to the destination failed", strDeviceID);
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "route_device_control_response_proc() transfer_xml_message_to_dest Error:device_id=%s\r\n", caller_id);
    }

    return i;
}

/*****************************************************************************
 �� �� ��  : route_notify_alarm_response_proc
 ��������  : �ϼ�����CMS���͹����ĸ澯��Ϣ��Ӧ����
 �������  : route_info_t* pRouteInfo
             char* caller_id
             char* callee_id
             CPacket& inPacket
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��5��20��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int route_notify_alarm_response_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, CPacket& inPacket)
{
    /* ������Ӧ��Ϣ��ʱ����Ҫ����

      ������������ֶ�:
      <!-- �������ͣ�����֪ͨ����ѡ�� -->
      <element name="CmdType" fixed ="Alarm" />
      <!-- �������кţ���ѡ�� -->
      <element name="SN" type="integer" minInclusive value = "1" />
      <!-- Ŀ���豸���루��ѡ�� -->
      <element name="DeviceID" type="tg:deviceIDType" />
      <!-- ִ�н����־����ѡ�� -->
      <element name="Result" type="tg:resultType" />
      */

    return 0;
}

/*****************************************************************************
 �� �� ��  : route_query_catalog_response_proc
 ��������  : �ϼ�����CMS���͹�����Ŀ¼��ѯ��Ӧ��Ϣ����
 �������  : route_info_t* pRouteInfo
             char* caller_id
             char* callee_id
             CPacket& inPacket
             DBOper* pRoute_Srv_dboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��5��21��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int route_query_catalog_response_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, CPacket& inPacket, DBOper* pRoute_Srv_dboper)
{
    int i = 0;
    int logic_device_pos = -1;
    char strSN[32] = {0};
    char strDeviceIndex[64] = {0};
    char strDeviceID[32] = {0};
    char strGBLogicDeviceID[32] = {0};
    char strSumNum[16] = {0};
    string strDeviceListNum = "";
    int iSumNum = 0;
    int iDeviceListNum = 0;
    int iItemNumCount = 0;
    char strName[MAX_128CHAR_STRING_LEN + 4] = {0};
    char strEnable[64] = {0};
    char strAlarmDeviceSubType[64] = {0};
    char strCtrlEnable[64] = {0};
    char strMicEnable[64] = {0};
    char strFrameCount[64] = {0};
    char strStreamCount[64] = {0};
    char strAlarmLengthOfTime[64] = {0};
    char strManufacturer[MAX_128CHAR_STRING_LEN + 4] = {0};
    char strModel[MAX_128CHAR_STRING_LEN + 4] = {0};
    char strOwner[MAX_128CHAR_STRING_LEN + 4] = {0};
    char strCivilCode[MAX_128CHAR_STRING_LEN + 4] = {0};
    char strBlock[MAX_128CHAR_STRING_LEN + 4] = {0};
    char strAddress[MAX_128CHAR_STRING_LEN + 4] = {0};
    char strParental[16] = {0};
    char strParentID[MAX_128CHAR_STRING_LEN + 4] = {0};
    char strSafetyWay[16] = {0};
    char strRegisterWay[16] = {0};
    char strCertNum[MAX_128CHAR_STRING_LEN + 4] = {0};
    char strCertifiable[16] = {0};
    char strErrCode[16] = {0};
    char strEndTime[MAX_128CHAR_STRING_LEN + 4] = {0};
    char strSecrecy[16] = {0};
    char strIPAddress[MAX_IP_LEN] = {0};
    char strPort[16] = {0};
    char strPassword[MAX_128CHAR_STRING_LEN + 4] = {0};
    char strStatus[16] = {0};
    char strLongitude[64] = {0};
    char strLatitude[64] = {0};
    char strMapLayer[MAX_128CHAR_STRING_LEN + 4] = {0};
    char strCMSID[MAX_ID_LEN + 4] = {0};
    char strAlarmPriority[32] = {0};/*2016.10.10 add for RCU*/
    char strGuard[32] = {0};/*2016.10.10 add for RCU*/
    char strValue[256] = {0};/*2016.10.10 add for RCU*/
    char strUnit[32] = {0};/*2016.10.10 add for RCU*/
    char strPTZType[64] = {0};
    GBLogicDevice_info_t* pOldGBLogicDeviceInfo = NULL;
    GBLogicDevice_info_t* pNewGBLogicDeviceInfo = NULL;
    DOMElement* ItemAccNode = NULL;
    char strDeviceType[4] = {0};
    int iPTZType = 0;
    char* pTmp = NULL;
    //char* tmp_civil_code;

    if (NULL == pRouteInfo)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_query_catalog_response_proc() exit---: Route Info Error \r\n");
        return -1;
    }

    if (NULL == caller_id || NULL == callee_id || NULL == pRoute_Srv_dboper)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_query_catalog_response_proc() exit---: Param Error \r\n");
        return -1;
    }

    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_catalog_response_proc() Enter---: caller_id=%s, callee_id=%s\r\n", caller_id, callee_id);
    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMSĿ¼��ѯ��Ӧ��Ϣ:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Superior CMS directory search response process :Superior CMS, ID = % s = % s IP address, port number = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);

    /* �����豸��Ϣ��ѯ��Ӧ��Ϣֱ��ת������������
          �������̼�9.5.2

          ������������ֶ�:
          <!-- �������ͣ��豸Ŀ¼��ѯ����ѡ�� -->
          <element name="CmdType" fixed ="Catalog" />
          <!-- �������кţ���ѡ�� -->
          <element name="SN" type="integer" minInclusive value = "1" />
          <!-- Ŀ���豸���豸���루��ѡ�� -->
          <element name="DeviceID" type="tg:deviceIDType" />
          <!-- �豸Ŀ¼���б�,num��ʾĿ¼����� -->
          <element name="DeviceList">
          <attribute name="Num" type="integer"/>
          <choice minOccurs= "0" maxOccurs= " unbounded " >
          <element name="Item" type="tg:itemType"/>
          </choice>
          </element>
          <!-- ��չ��Ϣ���ɶ��� -->
          <element name= "Info" minOccurs= "0" maxOccurs="unbounded">
          <restriction base= "string">
          <maxLength value= "1024" />
          </restriction>
          </element>
      */

    /* �߼��豸��Ϣһ�����豸ע��ɹ�֮��CMS��������Ĳ�ѯ */
    /* ȡ������*/
    inPacket.GetElementValue((char*)"SN", strSN);
    inPacket.GetElementValue((char*)"DeviceID", strDeviceID);
    inPacket.GetElementValue((char*)"SumNum", strSumNum);
    inPacket.GetElementAttr((char*)"DeviceList", (char*)"Num", strDeviceListNum);

    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO,  "route_query_catalog_response_proc() \
    \r\n XML Para: \
    \r\n SN=%s \
    \r\n DeviceID=%s \
    \r\n SumNum=%s \
    \r\n DeviceList Num=%s \r\n ", strSN, strDeviceID, strSumNum, (char*)strDeviceListNum.c_str());

    if (strSumNum[0] == '\0')
    {
        SystemLog(EV9000_CMS_NOTIFY_CATALOG_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMSĿ¼��ѯ��Ӧ��Ϣ����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, ԭ��=û�л�ȡ��ǰ���ϱ����߼�ͨ��Ŀ¼����", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
        EnSystemLog(EV9000_CMS_NOTIFY_CATALOG_ERROR, EV9000_LOG_LEVEL_ERROR, "Superior CMS directory search response process failed:front device ID=%s, cause=did not find totoal logic channel number reported by front-end.", caller_id);
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR,  "route_query_catalog_response_proc() exit---: Get Sun Num Error \r\n");
        return -1;
    }

    if (0 != sstrcmp(strDeviceID, pRouteInfo->server_id))
    {
        SystemLog(EV9000_CMS_NOTIFY_CATALOG_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMSĿ¼��ѯ��Ӧ��Ϣ����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, ԭ��=ǰ���豸��XML������豸ID����, XML�豸ID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID);
        EnSystemLog(EV9000_CMS_NOTIFY_CATALOG_ERROR, EV9000_LOG_LEVEL_ERROR, "Superior CMS directory search response process failed:front device ID=%s, cause=device ID in XML of front-end device incorrect, XMLdeviceID=%s", caller_id, strDeviceID);
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR,  "route_query_catalog_response_proc() exit---: DeviceID Error:DeviceID=%s \r\n", strDeviceID);
        return -1;
    }

    /* ���豸��Ϣд���׼�߼��豸�� */
    iSumNum = osip_atoi(strSumNum);
    iDeviceListNum = osip_atoi((char*)strDeviceListNum.c_str());
    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE,  "route_query_catalog_response_proc() SumNum=%d, DeviceListNum=%d \r\n", iSumNum, iDeviceListNum);
    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMSĿ¼��ѯ��Ӧ��Ϣ:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �ϱ����߼��豸����=%d, �����ϱ����߼��豸����=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, iSumNum, iDeviceListNum);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Superior CMS directory search response :total number of logic device reported=%d, number of logic device reported this time=%d", iSumNum, iDeviceListNum);

    if (iSumNum > 0)
    {
        if (iDeviceListNum <= 0)
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR,  "route_query_catalog_response_proc() exit---: DeviceListNum Error \r\n");
            return -1;
        }

        if (pRouteInfo->CataLogNumCount == 0) /* ��һ���յ����͵�λ��Ϣ����SN��ֵ */
        {
            pRouteInfo->CataLogSN = osip_atoi(strSN);

            /* �Ƚ���·��������߼�ͨ������Ϊɾ��״̬ */
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMSĿ¼��ѯ��Ӧ��Ϣ:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, ���ø�CMS����������ͨ��ɾ����ʶ, ��������=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, iSumNum);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Superior CMS push directory message: set all channel channel delete notification that this CMS belongs to, superior CMS ID=%s, IP=%s, total push number=%d", pRouteInfo->server_id, pRouteInfo->server_ip, iSumNum);
            DEBUG_TRACE(MODULE_ROUTE, LOG_INFO,  "route_query_catalog_response_proc() Proc Catalog Item Begin---:server_id=%s, SumNum=%d \r\n", pRouteInfo->server_id, iSumNum);
            i = SetGBLogicDeviceInfoDelFlagForRoute(pRouteInfo);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE,  "route_query_catalog_response_proc() SetGBLogicDeviceInfoDelFlagForRoute:i=%d \r\n", i);

            /* �ϱ�����ͳ�� */
            pRouteInfo->CataLogNumCount += iDeviceListNum;
            DEBUG_TRACE(MODULE_ROUTE, LOG_INFO,  "route_query_catalog_response_proc() DeviceListNum=%d, RouteInfo:server_id=%s,CataLogNumCount=%d \r\n", iDeviceListNum, pRouteInfo->server_id, pRouteInfo->CataLogNumCount);
        }
        else
        {
            if (pRouteInfo->CataLogSN == (unsigned int)osip_atoi(strSN))
            {
                /* �ϱ�����ͳ�� */
                pRouteInfo->CataLogNumCount += iDeviceListNum;
                DEBUG_TRACE(MODULE_ROUTE, LOG_INFO,  "route_query_catalog_response_proc() DeviceListNum=%d, RouteInfo:server_id=%s,CataLogNumCount=%d \r\n", iDeviceListNum, pRouteInfo->server_id, pRouteInfo->CataLogNumCount);
            }
        }

        /* ��ȡ���е�Item ���� */
        ItemAccNode = inPacket.SearchElement((char*)"Item");

        if (!ItemAccNode)
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR,  "route_query_catalog_response_proc() exit---: Get Item Node Error \r\n");
            return -1;
        }

        inPacket.SetCurrentElement(ItemAccNode);

        while (ItemAccNode)
        {
            iItemNumCount++;

            if (iItemNumCount > iDeviceListNum)
            {
                break;
            }

            memset(strDeviceIndex, 0, 64);
            inPacket.GetElementValue((char*)"ID", strDeviceIndex);

            memset(strGBLogicDeviceID, 0, 32);
            inPacket.GetElementValue((char*)"DeviceID", strGBLogicDeviceID);

            memset(strName, 0, MAX_128CHAR_STRING_LEN + 4);
            inPacket.GetElementValue((char*)"Name", strName);

            memset(strEnable, 0, 64);
            inPacket.GetElementValue((char*)"Enable", strEnable);

            memset(strAlarmDeviceSubType, 0, 64);
            inPacket.GetElementValue((char*)"ChlType", strAlarmDeviceSubType);

            memset(strCtrlEnable, 0, 64);
            inPacket.GetElementValue((char*)"CtrlEnable", strCtrlEnable);

            memset(strMicEnable, 0, 64);
            inPacket.GetElementValue((char*)"MicEnable", strMicEnable);

            memset(strFrameCount, 0, 64);
            inPacket.GetElementValue((char*)"FrameCount", strFrameCount);

            memset(strStreamCount, 0, 64);
            inPacket.GetElementValue((char*)"StreamCount", strStreamCount);

            memset(strAlarmLengthOfTime, 0, 64);
            inPacket.GetElementValue((char*)"AlarmLengthOfTime", strAlarmLengthOfTime);

            memset(strManufacturer, 0, MAX_128CHAR_STRING_LEN + 4);
            inPacket.GetElementValue((char*)"Manufacturer", strManufacturer);

            memset(strModel, 0, MAX_128CHAR_STRING_LEN + 4);
            inPacket.GetElementValue((char*)"Model", strModel);

            memset(strOwner, 0, MAX_128CHAR_STRING_LEN + 4);
            inPacket.GetElementValue((char*)"Owner", strOwner);

            memset(strCivilCode, 0, MAX_128CHAR_STRING_LEN + 4);
            inPacket.GetElementValue((char*)"CivilCode", strCivilCode);

            memset(strBlock, 0, MAX_128CHAR_STRING_LEN + 4);
            inPacket.GetElementValue((char*)"Block", strBlock);

            memset(strAddress, 0, MAX_128CHAR_STRING_LEN + 4);
            inPacket.GetElementValue((char*)"Address", strAddress);

            memset(strParental, 0, 16);
            inPacket.GetElementValue((char*)"Parental", strParental);

            memset(strParentID, 0, MAX_128CHAR_STRING_LEN + 4);
            inPacket.GetElementValue((char*)"ParentID", strParentID);

            memset(strSafetyWay, 0, 16);
            inPacket.GetElementValue((char*)"SafetyWay", strSafetyWay);

            memset(strRegisterWay, 0, 16);
            inPacket.GetElementValue((char*)"RegisterWay", strRegisterWay);

            memset(strCertNum, 0, MAX_128CHAR_STRING_LEN + 4);
            inPacket.GetElementValue((char*)"CertNum", strCertNum);

            memset(strCertifiable, 0, 16);
            inPacket.GetElementValue((char*)"Certifiable", strCertifiable);

            memset(strErrCode, 0, 16);
            inPacket.GetElementValue((char*)"ErrCode", strErrCode);

            memset(strEndTime, 0, MAX_128CHAR_STRING_LEN + 4);
            inPacket.GetElementValue((char*)"EndTime", strEndTime);

            memset(strSecrecy, 0, 16);
            inPacket.GetElementValue((char*)"Secrecy", strSecrecy);

            memset(strIPAddress, 0, 16);
            inPacket.GetElementValue((char*)"IPAddress", strIPAddress);

            memset(strPort, 0, 16);
            inPacket.GetElementValue((char*)"Port", strPort);

            memset(strPassword, 0, MAX_128CHAR_STRING_LEN + 4);
            inPacket.GetElementValue((char*)"Password", strPassword);

            memset(strStatus, 0, 16);
            inPacket.GetElementValue((char*)"Status", strStatus);

            memset(strLongitude, 0, 64);
            inPacket.GetElementValue((char*)"Longitude", strLongitude);

            memset(strLatitude, 0, 64);
            inPacket.GetElementValue((char*)"Latitude", strLatitude);

            memset(strMapLayer, 0, MAX_128CHAR_STRING_LEN + 4);
            inPacket.GetElementValue((char*)"MapLayer", strMapLayer);

            memset(strCMSID, 0, 24);
            inPacket.GetElementValue((char*)"CMSID", strCMSID);

#if 1 /*2016.10.10 add for RCU*/
            memset(strValue, 0, 256);
            inPacket.GetElementValue((char*)"Value", strValue);

            memset(strUnit, 0, 32);
            inPacket.GetElementValue((char*)"Unit", strUnit);

            memset(strGuard, 0, 32);
            inPacket.GetElementValue((char*)"Guard", strGuard);

            memset(strAlarmPriority, 0, 32);
            inPacket.GetElementValue((char*)"AlarmPriority", strAlarmPriority);
#endif/*2016.10.10 add for RCU*/

            DEBUG_TRACE(MODULE_ROUTE, LOG_INFO,  "route_query_catalog_response_proc() ItemCount=%d, GBLogicDeviceID=%s, Name=%s, Status=%s \r\n", iItemNumCount, strGBLogicDeviceID, strName, strStatus);
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMSĿ¼��ѯ��Ӧ��Ϣ:�ϼ�:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �ϱ����߼��豸ID=%s, �߼���λ����=%s, ״̬=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strGBLogicDeviceID, strName, strStatus);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Superior CMS directory query response message: superior: the superior CMS, ID = % s = % s IP address and port number = % d, report the logical device ID = % s, logical point name = % s, state = % s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strGBLogicDeviceID, strName, strStatus);

            if (strGBLogicDeviceID[0] == '\0')
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR,  "route_query_catalog_response_proc() exit---: GBLogic Device ID Error \r\n");
                ItemAccNode = inPacket.SearchNextElement(true);
                continue;
            }

            if (20 != strlen(strGBLogicDeviceID))
            {
                SystemLog(EV9000_CMS_NOTIFY_CATALOG_ERROR, EV9000_LOG_LEVEL_WARNING, "�ϼ�CMSĿ¼��ѯ��Ӧ��Ϣ:�ϼ�:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �ϱ����߼��豸ID=%s, �߼���λ����=%s, �߼��豸ID���Ȳ��Ϸ�, �߼��豸ID����=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strGBLogicDeviceID, strName, strlen(strGBLogicDeviceID));
                EnSystemLog(EV9000_CMS_NOTIFY_CATALOG_ERROR, EV9000_LOG_LEVEL_WARNING, "Superior CMS directory search response :Logic device ID reported=%s, logic point name=%s, logice device ID length is not valid, logic device ID length=%d", strGBLogicDeviceID, strName, strlen(strGBLogicDeviceID));
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR,  "route_query_catalog_response_proc() exit---: GBLogic Device ID=%s, Length=%d Error \r\n", strGBLogicDeviceID, strlen(strGBLogicDeviceID));
                ItemAccNode = inPacket.SearchNextElement(true);
                continue;
            }

            /* ���Ҿɵ��߼��豸,���ɵ��߼��豸�Ƿ��Ǳ������߼��豸 */
            pOldGBLogicDeviceInfo = GBLogicDevice_info_find(strGBLogicDeviceID);

            if (NULL != pOldGBLogicDeviceInfo)
            {
                if (1 == pOldGBLogicDeviceInfo->del_mark)
                {
                    /* �Ƴ�ɾ����ʶ */
                    pOldGBLogicDeviceInfo->del_mark = 0;
                }

                /* ����·��ĵ�λ���ڱ���CMS�п����ҵ�������Ҳ�Ƿ�������ĵ�λ���򲻴���,Ĭ�ϻ��Ǳ���ĵ�λ */
                if (0 == pOldGBLogicDeviceInfo->other_realm)
                {
                    SystemLog(EV9000_CMS_NOTIFY_CATALOG_ERROR, EV9000_LOG_LEVEL_WARNING, "�ϼ�CMSĿ¼��ѯ��Ӧ��Ϣ:�ϼ�:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �ϱ����߼��豸ID=%s, �߼���λ����=%s, ���߼��豸�Ǳ���CMS���߼��豸", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strGBLogicDeviceID, strName);
                    EnSystemLog(EV9000_CMS_NOTIFY_CATALOG_ERROR, EV9000_LOG_LEVEL_WARNING, "Superior CMS push point info:Logic device ID reported=%s, logic point name=%s, this logic device is parallel CMS logic device", strGBLogicDeviceID, strName);
                    DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR,  "route_query_catalog_response_proc() exit---: GBLogic Device ID=%s, is Local Realm LogicDevice \r\n", strGBLogicDeviceID);
                    ItemAccNode = inPacket.SearchNextElement(true);
                    continue;
                }
            }

            i = GBLogicDevice_info_init(&pNewGBLogicDeviceInfo);

            if (i != 0)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR,  "route_query_catalog_response_proc() exit---: GBLogicDevice Info Init Error \r\n");
                ItemAccNode = inPacket.SearchNextElement(true);
                continue;
            }

            /* ����Ϣд���µĽṹ */
            /* ��λͳһ��� */
            if (strGBLogicDeviceID[0] != '\0')
            {
                osip_strncpy(pNewGBLogicDeviceInfo->device_id, strGBLogicDeviceID, MAX_ID_LEN);
            }

            /* �߼��豸���� */
            if (strDeviceIndex[0] != '\0')
            {
                pNewGBLogicDeviceInfo->id = strtoul(strDeviceIndex, NULL, 10);
            }
            else
            {
                if ('\0' != pNewGBLogicDeviceInfo->device_id[0])
                {
                    pNewGBLogicDeviceInfo->id = crc32((unsigned char*)pNewGBLogicDeviceInfo->device_id, MAX_ID_LEN);
                }
            }

            /* ������CMSͳһ��ţ�������¼�CMS�ϱ��ģ�����ڸ����ݣ�����Ǿ��������豸�����CMSID */
            if (strCMSID[0] != '\0')
            {
                osip_strncpy(pNewGBLogicDeviceInfo->cms_id, strCMSID, MAX_ID_LEN);
            }
            else
            {
                osip_strncpy(pNewGBLogicDeviceInfo->cms_id, local_cms_id_get(), MAX_ID_LEN);
            }

            /* ��λ���� */
            if (strName[0] != '\0')
            {
                osip_strncpy(pNewGBLogicDeviceInfo->device_name, strName, MAX_128CHAR_STRING_LEN);
            }

            /* �豸���� */
            if ('\0' != pNewGBLogicDeviceInfo->device_id[0])
            {
                pTmp = &pNewGBLogicDeviceInfo->device_id[10];
                osip_strncpy(strDeviceType, pTmp, 3);
                pNewGBLogicDeviceInfo->device_type = osip_atoi(strDeviceType);
            }
            else
            {
                pNewGBLogicDeviceInfo->device_type = EV9000_DEVICETYPE_IPC;
            }

            /* �Ƿ����� */
            if (strEnable[0] != '\0')
            {
                pNewGBLogicDeviceInfo->enable = osip_atoi(strEnable);
            }
            else
            {
                pNewGBLogicDeviceInfo->enable = 1;
            }

            /* �Ƿ�ɿ� */
            if (strPTZType[0] != '\0') /* ����������չЭ��, 1-�����2-����3-�̶�ǹ���� 4-ң��ǹ�� */
            {
                iPTZType = osip_atoi(strPTZType);

                pNewGBLogicDeviceInfo->ctrl_enable = iPTZType;
            }
            else
            {
                if (strCtrlEnable[0] != '\0')
                {
                    if (0 == sstrcmp((char*)"Disable", strCtrlEnable))
                    {
                        pNewGBLogicDeviceInfo->ctrl_enable = 0;
                    }
                    else if (0 == sstrcmp((char*)"Enable", strCtrlEnable))
                    {
                        pNewGBLogicDeviceInfo->ctrl_enable = 1;
                    }
                    else
                    {
                        pNewGBLogicDeviceInfo->ctrl_enable = 0;
                    }
                }
                else
                {
                    pNewGBLogicDeviceInfo->ctrl_enable = 0;
                }
            }

            /* �Ƿ�֧�ֶԽ� */
            if (strMicEnable[0] != '\0')
            {
                if (0 == sstrcmp((char*)"Disable", strMicEnable))
                {
                    pNewGBLogicDeviceInfo->mic_enable = 0;
                }
                else if (0 == sstrcmp((char*)"Enable", strMicEnable))
                {
                    pNewGBLogicDeviceInfo->mic_enable = 1;
                }
                else
                {
                    pNewGBLogicDeviceInfo->mic_enable = 0;
                }
            }
            else
            {
                pNewGBLogicDeviceInfo->mic_enable = 0;
            }

            /* ֡�� */
            if (strFrameCount[0] != '\0')
            {
                pNewGBLogicDeviceInfo->frame_count = osip_atoi(strFrameCount);
            }
            else
            {
                pNewGBLogicDeviceInfo->frame_count = 25;
            }

            /* �Ƿ�֧�ֶ����� */
            if (strStreamCount[0] != '\0')
            {
                pNewGBLogicDeviceInfo->stream_count = osip_atoi(strStreamCount);
            }
            else
            {
                if (pNewGBLogicDeviceInfo->device_type == EV9000_DEVICETYPE_CAMERA
                    || pNewGBLogicDeviceInfo->device_type == EV9000_DEVICETYPE_IPC
                    || pNewGBLogicDeviceInfo->device_type == EV9000_DEVICETYPE_SCREEN
                    || pNewGBLogicDeviceInfo->device_type == EV9000_DEVICETYPE_CODER)
                {
                    pNewGBLogicDeviceInfo->stream_count = 1;
                }
                else
                {
                    pNewGBLogicDeviceInfo->stream_count = 0;
                }
            }

            /* ֡�� */
            if (strAlarmLengthOfTime[0] != '\0')
            {
                pNewGBLogicDeviceInfo->alarm_duration = osip_atoi(strAlarmLengthOfTime);
            }
            else
            {
                pNewGBLogicDeviceInfo->alarm_duration = 0;
            }

            /* �Ƿ����������� */
            pNewGBLogicDeviceInfo->other_realm = 1;

            /* �豸������ */
            if (strManufacturer[0] != '\0')
            {
                osip_strncpy(pNewGBLogicDeviceInfo->manufacturer, strManufacturer, MAX_128CHAR_STRING_LEN);
            }

            /* �豸�ͺ� */
            if (strModel[0] != '\0')
            {
                osip_strncpy(pNewGBLogicDeviceInfo->model, strModel, MAX_128CHAR_STRING_LEN);
            }

            /* �豸���� */
            if (strOwner[0] != '\0')
            {
                osip_strncpy(pNewGBLogicDeviceInfo->owner, strOwner, MAX_128CHAR_STRING_LEN);
            }

            /* ���� */
            if (strBlock[0] != '\0')
            {
                osip_strncpy(pNewGBLogicDeviceInfo->block, strBlock, MAX_128CHAR_STRING_LEN);
            }

            /* ��װ��ַ */
            if (strAddress[0] != '\0')
            {
                osip_strncpy(pNewGBLogicDeviceInfo->address, strAddress, MAX_128CHAR_STRING_LEN);
            }

            /* �Ƿ������豸 */
            if (strParental[0] != '\0')
            {
                pNewGBLogicDeviceInfo->parental = osip_atoi(strParental);
            }

            /* ���豸/����/ϵͳID */
            if (strParentID[0] != '\0')
            {
                osip_strncpy(pNewGBLogicDeviceInfo->parentID, strParentID, MAX_128CHAR_STRING_LEN);
            }

            /* ���ȫģʽ*/
            if (strSafetyWay[0] != '\0')
            {
                pNewGBLogicDeviceInfo->safety_way = osip_atoi(strSafetyWay);
            }

            /* ע�᷽ʽ */
            if (strRegisterWay[0] != '\0')
            {
                pNewGBLogicDeviceInfo->register_way = osip_atoi(strRegisterWay);
            }

            /* ֤�����к�*/
            if (strCertNum[0] != '\0')
            {
                osip_strncpy(pNewGBLogicDeviceInfo->cert_num, strCertNum, MAX_128CHAR_STRING_LEN);
            }

            /* ֤����Ч��ʶ */
            if (strCertifiable[0] != '\0')
            {
                pNewGBLogicDeviceInfo->certifiable = osip_atoi(strCertifiable);
            }

            /* ��Чԭ���� */
            if (strErrCode[0] != '\0')
            {
                pNewGBLogicDeviceInfo->error_code = osip_atoi(strErrCode);
            }

            /* ֤����ֹ��Ч��*/
            if (strEndTime[0] != '\0')
            {
                osip_strncpy(pNewGBLogicDeviceInfo->end_time, strEndTime, MAX_128CHAR_STRING_LEN);
            }

            /* �������� */
            if (strSecrecy[0] != '\0')
            {
                pNewGBLogicDeviceInfo->secrecy = osip_atoi(strSecrecy);
            }

            /* IP��ַ*/
            if (strIPAddress[0] != '\0')
            {
                osip_strncpy(pNewGBLogicDeviceInfo->ip_address, strIPAddress, MAX_IP_LEN);
            }
            else
            {
                osip_strncpy(pNewGBLogicDeviceInfo->ip_address, pRouteInfo->server_ip, MAX_IP_LEN);
            }

            /* �˿ں�*/
            if (strPort[0] != '\0')
            {
                pNewGBLogicDeviceInfo->port = osip_atoi(strPort);
            }
            else
            {
                pNewGBLogicDeviceInfo->port = pRouteInfo->server_port;
            }

            /* ����*/
            if (strPassword[0] != '\0')
            {
                osip_strncpy(pNewGBLogicDeviceInfo->password, strPassword, MAX_128CHAR_STRING_LEN);
            }

            /* ��λ״̬ */
            if (strStatus[0] != '\0' && (0 == sstrcmp(strStatus, (char*)"ON") || 0 == sstrcmp(strStatus, (char*)"ONLINE")))
            {
                pNewGBLogicDeviceInfo->status = 1;
            }
            else if (strStatus[0] != '\0' && (0 == sstrcmp(strStatus, (char*)"OFF") || 0 == sstrcmp(strStatus, (char*)"OFFLINE")))
            {
                pNewGBLogicDeviceInfo->status = 0;
                pNewGBLogicDeviceInfo->intelligent_status = INTELLIGENT_STATUS_NULL;
                pNewGBLogicDeviceInfo->alarm_status = ALARM_STATUS_NULL;
            }
            else if (strStatus[0] != '\0' && 0 == sstrcmp(strStatus, (char*)"NOVIDEO"))
            {
                pNewGBLogicDeviceInfo->status = 2;
            }
            else if (strStatus[0] != '\0' && 0 == sstrcmp(strStatus, (char*)"VLOST"))
            {
                pNewGBLogicDeviceInfo->status = 2;
            }
            else if (strStatus[0] != '\0' && 0 == sstrcmp(strStatus, (char*)"INTELLIGENT"))
            {
                pNewGBLogicDeviceInfo->status = 1;
                pNewGBLogicDeviceInfo->intelligent_status = INTELLIGENT_STATUS_ON;
            }
            else if (strStatus[0] != '\0' && 0 == sstrcmp(strStatus, (char*)"CLOSE"))
            {
                pNewGBLogicDeviceInfo->status = 1;
                pNewGBLogicDeviceInfo->alarm_status = ALARM_STATUS_CLOSE;
            }
            else if (strStatus[0] != '\0' && 0 == sstrcmp(strStatus, (char*)"APART"))
            {
                pNewGBLogicDeviceInfo->status = 1;
                pNewGBLogicDeviceInfo->alarm_status = ALARM_STATUS_APART;
            }
            else
            {
                pNewGBLogicDeviceInfo->status = 0;
            }

            /* ���� */
            if (strLongitude[0] != '\0')
            {
                pNewGBLogicDeviceInfo->longitude = strtod(strLongitude, (char**) NULL);
            }

            /* γ�� */
            if (strLatitude[0] != '\0')
            {
                pNewGBLogicDeviceInfo->latitude = strtod(strLatitude, (char**) NULL);
            }

            /* ����ͼ�� */
            if (strMapLayer[0] != '\0')
            {
                osip_strncpy(pNewGBLogicDeviceInfo->map_layer, strMapLayer, MAX_128CHAR_STRING_LEN);
            }

            /* �����豸������ */
            if (strAlarmDeviceSubType[0] != '\0')
            {
                pNewGBLogicDeviceInfo->alarm_device_sub_type = osip_atoi(strAlarmDeviceSubType);
            }
            else
            {
                pNewGBLogicDeviceInfo->alarm_device_sub_type = 0;
            }

#if 1 /*2016.10.10 add for RCU*/

            /* Guard*/
            if (strGuard[0] != '\0')
            {
                pNewGBLogicDeviceInfo->guard_type = osip_atoi(strGuard);
            }

            /* �������� */
            if (strAlarmPriority[0] != '\0')
            {
                pNewGBLogicDeviceInfo->AlarmPriority = osip_atoi(strAlarmPriority);
            }

            /* ��λ */
            if (strUnit[0] != '\0')
            {
                osip_strncpy(pNewGBLogicDeviceInfo->Unit, strUnit, 32);
            }

            /* Value */
            if (strValue[0] != '\0')
            {
                osip_strncpy(pNewGBLogicDeviceInfo->Value, strValue, 256);
            }

#endif /*2016.10.10 add for RCU*/

            /* ���ݾɵ��߼��豸�Ƿ�����ж� */
            if (NULL != pOldGBLogicDeviceInfo)
            {
                i = GBLogicDeviceCatalogInfoProcForRoute(pNewGBLogicDeviceInfo, pOldGBLogicDeviceInfo, pRoute_Srv_dboper);

                if (i < 0)
                {
                    DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_catalog_response_proc() GBLogicDeviceCatalogInfoProcForRoute Error:i=%d \r\n", i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE,  "route_query_catalog_response_proc() GBLogicDeviceCatalogInfoProcForRoute OK:iRet=%d \r\n", i);
                }

                GBLogicDevice_info_free(pNewGBLogicDeviceInfo);
                osip_free(pNewGBLogicDeviceInfo);
                pNewGBLogicDeviceInfo = NULL;
            }
            else
            {
                /* ����߼��豸��Ϣ */
                logic_device_pos = GBLogicDevice_info_add(pNewGBLogicDeviceInfo);

                //DEBUG_TRACE(MODULE_ROUTE, LOG_INFO,  "route_query_catalog_response_proc() GBLogicDevice_info_add:logic_device_pos=%d \r\n", logic_device_pos);

                if (logic_device_pos < 0)
                {
                    GBLogicDevice_info_free(pNewGBLogicDeviceInfo);
                    osip_free(pNewGBLogicDeviceInfo);
                    pNewGBLogicDeviceInfo = NULL;
                    DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR,  "route_query_catalog_response_proc() exit---: GBLogicDevice Info Add Error \r\n");
                    ItemAccNode = inPacket.SearchNextElement(true);
                    continue;
                }

                /* ���������Ϣ���¼�CMS  */
                i = SendNotifyCatalogToSubCMS(pNewGBLogicDeviceInfo, 0, pRoute_Srv_dboper);

                if (i < 0)
                {
                    DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR,  "route_query_catalog_response_proc() SendNotifyCatalogToSubCMS Error:iRet=%d \r\n", i);
                }
                else if (i > 0)
                {
                    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE,  "route_query_catalog_response_proc() SendNotifyCatalogToSubCMS OK:iRet=%d \r\n", i);
                }

                /* �������ݿ� */
                i = AddGBLogicDeviceInfo2DBForRoute(strGBLogicDeviceID, pRoute_Srv_dboper);

                if (i < 0)
                {
                    DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_catalog_response_proc() AddGBLogicDeviceInfo2DBForRoute ERROR:device_id=%s, iRet=%d \r\n", strGBLogicDeviceID, i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_catalog_response_proc() AddGBLogicDeviceInfo2DBForRoute OK:device_id=%s, iRet=%d \r\n", strGBLogicDeviceID, i);
                }
            }

            ItemAccNode = inPacket.SearchNextElement(true);
        }

        if (pRouteInfo->CataLogSN == (unsigned int)osip_atoi(strSN))
        {
            /* �ϱ����������� */
            if (pRouteInfo->CataLogNumCount >= iSumNum)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_INFO,  "route_query_catalog_response_proc() Proc Catalog Item End---:server_id=%s, CataLogNumCount=%d, SumNum=%d \r\n", pRouteInfo->server_id, pRouteInfo->CataLogNumCount, iSumNum);
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "�ϼ�CMS����Ŀ¼��Ϣ:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, ��ʼ����ɾ����ʶɾ����CMS�����ͨ��", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Superior CMS push directory message:start to delete channel belong to this CMS accorrding to delete notification, superior CMS ID=%s, IP=%s", pRouteInfo->server_id, pRouteInfo->server_ip);

                pRouteInfo->CataLogNumCount = 0;

                /* ����ɾ����ʶ�������߼��豸���ñ�ʶ */
                i = SetGBLogicDeviceInfoEnableFlagByDelMarkForRoute(pRouteInfo, pRoute_Srv_dboper);
            }
        }
    }

    return i;
}

/*****************************************************************************
 �� �� ��  : route_device_info_response_proc
 ��������  : �ϼ�����CMS���͹������豸�豸��Ϣ��ѯ��Ӧ����
 �������  : route_info_t* pRouteInfo
             char* caller_id
             char* callee_id
             CPacket& inPacket
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��5��20��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int route_device_info_response_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, CPacket& inPacket)
{
#if 0

    int i = 0;
    int device_pos = -1;
    GBDevice_info_t* pGBDeviceInfo = NULL;

    /* �����豸��Ϣ��ѯ��Ӧ��Ϣֱ��ת������������
          �������̼�9.5.2

          ������������ֶ�:
          <!-- �������ͣ��豸��Ϣ��ѯ����ѡ�� -->
          <element name="CmdType" fixed ="DeviceInfo" />
          <!-- �������кţ���ѡ�� -->
          <element name="SN" type="integer" minInclusive value = "1" />
          <!-- Ŀ���豸���豸���루��ѡ�� -->
          <element name="DeviceID" type="tg:deviceIDType" />
          <!-- ��ѯ�������ѡ�� -->
          <element name="Result" type="tg:resultType" />
          <!-- �豸�����̣���ѡ�� -->
          <element name ="Manufacturer" type="normalizedString" minOccurs= "0"/ >
          <!-- �豸�ͺţ���ѡ�� -->
          <element name ="Model" type="string" minOccurs= "0"/>
          <!-- �豸�̼��汾����ѡ�� -->
          <element name ="Firmware" type="string" minOccurs= "0"/>
          <!-- ��Ƶ����ͨ��������ѡ�� -->
          <element name ="Channel" type="integer" minInclusive value = "0" minOccurs= "0"/ >
          <!-- ��չ��Ϣ���ɶ��� -->
          <element name= "Info" minOccurs= "0" maxOccurs="unbounded">
          <restriction base= "string">
          <maxLength value= "1024" />
          </restriction>
          </element>
      */

    /* ����callee_id ���������豸*/
    device_pos = GBDevice_info_find_by_GBLogicDevice(callee_id);

    if (device_pos >= 0)
    {
        pGBDeviceInfo = GBDevice_info_get(device_pos);

        if (NULL != pGBDeviceInfo)
        {
            i = SIP_ProxyBuildTargetAndSendMessage(caller_id, callee_id, pGBDeviceInfo->device_id, (char*)inPacket.GetXml(NULL).c_str(), inPacket.GetXml(NULL).length());
        }
    }
    else
    {
        i = SIP_ProxyBuildTargetAndSendMessage(caller_id, callee_id, callee_id, (char*)inPacket.GetXml(NULL).c_str(), inPacket.GetXml(NULL).length());
    }

#endif
    return 0;
}

/*****************************************************************************
 �� �� ��  : route_device_status_response_proc
 ��������  : �ϼ�����CMS���͹������豸״̬��ѯ��Ӧ����
 �������  : route_info_t* pRouteInfo
             char* caller_id
             char* callee_id
             CPacket& inPacket
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��5��20��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int route_device_status_response_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, CPacket& inPacket)
{
#if 0

    int i = 0;
    int device_pos = -1;
    GBDevice_info_t* pGBDeviceInfo = NULL;

    /* �����豸��Ϣ��ѯ��Ӧ��Ϣֱ��ת������������
          �������̼�9.5.2

          ������������ֶ�:
          <!-- �������ͣ��豸״̬��ѯ����ѡ�� -->
          <element name="CmdType" fixed ="DeviceStatus" />
          <!-- �������кţ���ѡ�� -->
          <element name="SN" type="integer" minInclusive value = "1" />
          <!-- Ŀ���豸���豸���루��ѡ�� -->
          <element name="DeviceID" type="tg:deviceIDType" />
          <!-- ��ѯ�����־����ѡ�� -->
          <element name="Result" type="tg:resultType" />
          <!-- �Ƿ����ߣ���ѡ�� -->
          <element name="Online" >
          <restriction base= "string">
          <enumeration value= "ONLINE" />
          <enumeration value= "OFFLINE" />
          </restriction>
          </element>
          <!-- �Ƿ�������������ѡ�� -->
          <element name="Status" type="tg:relultType" />
          <!-- ����������ԭ�򣨿�ѡ�� -->
          <element name="Reason" type="string" minOccurs= "0"/>
          <!-- �Ƿ���루��ѡ�� -->
          <element name="Encode" type="tg:statusType" minOccurs= "0"/>
          <!-- �Ƿ�¼�񣨿�ѡ�� -->
          <element name="Record" type=" tg:statusType" minOccurs= "0"/>
          <!-- �豸ʱ������ڣ���ѡ�� -->
          <element name ="DeviceTime" type="dateTime" minOccurs= "0"/>
          <!-- �����豸״̬�б�,num��ʾ�б����������ѡ�� -->
          <element name="Alarmstatus" minOccurs="0">
          <attribute name="Num" type="integer"/>
          <element name="Item" minOccurs="0" maxOccurs=" unbounded ">
          <simpleType>
          <sequence>
          <!-- �����豸���루��ѡ�� -->
          <element name="DeviceID" type=" tg:deviceIDType " minOccurs= "0"/>
          <!-- �����豸״̬����ѡ�� -->
          <element name="Status" minOccurs= "0">
          <restriction base="string">
          <enumeration value="ONDUTY"/>
          <enumeration value="OFFDUTY"/>
          <enumeration value="ALARM"/>
          </restriction>
          </element>
          </sequence>
          </simpleType>
          </element>
          </element>
          <!-- ��չ��Ϣ���ɶ��� -->
          <element name= "Info" minOccurs= "0" maxOccurs="unbounded">
          <restriction base= "string">
          <maxLength value= "1024" />
          </restriction>
          </element>
      */

    /* ����callee_id ���������豸*/
    device_pos = GBDevice_info_find_by_GBLogicDevice(callee_id);

    if (device_pos >= 0)
    {
        pGBDeviceInfo = GBDevice_info_get(device_pos);

        if (NULL != pGBDeviceInfo)
        {
            i = SIP_ProxyBuildTargetAndSendMessage(caller_id, callee_id, pGBDeviceInfo->device_id, (char*)inPacket.GetXml(NULL).c_str(), inPacket.GetXml(NULL).length());
        }
    }
    else
    {
        i = SIP_ProxyBuildTargetAndSendMessage(caller_id, callee_id, callee_id, (char*)inPacket.GetXml(NULL).c_str(), inPacket.GetXml(NULL).length());
    }

#endif
    return 0;
}

/*****************************************************************************
 �� �� ��  : route_record_info_response_proc
 ��������  : �ϼ�����CMS���͹�����¼����Ϣ��ѯ��Ӧ����
 �������  : route_info_t* pRouteInfo
             char* caller_id
             char* callee_id
             CPacket& inPacket
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��5��20��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int route_record_info_response_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, CPacket& inPacket)
{
    /* �豸����Ƶ�ļ�����, ת�����û�
      */
    int i = 0;
    int xml_pos = -1;
    char strSN[32] = {0};
    char strDeviceID[32] = {0};
    char strSumNum[16] = {0};
    string strRecordListNum = "";
    int iSumNum = 0;
    int iRecordListNum = 0;
    unsigned int transfer_xml_sn = 0;

    if (NULL == pRouteInfo)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_record_info_response_proc() exit---: GBDevice Info Error \r\n");
        return -1;
    }

    if (NULL == caller_id || NULL == callee_id)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_record_info_response_proc() exit---: Param Error \r\n");
        return -1;
    }

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS¼����Ϣ��ѯ��Ӧ��Ϣ:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Superior CMS video information query response message:front-end device ID=%s", caller_id);

    /* ȡ������*/
    inPacket.GetElementValue((char*)"SN", strSN);
    inPacket.GetElementValue((char*)"DeviceID", strDeviceID);
    inPacket.GetElementValue((char*)"SumNum", strSumNum);
    inPacket.GetElementAttr((char*)"RecordList", (char*)"Num", strRecordListNum);

    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO,  "route_record_info_response_proc() \
    \r\n XML Para: \
    \r\n SN=%s \
    \r\n DeviceID=%s \
    \r\n SumNum=%s \
    \r\n RecordList Num=%s \r\n ", strSN, strDeviceID, strSumNum, (char*)strRecordListNum.c_str());

    if (strSumNum[0] == '\0')
    {
        SystemLog(EV9000_CMS_QUERY_RECORD_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS¼����Ϣ��ѯ��Ӧ��Ϣ����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, ԭ��=û�л�ȡ��ǰ���ϱ���¼���¼����", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
        EnSystemLog(EV9000_CMS_QUERY_RECORD_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "Superior CMS video information query response message process failed:front-end deviceID=%s, cause=did not get total number of video reported", caller_id);
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR,  "route_record_info_response_proc() exit---: Get Sun Num Error \r\n");
        return -1;
    }

    /* ���豸��Ϣд���׼�߼��豸�� */
    iSumNum = osip_atoi(strSumNum);
    iRecordListNum = osip_atoi((char*)strRecordListNum.c_str());
    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE,  "route_record_info_response_proc() SumNum=%d, RecordListNum=%d \r\n", iSumNum, iRecordListNum);
    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS¼����Ϣ��ѯ��Ӧ��Ϣ:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �ϱ���¼������=%d, �����ϱ���¼������=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, iSumNum, iRecordListNum);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Superior CMS video information query response message:front-end device ID=%s, total video number reported=%d, video number reported this time=%d", caller_id, iSumNum, iRecordListNum);

    /* �����Ƿ����û���ѯ�����ϼ���ѯ�� */
    transfer_xml_sn = strtoul(strSN, NULL, 10);
    xml_pos = transfer_xml_msg_find(XML_QUERY_RECORDINFO, strDeviceID, transfer_xml_sn);

    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_record_info_response_proc() transfer_xml_msg_find:Type=%d, DeviceID=%s, transfer_xml_sn=%d, xml_pos=%d \r\n", XML_QUERY_RECORDINFO, strDeviceID, transfer_xml_sn, xml_pos);

    if (xml_pos >= 0)
    {
        i = transfer_xml_message_to_dest(xml_pos, iSumNum, iRecordListNum, inPacket);

        if (i != 0)
        {
            SystemLog(EV9000_CMS_QUERY_RECORD_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS¼����Ϣ��ѯ��Ӧ��Ϣ����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, ԭ��=����XML��SNת����Ŀ�ĵ�ʧ��, xml_pos=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, xml_pos);
            EnSystemLog(EV9000_CMS_QUERY_RECORD_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "Superior CMS video information query response message process failed:front-end device ID=%s:front-end device ID=%s, cause=forward to destination failed accorrding to XML SN", caller_id);
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_record_info_response_proc() transfer_xml_message_to_dest Error:device_id=%s\r\n", caller_id);
        }
        else
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS¼����Ϣ��ѯ��Ӧ��Ϣ����ɹ�:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d,����XML��SNת����Ŀ�ĵ�, xml_pos=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, xml_pos);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Superior CMS video information query response message process success:Superior CMS, ID = % s = % s IP address, port number = % d, according to the XML SN forwarded to destination, xml_pos = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, xml_pos);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_record_info_response_proc() transfer_xml_message_to_dest OK:device_id=%s\r\n", caller_id);
        }
    }
    else
    {
        SystemLog(EV9000_CMS_QUERY_RECORD_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS¼����Ϣ��ѯ��Ӧ��Ϣ����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, ԭ��=����XML��SN����Ŀ�ĵ�ʧ��, transfer_xml_sn=%d, strDeviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, transfer_xml_sn, strDeviceID);
        EnSystemLog(EV9000_CMS_QUERY_RECORD_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "Superior CMS video information query response message process failed:front-end device ID=%s:front-end device ID=%s, cause=find to destination failed accorrding to XML SN", caller_id);
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_record_info_response_proc() transfer_xml_message_to_dest Error:device_id=%s\r\n", caller_id);
    }

    return 0;
}

/*****************************************************************************
 �� �� ��  : route_subscribe_query_catalog_proc
 ��������  : �ϼ�����·�ɹ��������Ŀ¼������Ϣ�Ĵ���
 �������  : route_info_t* pRouteInfo
             char* caller_id
             char* callee_id
             int event_id
             int subscribe_expires
             CPacket& inPacket
             DBOper* pRoute_Srv_dboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��6��10�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int route_subscribe_query_catalog_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, int event_id, int subscribe_expires, CPacket& inPacket, DBOper* pRoute_Srv_dboper)
{
    int i = 0;
    char strSN[32] = {0};
    char strDeviceID[32] = {0};

    if (NULL == pRouteInfo)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_subscribe_query_catalog_proc() exit---: Route Info Error \r\n");
        return -1;
    }

    if (NULL == caller_id || NULL == callee_id || NULL == pRoute_Srv_dboper)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_subscribe_query_catalog_proc() exit---: Route Srv DB Oper Error \r\n");
        return -1;
    }

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS������Ŀ¼������Ϣ:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Directory subscription information from superior CMS:Superior CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);

    /* ȡ������*/
    inPacket.GetElementValue((char*)"SN", strSN);
    inPacket.GetElementValue((char*)"DeviceID", strDeviceID);

    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO,  "route_subscribe_query_catalog_proc() \
            \r\n XML Para: \
            \r\n SN=%s, DeviceID=%s\r\n", strSN, strDeviceID);

    /* �鿴�����Ƿ��Ǳ�CMS ID */
    if (0 != strncmp(strDeviceID, local_cms_id_get(), 20))
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS������Ŀ¼������Ϣ����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, ԭ��=%s:%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"��ѯ��ID���Ǳ�CMS��ID", strDeviceID);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Directory subscription information from superior CMS process failed:Requester ID=%s, IPaddress=%s, port number=%d, cause=%s:%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"ID searched does not belong to this CMS", strDeviceID);
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_subscribe_query_catalog_proc() exit---: Device ID Not Belong To Mine CMSID:DeviceID=%s \r\n", strDeviceID);
        return -1;
    }

    if (0 == pRouteInfo->catalog_subscribe_flag)
    {
        /* ����״̬ */
        pRouteInfo->catalog_subscribe_flag = 1;
        pRouteInfo->catalog_subscribe_expires = subscribe_expires;
        pRouteInfo->catalog_subscribe_expires_count = subscribe_expires;

        if (event_id > 0)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS������Ŀ¼������Ϣ:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, CMS����֮���յ��ĳ�ʼ����", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
            pRouteInfo->catalog_subscribe_event_id = event_id;
        }
        else
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS������Ŀ¼������Ϣ:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, CMS����֮���յ��ĳ�ʼ����, û��Я��Event ID�ֶ�", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
            pRouteInfo->catalog_subscribe_event_id = 99999;
        }

        /* ���͵���״̬��λ���ϼ� */
        i = SendAllOfflineDeviceStatusTo3PartyRouteCMS(pRouteInfo);
    }
    else
    {
        pRouteInfo->catalog_subscribe_expires = subscribe_expires;
        pRouteInfo->catalog_subscribe_expires_count = subscribe_expires;

        if (event_id > 0)
        {
            if (pRouteInfo->catalog_subscribe_event_id != event_id)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS������Ŀ¼������Ϣ:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �յ��ϼ�CMS����֮��ĳ�ʼ����", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                pRouteInfo->catalog_subscribe_event_id = event_id;

                /* ���͵���״̬��λ���ϼ� */
                i = SendAllOfflineDeviceStatusTo3PartyRouteCMS(pRouteInfo);
            }
            else
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS������Ŀ¼������Ϣ:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �յ��ϼ�CMS��ˢ�¶�����Ϣ", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
            }
        }
        else /* �ϼ�ƽ̨������ʱ��û�з�������ֶ�ʱ��ÿ�ζ����� */
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS������Ŀ¼������Ϣ:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �յ��ϼ�CMS��ˢ�¶�����Ϣ, û��Я��Event ID�ֶ�", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
            pRouteInfo->catalog_subscribe_event_id = 99999;

            /* ���͵���״̬��λ���ϼ� */
            i = SendAllOfflineDeviceStatusTo3PartyRouteCMS(pRouteInfo);
        }
    }

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS������Ŀ¼������Ϣ����ɹ�:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �����������ߵ�λ״̬��Ϣ���ϼ�ƽ̨", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Directory subscription information from superior CMS process success:Superior CMS, ID = % s = % s IP address, port number = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);

    return i;
}

int route_subscribe_witin_dialog_query_catalog_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, int dialog_index, int subscribe_expires, CPacket& inPacket, DBOper* pRoute_Srv_dboper)
{
    int i = 0;
    char strSN[32] = {0};
    char strDeviceID[32] = {0};

    if (NULL == pRouteInfo)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_subscribe_witin_dialog_query_catalog_proc() exit---: Route Info Error \r\n");
        return -1;
    }

    if (NULL == caller_id || NULL == callee_id || NULL == pRoute_Srv_dboper)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_subscribe_witin_dialog_query_catalog_proc() exit---: Route Srv DB Oper Error \r\n");
        return -1;
    }

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS������Ŀ¼������Ϣ:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Directory subscription information from superior CMS:Superior CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);

    /* ȡ������*/
    inPacket.GetElementValue((char*)"SN", strSN);
    inPacket.GetElementValue((char*)"DeviceID", strDeviceID);

    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO,  "route_subscribe_witin_dialog_query_catalog_proc() \
            \r\n XML Para: \
            \r\n SN=%s, DeviceID=%s\r\n", strSN, strDeviceID);

    /* �鿴�����Ƿ��Ǳ�CMS ID */
    if (0 != strncmp(strDeviceID, local_cms_id_get(), 20))
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS������Ŀ¼������Ϣ����ʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, ԭ��=%s:%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"��ѯ��ID���Ǳ�CMS��ID", strDeviceID);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Directory subscription information from superior CMS process failed:Requester ID=%s, IPaddress=%s, port number=%d, cause=%s:%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"ID searched does not belong to this CMS", strDeviceID);
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_subscribe_witin_dialog_query_catalog_proc() exit---: Device ID Not Belong To Mine CMSID:DeviceID=%s \r\n", strDeviceID);
        return -1;
    }

    if (subscribe_expires > 0)
    {
        if (0 == pRouteInfo->catalog_subscribe_flag)
        {
            /* ����״̬ */
            pRouteInfo->catalog_subscribe_flag = 1;
            pRouteInfo->catalog_subscribe_expires = subscribe_expires;
            pRouteInfo->catalog_subscribe_expires_count = subscribe_expires;
            pRouteInfo->catalog_subscribe_dialog_index = dialog_index;

            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS������Ŀ¼������Ϣ:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, CMS����֮���յ��ĳ�ʼ����", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);

        }
        else
        {
            pRouteInfo->catalog_subscribe_expires = subscribe_expires;
            pRouteInfo->catalog_subscribe_expires_count = subscribe_expires;
            pRouteInfo->catalog_subscribe_dialog_index = dialog_index;

            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS������Ŀ¼������Ϣ:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �յ��ϼ�CMS��ˢ�¶�����Ϣ", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
        }

        /* ���͵���״̬��λ���ϼ� */
        i = SendAllOfflineDeviceStatusTo3PartyRouteCMS(pRouteInfo);
    }

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS������Ŀ¼������Ϣ����ɹ�:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �����������ߵ�λ״̬��Ϣ���ϼ�ƽ̨", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Directory subscription information from superior CMS process success:Superior CMS, ID = % s = % s IP address, port number = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);

    return i;
}

/*****************************************************************************
 �� �� ��  : route_get_service_id_response_proc
 ��������  : ��ȡ������ID����Ӧ����
 �������  : char* caller_id
             char* caller_ip
             int caller_port
             char* callee_id
             char* local_ip
             int local_port
             char* pcSN
             char* pcServerID
             DBOper* pRoute_Srv_dboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2016��6��13��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int route_get_service_id_response_proc(char* caller_id, char* caller_ip, int caller_port, char* callee_id, char* local_ip, int local_port, char* pcSN, char* pcServerID, DBOper* pRoute_Srv_dboper)
{
    int i = 0;
    int route_pos = -1;
    route_info_t* pRouteInfo = NULL;
    string strSQL = "";
    char strID[32] = {0};

    if (NULL == pRoute_Srv_dboper)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_get_service_id_response_proc() exit---: Route Srv dboper Error \r\n");
        return -1;
    }

    if ((NULL == caller_id) || (NULL == caller_ip) || (caller_port <= 0))
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_get_service_id_response_proc() exit---: Caller Info Error \r\n");
        return -1;
    }

    if ((NULL == callee_id) || (NULL == local_ip) || (local_port <= 0))
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_get_service_id_response_proc() exit---: Callee Info Error \r\n");
        return -1;
    }

    if ((NULL == pcServerID) || (pcServerID[0] == '\0'))
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_get_service_id_response_proc() exit---: Server ID Error \r\n");
        return -1;
    }

    /* ����·����Ϣ */
    route_pos = route_info_find_by_host_and_port(caller_ip, caller_port);

    if (route_pos < 0)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_get_service_id_response_proc() exit---: route_info_find_by_host_and_port Error:server IP=%s, server port=%d \r\n", caller_ip, caller_port);
        return -1;
    }

    pRouteInfo = route_info_get(route_pos);

    if (NULL == pRouteInfo)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_get_service_id_response_proc() exit---: route_info_get Error:route_pos=%d \r\n", route_pos);
        return -1;
    }

    /* ���µ��ڴ� */
    memset(pRouteInfo->server_id, 0, MAX_ID_LEN);
    osip_strncpy(pRouteInfo->server_id, pcServerID, MAX_ID_LEN);

    /* �������ݿ� */
    strSQL.clear();
    strSQL = "UPDATE RouteNetConfig SET ServerID = '";
    strSQL += pcServerID;
    strSQL += "' WHERE ID = ";
    snprintf(strID, 16, "%d", pRouteInfo->id);
    strSQL += strID;

    i = pRoute_Srv_dboper->DB_Update(strSQL.c_str(), 1);

    if (i < 0)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_get_service_id_response_proc() DB Oper Error:strSQL=%s, iRet=%d \r\n", strSQL.c_str(), i);
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_get_service_id_response_proc() ErrorMsg=%s\r\n", pRoute_Srv_dboper->GetLastDbErrorMsg());
    }

    return i;
}

/*****************************************************************************
 �� �� ��  : RouteGetGBDeviceListAndSendCataLogToCMS
 ��������  : �ϼ�����CMS���͹����Ļ�ȡ�豸��Ϣ�����䷢�͸��ϼ�CMS
 �������  : route_info_t* pRouteInfo
             char* caller_id
             char* strDeviceID
             char* strSN
             DBOper* pRoute_Srv_dboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��2��10�� ���ڶ�
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int RouteGetGBDeviceListAndSendCataLogToCMS(route_info_t* pRouteInfo, char* caller_id, char* strDeviceID, char* strSN, DBOper* pRoute_Srv_dboper)
{
    int i = 0;
    static int iWaitCount = 0;

    if (NULL == pRouteInfo)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "RouteGetGBDeviceListAndSendCataLogToCMS() exit---: Route Info Error \r\n");
        return -1;
    }

    if ((NULL == caller_id) || (NULL == strDeviceID) || (NULL == strSN) || (NULL == pRoute_Srv_dboper))
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "RouteGetGBDeviceListAndSendCataLogToCMS() exit---: Param Error \r\n");
        return -1;
    }

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS��ȡ���߼�����Ϣ:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Access logic device information from superior CMS: superior CMS, ID = % s = % s IP address, port number = % d ", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);

    if (1 == pRouteInfo->catlog_get_status)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS��ȡ���߼�����Ϣʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, ԭ��=�ϴλ�ȡCatlog��û�н���", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
        return 0;
    }
    else
    {
        /* ������ݿ��Ƿ����ڸ��� */
        pRouteInfo->catlog_get_status = 1;

        while (checkIfHasDBRefresh() && iWaitCount < 12)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "�ϼ�CMS��ȡ���߼�����Ϣ,���ݿ����ڸ���,�ȴ����ݿ�������:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
            osip_usleep(5000000);

            iWaitCount++;
        }

        if (iWaitCount >= 12)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS��ȡ���߼�����Ϣʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, ԭ��=�ȴ��������ݿ���ɳ�ʱ,���Ժ��ٻ�ȡ", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
            iWaitCount = 0;
            pRouteInfo->catlog_get_status = 0;
            return 0;
        }

        iWaitCount = 0;

        if (1 == pRouteInfo->three_party_flag) /* ��Ҫ�����������������Ϣ */
        {
            i = RouteGetGBDeviceListAndSendCataLogTo3PartyCMS(pRouteInfo, caller_id, strDeviceID, strSN, pRoute_Srv_dboper);
        }
        else
        {
            i = RouteGetGBDeviceListAndSendCataLogToOwnerCMS(pRouteInfo, caller_id, strDeviceID, strSN, pRoute_Srv_dboper);
        }

        pRouteInfo->catlog_get_status = 0;
    }

    return i;
}

/*****************************************************************************
 �� �� ��  : RouteGetGBDeviceListAndSendCataLogToOwnerCMS
 ��������  : �ϼ�����CMS���͹����Ļ�ȡ�豸��Ϣ�����䷢�͸��ϼ�CMS
 �������  : route_info_t* pRouteInfo
             char* caller_id
             char* strDeviceID
             char* strSN
             DBOper* pRoute_Srv_dboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��9��6��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int RouteGetGBDeviceListAndSendCataLogToOwnerCMS(route_info_t* pRouteInfo, char* caller_id, char* strDeviceID, char* strSN, DBOper* pRoute_Srv_dboper)
{
    int i = 0;
    int index = 0;
    int record_count = 0; /* ��¼�� */
    int send_count = 0;   /* ���͵Ĵ��� */
    int query_count = 0;  /* ��ѯ����ͳ�� */
    DOMElement* ListAccNode = NULL;

    string strSQL = "";
    vector<string> DeviceIDVector;

    if (NULL == pRouteInfo)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "RouteGetGBDeviceListAndSendCataLogToOwnerCMS() exit---: Route Info Error \r\n");
        return -1;
    }

    if ((NULL == caller_id) || (NULL == strDeviceID) || (NULL == strSN) || (NULL == pRoute_Srv_dboper))
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "RouteGetGBDeviceListAndSendCataLogToOwnerCMS() exit---: Param Error \r\n");
        return -1;
    }

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS��ȡ���߼�����Ϣ, �ϼ�Ϊ����ƽ̨:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Superior CMS access logic device information :Superior CMS, ID = % s = % s IP address, port number = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
    /* �ϼ�cms��ѯ���豸�б��Ǳ���cms�������豸�б�
     */
    DeviceIDVector.clear();

    /* ������е��߼��豸 */
    i = AddAllGBLogicDeviceIDToVectorForRoute(DeviceIDVector, pRouteInfo->id, pRouteInfo->three_party_flag, pRouteInfo->link_type, pRoute_Srv_dboper);

    /* 4����ȡ�����е��豸���� */
    record_count = DeviceIDVector.size();

    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "RouteGetGBDeviceListAndSendCataLogToOwnerCMS() record_count=%d \r\n", record_count);

    /* 5�������¼��Ϊ0 */
    if (record_count == 0)
    {
        /* �ظ���Ӧ,�齨��Ϣ */
        CPacket outPacket;
        DOMElement* AccNode = NULL;

        outPacket.SetRootTag("Response");

        AccNode = outPacket.CreateElement((char*)"CmdType");
        outPacket.SetElementValue(AccNode, (char*)"Catalog");

        AccNode = outPacket.CreateElement((char*)"SN");
        outPacket.SetElementValue(AccNode, strSN);

        AccNode = outPacket.CreateElement((char*)"DeviceID");
        outPacket.SetElementValue(AccNode, strDeviceID);

        AccNode = outPacket.CreateElement((char*)"SumNum");
        outPacket.SetElementValue(AccNode, (char*)"0");

        ListAccNode = outPacket.CreateElement((char*)"DeviceList");
        outPacket.SetElementAttr(ListAccNode, (char*)"Num", (char*)"0");

        /* ת����Ϣ���ϼ�CMS */
        i = SIP_SendMessage(NULL, local_cms_id_get(), caller_id, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

        if (i != 0)
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "RouteGetGBDeviceListAndSendCataLogToOwnerCMS() SIP_SendMessage Error:caller_id=%s, caller_ip=%s, caller_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
        }
        else
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "RouteGetGBDeviceListAndSendCataLogToOwnerCMS() SIP_SendMessage OK:caller_id=%s, caller_ip=%s, caller_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
        }

        SystemLog(EV9000_CMS_GET_CATALOG_ERROR, EV9000_LOG_LEVEL_WARNING, "�ϼ�CMS��ȡ�߼��豸��Ϣ��Ϣʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, ԭ��=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"δ��ѯ�����ݿ��¼");
        EnSystemLog(EV9000_CMS_GET_CATALOG_ERROR, EV9000_LOG_LEVEL_WARNING, "Access logic device info from superior CMS message failed:Requester ID=%s, IPaddress=%s, port number=%d, cause=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"Database record not found");
        DEBUG_TRACE(MODULE_ROUTE, LOG_WARN, "RouteGetGBDeviceListAndSendCataLogToOwnerCMS() exit---: No Record Count \r\n");
        return i;
    }

    /* 6��ѭ��������������ȡ�û����豸��Ϣ������xml�� */
    CPacket* pOutPacket = NULL;

    for (index = 0; index < record_count; index++)
    {
        //DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "RouteGetGBDeviceListAndSendCataLogToCMS() DeviceIndex=%u \r\n", device_index);

        /* �����¼������4����Ҫ�ִη��� */
        query_count++;

        /* ����XMLͷ�� */
        i = CreateGBLogicDeviceCatalogResponseXMLHeadForRoute(&pOutPacket, query_count, record_count, strSN, strDeviceID, &ListAccNode);

        /* ����Item ֵ */
        i = AddLogicDeviceInfoToXMLItemForRoute(pOutPacket, ListAccNode, (char*)DeviceIDVector[index].c_str(), pRouteInfo->three_party_flag, pRoute_Srv_dboper);

        if ((query_count % MAX_ROUTE_CATALOG_COUT_SEND == 0) || (query_count == record_count))
        {
            if (NULL != pOutPacket)
            {
                send_count++;

                /* ת����Ϣ���ϼ�CMS */
                i |= SIP_SendMessage(NULL, local_cms_id_get(), caller_id, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)pOutPacket->GetXml(NULL).c_str(), pOutPacket->GetXml(NULL).length());

                if (i != 0)
                {
                    SystemLog(EV9000_CMS_GET_CATALOG_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS��ȡ�߼��豸��Ϣ��Ϣ, ����Message��Ϣ���ϼ�CMSʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, ���ʹ���=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, send_count);
                    EnSystemLog(EV9000_CMS_GET_CATALOG_ERROR, EV9000_LOG_LEVEL_ERROR, "Access logic device info from superior CMS message, Send Message Message to superiors CMS failure: the superior CMS, ID = % s = % s IP address, port number = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                    DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "RouteGetGBDeviceListAndSendCataLogToOwnerCMS() SIP_SendMessage Error:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                }
                else
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS��ȡ�߼��豸��Ϣ��Ϣ, ����Message��Ϣ���ϼ�CMS�ɹ�:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, ���ʹ���=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, send_count);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Access logic device info from superior CMS message, Send Message Message to superiors CMS success:Superior CMS, ID = % s = % s IP address, port number = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "RouteGetGBDeviceListAndSendCataLogToOwnerCMS() SIP_SendMessage OK:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                }

                delete pOutPacket;
                pOutPacket = NULL;
            }
        }
    }

    if (i == 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS��ȡ�߼��豸��Ϣ�ɹ�:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d,�߼��豸��Ŀ=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, record_count);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Access logic device info from superior CMS message, Send Message Message to superiors CMS success:Superior CMS, ID = % s = % s IP address, port number = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
    }
    else
    {
        SystemLog(EV9000_CMS_GET_CATALOG_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS��ȡ�߼��豸��Ϣʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, ԭ��=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"����SIP��Ӧ��Ϣʧ��");
        EnSystemLog(EV9000_CMS_GET_CATALOG_ERROR, EV9000_LOG_LEVEL_ERROR, "Access logic device info from superior CMS message, Send Message Message to superiors CMS failure: the superior CMS, ID = % s = % s IP address, port number = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
    }

    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "RouteGetGBDeviceListAndSendCataLogToOwnerCMS Exit--- \r\n");

    return 0;
}

/*****************************************************************************
 �� �� ��  : RouteGetGBDeviceListAndSendCataLogTo3PartyCMS
 ��������  : �������ϼ�����CMS���͹����Ļ�ȡ�豸��Ϣ�����䷢�͸��ϼ�CMS
 �������  : route_info_t* pRouteInfo
             char* caller_id
             char* strDeviceID
             char* strSN
             DBOper* pRoute_Srv_dboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��9��6��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int RouteGetGBDeviceListAndSendCataLogTo3PartyCMS(route_info_t* pRouteInfo, char* caller_id, char* strDeviceID, char* strSN, DBOper* pRoute_Srv_dboper)
{
    int i = 0;
    int index = 0;
    int group_record_count = 0; /* ��¼�� */
    int device_record_count = 0; /* ��¼�� */
    int record_count = 0; /* ��¼�� */
    int send_count = 0;   /* ���͵Ĵ��� */
    int query_count = 0;  /* ��ѯ����ͳ�� */
    DOMElement* ListAccNode = NULL;
    primary_group_t* pPrimaryGroup = NULL;
    int tcp_socket = -1;

    string strSQL = "";
    vector<string> GroupIDVector;
    vector<string> DeviceIDVector;

    if (NULL == pRouteInfo)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "RouteGetGBDeviceListAndSendCataLogTo3PartyCMS() exit---: Route Info Error \r\n");
        return -1;
    }

    if ((NULL == caller_id) || (NULL == strDeviceID) || (NULL == strSN) || (NULL == pRoute_Srv_dboper))
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "RouteGetGBDeviceListAndSendCataLogTo3PartyCMS() exit---: Param Error \r\n");
        return -1;
    }

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS��ȡ���߼�����Ϣ,�ϼ�Ϊ������ƽ̨:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Access logic device info from superior CMS message:Superior CMS, ID = % s = % s IP address, port number = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);

    /* ��ȡ����������������б� */
    GroupIDVector.clear();

    i = AddGblLogicDeviceGroupToVectorForRoute(GroupIDVector);

    /* ��ȡ�����е��豸���� */
    group_record_count = GroupIDVector.size();

    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "RouteGetGBDeviceListAndSendCataLogTo3PartyCMS() group_record_count=%d \r\n", group_record_count);

    /* ��ȡ�߼��豸�б� */
    DeviceIDVector.clear();

    i = AddAllGBLogicDeviceIDToVectorForRoute(DeviceIDVector, pRouteInfo->id, pRouteInfo->three_party_flag, pRouteInfo->link_type, pRoute_Srv_dboper);

    /* ��ȡ�����е��豸���� */
    device_record_count = DeviceIDVector.size();

    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "RouteGetGBDeviceListAndSendCataLogTo3PartyCMS() device_record_count=%d \r\n", device_record_count);

    /* 4����ȡ���� */
    record_count = group_record_count + device_record_count;

    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "RouteGetGBDeviceListAndSendCataLogTo3PartyCMS() record_count=%d \r\n", record_count);

    /* 5�������¼��Ϊ0 */
    if (record_count == 0)
    {
        /* �ظ���Ӧ,�齨��Ϣ */
        CPacket outPacket;
        DOMElement* AccNode = NULL;

        outPacket.SetRootTag("Response");

        AccNode = outPacket.CreateElement((char*)"CmdType");
        outPacket.SetElementValue(AccNode, (char*)"Catalog");

        AccNode = outPacket.CreateElement((char*)"SN");
        outPacket.SetElementValue(AccNode, strSN);

        AccNode = outPacket.CreateElement((char*)"DeviceID");
        outPacket.SetElementValue(AccNode, strDeviceID);

        AccNode = outPacket.CreateElement((char*)"SumNum");
        outPacket.SetElementValue(AccNode, (char*)"0");

        ListAccNode = outPacket.CreateElement((char*)"DeviceList");
        outPacket.SetElementAttr(ListAccNode, (char*)"Num", (char*)"0");

        /* ת����Ϣ���ϼ�CMS */
        i = SIP_SendMessage(NULL, local_cms_id_get(), caller_id, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

        if (i != 0)
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "RouteGetGBDeviceListAndSendCataLogTo3PartyCMS() SIP_SendMessage Error:caller_id=%s, caller_ip=%s, caller_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
        }
        else
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "RouteGetGBDeviceListAndSendCataLogTo3PartyCMS() SIP_SendMessage OK:caller_id=%s, caller_ip=%s, caller_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
        }

        SystemLog(EV9000_CMS_GET_CATALOG_ERROR, EV9000_LOG_LEVEL_WARNING, "�ϼ�CMS��ȡ�߼��豸��Ϣʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, ԭ��=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"δ��ѯ�����ݿ��¼");
        EnSystemLog(EV9000_CMS_GET_CATALOG_ERROR, EV9000_LOG_LEVEL_WARNING, "Access logic device info from superior CMS message failed:Requester ID=%s, IPaddress=%s, port number=%d, cause=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"Database record not found");
        DEBUG_TRACE(MODULE_ROUTE, LOG_WARN, "RouteGetGBDeviceListAndSendCataLogTo3PartyCMS() exit---: No Record Count \r\n");
        return i;
    }

    /* 6��ѭ��������������ȡ�û����豸��Ϣ������xml�� */
    CPacket* pOutPacket = NULL;

    /* �Ȳ���TCP���� */
    tcp_socket = CMS_CreateSIPTCPConnect(pRouteInfo->server_ip, pRouteInfo->server_port);

    if (tcp_socket >= 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS��ȡ�߼��豸��Ϣ, ����SIP TCP�ɹ�,��ͨ��SIP TCP����Ŀ¼Catalog��Ϣ���������ϼ�CMS:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, tcp_socket=%d", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, tcp_socket);

        for (index = 0; index < record_count; index++)
        {
            /* �����¼������4����Ҫ�ִη��� */
            query_count++;

            /* ����XMLͷ�� */
            i = CreateGBLogicDeviceCatalogResponseXMLHeadForRoute(&pOutPacket, query_count, record_count, strSN, strDeviceID, &ListAccNode);

            if (index < group_record_count)
            {
                pPrimaryGroup = get_primary_group((char*)GroupIDVector[index].c_str());

                if (NULL != pPrimaryGroup)
                {
                    /* ����Item ֵ */
                    i = AddLogicDeviceGroupInfoToXMLItemForRoute(pOutPacket, ListAccNode, pPrimaryGroup->group_code, pPrimaryGroup->group_name, pPrimaryGroup->parent_code);
                }
                else
                {
                    /* ����Item ֵ */
                    i = AddLogicDeviceGroupInfoToXMLItemForRoute(pOutPacket, ListAccNode, (char*)"", (char*)"", NULL);
                }
            }
            else if (index >= group_record_count)
            {
                /* ����Item ֵ */
                i = AddLogicDeviceInfoToXMLItemForRoute(pOutPacket, ListAccNode, (char*)DeviceIDVector[index - group_record_count].c_str(), pRouteInfo->three_party_flag, pRoute_Srv_dboper);
            }

            if ((query_count % MAX_ROUTE_CATALOG_COUT_SEND == 0) || (query_count == record_count))
            {
                if (NULL != pOutPacket)
                {
                    send_count++;

                    /* ת����Ϣ���ϼ�CMS */
                    i |= SIP_SendMessage_By_TCP(NULL, local_cms_id_get(), caller_id, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)pOutPacket->GetXml(NULL).c_str(), pOutPacket->GetXml(NULL).length(), tcp_socket);

                    if (i != 0)
                    {
                        if (i == -2)
                        {
                            SystemLog(EV9000_CMS_GET_CATALOG_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS��ȡ�߼��豸��Ϣ��Ϣ, ͨ��TCP����Message��Ϣ���ϼ�CMSʧ��,TCP�����쳣�ر�:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, tcp_socket=%d, ���ʹ���=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, tcp_socket, send_count);
                            EnSystemLog(EV9000_CMS_GET_CATALOG_ERROR, EV9000_LOG_LEVEL_ERROR, "Access logic device info from superior CMS,Send Message Message to superiors CMS failure: the superior CMS, ID = % s = % s IP address, port number = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "RouteGetGBDeviceListAndSendCataLogTo3PartyCMS() SIP_SendMessage_By_TCP Error:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                            break;
                        }
                        else
                        {
                            SystemLog(EV9000_CMS_GET_CATALOG_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS��ȡ�߼��豸��Ϣ��Ϣ, ͨ��TCP����Message��Ϣ���ϼ�CMSʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, tcp_socket=%d, ���ʹ���=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, tcp_socket, send_count);
                            EnSystemLog(EV9000_CMS_GET_CATALOG_ERROR, EV9000_LOG_LEVEL_ERROR, "Access logic device info from superior CMS,Send Message Message to superiors CMS failure: the superior CMS, ID = % s = % s IP address, port number = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "RouteGetGBDeviceListAndSendCataLogTo3PartyCMS() SIP_SendMessage_By_TCP Error:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                        }
                    }
                    else
                    {
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS��ȡ�߼��豸��Ϣ��Ϣ, ͨ��TCP����Message��Ϣ���ϼ�CMS�ɹ�:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, tcp_socket=%d, ���ʹ���=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, tcp_socket, send_count);
                        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Access logic device info from superior CMS,Send Message Message to superiors CMS success: the superior CMS, ID = % s = % s IP address, port number = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "RouteGetGBDeviceListAndSendCataLogTo3PartyCMS() SIP_SendMessage OK:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                    }

                    delete pOutPacket;
                    pOutPacket = NULL;
                }
            }
        }

        CMS_CloseSIPTCPConnect(tcp_socket);
    }
    else
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS��ȡ�߼��豸��Ϣ, ����SIP TCPʧ��,��ͨ��SIP UDP����Ŀ¼Catalog��Ϣ���������ϼ�CMS:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, tcp_socket=%d", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, tcp_socket);

        for (index = 0; index < record_count; index++)
        {
            /* �����¼������4����Ҫ�ִη��� */
            query_count++;

            /* ����XMLͷ�� */
            i = CreateGBLogicDeviceCatalogResponseXMLHeadForRoute(&pOutPacket, query_count, record_count, strSN, strDeviceID, &ListAccNode);

            if (index < group_record_count)
            {
                pPrimaryGroup = get_primary_group((char*)GroupIDVector[index].c_str());

                if (NULL != pPrimaryGroup)
                {
                    /* ����Item ֵ */
                    i = AddLogicDeviceGroupInfoToXMLItemForRoute(pOutPacket, ListAccNode, pPrimaryGroup->group_code, pPrimaryGroup->group_name, pPrimaryGroup->parent_code);
                }
                else
                {
                    /* ����Item ֵ */
                    i = AddLogicDeviceGroupInfoToXMLItemForRoute(pOutPacket, ListAccNode, (char*)"", (char*)"", NULL);
                }
            }
            else if (index >= group_record_count)
            {
                /* ����Item ֵ */
                i = AddLogicDeviceInfoToXMLItemForRoute(pOutPacket, ListAccNode, (char*)DeviceIDVector[index - group_record_count].c_str(), pRouteInfo->three_party_flag, pRoute_Srv_dboper);
            }

            if ((query_count % MAX_ROUTE_CATALOG_COUT_SEND == 0) || (query_count == record_count))
            {
                if (NULL != pOutPacket)
                {
                    send_count++;

                    /* ת����Ϣ���ϼ�CMS */
                    i |= SIP_SendMessage(NULL, local_cms_id_get(), caller_id, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)pOutPacket->GetXml(NULL).c_str(), pOutPacket->GetXml(NULL).length());

                    if (i != 0)
                    {
                        SystemLog(EV9000_CMS_GET_CATALOG_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS��ȡ�߼��豸��Ϣ��Ϣ, ����Message��Ϣ���ϼ�CMSʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, ���ʹ���=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, send_count);
                        EnSystemLog(EV9000_CMS_GET_CATALOG_ERROR, EV9000_LOG_LEVEL_ERROR, "Access logic device info from superior CMS,Send Message Message to superiors CMS failure: the superior CMS, ID = % s = % s IP address, port number = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "RouteGetGBDeviceListAndSendCataLogTo3PartyCMS() SIP_SendMessage Error:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                    }
                    else
                    {
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS��ȡ�߼��豸��Ϣ��Ϣ, ����Message��Ϣ���ϼ�CMS�ɹ�:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, ���ʹ���=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, send_count);
                        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Access logic device info from superior CMS,Send Message Message to superiors CMS success: the superior CMS, ID = % s = % s IP address, port number = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "RouteGetGBDeviceListAndSendCataLogTo3PartyCMS() SIP_SendMessage OK:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                    }

                    delete pOutPacket;
                    pOutPacket = NULL;
                }
            }
        }
    }

    if (i == 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS��ȡ�߼��豸��Ϣ�ɹ�:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d,�߼��豸��Ŀ=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, record_count);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Access logic device info from superior CMS:Superior CMS, ID = % s = % s IP address, port number = % d, logical device number = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, record_count);
    }
    else
    {
        SystemLog(EV9000_CMS_GET_CATALOG_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS��ȡ�߼��豸��Ϣʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, ԭ��=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"����SIP��Ӧ��Ϣʧ��");
        EnSystemLog(EV9000_CMS_GET_CATALOG_ERROR, EV9000_LOG_LEVEL_ERROR, "Access logic device info from superior CMS failed:Requester ID=%s, IPaddress=%s, port number=%d, cause=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"Send SIP response message failed.");
    }

    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "RouteGetGBDeviceListAndSendCataLogTo3PartyCMS Exit--- \r\n");

    return 0;
}

/*****************************************************************************
 �� �� ��  : AddLogicDeviceInfoToXMLItemForRoute
 ��������  : ����߼��豸��Ϣ��XML��Item
 �������  : CPacket* pOutPacket
             DOMElement* ListAccNode
             char* device_id
             int iThreePartyFlag
             DBOper* pRoute_Srv_dboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��7��27�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int AddLogicDeviceInfoToXMLItemForRoute(CPacket* pOutPacket, DOMElement* ListAccNode, char* device_id, int iThreePartyFlag, DBOper* pRoute_Srv_dboper)
{
    int iRet = 0;
    GBLogicDevice_info_t* pGBLogicDeviceInfo = NULL;
    //GBLogicDevice_info_t* pDBGBLogicDeviceInfo = NULL;
    DOMElement* ItemAccNode = NULL;
    DOMElement* AccNode = NULL;
    DOMElement* ItemInfoNode = NULL;

    char strID[64] = {0};
    char strParental[16] = {0};
    char strSafetyWay[16] = {0};
    char strRegisterWay[16] = {0};
    char strCertifiable[16] = {0};
    char strErrCode[16] = {0};
    char strSecrecy[16] = {0};
    char strPort[16] = {0};
    char strLongitude[64] = {0};
    char strLatitude[64] = {0};
    char strAlarmDeviceSubType[64] = {0};
    char strStreamCount[16] = {0};
    char strFrameCount[16] = {0};
    char strAlarmLengthOfTime[16] = {0};
    char strPTZType[16] = {0};

    if (NULL == pOutPacket || NULL == ListAccNode || NULL == device_id)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "AddLogicDeviceInfoToXMLItemForRoute() exit---: Param Error \r\n");
        return -1;
    }

    /* ��дXML����*/
    pOutPacket->SetCurrentElement(ListAccNode);
    ItemAccNode = pOutPacket->CreateElement((char*)"Item");
    pOutPacket->SetCurrentElement(ItemAccNode);

    /* ����Index ��ȡ�߼��豸��Ϣ��������ֻ�����������豸����û�����ߣ����ݿ���ڴ��ж�û�е�*/
    pGBLogicDeviceInfo = GBLogicDevice_info_find(device_id);

    if (NULL != pGBLogicDeviceInfo)
    {
        if (!iThreePartyFlag) /* �ǵ�����ƽ̨ */
        {
            /* �豸���� */
            AccNode = pOutPacket->CreateElement((char*)"ID");
            snprintf(strID, 64, "%u", pGBLogicDeviceInfo->id);
            pOutPacket->SetElementValue(AccNode, strID);
        }

        /* �豸ͳһ��� */
        AccNode = pOutPacket->CreateElement((char*)"DeviceID");
        pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->device_id);

        /* ��λ���� */
        AccNode = pOutPacket->CreateElement((char*)"Name");
        pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->device_name);

        if (!iThreePartyFlag) /* �ǵ�����ƽ̨ */
        {
            /* �Ƿ�����*/
            AccNode = pOutPacket->CreateElement((char*)"Enable");

            if (0 == pGBLogicDeviceInfo->enable)
            {
                pOutPacket->SetElementValue(AccNode, (char*)"0");
            }
            else
            {
                pOutPacket->SetElementValue(AccNode, (char*)"1");
            }

            /* �Ƿ�ɿ� */
            AccNode = pOutPacket->CreateElement((char*)"CtrlEnable");

            if (1 == pGBLogicDeviceInfo->ctrl_enable)
            {
                pOutPacket->SetElementValue(AccNode, (char*)"Enable");
            }
            else
            {
                pOutPacket->SetElementValue(AccNode, (char*)"Disable");
            }

            /* �Ƿ�֧�ֶԽ� */
            AccNode = pOutPacket->CreateElement((char*)"MicEnable");

            if (0 == pGBLogicDeviceInfo->mic_enable)
            {
                pOutPacket->SetElementValue(AccNode, (char*)"Disable");
            }
            else
            {
                pOutPacket->SetElementValue(AccNode, (char*)"Enable");
            }

            /* ֡�� */
            AccNode = pOutPacket->CreateElement((char*)"FrameCount");
            snprintf(strFrameCount, 16, "%d", pGBLogicDeviceInfo->frame_count);
            pOutPacket->SetElementValue(AccNode, strFrameCount);

            /* �Ƿ�֧�ֶ����� */
            AccNode = pOutPacket->CreateElement((char*)"StreamCount");
            snprintf(strStreamCount, 16, "%d", pGBLogicDeviceInfo->stream_count);
            pOutPacket->SetElementValue(AccNode, strStreamCount);

            /* �澯ʱ�� */
            AccNode = pOutPacket->CreateElement((char*)"AlarmLengthOfTime");
            snprintf(strAlarmLengthOfTime, 16, "%d", pGBLogicDeviceInfo->alarm_duration);
            pOutPacket->SetElementValue(AccNode, strAlarmLengthOfTime);
        }

        /* �豸������ */
        AccNode = pOutPacket->CreateElement((char*)"Manufacturer");
        pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->manufacturer);

        /* �豸�ͺ� */
        AccNode = pOutPacket->CreateElement((char*)"Model");
        pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->model);

        /* �豸���� */
        AccNode = pOutPacket->CreateElement((char*)"Owner");
        pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->owner);

        /* �������� */
        AccNode = pOutPacket->CreateElement((char*)"CivilCode");

        if ('\0' == pGBLogicDeviceInfo->civil_code[0])
        {
            pOutPacket->SetElementValue(AccNode, local_civil_code_get());
        }
        else
        {
            pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->civil_code);
        }

        /* ���� */
        AccNode = pOutPacket->CreateElement((char*)"Block");
        pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->block);

        /* ��װ��ַ */
        AccNode = pOutPacket->CreateElement((char*)"Address");
        pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->address);

        /* �Ƿ������豸 */
        AccNode = pOutPacket->CreateElement((char*)"Parental");
        snprintf(strParental, 16, "%d", pGBLogicDeviceInfo->parental);
        pOutPacket->SetElementValue(AccNode, strParental);

        /* ���豸/����/ϵͳID, ������ƽ̨�Խӵ�ʱ��ͳһʹ�ñ���CMS ID */
        AccNode = pOutPacket->CreateElement((char*)"ParentID");

        if ('\0' == pGBLogicDeviceInfo->virtualParentID[0])
        {
            pOutPacket->SetElementValue(AccNode, local_cms_id_get());
        }
        else
        {
            pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->virtualParentID);
        }

        /* ���ȫģʽ*/
        AccNode = pOutPacket->CreateElement((char*)"SafetyWay");
        snprintf(strSafetyWay, 16, "%d", pGBLogicDeviceInfo->safety_way);
        pOutPacket->SetElementValue(AccNode, strSafetyWay);

        /* ע�᷽ʽ */
        AccNode = pOutPacket->CreateElement((char*)"RegisterWay");
        snprintf(strRegisterWay, 16, "%d", pGBLogicDeviceInfo->register_way);
        pOutPacket->SetElementValue(AccNode, strRegisterWay);

        /* ֤�����к�*/
        AccNode = pOutPacket->CreateElement((char*)"CertNum");
        pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->cert_num);

        /* ֤����Ч��ʶ */
        AccNode = pOutPacket->CreateElement((char*)"Certifiable");
        snprintf(strCertifiable, 16, "%d", pGBLogicDeviceInfo->certifiable);
        pOutPacket->SetElementValue(AccNode, strCertifiable);

        /* ��Чԭ���� */
        AccNode = pOutPacket->CreateElement((char*)"ErrCode");
        snprintf(strErrCode, 16, "%d", pGBLogicDeviceInfo->error_code);
        pOutPacket->SetElementValue(AccNode, strErrCode);

        /* ֤����ֹ��Ч��*/
        AccNode = pOutPacket->CreateElement((char*)"EndTime");
        pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->end_time);

        /* �������� */
        AccNode = pOutPacket->CreateElement((char*)"Secrecy");
        snprintf(strSecrecy, 16, "%d", pGBLogicDeviceInfo->secrecy);
        pOutPacket->SetElementValue(AccNode, strSecrecy);

        /* IP��ַ*/
        AccNode = pOutPacket->CreateElement((char*)"IPAddress");
        pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->ip_address);

        /* �˿ں� */
        AccNode = pOutPacket->CreateElement((char*)"Port");
        snprintf(strPort, 16, "%d", pGBLogicDeviceInfo->port);
        pOutPacket->SetElementValue(AccNode, strPort);

        /* ����*/
        AccNode = pOutPacket->CreateElement((char*)"Password");
        pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->password);

        /* ��λ״̬ */
        AccNode = pOutPacket->CreateElement((char*)"Status");

        if (iThreePartyFlag) /* ������ƽ̨ */
        {
            if (1 == pGBLogicDeviceInfo->status)
            {
                pOutPacket->SetElementValue(AccNode, (char*)"ON");
            }
            else if (2 == pGBLogicDeviceInfo->status)
            {
                pOutPacket->SetElementValue(AccNode, (char*)"VLOST");
            }
            else
            {
                pOutPacket->SetElementValue(AccNode, (char*)"OFF");
            }
        }
        else
        {
            if (1 == pGBLogicDeviceInfo->status)
            {
                if (INTELLIGENT_STATUS_ON == pGBLogicDeviceInfo->intelligent_status)
                {
                    pOutPacket->SetElementValue(AccNode, (char*)"INTELLIGENT");
                }
                else if (ALARM_STATUS_CLOSE == pGBLogicDeviceInfo->alarm_status)
                {
                    pOutPacket->SetElementValue(AccNode, (char*)"CLOSE");
                }
                else if (ALARM_STATUS_APART == pGBLogicDeviceInfo->alarm_status)
                {
                    pOutPacket->SetElementValue(AccNode, (char*)"APART");
                }
                else
                {
                    pOutPacket->SetElementValue(AccNode, (char*)"ON");
                }
            }
            else if (2 == pGBLogicDeviceInfo->status)
            {
                pOutPacket->SetElementValue(AccNode, (char*)"NOVIDEO");
            }
            else
            {
                pOutPacket->SetElementValue(AccNode, (char*)"OFF");
            }
        }

        /* ���� */
        AccNode = pOutPacket->CreateElement((char*)"Longitude");
        snprintf(strLongitude, 64, "%.16lf", pGBLogicDeviceInfo->longitude);
        pOutPacket->SetElementValue(AccNode, strLongitude);

        /* γ�� */
        AccNode = pOutPacket->CreateElement((char*)"Latitude");
        snprintf(strLatitude, 64, "%.16lf", pGBLogicDeviceInfo->latitude);
        pOutPacket->SetElementValue(AccNode, strLatitude);

        if (!iThreePartyFlag) /* �ǵ�����ƽ̨ */
        {
            /* ����ͼ�� */
            AccNode = pOutPacket->CreateElement((char*)"MapLayer");
            pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->map_layer);

            /* �����豸������ */
            AccNode = pOutPacket->CreateElement((char*)"ChlType");
            snprintf(strAlarmDeviceSubType, 64, "%u", pGBLogicDeviceInfo->alarm_device_sub_type);
            pOutPacket->SetElementValue(AccNode, strAlarmDeviceSubType);

            /* ������CMS ID */
            AccNode = pOutPacket->CreateElement((char*)"CMSID");
            pOutPacket->SetElementValue(AccNode, local_cms_id_get());

            /* ��չ��Info�ֶ� */
            pOutPacket->SetCurrentElement(ItemAccNode);
            ItemInfoNode = pOutPacket->CreateElement((char*)"Info");
            pOutPacket->SetCurrentElement(ItemInfoNode);

            /* �Ƿ�ɿ� */
            AccNode = pOutPacket->CreateElement((char*)"PTZType");

            if (pGBLogicDeviceInfo->ctrl_enable <= 0)
            {
                snprintf(strPTZType, 16, "%u", 3);
            }
            else
            {
                snprintf(strPTZType, 16, "%u", pGBLogicDeviceInfo->ctrl_enable);
            }

            pOutPacket->SetElementValue(AccNode, strPTZType);
        }
    }
    else
    {
        iRet = load_db_data_to_LogicDevice_info_list_by_device_id(pRoute_Srv_dboper, device_id);

        if (iRet == 0)
        {
            pGBLogicDeviceInfo = GBLogicDevice_info_find(device_id);

            if (NULL != pGBLogicDeviceInfo)
            {
                if (!iThreePartyFlag) /* �ǵ�����ƽ̨ */
                {
                    /* �豸���� */
                    AccNode = pOutPacket->CreateElement((char*)"ID");
                    snprintf(strID, 64, "%u", pGBLogicDeviceInfo->id);
                    pOutPacket->SetElementValue(AccNode, strID);
                }

                /* �豸ͳһ��� */
                AccNode = pOutPacket->CreateElement((char*)"DeviceID");
                pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->device_id);

                /* ��λ���� */
                AccNode = pOutPacket->CreateElement((char*)"Name");
                pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->device_name);

                if (!iThreePartyFlag) /* �ǵ�����ƽ̨ */
                {
                    /* �Ƿ�����*/
                    AccNode = pOutPacket->CreateElement((char*)"Enable");

                    if (0 == pGBLogicDeviceInfo->enable)
                    {
                        pOutPacket->SetElementValue(AccNode, (char*)"0");
                    }
                    else
                    {
                        pOutPacket->SetElementValue(AccNode, (char*)"1");
                    }

                    /* �Ƿ�ɿ� */
                    AccNode = pOutPacket->CreateElement((char*)"CtrlEnable");

                    if (1 == pGBLogicDeviceInfo->ctrl_enable)
                    {
                        pOutPacket->SetElementValue(AccNode, (char*)"Enable");
                    }
                    else
                    {
                        pOutPacket->SetElementValue(AccNode, (char*)"Disable");
                    }

                    /* �Ƿ�֧�ֶԽ� */
                    AccNode = pOutPacket->CreateElement((char*)"MicEnable");

                    if (0 == pGBLogicDeviceInfo->mic_enable)
                    {
                        pOutPacket->SetElementValue(AccNode, (char*)"Disable");
                    }
                    else
                    {
                        pOutPacket->SetElementValue(AccNode, (char*)"Enable");
                    }

                    /* ֡�� */
                    AccNode = pOutPacket->CreateElement((char*)"FrameCount");
                    snprintf(strFrameCount, 16, "%d", pGBLogicDeviceInfo->frame_count);
                    pOutPacket->SetElementValue(AccNode, strFrameCount);

                    /* �Ƿ�֧�ֶ����� */
                    AccNode = pOutPacket->CreateElement((char*)"StreamCount");
                    snprintf(strStreamCount, 16, "%d", pGBLogicDeviceInfo->stream_count);
                    pOutPacket->SetElementValue(AccNode, strStreamCount);

                    /* �澯ʱ�� */
                    AccNode = pOutPacket->CreateElement((char*)"AlarmLengthOfTime");
                    snprintf(strAlarmLengthOfTime, 16, "%d", pGBLogicDeviceInfo->alarm_duration);
                    pOutPacket->SetElementValue(AccNode, strAlarmLengthOfTime);
                }

                /* �豸������ */
                AccNode = pOutPacket->CreateElement((char*)"Manufacturer");
                pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->manufacturer);

                /* �豸�ͺ� */
                AccNode = pOutPacket->CreateElement((char*)"Model");
                pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->model);

                /* �豸���� */
                AccNode = pOutPacket->CreateElement((char*)"Owner");
                pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->owner);

                /* �������� */
                AccNode = pOutPacket->CreateElement((char*)"CivilCode");

                if ('\0' == pGBLogicDeviceInfo->civil_code[0])
                {
                    pOutPacket->SetElementValue(AccNode, local_civil_code_get());
                }
                else
                {
                    pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->civil_code);
                }

                /* ���� */
                AccNode = pOutPacket->CreateElement((char*)"Block");
                pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->block);

                /* ��װ��ַ */
                AccNode = pOutPacket->CreateElement((char*)"Address");
                pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->address);

                /* �Ƿ������豸 */
                AccNode = pOutPacket->CreateElement((char*)"Parental");
                snprintf(strParental, 16, "%d", pGBLogicDeviceInfo->parental);
                pOutPacket->SetElementValue(AccNode, strParental);

                /* ���豸/����/ϵͳID */
                AccNode = pOutPacket->CreateElement((char*)"ParentID");

                if ('\0' == pGBLogicDeviceInfo->virtualParentID[0])
                {
                    pOutPacket->SetElementValue(AccNode, local_cms_id_get());
                }
                else
                {
                    pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->virtualParentID);
                }

                /* ���ȫģʽ*/
                AccNode = pOutPacket->CreateElement((char*)"SafetyWay");
                snprintf(strSafetyWay, 16, "%d", pGBLogicDeviceInfo->safety_way);
                pOutPacket->SetElementValue(AccNode, strSafetyWay);

                /* ע�᷽ʽ */
                AccNode = pOutPacket->CreateElement((char*)"RegisterWay");
                snprintf(strRegisterWay, 16, "%d", pGBLogicDeviceInfo->register_way);
                pOutPacket->SetElementValue(AccNode, strRegisterWay);

                /* ֤�����к�*/
                AccNode = pOutPacket->CreateElement((char*)"CertNum");
                pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->cert_num);

                /* ֤����Ч��ʶ */
                AccNode = pOutPacket->CreateElement((char*)"Certifiable");
                snprintf(strCertifiable, 16, "%d", pGBLogicDeviceInfo->certifiable);
                pOutPacket->SetElementValue(AccNode, strCertifiable);

                /* ��Чԭ���� */
                AccNode = pOutPacket->CreateElement((char*)"ErrCode");
                snprintf(strErrCode, 16, "%d", pGBLogicDeviceInfo->error_code);
                pOutPacket->SetElementValue(AccNode, strErrCode);

                /* ֤����ֹ��Ч��*/
                AccNode = pOutPacket->CreateElement((char*)"EndTime");
                pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->end_time);

                /* �������� */
                AccNode = pOutPacket->CreateElement((char*)"Secrecy");
                snprintf(strSecrecy, 16, "%d", pGBLogicDeviceInfo->secrecy);
                pOutPacket->SetElementValue(AccNode, strSecrecy);

                /* IP��ַ*/
                AccNode = pOutPacket->CreateElement((char*)"IPAddress");
                pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->ip_address);

                /* �˿ں� */
                AccNode = pOutPacket->CreateElement((char*)"Port");
                snprintf(strPort, 16, "%d", pGBLogicDeviceInfo->port);
                pOutPacket->SetElementValue(AccNode, strPort);

                /* ����*/
                AccNode = pOutPacket->CreateElement((char*)"Password");
                pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->password);

                /* ��λ״̬ */
                AccNode = pOutPacket->CreateElement((char*)"Status");

                if (iThreePartyFlag) /* ������ƽ̨ */
                {
                    if (1 == pGBLogicDeviceInfo->status)
                    {
                        pOutPacket->SetElementValue(AccNode, (char*)"ON");
                    }
                    else if (2 == pGBLogicDeviceInfo->status)
                    {
                        pOutPacket->SetElementValue(AccNode, (char*)"VLOST");
                    }
                    else
                    {
                        pOutPacket->SetElementValue(AccNode, (char*)"OFF");
                    }
                }
                else
                {
                    if (1 == pGBLogicDeviceInfo->status)
                    {
                        if (INTELLIGENT_STATUS_ON == pGBLogicDeviceInfo->intelligent_status)
                        {
                            pOutPacket->SetElementValue(AccNode, (char*)"INTELLIGENT");
                        }
                        else if (ALARM_STATUS_CLOSE == pGBLogicDeviceInfo->alarm_status)
                        {
                            pOutPacket->SetElementValue(AccNode, (char*)"CLOSE");
                        }
                        else if (ALARM_STATUS_APART == pGBLogicDeviceInfo->alarm_status)
                        {
                            pOutPacket->SetElementValue(AccNode, (char*)"APART");
                        }
                        else
                        {
                            pOutPacket->SetElementValue(AccNode, (char*)"ON");
                        }
                    }
                    else if (2 == pGBLogicDeviceInfo->status)
                    {
                        pOutPacket->SetElementValue(AccNode, (char*)"NOVIDEO");
                    }
                    else
                    {
                        pOutPacket->SetElementValue(AccNode, (char*)"OFF");
                    }
                }

                /* ���� */
                AccNode = pOutPacket->CreateElement((char*)"Longitude");
                snprintf(strLongitude, 64, "%.16lf", pGBLogicDeviceInfo->longitude);
                pOutPacket->SetElementValue(AccNode, strLongitude);

                /* γ�� */
                AccNode = pOutPacket->CreateElement((char*)"Latitude");
                snprintf(strLatitude, 64, "%.16lf", pGBLogicDeviceInfo->latitude);
                pOutPacket->SetElementValue(AccNode, strLatitude);

                if (!iThreePartyFlag) /* �ǵ�����ƽ̨ */
                {
                    /* ����ͼ�� */
                    AccNode = pOutPacket->CreateElement((char*)"MapLayer");
                    pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->map_layer);

                    /* �����豸������ */
                    AccNode = pOutPacket->CreateElement((char*)"ChlType");
                    snprintf(strAlarmDeviceSubType, 64, "%u", pGBLogicDeviceInfo->alarm_device_sub_type);
                    pOutPacket->SetElementValue(AccNode, strAlarmDeviceSubType);

                    /* ������CMS ID */
                    AccNode = pOutPacket->CreateElement((char*)"CMSID");
                    pOutPacket->SetElementValue(AccNode, local_cms_id_get());

                    /* ��չ��Info�ֶ� */
                    pOutPacket->SetCurrentElement(ItemAccNode);
                    ItemInfoNode = pOutPacket->CreateElement((char*)"Info");
                    pOutPacket->SetCurrentElement(ItemInfoNode);

                    /* �Ƿ�ɿ� */
                    AccNode = pOutPacket->CreateElement((char*)"PTZType");

                    if (pGBLogicDeviceInfo->ctrl_enable <= 0)
                    {
                        snprintf(strPTZType, 16, "%u", 3);
                    }
                    else
                    {
                        snprintf(strPTZType, 16, "%u", pGBLogicDeviceInfo->ctrl_enable);
                    }

                    pOutPacket->SetElementValue(AccNode, strPTZType);
                }
            }
            else
            {
                if (!iThreePartyFlag) /* �ǵ�����ƽ̨ */
                {
                    /* �豸���� */
                    AccNode = pOutPacket->CreateElement((char*)"ID");
                    pOutPacket->SetElementValue(AccNode, (char*)"");
                }

                /* �豸ͳһ��� */
                AccNode = pOutPacket->CreateElement((char*)"DeviceID");
                pOutPacket->SetElementValue(AccNode, (char*)"");

                /* ��λ���� */
                AccNode = pOutPacket->CreateElement((char*)"Name");
                pOutPacket->SetElementValue(AccNode, (char*)"");

                /* �豸������ */
                AccNode = pOutPacket->CreateElement((char*)"Manufacturer");
                pOutPacket->SetElementValue(AccNode, (char*)"");

                /* �豸�ͺ� */
                AccNode = pOutPacket->CreateElement((char*)"Model");
                pOutPacket->SetElementValue(AccNode, (char*)"");

                /* �豸���� */
                AccNode = pOutPacket->CreateElement((char*)"Owner");
                pOutPacket->SetElementValue(AccNode, (char*)"");

                /* �������� */
                AccNode = pOutPacket->CreateElement((char*)"CivilCode");
                pOutPacket->SetElementValue(AccNode, (char*)"");

                /* ���� */
                AccNode = pOutPacket->CreateElement((char*)"Block");
                pOutPacket->SetElementValue(AccNode, (char*)"");

                /* ��װ��ַ */
                AccNode = pOutPacket->CreateElement((char*)"Address");
                pOutPacket->SetElementValue(AccNode, (char*)"");

                /* �Ƿ������豸 */
                AccNode = pOutPacket->CreateElement((char*)"Parental");
                pOutPacket->SetElementValue(AccNode, (char*)"");

                /* ���豸/����/ϵͳID */
                AccNode = pOutPacket->CreateElement((char*)"ParentID");
                pOutPacket->SetElementValue(AccNode, (char*)"");

                /* ���ȫģʽ*/
                AccNode = pOutPacket->CreateElement((char*)"SafetyWay");
                pOutPacket->SetElementValue(AccNode, (char*)"");

                /* ע�᷽ʽ */
                AccNode = pOutPacket->CreateElement((char*)"RegisterWay");
                pOutPacket->SetElementValue(AccNode, (char*)"");

                /* ֤�����к�*/
                AccNode = pOutPacket->CreateElement((char*)"CertNum");
                pOutPacket->SetElementValue(AccNode, (char*)"");

                /* ֤����Ч��ʶ */
                AccNode = pOutPacket->CreateElement((char*)"Certifiable");
                pOutPacket->SetElementValue(AccNode, (char*)"");

                /* ��Чԭ���� */
                AccNode = pOutPacket->CreateElement((char*)"ErrCode");
                pOutPacket->SetElementValue(AccNode, (char*)"");

                /* ֤����ֹ��Ч��*/
                AccNode = pOutPacket->CreateElement((char*)"EndTime");
                pOutPacket->SetElementValue(AccNode, (char*)"");

                /* �������� */
                AccNode = pOutPacket->CreateElement((char*)"Secrecy");
                pOutPacket->SetElementValue(AccNode, (char*)"");

                /* IP��ַ*/
                AccNode = pOutPacket->CreateElement((char*)"IPAddress");
                pOutPacket->SetElementValue(AccNode, (char*)"");

                /* �˿ں� */
                AccNode = pOutPacket->CreateElement((char*)"Port");
                pOutPacket->SetElementValue(AccNode, (char*)"");

                /* ����*/
                AccNode = pOutPacket->CreateElement((char*)"Password");
                pOutPacket->SetElementValue(AccNode, (char*)"");

                /* ��λ״̬ */
                AccNode = pOutPacket->CreateElement((char*)"Status");
                pOutPacket->SetElementValue(AccNode, (char*)"OFF");

                /* ���� */
                AccNode = pOutPacket->CreateElement((char*)"Longitude");
                pOutPacket->SetElementValue(AccNode, (char*)"");

                /* γ�� */
                AccNode = pOutPacket->CreateElement((char*)"Latitude");
                pOutPacket->SetElementValue(AccNode, (char*)"");

                if (!iThreePartyFlag) /* �ǵ�����ƽ̨ */
                {
                    /* ������CMS ID */
                    AccNode = pOutPacket->CreateElement((char*)"CMSID");
                    pOutPacket->SetElementValue(AccNode, (char*)"");
                }

                /* ��չ��Info�ֶ� */
                pOutPacket->SetCurrentElement(ItemAccNode);
                ItemInfoNode = pOutPacket->CreateElement((char*)"Info");
                pOutPacket->SetCurrentElement(ItemInfoNode);

                /* �Ƿ�ɿ� */
                AccNode = pOutPacket->CreateElement((char*)"PTZType");
                pOutPacket->SetElementValue(AccNode, (char*)"");
            }
        }
        else
        {
            if (!iThreePartyFlag) /* �ǵ�����ƽ̨ */
            {
                /* �豸���� */
                AccNode = pOutPacket->CreateElement((char*)"ID");
                pOutPacket->SetElementValue(AccNode, (char*)"");
            }

            /* �豸ͳһ��� */
            AccNode = pOutPacket->CreateElement((char*)"DeviceID");
            pOutPacket->SetElementValue(AccNode, (char*)"");

            /* ��λ���� */
            AccNode = pOutPacket->CreateElement((char*)"Name");
            pOutPacket->SetElementValue(AccNode, (char*)"");

            /* �豸������ */
            AccNode = pOutPacket->CreateElement((char*)"Manufacturer");
            pOutPacket->SetElementValue(AccNode, (char*)"");

            /* �豸�ͺ� */
            AccNode = pOutPacket->CreateElement((char*)"Model");
            pOutPacket->SetElementValue(AccNode, (char*)"");

            /* �豸���� */
            AccNode = pOutPacket->CreateElement((char*)"Owner");
            pOutPacket->SetElementValue(AccNode, (char*)"");

            /* �������� */
            AccNode = pOutPacket->CreateElement((char*)"CivilCode");
            pOutPacket->SetElementValue(AccNode, (char*)"");

            /* ���� */
            AccNode = pOutPacket->CreateElement((char*)"Block");
            pOutPacket->SetElementValue(AccNode, (char*)"");

            /* ��װ��ַ */
            AccNode = pOutPacket->CreateElement((char*)"Address");
            pOutPacket->SetElementValue(AccNode, (char*)"");

            /* �Ƿ������豸 */
            AccNode = pOutPacket->CreateElement((char*)"Parental");
            pOutPacket->SetElementValue(AccNode, (char*)"");

            /* ���豸/����/ϵͳID */
            AccNode = pOutPacket->CreateElement((char*)"ParentID");
            pOutPacket->SetElementValue(AccNode, (char*)"");

            /* ���ȫģʽ*/
            AccNode = pOutPacket->CreateElement((char*)"SafetyWay");
            pOutPacket->SetElementValue(AccNode, (char*)"");

            /* ע�᷽ʽ */
            AccNode = pOutPacket->CreateElement((char*)"RegisterWay");
            pOutPacket->SetElementValue(AccNode, (char*)"");

            /* ֤�����к�*/
            AccNode = pOutPacket->CreateElement((char*)"CertNum");
            pOutPacket->SetElementValue(AccNode, (char*)"");

            /* ֤����Ч��ʶ */
            AccNode = pOutPacket->CreateElement((char*)"Certifiable");
            pOutPacket->SetElementValue(AccNode, (char*)"");

            /* ��Чԭ���� */
            AccNode = pOutPacket->CreateElement((char*)"ErrCode");
            pOutPacket->SetElementValue(AccNode, (char*)"");

            /* ֤����ֹ��Ч��*/
            AccNode = pOutPacket->CreateElement((char*)"EndTime");
            pOutPacket->SetElementValue(AccNode, (char*)"");

            /* �������� */
            AccNode = pOutPacket->CreateElement((char*)"Secrecy");
            pOutPacket->SetElementValue(AccNode, (char*)"");

            /* IP��ַ*/
            AccNode = pOutPacket->CreateElement((char*)"IPAddress");
            pOutPacket->SetElementValue(AccNode, (char*)"");

            /* �˿ں� */
            AccNode = pOutPacket->CreateElement((char*)"Port");
            pOutPacket->SetElementValue(AccNode, (char*)"");

            /* ����*/
            AccNode = pOutPacket->CreateElement((char*)"Password");
            pOutPacket->SetElementValue(AccNode, (char*)"");

            /* ��λ״̬ */
            AccNode = pOutPacket->CreateElement((char*)"Status");
            pOutPacket->SetElementValue(AccNode, (char*)"OFF");

            /* ���� */
            AccNode = pOutPacket->CreateElement((char*)"Longitude");
            pOutPacket->SetElementValue(AccNode, (char*)"");

            /* γ�� */
            AccNode = pOutPacket->CreateElement((char*)"Latitude");
            pOutPacket->SetElementValue(AccNode, (char*)"");

            if (!iThreePartyFlag) /* �ǵ�����ƽ̨ */
            {
                /* ������CMS ID */
                AccNode = pOutPacket->CreateElement((char*)"CMSID");
                pOutPacket->SetElementValue(AccNode, (char*)"");
            }

            /* ��չ��Info�ֶ� */
            pOutPacket->SetCurrentElement(ItemAccNode);
            ItemInfoNode = pOutPacket->CreateElement((char*)"Info");
            pOutPacket->SetCurrentElement(ItemInfoNode);

            /* �Ƿ�ɿ� */
            AccNode = pOutPacket->CreateElement((char*)"PTZType");
            pOutPacket->SetElementValue(AccNode, (char*)"");
        }
    }

    return 0;
}

/*****************************************************************************
 �� �� ��  : AddLogicDeviceInfoToXMLItemForRouteNotify
 ��������  : ����߼��豸��Ϣ��XML��Item
 �������  : CPacket* pOutPacket
             DOMElement* ListAccNode
             char* device_id
             int iThreePartyFlag
             DBOper* pRoute_Srv_dboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��7��27�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int AddLogicDeviceInfoToXMLItemForRouteNotify(CPacket* pOutPacket, DOMElement* ListAccNode, char* device_id, int iThreePartyFlag, DBOper* pRoute_Srv_dboper)
{
    int iRet = 0;
    GBLogicDevice_info_t* pGBLogicDeviceInfo = NULL;
    //GBLogicDevice_info_t* pDBGBLogicDeviceInfo = NULL;
    DOMElement* ItemAccNode = NULL;
    DOMElement* AccNode = NULL;
    DOMElement* ItemInfoNode = NULL;

    char strID[64] = {0};
    char strParental[16] = {0};
    char strSafetyWay[16] = {0};
    char strRegisterWay[16] = {0};
    char strCertifiable[16] = {0};
    char strErrCode[16] = {0};
    char strSecrecy[16] = {0};
    char strPort[16] = {0};
    char strLongitude[64] = {0};
    char strLatitude[64] = {0};
    char strAlarmDeviceSubType[64] = {0};
    char strStreamCount[16] = {0};
    char strFrameCount[16] = {0};
    char strAlarmLengthOfTime[16] = {0};
    char strPTZType[16] = {0};

    if (NULL == pOutPacket || NULL == ListAccNode || NULL == device_id)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "AddLogicDeviceInfoToXMLItemForRouteNotify() exit---: Param Error \r\n");
        return -1;
    }

    /* ��дXML����*/
    pOutPacket->SetCurrentElement(ListAccNode);
    ItemAccNode = pOutPacket->CreateElement((char*)"Item");
    pOutPacket->SetCurrentElement(ItemAccNode);

    /* ����Index ��ȡ�߼��豸��Ϣ��������ֻ�����������豸����û�����ߣ����ݿ���ڴ��ж�û�е�*/
    pGBLogicDeviceInfo = GBLogicDevice_info_find(device_id);

    if (NULL != pGBLogicDeviceInfo)
    {
        /* �¼����� */
        AccNode = pOutPacket->CreateElement((char*)"Event");
        pOutPacket->SetElementValue(AccNode, (char*)"ADD");

        if (!iThreePartyFlag) /* �ǵ�����ƽ̨ */
        {
            /* �豸���� */
            AccNode = pOutPacket->CreateElement((char*)"ID");
            snprintf(strID, 64, "%u", pGBLogicDeviceInfo->id);
            pOutPacket->SetElementValue(AccNode, strID);
        }

        /* �豸ͳһ��� */
        AccNode = pOutPacket->CreateElement((char*)"DeviceID");
        pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->device_id);

        /* ��λ���� */
        AccNode = pOutPacket->CreateElement((char*)"Name");
        pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->device_name);

        if (!iThreePartyFlag) /* �ǵ�����ƽ̨ */
        {
            /* �Ƿ�����*/
            AccNode = pOutPacket->CreateElement((char*)"Enable");

            if (0 == pGBLogicDeviceInfo->enable)
            {
                pOutPacket->SetElementValue(AccNode, (char*)"0");
            }
            else
            {
                pOutPacket->SetElementValue(AccNode, (char*)"1");
            }

            /* �Ƿ�ɿ� */
            AccNode = pOutPacket->CreateElement((char*)"CtrlEnable");

            if (1 == pGBLogicDeviceInfo->ctrl_enable)
            {
                pOutPacket->SetElementValue(AccNode, (char*)"Enable");
            }
            else
            {
                pOutPacket->SetElementValue(AccNode, (char*)"Disable");
            }

            /* �Ƿ�֧�ֶԽ� */
            AccNode = pOutPacket->CreateElement((char*)"MicEnable");

            if (0 == pGBLogicDeviceInfo->mic_enable)
            {
                pOutPacket->SetElementValue(AccNode, (char*)"Disable");
            }
            else
            {
                pOutPacket->SetElementValue(AccNode, (char*)"Enable");
            }

            /* ֡�� */
            AccNode = pOutPacket->CreateElement((char*)"FrameCount");
            snprintf(strFrameCount, 16, "%d", pGBLogicDeviceInfo->frame_count);
            pOutPacket->SetElementValue(AccNode, strFrameCount);

            /* �Ƿ�֧�ֶ����� */
            AccNode = pOutPacket->CreateElement((char*)"StreamCount");
            snprintf(strStreamCount, 16, "%d", pGBLogicDeviceInfo->stream_count);
            pOutPacket->SetElementValue(AccNode, strStreamCount);

            /* �澯ʱ�� */
            AccNode = pOutPacket->CreateElement((char*)"AlarmLengthOfTime");
            snprintf(strAlarmLengthOfTime, 16, "%d", pGBLogicDeviceInfo->alarm_duration);
            pOutPacket->SetElementValue(AccNode, strAlarmLengthOfTime);
        }

        /* �豸������ */
        AccNode = pOutPacket->CreateElement((char*)"Manufacturer");
        pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->manufacturer);

        /* �豸�ͺ� */
        AccNode = pOutPacket->CreateElement((char*)"Model");
        pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->model);

        /* �豸���� */
        AccNode = pOutPacket->CreateElement((char*)"Owner");
        pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->owner);

        /* �������� */
        AccNode = pOutPacket->CreateElement((char*)"CivilCode");

        if ('\0' == pGBLogicDeviceInfo->civil_code[0])
        {
            pOutPacket->SetElementValue(AccNode, local_civil_code_get());
        }
        else
        {
            pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->civil_code);
        }

        /* ���� */
        AccNode = pOutPacket->CreateElement((char*)"Block");
        pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->block);

        /* ��װ��ַ */
        AccNode = pOutPacket->CreateElement((char*)"Address");
        pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->address);

        /* �Ƿ������豸 */
        AccNode = pOutPacket->CreateElement((char*)"Parental");
        snprintf(strParental, 16, "%d", pGBLogicDeviceInfo->parental);
        pOutPacket->SetElementValue(AccNode, strParental);

        /* ���豸/����/ϵͳID, ������ƽ̨�Խӵ�ʱ��ͳһʹ�ñ���CMS ID */
        AccNode = pOutPacket->CreateElement((char*)"ParentID");

        if ('\0' == pGBLogicDeviceInfo->virtualParentID[0])
        {
            pOutPacket->SetElementValue(AccNode, local_cms_id_get());
        }
        else
        {
            pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->virtualParentID);
        }

        /* ���ȫģʽ*/
        AccNode = pOutPacket->CreateElement((char*)"SafetyWay");
        snprintf(strSafetyWay, 16, "%d", pGBLogicDeviceInfo->safety_way);
        pOutPacket->SetElementValue(AccNode, strSafetyWay);

        /* ע�᷽ʽ */
        AccNode = pOutPacket->CreateElement((char*)"RegisterWay");
        snprintf(strRegisterWay, 16, "%d", pGBLogicDeviceInfo->register_way);
        pOutPacket->SetElementValue(AccNode, strRegisterWay);

        /* ֤�����к�*/
        AccNode = pOutPacket->CreateElement((char*)"CertNum");
        pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->cert_num);

        /* ֤����Ч��ʶ */
        AccNode = pOutPacket->CreateElement((char*)"Certifiable");
        snprintf(strCertifiable, 16, "%d", pGBLogicDeviceInfo->certifiable);
        pOutPacket->SetElementValue(AccNode, strCertifiable);

        /* ��Чԭ���� */
        AccNode = pOutPacket->CreateElement((char*)"ErrCode");
        snprintf(strErrCode, 16, "%d", pGBLogicDeviceInfo->error_code);
        pOutPacket->SetElementValue(AccNode, strErrCode);

        /* ֤����ֹ��Ч��*/
        AccNode = pOutPacket->CreateElement((char*)"EndTime");
        pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->end_time);

        /* �������� */
        AccNode = pOutPacket->CreateElement((char*)"Secrecy");
        snprintf(strSecrecy, 16, "%d", pGBLogicDeviceInfo->secrecy);
        pOutPacket->SetElementValue(AccNode, strSecrecy);

        /* IP��ַ*/
        AccNode = pOutPacket->CreateElement((char*)"IPAddress");
        pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->ip_address);

        /* �˿ں� */
        AccNode = pOutPacket->CreateElement((char*)"Port");
        snprintf(strPort, 16, "%d", pGBLogicDeviceInfo->port);
        pOutPacket->SetElementValue(AccNode, strPort);

        /* ����*/
        AccNode = pOutPacket->CreateElement((char*)"Password");
        pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->password);

        /* ��λ״̬ */
        AccNode = pOutPacket->CreateElement((char*)"Status");

        if (iThreePartyFlag) /* ������ƽ̨ */
        {
            if (1 == pGBLogicDeviceInfo->status)
            {
                pOutPacket->SetElementValue(AccNode, (char*)"ON");
            }
            else if (2 == pGBLogicDeviceInfo->status)
            {
                pOutPacket->SetElementValue(AccNode, (char*)"VLOST");
            }
            else
            {
                pOutPacket->SetElementValue(AccNode, (char*)"OFF");
            }
        }
        else
        {
            if (1 == pGBLogicDeviceInfo->status)
            {
                if (INTELLIGENT_STATUS_ON == pGBLogicDeviceInfo->intelligent_status)
                {
                    pOutPacket->SetElementValue(AccNode, (char*)"INTELLIGENT");
                }
                else if (ALARM_STATUS_CLOSE == pGBLogicDeviceInfo->alarm_status)
                {
                    pOutPacket->SetElementValue(AccNode, (char*)"CLOSE");
                }
                else if (ALARM_STATUS_APART == pGBLogicDeviceInfo->alarm_status)
                {
                    pOutPacket->SetElementValue(AccNode, (char*)"APART");
                }
                else
                {
                    pOutPacket->SetElementValue(AccNode, (char*)"ON");
                }
            }
            else if (2 == pGBLogicDeviceInfo->status)
            {
                pOutPacket->SetElementValue(AccNode, (char*)"NOVIDEO");
            }
            else
            {
                pOutPacket->SetElementValue(AccNode, (char*)"OFF");
            }
        }

        /* ���� */
        AccNode = pOutPacket->CreateElement((char*)"Longitude");
        snprintf(strLongitude, 64, "%.16lf", pGBLogicDeviceInfo->longitude);
        pOutPacket->SetElementValue(AccNode, strLongitude);

        /* γ�� */
        AccNode = pOutPacket->CreateElement((char*)"Latitude");
        snprintf(strLatitude, 64, "%.16lf", pGBLogicDeviceInfo->latitude);
        pOutPacket->SetElementValue(AccNode, strLatitude);

        if (!iThreePartyFlag) /* �ǵ�����ƽ̨ */
        {
            /* ����ͼ�� */
            AccNode = pOutPacket->CreateElement((char*)"MapLayer");
            pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->map_layer);

            /* �����豸������ */
            AccNode = pOutPacket->CreateElement((char*)"ChlType");
            snprintf(strAlarmDeviceSubType, 64, "%u", pGBLogicDeviceInfo->alarm_device_sub_type);
            pOutPacket->SetElementValue(AccNode, strAlarmDeviceSubType);

            /* ������CMS ID */
            AccNode = pOutPacket->CreateElement((char*)"CMSID");
            pOutPacket->SetElementValue(AccNode, local_cms_id_get());
        }

        /* ��չ��Info�ֶ� */
        pOutPacket->SetCurrentElement(ItemAccNode);
        ItemInfoNode = pOutPacket->CreateElement((char*)"Info");
        pOutPacket->SetCurrentElement(ItemInfoNode);

        /* �Ƿ�ɿ� */
        AccNode = pOutPacket->CreateElement((char*)"PTZType");

        if (pGBLogicDeviceInfo->ctrl_enable <= 0)
        {
            snprintf(strPTZType, 16, "%u", 3);
        }
        else
        {
            snprintf(strPTZType, 16, "%u", pGBLogicDeviceInfo->ctrl_enable);
        }

        pOutPacket->SetElementValue(AccNode, strPTZType);
    }
    else
    {
        iRet = load_db_data_to_LogicDevice_info_list_by_device_id(pRoute_Srv_dboper, device_id);

        if (iRet == 0)
        {
            pGBLogicDeviceInfo = GBLogicDevice_info_find(device_id);

            if (NULL != pGBLogicDeviceInfo)
            {
                if (!iThreePartyFlag) /* �ǵ�����ƽ̨ */
                {
                    /* �豸���� */
                    AccNode = pOutPacket->CreateElement((char*)"ID");
                    snprintf(strID, 64, "%u", pGBLogicDeviceInfo->id);
                    pOutPacket->SetElementValue(AccNode, strID);
                }

                /* �豸ͳһ��� */
                AccNode = pOutPacket->CreateElement((char*)"DeviceID");
                pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->device_id);

                /* ��λ���� */
                AccNode = pOutPacket->CreateElement((char*)"Name");
                pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->device_name);

                if (!iThreePartyFlag) /* �ǵ�����ƽ̨ */
                {
                    /* �Ƿ�����*/
                    AccNode = pOutPacket->CreateElement((char*)"Enable");

                    if (0 == pGBLogicDeviceInfo->enable)
                    {
                        pOutPacket->SetElementValue(AccNode, (char*)"0");
                    }
                    else
                    {
                        pOutPacket->SetElementValue(AccNode, (char*)"1");
                    }

                    /* �Ƿ�ɿ� */
                    AccNode = pOutPacket->CreateElement((char*)"CtrlEnable");

                    if (1 == pGBLogicDeviceInfo->ctrl_enable)
                    {
                        pOutPacket->SetElementValue(AccNode, (char*)"Enable");
                    }
                    else
                    {
                        pOutPacket->SetElementValue(AccNode, (char*)"Disable");
                    }

                    /* �Ƿ�֧�ֶԽ� */
                    AccNode = pOutPacket->CreateElement((char*)"MicEnable");

                    if (0 == pGBLogicDeviceInfo->mic_enable)
                    {
                        pOutPacket->SetElementValue(AccNode, (char*)"Disable");
                    }
                    else
                    {
                        pOutPacket->SetElementValue(AccNode, (char*)"Enable");
                    }

                    /* ֡�� */
                    AccNode = pOutPacket->CreateElement((char*)"FrameCount");
                    snprintf(strFrameCount, 16, "%d", pGBLogicDeviceInfo->frame_count);
                    pOutPacket->SetElementValue(AccNode, strFrameCount);

                    /* �Ƿ�֧�ֶ����� */
                    AccNode = pOutPacket->CreateElement((char*)"StreamCount");
                    snprintf(strStreamCount, 16, "%d", pGBLogicDeviceInfo->stream_count);
                    pOutPacket->SetElementValue(AccNode, strStreamCount);

                    /* �澯ʱ�� */
                    AccNode = pOutPacket->CreateElement((char*)"AlarmLengthOfTime");
                    snprintf(strAlarmLengthOfTime, 16, "%d", pGBLogicDeviceInfo->alarm_duration);
                    pOutPacket->SetElementValue(AccNode, strAlarmLengthOfTime);
                }

                /* �豸������ */
                AccNode = pOutPacket->CreateElement((char*)"Manufacturer");
                pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->manufacturer);

                /* �豸�ͺ� */
                AccNode = pOutPacket->CreateElement((char*)"Model");
                pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->model);

                /* �豸���� */
                AccNode = pOutPacket->CreateElement((char*)"Owner");
                pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->owner);

                /* �������� */
                AccNode = pOutPacket->CreateElement((char*)"CivilCode");

                if ('\0' == pGBLogicDeviceInfo->civil_code[0])
                {
                    pOutPacket->SetElementValue(AccNode, local_civil_code_get());
                }
                else
                {
                    pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->civil_code);
                }

                /* ���� */
                AccNode = pOutPacket->CreateElement((char*)"Block");
                pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->block);

                /* ��װ��ַ */
                AccNode = pOutPacket->CreateElement((char*)"Address");
                pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->address);

                /* �Ƿ������豸 */
                AccNode = pOutPacket->CreateElement((char*)"Parental");
                snprintf(strParental, 16, "%d", pGBLogicDeviceInfo->parental);
                pOutPacket->SetElementValue(AccNode, strParental);

                /* ���豸/����/ϵͳID */
                AccNode = pOutPacket->CreateElement((char*)"ParentID");

                if ('\0' == pGBLogicDeviceInfo->virtualParentID[0])
                {
                    pOutPacket->SetElementValue(AccNode, local_cms_id_get());
                }
                else
                {
                    pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->virtualParentID);
                }

                /* ���ȫģʽ*/
                AccNode = pOutPacket->CreateElement((char*)"SafetyWay");
                snprintf(strSafetyWay, 16, "%d", pGBLogicDeviceInfo->safety_way);
                pOutPacket->SetElementValue(AccNode, strSafetyWay);

                /* ע�᷽ʽ */
                AccNode = pOutPacket->CreateElement((char*)"RegisterWay");
                snprintf(strRegisterWay, 16, "%d", pGBLogicDeviceInfo->register_way);
                pOutPacket->SetElementValue(AccNode, strRegisterWay);

                /* ֤�����к�*/
                AccNode = pOutPacket->CreateElement((char*)"CertNum");
                pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->cert_num);

                /* ֤����Ч��ʶ */
                AccNode = pOutPacket->CreateElement((char*)"Certifiable");
                snprintf(strCertifiable, 16, "%d", pGBLogicDeviceInfo->certifiable);
                pOutPacket->SetElementValue(AccNode, strCertifiable);

                /* ��Чԭ���� */
                AccNode = pOutPacket->CreateElement((char*)"ErrCode");
                snprintf(strErrCode, 16, "%d", pGBLogicDeviceInfo->error_code);
                pOutPacket->SetElementValue(AccNode, strErrCode);

                /* ֤����ֹ��Ч��*/
                AccNode = pOutPacket->CreateElement((char*)"EndTime");
                pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->end_time);

                /* �������� */
                AccNode = pOutPacket->CreateElement((char*)"Secrecy");
                snprintf(strSecrecy, 16, "%d", pGBLogicDeviceInfo->secrecy);
                pOutPacket->SetElementValue(AccNode, strSecrecy);

                /* IP��ַ*/
                AccNode = pOutPacket->CreateElement((char*)"IPAddress");
                pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->ip_address);

                /* �˿ں� */
                AccNode = pOutPacket->CreateElement((char*)"Port");
                snprintf(strPort, 16, "%d", pGBLogicDeviceInfo->port);
                pOutPacket->SetElementValue(AccNode, strPort);

                /* ����*/
                AccNode = pOutPacket->CreateElement((char*)"Password");
                pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->password);

                /* ��λ״̬ */
                AccNode = pOutPacket->CreateElement((char*)"Status");

                if (iThreePartyFlag) /* ������ƽ̨ */
                {
                    if (1 == pGBLogicDeviceInfo->status)
                    {
                        pOutPacket->SetElementValue(AccNode, (char*)"ON");
                    }
                    else if (2 == pGBLogicDeviceInfo->status)
                    {
                        pOutPacket->SetElementValue(AccNode, (char*)"VLOST");
                    }
                    else
                    {
                        pOutPacket->SetElementValue(AccNode, (char*)"OFF");
                    }
                }
                else
                {
                    if (1 == pGBLogicDeviceInfo->status)
                    {
                        if (INTELLIGENT_STATUS_ON == pGBLogicDeviceInfo->intelligent_status)
                        {
                            pOutPacket->SetElementValue(AccNode, (char*)"INTELLIGENT");
                        }
                        else if (ALARM_STATUS_CLOSE == pGBLogicDeviceInfo->alarm_status)
                        {
                            pOutPacket->SetElementValue(AccNode, (char*)"CLOSE");
                        }
                        else if (ALARM_STATUS_APART == pGBLogicDeviceInfo->alarm_status)
                        {
                            pOutPacket->SetElementValue(AccNode, (char*)"APART");
                        }
                        else
                        {
                            pOutPacket->SetElementValue(AccNode, (char*)"ON");
                        }
                    }
                    else if (2 == pGBLogicDeviceInfo->status)
                    {
                        pOutPacket->SetElementValue(AccNode, (char*)"NOVIDEO");
                    }
                    else
                    {
                        pOutPacket->SetElementValue(AccNode, (char*)"OFF");
                    }
                }

                /* ���� */
                AccNode = pOutPacket->CreateElement((char*)"Longitude");
                snprintf(strLongitude, 64, "%.16lf", pGBLogicDeviceInfo->longitude);
                pOutPacket->SetElementValue(AccNode, strLongitude);

                /* γ�� */
                AccNode = pOutPacket->CreateElement((char*)"Latitude");
                snprintf(strLatitude, 64, "%.16lf", pGBLogicDeviceInfo->latitude);
                pOutPacket->SetElementValue(AccNode, strLatitude);

                if (!iThreePartyFlag) /* �ǵ�����ƽ̨ */
                {
                    /* ����ͼ�� */
                    AccNode = pOutPacket->CreateElement((char*)"MapLayer");
                    pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->map_layer);

                    /* �����豸������ */
                    AccNode = pOutPacket->CreateElement((char*)"ChlType");
                    snprintf(strAlarmDeviceSubType, 64, "%u", pGBLogicDeviceInfo->alarm_device_sub_type);
                    pOutPacket->SetElementValue(AccNode, strAlarmDeviceSubType);

                    /* ������CMS ID */
                    AccNode = pOutPacket->CreateElement((char*)"CMSID");
                    pOutPacket->SetElementValue(AccNode, local_cms_id_get());
                }

                /* ��չ��Info�ֶ� */
                pOutPacket->SetCurrentElement(ItemAccNode);
                ItemInfoNode = pOutPacket->CreateElement((char*)"Info");
                pOutPacket->SetCurrentElement(ItemInfoNode);

                /* �Ƿ�ɿ� */
                AccNode = pOutPacket->CreateElement((char*)"PTZType");

                if (pGBLogicDeviceInfo->ctrl_enable <= 0)
                {
                    snprintf(strPTZType, 16, "%u", 3);
                }
                else
                {
                    snprintf(strPTZType, 16, "%u", pGBLogicDeviceInfo->ctrl_enable);
                }

                pOutPacket->SetElementValue(AccNode, strPTZType);
            }
            else
            {
                if (!iThreePartyFlag) /* �ǵ�����ƽ̨ */
                {
                    /* �豸���� */
                    AccNode = pOutPacket->CreateElement((char*)"ID");
                    pOutPacket->SetElementValue(AccNode, (char*)"");
                }

                /* �豸ͳһ��� */
                AccNode = pOutPacket->CreateElement((char*)"DeviceID");
                pOutPacket->SetElementValue(AccNode, (char*)"");

                /* ��λ���� */
                AccNode = pOutPacket->CreateElement((char*)"Name");
                pOutPacket->SetElementValue(AccNode, (char*)"");

                /* �豸������ */
                AccNode = pOutPacket->CreateElement((char*)"Manufacturer");
                pOutPacket->SetElementValue(AccNode, (char*)"");

                /* �豸�ͺ� */
                AccNode = pOutPacket->CreateElement((char*)"Model");
                pOutPacket->SetElementValue(AccNode, (char*)"");

                /* �豸���� */
                AccNode = pOutPacket->CreateElement((char*)"Owner");
                pOutPacket->SetElementValue(AccNode, (char*)"");

                /* �������� */
                AccNode = pOutPacket->CreateElement((char*)"CivilCode");
                pOutPacket->SetElementValue(AccNode, (char*)"");

                /* ���� */
                AccNode = pOutPacket->CreateElement((char*)"Block");
                pOutPacket->SetElementValue(AccNode, (char*)"");

                /* ��װ��ַ */
                AccNode = pOutPacket->CreateElement((char*)"Address");
                pOutPacket->SetElementValue(AccNode, (char*)"");

                /* �Ƿ������豸 */
                AccNode = pOutPacket->CreateElement((char*)"Parental");
                pOutPacket->SetElementValue(AccNode, (char*)"");

                /* ���豸/����/ϵͳID */
                AccNode = pOutPacket->CreateElement((char*)"ParentID");
                pOutPacket->SetElementValue(AccNode, (char*)"");

                /* ���ȫģʽ*/
                AccNode = pOutPacket->CreateElement((char*)"SafetyWay");
                pOutPacket->SetElementValue(AccNode, (char*)"");

                /* ע�᷽ʽ */
                AccNode = pOutPacket->CreateElement((char*)"RegisterWay");
                pOutPacket->SetElementValue(AccNode, (char*)"");

                /* ֤�����к�*/
                AccNode = pOutPacket->CreateElement((char*)"CertNum");
                pOutPacket->SetElementValue(AccNode, (char*)"");

                /* ֤����Ч��ʶ */
                AccNode = pOutPacket->CreateElement((char*)"Certifiable");
                pOutPacket->SetElementValue(AccNode, (char*)"");

                /* ��Чԭ���� */
                AccNode = pOutPacket->CreateElement((char*)"ErrCode");
                pOutPacket->SetElementValue(AccNode, (char*)"");

                /* ֤����ֹ��Ч��*/
                AccNode = pOutPacket->CreateElement((char*)"EndTime");
                pOutPacket->SetElementValue(AccNode, (char*)"");

                /* �������� */
                AccNode = pOutPacket->CreateElement((char*)"Secrecy");
                pOutPacket->SetElementValue(AccNode, (char*)"");

                /* IP��ַ*/
                AccNode = pOutPacket->CreateElement((char*)"IPAddress");
                pOutPacket->SetElementValue(AccNode, (char*)"");

                /* �˿ں� */
                AccNode = pOutPacket->CreateElement((char*)"Port");
                pOutPacket->SetElementValue(AccNode, (char*)"");

                /* ����*/
                AccNode = pOutPacket->CreateElement((char*)"Password");
                pOutPacket->SetElementValue(AccNode, (char*)"");

                /* ��λ״̬ */
                AccNode = pOutPacket->CreateElement((char*)"Status");
                pOutPacket->SetElementValue(AccNode, (char*)"OFF");

                /* ���� */
                AccNode = pOutPacket->CreateElement((char*)"Longitude");
                pOutPacket->SetElementValue(AccNode, (char*)"");

                /* γ�� */
                AccNode = pOutPacket->CreateElement((char*)"Latitude");
                pOutPacket->SetElementValue(AccNode, (char*)"");

                if (!iThreePartyFlag) /* �ǵ�����ƽ̨ */
                {
                    /* ������CMS ID */
                    AccNode = pOutPacket->CreateElement((char*)"CMSID");
                    pOutPacket->SetElementValue(AccNode, (char*)"");
                }

                /* ��չ��Info�ֶ� */
                pOutPacket->SetCurrentElement(ItemAccNode);
                ItemInfoNode = pOutPacket->CreateElement((char*)"Info");
                pOutPacket->SetCurrentElement(ItemInfoNode);

                /* �Ƿ�ɿ� */
                AccNode = pOutPacket->CreateElement((char*)"PTZType");
                pOutPacket->SetElementValue(AccNode, (char*)"");
            }
        }
        else
        {
            if (!iThreePartyFlag) /* �ǵ�����ƽ̨ */
            {
                /* �豸���� */
                AccNode = pOutPacket->CreateElement((char*)"ID");
                pOutPacket->SetElementValue(AccNode, (char*)"");
            }

            /* �豸ͳһ��� */
            AccNode = pOutPacket->CreateElement((char*)"DeviceID");
            pOutPacket->SetElementValue(AccNode, (char*)"");

            /* ��λ���� */
            AccNode = pOutPacket->CreateElement((char*)"Name");
            pOutPacket->SetElementValue(AccNode, (char*)"");

            /* �豸������ */
            AccNode = pOutPacket->CreateElement((char*)"Manufacturer");
            pOutPacket->SetElementValue(AccNode, (char*)"");

            /* �豸�ͺ� */
            AccNode = pOutPacket->CreateElement((char*)"Model");
            pOutPacket->SetElementValue(AccNode, (char*)"");

            /* �豸���� */
            AccNode = pOutPacket->CreateElement((char*)"Owner");
            pOutPacket->SetElementValue(AccNode, (char*)"");

            /* �������� */
            AccNode = pOutPacket->CreateElement((char*)"CivilCode");
            pOutPacket->SetElementValue(AccNode, (char*)"");

            /* ���� */
            AccNode = pOutPacket->CreateElement((char*)"Block");
            pOutPacket->SetElementValue(AccNode, (char*)"");

            /* ��װ��ַ */
            AccNode = pOutPacket->CreateElement((char*)"Address");
            pOutPacket->SetElementValue(AccNode, (char*)"");

            /* �Ƿ������豸 */
            AccNode = pOutPacket->CreateElement((char*)"Parental");
            pOutPacket->SetElementValue(AccNode, (char*)"");

            /* ���豸/����/ϵͳID */
            AccNode = pOutPacket->CreateElement((char*)"ParentID");
            pOutPacket->SetElementValue(AccNode, (char*)"");

            /* ���ȫģʽ*/
            AccNode = pOutPacket->CreateElement((char*)"SafetyWay");
            pOutPacket->SetElementValue(AccNode, (char*)"");

            /* ע�᷽ʽ */
            AccNode = pOutPacket->CreateElement((char*)"RegisterWay");
            pOutPacket->SetElementValue(AccNode, (char*)"");

            /* ֤�����к�*/
            AccNode = pOutPacket->CreateElement((char*)"CertNum");
            pOutPacket->SetElementValue(AccNode, (char*)"");

            /* ֤����Ч��ʶ */
            AccNode = pOutPacket->CreateElement((char*)"Certifiable");
            pOutPacket->SetElementValue(AccNode, (char*)"");

            /* ��Чԭ���� */
            AccNode = pOutPacket->CreateElement((char*)"ErrCode");
            pOutPacket->SetElementValue(AccNode, (char*)"");

            /* ֤����ֹ��Ч��*/
            AccNode = pOutPacket->CreateElement((char*)"EndTime");
            pOutPacket->SetElementValue(AccNode, (char*)"");

            /* �������� */
            AccNode = pOutPacket->CreateElement((char*)"Secrecy");
            pOutPacket->SetElementValue(AccNode, (char*)"");

            /* IP��ַ*/
            AccNode = pOutPacket->CreateElement((char*)"IPAddress");
            pOutPacket->SetElementValue(AccNode, (char*)"");

            /* �˿ں� */
            AccNode = pOutPacket->CreateElement((char*)"Port");
            pOutPacket->SetElementValue(AccNode, (char*)"");

            /* ����*/
            AccNode = pOutPacket->CreateElement((char*)"Password");
            pOutPacket->SetElementValue(AccNode, (char*)"");

            /* ��λ״̬ */
            AccNode = pOutPacket->CreateElement((char*)"Status");
            pOutPacket->SetElementValue(AccNode, (char*)"OFF");

            /* ���� */
            AccNode = pOutPacket->CreateElement((char*)"Longitude");
            pOutPacket->SetElementValue(AccNode, (char*)"");

            /* γ�� */
            AccNode = pOutPacket->CreateElement((char*)"Latitude");
            pOutPacket->SetElementValue(AccNode, (char*)"");

            if (!iThreePartyFlag) /* �ǵ�����ƽ̨ */
            {
                /* ������CMS ID */
                AccNode = pOutPacket->CreateElement((char*)"CMSID");
                pOutPacket->SetElementValue(AccNode, (char*)"");
            }

            /* ��չ��Info�ֶ� */
            pOutPacket->SetCurrentElement(ItemAccNode);
            ItemInfoNode = pOutPacket->CreateElement((char*)"Info");
            pOutPacket->SetCurrentElement(ItemInfoNode);

            /* �Ƿ�ɿ� */
            AccNode = pOutPacket->CreateElement((char*)"PTZType");
            pOutPacket->SetElementValue(AccNode, (char*)"");
        }
    }

    return 0;
}

/*****************************************************************************
 �� �� ��  : AddLogicDeviceGroupInfoToXMLItemForRoute
 ��������  : ����߼��豸������Ϣ��Catalog XML��
 �������  : CPacket* pOutPacket
             DOMElement* ListAccNode
             char* group_code
             char* group_name
             char* parent_code
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��9��6��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int AddLogicDeviceGroupInfoToXMLItemForRoute(CPacket* pOutPacket, DOMElement* ListAccNode, char* group_code, char* group_name, char* parent_code)
{
    DOMElement* ItemAccNode = NULL;
    DOMElement* AccNode = NULL;

    if (NULL == pOutPacket || NULL == ListAccNode || NULL == group_code || NULL == group_name)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "AddLogicDeviceGroupInfoToXMLItemForRoute() exit---: Param Error \r\n");
        return -1;
    }

    /* ��дXML����*/
    pOutPacket->SetCurrentElement(ListAccNode);
    ItemAccNode = pOutPacket->CreateElement((char*)"Item");
    pOutPacket->SetCurrentElement(ItemAccNode);

    /* �豸ͳһ��� */
    AccNode = pOutPacket->CreateElement((char*)"DeviceID");
    pOutPacket->SetElementValue(AccNode, group_code);

    /* ��λ���� */
    AccNode = pOutPacket->CreateElement((char*)"Name");
    pOutPacket->SetElementValue(AccNode, group_name);

    /* ���豸/����/ϵͳID, ������ƽ̨�Խӵ�ʱ��ͳһʹ�ñ���CMS ID */
    if (NULL != parent_code && parent_code[0] != '\0')
    {
        AccNode = pOutPacket->CreateElement((char*)"ParentID");
        pOutPacket->SetElementValue(AccNode, parent_code);
    }

    return 0;
}

/*****************************************************************************
 �� �� ��  : CreateGBLogicDeviceCatalogResponseXMLHead
 ��������  : �����ϼ�CMS�����Ļ�ȡ�߼��豸�б��Ӧ��ϢXMLͷ��
 �������  : CPacket** pOutPacket
             int query_count
             int record_count
             char* strSN
             char* strDeviceID
             DOMElement** ListAccNode
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��7��27�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int CreateGBLogicDeviceCatalogResponseXMLHeadForRoute(CPacket** pOutPacket, int query_count, int record_count, char* strSN, char* strDeviceID, DOMElement** ListAccNode)
{
    DOMElement* AccNode = NULL;

    char strSumNum[32] = {0};
    char strRecordCount[32] = {0};

    snprintf(strSumNum, 32, "%d", record_count);

    /* ��ӷ��͵�xmlͷ�� */
    if (query_count == 1)
    {
        *pOutPacket = new CPacket();

        if (NULL == *pOutPacket)
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "CreateGBLogicDeviceCatalogResponseXMLHeadForRoute() exit---: Create XML Packet Error \r\n");
            return -1;
        }

        if (record_count <= MAX_ROUTE_CATALOG_COUT_SEND)
        {
            snprintf(strRecordCount, 32, "%d", record_count);
        }
        else
        {
            snprintf(strRecordCount, 32, "%d", MAX_ROUTE_CATALOG_COUT_SEND);
        }

        (*pOutPacket)->SetRootTag("Response");

        AccNode = (*pOutPacket)->CreateElement((char*)"CmdType");
        (*pOutPacket)->SetElementValue(AccNode, (char*)"Catalog");

        AccNode = (*pOutPacket)->CreateElement((char*)"SN");
        (*pOutPacket)->SetElementValue(AccNode, strSN);

        AccNode = (*pOutPacket)->CreateElement((char*)"DeviceID");
        (*pOutPacket)->SetElementValue(AccNode, strDeviceID);

        AccNode = (*pOutPacket)->CreateElement((char*)"SumNum");
        (*pOutPacket)->SetElementValue(AccNode, strSumNum);

        (*ListAccNode) = (*pOutPacket)->CreateElement((char*)"DeviceList");
        (*pOutPacket)->SetElementAttr((*ListAccNode), (char*)"Num", strRecordCount);
    }
    else if ((query_count % MAX_ROUTE_CATALOG_COUT_SEND == 1) && (record_count - query_count >= MAX_ROUTE_CATALOG_COUT_SEND))
    {
        *pOutPacket = new CPacket();

        if (NULL == *pOutPacket)
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "CreateGBLogicDeviceCatalogResponseXMLHeadForRoute() exit---: Create XML Packet Error \r\n");
            return -1;
        }

        snprintf(strRecordCount, 32, "%d", MAX_ROUTE_CATALOG_COUT_SEND);
        (*pOutPacket)->SetRootTag("Response");

        AccNode = (*pOutPacket)->CreateElement((char*)"CmdType");
        (*pOutPacket)->SetElementValue(AccNode, (char*)"Catalog");

        AccNode = (*pOutPacket)->CreateElement((char*)"SN");
        (*pOutPacket)->SetElementValue(AccNode, strSN);

        AccNode = (*pOutPacket)->CreateElement((char*)"DeviceID");
        (*pOutPacket)->SetElementValue(AccNode, strDeviceID);

        AccNode = (*pOutPacket)->CreateElement((char*)"SumNum");
        (*pOutPacket)->SetElementValue(AccNode, strSumNum);

        (*ListAccNode) = (*pOutPacket)->CreateElement((char*)"DeviceList");
        (*pOutPacket)->SetElementAttr((*ListAccNode), (char*)"Num", strRecordCount);
    }
    else if ((query_count % MAX_ROUTE_CATALOG_COUT_SEND == 1) && (record_count - query_count < MAX_ROUTE_CATALOG_COUT_SEND))
    {
        *pOutPacket = new CPacket();

        if (NULL == *pOutPacket)
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "CreateGBLogicDeviceCatalogResponseXMLHeadForRoute() exit---: Create XML Packet Error \r\n");
            return -1;
        }

        snprintf(strRecordCount, 32, "%d", record_count - query_count + 1);
        (*pOutPacket)->SetRootTag("Response");

        AccNode = (*pOutPacket)->CreateElement((char*)"CmdType");
        (*pOutPacket)->SetElementValue(AccNode, (char*)"Catalog");

        AccNode = (*pOutPacket)->CreateElement((char*)"SN");
        (*pOutPacket)->SetElementValue(AccNode, strSN);

        AccNode = (*pOutPacket)->CreateElement((char*)"DeviceID");
        (*pOutPacket)->SetElementValue(AccNode, strDeviceID);

        AccNode = (*pOutPacket)->CreateElement((char*)"SumNum");
        (*pOutPacket)->SetElementValue(AccNode, strSumNum);

        (*ListAccNode) = (*pOutPacket)->CreateElement((char*)"DeviceList");
        (*pOutPacket)->SetElementAttr((*ListAccNode), (char*)"Num", strRecordCount);
    }

    return 0;
}

/*****************************************************************************
 �� �� ��  : CreateGBLogicDeviceCatalogNotifyXMLHeadForRoute
 ��������  : �����߼��豸�б�֪ͨ��ϢXMLͷ��
 �������  : CPacket** pOutPacket
             int query_count
             int record_count
             char* strSN
             char* strDeviceID
             DOMElement** ListAccNode
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��7��27�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int CreateGBLogicDeviceCatalogNotifyXMLHeadForRoute(CPacket** pOutPacket, int query_count, int record_count, char* strSN, char* strDeviceID, DOMElement** ListAccNode)
{
    DOMElement* AccNode = NULL;

    char strSumNum[32] = {0};
    char strRecordCount[32] = {0};

    snprintf(strSumNum, 32, "%d", record_count);

    /* ��ӷ��͵�xmlͷ�� */
    if (query_count == 1)
    {
        *pOutPacket = new CPacket();

        if (NULL == *pOutPacket)
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "CreateGBLogicDeviceCatalogNotifyXMLHeadForRoute() exit---: Create XML Packet Error \r\n");
            return -1;
        }

        if (record_count <= MAX_ROUTE_CATALOG_COUT_SEND)
        {
            snprintf(strRecordCount, 32, "%d", record_count);
        }
        else
        {
            snprintf(strRecordCount, 32, "%d", MAX_ROUTE_CATALOG_COUT_SEND);
        }

        (*pOutPacket)->SetRootTag("Notify");

        AccNode = (*pOutPacket)->CreateElement((char*)"CmdType");
        (*pOutPacket)->SetElementValue(AccNode, (char*)"Catalog");

        AccNode = (*pOutPacket)->CreateElement((char*)"SN");
        (*pOutPacket)->SetElementValue(AccNode, strSN);

        AccNode = (*pOutPacket)->CreateElement((char*)"DeviceID");
        (*pOutPacket)->SetElementValue(AccNode, strDeviceID);

        AccNode = (*pOutPacket)->CreateElement((char*)"SumNum");
        (*pOutPacket)->SetElementValue(AccNode, strSumNum);

        (*ListAccNode) = (*pOutPacket)->CreateElement((char*)"DeviceList");
        (*pOutPacket)->SetElementAttr((*ListAccNode), (char*)"Num", strRecordCount);
    }
    else if ((query_count % MAX_ROUTE_CATALOG_COUT_SEND == 1) && (record_count - query_count >= MAX_ROUTE_CATALOG_COUT_SEND))
    {
        *pOutPacket = new CPacket();

        if (NULL == *pOutPacket)
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "CreateGBLogicDeviceCatalogNotifyXMLHeadForRoute() exit---: Create XML Packet Error \r\n");
            return -1;
        }

        snprintf(strRecordCount, 32, "%d", MAX_ROUTE_CATALOG_COUT_SEND);
        (*pOutPacket)->SetRootTag("Notify");

        AccNode = (*pOutPacket)->CreateElement((char*)"CmdType");
        (*pOutPacket)->SetElementValue(AccNode, (char*)"Catalog");

        AccNode = (*pOutPacket)->CreateElement((char*)"SN");
        (*pOutPacket)->SetElementValue(AccNode, strSN);

        AccNode = (*pOutPacket)->CreateElement((char*)"DeviceID");
        (*pOutPacket)->SetElementValue(AccNode, strDeviceID);

        AccNode = (*pOutPacket)->CreateElement((char*)"SumNum");
        (*pOutPacket)->SetElementValue(AccNode, strSumNum);

        (*ListAccNode) = (*pOutPacket)->CreateElement((char*)"DeviceList");
        (*pOutPacket)->SetElementAttr((*ListAccNode), (char*)"Num", strRecordCount);
    }
    else if ((query_count % MAX_ROUTE_CATALOG_COUT_SEND == 1) && (record_count - query_count < MAX_ROUTE_CATALOG_COUT_SEND))
    {
        *pOutPacket = new CPacket();

        if (NULL == *pOutPacket)
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "CreateGBLogicDeviceCatalogNotifyXMLHeadForRoute() exit---: Create XML Packet Error \r\n");
            return -1;
        }

        snprintf(strRecordCount, 32, "%d", record_count - query_count + 1);
        (*pOutPacket)->SetRootTag("Notify");

        AccNode = (*pOutPacket)->CreateElement((char*)"CmdType");
        (*pOutPacket)->SetElementValue(AccNode, (char*)"Catalog");

        AccNode = (*pOutPacket)->CreateElement((char*)"SN");
        (*pOutPacket)->SetElementValue(AccNode, strSN);

        AccNode = (*pOutPacket)->CreateElement((char*)"DeviceID");
        (*pOutPacket)->SetElementValue(AccNode, strDeviceID);

        AccNode = (*pOutPacket)->CreateElement((char*)"SumNum");
        (*pOutPacket)->SetElementValue(AccNode, strSumNum);

        (*ListAccNode) = (*pOutPacket)->CreateElement((char*)"DeviceList");
        (*pOutPacket)->SetElementAttr((*ListAccNode), (char*)"Num", strRecordCount);
    }

    return 0;
}

/*****************************************************************************
 �� �� ��  : CreateRecordInfoQueryResponseXMLHeadForRoute
 ��������  : ������ȡ¼���ļ���ѯ��ӦXMLͷ��
 �������  : CPacket** pOutPacket
             int query_count
             int record_count
             char* strSN
             char* strDeviceID
             char* strDeviceName
             DOMElement** ListAccNode
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��8��6�� ���ڶ�
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int CreateRecordInfoQueryResponseXMLHeadForRoute(CPacket** pOutPacket, int query_count, int record_count, char* strSN, char* strDeviceID, char* strDeviceName, DOMElement** ListAccNode)
{
    DOMElement* AccNode = NULL;

    char strSumNum[32] = {0};
    char strRecordCount[32] = {0};

    snprintf(strSumNum, 32, "%d", record_count);

    /* ��ӷ��͵�xmlͷ�� */
    if (query_count == 1)
    {
        *pOutPacket = new CPacket();

        if (NULL == *pOutPacket)
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "CreateRecordInfoQueryResponseXMLHeadForRoute() exit---: Create XML Packet Error \r\n");
            return -1;
        }

        if (record_count <= MAX_ROUTE_RECORD_INFO_COUT_SEND)
        {
            snprintf(strRecordCount, 32, "%d", record_count);
        }
        else
        {
            snprintf(strRecordCount, 32, "%d", MAX_ROUTE_RECORD_INFO_COUT_SEND);
        }

        (*pOutPacket)->SetRootTag("Response");

        AccNode = (*pOutPacket)->CreateElement((char*)"CmdType");
        (*pOutPacket)->SetElementValue(AccNode, (char*)"RecordInfo");

        AccNode = (*pOutPacket)->CreateElement((char*)"SN");

        if (NULL != strSN)
        {
            (*pOutPacket)->SetElementValue(AccNode, strSN);
        }
        else
        {
            (*pOutPacket)->SetElementValue(AccNode, (char*)"");
        }

        AccNode = (*pOutPacket)->CreateElement((char*)"DeviceID");

        if (NULL != strDeviceID)
        {
            (*pOutPacket)->SetElementValue(AccNode, strDeviceID);
        }
        else
        {
            (*pOutPacket)->SetElementValue(AccNode, (char*)"");
        }

        AccNode = (*pOutPacket)->CreateElement((char*)"Name");

        if (NULL != strDeviceName)
        {
            (*pOutPacket)->SetElementValue(AccNode, strDeviceName);
        }
        else
        {
            (*pOutPacket)->SetElementValue(AccNode, (char*)" ");
        }

        AccNode = (*pOutPacket)->CreateElement((char*)"SumNum");
        (*pOutPacket)->SetElementValue(AccNode, strSumNum);

        (*ListAccNode) = (*pOutPacket)->CreateElement((char*)"RecordList");
        (*pOutPacket)->SetElementAttr((*ListAccNode), (char*)"Num", strRecordCount);
    }
    else if ((query_count % MAX_ROUTE_RECORD_INFO_COUT_SEND == 1) && (record_count - query_count >= MAX_ROUTE_RECORD_INFO_COUT_SEND))
    {
        *pOutPacket = new CPacket();

        if (NULL == *pOutPacket)
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "CreateRecordInfoQueryResponseXMLHeadForRoute() exit---: Create XML Packet Error \r\n");
            return -1;
        }

        snprintf(strRecordCount, 32, "%d", MAX_ROUTE_RECORD_INFO_COUT_SEND);
        (*pOutPacket)->SetRootTag("Response");

        AccNode = (*pOutPacket)->CreateElement((char*)"CmdType");
        (*pOutPacket)->SetElementValue(AccNode, (char*)"RecordInfo");

        AccNode = (*pOutPacket)->CreateElement((char*)"SN");

        if (NULL != strSN)
        {
            (*pOutPacket)->SetElementValue(AccNode, strSN);
        }
        else
        {
            (*pOutPacket)->SetElementValue(AccNode, (char*)"");
        }

        AccNode = (*pOutPacket)->CreateElement((char*)"DeviceID");

        if (NULL != strDeviceID)
        {
            (*pOutPacket)->SetElementValue(AccNode, strDeviceID);
        }
        else
        {
            (*pOutPacket)->SetElementValue(AccNode, (char*)"");
        }

        AccNode = (*pOutPacket)->CreateElement((char*)"Name");

        if (NULL != strDeviceName)
        {
            (*pOutPacket)->SetElementValue(AccNode, strDeviceName);
        }
        else
        {
            (*pOutPacket)->SetElementValue(AccNode, (char*)" ");
        }

        AccNode = (*pOutPacket)->CreateElement((char*)"SumNum");
        (*pOutPacket)->SetElementValue(AccNode, strSumNum);

        (*ListAccNode) = (*pOutPacket)->CreateElement((char*)"RecordList");
        (*pOutPacket)->SetElementAttr((*ListAccNode), (char*)"Num", strRecordCount);

    }
    else if ((query_count % MAX_ROUTE_RECORD_INFO_COUT_SEND == 1) && (record_count - query_count < MAX_ROUTE_RECORD_INFO_COUT_SEND))
    {
        *pOutPacket = new CPacket();

        if (NULL == *pOutPacket)
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "CreateRecordInfoQueryResponseXMLHeadForRoute() exit---: Create XML Packet Error \r\n");
            return -1;
        }

        snprintf(strRecordCount, 32, "%d", record_count - query_count + 1);
        (*pOutPacket)->SetRootTag("Response");

        AccNode = (*pOutPacket)->CreateElement((char*)"CmdType");
        (*pOutPacket)->SetElementValue(AccNode, (char*)"RecordInfo");

        AccNode = (*pOutPacket)->CreateElement((char*)"SN");

        if (NULL != strSN)
        {
            (*pOutPacket)->SetElementValue(AccNode, strSN);
        }
        else
        {
            (*pOutPacket)->SetElementValue(AccNode, (char*)"");
        }

        AccNode = (*pOutPacket)->CreateElement((char*)"DeviceID");

        if (NULL != strDeviceID)
        {
            (*pOutPacket)->SetElementValue(AccNode, strDeviceID);
        }
        else
        {
            (*pOutPacket)->SetElementValue(AccNode, (char*)"");
        }

        AccNode = (*pOutPacket)->CreateElement((char*)"Name");

        if (NULL != strDeviceName)
        {
            (*pOutPacket)->SetElementValue(AccNode, strDeviceName);
        }
        else
        {
            (*pOutPacket)->SetElementValue(AccNode, (char*)" ");
        }

        AccNode = (*pOutPacket)->CreateElement((char*)"SumNum");
        (*pOutPacket)->SetElementValue(AccNode, strSumNum);

        (*ListAccNode) = (*pOutPacket)->CreateElement((char*)"RecordList");
        (*pOutPacket)->SetElementAttr((*ListAccNode), (char*)"Num", strRecordCount);

    }

    return 0;
}

/*****************************************************************************
 �� �� ��  : AddRecordInfoToXMLItemForRoute
 ��������  : ����·�����¼���ļ���Ϣ��XML��Item
 �������  : CPacket* pOutPacket
             DOMElement* ListAccNode
             VideoRecord& stVideoRecord
             char* strDeviceID
             char* strDeviceName
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��7��15�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int AddRecordInfoToXMLItemForRoute(CPacket* pOutPacket, DOMElement* ListAccNode, VideoRecord& stVideoRecord, char* strDeviceID, char* strDeviceName)
{
    DOMElement* ItemAccNode = NULL;
    DOMElement* AccNode = NULL;

    //char strStorageIndex[32] = {0};
    char strStartTime[32] = {0};
    char strEndTime[32] = {0};
    char str_date[12] = {0};
    char str_time[12] = {0};
    //char strSize[32] = {0};
    time_t sStartTime;
    time_t sStopTime;
    struct tm local_start_time = { 0 };
    struct tm local_stop_time = { 0 };

    if (NULL == pOutPacket || NULL == ListAccNode)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "AddRecordInfoToXMLItemForRoute() exit---: Param Error \r\n");
        return -1;
    }

    /* ��дXML����*/
    pOutPacket->SetCurrentElement(ListAccNode);
    ItemAccNode = pOutPacket->CreateElement((char*)"Item");
    pOutPacket->SetCurrentElement(ItemAccNode);

    AccNode = pOutPacket->CreateElement((char*)"DeviceID");

    if (NULL != strDeviceID)
    {
        pOutPacket->SetElementValue(AccNode, strDeviceID);
    }
    else
    {
        pOutPacket->SetElementValue(AccNode, (char*)"");
    }

    AccNode = pOutPacket->CreateElement((char*)"Name");

    if (NULL != strDeviceName)
    {
        pOutPacket->SetElementValue(AccNode, strDeviceName);
    }
    else
    {
        pOutPacket->SetElementValue(AccNode, (char*)" ");
    }

#if 0
    AccNode = pOutPacket->CreateElement((char*)"StorageIndex");
    snprintf(strStorageIndex, 32, "%d", stVideoRecord.StorageIndex);
    pOutPacket->SetElementValue(AccNode, strStorageIndex);
#endif

    AccNode = pOutPacket->CreateElement((char*)"StartTime");
    sStartTime = stVideoRecord.StartTime;
    localtime_r(&sStartTime, &local_start_time);
    memset(str_date, 0, 12);
    memset(str_time, 0, 12);
    memset(strStartTime, 0, 32);
    strftime(str_date, sizeof(str_date), "%Y-%m-%d", &local_start_time);
    strftime(str_time, sizeof(str_time), "%H:%M:%S", &local_start_time);
    snprintf(strStartTime, 32, "%sT%s", str_date, str_time);
    //DEBUG_TRACE(MODULE_USER, LOG_INFO,"AddDeviceGroupConfigToXMLItem() strStartTime=%s \r\n", strStartTime);
    pOutPacket->SetElementValue(AccNode, strStartTime);

    AccNode = pOutPacket->CreateElement((char*)"EndTime");
    sStopTime = stVideoRecord.StopTime;
    localtime_r(&sStopTime, &local_stop_time);
    memset(str_date, 0, 12);
    memset(str_time, 0, 12);
    memset(strEndTime, 0, 32);
    strftime(str_date, sizeof(str_date), "%Y-%m-%d", &local_stop_time);
    strftime(str_time, sizeof(str_time), "%H:%M:%S", &local_stop_time);
    snprintf(strEndTime, 32, "%sT%s", str_date, str_time);
    //DEBUG_TRACE(MODULE_USER, LOG_INFO,"AddDeviceGroupConfigToXMLItem() strEndTime=%s \r\n", strEndTime);
    pOutPacket->SetElementValue(AccNode, strEndTime);

#if 0
    AccNode = pOutPacket->CreateElement((char*)"Size");
    snprintf(strSize, 32, "%d", stVideoRecord.Size);
    pOutPacket->SetElementValue(AccNode, strSize);

    AccNode = pOutPacket->CreateElement((char*)"StorageIP");

    if (stVideoRecord.StorageIP.length() > 0)
    {
        pOutPacket->SetElementValue(AccNode, (char*)stVideoRecord.StorageIP.c_str());
    }
    else
    {
        pOutPacket->SetElementValue(AccNode, (char*)"");
    }

    AccNode = pOutPacket->CreateElement((char*)"FilePath");

    if (stVideoRecord.StoragePath.length() > 0)
    {
        pOutPacket->SetElementValue(AccNode, (char*)stVideoRecord.StoragePath.c_str());
    }
    else
    {
        pOutPacket->SetElementValue(AccNode, (char*)"");
    }

    AccNode = pOutPacket->CreateElement((char*)"Address");
    pOutPacket->SetElementValue(AccNode, (char*)"Address 1");
#endif

    AccNode = pOutPacket->CreateElement((char*)"Secrecy");
    pOutPacket->SetElementValue(AccNode, (char*)"0");

    AccNode = pOutPacket->CreateElement((char*)"Type");
    pOutPacket->SetElementValue(AccNode, (char*)"time");

    return 0;
}

/*****************************************************************************
 �� �� ��  : checkIfHasDBRefresh
 ��������  : ������ݿ��Ƿ����ڸ���
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2017��2��20��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int checkIfHasDBRefresh()
{
    if (db_GBLogicDeviceInfo_reload_mark
        || db_GBDeviceInfo_reload_mark
        || db_GroupInfo_reload_mark
        || db_GroupMapInfo_reload_mark)
    {
        return 1;
    }

    return 0;
}

/*****************************************************************************
 �� �� ��  : RouteLockDeviceProc
 ��������  : �ϼ�ƽ̨������������
 �������  : char* strLockCmd
             char* strDeviceID
             route_info_t* pRouteInfo
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2016��4��7��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int RouteLockDeviceProc(char* strLockCmd, char* strDeviceID, route_info_t* pRouteInfo)
{
    int i = 0;
    user_info_t* pOldUserInfo = NULL;
    GBLogicDevice_info_t* pGBLogicDeviceInfo = NULL;

    if (NULL == strLockCmd || NULL == strDeviceID || NULL == pRouteInfo)
    {
        return -1;
    }

    pGBLogicDeviceInfo = GBLogicDevice_info_find(strDeviceID);

    if (NULL != pGBLogicDeviceInfo)
    {
        if (0 == sstrcmp(strLockCmd, (char*)"Lock")) /* ���� */
        {
            if (LOCK_STATUS_OFF == pGBLogicDeviceInfo->lock_status) /* û������ */
            {
                pGBLogicDeviceInfo->lock_status = LOCK_STATUS_ROUTE_LOCK;
                pGBLogicDeviceInfo->pLockUserInfo = NULL;
                pGBLogicDeviceInfo->pLockRouteInfo = pRouteInfo;

                i = device_auto_unlock_use(pGBLogicDeviceInfo->id);

                if (i != 0)
                {
                    SystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�������豸���������, ������λʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=%s", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, (char*)"��ӵ��Զ���������ʧ��");
                    EnSystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "Device control command process from superior CMS, lock point failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=%s", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, (char*)"Adding to the list of auto unlock failed.");
                    DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "RouteLockDeviceProc() device_auto_unlock_use Error \r\n");
                }
                else
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS�������豸���������, ������λ�ɹ�:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Device control command process from superior CMS, lock point success:Superior CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID);
                    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "RouteLockDeviceProc() device_auto_unlock_use OK \r\n");
                }
            }
            else if (LOCK_STATUS_USER_LOCK == pGBLogicDeviceInfo->lock_status) /* �Ѿ����û����� */
            {
                if (NULL != pGBLogicDeviceInfo->pLockUserInfo) /* ԭ���û��������� */
                {
                    /* �ϼ�ƽֱ̨����ռ�����û�����Ȩ�� */
                    pOldUserInfo = pGBLogicDeviceInfo->pLockUserInfo;

                    pGBLogicDeviceInfo->lock_status = LOCK_STATUS_ROUTE_LOCK;
                    pGBLogicDeviceInfo->pLockUserInfo = NULL;
                    pGBLogicDeviceInfo->pLockRouteInfo = pRouteInfo;

                    i = device_auto_unlock_use(pGBLogicDeviceInfo->id);

                    if (i != 0)
                    {
                        SystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�������豸���������, ������λʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=%s", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, (char*)"��ӵ��Զ���������ʧ��");
                        EnSystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "Device control command process from superior CMS, lock point failed:Requester ID=%s, IP address=%s, port number=%d, Logic Device ID=%s, reason=%s", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, (char*)"Adding to the list of auto unlock failed.");
                        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "RouteLockDeviceProc() device_auto_unlock_use Error \r\n");
                    }
                    else
                    {
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "�ϼ�CMS�������豸���������, ��ռ����ԭ��������λ:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ�������û�ID=%s, ԭ�������û�IP��ַ=%s, ԭ�������û��˿ں�=%d", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, pOldUserInfo->user_id, pOldUserInfo->login_ip, pOldUserInfo->login_port);
                        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Device control command process from superior CMS,Preemption lock  original locking point: original superior CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s, original lock user ID = % s, original lock users IP address = % s, locking client old slogan = % d", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, pOldUserInfo->user_id, pOldUserInfo->login_ip, pOldUserInfo->login_port);
                        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "RouteLockDeviceProc() device_auto_unlock_use OK \r\n");
                    }
                }
                else /* ԭ���û�������Ч */
                {
                    pGBLogicDeviceInfo->lock_status = LOCK_STATUS_ROUTE_LOCK;
                    pGBLogicDeviceInfo->pLockUserInfo = NULL;
                    pGBLogicDeviceInfo->pLockRouteInfo = pRouteInfo;

                    i = device_auto_unlock_use(pGBLogicDeviceInfo->id);

                    if (i != 0)
                    {
                        SystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�������豸���������, ������λʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=%s", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, (char*)"��ӵ��Զ���������ʧ��");
                        EnSystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "Device control command process from superior CMS, lock point failed:Requester ID=%s, IP address=%s, port number=%d, reason=%s", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, (char*)"Adding to the list of auto unlock failed.");
                        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "RouteLockDeviceProc() device_auto_unlock_use Error \r\n");
                    }
                    else
                    {
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "�ϼ�CMS�������豸���������, ԭ�������û�ʧЧ, ������λ�ɹ�:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID);
                        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Device control command process from superior CMS, original lock users failure ,Locking point success: the higher the CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID);
                        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "RouteLockDeviceProc() device_auto_unlock_use OK \r\n");
                    }
                }
            }
            else if (LOCK_STATUS_ROUTE_LOCK == pGBLogicDeviceInfo->lock_status) /* �Ѿ����ϼ����� */
            {
                if (NULL != pGBLogicDeviceInfo->pLockRouteInfo) /* ԭ���ϼ��������� */
                {
                    /* �ж��ϼ��Ƿ���Ч */
                    if (0 == pGBLogicDeviceInfo->pLockRouteInfo->reg_status) /* ԭ���ϼ�������Ч */
                    {
                        /* ����Ϊ�µ��ϼ����� */
                        pGBLogicDeviceInfo->lock_status = LOCK_STATUS_ROUTE_LOCK;
                        pGBLogicDeviceInfo->pLockUserInfo = NULL;
                        pGBLogicDeviceInfo->pLockRouteInfo = pRouteInfo;

                        i = device_auto_unlock_use(pGBLogicDeviceInfo->id);

                        if (i != 0)
                        {
                            SystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�������豸���������, ������λʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=%s", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, (char*)"��ӵ��Զ���������ʧ��");
                            EnSystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "Device control command process from superior CMS, lock point failed:Requester ID=%s, IP address=%s, port number=%d, Logic Device ID=%s, reason=%s", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, (char*)"Adding to the list of auto unlock failed.");
                            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "RouteLockDeviceProc() device_auto_unlock_use Error \r\n");
                        }
                        else
                        {
                            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "�ϼ�CMS�������豸���������, ԭ���ϼ�������ϢʧЧ������Ϊ�û�������λ�ɹ�:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID);
                            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Device control command process from superior CMS, Original superior lock information failure, change the lock for the user level success: the superior CMS, ID = % s = % s IP address, port number = % d, logical device ID=%s", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID);
                            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "RouteLockDeviceProc() device_auto_unlock_use OK \r\n");
                        }
                    }
                    else /* ��Ч */
                    {
                        SystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�������豸���������, ������λʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=%s, �������ϼ�CMS ID=%s, �������ϼ�CMS IP��ַ=%s, �������ϼ�CMS �˿ں�=%d", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, (char*)"��λ�Ѿ����ϼ�ƽ̨����", pGBLogicDeviceInfo->pLockRouteInfo->server_id, pGBLogicDeviceInfo->pLockRouteInfo->server_ip, pGBLogicDeviceInfo->pLockRouteInfo->server_port);
                        EnSystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "Device control command process from superior CMS, lock point failed:Requester ID=%s, IP address=%s, port number=%d, Logic Device ID=%s, reason=%s", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, (char*)"Device Has locked by route CMS");
                        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "RouteLockDeviceProc() device_auto_unlock_use Error \r\n");
                    }
                }
                else /* ԭ���ϼ�������Ч */
                {
                    /* ����Ϊ�ϼ����� */
                    pGBLogicDeviceInfo->lock_status = LOCK_STATUS_ROUTE_LOCK;
                    pGBLogicDeviceInfo->pLockUserInfo = NULL;
                    pGBLogicDeviceInfo->pLockRouteInfo = pRouteInfo;

                    i = device_auto_unlock_use(pGBLogicDeviceInfo->id);

                    if (i != 0)
                    {
                        SystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�������豸���������, ������λʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=%s", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, (char*)"��ӵ��Զ���������ʧ��");
                        EnSystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "Device control command process from superior CMS, lock point failed:Requester ID=%s, IP address=%s, port number=%d, Logic Device ID=%s, reason=%s", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, (char*)"Adding to the list of auto unlock failed.");
                        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "RouteLockDeviceProc() device_auto_unlock_use Error \r\n");
                    }
                    else
                    {
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "�ϼ�CMS�������豸���������, ԭ���ϼ�������ϢʧЧ������������λ�ɹ�:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID);
                        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, " Device control command process from superior CMS, the original superior locking information failure, Locking point success again: locking point superior CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s ", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID);
                        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "RouteLockDeviceProc() device_auto_unlock_use OK \r\n");
                    }
                }
            }
        }
        else if (0 == sstrcmp(strLockCmd, (char*)"UnLock")) /* ������� */
        {
            if (LOCK_STATUS_USER_LOCK == pGBLogicDeviceInfo->lock_status) /* �Ѿ����û����� */
            {
                if (NULL != pGBLogicDeviceInfo->pLockUserInfo) /* �ж��û������Ƿ���Ч */
                {
                    /* �ϼ�ƽֱ̨�ӽ��������û�����Ȩ�� */
                    pOldUserInfo = pGBLogicDeviceInfo->pLockUserInfo;

                    pGBLogicDeviceInfo->lock_status = LOCK_STATUS_OFF;
                    pGBLogicDeviceInfo->pLockUserInfo = NULL;
                    pGBLogicDeviceInfo->pLockRouteInfo = NULL;

                    i = device_auto_unlock_remove(pGBLogicDeviceInfo->id);

                    if (i != 0)
                    {
                        SystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�������豸���������, ���������λʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=%s", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, (char*)"���Զ����������Ƴ�ʧ��");
                        EnSystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "Device control command process from superior CMS, unlock point failed:Requester ID=%s, IP address=%s, port number=%d, Logic Device ID=%s, reason=%s", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, (char*)"Remove from the list of auto unlock failed.");
                        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "RouteLockDeviceProc() device_auto_unlock_remove Error \r\n");
                    }
                    else
                    {
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "�ϼ�CMS�������豸���������, ���ԭ���û������ĵ�λ�ɹ�:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ���������û�ID=%s, �������û�IP��ַ=%s, �������û��˿ں�=%d", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, pOldUserInfo->user_id, pOldUserInfo->login_ip, pOldUserInfo->login_port);
                        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Device control command process from superior CMS, Remove the original user locking point success: the superior CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s, the original lock user ID = % s, lock the user IP address = % s, locking the client number ", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, pOldUserInfo->user_id, pOldUserInfo->login_ip, pOldUserInfo->login_port);
                        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "RouteLockDeviceProc() device_auto_unlock_remove OK \r\n");
                    }
                }
                else /* ԭ���û�������Ч */
                {
                    pGBLogicDeviceInfo->lock_status = LOCK_STATUS_OFF;
                    pGBLogicDeviceInfo->pLockUserInfo = NULL;
                    pGBLogicDeviceInfo->pLockRouteInfo = NULL;

                    i = device_auto_unlock_remove(pGBLogicDeviceInfo->id);

                    if (i != 0)
                    {
                        SystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�������豸���������, ���������λʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=%s", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, (char*)"���Զ����������Ƴ�ʧ��");
                        EnSystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "Device control command process from superior CMS, unlock point failed:Requester ID=%s, IP address=%s, port number=%d, Logic Device ID=%s, reason=%s", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, (char*)"Remove from the list of auto unlock failed.");
                        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "RouteLockDeviceProc() device_auto_unlock_remove Error \r\n");
                    }
                    else
                    {
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "�ϼ�CMS�������豸���������, ԭ�������û��Ѿ���Ч, ���������λ�ɹ�:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID);
                        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Device control command process from superior CMS, Remove the original user locking point success: the superior CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s, the original lock user ID = % s, lock the user IP address = % s, locking the client number ", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, pOldUserInfo->user_id, pOldUserInfo->login_ip, pOldUserInfo->login_port);
                        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "RouteLockDeviceProc() device_auto_unlock_remove OK \r\n");
                    }
                }
            }
            else if (LOCK_STATUS_ROUTE_LOCK == pGBLogicDeviceInfo->lock_status)
            {
                if (NULL != pGBLogicDeviceInfo->pLockRouteInfo) /* �Ѿ����ϼ����� */
                {
                    /* �ж��Ƿ��������Ƿ���Ǹ��ϼ�Route */
                    if (pGBLogicDeviceInfo->pLockRouteInfo == pRouteInfo)
                    {
                        pGBLogicDeviceInfo->lock_status = LOCK_STATUS_OFF;
                        pGBLogicDeviceInfo->pLockUserInfo = NULL;
                        pGBLogicDeviceInfo->pLockRouteInfo = NULL;

                        i = device_auto_unlock_remove(pGBLogicDeviceInfo->id);

                        if (i != 0)
                        {
                            SystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�������豸���������, ���������λʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=%s", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, (char*)"���Զ����������Ƴ�ʧ��");
                            EnSystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "Device control command process from superior CMS, unlock point failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=%s", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, (char*)"Remove from the list of auto unlock failed.");
                            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "RouteLockDeviceProc() device_auto_unlock_remove Error \r\n");
                        }
                        else
                        {
                            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�CMS�������豸���������, ���������λ�ɹ�:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID);
                            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Device control command process from superior CMS, Remove the original user locking point success: the superior CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s, the original lock user ID = % s, lock the user IP address = % s, locking the client number ", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, pOldUserInfo->user_id, pOldUserInfo->login_ip, pOldUserInfo->login_port);
                            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "RouteLockDeviceProc() device_auto_unlock_remove OK \r\n");
                        }
                    }
                    else /* �����ϼ����� */
                    {
                        /* �ж�ԭ���ϼ��Ƿ���Ч */
                        if (0 == pGBLogicDeviceInfo->pLockRouteInfo->reg_status) /* ԭ���ϼ�������Ч */
                        {
                            pGBLogicDeviceInfo->lock_status = LOCK_STATUS_OFF;
                            pGBLogicDeviceInfo->pLockUserInfo = NULL;
                            pGBLogicDeviceInfo->pLockRouteInfo = NULL;

                            i = device_auto_unlock_remove(pGBLogicDeviceInfo->id);

                            if (i != 0)
                            {
                                SystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�������豸���������, ���������λʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=%s", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, (char*)"���Զ����������Ƴ�ʧ��");
                                EnSystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "Device control command process from superior CMS, unlock point failed:Requester ID=%s, IP address=%s, port number=%d, Logic Device ID=%s, reason=%s", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, (char*)"Remove from the list of auto unlock failed.");
                                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "RouteLockDeviceProc() device_auto_unlock_remove Error \r\n");
                            }
                            else
                            {
                                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "�ϼ�CMS�������豸���������, ԭ�������ϼ�ƽ̨�Ѿ���Ч, ���������λ�ɹ�:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID);
                                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Device control command process from superior CMS, Locking the superior platform is invalid, unlocked original point success: the higher the CMS, ID = % s = % s IP address, port number = % d, logical device ID = %s", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID);
                                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "RouteLockDeviceProc() device_auto_unlock_remove OK \r\n");
                            }
                        }
                        else /* ��Ч */
                        {
                            SystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�������豸���������, ���������λʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=%s, �������ϼ�CMS ID=%s, �������ϼ�CMS IP��ַ=%s, �������ϼ�CMS �˿ں�=%d", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, (char*)"��λ�Ѿ��������ϼ�CMS����", pGBLogicDeviceInfo->pLockRouteInfo->server_id, pGBLogicDeviceInfo->pLockRouteInfo->server_ip, pGBLogicDeviceInfo->pLockRouteInfo->server_port);
                            EnSystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "Device control command process from superior CMS, lock point failed:Requester ID=%s, IP address=%s, port number=%d, Logic Device ID=%s, reason=%s", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, (char*)"Device Has locked by high CMS");
                            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "RouteLockDeviceProc() device_auto_unlock_use Error \r\n");
                        }
                    }
                }
                else /* ԭ���ϼ�������Ч */
                {
                    pGBLogicDeviceInfo->lock_status = LOCK_STATUS_OFF;
                    pGBLogicDeviceInfo->pLockUserInfo = NULL;
                    pGBLogicDeviceInfo->pLockRouteInfo = NULL;

                    i = device_auto_unlock_remove(pGBLogicDeviceInfo->id);

                    if (i != 0)
                    {
                        SystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�������豸���������, ���������λʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=%s", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, (char*)"���Զ����������Ƴ�ʧ��");
                        EnSystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "Device control command process from superior CMS, unlock point failed:Requester ID=%s, IP address=%s, port number=%d, Logic Device ID=%s, reason=%s", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, (char*)"Remove from the list of auto unlock failed.");
                        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "RouteLockDeviceProc() device_auto_unlock_remove Error \r\n");
                    }
                    else
                    {
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "�ϼ�CMS�������豸���������, ԭ�������û��Ѿ���Ч, ���������λ�ɹ�:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID);
                        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Device control command process from superior CMS, Unlocked point success: the higher the CMS, ID = % s = % s IP address, port number = % d, logical device ID = %s", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID);
                        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "RouteLockDeviceProc() device_auto_unlock_remove OK \r\n");
                    }
                }
            }
            else if (LOCK_STATUS_OFF == pGBLogicDeviceInfo->lock_status) /* û������ */
            {

            }
        }
        else
        {
            SystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�������豸���������, ������λʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��=��֧�ֵ���������:LockCmd=%s", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, strLockCmd);
            EnSystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "Device control command process from superior CMS, Lock point failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=unsupportable lock command:LockCmd=%s", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, strLockCmd);
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "RouteLockDeviceProc() Lock Cmd Error: LockCmd=%s \r\n", strLockCmd);
            return -1;
        }
    }
    else
    {
        SystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "�ϼ�CMS�������豸���������, ������λʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸ID=%s, ԭ��:DeviceID=%s", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID);
        EnSystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "Device control command process from superior CMS, Lock point failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=find logical device failed:DeviceID=%s", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, strDeviceID);
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "RouteLockDeviceProc() exit---: Find GB LogicDevice Info Error: DeviceID=%s \r\n", strDeviceID);
        return -1;
    }

    return 0;
}

/*****************************************************************************
 �� �� ��  : StopRouteService
 ��������  : ֹͣ����·������
 �������  : route_info_t* pRouteInfo
             char * device_id
             int stream_type
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2016��10��8��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int StopRouteService(route_info_t* pRouteInfo, char * device_id, int stream_type)
{
    int i = 0;
    int cr_pos = -1;
    int other_cr_pos = -1;
    cr_t* pOtherCrData = NULL;
    cr_t* pCrData = NULL;

    if ((NULL == pRouteInfo) || (NULL == device_id))
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "StopRouteService() exit---: Param Error \r\n");
        return -1;
    }

    /* 1������DEC �˵�Dialog Index ���Һ��м�¼��Ϣ */
    cr_pos = call_record_find_by_callerinfo_and_calleeid(pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, device_id, stream_type);
    //DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "StopDecService() call_record_find_by_callerid_and_calleeid:cr_pos=%d \r\n", cr_pos);

    if (cr_pos < 0)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_WARN, "StopRouteService() exit---: Not Find Call Record Info:server_id=%s, server_ip=%s, server_port=%d, device_id=%s, stream_type=%d, cr_pos=%d \r\n", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, device_id, stream_type, cr_pos);
        return -2;
    }

    pCrData = call_record_get(cr_pos);

    if (NULL == pCrData)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "StopRouteService() exit---: Get Call Record Error \r\n");
        return -1;
    }

    /* 3��֪ͨTSUֹͣ��������*/
    if ((CALL_TYPE_RECORD_PLAY == pCrData->call_type)
        || (CALL_TYPE_DOWNLOAD == pCrData->call_type))
    {
        i = notify_tsu_delete_replay_task(pCrData->tsu_ip, pCrData->task_id);

        if (i < 0)
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_WARN, "StopRouteService() notify_tsu_delete_replay_task Error:tsu_ip=%s, task_id=%s, i=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "StopRouteService() notify_tsu_delete_replay_task OK:tsu_ip=%s, task_id=%s, i=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
        }
    }
    else
    {
        i = notify_tsu_delete_transfer_task(pCrData->tsu_ip, pCrData->task_id);

        if (i < 0)
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_WARN, "StopRouteService() notify_tsu_delete_transfer_task Error:tsu_ip=%s, task_id=%s, i=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "StopRouteService() notify_tsu_delete_transfer_task OK:tsu_ip=%s, task_id=%s, i=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
        }
    }

    /* 4������Bye �����в� */
    i = SIP_SendBye(pCrData->caller_ua_index);
    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "StopRouteService() SIP_SendBye To Caller:caller_ua_index=%d, i=%d \r\n", pCrData->caller_ua_index, i);

    /* ���Ƿ���ǰ������ */
    if (pCrData->callee_ua_index >= 0)
    {
        /* �鿴�Ƿ��������ͻ���ҵ�� */
        other_cr_pos = is_GBLogic_device_has_other_service(pCrData->callee_id, pCrData->callee_stream_type, cr_pos);
        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "StopRouteService() is_GBLogic_device_has_other_service:other_cr_pos=%d \r\n", other_cr_pos);

        if (other_cr_pos < 0) /* û������ҵ�� */
        {
            /*����Bye �����в� */
            i = SIP_SendBye(pCrData->callee_ua_index);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "StopRouteService() SIP_SendBye To Callee:callee_ua_index=%d, i=%d \r\n", pCrData->callee_ua_index, i);
        }
        else
        {
            pOtherCrData = call_record_get(other_cr_pos);

            if (NULL != pOtherCrData)
            {
                pOtherCrData->callee_ua_index = pCrData->callee_ua_index; /* ��ǰ�˵ĻỰ����������¸�ҵ�� */
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "StopRouteService() callee_ua_index=%d copy from %d to %d \r\n", pOtherCrData->callee_ua_index, cr_pos, other_cr_pos);
            }
        }
    }

    /* 5���Ƴ����м�¼��Ϣ */
    i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
    i = call_record_remove(cr_pos);

    if (0 != i)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "StopRouteService() call_record_remove Error:cr_pos=%d, i=%d \r\n", cr_pos, i);
    }
    else
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "StopRouteService() call_record_remove OK:cr_pos=%d, i=%d \r\n", cr_pos, i);
    }

    return cr_pos;
}

#if DECS("ͨ��WebServer�ӿڻ�ȡѹ��������")
/*****************************************************************************
 �� �� ��  : get_compress_task_from_webservice_proc
 ��������  : ͨ��WebServer�ӿڻ�ȡѹ������
 �������  : char* platform_ip
             int iTaskBeginTime
             int iTaskEndTime
             DBOper* ptDBoper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2017��6��11��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int get_compress_task_from_webservice_proc(char* platform_ip, int iTaskBeginTime, int iTaskEndTime, DBOper* ptDBoper)
{
    int iRet = 0;

    if (NULL == platform_ip || platform_ip[0] == '\0')
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR,  "get_compress_task_from_webservice_proc() exit---: platform_ip Error \r\n");
        return -1;
    }

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "���ϼ�ƽ̨��ȡѹ��������Ϣ,ƽ̨IP=%s: ��ʼ---", platform_ip);

    /* 3��ͨ��httpЭ���ȡVMS�����real device ��Ϣ */
    iRet = get_compress_task_by_http(platform_ip, iTaskBeginTime, iTaskEndTime, ptDBoper);

    if (0 != iRet)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "���ϼ�ƽ̨��ȡѹ��������Ϣ,ʧ��");
        return iRet;
    }

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "���ϼ�ƽ̨��ȡѹ��������Ϣ: ����---");
    return 0;
}

/*****************************************************************************
 �� �� ��  : get_compress_task_by_http
 ��������  : ��ȡѹ������
 �������  : char* platform_ip
             int iTaskBeginTime
             int iTaskEndTime
             DBOper* ptDBoper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2017��6��11��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int get_compress_task_by_http(char* platform_ip, int iTaskBeginTime, int iTaskEndTime, DBOper* ptDBoper)
{
    int iRet = 0;
    int task_count = 0;
    string strXMLBuffer = "";
    CPacket inPacket;

    DOMElement* ItemAccNode = NULL;
    char strReturnFiles[1024] = {0};
    jly_yspb_t stYSPB;/* ��Ӧ�����ݿ�ṹ�ֶ� */

    char strTaskBeginTime[64] = {0};
    char strTaskEndTime[64] = {0};

    if (NULL == platform_ip || platform_ip[0] == '\0')
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR,  "get_compress_task_by_http() exit---: platform_ip Error \r\n");
        return -1;
    }

    iRet = format_time(iTaskBeginTime, strTaskBeginTime);
    iRet = format_time(iTaskEndTime, strTaskEndTime);

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_IMPORTANT, "ͨ��WebService�ӿڻ�ȡѹ��������Ϣ��ʼ:platform_ip=%s, ���λ�ȡ����Ŀ�ʼʱ��=%s, ����ʱ��=%s", platform_ip, strTaskBeginTime, strTaskEndTime);

    strXMLBuffer.clear();
    iRet = interface_queryObjectInfo(iTaskBeginTime, iTaskEndTime, strXMLBuffer);
    DEBUG_TRACE(MODULE_COMMON, LOG_INFO,  "get_compress_task_by_http: interface_queryObjectInfo Response XML msg=\r\n%s \r\n", (char*)strXMLBuffer.c_str());

    if (iRet != 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "ͨ��WebService�ӿڻ�ȡѹ��������Ϣʧ��,ԭ��:����WebService��ѯ�ӿ�ʧ��,platform_ip=%s", platform_ip);
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR,  "get_compress_task_by_http() exit---: interface_queryObjectInfo Error\r\n");
        return iRet;
    }

    //����XML
    iRet = inPacket.BuiltTree((char*)strXMLBuffer.c_str(), strXMLBuffer.length());//����DOM���ṹ.

    if (iRet < 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "ͨ��WebService�ӿڻ�ȡѹ��������Ϣʧ��,ԭ��:����Http��Ӧ���Ķ�Ӧ��XML��Ϣ��ʧ��,platform_ip=%s, XMLBuffer=%s", platform_ip, (char*)strXMLBuffer.c_str());
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR,  "get_compress_task_by_http() exit---: XML Build Tree Error msg=\r\n%s \r\n", (char*)strXMLBuffer.c_str());
        return FILE_COMPRESS_GET_XML_ERROR;
    }

    /* ��ȡ���е�Item ���� */
    ItemAccNode = inPacket.SearchElement((char*)"rows");

    if (!ItemAccNode)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "ͨ��WebService�ӿڻ�ȡѹ��������Ϣʧ��,ԭ��:����Http��Ӧ���Ķ�Ӧ��XML��Ϣ���е�rowsʧ��,platform_ip=%s, XMLBuffer=%s", platform_ip, (char*)strXMLBuffer.c_str());
        return FILE_COMPRESS_GET_XML_ERROR;
    }

    /* ѭ����ȡ���ݣ������µ����� */
    inPacket.SetCurrentElement(ItemAccNode);

    while (ItemAccNode)
    {
        memset(strReturnFiles, 0, 1024);
        inPacket.GetElementValue((char*)"rows", strReturnFiles);

        /* ������XML�ֶ� */
        memset(&stYSPB, 0, sizeof(jly_yspb_t));
        analysis_return_fileds(strReturnFiles, &stYSPB);

        task_count++;
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_IMPORTANT, "ͨ��WebService�ӿڻ�ȡѹ��������Ϣ, ������Ϣ:������=%d, ��¼���=%s, �ļ�����=%s, ��׺����=%s, �ļ���С=%d, �ϴ���λ=%s, �ϴ�ʱ��=%d, �洢·��=%s", task_count, stYSPB.jlbh, stYSPB.wjmc, stYSPB.kzm, stYSPB.wjdx, stYSPB.scdw, stYSPB.scsj, stYSPB.cclj);

        /* ���ѹ������ */
        iRet = AddCompressTask(platform_ip, &stYSPB, ptDBoper);

        if (0 != iRet)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR,  "get_compress_task_by_http() exit---: AddCompressTask Error:platform_ip=%s, iRet=%d \r\n", platform_ip, iRet);
            continue;
        }

        ItemAccNode = inPacket.SearchNextElement(true);
    }

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_IMPORTANT, "ͨ��WebService�ӿڻ�ȡѹ��������Ϣ����:platform_ip=%s, ���λ�ȡ����Ŀ�ʼʱ��=%s, ����ʱ��=%s, ���λ�ȡ����������=%d", platform_ip, strTaskBeginTime, strTaskEndTime, task_count);

    return 0;
}

/*****************************************************************************
 �� �� ��  : analysis_return_fileds
 ��������  : ���������ֶ�
 �������  : char* strTime
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��8��6�� ���ڶ�
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int analysis_return_fileds(char* strReturnFiles, jly_yspb_t* pstYSPB)
{
    char* tmp = NULL;
    char strFirstTmp[512] = {0};                     /* ǰ����ַ��� */
    char strLastTmp[512] = {0};                      /* ʣ�µ��ַ��� */
    char strLast[512] = {0};                         /* ʣ�µ��ַ��� */

    char jlbh[64 + 4];                               /*1����¼���(jlbh)���ַ��� */
    char wjmc[128 + 4];                              /*2���ļ�����(wjmc)���ַ���*/
    char kzm[32 + 4];                                /*3���ļ���׺����(kzm)���ַ���*/
    int wjdx;                                        /*4���ļ���С(wjdx)������*/
    char scdw[128 + 4];                              /*5���ϴ���λ(scdw)���ַ���*/
    char strscsj[128 + 4];                           /*6���ϴ�ʱ���ַ���(scsj)����ʽ��20170527120000*/
    int scsj;                                        /*6���ϴ�ʱ��(scsj)����ʽ��20170527120000*/
    char cclj[128 + 4];                              /*7���洢·��(cclj)���ַ���*/

    if (NULL == strReturnFiles || NULL == pstYSPB)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "analysis_return_fileds() exit---: Param Error \r\n");
        return -1;
    }

    /* 1��������¼��� */
    tmp = strchr(strReturnFiles, '$');

    if (tmp != NULL)
    {
        memset(strFirstTmp, 0, 512);
        memset(strLastTmp, 0, 512);
        osip_strncpy(strFirstTmp, strReturnFiles, tmp - strReturnFiles);
        osip_strncpy(strLastTmp, tmp + 3, strReturnFiles + strlen(strReturnFiles) - tmp - 3);

        memset(jlbh, 0, 64 + 4);
        osip_strncpy(jlbh, strFirstTmp, 64);
        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "analysis_return_fileds() strFirstTmp=%s, jlbh=%s, strLastTmp=%s \r\n", strFirstTmp, jlbh, strLastTmp);
    }
    else
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "analysis_return_fileds() exit---: Analysis jlbh Error \r\n");
        return -1;
    }

    /* 2�������ļ����� */
    osip_strncpy(strLast, strLastTmp, strlen(strLastTmp));
    tmp = strchr(strLast, '$');

    if (tmp != NULL)
    {
        memset(strFirstTmp, 0, 512);
        memset(strLastTmp, 0, 512);
        osip_strncpy(strFirstTmp, strLast, tmp - strLast);
        osip_strncpy(strLastTmp, tmp + 3, strLast + strlen(strLast) - tmp - 3);

        memset(wjmc, 0, 128 + 4);
        osip_strncpy(wjmc, strFirstTmp, 128);
        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "analysis_return_fileds() strFirstTmp=%s, wjmc=%s, strLastTmp=%s \r\n", strFirstTmp, wjmc, strLastTmp);
    }
    else
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "analysis_return_fileds() exit---: Analysis wjmc Error \r\n");
        return -1;
    }

    /* 3�������ļ���׺�� */
    osip_strncpy(strLast, strLastTmp, strlen(strLastTmp));
    tmp = strchr(strLast, '$');

    if (tmp != NULL)
    {
        memset(strFirstTmp, 0, 512);
        memset(strLastTmp, 0, 512);
        osip_strncpy(strFirstTmp, strLast, tmp - strLast);
        osip_strncpy(strLastTmp, tmp + 3, strLast + strlen(strLast) - tmp - 3);

        memset(kzm, 0, 32 + 4);
        osip_strncpy(kzm, strFirstTmp, 32);
        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "analysis_return_fileds() strFirstTmp=%s, kzm=%s, strLastTmp=%s \r\n", strFirstTmp, kzm, strLastTmp);
    }
    else
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "analysis_return_fileds() exit---: Analysis kzm Error \r\n");
        return -1;
    }

    /* 4�������ļ���С */
    osip_strncpy(strLast, strLastTmp, strlen(strLastTmp));
    tmp = strchr(strLast, '$');

    if (tmp != NULL)
    {
        memset(strFirstTmp, 0, 512);
        memset(strLastTmp, 0, 512);
        osip_strncpy(strFirstTmp, strLast, tmp - strLast);
        osip_strncpy(strLastTmp, tmp + 3, strLast + strlen(strLast) - tmp - 3);

        wjdx = osip_atoi(strFirstTmp);
        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "analysis_return_fileds() strFirstTmp=%s, wjdx=%d, strLastTmp=%s \r\n", strFirstTmp, wjdx, strLastTmp);
    }
    else
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "analysis_return_fileds() exit---: Analysis kzm Error \r\n");
        return -1;
    }

    /* 5�������ϴ���λ */
    osip_strncpy(strLast, strLastTmp, strlen(strLastTmp));
    tmp = strchr(strLast, '$');

    if (tmp != NULL)
    {
        memset(strFirstTmp, 0, 512);
        memset(strLastTmp, 0, 512);
        osip_strncpy(strFirstTmp, strLast, tmp - strLast);
        osip_strncpy(strLastTmp, tmp + 3, strLast + strlen(strLast) - tmp - 3);

        memset(scdw, 0, 128 + 4);
        osip_strncpy(scdw, strFirstTmp, 128);
        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "analysis_return_fileds() strFirstTmp=%s, scdw=%s, strLastTmp=%s \r\n", strFirstTmp, scdw, strLastTmp);
    }
    else
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "analysis_return_fileds() exit---: Analysis scdw Error \r\n");
        return -1;
    }

    /* 6�������ϴ�ʱ��ʹ洢·�� */
    osip_strncpy(strLast, strLastTmp, strlen(strLastTmp));
    tmp = strchr(strLast, '$');

    if (tmp != NULL)
    {
        memset(strFirstTmp, 0, 512);
        memset(strLastTmp, 0, 512);
        osip_strncpy(strFirstTmp, strLast, tmp - strLast);
        osip_strncpy(strLastTmp, tmp + 3, strLast + strlen(strLast) - tmp - 3);

        memset(strscsj, 0, 128 + 4);
        memset(cclj, 0, 128 + 4);
        osip_strncpy(strscsj, strFirstTmp, 128);
        osip_strncpy(cclj, strLastTmp, 128);
        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "analysis_return_fileds() strFirstTmp=%s, strscsj=%s, cclj=%s, strLastTmp=%s \r\n", strFirstTmp, strscsj, cclj, strLastTmp);
    }
    else
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "analysis_return_fileds() exit---: Analysis scdw Error \r\n");
        return -1;
    }

    /* ����ʱ�� */
    scsj = analysis_time_for_compress(strscsj);

    osip_strncpy(pstYSPB->jlbh, jlbh, 64);
    osip_strncpy(pstYSPB->wjmc, wjmc, 128);
    osip_strncpy(pstYSPB->kzm, kzm, 32);
    pstYSPB->wjdx = wjdx;
    osip_strncpy(pstYSPB->scdw, scdw, 128);
    pstYSPB->scsj = scsj;
    osip_strncpy(pstYSPB->cclj, cclj, 128);

    return 0;
}

/*****************************************************************************
 �� �� ��  : analysis_time_for_compress
 ��������  : ʱ�����
 �������  : char* strTime
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��8��6�� ���ڶ�
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int analysis_time_for_compress(char* strTime)
{
    char pcYear[5] = {0};
    char pcMonth[3] = {0};
    char pcDay[3] = {0};
    char pcHour[3] = {0};
    char pcMinute[3] = {0};
    char pcSecond[3] = {0};

    struct tm stm;
    time_t timep;

    if (NULL == strTime)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "analysis_time_for_compress() exit---: Param Error \r\n");
        return 0;
    }

    /* ��ʽ��20170527120000 */
    osip_strncpy(pcYear, strTime, 4);
    osip_strncpy(pcMonth, &strTime[4], 2);
    osip_strncpy(pcDay, &strTime[6], 2);
    osip_strncpy(pcHour, &strTime[8], 2);
    osip_strncpy(pcMinute, &strTime[10], 2);
    osip_strncpy(pcSecond, &strTime[12], 2);

    /* ת����time_t */
    stm.tm_year = osip_atoi(pcYear) - 1900;
    stm.tm_mon = osip_atoi(pcMonth) - 1;
    stm.tm_mday = osip_atoi(pcDay);

    stm.tm_hour = osip_atoi(pcHour);
    stm.tm_min = osip_atoi(pcMinute);
    stm.tm_sec = osip_atoi(pcSecond);

    timep = mktime(&stm);

    return timep;
}

#endif

