
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
#include "route/route_info_mgn.inc"

#include "service/preset_proc.inc"

/*----------------------------------------------*
 * �ⲿ����˵��                                 *
 *----------------------------------------------*/

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
preset_auto_back_list_t* g_PresetAutoBackList = NULL;        /* Ԥ��λ�Զ���λ���� */
device_auto_unlock_list_t* g_DeviceAutoUnlockList = NULL;    /* ��λ�Զ��������� */

/*----------------------------------------------*
 * ��������                                     *
 *----------------------------------------------*/

/*----------------------------------------------*
 * �궨��                                       *
 *----------------------------------------------*/

#if DECS("Ԥ��λ�Զ���λ����")
/*****************************************************************************
 �� �� ��  : preset_auto_back_init
 ��������  : Ԥ��λ�Զ���λ�ṹ��ʼ��
 �������  : preset_auto_back_t** node
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��9��18�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int preset_auto_back_init(preset_auto_back_t** node)
{
    *node = (preset_auto_back_t*)osip_malloc(sizeof(preset_auto_back_t));

    if (*node == NULL)
    {
        return -1;
    }

    (*node)->uDeviceIndex = 0;
    (*node)->uPresetID = 0;
    (*node)->iDurationTime = 0;
    (*node)->iDurationTimeCount = 0;
    (*node)->iStatus = 0;
    return 0;
}

/*****************************************************************************
 �� �� ��  : preset_auto_back_free
 ��������  : Ԥ��λ�Զ���λ�ṹ�ͷ�
 �������  : preset_auto_back_t* node
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��9��18�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
void preset_auto_back_free(preset_auto_back_t* node)
{
    if (node == NULL)
    {
        return;
    }

    node->uDeviceIndex = 0;
    node->uPresetID = 0;
    node->iDurationTime = 0;
    node->iDurationTimeCount = 0;
    node->iStatus = 0;

    return;
}

/*****************************************************************************
 �� �� ��  : preset_auto_back_list_init
 ��������  : Ԥ��λ�Զ���λ������г�ʼ��
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��9��18�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int preset_auto_back_list_init()
{
    g_PresetAutoBackList = (preset_auto_back_list_t*)osip_malloc(sizeof(preset_auto_back_list_t));

    if (g_PresetAutoBackList == NULL)
    {
        return -1;
    }

    /* init duration list*/
    g_PresetAutoBackList->preset_auto_back_list = (osip_list_t*)osip_malloc(sizeof(osip_list_t));

    if (NULL == g_PresetAutoBackList->preset_auto_back_list)
    {
        osip_free(g_PresetAutoBackList);
        g_PresetAutoBackList = NULL;
        return -1;
    }

    osip_list_init(g_PresetAutoBackList->preset_auto_back_list);

#ifdef MULTI_THR
    /* init smutex */
    g_PresetAutoBackList->lock = (osip_mutex_t*)osip_mutex_init();

    if (NULL == g_PresetAutoBackList->lock)
    {
        osip_free(g_PresetAutoBackList->preset_auto_back_list);
        g_PresetAutoBackList->preset_auto_back_list = NULL;
        osip_free(g_PresetAutoBackList);
        g_PresetAutoBackList = NULL;
        return -1;
    }

#endif
    return 0;
}

/*****************************************************************************
 �� �� ��  : alarm_duration_list_clean
 ��������  : �澯��ʱ�����������
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��9��18�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int preset_auto_back_list_clean()
{
    preset_auto_back_t* node = NULL;

    if (NULL == g_PresetAutoBackList)
    {
        return -1;
    }

#ifdef MULTI_THR
    CMS_GBL_SMUTEX_LOCK((struct osip_mutex*)g_PresetAutoBackList->lock);
#endif

    while (!osip_list_eol(g_PresetAutoBackList->preset_auto_back_list, 0))
    {
        node = (preset_auto_back_t*)osip_list_get(g_PresetAutoBackList->preset_auto_back_list, 0);

        if (NULL != node)
        {
            osip_list_remove(g_PresetAutoBackList->preset_auto_back_list, 0);
            preset_auto_back_free(node);
            osip_free(node);
            node = NULL;
        }
    }

#ifdef MULTI_THR
    CMS_GBL_SMUTEX_UNLOCK((struct osip_mutex*)g_PresetAutoBackList->lock);
#endif

    return 0;

}

/*****************************************************************************
 �� �� ��  : alarm_duration_list_clean
 ��������  : �澯��ʱ��������ͷ�
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��9��18�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
void preset_auto_back_list_free()
{
    if (NULL != g_PresetAutoBackList)
    {
        preset_auto_back_list_clean();
        osip_free(g_PresetAutoBackList->preset_auto_back_list);
        g_PresetAutoBackList->preset_auto_back_list = NULL;
#ifdef MULTI_THR
        osip_mutex_destroy((struct osip_mutex*)g_PresetAutoBackList->lock);
        g_PresetAutoBackList->lock = NULL;
#endif
    }

    return;
}

/*****************************************************************************
 �� �� ��  : alarm_duration_find
 ��������  : Ԥ��λ�Զ���λ����
 �������  : unsigned int uDeviceIndex
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��9��18�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
preset_auto_back_t* preset_auto_back_find(unsigned int uDeviceIndex)
{
    int i = 0;
    preset_auto_back_t* node = NULL;

    if (NULL == g_PresetAutoBackList || uDeviceIndex <= 0)
    {
        return NULL;
    }

    i = 0;

    CMS_GBL_SMUTEX_LOCK((struct osip_mutex*)g_PresetAutoBackList->lock);

    while (!osip_list_eol(g_PresetAutoBackList->preset_auto_back_list, i))
    {
        node = (preset_auto_back_t*)osip_list_get(g_PresetAutoBackList->preset_auto_back_list, i);

        if (NULL == node)
        {
            i++;
            continue;
        }

        if (node->uDeviceIndex == uDeviceIndex)
        {
            CMS_GBL_SMUTEX_UNLOCK((struct osip_mutex*)g_PresetAutoBackList->lock);
            return node;
        }

        i++;
    }

    CMS_GBL_SMUTEX_UNLOCK((struct osip_mutex*)g_PresetAutoBackList->lock);
    return NULL;
}

/*****************************************************************************
 �� �� ��  : preset_auto_back_find2
 ��������  : Ԥ��λ�Զ���λ����
 �������  : unsigned int uDeviceIndex
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��11��18�� ���ڶ�
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
preset_auto_back_t* preset_auto_back_find2(unsigned int uDeviceIndex)
{
    int i = 0;
    preset_auto_back_t* node = NULL;

    if (NULL == g_PresetAutoBackList || uDeviceIndex <= 0)
    {
        return NULL;
    }

    i = 0;

    while (!osip_list_eol(g_PresetAutoBackList->preset_auto_back_list, i))
    {
        node = (preset_auto_back_t*)osip_list_get(g_PresetAutoBackList->preset_auto_back_list, i);

        if (NULL == node)
        {
            i++;
            continue;
        }

        if (node->uDeviceIndex == uDeviceIndex)
        {
            return node;
        }

        i++;
    }

    return NULL;
}

/*****************************************************************************
 �� �� ��  : preset_auto_back_use
 ��������  : ����Ԥ��λ�Զ���λ
 �������  : unsigned int uDeviceIndex
             unsigned int uPresetID
             int iDurationTime
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��9��18�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int preset_auto_back_use(unsigned int uDeviceIndex, unsigned int uPresetID, int iDurationTime)
{
    int i = 0;
    preset_auto_back_t* node = NULL;

    if (NULL == g_PresetAutoBackList || NULL == g_PresetAutoBackList->preset_auto_back_list)
    {
        return -1;
    }

    CMS_GBL_SMUTEX_LOCK((struct osip_mutex*)g_PresetAutoBackList->lock);

    node = preset_auto_back_find2(uDeviceIndex);

    if (node == NULL)
    {
        i = preset_auto_back_init(&node);

        if (i != 0)
        {
            CMS_GBL_SMUTEX_UNLOCK((struct osip_mutex*)g_PresetAutoBackList->lock);
            return -1;
        }

        node->uDeviceIndex = uDeviceIndex;
        node->uPresetID = uPresetID;
        node->iDurationTime = iDurationTime;
        node->iDurationTimeCount = iDurationTime;
        node->iStatus = 0;

        osip_list_add(g_PresetAutoBackList->preset_auto_back_list, node, -1);
    }
    else
    {
        node->uPresetID = uPresetID;
        node->iDurationTime = iDurationTime;
        node->iDurationTimeCount = iDurationTime;
        node->iStatus = 0;
    }

    CMS_GBL_SMUTEX_UNLOCK((struct osip_mutex*)g_PresetAutoBackList->lock);

    return 0;
}

/*****************************************************************************
 �� �� ��  : preset_auto_back_update
 ��������  : ����Ԥ��λ�Զ���λ
 �������  : unsigned int uDeviceIndex
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��11��18�� ���ڶ�
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int preset_auto_back_update(unsigned int uDeviceIndex)
{
    int i = 0;
    preset_auto_back_t* node = NULL;

    if (NULL == g_PresetAutoBackList || NULL == g_PresetAutoBackList->preset_auto_back_list)
    {
        return -1;
    }

    CMS_GBL_SMUTEX_LOCK((struct osip_mutex*)g_PresetAutoBackList->lock);

    node = preset_auto_back_find2(uDeviceIndex);

    if (node != NULL)
    {
        node->iDurationTimeCount = node->iDurationTime;
        node->iStatus = 1;
        i = 1;
    }

    CMS_GBL_SMUTEX_UNLOCK((struct osip_mutex*)g_PresetAutoBackList->lock);

    return i;
}

/*****************************************************************************
 �� �� ��  : preset_auto_back_remove
 ��������  : �Ƴ�Ԥ��λ�Զ���λ
 �������  : unsigned int uDeviceIndex
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��11��18�� ���ڶ�
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int preset_auto_back_remove(unsigned int uDeviceIndex)
{
    int pos = 0;
    preset_auto_back_t* node = NULL;

    if (NULL == g_PresetAutoBackList || NULL == g_PresetAutoBackList->preset_auto_back_list)
    {
        return -1;
    }

    CMS_GBL_SMUTEX_LOCK((struct osip_mutex*)g_PresetAutoBackList->lock);

    pos = 0;

    while (!osip_list_eol(g_PresetAutoBackList->preset_auto_back_list, pos))
    {
        node = (preset_auto_back_t*)osip_list_get(g_PresetAutoBackList->preset_auto_back_list, pos);

        if (NULL == node)
        {
            osip_list_remove(g_PresetAutoBackList->preset_auto_back_list, pos);
            continue;
        }

        if (node->uDeviceIndex == uDeviceIndex)
        {
            osip_list_remove(g_PresetAutoBackList->preset_auto_back_list, pos);
            preset_auto_back_free(node);
            osip_free(node);
            node = NULL;
            CMS_GBL_SMUTEX_UNLOCK((struct osip_mutex*)g_PresetAutoBackList->lock);
            return pos;
        }

        pos++;
    }

    CMS_GBL_SMUTEX_UNLOCK((struct osip_mutex*)g_PresetAutoBackList->lock);

    return 0;
}

/*****************************************************************************
 �� �� ��  : scan_preset_auto_back_list
 ��������  : ɨ��Ԥ��λ�Զ���λ�������
 �������  :
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��9��18�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
void scan_preset_auto_back_list()
{
    int iRet = 0;
    int pos = 0;
    preset_auto_back_t* node = NULL;
    preset_auto_back_t* proc_node = NULL;
    needtoproc_preset_auto_back_queue needproc;

    if (NULL == g_PresetAutoBackList)
    {
        return;
    }

    needproc.clear();
    CMS_GBL_SMUTEX_LOCK((struct osip_mutex*)g_PresetAutoBackList->lock);

    if (osip_list_size(g_PresetAutoBackList->preset_auto_back_list) <= 0)
    {
        CMS_GBL_SMUTEX_UNLOCK((struct osip_mutex*)g_PresetAutoBackList->lock);
        return;
    }

    pos = 0;

    while (!osip_list_eol(g_PresetAutoBackList->preset_auto_back_list, pos))
    {
        node = (preset_auto_back_t*)osip_list_get(g_PresetAutoBackList->preset_auto_back_list, pos);

        if (NULL == node)
        {
            osip_list_remove(g_PresetAutoBackList->preset_auto_back_list, pos);
            continue;
        }

        if (1 == node->iStatus) /* ��Ҫ�Զ���λ�� */
        {
            node->iDurationTimeCount--;

            if (node->iDurationTimeCount <= 0)
            {
                needproc.push_back(node);
            }
        }

        pos++;
    }

    CMS_GBL_SMUTEX_UNLOCK((struct osip_mutex*)g_PresetAutoBackList->lock);

    /* ������Ҫ���͹�λ��Ϣ�� */
    while (!needproc.empty())
    {
        proc_node = (preset_auto_back_t*) needproc.front();
        needproc.pop_front();

        if (NULL != proc_node)
        {
            iRet = preset_auto_back_proc(proc_node->uDeviceIndex, proc_node->uPresetID);

            if (iRet != 0)
            {
                if (iRet == -2) /* ��λ��������, ��Ҫ�ٴμ�ʱ����λ */
                {
                    node->iDurationTimeCount = node->iDurationTime;
                }
                else
                {
                    proc_node->iStatus = 0;
                }

                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "scan_preset_auto_back_list() preset_auto_back_proc Error:DeviceIndex=%u, PresetID=%d, iRet=%d \r\n", node->uDeviceIndex, node->uPresetID, iRet);
            }
            else
            {
                proc_node->iStatus = 0;
                DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "scan_preset_auto_back_list() preset_auto_back_proc OK:DeviceIndex=%u, PresetID=%d, iRet=%d \r\n", node->uDeviceIndex, node->uPresetID, iRet);
            }
        }
    }

    needproc.clear();

    return;
}

/*****************************************************************************
 �� �� ��  : preset_auto_back_proc
 ��������  : Ԥ��λ��λ����
 �������  : unsigned int uDeviceIndex
             unsigned int uPresetID
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��9��18�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int preset_auto_back_proc(unsigned int uDeviceIndex, unsigned int uPresetID)
{
    int iRet = 0;
    GBLogicDevice_info_t* pDestGBLogicDeviceInfo = NULL;
    GBDevice_info_t* pDestGBDeviceInfo = NULL;

    int iCalleeRoutePos = 0;
    route_info_t* pCalleeRouteInfo = NULL;

    if (uDeviceIndex <= 0)
    {
        return -1;
    }

    /* ��ȡĿ�Ķ˵��豸��Ϣ */
    pDestGBLogicDeviceInfo = GBLogicDevice_info_find_by_device_index(uDeviceIndex);

    if (NULL != pDestGBLogicDeviceInfo)
    {
        /* �����߼��豸����������жϣ�������Ϣ���� */
        if (1 == pDestGBLogicDeviceInfo->other_realm)
        {
            /* �����ϼ�·����Ϣ */
            iCalleeRoutePos = route_info_find(pDestGBLogicDeviceInfo->cms_id);

            if (iCalleeRoutePos >= 0)
            {
                pCalleeRouteInfo = route_info_get(iCalleeRoutePos);

                if (NULL != pCalleeRouteInfo)
                {
                    iRet = SendExecuteDevicePresetMessageToRoute(pDestGBLogicDeviceInfo, pCalleeRouteInfo, uPresetID);

                    if (iRet != 0)
                    {
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Ԥ֪λ�Զ���λ, ִ�е�λԤ��λ, ���͵��ϼ�CMSʧ��:��λID=%s, ��λ����=%s, Ԥ��λID=%u", pDestGBLogicDeviceInfo->device_id, pDestGBLogicDeviceInfo->device_name, uPresetID);
                        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Predict a automatic homing, executive level presets, sent to the superior CMS failure: point ID = % s, point name = % s, preset ID = % u", pDestGBLogicDeviceInfo->device_id, pDestGBLogicDeviceInfo->device_name, uPresetID);
                    }
                    else
                    {
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Ԥ֪λ�Զ���λ, ִ�е�λԤ��λ, ���͵��ϼ�CMS�ɹ�:��λID=%s, ��λ����=%s, Ԥ��λID=%u", pDestGBLogicDeviceInfo->device_id, pDestGBLogicDeviceInfo->device_name, uPresetID);
                        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Predict a automatic homing, executive level presets, sent to the superior CMS success: point ID = % s, point name = % s, preset ID = % u", pDestGBLogicDeviceInfo->device_id, pDestGBLogicDeviceInfo->device_name, uPresetID);
                    }
                }
                else
                {
                    DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "preset_auto_back_proc() Get Dest Route Info Error, cms_id=%s\r\n", pDestGBLogicDeviceInfo->cms_id);
                }
            }
            else
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "preset_auto_back_proc() Find Dest Route Info Error, cms_id=%s\r\n", pDestGBLogicDeviceInfo->cms_id);
            }
        }
        else
        {
            pDestGBDeviceInfo = GBDevice_info_get_by_stream_type(pDestGBLogicDeviceInfo, EV9000_STREAM_TYPE_MASTER);

            if (NULL != pDestGBDeviceInfo)
            {
                iRet = SendExecuteDevicePresetMessage(pDestGBLogicDeviceInfo, pDestGBDeviceInfo, uPresetID);

                if (iRet != 0)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Ԥ֪λ�Զ���λ, ִ�е�λԤ��λ, ���͵�ǰ��ʧ��:��λID=%s, ��λ����=%s, Ԥ��λID=%u", pDestGBLogicDeviceInfo->device_id, pDestGBLogicDeviceInfo->device_name, uPresetID);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Predict an automatic homing, executive level presets, sent to the front end failure: point ID = % s, point name = % s, preset ID = % u", pDestGBLogicDeviceInfo->device_id, pDestGBLogicDeviceInfo->device_name, uPresetID);
                }
                else
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Ԥ֪λ�Զ���λ, ִ�е�λԤ��λ, ���͵�ǰ�˳ɹ�:��λID=%s, ��λ����=%s, Ԥ��λID=%u", pDestGBLogicDeviceInfo->device_id, pDestGBLogicDeviceInfo->device_name, uPresetID);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Predict an automatic homing, executive level presets, sent to the front end success: point ID = % s, point name = % s, preset ID = % u", pDestGBLogicDeviceInfo->device_id, pDestGBLogicDeviceInfo->device_name, uPresetID);
                }
            }
            else
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "preset_auto_back_proc() Get DestGBDeviceInfo Error:device_id=%s \r\n", pDestGBLogicDeviceInfo->device_id);
                return -1;
            }
        }
    }
    else
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "preset_auto_back_proc() Find Dest GBLogic Device Info Error, DeviceIndex=%u\r\n", uDeviceIndex);
        return -1;
    }

    return iRet;
}

/*****************************************************************************
 �� �� ��  : ShowPresetAutoBackTaskInfo
 ��������  : ��ʾ��λ�Զ���λ��Ϣ
 �������  : int sock
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2016��4��8��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
void ShowPresetAutoBackTaskInfo(int sock)
{
    int i = 0;
    char strLine[] = "\r--------------------------------------------------------------\r\n";
    char strHead[] = "\rDeviceIndex  PresetID  Status  DurationTime  DurationTimeCount\r\n";
    preset_auto_back_t* node = NULL;
    char rbuf[128] = {0};

    if (sock > 0)
    {
        send(sock, strLine, strlen(strLine), 0);
        send(sock, strHead, strlen(strHead), 0);
    }

    if ((NULL == g_PresetAutoBackList) || (NULL == g_PresetAutoBackList->preset_auto_back_list))
    {
        return;
    }

    CMS_GBL_SMUTEX_LOCK((struct osip_mutex*)g_PresetAutoBackList->lock);

    if (osip_list_size(g_PresetAutoBackList->preset_auto_back_list) <= 0)
    {
        CMS_GBL_SMUTEX_UNLOCK((struct osip_mutex*)g_PresetAutoBackList->lock);
        return;
    }

    for (i = 0; i < osip_list_size(g_PresetAutoBackList->preset_auto_back_list); i++)
    {
        node = (preset_auto_back_t*)osip_list_get(g_PresetAutoBackList->preset_auto_back_list, i);

        if (NULL == node)
        {
            continue;
        }

        snprintf(rbuf, 128, "\r%-11u  %-8u  %-6d  %-12d  %-17d\r\n", node->uDeviceIndex, node->uPresetID, node->iStatus, node->iDurationTime, node->iDurationTimeCount);

        if (sock > 0)
        {
            send(sock, rbuf, strlen(rbuf), 0);
        }
    }

    CMS_GBL_SMUTEX_UNLOCK((struct osip_mutex*)g_PresetAutoBackList->lock);

    if (sock > 0)
    {
        send(sock, strLine, strlen(strLine), 0);
    }

    return;
}
#endif

#if DECS("��λ�Զ���������")
/*****************************************************************************
 �� �� ��  : device_auto_unlock_init
 ��������  : ��λ�Զ������ṹ��ʼ��
 �������  : device_auto_unlock_t** node
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��9��18�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int device_auto_unlock_init(device_auto_unlock_t** node)
{
    *node = (device_auto_unlock_t*)osip_malloc(sizeof(device_auto_unlock_t));

    if (*node == NULL)
    {
        return -1;
    }

    (*node)->uDeviceIndex = 0;
    (*node)->iDurationTimeCount = 0;
    (*node)->iStatus = 0;
    return 0;
}

/*****************************************************************************
 �� �� ��  : device_auto_unlock_free
 ��������  : ��λ�Զ������ṹ�ͷ�
 �������  : device_auto_unlock_t* node
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��9��18�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
void device_auto_unlock_free(device_auto_unlock_t* node)
{
    if (node == NULL)
    {
        return;
    }

    node->uDeviceIndex = 0;
    node->iDurationTimeCount = 0;
    node->iStatus = 0;

    return;
}

/*****************************************************************************
 �� �� ��  : device_auto_unlock_list_init
 ��������  : ��λ�Զ�����������г�ʼ��
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��9��18�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int device_auto_unlock_list_init()
{
    g_DeviceAutoUnlockList = (device_auto_unlock_list_t*)osip_malloc(sizeof(device_auto_unlock_list_t));

    if (g_DeviceAutoUnlockList == NULL)
    {
        return -1;
    }

    /* init duration list*/
    g_DeviceAutoUnlockList->device_auto_unlock_list = (osip_list_t*)osip_malloc(sizeof(osip_list_t));

    if (NULL == g_DeviceAutoUnlockList->device_auto_unlock_list)
    {
        osip_free(g_DeviceAutoUnlockList);
        g_DeviceAutoUnlockList = NULL;
        return -1;
    }

    osip_list_init(g_DeviceAutoUnlockList->device_auto_unlock_list);

#ifdef MULTI_THR
    /* init smutex */
    g_DeviceAutoUnlockList->lock = (osip_mutex_t*)osip_mutex_init();

    if (NULL == g_DeviceAutoUnlockList->lock)
    {
        osip_free(g_DeviceAutoUnlockList->device_auto_unlock_list);
        g_DeviceAutoUnlockList->device_auto_unlock_list = NULL;
        osip_free(g_DeviceAutoUnlockList);
        g_DeviceAutoUnlockList = NULL;
        return -1;
    }

#endif
    return 0;
}

/*****************************************************************************
 �� �� ��  : alarm_duration_list_clean
 ��������  : �澯��ʱ�����������
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��9��18�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int device_auto_unlock_list_clean()
{
    device_auto_unlock_t* node = NULL;

    if (NULL == g_DeviceAutoUnlockList)
    {
        return -1;
    }

#ifdef MULTI_THR
    CMS_GBL_SMUTEX_LOCK((struct osip_mutex*)g_DeviceAutoUnlockList->lock);
#endif

    while (!osip_list_eol(g_DeviceAutoUnlockList->device_auto_unlock_list, 0))
    {
        node = (device_auto_unlock_t*)osip_list_get(g_DeviceAutoUnlockList->device_auto_unlock_list, 0);

        if (NULL != node)
        {
            osip_list_remove(g_DeviceAutoUnlockList->device_auto_unlock_list, 0);
            device_auto_unlock_free(node);
            osip_free(node);
            node = NULL;
        }
    }

#ifdef MULTI_THR
    CMS_GBL_SMUTEX_UNLOCK((struct osip_mutex*)g_DeviceAutoUnlockList->lock);
#endif

    return 0;

}

/*****************************************************************************
 �� �� ��  : alarm_duration_list_clean
 ��������  : �澯��ʱ��������ͷ�
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��9��18�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
void device_auto_unlock_list_free()
{
    if (NULL != g_DeviceAutoUnlockList)
    {
        device_auto_unlock_list_clean();
        osip_free(g_DeviceAutoUnlockList->device_auto_unlock_list);
        g_DeviceAutoUnlockList->device_auto_unlock_list = NULL;
#ifdef MULTI_THR
        osip_mutex_destroy((struct osip_mutex*)g_DeviceAutoUnlockList->lock);
        g_DeviceAutoUnlockList->lock = NULL;
#endif
    }

    return;
}

/*****************************************************************************
 �� �� ��  : alarm_duration_find
 ��������  : ��λ�Զ���������
 �������  : unsigned int uDeviceIndex
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��9��18�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
device_auto_unlock_t* device_auto_unlock_find(unsigned int uDeviceIndex)
{
    int i = 0;
    device_auto_unlock_t* node = NULL;

    if (NULL == g_DeviceAutoUnlockList || uDeviceIndex <= 0)
    {
        return NULL;
    }

    i = 0;

    CMS_GBL_SMUTEX_LOCK((struct osip_mutex*)g_DeviceAutoUnlockList->lock);

    while (!osip_list_eol(g_DeviceAutoUnlockList->device_auto_unlock_list, i))
    {
        node = (device_auto_unlock_t*)osip_list_get(g_DeviceAutoUnlockList->device_auto_unlock_list, i);

        if (NULL == node)
        {
            i++;
            continue;
        }

        if (node->uDeviceIndex == uDeviceIndex)
        {
            CMS_GBL_SMUTEX_UNLOCK((struct osip_mutex*)g_DeviceAutoUnlockList->lock);
            return node;
        }

        i++;
    }

    CMS_GBL_SMUTEX_UNLOCK((struct osip_mutex*)g_DeviceAutoUnlockList->lock);
    return NULL;
}

/*****************************************************************************
 �� �� ��  : device_auto_unlock_find2
 ��������  : ��λ�Զ���������
 �������  : unsigned int uDeviceIndex
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��11��18�� ���ڶ�
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
device_auto_unlock_t* device_auto_unlock_find2(unsigned int uDeviceIndex)
{
    int i = 0;
    device_auto_unlock_t* node = NULL;

    if (NULL == g_DeviceAutoUnlockList || uDeviceIndex <= 0)
    {
        return NULL;
    }

    i = 0;

    while (!osip_list_eol(g_DeviceAutoUnlockList->device_auto_unlock_list, i))
    {
        node = (device_auto_unlock_t*)osip_list_get(g_DeviceAutoUnlockList->device_auto_unlock_list, i);

        if (NULL == node)
        {
            i++;
            continue;
        }

        if (node->uDeviceIndex == uDeviceIndex)
        {
            return node;
        }

        i++;
    }

    return NULL;
}

/*****************************************************************************
 �� �� ��  : device_auto_unlock_use
 ��������  : ���ӵ�λ�Զ�����
 �������  : unsigned int uDeviceIndex
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��9��18�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int device_auto_unlock_use(unsigned int uDeviceIndex)
{
    int i = 0;
    device_auto_unlock_t* node = NULL;

    if (NULL == g_DeviceAutoUnlockList || NULL == g_DeviceAutoUnlockList->device_auto_unlock_list)
    {
        return -1;
    }

    CMS_GBL_SMUTEX_LOCK((struct osip_mutex*)g_DeviceAutoUnlockList->lock);

    node = device_auto_unlock_find2(uDeviceIndex);

    if (node == NULL)
    {
        i = device_auto_unlock_init(&node);

        if (i != 0)
        {
            CMS_GBL_SMUTEX_UNLOCK((struct osip_mutex*)g_DeviceAutoUnlockList->lock);
            return -1;
        }

        node->uDeviceIndex = uDeviceIndex;
        node->iDurationTimeCount = local_device_unlock_time_get();
        node->iStatus = 1;

        osip_list_add(g_DeviceAutoUnlockList->device_auto_unlock_list, node, -1);
    }
    else
    {
        node->iDurationTimeCount = local_device_unlock_time_get();
        node->iStatus = 1;
    }

    CMS_GBL_SMUTEX_UNLOCK((struct osip_mutex*)g_DeviceAutoUnlockList->lock);

    return 0;
}

/*****************************************************************************
 �� �� ��  : device_auto_unlock_update
 ��������  : ���µ�λ�Զ�����ʱ��
 �������  : unsigned int uDeviceIndex
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2016��4��7��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int device_auto_unlock_update(unsigned int uDeviceIndex)
{
    int i = 0;
    device_auto_unlock_t* node = NULL;

    if (NULL == g_DeviceAutoUnlockList || NULL == g_DeviceAutoUnlockList->device_auto_unlock_list)
    {
        return -1;
    }

    CMS_GBL_SMUTEX_LOCK((struct osip_mutex*)g_DeviceAutoUnlockList->lock);

    node = device_auto_unlock_find2(uDeviceIndex);

    if (node != NULL)
    {
        node->iDurationTimeCount = local_device_unlock_time_get();
        node->iStatus = 1;
        i = 1;
    }

    CMS_GBL_SMUTEX_UNLOCK((struct osip_mutex*)g_DeviceAutoUnlockList->lock);

    return i;
}

/*****************************************************************************
 �� �� ��  : device_auto_unlock_remove
 ��������  : �Ƴ���λ�Զ�����
 �������  : unsigned int uDeviceIndex
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��11��18�� ���ڶ�
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int device_auto_unlock_remove(unsigned int uDeviceIndex)
{
    int pos = 0;
    device_auto_unlock_t* node = NULL;

    if (NULL == g_DeviceAutoUnlockList || NULL == g_DeviceAutoUnlockList->device_auto_unlock_list)
    {
        return -1;
    }

    CMS_GBL_SMUTEX_LOCK((struct osip_mutex*)g_DeviceAutoUnlockList->lock);

    pos = 0;

    while (!osip_list_eol(g_DeviceAutoUnlockList->device_auto_unlock_list, pos))
    {
        node = (device_auto_unlock_t*)osip_list_get(g_DeviceAutoUnlockList->device_auto_unlock_list, pos);

        if (NULL == node)
        {
            osip_list_remove(g_DeviceAutoUnlockList->device_auto_unlock_list, pos);
            continue;
        }

        if (node->uDeviceIndex == uDeviceIndex)
        {
            osip_list_remove(g_DeviceAutoUnlockList->device_auto_unlock_list, pos);
            device_auto_unlock_free(node);
            osip_free(node);
            node = NULL;
            CMS_GBL_SMUTEX_UNLOCK((struct osip_mutex*)g_DeviceAutoUnlockList->lock);
            return pos;
        }

        pos++;
    }

    CMS_GBL_SMUTEX_UNLOCK((struct osip_mutex*)g_DeviceAutoUnlockList->lock);

    return 0;
}

/*****************************************************************************
 �� �� ��  : scan_device_auto_unlock_list
 ��������  : ɨ���λ�Զ������������
 �������  :
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��9��18�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
void scan_device_auto_unlock_list()
{
    int pos = 0;
    int iRet = 0;
    device_auto_unlock_t* node = NULL;
    device_auto_unlock_t* proc_node = NULL;
    needtoproc_device_auto_unlock_queue needproc;

    if (NULL == g_DeviceAutoUnlockList)
    {
        return;
    }

    needproc.clear();
    CMS_GBL_SMUTEX_LOCK((struct osip_mutex*)g_DeviceAutoUnlockList->lock);

    if (osip_list_size(g_DeviceAutoUnlockList->device_auto_unlock_list) <= 0)
    {
        CMS_GBL_SMUTEX_UNLOCK((struct osip_mutex*)g_DeviceAutoUnlockList->lock);
        return;
    }

    pos = 0;

    while (!osip_list_eol(g_DeviceAutoUnlockList->device_auto_unlock_list, pos))
    {
        node = (device_auto_unlock_t*)osip_list_get(g_DeviceAutoUnlockList->device_auto_unlock_list, pos);

        if (NULL == node)
        {
            osip_list_remove(g_DeviceAutoUnlockList->device_auto_unlock_list, pos);
            continue;
        }

        if (node->iStatus <= 0)
        {
            osip_list_remove(g_DeviceAutoUnlockList->device_auto_unlock_list, pos);
            device_auto_unlock_free(node);
            osip_free(node);
            node = NULL;
            continue;
        }

        if (1 == node->iStatus) /* ��Ҫ�Զ������� */
        {
            node->iDurationTimeCount--;

            if (node->iDurationTimeCount <= 0)
            {
                needproc.push_back(node);
            }
        }

        pos++;
    }

    CMS_GBL_SMUTEX_UNLOCK((struct osip_mutex*)g_DeviceAutoUnlockList->lock);

    /* ������Ҫ�Զ������� */
    while (!needproc.empty())
    {
        proc_node = (device_auto_unlock_t*) needproc.front();
        needproc.pop_front();

        if (NULL != proc_node)
        {
            iRet = device_auto_unlock_proc(proc_node->uDeviceIndex);

            if (iRet != 0)
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "scan_device_auto_unlock_list() device_auto_unlock_proc Error:DeviceIndex=%u, iRet=%d \r\n", node->uDeviceIndex, iRet);
            }
            else
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "scan_device_auto_unlock_list() device_auto_unlock_proc OK:DeviceIndex=%u, iRet=%d \r\n", node->uDeviceIndex, iRet);
            }

            proc_node->iStatus = 0;
        }
    }

    needproc.clear();

    return;
}

/*****************************************************************************
 �� �� ��  : device_auto_unlock_proc
 ��������  : �豸�Զ���������
 �������  : unsigned int uDeviceIndex
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��1��4�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int device_auto_unlock_proc(unsigned int uDeviceIndex)
{
    GBLogicDevice_info_t* pGBLogicDeviceInfo = NULL;

    if (uDeviceIndex <= 0)
    {
        return -1;
    }

    /* ��ȡ�豸��Ϣ */
    pGBLogicDeviceInfo = GBLogicDevice_info_find_by_device_index(uDeviceIndex);

    if (NULL != pGBLogicDeviceInfo)
    {
        pGBLogicDeviceInfo->lock_status = LOCK_STATUS_OFF;
        pGBLogicDeviceInfo->pLockUserInfo = NULL;
        pGBLogicDeviceInfo->pLockRouteInfo = NULL;

        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "��λ������ʱ, �Զ����������λ:�߼��豸ID=%s", pGBLogicDeviceInfo->device_id);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Device lock time out, auto unlock:User ID=%s, User IP=%s, Port=%d, DeviceID=%s", pGBLogicDeviceInfo->device_id);
    }
    else
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "device_auto_unlock_proc() Find Dest GBLogic Device Info Error, DeviceIndex=%u\r\n", uDeviceIndex);
        return -1;
    }

    return 0;
}

/*****************************************************************************
 �� �� ��  : ShowDeviceAutoUnLockTaskInfo
 ��������  : ��ʾ��λ�Զ�������Ϣ
 �������  : int sock
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2016��4��8��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
void ShowDeviceAutoUnLockTaskInfo(int sock)
{
    int i = 0;
    char strLine[] = "\r--------------------------------------\r\n";
    char strHead[] = "\rDeviceIndex  Status  DurationTimeCount\r\n";
    device_auto_unlock_t* node = NULL;
    char rbuf[128] = {0};

    if (sock > 0)
    {
        send(sock, strLine, strlen(strLine), 0);
        send(sock, strHead, strlen(strHead), 0);
    }

    if ((NULL == g_DeviceAutoUnlockList) || (NULL == g_DeviceAutoUnlockList->device_auto_unlock_list))
    {
        return;
    }

    CMS_GBL_SMUTEX_LOCK((struct osip_mutex*)g_DeviceAutoUnlockList->lock);

    if (osip_list_size(g_DeviceAutoUnlockList->device_auto_unlock_list) <= 0)
    {
        CMS_GBL_SMUTEX_UNLOCK((struct osip_mutex*)g_DeviceAutoUnlockList->lock);
        return;
    }

    for (i = 0; i < osip_list_size(g_DeviceAutoUnlockList->device_auto_unlock_list); i++)
    {
        node = (device_auto_unlock_t*)osip_list_get(g_DeviceAutoUnlockList->device_auto_unlock_list, i);

        if (NULL == node)
        {
            continue;
        }

        snprintf(rbuf, 128, "\r%-11u  %-6d  %-17d\r\n", node->uDeviceIndex, node->iStatus, node->iDurationTimeCount);

        if (sock > 0)
        {
            send(sock, rbuf, strlen(rbuf), 0);
        }
    }

    CMS_GBL_SMUTEX_UNLOCK((struct osip_mutex*)g_DeviceAutoUnlockList->lock);

    if (sock > 0)
    {
        send(sock, strLine, strlen(strLine), 0);
    }

    return;
}
#endif
