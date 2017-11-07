
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

#include "user/user_info_mgn.inc"

#include "service/cruise_srv_proc.inc"


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
cruise_srv_list_t* g_CruiseSrvList = NULL;    /* Ѳ��ҵ����� */
int db_CruiseSrvInfo_reload_mark = 0;         /* Ѳ��ҵ�����ݿ���±�ʶ:0:����Ҫ���£�1:��Ҫ�������ݿ� */

/*----------------------------------------------*
 * ��������                                     *
 *----------------------------------------------*/

/*----------------------------------------------*
 * �궨��                                       *
 *----------------------------------------------*/
#if DECS("Ѳ��ҵ�����")

/*****************************************************************************
 �� �� ��  : cruise_action_init
 ��������  : Ѳ�������ṹ��ʼ��
 �������  : cruise_action_t** cruise_action
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��4��3�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int cruise_action_init(cruise_action_t** cruise_action)
{
    *cruise_action = (cruise_action_t*)osip_malloc(sizeof(cruise_action_t));

    if (*cruise_action == NULL)
    {
        DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_DEBUG, "cruise_action_init() exit---: *cruise_srv Smalloc Error \r\n");
        return -1;
    }

    (*cruise_action)->device_index = 0;
    (*cruise_action)->iPresetID = 0;
    (*cruise_action)->iStatus = 0;
    (*cruise_action)->iLiveTime = 0;
    (*cruise_action)->iLiveTimeCount = 0;
    (*cruise_action)->del_mark = 0;

    return 0;
}

/*****************************************************************************
 �� �� ��  : cruise_action_free
 ��������  : Ѳ�������ṹ�ͷ�
 �������  : cruise_action_t* cruise_action
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��4��3�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
void cruise_action_free(cruise_action_t* cruise_action)
{
    if (cruise_action == NULL)
    {
        //DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_DEBUG, "cruise_action_free() exit---: Param Error \r\n");
        return;
    }

    cruise_action->device_index = 0;
    cruise_action->iPresetID = 0;
    cruise_action->iStatus = 0;
    cruise_action->iLiveTime = 0;
    cruise_action->iLiveTimeCount = 0;
    cruise_action->del_mark = 0;

    osip_free(cruise_action);
    cruise_action = NULL;

    return;
}

/*****************************************************************************
 �� �� ��  : cruise_action_find
 ��������  : �����߼��豸������Ԥ��λ����Ѳ������
 �������  : unsigned int device_index
             unsigned int iPresetID
             osip_list_t* pCruiseActionList
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��4��3�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int cruise_action_find(unsigned int device_index, unsigned int iPresetID, osip_list_t* pCruiseActionList)
{
    int i = 0;
    cruise_action_t* pCruiseAction = NULL;

    if (device_index <= 0 || NULL == pCruiseActionList)
    {
        DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_DEBUG, "cruise_action_find() exit---: Cruise Action List Error \r\n");
        return -1;
    }

    for (i = 0; i < osip_list_size(pCruiseActionList); i++)
    {
        pCruiseAction = (cruise_action_t*)osip_list_get(pCruiseActionList, i);

        if (NULL == pCruiseAction || pCruiseAction->device_index <= 0)
        {
            continue;
        }

        if (pCruiseAction->device_index == device_index
            && pCruiseAction->iPresetID == iPresetID)
        {
            return i;
        }
    }

    return -1;
}

/*****************************************************************************
 �� �� ��  : cruise_action_get
 ��������  : Ѳ��������ȡ
 �������  : int pos
                            osip_list_t* pCruiseActionList
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��4��3�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
cruise_action_t* cruise_action_get(int pos, osip_list_t* pCruiseActionList)
{
    cruise_action_t* pCruiseAction = NULL;

    if (NULL == pCruiseActionList)
    {
        DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_DEBUG, "cruise_action_find() exit---: Cruise Action List Error \r\n");
        return NULL;
    }

    if (pos < 0 || (pos >= osip_list_size(pCruiseActionList)))
    {
        DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_DEBUG, "cruise_action_find() exit---: Pos Error \r\n");
        return NULL;
    }

    pCruiseAction = (cruise_action_t*)osip_list_get(pCruiseActionList, pos);

    if (NULL == pCruiseAction)
    {
        return NULL;
    }
    else
    {
        return pCruiseAction;
    }
}

/*****************************************************************************
 �� �� ��  : cruise_srv_init
 ��������  : Ѳ��ҵ��ṹ��ʼ��
 �������  : cruise_srv_t ** cruise_srv
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��11��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int cruise_srv_init(cruise_srv_t** cruise_srv)
{
    *cruise_srv = (cruise_srv_t*)osip_malloc(sizeof(cruise_srv_t));

    if (*cruise_srv == NULL)
    {
        DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_DEBUG, "cruise_srv_init() exit---: *cruise_srv Smalloc Error \r\n");
        return -1;
    }

    (*cruise_srv)->cruise_id = 0;
    (*cruise_srv)->status = 0;
    (*cruise_srv)->cruise_type = 0;
    (*cruise_srv)->cruise_name[0] = '\0';
    (*cruise_srv)->start_time = 0;
    (*cruise_srv)->duration_time = 0;
    (*cruise_srv)->duration_time_count = 0;
    (*cruise_srv)->current_pos = 0;

    (*cruise_srv)->del_mark = 0;
    (*cruise_srv)->send_mark = 0;

    (*cruise_srv)->pCruiseActionList = (osip_list_t*)osip_malloc(sizeof(osip_list_t));

    if (NULL == (*cruise_srv)->pCruiseActionList)
    {
        osip_free(*cruise_srv);
        *cruise_srv = NULL;
        DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_DEBUG, "cruise_srv_init() exit---: Cruise Action List Init Error \r\n");
        return -1;
    }

    osip_list_init((*cruise_srv)->pCruiseActionList);

    return 0;
}

/*****************************************************************************
 �� �� ��  : cruise_srv_free
 ��������  : Ѳ��ҵ��ṹ�ͷ�
 �������  : cruise_srv_t * cruise_srv
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��11��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void cruise_srv_free(cruise_srv_t* cruise_srv)
{
    if (cruise_srv == NULL)
    {
        //DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_DEBUG, "cruise_srv_free() exit---: Param Error \r\n");
        return;
    }

    cruise_srv->cruise_id = 0;
    cruise_srv->status = 0;
    cruise_srv->cruise_type = 0;

    memset(cruise_srv->cruise_name, 0, MAX_128CHAR_STRING_LEN + 4);

    cruise_srv->start_time = 0;
    cruise_srv->duration_time = 0;
    cruise_srv->duration_time_count = 0;
    cruise_srv->current_pos = 0;

    cruise_srv->del_mark = 0;
    cruise_srv->send_mark = 0;

    if (NULL != cruise_srv->pCruiseActionList)
    {
        osip_list_special_free(cruise_srv->pCruiseActionList, (void (*)(void*))&cruise_action_free);
        osip_free(cruise_srv->pCruiseActionList);
        cruise_srv->pCruiseActionList = NULL;
    }

    osip_free(cruise_srv);
    cruise_srv = NULL;

    return;
}

/*****************************************************************************
 �� �� ��  : cruise_srv_find
 ��������  : ����ID����Ѳ��ҵ��
 �������  : unsigned int id
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��10��17��
    ��    ��   : �û�·����Ϣ����
    �޸�����   : �����ɺ���

*****************************************************************************/
int cruise_srv_find(unsigned int id)
{
    int i = 0;
    cruise_srv_t* pCruiseSrv = NULL;

    if (id <= 0 || NULL == g_CruiseSrvList || NULL == g_CruiseSrvList->pCruiseSrvList)
    {
        //DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_DEBUG, "cruise_srv_find() exit---: Cruise Action List Error \r\n");
        return -1;
    }

    CRUISE_SMUTEX_LOCK();

    for (i = 0; i < osip_list_size(g_CruiseSrvList->pCruiseSrvList); i++)
    {
        pCruiseSrv = (cruise_srv_t*)osip_list_get(g_CruiseSrvList->pCruiseSrvList, i);

        if (NULL == pCruiseSrv || pCruiseSrv->cruise_id <= 0)
        {
            continue;
        }

        if (pCruiseSrv->cruise_id == id)
        {
            CRUISE_SMUTEX_UNLOCK();
            return i;
        }
    }

    CRUISE_SMUTEX_UNLOCK();
    return -1;
}

/*****************************************************************************
 �� �� ��  : cruise_srv_get
 ��������  : ��ȡѲ��ҵ��
 �������  : int pos
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��10��17��
    ��    ��   : �û�·����Ϣ����
    �޸�����   : �����ɺ���

*****************************************************************************/
cruise_srv_t* cruise_srv_get(int pos)
{
    cruise_srv_t* pCruiseSrv = NULL;

    if (NULL == g_CruiseSrvList || NULL == g_CruiseSrvList->pCruiseSrvList)
    {
        DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_DEBUG, "cruise_srv_get() exit---: Cruise Action List Error \r\n");
        return NULL;
    }

    if (pos < 0 || (pos >= osip_list_size(g_CruiseSrvList->pCruiseSrvList)))
    {
        DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_DEBUG, "cruise_srv_get() exit---: Pos Error \r\n");
        return NULL;
    }

    pCruiseSrv = (cruise_srv_t*)osip_list_get(g_CruiseSrvList->pCruiseSrvList, pos);

    if (NULL == pCruiseSrv)
    {
        return NULL;
    }
    else
    {
        return pCruiseSrv;
    }
}

/*****************************************************************************
 �� �� ��  : cruise_srv_list_init
 ��������  : Ѳ��ҵ����г�ʼ��
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
int cruise_srv_list_init()
{
    g_CruiseSrvList = (cruise_srv_list_t*)osip_malloc(sizeof(cruise_srv_list_t));

    if (g_CruiseSrvList == NULL)
    {
        DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_DEBUG, "cruise_srv_list_init() exit---: g_CruiseSrvList Smalloc Error \r\n");
        return -1;
    }

    g_CruiseSrvList->pCruiseSrvList = (osip_list_t*)osip_malloc(sizeof(osip_list_t));

    if (NULL == g_CruiseSrvList->pCruiseSrvList)
    {
        osip_free(g_CruiseSrvList);
        g_CruiseSrvList = NULL;
        DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_DEBUG, "cruise_srv_list_init() exit---: Cruise Srv List Init Error \r\n");
        return -1;
    }

    osip_list_init(g_CruiseSrvList->pCruiseSrvList);

#ifdef MULTI_THR
    /* init smutex */
    g_CruiseSrvList->lock = (osip_mutex_t*)osip_mutex_init();

    if (NULL == g_CruiseSrvList->lock)
    {
        osip_free(g_CruiseSrvList->pCruiseSrvList);
        g_CruiseSrvList->pCruiseSrvList = NULL;
        osip_free(g_CruiseSrvList);
        g_CruiseSrvList = NULL;
        DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_DEBUG, "cruise_srv_list_init() exit---: Cruise Srv List Lock Init Error \r\n");
        return -1;
    }

#endif
    return 0;
}

/*****************************************************************************
 �� �� ��  : cruise_srv_list_free
 ��������  : Ѳ��ҵ������ͷ�
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
void cruise_srv_list_free()
{
    if (NULL == g_CruiseSrvList)
    {
        DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_DEBUG, "cruise_srv_list_free() exit---: Param Error \r\n");
        return;
    }

    if (NULL != g_CruiseSrvList->pCruiseSrvList)
    {
        osip_list_special_free(g_CruiseSrvList->pCruiseSrvList, (void (*)(void*))&cruise_srv_free);
        osip_free(g_CruiseSrvList->pCruiseSrvList);
        g_CruiseSrvList->pCruiseSrvList = NULL;
    }

#ifdef MULTI_THR

    if (NULL != g_CruiseSrvList->lock)
    {
        osip_mutex_destroy((struct osip_mutex*)g_CruiseSrvList->lock);
        g_CruiseSrvList->lock = NULL;
    }

#endif
    osip_free(g_CruiseSrvList);
    g_CruiseSrvList = NULL;
    return;
}

/*****************************************************************************
 �� �� ��  : cruise_srv_list_lock
 ��������  : Ѳ��ҵ���������
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
int cruise_srv_list_lock()
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_CruiseSrvList == NULL || g_CruiseSrvList->lock == NULL)
    {
        //DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_DEBUG, "cruise_srv_list_lock() exit---: Param Error \r\n");
        return -1;
    }

    iRet = osip_mutex_lock((struct osip_mutex*)g_CruiseSrvList->lock);
#endif
    return iRet;
}

/*****************************************************************************
 �� �� ��  : cruise_srv_list_unlock
 ��������  : Ѳ��ҵ�����
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
int cruise_srv_list_unlock()
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_CruiseSrvList == NULL || g_CruiseSrvList->lock == NULL)
    {
        //DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_DEBUG, "cruise_srv_list_unlock() exit---: Param Error \r\n");
        return -1;
    }

    iRet = osip_mutex_unlock((struct osip_mutex*)g_CruiseSrvList->lock);

#endif
    return iRet;
}

/*****************************************************************************
 �� �� ��  : debug_cruise_srv_list_lock
 ��������  : Ѳ��ҵ���������
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
int debug_cruise_srv_list_lock(const char* file, int line, const char* func)
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_CruiseSrvList == NULL || g_CruiseSrvList->lock == NULL)
    {
        DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_DEBUG, "debug_cruise_srv_list_lock() exit---: Param Error \r\n");
        return -1;
    }

    iRet = osip_debug_mutex_lock((struct osip_mutex*)g_CruiseSrvList->lock, file, line, func);

#endif
    return iRet;
}

/*****************************************************************************
 �� �� ��  : debug_cruise_srv_list_unlock
 ��������  : Ѳ��ҵ�����
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
int debug_cruise_srv_list_unlock(const char* file, int line, const char* func)
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_CruiseSrvList == NULL || g_CruiseSrvList->lock == NULL)
    {
        DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_DEBUG, "debug_cruise_srv_list_unlock() exit---: Param Error \r\n");
        return -1;
    }

    iRet = osip_debug_mutex_unlock((struct osip_mutex*)g_CruiseSrvList->lock, file, line, func);

#endif
    return iRet;
}

/*****************************************************************************
 �� �� ��  : cruise_srv_add
 ��������  : Ѳ��ҵ�����
 �������  : cruise_srv_t* pCruiseSrv
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��4��3�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int cruise_srv_add(cruise_srv_t* pCruiseSrv)
{
    int i = 0;

    if (pCruiseSrv == NULL)
    {
        //DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_DEBUG, "cruise_srv_add() exit---: Param Error \r\n");
        return -1;
    }

    CRUISE_SMUTEX_LOCK();

    i = osip_list_add(g_CruiseSrvList->pCruiseSrvList, pCruiseSrv, -1); /* add to list tail */

    //DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_TRACE, "cruise_srv_add() CruiseSrv:CruiseID=%u, StartTime=%d, DurationTime=%d, i=%d \r\n", pCruiseSrv->cruise_id, pCruiseSrv->start_time, pCruiseSrv->duration_time, i);

    if (i < 0)
    {
        CRUISE_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_ERROR, "cruise_srv_add() exit---: List Add Error \r\n");
        return -1;
    }

    CRUISE_SMUTEX_UNLOCK();
    return i - 1;
}

/*****************************************************************************
 �� �� ��  : cruise_srv_remove
 ��������  : �Ӷ������Ƴ�Ѳ��ҵ��
 �������  : int pos
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��11��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int cruise_srv_remove(int pos)
{
    cruise_srv_t* pCruiseSrv = NULL;

    CRUISE_SMUTEX_LOCK();

    if (g_CruiseSrvList == NULL || pos < 0 || (pos >= osip_list_size(g_CruiseSrvList->pCruiseSrvList)))
    {
        //DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_DEBUG, "cruise_srv_remove() exit---: Param Error \r\n");
        CRUISE_SMUTEX_UNLOCK();
        return -1;
    }

    pCruiseSrv = (cruise_srv_t*)osip_list_get(g_CruiseSrvList->pCruiseSrvList, pos);

    if (NULL == pCruiseSrv)
    {
        //DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_ERROR, "cruise_srv_remove() exit---: List Get Error \r\n");
        CRUISE_SMUTEX_UNLOCK();
        return -1;
    }

    osip_list_remove(g_CruiseSrvList->pCruiseSrvList, pos);
    cruise_srv_free(pCruiseSrv);
    pCruiseSrv = NULL;
    CRUISE_SMUTEX_UNLOCK();
    return 0;
}

/*****************************************************************************
 �� �� ��  : scan_cruise_srv_list
 ��������  : ɨ��Ѳ��ҵ����Ϣ����
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
void scan_cruise_srv_list(DBOper* pCruise_Srv_dboper)
{
    int i = 0;
    int iRet = 0;
    cruise_srv_t* pCruiseSrv = NULL;
    needtoproc_cruisesrv_queue needToProc;
    needtoproc_cruisesrv_queue needToStop;
    needtoproc_cruisesrv_queue needToSend;

    if ((NULL == g_CruiseSrvList) || (NULL == g_CruiseSrvList->pCruiseSrvList))
    {
        //DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_DEBUG, "scan_cruise_srv_list() exit---: Param Error \r\n");
        return;
    }

    needToProc.clear();
    needToStop.clear();
    needToSend.clear();

    CRUISE_SMUTEX_LOCK();

    if (osip_list_size(g_CruiseSrvList->pCruiseSrvList) <= 0)
    {
        CRUISE_SMUTEX_UNLOCK();
        return;
    }

    for (i = 0; i < osip_list_size(g_CruiseSrvList->pCruiseSrvList); i++)
    {
        pCruiseSrv = (cruise_srv_t*)osip_list_get(g_CruiseSrvList->pCruiseSrvList, i);

        if (NULL == pCruiseSrv)
        {
            continue;
        }

        //DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_TRACE, "scan_cruise_srv_list() cruise_id=%u, CruiseSrv: StartTime=%d, DurationTime=%d, DurationTimeCount=%d, DelMark=%d, Status=%d \r\n", pCruiseSrv->cruise_id, pCruiseSrv->start_time, pCruiseSrv->duration_time, pCruiseSrv->duration_time_count, pCruiseSrv->del_mark, pCruiseSrv->status);

        if (1 == pCruiseSrv->del_mark) /* Ҫɾ�������� */
        {
            if (1 == pCruiseSrv->status || 4 == pCruiseSrv->status)
            {
                needToStop.push_back(pCruiseSrv);
                pCruiseSrv->duration_time_count = 0;
            }
            else
            {
                continue;
            }
        }

        if (2 == pCruiseSrv->status) /* ��Ҫ����Ѳ�� */
        {
            cruise_action_release(pCruiseSrv);
            pCruiseSrv->status = 1;
            pCruiseSrv->duration_time_count = 0;
        }
        else if (3 == pCruiseSrv->status)  /* ��ҪֹͣѲ�� */
        {
            needToStop.push_back(pCruiseSrv);
            pCruiseSrv->duration_time_count = 0;
        }
        else if (4 == pCruiseSrv->status)  /* ��Ҫ����֪ͨ���ͻ��� */
        {
            needToSend.push_back(pCruiseSrv);
            pCruiseSrv->status = 1;
        }

        if (1 == pCruiseSrv->status)  /* ���Ƿ�ҪֹͣѲ�� */
        {
            if (0 != pCruiseSrv->cruise_type) /* Ԥ��ִ�е���Ѳ */
            {
                needToProc.push_back(pCruiseSrv);
            }
            else
            {
                if (pCruiseSrv->duration_time > 0 && pCruiseSrv->duration_time_count > 0
                    && pCruiseSrv->duration_time_count >= pCruiseSrv->duration_time)
                {
                    needToStop.push_back(pCruiseSrv);
                    pCruiseSrv->duration_time_count = 0;
                }
                else
                {
                    needToProc.push_back(pCruiseSrv);

                    if (pCruiseSrv->duration_time > 0)
                    {
                        pCruiseSrv->duration_time_count++;
                    }
                }
            }
        }
    }

    CRUISE_SMUTEX_UNLOCK();

    /* ������Ҫ��ʼ�� */
    while (!needToProc.empty())
    {
        pCruiseSrv = (cruise_srv_t*) needToProc.front();
        needToProc.pop_front();

        if (NULL != pCruiseSrv)
        {
            iRet = cruise_action_proc(pCruiseSrv);

            if (iRet < 0)
            {
                DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_ERROR, "scan_cruise_srv_list() cruise_action_proc Error:cruise_id=%u, iRet=%d \r\n", pCruiseSrv->cruise_id, iRet);
            }
            else
            {
                DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_TRACE, "scan_cruise_srv_list() cruise_action_proc OK:cruise_id=%u, iRet=%d \r\n", pCruiseSrv->cruise_id, iRet);
            }

            if (0 == pCruiseSrv->send_mark)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "ִ��Ѳ�����Ϳ�ʼѲ��:CruiseID=%u, CruiseName=%s", pCruiseSrv->cruise_id, pCruiseSrv->cruise_name);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Send notify execute cruise action:CruiseID=%u, CruiseName=%s", pCruiseSrv->cruise_id, pCruiseSrv->cruise_name);

                /* ֪ͨ�ͻ��� */
                iRet = SendNotifyExecuteCruiseActionToOnlineUser(pCruiseSrv->cruise_id, pCruiseSrv->cruise_name, 0, pCruise_Srv_dboper);

                if (iRet < 0)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "ִ��Ѳ�����Ϳ�ʼѲ��ʧ��:CruiseID=%u, CruiseName=%s", pCruiseSrv->cruise_id, pCruiseSrv->cruise_name);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Fail to send notify execute cruise action:CruiseID=%u, CruiseName=%s", pCruiseSrv->cruise_id, pCruiseSrv->cruise_name);
                    DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_ERROR, "scan_cruise_srv_list() SendNotifyExecuteCruiseActionToOnlineUser Start Error: cruise_id=%u, iRet=%d \r\n", pCruiseSrv->cruise_id, iRet);
                }
                else if (iRet > 0)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "ִ��Ѳ�����Ϳ�ʼѲ���ɹ�:CruiseID=%u, CruiseName=%s", pCruiseSrv->cruise_id, pCruiseSrv->cruise_name);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Succeed to send notify execute cruise action:CruiseID=%u, CruiseName=%s", pCruiseSrv->cruise_id, pCruiseSrv->cruise_name);
                    DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_TRACE, "scan_cruise_srv_list() SendNotifyExecuteCruiseActionToOnlineUser Start OK: cruise_id=%u, iRet=%d \r\n", pCruiseSrv->cruise_id, iRet);
                }

                pCruiseSrv->send_mark = 1;

                /* ����״̬ */
                iRet = UpdateCruiseConfigStatus2DB(pCruiseSrv->cruise_id, 1, pCruise_Srv_dboper);

                if (iRet < 0)
                {
                    DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_ERROR, "scan_cruise_srv_list() UpdateCruiseConfigStatus2DB Start Error: cruise_id=%u, iRet=%d \r\n", pCruiseSrv->cruise_id, iRet);
                }
                else
                {
                    DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_TRACE, "scan_cruise_srv_list() UpdateCruiseConfigStatus2DB Start OK: cruise_id=%u, iRet=%d \r\n", pCruiseSrv->cruise_id, iRet);
                }
            }
        }
    }

    needToProc.clear();

    /* ������Ҫ���͵� */
    while (!needToSend.empty())
    {
        pCruiseSrv = (cruise_srv_t*) needToSend.front();
        needToSend.pop_front();

        if (NULL != pCruiseSrv)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "ִ��Ѳ�����Ϳ�ʼѲ��:CruiseID=%u, CruiseName=%s", pCruiseSrv->cruise_id, pCruiseSrv->cruise_name);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Send notify execute cruise action:CruiseID=%u, CruiseName=%s", pCruiseSrv->cruise_id, pCruiseSrv->cruise_name);


            /* ֪ͨ�ͻ��� */
            iRet = SendNotifyExecuteCruiseActionToOnlineUser(pCruiseSrv->cruise_id, pCruiseSrv->cruise_name, 0, pCruise_Srv_dboper);

            if (iRet < 0)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "ִ��Ѳ�����Ϳ�ʼѲ��ʧ��:CruiseID=%u, CruiseName=%s", pCruiseSrv->cruise_id, pCruiseSrv->cruise_name);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Fail to send notify execute cruise action:CruiseID=%u, CruiseName=%s", pCruiseSrv->cruise_id, pCruiseSrv->cruise_name);
                DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_ERROR, "scan_cruise_srv_list() SendNotifyExecuteCruiseActionToOnlineUser Start Error: cruise_id=%u, iRet=%d \r\n", pCruiseSrv->cruise_id, iRet);
            }
            else if (iRet > 0)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "ִ��Ѳ�����Ϳ�ʼѲ���ɹ�:CruiseID=%u, CruiseName=%s", pCruiseSrv->cruise_id, pCruiseSrv->cruise_name);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Succeed to send notify execute cruise action:CruiseID=%u, CruiseName=%s", pCruiseSrv->cruise_id, pCruiseSrv->cruise_name);
                DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_TRACE, "scan_cruise_srv_list() SendNotifyExecuteCruiseActionToOnlineUser Start OK: cruise_id=%u, iRet=%d \r\n", pCruiseSrv->cruise_id, iRet);
            }
        }
    }

    needToSend.clear();

    /* ������Ҫֹͣ�� */
    while (!needToStop.empty())
    {
        pCruiseSrv = (cruise_srv_t*) needToStop.front();
        needToStop.pop_front();

        if (NULL != pCruiseSrv)
        {
            iRet = cruise_action_stop(pCruiseSrv);

            if (iRet < 0)
            {
                DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_ERROR, "scan_cruise_srv_list() cruise_action_stop Error:cruise_id=%u, iRet=%d \r\n", pCruiseSrv->cruise_id, iRet);
            }
            else
            {
                DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_TRACE, "scan_cruise_srv_list() cruise_action_stop OK:cruise_id=%u, iRet=%d \r\n", pCruiseSrv->cruise_id, iRet);
            }

            if (iRet >= 0)
            {
                pCruiseSrv->status = 0;
                pCruiseSrv->duration_time_count = 0;

                if (iRet > 0)
                {
                    if (1 == pCruiseSrv->send_mark) /* �û��ֶ�ֹͣ�Ĳ���Ҫ���͸��ͻ��� */
                    {
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "ִ��Ѳ������ֹͣѲ��:CruiseID=%u, CruiseName=%s", pCruiseSrv->cruise_id, pCruiseSrv->cruise_name);
                        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Send notify stop cruise action:CruiseID=%u, CruiseName=%s", pCruiseSrv->cruise_id, pCruiseSrv->cruise_name);

                        /* ֪ͨ�ͻ��� */
                        iRet = SendNotifyExecuteCruiseActionToOnlineUser(pCruiseSrv->cruise_id, pCruiseSrv->cruise_name, 1, pCruise_Srv_dboper);

                        if (iRet < 0)
                        {
                            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "ִ��Ѳ������ֹͣѲ��ʧ��:CruiseID=%u, CruiseName=%s", pCruiseSrv->cruise_id, pCruiseSrv->cruise_name);
                            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Fail to send notify stop cruise action:CruiseID=%u, CruiseName=%s", pCruiseSrv->cruise_id, pCruiseSrv->cruise_name);
                            DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_ERROR, "scan_cruise_srv_list() SendNotifyExecuteCruiseActionToOnlineUser Start Error: cruise_id=%u, iRet=%d \r\n", pCruiseSrv->cruise_id, iRet);
                        }
                        else if (iRet > 0)
                        {
                            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "ִ��Ѳ������ֹͣѲ���ɹ�:CruiseID=%u, CruiseName=%s", pCruiseSrv->cruise_id, pCruiseSrv->cruise_name);
                            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Succeed to send notify stop cruise action:CruiseID=%u, CruiseName=%s", pCruiseSrv->cruise_id, pCruiseSrv->cruise_name);
                            DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_TRACE, "scan_cruise_srv_list() SendNotifyExecuteCruiseActionToOnlineUser Start OK: cruise_id=%u, iRet=%d \r\n", pCruiseSrv->cruise_id, iRet);
                        }
                    }
                }

                /* ����״̬ */
                iRet = UpdateCruiseConfigStatus2DB(pCruiseSrv->cruise_id, 0, pCruise_Srv_dboper);

                if (iRet < 0)
                {
                    DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_ERROR, "scan_cruise_srv_list() UpdateCruiseConfigStatus2DB Start Error: cruise_id=%u, iRet=%d \r\n", pCruiseSrv->cruise_id, iRet);
                }
                else
                {
                    DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_TRACE, "scan_cruise_srv_list() UpdateCruiseConfigStatus2DB Start OK: cruise_id=%u, iRet=%d \r\n", pCruiseSrv->cruise_id, iRet);
                }

                pCruiseSrv->send_mark = 0;
            }
        }
    }

    needToStop.clear();

    return;
}
#endif

/*****************************************************************************
 �� �� ��  : cruise_action_release
 ��������  : Ѳ�������ͷ�
 �������  : cruise_srv_t* pCruiseSrv
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2016��6��30��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int cruise_action_release(cruise_srv_t* pCruiseSrv)
{
    int i = 0;
    cruise_action_t* pCruiseAction = NULL;

    if (NULL == pCruiseSrv || NULL == pCruiseSrv->pCruiseActionList)
    {
        return -1;
    }

    for (i = 0; i < osip_list_size(pCruiseSrv->pCruiseActionList); i++)
    {
        pCruiseAction = (cruise_action_t*)osip_list_get(pCruiseSrv->pCruiseActionList, i);

        if (NULL == pCruiseAction)
        {
            continue;
        }

        if (1 == pCruiseAction->iStatus)
        {
            pCruiseAction->iStatus = 0;
            pCruiseAction->iLiveTimeCount = 0;
        }
    }

    pCruiseSrv->current_pos = 0;

    return 0;
}

/*****************************************************************************
 �� �� ��  : cruise_action_stop
 ��������  : ֹͣԤ������
 �������  : cruise_srv_t* pCruiseSrv
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��4��2�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int cruise_action_stop(cruise_srv_t* pCruiseSrv)
{
    int i = 0;
    cruise_action_t* pCruiseAction = NULL;
    int iNotifyUserFlag = 1;

    if (NULL == pCruiseSrv || NULL == pCruiseSrv->pCruiseActionList)
    {
        DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_DEBUG, "cruise_action_stop() exit---: Cruise Action List Error \r\n");
        return -1;
    }

    for (i = 0; i < osip_list_size(pCruiseSrv->pCruiseActionList); i++)
    {
        pCruiseAction = (cruise_action_t*)osip_list_get(pCruiseSrv->pCruiseActionList, i);

        if (NULL == pCruiseAction)
        {
            continue;
        }

        DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_TRACE, "cruise_action_stop() CruiseAction: CruiseID=%u, CurrentPos=%d, device_index=%u, PresetID=%d \r\n", pCruiseSrv->cruise_id, pCruiseSrv->current_pos, pCruiseAction->device_index, pCruiseAction->iPresetID);

        if (1 == pCruiseAction->iStatus)
        {
            pCruiseAction->iStatus = 0;
            pCruiseAction->iLiveTimeCount = 0;
        }
    }

    pCruiseSrv->current_pos = 0;

    return iNotifyUserFlag;
}

/*****************************************************************************
 �� �� ��  : cruise_action_proc
 ��������  : ִ��Ԥ������
 �������  : cruise_srv_t* pCruiseSrv
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��4��2�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int cruise_action_proc(cruise_srv_t* pCruiseSrv)
{
    int iRet = 0;
    int i = 0;
    cruise_action_t* pCruiseAction = NULL;
    int iNotifyUserFlag = 0;

    if (NULL == pCruiseSrv || NULL == pCruiseSrv->pCruiseActionList)
    {
        DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_DEBUG, "cruise_action_proc() exit---: Cruise Action List Error \r\n");
        return -1;
    }

    for (i = 0; i < osip_list_size(pCruiseSrv->pCruiseActionList); i++)
    {
        pCruiseAction = (cruise_action_t*)osip_list_get(pCruiseSrv->pCruiseActionList, i);

        if (NULL == pCruiseAction)
        {
            DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_ERROR, "cruise_action_proc() Get CruiseActionPreset Error\r\n");
            pCruiseSrv->current_pos++;
            continue;
        }

        DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_TRACE, "cruise_action_proc() cruise_id=%d, current_pos=%d, DeviceIndex=%u, PresetID=%d, LiveTime=%d, LiveTimeCount=%d, Status=%d, del_mark=%d \r\n", pCruiseSrv->cruise_id, pCruiseSrv->current_pos, pCruiseAction->device_index, pCruiseAction->iPresetID, pCruiseAction->iLiveTime, pCruiseAction->iLiveTimeCount, pCruiseAction->iStatus, pCruiseAction->del_mark);

        if (1 == pCruiseAction->del_mark) /* ��Ҫɾ���ģ����Ƿ���Ҫֹͣ */
        {
            if (i == pCruiseSrv->current_pos)
            {
                if (1 == pCruiseAction->iStatus)
                {
                    pCruiseAction->iLiveTimeCount = 0;
                    pCruiseAction->iStatus = 0;
                    pCruiseSrv->current_pos = i + 1;
                    iNotifyUserFlag = 2;
                }
            }

            if (pCruiseSrv->current_pos >= osip_list_size(pCruiseSrv->pCruiseActionList))
            {
                DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_TRACE, "cruise_action_proc() CurrentPos=%d \r\n", pCruiseSrv->current_pos, iRet);
                pCruiseSrv->current_pos = 0;
            }
        }
        else
        {
            if (i == pCruiseSrv->current_pos) /* ���ڵ�ǰѲ�� */
            {
                if (0 == pCruiseAction->iStatus)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "ִ��Ѳ��: DeviceIndex=%u, PresetID=%u", pCruiseAction->device_index, pCruiseAction->iPresetID);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Launch navigation: DeviceIndex=%u, PresetID=%u", pCruiseAction->device_index, pCruiseAction->iPresetID);

                    iRet = ExecuteDevicePresetByPresetIDAndDeviceIndex(pCruiseAction->iPresetID, pCruiseAction->device_index, &g_DBOper);

                    if (iRet < 0)
                    {
                        DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_ERROR, "cruise_action_proc() ExecuteDevicePresetByPresetIDAndDeviceIndex Error: DeviceIndex=%u, PresetID=%u, CurrentPos=%d, iRet=%d\r\n", pCruiseAction->device_index, pCruiseAction->iPresetID, pCruiseSrv->current_pos, iRet);
                    }
                    else
                    {
                        DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_TRACE, "cruise_action_proc() ExecuteDevicePresetByPresetIDAndDeviceIndex OK: DeviceIndex=%u, PresetID=%u, CurrentPos=%d, iRet=%d\r\n", pCruiseAction->device_index, pCruiseAction->iPresetID, pCruiseSrv->current_pos, iRet);
                    }

                    pCruiseAction->iStatus = 1;
                    iNotifyUserFlag = 1;
                }
                else
                {
                    pCruiseAction->iLiveTimeCount++;
                    DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_TRACE, "cruise_action_proc() DeviceIndex=%u, PresetID=%u, LiveTimeCount=%d, CurrentPos=%d\r\n", pCruiseAction->device_index, pCruiseAction->iPresetID, pCruiseAction->iLiveTimeCount, pCruiseSrv->current_pos);
                }

                if (pCruiseAction->iLiveTimeCount >= pCruiseAction->iLiveTime)
                {
                    pCruiseAction->iLiveTimeCount = 0;
                    pCruiseSrv->current_pos = i + 1;
                    pCruiseAction->iStatus = 0;
                    iNotifyUserFlag = 2;
                }
            }

            if (pCruiseSrv->current_pos >= osip_list_size(pCruiseSrv->pCruiseActionList))
            {
                DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_TRACE, "cruise_action_proc() CurrentPos=%d \r\n", pCruiseSrv->current_pos, iRet);
                pCruiseSrv->current_pos = 0;

                /* �ٴ�������Ԥ���жϵ�Ѳ�� */
                if (1 == pCruiseSrv->cruise_type) /* Ԥ��ִ�е�Ѳ����ִ��һ��Ͳ�Ҫִ����,��Ҫֹͣ */
                {
                    pCruiseSrv->status = 3;
                }
                else if (2 == pCruiseSrv->cruise_type) /* ��Ԥ���жϵ�Ѳ������Ҫ�ٴ����� */
                {
                    pCruiseSrv->status = 2;
                    pCruiseSrv->cruise_type = 0;
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "������Ԥ���жϵ�Ѳ��: Ѳ��ID=%u, Ѳ������=%s, ��ʼʱ��=%d, ����ʱ��=%d", pCruiseSrv->cruise_id, pCruiseSrv->cruise_name, pCruiseSrv->start_time, pCruiseSrv->duration_time);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Start cruise that plan interrupt : cruise ID=%u, cruise name=%s, start time=%d, during time=%d", pCruiseSrv->cruise_id, pCruiseSrv->cruise_name, pCruiseSrv->start_time, pCruiseSrv->duration_time);
                }
                else if (3 == pCruiseSrv->cruise_type) /* Ԥ��ִ�е�Ѳ�������û�ִ��֮����Ҫ�ٴ����� */
                {
                    pCruiseSrv->status = 2;
                    pCruiseSrv->cruise_type = 0;
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�������û�ִ�е�Ԥ��Ѳ��: Ѳ��ID=%u, Ѳ������=%s, ��ʼʱ��=%d, ����ʱ��=%d", pCruiseSrv->cruise_id, pCruiseSrv->cruise_name, pCruiseSrv->start_time, pCruiseSrv->duration_time);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Start cruise that users interrupt: cruise ID=%u, cruise name=%s, start time=%d, during time=%d", pCruiseSrv->cruise_id, pCruiseSrv->cruise_name, pCruiseSrv->start_time, pCruiseSrv->duration_time);
                }
            }
        }
    }

    return iNotifyUserFlag;
}

/*****************************************************************************
 �� �� ��  : start_cruise_srv_by_id
 ��������  : ����ID����Ѳ������
 �������  : user_info_t* pUserInfo
             int id
             DBOper* pCruise_Srv_dboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��4��2�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int start_cruise_srv_by_id(user_info_t* pUserInfo, unsigned int id, DBOper* pCruise_Srv_dboper)
{
    int i = 0;
    int iRet = 0;
    int cruise_pos = -1;
    string strSQL = "";
    int record_count = 0;
    cruise_srv_t* pCruiseSrv = NULL;
    char strCruiseID[32] = {0};

    if (NULL == pCruise_Srv_dboper || NULL == pUserInfo)
    {
        return -1;
    }

    cruise_pos = cruise_srv_find(id);

    DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_TRACE, "start_cruise_srv_by_id() cruise_srv_find:id=%u, cruise_pos=%d \r\n", id, cruise_pos);

    if (cruise_pos < 0)
    {
        snprintf(strCruiseID, 32, "%u", id);
        strSQL.clear();
        strSQL = "select * from CruiseConfig WHERE ID = ";
        strSQL += strCruiseID;

        record_count = pCruise_Srv_dboper->DB_Select(strSQL.c_str(), 1);

        if (record_count < 0)
        {
            DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_ERROR, "start_cruise_srv_by_id() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
            DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_ERROR, "start_cruise_srv_by_id() ErrorMsg=%s\r\n", pCruise_Srv_dboper->GetLastDbErrorMsg());
            return -1;
        }
        else if (record_count == 0)
        {
            DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_WARN, "start_cruise_srv_by_id() exit---: No Record Count \r\n");
            return 0;
        }


        unsigned int uCruiseID = 0;
        string strCruiseName = "";
        int iStartTime = 0;
        int iDurationTime = 0;

        pCruise_Srv_dboper->GetFieldValue("ID", uCruiseID);
        pCruise_Srv_dboper->GetFieldValue("CruiseName", strCruiseName);
        pCruise_Srv_dboper->GetFieldValue("StartTime", iStartTime);
        pCruise_Srv_dboper->GetFieldValue("DurationTime", iDurationTime);

        DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_TRACE, "start_cruise_srv_by_id() CruiseID=%u, CruiseName=%s, StartTime=%d, DurationTime=%d \r\n", uCruiseID, strCruiseName.c_str(), iStartTime, iDurationTime);

        i = cruise_srv_init(&pCruiseSrv);

        if (i != 0)
        {
            DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_ERROR, "start_cruise_srv_by_id() cruise_srv_init:i=%d \r\n", i);
            return -1;
        }

        pCruiseSrv->status = 2;
        pCruiseSrv->cruise_type = 0;
        pCruiseSrv->cruise_id = uCruiseID;

        if (!strCruiseName.empty())
        {
            osip_strncpy(pCruiseSrv->cruise_name, (char*)strCruiseName.c_str(), MAX_128CHAR_STRING_LEN);
        }

        pCruiseSrv->start_time = iStartTime;
        pCruiseSrv->duration_time = iDurationTime;
        pCruiseSrv->duration_time_count = 0;
        pCruiseSrv->current_pos = 0;

        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "����ֶ�����ִ�е�Ѳ��: Ѳ��ID=%u, Ѳ������=%s, ��ʼʱ��=%d, ����ʱ��=%d", pCruiseSrv->cruise_id, pCruiseSrv->cruise_name, pCruiseSrv->start_time, pCruiseSrv->duration_time);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Add a manually triggered navigation: cruise_id=%d, cruise_name=%s, start_time=%d, duration_time=%d", pCruiseSrv->cruise_id, pCruiseSrv->cruise_name, pCruiseSrv->start_time, pCruiseSrv->duration_time);

        /* ��Ӷ������ݵ����� */
        i = add_cruise_action_data_to_srv_list_proc(pCruiseSrv->cruise_id, pCruiseSrv->pCruiseActionList, pCruise_Srv_dboper);

        if (i < 0)
        {
            cruise_srv_free(pCruiseSrv);
            pCruiseSrv = NULL;
            DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_ERROR, "start_cruise_srv_by_id() add_cruise_action_data_to_srv_list_proc:i=%d \r\n", i);
            return -1;
        }

        /* ��ӵ����� */
        if (cruise_srv_add(pCruiseSrv) < 0)
        {
            cruise_srv_free(pCruiseSrv);
            pCruiseSrv = NULL;
            DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_ERROR, "start_cruise_srv_by_id() Cruise Srv Add Error");
            return -1;
        }

        /* ֪ͨ�ͻ��� */
        if (0 == pCruiseSrv->send_mark)
        {
            pCruiseSrv->send_mark = 1;

            /* ����״̬ */
            iRet = UpdateCruiseConfigStatus2DB(pCruiseSrv->cruise_id, 1, pCruise_Srv_dboper);

            if (iRet < 0)
            {
                DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_ERROR, "start_cruise_srv_by_id() UpdateCruiseConfigStatus2DB Start Error: cruise_id=%u, iRet=%d", pCruiseSrv->cruise_id, iRet);
            }
            else
            {
                DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_TRACE, "start_cruise_srv_by_id() UpdateCruiseConfigStatus2DB Start OK: cruise_id=%u, iRet=%d", pCruiseSrv->cruise_id, iRet);
            }
        }
    }
    else
    {
        pCruiseSrv = cruise_srv_get(cruise_pos);

        if (NULL != pCruiseSrv)
        {
            if (0 == pCruiseSrv->status || 3 == pCruiseSrv->status)
            {
                pCruiseSrv->status = 2;
                pCruiseSrv->cruise_type = 0;
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ֶ�����ִ�е�Ѳ��: Ѳ��ID=%u, Ѳ������=%s, ��ʼʱ��=%d, ����ʱ��=%d", pCruiseSrv->cruise_id, pCruiseSrv->cruise_name, pCruiseSrv->start_time, pCruiseSrv->duration_time);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Manually triggered navigation: cruise_id=%d, cruise_name=%s, start_time=%d, duration_time=%d", pCruiseSrv->cruise_id, pCruiseSrv->cruise_name, pCruiseSrv->start_time, pCruiseSrv->duration_time);

                /* ֪ͨ�ͻ��� */
                if (0 == pCruiseSrv->send_mark)
                {
                    pCruiseSrv->send_mark = 1;

                    /* ����״̬ */
                    iRet = UpdateCruiseConfigStatus2DB(pCruiseSrv->cruise_id, 1, pCruise_Srv_dboper);

                    if (iRet < 0)
                    {
                        DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_ERROR, "start_cruise_srv_by_id() UpdateCruiseConfigStatus2DB Start Error: cruise_id=%u, iRet=%d", pCruiseSrv->cruise_id, iRet);
                    }
                    else
                    {
                        DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_TRACE, "start_cruise_srv_by_id() UpdateCruiseConfigStatus2DB Start OK: cruise_id=%u, iRet=%d", pCruiseSrv->cruise_id, iRet);
                    }
                }

                DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_TRACE, "start_cruise_srv_by_id() CruiseID=%u, CruiseName=%s, StartTime=%d, DurationTime=%d \r\n", pCruiseSrv->cruise_id, pCruiseSrv->cruise_name, pCruiseSrv->start_time, pCruiseSrv->duration_time);
            }
            else if (1 == pCruiseSrv->status) /* �ٴη���һ�� */
            {
                if (0 == pCruiseSrv->cruise_type)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ֶ�����ִ�е�Ѳ���Ѿ�����ִ��: Ѳ��ID=%u, Ѳ������=%s, ��ʼʱ��=%d, ����ʱ��=%d", pCruiseSrv->cruise_id, pCruiseSrv->cruise_name, pCruiseSrv->start_time, pCruiseSrv->duration_time);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Manually triggered navigation being executed already: cruise_id=%d, cruise_name=%s, start_time=%d, duration_time=%d:", pCruiseSrv->cruise_id, pCruiseSrv->cruise_name, pCruiseSrv->start_time, pCruiseSrv->duration_time);
                }
                else if (1 == pCruiseSrv->cruise_type)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ֶ�����ִ�е�Ѳ���Ѿ���Ԥ����������ִ��, Ԥ��Ѳ��ִ�����֮������ִ��: Ѳ��ID=%u, Ѳ������=%s, ��ʼʱ��=%d, ����ʱ��=%d", pCruiseSrv->cruise_id, pCruiseSrv->cruise_name, pCruiseSrv->start_time, pCruiseSrv->duration_time);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Manually triggered navigation being executed by plan trigger already, will continue execute after completing plan cruise: cruise_id=%d, cruise_name=%s, start_time=%d, duration_time=%d:", pCruiseSrv->cruise_id, pCruiseSrv->cruise_name, pCruiseSrv->start_time, pCruiseSrv->duration_time);
                    pCruiseSrv->cruise_type = 3;
                }
                else if (2 == pCruiseSrv->cruise_type)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ֶ�����ִ�е�Ѳ���Ѿ���Ԥ�������ж�ִ��, Ԥ��Ѳ��ִ�����֮������ִ��: Ѳ��ID=%u, Ѳ������=%s, ��ʼʱ��=%d, ����ʱ��=%d", pCruiseSrv->cruise_id, pCruiseSrv->cruise_name, pCruiseSrv->start_time, pCruiseSrv->duration_time);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Manually triggered navigation being executed by plan trigger interrupt already, will continue execute after completing plan cruise: cruise_id=%d, cruise_name=%s, start_time=%d, duration_time=%d:", pCruiseSrv->cruise_id, pCruiseSrv->cruise_name, pCruiseSrv->start_time, pCruiseSrv->duration_time);
                }
                else if (3 == pCruiseSrv->cruise_type)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ֶ�����ִ�е�Ѳ���Ѿ���Ԥ����������ִ��, Ԥ��Ѳ��ִ�����֮������ִ��: Ѳ��ID=%u, Ѳ������=%s, ��ʼʱ��=%d, ����ʱ��=%d", pCruiseSrv->cruise_id, pCruiseSrv->cruise_name, pCruiseSrv->start_time, pCruiseSrv->duration_time);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Manually triggered navigation being executed by plan trigger already, will continue execute after completing plan cruise: cruise_id=%d, cruise_name=%s, start_time=%d, duration_time=%d:", pCruiseSrv->cruise_id, pCruiseSrv->cruise_name, pCruiseSrv->start_time, pCruiseSrv->duration_time);
                }
            }
            else
            {
                DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_ERROR, "start_cruise_srv_by_id() CruiseID=%u, status=%d \r\n", pCruiseSrv->cruise_id, pCruiseSrv->status);
            }
        }
        else
        {
            DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_ERROR, "start_cruise_srv_by_id() cruise_srv_get Error:cruise_pos=%d \r\n", cruise_pos);
        }
    }

    return 0;
}

/*****************************************************************************
 �� �� ��  : start_cruise_srv_by_id_for_plan
 ��������  : Ԥ������ִ��Ѳ��
 �������  : unsigned int id
             DBOper* pCruise_Srv_dboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2016��6��30��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int start_cruise_srv_by_id_for_plan(unsigned int id, DBOper* pCruise_Srv_dboper)
{
    int i = 0;
    int iRet = 0;
    int cruise_pos = -1;
    string strSQL = "";
    int record_count = 0;
    cruise_srv_t* pCruiseSrv = NULL;
    char strCruiseID[32] = {0};

    if (NULL == pCruise_Srv_dboper)
    {
        return -1;
    }

    cruise_pos = cruise_srv_find(id);

    DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_TRACE, "start_cruise_srv_by_id_for_plan() cruise_srv_find:id=%u, cruise_pos=%d \r\n", id, cruise_pos);

    if (cruise_pos < 0)
    {
        snprintf(strCruiseID, 32, "%u", id);
        strSQL.clear();
        strSQL = "select * from CruiseConfig WHERE ID = ";
        strSQL += strCruiseID;

        record_count = pCruise_Srv_dboper->DB_Select(strSQL.c_str(), 1);

        if (record_count < 0)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Ԥ���������ִ�е�Ѳ��ʧ��: Ѳ��ID=%u, ԭ��=%s:%s", id, (char*)"��ѯ���ݿ�ʧ��", pCruise_Srv_dboper->GetLastDbErrorMsg());
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Fail to excute cruise that plan trigger adds: cruise_ID=%u, cause=%s:", id, (char*)"Failed to query the database", pCruise_Srv_dboper->GetLastDbErrorMsg());

            DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_ERROR, "start_cruise_srv_by_id_for_plan() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
            DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_ERROR, "start_cruise_srv_by_id_for_plan() ErrorMsg=%s\r\n", pCruise_Srv_dboper->GetLastDbErrorMsg());
            return -1;
        }
        else if (record_count == 0)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Ԥ���������ִ�е�Ѳ��ʧ��: Ѳ��ID=%u, ԭ��=%s", id, (char*)"û�в�ѯ��Ѳ�����ݿ��¼");
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Fail to excute cruise that plan trigger adds: cruise_ID=%u, cause=%s:", id, (char*)"Failed to query the database record of cruise");

            DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_WARN, "start_cruise_srv_by_id_for_plan() exit---: No Record Count \r\n");
            return 0;
        }


        unsigned int uCruiseID = 0;
        string strCruiseName = "";
        int iStartTime = 0;
        int iDurationTime = 0;

        pCruise_Srv_dboper->GetFieldValue("ID", uCruiseID);
        pCruise_Srv_dboper->GetFieldValue("CruiseName", strCruiseName);
        pCruise_Srv_dboper->GetFieldValue("StartTime", iStartTime);
        pCruise_Srv_dboper->GetFieldValue("DurationTime", iDurationTime);

        DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_TRACE, "start_cruise_srv_by_id_for_plan() CruiseID=%u, CruiseName=%s, StartTime=%d, DurationTime=%d \r\n", uCruiseID, strCruiseName.c_str(), iStartTime, iDurationTime);

        i = cruise_srv_init(&pCruiseSrv);

        if (i != 0)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Ԥ���������ִ�е�Ѳ��ʧ��: Ѳ��ID=%u, ԭ��=%s", id, (char*)"Ѳ�����ݳ�ʼ��ʧ��");
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Fail to excute cruise that plan trigger adds: cruise_ID=%u, cause=%s:", id, (char*)"Cruise data failed to initialize");

            DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_ERROR, "start_cruise_srv_by_id_for_plan() cruise_srv_init:i=%d \r\n", i);
            return -1;
        }

        pCruiseSrv->status = 2;
        pCruiseSrv->cruise_type = 1;
        pCruiseSrv->cruise_id = uCruiseID;

        if (!strCruiseName.empty())
        {
            osip_strncpy(pCruiseSrv->cruise_name, (char*)strCruiseName.c_str(), MAX_128CHAR_STRING_LEN);
        }

        pCruiseSrv->start_time = iStartTime;
        pCruiseSrv->duration_time = iDurationTime;
        pCruiseSrv->duration_time_count = 0;
        pCruiseSrv->current_pos = 0;

        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Ԥ���������ִ�е�Ѳ��: Ѳ��ID=%u, Ѳ������=%s, ��ʼʱ��=%d, ����ʱ��=%d", pCruiseSrv->cruise_id, pCruiseSrv->cruise_name, pCruiseSrv->start_time, pCruiseSrv->duration_time);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Add a manually triggered navigation: cruise_id=%d, cruise_name=%s, start_time=%d, duration_time=%d", pCruiseSrv->cruise_id, pCruiseSrv->cruise_name, pCruiseSrv->start_time, pCruiseSrv->duration_time);

        /* ��Ӷ������ݵ����� */
        i = add_cruise_action_data_to_srv_list_proc(pCruiseSrv->cruise_id, pCruiseSrv->pCruiseActionList, pCruise_Srv_dboper);

        if (i < 0)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Ԥ���������ִ�е�Ѳ��ʧ��: Ѳ��ID=%u, ԭ��=%s", pCruiseSrv->cruise_id, (char*)"���Ѳ����������ʧ��");
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Fail to excute cruise that plan trigger adds: cruise_ID=%u, cause=%s:", pCruiseSrv->cruise_id, (char*)"Fail to add cruise operation data");

            cruise_srv_free(pCruiseSrv);
            pCruiseSrv = NULL;
            DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_ERROR, "start_cruise_srv_by_id_for_plan() add_cruise_action_data_to_srv_list_proc:i=%d \r\n", i);
            return -1;
        }

        /* ��ӵ����� */
        if (cruise_srv_add(pCruiseSrv) < 0)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Ԥ���������ִ�е�Ѳ��ʧ��: Ѳ��ID=%u, ԭ��=%s", pCruiseSrv->cruise_id, (char*)"���Ѳ������ʧ��");
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Fail to excute cruise that plan trigger adds: cruise_ID=%u, cause=%s:", pCruiseSrv->cruise_id, (char*)"Fail to add cruise data");

            cruise_srv_free(pCruiseSrv);
            pCruiseSrv = NULL;
            DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_ERROR, "start_cruise_srv_by_id_for_plan() Cruise Srv Add Error");
            return -1;
        }

        /* ֪ͨ�ͻ��� */
        if (0 == pCruiseSrv->send_mark)
        {
            pCruiseSrv->send_mark = 1;

            /* ����״̬ */
            iRet = UpdateCruiseConfigStatus2DB(pCruiseSrv->cruise_id, 1, pCruise_Srv_dboper);

            if (iRet < 0)
            {
                DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_ERROR, "start_cruise_srv_by_id_for_plan() UpdateCruiseConfigStatus2DB Start Error: cruise_id=%u, iRet=%d", pCruiseSrv->cruise_id, iRet);
            }
            else
            {
                DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_TRACE, "start_cruise_srv_by_id_for_plan() UpdateCruiseConfigStatus2DB Start OK: cruise_id=%u, iRet=%d", pCruiseSrv->cruise_id, iRet);
            }
        }
    }
    else
    {
        pCruiseSrv = cruise_srv_get(cruise_pos);

        if (NULL != pCruiseSrv)
        {
            if (0 == pCruiseSrv->status || 3 == pCruiseSrv->status)
            {
                pCruiseSrv->status = 2;
                pCruiseSrv->cruise_type = 1;
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Ԥ���������ִ�е�Ѳ��: Ѳ��ID=%u, Ѳ������=%s, ��ʼʱ��=%d, ����ʱ��=%d", pCruiseSrv->cruise_id, pCruiseSrv->cruise_name, pCruiseSrv->start_time, pCruiseSrv->duration_time);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Manually triggered navigation: cruise_id=%u, cruise_name=%s, start_time=%d, duration_time=%d", pCruiseSrv->cruise_id, pCruiseSrv->cruise_name, pCruiseSrv->start_time, pCruiseSrv->duration_time);

                /* ֪ͨ�ͻ��� */
                if (0 == pCruiseSrv->send_mark)
                {
                    pCruiseSrv->send_mark = 1;

                    /* ����״̬ */
                    iRet = UpdateCruiseConfigStatus2DB(pCruiseSrv->cruise_id, 1, pCruise_Srv_dboper);

                    if (iRet < 0)
                    {
                        DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_ERROR, "start_cruise_srv_by_id_for_plan() UpdateCruiseConfigStatus2DB Start Error: cruise_id=%u, iRet=%d", pCruiseSrv->cruise_id, iRet);
                    }
                    else
                    {
                        DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_TRACE, "start_cruise_srv_by_id_for_plan() UpdateCruiseConfigStatus2DB Start OK: cruise_id=%u, iRet=%d", pCruiseSrv->cruise_id, iRet);
                    }
                }

                DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_TRACE, "start_cruise_srv_by_id_for_plan() CruiseID=%u, CruiseName=%s, StartTime=%d, DurationTime=%d \r\n", pCruiseSrv->cruise_id, pCruiseSrv->cruise_name, pCruiseSrv->start_time, pCruiseSrv->duration_time);
            }
            else if (1 == pCruiseSrv->status) /* �ٴη���һ�� */
            {
                if (0 == pCruiseSrv->cruise_type)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Ԥ���������ִ�е�Ѳ������ִ��, �ж�ִ��ԭ��Ѳ��, ����ִ��Ԥ��Ѳ��֮���ٴ�ִ��ԭ��Ѳ��: Ѳ��ID=%u, Ѳ������=%s, ��ʼʱ��=%d, ����ʱ��=%d", pCruiseSrv->cruise_id, pCruiseSrv->cruise_name, pCruiseSrv->start_time, pCruiseSrv->duration_time);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Plans to add cruise trigger execution is being executed, the interrupt execution of the original cruise, priority implementation plan after the implementation of the existing cruise cruise again: cruise_ID=%u, cruise_name=%s, start_time=%d, during_time=%d", pCruiseSrv->cruise_id, pCruiseSrv->cruise_name, pCruiseSrv->start_time, pCruiseSrv->duration_time);
                    pCruiseSrv->status = 2;
                    pCruiseSrv->cruise_type = 2;
                }
                else if (1 == pCruiseSrv->cruise_type)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Ԥ���������ִ�е�Ѳ������ִ��, �ٴδ�������ִ��, Ѳ��ID=%u, Ѳ������=%s, ��ʼʱ��=%d, ����ʱ��=%d", pCruiseSrv->cruise_id, pCruiseSrv->cruise_name, pCruiseSrv->start_time, pCruiseSrv->duration_time);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Trigger execution plan to add cruise being implemented, re-execute the trigger again, cruise_ID=%u, cruise_name=%s, start_time=%d, during_time=%d", pCruiseSrv->cruise_id, pCruiseSrv->cruise_name, pCruiseSrv->start_time, pCruiseSrv->duration_time);
                    pCruiseSrv->status = 2;
                }
                else if (2 == pCruiseSrv->cruise_type)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Ԥ���������ִ�е�Ѳ������ִ��, �ٴδ�������ִ��, Ѳ��ID=%u, Ѳ������=%s, ��ʼʱ��=%d, ����ʱ��=%d", pCruiseSrv->cruise_id, pCruiseSrv->cruise_name, pCruiseSrv->start_time, pCruiseSrv->duration_time);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Trigger execution plan to add cruise being implemented, re-execute the trigger again, cruise_ID=%u, cruise_name=%s, start_time=%d, during_time=%d", pCruiseSrv->cruise_id, pCruiseSrv->cruise_name, pCruiseSrv->start_time, pCruiseSrv->duration_time);
                    pCruiseSrv->status = 2;
                }
                else if (3 == pCruiseSrv->cruise_type)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Ԥ���������ִ�е�Ѳ������ִ��, �ٴδ�������ִ��, Ѳ��ID=%u, Ѳ������=%s, ��ʼʱ��=%d, ����ʱ��=%d", pCruiseSrv->cruise_id, pCruiseSrv->cruise_name, pCruiseSrv->start_time, pCruiseSrv->duration_time);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Trigger execution plan to add cruise being implemented, re-execute the trigger again, cruise_ID=%u, cruise_name=%s, start_time=%d, during_time=%d", pCruiseSrv->cruise_id, pCruiseSrv->cruise_name, pCruiseSrv->start_time, pCruiseSrv->duration_time);
                    pCruiseSrv->status = 2;
                }
            }
            else
            {
                DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_ERROR, "start_cruise_srv_by_id_for_plan() CruiseID=%u, status=%d \r\n", pCruiseSrv->cruise_id, pCruiseSrv->status);
            }
        }
        else
        {
            DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_ERROR, "start_cruise_srv_by_id_for_plan() cruise_srv_get Error:cruise_pos=%d \r\n", cruise_pos);
        }
    }

    return 0;
}


/*****************************************************************************
 �� �� ��  : stop_cruise_srv_by_id
 ��������  : ����IDֹͣѲ������
 �������  : int id
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��10��17��
    ��    ��   : �û�·����Ϣ����
    �޸�����   : �����ɺ���

*****************************************************************************/
int stop_cruise_srv_by_id(unsigned int id)
{
    int pos = -1;
    cruise_srv_t* pCruiseSrv = NULL;

    pos = cruise_srv_find(id);

    if (pos >= 0)
    {
        pCruiseSrv = cruise_srv_get(pos);

        if (NULL != pCruiseSrv)
        {
            if (1 == pCruiseSrv->status || 4 == pCruiseSrv->status)
            {
                pCruiseSrv->status = 3;
                pCruiseSrv->send_mark = 0;
            }
            else if (2 == pCruiseSrv->status)
            {
                pCruiseSrv->status = 0;
                pCruiseSrv->send_mark = 0;
            }
            else
            {
                DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_TRACE, "stop_cruise_srv_by_id() cruise_id=%u, status=%d\r\n", pCruiseSrv->cruise_id, pCruiseSrv->status);
            }
        }
    }

    return 0;
}

/*****************************************************************************
 �� �� ��  : set_cruise_srv_list_del_mark
 ��������  : ����Ѳ��ҵ��ɾ����ʶ
 �������  : int del_mark
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��9��4�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int set_cruise_srv_list_del_mark(int del_mark)
{
    int pos1 = 0;
    int pos2 = 0;
    cruise_srv_t* pCruiseSrv = NULL;
    cruise_action_t* pCruiseAction = NULL;

    if ((NULL == g_CruiseSrvList) || (NULL == g_CruiseSrvList->pCruiseSrvList))
    {
        //DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_DEBUG, "set_cruise_srv_list_del_mark() exit---: Param Error \r\n");
        return -1;
    }

    CRUISE_SMUTEX_LOCK();

    if (osip_list_size(g_CruiseSrvList->pCruiseSrvList) <= 0)
    {
        CRUISE_SMUTEX_UNLOCK();
        return 0;
    }

    for (pos1 = 0; pos1 < osip_list_size(g_CruiseSrvList->pCruiseSrvList); pos1++)
    {
        pCruiseSrv = (cruise_srv_t*)osip_list_get(g_CruiseSrvList->pCruiseSrvList, pos1);

        if (NULL == pCruiseSrv)
        {
            continue;
        }

        pCruiseSrv->del_mark = del_mark;

        for (pos2 = 0; pos2 < osip_list_size(pCruiseSrv->pCruiseActionList); pos2++)
        {
            pCruiseAction = (cruise_action_t*)osip_list_get(pCruiseSrv->pCruiseActionList, pos2);

            if (NULL == pCruiseAction)
            {
                continue;
            }

            pCruiseAction->del_mark = del_mark;
        }
    }

    CRUISE_SMUTEX_UNLOCK();

    return 0;
}

/*****************************************************************************
 �� �� ��  : check_db_data_to_cruise_srv_list
 ��������  : �������м���Ƿ�����Ҫִ�е�Ѳ�����ݣ�����У�����ص��ڴ���
 �������  : DBOper* pCruise_Srv_dboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��4��2�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int check_db_data_to_cruise_srv_list(DBOper* pCruise_Srv_dboper)
{
    int i = 0;
    int iRet = 0;
    string strSQL = "";
    int record_count = 0;
    time_t now = time(NULL);
    int iTimeNow = 0;
    struct tm tp = {0};
    int while_count = 0;
    cruise_srv_t* pCruiseSrv2 = NULL;

    if (NULL == pCruise_Srv_dboper)
    {
        return -1;
    }

    strSQL.clear();
    strSQL = "select * from CruiseConfig order by StartTime asc";

    record_count = pCruise_Srv_dboper->DB_Select(strSQL.c_str(), 1);

    if (record_count < 0)
    {
        DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_ERROR, "check_db_data_to_cruise_srv_list() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
        DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_ERROR, "check_db_data_to_cruise_srv_list() ErrorMsg=%s\r\n", pCruise_Srv_dboper->GetLastDbErrorMsg());
        return -1;
    }
    else if (record_count == 0)
    {
        //DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_DEBUG, "check_db_data_to_cruise_srv_list_for_start() exit---: No Record Count \r\n");
        return 0;
    }

    localtime_r(&now, &tp);
    iTimeNow = tp.tm_hour * 3600 + tp.tm_min * 60 + tp.tm_sec;

    DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_TRACE, "check_db_data_to_cruise_srv_list:record_count=%d \r\n", record_count);

    do
    {
        int i = 0;
        unsigned int uCruiseID = 0;
        string strCruiseName = "";
        int iOldStartTime = 0;
        int iStartTime = 0;
        int iDurationTime = 0;
        int iScheduledRun = 0;
        int cruise_pos = -1;
        int iResved1 = 0;

        while_count++;

        if (while_count % 10000 == 0)
        {
            DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_WARN, "check_db_data_to_cruise_srv_list() While Count=%d \r\n", while_count);
        }

        pCruise_Srv_dboper->GetFieldValue("ID", uCruiseID);
        pCruise_Srv_dboper->GetFieldValue("CruiseName", strCruiseName);
        pCruise_Srv_dboper->GetFieldValue("StartTime", iStartTime);
        pCruise_Srv_dboper->GetFieldValue("DurationTime", iDurationTime);
        pCruise_Srv_dboper->GetFieldValue("ScheduledRun", iScheduledRun);
        pCruise_Srv_dboper->GetFieldValue("Resved1", iResved1);

        /* ���Ҷ��У������������Ƿ��Ѿ����� */
        cruise_pos = cruise_srv_find(uCruiseID);

        DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_TRACE, "check_db_data_to_cruise_srv_list() CruiseID=%u:cruise_pos=%d \r\n", uCruiseID, cruise_pos);

        if (cruise_pos < 0) /* ��ӵ�Ҫִ�ж��� */
        {
            cruise_srv_t* pCruiseSrv = NULL;

            i = cruise_srv_init(&pCruiseSrv);

            if (i != 0)
            {
                DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_ERROR, "check_db_data_to_cruise_srv_list() cruise_srv_init:i=%d \r\n", i);
                continue;
            }

            pCruiseSrv->cruise_id = uCruiseID;

            if (!strCruiseName.empty())
            {
                osip_strncpy(pCruiseSrv->cruise_name, (char*)strCruiseName.c_str(), MAX_128CHAR_STRING_LEN);
            }

            pCruiseSrv->start_time = iStartTime;
            pCruiseSrv->duration_time = iDurationTime;
            pCruiseSrv->del_mark = 0;

            if ((iTimeNow == iStartTime) || (iTimeNow > iStartTime && iTimeNow - iStartTime < 30)) /* 30��֮�ڵĲ����� */
            {
                if (iScheduledRun)
                {
                    pCruiseSrv->status = 2;
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "��Ӷ�ʱ����ִ�е�Ѳ��: cruise_id=%d, cruise_name=%s, start_time=%d, duration_time=%d", pCruiseSrv->cruise_id, pCruiseSrv->cruise_name, pCruiseSrv->start_time, pCruiseSrv->duration_time);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Add timed navigation: cruise_id=%d, cruise_name=%s, start_time=%d, duration_time=%d", pCruiseSrv->cruise_id, pCruiseSrv->cruise_name, pCruiseSrv->start_time, pCruiseSrv->duration_time);
                }
            }
            else if (0 == iStartTime && 0 == iDurationTime && 1 == iResved1) /* �ֶ�ִ�еģ�״̬��1����Ҫ���� */
            {
                if (!iScheduledRun)
                {
                    pCruiseSrv->status = 2;
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "����ֶ��ٷ�ִ�еĵ���û��ֹͣ��Ѳ��: cruise_id=%d, cruise_name=%s, start_time=%d, duration_time=%d", pCruiseSrv->cruise_id, pCruiseSrv->cruise_name, pCruiseSrv->start_time, pCruiseSrv->duration_time);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Add a manually triggered but not stopped navigation: cruise_id=%d, cruise_name=%s, start_time=%d, duration_time=%d", pCruiseSrv->cruise_id, pCruiseSrv->cruise_name, pCruiseSrv->start_time, pCruiseSrv->duration_time);
                }
            }

            /* ��ӵ����� */
            if (cruise_srv_add(pCruiseSrv) < 0)
            {
                cruise_srv_free(pCruiseSrv);
                pCruiseSrv = NULL;
                DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_ERROR, "check_db_data_to_cruise_srv_list() Cruise Srv Add Error\r\n");
                continue;
            }
            else
            {
                DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_INFO, "check_db_data_to_cruise_srv_list() cruise_srv_add:cruise_id=%u, StartTime=%d,DurationTime=%d,Status=%d, \r\n", pCruiseSrv->cruise_id, iStartTime, iDurationTime, pCruiseSrv->status);
            }
        }
        else
        {
            cruise_srv_t* pCruiseSrv = NULL;

            pCruiseSrv = cruise_srv_get(cruise_pos);

            if (NULL != pCruiseSrv)
            {
                /* �������Ƿ��б仯 */
                if (!strCruiseName.empty())
                {
                    if (0 != sstrcmp(pCruiseSrv->cruise_name, (char*)strCruiseName.c_str()))
                    {
                        memset(pCruiseSrv->cruise_name, 0, MAX_128CHAR_STRING_LEN + 4);
                        osip_strncpy(pCruiseSrv->cruise_name, (char*)strCruiseName.c_str(), MAX_128CHAR_STRING_LEN);
                    }
                }
                else
                {
                    memset(pCruiseSrv->cruise_name, 0, MAX_128CHAR_STRING_LEN + 4);
                }

                iOldStartTime = pCruiseSrv->start_time;
                pCruiseSrv->start_time = iStartTime;
                pCruiseSrv->duration_time = iDurationTime;
                pCruiseSrv->del_mark = 0;

                if (0 == pCruiseSrv->status || 3 == pCruiseSrv->status)
                {
                    if ((iTimeNow == iStartTime) || (iTimeNow > iStartTime && iTimeNow - iStartTime < 30)) /* 30��֮�ڵĲ����� */
                    {
                        if (iScheduledRun)
                        {
                            pCruiseSrv->status = 2;
                            //DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_INFO, "check_db_data_to_cruise_srv_list() cruise_srv_add:cruise_id=%u, StartTime=%d,DurationTime=%d,Status=%d, \r\n", pCruiseSrv->cruise_id, iStartTime, iDurationTime, pCruiseSrv->status);
                            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "��Ӷ�ʱ�ٷ�ִ�е�Ѳ��: cruise_id=%d, cruise_name=%s, start_time=%d, duration_time=%d", pCruiseSrv->cruise_id, pCruiseSrv->cruise_name, pCruiseSrv->start_time, pCruiseSrv->duration_time);
                            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Add timed navigation: cruise_id=%d, cruise_name=%s, start_time=%d, duration_time=%d", pCruiseSrv->cruise_id, pCruiseSrv->cruise_name, pCruiseSrv->start_time, pCruiseSrv->duration_time);
                        }
                    }
                    else if (0 == iStartTime && 0 == iDurationTime && 1 == iResved1) /* �ֶ�ִ�еģ�״̬��1����Ҫ���� */
                    {
                        if (!iScheduledRun)
                        {
                            pCruiseSrv->status = 2;
                            //DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_INFO, "check_db_data_to_cruise_srv_list() cruise_srv_add:cruise_id=%u, StartTime=%d,DurationTime=%d,Status=%d, \r\n", pCruiseSrv->cruise_id, iStartTime, iDurationTime, pCruiseSrv->status);
                            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "����ֶ��ٷ�ִ�еĵ���û��ֹͣ��Ѳ��: cruise_id=%d, cruise_name=%s, start_time=%d, duration_time=%d", pCruiseSrv->cruise_id, pCruiseSrv->cruise_name, pCruiseSrv->start_time, pCruiseSrv->duration_time);
                            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Add a manually triggered but not stopped navigation: cruise_id=%d, cruise_name=%s, start_time=%d, duration_time=%d", pCruiseSrv->cruise_id, pCruiseSrv->cruise_name, pCruiseSrv->start_time, pCruiseSrv->duration_time);

                        }
                    }
                }
                else if (1 == pCruiseSrv->status) /* �����޸�������ʱ�䣬�ٷ���һ�� */
                {
                    if ((iStartTime != iOldStartTime) && ((iTimeNow == iStartTime) || (iTimeNow > iStartTime && iTimeNow - iStartTime < 30))) /* 30��֮�ڵĲ����� */
                    {
                        if (iScheduledRun)
                        {
                            pCruiseSrv->status = 4;
                            DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_INFO, "check_db_data_to_cruise_srv_list() cruise_srv_add:cruise_id=%u, StartTime=%d,DurationTime=%d,Status=%d, \r\n", pCruiseSrv->cruise_id, iStartTime, iDurationTime, pCruiseSrv->status);
                        }
                    }
                }
            }
        }
    }
    while (pCruise_Srv_dboper->MoveNext() >= 0);

    /* ���Ŀ�����ݺ�Դ���� */
    for (i = 0; i < osip_list_size(g_CruiseSrvList->pCruiseSrvList); i++)
    {
        pCruiseSrv2 = (cruise_srv_t*)osip_list_get(g_CruiseSrvList->pCruiseSrvList, i);

        if (NULL == pCruiseSrv2)
        {
            continue;
        }

        /* ��Ӷ������ݵ����� */
        iRet = add_cruise_action_data_to_srv_list_proc(pCruiseSrv2->cruise_id, pCruiseSrv2->pCruiseActionList, pCruise_Srv_dboper);

        if (iRet < 0)
        {
            DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_ERROR, "check_db_data_to_cruise_srv_list() add_cruise_action_data_to_srv_list_proc Error:cruise_id=%u \r\n", pCruiseSrv2->cruise_id);
        }
        else
        {
            DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_TRACE, "check_db_data_to_cruise_srv_list() add_cruise_action_data_to_srv_list_proc:cruise_id=%u, iRet=%d \r\n", pCruiseSrv2->cruise_id, iRet);
        }
    }

    return iRet;
}

/*****************************************************************************
 �� �� ��  : add_cruise_action_data_to_srv_list_proc
 ��������  : ���Ѳ�����������ݵ�Ѳ��ҵ�����
 �������  : int cruise_id
             osip_list_t* pCruiseActionList
             DBOper* pCruise_Srv_dboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��4��2�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int add_cruise_action_data_to_srv_list_proc(unsigned int cruise_id, osip_list_t* pCruiseActionList, DBOper* pCruise_Srv_dboper)
{
    int i = 0;
    int pos = -1;
    int iRet = 0;
    int record_count = 0;
    char strCruiseID[32] = {0};

    string strSQL = "";
    int while_count = 0;

    if (NULL == pCruise_Srv_dboper)
    {
        DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_DEBUG, "add_cruise_action_data_to_srv_list_proc() exit---: Cruise Srv DB Oper Error \r\n");
        return -1;
    }

    if (NULL == pCruiseActionList)
    {
        DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_DEBUG, "add_cruise_action_data_to_srv_list_proc() exit---: Cruise Action List Error \r\n");
        return -1;
    }

    /* ����cruise_id����ѯѲ����������ȡѲ���ľ������� */
    strSQL.clear();
    snprintf(strCruiseID, 32, "%u", cruise_id);
    strSQL = "select * from CruiseActionConfig WHERE CruiseID = ";
    strSQL += strCruiseID;
    strSQL += " order by SortID asc";

    record_count = pCruise_Srv_dboper->DB_Select(strSQL.c_str(), 1);

    DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_TRACE, "add_cruise_action_data_to_srv_list_proc:CruiseID=%u, record_count=%d \r\n", cruise_id, record_count);

    if (record_count < 0)
    {
        DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_ERROR, "add_cruise_action_data_to_srv_list_proc() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
        DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_ERROR, "add_cruise_action_data_to_srv_list_proc() ErrorMsg=%s\r\n", pCruise_Srv_dboper->GetLastDbErrorMsg());
        return -1;
    }
    else if (record_count == 0)
    {
        //DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_WARN, "add_cruise_action_dest_data_to_srv_list_proc() exit---: No Record Count \r\n");
        return 0;
    }

    /* ѭ�����Ѳ���������� */
    do
    {
        unsigned int iDeviceIndx = 0;
        unsigned int iPresetID = 0;
        int iLiveTime = 0;

        while_count++;

        if (while_count % 10000 == 0)
        {
            DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_WARN, "add_cruise_action_data_to_srv_list_proc() While Count=%d \r\n", while_count);
        }

        pCruise_Srv_dboper->GetFieldValue("DeviceIndex", iDeviceIndx);
        pCruise_Srv_dboper->GetFieldValue("PresetID", iPresetID);
        pCruise_Srv_dboper->GetFieldValue("LiveTime", iLiveTime);

        if (iDeviceIndx <= 0)
        {
            DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_WARN, "add_cruise_action_data_to_srv_list_proc() DeviceIndx Empty \r\n");
            continue;
        }

        if (iPresetID <= 0)
        {
            DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_WARN, "add_cruise_action_data_to_srv_list_proc() PresetID Empty \r\n");
            continue;
        }

        if (iLiveTime <= 0)
        {
            DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_WARN, "add_cruise_action_data_to_srv_list_proc() LiveTime Empty \r\n");
            continue;
        }

        /* ����DeviceIndex���Ҷ����� */
        pos = cruise_action_find(iDeviceIndx, iPresetID, pCruiseActionList);

        DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_TRACE, "add_cruise_action_data_to_srv_list_proc() DeviceIndex=%u, PresetID=%d, pos=%d \r\n", iDeviceIndx, iPresetID, pos);

        if (pos < 0)
        {
            cruise_action_t* pCruiseAction = NULL;

            /* ���Ѳ������ */
            iRet = cruise_action_init(&pCruiseAction);

            if (0 != iRet)
            {
                DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_ERROR, "add_cruise_action_data_to_srv_list_proc() Cruise Action Init Error \r\n");
                continue;
            }

            pCruiseAction->device_index = iDeviceIndx;
            pCruiseAction->iPresetID = iPresetID;
            pCruiseAction->iStatus = 0;
            pCruiseAction->iLiveTime = iLiveTime;
            pCruiseAction->iLiveTimeCount = 0;
            pCruiseAction->del_mark = 0;

            /* ��ӵ����� */
            i = osip_list_add(pCruiseActionList, pCruiseAction, -1); /* add to list tail */

            DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_TRACE, "add_cruise_action_data_to_srv_list_proc() CruiseAction:CruiseID=%u, device_index=%u, i=%d \r\n", cruise_id, pCruiseAction->device_index, i);

            if (i < 0)
            {
                cruise_action_free(pCruiseAction);
                pCruiseAction = NULL;
                DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_ERROR, "add_cruise_action_data_to_srv_list_proc() Cruise Action Add Error \r\n");
                continue;
            }
        }
        else
        {
            cruise_action_t* pCruiseAction = NULL;

            pCruiseAction = (cruise_action_t*)osip_list_get(pCruiseActionList, pos);

            if (NULL != pCruiseAction)
            {
                pCruiseAction->del_mark = 0;

                if (0 == pCruiseAction->iStatus)
                {
                    pCruiseAction->iPresetID = iPresetID;
                    pCruiseAction->iLiveTime = iLiveTime;
                    pCruiseAction->iLiveTimeCount = 0;
                }
            }
        }
    }
    while (pCruise_Srv_dboper->MoveNext() >= 0);

    DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_TRACE, "add_cruise_action_data_to_srv_list_proc:CruiseActionList.size()=%d \r\n", osip_list_size(pCruiseActionList));

    return osip_list_size(pCruiseActionList);
}

/*****************************************************************************
 �� �� ��  : delete_cruise_srv_data
 ��������  : ɾ�����н�����Ѳ������
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��4��2�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
void delete_cruise_srv_data()
{
    int pos1 = 0;
    int pos2 = 0;
    cruise_srv_t* pCruiseSrv = NULL;
    cruise_action_t* pCruiseAction = NULL;

    if ((NULL == g_CruiseSrvList) || (NULL == g_CruiseSrvList->pCruiseSrvList))
    {
        //DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_DEBUG, "delete_cruise_srv_data() exit---: Param Error \r\n");
        return;
    }

    CRUISE_SMUTEX_LOCK();

    if (osip_list_size(g_CruiseSrvList->pCruiseSrvList) <= 0)
    {
        CRUISE_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_WARN, "delete_cruise_srv_data() exit---: Cruise Srv List NULL \r\n");
        return;
    }

    pos1 = 0;

    while (!osip_list_eol(g_CruiseSrvList->pCruiseSrvList, pos1))
    {
        pCruiseSrv = (cruise_srv_t*)osip_list_get(g_CruiseSrvList->pCruiseSrvList, pos1);

        if (NULL == pCruiseSrv)
        {
            osip_list_remove(g_CruiseSrvList->pCruiseSrvList, pos1);
            continue;
        }

        if (1 == pCruiseSrv->del_mark) /* ɾ��Ѳ�� */
        {
            osip_list_remove(g_CruiseSrvList->pCruiseSrvList, pos1);
            cruise_srv_free(pCruiseSrv);
            pCruiseSrv = NULL;
        }
        else
        {
            /* ʱ����Ϣ���� */
            if (NULL == pCruiseSrv->pCruiseActionList)
            {
                pos1++;
                continue;
            }

            if (osip_list_size(pCruiseSrv->pCruiseActionList) <= 0)
            {
                pos1++;
                continue;
            }

            pos2 = 0;

            while (!osip_list_eol(pCruiseSrv->pCruiseActionList, pos2))
            {
                pCruiseAction = (cruise_action_t*)osip_list_get(pCruiseSrv->pCruiseActionList, pos2);

                if (NULL == pCruiseAction)
                {
                    osip_list_remove(pCruiseSrv->pCruiseActionList, pos2);
                    continue;
                }

                if (1 == pCruiseAction->del_mark) /* ɾ��Ѳ�� */
                {
                    if (pCruiseSrv->current_pos > pos2) /* ���Ѳ���ĵ�����Ҫɾ����֮��λ�ã���λ����Ҫ����һ */
                    {
                        pCruiseSrv->current_pos--;
                    }

                    osip_list_remove(pCruiseSrv->pCruiseActionList, pos2);
                    cruise_action_free(pCruiseAction);
                    pCruiseAction = NULL;
                }
                else
                {
                    pos2++;
                }
            }

            pos1++;
        }
    }

    CRUISE_SMUTEX_UNLOCK();

    return;
}

/*****************************************************************************
 �� �� ��  : SendNotifyExecuteCruiseActionToOnlineUser
 ��������  : ����Ѳ��ִ��֪ͨ�����߿ͻ���
 �������  : int iCruiseID
             char* cruise_name
             int iType
             DBOper* pCruise_Srv_dboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��4��2�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int SendNotifyExecuteCruiseActionToOnlineUser(unsigned int uCruiseID, char* cruise_name, int iType, DBOper* pCruise_Srv_dboper)
{
    int i = 0;
    int iRet = 0;
    int index = 0;
    CPacket outPacket;
    DOMElement* AccNode = NULL;
    char strCruiseID[32] = {0};
    vector<unsigned int> UserIndexVector;
    int iUserIndexCount = 0;
    unsigned int uUserIndex = 0;

    /*
     <?xml version="1.0"?>
         <Notify>
         <CmdType>ExecuteCruise</CmdType>
         <SN>1234</SN>
         <CruiseID>��ѯID</CruiseID>
         </Notify>
     */

    /* �齨XML��Ϣ */
    outPacket.SetRootTag("Notify");
    AccNode = outPacket.CreateElement((char*)"CmdType");
    outPacket.SetElementValue(AccNode, (char*)"ExecuteCruise");

    AccNode = outPacket.CreateElement((char*)"SN");
    outPacket.SetElementValue(AccNode, (char*)"1234");

    AccNode = outPacket.CreateElement((char*)"CruiseID");
    snprintf(strCruiseID, 32, "%u", uCruiseID);
    outPacket.SetElementValue(AccNode, strCruiseID);

    AccNode = outPacket.CreateElement((char*)"CruiseName");

    if (NULL == cruise_name)
    {
        outPacket.SetElementValue(AccNode, (char*)"");
    }
    else
    {
        outPacket.SetElementValue(AccNode, cruise_name);
    }

    AccNode = outPacket.CreateElement((char*)"RunFlag");

    if (iType == 0)
    {
        outPacket.SetElementValue(AccNode, (char*)"Start");
    }
    else
    {
        outPacket.SetElementValue(AccNode, (char*)"Stop");
    }

    /* ��ȡcruise�û�Ȩ�ޱ� */
    UserIndexVector.clear();
    iRet = get_user_index_from_user_cruise_config(strCruiseID, UserIndexVector, pCruise_Srv_dboper);

    iUserIndexCount = UserIndexVector.size();

    if (iUserIndexCount <= 0)
    {
        DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_WARN, "SendNotifyExecuteCruiseActionToOnlineUser() exit---: Get User Index NULL \r\n");
        return 0;
    }

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "����Ѳ��ִ��֪ͨ�����߿ͻ���: ��ѯ�����û���������=%d", iUserIndexCount);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Send cruising executing notice to the client online: The total number of queries to the user index=%d", iUserIndexCount);

    /* ѭ���������� */
    for (index = 0; index < iUserIndexCount; index++)
    {
        /* ��ȡ�û����� */
        uUserIndex = UserIndexVector[index];

        DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_TRACE, "SendNotifyExecuteCruiseActionToOnlineUser() index=%d, UserIndex=%u \r\n", index, uUserIndex);

        i |= SendMessageToOnlineUserByUserIndex(uUserIndex, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());
    }

    return i;
}

/*****************************************************************************
 �� �� ��  : UpdateCruiseConfigStatus2DB
 ��������  : ����Ѳ������״̬�����ݿ�
 �������  : int cruise_id
             int status
             DBOper* pCruise_Srv_dboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��10��31�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int UpdateCruiseConfigStatus2DB(int cruise_id, int status, DBOper* pCruise_Srv_dboper)
{
    int iRet = 0;
    string strSQL = "";
    char strCruiseID[64] = {0};
    char strStatus[16] = {0};

    //printf("\r\n UpdateUserRegInfo2DB() Enter--- \r\n");

    if (cruise_id <= 0)
    {
        DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_DEBUG, "UpdateCruiseConfigStatus2DB() exit---: Param Error \r\n");
        return -1;
    }

    if (NULL == pCruise_Srv_dboper)
    {
        DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_DEBUG, "UpdateCruiseConfigStatus2DB() exit---: Cruise Srv DB Oper Error \r\n");
        return -1;
    }

    snprintf(strCruiseID, 64, "%d", cruise_id);
    snprintf(strStatus, 16, "%d", status);

    /* �������ݿ� */
    strSQL.clear();
    strSQL = "UPDATE CruiseConfig SET Resved1 = ";
    strSQL += strStatus;
    strSQL += " WHERE ID = ";
    strSQL += strCruiseID;

    iRet = pCruise_Srv_dboper->DB_Update(strSQL.c_str(), 1);

    if (iRet < 0)
    {
        DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_ERROR, "UpdateCruiseConfigStatus2DB() DB Oper Error: strSQL=%s, iRet=%d \r\n", strSQL.c_str(), iRet);
        DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_ERROR, "UpdateCruiseConfigStatus2DB() ErrorMsg=%s\r\n", pCruise_Srv_dboper->GetLastDbErrorMsg());
    }

    return iRet;
}

/*****************************************************************************
 �� �� ��  : get_user_index_from_user_cruise_config
 ��������  : ���û�Ѳ��Ȩ�ޱ������ȡ�û�����
 �������  : char* pcCruiseID
             vector<unsigned int>& UserIndexVector
             DBOper* pDBOper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��1��5�� ����һ
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int get_user_index_from_user_cruise_config(char* pcCruiseID, vector<unsigned int>& UserIndexVector, DBOper* pDBOper)
{
    int iRet = 0;
    int record_count = 0;
    int while_count = 0;
    string strSQL = "";

    if (NULL == pcCruiseID || NULL == pDBOper)
    {
        DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_DEBUG, "get_user_index_from_user_cruise_config() exit---: Param Error \r\n");
        return -1;
    }

    strSQL.clear();
    strSQL = "SELECT UserID FROM UserCruiseConfig WHERE CruiseID = ";
    strSQL += pcCruiseID;

    record_count = pDBOper->DB_Select(strSQL.c_str(), 1);

    if (record_count < 0)
    {
        DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_ERROR, "get_user_index_from_user_cruise_config() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
        DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_ERROR, "get_user_index_from_user_cruise_config() ErrorMsg=%s\r\n", pDBOper->GetLastDbErrorMsg());
        return -1;
    }
    else if (record_count == 0)
    {
        //DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_WARN, "get_user_index_from_user_cruise_config() No Record \r\n");
        return 0;
    }

    /* ѭ���������ݿ�*/
    do
    {
        while_count++;

        if (while_count % 10000 == 0)
        {
            DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_WARN, "get_user_index_from_user_cruise_config() While Count=%d \r\n", while_count);
        }

        unsigned int uUserIndex = 0;

        pDBOper->GetFieldValue("UserID", uUserIndex);

        iRet = AddUserIndexToUserIndexVector(UserIndexVector, uUserIndex);
    }
    while (pDBOper->MoveNext() >= 0);

    return 0;
}

/*****************************************************************************
 �� �� ��  : ShowCruiseTaskInfo
 ��������  : ��ʾ��ǰѲ��������Ϣ
 �������  : int sock
             int status
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��8��7�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
void ShowCruiseTaskInfo(int sock, int status)
{
    int i = 0, j = 0;
    char strLine[] = "\r--------------------------------------------------------------------------------------------------------\r\n";
    char strHead[] = "\rCruiseID StartTime DurationTime DurationTimeCount CurrentPos DeviceIndex PresetID LiveTime LiveTimeCount\r\n";
    cruise_srv_t* pCruiseSrv = NULL;
    cruise_action_t* pCruiseAction = NULL;
    char rbuf[128] = {0};

    if (sock > 0)
    {
        send(sock, strLine, strlen(strLine), 0);
        send(sock, strHead, strlen(strHead), 0);
    }

    if ((NULL == g_CruiseSrvList) || (NULL == g_CruiseSrvList->pCruiseSrvList))
    {
        return;
    }

    CRUISE_SMUTEX_LOCK();

    if (osip_list_size(g_CruiseSrvList->pCruiseSrvList) <= 0)
    {
        CRUISE_SMUTEX_UNLOCK();
        return;
    }

    for (i = 0; i < osip_list_size(g_CruiseSrvList->pCruiseSrvList); i++)
    {
        pCruiseSrv = (cruise_srv_t*)osip_list_get(g_CruiseSrvList->pCruiseSrvList, i);

        if (NULL == pCruiseSrv || NULL == pCruiseSrv->pCruiseActionList)
        {
            continue;
        }

        if (status <= 1)
        {
            if (pCruiseSrv->status != status) /* û��Ѳ���ĺ��� */
            {
                continue;
            }
        }

        /* ���Ҿ���Ѳ������ */
        for (j = 0; j < osip_list_size(pCruiseSrv->pCruiseActionList); j++)
        {
            pCruiseAction = (cruise_action_t*)osip_list_get(pCruiseSrv->pCruiseActionList, j);

            if (NULL == pCruiseAction)
            {
                continue;
            }

            if (status <= 1)
            {
                if (1 == status && j != pCruiseSrv->current_pos)  /* ���ǵ�ǰ���ڵ�ǰѲ���ĺ��� */
                {
                    continue;
                }

                if (status != pCruiseAction->iStatus) /* û��������Ѳ������ */
                {
                    continue;
                }
            }

            snprintf(rbuf, 128, "\r%-8u %-9u %-12d %-17d %-10d %-11u %-8d %-8u %-13d\r\n", pCruiseSrv->cruise_id, pCruiseSrv->start_time, pCruiseSrv->duration_time, pCruiseSrv->duration_time_count, pCruiseSrv->current_pos, pCruiseAction->device_index, pCruiseAction->iPresetID, pCruiseAction->iLiveTime, pCruiseAction->iLiveTimeCount);

            if (sock > 0)
            {
                send(sock, rbuf, strlen(rbuf), 0);
            }
        }
    }

    CRUISE_SMUTEX_UNLOCK();

    if (sock > 0)
    {
        send(sock, strLine, strlen(strLine), 0);
    }

    return;
}

/*****************************************************************************
 �� �� ��  : StopCruiseTask
 ��������  : ֹͣѲ������
 �������  : int sock
             unsigned int cruise_id
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��8��7�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int StopCruiseTask(int sock, unsigned int cruise_id)
{
    int iRet = 0;
    char rbuf[128] = {0};

    /* ֹͣҵ�� */
    iRet = stop_cruise_srv_by_id(cruise_id);

    if (sock > 0)
    {
        memset(rbuf, 0, 128);

        if (0 == iRet)
        {
            snprintf(rbuf, 128, "\rֹͣѲ������ɹ�: Ѳ��ID=%u\r\n$", cruise_id);
            send(sock, rbuf, strlen(rbuf), 0);
        }
        else
        {
            snprintf(rbuf, 128, "\rֹͣѲ������ʧ��: Ѳ��ID=%u\r\n$", cruise_id);
            send(sock, rbuf, strlen(rbuf), 0);
        }
    }

    return 0;
}

/*****************************************************************************
 �� �� ��  : StopAllCruiseTask
 ��������  : ֹͣ����Ѳ������
 �������  : int sock
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��8��7�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int StopAllCruiseTask(int sock)
{
    int i = 0;
    int iRet = 0;
    cruise_srv_t* pCruiseSrv = NULL;
    needtoproc_cruisesrv_queue needToStop;
    char rbuf[128] = {0};

    if ((NULL == g_CruiseSrvList) || (NULL == g_CruiseSrvList->pCruiseSrvList))
    {
        return -1;
    }

    needToStop.clear();

    CRUISE_SMUTEX_LOCK();

    if (osip_list_size(g_CruiseSrvList->pCruiseSrvList) <= 0)
    {
        CRUISE_SMUTEX_UNLOCK();
        return 0;
    }

    for (i = 0; i < osip_list_size(g_CruiseSrvList->pCruiseSrvList); i++)
    {
        pCruiseSrv = (cruise_srv_t*)osip_list_get(g_CruiseSrvList->pCruiseSrvList, i);

        if (NULL == pCruiseSrv)
        {
            continue;
        }

        needToStop.push_back(pCruiseSrv);
        pCruiseSrv->duration_time_count = 0;
    }

    CRUISE_SMUTEX_UNLOCK();

    while (!needToStop.empty())
    {
        pCruiseSrv = (cruise_srv_t*) needToStop.front();
        needToStop.pop_front();

        if (NULL != pCruiseSrv)
        {
            if (1 == pCruiseSrv->status || 4 == pCruiseSrv->status)
            {
                pCruiseSrv->status = 3;
            }
            else if (2 == pCruiseSrv->status)
            {
                pCruiseSrv->status = 0;

                /* ֪ͨ�ͻ��� */ /* ֹͣѲ��������Ҫ���͸��ͻ��� */
                //iRet = SendNotifyExecuteCruiseActionToOnlineUser(pCruiseSrv->cruise_id, 1);

                pCruiseSrv->send_mark = 1;
            }

            if (sock > 0)
            {
                memset(rbuf, 0, 128);

                if (iRet >= 0)
                {
                    snprintf(rbuf, 128, "\rֹͣѲ������ɹ�: Ѳ��ID=%d\r\n", pCruiseSrv->cruise_id);
                    send(sock, rbuf, strlen(rbuf), 0);
                }
                else
                {
                    snprintf(rbuf, 128, "\rֹͣѲ������ʧ��: Ѳ��ID=%d\r\n", pCruiseSrv->cruise_id);
                    send(sock, rbuf, strlen(rbuf), 0);
                }
            }
        }
    }

    needToStop.clear();

    return 0;
}

/*****************************************************************************
 �� �� ��  : CruiseSrvConfig_db_refresh_proc
 ��������  : ����Ѳ��ҵ��������Ϣ���ݿ���²�����ʶ
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2016��10��18��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int CruiseSrvConfig_db_refresh_proc()
{
    if (1 == db_CruiseSrvInfo_reload_mark) /* ����ִ�� */
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Ѳ��ҵ���������ݿ���Ϣ����ͬ��");
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Cruise Srv Info database information are synchronized");
        return 0;
    }

    db_CruiseSrvInfo_reload_mark = 1;
    return 0;
}

/*****************************************************************************
 �� �� ��  : check_CruiseSrvConfig_need_to_reload_begin
 ��������  : ����Ƿ���Ҫͬ��Ѳ��ҵ�����ÿ�ʼ
 �������  : DBOper* pDboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2016��10��18��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
void check_CruiseSrvConfig_need_to_reload_begin(DBOper* pDboper)
{
    /* ����Ƿ���Ҫ�������ݿ��ʶ */
    if (!db_CruiseSrvInfo_reload_mark)
    {
        return;
    }

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "ͬ��Ѳ��ҵ���������ݿ���Ϣ: ��ʼ---");
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Synchronization Cruise Srv info database information: begain---");

    /* ����Ѳ�����е�ɾ����ʶ */
    set_cruise_srv_list_del_mark(1);

    /* �����ݿ��еı仯����ͬ�����ڴ� */
    check_db_data_to_cruise_srv_list(pDboper);

    return;
}

/*****************************************************************************
 �� �� ��  : check_CruiseSrvConfig_need_to_reload_end
 ��������  : ����Ƿ���Ҫͬ��Ѳ��ҵ�����ñ����
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2016��10��18��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
void check_CruiseSrvConfig_need_to_reload_end()
{
    /* ����Ƿ���Ҫ�������ݿ��ʶ */
    if (!db_CruiseSrvInfo_reload_mark)
    {
        return;
    }

    /* ɾ���Ѿ�ֹͣ����Ѳ���� */
    delete_cruise_srv_data();

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "ͬ��Ѳ��ҵ���������ݿ���Ϣ: ����---");
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Synchronization Cruise Srv info database information: end---");
    db_CruiseSrvInfo_reload_mark = 0;

    return;
}
