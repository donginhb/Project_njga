
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
#include "common/gblconfig_proc.inc"
#include "common/db_proc.h"

#include "resource/resource_info_mgn.inc"

#include "record/record_srv_proc.inc"
#include "service/call_func_proc.inc"
#include "service/alarm_proc.inc"
#include "service/compress_task_proc.inc"

#include "device/device_srv_proc.inc"
#include "device/device_reg_proc.inc"

#include "user/user_srv_proc.inc"

/*----------------------------------------------*
 * �ⲿ����˵��                                 *
 *----------------------------------------------*/
extern int cms_run_status;  /* 0:û������,1:�������� */
extern BOARD_NET_ATTR  g_BoardNetConfig;
extern gbl_conf_t* pGblconf;              /* ȫ��������Ϣ */
extern int g_AlarmMsgSendToUserFlag;      /* ������Ϣ�Ƿ��͸��û�,Ĭ�Ϸ��� */
extern int g_AlarmMsgSendToRouteFlag;     /* ������Ϣ�Ƿ��͸��ϼ�·�ɣ�Ĭ�ϲ����� */
extern char g_StrConLog[2][100];

/*----------------------------------------------*
 * �ⲿ����ԭ��˵��                             *
 *----------------------------------------------*/

/*----------------------------------------------*
 * �ڲ�����ԭ��˵��                             *
 *----------------------------------------------*/
int g_iCurrentAudioTSUIndex = 0; /* ��ǰ��Ƶת����TSU���� */

/*----------------------------------------------*
 * ȫ�ֱ���                                     *
 *----------------------------------------------*/
unsigned int uTSUAlarmMsgSn = 0;
unsigned int g_TSUUpLoadFileFlag = 0;      /* TSU�Ƿ���Ҫ�����ϱ�¼���ļ���ʶ */

unsigned long long iTSUInfoLockCount = 0;
unsigned long long iTSUInfoUnLockCount = 0;

unsigned long long iTSURecordInfoLockCount = 0;
unsigned long long iTSURecordInfoUnLockCount = 0;

/*----------------------------------------------*
 * ģ�鼶����                                   *
 *----------------------------------------------*/
tsu_register_queue g_TSURegMsgQueue; /* TSU ע����Ϣ���� */
tsu_task_attribute_queue g_TSUTaskMsgQueue; /* TSU ������Ϣ���� */
tsu_audio_task_attribute_queue g_TSUAudioTaskMsgQueue; /* TSU��Ƶ������Ϣ���� */
tsu_no_stream_msg_queue g_TSUNoStreamMsgQueue; /* TSU ֪ͨû��������Ϣ���� */
tsu_creat_task_result_msg_queue  g_TSUCreatTaskResultMsgQueue; /* TSU ֪ͨ������������Ϣ���� */
tsu_alarm_msg_queue  g_TSUAlarmMsgQueue; /* TSU �澯��Ϣ���� */

TSU_Resource_Info_MAP g_TSUResourceInfoMap; /* TSU ��Դ��Ϣ���� */
#ifdef MULTI_THR
osip_mutex_t* g_TSUResourceInfoMapLock = NULL;
#endif


/*----------------------------------------------*
 * ��������                                     *
 *----------------------------------------------*/

/*----------------------------------------------*
 * �궨��                                       *
 *----------------------------------------------*/
#if DECS("TSU ע����Ϣ����")
/*****************************************************************************
 �� �� ��  : tsu_reg_msg_init
 ��������  : TSU ע����Ϣ�ṹ��ʼ��
 �������  : tsu_register_t ** tsu_reg_msg
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��11��13��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int tsu_reg_msg_init(tsu_register_t** tsu_reg_msg)
{
    *tsu_reg_msg = (tsu_register_t*)osip_malloc(sizeof(tsu_register_t));

    if (*tsu_reg_msg == NULL)
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_DEBUG, "tsu_reg_msg_init() exit---: *tsu_reg_msg Smalloc Error \r\n");
        return -1;
    }

    (*tsu_reg_msg)->iMsgType = 0;
    (*tsu_reg_msg)->pcTsuID[0] = '\0';
    (*tsu_reg_msg)->pcTsuVideoIP[0] = '\0';
    (*tsu_reg_msg)->pcTsuDeviceIP[0] = '\0';
    (*tsu_reg_msg)->iExpires = -1;
    (*tsu_reg_msg)->iRefresh = -1;
    (*tsu_reg_msg)->iTsuType = -1;

    return 0;
}

/*****************************************************************************
 �� �� ��  : tsu_reg_msg_free
 ��������  : TSU ע����Ϣ�ṹ�ͷ�
 �������  : tsu_register_t * tsu_reg_msg
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��11��13��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void tsu_reg_msg_free(tsu_register_t* tsu_reg_msg)
{
    if (tsu_reg_msg == NULL)
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_DEBUG, "tsu_reg_msg_free() exit---: Param Error \r\n");
        return;
    }

    tsu_reg_msg->iMsgType = 0;
    memset(tsu_reg_msg->pcTsuID, 0, MAX_ID_LEN + 4);
    tsu_reg_msg->iSlotID = -1;
    memset(tsu_reg_msg->pcTsuVideoIP, 0, 16);
    tsu_reg_msg->iVideoIPEth = -1;
    memset(tsu_reg_msg->pcTsuDeviceIP, 0, 16);
    tsu_reg_msg->iDeviceIPEth = -1;

    tsu_reg_msg->iExpires = -1;
    tsu_reg_msg->iRefresh = -1;
    tsu_reg_msg->iTsuType = -1;

    return;
}

/*****************************************************************************
 �� �� ��  : tsu_reg_msg_list_init
 ��������  : TSU ע����Ϣ���г�ʼ��
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��11��13��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int tsu_reg_msg_list_init()
{
    g_TSURegMsgQueue.clear();

    return 0;
}

/*****************************************************************************
 �� �� ��  : tsu_reg_msg_list_free
 ��������  : TSU ע����Ϣ�����ͷ�
 �������  : tsu_reg_msg_list_t * tsu_reg_msg_list
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��11��13��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void tsu_reg_msg_list_free()
{
    tsu_register_t* pTsuRegMsg = NULL;

    while (!g_TSURegMsgQueue.empty())
    {
        pTsuRegMsg = (tsu_register_t*) g_TSURegMsgQueue.front();
        g_TSURegMsgQueue.pop_front();

        if (NULL != pTsuRegMsg)
        {
            tsu_reg_msg_free(pTsuRegMsg);
            osip_free(pTsuRegMsg);
            pTsuRegMsg = NULL;
        }
    }

    g_TSURegMsgQueue.clear();

    return;
}

/*****************************************************************************
 �� �� ��  : tsu_reg_msg_add
 ��������  : ���TSU ע����Ϣ��������
 �������  : int iMsgType
             char* pcTsuID
             int iSlotID
             char* pcTsuVideoIP
             int iVideoIPEth
             char* pcTsuDeviceIP
             int iDeviceIPEth
             int iExpires
             int iRefresh
             int iTsuType
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��11��13��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int tsu_reg_msg_add(int iMsgType, char* pcTsuID, int iSlotID, char* pcTsuVideoIP, int iVideoIPEth, char* pcTsuDeviceIP, int iDeviceIPEth, int iExpires, int iRefresh, int iTsuType)
{
    tsu_register_t* pTsuRegMsg = NULL;
    int iRet = 0;

    if (NULL == pcTsuID)
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_DEBUG, "tsu_reg_msg_add() exit---: Param Error \r\n");
        return -1;
    }

    iRet = tsu_reg_msg_init(&pTsuRegMsg);

    if (iRet != 0)
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_ERROR, "tsu_reg_msg_add() exit---: Message Init Error \r\n");
        return -1;
    }

    pTsuRegMsg->iMsgType = iMsgType;

    if (NULL != pcTsuID)
    {
        osip_strncpy(pTsuRegMsg->pcTsuID, pcTsuID, MAX_ID_LEN);
    }

    pTsuRegMsg->iSlotID = iSlotID;

    if (NULL != pcTsuVideoIP)
    {
        osip_strncpy(pTsuRegMsg->pcTsuVideoIP, pcTsuVideoIP, 15);
    }

    pTsuRegMsg->iVideoIPEth = iVideoIPEth;

    if (NULL != pcTsuDeviceIP)
    {
        osip_strncpy(pTsuRegMsg->pcTsuDeviceIP, pcTsuDeviceIP, 15);
    }

    pTsuRegMsg->iDeviceIPEth = iDeviceIPEth;

    pTsuRegMsg->iExpires = iExpires;
    pTsuRegMsg->iRefresh = iRefresh;
    pTsuRegMsg->iTsuType = iTsuType;

    g_TSURegMsgQueue.push_back(pTsuRegMsg);

    return 0;
}

/*****************************************************************************
 �� �� ��  : scan_tsu_reg_msg_list
 ��������  : ɨ��TSU ע����Ϣ����
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��11��13 �� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void scan_tsu_reg_msg_list(DBOper* pTsu_Reg_dboper)
{
    int iRet = 0;
    tsu_register_t* pTsuRegMsg = NULL;

    while (!g_TSURegMsgQueue.empty())
    {
        pTsuRegMsg = (tsu_register_t*) g_TSURegMsgQueue.front();
        g_TSURegMsgQueue.pop_front();

        if (NULL != pTsuRegMsg)
        {
            break;
        }
    }

    if (NULL != pTsuRegMsg)
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_INFO,  "scan_tsu_reg_msg_list() \
        \r\n iMsgType=%d \
        \r\n sTsuID=%s \
        \r\n iSlotID=%d \
        \r\n sTsuVideoIP=%s \
        \r\n iVideoIPEth=%d \
        \r\n sTsuDeviceIP=%s \
        \r\n iDeviceIPEth=%d \
        \r\n iExpires=%d \
        \r\n iRefresh=%d \
        \r\n iTsuType=%d \
        \r\n ", pTsuRegMsg->iMsgType, pTsuRegMsg->pcTsuID, pTsuRegMsg->iSlotID, pTsuRegMsg->pcTsuVideoIP, pTsuRegMsg->iVideoIPEth, pTsuRegMsg->pcTsuDeviceIP, pTsuRegMsg->iDeviceIPEth, pTsuRegMsg->iExpires, pTsuRegMsg->iRefresh, pTsuRegMsg->iTsuType);

        iRet = tsu_reg_msg_proc(pTsuRegMsg->iMsgType, pTsuRegMsg->pcTsuID, pTsuRegMsg->iSlotID, pTsuRegMsg->pcTsuVideoIP, pTsuRegMsg->iVideoIPEth, pTsuRegMsg->pcTsuDeviceIP, pTsuRegMsg->iDeviceIPEth, pTsuRegMsg->iExpires, pTsuRegMsg->iRefresh, pTsuRegMsg->iTsuType, pTsu_Reg_dboper);
        tsu_reg_msg_free(pTsuRegMsg);
        osip_free(pTsuRegMsg);
        pTsuRegMsg = NULL;
    }

    return;
}

/*****************************************************************************
 �� �� ��  : tsu_reg_msg_proc
 ��������  : TSUע����Ϣ����
 �������  : int iMsgType
             char* pcTsuID
             int iSlotID
             char* pcTsuVideoIP
             int iVideoIPEth
             char* pcTsuDeviceIP
             int iDeviceIPEth
             int iExpires
             int iRefresh
             int iTsuType
             DBOper* pTsu_Reg_dboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��11��13��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int tsu_reg_msg_proc(int iMsgType, char* pcTsuID, int iSlotID, char* pcTsuVideoIP, int iVideoIPEth, char* pcTsuDeviceIP, int iDeviceIPEth, int iExpires, int iRefresh, int iTsuType, DBOper* pTsu_Reg_dboper)
{
    int iRet = 0;

    if (1 == iMsgType)
    {
        iRet = tsu_audio_reg_msg_proc(pcTsuID, iExpires, iRefresh);
    }
    else
    {
        iRet = tsu_video_reg_msg_proc(pcTsuID, iSlotID, pcTsuVideoIP, iVideoIPEth, pcTsuDeviceIP, iDeviceIPEth, iExpires, iRefresh, iTsuType, pTsu_Reg_dboper);
    }

    return 0;
}

/*****************************************************************************
 �� �� ��  : tsu_video_reg_msg_proc
 ��������  : TSU��Ƶҵ��ע����Ϣ����
 �������  : char* pcTsuID
             int iSlotID
             char* pcTsuVideoIP
             int iVideoIPEth
             char* pcTsuDeviceIP
             int iDeviceIPEth
             int iExpires
             int iRefresh
             int iTsuType
             DBOper* pTsu_Reg_dboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��11��13��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int tsu_video_reg_msg_proc(char* pcTsuID, int iSlotID, char* pcTsuVideoIP, int iVideoIPEth, char* pcTsuDeviceIP, int iDeviceIPEth, int iExpires, int iRefresh, int iTsuType, DBOper* pTsu_Reg_dboper)
{
    int i = 0;
    int pos = -1;
    int iRet = 0;
    int iEthNum = 0;
    char strEthName[MAX_IP_LEN] = {0};
    int iBoardIndex = 0;
    char* tmp = NULL;
    ip_pair_t* pIPaddr = NULL;
    tsu_resource_info_t* pTsuResourceInfo = NULL;
    char strDeviceType[16] = {0};
    int iAssignRecord = 0;

    printf("tsu_reg_msg_proc() Enter---: TsuID=%s \r\n", pcTsuID);

    if (NULL == pcTsuVideoIP || NULL == pcTsuID || NULL == pTsu_Reg_dboper)
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_DEBUG, "tsu_reg_msg_proc() exit---:  Param Error \r\n");
        SystemLog(EV9000_CMS_TSU_REGISTER_ERROR, EV9000_LOG_LEVEL_ERROR, "TSUע��ʧ��:TSU ID=%s, SlotID=%d, IP��ַ=%s, ԭ��=%s", pcTsuID, iSlotID, pcTsuVideoIP, (char*)"��������");
        EnSystemLog(EV9000_CMS_TSU_REGISTER_ERROR, EV9000_LOG_LEVEL_ERROR, "TSU registration failed: TSU ID=%s SlotID=%d,IP address =%s, cause=%s", pcTsuID, iSlotID, pcTsuVideoIP, (char*)"Parameter error");

        return -1;
    }

    if (IsLocalHost(pcTsuVideoIP))
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_DEBUG, "tsu_reg_msg_proc() exit---:  TsuVideoIP=%s Is Local IP \r\n", pcTsuVideoIP);
        SystemLog(EV9000_CMS_TSU_REGISTER_ERROR, EV9000_LOG_LEVEL_ERROR, "TSUע��ʧ��:TSU ID=%s, SlotID=%d, IP��ַ=%s, ԭ��=%s", pcTsuID, iSlotID, pcTsuVideoIP, (char*)"TSU IP��ַ�ͱ���IP��ַһ��");
        EnSystemLog(EV9000_CMS_TSU_REGISTER_ERROR, EV9000_LOG_LEVEL_ERROR, "TSU registration failed: TSU ID=%s SlotID=%d,IP address =%s, cause=%s", pcTsuID, iSlotID, pcTsuVideoIP, (char*)"TSU IP address is same with local IP address");

        return -1;
    }

    /* ��ȡ�������ñ�BoardConfig ������*/
    if (0 == sstrcmp(pcTsuVideoIP, (char*)"127.0.0.1")
        || 0 == sstrcmp(pcTsuVideoIP, (char*)"0.0.0.0")) /* ����TSUע�ᣬֱ��ʹ��CMS��IP��ַ */
    {
        iBoardIndex = SetBoardConfigTable(pcTsuID, 0, LOGIC_BOARD_TSU, pTsu_Reg_dboper, &iAssignRecord);

        if (iBoardIndex > 0)
        {
            for (i = 0; i < osip_list_size(pGblconf->pLocalIPAddrList); i++)
            {
                pIPaddr = (ip_pair_t*)osip_list_get(pGblconf->pLocalIPAddrList, i);

                if (NULL == pIPaddr || pIPaddr->local_ip[0] == '\0')
                {
                    continue;
                }

                if (0 == strncmp(pIPaddr->eth_name, (char*)"eth", 3))
                {
                    tmp = &pIPaddr->eth_name[3];
                    iEthNum = osip_atoi(tmp);

                    iRet = SetBoardNetConfigTable(iBoardIndex, iEthNum, pIPaddr->local_ip, 1, pTsu_Reg_dboper);
                }
                else if (0 == strncmp(pIPaddr->eth_name, (char*)"bond", 4)) /* �����ڵ�֧�� */
                {
                    tmp = &pIPaddr->eth_name[4];
                    iEthNum = osip_atoi(tmp);

                    iRet = SetBoardNetConfigTable(iBoardIndex, iEthNum, pIPaddr->local_ip, 1, pTsu_Reg_dboper);
                }
                else if (0 == strncmp(pIPaddr->eth_name, (char*)"wlan", 4)) /* �������ڵ�֧�� */
                {
                    tmp = &pIPaddr->eth_name[4];
                    iEthNum = osip_atoi(tmp);

                    iRet = SetBoardNetConfigTable(iBoardIndex, iEthNum, pIPaddr->local_ip, 1, pTsu_Reg_dboper);
                }
            }
        }
        else
        {
            SystemLog(EV9000_CMS_TSU_REGISTER_ERROR, EV9000_LOG_LEVEL_ERROR, "TSUע��ʧ��:TSU ID=%s, SlotID=%d, IP��ַ=%s, ԭ��=%s", pcTsuID, iSlotID, pcTsuVideoIP, (char*)"��ȡTSU���ݿ�����ʧ��");
            EnSystemLog(EV9000_CMS_TSU_REGISTER_ERROR, EV9000_LOG_LEVEL_ERROR, "TSU registration failed: TSU ID=%s SlotID=%d,IP address =%s, cause=%s", pcTsuID, iSlotID, pcTsuVideoIP, (char*)"Access TSU database index failed");

            DEBUG_TRACE(MODULE_RESOURCE, LOG_ERROR, "tsu_reg_msg_proc() exit---: Tsu Index Error \r\n");
            return -1;
        }
    }
    else
    {
        iBoardIndex = SetBoardConfigTable(pcTsuID, iSlotID, LOGIC_BOARD_TSU, pTsu_Reg_dboper, &iAssignRecord);

        if (iBoardIndex > 0)
        {
            /* ������Ƶ��IP��ַ */
            iRet = SetBoardNetConfigTable(iBoardIndex, iVideoIPEth, pcTsuVideoIP, 1, pTsu_Reg_dboper);
            DEBUG_TRACE(MODULE_RESOURCE, LOG_TRACE, "tsu_reg_msg_proc() TsuID=%s, DeviceIPEth=%d, TsuVideoIP=%s, UsedFlag=%d \r\n", pcTsuID, iVideoIPEth, pcTsuVideoIP, g_BoardNetConfig.tCmsUsingVideoIP.UsedFlag);

            /* �����豸��IP��ַ */
            if (NULL == pcTsuDeviceIP || pcTsuDeviceIP[0] == '\0')
            {
                iRet = SetBoardNetConfigTable(iBoardIndex, iDeviceIPEth, NULL, 0, pTsu_Reg_dboper);
                DEBUG_TRACE(MODULE_RESOURCE, LOG_TRACE, "tsu_reg_msg_proc() TsuID=%s, DeviceIPEth=%d, TsuDeviceIP=NULL, UsedFlag=%d \r\n", pcTsuID, iDeviceIPEth, g_BoardNetConfig.tCmsUsingDeviceIP.UsedFlag);
            }
            else
            {
                iRet = SetBoardNetConfigTable(iBoardIndex, iDeviceIPEth, pcTsuDeviceIP, g_BoardNetConfig.tCmsUsingDeviceIP.UsedFlag, pTsu_Reg_dboper);
                DEBUG_TRACE(MODULE_RESOURCE, LOG_TRACE, "tsu_reg_msg_proc() TsuID=%s, DeviceIPEth=%d, TsuDeviceIP=%s, UsedFlag=%d \r\n", pcTsuID, iDeviceIPEth, pcTsuDeviceIP, g_BoardNetConfig.tCmsUsingDeviceIP.UsedFlag);
            }
        }
        else
        {
            SystemLog(EV9000_CMS_TSU_REGISTER_ERROR, EV9000_LOG_LEVEL_ERROR, "TSUע��ʧ��:TSU ID=%s, SlotID=%d, IP��ַ=%s, ԭ��=%s", pcTsuID, iSlotID, pcTsuVideoIP, (char*)"��ȡTSU���ݿ�����ʧ��");
            EnSystemLog(EV9000_CMS_TSU_REGISTER_ERROR, EV9000_LOG_LEVEL_ERROR, "TSU registration failed: TSU ID=%s SlotID=%d,IP address =%s, cause=%s", pcTsuID, iSlotID, pcTsuVideoIP, (char*)"Access TSU database index failed");

            DEBUG_TRACE(MODULE_RESOURCE, LOG_ERROR, "tsu_reg_msg_proc() exit---: Tsu Index Error \r\n");
            return -1;
        }
    }

    if (0 != iRet)
    {
        SystemLog(EV9000_CMS_TSU_REGISTER_ERROR, EV9000_LOG_LEVEL_ERROR, "TSUע��ʧ��:TSU ID=%s, SlotID=%d, IP��ַ=%s, ԭ��=%s", pcTsuID, iSlotID, pcTsuVideoIP, (char*)"����TSU���ݿ�������Ϣʧ��");
        EnSystemLog(EV9000_CMS_TSU_REGISTER_ERROR, EV9000_LOG_LEVEL_ERROR, "TSU registration failed: TSU ID=%s SlotID=%d,IP address =%s, cause=%s", pcTsuID, iSlotID, pcTsuVideoIP, (char*)"set TSU database network info failed");

        DEBUG_TRACE(MODULE_RESOURCE, LOG_ERROR, "tsu_reg_msg_proc() exit---: Set Tsu Net Info Error \r\n");
        return -1;
    }

    /* ���µ�������״̬ */
    iRet = UpdateBoardConfigTableStatus(pcTsuID, LOGIC_BOARD_TSU, 1, pTsu_Reg_dboper);

    /* �����ڴ��е�TSU */
    pos = tsu_resource_info_find(pcTsuID);

    DEBUG_TRACE(MODULE_RESOURCE, LOG_TRACE, "tsu_reg_msg_proc() tsu_resource_info_find:TsuID=%s, pos=%d \r\n", pcTsuID, pos);
    printf("tsu_reg_msg_proc() tsu_resource_info_find:TsuID=%s, pos=%d \r\n", pcTsuID, pos);

    if (pos >= 0) /* �Ѿ����� */
    {
        pTsuResourceInfo = tsu_resource_info_get(pos);

        if (NULL == pTsuResourceInfo)
        {
            SystemLog(EV9000_CMS_TSU_REGISTER_ERROR, EV9000_LOG_LEVEL_ERROR, "TSUע��ʧ��:TSU ID=%s, SlotID=%d, IP��ַ=%s, ԭ��=%s", pcTsuID, iSlotID, pcTsuVideoIP, (char*)"��ȡTSU��Ϣʧ��");
            EnSystemLog(EV9000_CMS_TSU_REGISTER_ERROR, EV9000_LOG_LEVEL_ERROR, "TSU registration failed: TSU ID=%s SlotID=%d,IP address =%s, cause=%s", pcTsuID, iSlotID, pcTsuVideoIP, (char*)"Access TSU info failed");

            DEBUG_TRACE(MODULE_RESOURCE, LOG_ERROR, "tsu_reg_msg_proc() exit---: Get TSU Resource Info Error \r\n");
            return -1;
        }

        /* ���TSU�Ӳ�ָ��¼����ָ��¼����Ҫֹͣԭ��TSU��������¼������ */
        if (pTsuResourceInfo->iTsuType == 0 && iAssignRecord)
        {
            iRet = StopRecordServiceTaskByTSUID(pcTsuID);

            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "TSU���ͷ����仯, �Ӳ�ָ��¼�����ͱ仯Ϊָ��¼������:TSU ID=%s, ��Ƶ��IP��ַ=%s, �豸��IP��ַ=%s, ����=%d", pTsuResourceInfo->tsu_device_id, pcTsuVideoIP, pcTsuDeviceIP, pTsuResourceInfo->iTsuIndex);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "TSU type changed, change from unspecified video type to specified video type:TSU ID=%s, video network IP address=%s, device network IP address=%s, index=%d", pTsuResourceInfo->tsu_device_id, pcTsuVideoIP, pcTsuDeviceIP, pTsuResourceInfo->iTsuIndex);
        }

        pTsuResourceInfo->iTsuType = iAssignRecord;

        if (0 == sstrcmp(pcTsuVideoIP, (char*)"127.0.0.1")
            || 0 == sstrcmp(pcTsuVideoIP, (char*)"0.0.0.0"))
        {
            if (NULL != pTsuResourceInfo->pTSUIPAddrList && osip_list_size(pTsuResourceInfo->pTSUIPAddrList) <= 0)
            {
                i = osip_list_clone(pGblconf->pLocalIPAddrList, pTsuResourceInfo->pTSUIPAddrList, (int (*)(void*, void**))&ip_pair_clone);

                if (i != 0)
                {
                    SystemLog(EV9000_CMS_TSU_REGISTER_ERROR, EV9000_LOG_LEVEL_ERROR, "TSUע��ʧ��:TSU ID=%s, SlotID=%d, IP��ַ=%s, ԭ��=%s", pcTsuID, iSlotID, pcTsuVideoIP, (char*)"����IP��ַ��Ϣʧ��");
                    EnSystemLog(EV9000_CMS_TSU_REGISTER_ERROR, EV9000_LOG_LEVEL_ERROR, "TSU registration failed: TSU ID=%s SlotID=%d,IP address =%s, cause=%s", pcTsuID, iSlotID, pcTsuVideoIP, (char*)"Copy IP address info failed");
                    DEBUG_TRACE(MODULE_RESOURCE, LOG_ERROR, "tsu_reg_msg_proc() exit---: TSU IP Addr List Clone Error \r\n");
                    return -1;
                }
            }
        }
        else
        {
            /* �鿴�ϱ���IP��ַ�Ƿ��б仯 */
            if (NULL != pTsuResourceInfo->pTSUIPAddrList)
            {
                if (osip_list_size(pTsuResourceInfo->pTSUIPAddrList) > 0)
                {
                    for (i = 0; i < osip_list_size(pTsuResourceInfo->pTSUIPAddrList); i++)
                    {
                        pIPaddr = (ip_pair_t*)osip_list_get(pTsuResourceInfo->pTSUIPAddrList, i);

                        if (NULL == pIPaddr || pIPaddr->local_ip[0] == '\0')
                        {
                            continue;
                        }

                        if (0 == strncmp(pIPaddr->eth_name, (char*)"eth", 3))
                        {
                            tmp = &pIPaddr->eth_name[3];
                            iEthNum = osip_atoi(tmp);
                        }
                        else if (0 == strncmp(pIPaddr->eth_name, (char*)"bond", 4)) /* �����ڵ�֧�� */
                        {
                            tmp = &pIPaddr->eth_name[4];
                            iEthNum = osip_atoi(tmp);
                        }
                        else if (0 == strncmp(pIPaddr->eth_name, (char*)"wlan", 4)) /* �������ڵ�֧�� */
                        {
                            tmp = &pIPaddr->eth_name[4];
                            iEthNum = osip_atoi(tmp);
                        }
                        else if (0 == strncmp(pIPaddr->eth_name, (char*)"mgmt", 4)) /* �������ڵ�֧�� */
                        {
                            tmp = &pIPaddr->eth_name[4];
                            iEthNum = osip_atoi(tmp);
                        }
                        else
                        {
                            continue;
                        }

                        if (IP_ADDR_VIDEO == pIPaddr->ip_type)
                        {
                            if (0 != strcmp(pIPaddr->local_ip, pcTsuVideoIP))
                            {
                                memset(pIPaddr->local_ip, 0, MAX_IP_LEN);
                                osip_strncpy(pIPaddr->local_ip, pcTsuVideoIP, MAX_IP_LEN);
                            }

                            if (iEthNum != iVideoIPEth)
                            {
                                memset(strEthName, 0, MAX_IP_LEN);
                                snprintf(strEthName, MAX_IP_LEN, "eth%d", iVideoIPEth);
                                osip_strncpy(pIPaddr->eth_name, strEthName, MAX_IP_LEN);
                            }
                        }
                        else if (IP_ADDR_DEVICE == pIPaddr->ip_type)
                        {
                            if (0 != strcmp(pIPaddr->local_ip, pcTsuDeviceIP))
                            {
                                memset(pIPaddr->local_ip, 0, MAX_IP_LEN);
                                osip_strncpy(pIPaddr->local_ip, pcTsuDeviceIP, MAX_IP_LEN);
                            }

                            if (iEthNum != iDeviceIPEth)
                            {
                                memset(strEthName, 0, MAX_IP_LEN);
                                snprintf(strEthName, MAX_IP_LEN, "eth%d", iDeviceIPEth);
                                osip_strncpy(pIPaddr->eth_name, strEthName, MAX_IP_LEN);
                            }
                        }
                    }

                    /* ���豸���Ƿ���ڣ�����ǰһ��û�������������� */
                    if (osip_list_size(pTsuResourceInfo->pTSUIPAddrList) == 1)
                    {
                        if (NULL != pcTsuDeviceIP)
                        {
                            memset(strEthName, 0, MAX_IP_LEN);
                            snprintf(strEthName, MAX_IP_LEN, "eth%d", iDeviceIPEth);
                            i = tsu_ip_pair_add(pTsuResourceInfo, strEthName, IP_ADDR_DEVICE, pcTsuDeviceIP, 5060);
                            DEBUG_TRACE(MODULE_RESOURCE, LOG_TRACE, "tsu_reg_msg_proc() tsu_ip_pair_add:eth_name=%s, ip=%s, port=%d, i=%d \r\n", strEthName, pcTsuDeviceIP, 5060, i);
                        }
                    }
                }
                else
                {
                    if (NULL != pcTsuVideoIP)
                    {
                        memset(strEthName, 0, MAX_IP_LEN);
                        snprintf(strEthName, MAX_IP_LEN, "eth%d", iVideoIPEth);
                        i = tsu_ip_pair_add(pTsuResourceInfo, strEthName, IP_ADDR_VIDEO, pcTsuVideoIP, 5060);
                        DEBUG_TRACE(MODULE_RESOURCE, LOG_TRACE, "tsu_reg_msg_proc() tsu_ip_pair_add:eth_name=%s, ip=%s, port=%d, i=%d \r\n", strEthName, pcTsuVideoIP, 5060, i);
                    }

                    if (NULL != pcTsuDeviceIP)
                    {
                        memset(strEthName, 0, MAX_IP_LEN);
                        snprintf(strEthName, MAX_IP_LEN, "eth%d", iDeviceIPEth);
                        i = tsu_ip_pair_add(pTsuResourceInfo, strEthName, IP_ADDR_DEVICE, pcTsuDeviceIP, 5060);
                        DEBUG_TRACE(MODULE_RESOURCE, LOG_TRACE, "tsu_reg_msg_proc() tsu_ip_pair_add:eth_name=%s, ip=%s, port=%d, i=%d \r\n", strEthName, pcTsuDeviceIP, 5060, i);
                    }
                }
            }
        }
    }
    else /* ���� */
    {
        pos = tsu_resource_info_add(pcTsuID, iBoardIndex, iAssignRecord);

        if (pos < 0)
        {
            SystemLog(EV9000_CMS_TSU_REGISTER_ERROR, EV9000_LOG_LEVEL_ERROR, "TSUע��ʧ��:TSU ID=%s, SlotID=%d, IP��ַ=%s, ԭ��=%s", pcTsuID, iSlotID, pcTsuVideoIP, (char*)"��ӵ�TSU��Ϣ����ʧ��");
            EnSystemLog(EV9000_CMS_TSU_REGISTER_ERROR, EV9000_LOG_LEVEL_ERROR, "TSU registration failed: TSU ID=%s SlotID=%d,IP address =%s, cause=%s", pcTsuID, iSlotID, pcTsuVideoIP, (char*)"add to TSU info list failed");

            DEBUG_TRACE(MODULE_RESOURCE, LOG_ERROR, "tsu_reg_msg_proc() exit---: TSU Resource Info Add Error \r\n");
            return -1;
        }

        pTsuResourceInfo = tsu_resource_info_get(pos);

        if (NULL == pTsuResourceInfo)
        {
            SystemLog(EV9000_CMS_TSU_REGISTER_ERROR, EV9000_LOG_LEVEL_ERROR, "TSUע��ʧ��:TSU ID=%s, SlotID=%d, IP��ַ=%s, ԭ��=%s", pcTsuID, iSlotID, pcTsuVideoIP, (char*)"��ȡTSU��Ϣʧ��");
            EnSystemLog(EV9000_CMS_TSU_REGISTER_ERROR, EV9000_LOG_LEVEL_ERROR, "TSU registration failed: TSU ID=%s SlotID=%d,IP address =%s, cause=%s", pcTsuID, iSlotID, pcTsuVideoIP, (char*)"Access TSU info failed");

            DEBUG_TRACE(MODULE_RESOURCE, LOG_ERROR, "tsu_reg_msg_proc() exit---: Get TSU Resource Info Error \r\n");
            return -1;
        }

        if (0 == sstrcmp(pcTsuVideoIP, (char*)"127.0.0.1")
            || 0 == sstrcmp(pcTsuVideoIP, (char*)"0.0.0.0"))
        {
            i = osip_list_clone(pGblconf->pLocalIPAddrList, pTsuResourceInfo->pTSUIPAddrList, (int (*)(void*, void**))&ip_pair_clone);

            if (i != 0)
            {
                SystemLog(EV9000_CMS_TSU_REGISTER_ERROR, EV9000_LOG_LEVEL_ERROR, "TSUע��ʧ��:TSU ID=%s, SlotID=%d, IP��ַ=%s, ԭ��=%s", pcTsuID, iSlotID, pcTsuVideoIP, (char*)"����IP��ַ��Ϣʧ��");
                EnSystemLog(EV9000_CMS_TSU_REGISTER_ERROR, EV9000_LOG_LEVEL_ERROR, "TSU registration failed: TSU ID=%s SlotID=%d,IP address =%s, cause=%s", pcTsuID, iSlotID, pcTsuVideoIP, (char*)"Copy IP address info failed");

                DEBUG_TRACE(MODULE_RESOURCE, LOG_ERROR, "tsu_reg_msg_proc() exit---: TSU IP Addr List Clone Error \r\n");
                return -1;
            }
        }
        else
        {
            if (NULL != pcTsuVideoIP)
            {
                memset(strEthName, 0, MAX_IP_LEN);
                snprintf(strEthName, MAX_IP_LEN, "eth%d", iVideoIPEth);
                i = tsu_ip_pair_add(pTsuResourceInfo, strEthName, IP_ADDR_VIDEO, pcTsuVideoIP, 5060);
                DEBUG_TRACE(MODULE_RESOURCE, LOG_TRACE, "tsu_reg_msg_proc() tsu_ip_pair_add:eth_name=%s, ip=%s, port=%d, i=%d \r\n", strEthName, pcTsuVideoIP, 5060, i);
            }

            if (NULL != pcTsuDeviceIP)
            {
                memset(strEthName, 0, MAX_IP_LEN);
                snprintf(strEthName, MAX_IP_LEN, "eth%d", iDeviceIPEth);
                i = tsu_ip_pair_add(pTsuResourceInfo, strEthName, IP_ADDR_VIDEO, pcTsuDeviceIP, 5060);
                DEBUG_TRACE(MODULE_RESOURCE, LOG_TRACE, "tsu_reg_msg_proc() tsu_ip_pair_add:eth_name=%s, ip=%s, port=%d, i=%d \r\n", strEthName, pcTsuDeviceIP, 5060, i);
            }
        }
    }

    pTsuResourceInfo->iExpires = iExpires;
    //pTsuResourceInfo->iTsuType = iTsuType;

    iRet = set_tsu_index_id(get_tsu_ip(pTsuResourceInfo, default_eth_name_get()), pTsuResourceInfo->iTsuIndex);
    DEBUG_TRACE(MODULE_RESOURCE, LOG_TRACE, "tsu_reg_msg_proc() set_tsu_index_id:tsu_device_id=%s, TsuIndex=%d, i=%d \r\n", pTsuResourceInfo->tsu_device_id, pTsuResourceInfo->iTsuIndex, i);
    printf("tsu_reg_msg_proc() set_tsu_index_id:tsu_device_id=%s, TsuIndex=%d, i=%d \r\n", pTsuResourceInfo->tsu_device_id, pTsuResourceInfo->iTsuIndex, i);

    if (0 == iRefresh) /* �״ε�¼����Ҫֹ֮ͣǰ���������� */
    {
        /* ����֮ǰ�Ǵﵽ����״̬����Ҫ����һ��״̬ */
        pTsuResourceInfo->iStatus = 1;

        iRet = StopAllServiceTaskByTSUID(pcTsuID);

        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "TSU����������:TSU ID=%s, ��Ƶ��IP��ַ=%s, �豸��IP��ַ=%s, ����=%d", pTsuResourceInfo->tsu_device_id, pcTsuVideoIP, pcTsuDeviceIP, pTsuResourceInfo->iTsuIndex);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "TSU online:TSU ID=%s, video network IP address=%s, device network IPaddress=%s, index=%d", pTsuResourceInfo->tsu_device_id, pcTsuVideoIP, pcTsuDeviceIP, pTsuResourceInfo->iTsuIndex);

        if (0 != iRet)
        {
            DEBUG_TRACE(MODULE_RESOURCE, LOG_ERROR, "tsu_reg_msg_proc() ADD: TSU ID=%s, Video IP=%s, Device IP=%s, Index=%d, StopAllServiceTaskByTSUID Error \r\n", pTsuResourceInfo->tsu_device_id, pcTsuVideoIP, pcTsuDeviceIP, pTsuResourceInfo->iTsuIndex);
        }
        else
        {
            DEBUG_TRACE(MODULE_RESOURCE, LOG_TRACE, "tsu_reg_msg_proc() ADD: TSU ID=%s, Video IP=%s, Device IP=%s, Index=%d, StopAllServiceTaskByTSUID OK \r\n", pTsuResourceInfo->tsu_device_id, pcTsuVideoIP, pcTsuDeviceIP, pTsuResourceInfo->iTsuIndex);
        }
    }
    else if (1 == iRefresh)
    {
        /* ���ԭ���ǵ��߻��ߴﵽ����״̬������Ϊ����״̬ */
        if (pTsuResourceInfo->iStatus == 0 || pTsuResourceInfo->iStatus == 2)
        {
            pTsuResourceInfo->iStatus = 1;
        }

        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "TSUˢ��ע��:TSU ID=%s, ��Ƶ��IP��ַ=%s, �豸��IP��ַ=%s, ����=%d", pTsuResourceInfo->tsu_device_id, pcTsuVideoIP, pcTsuDeviceIP, pTsuResourceInfo->iTsuIndex);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "TSU Refresh:TSU ID=%s, video network IP address=%s, device network IPaddress=%s, index=%d", pTsuResourceInfo->tsu_device_id, pcTsuVideoIP, pcTsuDeviceIP, pTsuResourceInfo->iTsuIndex);
    }

    if (1 == pTsuResourceInfo->iUpLoadFlag) /* ��Ҫ�����ϱ�¼���ļ� */
    {
        i = notify_tsu_upload_file_record(get_tsu_ip(pTsuResourceInfo, default_eth_name_get()));

        if (0 == i)
        {
            pTsuResourceInfo->iUpLoadFlag = 0;
            DEBUG_TRACE(MODULE_RESOURCE, LOG_TRACE, "tsu_reg_msg_proc() notify_tsu_upload_file_record OK: tsu_ip=%s \r\n", get_tsu_ip(pTsuResourceInfo, default_eth_name_get()));
        }
        else
        {
            DEBUG_TRACE(MODULE_RESOURCE, LOG_ERROR, "tsu_reg_msg_proc() notify_tsu_upload_file_record Error: tsu_ip=%s \r\n", get_tsu_ip(pTsuResourceInfo, default_eth_name_get()));
        }

        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "֪ͨTSU�����ϱ�¼���ļ�:TSU IP=%s", get_tsu_ip(pTsuResourceInfo, default_eth_name_get()));
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Notify TSU report video file again:TSU IP=%s", get_tsu_ip(pTsuResourceInfo, default_eth_name_get()));
    }

    /* ������˽ṹ����Ϣ */
    snprintf(strDeviceType, 16, "%u", EV9000_DEVICETYPE_MEDIASERVER);
    i = AddTopologyPhyDeviceInfo2DB(pcTsuID, (char*)"", strDeviceType, get_tsu_ip(pTsuResourceInfo, default_eth_name_get()), (char*)"1", local_cms_id_get(), (char*)"1", pTsu_Reg_dboper);

    DEBUG_TRACE(MODULE_RESOURCE, LOG_TRACE, "tsu_reg_msg_proc() exit---: tsu_ip=%s, iTsuIndex=%d \r\n", get_tsu_ip(pTsuResourceInfo, default_eth_name_get()), pTsuResourceInfo->iTsuIndex);
    printf("tsu_reg_msg_proc() exit---: tsu_ip=%s, iTsuIndex=%d \r\n", get_tsu_ip(pTsuResourceInfo, default_eth_name_get()), pTsuResourceInfo->iTsuIndex);
    return 0;
}

/*****************************************************************************
 �� �� ��  : tsu_audio_reg_msg_proc
 ��������  : TSU��Ƶҵ��ע����Ϣ����
 �������  : char* pcTsuID
             int iExpires
             int iRefresh
             DBOper* pTsu_Reg_dboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��11��13��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int tsu_audio_reg_msg_proc(char* pcTsuID, int iExpires, int iRefresh)
{
    int iRet = 0;
    int tsu_pos = -1;
    tsu_resource_info_t* pTsuResourceInfo = NULL;

    if (NULL == pcTsuID)
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_DEBUG, "tsu_audio_reg_msg_proc() exit---:  Param Error \r\n");
        SystemLog(EV9000_CMS_TSU_REGISTER_ERROR, EV9000_LOG_LEVEL_ERROR, "TSU��Ƶת��ҵ��ע��ʧ��:TSU ID=%s, ԭ��=%s", pcTsuID, (char*)"��������");
        EnSystemLog(EV9000_CMS_TSU_REGISTER_ERROR, EV9000_LOG_LEVEL_ERROR, "TSU video forward service registration failed:TSU ID=%s, cause=%s", pcTsuID, (char*)"parameter error");

        return -1;
    }

    /* ��ȡ��Ӧ��TSU��Ϣ */
    tsu_pos = tsu_resource_info_find(pcTsuID);

    if (tsu_pos < 0)
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_ERROR, "tsu_audio_reg_msg_proc() exit---: Find TSU Resource Info Error:TsuID=%s \r\n", pcTsuID);
        SystemLog(EV9000_CMS_TSU_REGISTER_ERROR, EV9000_LOG_LEVEL_ERROR, "TSU��Ƶת��ҵ��ע��ʧ��:TSU ID=%s, ԭ��=%s", pcTsuID, (char*)"���Ҷ�Ӧ��TSU��Դʧ��");
        EnSystemLog(EV9000_CMS_TSU_REGISTER_ERROR, EV9000_LOG_LEVEL_ERROR, "TSU video forward service registration failed:TSU ID=%s, cause =%s", pcTsuID, (char*)"search for corresponding TSU resource failed");

        return -1;
    }

    pTsuResourceInfo = tsu_resource_info_get(tsu_pos);

    if (NULL == pTsuResourceInfo)
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_ERROR, "tsu_audio_reg_msg_proc() exit---: Get TSU Resource Info Error:tsu_pos=%d \r\n", tsu_pos);
        SystemLog(EV9000_CMS_TSU_REGISTER_ERROR, EV9000_LOG_LEVEL_ERROR, "TSU��Ƶת��ҵ��ע��ʧ��:TSU ID=%s, ԭ��=%s", pcTsuID, (char*)"��ȡ��Ӧ��TSU��Դʧ��");
        EnSystemLog(EV9000_CMS_TSU_REGISTER_ERROR, EV9000_LOG_LEVEL_ERROR, "TSU video forward service registration failed:TSU ID=%s, cause=%s", pcTsuID, (char*)"access corresponding TSU resource failed");

        return -1;
    }

    if (pTsuResourceInfo->iAudioStatus == 0)
    {
        pTsuResourceInfo->iAudioStatus = 1;
    }

    pTsuResourceInfo->iAudioExpires = iExpires;

    if (0 == iRefresh) /* �״ε�¼����Ҫֹ֮ͣǰ���������� */
    {
        iRet = StopAudioServiceTaskByTSUID(pcTsuID);

        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "TSU��Ƶת��ҵ������:TSU ID=%s", pTsuResourceInfo->tsu_device_id);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "TSU video forward service online:TSU ID=%s", pTsuResourceInfo->tsu_device_id);

        if (0 != iRet)
        {
            DEBUG_TRACE(MODULE_RESOURCE, LOG_ERROR, "tsu_audio_reg_msg_proc() Audio ADD: TSU ID=%s, StopAudioServiceTaskByTSUID Error \r\n", pTsuResourceInfo->tsu_device_id);
        }
        else
        {
            DEBUG_TRACE(MODULE_RESOURCE, LOG_TRACE, "tsu_audio_reg_msg_proc() Audio ADD: TSU ID=%s, StopAudioServiceTaskByTSUID OK \r\n", pTsuResourceInfo->tsu_device_id);
        }
    }

    return 0;
}
#endif

#if DECS("TSU ������Ϣ����")
/*****************************************************************************
 �� �� ��  : tsu_task_attribute_msg_init
 ��������  : TSU������Ϣ�ṹ��ʼ��
 �������  : tsu_task_attribute_t ** tsu_task_attribute_msg
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��11��13��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int tsu_task_attribute_msg_init(tsu_task_attribute_t** tsu_task_attribute_msg)
{
    *tsu_task_attribute_msg = (tsu_task_attribute_t*)osip_malloc(sizeof(tsu_task_attribute_t));

    if (*tsu_task_attribute_msg == NULL)
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_DEBUG, "tsu_task_attribute_msg_init() exit---: *tsu_task_attribute_msg Smalloc Error \r\n");
        return -1;
    }

    (*tsu_task_attribute_msg)->iMsgType = -1;
    (*tsu_task_attribute_msg)->pcTsuID[0] = '\0';
    (*tsu_task_attribute_msg)->iTaskType = -1;
    (*tsu_task_attribute_msg)->pcTaskID[0] = '\0';

    return 0;
}

/*****************************************************************************
 �� �� ��  : tsu_task_attribute_msg_free
 ��������  : TSU ������Ϣ�ṹ�ͷ�
 �������  : tsu_task_attribute_t * tsu_task_attribute_msg
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��11��13��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void tsu_task_attribute_msg_free(tsu_task_attribute_t* tsu_task_attribute_msg)
{
    if (tsu_task_attribute_msg == NULL)
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_DEBUG, "tsu_task_attribute_msg_free() exit---: Param Error \r\n");
        return;
    }

    tsu_task_attribute_msg->iMsgType = -1;

    memset(tsu_task_attribute_msg->pcTsuID, 0, MAX_ID_LEN + 4);

    tsu_task_attribute_msg->iTaskType = -1;

    memset(tsu_task_attribute_msg->pcTaskID, 0, MAX_TSU_TASK_LEN + 4);

    return;
}

/*****************************************************************************
 �� �� ��  : tsu_task_attribute_msg_list_init
 ��������  : TSU ������Ϣ���г�ʼ��
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��11��13��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int tsu_task_attribute_msg_list_init()
{
    g_TSUTaskMsgQueue.clear();

    return 0;
}

/*****************************************************************************
 �� �� ��  : tsu_task_attribute_msg_list_free
 ��������  : TSU ������Ϣ�����ͷ�
 �������  :
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��11��13��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void tsu_task_attribute_msg_list_free()
{
    tsu_task_attribute_t* pTsuTaskMsg = NULL;

    while (!g_TSUTaskMsgQueue.empty())
    {
        pTsuTaskMsg = (tsu_task_attribute_t*) g_TSUTaskMsgQueue.front();
        g_TSUTaskMsgQueue.pop_front();

        if (NULL != pTsuTaskMsg)
        {
            tsu_task_attribute_msg_free(pTsuTaskMsg);
            osip_free(pTsuTaskMsg);
            pTsuTaskMsg = NULL;
        }
    }

    g_TSUTaskMsgQueue.clear();

    return;
}

/*****************************************************************************
 �� �� ��  : tsu_task_attribute_msg_add
 ��������  : ���TSU ������Ϣ��������
 �������  :int iMsgType
                           char* pcTsuID
                           int iTaskType
                           char* pcTaskID
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��11��13��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int tsu_task_attribute_msg_add(int iMsgType, char* pcTsuID, int iTaskType, char* pcTaskID)
{
    tsu_task_attribute_t* pTsuTaskMsg = NULL;
    int iRet = 0;

    if (NULL == pcTsuID || NULL == pcTaskID)
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_DEBUG, "tsu_task_attribute_msg_add() exit---: Param Error \r\n");
        return -1;
    }

    iRet = tsu_task_attribute_msg_init(&pTsuTaskMsg);

    if (iRet != 0)
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_ERROR, "tsu_task_attribute_msg_add() exit---: Message Init Error \r\n");
        return -1;
    }

    pTsuTaskMsg->iMsgType = iMsgType;

    if (NULL != pcTsuID)
    {
        osip_strncpy(pTsuTaskMsg->pcTsuID, pcTsuID, MAX_ID_LEN);
    }

    pTsuTaskMsg->iTaskType = iTaskType;

    if (NULL != pcTaskID)
    {
        osip_strncpy(pTsuTaskMsg->pcTaskID, pcTaskID, MAX_TSU_TASK_LEN);
    }

    g_TSUTaskMsgQueue.push_back(pTsuTaskMsg);

    return 0;
}

/*****************************************************************************
 �� �� ��  : scan_tsu_task_attribute_msg_list
 ��������  : ɨ��TSU ������Ϣ����
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��11��13 �� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void scan_tsu_task_attribute_msg_list()
{
    int iRet = 0;
    tsu_task_attribute_t* pTsuTaskMsg = NULL;

    while (!g_TSUTaskMsgQueue.empty())
    {
        pTsuTaskMsg = (tsu_task_attribute_t*) g_TSUTaskMsgQueue.front();
        g_TSUTaskMsgQueue.pop_front();

        if (NULL != pTsuTaskMsg)
        {
            break;
        }
    }

    if (NULL != pTsuTaskMsg)
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_INFO,  "scan_tsu_task_attribute_msg_list() \
        \r\n MsgType=%d \
        \r\n TsuID=%s \
        \r\n TaskType=%d \
        \r\n TaskID=%s \
        \r\n ", pTsuTaskMsg->iMsgType, pTsuTaskMsg->pcTsuID, pTsuTaskMsg->iTaskType, pTsuTaskMsg->pcTaskID);

        iRet = tsu_task_attribute_msg_proc(pTsuTaskMsg->iMsgType, pTsuTaskMsg->pcTsuID, pTsuTaskMsg->iTaskType, pTsuTaskMsg->pcTaskID);
        tsu_task_attribute_msg_free(pTsuTaskMsg);
        osip_free(pTsuTaskMsg);
        pTsuTaskMsg = NULL;
    }

    return;
}

/*****************************************************************************
 �� �� ��  : tsu_task_attribute_msg_proc
 ��������  : TSU������Ϣ����
 �������  : int iMsgType
             char* pcTsuID
             int iTaskType
             char* pcTaskID
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��11��13��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int tsu_task_attribute_msg_proc(int iMsgType, char* pcTsuID, int iTaskType, char* pcTaskID)
{
    int iRet = 0;
    int tsu_pos = -1;
    int cr_pos = -1;
    char* tsu_ip = NULL;
    cr_t* pCrData = NULL;
    tsu_resource_info_t* pTsuResourceInfo = NULL;

    if (NULL == pcTsuID || NULL == pcTaskID)
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_DEBUG, "tsu_task_attribute_msg_proc() exit---: Param Error \r\n");
        return -1;
    }

    if (0 == iMsgType) /* �ϱ������� */
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "TSU�ϱ�������:TSUID=%s, TaskID=%s", pcTsuID, pcTaskID);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "TSU Notify Task Proc:TSUID=%s, TaskID=%s", pcTsuID, pcTaskID);

        /* ��ȡTSU����Ϣ */
        tsu_pos = tsu_resource_info_find(pcTsuID);

        if (tsu_pos < 0)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "TSU�ϱ�������ʧ��:TSUID=%s, ԭ��=%s", pcTsuID, (char*)"����TSU ID���Ҷ�Ӧ��TSU��Ϣʧ��");
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "TSU Notify Task Proc:TSUID=%s, Reason=%s", pcTsuID, (char*)"Find TSU Info Error");

            DEBUG_TRACE(MODULE_RESOURCE, LOG_ERROR, "tsu_task_attribute_msg_proc() exit---: Find TSU Resource Info Error:TsuID=%s \r\n", pcTsuID);
            return -1;
        }

        pTsuResourceInfo = tsu_resource_info_get(tsu_pos);

        if (NULL == pTsuResourceInfo)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "TSU�ϱ�������ʧ��:TSUID=%s, ԭ��=%s, tsu_pos=%d", pcTsuID, (char*)"��ȡ��Ӧ��TSU��Ϣʧ��", tsu_pos);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "TSU Notify Task Proc:TSUID=%s, Reason=%s, tsu_pos=%d", pcTsuID, (char*)"Get TSU Info Error", tsu_pos);

            DEBUG_TRACE(MODULE_RESOURCE, LOG_ERROR, "tsu_task_attribute_msg_proc() exit---: Get TSU Resource Info Error:tsu_pos=%d \r\n", tsu_pos);
            return -1;
        }

        /* ����ת������ */
        cr_pos = call_record_find_by_task_id(pcTaskID);

        if (cr_pos >= 0)
        {
            iRet = set_call_record_tsu_expire(cr_pos);

            if (0 != iRet)
            {
                DEBUG_TRACE(MODULE_RESOURCE, LOG_ERROR, "tsu_task_attribute_msg_proc() set_call_record_tsu_expire Error:Task ID=%s, cr_pos=%d \r\n", pcTaskID, cr_pos);
            }

            pCrData = call_record_get(cr_pos);

            if (NULL != pCrData)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "TSU�ϱ����������:TSUID=%s, TaskID=%s, �Ѿ����ڵ�����:cr_pos=%d", pcTsuID, pcTaskID, cr_pos);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "TSU Notify Task Proc:TSUID=%s, TaskID=%s, exit task:cr_pos=%d", pcTsuID, pcTaskID, cr_pos);
                DEBUG_TRACE(MODULE_RESOURCE, LOG_INFO, "tsu_task_attribute_msg_proc() exit---: Find Call Record Info OK:Task ID=%s, cr_pos=%d, call_type=%d \r\n", pcTaskID, cr_pos, pCrData->call_type);
            }

            return 0;
        }
        else
        {
            DEBUG_TRACE(MODULE_RESOURCE, LOG_WARN, "tsu_task_attribute_msg_proc() Not Find Call Record Info:Task ID=%s, cr_pos=%d \r\n", pcTaskID, cr_pos);
        }

        /* ��ȡ��TSUͨ�ŵ�IP��ַ */
        tsu_ip = get_tsu_ip(pTsuResourceInfo, default_eth_name_get());

        if (NULL == tsu_ip)
        {
            DEBUG_TRACE(MODULE_RESOURCE, LOG_ERROR, "tsu_task_attribute_msg_proc() exit---: Get TSU IP Error:eth_name=%s \r\n", default_eth_name_get());
            return -1;
        }

        //type = 1 //¼��
        //type = 2 //ת��
        //type = 3 //�طŻ�������

        /* û�е�ת��������Ҫ�Ƴ� */
        if (1 == iTaskType)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "�Ƴ�TSU�ϱ��Ĳ����ڵ�����:TSU IP=%s, TaskID=%s", tsu_ip, pcTaskID);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Remove non existing task reported by TSU:TSU IP=%s, TaskID=%s", tsu_ip, pcTaskID);

            iRet = notify_tsu_delete_record_task(tsu_ip, pcTaskID);
            //DEBUG_TRACE(MODULE_RESOURCE, LOG_INFO, "tsu_task_attribute_msg_proc() notify_tsu_delete_record_task:ID=%s,iRet=%d \r\n", pcTaskID, iRet);

            if (iRet < 0)
            {
                DEBUG_TRACE(MODULE_RESOURCE, LOG_WARN, "tsu_task_attribute_msg_proc() notify_tsu_delete_record_task Error:tsu_ip=%s, task_id=%s, i=%d \r\n", tsu_ip, pcTaskID, iRet);
            }
            else
            {
                DEBUG_TRACE(MODULE_RESOURCE, LOG_TRACE, "tsu_task_attribute_msg_proc() notify_tsu_delete_record_task OK:tsu_ip=%s, task_id=%s, i=%d \r\n", tsu_ip, pcTaskID, iRet);
            }

            /* ����TSU ״̬ */
            if (2 == pTsuResourceInfo->iStatus)
            {
                pTsuResourceInfo->iStatus = 1;
            }
        }
        else if (2 == iTaskType)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "�Ƴ�TSU�ϱ��Ĳ����ڵ�����:TSU IP=%s, TaskID=%s", tsu_ip, pcTaskID);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Remove non existing task reported by TSU:TSU IP=%s, TaskID=%s", tsu_ip, pcTaskID);

            iRet = notify_tsu_delete_transfer_task(tsu_ip, pcTaskID);

            if (iRet < 0)
            {
                DEBUG_TRACE(MODULE_RESOURCE, LOG_WARN, "tsu_task_attribute_msg_proc() notify_tsu_delete_transfer_task Error:tsu_ip=%s, task_id=%s, i=%d \r\n", tsu_ip, pcTaskID, iRet);
            }
            else
            {
                DEBUG_TRACE(MODULE_RESOURCE, LOG_TRACE, "tsu_task_attribute_msg_proc() notify_tsu_delete_transfer_task OK:tsu_ip=%s, task_id=%s, i=%d \r\n", tsu_ip, pcTaskID, iRet);
            }
        }
        else if (3 == iTaskType)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�Ƴ�TSU�ϱ��Ĳ����ڵ�����:TSU IP=%s, TaskID=%s", tsu_ip, pcTaskID);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Remove non existing task reported by TSU:TSU IP=%s, TaskID=%s", tsu_ip, pcTaskID);

            iRet = notify_tsu_delete_replay_task(tsu_ip, pcTaskID);

            if (iRet < 0)
            {
                DEBUG_TRACE(MODULE_RESOURCE, LOG_WARN, "tsu_task_attribute_msg_proc() Notify TSU Delete Replay Task Error:tsu_ip=%s, task_id=%s, i=%d \r\n", tsu_ip, pcTaskID, iRet);
            }
            else
            {
                DEBUG_TRACE(MODULE_RESOURCE, LOG_TRACE, "tsu_task_attribute_msg_proc() Notify TSU Delete Replay Task OK:tsu_ip=%s, task_id=%s, i=%d \r\n", tsu_ip, pcTaskID, iRet);
            }

            pCrData = call_record_get(cr_pos);

            if (NULL != pCrData)
            {
                /* ����Bye �����в� */
                iRet = SIP_SendBye(pCrData->caller_ua_index);
                DEBUG_TRACE(MODULE_RESOURCE, LOG_TRACE, "tsu_task_attribute_msg_proc() SIP_SendBye:caller_ua_index=%d, iRet=%d \r\n", pCrData->caller_ua_index, iRet);
            }

            /* �Ƴ����м�¼��Ϣ */
            iRet = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
            iRet = call_record_remove(cr_pos);

            if (0 != iRet)
            {
                DEBUG_TRACE(MODULE_RESOURCE, LOG_ERROR, "tsu_task_attribute_msg_proc() call_record_remove Error:cr_pos=%d \r\n", cr_pos);
            }
            else
            {
                DEBUG_TRACE(MODULE_RESOURCE, LOG_INFO, "tsu_task_attribute_msg_proc() call_record_remove OK:cr_pos=%d \r\n", cr_pos);
            }
        }
    }
    else if (1 == iMsgType) /* �ط�����֪ͨ���� */
    {
        SystemLog(EV9000_CMS_TSU_NOTIFY_PLAYCLOSE, EV9000_LOG_LEVEL_WARNING, "TSU֪ͨ�ط��������, TsuID=%s, TaskID=%s", pcTsuID, pcTaskID);
        EnSystemLog(EV9000_CMS_TSU_NOTIFY_PLAYCLOSE, EV9000_LOG_LEVEL_WARNING, "TSU nofity playback task is over, TsuID=%s, TaskID=%s", pcTsuID, pcTaskID);

        iRet = StopRecordPlayServiceByTaskID(pcTsuID, iTaskType, pcTaskID);

        if (0 != iRet)
        {
            DEBUG_TRACE(MODULE_RESOURCE, LOG_ERROR, "tsu_task_attribute_msg_proc() StopRecordPlayServiceByTaskID Error:TsuID=%s, TaskType=%d, TaskID=%s \r\n", pcTsuID, iTaskType, pcTaskID);
        }
        else
        {
            DEBUG_TRACE(MODULE_RESOURCE, LOG_TRACE, "tsu_task_attribute_msg_proc() StopRecordPlayServiceByTaskID OK:TsuID=%s, TaskType=%d, TaskID=%s \r\n", pcTsuID, iTaskType, pcTaskID);
        }
    }
    else if (2 == iMsgType) /* ��ͣ���� */
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "TSU֪ͨ��ͣ����, TsuID=%s, TaskID=%s", pcTsuID, pcTaskID);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "TSU notify to pause play, TsuID=%s, TaskID=%s", pcTsuID, pcTaskID);

        iRet = SendInfoToCalleeByTaskID(pcTsuID, 1, pcTaskID);

        if (0 != iRet)
        {
            DEBUG_TRACE(MODULE_RESOURCE, LOG_ERROR, "tsu_task_attribute_msg_proc() SendInfoToCalleeByTaskID Error:TsuID=%s, TaskID=%s \r\n", pcTsuID, pcTaskID);
        }
        else
        {
            DEBUG_TRACE(MODULE_RESOURCE, LOG_TRACE, "tsu_task_attribute_msg_proc() SendInfoToCalleeByTaskID OK:TsuID=%s, TaskID=%s \r\n", pcTsuID, pcTaskID);
        }
    }
    else if (3 == iMsgType) /* �ָ�����*/
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "TSU֪ͨ�ָ�����, TsuID=%s, TaskID=%s", pcTsuID, pcTaskID);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "TSU notify recover play, TsuID=%s, TaskID=%s", pcTsuID, pcTaskID);

        iRet = SendInfoToCalleeByTaskID(pcTsuID, 2, pcTaskID);

        if (0 != iRet)
        {
            DEBUG_TRACE(MODULE_RESOURCE, LOG_ERROR, "tsu_task_attribute_msg_proc() SendInfoToCalleeByTaskID Error:TsuID=%s, TaskID=%s \r\n", pcTsuID, pcTaskID);
        }
        else
        {
            DEBUG_TRACE(MODULE_RESOURCE, LOG_TRACE, "tsu_task_attribute_msg_proc() SendInfoToCalleeByTaskID OK:TsuID=%s, TaskID=%s \r\n", pcTsuID, pcTaskID);
        }
    }
    else if (4 == iMsgType)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "TSU֪ͨ����TCP���ӶϿ�, TSUID=%s, TaskID=%s", pcTsuID, pcTaskID);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "TSU Notify Media TCP End:TSUID=%s, TaskID=%s", pcTsuID, pcTaskID);

        iRet = StopAllServiceTaskByTaskID(pcTaskID);

        if (0 != iRet)
        {
            DEBUG_TRACE(MODULE_RESOURCE, LOG_ERROR, "tsu_task_attribute_msg_proc() StopAllServiceTaskByTaskID Error:TsuID=%s, TaskID=%s \r\n", pcTsuID, pcTaskID);
        }
        else
        {
            DEBUG_TRACE(MODULE_RESOURCE, LOG_TRACE, "tsu_task_attribute_msg_proc() StopAllServiceTaskByTaskID OK:TsuID=%s, TaskID=%s \r\n", pcTsuID, pcTaskID);
        }
    }

    return iRet;
}
#endif

#if DECS("TSU��Ƶ������Ϣ����")
/*****************************************************************************
 �� �� ��  : tsu_audio_task_attribute_msg_init
 ��������  : TSU��Ƶ������Ϣ�ṹ��ʼ��
 �������  : tsu_audio_task_attribute_t ** tsu_audio_task_attribute_msg
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��11��13��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int tsu_audio_task_attribute_msg_init(tsu_audio_task_attribute_t** tsu_audio_task_attribute_msg)
{
    *tsu_audio_task_attribute_msg = (tsu_audio_task_attribute_t*)osip_malloc(sizeof(tsu_audio_task_attribute_t));

    if (*tsu_audio_task_attribute_msg == NULL)
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_DEBUG, "tsu_audio_task_attribute_msg_init() exit---: *tsu_audio_task_attribute_msg Smalloc Error \r\n");
        return -1;
    }

    (*tsu_audio_task_attribute_msg)->pcTsuID[0] = '\0';
    (*tsu_audio_task_attribute_msg)->pcReceiveIP[0] = '\0';
    (*tsu_audio_task_attribute_msg)->iReceivePort = 0;

    return 0;
}

/*****************************************************************************
 �� �� ��  : tsu_audio_task_attribute_msg_free
 ��������  : TSU ������Ϣ�ṹ�ͷ�
 �������  : tsu_audio_task_attribute_t * tsu_audio_task_attribute_msg
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��05��09��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void tsu_audio_task_attribute_msg_free(tsu_audio_task_attribute_t* tsu_audio_task_attribute_msg)
{
    if (tsu_audio_task_attribute_msg == NULL)
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_DEBUG, "tsu_audio_task_attribute_msg_free() exit---: Param Error \r\n");
        return;
    }

    memset(tsu_audio_task_attribute_msg->pcTsuID, 0, MAX_ID_LEN + 4);
    memset(tsu_audio_task_attribute_msg->pcReceiveIP, 0, 16);
    tsu_audio_task_attribute_msg->iReceivePort = 0;

    return;
}

/*****************************************************************************
 �� �� ��  : tsu_audio_task_attribute_msg_list_init
 ��������  : TSU ������Ϣ���г�ʼ��
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��05��09��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int tsu_audio_task_attribute_msg_list_init()
{
    g_TSUAudioTaskMsgQueue.clear();

    return 0;
}

/*****************************************************************************
 �� �� ��  : tsu_audio_task_attribute_msg_list_free
 ��������  : TSU ������Ϣ�����ͷ�
 �������  :
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��05��09��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void tsu_audio_task_attribute_msg_list_free()
{
    tsu_audio_task_attribute_t* pTsuAudioTaskMsg = NULL;

    while (!g_TSUAudioTaskMsgQueue.empty())
    {
        pTsuAudioTaskMsg = (tsu_audio_task_attribute_t*) g_TSUAudioTaskMsgQueue.front();
        g_TSUAudioTaskMsgQueue.pop_front();

        if (NULL != pTsuAudioTaskMsg)
        {
            tsu_audio_task_attribute_msg_free(pTsuAudioTaskMsg);
            osip_free(pTsuAudioTaskMsg);
            pTsuAudioTaskMsg = NULL;
        }
    }

    g_TSUAudioTaskMsgQueue.clear();

    return;
}

/*****************************************************************************
 �� �� ��  : tsu_audio_task_attribute_msg_add
 ��������  : ���TSU ������Ϣ��������
 �������  : char* pcTsuID
             char* pcReceiveIP
             int iReceivePort
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��05��09��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int tsu_audio_task_attribute_msg_add(char* pcTsuID, char* pcReceiveIP, int iReceivePort)
{
    tsu_audio_task_attribute_t* pTsuAudioTaskMsg = NULL;
    int iRet = 0;

    if (NULL == pcTsuID || NULL == pcReceiveIP || iReceivePort <= 0)
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_DEBUG, "tsu_audio_task_attribute_msg_add() exit---: Param Error \r\n");
        return -1;
    }

    iRet = tsu_audio_task_attribute_msg_init(&pTsuAudioTaskMsg);

    if (iRet != 0)
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_ERROR, "tsu_audio_task_attribute_msg_add() exit---: Message Init Error \r\n");
        return -1;
    }

    if (NULL != pcTsuID)
    {
        osip_strncpy(pTsuAudioTaskMsg->pcTsuID, pcTsuID, MAX_ID_LEN);
    }

    if (NULL != pcReceiveIP)
    {
        osip_strncpy(pTsuAudioTaskMsg->pcReceiveIP, pcReceiveIP, 15);
    }

    pTsuAudioTaskMsg->iReceivePort = iReceivePort;

    g_TSUAudioTaskMsgQueue.push_back(pTsuAudioTaskMsg);

    return 0;
}

/*****************************************************************************
 �� �� ��  : scan_tsu_audio_task_attribute_msg_list
 ��������  : ɨ��TSU��Ƶ������Ϣ����
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��11��13 �� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void scan_tsu_audio_task_attribute_msg_list()
{
    int iRet = 0;
    tsu_audio_task_attribute_t* pTsuAudioTaskMsg = NULL;

    while (!g_TSUAudioTaskMsgQueue.empty())
    {
        pTsuAudioTaskMsg = (tsu_audio_task_attribute_t*) g_TSUAudioTaskMsgQueue.front();
        g_TSUAudioTaskMsgQueue.pop_front();

        if (NULL != pTsuAudioTaskMsg)
        {
            break;
        }
    }

    if (NULL != pTsuAudioTaskMsg)
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_INFO,  "scan_tsu_audio_task_attribute_msg_list() \
        \r\n TsuID=%s \
        \r\n ReceiveIP=%s \
        \r\n ReceivePort=%s \
        \r\n ", pTsuAudioTaskMsg->pcTsuID, pTsuAudioTaskMsg->pcReceiveIP, pTsuAudioTaskMsg->iReceivePort);

        iRet = tsu_audio_task_attribute_msg_proc(pTsuAudioTaskMsg->pcTsuID, pTsuAudioTaskMsg->pcReceiveIP, pTsuAudioTaskMsg->iReceivePort);
        tsu_audio_task_attribute_msg_free(pTsuAudioTaskMsg);
        osip_free(pTsuAudioTaskMsg);
        pTsuAudioTaskMsg = NULL;
    }

    return;
}

/*****************************************************************************
 �� �� ��  : tsu_audio_task_attribute_msg_proc
 ��������  : TSU��Ƶ������Ϣ����
 �������  : char* pcTsuID
             char* pcReceiveIP
             int iReceivePort
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��05��09��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int tsu_audio_task_attribute_msg_proc(char* pcTsuID, char* pcReceiveIP, int iReceivePort)
{
    int iRet = 0;
    int tsu_pos = -1;
    int cr_pos = -1;
    char* tsu_ip = NULL;
    tsu_resource_info_t* pTsuResourceInfo = NULL;

    if (NULL == pcTsuID || NULL == pcReceiveIP || iReceivePort <= 0)
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_DEBUG, "tsu_audio_task_attribute_msg_proc() exit---: Param Error \r\n");
        return -1;
    }

    /* ����ת������ */
    cr_pos = audio_call_record_find_by_send_info(pcReceiveIP, iReceivePort);

    if (cr_pos >= 0)
    {
        iRet = set_call_record_tsu_expire(cr_pos);

        if (0 != iRet)
        {
            DEBUG_TRACE(MODULE_RESOURCE, LOG_ERROR, "tsu_audio_task_attribute_msg_proc() set_call_record_tsu_expire Error:Receive IP=%s, Receive Port=%d, cr_pos=%d \r\n", pcReceiveIP, iReceivePort, cr_pos);
        }

        DEBUG_TRACE(MODULE_RESOURCE, LOG_INFO, "tsu_audio_task_attribute_msg_proc() exit---: Find Call Record Info OK:Receive IP=%s, Receive Port=%d, cr_pos=%d \r\n", pcReceiveIP, iReceivePort, cr_pos);
        return 0;
    }
    else
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_WARN, "tsu_audio_task_attribute_msg_proc() Not Find Call Record Info:Receive IP=%s, Receive Port=%d, cr_pos=%d \r\n", pcReceiveIP, iReceivePort, cr_pos);
    }

    /* ��ȡTSU����Ϣ */
    tsu_pos = tsu_resource_info_find(pcTsuID);

    if (tsu_pos < 0)
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_ERROR, "tsu_audio_task_attribute_msg_proc() exit---: Find TSU Resource Info Error:TsuID=%s \r\n", pcTsuID);
        return -1;
    }

    pTsuResourceInfo = tsu_resource_info_get(tsu_pos);

    if (NULL == pTsuResourceInfo)
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_ERROR, "tsu_audio_task_attribute_msg_proc() exit---: Get TSU Resource Info Error:tsu_pos=%d \r\n", tsu_pos);
        return -1;
    }

    /* ��ȡ��TSUͨ�ŵ�IP��ַ */
    tsu_ip = get_tsu_ip(pTsuResourceInfo, default_eth_name_get());

    if (NULL == tsu_ip)
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_ERROR, "tsu_audio_task_attribute_msg_proc() exit---: Get TSU IP Error:eth_name=%s \r\n", default_eth_name_get());
        return -1;
    }

    /* û�е�ת��������Ҫ�Ƴ� */
    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "�Ƴ�TSU�ϱ��Ĳ����ڵ���Ƶת������:TSU IP=%s, Receive IP=%s, Receive Port=%d", tsu_ip, pcReceiveIP, iReceivePort);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Remove non exist voice forwarding tasks reported by TSU:TSU IP=%s, Receive IP=%s, Receive Port=%d", tsu_ip, pcReceiveIP, iReceivePort);

    iRet = notify_tsu_delete_audio_transfer_task(tsu_ip, pcReceiveIP, iReceivePort);

    if (iRet < 0)
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_WARN, "tsu_audio_task_attribute_msg_proc() notify_tsu_delete_audio_transfer_task Error:tsu_ip=%s, Receive IP=%s, Receive Port=%d, i=%d \r\n", tsu_ip, pcReceiveIP, iReceivePort, iRet);
    }
    else
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_TRACE, "tsu_audio_task_attribute_msg_proc() notify_tsu_delete_audio_transfer_task OK:tsu_ip=%s, Receive IP=%s, Receive Port=%d, i=%d \r\n", tsu_ip, pcReceiveIP, iReceivePort, iRet);
    }

    return iRet;
}
#endif

#if DECS("TSU û������֪ͨ��Ϣ����")
/*****************************************************************************
 �� �� ��  : tsu_no_stream_msg_init
 ��������  : TSUû��������Ϣ�ṹ��ʼ��
 �������  : tsu_no_stream_msg_t ** tsu_no_stream_msg
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��11��13��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int tsu_no_stream_msg_init(tsu_no_stream_msg_t** tsu_no_stream_msg)
{
    *tsu_no_stream_msg = (tsu_no_stream_msg_t*)osip_malloc(sizeof(tsu_no_stream_msg_t));

    if (*tsu_no_stream_msg == NULL)
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_DEBUG, "tsu_no_stream_msg_init() exit---: *tsu_no_stream_msg_t Smalloc Error \r\n");
        return -1;
    }

    (*tsu_no_stream_msg)->pcDeviceID[0] = '\0';

    return 0;
}

/*****************************************************************************
 �� �� ��  : tsu_no_stream_msg_free
 ��������  : TSU û��������Ϣ�ṹ�ͷ�
 �������  : tsu_no_stream_msg_t * tsu_no_stream_msg
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��11��13��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void tsu_no_stream_msg_free(tsu_no_stream_msg_t* tsu_no_stream_msg)
{
    if (tsu_no_stream_msg == NULL)
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_DEBUG, "tsu_no_stream_msg_free() exit---: Param Error \r\n");
        return;
    }

    memset(tsu_no_stream_msg->pcDeviceID, 0, MAX_ID_LEN + 4);

    return;
}

/*****************************************************************************
 �� �� ��  : tsu_no_stream_msg_list_init
 ��������  : TSUû��������Ϣ���г�ʼ��
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��11��13��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int tsu_no_stream_msg_list_init()
{
    g_TSUNoStreamMsgQueue.clear();

    return 0;
}

/*****************************************************************************
 �� �� ��  : tsu_no_stream_msg_list_free
 ��������  : TSU û��������Ϣ�����ͷ�
 �������  :
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��11��13��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void tsu_no_stream_msg_list_free()
{
    tsu_no_stream_msg_t* pTsuNoStreamMsg = NULL;

    while (!g_TSUNoStreamMsgQueue.empty())
    {
        pTsuNoStreamMsg = (tsu_no_stream_msg_t*) g_TSUNoStreamMsgQueue.front();
        g_TSUNoStreamMsgQueue.pop_front();

        if (NULL != pTsuNoStreamMsg)
        {
            tsu_no_stream_msg_free(pTsuNoStreamMsg);
            osip_free(pTsuNoStreamMsg);
            pTsuNoStreamMsg = NULL;
        }
    }

    g_TSUNoStreamMsgQueue.clear();

    return;
}

/*****************************************************************************
 �� �� ��  : tsu_no_stream_msg_add
 ��������  : ���TSU û��������Ϣ��������
 �������  : char* user_id
                            int reg_info_index
                            int reg_routes_index
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��11��13��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int tsu_no_stream_msg_add(char* pcDeviceID)
{
    tsu_no_stream_msg_t* pTsuNoStreamMsg = NULL;
    int iRet = 0;

    if (NULL == pcDeviceID)
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_DEBUG, "tsu_no_stream_msg_add() exit---: Param Error \r\n");
        return -1;
    }

    iRet = tsu_no_stream_msg_init(&pTsuNoStreamMsg);

    if (iRet != 0)
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_ERROR, "tsu_no_stream_msg_add() exit---: Message Init Error \r\n");
        return -1;
    }

    if (NULL != pcDeviceID)
    {
        osip_strncpy(pTsuNoStreamMsg->pcDeviceID, pcDeviceID, MAX_ID_LEN + 2);
    }

    g_TSUNoStreamMsgQueue.push_back(pTsuNoStreamMsg);

    return 0;
}

/*****************************************************************************
 �� �� ��  : scan_tsu_no_stream_msg_list
 ��������  : ɨ��TSU û��������Ϣ����
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��11��13 �� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void scan_tsu_no_stream_msg_list()
{
    int iRet = 0;
    tsu_no_stream_msg_t* pTsuNoStreamMsg = NULL;

    while (!g_TSUNoStreamMsgQueue.empty())
    {
        pTsuNoStreamMsg = (tsu_no_stream_msg_t*) g_TSUNoStreamMsgQueue.front();

        if (NULL != pTsuNoStreamMsg)
        {
            g_TSUNoStreamMsgQueue.pop_front();
            break;
        }
    }

    if (NULL != pTsuNoStreamMsg)
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_INFO,  "scan_tsu_no_stream_msg_list() \
        \r\n DeviceID=%s \
        \r\n ", pTsuNoStreamMsg->pcDeviceID);

        iRet = tsu_no_stream_msg_proc(pTsuNoStreamMsg->pcDeviceID);
        tsu_no_stream_msg_free(pTsuNoStreamMsg);
        osip_free(pTsuNoStreamMsg);
        pTsuNoStreamMsg = NULL;
    }

    return;
}

/*****************************************************************************
 �� �� ��  : tsu_no_stream_msg_proc
 ��������  : TSUû��������Ϣ����
 �������  : char* pcDeviceID
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��11��13��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int tsu_no_stream_msg_proc(char* pcDeviceID)
{
    int iRet = 0;
    GBLogicDevice_info_t* pGBLogicDeviceInfo = NULL;
    GBDevice_info_t* pGBDeviceInfo = NULL;
    record_info_t* pRecordInfo = NULL;
    char strDeviceID[MAX_ID_LEN + 4] = {0};
    char strServiceType[2] = {0};
    char strRecordType[2] = {0};
    int iServiceType = 0;
    int iRecordType = 0;
    int iStreamType = 0;

    if (NULL == pcDeviceID)
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_DEBUG, "tsu_no_stream_msg_proc() exit---: Param Error \r\n");
        return -1;
    }

    osip_strncpy(strDeviceID, pcDeviceID, 20);
    osip_strncpy(strServiceType, &pcDeviceID[20], 1);
    osip_strncpy(strRecordType, &pcDeviceID[21], 1);

    pGBLogicDeviceInfo = GBLogicDevice_info_find(strDeviceID);

    if (NULL == pGBLogicDeviceInfo)
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_ERROR, "tsu_no_stream_msg_proc() exit---: Get GBLogicDevice Info Error:DeviceID=%s \r\n", strDeviceID);
        return -1;
    }

    pGBLogicDeviceInfo->no_stream_count++;

    /* ȷ�������� */
    iServiceType = osip_atoi(strServiceType);
    iRecordType = strtol(strRecordType, NULL, 16);

    if (1 == iServiceType) /* ��¼�� */
    {
        if (iRecordType == EV9000_RECORD_TYPE_NORMAL)
        {
            /* ����¼����Ϣ��ȷ�������� */
            pRecordInfo = record_info_get_by_record_type(pGBLogicDeviceInfo->id, iRecordType);

            if (NULL == pRecordInfo)
            {
                DEBUG_TRACE(MODULE_RESOURCE, LOG_ERROR, "tsu_no_stream_msg_proc() exit---: Get Record Info Error:DeviceID=%s, RecordType=%d \r\n", pGBLogicDeviceInfo->device_id, iRecordType);
                return -1;
            }

            iStreamType = pRecordInfo->stream_type;

        }
        else if (iRecordType == EV9000_RECORD_TYPE_INTELLIGENCE)
        {
            iStreamType = EV9000_STREAM_TYPE_INTELLIGENCE;
        }
        else
        {
            iStreamType = EV9000_STREAM_TYPE_MASTER;
        }
    }
    else /* ����¼�� */
    {
        iStreamType = iRecordType;
    }

    DEBUG_TRACE(MODULE_RESOURCE, LOG_INFO, "tsu_no_stream_msg_proc() DeviceID=%s, GBLogicDeviceID=%s, ServiceType=%d, RecordType=%d, StreamType=%d \r\n", pcDeviceID, strDeviceID, iServiceType, iRecordType, iStreamType);

    pGBDeviceInfo = GBDevice_info_get_by_stream_type(pGBLogicDeviceInfo, iStreamType);

    if (NULL == pGBDeviceInfo)
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_ERROR, "tsu_no_stream_msg_proc() exit---: Get GBDevice Info Error:StreamType=%d \r\n", iStreamType);
        return -1;
    }

    SystemLog(EV9000_CMS_TSU_NOTIFY_NOSTREAM, EV9000_LOG_LEVEL_ERROR, "TSU֪ͨǰ��û������:�߼��豸ID=%s, ý��������=%d, IP��ַ=%s, ֪ͨ����=%d", pcDeviceID, iStreamType, pGBDeviceInfo->login_ip, pGBLogicDeviceInfo->no_stream_count);
    EnSystemLog(EV9000_CMS_TSU_NOTIFY_NOSTREAM, EV9000_LOG_LEVEL_ERROR, "TSU notify currently there is not stream: logic device ID=%s, media stream type=%d, IP address=%s, notify time=%d", pcDeviceID, iStreamType, pGBDeviceInfo->login_ip, pGBLogicDeviceInfo->no_stream_count);

    //DEBUG_TRACE(MODULE_RESOURCE, LOG_INFO, "tsu_no_stream_msg_proc() Enter------------: DeviceID=%s \r\n", pcDeviceID);
    /* ֹͣǰ�˵����� */
    if (pGBDeviceInfo->device_type == EV9000_DEVICETYPE_INTELLIGENTANALYSIS
        || pGBDeviceInfo->device_type == EV9000_DEVICETYPE_SIPSERVER)
    {
        iRet = StopAllServiceTaskByLogicDeviceIDAndStreamType(strDeviceID, iStreamType);

        if (0 != iRet)
        {
            DEBUG_TRACE(MODULE_RESOURCE, LOG_ERROR, "tsu_no_stream_msg_proc() StopAllServiceTaskByLogicDeviceIDAndStreamType Error:device_id=%s, StreamType=%d, iRet=%d \r\n", pGBLogicDeviceInfo->device_id, iStreamType, iRet);
        }
        else
        {
            DEBUG_TRACE(MODULE_RESOURCE, LOG_TRACE, "tsu_no_stream_msg_proc() StopAllServiceTaskByLogicDeviceIDAndStreamType OK:device_id=%s, StreamType=%d, iRet=%d \r\n", pGBLogicDeviceInfo->device_id, iStreamType, iRet);
        }
    }
    else
    {
        iRet = StopAllServiceTaskByLogicDeviceIDAndStreamType(strDeviceID, iStreamType);

        if (0 != iRet)
        {
            DEBUG_TRACE(MODULE_RESOURCE, LOG_ERROR, "tsu_no_stream_msg_proc() StopAllServiceTaskByLogicDeviceIDAndStreamType Error:device_id=%s, StreamType=%d, iRet=%d \r\n", pGBLogicDeviceInfo->device_id, iStreamType, iRet);
        }
        else
        {
            DEBUG_TRACE(MODULE_RESOURCE, LOG_TRACE, "tsu_no_stream_msg_proc() StopAllServiceTaskByLogicDeviceIDAndStreamType OK:device_id=%s, StreamType=%d, iRet=%d \r\n", pGBLogicDeviceInfo->device_id, iStreamType, iRet);
        }
    }

    if (pGBDeviceInfo->device_type == EV9000_DEVICETYPE_IPC)
    {
        if (pGBLogicDeviceInfo->no_stream_count > 10)
        {
            /* ͨ��ping���һ��ǰ���Ƿ����� */
            iRet = checkGBDeviceIsOnline(pGBDeviceInfo->login_ip);

            if (0 == iRet)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "TSU֪ͨǰ���豸û�������ﵽ10��, CMS����pingǰ���豸ʧ��, ����ע��ǰ���豸��¼:ǰ���豸ID=%s, IP��ַ=%s, �˿ں�=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "TSU notify currently there is not stream, ping error, front-end device log off:front-end device ID=%s, IP address=%s, Port number=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);

                SIP_UASRemoveRegisterInfo(pGBDeviceInfo->reg_info_index);

                iRet = GBDevice_reg_msg_add(pGBDeviceInfo->device_id, pGBDeviceInfo->device_type, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, NULL, 0, pGBDeviceInfo->reg_info_index, 0);

                if (iRet != 0)
                {
                    DEBUG_TRACE(MODULE_RESOURCE, LOG_ERROR, "tsu_no_stream_msg_proc() GBDevice_reg_msg_add Error:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                }
                else
                {
                    DEBUG_TRACE(MODULE_RESOURCE, LOG_INFO, "tsu_no_stream_msg_proc() GBDevice_reg_msg_add OK:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                }
            }

            pGBLogicDeviceInfo->no_stream_count = 0;
        }
    }

    //DEBUG_TRACE(MODULE_RESOURCE, LOG_INFO, "tsu_no_stream_msg_proc() Exit------------: iRet=%d \r\n", iRet);

    return iRet;
}
#endif

#if DECS("TSU ����������֪ͨ��Ϣ����")
/*****************************************************************************
 �� �� ��  : tsu_creat_task_result_msg_init
 ��������  : TSU������������Ϣ�ṹ��ʼ��
 �������  : tsu_creat_task_result_msg_t ** tsu_creat_task_result_msg
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��11��13��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int tsu_creat_task_result_msg_init(tsu_creat_task_result_msg_t** tsu_creat_task_result_msg)
{
    *tsu_creat_task_result_msg = (tsu_creat_task_result_msg_t*)osip_malloc(sizeof(tsu_creat_task_result_msg_t));

    if (*tsu_creat_task_result_msg == NULL)
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_DEBUG, "tsu_creat_task_result_msg_init() exit---: *tsu_creat_task_result_msg Smalloc Error \r\n");
        return -1;
    }

    (*tsu_creat_task_result_msg)->strZRVDeviceIP[0] = '\0';
    (*tsu_creat_task_result_msg)->pcTaskID[0] = '\0';
    (*tsu_creat_task_result_msg)->iResult = 0;
    (*tsu_creat_task_result_msg)->iErrCode = 0;
    (*tsu_creat_task_result_msg)->iCompressBeginTime = 0;
    (*tsu_creat_task_result_msg)->iCompressEndTime = 0;
    (*tsu_creat_task_result_msg)->iYSHFileSize = 0;
    memset((*tsu_creat_task_result_msg)->pcDestUrl, 0, 128);

    return 0;
}

/*****************************************************************************
 �� �� ��  : tsu_creat_task_result_msg_free
 ��������  : TSU������������Ϣ�ṹ�ͷ�
 �������  : tsu_creat_task_result_msg_t * tsu_creat_task_result_msg
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��11��13��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void tsu_creat_task_result_msg_free(tsu_creat_task_result_msg_t* tsu_creat_task_result_msg)
{
    if (tsu_creat_task_result_msg == NULL)
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_DEBUG, "tsu_creat_task_result_msg_free() exit---: Param Error \r\n");
        return;
    }

    memset(tsu_creat_task_result_msg->strZRVDeviceIP, 0, MAX_IP_LEN);
    memset(tsu_creat_task_result_msg->pcTaskID, 0, MAX_TSU_TASK_LEN + 4);
    tsu_creat_task_result_msg->iResult = 0;
    tsu_creat_task_result_msg->iErrCode = 0;
    tsu_creat_task_result_msg->iCompressBeginTime = 0;
    tsu_creat_task_result_msg->iCompressEndTime = 0;
    tsu_creat_task_result_msg->iYSHFileSize = 0;
    memset(tsu_creat_task_result_msg->pcDestUrl, 0, 128);

    return;
}

/*****************************************************************************
 �� �� ��  : tsu_creat_task_result_msg_list_init
 ��������  : TSU������������Ϣ���г�ʼ��
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��11��13��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int tsu_creat_task_result_msg_list_init()
{
    g_TSUCreatTaskResultMsgQueue.clear();

    return 0;
}

/*****************************************************************************
 �� �� ��  : tsu_creat_task_result_msg_list_free
 ��������  : TSU������������Ϣ�����ͷ�
 �������  :
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��11��13��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void tsu_creat_task_result_msg_list_free()
{
    tsu_creat_task_result_msg_t* pTsuCreatTaskResultMsg = NULL;

    while (!g_TSUCreatTaskResultMsgQueue.empty())
    {
        pTsuCreatTaskResultMsg = (tsu_creat_task_result_msg_t*) g_TSUCreatTaskResultMsgQueue.front();
        g_TSUCreatTaskResultMsgQueue.pop_front();

        if (NULL != pTsuCreatTaskResultMsg)
        {
            tsu_creat_task_result_msg_free(pTsuCreatTaskResultMsg);
            osip_free(pTsuCreatTaskResultMsg);
            pTsuCreatTaskResultMsg = NULL;
        }
    }

    g_TSUCreatTaskResultMsgQueue.clear();

    return;
}

/*****************************************************************************
 �� �� ��  : tsu_creat_task_result_msg_add
 ��������  : ���TSU������������Ϣ��������
 �������  : char* zrv_device_ip
             char* pcTaskID
             int iResult
             int iErrCode
             int iYSHFileSize
             char* pcDestUrl
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��11��13��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int tsu_creat_task_result_msg_add(char* zrv_device_ip, char* pcTaskID, int iResult, int iErrCode, int iCompressBeginTime, int iCompressEndTime, int iYSHFileSize, char* pcDestUrl)
{
    tsu_creat_task_result_msg_t* pTsuCreatTaskResultMsg = NULL;
    int iRet = 0;

    if (NULL == zrv_device_ip || zrv_device_ip[0] == '\0'
        || NULL == pcTaskID || pcTaskID[0] == '\0')
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_DEBUG, "tsu_creat_task_result_msg_add() exit---: Param Error \r\n");
        return -1;
    }

    iRet = tsu_creat_task_result_msg_init(&pTsuCreatTaskResultMsg);

    if (iRet != 0)
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_ERROR, "tsu_creat_task_result_msg_add() exit---: Message Init Error \r\n");
        return -1;
    }

    osip_strncpy(pTsuCreatTaskResultMsg->strZRVDeviceIP, zrv_device_ip, MAX_IP_LEN);
    osip_strncpy(pTsuCreatTaskResultMsg->pcTaskID, pcTaskID, MAX_TSU_TASK_LEN);

    pTsuCreatTaskResultMsg->iResult = iResult;
    pTsuCreatTaskResultMsg->iErrCode = iErrCode;

    if (iCompressBeginTime > 0)
    {
        pTsuCreatTaskResultMsg->iCompressBeginTime = iCompressBeginTime;
    }

    if (iCompressEndTime > 0)
    {
        pTsuCreatTaskResultMsg->iCompressEndTime = iCompressEndTime;
    }

    pTsuCreatTaskResultMsg->iYSHFileSize = iYSHFileSize;

    if (NULL != pcDestUrl)
    {
        osip_strncpy(pTsuCreatTaskResultMsg->pcDestUrl, pcDestUrl, 128);
    }

    g_TSUCreatTaskResultMsgQueue.push_back(pTsuCreatTaskResultMsg);

    return 0;
}

/*****************************************************************************
 �� �� ��  : scan_tsu_creat_task_result_msg_list
 ��������  : ɨ��TSU������������Ϣ����
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��11��13 �� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void scan_tsu_creat_task_result_msg_list(DBOper* pDbOper)
{
    int iRet = 0;
    tsu_creat_task_result_msg_t* pTsuCreatTaskResultMsg = NULL;

    while (!g_TSUCreatTaskResultMsgQueue.empty())
    {
        pTsuCreatTaskResultMsg = (tsu_creat_task_result_msg_t*) g_TSUCreatTaskResultMsgQueue.front();
        g_TSUCreatTaskResultMsgQueue.pop_front();

        if (NULL != pTsuCreatTaskResultMsg)
        {
            break;
        }
    }

    if (NULL != pTsuCreatTaskResultMsg)
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_INFO,  "scan_tsu_creat_task_result_msg_list() \
        \r\n ZRVDeviceIP=%s, TaskID=%s, Result=%d, ErrCode=%d, CompressBeginTime=%d, CompressEndTime=%d, YSHFileSize=%d, DestUrl=%s \r\n ", pTsuCreatTaskResultMsg->strZRVDeviceIP, pTsuCreatTaskResultMsg->pcTaskID, pTsuCreatTaskResultMsg->iResult, pTsuCreatTaskResultMsg->iErrCode, pTsuCreatTaskResultMsg->iCompressBeginTime, pTsuCreatTaskResultMsg->iCompressEndTime, pTsuCreatTaskResultMsg->iYSHFileSize, pTsuCreatTaskResultMsg->pcDestUrl);

        iRet = tsu_creat_task_result_msg_proc(pTsuCreatTaskResultMsg->strZRVDeviceIP, pTsuCreatTaskResultMsg->pcTaskID, pTsuCreatTaskResultMsg->iResult, pTsuCreatTaskResultMsg->iErrCode, pTsuCreatTaskResultMsg->iCompressBeginTime, pTsuCreatTaskResultMsg->iCompressEndTime, pTsuCreatTaskResultMsg->iYSHFileSize, pTsuCreatTaskResultMsg->pcDestUrl, pDbOper);

        tsu_creat_task_result_msg_free(pTsuCreatTaskResultMsg);
        osip_free(pTsuCreatTaskResultMsg);
        pTsuCreatTaskResultMsg = NULL;
    }

    return;
}

/*****************************************************************************
 �� �� ��  : tsu_creat_task_result_msg_proc
 ��������  : TSU������������Ϣ����
 �������  : char* pcZRVDeviceIP
             char* pcTaskID
             int iResult
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��11��13��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int tsu_creat_task_result_msg_proc(char* pcZRVDeviceIP, char* pcTaskID, int iResult, int iErrCode, int iCompressBeginTime, int iCompressEndTime, int iYSHFileSize, char* pcDestUrl, DBOper* pDbOper)
{
    int iRet = 0;

    if (NULL == pcTaskID)
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_DEBUG, "tsu_creat_task_result_msg_proc() exit---: Param Error \r\n");
        return -1;
    }

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "ZRV�豸֪ͨѹ����������Ϣ��ʼ����:ZRVDeviceIP=%s, RecordNum=%s, Result=%d, CompressBeginTime=%d, CompressEndTime=%d, YSHFileSize=%d, ErrorCode=%d, DestUrl=%s", pcZRVDeviceIP, pcTaskID, iResult, iErrCode, iCompressBeginTime, iCompressEndTime, iYSHFileSize, pcDestUrl);

    /* ���·�����Ϣ */
    iRet = UpdateCompressTaskAssignInfo(pcTaskID, 1, pcZRVDeviceIP, pDbOper);
    DEBUG_TRACE(MODULE_RESOURCE, LOG_INFO, "tsu_creat_task_result_msg_proc() UpdateCompressTaskAssignInfo:CompressTaskID=%s, device_ip=%s, iRet=%d\r\n", pcTaskID, pcZRVDeviceIP, iRet);

    /* ����״̬ */
    iRet = UpdateCompressTaskResultInfo(pcTaskID, 2, iResult, iErrCode, iCompressBeginTime, iCompressEndTime, iYSHFileSize, pcDestUrl, pDbOper);
    DEBUG_TRACE(MODULE_RESOURCE, LOG_INFO, "tsu_creat_task_result_msg_proc() UpdateCompressTaskResultInfo:CompressTaskID=%s, device_ip=%s, iRet=%d\r\n", pcTaskID, pcZRVDeviceIP, iRet);

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_IMPORTANT, "ZRV�豸֪ͨѹ����������Ϣ�������:ZRVDeviceIP=%s, RecordNum=%s, Result=%d, iRet=%d", pcZRVDeviceIP, pcTaskID, iResult, iRet);

    return iRet;
}
#endif

#if DECS("TSU �澯��Ϣ����")
/*****************************************************************************
 �� �� ��  : tsu_alarm_msg_init
 ��������  : TSU������Ϣ�ṹ��ʼ��
 �������  : tsu_alarm_msg_t ** tsu_alarm_msg
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��11��13��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int tsu_alarm_msg_init(tsu_alarm_msg_t** tsu_alarm_msg)
{
    *tsu_alarm_msg = (tsu_alarm_msg_t*)osip_malloc(sizeof(tsu_alarm_msg_t));

    if (*tsu_alarm_msg == NULL)
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_DEBUG, "tsu_alarm_msg_init() exit---: *tsu_alarm_msg Smalloc Error \r\n");
        return -1;
    }

    (*tsu_alarm_msg)->iTSUIndex = 0;
    (*tsu_alarm_msg)->iType = 0;
    (*tsu_alarm_msg)->iLevel = 0;
    (*tsu_alarm_msg)->iTime = 0;
    (*tsu_alarm_msg)->strInfo[0] = '\0';

    return 0;
}

/*****************************************************************************
 �� �� ��  : tsu_alarm_msg_free
 ��������  : TSU������Ϣ�ṹ�ͷ�
 �������  : tsu_alarm_msg_t * tsu_alarm_msg
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��11��13��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void tsu_alarm_msg_free(tsu_alarm_msg_t* tsu_alarm_msg)
{
    if (tsu_alarm_msg == NULL)
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_DEBUG, "tsu_alarm_msg_free() exit---: Param Error \r\n");
        return;
    }

    tsu_alarm_msg->iTSUIndex = 0;
    tsu_alarm_msg->iType = 0;
    tsu_alarm_msg->iLevel = 0;
    tsu_alarm_msg->iTime = 0;
    memset(tsu_alarm_msg->strInfo, 0, 128 + 4);

    return;
}

/*****************************************************************************
 �� �� ��  : tsu_alarm_msg_list_init
 ��������  : TSU������Ϣ���г�ʼ��
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��11��13��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int tsu_alarm_msg_list_init()
{
    g_TSUAlarmMsgQueue.clear();

    return 0;
}

/*****************************************************************************
 �� �� ��  : tsu_alarm_msg_list_free
 ��������  : TSU������Ϣ�����ͷ�
 �������  :
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��11��13��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void tsu_alarm_msg_list_free()
{
    tsu_alarm_msg_t* pTsuAlarmMsg = NULL;

    while (!g_TSUAlarmMsgQueue.empty())
    {
        pTsuAlarmMsg = (tsu_alarm_msg_t*) g_TSUAlarmMsgQueue.front();
        g_TSUAlarmMsgQueue.pop_front();

        if (NULL != pTsuAlarmMsg)
        {
            tsu_alarm_msg_free(pTsuAlarmMsg);
            osip_free(pTsuAlarmMsg);
            pTsuAlarmMsg = NULL;
        }
    }

    g_TSUAlarmMsgQueue.clear();

    return;
}

/*****************************************************************************
 �� �� ��  : tsu_alarm_msg_add
 ��������  : ���TSU������Ϣ��������
 �������  : int iTSUIndex
             int iType
             int iLevel
             int iTime
             char* pcInfo
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��11��13��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int tsu_alarm_msg_add(int iTSUIndex, int iType, int iLevel, int iTime, char* pcInfo)
{
    int iRet = 0;
    tsu_alarm_msg_t* pTsuAlarmMsg = NULL;

    iRet = tsu_alarm_msg_init(&pTsuAlarmMsg);

    if (iRet != 0)
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_ERROR, "tsu_alarm_msg_add() exit---: Message Init Error \r\n");
        return -1;
    }

    pTsuAlarmMsg->iTSUIndex = iTSUIndex;
    pTsuAlarmMsg->iType = iType;
    pTsuAlarmMsg->iLevel = iLevel;
    pTsuAlarmMsg->iTime = iTime;

    if (NULL != pcInfo)
    {
        osip_strncpy(pTsuAlarmMsg->strInfo, pcInfo, 128);
    }

    g_TSUAlarmMsgQueue.push_back(pTsuAlarmMsg);

    return 0;
}

/*****************************************************************************
 �� �� ��  : scan_tsu_alarm_msg_list
 ��������  : ɨ��TSU������Ϣ����
 �������  : DBOper* pDBoper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��11��13 �� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void scan_tsu_alarm_msg_list(thread_proc_t* run)
{
    int iRet = 0;
    tsu_alarm_msg_t* pTsuAlarmMsg = NULL;
    static int connect_interval = 0;

    if (NULL == run)
    {
        return;
    }

    if (NULL == run->pDbOper)
    {
        return;
    }

    if (!run->iLogDBOperConnectStatus)
    {
        connect_interval++;

        if (connect_interval >= 60 * 200)
        {
            if (run->pLogDbOper->Connect(g_StrConLog, (char*)"") < 0)
            {
                run->iLogDBOperConnectStatus = 0;
            }
            else
            {
                run->iLogDBOperConnectStatus = 1;
            }

            connect_interval = 0;
        }
    }

    while (!g_TSUAlarmMsgQueue.empty())
    {
        pTsuAlarmMsg = (tsu_alarm_msg_t*) g_TSUAlarmMsgQueue.front();
        g_TSUAlarmMsgQueue.pop_front();

        if (NULL != pTsuAlarmMsg)
        {
            break;
        }
    }

    if (NULL != pTsuAlarmMsg)
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_INFO,  "scan_tsu_alarm_msg_list() \
            \r\n TSUAlarmMsg.iTSUIndex=%d \
            \r\n TSUAlarmMsg.iType=%d \
            \r\n TSUAlarmMsg.iLevel=%d \
            \r\n TSUAlarmMsg.iTime=%d \
            \r\n TSUAlarmMsg.strInfo=%s \
            \r\n ", pTsuAlarmMsg->iTSUIndex, pTsuAlarmMsg->iType, pTsuAlarmMsg->iLevel, pTsuAlarmMsg->iTime, pTsuAlarmMsg->strInfo);

        if (!run->iLogDBOperConnectStatus)
        {
            iRet = tsu_alarm_msg_proc(pTsuAlarmMsg, NULL);
        }
        else
        {
            iRet = tsu_alarm_msg_proc(pTsuAlarmMsg, run->pLogDbOper);
        }

        tsu_alarm_msg_free(pTsuAlarmMsg);
        osip_free(pTsuAlarmMsg);
        pTsuAlarmMsg = NULL;
    }

    return;
}

/*****************************************************************************
 �� �� ��  : tsu_alarm_msg_proc
 ��������  : TSU������Ϣ����
 �������  : tsu_alarm_msg_t* pTsuAlarmMsg
             DBOper* pDBoper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��11��13��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int tsu_alarm_msg_proc(tsu_alarm_msg_t* pTsuAlarmMsg, DBOper* pDBoper)
{
    int iRet = 0;
    CPacket outPacket;
    DOMElement* AccNode = NULL;
    char strTSUIndex[16] = {0};                  /* TSU���� */
    char strType[16] = {0};                      /* ������� */
    char strLevel[16] = {0};                     /* �������� */
    char strTime[16] = {0};                      /* ����ʱ�� */
    tsu_resource_info_t* pTsuResourceInfo = NULL;
    char strMsgSN[32] = {0};                     /* ���к� */
    string strNote = "";

    if (NULL == pTsuAlarmMsg)
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_DEBUG, "tsu_alarm_msg_proc() exit---: Param Error \r\n");
        return -1;
    }

    snprintf(strTSUIndex, 16, "%d", pTsuAlarmMsg->iTSUIndex);
    snprintf(strLevel, 16, "%d", pTsuAlarmMsg->iLevel);
    snprintf(strType, 16, "%d", pTsuAlarmMsg->iType);
    snprintf(strTime, 16, "%d", pTsuAlarmMsg->iTime);

    if (TSU_MOUNT_DISK_FAILED == pTsuAlarmMsg->iType)
    {
        strNote = "ϵͳ�յ�TSU�ı�����Ϣ:TSU����ID=";
        strNote += strTSUIndex;
        strNote += ", ��������=";
        strNote += strType;
        strNote += ", ��������=";
        strNote += strType;
        strNote += ", ������������=";
        strNote += pTsuAlarmMsg->strInfo;

        //iRet = shdb_system_operate_cmd_proc(EV9000_SHDB_DVR_DISK_ERROR, (char*)strNote.c_str());
    }

    /* ����������д�뱨����¼��*/
    iRet = write_tsu_alarm_to_db_proc(strTSUIndex, strType, strLevel, strTime, pTsuAlarmMsg->strInfo, pDBoper);

    if (iRet < 0)
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_ERROR, "tsu_alarm_msg_proc() write_tsu_alarm_to_db_proc Error:iRet=%d \r\n", iRet);
    }
    else
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_TRACE, "tsu_alarm_msg_proc() write_tsu_alarm_to_db_proc OK:iRet=%d \r\n", iRet);
    }

    /* ���ͱ������ݸ��ϼ����������û� */
    if (g_AlarmMsgSendToUserFlag || g_AlarmMsgSendToRouteFlag)
    {
        outPacket.SetRootTag("Notify");

        AccNode = outPacket.CreateElement((char*)"CmdType");
        outPacket.SetElementValue(AccNode, (char*)"Alarm");

        AccNode = outPacket.CreateElement((char*)"SN");
        uTSUAlarmMsgSn++;
        snprintf(strMsgSN, 32, "%u", uTSUAlarmMsgSn);
        outPacket.SetElementValue(AccNode, strMsgSN);

        AccNode = outPacket.CreateElement((char*)"DeviceID");
        pTsuResourceInfo = tsu_resource_info_get_by_index(pTsuAlarmMsg->iTSUIndex);

        if (NULL != pTsuResourceInfo)
        {
            outPacket.SetElementValue(AccNode, pTsuResourceInfo->tsu_device_id);
        }
        else
        {
            outPacket.SetElementValue(AccNode, (char*)"");
        }

        AccNode = outPacket.CreateElement((char*)"AlarmPriority");
        outPacket.SetElementValue(AccNode, strLevel);

        AccNode = outPacket.CreateElement((char*)"AlarmMethod");
        outPacket.SetElementValue(AccNode, strType);

        AccNode = outPacket.CreateElement((char*)"AlarmTime");
        outPacket.SetElementValue(AccNode, strTime);

        AccNode = outPacket.CreateElement((char*)"AlarmDescription");
        outPacket.SetElementValue(AccNode, pTsuAlarmMsg->strInfo);

        if (g_AlarmMsgSendToUserFlag)
        {
            /* ���ͱ������ݸ��ͻ����û� */
            iRet = send_alarm_to_user_proc(outPacket);

            if (iRet < 0)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "����TSU�澯��Ϣ�������û�ʧ��");
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "TSU alarm messages sent to the online user failed");
                DEBUG_TRACE(MODULE_RESOURCE, LOG_ERROR, "tsu_alarm_msg_proc() send_alarm_to_user_proc Error\r\n");
            }
            else if (iRet > 0)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "����TSU�澯��Ϣ�������û��ɹ�");
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "TSU alarm messages sent to the online user successfully");
                DEBUG_TRACE(MODULE_RESOURCE, LOG_TRACE, "tsu_alarm_msg_proc() send_alarm_to_user_proc OK\r\n");
            }
        }

        if (g_AlarmMsgSendToRouteFlag)
        {
            /* ���͸��ϼ�CMS */
            iRet = SendMessageToOwnerRouteCMSExceptMMS(NULL, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

            if (iRet < 0)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "����TSU�澯��Ϣ�������ϼ�ʧ��");
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "TSU alarm messages sent to the online user failed");
                DEBUG_TRACE(MODULE_RESOURCE, LOG_ERROR, "tsu_alarm_msg_proc() send_alarm_to_user_proc Error\r\n");
            }
            else if (iRet > 0)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "����TSU�澯��Ϣ�������ϼ��ɹ�");
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "TSU alarm messages sent to the online user successfully");
                DEBUG_TRACE(MODULE_RESOURCE, LOG_TRACE, "tsu_alarm_msg_proc() send_alarm_to_user_proc OK\r\n");
            }
        }
    }

    return iRet;
}
#endif

#if DECS("��Դ��Ϣ����")
/*****************************************************************************
 �� �� ��  : tsu_record_info_lock
 ��������  : TSU¼����Ϣ����
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��6��7�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int tsu_record_info_lock(tsu_resource_info_t* tsu_resource_info)
{
    int iRet = 0;

    if ((tsu_resource_info == NULL) || (0 == tsu_resource_info->iUsed))
    {
        //DEBUG_TRACE(MODULE_RESOURCE, LOG_DEBUG, "tsu_record_info_lock() exit---: Param Error \r\n");
        return -1;
    }

#ifdef MULTI_THR

    if (NULL == tsu_resource_info->pRecordInfoListLock)
    {
        //DEBUG_TRACE(MODULE_RESOURCE, LOG_DEBUG, "tsu_record_info_lock() exit---: Record Info List Lock Error \r\n");
        return -1;
    }

    iRet = osip_mutex_lock((struct osip_mutex*)tsu_resource_info->pRecordInfoListLock);

#endif
    return iRet;
}

/*****************************************************************************
 �� �� ��  : tsu_record_info_unlock
 ��������  : TSU ¼����Ϣ����
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��6��7�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int tsu_record_info_unlock(tsu_resource_info_t* tsu_resource_info)
{
    int iRet = 0;

    if ((tsu_resource_info == NULL) || (0 == tsu_resource_info->iUsed))
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_DEBUG, "tsu_record_info_unlock() exit---: Param Error \r\n");
        return -1;
    }

#ifdef MULTI_THR

    if (NULL == tsu_resource_info->pRecordInfoListLock)
    {
        //DEBUG_TRACE(MODULE_RESOURCE, LOG_DEBUG, "tsu_record_info_unlock() exit---: Record Info List Lock Error \r\n");
        return -1;
    }

    iRet = osip_mutex_unlock((struct osip_mutex*)tsu_resource_info->pRecordInfoListLock);

#endif
    return iRet;
}

/*****************************************************************************
 �� �� ��  : debug_tsu_record_info_lock
 ��������  : TSU ¼����Ϣ����
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��6��7�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int debug_tsu_record_info_lock(tsu_resource_info_t* tsu_resource_info, const char* file, int line, const char* func)
{
    int iRet = 0;

    if ((tsu_resource_info == NULL) || (0 == tsu_resource_info->iUsed))
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_DEBUG, "debug_tsu_record_info_lock() exit---: Param Error \r\n");
        return -1;
    }

#ifdef MULTI_THR

    if (NULL == tsu_resource_info->pRecordInfoListLock)
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_DEBUG, "debug_transfer_info_lock() exit---: Record Info List Lock Error \r\n");
        return -1;
    }

    iRet = osip_debug_mutex_lock((struct osip_mutex*)tsu_resource_info->pRecordInfoListLock, file, line, func);

    iTSURecordInfoLockCount++;

    if (0 != iRet)
    {
        printf("\r\n**********%s:%d:%s:debug_tsu_record_info_lock:iRet=%d**********\r\n", file, line, func, iRet);
        /* fprintf(stdout, "\r\n%s:%d:%s:osip_debug_mutex_lock:iRet=%d\r\n", file, line, func, iRet); */
        /* fflush(stdout); */
    }
    else
    {
        if (iTSURecordInfoLockCount != iTSURecordInfoUnLockCount + 1)
        {
            //printf("\r\n**********%s:%d:%s:debug_tsu_record_info_lock:iRet=%d, iTSURecordInfoLockCount=%lld**********\r\n", file, line, func, iRet, iTSURecordInfoLockCount);
        }
        else
        {
            //printf("\r\n%s:%d:%s:debug_tsu_record_info_lock:iRet=%d, iTSURecordInfoLockCount=%lld", file, line, func, iRet, iTSURecordInfoLockCount);
        }
    }

#endif
    return iRet;
}

/*****************************************************************************
 �� �� ��  : debug_tsu_record_info_unlock
 ��������  : TSU ¼����Ϣ����
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��6��7�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int debug_tsu_record_info_unlock(tsu_resource_info_t* tsu_resource_info, const char* file, int line, const char* func)
{
    int iRet = 0;

    if ((tsu_resource_info == NULL) || (0 == tsu_resource_info->iUsed))
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_DEBUG, "debug_tsu_record_info_unlock() exit---: Param Error \r\n");
        return -1;
    }

#ifdef MULTI_THR

    if (NULL == tsu_resource_info->pRecordInfoListLock)
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_DEBUG, "debug_tsu_record_info_unlock() exit---: Record Info List Lock Error \r\n");
        return -1;
    }

    iRet = osip_debug_mutex_unlock((struct osip_mutex*)tsu_resource_info->pRecordInfoListLock, file, line, func);

    iTSURecordInfoUnLockCount++;

    if (0 != iRet)
    {
        printf("\r\n**********%s:%d:%s:debug_tsu_record_info_unlock:iRet=%d**********\r\n", file, line, func, iRet);
        /* fprintf(stdout, "\r\n%s:%d:%s:osip_debug_mutex_lock:iRet=%d\r\n", file, line, func, iRet); */
        /* fflush(stdout); */
    }
    else
    {
        if (iTSURecordInfoLockCount != iTSURecordInfoUnLockCount)
        {
            //printf("\r\n**********%s:%d:%s:debug_tsu_record_info_unlock:iRet=%d, iTSURecordInfoUnLockCount=%lld**********\r\n", file, line, func, iRet, iTSURecordInfoUnLockCount);
        }
        else
        {
            //printf("\r\n%s:%d:%s:debug_tsu_record_info_unlock:iRet=%d, iTSURecordInfoUnLockCount=%lld", file, line, func, iRet, iTSURecordInfoUnLockCount);
        }
    }

#endif
    return iRet;
}

/*****************************************************************************
 �� �� ��  : tsu_resource_info_init
 ��������  : ��Դ��Ϣ�ṹ��ʼ��
 �������  : tsu_resource_info_t ** tsu_resource_info
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��11��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int tsu_resource_info_init(tsu_resource_info_t** tsu_resource_info)
{
    *tsu_resource_info = new tsu_resource_info_t;

    if (*tsu_resource_info == NULL)
    {
        //DEBUG_TRACE(MODULE_RESOURCE, LOG_DEBUG, "tsu_resource_info_init() exit---: *tsu_resource_info Smalloc Error \r\n");
        return -1;
    }

    (*tsu_resource_info)->iUsed = 0;
    (*tsu_resource_info)->iTsuIndex = 0;
    (*tsu_resource_info)->iTsuType = 0;
    (*tsu_resource_info)->tsu_device_id[0] = '\0';
    (*tsu_resource_info)->iStatus = 0;
    (*tsu_resource_info)->iExpires = 0;
    (*tsu_resource_info)->iAudioStatus = 0;
    (*tsu_resource_info)->iAudioExpires = 0;
    (*tsu_resource_info)->iUpLoadFlag = 0;

    /* TSU IP��ַ���г�ʼ�� */
    (*tsu_resource_info)->pTSUIPAddrList = (osip_list_t*)osip_malloc(sizeof(osip_list_t));

    if (NULL == (*tsu_resource_info)->pTSUIPAddrList)
    {
        osip_free(*tsu_resource_info);
        (*tsu_resource_info) = NULL;
        //DEBUG_TRACE(MODULE_RESOURCE, LOG_DEBUG, "tsu_resource_info_init() exit---: TSU IP Addr List Init Error \r\n");
        return -1;
    }

    osip_list_init((*tsu_resource_info)->pTSUIPAddrList);

    /* ¼����Ϣ���г�ʼ�� */
    (*tsu_resource_info)->pRecordInfoList = (osip_list_t*)osip_malloc(sizeof(osip_list_t));

    if (NULL == (*tsu_resource_info)->pRecordInfoList)
    {
        osip_free((*tsu_resource_info)->pTSUIPAddrList);
        (*tsu_resource_info)->pTSUIPAddrList = NULL;

        osip_free(*tsu_resource_info);
        (*tsu_resource_info) = NULL;
        //DEBUG_TRACE(MODULE_RESOURCE, LOG_DEBUG, "tsu_resource_info_init() exit---: Record Info List Init Error \r\n");
        return -1;
    }

    osip_list_init((*tsu_resource_info)->pRecordInfoList);

#ifdef MULTI_THR
    (*tsu_resource_info)->pRecordInfoListLock = (osip_mutex_t*)osip_mutex_init();

    if (NULL == (*tsu_resource_info)->pRecordInfoListLock)
    {
        osip_free((*tsu_resource_info)->pTSUIPAddrList);
        (*tsu_resource_info)->pTSUIPAddrList = NULL;

        osip_free((*tsu_resource_info)->pRecordInfoList);
        (*tsu_resource_info)->pRecordInfoList = NULL;

        osip_free(*tsu_resource_info);
        (*tsu_resource_info) = NULL;
        //DEBUG_TRACE(MODULE_RESOURCE, LOG_DEBUG, "tsu_resource_info_init() exit---: Record Info List Lock Init Error \r\n");
        return -1;
    }

#endif

    return 0;
}

/*****************************************************************************
 �� �� ��  : tsu_resource_info_free
 ��������  : ��Դ��Ϣ�ṹ�ͷ�
 �������  : tsu_resource_info_t * tsu_resource_info
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��11��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void tsu_resource_info_free(tsu_resource_info_t* tsu_resource_info)
{
    if (tsu_resource_info == NULL)
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_DEBUG, "tsu_resource_info_free() exit---: Param Error \r\n");
        return;
    }

    tsu_resource_info->iUsed = 0;
    tsu_resource_info->iTsuIndex = 0;
    tsu_resource_info->iTsuType = 0;

    memset(tsu_resource_info->tsu_device_id, 0, MAX_ID_LEN + 4);

    tsu_resource_info->iStatus = 0;
    tsu_resource_info->iExpires = 0;
    tsu_resource_info->iAudioStatus = 0;
    tsu_resource_info->iAudioExpires = 0;

    tsu_resource_info->iUpLoadFlag = 0;

    if (NULL != tsu_resource_info->pTSUIPAddrList)
    {
        osip_list_special_free(tsu_resource_info->pTSUIPAddrList, (void (*)(void*))&ip_pair_free);
        osip_free(tsu_resource_info->pTSUIPAddrList);
        tsu_resource_info->pTSUIPAddrList = NULL;
    }

    if (NULL != tsu_resource_info->pRecordInfoList)
    {
        while (!osip_list_eol(tsu_resource_info->pRecordInfoList, 0))
        {
            osip_list_remove(tsu_resource_info->pRecordInfoList, 0);
        }

        osip_free(tsu_resource_info->pRecordInfoList);
        tsu_resource_info->pRecordInfoList = NULL;
    }

#ifdef MULTI_THR

    if (NULL != tsu_resource_info->pRecordInfoListLock)
    {
        osip_mutex_destroy((struct osip_mutex*)tsu_resource_info->pRecordInfoListLock);
        tsu_resource_info->pRecordInfoListLock = NULL;
    }

#endif

    return;
}

/*****************************************************************************
 �� �� ��  : tsu_resource_info_list_init
 ��������  : ��Դ��Ϣ���г�ʼ��
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
int tsu_resource_info_list_init()
{
    g_TSUResourceInfoMap.clear();

#ifdef MULTI_THR
    g_TSUResourceInfoMapLock = (osip_mutex_t*)osip_mutex_init();

    if (NULL == g_TSUResourceInfoMapLock)
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_DEBUG, "tsu_resource_info_list_init() exit---: TSU Resource Info Map Lock Init Error \r\n");
        return -1;
    }

#endif
    return 0;
}

/*****************************************************************************
 �� �� ��  : tsu_resource_info_list_free
 ��������  : ��Դ��Ϣ�����ͷ�
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
void tsu_resource_info_list_free()
{
    TSU_Resource_Info_Iterator Itr;
    tsu_resource_info_t* pTsuResourceInfo = NULL;

    for (Itr = g_TSUResourceInfoMap.begin(); Itr != g_TSUResourceInfoMap.end(); Itr++)
    {
        pTsuResourceInfo = Itr->second;

        if (NULL != pTsuResourceInfo)
        {
            if (1 == pTsuResourceInfo->iUsed)
            {
                tsu_resource_info_free(pTsuResourceInfo);
            }

            delete pTsuResourceInfo;
            pTsuResourceInfo = NULL;
        }
    }

    g_TSUResourceInfoMap.clear();

#ifdef MULTI_THR

    if (NULL != g_TSUResourceInfoMapLock)
    {
        osip_mutex_destroy((struct osip_mutex*)g_TSUResourceInfoMapLock);
        g_TSUResourceInfoMapLock = NULL;
    }

#endif
    return;
}

/*****************************************************************************
 �� �� ��  : tsu_resource_info_list_lock
 ��������  : ��Դ��Ϣ��������
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
int tsu_resource_info_list_lock()
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_TSUResourceInfoMapLock == NULL)
    {
        //DEBUG_TRACE(MODULE_RESOURCE, LOG_DEBUG, "tsu_resource_info_list_lock() exit---: Param Error \r\n");
        return -1;
    }

    iRet = osip_mutex_lock((struct osip_mutex*)g_TSUResourceInfoMapLock);

#endif
    return iRet;
}

/*****************************************************************************
 �� �� ��  : tsu_resource_info_list_unlock
 ��������  : ��Դ��Ϣ���н���
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
int tsu_resource_info_list_unlock()
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_TSUResourceInfoMapLock == NULL)
    {
        //DEBUG_TRACE(MODULE_RESOURCE, LOG_DEBUG, "tsu_resource_info_list_unlock() exit---: Param Error \r\n");
        return -1;
    }

    iRet = osip_mutex_unlock((struct osip_mutex*)g_TSUResourceInfoMapLock);

#endif
    return iRet;
}

/*****************************************************************************
 �� �� ��  : tsu_resource_info_list_lock
 ��������  : ��Դ��Ϣ��������
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
int debug_tsu_resource_info_list_lock(const char* file, int line, const char* func)
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_TSUResourceInfoMapLock == NULL)
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_DEBUG, "debug_tsu_resource_info_list_lock() exit---: Param Error \r\n");
        return -1;
    }

    iRet = osip_debug_mutex_lock((struct osip_mutex*)g_TSUResourceInfoMapLock, file, line, func);

    iTSUInfoLockCount++;

    if (0 != iRet)
    {
        printf("\r\n**********%s:%d:%s:debug_tsu_resource_info_list_lock:iRet=%d**********\r\n", file, line, func, iRet);
        /* fprintf(stdout, "\r\n%s:%d:%s:osip_debug_mutex_lock:iRet=%d\r\n", file, line, func, iRet); */
        /* fflush(stdout); */
    }
    else
    {
        if (iTSUInfoLockCount != iTSUInfoUnLockCount + 1)
        {
            //printf("\r\n**********%s:%d:%s:debug_tsu_resource_info_list_lock:iRet=%d, iTSUInfoLockCount=%lld**********\r\n", file, line, func, iRet, iTSUInfoLockCount);
        }
        else
        {
            //printf("\r\n%s:%d:%s:debug_tsu_resource_info_list_lock:iRet=%d, iTSUInfoLockCount=%lld", file, line, func, iRet, iTSUInfoLockCount);
        }
    }

#endif
    return iRet;
}

/*****************************************************************************
 �� �� ��  : tsu_resource_info_list_unlock
 ��������  : ��Դ��Ϣ���н���
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
int debug_tsu_resource_info_list_unlock(const char* file, int line, const char* func)
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_TSUResourceInfoMapLock == NULL)
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_DEBUG, "debug_tsu_resource_info_list_unlock() exit---: Param Error \r\n");
        return -1;
    }

    iRet = osip_debug_mutex_unlock((struct osip_mutex*)g_TSUResourceInfoMapLock, file, line, func);

    iTSUInfoUnLockCount++;

    if (0 != iRet)
    {
        printf("\r\n**********%s:%d:%s:debug_tsu_resource_info_list_unlock:iRet=%d**********\r\n", file, line, func, iRet);
        /* fprintf(stdout, "\r\n%s:%d:%s:osip_debug_mutex_lock:iRet=%d\r\n", file, line, func, iRet); */
        /* fflush(stdout); */
    }
    else
    {
        if (iTSUInfoLockCount != iTSUInfoUnLockCount)
        {
            //printf("\r\n**********%s:%d:%s:debug_tsu_resource_info_list_unlock:iRet=%d, iTSUInfoUnLockCount=%lld**********\r\n", file, line, func, iRet, iTSUInfoUnLockCount);
        }
        else
        {
            //printf("\r\n%s:%d:%s:debug_tsu_resource_info_list_unlock:iRet=%d, iTSUInfoUnLockCount=%lld", file, line, func, iRet, iTSUInfoUnLockCount);
        }
    }

#endif
    return iRet;
}

/*****************************************************************************
 �� �� ��  : tsu_resource_info_add
 ��������  : �����Դ��Ϣ��������
 �������  : char* tsu_resource_id
             int iTsuIndex
             int iTsuType
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��11��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int tsu_resource_info_add(char* tsu_device_id, int iTsuIndex, int iTsuType)
{
    tsu_resource_info_t* pTsuResourceInfo = NULL;
    int i = 0;
    int pos = -1;
    TSU_Resource_Info_Iterator Itr;

    if (tsu_device_id == NULL)
    {
        //DEBUG_TRACE(MODULE_RESOURCE, LOG_DEBUG, "tsu_resource_info_add() exit---: Param Error \r\n");
        return -1;
    }

    TSU_SMUTEX_LOCK();

    for (Itr = g_TSUResourceInfoMap.begin(); Itr != g_TSUResourceInfoMap.end(); Itr++)
    {
        pTsuResourceInfo = Itr->second;

        if (0 == pTsuResourceInfo->iUsed)
        {
            /* �ҵ����е�λ�� */
            pTsuResourceInfo->iUsed = 1;
            pTsuResourceInfo->iUpLoadFlag = g_TSUUpLoadFileFlag;
            pTsuResourceInfo->iTsuType = iTsuType;
            osip_strncpy(pTsuResourceInfo->tsu_device_id, tsu_device_id, MAX_ID_LEN);
            pTsuResourceInfo->iTsuIndex = iTsuIndex;
            //DEBUG_TRACE(MODULE_RESOURCE, LOG_INFO, "tsu_resource_info_add() exit---: pos=%d \r\n", Itr->first);
            TSU_SMUTEX_UNLOCK();
            return Itr->first;
        }
    }

    /* û�п��е�λ�� */
    i = tsu_resource_info_init(&pTsuResourceInfo);

    if (i != 0)
    {
        TSU_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_RESOURCE, LOG_ERROR, "tsu_resource_info_add() exit---: TSU Resource Info Init Error \r\n");
        return -1;
    }

    pTsuResourceInfo->iUsed = 1;
    pTsuResourceInfo->iUpLoadFlag = g_TSUUpLoadFileFlag;
    pTsuResourceInfo->iTsuType = iTsuType;
    osip_strncpy(pTsuResourceInfo->tsu_device_id, tsu_device_id, MAX_ID_LEN);
    pTsuResourceInfo->iTsuIndex = iTsuIndex;

    pos = g_TSUResourceInfoMap.size();
    g_TSUResourceInfoMap[pos] = pTsuResourceInfo;

    //DEBUG_TRACE(MODULE_RESOURCE, LOG_INFO, "tsu_resource_info_add() exit---: pos=%d \r\n", pos);
    TSU_SMUTEX_UNLOCK();
    return pos;
}

/*****************************************************************************
 �� �� ��  : tsu_resource_info_find
 ��������  : TSU��Դ��Ϣ����
 �������  : char* tsu_id
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��6��6��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int tsu_resource_info_find(char* tsu_id)
{
    tsu_resource_info_t* pTsuResourceInfo = NULL;
    TSU_Resource_Info_Iterator Itr;

    if (NULL == tsu_id)
    {
        //DEBUG_TRACE(MODULE_RESOURCE, LOG_DEBUG, "tsu_resource_info_find() exit---: Param Error \r\n");
        return -1;
    }

    TSU_SMUTEX_LOCK();

    if (g_TSUResourceInfoMap.size() <= 0)
    {
        TSU_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_RESOURCE, LOG_DEBUG, "tsu_resource_info_find() exit---: TSU Resource Info Map NULL \r\n");
        return -1;
    }

    for (Itr = g_TSUResourceInfoMap.begin(); Itr != g_TSUResourceInfoMap.end(); Itr++)
    {
        pTsuResourceInfo = Itr->second;

        if ((NULL == pTsuResourceInfo)
            || (0 == pTsuResourceInfo->iUsed)
            || (pTsuResourceInfo->tsu_device_id[0] == '\0'))
        {
            continue;
        }

        if (sstrcmp(pTsuResourceInfo->tsu_device_id , tsu_id) == 0)
        {
            TSU_SMUTEX_UNLOCK();
            return Itr->first;
        }
    }

    TSU_SMUTEX_UNLOCK();
    return -1;
}

/*****************************************************************************
 �� �� ��  : tsu_resource_info_find_by_index
 ��������  : ����TSU����������TSU
 �������  : int tsu_index
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��4��11�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int tsu_resource_info_find_by_index(int tsu_index)
{
    tsu_resource_info_t* pTsuResourceInfo = NULL;
    TSU_Resource_Info_Iterator Itr;

    int error_count = 0;
    int unused_count = 0;
    int offline_count = 0;
    int unassign_count = 0;

    if (tsu_index < 0)
    {
        //DEBUG_TRACE(MODULE_RESOURCE, LOG_DEBUG, "tsu_resource_info_find_by_index() exit---: Param Error \r\n");
        return -1;
    }

    TSU_SMUTEX_LOCK();

    if (g_TSUResourceInfoMap.size() <= 0)
    {
        TSU_SMUTEX_UNLOCK();
        return -2;
    }

    for (Itr = g_TSUResourceInfoMap.begin(); Itr != g_TSUResourceInfoMap.end(); Itr++)
    {
        pTsuResourceInfo = Itr->second;

        /* ����Ĺ��˵� */
        if ((NULL == pTsuResourceInfo)
            || (pTsuResourceInfo->tsu_device_id[0] == '\0'))
        {
            error_count++;
            continue;
        }

        /* δʹ�õĹ��˵� */
        if (0 == pTsuResourceInfo->iUsed)
        {
            unused_count++;
            continue;
        }

        /* ���ߵ�TSU���˵� */
        if (pTsuResourceInfo->iStatus == 0)
        {
            offline_count++;
            continue;
        }

        if (pTsuResourceInfo->iTsuIndex < 0)
        {
            unassign_count++;
            continue;
        }

        if (pTsuResourceInfo->iTsuIndex == tsu_index)
        {
            TSU_SMUTEX_UNLOCK();
            return Itr->first;
        }
    }

    TSU_SMUTEX_UNLOCK();

    if (g_TSUResourceInfoMap.size() == error_count)
    {
        return -3;
    }
    else if (g_TSUResourceInfoMap.size() == unused_count)
    {
        return -4;
    }
    else if (g_TSUResourceInfoMap.size() == offline_count)
    {
        return -5;
    }
    else if (g_TSUResourceInfoMap.size() == unassign_count)
    {
        return -10;
    }

    return -1;
}

/*****************************************************************************
 �� �� ��  : tsu_resource_info_get
 ��������  : ��ȡTSU��Դ��Ϣ
 �������  : int pos
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��6��6��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
tsu_resource_info_t* tsu_resource_info_get(int pos)
{
    tsu_resource_info_t* pTsuResourceInfo = NULL;

    if (!is_valid_tsu_resource_info_index(pos))
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_DEBUG, "tsu_resource_info_get() exit---: TSU Resource Info Index Error \r\n");
        return NULL;
    }

    pTsuResourceInfo = g_TSUResourceInfoMap[pos];

    if (0 == pTsuResourceInfo->iUsed)
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_ERROR, "tsu_resource_info_get() exit---: TSU Resource Info UnUsed:pos=%d \r\n", pos);
        return NULL;
    }

    return pTsuResourceInfo;
}

/*****************************************************************************
 �� �� ��  : tsu_resource_info_get_by_index
 ��������  : ����TSU������ȡTSU��Ϣ
 �������  : int tsu_index
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��9��4�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
tsu_resource_info_t* tsu_resource_info_get_by_index(int tsu_index)
{
    tsu_resource_info_t* pTsuResourceInfo = NULL;
    TSU_Resource_Info_Iterator Itr;

    if (tsu_index < 0)
    {
        //DEBUG_TRACE(MODULE_RESOURCE, LOG_DEBUG, "tsu_resource_info_get_by_index() exit---: Param Error \r\n");
        return NULL;
    }

    TSU_SMUTEX_LOCK();

    if (g_TSUResourceInfoMap.size() <= 0)
    {
        TSU_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_RESOURCE, LOG_DEBUG, "tsu_resource_info_find() exit---: TSU Resource Info Map NULL \r\n");
        return NULL;
    }

    for (Itr = g_TSUResourceInfoMap.begin(); Itr != g_TSUResourceInfoMap.end(); Itr++)
    {
        pTsuResourceInfo = Itr->second;

        if ((NULL == pTsuResourceInfo)
            || (0 == pTsuResourceInfo->iUsed)
            || (pTsuResourceInfo->iTsuIndex < 0))
        {
            continue;
        }

        if (pTsuResourceInfo->iTsuIndex == tsu_index)
        {
            TSU_SMUTEX_UNLOCK();
            return pTsuResourceInfo;
        }
    }

    TSU_SMUTEX_UNLOCK();
    return NULL;
}

/*****************************************************************************
 �� �� ��  : is_valid_tsu_resource_info_index
 ��������  : �Ƿ��ǺϷ���TSU��Դ��Ϣ
 �������  : int index
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��6��7�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int is_valid_tsu_resource_info_index(int index)
{
    if (index < 0 || index >= (int)g_TSUResourceInfoMap.size())
    {
        return 0;
    }

    return 1;
}

/*****************************************************************************
 �� �� ��  : scan_resource_info_list
 ��������  : ɨ��TSU ��Դ��Ϣ����
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��5��30��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void scan_resource_info_list(DBOper* pDboper)
{
    int iRet = 0;
    int index = -1;
    tsu_resource_info_t* pTsuResourceInfo = NULL;
    TSU_Resource_Info_Iterator Itr;
    vector<string> TSUIDVector;
    vector<string> AudioTSUIDVector;

    TSUIDVector.clear();
    AudioTSUIDVector.clear();

    string strNote = "";

    TSU_SMUTEX_LOCK();

    if (g_TSUResourceInfoMap.size() <= 0)
    {
        TSU_SMUTEX_UNLOCK();
        return;
    }

    for (Itr = g_TSUResourceInfoMap.begin(); Itr != g_TSUResourceInfoMap.end(); Itr++)
    {
        pTsuResourceInfo = Itr->second;

        if ((NULL == pTsuResourceInfo)
            || (0 == pTsuResourceInfo->iUsed)
            || (pTsuResourceInfo->tsu_device_id[0] == '\0'))
        {
            continue;
        }

        if (0 != pTsuResourceInfo->iStatus)
        {
            if (pTsuResourceInfo->iExpires > 0)
            {
                pTsuResourceInfo->iExpires--;
                //DEBUG_TRACE(MODULE_RESOURCE, LOG_INFO, "scan_resource_info_list() TSUID=%s,Expires=%d \r\n", pTsuResourceInfo->tsu_device_id, pTsuResourceInfo->iExpires);
            }

            if (pTsuResourceInfo->iExpires <= 0)
            {
                pTsuResourceInfo->iStatus = 0;

                TSUIDVector.push_back(pTsuResourceInfo->tsu_device_id);
                //SystemLog(EV9000_CMS_TSU_OFFLINE, EV9000_LOG_LEVEL_WARNING, "TSU��ʱ����:TSU ID=%s, ����=%d", pTsuResourceInfo->tsu_device_id, pTsuResourceInfo->iTsuIndex);
                //DEBUG_TRACE(MODULE_RESOURCE, LOG_WARN, "scan_resource_info_list() TSU ID=%s, Index=%d Off Line \r\n", pTsuResourceInfo->tsu_device_id, pTsuResourceInfo->iTsuIndex);

                break;
            }
        }

        if (0 != pTsuResourceInfo->iAudioStatus)
        {
            if (pTsuResourceInfo->iAudioExpires > 0)
            {
                pTsuResourceInfo->iAudioExpires--;
            }

            if (pTsuResourceInfo->iAudioExpires <= 0)
            {
                pTsuResourceInfo->iAudioStatus = 0;

                AudioTSUIDVector.push_back(pTsuResourceInfo->tsu_device_id);
                //SystemLog(EV9000_CMS_TSU_OFFLINE, EV9000_LOG_LEVEL_WARNING, "TSU��Ƶת��ҵ��ʱ����:TSU ID=%s, ����=%d", pTsuResourceInfo->tsu_device_id, pTsuResourceInfo->iTsuIndex);
                //EnSystemLog(EV9000_CMS_TSU_OFFLINE, EV9000_LOG_LEVEL_WARNING, "TSU voice forwarding task time out offline: TSU ID=%s, index=%d", pTsuResourceInfo->tsu_device_id, pTsuResourceInfo->iTsuIndex);
                //DEBUG_TRACE(MODULE_RESOURCE, LOG_WARN, "scan_resource_info_list() TSU ID=%s, Index=%d Audio Service Off Line \r\n", pTsuResourceInfo->tsu_device_id, pTsuResourceInfo->iTsuIndex);
                break;
            }
        }
    }

    TSU_SMUTEX_UNLOCK();

    if (TSUIDVector.size() > 0)
    {
        for (index = 0; index < (int)TSUIDVector.size(); index++)
        {
            /* TSU���ߣ���Ҫ�������ת������ͣ��,ת������TSU */
            /* ֹͣ¼������ֹ֮ͣ��¼������̻߳����·���¼�����񣬲���Ҫ�ֶ����� */
            /* ʵʱ��Ƶ����ֹ֮ͣ����Ҫ�û����·��� */
            SystemLog(EV9000_CMS_TSU_OFFLINE, EV9000_LOG_LEVEL_ERROR, "TSU��ʱ����:TSU ID=%s", (char*)TSUIDVector[index].c_str());
            EnSystemLog(EV9000_CMS_TSU_OFFLINE, EV9000_LOG_LEVEL_ERROR, "TSU timeout offline:TSU ID=%s", (char*)TSUIDVector[index].c_str());
            iRet = StopAllServiceTaskByTSUID((char*)TSUIDVector[index].c_str());

            if (0 != iRet)
            {
                DEBUG_TRACE(MODULE_RESOURCE, LOG_ERROR, "scan_resource_info_list() StopAllServiceTaskByTSUID Error: TSUIDVector=%s \r\n", (char*)TSUIDVector[index].c_str());
            }
            else
            {
                DEBUG_TRACE(MODULE_RESOURCE, LOG_TRACE, "scan_resource_info_list() StopAllServiceTaskByTSUID OK: TSUIDVector=%s \r\n", (char*)TSUIDVector[index].c_str());
            }

            /* ֪ͨ�����û���TSU ���� */
            iRet = SendTSUOffLineAlarmToAllClientUser(index);

            if (0 != iRet)
            {
                DEBUG_TRACE(MODULE_RESOURCE, LOG_ERROR, "scan_resource_info_list() SendTSUOffLineAlarmToAllClientUser Error: index=%d \r\n", index);
            }
            else
            {
                DEBUG_TRACE(MODULE_RESOURCE, LOG_TRACE, "scan_resource_info_list() SendTSUOffLineAlarmToAllClientUser OK: index=%d \r\n", index);
            }

            /* ���µ�������״̬ */
            iRet = UpdateBoardConfigTableStatus((char*)TSUIDVector[index].c_str(), LOGIC_BOARD_TSU, 0, pDboper);

            /* �������˽ṹ��״̬ */
            iRet = UpdateTopologyPhyDeviceStatus2DB((char*)TSUIDVector[index].c_str(), (char*)"0", pDboper);

            strNote = "ϵͳý��ת����Դ��ʱ����:ý��ת����ԴID=";
            strNote += TSUIDVector[index];

            //iRet = shdb_system_operate_cmd_proc(EV9000_SHDB_OTHER_VIDEO_EVENT, (char*)strNote.c_str());
        }
    }

    TSUIDVector.clear();

    /* ��Ƶת��ҵ�� */
    if (AudioTSUIDVector.size() > 0)
    {
        for (index = 0; index < (int)AudioTSUIDVector.size(); index++)
        {
            /* TSU��Ƶҵ�����ߣ���Ҫֹͣ�������Ƶת��ҵ�� */
            SystemLog(EV9000_CMS_TSU_OFFLINE, EV9000_LOG_LEVEL_ERROR, "TSU��Ƶת��ҵ��ʱ����:TSU ID=%s", (char*)AudioTSUIDVector[index].c_str());
            EnSystemLog(EV9000_CMS_TSU_OFFLINE, EV9000_LOG_LEVEL_ERROR, "TSU audio forwarding business offline overtime:TSU ID=%s", (char*)AudioTSUIDVector[index].c_str());
            iRet = StopAudioServiceTaskByTSUID((char*)AudioTSUIDVector[index].c_str());

            if (0 != iRet)
            {
                DEBUG_TRACE(MODULE_RESOURCE, LOG_ERROR, "scan_resource_info_list() StopAllServiceTaskByTSUID Error: TSUIDVector=%s \r\n", (char*)AudioTSUIDVector[index].c_str());
            }
            else
            {
                DEBUG_TRACE(MODULE_RESOURCE, LOG_TRACE, "scan_resource_info_list() StopAllServiceTaskByTSUID OK: TSUIDVector=%s \r\n", (char*)AudioTSUIDVector[index].c_str());
            }
        }
    }

    AudioTSUIDVector.clear();

    return;
}
#endif

/*****************************************************************************
 �� �� ��  : get_max_remain_disk_tsu
 ��������  : ��ȡ����ʣ����������TSU
 �������  : TsuStateInfo& stMaxTsuStateInfo
             int& iMaxRemain ���ʣ������
             int& iOldMaxRemain �ϵ����ʣ������
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��8��8�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int get_max_remain_disk_tsu(TsuStateInfo& stMaxTsuStateInfo, long& iMaxRemain, long iOldMaxRemain)
{
    int iRet = 0;
    int tsu_pos = -1;
    TSU_Resource_Info_Iterator Itr;

    unsigned long iM = 0; /* TSU�ļ�ʱ��Ҫ����= N * D * C */
    long iRemain = 0; /* ʣ�������= A-M */
    long iCurrentMaxRemain = 0; /* ��ǰ���ʣ�������= A-M */

    tsu_resource_info_t* pTsuResourceInfo = NULL;
    TsuStateInfo stTsuStateInfo;
    RecordStateInfoList stRecordStateInfoList;

    int error_count = 0;
    int unused_count = 0;
    int offline_count = 0;
    int assign_count = 0;
    int uplimit_count = 0;
    int getstate_count = 0;
    int nodisk_count = 0;

    if (g_TSUResourceInfoMap.size() <= 0)
    {
        return -2;
    }

    for (Itr = g_TSUResourceInfoMap.begin(); Itr != g_TSUResourceInfoMap.end(); Itr++)
    {
        pTsuResourceInfo = Itr->second;

        /* ����Ĺ��˵� */
        if ((NULL == pTsuResourceInfo)
            || (pTsuResourceInfo->tsu_device_id[0] == '\0'))
        {
            error_count++;
            continue;
        }

        /* δʹ�õĹ��˵� */
        if (0 == pTsuResourceInfo->iUsed)
        {
            unused_count++;
            continue;
        }

        /* ���ߵ�TSU���˵� */
        if (pTsuResourceInfo->iStatus == 0)
        {
            DEBUG_TRACE(MODULE_RESOURCE, LOG_ERROR, "get_max_remain_disk_tsu() get_tsu_state_info: TSU Status Off Line:tsu_id=%s \r\n", pTsuResourceInfo->tsu_device_id);
            offline_count++;
            continue;
        }

        /* û�������Ĺ��˵� */
        if (pTsuResourceInfo->iTsuIndex <= 0)
        {
            DEBUG_TRACE(MODULE_RESOURCE, LOG_ERROR, "get_max_remain_disk_tsu() get_tsu_state_info: TSU Index Error:tsu_id=%s \r\n", pTsuResourceInfo->tsu_device_id);
            offline_count++;
            continue;
        }

        /* ָ��¼���TSU���˵� */
        if (pTsuResourceInfo->iTsuType == 1)
        {
            DEBUG_TRACE(MODULE_RESOURCE, LOG_ERROR, "get_max_remain_disk_tsu() get_tsu_state_info: TSU For Assigned Record:tsu_id=%s \r\n", pTsuResourceInfo->tsu_device_id);
            assign_count++;
            continue;
        }

        /* �ﵽ���� */
        if (pTsuResourceInfo->iStatus == 2)
        {
            DEBUG_TRACE(MODULE_RESOURCE, LOG_ERROR, "get_max_remain_disk_tsu() get_tsu_state_info: TSU UP Limit:tsu_id=%s \r\n", pTsuResourceInfo->tsu_device_id);
            uplimit_count++;
            continue;
        }

        iRet = get_tsu_state_info(pTsuResourceInfo, stTsuStateInfo, stRecordStateInfoList);

        if (iRet < 0)
        {
            DEBUG_TRACE(MODULE_RESOURCE, LOG_ERROR, "get_max_remain_disk_tsu() get_tsu_state_info Error:tsu_id=%s \r\n", pTsuResourceInfo->tsu_device_id);
            getstate_count++;
            continue;
        }

        /* û�й��ش��� */
        if (stTsuStateInfo.DiskMaxSize <= 0)
        {
            DEBUG_TRACE(MODULE_RESOURCE, LOG_ERROR, "get_max_remain_disk_tsu() get_tsu_state_info: No Disk On TSU :tsu_id=%s \r\n", pTsuResourceInfo->tsu_device_id);
            pTsuResourceInfo->iStatus = 3;
            nodisk_count++;
            continue;
        }

        /* ���ش���֮��״̬�ָ� */
        if (pTsuResourceInfo->iStatus == 3)
        {
            pTsuResourceInfo->iStatus = 1;
        }

        /* TSU�ļ�ʱ��Ҫ���� M */
        iM = get_tsu_has_record_total_disk(pTsuResourceInfo);

        /* TSU ʣ������ */
        iRemain = stTsuStateInfo.DiskMaxSize - iM;

        DEBUG_TRACE(MODULE_RESOURCE, LOG_TRACE, "get_max_remain_disk_tsu() tsu_id=%s, iM=%lu, iRemain=%ld \r\n", pTsuResourceInfo->tsu_device_id, iM, iRemain);

        if (iRemain <= 0)
        {
            continue;
        }

        if (iOldMaxRemain > 0 && iRemain >= iOldMaxRemain) /* �ų��ɵ����ʣ������ */
        {
            continue;
        }

        /* ���ʣ���������ڵ�ǰ���������������µ�ǰ�������ѡ�� */
        if (iRemain > iCurrentMaxRemain)
        {
            iCurrentMaxRemain = iRemain;
            tsu_pos = Itr->first;

            stMaxTsuStateInfo = stTsuStateInfo;
            iMaxRemain = iCurrentMaxRemain;
        }
    }

    if (g_TSUResourceInfoMap.size() == error_count)
    {
        return -3;
    }
    else if (g_TSUResourceInfoMap.size() == unused_count)
    {
        return -4;
    }
    else if (g_TSUResourceInfoMap.size() == offline_count)
    {
        return -5;
    }
    else if (g_TSUResourceInfoMap.size() == assign_count)
    {
        return -6;
    }
    else if (g_TSUResourceInfoMap.size() == uplimit_count)
    {
        return -7;
    }
    else if (g_TSUResourceInfoMap.size() == nodisk_count)
    {
        return -8;
    }
    else if (g_TSUResourceInfoMap.size() == getstate_count)
    {
        return -9;
    }

    return tsu_pos;
}

/*****************************************************************************
 �� �� ��  : get_min_record_info_tsu
 ��������  : ��ȡ¼���λ���ٵ�TSU
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��1��13�� ���ڶ�
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int get_min_record_info_tsu()
{
    int min_tsu_pos = -1;
    int min_record_count = -1;
    int record_count = 0;

    TSU_Resource_Info_Iterator Itr;
    tsu_resource_info_t* pTsuResourceInfo = NULL;

    int error_count = 0;
    int unused_count = 0;
    int offline_count = 0;
    int assign_count = 0;
    int uplimit_count = 0;
    int nodisk_count = 0;

    if (g_TSUResourceInfoMap.size() <= 0)
    {
        return -2;
    }

    for (Itr = g_TSUResourceInfoMap.begin(); Itr != g_TSUResourceInfoMap.end(); Itr++)
    {
        pTsuResourceInfo = Itr->second;

        /* ����Ĺ��˵� */
        if ((NULL == pTsuResourceInfo)
            || (pTsuResourceInfo->tsu_device_id[0] == '\0'))
        {
            error_count++;
            continue;
        }

        /* δʹ�õĹ��˵� */
        if (0 == pTsuResourceInfo->iUsed)
        {
            unused_count++;
            continue;
        }

        /* ���ߵ�TSU���˵� */
        if (pTsuResourceInfo->iStatus == 0)
        {
            offline_count++;
            continue;
        }

        /* û�������Ĺ��˵� */
        if (pTsuResourceInfo->iTsuIndex <= 0)
        {
            offline_count++;
            continue;
        }

        /* ָ��¼���TSU���˵� */
        if (pTsuResourceInfo->iTsuType == 1)
        {
            assign_count++;
            continue;
        }

        /* �ﵽ���� */
        if (pTsuResourceInfo->iStatus == 2)
        {
            uplimit_count++;
            continue;
        }

        /* û�й��ش��� */
        if (pTsuResourceInfo->iStatus == 3)
        {
            nodisk_count++;
            continue;
        }

        /* ��ȡ��TSU�ϵ�¼���λ��Ϣ */
        TSU_RECORD_SMUTEX_LOCK(pTsuResourceInfo);

        record_count = osip_list_size(pTsuResourceInfo->pRecordInfoList);

        if (-1 == min_record_count) /* ����һ����ֵ, ȷ������ȡ��һ�����õ�TSU */
        {
            min_record_count = record_count;
            min_tsu_pos = Itr->first;
        }

        if (record_count < min_record_count) /*  */
        {
            min_record_count = record_count;
            min_tsu_pos = Itr->first;
        }

        TSU_RECORD_SMUTEX_UNLOCK(pTsuResourceInfo);
    }

    if (g_TSUResourceInfoMap.size() == error_count)
    {
        return -3;
    }
    else if (g_TSUResourceInfoMap.size() == unused_count)
    {
        return -4;
    }
    else if (g_TSUResourceInfoMap.size() == offline_count)
    {
        return -5;
    }
    else if (g_TSUResourceInfoMap.size() == assign_count)
    {
        return -6;
    }
    else if (g_TSUResourceInfoMap.size() == uplimit_count)
    {
        return -7;
    }
    else if (g_TSUResourceInfoMap.size() == nodisk_count)
    {
        return -8;
    }

    return min_tsu_pos;
}

/*****************************************************************************
 �� �� ��  : get_idle_tsu_by_resource_balance_for_record
 ��������  : ¼�������ʱ���ȡ���е�TSU��Դ
 �������  : int storagelife  ��ǰ���õ�����
                            int iBandwidth ��ǰ���õ�¼�����
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��8��8�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int get_idle_tsu_by_resource_balance_for_record(int storagelife, int iBandwidth)
{
    int tsu_pos = -1;
    long iOldMaxRemain = 0; /* ���ʣ�������= A-M */

    long iMaxRemain = 0; /* ���ʣ�������= A-M */

    TsuStateInfo stMaxTsuStateInfo;

    tsu_pos = get_max_remain_disk_tsu(stMaxTsuStateInfo, iMaxRemain, -1);

    DEBUG_TRACE(MODULE_RESOURCE, LOG_TRACE, "get_idle_tsu_by_resource_balance_for_record() get_max_remain_disk_tsu:tsu_pos=%d, iMaxRemain=%ld \r\n", tsu_pos, iMaxRemain);

    while (tsu_pos >= 0)
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_TRACE, "get_idle_tsu_by_resource_balance_for_record() TsuMaxBandwidth=%d, IMTsuStorageTotalBandwidth=%d, iBandwidth=%d \r\n", stMaxTsuStateInfo.TsuMaxBandwidth, stMaxTsuStateInfo.IMTsuStorageTotalBandwidth, iBandwidth);

        /* ��B��TSU���ܴ��� >  �� E(˲ʱ����) + D����ǰ��Ҫ���ȵ�¼�����������ǰ¼�����ڸ�TSU�� */
        //(iMaxRemain > ((storagelife * 3600 * 24 * iBandwidth)/1024))
        if ((stMaxTsuStateInfo.TsuMaxBandwidth > stMaxTsuStateInfo.IMTsuStorageTotalBandwidth + iBandwidth))
        {
            return tsu_pos;
        }
        else    /* ��B��TSU���ܴ��� <=  �� E(˲ʱ����) + D����ǰ��Ҫ���ȵ�¼���������ѡ��A-M��֮��TSU��һ�������� */
        {
            iOldMaxRemain = iMaxRemain;
            tsu_pos = get_max_remain_disk_tsu(stMaxTsuStateInfo, iMaxRemain, iOldMaxRemain);
            DEBUG_TRACE(MODULE_RESOURCE, LOG_INFO, "get_idle_tsu_by_resource_balance_for_record() get_max_remain_disk_tsu:tsu_pos=%d, iMaxRemain=%ld \r\n", tsu_pos, iMaxRemain);
        }
    }

    /* ���û�ҵ�TSU, ��һ��¼��·�����ٵ�TSU */
    tsu_pos = get_min_record_info_tsu();
    DEBUG_TRACE(MODULE_RESOURCE, LOG_INFO, "get_idle_tsu_by_resource_balance_for_record() get_min_record_info_tsu:tsu_pos=%d \r\n", tsu_pos);

    return tsu_pos;
}

/*****************************************************************************
 �� �� ��  : get_idle_tsu_by_resource_balance_for_transfer
 ��������  : ת�������ʱ���ȡ���е�TSU��Դ
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��8��8�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int get_idle_tsu_by_resource_balance_for_transfer()
{
    int iRet = 0;
    int tsu_pos = -1;
    TSU_Resource_Info_Iterator Itr;

    tsu_resource_info_t* pTsuResourceInfo = NULL;
    TsuStateInfo stTsuStateInfo;
    RecordStateInfoList stRecordStateInfoList;

    int iCurrentMinBandWidth = -1;
    int iBandWidth = -1;
    char strEthName1[16] = {0};
    char strEthName2[16] = {0};

    int error_count = 0;
    int unused_count = 0;
    int offline_count = 0;
    int getstate_count = 0;

    if (g_TSUResourceInfoMap.size() <= 0)
    {
        return -2;
    }

    for (Itr = g_TSUResourceInfoMap.begin(); Itr != g_TSUResourceInfoMap.end(); Itr++)
    {
        pTsuResourceInfo = Itr->second;

        /* ����Ĺ��˵� */
        if ((NULL == pTsuResourceInfo)
            || (pTsuResourceInfo->tsu_device_id[0] == '\0'))
        {
            error_count++;
            continue;
        }

        /* δʹ�õĹ��˵� */
        if (0 == pTsuResourceInfo->iUsed)
        {
            unused_count++;
            continue;
        }

        /* ���ߵ�TSU���˵� */
        if (pTsuResourceInfo->iStatus == 0)
        {
            offline_count++;
            continue;
        }

        iRet = get_tsu_state_info(pTsuResourceInfo, stTsuStateInfo, stRecordStateInfoList);

        if (iRet < 0)
        {
            getstate_count++;
            continue;
        }

        /* ���㵱ǰ�ܵĴ��� */
        iBandWidth = stTsuStateInfo.IMTsuRecvTotalBandwidth + stTsuStateInfo.IMTsuTransferTotalBandwidth;

        /* ��ȡ���ĵ�һ�����ݸ�ֵ */
        if (iCurrentMinBandWidth < 0)
        {
            iCurrentMinBandWidth = iBandWidth;
            tsu_pos = Itr->first;
        }

        /* �����ǰ�ܴ���С�ڵ�ǰ��С�Ĵ�������� */
        if (iBandWidth < iCurrentMinBandWidth)
        {
            iCurrentMinBandWidth = iBandWidth;
            tsu_pos = Itr->first;
        }
    }

    if (g_TSUResourceInfoMap.size() == error_count)
    {
        return -3;
    }
    else if (g_TSUResourceInfoMap.size() == unused_count)
    {
        return -4;
    }
    else if (g_TSUResourceInfoMap.size() == offline_count)
    {
        return -5;
    }
    else if (g_TSUResourceInfoMap.size() == getstate_count)
    {
        return -9;
    }

    return tsu_pos;
}

/*****************************************************************************
 �� �� ��  : get_idle_tsu_by_resource_balance_for_replay
 ��������  : ¼��ط������ʱ���ȡ���е�TSU��Դ
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��8��8�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int get_idle_tsu_by_resource_balance_for_replay()
{
    int iRet = 0;
    int tsu_pos = -1;
    TSU_Resource_Info_Iterator Itr;

    tsu_resource_info_t* pTsuResourceInfo = NULL;
    TsuStateInfo stTsuStateInfo;
    RecordStateInfoList stRecordStateInfoList;

    int iCurrentMinBandWidth = -1;
    int iBandWidth = -1;
    char strEthName1[16] = {0};
    char strEthName2[16] = {0};

    int error_count = 0;
    int unused_count = 0;
    int offline_count = 0;
    int getstate_count = 0;

    if (g_TSUResourceInfoMap.size() <= 0)
    {
        //DEBUG_TRACE(MODULE_RESOURCE, LOG_DEBUG, "get_idle_tsu_by_resource_balance_for_replay() exit---: TSU Resource Info Map NULL \r\n");
        return -1;
    }

    for (Itr = g_TSUResourceInfoMap.begin(); Itr != g_TSUResourceInfoMap.end(); Itr++)
    {
        pTsuResourceInfo = Itr->second;

        /* ����Ĺ��˵� */
        if ((NULL == pTsuResourceInfo)
            || (pTsuResourceInfo->tsu_device_id[0] == '\0'))
        {
            error_count++;
            continue;
        }

        /* δʹ�õĹ��˵� */
        if (0 == pTsuResourceInfo->iUsed)
        {
            unused_count++;
            continue;
        }

        /* ���ߵ�TSU���˵� */
        if (pTsuResourceInfo->iStatus == 0)
        {
            offline_count++;
            continue;
        }

        /* X86����SX������ʹ�ñ�����TSU */
        if (is_local_tsu(pTsuResourceInfo, local_ip_get(default_eth_name_get())))
        {
            tsu_pos = Itr->first;
            break;
        }

        iRet = get_tsu_state_info(pTsuResourceInfo, stTsuStateInfo, stRecordStateInfoList);

        if (iRet < 0)
        {
            getstate_count++;
            continue;
        }

        /* ���㵱ǰ�ܵĴ��� */
        iBandWidth = stTsuStateInfo.IMTsuRecvTotalBandwidth + stTsuStateInfo.IMTsuTransferTotalBandwidth;

        /* ��ȡ���ĵ�һ�����ݸ�ֵ */
        if (iCurrentMinBandWidth < 0)
        {
            iCurrentMinBandWidth = iBandWidth;
            tsu_pos = Itr->first;
        }

        /* �����ǰ�ܴ���С�ڵ�ǰ��С�Ĵ�������� */
        if (iBandWidth < iCurrentMinBandWidth)
        {
            iCurrentMinBandWidth = iBandWidth;
            tsu_pos = Itr->first;
        }
    }

    if (g_TSUResourceInfoMap.size() == error_count)
    {
        return -3;
    }
    else if (g_TSUResourceInfoMap.size() == unused_count)
    {
        return -4;
    }
    else if (g_TSUResourceInfoMap.size() == offline_count)
    {
        return -5;
    }
    else if (g_TSUResourceInfoMap.size() == getstate_count)
    {
        return -9;
    }

    return tsu_pos;
}

/*****************************************************************************
 �� �� ��  : get_idle_tsu_by_resource_balance_for_query_record
 ��������  : ��ѯ¼����Ϣ��ʱ���ȡ���е�TSU��Դ
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2017��2��15��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int get_idle_tsu_by_resource_balance_for_query_record()
{
    int iRet = 0;
    int tsu_pos = -1;
    TSU_Resource_Info_Iterator Itr;

    tsu_resource_info_t* pTsuResourceInfo = NULL;
    TsuStateInfo stTsuStateInfo;
    RecordStateInfoList stRecordStateInfoList;

    int iCurrentMinBandWidth = -1;
    int iBandWidth = -1;
    char strEthName1[16] = {0};
    char strEthName2[16] = {0};

    int error_count = 0;
    int unused_count = 0;
    int offline_count = 0;
    int getstate_count = 0;

    if (g_TSUResourceInfoMap.size() <= 0)
    {
        //DEBUG_TRACE(MODULE_RESOURCE, LOG_DEBUG, "get_idle_tsu_by_resource_balance_for_replay() exit---: TSU Resource Info Map NULL \r\n");
        return -1;
    }

    for (Itr = g_TSUResourceInfoMap.begin(); Itr != g_TSUResourceInfoMap.end(); Itr++)
    {
        pTsuResourceInfo = Itr->second;

        /* ����Ĺ��˵� */
        if ((NULL == pTsuResourceInfo)
            || (pTsuResourceInfo->tsu_device_id[0] == '\0'))
        {
            error_count++;
            continue;
        }

        /* δʹ�õĹ��˵� */
        if (0 == pTsuResourceInfo->iUsed)
        {
            unused_count++;
            continue;
        }

        /* ���ߵ�TSU���˵� */
        if (pTsuResourceInfo->iStatus == 0)
        {
            offline_count++;
            continue;
        }

        /* X86����SX������ʹ�ñ�����TSU */
        if (is_local_tsu(pTsuResourceInfo, local_ip_get(default_eth_name_get())))
        {
            tsu_pos = Itr->first;
            break;
        }

        iRet = get_tsu_state_info(pTsuResourceInfo, stTsuStateInfo, stRecordStateInfoList);

        if (iRet < 0)
        {
            getstate_count++;
            continue;
        }

        /* ���㵱ǰ�ܵĴ��� */
        iBandWidth = stTsuStateInfo.IMTsuRecvTotalBandwidth + stTsuStateInfo.IMTsuTransferTotalBandwidth;

        /* ��ȡ���ĵ�һ�����ݸ�ֵ */
        if (iCurrentMinBandWidth < 0)
        {
            iCurrentMinBandWidth = iBandWidth;
            tsu_pos = Itr->first;
        }

        /* �����ǰ�ܴ���С�ڵ�ǰ��С�Ĵ�������� */
        if (iBandWidth < iCurrentMinBandWidth)
        {
            iCurrentMinBandWidth = iBandWidth;
            tsu_pos = Itr->first;
        }
    }

    if (g_TSUResourceInfoMap.size() == error_count)
    {
        return -3;
    }
    else if (g_TSUResourceInfoMap.size() == unused_count)
    {
        return -4;
    }
    else if (g_TSUResourceInfoMap.size() == offline_count)
    {
        return -5;
    }
    else if (g_TSUResourceInfoMap.size() == getstate_count)
    {
        return -9;
    }

    return tsu_pos;
}

/*****************************************************************************
 �� �� ��  : get_idle_tsu_for_audio_transfer
 ��������  : ��ȡ��Ƶת����TSU��Ϣ
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��5��9�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int get_idle_tsu_for_audio_transfer()
{
    int tsu_pos = -1;
    TSU_Resource_Info_Iterator Itr;
    tsu_resource_info_t* pTsuResourceInfo = NULL;

    if (g_TSUResourceInfoMap.size() <= 0)
    {
        //DEBUG_TRACE(MODULE_RESOURCE, LOG_DEBUG, "get_idle_tsu_by_resource_balance_for_transfer() exit---: TSU Resource Info Map NULL \r\n");
        return -1;
    }

    for (Itr = g_TSUResourceInfoMap.begin(); Itr != g_TSUResourceInfoMap.end(); Itr++)
    {
        pTsuResourceInfo = Itr->second;

        if ((NULL == pTsuResourceInfo)
            || (0 == pTsuResourceInfo->iUsed)
            || (0 == pTsuResourceInfo->iAudioStatus)
            || (pTsuResourceInfo->tsu_device_id[0] == '\0'))
        {
            continue;
        }

        /* ǰ���ѡ���Ĺ��˵� */
        if (Itr->first < g_iCurrentAudioTSUIndex)
        {
            continue;
        }

        tsu_pos = Itr->first;
        g_iCurrentAudioTSUIndex = tsu_pos;
        break;
    }

    if (g_iCurrentAudioTSUIndex >= (int)g_TSUResourceInfoMap.size() - 1)
    {
        g_iCurrentAudioTSUIndex = 0;
    }

    return tsu_pos;
}

/*****************************************************************************
 �� �� ��  : get_recv_port_by_tsu_resource
 ��������  : ����TSU��Ϣ��ȡTSU�Ľ��ն˿ں�
 �������  : char* tsp_ip
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��6��6�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int get_recv_port_by_tsu_resource(char* tsp_ip)
{
    if (NULL == tsp_ip)
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_DEBUG, "get_recv_port_by_tsu_resource() exit---: Param Error \r\n");
        return -1;
    }

    //return TSU_ICE_Interface_GetRecvPort(tsp_ip);
    return 0;
}

/*****************************************************************************
 �� �� ��  : get_send_port_by_tsu_resource
 ��������  : ����TSU��Ϣ��ȡTSU�ķ��Ͷ˿ں�
 �������  : char* tsp_ip
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��6��6�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int get_send_port_by_tsu_resource(char* tsp_ip)
{
    int port = 0;

    if (NULL == tsp_ip)
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_DEBUG, "get_send_port_by_tsu_resource() exit---: Param Error \r\n");
        return -1;
    }

    //port = TSU_ICE_Interface_GetSendPort(tsp_ip);

    if (port <= 0)
    {
        /* ������Ͷ˿�С��0����ô�ٻ�ȡһ�½��ն˿ڣ�����ǰ�汾���� */
        //port = TSU_ICE_Interface_GetRecvPort(tsp_ip);
    }

    return port;
}

/*****************************************************************************
 �� �� ��  : get_tsu_resource_id
 ��������  : ��ȡTSU��ID
 �������  : int pos
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��7��29�� ����һ
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
char * get_tsu_resource_id(int pos)
{
    tsu_resource_info_t* pTsuResourceInfo = NULL;

    if (!is_valid_tsu_resource_info_index(pos))
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_DEBUG, "get_tsu_resource_id() exit---: TSU Resource Info Index Error \r\n");
        return NULL;
    }

    pTsuResourceInfo = g_TSUResourceInfoMap[pos];

    if (0 == pTsuResourceInfo->iUsed)
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_ERROR, "get_tsu_resource_id() exit---: TSU Resource Info UnUsed:pos=%d \r\n", pos);
        return NULL;
    }

    return pTsuResourceInfo->tsu_device_id;
}

/*****************************************************************************
 �� �� ��  : get_all_tsu_disk_info
 ��������  : ��ȡ����TSU�Ĵ��̿ռ���Ϣ
 �������  : int& iTotalDisk
             int& iRemainDisk
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2016��3��17��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int get_all_tsu_disk_info(int& iTotalDisk, int& iRemainDisk)
{
    int iRet = 0;
    char* tsu_ip = NULL;
    int iTotalDiskSize = 0;
    int iRemainDiskSize = 0;
    TSU_Resource_Info_Iterator Itr;
    tsu_resource_info_t* pTsuResourceInfo = NULL;

    if (g_TSUResourceInfoMap.size() <= 0)
    {
        //DEBUG_TRACE(MODULE_RESOURCE, LOG_DEBUG, "get_max_remain_disk_tsu() exit---: TSU Resource Info Map NULL \r\n");
        return -1;
    }

    for (Itr = g_TSUResourceInfoMap.begin(); Itr != g_TSUResourceInfoMap.end(); Itr++)
    {
        pTsuResourceInfo = Itr->second;

        if ((NULL == pTsuResourceInfo)
            || (0 == pTsuResourceInfo->iUsed)
            || (0 == pTsuResourceInfo->iStatus)
            || (pTsuResourceInfo->tsu_device_id[0] == '\0'))
        {
            continue;
        }

        /* ��ȡTSU IP ��ַ */
        tsu_ip = get_tsu_ip(pTsuResourceInfo, default_eth_name_get());

        if (NULL == tsu_ip)
        {
            DEBUG_TRACE(MODULE_RESOURCE, LOG_ERROR,  "get_all_tsu_disk_info() exit---: get_tsu_ip Error: default_eth_name=%s\r\n", default_eth_name_get());
            continue;
        }

        iRet = get_tsu_disk_info(tsu_ip, iTotalDiskSize, iRemainDiskSize);

        if (iRet < 0)
        {
            DEBUG_TRACE(MODULE_RESOURCE, LOG_ERROR, "get_all_tsu_disk_info() get_tsu_disk_info Error:tsu_ip=%s \r\n", tsu_ip);
            continue;
        }

        iTotalDisk += iTotalDiskSize;
        iRemainDisk += iRemainDiskSize;
    }

    return 0;
}

/*****************************************************************************
 �� �� ��  : notify_tsu_add_record_task
 ��������  : ֪ͨTSU ��ʼ¼��
 �������  : cr_t* pCrData
             int service_type:��������: 0:ʵʱ��Ƶ��1:¼��
             int stream_type:��������:ʵʱ��Ƶ��ʱ�����������ͣ�¼���ʱ����¼������
             int storagelife
             int bandwidth
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��6��6�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int notify_tsu_add_record_task(cr_t* pCrData, int service_type, int stream_type, int storagelife, int bandwidth)
{
    int iRet = 0;
    string task_id = "";

    if ((pCrData == NULL) || (0 == pCrData->iUsed))
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_DEBUG, "notify_tsu_add_record_task() exit---: Param Error \r\n");
        return -1;
    }

#if 0
    iRet = TSU_ICE_Interface_AddRecordTask(pCrData->tsu_ip, pCrData->callee_id, pCrData->callee_sdp_ip, pCrData->callee_sdp_port, pCrData->callee_framerate, pCrData->callee_onvif_url, pCrData->callee_transfer_type, service_type, stream_type, storagelife, bandwidth, pCrData->tsu_code, pCrData->tsu_recv_port, task_id);

    if (iRet >= 0)
    {
        if (('\0' != pCrData->task_id[0]) && (task_id.length() > 0))
        {
            memset(pCrData->task_id, 0, MAX_TSU_TASK_LEN + 4);
            osip_strncpy(pCrData->task_id, task_id.c_str(), MAX_TSU_TASK_LEN);
        }
        else if (('\0' == pCrData->task_id[0]) && (task_id.length() > 0))
        {
            osip_strncpy(pCrData->task_id, task_id.c_str(), MAX_TSU_TASK_LEN);
        }
    }

#endif

    return iRet;
}

/*****************************************************************************
 �� �� ��  : notify_tsu_delete_record_task
 ��������  : ֪ͨTSU ֹͣ¼��
 �������  : char* tsp_ip
                            char* task_id
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��6��6�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int notify_tsu_delete_record_task(char* tsp_ip, char* task_id)
{
    int iRet = 0;

    if ((NULL == tsp_ip) || (NULL == task_id))
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_DEBUG, "notify_tsu_delete_record_task() exit---: Param Error \r\n");
        return -1;
    }

    //iRet = TSU_ICE_Interface_DeleteRecordTask(tsp_ip, task_id);

    return iRet;
}

/*****************************************************************************
 �� �� ��  : notify_tsu_pause_record
 ��������  : ֪ͨTSU��ͣ¼��
 �������  : char* tsp_ip
                            char* task_id
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��8��8�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int notify_tsu_pause_record(char* tsp_ip, char* task_id)
{
    int iRet = 0;

    if ((NULL == tsp_ip) || (NULL == task_id))
    {
        //DEBUG_TRACE(MODULE_RESOURCE, LOG_DEBUG, "notify_tsu_pause_record() exit---: Param Error \r\n");
        return -1;
    }

    //iRet = TSU_ICE_Interface_PauseTsuRecord(tsp_ip, task_id);

    return iRet;
}

/*****************************************************************************
 �� �� ��  : notify_tsu_resume_record
 ��������  : ֪ͨTSU�ָ�¼��
 �������  : char* tsp_ip
                            char* task_id
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��8��8�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int notify_tsu_resume_record(char* tsp_ip, char* task_id)
{
    int iRet = 0;

    if ((NULL == tsp_ip) || (NULL == task_id))
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_DEBUG, "notify_tsu_resume_record() exit---: Param Error \r\n");
        return -1;
    }

    //iRet = TSU_ICE_Interface_ResumeTsuRecord(tsp_ip, task_id);

    return iRet;
}

/*****************************************************************************
 �� �� ��  : notify_tsu_add_transfer_task
 ��������  : ֪ͨTSU ��ʼת��
 �������  : cr_t* pCrData
             int service_type:��������: 0:ʵʱ��Ƶ��1:¼��
             int stream_type:��������:ʵʱ��Ƶ��ʱ�����������ͣ�¼���ʱ����¼������
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��7��5�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int notify_tsu_add_transfer_task(cr_t* pCrData, int service_type, int stream_type)
{
    int iRet = 0;
    string task_id = "";

    if ((pCrData == NULL) || (0 == pCrData->iUsed))
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_DEBUG, "notify_tsu_add_transfer_task() exit---: Param Error \r\n");
        return -1;
    }

#if 0
    printf("\r\n\r\n ************************************************* \r\n");
    printf(" notify_tsu_add_transfer_task() pCrData->tsu_ip=%s \r\n", pCrData->tsu_ip);
    printf(" notify_tsu_add_transfer_task() pCrData->callee_id=%s \r\n", pCrData->callee_id);
    printf(" notify_tsu_add_transfer_task() pCrData->callee_sdp_ip=%s \r\n", pCrData->callee_sdp_ip);
    printf(" notify_tsu_add_transfer_task() pCrData->callee_sdp_port=%d \r\n", pCrData->callee_sdp_port);
    printf(" notify_tsu_add_transfer_task() pCrData->caller_id=%s \r\n", pCrData->caller_id);
    printf(" notify_tsu_add_transfer_task() pCrData->caller_sdp_ip=%s \r\n", pCrData->caller_sdp_ip);
    printf(" notify_tsu_add_transfer_task() pCrData->caller_sdp_port=%d \r\n", pCrData->caller_sdp_port);
    printf(" notify_tsu_add_transfer_task() pCrData->tsu_code=%d \r\n", pCrData->tsu_code);
    printf(" notify_tsu_add_transfer_task() pCrData->tsu_port=%d \r\n", pCrData->tsu_port);
    printf(" ************************************************* \r\n\r\n ");

    iRet = TSU_ICE_Interface_AddTransferTaskEx(pCrData->tsu_ip, pCrData->callee_id, pCrData->callee_sdp_ip, pCrData->callee_sdp_port, pCrData->callee_framerate, pCrData->callee_onvif_url, pCrData->callee_transfer_type, service_type, stream_type, pCrData->caller_id, pCrData->caller_sdp_ip, pCrData->caller_sdp_port, pCrData->caller_transfer_type, pCrData->tsu_code, pCrData->tsu_recv_port, pCrData->tsu_send_port, task_id);

    if (iRet >= 0)
    {
        if (('\0' != pCrData->task_id[0]) && (task_id.length() > 0))
        {
            memset(pCrData->task_id, 0, MAX_TSU_TASK_LEN + 4);
            osip_strncpy(pCrData->task_id, task_id.c_str(), MAX_TSU_TASK_LEN);
        }
        else if (('\0' == pCrData->task_id[0]) && (task_id.length() > 0))
        {
            osip_strncpy(pCrData->task_id, task_id.c_str(), MAX_TSU_TASK_LEN);
        }
    }
    else
    {
        iRet = TSU_ICE_Interface_AddTransferTask(pCrData->tsu_ip, pCrData->callee_id, pCrData->callee_sdp_ip, pCrData->callee_sdp_port, pCrData->callee_framerate, pCrData->callee_onvif_url, pCrData->callee_transfer_type, service_type, stream_type, pCrData->caller_id, pCrData->caller_sdp_ip, pCrData->caller_sdp_port, pCrData->caller_transfer_type, pCrData->tsu_code, pCrData->tsu_recv_port, task_id);

        if (iRet >= 0)
        {
            if (('\0' != pCrData->task_id[0]) && (task_id.length() > 0))
            {
                memset(pCrData->task_id, 0, MAX_TSU_TASK_LEN + 4);
                osip_strncpy(pCrData->task_id, task_id.c_str(), MAX_TSU_TASK_LEN);
            }
            else if (('\0' == pCrData->task_id[0]) && (task_id.length() > 0))
            {
                osip_strncpy(pCrData->task_id, task_id.c_str(), MAX_TSU_TASK_LEN);
            }
        }
    }

#endif

    return iRet;
}

/*****************************************************************************
 �� �� ��  : notify_tsu_add_transfer_for_replay_task
 ��������  : ֪ͨTSU ��ʼ¼��طŵ�ת��
 �������  : cr_t* pCrData
             int service_type:��������: 0:ʵʱ��Ƶ��1:¼��
             int stream_type:��������:ʵʱ��Ƶ��ʱ�����������ͣ�¼���ʱ����¼������
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��3��7�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int notify_tsu_add_transfer_for_replay_task(cr_t* pCrData, int service_type, int stream_type)
{
    int iRet = 0;
    string task_id = "";

    if ((pCrData == NULL) || (0 == pCrData->iUsed))
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_DEBUG, "notify_tsu_add_transfer_for_replay_task() exit---: Param Error \r\n");
        return -1;
    }

#if 0
    printf("\r\n\r\n ************************************************* \r\n");
    printf(" notify_tsu_add_transfer_for_replay_task() pCrData->tsu_ip=%s \r\n", pCrData->tsu_ip);
    printf(" notify_tsu_add_transfer_for_replay_task() pCrData->callee_id=%s \r\n", pCrData->callee_id);
    printf(" notify_tsu_add_transfer_for_replay_task() pCrData->callee_sdp_ip=%s \r\n", pCrData->callee_sdp_ip);
    printf(" notify_tsu_add_transfer_for_replay_task() pCrData->callee_sdp_port=%d \r\n", pCrData->callee_sdp_port);
    printf(" notify_tsu_add_transfer_for_replay_task() pCrData->caller_id=%s \r\n", pCrData->caller_id);
    printf(" notify_tsu_add_transfer_for_replay_task() pCrData->caller_sdp_ip=%s \r\n", pCrData->caller_sdp_ip);
    printf(" notify_tsu_add_transfer_for_replay_task() pCrData->caller_sdp_port=%d \r\n", pCrData->caller_sdp_port);
    printf(" notify_tsu_add_transfer_for_replay_task() pCrData->tsu_code=%d \r\n", pCrData->tsu_code);
    printf(" notify_tsu_add_transfer_for_replay_task() pCrData->tsu_port=%d \r\n", pCrData->tsu_port);
    printf(" ************************************************* \r\n\r\n ");

    iRet = TSU_ICE_Interface_AddReplayTransferTaskEx(pCrData->tsu_ip, pCrData->callee_id, pCrData->callee_sdp_ip, pCrData->callee_sdp_port, pCrData->callee_framerate, pCrData->callee_onvif_url, pCrData->callee_transfer_type, service_type, stream_type, pCrData->caller_id, pCrData->caller_sdp_ip, pCrData->caller_sdp_port, pCrData->caller_transfer_type, pCrData->tsu_code, pCrData->tsu_recv_port, pCrData->tsu_send_port, task_id);

    if (iRet >= 0)
    {
        if (('\0' != pCrData->task_id[0]) && (task_id.length() > 0))
        {
            memset(pCrData->task_id, 0, MAX_TSU_TASK_LEN + 4);
            osip_strncpy(pCrData->task_id, task_id.c_str(), MAX_TSU_TASK_LEN);
        }
        else if (('\0' == pCrData->task_id[0]) && (task_id.length() > 0))
        {
            osip_strncpy(pCrData->task_id, task_id.c_str(), MAX_TSU_TASK_LEN);
        }
    }
    else
    {
        iRet = TSU_ICE_Interface_AddReplayTransferTask(pCrData->tsu_ip, pCrData->callee_id, pCrData->callee_sdp_ip, pCrData->callee_sdp_port, pCrData->callee_framerate, pCrData->callee_onvif_url, pCrData->callee_transfer_type, service_type, stream_type, pCrData->caller_id, pCrData->caller_sdp_ip, pCrData->caller_sdp_port, pCrData->caller_transfer_type, pCrData->tsu_code, pCrData->tsu_recv_port, task_id);

        if (iRet >= 0)
        {
            if (('\0' != pCrData->task_id[0]) && (task_id.length() > 0))
            {
                memset(pCrData->task_id, 0, MAX_TSU_TASK_LEN + 4);
                osip_strncpy(pCrData->task_id, task_id.c_str(), MAX_TSU_TASK_LEN);
            }
            else if (('\0' == pCrData->task_id[0]) && (task_id.length() > 0))
            {
                osip_strncpy(pCrData->task_id, task_id.c_str(), MAX_TSU_TASK_LEN);
            }
        }
    }

#endif

    return iRet;
}

/*****************************************************************************
 �� �� ��  : notify_tsu_delete_transfer_task
 ��������  : ֪ͨTSU ֹͣת��
 �������  : char* tsp_ip
                            char* task_id
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��7��5�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int notify_tsu_delete_transfer_task(char* tsp_ip, char* task_id)
{
    int iRet = 0;

    if ((NULL == tsp_ip) || (NULL == task_id))
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_DEBUG, "notify_tsu_delete_transfer_task() exit---: Param Error \r\n");
        return -1;
    }

    //iRet = TSU_ICE_Interface_DeleteTransferTask(tsp_ip, task_id);

    return iRet;
}

/*****************************************************************************
 �� �� ��  : notify_tsu_add_replay_task
 ��������  : ֪ͨTSU ��ӻط�����
 �������  : cr_t* pCrData
             int service_type:��������: 0:ʵʱ��Ƶ��1:¼��
             int stream_type:��������:ʵʱ��Ƶ��ʱ�����������ͣ�¼���ʱ����¼������
             int start_time
             int end_time
             int play_time
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��7��5�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int notify_tsu_add_replay_task(cr_t* pCrData, int service_type, int stream_type, int start_time, int end_time, int play_time)
{
    int iRet = 0;
    string task_id = "";
    int code = 0;

    if ((pCrData == NULL) || (0 == pCrData->iUsed))
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_DEBUG, "notify_tsu_add_replay_task() exit---: Param Error \r\n");
        return -1;
    }

#if 0
    iRet = TSU_ICE_Interface_AddReplayTask(pCrData->tsu_ip, pCrData->callee_id, pCrData->callee_sdp_ip, pCrData->callee_sdp_port, pCrData->callee_framerate, service_type, stream_type, pCrData->caller_id, pCrData->caller_sdp_ip, pCrData->caller_sdp_port, pCrData->caller_transfer_type, start_time, end_time, play_time, pCrData->tsu_send_port, task_id, code);

    if (iRet >= 0)
    {
        if (('\0' != pCrData->task_id[0]) && (task_id.length() > 0))
        {
            memset(pCrData->task_id, 0, MAX_TSU_TASK_LEN + 4);
            osip_strncpy(pCrData->task_id, task_id.c_str(), MAX_TSU_TASK_LEN);
        }
        else if (('\0' == pCrData->task_id[0]) && (task_id.length() > 0))
        {
            osip_strncpy(pCrData->task_id, task_id.c_str(), MAX_TSU_TASK_LEN);
        }

        pCrData->tsu_code = code;

        DEBUG_TRACE(MODULE_RESOURCE, LOG_TRACE, "notify_tsu_add_replay_task() tsu_code=%d \r\n", pCrData->tsu_code);

        return 0;
    }

#endif

    return iRet;
}

/*****************************************************************************
 �� �� ��  : notify_tsu_delete_replay_task
 ��������  : ֪ͨTSU ɾ���ط�����
 �������  : char* tsp_ip
                            char* task_id
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��7��5�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int notify_tsu_delete_replay_task(char* tsp_ip, char* task_id)
{
    int iRet = 0;

    if ((NULL == tsp_ip) || (NULL == task_id))
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_DEBUG, "notify_tsu_delete_replay_task() exit---: Param Error \r\n");
        return -1;
    }

    //iRet = TSU_ICE_Interface_DeleteReplayTask(tsp_ip, task_id);

    return iRet;
}

/*****************************************************************************
 �� �� ��  : notify_tsu_update_mysql_record_stoptime
 ��������  : ֪ͨTSU����¼���¼��Ľ���ʱ��
 �������  : char* tsp_ip
             char* task_id
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��2��4�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int notify_tsu_update_mysql_record_stoptime(char* tsp_ip, char* task_id)
{
    int iRet = 0;

    if ((NULL == tsp_ip) || (NULL == task_id))
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_DEBUG, "notify_tsu_update_mysql_record_stoptime() exit---: Param Error \r\n");
        return -1;
    }

    //iRet = TSU_ICE_Interface_UpdateMysqlRecordStopTimeByTaskId(tsp_ip, task_id);

    return iRet;
}

/*****************************************************************************
 �� �� ��  : notify_tsu_query_replay_list
 ��������  : ֪ͨTSU��ѯ¼���ļ��б�
 �������  : tsu_resource_info_t* pTsuResourceInfo
             char* device_id
             int service_type
             int stream_type
             int start_time
             int end_time
 �������  : VideoRecordList& stVideoRecordList
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��8��5�� ����һ
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int notify_tsu_query_replay_list(tsu_resource_info_t* pTsuResourceInfo, char* device_id, int service_type, int stream_type, int start_time, int end_time, VideoRecordList& stVideoRecordList)
{
    unsigned int i = 0;
    int iRet = 0;
    char* tsu_ip = NULL;
    //RecordList stRecList;

    if ((pTsuResourceInfo == NULL) || (0 == pTsuResourceInfo->iUsed))
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_DEBUG, "notify_tsu_query_replay_list() exit---: Param Error \r\n");
        return -1;
    }

    /* ��ȡ��TSUͨ�ŵ�IP��ַ */
    tsu_ip = get_tsu_ip(pTsuResourceInfo, default_eth_name_get());

#if 0
    stRecList.clear();

    iRet = TSU_ICE_Interface_QueryReplayList(tsu_ip, device_id, service_type, stream_type, start_time, end_time, stRecList);

    if ((iRet >= 0) && (stRecList.size() > 0))
    {
        stVideoRecordList.clear();

        for (i = 0 ; i < stRecList.size() ; i++)
        {
            DEBUG_TRACE(MODULE_RESOURCE, LOG_INFO, "notify_tsu_query_replay_list() \
            \r\n stRecList[%d].DeviceIndex=%u \
            \r\n stRecList[%d].StorageIndex=%d \
            \r\n stRecList[%d].StartTime=%d \
            \r\n stRecList[%d].StopTime=%d \
            \r\n stRecList[%d].Size=%d \
            \r\n stRecList[%d].StorageIP=%s \
            \r\n stRecList[%d].StoragePath=%s \
            \r\n stRecList[%d].Type=%d \
            \r\n", i, stRecList[i].DeviceIndex,
                        i, stRecList[i].StorageIndex,
                        i, stRecList[i].StartTime,
                        i, stRecList[i].StopTime,
                        i, stRecList[i].Size,
                        i, stRecList[i].StorageIP.c_str(),
                        i, stRecList[i].StoragePath.c_str(),
                        i, stRecList[i].Type);

            VideoRecord stVideoRecord;

            stVideoRecord.DeviceIndex = stRecList[i].DeviceIndex;
            stVideoRecord.StorageIndex = stRecList[i].StorageIndex;
            stVideoRecord.StartTime = stRecList[i].StartTime;
            stVideoRecord.StopTime = stRecList[i].StopTime;
            stVideoRecord.Size = stRecList[i].Size;
            stVideoRecord.StorageIP = stRecList[i].StorageIP;
            stVideoRecord.StoragePath = stRecList[i].StoragePath;
            stVideoRecord.Type = stRecList[i].Type;

            stVideoRecordList.push_back(stVideoRecord);
        }
    }

#endif

    return iRet;
}

/*****************************************************************************
 �� �� ��  : notify_tsu_start_replay
 ��������  : ֪ͨTSU ��ʼ�ط�
 �������  : char* tsp_ip
                            char* task_id
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��7��9�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int notify_tsu_start_replay(char* tsp_ip, char* task_id)
{
    int iRet = 0;
    int i = 0;

    if ((NULL == tsp_ip) || (NULL == task_id))
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_DEBUG, "notify_tsu_start_replay() exit---: Param Error \r\n");
        return -1;
    }

#if 0

    for (i = 0; i < 3; i++)
    {
        iRet = TSU_ICE_Interface_StartReplay(tsp_ip, task_id);

        if (iRet >= 0)
        {
            return iRet;
        }
    }

#endif

    return iRet;
}

/*****************************************************************************
 �� �� ��  : notify_tsu_stop_replay
 ��������  : ֪ͨTSU ֹͣ�ط�
 �������  : char* tsp_ip
                            char* task_id
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��7��9�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int notify_tsu_stop_replay(char* tsp_ip, char* task_id)
{
    int iRet = 0;

    if ((NULL == tsp_ip) || (NULL == task_id))
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_DEBUG, "notify_tsu_stop_replay() exit---: Param Error \r\n");
        return -1;
    }

    //iRet = TSU_ICE_Interface_StopReplay(tsp_ip, task_id);

    return iRet;
}

/*****************************************************************************
 �� �� ��  : notify_tsu_resume_replay
 ��������  : ֪ͨTSU �ָ��ط�
 �������  : char* tsp_ip
                            char* task_id
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��7��9�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int notify_tsu_resume_replay(char* tsp_ip, char* task_id)
{
    int iRet = 0;

    if ((NULL == tsp_ip) || (NULL == task_id))
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_DEBUG, "notify_tsu_resume_replay() exit---: Param Error \r\n");
        return -1;
    }

    //iRet = TSU_ICE_Interface_ResumeReplay(tsp_ip, task_id);

    return iRet;
}

/*****************************************************************************
 �� �� ��  : notify_tsu_pause_replay
 ��������  : ֪ͨTSU ��ͣ�ط�
 �������  : char* tsp_ip
                            char* task_id
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��7��9�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int notify_tsu_pause_replay(char* tsp_ip, char* task_id)
{
    int iRet = 0;

    if ((NULL == tsp_ip) || (NULL == task_id))
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_DEBUG, "notify_tsu_pause_replay() exit---: Param Error \r\n");
        return -1;
    }

    //iRet = TSU_ICE_Interface_PauseReplay(tsp_ip, task_id);

    return iRet;
}

/*****************************************************************************
 �� �� ��  : notify_tsu_seek_replay
 ��������  : ֪ͨTSU �ض�ʱ���ط�
 �������  : char* tsp_ip
                            char* task_id
                            int timeSeek
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��7��9�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int notify_tsu_seek_replay(char* tsp_ip, char* task_id, int timeSeek)
{
    int iRet = 0;

    if ((NULL == tsp_ip) || (NULL == task_id))
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_DEBUG, "notify_tsu_seek_replay() exit---: Param Error \r\n");
        return -1;
    }

    //iRet = TSU_ICE_Interface_SeekReplay(tsp_ip, task_id, timeSeek);

    return iRet;
}

/*****************************************************************************
 �� �� ��  : notify_set_replay_speed
 ��������  : ֪ͨTSU���ûط��ٶ�
 �������  : char* tsp_ip
             char* task_id
             float iSpeed
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��10��11�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int notify_set_replay_speed(char* tsp_ip, char* task_id, float iSpeed)
{
    int iRet = 0;

    if ((NULL == tsp_ip) || (NULL == task_id))
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_DEBUG, "notify_set_replay_speed() exit---: Param Error \r\n");
        return -1;
    }

    //iRet = TSU_ICE_Interface_SetReplaySpeed(tsp_ip, task_id, iSpeed);

    return iRet;
}

/*****************************************************************************
 �� �� ��  : notify_tsu_upload_file_record
 ��������  : ֪ͨTSU�����ϱ�¼���ļ�
 �������  : char* tsu_ip
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��1��9�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int notify_tsu_upload_file_record(char* tsu_ip)
{
    if (NULL == tsu_ip)
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_DEBUG, "notify_tsu_upload_file_record() exit---: Param Error \r\n");
        return -1;
    }

    //return TSU_ICE_Interface_UploadFileRecord(tsu_ip);
}

/*****************************************************************************
 �� �� ��  : get_tsu_free_cpu
 ��������  : ��ȡTSU�Ŀ���CPU�ٷֱ�
 �������  : char* tsu_ip
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��7��4�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int get_tsu_free_cpu(char* tsu_ip)
{
    if (NULL == tsu_ip)
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_DEBUG, "get_tsu_free_cpu() exit---: Param Error \r\n");
        return -1;
    }

    //return TSU_ICE_Interface_GetCPUFree(tsu_ip);
}

/*****************************************************************************
 �� �� ��  : get_tsu_free_mem
 ��������  : ��ȡTSU�Ŀ����ڴ�ٷֱ�
 �������  : char* tsu_ip
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��7��4�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int get_tsu_free_mem(char* tsu_ip)
{
    if (NULL == tsu_ip)
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_DEBUG, "get_tsu_free_mem() exit---: Param Error \r\n");
        return -1;
    }

    //return TSU_ICE_Interface_GetMemFree(tsu_ip);
}

/*****************************************************************************
 �� �� ��  : get_tsu_state_info
 ��������  : ��ȡTSU��״̬����
 �������  : tsu_resource_info_t* pTsuResourceInfo
                            TsuStateInfo& stTsuStateInfo
                            RecordStateInfoList& stRecordStateInfoList
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��8��8�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int get_tsu_state_info(tsu_resource_info_t* pTsuResourceInfo, TsuStateInfo& stTsuStateInfo, RecordStateInfoList& stRecordStateInfoList)
{
    unsigned int i = 0;
    int iRet = 0;
    char* tsu_ip = NULL;
    //TsuState stTsuState;
    //TsuRecordList stTsuRecordList;

    if ((pTsuResourceInfo == NULL) || (0 == pTsuResourceInfo->iUsed))
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_DEBUG, "get_tsu_state_info() exit---: Param Error \r\n");
        return -1;
    }

    /* ��ȡ��TSUͨ�ŵ�IP��ַ */
    tsu_ip = get_tsu_ip(pTsuResourceInfo, default_eth_name_get());

#if 0
    stTsuRecordList.clear();

    iRet = TSU_ICE_Interface_GetTsuState(tsu_ip, stTsuState, stTsuRecordList);

    if (iRet >= 0)
    {
        stRecordStateInfoList.clear();

        for (i = 0 ; i < stTsuRecordList.size() ; i++)
        {
            DEBUG_TRACE(MODULE_RESOURCE, LOG_INFO, "get_tsu_state_info() \
            \r\n stTsuRecordList[%d].storagelife=%d \
            \r\n stTsuRecordList[%d].bandwidth=%d \
            \r\n", i, stTsuRecordList[i].storagelife, i, stTsuRecordList[i].bandwidth);

            RecordStateInfo stRecordStateInfo;

            stRecordStateInfo.storagelife = stTsuRecordList[i].storagelife;
            stRecordStateInfo.bandwidth = stTsuRecordList[i].bandwidth;

            stRecordStateInfoList.push_back(stRecordStateInfo);
        }

        DEBUG_TRACE(MODULE_RESOURCE, LOG_INFO, "get_tsu_state_info():ip=%s \
        \r\n stTsuState.DiskMaxSize=%d \
        \r\n stTsuState.DiskMaxWriteWidth=%d \
        \r\n stTsuState.TsuMaxBandwidth=%d \
        \r\n stTsuState.IMTsuRecvTotalBandwidth=%d \
        \r\n stTsuState.IMTsuTransferTotalBandwidth=%d \
        \r\n stTsuState.IMTsuStorageTotalBandwidth=%d \
        \r\n stTsuState.IMTsuReplayTotalBandwidth=%d \
        \r\n", tsu_ip, stTsuState.DiskMaxSize, stTsuState.DiskMaxWriteWidth, stTsuState.TsuMaxBandwidth, stTsuState.IMTsuRecvTotalBandwidth, stTsuState.IMTsuTransferTotalBandwidth, stTsuState.IMTsuStorageTotalBandwidth, stTsuState.IMTsuReplayTotalBandwidth);

        stTsuStateInfo.DiskMaxSize = stTsuState.DiskMaxSize;
        stTsuStateInfo.DiskMaxWriteWidth = stTsuState.DiskMaxWriteWidth;
        stTsuStateInfo.TsuMaxBandwidth = stTsuState.TsuMaxBandwidth;
        stTsuStateInfo.IMTsuRecvTotalBandwidth = stTsuState.IMTsuRecvTotalBandwidth;
        stTsuStateInfo.IMTsuTransferTotalBandwidth = stTsuState.IMTsuTransferTotalBandwidth;
        stTsuStateInfo.IMTsuStorageTotalBandwidth = stTsuState.IMTsuStorageTotalBandwidth;
        stTsuStateInfo.IMTsuReplayTotalBandwidth = stTsuState.IMTsuReplayTotalBandwidth;
    }

#endif

    return iRet;
}

/*****************************************************************************
 �� �� ��  : get_tsu_has_record_total_disk
 ��������  : ��ȡĳ��TSU�ϵ���¼����ܿռ�
 �������  : tsu_resource_info_t* pTsuResourceInfo
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��10��8��
    ��    ��   : �û�·����Ϣ����
    �޸�����   : �����ɺ���

*****************************************************************************/
unsigned long get_tsu_has_record_total_disk(tsu_resource_info_t* pTsuResourceInfo)
{
    unsigned long iM = 0;
    record_info_t*  pRecodInfo = NULL;
    int pos = -1;

    if (NULL == pTsuResourceInfo)
    {
        //DEBUG_TRACE(MODULE_RESOURCE, LOG_DEBUG, "get_tsu_has_record_total_disk() exit---: Param Error \r\n");
        return -1;
    }

    if (pTsuResourceInfo->iUsed == 0
        || pTsuResourceInfo->iStatus == 0
        || pTsuResourceInfo->iStatus == 2
        || pTsuResourceInfo->iStatus == 3)
    {
        //DEBUG_TRACE(MODULE_RESOURCE, LOG_DEBUG, "get_tsu_has_record_total_disk() exit---: TSU Resource Info Status Error \r\n");
        return -1;
    }

    TSU_RECORD_SMUTEX_LOCK(pTsuResourceInfo);

    if (osip_list_size(pTsuResourceInfo->pRecordInfoList) <= 0)
    {
        TSU_RECORD_SMUTEX_UNLOCK(pTsuResourceInfo);
        return 0;
    }

    for (pos = 0; pos < osip_list_size(pTsuResourceInfo->pRecordInfoList); pos++)
    {
        pRecodInfo = (record_info_t*)osip_list_get(pTsuResourceInfo->pRecordInfoList, pos);

        if ((NULL == pRecodInfo) || (pRecodInfo->device_index < 0))
        {
            continue;
        }

        iM += ((pRecodInfo->record_days * 3600 * 24) * (pRecodInfo->bandwidth / 8)) / 1024;
    }

    TSU_RECORD_SMUTEX_UNLOCK(pTsuResourceInfo);
    return iM;
}

/*****************************************************************************
 �� �� ��  : set_tsu_index_id
 ��������  : ����TSU������ID
 �������  : char* tsu_ip
             int iTsuIndexID
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��11��13��
    ��    ��   : �û�·����Ϣ����
    �޸�����   : �����ɺ���

*****************************************************************************/
int set_tsu_index_id(char* tsu_ip, int iTsuIndexID)
{
    if (NULL == tsu_ip)
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_DEBUG, "set_tsu_index_id() exit---: Param Error \r\n");
        return -1;
    }

    //return TSU_ICE_Interface_SetTsuIndexID(tsu_ip, iTsuIndexID);
}

/*****************************************************************************
 �� �� ��  : set_tsu_index_id2
 ��������  : ����TSU������ID
 �������  : int tsu_resource_index
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2017��3��14��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int set_tsu_index_id2(int tsu_resource_index)
{
    tsu_resource_info_t* pTsuResourceInfo = NULL;

    if (tsu_resource_index < 0)
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_DEBUG, "set_tsu_index_id2() exit---: Param Error \r\n");
        return -1;
    }

    /* ��ȡTSU��Դ��Ϣ */
    pTsuResourceInfo = tsu_resource_info_get(tsu_resource_index);

    if (NULL == pTsuResourceInfo)
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_ERROR, "set_tsu_index_id2() exit---: Get TSU Resource Info Error:tsu_resource_index=%d \r\n", tsu_resource_index);
        return -1;
    }

    //return TSU_ICE_Interface_SetTsuIndexID(get_tsu_ip(pTsuResourceInfo, default_eth_name_get()), pTsuResourceInfo->iTsuIndex);
}

/*****************************************************************************
 �� �� ��  : tsu_register
 ��������  :  TSUע��֪ͨ
 �������  : const ::std::string& sTsuID
                            int iSlotID
                            const ::std::string& sTsuVideoIP
                            int iVideoIPEth
                            const ::std::string& sTsuDeviceIP
                            int iDeviceIPEth
                            int iExpires
                            int iRefresh
                            int iTsuType
                            int& iTsuIndex
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��7��2�� ���ڶ�
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int tsu_register(const ::std::string& sTsuID, int iSlotID, const ::std::string& sTsuVideoIP, int iVideoIPEth, const ::std::string& sTsuDeviceIP, int iDeviceIPEth, int iExpires, int iRefresh, int iTsuType, int& iTsuIndex)
{
    int iRet = 0;

    DEBUG_TRACE(MODULE_RESOURCE, LOG_INFO,  "tsu_register() \
    \r\n In Param: \
    \r\n sTsuID=%s \
    \r\n iSlotID=%d \
    \r\n sTsuVideoIP=%s \
    \r\n iVideoIPEth=%d \
    \r\n sTsuDeviceIP=%s \
    \r\n iDeviceIPEth=%d \
    \r\n iExpires=%d \
    \r\n iRefresh=%d \
    \r\n iTsuType=%d \
    \r\n ", sTsuID.c_str(), iSlotID, sTsuVideoIP.c_str(), iVideoIPEth, sTsuDeviceIP.c_str(), iDeviceIPEth, iExpires, iRefresh, iTsuType);

    if (!cms_run_status)
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_WARN, "tsu_register() exit---: CMS Not Enter Main Loop \r\n");
        return -1;
    }

    if (sTsuID.length() <= 0 || sTsuVideoIP.length() <= 0)
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_ERROR, "tsu_register() exit---: Tsu ID Error \r\n");
        return -1;
    }

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�յ�TSUע����Ϣ:TSU ID=%s, SlotID=%d, ��Ƶ��IP��ַ=%s, ��Ƶ������=%d, �豸��IP��ַ=%s, �豸������=%d, ע����Ч��=%d, �Ƿ�ˢ��=%d, TSU����=%d", sTsuID.c_str(), iSlotID, sTsuVideoIP.c_str(), iVideoIPEth, sTsuDeviceIP.c_str(), iDeviceIPEth, iExpires, iRefresh, iTsuType);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Receive TSU registration Message: TSU ID=%s SlotID=%d, TsuVideoIP=%s, VideoIPEth=%d, TsuDeviceIP=%s, DeviceIPEth=%d, Expires=%d, Refresh=%d, TsuType=%d", sTsuID.c_str(), iSlotID, sTsuVideoIP.c_str(), iVideoIPEth, sTsuDeviceIP.c_str(), iDeviceIPEth, iExpires, iRefresh, iTsuType);

    if (sTsuDeviceIP.length() > 0)
    {
        iRet = tsu_reg_msg_add(0, (char*)sTsuID.c_str(), iSlotID, (char*)sTsuVideoIP.c_str(), iVideoIPEth, (char*)sTsuDeviceIP.c_str(), iDeviceIPEth, iExpires, iRefresh, iTsuType);
    }
    else
    {
        iRet = tsu_reg_msg_add(0, (char*)sTsuID.c_str(), iSlotID, (char*)sTsuVideoIP.c_str(), iVideoIPEth, NULL, iDeviceIPEth, iExpires, iRefresh, iTsuType);
    }

    if (iRet != 0)
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_ERROR, "tsu_register() tsu_reg_msg_add Error \r\n");
        return -1;
    }

    return iRet;
}

/*****************************************************************************
 �� �� ��  : tsu_audio_register
 ��������  : TSU����Ƶ����ע�����
 �������  : const ::std::string& sTsuID
             int iExpires
             int iRefresh
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��5��9�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int tsu_audio_register(const ::std::string& sTsuID, int iExpires, int iRefresh)
{
    int iRet = 0;

    DEBUG_TRACE(MODULE_RESOURCE, LOG_INFO,  "tsu_audio_register() \
    \r\n In Param: \
    \r\n sTsuID=%s \
    \r\n iExpires=%d \
    \r\n iRefresh=%d \
    \r\n ", sTsuID.c_str(), iExpires, iRefresh);

    if (!cms_run_status)
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_WARN, "tsu_audio_register() exit---: CMS Not Enter Main Loop \r\n");
        return -1;
    }

    if (sTsuID.length() <= 0)
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_ERROR, "tsu_audio_register() exit---: Tsu ID Error \r\n");
        return -1;
    }

    iRet = tsu_reg_msg_add(1, (char*)sTsuID.c_str(), 0, NULL, 0, NULL, 0, iExpires, iRefresh, 0);

    if (iRet != 0)
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_ERROR, "tsu_audio_register() tsu_reg_msg_add Error \r\n");
        return -1;
    }

    return iRet;
}

/*****************************************************************************
 �� �� ��  : tsu_get_time
 ��������  : TSU��ȡʱ��
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��7��2�� ���ڶ�
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int tsu_get_time()
{
    time_t now;

    now = time(NULL);
    //DEBUG_TRACE(MODULE_RESOURCE, LOG_DEBUG, "tsu_get_time() now=%d \r\n", now);
    return now;
}

/*****************************************************************************
 �� �� ��  : tsu_notify_play_end
 ��������  : TSU�ϱ����񲥷����
 �������  :
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��8��5�� ����һ
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int tsu_notify_play_end()
{
    int iRet = 0;

    if (!cms_run_status)
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_WARN, "tsu_notify_play_end() exit---: CMS Not Enter Main Loop \r\n");
        return -1;
    }

#if 0

    if (tTSUTaskAttribute.sTsuID.length() <= 0 || tTSUTaskAttribute.id.length() <= 0)
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_ERROR, "tsu_notify_play_end() exit---: Tsu ID Error \r\n");
        return -1;
    }

    iRet = tsu_task_attribute_msg_add(1, (char*)tTSUTaskAttribute.sTsuID.c_str(), tTSUTaskAttribute.type, (char*)tTSUTaskAttribute.id.c_str());
#endif

    return iRet;
}

/*****************************************************************************
 �� �� ��  : tsu_notify_pause_play
 ��������  : TSU֪ͨ��ͣ��������
 �������  :
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��5��5�� ����һ
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int tsu_notify_pause_play()
{
    int iRet = 0;

    if (!cms_run_status)
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_WARN, "tsu_notify_pause_play() exit---: CMS Not Enter Main Loop \r\n");
        return -1;
    }

#if 0

    if (tTSUTaskAttribute.sTsuID.length() <= 0 || tTSUTaskAttribute.id.length() <= 0)
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_ERROR, "tsu_notify_pause_play() exit---: Tsu ID Error \r\n");
        return -1;
    }

    iRet = tsu_task_attribute_msg_add(2, (char*)tTSUTaskAttribute.sTsuID.c_str(), tTSUTaskAttribute.type, (char*)tTSUTaskAttribute.id.c_str());
#endif

    return iRet;
}

/*****************************************************************************
 �� �� ��  : tsu_notify_resume_play
 ��������  : TSU֪ͨ�ָ���������
 �������  :
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��5��5�� ����һ
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int tsu_notify_resume_play()
{
    int iRet = 0;

    if (!cms_run_status)
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_WARN, "tsu_notify_resume_play() exit---: CMS Not Enter Main Loop \r\n");
        return -1;
    }

#if 0

    if (tTSUTaskAttribute.sTsuID.length() <= 0 || tTSUTaskAttribute.id.length() <= 0)
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_ERROR, "tsu_notify_resume_play() exit---: Tsu ID Error \r\n");
        return -1;
    }

    iRet = tsu_task_attribute_msg_add(3, (char*)tTSUTaskAttribute.sTsuID.c_str(), tTSUTaskAttribute.type, (char*)tTSUTaskAttribute.id.c_str());
#endif

    return iRet;
}

/*****************************************************************************
 �� �� ��  : tsu_notify_current_task
 ��������  : TSU�ϱ���ǰ��������
 �������  :
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��8��5�� ����һ
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int tsu_notify_current_task()
{
    unsigned int i = 0;
    int iRet = 0;

    if (!cms_run_status)
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_WARN, "tsu_notify_current_task() exit---: CMS Not Enter Main Loop \r\n");
        return -1;
    }

    //DEBUG_TRACE(MODULE_RESOURCE, LOG_INFO,  "tsu_notify_current_task() tTaskAttributeList.size()=%d \r\n ", tTaskAttributeList.size());

#if 0

    if (tTaskAttributeList.size() > 0)
    {
        for (i = 0 ; i < tTaskAttributeList.size() ; i++)
        {
            DEBUG_TRACE(MODULE_RESOURCE, LOG_INFO,  "tsu_notify_current_task() \
            \r\n TSUTaskAttribute [%d] Param: \
            \r\n tTaskAttributeList[%d].sTsuID=%s \
            \r\n tTaskAttributeList[%d].type=%d \
            \r\n tTaskAttributeList[%d].id=%s \
            \r\n ", i, i, tTaskAttributeList[i].sTsuID.c_str(), i, tTaskAttributeList[i].type, i, tTaskAttributeList[i].id.c_str());

            if (tTaskAttributeList[i].sTsuID.length() <= 0 || tTaskAttributeList[i].id.length() <= 0)
            {
                DEBUG_TRACE(MODULE_RESOURCE, LOG_ERROR, "tsu_notify_current_task() exit---: TSU Resource ID Error \r\n");
                continue;
            }

            iRet = tsu_task_attribute_msg_add(0, (char*)tTaskAttributeList[i].sTsuID.c_str(), tTaskAttributeList[i].type, (char*)tTaskAttributeList[i].id.c_str());

            if (iRet != 0)
            {
                DEBUG_TRACE(MODULE_RESOURCE, LOG_ERROR, "tsu_notify_current_task() tsu_task_attribute_msg_add Error \r\n");
            }
        }
    }

#endif

    return 0;
}

/*****************************************************************************
 �� �� ��  : tsu_notify_device_no_stream
 ��������  : TSU֪ͨǰ���豸û������
 �������  : const ::std::string& sDeviceID
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��8��16�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int tsu_notify_device_no_stream(const ::std::string& sDeviceID)
{
    int iRet = 0;

    if (sDeviceID.empty())
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_DEBUG, "tsu_notify_device_no_stream() exit---: Param Error \r\n");
        return -1;
    }

    DEBUG_TRACE(MODULE_RESOURCE, LOG_INFO,  "tsu_notify_device_no_stream() \
    \r\n In Param: \
    \r\n sDeviceID=%s \
    \r\n", sDeviceID.c_str());

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�յ�TSU֪ͨǰ��û������֪ͨ��Ϣ:�߼��豸ID=%s", sDeviceID.c_str());
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "After notice of TSU front-end no stream notification message: logical device ID = % s", sDeviceID.c_str());

    if (!cms_run_status)
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_WARN, "tsu_notify_device_no_stream() exit---: CMS Not Enter Main Loop \r\n");
        return -1;
    }

    if (sDeviceID.length() <= 0)
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_ERROR, "tsu_notify_device_no_stream() exit---: Device ID Error \r\n");
        return -1;
    }

    iRet = tsu_no_stream_msg_add((char*)sDeviceID.c_str());

    if (iRet != 0)
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_ERROR, "tsu_notify_device_no_stream() tsu_no_stream_msg_add Error \r\n");
        return -1;
    }

    return iRet;
}

/*****************************************************************************
 �� �� ��  : tsu_notify_tcp_transfer_end
 ��������  : TSU֪ͨTCP�������ӽ�����Ϣ����
 �������  : const ::std::string& strTranferSessionID
             int iType
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2016��10��8��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int tsu_notify_tcp_transfer_end(const ::std::string& strTranferSessionID, int iType)
{
    int iRet = 0;

    if (strTranferSessionID.empty())
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_DEBUG, "tsu_notify_tcp_transfer_end() exit---: Param Error \r\n");
        return -1;
    }

    DEBUG_TRACE(MODULE_RESOURCE, LOG_INFO,  "tsu_notify_tcp_transfer_end() \
    \r\n In Param: \
    \r\n strTranferSessionID=%s \
    \r\n", strTranferSessionID.c_str());

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�յ�TSU֪ͨTCP���ӽ���֪ͨ��Ϣ:����ID=%s", strTranferSessionID.c_str());
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "After notice of TSU tcp end notification message: task ID = % s", strTranferSessionID.c_str());

    if (!cms_run_status)
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_WARN, "tsu_notify_tcp_transfer_end() exit---: CMS Not Enter Main Loop \r\n");
        return -1;
    }

    if (strTranferSessionID.length() <= 0)
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_ERROR, "tsu_notify_tcp_transfer_end() exit---: Session ID Error \r\n");
        return -1;
    }

    iRet = tsu_task_attribute_msg_add(4, "1", iType, (char*)strTranferSessionID.c_str());

    return iRet;
}

/*****************************************************************************
 �� �� ��  : tsu_notify_cpu_temperature
 ��������  : TSU֪ͨ��ǰ��λCPU�¶�
 �������  : int iSlotID
                            int iTemperature
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��4��23�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
void tsu_notify_cpu_temperature(int iSlotID, int iTemperature)
{
    if (iSlotID < 0)
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_DEBUG, "tsu_notify_cpu_temperature() exit---: Param Error \r\n");
        return;
    }

    DEBUG_TRACE(MODULE_RESOURCE, LOG_INFO,  "tsu_notify_cpu_temperature() \
    \r\n In Param: \
    \r\n SlotID=%d \
    \r\n Temperature=%d \
    \r\n", iSlotID, iTemperature);

    if (!cms_run_status)
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_WARN, "tsu_notify_cpu_temperature() exit---: CMS Not Enter Main Loop \r\n");
        return;
    }

    if (iSlotID > SHELF_SLOT_NUM_LX || iSlotID <= 0)
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_ERROR, "tsu_notify_cpu_temperature() exit---: SlotID Error \r\n");
        return;
    }

    return;
}

/*****************************************************************************
 �� �� ��  : tsu_notify_alarm_msg
 ��������  : TSU֪ͨ�澯��Ϣ
 �������  :
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��9��4�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int tsu_notify_alarm_msg()
{
    int iRet = 0;

    if (!cms_run_status)
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_WARN, "tsu_notify_alarm_msg() exit---: CMS Not Enter Main Loop \r\n");
        return -1;
    }

#if 0
    iRet = tsu_alarm_msg_add(tTSUAlarmMsg.iTSUIndex, tTSUAlarmMsg.iType, tTSUAlarmMsg.iLevel, tTSUAlarmMsg.iTime, (char*)tTSUAlarmMsg.strInfo.c_str());

    if (iRet != 0)
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_ERROR, "tsu_notify_alarm_msg() tsu_alarm_msg_add Error \r\n");
        return -1;
    }

#endif

    return iRet;
}

/*****************************************************************************
 �� �� ��  : tsu_send_image_result
 ��������  : TSU���ͽ�ͼ�������
 �������  : int iType
             int iResult
             const ::std::string& strDeviceID
             int iChannelID
             const ::std::string& strGuid
             int iPicCount
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2016��3��17��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int tsu_send_image_result(int iType, int iResult, const ::std::string& strDeviceID, int iChannelID, const ::std::string& strGuid, int iPicCount)
{
    int iRet = 0;
    char strPIC[1024 * 512] = {0};

    if (!cms_run_status)
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_WARN, "tsu_send_image_result() exit---: CMS Not Enter Main Loop \r\n");
        return -1;
    }

    if (strDeviceID.empty())
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_DEBUG, "tsu_send_image_result() exit---: DeviceID Error \r\n");
        return -1;
    }

    if (strGuid.empty())
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_ERROR, "tsu_send_image_result() exit---: Guid Error \r\n");
        return -1;
    }

#if 0

    if (0 == strPicContent.size())
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_ERROR, "tsu_send_image_result() exit---: Picture Content Error \r\n");
        return -1;
    }

    DEBUG_TRACE(MODULE_RESOURCE, LOG_INFO,  "tsu_send_image_result() \
    \r\n In Param: \
    \r\n Type=%d \
    \r\n Result=%d \
    \r\n DeviceID=%s \
    \r\n ChannelID=%d \
    \r\n Guid=%s \
    \r\n PicCount=%d \
    \r\n PicSize=%d \
    \r\n", iType, iResult, strDeviceID.c_str(), iChannelID, strGuid.c_str(), iPicCount, strPicContent.size());

    if (strPicContent.size() < 1024 * 512)
    {
        memcpy(strPIC, (char*)&strPicContent[0], strPicContent.size());
    }
    else
    {
        memcpy(strPIC, (char*)&strPicContent[0], 1024 * 512 - 1);
    }

    iRet = shdb_tsu_send_image_result_proc(iType, iResult, strDeviceID, iChannelID, strGuid, iPicCount, strPIC);
    DEBUG_TRACE(MODULE_COMMON, LOG_INFO,  "tsu_send_image_result() exit---: shdb_tsu_send_image_result_proc: iRet=%d\r\n", iRet);
#endif

    return iRet;
}

/*****************************************************************************
 �� �� ��  : tsu_ip_pair_add
 ��������  : ���TSU IP��ַ
 �������  : tsu_resource_info_t* pTsuResourceInfo
             char* eth_name
             ip_addr_type_t ip_type
             char* local_ip
             int local_port
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��8��18�� ����һ
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int tsu_ip_pair_add(tsu_resource_info_t* pTsuResourceInfo, char* eth_name, ip_addr_type_t ip_type, char* local_ip, int local_port)
{
    ip_pair_t* pIPaddr = NULL;
    int i = 0;

    if (pTsuResourceInfo == NULL || NULL == pTsuResourceInfo->pTSUIPAddrList)
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_DEBUG, "tsu_ip_pair_add() exit---: Param Error \r\n");
        return -1;
    }

    if (NULL == eth_name || NULL == local_ip || local_port <= 0)
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_ERROR, "tsu_ip_pair_add() exit---: Param Error 2 \r\n");
        return -1;
    }

    i = ip_pair_init(&pIPaddr);

    if (i != 0)
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_ERROR, "tsu_ip_pair_add() exit---: IP Pair Init Error \r\n");
        return -1;
    }

    memset(pIPaddr->eth_name, 0, MAX_IP_LEN);
    osip_strncpy(pIPaddr->eth_name, eth_name, MAX_IP_LEN);

    pIPaddr->ip_type = ip_type;

    memset(pIPaddr->local_ip, 0, MAX_IP_LEN);
    osip_strncpy(pIPaddr->local_ip, local_ip, MAX_IP_LEN);

    pIPaddr->local_port = local_port;

    i = osip_list_add(pTsuResourceInfo->pTSUIPAddrList, pIPaddr, -1); /* add to list tail */

    if (i == -1)
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_ERROR, "tsu_ip_pair_add() exit---: IP Pair List Add Error \r\n");
        ip_pair_free(pIPaddr);
        pIPaddr = NULL;
        return -1;
    }

    return i - 1;
}

/*****************************************************************************
 �� �� ��  : get_tsu_ip
 ��������  : ��ȡTSU IP��ַ
 �������  : tsu_resource_info_t* pTsuResourceInfo
             char* eth_name
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��6��17�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
char* get_tsu_ip(tsu_resource_info_t* pTsuResourceInfo, char* eth_name)
{
    char* tsu_ip = NULL;
    ip_addr_type_t ip_type = IP_ADDR_NULL;

    ip_type = get_local_ip_type(eth_name);

    DEBUG_TRACE(MODULE_RESOURCE, LOG_TRACE, "get_tsu_ip(): ip_type=%d \r\n", ip_type);

    tsu_ip = tsu_ip_get_by_type(pTsuResourceInfo, ip_type);

    if (NULL == tsu_ip || tsu_ip[0] == '\0')
    {
        return NULL;
    }

    return tsu_ip;
}

/*****************************************************************************
 �� �� ��  : is_local_tsu
 ��������  : �Ƿ��Ǳ���TSU
 �������  : tsu_resource_info_t* pTsuResourceInfo
             char* ip_addr
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2017��2��15��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int is_local_tsu(tsu_resource_info_t* pTsuResourceInfo, char* ip_addr)
{
    int i = 0;
    ip_pair_t* pIPaddr = NULL;

    if (ip_addr == NULL || pTsuResourceInfo == NULL || NULL == pTsuResourceInfo->pTSUIPAddrList)
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_DEBUG, "is_local_tsu() exit---: Param Error \r\n");
        return 0;
    }

    if (osip_list_size(pTsuResourceInfo->pTSUIPAddrList) <= 0)
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_TRACE, "is_local_tsu() exit---: No TSU IP Addr \r\n");
        return 0;
    }

    for (i = 0; i < osip_list_size(pTsuResourceInfo->pTSUIPAddrList); i++)
    {
        pIPaddr = (ip_pair_t*)osip_list_get(pTsuResourceInfo->pTSUIPAddrList, i);

        if (NULL == pIPaddr || pIPaddr->eth_name[0] == '\0')
        {
            continue;
        }

        if (0 == sstrcmp(pIPaddr->local_ip, ip_addr))
        {
            return 1;
        }
    }

    return 0;
}

/*****************************************************************************
 �� �� ��  : tsu_ip_get_by_ethname
 ��������  : ��ȡTSU IP��ַ
 �������  : tsu_resource_info_t* pTsuResourceInfo
             char* eth_name
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��8��18�� ����һ
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
char* tsu_ip_get_by_ethname(tsu_resource_info_t* pTsuResourceInfo, char* eth_name)
{
    int i = 0;
    ip_pair_t* pIPaddr = NULL;

    if (NULL == eth_name || pTsuResourceInfo == NULL || NULL == pTsuResourceInfo->pTSUIPAddrList)
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_DEBUG, "tsu_ip_get_by_ethname() exit---: Param Error \r\n");
        return NULL;
    }

    if (osip_list_size(pTsuResourceInfo->pTSUIPAddrList) <= 0)
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_TRACE, "tsu_ip_get_by_ethname() exit---: No TSU IP Addr \r\n");
        return NULL;
    }

    for (i = 0; i < osip_list_size(pTsuResourceInfo->pTSUIPAddrList); i++)
    {
        pIPaddr = (ip_pair_t*)osip_list_get(pTsuResourceInfo->pTSUIPAddrList, i);

        if (NULL == pIPaddr || pIPaddr->eth_name[0] == '\0')
        {
            continue;
        }

        if (0 == sstrcmp(eth_name, pIPaddr->eth_name))
        {
            return pIPaddr->local_ip;
        }
    }

    return NULL;
}

/*****************************************************************************
 �� �� ��  : tsu_ip_get_by_type
 ��������  : ����ip��ַ���Ͳ���TSU��IP��ַ
 �������  : tsu_resource_info_t* pTsuResourceInfo
             ip_addr_type_t ip_type
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��6��17�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
char* tsu_ip_get_by_type(tsu_resource_info_t* pTsuResourceInfo, ip_addr_type_t ip_type)
{
    int i = 0;
    ip_pair_t* pIPaddr = NULL;

    if (ip_type == IP_ADDR_NULL || pTsuResourceInfo == NULL || NULL == pTsuResourceInfo->pTSUIPAddrList)
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_DEBUG, "tsu_ip_get_by_type() exit---: Param Error \r\n");
        return NULL;
    }

    if (osip_list_size(pTsuResourceInfo->pTSUIPAddrList) <= 0)
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_TRACE, "tsu_ip_get_by_type() exit---: No TSU IP Addr \r\n");
        return NULL;
    }

    for (i = 0; i < osip_list_size(pTsuResourceInfo->pTSUIPAddrList); i++)
    {
        pIPaddr = (ip_pair_t*)osip_list_get(pTsuResourceInfo->pTSUIPAddrList, i);

        if (NULL == pIPaddr || pIPaddr->eth_name[0] == '\0')
        {
            continue;
        }

        if (pIPaddr->ip_type == ip_type)
        {
            return pIPaddr->local_ip;
        }
    }

    return NULL;
}

/*****************************************************************************
 �� �� ��  : SetTSUStatus
 ��������  : ����TSU��״̬
 �������  : int tsu_resource_index
             int Status
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��3��11�� ���ڶ�
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int SetTSUStatus(int tsu_resource_index, int Status)
{
    tsu_resource_info_t* pTsuResourceInfo = NULL;

    if (tsu_resource_index < 0)
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_DEBUG, "SetTSUStatus() exit---: Param Error \r\n");
        return -1;
    }

    /* ��ȡTSU��Դ��Ϣ */
    pTsuResourceInfo = tsu_resource_info_get(tsu_resource_index);

    if (NULL == pTsuResourceInfo)
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_ERROR, "SetTSUStatus() exit---: Get TSU Resource Info Error:tsu_resource_index=%d \r\n", tsu_resource_index);
        return -1;
    }

    if (2 == Status)
    {
        if (1 == pTsuResourceInfo->iStatus)
        {
            pTsuResourceInfo->iStatus = Status;
        }
    }
    else if (1 == Status)
    {
        if (2 == pTsuResourceInfo->iStatus)
        {
            pTsuResourceInfo->iStatus = Status;
        }
    }

    return 0;
}

/*****************************************************************************
 �� �� ��  : AddRecordInfoToTSU
 ��������  : ���¼����Ϣ��TSU��
 �������  : tsu_resource_info_t* pTsuResourceInfo
                            record_info_t* pRecodInfo
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��12��5��
    ��    ��   : �û�·����Ϣ����
    �޸�����   : �����ɺ���

*****************************************************************************/
int AddRecordInfoToTSU(tsu_resource_info_t* pTsuResourceInfo, record_info_t* pRecodInfo)
{
    int i = 0;

    if ((NULL == pTsuResourceInfo) || (NULL == pRecodInfo))
    {
        return -1;
    }

    /* TSU����¼��ת����Դ */
    TSU_RECORD_SMUTEX_LOCK(pTsuResourceInfo);
    i = osip_list_add(pTsuResourceInfo->pRecordInfoList, pRecodInfo, -1); /* add to list tail */
    TSU_RECORD_SMUTEX_UNLOCK(pTsuResourceInfo);

    return i - 1;
}

/*****************************************************************************
 �� �� ��  : RemoveRecordInfoFromTSU
 ��������  : �Ƴ�TSU�����¼��¼����Ϣ
 �������  : tsu_resource_info_t* pTsuResourceInfo
             unsigned int device_index
             int stream_type
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��12��5��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int RemoveRecordInfoFromTSU(tsu_resource_info_t* pTsuResourceInfo, unsigned int device_index, int stream_type)
{
    int pos = -1;
    record_info_t* pRecordInfo = NULL;

    if ((NULL == pTsuResourceInfo) || (device_index < 0))
    {
        return -1;
    }

    TSU_RECORD_SMUTEX_LOCK(pTsuResourceInfo);  /* TSU��¼����Դ�� */

    if (osip_list_size(pTsuResourceInfo->pRecordInfoList) <= 0)
    {
        TSU_RECORD_SMUTEX_UNLOCK(pTsuResourceInfo);
        //DEBUG_TRACE(MODULE_RESOURCE, LOG_TRACE, "RemoveRecordInfoFromTSU() exit---: pTsuResourceInfo->pRecordInfoList Info List NULL \r\n");
        return 0;
    }

    for (pos = 0; pos < osip_list_size(pTsuResourceInfo->pRecordInfoList); pos++)
    {
        pRecordInfo = (record_info_t*)osip_list_get(pTsuResourceInfo->pRecordInfoList, pos);

        if ((NULL == pRecordInfo) || (pRecordInfo->device_index < 0))
        {
            continue;
        }

        if (pRecordInfo->device_index == device_index && pRecordInfo->stream_type == stream_type)
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
            osip_list_remove(pTsuResourceInfo->pRecordInfoList, pos);
        }
    }

    TSU_RECORD_SMUTEX_UNLOCK(pTsuResourceInfo);
    return 0;
}

int TSUResourceIPAddrListClone(const osip_list_t* src, cr_t* pCurrentCrData)
{
    int pos = 0;
    ip_pair_t* pSourceIPaddr = NULL;

    if (NULL == src || NULL == pCurrentCrData)
    {
        return -1;
    }

    TSU_SMUTEX_LOCK();

    if (osip_list_size(src) > 0)
    {
        for (pos = 0; pos < osip_list_size(src); pos++)
        {
            pSourceIPaddr = (ip_pair_t*)osip_list_get(src, pos);

            if (NULL == pSourceIPaddr)
            {
                continue;
            }

            if (IP_ADDR_VIDEO == pSourceIPaddr->ip_type)
            {
                memcpy(&pCurrentCrData->TSUVideoIP, pSourceIPaddr, sizeof(ip_pair_t));
            }
            else if (IP_ADDR_DEVICE == pSourceIPaddr->ip_type)
            {
                memcpy(&pCurrentCrData->TSUDeviceIP, pSourceIPaddr, sizeof(ip_pair_t));
            }
        }
    }

    TSU_SMUTEX_UNLOCK();
    return 0;
}

/*****************************************************************************
 �� �� ��  : ShowTSUInfo
 ��������  : �鿴TSU��Ϣ
 �������  : int sock
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��7��27�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void ShowTSUInfo(int sock, int type)
{
    int i = 0;
    int iSendFlag = 0;
    char strLine[] = "\r-----------------------------------------------------------------------\r\n";
    char strHead[] = "\rTSU Index   TSU ID               TSU Type Status   Audio Status Expires\r\n";
    char strLineIP[] = "\rTSU IP Addr Info:>>>>>>>>>>>>>>>>>>>>>>>\r\n";
    char strHeadIP[] = "\rEthName    IP Type  IP Addr         Port\r\n";
    tsu_resource_info_t* pTsuResourceInfo = NULL;
    TSU_Resource_Info_Iterator Itr;
    char rbuf[256] = {0};
    ip_pair_t* pIPaddr = NULL;

    if (g_TSUResourceInfoMap.size() <= 0)
    {
        return;
    }

    if (sock > 0)
    {
        send(sock, strLine, strlen(strLine), 0);
    }

    for (Itr = g_TSUResourceInfoMap.begin(); Itr != g_TSUResourceInfoMap.end(); Itr++)
    {
        pTsuResourceInfo = Itr->second;

        if ((NULL == pTsuResourceInfo)
            || (0 == pTsuResourceInfo->iUsed))
        {
            continue;
        }

        if (type <= 1)
        {
            if (type != pTsuResourceInfo->iStatus)
            {
                continue;
            }
        }

        if (sock > 0)
        {
            send(sock, strHead, strlen(strHead), 0);
        }

        /* ����״̬ */
        if (0 == pTsuResourceInfo->iStatus)
        {
            snprintf(rbuf, 128, "\r%-11d %-20s %-8d %-8s %-12d %-7d\r\n", pTsuResourceInfo->iTsuIndex, pTsuResourceInfo->tsu_device_id, pTsuResourceInfo->iTsuType, (char*)"Off Line", pTsuResourceInfo->iAudioStatus, pTsuResourceInfo->iExpires);
        }
        else if (1 == pTsuResourceInfo->iStatus)
        {
            snprintf(rbuf, 128, "\r%-11d %-20s %-8d %-8s %-12d %-7d\r\n", pTsuResourceInfo->iTsuIndex, pTsuResourceInfo->tsu_device_id, pTsuResourceInfo->iTsuType, (char*)"On Line", pTsuResourceInfo->iAudioStatus, pTsuResourceInfo->iExpires);
        }
        else if (2 == pTsuResourceInfo->iStatus)
        {
            snprintf(rbuf, 128, "\r%-11d %-20s %-8d %-8s %-12d %-7d\r\n", pTsuResourceInfo->iTsuIndex, pTsuResourceInfo->tsu_device_id, pTsuResourceInfo->iTsuType, (char*)"UP Limit", pTsuResourceInfo->iAudioStatus, pTsuResourceInfo->iExpires);
        }
        else if (3 == pTsuResourceInfo->iStatus)
        {
            snprintf(rbuf, 128, "\r%-11d %-20s %-8d %-8s %-12d %-7d\r\n", pTsuResourceInfo->iTsuIndex, pTsuResourceInfo->tsu_device_id, pTsuResourceInfo->iTsuType, (char*)"No Disk", pTsuResourceInfo->iAudioStatus, pTsuResourceInfo->iExpires);
        }
        else
        {
            snprintf(rbuf, 128, "\r%-11d %-20s %-8d %-8s %-12d %-7d\r\n", pTsuResourceInfo->iTsuIndex, pTsuResourceInfo->tsu_device_id, pTsuResourceInfo->iTsuType, (char*)"Unknow", pTsuResourceInfo->iAudioStatus, pTsuResourceInfo->iExpires);
        }

        if (sock > 0)
        {
            send(sock, rbuf, strlen(rbuf), 0);
        }

        send(sock, strLineIP, strlen(strLineIP), 0);

        /* ���Ͷ�Ӧ��IP��ַ */
        if (sock > 0)
        {
            send(sock, strHeadIP, strlen(strHeadIP), 0);
        }

        if (NULL != pTsuResourceInfo->pTSUIPAddrList)
        {
            if (osip_list_size(pTsuResourceInfo->pTSUIPAddrList) > 0)
            {
                for (i = 0; i < osip_list_size(pTsuResourceInfo->pTSUIPAddrList); i++)
                {
                    pIPaddr = (ip_pair_t*)osip_list_get(pTsuResourceInfo->pTSUIPAddrList, i);

                    if (NULL == pIPaddr || pIPaddr->eth_name[0] == '\0')
                    {
                        continue;
                    }

                    snprintf(rbuf, 256, "\r%-10s %-8d %-15s %-8d\r\n", pIPaddr->eth_name, pIPaddr->ip_type, pIPaddr->local_ip, pIPaddr->local_port);

                    if (sock > 0)
                    {
                        send(sock, rbuf, strlen(rbuf), 0);
                    }
                }
            }
        }

        if (sock > 0)
        {
            send(sock, strLine, strlen(strLine), 0);
            iSendFlag = 1;
        }
    }

    if (0 == iSendFlag)
    {
        if (sock > 0)
        {
            send(sock, strLine, strlen(strLine), 0);
        }
    }

    return;
}

/*****************************************************************************
 �� �� ��  : SendTSUOffLineAlarmToAllClientUser
 ��������  : ����TSU���ߵĸ澯��Ϣ�������û�
 �������  : char* tsu_id
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��8��7�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int SendTSUOffLineAlarmToAllClientUser(int tsu_resource_index)
{
    int iRet = 0;
    char* tsu_ip = NULL;
    tsu_resource_info_t* pTsuResourceInfo = NULL;
    unsigned int uType = EV9000_ALARM_LOGIC_DEVICE_ERROR;

    pTsuResourceInfo = tsu_resource_info_get(tsu_resource_index);

    if (NULL == pTsuResourceInfo)
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_ERROR, "SendTSUOffLineAlarmToAllClientUser() exit---: Get Tsu Resource Info Error:tsu_resource_index=%d \r\n", tsu_resource_index);
        return -1;
    }

    tsu_ip = get_tsu_ip(pTsuResourceInfo, default_eth_name_get());

    if (NULL != tsu_ip)
    {
        iRet = SystemFaultAlarm(tsu_resource_index, pTsuResourceInfo->tsu_device_id, uType, (char*)"2", (char*)"0x01400002", "TSU����:TSU ID=%s, TSU IP��ַ=%s", pTsuResourceInfo->tsu_device_id, tsu_ip);
        iRet = EnSystemFaultAlarm(tsu_resource_index, pTsuResourceInfo->tsu_device_id, uType, (char*)"2", (char*)"0x01400002", "TSU off line:TSU ID=%s, TSU IP addr=%s", pTsuResourceInfo->tsu_device_id, tsu_ip);
    }
    else
    {
        iRet = SystemFaultAlarm(tsu_resource_index, pTsuResourceInfo->tsu_device_id, uType, (char*)"2", (char*)"0x01400002", "TSU����:TSU ID=%s, TSU IP��ַ=%s", pTsuResourceInfo->tsu_device_id, (char*)"");
        iRet = EnSystemFaultAlarm(tsu_resource_index, pTsuResourceInfo->tsu_device_id, uType, (char*)"2", (char*)"0x01400002", "TSU off line:TSU ID=%s, TSU IP addr=%s", pTsuResourceInfo->tsu_device_id, (char*)"");
    }

    return iRet;
}

/*****************************************************************************
 �� �� ��  : AddTSUIPAddrToTSUInfoList
 ��������  : ���TSU��IP��ַ��Ϣ��TSU��Ϣ����
 �������  : DBOper* pDBoper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��9��1�� ����һ
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int AddTSUIPAddrToTSUInfoList(DBOper* pDBoper)
{
    int iRet = 0;
    tsu_resource_info_t* pTsuResourceInfo = NULL;
    TSU_Resource_Info_Iterator Itr;

    DEBUG_TRACE(MODULE_RESOURCE, LOG_INFO, "\r\n ********************************************** \
    \r\n |AddTSUIPAddrToTSUInfoList:BEGIN \
    \r\n ********************************************** \r\n");


    if (NULL == pDBoper || g_TSUResourceInfoMap.size() <= 0)
    {
        return -1;
    }

    for (Itr = g_TSUResourceInfoMap.begin(); Itr != g_TSUResourceInfoMap.end(); Itr++)
    {
        pTsuResourceInfo = Itr->second;

        if ((NULL == pTsuResourceInfo)
            || (0 == pTsuResourceInfo->iUsed)
            || (pTsuResourceInfo->tsu_device_id[0] == '\0'))
        {
            continue;
        }

        iRet = osip_list_clone(pGblconf->pLocalIPAddrList, pTsuResourceInfo->pTSUIPAddrList, (int (*)(void*, void**))&ip_pair_clone);

        if (iRet != 0)
        {
            DEBUG_TRACE(MODULE_RESOURCE, LOG_ERROR, "AddTSUIPAddrToTSUInfoList() exit---: TSU IP Addr List Clone Error \r\n");
            continue;
        }
    }

    DEBUG_TRACE(MODULE_RESOURCE, LOG_INFO, "\r\n ********************************************** \
    \r\n |AddTSUIPAddrToTSUInfoList:END \
    \r\n ********************************************** \r\n");

    return 0;
}

/*****************************************************************************
 �� �� ��  : write_tsu_alarm_to_db_proc
 ��������  : ��TSU�澯��Ϣд�����ݿ�
 �������  : char* pcTSUIndex
             char* pcType
             char* pcLevel
             char* pcTime
             char* pcInfo
             DBOper* pDboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��9��4�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int write_tsu_alarm_to_db_proc(char* pcTSUIndex, char* pcType, char* pcLevel, char* pcTime, char* pcInfo, DBOper* pDboper)
{
    int iRet = 0;
    char* local_ip = NULL;
    string strInsertSQL = "";
    char strFromType[16] = {0};
    char strDeviceIndex[32] = {0};

    if (NULL == pcTSUIndex || NULL == pDboper)
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_DEBUG, "write_tsu_alarm_to_db_proc() exit---: Param Error \r\n");
        return -1;
    }

    snprintf(strFromType, 16, "%d", EV9000_LOG_FROMTYPE_CMS);
    snprintf(strDeviceIndex, 16, "%d", local_cms_index_get());

    /* ����SQL��� */
    strInsertSQL.clear();

    strInsertSQL = "insert into SystemLogRecord (FromType,DeviceIndex,DeviceIP,LogType,LogLevel,LogTime,LogInfo) values (";

    strInsertSQL += "'";
    strInsertSQL += strFromType;
    strInsertSQL += "'";

    strInsertSQL += ",";

    strInsertSQL += "'";
    strInsertSQL += strDeviceIndex;
    strInsertSQL += "'";

    strInsertSQL += ",";

    strInsertSQL += "'";
    local_ip = local_ip_get(default_eth_name_get());

    if (NULL != local_ip)
    {
        strInsertSQL += local_ip;
    }

    strInsertSQL += "'";

    strInsertSQL += ",";

    /* ���� */
    strInsertSQL += pcType;

    strInsertSQL += ",";

    /* ���� */
    strInsertSQL += pcLevel;

    strInsertSQL += ",";

    /* ʱ�� */
    strInsertSQL += pcTime;

    strInsertSQL += ",";

    /* ���� */
    strInsertSQL += "'";
    strInsertSQL += pcInfo;
    strInsertSQL += "'";

    strInsertSQL += ")";

    iRet = pDboper->DB_Insert("", "", strInsertSQL.c_str(), 1);

    if (iRet < 0)
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_ERROR, "write_tsu_alarm_to_db_proc() DB Oper Error:strInsertSQL=%s, iRet=%d \r\n", strInsertSQL.c_str(), iRet);
        DEBUG_TRACE(MODULE_RESOURCE, LOG_ERROR, "write_tsu_alarm_to_db_proc() ErrorMsg=%s\r\n", pDboper->GetLastDbErrorMsg());
    }

    return iRet;
}

/*****************************************************************************
 �� �� ��  : get_tsu_audio_recv_port
 ��������  : ��ȡTSU����Ƶ���ն˿�
 �������  : char* tsu_ip
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��5��7�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int get_tsu_audio_recv_port(char* tsu_ip)
{
    if (NULL == tsu_ip)
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_DEBUG, "get_tsu_audio_recv_port() exit---: Param Error \r\n");
        return -1;
    }

    //return TSU_ICE_Audio_Interface_GetRecvPort(tsu_ip);
    return 0;
}

/*****************************************************************************
 �� �� ��  : get_tsu_audio_send_port
 ��������  : ��ȡTSU����Ƶ���Ͷ˿�
 �������  : char* tsu_ip
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��5��7�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int get_tsu_audio_send_port(char* tsu_ip)
{
    if (NULL == tsu_ip)
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_DEBUG, "get_tsu_audio_send_port() exit---: Param Error \r\n");
        return -1;
    }

    //return TSU_ICE_Audio_Interface_GetSendPort(tsu_ip);
    return 0;
}

/*****************************************************************************
 �� �� ��  : notify_tsu_add_audio_transfer_task
 ��������  : ֪ͨTSU������Ƶת������
 �������  : cr_t* pCrData
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��5��7�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int notify_tsu_add_audio_transfer_task(cr_t* pCrData)
{
    int iRet = 0;

    if ((pCrData == NULL) || (0 == pCrData->iUsed))
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_DEBUG, "notify_tsu_add_audio_transfer_task() exit---: Param Error \r\n");
        return -1;
    }

    //iRet = TSU_ICE_Audio_Interface_AddAudioObject(pCrData->tsu_ip, pCrData->caller_ip, pCrData->caller_sdp_port, pCrData->callee_ip, pCrData->callee_sdp_port, pCrData->tsu_send_port);

    return iRet;
}

/*****************************************************************************
 �� �� ��  : notify_tsu_delete_audio_transfer_task
 ��������  : ֪ͨTSUɾ����Ƶת������
 �������  : char* tsu_ip
             char* receive_ip
             int receive_port
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��5��7�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int notify_tsu_delete_audio_transfer_task(char* tsu_ip, char* receive_ip, int receive_port)
{
    int iRet = 0;

    if ((tsu_ip == NULL) || (NULL == receive_ip) || (receive_port <= 0))
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_DEBUG, "notify_tsu_delete_audio_transfer_task() exit---: Param Error \r\n");
        return -1;
    }

    //iRet = TSU_ICE_Audio_Interface_DeleteAudioObject(tsu_ip, receive_ip, receive_port);

    return iRet;
}

/*****************************************************************************
 �� �� ��  : notify_tsu_set_alarm_record
 ��������  : ֪ͨTSU���ø澯¼��
 �������  : char* tsu_ip
             char* task_id
             int iType
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��11��3��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int notify_tsu_set_alarm_record(char* tsu_ip, char* task_id, int iType)
{
    int iRet = 0;

    if ((NULL == tsu_ip) || (NULL == task_id))
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_DEBUG, "notify_tsu_set_alarm_record() exit---: Param Error \r\n");
        return -1;
    }

    //iRet = TSU_ICE_Interface_SetAlarmRecord(tsu_ip, task_id, iType);

    return iRet;
}

/*****************************************************************************
 �� �� ��  : get_tsu_disk_info
 ��������  : �Ϻ��ر��ȡTSU�Ĵ洢��Ϣ
 �������  : char* tsu_ip
             int& iTotalSize
             int& iFreeSize
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2016��3��16��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int get_tsu_disk_info(char* tsu_ip, int& iTotalSize, int& iFreeSize)
{
    int iRet = 0;

    if (NULL == tsu_ip)
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_DEBUG, "get_tsu_disk_info() exit---: Param Error \r\n");
        return -1;
    }

    //iRet = TSU_ICE_Interface_GetDiskInfo(tsu_ip, iTotalSize, iFreeSize);

    return iRet;
}

/*****************************************************************************
 �� �� ��  : notify_tsu_capture_image
 ��������  : �Ϻ��ر�֪ͨTSUץȡͼƬ
 �������  : char* tsu_ip
             int iType
             char* pcSenderID
             int iChannelID
             char* pcGuid
             int iBeforeTime
             int iAfterTime
             int iInterval
             int iTotalNum
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2016��3��16��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int notify_tsu_capture_image(char* tsu_ip, int iType, char* pcSenderID, int iChannelID, char* pcGuid, int iBeforeTime, int iAfterTime, int iInterval, int iTotalNum)
{
    int iRet = 0;

    if (NULL == tsu_ip)
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_DEBUG, "notify_tsu_capture_image() exit---: Param Error \r\n");
        return -1;
    }

    //iRet = TSU_ICE_Interface_NotifyTSUCaptureImage(tsu_ip, iType, pcSenderID, iChannelID, pcGuid, iBeforeTime, iAfterTime, iInterval, iTotalNum);

    return iRet;
}

