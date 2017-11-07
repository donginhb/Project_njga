
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

#include "user/user_srv_proc.inc"
#include "device/device_srv_proc.inc"

#include "service/poll_srv_proc.inc"

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
poll_srv_list_t* g_PollSrvList = NULL;    /* ��Ѳҵ����� */
int db_PollSrvInfo_reload_mark = 0;       /* ��Ѳҵ�����ݿ���±�ʶ:0:����Ҫ���£�1:��Ҫ�������ݿ� */

/*----------------------------------------------*
 * ��������                                     *
 *----------------------------------------------*/

/*----------------------------------------------*
 * �궨��                                       *
 *----------------------------------------------*/

#if DECS("��Ѳҵ�����")
/*****************************************************************************
 �� �� ��  : poll_action_source_init
 ��������  : ��Ѳ����Դ�ṹ��ʼ��
 �������  : poll_action_source_t** poll_action_source
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��4��3�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int poll_action_source_init(poll_action_source_t** poll_action_source)
{
    *poll_action_source = (poll_action_source_t*)osip_malloc(sizeof(poll_action_source_t));

    if (*poll_action_source == NULL)
    {
        DEBUG_TRACE(MODULE_POLL_SRV, LOG_DEBUG, "poll_action_source_init() exit---: *poll_srv Smalloc Error \r\n");
        return -1;
    }

    (*poll_action_source)->iStatus = 0;
    (*poll_action_source)->iType = 0;
    (*poll_action_source)->pcSourceID[0] = '\0';
    (*poll_action_source)->iSourceStreamType = 0;
    (*poll_action_source)->iLiveTime = 0;
    (*poll_action_source)->iLiveTimeCount = 0;
    (*poll_action_source)->iConnectFlag = 0;
    (*poll_action_source)->iConnectCount = 0;
    (*poll_action_source)->del_mark = 0;
    return 0;
}

/*****************************************************************************
 �� �� ��  : poll_action_source_free
 ��������  : ��Ѳ����Դ�ṹ�ͷ�
 �������  : poll_action_source_t* poll_action_source
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��4��3�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
void poll_action_source_free(poll_action_source_t* poll_action_source)
{
    if (poll_action_source == NULL)
    {
        //DEBUG_TRACE(MODULE_POLL_SRV, LOG_DEBUG, "poll_action_source_free() exit---: Param Error \r\n");
        return;
    }

    poll_action_source->iStatus = 0;
    poll_action_source->iType = 0;

    memset(poll_action_source->pcSourceID, 0, MAX_ID_LEN + 4);

    poll_action_source->iSourceStreamType = 0;
    poll_action_source->iLiveTime = 0;
    poll_action_source->iLiveTimeCount = 0;
    poll_action_source->iConnectFlag = 0;
    poll_action_source->iConnectCount = 0;
    poll_action_source->del_mark = 0;

    osip_free(poll_action_source);
    poll_action_source = NULL;

    return;
}

/*****************************************************************************
 �� �� ��  : poll_action_source_find
 ��������  : ����ԴID ��Ѳ����Դ����
 �������  : char* pcSourceID
             osip_list_t* pPollActionSourceList
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��4��2�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int poll_action_source_find(char* pcSourceID, osip_list_t* pPollActionSourceList)
{
    int i = 0;
    poll_action_source_t* pPollActionSource = NULL;

    if (NULL == pcSourceID || NULL == pPollActionSourceList)
    {
        DEBUG_TRACE(MODULE_POLL_SRV, LOG_DEBUG, "poll_action_source_find() exit---: Poll Action List Error \r\n");
        return -1;
    }

    for (i = 0; i < osip_list_size(pPollActionSourceList); i++)
    {
        pPollActionSource = (poll_action_source_t*)osip_list_get(pPollActionSourceList, i);

        if (NULL == pPollActionSource || '\0' == pPollActionSource->pcSourceID[0])
        {
            continue;
        }

        if (0 == sstrcmp(pPollActionSource->pcSourceID, pcSourceID))
        {
            return i;
        }
    }

    return -1;
}

/*****************************************************************************
 �� �� ��  : next_poll_action_source_get
 ��������  : ��ȡ��һ������ǽ��Ѳ������Դ��Ϣ
 �������  : int current_pos
             osip_list_t* pPollActionSourceList
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��11��25�� ���ڶ�
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
poll_action_source_t* next_poll_action_source_get(int current_pos, osip_list_t* pPollActionSourceList)
{
    int i = 0;
    int next_pos = 0;
    poll_action_source_t* pPollActionSource = NULL;

    if (current_pos < 0 || NULL == pPollActionSourceList)
    {
        DEBUG_TRACE(MODULE_POLL_SRV, LOG_DEBUG, "next_poll_action_source_get() exit---: Poll Action List Error \r\n");
        return NULL;
    }

    if (current_pos + 1 >= osip_list_size(pPollActionSourceList))
    {
        next_pos = 0;
    }
    else
    {
        next_pos = current_pos + 1;
    }

    /* �ӵ�ǰλ�ÿ�ʼ������� */
    for (i = next_pos; i < osip_list_size(pPollActionSourceList); i++)
    {
        pPollActionSource = (poll_action_source_t*)osip_list_get(pPollActionSourceList, i);

        if (NULL == pPollActionSource || '\0' == pPollActionSource->pcSourceID[0])
        {
            continue;
        }

        if (PLANACTION_TVWALL == pPollActionSource->iType)
        {
            return pPollActionSource;
        }
    }

    /* ����ӵ�ǰλ�ÿ�ʼ������ң�û���ҵ�����ô�ڴӿ�ʼ���ҵ���ǰλ�� */
    for (i = 0; i < next_pos; i++)
    {
        pPollActionSource = (poll_action_source_t*)osip_list_get(pPollActionSourceList, i);

        if (NULL == pPollActionSource || '\0' == pPollActionSource->pcSourceID[0])
        {
            continue;
        }

        if (PLANACTION_TVWALL == pPollActionSource->iType)
        {
            return pPollActionSource;
        }
    }

    return NULL;
}

/*****************************************************************************
 �� �� ��  : poll_action_init
 ��������  : ��Ѳ�����ṹ��ʼ��
 �������  : poll_action_t** poll_action
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��4��3�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int poll_action_init(poll_action_t** poll_action)
{
    *poll_action = (poll_action_t*)osip_malloc(sizeof(poll_action_t));

    if (*poll_action == NULL)
    {
        DEBUG_TRACE(MODULE_POLL_SRV, LOG_DEBUG, "poll_action_init() exit---: *poll_srv Smalloc Error \r\n");
        return -1;
    }

    (*poll_action)->poll_id = 0;
    (*poll_action)->pcDestID[0] = '\0';
    (*poll_action)->del_mark = 0;
    (*poll_action)->current_pos = 0;

    (*poll_action)->pPollActionSourceList = (osip_list_t*)osip_malloc(sizeof(osip_list_t));

    if (NULL == (*poll_action)->pPollActionSourceList)
    {
        osip_free(*poll_action);
        *poll_action = NULL;
        DEBUG_TRACE(MODULE_POLL_SRV, LOG_DEBUG, "poll_action_init() exit---: Poll Action List Init Error \r\n");
        return -1;
    }

    osip_list_init((*poll_action)->pPollActionSourceList);

    return 0;
}

/*****************************************************************************
 �� �� ��  : poll_action_free
 ��������  : ��Ѳ�����ṹ�ͷ�
 �������  : poll_action_t* poll_action
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��4��3�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
void poll_action_free(poll_action_t* poll_action)
{
    if (poll_action == NULL)
    {
        DEBUG_TRACE(MODULE_POLL_SRV, LOG_DEBUG, "poll_action_free() exit---: Param Error \r\n");
        return;
    }

    poll_action->poll_id = 0;

    memset(poll_action->pcDestID, 0, MAX_ID_LEN + 4);

    poll_action->del_mark = 0;
    poll_action->current_pos = 0;

    if (NULL != poll_action->pPollActionSourceList)
    {
        osip_list_special_free(poll_action->pPollActionSourceList, (void (*)(void*))&poll_action_source_free);
        osip_free(poll_action->pPollActionSourceList);
        poll_action->pPollActionSourceList = NULL;
    }

    osip_free(poll_action);
    poll_action = NULL;

    return;
}

/*****************************************************************************
 �� �� ��  : poll_action_find
 ��������  : ����Ŀ��ID������Ѳ����
 �������  : char* pcDestID
             osip_list_t* pPollActionList
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��4��3�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int poll_action_find(char* pcDestID, osip_list_t* pPollActionList)
{
    int i = 0;
    poll_action_t* pPollAction = NULL;

    if (NULL == pcDestID || NULL == pPollActionList)
    {
        DEBUG_TRACE(MODULE_POLL_SRV, LOG_DEBUG, "poll_action_find() exit---: Poll Action List Error \r\n");
        return -1;
    }

    for (i = 0; i < osip_list_size(pPollActionList); i++)
    {
        pPollAction = (poll_action_t*)osip_list_get(pPollActionList, i);

        if (NULL == pPollAction || '\0' == pPollAction->pcDestID[0])
        {
            continue;
        }

        if (0 == sstrcmp(pPollAction->pcDestID, pcDestID))
        {
            return i;
        }
    }

    return -1;
}

/*****************************************************************************
 �� �� ��  : poll_action_get
 ��������  : ��Ѳ������ȡ
 �������  : int pos
                            osip_list_t* pPollActionList
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��4��3�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
poll_action_t* poll_action_get(int pos, osip_list_t* pPollActionList)
{
    poll_action_t* pPollAction = NULL;

    if (NULL == pPollActionList)
    {
        DEBUG_TRACE(MODULE_POLL_SRV, LOG_DEBUG, "poll_action_find() exit---: Poll Action List Error \r\n");
        return NULL;
    }

    if (pos < 0 || (pos >= osip_list_size(pPollActionList)))
    {
        DEBUG_TRACE(MODULE_POLL_SRV, LOG_DEBUG, "poll_action_find() exit---: Pos Error \r\n");
        return NULL;
    }

    pPollAction = (poll_action_t*)osip_list_get(pPollActionList, pos);

    if (NULL == pPollAction)
    {
        return NULL;
    }
    else
    {
        return pPollAction;
    }
}

/*****************************************************************************
 �� �� ��  : poll_srv_init
 ��������  : ��Ѳҵ��ṹ��ʼ��
 �������  : poll_srv_t ** poll_srv
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��11��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int poll_srv_init(poll_srv_t** poll_srv)
{
    *poll_srv = (poll_srv_t*)osip_malloc(sizeof(poll_srv_t));

    if (*poll_srv == NULL)
    {
        DEBUG_TRACE(MODULE_POLL_SRV, LOG_DEBUG, "poll_srv_init() exit---: *poll_srv Smalloc Error \r\n");
        return -1;
    }

    (*poll_srv)->poll_id = 0;
    (*poll_srv)->status = 0;
    (*poll_srv)->poll_name[0] = '\0';
    (*poll_srv)->start_time = 0;
    (*poll_srv)->duration_time = 0;
    (*poll_srv)->duration_time_count = 0;
    (*poll_srv)->del_mark = 0;
    (*poll_srv)->send_mark = 0;

    (*poll_srv)->pPollActionList = (osip_list_t*)osip_malloc(sizeof(osip_list_t));

    if (NULL == (*poll_srv)->pPollActionList)
    {
        osip_free(*poll_srv);
        *poll_srv = NULL;
        DEBUG_TRACE(MODULE_POLL_SRV, LOG_DEBUG, "poll_srv_init() exit---: Poll Action List Init Error \r\n");
        return -1;
    }

    osip_list_init((*poll_srv)->pPollActionList);

    return 0;
}

/*****************************************************************************
 �� �� ��  : poll_srv_free
 ��������  : ��Ѳҵ��ṹ�ͷ�
 �������  : poll_srv_t * poll_srv
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��11��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void poll_srv_free(poll_srv_t* poll_srv)
{
    if (poll_srv == NULL)
    {
        //DEBUG_TRACE(MODULE_POLL_SRV, LOG_DEBUG, "poll_srv_free() exit---: Param Error \r\n");
        return;
    }

    poll_srv->poll_id = 0;
    poll_srv->status = 0;

    memset(poll_srv->poll_name, 0, MAX_128CHAR_STRING_LEN + 4);

    poll_srv->start_time = 0;
    poll_srv->duration_time = 0;
    poll_srv->duration_time_count = 0;
    poll_srv->del_mark = 0;
    poll_srv->send_mark = 0;

    if (NULL != poll_srv->pPollActionList)
    {
        osip_list_special_free(poll_srv->pPollActionList, (void (*)(void*))&poll_action_free);
        osip_free(poll_srv->pPollActionList);
        poll_srv->pPollActionList = NULL;
    }

    osip_free(poll_srv);
    poll_srv = NULL;

    return;
}

/*****************************************************************************
 �� �� ��  : poll_srv_find
 ��������  : ����ID������Ѳҵ��
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
int poll_srv_find(unsigned int id)
{
    int i = 0;
    poll_srv_t* pPollSrv = NULL;

    if (id <= 0 || NULL == g_PollSrvList || NULL == g_PollSrvList->pPollSrvList)
    {
        //DEBUG_TRACE(MODULE_POLL_SRV, LOG_DEBUG, "poll_srv_find() exit---: Poll Action List Error \r\n");
        return -1;
    }

    POLL_SMUTEX_LOCK();

    for (i = 0; i < osip_list_size(g_PollSrvList->pPollSrvList); i++)
    {
        pPollSrv = (poll_srv_t*)osip_list_get(g_PollSrvList->pPollSrvList, i);

        if (NULL == pPollSrv || pPollSrv->poll_id < 0)
        {
            continue;
        }

        if (pPollSrv->poll_id == id)
        {
            POLL_SMUTEX_UNLOCK();
            return i;
        }
    }

    POLL_SMUTEX_UNLOCK();
    return -1;
}

/*****************************************************************************
 �� �� ��  : poll_srv_get
 ��������  : ��ȡ��Ѳҵ��
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
poll_srv_t* poll_srv_get(int pos)
{
    poll_srv_t* pPollSrv = NULL;

    if (NULL == g_PollSrvList || NULL == g_PollSrvList->pPollSrvList)
    {
        DEBUG_TRACE(MODULE_POLL_SRV, LOG_DEBUG, "poll_srv_get() exit---: Poll Action List Error \r\n");
        return NULL;
    }

    if (pos < 0 || (pos >= osip_list_size(g_PollSrvList->pPollSrvList)))
    {
        DEBUG_TRACE(MODULE_POLL_SRV, LOG_DEBUG, "poll_srv_get() exit---: Pos Error \r\n");
        return NULL;
    }

    pPollSrv = (poll_srv_t*)osip_list_get(g_PollSrvList->pPollSrvList, pos);

    if (NULL == pPollSrv)
    {
        return NULL;
    }
    else
    {
        return pPollSrv;
    }
}

/*****************************************************************************
 �� �� ��  : poll_srv_list_init
 ��������  : ��Ѳҵ����г�ʼ��
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
int poll_srv_list_init()
{
    g_PollSrvList = (poll_srv_list_t*)osip_malloc(sizeof(poll_srv_list_t));

    if (g_PollSrvList == NULL)
    {
        DEBUG_TRACE(MODULE_POLL_SRV, LOG_DEBUG, "poll_srv_list_init() exit---: g_PollSrvList Smalloc Error \r\n");
        return -1;
    }

    g_PollSrvList->pPollSrvList = (osip_list_t*)osip_malloc(sizeof(osip_list_t));

    if (NULL == g_PollSrvList->pPollSrvList)
    {
        osip_free(g_PollSrvList);
        g_PollSrvList = NULL;
        DEBUG_TRACE(MODULE_POLL_SRV, LOG_DEBUG, "poll_srv_list_init() exit---: Poll Srv List Init Error \r\n");
        return -1;
    }

    osip_list_init(g_PollSrvList->pPollSrvList);

#ifdef MULTI_THR
    /* init smutex */
    g_PollSrvList->lock = (osip_mutex_t*)osip_mutex_init();

    if (NULL == g_PollSrvList->lock)
    {
        osip_free(g_PollSrvList->pPollSrvList);
        g_PollSrvList->pPollSrvList = NULL;
        osip_free(g_PollSrvList);
        g_PollSrvList = NULL;
        DEBUG_TRACE(MODULE_POLL_SRV, LOG_DEBUG, "poll_srv_list_init() exit---: Poll Srv List Lock Init Error \r\n");
        return -1;
    }

#endif
    return 0;
}

/*****************************************************************************
 �� �� ��  : poll_srv_list_free
 ��������  : ��Ѳҵ������ͷ�
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
void poll_srv_list_free()
{
    if (NULL == g_PollSrvList)
    {
        DEBUG_TRACE(MODULE_POLL_SRV, LOG_DEBUG, "poll_srv_list_free() exit---: Param Error \r\n");
        return;
    }

    if (NULL != g_PollSrvList->pPollSrvList)
    {
        osip_list_special_free(g_PollSrvList->pPollSrvList, (void (*)(void*))&poll_srv_free);
        osip_free(g_PollSrvList->pPollSrvList);
        g_PollSrvList->pPollSrvList = NULL;
    }

#ifdef MULTI_THR

    if (NULL != g_PollSrvList->lock)
    {
        osip_mutex_destroy((struct osip_mutex*)g_PollSrvList->lock);
        g_PollSrvList->lock = NULL;
    }

#endif
    osip_free(g_PollSrvList);
    g_PollSrvList = NULL;
    return;
}

/*****************************************************************************
 �� �� ��  : poll_srv_list_lock
 ��������  : ��Ѳҵ���������
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
int poll_srv_list_lock()
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_PollSrvList == NULL || g_PollSrvList->lock == NULL)
    {
        //DEBUG_TRACE(MODULE_POLL_SRV, LOG_DEBUG, "poll_srv_list_lock() exit---: Param Error \r\n");
        return -1;
    }

    iRet = osip_mutex_lock((struct osip_mutex*)g_PollSrvList->lock);
#endif
    return iRet;
}

/*****************************************************************************
 �� �� ��  : poll_srv_list_unlock
 ��������  : ��Ѳҵ�����
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
int poll_srv_list_unlock()
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_PollSrvList == NULL || g_PollSrvList->lock == NULL)
    {
        //DEBUG_TRACE(MODULE_POLL_SRV, LOG_DEBUG, "poll_srv_list_unlock() exit---: Param Error \r\n");
        return -1;
    }

    iRet = osip_mutex_unlock((struct osip_mutex*)g_PollSrvList->lock);

#endif
    return iRet;
}

/*****************************************************************************
 �� �� ��  : debug_poll_srv_list_lock
 ��������  : ��Ѳҵ���������
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
int debug_poll_srv_list_lock(const char* file, int line, const char* func)
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_PollSrvList == NULL || g_PollSrvList->lock == NULL)
    {
        DEBUG_TRACE(MODULE_POLL_SRV, LOG_DEBUG, "debug_poll_srv_list_lock() exit---: Param Error \r\n");
        return -1;
    }

    iRet = osip_debug_mutex_lock((struct osip_mutex*)g_PollSrvList->lock, file, line, func);

#endif
    return iRet;
}

/*****************************************************************************
 �� �� ��  : debug_poll_srv_list_unlock
 ��������  : ��Ѳҵ�����
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
int debug_poll_srv_list_unlock(const char* file, int line, const char* func)
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_PollSrvList == NULL || g_PollSrvList->lock == NULL)
    {
        DEBUG_TRACE(MODULE_POLL_SRV, LOG_DEBUG, "debug_poll_srv_list_unlock() exit---: Param Error \r\n");
        return -1;
    }

    iRet = osip_debug_mutex_unlock((struct osip_mutex*)g_PollSrvList->lock, file, line, func);

#endif
    return iRet;
}

/*****************************************************************************
 �� �� ��  : poll_srv_add
 ��������  : ��Ѳҵ�����
 �������  : poll_srv_t* pPollSrv
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��4��3�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int poll_srv_add(poll_srv_t* pPollSrv)
{
    int i = 0;

    if (pPollSrv == NULL)
    {
        //DEBUG_TRACE(MODULE_POLL_SRV, LOG_DEBUG, "poll_srv_add() exit---: Param Error \r\n");
        return -1;
    }

    POLL_SMUTEX_LOCK();

    i = osip_list_add(g_PollSrvList->pPollSrvList, pPollSrv, -1); /* add to list tail */

    //DEBUG_TRACE(MODULE_POLL_SRV, LOG_TRACE, "poll_srv_add() PollSrv:PollID=%u, StartTime=%d, DurationTime=%d, i=%d \r\n", pPollSrv->poll_id, pPollSrv->start_time, pPollSrv->duration_time, i);

    if (i < 0)
    {
        POLL_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_POLL_SRV, LOG_ERROR, "poll_srv_add() exit---: List Add Error \r\n");
        return -1;
    }

    POLL_SMUTEX_UNLOCK();
    return i - 1;
}

/*****************************************************************************
 �� �� ��  : poll_srv_remove
 ��������  : �Ӷ������Ƴ���Ѳҵ��
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
int poll_srv_remove(int pos)
{
    poll_srv_t* pPollSrv = NULL;

    POLL_SMUTEX_LOCK();

    if (g_PollSrvList == NULL || pos < 0 || (pos >= osip_list_size(g_PollSrvList->pPollSrvList)))
    {
        //DEBUG_TRACE(MODULE_POLL_SRV, LOG_DEBUG, "poll_srv_remove() exit---: Param Error \r\n");
        POLL_SMUTEX_UNLOCK();
        return -1;
    }

    pPollSrv = (poll_srv_t*)osip_list_get(g_PollSrvList->pPollSrvList, pos);

    if (NULL == pPollSrv)
    {
        //DEBUG_TRACE(MODULE_POLL_SRV, LOG_ERROR, "poll_srv_remove() exit---: List Get Error \r\n");
        POLL_SMUTEX_UNLOCK();
        return -1;
    }

    osip_list_remove(g_PollSrvList->pPollSrvList, pos);
    poll_srv_free(pPollSrv);
    pPollSrv = NULL;
    POLL_SMUTEX_UNLOCK();
    return 0;
}

/*****************************************************************************
 �� �� ��  : scan_poll_srv_list
 ��������  : ɨ����Ѳҵ����Ϣ����
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
void scan_poll_srv_list(DBOper* pPoll_Srv_dboper)
{
    int i = 0;
    int iRet = 0;
    poll_srv_t* pPollSrv = NULL;
    needtoproc_pollsrv_queue needToProc;
    needtoproc_pollsrv_queue needToStop;
    needtoproc_pollsrv_queue needToSend;

    if ((NULL == g_PollSrvList) || (NULL == g_PollSrvList->pPollSrvList))
    {
        DEBUG_TRACE(MODULE_POLL_SRV, LOG_DEBUG, "scan_poll_srv_list() exit---: Param Error \r\n");
        return;
    }

    needToProc.clear();
    needToStop.clear();
    needToSend.clear();

    POLL_SMUTEX_LOCK();

    if (osip_list_size(g_PollSrvList->pPollSrvList) <= 0)
    {
        POLL_SMUTEX_UNLOCK();
        return;
    }

    for (i = 0; i < osip_list_size(g_PollSrvList->pPollSrvList); i++)
    {
        pPollSrv = (poll_srv_t*)osip_list_get(g_PollSrvList->pPollSrvList, i);

        if (NULL == pPollSrv)
        {
            continue;
        }

        //DEBUG_TRACE(MODULE_POLL_SRV, LOG_TRACE, "scan_poll_srv_list() poll_id=%u, PollSrv: StartTime=%d, DurationTime=%d, DurationTimeCount=%d, DelMark=%d, Status=%d \r\n", pPollSrv->poll_id, pPollSrv->start_time, pPollSrv->duration_time, pPollSrv->duration_time_count, pPollSrv->del_mark, pPollSrv->status);

        if (1 == pPollSrv->del_mark) /* Ҫɾ�������� */
        {
            if (1 == pPollSrv->status || 4 == pPollSrv->status)
            {
                needToStop.push_back(pPollSrv);
                pPollSrv->duration_time_count = 0;
            }
            else
            {
                continue;
            }
        }

        if (2 == pPollSrv->status) /* ��Ҫ������Ѳ */
        {
            pPollSrv->status = 1;
            pPollSrv->duration_time_count = 0;
        }
        else if (3 == pPollSrv->status)  /* ��Ҫֹͣ��Ѳ */
        {
            needToStop.push_back(pPollSrv);
            pPollSrv->duration_time_count = 0;
        }
        else if (4 == pPollSrv->status)  /* ��Ҫ����֪ͨ���ͻ��� */
        {
            needToSend.push_back(pPollSrv);
            pPollSrv->status = 1;
        }

        if (1 == pPollSrv->status)  /* ���Ƿ�Ҫֹͣ��Ѳ */
        {
            if (pPollSrv->duration_time > 0 && pPollSrv->duration_time_count > 0
                && pPollSrv->duration_time_count >= pPollSrv->duration_time)
            {
                needToStop.push_back(pPollSrv);
                pPollSrv->duration_time_count = 0;
            }
            else
            {
                needToProc.push_back(pPollSrv);

                if (pPollSrv->duration_time > 0)
                {
                    pPollSrv->duration_time_count++;
                }
            }
        }
    }

    POLL_SMUTEX_UNLOCK();

    /* ������Ҫ��ʼ�� */
    while (!needToProc.empty())
    {
        pPollSrv = (poll_srv_t*) needToProc.front();
        needToProc.pop_front();

        if (NULL != pPollSrv)
        {
            iRet = poll_action_proc(pPollSrv->pPollActionList);

            if (iRet < 0)
            {
                DEBUG_TRACE(MODULE_POLL_SRV, LOG_ERROR, "scan_poll_srv_list() poll_action_proc Error:poll_id=%u, iRet=%d \r\n", pPollSrv->poll_id, iRet);
            }
            else
            {
                DEBUG_TRACE(MODULE_POLL_SRV, LOG_TRACE, "scan_poll_srv_list() poll_action_proc OK:poll_id=%u, iRet=%d \r\n", pPollSrv->poll_id, iRet);
            }

            if (0 == pPollSrv->send_mark)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "ִ����Ѳ����PC��Ļ��ʼ��Ѳ:PollID=%u, PollName=%s", pPollSrv->poll_id, pPollSrv->poll_name);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Perform started round tour round tour to send PC screen", pPollSrv->poll_id, pPollSrv->poll_name);

                /* ֪ͨ�ͻ��� */
                iRet = SendNotifyExecutePollActionToOnlineUser(pPollSrv->poll_id, pPollSrv->poll_name, 0, pPoll_Srv_dboper);

                if (iRet < 0)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "ִ����Ѳ����PC��Ļ��ʼ��Ѳʧ��:PollID=%u, PollName=%s", pPollSrv->poll_id, pPollSrv->poll_name);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Perform started round tour round tour to send PC screen failure:PollID=%u, PollName=%s", pPollSrv->poll_id, pPollSrv->poll_name);
                    DEBUG_TRACE(MODULE_POLL_SRV, LOG_ERROR, "scan_poll_srv_list() SendNotifyExecutePollActionToOnlineUser Error:  iRet=%d\r\n", iRet);
                }
                else if (iRet > 0)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "ִ����Ѳ����PC��Ļ��ʼ��Ѳ�ɹ�:PollID=%u, PollName=%s", pPollSrv->poll_id, pPollSrv->poll_name);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Perform round tour started round tour successfully sent PC screen:PollID=%u, PollName=%s", pPollSrv->poll_id, pPollSrv->poll_name);
                    DEBUG_TRACE(MODULE_POLL_SRV, LOG_TRACE, "scan_poll_srv_list() SendNotifyExecutePollActionToOnlineUser OK: iRet=%d\r\n", iRet);
                }

                pPollSrv->send_mark = 1;

                /* ����״̬ */
                iRet = UpdatePollConfigStatus2DB(pPollSrv->poll_id, 1, pPoll_Srv_dboper);

                if (iRet < 0)
                {
                    DEBUG_TRACE(MODULE_POLL_SRV, LOG_ERROR, "scan_poll_srv_list() UpdatePollConfigStatus2DB Start Error: poll_id=%u, iRet=%d \r\n", pPollSrv->poll_id, iRet);
                }
                else
                {
                    DEBUG_TRACE(MODULE_POLL_SRV, LOG_TRACE, "scan_poll_srv_list() UpdatePollConfigStatus2DB Start OK: poll_id=%u, iRet=%d \r\n", pPollSrv->poll_id, iRet);
                }
            }
        }
    }

    needToProc.clear();

    /* ������Ҫ���͵� */
    while (!needToSend.empty())
    {
        pPollSrv = (poll_srv_t*) needToSend.front();
        needToSend.pop_front();

        if (NULL != pPollSrv)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "ִ����Ѳ����PC��Ļ��ʼ��Ѳ:PollID=%u, PollName=%s", pPollSrv->poll_id, pPollSrv->poll_name);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Perform started round tour round tour to send PC screen:PollID=%u, PollName=%s", pPollSrv->poll_id, pPollSrv->poll_name);

            /* ֪ͨ�ͻ��� */
            iRet = SendNotifyExecutePollActionToOnlineUser(pPollSrv->poll_id, pPollSrv->poll_name, 0, pPoll_Srv_dboper);

            if (iRet < 0)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "ִ����Ѳ����PC��Ļ��ʼ��Ѳʧ��:PollID=%u, PollName=%s", pPollSrv->poll_id, pPollSrv->poll_name);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Perform started round tour round tour to send PC screen failure:PollID=%u, PollName=%s", pPollSrv->poll_id, pPollSrv->poll_name);
                DEBUG_TRACE(MODULE_POLL_SRV, LOG_ERROR, "scan_poll_srv_list() SendNotifyExecutePollActionToOnlineUser Error: iRet=%d\r\n", iRet);
            }
            else if (iRet > 0)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "ִ����Ѳ����PC��Ļ��ʼ��Ѳ�ɹ�:PollID=%u, PollName=%s", pPollSrv->poll_id, pPollSrv->poll_name);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Perform round tour started round tour successfully sent PC screen:PollID=%u, PollName=%s", pPollSrv->poll_id, pPollSrv->poll_name);
                DEBUG_TRACE(MODULE_POLL_SRV, LOG_TRACE, "scan_poll_srv_list() SendNotifyExecutePollActionToOnlineUser OK: iRet=%d\r\n", iRet);
            }
        }
    }

    needToSend.clear();

    /* ������Ҫֹͣ�� */
    while (!needToStop.empty())
    {
        pPollSrv = (poll_srv_t*) needToStop.front();
        needToStop.pop_front();

        if (NULL != pPollSrv)
        {
            iRet = poll_action_stop(pPollSrv->pPollActionList);

            if (iRet < 0)
            {
                DEBUG_TRACE(MODULE_POLL_SRV, LOG_ERROR, "scan_poll_srv_list() poll_action_stop Error:poll_id=%u, iRet=%d \r\n", pPollSrv->poll_id, iRet);
            }
            else
            {
                DEBUG_TRACE(MODULE_POLL_SRV, LOG_TRACE, "scan_poll_srv_list() poll_action_stop OK:poll_id=%u, iRet=%d \r\n", pPollSrv->poll_id, iRet);
            }

            if (iRet >= 0)
            {
                pPollSrv->status = 0;
                pPollSrv->duration_time_count = 0;

#if 1 /* ����ĵ���ǽ��Ѳ����Ҫ֪ͨ�ͻ��� */

                if (iRet > 0) /* ����ĵ���ǽ��Ѳ����Ҫ֪ͨ�ͻ��� */
                {
                    if (1 == pPollSrv->send_mark) /* �û��ֶ�ֹͣ�Ĳ���Ҫ���͸��ͻ��� */
                    {
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "ִ����Ѳ����PC��Ļֹͣ��Ѳ:PollID=%u, PollName=%s", pPollSrv->poll_id, pPollSrv->poll_name);
                        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "SendNotifyExecutePollActionStopPCPollAction:PollID=%u, PollName=%s", pPollSrv->poll_id, pPollSrv->poll_name);

                        /* ֪ͨ�ͻ��� */
                        iRet = SendNotifyExecutePollActionToOnlineUser(pPollSrv->poll_id, pPollSrv->poll_name, 1, pPoll_Srv_dboper);

                        if (iRet < 0)
                        {
                            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "ִ����Ѳ����PC��Ļֹͣ��Ѳʧ��:PollID=%u, PollName=%s", pPollSrv->poll_id, pPollSrv->poll_name);
                            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "SendNotifyExecutePollActionStopPCPollAction Error:PollID=%u, PollName=%s", pPollSrv->poll_id, pPollSrv->poll_name);
                            DEBUG_TRACE(MODULE_POLL_SRV, LOG_ERROR, "scan_poll_srv_list() SendNotifyExecutePollActionToOnlineUser Error: iRet=%d\r\n", iRet);
                        }
                        else if (iRet > 0)
                        {
                            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "ִ����Ѳ����PC��Ļֹͣ��Ѳ�ɹ�:PollID=%u, PollName=%s", pPollSrv->poll_id, pPollSrv->poll_name);
                            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "SendNotifyExecutePollActionStopPCPollAction ok:PollID=%u, PollName=%s", pPollSrv->poll_id, pPollSrv->poll_name);
                            DEBUG_TRACE(MODULE_POLL_SRV, LOG_TRACE, "scan_poll_srv_list() SendNotifyExecutePollActionToOnlineUser OK: iRet=%d\r\n", iRet);
                        }
                    }
                }

#endif
                /* ����״̬ */
                iRet = UpdatePollConfigStatus2DB(pPollSrv->poll_id, 0, pPoll_Srv_dboper);

                if (iRet < 0)
                {
                    DEBUG_TRACE(MODULE_POLL_SRV, LOG_ERROR, "scan_poll_srv_list() UpdatePollConfigStatus2DB Start Error: poll_id=%u, iRet=%d \r\n", pPollSrv->poll_id, iRet);
                }
                else
                {
                    DEBUG_TRACE(MODULE_POLL_SRV, LOG_TRACE, "scan_poll_srv_list() UpdatePollConfigStatus2DB Start OK: poll_id=%u, iRet=%d \r\n", pPollSrv->poll_id, iRet);
                }

                pPollSrv->send_mark = 0;
            }
        }
    }

    needToStop.clear();

    return;
}
#endif

/*****************************************************************************
 �� �� ��  : poll_action_stop
 ��������  : ֹͣԤ������
 �������  : osip_list_t* pPollActionList
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��4��2�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int poll_action_stop(osip_list_t* pPollActionList)
{
    //int iRet = 0;
    int i = 0, j = 0;
    poll_action_t* pPollAction = NULL;
    poll_action_source_t* pPollActionSource = NULL;
    int iNotifyUserFlag = 1;

    if (NULL == pPollActionList)
    {
        DEBUG_TRACE(MODULE_POLL_SRV, LOG_DEBUG, "poll_action_stop() exit---: Poll Action List Error \r\n");
        return -1;
    }

    for (i = 0; i < osip_list_size(pPollActionList); i++)
    {
        pPollAction = (poll_action_t*)osip_list_get(pPollActionList, i);

        if (NULL == pPollAction)
        {
            continue;
        }

        DEBUG_TRACE(MODULE_POLL_SRV, LOG_TRACE, "poll_action_stop() PollAction: PollID=%u, DestID=%s, CurrentPos=%d \r\n", pPollAction->poll_id, pPollAction->pcDestID, pPollAction->current_pos);

        for (j = 0; j < osip_list_size(pPollAction->pPollActionSourceList); j++)
        {
            pPollActionSource = (poll_action_source_t*)osip_list_get(pPollAction->pPollActionSourceList, j);

            if (NULL == pPollActionSource)
            {
                continue;
            }

            DEBUG_TRACE(MODULE_POLL_SRV, LOG_TRACE, "poll_action_stop() PollActionSource: Type=%d, SourceID=%s, LiveTime=%d, LiveTimeCount=%d, Status=%d \r\n", pPollActionSource->iType, pPollActionSource->pcSourceID, pPollActionSource->iLiveTime, pPollActionSource->iLiveTimeCount, pPollActionSource->iStatus);

            if (PLANACTION_PC == pPollActionSource->iType)
            {
                iNotifyUserFlag = 0; /* ��PC��Ļ��Ѳ������Ҫ֪ͨ�ͻ��� */
            }

            if (1 == pPollActionSource->iStatus)
            {
#if 0 /* ֹͣ����ǽ��Ѳ��ʱ�򣬲���ֹͣ���� */

                iRet = StopDecService(pPollActionSource->pcSourceID, pPollAction->pcDestID);

                if (0 != iRet)
                {
                    DEBUG_TRACE(MODULE_POLL_SRV, LOG_ERROR, "poll_action_stop() StopDecService Error: DestID=%s, SourceID=%s, iRet=%d\r\n", pPollAction->pcDestID, pPollActionSource->pcSourceID, iRet);
                }
                else
                {
                    DEBUG_TRACE(MODULE_POLL_SRV, LOG_TRACE, "poll_action_stop() StopDecService OK: DestID=%s, SourceID=%s, iRet=%d\r\n", pPollAction->pcDestID, pPollActionSource->pcSourceID, iRet);
                }

#endif
                pPollActionSource->iStatus = 0;
                pPollActionSource->iLiveTimeCount = 0;
                //iNotifyUserFlag = 1;
            }
        }

        pPollAction->current_pos = 0;
    }

    return iNotifyUserFlag;
}

/*****************************************************************************
 �� �� ��  : poll_action_proc
 ��������  : ִ��Ԥ������
 �������  : osip_list_t* pPollActionList
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��4��2�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int poll_action_proc(osip_list_t* pPollActionList)
{
    int iRet = 0;
    int i = 0, j = 0;
    poll_action_t* pPollAction = NULL;
    poll_action_source_t* pPollActionSource = NULL;
    poll_action_source_t* pNextPollActionSource = NULL;
    GBLogicDevice_info_t* pDestGBLogicDeviceInfo = NULL;
    GBDevice_info_t* pDestGBDeviceInfo = NULL;
    int iNotifyUserFlag = 0;

    if (NULL == pPollActionList)
    {
        DEBUG_TRACE(MODULE_POLL_SRV, LOG_DEBUG, "poll_action_proc() exit---: Poll Action List Error \r\n");
        return -1;
    }

    for (i = 0; i < osip_list_size(pPollActionList); i++)
    {
        pPollAction = (poll_action_t*)osip_list_get(pPollActionList, i);

        if (NULL == pPollAction)
        {
            continue;
        }

        DEBUG_TRACE(MODULE_POLL_SRV, LOG_TRACE, "poll_action_proc() PollAction: PollID=%u, DestID=%s, CurrentPos=%d, del_mark=%d \r\n", pPollAction->poll_id, pPollAction->pcDestID, pPollAction->current_pos, pPollAction->del_mark);

        if (1 == pPollAction->del_mark) /* ��Ҫɾ���ģ����Ƿ���Ҫֹͣ */
        {
            for (j = 0; j < osip_list_size(pPollAction->pPollActionSourceList); j++)
            {
                pPollActionSource = (poll_action_source_t*)osip_list_get(pPollAction->pPollActionSourceList, j);

                if (NULL != pPollActionSource && j == pPollAction->current_pos)
                {
                    if (1 == pPollActionSource->iStatus)
                    {
                        iRet = StopDecService(pPollActionSource->pcSourceID, pPollAction->pcDestID);

                        if (0 != iRet)
                        {
                            DEBUG_TRACE(MODULE_POLL_SRV, LOG_ERROR, "poll_action_proc() StopDecService Error: DestID=%s, SourceID=%s, CurrentPos=%d, iRet=%d \r\n", pPollAction->pcDestID, pPollActionSource->pcSourceID, pPollAction->current_pos, iRet);
                        }
                        else
                        {
                            DEBUG_TRACE(MODULE_POLL_SRV, LOG_TRACE, "poll_action_proc() StopDecService OK: DestID=%s, SourceID=%s, CurrentPos=%d, iRet=%d \r\n", pPollAction->pcDestID, pPollActionSource->pcSourceID, pPollAction->current_pos, iRet);
                        }

                        pPollActionSource->iStatus = 0;
                        iNotifyUserFlag = 2;
                    }
                }
            }
        }
        else
        {
            for (j = 0; j < osip_list_size(pPollAction->pPollActionSourceList); j++)
            {
                pPollActionSource = (poll_action_source_t*)osip_list_get(pPollAction->pPollActionSourceList, j);

                if (NULL == pPollActionSource)
                {
                    DEBUG_TRACE(MODULE_POLL_SRV, LOG_ERROR, "poll_action_proc() Get PollActionSource Error\r\n");
                    pPollAction->current_pos++;
                    continue;
                }

                DEBUG_TRACE(MODULE_POLL_SRV, LOG_TRACE, "poll_action_proc() PollActionSource: Type=%d, SourceID=%s, LiveTime=%d, LiveTimeCount=%d, Status=%d, del_mark=%d \r\n", pPollActionSource->iType, pPollActionSource->pcSourceID, pPollActionSource->iLiveTime, pPollActionSource->iLiveTimeCount, pPollActionSource->iStatus, pPollActionSource->del_mark);

                if (PLANACTION_TVWALL == pPollActionSource->iType) /* ����ǽ��Ѳ */
                {
                    /* ��ȡĿ�Ķ˵��豸��Ϣ */
                    pDestGBLogicDeviceInfo = GBLogicDevice_info_find(pPollAction->pcDestID);

                    if (NULL == pDestGBLogicDeviceInfo)
                    {
                        DEBUG_TRACE(MODULE_POLL_SRV, LOG_ERROR, "poll_action_proc() Get DestGBLogicDeviceInfo Error\r\n");
                        pPollAction->current_pos++;
                        continue;
                    }

                    pDestGBDeviceInfo = GBDevice_info_get_by_stream_type(pDestGBLogicDeviceInfo, EV9000_STREAM_TYPE_MASTER);

                    if (NULL == pDestGBDeviceInfo || EV9000_DEVICETYPE_DECODER != pDestGBDeviceInfo->device_type) /* ֻ�е���ǽ�Ų��� */
                    {
                        DEBUG_TRACE(MODULE_POLL_SRV, LOG_ERROR, "poll_action_proc() DestGBDeviceInfo OR DestGBDeviceInfo Ddevice Type Error, device_id=%s, device_type=%d\r\n", pDestGBDeviceInfo->device_id, pDestGBDeviceInfo->device_type);
                        pPollAction->current_pos++;
                        continue;
                    }

                    if (j == pPollAction->current_pos) /* ���ڵ�ǰ��Ѳ */
                    {
                        if (1 == pPollActionSource->del_mark) /* ��Ҫɾ���ģ���ǰ������Ѳ�ģ�ɾ���˵ģ���Ҫֹͣ�� */
                        {
                            if (1 == pPollActionSource->iStatus)
                            {
                                pPollActionSource->iLiveTimeCount = 0;
                                pPollAction->current_pos = j + 1;

                                /* ������һ����Ѳ��ԴID�Ƿ�ͱ��ε�һ�������һ�����Ͳ���Ҫ�ر� */
                                pNextPollActionSource = next_poll_action_source_get(j, pPollAction->pPollActionSourceList);

                                if (NULL != pNextPollActionSource)
                                {
                                    if ((pNextPollActionSource->iSourceStreamType == pPollActionSource->iSourceStreamType)
                                        && (0 == sstrcmp(pNextPollActionSource->pcSourceID, pPollActionSource->pcSourceID)))
                                    {
                                        pNextPollActionSource->iConnectFlag = 1;
                                    }
                                    else
                                    {
                                        iRet = StopDecService(pPollActionSource->pcSourceID, pPollAction->pcDestID);

                                        if (0 != iRet)
                                        {
                                            DEBUG_TRACE(MODULE_POLL_SRV, LOG_ERROR, "poll_action_proc() StopDecService Error: DestID=%s, SourceID=%s, CurrentPos=%d, iRet=%d \r\n", pPollAction->pcDestID, pPollActionSource->pcSourceID, pPollAction->current_pos, iRet);
                                        }
                                        else
                                        {
                                            DEBUG_TRACE(MODULE_POLL_SRV, LOG_TRACE, "poll_action_proc() StopDecService OK: DestID=%s, SourceID=%s, CurrentPos=%d, iRet=%d \r\n", pPollAction->pcDestID, pPollActionSource->pcSourceID, pPollAction->current_pos, iRet);
                                        }
                                    }
                                }
                                else
                                {
                                    iRet = StopDecService(pPollActionSource->pcSourceID, pPollAction->pcDestID);

                                    if (0 != iRet)
                                    {
                                        DEBUG_TRACE(MODULE_POLL_SRV, LOG_ERROR, "poll_action_proc() StopDecService Error: DestID=%s, SourceID=%s, CurrentPos=%d, iRet=%d \r\n", pPollAction->pcDestID, pPollActionSource->pcSourceID, pPollAction->current_pos, iRet);
                                    }
                                    else
                                    {
                                        DEBUG_TRACE(MODULE_POLL_SRV, LOG_TRACE, "poll_action_proc() StopDecService OK: DestID=%s, SourceID=%s, CurrentPos=%d, iRet=%d \r\n", pPollAction->pcDestID, pPollActionSource->pcSourceID, pPollAction->current_pos, iRet);
                                    }
                                }

                                pPollActionSource->iStatus = 0;
                                pPollActionSource->iConnectFlag = 0;
                                iNotifyUserFlag = 2;
                            }
                        }
                        else
                        {
                            if (0 == pPollActionSource->iStatus)
                            {
                                if (0 == pPollActionSource->iConnectFlag)
                                {
                                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "ִ����Ѳ����ǽ:����ǽͨ��ID=%s, �߼��豸ID=%s", pPollAction->pcDestID, pPollActionSource->pcSourceID);
                                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "run polling open TV wall: TV wall channel ID=%s, logic device ID=%s", pPollAction->pcDestID, pPollActionSource->pcSourceID);

                                    if (pPollActionSource->iSourceStreamType <= 0)
                                    {
                                        iRet = start_connect_tv_proc(pPollActionSource->pcSourceID, EV9000_STREAM_TYPE_MASTER, pPollAction->pcDestID, 0);
                                    }
                                    else
                                    {
                                        iRet = start_connect_tv_proc(pPollActionSource->pcSourceID, pPollActionSource->iSourceStreamType, pPollAction->pcDestID, 0);
                                    }

                                    if (iRet < 0)
                                    {
                                        pPollActionSource->iConnectFlag = 0;
                                        DEBUG_TRACE(MODULE_POLL_SRV, LOG_ERROR, "poll_action_proc() start_connect_tv_proc Error: DestID=%s, SourceID=%s, CurrentPos=%d, iRet=%d\r\n", pDestGBLogicDeviceInfo->device_id, pPollActionSource->pcSourceID, pPollAction->current_pos, iRet);
                                    }
                                    else
                                    {
                                        pPollActionSource->iConnectFlag = 1;
                                        DEBUG_TRACE(MODULE_POLL_SRV, LOG_INFO, "poll_action_proc() start_connect_tv_proc OK: DestID=%s, SourceID=%s, CurrentPos=%d, iRet=%d\r\n", pDestGBLogicDeviceInfo->device_id, pPollActionSource->pcSourceID, pPollAction->current_pos, iRet);
                                    }
                                }

                                pPollActionSource->iStatus = 1;
                                iNotifyUserFlag = 1;
                            }
                            else
                            {
                                if (pPollActionSource->iConnectFlag == 0) /* ���û�����ӳɹ�����һֱ���� */
                                {
                                    if (pPollActionSource->iConnectCount >= 10)
                                    {
                                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "ִ����Ѳ����ǽ:����ǽͨ��ID=%s, �߼��豸ID=%s", pPollAction->pcDestID, pPollActionSource->pcSourceID);
                                        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "run polling open TV wall: TV wall channelID=%s, logic device ID=%s", pPollAction->pcDestID, pPollActionSource->pcSourceID);

                                        if (pPollActionSource->iSourceStreamType <= 0)
                                        {
                                            iRet = start_connect_tv_proc(pPollActionSource->pcSourceID, EV9000_STREAM_TYPE_MASTER, pPollAction->pcDestID, 0);
                                        }
                                        else
                                        {
                                            iRet = start_connect_tv_proc(pPollActionSource->pcSourceID, pPollActionSource->iSourceStreamType, pPollAction->pcDestID, 0);
                                        }

                                        if (iRet < 0)
                                        {
                                            pPollActionSource->iConnectFlag = 0;
                                            DEBUG_TRACE(MODULE_POLL_SRV, LOG_ERROR, "poll_action_proc() start_connect_tv_proc Error: DestID=%s, SourceID=%s, CurrentPos=%d, iRet=%d\r\n", pDestGBLogicDeviceInfo->device_id, pPollActionSource->pcSourceID, pPollAction->current_pos, iRet);
                                        }
                                        else
                                        {
                                            pPollActionSource->iConnectFlag = 1;
                                            DEBUG_TRACE(MODULE_POLL_SRV, LOG_INFO, "poll_action_proc() start_connect_tv_proc OK: DestID=%s, SourceID=%s, CurrentPos=%d, iRet=%d\r\n", pDestGBLogicDeviceInfo->device_id, pPollActionSource->pcSourceID, pPollAction->current_pos, iRet);
                                        }

                                        pPollActionSource->iConnectCount = 0;
                                    }
                                    else
                                    {
                                        pPollActionSource->iConnectCount++;
                                    }
                                }

                                pPollActionSource->iLiveTimeCount++;
                                DEBUG_TRACE(MODULE_POLL_SRV, LOG_TRACE, "poll_action_proc() DestID=%s, SourceID=%s, LiveTimeCount=%d, CurrentPos=%d\r\n", pDestGBLogicDeviceInfo->device_id, pPollActionSource->pcSourceID, pPollActionSource->iLiveTimeCount, pPollAction->current_pos);
                            }

                            if (pPollActionSource->iLiveTimeCount >= pPollActionSource->iLiveTime)
                            {
                                pPollActionSource->iLiveTimeCount = 0;
                                pPollAction->current_pos = j + 1;

                                /* ������һ����Ѳ��ԴID�Ƿ�ͱ��ε�һ�������һ�����Ͳ���Ҫ�ر� */
                                pNextPollActionSource = next_poll_action_source_get(j, pPollAction->pPollActionSourceList);

                                if (NULL != pNextPollActionSource)
                                {
                                    if ((pNextPollActionSource->iSourceStreamType == pPollActionSource->iSourceStreamType)
                                        && (0 == sstrcmp(pNextPollActionSource->pcSourceID, pPollActionSource->pcSourceID)))
                                    {
                                        pNextPollActionSource->iConnectFlag = 1;
                                    }
                                    else
                                    {
                                        iRet = StopDecService(pPollActionSource->pcSourceID, pPollAction->pcDestID);

                                        if (0 != iRet)
                                        {
                                            DEBUG_TRACE(MODULE_POLL_SRV, LOG_ERROR, "poll_action_proc() StopDecService Error: DestID=%s, SourceID=%s, CurrentPos=%d, iRet=%d \r\n", pPollAction->pcDestID, pPollActionSource->pcSourceID, pPollAction->current_pos, iRet);
                                        }
                                        else
                                        {
                                            DEBUG_TRACE(MODULE_POLL_SRV, LOG_TRACE, "poll_action_proc() StopDecService OK: DestID=%s, SourceID=%s, CurrentPos=%d, iRet=%d \r\n", pPollAction->pcDestID, pPollActionSource->pcSourceID, pPollAction->current_pos, iRet);
                                        }
                                    }
                                }
                                else
                                {
                                    iRet = StopDecService(pPollActionSource->pcSourceID, pPollAction->pcDestID);

                                    if (0 != iRet)
                                    {
                                        DEBUG_TRACE(MODULE_POLL_SRV, LOG_ERROR, "poll_action_proc() StopDecService Error: DestID=%s, SourceID=%s, CurrentPos=%d, iRet=%d \r\n", pPollAction->pcDestID, pPollActionSource->pcSourceID, pPollAction->current_pos, iRet);
                                    }
                                    else
                                    {
                                        DEBUG_TRACE(MODULE_POLL_SRV, LOG_TRACE, "poll_action_proc() StopDecService OK: DestID=%s, SourceID=%s, CurrentPos=%d, iRet=%d \r\n", pPollAction->pcDestID, pPollActionSource->pcSourceID, pPollAction->current_pos, iRet);
                                    }
                                }

                                pPollActionSource->iStatus = 0;
                                pPollActionSource->iConnectFlag = 0;
                                iNotifyUserFlag = 2;
                            }
                        }
                    }
                }
                else if (PLANACTION_PC == pPollActionSource->iType) /* PC��Ļ��Ѳ,���� */
                {
                    DEBUG_TRACE(MODULE_POLL_SRV, LOG_TRACE, "poll_action_proc() PollActionSource Type is PC Poll\r\n");
                    pPollAction->current_pos++;
                    iNotifyUserFlag = 1;
                    continue;
                }
                else
                {
                    DEBUG_TRACE(MODULE_POLL_SRV, LOG_TRACE, "poll_action_proc() PollActionSource Type is Unknow:Type=%d\r\n", pPollActionSource->iType);
                    pPollAction->current_pos++;

                    continue;
                }

                if (pPollAction->current_pos >= osip_list_size(pPollAction->pPollActionSourceList))
                {
                    DEBUG_TRACE(MODULE_POLL_SRV, LOG_TRACE, "poll_action_proc() CurrentPos=%d \r\n", pPollAction->current_pos, iRet);
                    pPollAction->current_pos = 0;
                }
            }
        }
    }

    return iNotifyUserFlag;
}

/*****************************************************************************
 �� �� ��  : start_poll_srv_by_id
 ��������  : ����ID������Ѳ����
 �������  : user_info_t* pUserInfo
             int id
             DBOper* pPoll_Srv_dboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��4��2�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int start_poll_srv_by_id(user_info_t* pUserInfo, unsigned int id, DBOper* pPoll_Srv_dboper)
{
    int i = 0;
    int iRet = 0;
    int poll_pos = -1;
    string strSQL = "";
    int record_count = 0;
    poll_srv_t* pPollSrv = NULL;
    char strPollID[32] = {0};

    if (NULL == pPoll_Srv_dboper || NULL == pUserInfo)
    {
        return -1;
    }

    poll_pos = poll_srv_find(id);

    DEBUG_TRACE(MODULE_POLL_SRV, LOG_TRACE, "start_poll_srv_by_id() poll_srv_find:id=%d, poll_pos=%d \r\n", id, poll_pos);

    if (poll_pos < 0)
    {
        snprintf(strPollID, 32, "%u", id);
        strSQL.clear();
        strSQL = "select * from PollConfig WHERE ID = ";
        strSQL += strPollID;

        record_count = pPoll_Srv_dboper->DB_Select(strSQL.c_str(), 1);

        if (record_count < 0)
        {
            DEBUG_TRACE(MODULE_POLL_SRV, LOG_ERROR, "start_poll_srv_by_id() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
            DEBUG_TRACE(MODULE_POLL_SRV, LOG_ERROR, "start_poll_srv_by_id() ErrorMsg=%s\r\n", pPoll_Srv_dboper->GetLastDbErrorMsg());
            return -1;
        }
        else if (record_count == 0)
        {
            DEBUG_TRACE(MODULE_POLL_SRV, LOG_WARN, "start_poll_srv_by_id() exit---: No Record Count \r\n");
            return 0;
        }


        unsigned int uPollID = 0;
        string strPollName = "";
        int iStartTime = 0;
        int iDurationTime = 0;

        pPoll_Srv_dboper->GetFieldValue("ID", uPollID);
        pPoll_Srv_dboper->GetFieldValue("PollName", strPollName);
        pPoll_Srv_dboper->GetFieldValue("StartTime", iStartTime);
        pPoll_Srv_dboper->GetFieldValue("DurationTime", iDurationTime);

        DEBUG_TRACE(MODULE_POLL_SRV, LOG_TRACE, "start_poll_srv_by_id() PollID=%u, PollName=%s, StartTime=%d, DurationTime=%d \r\n", uPollID, strPollName.c_str(), iStartTime, iDurationTime);

        i = poll_srv_init(&pPollSrv);

        if (i != 0)
        {
            DEBUG_TRACE(MODULE_POLL_SRV, LOG_ERROR, "start_poll_srv_by_id() poll_srv_init:i=%d \r\n", i);
            return -1;
        }

        pPollSrv->poll_id = uPollID;

        if (!strPollName.empty())
        {
            osip_strncpy(pPollSrv->poll_name, (char*)strPollName.c_str(), MAX_128CHAR_STRING_LEN);
        }

        pPollSrv->start_time = iStartTime;
        pPollSrv->duration_time = iDurationTime;
        pPollSrv->status = 2;

        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "����ֶ�����ִ�е���Ѳ: poll_id=%d, poll_name=%s, start_time=%d, duration_time=%d", pPollSrv->poll_id, pPollSrv->poll_name, pPollSrv->start_time, pPollSrv->duration_time);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Add a manually triggered polling: poll_id=%d, poll_name=%s, start_time=%d, duration_time=%d", pPollSrv->poll_id, pPollSrv->poll_name, pPollSrv->start_time, pPollSrv->duration_time);

        /* ��Ӷ������ݵ����� */
        i = add_poll_action_dest_data_to_srv_list_proc(pPollSrv->poll_id, pPollSrv->pPollActionList, pPoll_Srv_dboper);

        if (i < 0)
        {
            poll_srv_free(pPollSrv);
            pPollSrv = NULL;
            DEBUG_TRACE(MODULE_POLL_SRV, LOG_ERROR, "start_poll_srv_by_id() add_poll_action_data_to_srv_list_proc:i=%d \r\n", i);
            return -1;
        }

        /* ��ӵ����� */
        if (poll_srv_add(pPollSrv) < 0)
        {
            poll_srv_free(pPollSrv);
            pPollSrv = NULL;
            DEBUG_TRACE(MODULE_POLL_SRV, LOG_ERROR, "start_poll_srv_by_id() Poll Srv Add Error");
            return -1;
        }

        /* ֪ͨ�ͻ��� */
        if (0 == pPollSrv->send_mark)
        {

#if 0 /* �û��ֶ�ִ�е���Ѳ���������û� */

            iRet = SendNotifyExecutePollActionToExceptOnlineUser(pUserInfo, pPollSrv->poll_id, 0);

            if (0 != iRet)
            {
                DEBUG_TRACE(MODULE_POLL_SRV, LOG_ERROR, "start_poll_srv_by_id() SendNotifyExecutePollActionToExceptOnlineUser Start Error: poll_id=%u, iRet=%d", pPollSrv->poll_id, iRet);
            }
            else
            {
                DEBUG_TRACE(MODULE_POLL_SRV, LOG_INFO, "start_poll_srv_by_id() SendNotifyExecutePollActionToExceptOnlineUser Start OK: poll_id=%u, iRet=%d", pPollSrv->poll_id, iRet);
            }

#endif
            pPollSrv->send_mark = 1;

            /* ����״̬ */
            iRet = UpdatePollConfigStatus2DB(pPollSrv->poll_id, 1, pPoll_Srv_dboper);

            if (iRet < 0)
            {
                DEBUG_TRACE(MODULE_POLL_SRV, LOG_ERROR, "start_poll_srv_by_id() UpdatePollConfigStatus2DB Start Error: poll_id=%u, iRet=%d", pPollSrv->poll_id, iRet);
            }
            else
            {
                DEBUG_TRACE(MODULE_POLL_SRV, LOG_TRACE, "start_poll_srv_by_id() UpdatePollConfigStatus2DB Start OK: poll_id=%u, iRet=%d", pPollSrv->poll_id, iRet);
            }
        }
    }
    else
    {
        pPollSrv = poll_srv_get(poll_pos);

        if (NULL != pPollSrv)
        {
            if (0 == pPollSrv->status || 3 == pPollSrv->status)
            {
                pPollSrv->status = 2;
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "����ֶ�����ִ�е���Ѳ: poll_id=%d, poll_name=%s, start_time=%d, duration_time=%d", pPollSrv->poll_id, pPollSrv->poll_name, pPollSrv->start_time, pPollSrv->duration_time);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Add a manually triggered polling: poll_id=%d, poll_name=%s, start_time=%d, duration_time=%d", pPollSrv->poll_id, pPollSrv->poll_name, pPollSrv->start_time, pPollSrv->duration_time);

                /* ֪ͨ�ͻ��� */
                if (0 == pPollSrv->send_mark)
                {

#if 0 /* �û��ֶ�ִ�е���Ѳ���������û� */

                    iRet = SendNotifyExecutePollActionToExceptOnlineUser(pUserInfo, pPollSrv->poll_id, 0);

                    if (0 != iRet)
                    {
                        DEBUG_TRACE(MODULE_POLL_SRV, LOG_ERROR, "start_poll_srv_by_id() SendNotifyExecutePollActionToExceptOnlineUser Start Error: poll_id=%u, iRet=%d", pPollSrv->poll_id, iRet);
                    }
                    else
                    {
                        DEBUG_TRACE(MODULE_POLL_SRV, LOG_INFO, "start_poll_srv_by_id() SendNotifyExecutePollActionToExceptOnlineUser Start OK: poll_id=%u, iRet=%d", pPollSrv->poll_id, iRet);
                    }

#endif
                    pPollSrv->send_mark = 1;

                    /* ����״̬ */
                    iRet = UpdatePollConfigStatus2DB(pPollSrv->poll_id, 1, pPoll_Srv_dboper);

                    if (iRet < 0)
                    {
                        DEBUG_TRACE(MODULE_POLL_SRV, LOG_ERROR, "start_poll_srv_by_id() UpdatePollConfigStatus2DB Start Error: poll_id=%u, iRet=%d", pPollSrv->poll_id, iRet);
                    }
                    else
                    {
                        DEBUG_TRACE(MODULE_POLL_SRV, LOG_TRACE, "start_poll_srv_by_id() UpdatePollConfigStatus2DB Start OK: poll_id=%u, iRet=%d", pPollSrv->poll_id, iRet);
                    }
                }

                DEBUG_TRACE(MODULE_POLL_SRV, LOG_TRACE, "start_poll_srv_by_id() PollID=%u, PollName=%s, StartTime=%d, DurationTime=%d \r\n", pPollSrv->poll_id, pPollSrv->poll_name, pPollSrv->start_time, pPollSrv->duration_time);
            }
            else if (1 == pPollSrv->status) /* �ٴη���һ�� */
            {

#if 0 /* �û��ֶ�ִ�е���Ѳ���������û� */

                iRet = SendNotifyExecutePollActionToExceptOnlineUser(pUserInfo, pPollSrv->poll_id, 0);

                if (0 != iRet)
                {
                    DEBUG_TRACE(MODULE_POLL_SRV, LOG_ERROR, "start_poll_srv_by_id() SendNotifyExecutePollActionToExceptOnlineUser Start Error: poll_id=%u, iRet=%d", pPollSrv->poll_id, iRet);
                }
                else
                {
                    DEBUG_TRACE(MODULE_POLL_SRV, LOG_INFO, "start_poll_srv_by_id() SendNotifyExecutePollActionToExceptOnlineUser Start OK: poll_id=%u, iRet=%d", pPollSrv->poll_id, iRet);
                }

#endif
            }
            else
            {
                DEBUG_TRACE(MODULE_POLL_SRV, LOG_ERROR, "start_poll_srv_by_id() PollID=%u, status=%d \r\n", pPollSrv->poll_id, pPollSrv->status);
            }
        }
        else
        {
            DEBUG_TRACE(MODULE_POLL_SRV, LOG_ERROR, "start_poll_srv_by_id() poll_srv_get Error:poll_pos=%d \r\n", poll_pos);
        }
    }

    return 0;
}

/*****************************************************************************
 �� �� ��  : stop_poll_srv_by_id
 ��������  : ����IDֹͣ��Ѳ����
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
int stop_poll_srv_by_id(unsigned int id)
{
    //int iRet = -1;
    int pos = -1;
    poll_srv_t* pPollSrv = NULL;

    pos = poll_srv_find(id);

    if (pos >= 0)
    {
        pPollSrv = poll_srv_get(pos);

        if (NULL != pPollSrv)
        {
            if (1 == pPollSrv->status || 4 == pPollSrv->status)
            {
                pPollSrv->status = 3;
                pPollSrv->send_mark = 0;
            }
            else if (2 == pPollSrv->status)
            {
                pPollSrv->status = 0;

#if 0 /* ֹͣ��Ѳ����Ҫ֪ͨ�ͻ��� */

                /* ֪ͨ�ͻ��� */
                iRet = SendNotifyExecutePollActionToOnlineUser(pPollSrv->poll_id, 1);

                if (0 != iRet)
                {
                    DEBUG_TRACE(MODULE_POLL_SRV, LOG_ERROR, "stop_poll_srv_by_id() SendNotifyExecutePollActionToOnlineUser Error: poll_id=%u, iRet=%d\r\n", pPollSrv->poll_id, iRet);
                }
                else
                {
                    DEBUG_TRACE(MODULE_POLL_SRV, LOG_INFO, "stop_poll_srv_by_id() SendNotifyExecutePollActionToOnlineUser OK: poll_id=%u, iRet=%d\r\n", pPollSrv->poll_id, iRet);
                }

#endif
                pPollSrv->send_mark = 0;
            }
            else
            {
                DEBUG_TRACE(MODULE_POLL_SRV, LOG_TRACE, "stop_poll_srv_by_id() poll_id=%u, status=%d\r\n", pPollSrv->poll_id, pPollSrv->status);
            }
        }
    }

    return 0;
}

/*****************************************************************************
 �� �� ��  : set_poll_srv_list_del_mark
 ��������  : ������Ѳҵ��ɾ����ʶ
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
int set_poll_srv_list_del_mark(int del_mark)
{
    int pos1 = 0;
    int pos2 = 0;
    int pos3 = 0;
    poll_srv_t* pPollSrv = NULL;
    poll_action_t* pPollAction = NULL;
    poll_action_source_t* pPollActionSource = NULL;

    if ((NULL == g_PollSrvList) || (NULL == g_PollSrvList->pPollSrvList))
    {
        //DEBUG_TRACE(MODULE_POLL_SRV, LOG_DEBUG, "set_poll_srv_list_del_mark() exit---: Param Error \r\n");
        return -1;
    }

    POLL_SMUTEX_LOCK();

    if (osip_list_size(g_PollSrvList->pPollSrvList) <= 0)
    {
        POLL_SMUTEX_UNLOCK();
        return -1;
    }

    for (pos1 = 0; pos1 < osip_list_size(g_PollSrvList->pPollSrvList); pos1++)
    {
        pPollSrv = (poll_srv_t*)osip_list_get(g_PollSrvList->pPollSrvList, pos1);

        if (NULL == pPollSrv)
        {
            continue;
        }

        pPollSrv->del_mark = del_mark;

        for (pos2 = 0; pos2 < osip_list_size(pPollSrv->pPollActionList); pos2++)
        {
            pPollAction = (poll_action_t*)osip_list_get(pPollSrv->pPollActionList, pos2);

            if (NULL == pPollAction)
            {
                continue;
            }

            pPollAction->del_mark = del_mark;

            for (pos3 = 0; pos3 < osip_list_size(pPollAction->pPollActionSourceList); pos3++)
            {
                pPollActionSource = (poll_action_source_t*)osip_list_get(pPollAction->pPollActionSourceList, pos3);

                if (NULL == pPollActionSource)
                {
                    continue;
                }

                pPollActionSource->del_mark = del_mark;
            }
        }
    }

    POLL_SMUTEX_UNLOCK();

    return 0;
}

/*****************************************************************************
 �� �� ��  : check_db_data_to_poll_srv_list
 ��������  : �������м���Ƿ�����Ҫִ�е���Ѳ���ݣ�����У�����ص��ڴ���
 �������  : DBOper* pPoll_Srv_dboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��4��2�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int check_db_data_to_poll_srv_list(DBOper* pPoll_Srv_dboper)
{
    int i = 0;
    int iRet = 0;
    string strSQL = "";
    int record_count = 0;
    time_t now = time(NULL);
    int iTimeNow = 0;
    struct tm tp = {0};
    int while_count = 0;
    poll_srv_t* pPollSrv2 = NULL;

    if (NULL == pPoll_Srv_dboper)
    {
        return -1;
    }

    strSQL.clear();
    strSQL = "select * from PollConfig order by StartTime asc";

    record_count = pPoll_Srv_dboper->DB_Select(strSQL.c_str(), 1);

    if (record_count < 0)
    {
        DEBUG_TRACE(MODULE_POLL_SRV, LOG_ERROR, "check_db_data_to_poll_srv_list() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
        DEBUG_TRACE(MODULE_POLL_SRV, LOG_ERROR, "check_db_data_to_poll_srv_list() ErrorMsg=%s\r\n", pPoll_Srv_dboper->GetLastDbErrorMsg());
        return -1;
    }
    else if (record_count == 0)
    {
        //DEBUG_TRACE(MODULE_POLL_SRV, LOG_DEBUG, "check_db_data_to_poll_srv_list_for_start() exit---: No Record Count \r\n");
        return 0;
    }

    localtime_r(&now, &tp);
    iTimeNow = tp.tm_hour * 3600 + tp.tm_min * 60 + tp.tm_sec;

    DEBUG_TRACE(MODULE_POLL_SRV, LOG_TRACE, "check_db_data_to_poll_srv_list:record_count=%d \r\n", record_count);

    do
    {
        int i = 0;
        unsigned int uPollID = 0;
        string strPollName = "";
        int iOldStartTime = 0;
        int iStartTime = 0;
        int iDurationTime = 0;
        int iScheduledRun = 0;
        int poll_pos = -1;
        int iResved1 = 0;

        while_count++;

        if (while_count % 10000 == 0)
        {
            DEBUG_TRACE(MODULE_POLL_SRV, LOG_WARN, "check_db_data_to_poll_srv_list() While Count=%d \r\n", while_count);
        }

        pPoll_Srv_dboper->GetFieldValue("ID", uPollID);
        pPoll_Srv_dboper->GetFieldValue("PollName", strPollName);
        pPoll_Srv_dboper->GetFieldValue("StartTime", iStartTime);
        pPoll_Srv_dboper->GetFieldValue("DurationTime", iDurationTime);
        pPoll_Srv_dboper->GetFieldValue("ScheduledRun", iScheduledRun);
        pPoll_Srv_dboper->GetFieldValue("Resved1", iResved1);

        /* ���Ҷ��У������������Ƿ��Ѿ����� */
        poll_pos = poll_srv_find(uPollID);

        DEBUG_TRACE(MODULE_POLL_SRV, LOG_TRACE, "check_db_data_to_poll_srv_list() PollID=%u:poll_pos=%d \r\n", uPollID, poll_pos);

        if (poll_pos < 0) /* ��ӵ�Ҫִ�ж��� */
        {
            poll_srv_t* pPollSrv = NULL;

            i = poll_srv_init(&pPollSrv);

            if (i != 0)
            {
                DEBUG_TRACE(MODULE_POLL_SRV, LOG_ERROR, "check_db_data_to_poll_srv_list() poll_srv_init:i=%d \r\n", i);
                continue;
            }

            pPollSrv->poll_id = uPollID;

            if (!strPollName.empty())
            {
                osip_strncpy(pPollSrv->poll_name, (char*)strPollName.c_str(), MAX_128CHAR_STRING_LEN);
            }

            pPollSrv->start_time = iStartTime;
            pPollSrv->duration_time = iDurationTime;
            pPollSrv->del_mark = 0;

            if ((iTimeNow == iStartTime) || (iTimeNow > iStartTime && iTimeNow - iStartTime < 30)) /* 30��֮�ڵĲ����� */
            {
                if (iScheduledRun)
                {
                    pPollSrv->status = 2;
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "��Ӷ�ʱ�ٷ�ִ�е���Ѳ: poll_id=%d, poll_name=%s, start_time=%d, duration_time=%d", pPollSrv->poll_id, pPollSrv->poll_name, pPollSrv->start_time, pPollSrv->duration_time);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Add time triggered polling: poll_id=%d, poll_name=%s, start_time=%d, duration_time=%d", pPollSrv->poll_id, pPollSrv->poll_name, pPollSrv->start_time, pPollSrv->duration_time);

                }
            }
            else if (0 == iStartTime && 0 == iDurationTime && 1 == iResved1) /* �ֶ�ִ�еģ�״̬��1����Ҫ���� */
            {
                if (!iScheduledRun)
                {
                    pPollSrv->status = 2;
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "����ֶ��ٷ�ִ�еĵ���û��ֹͣ����Ѳ: poll_id=%d, poll_name=%s, start_time=%d, duration_time=%d", pPollSrv->poll_id, pPollSrv->poll_name, pPollSrv->start_time, pPollSrv->duration_time);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Add a manually triggered but not stopped polling: poll_id=%d, poll_name=%s, start_time=%d, duration_time=%d", pPollSrv->poll_id, pPollSrv->poll_name, pPollSrv->start_time, pPollSrv->duration_time);
                }
            }

            /* ��ӵ����� */
            if (poll_srv_add(pPollSrv) < 0)
            {
                poll_srv_free(pPollSrv);
                pPollSrv = NULL;
                DEBUG_TRACE(MODULE_POLL_SRV, LOG_ERROR, "check_db_data_to_poll_srv_list() Poll Srv Add Error\r\n");
                continue;
            }
            else
            {
                DEBUG_TRACE(MODULE_POLL_SRV, LOG_INFO, "check_db_data_to_poll_srv_list() poll_srv_add:poll_id=%u, StartTime=%d,DurationTime=%d,Status=%d, \r\n", pPollSrv->poll_id, iStartTime, iDurationTime, pPollSrv->status);
            }
        }
        else
        {
            poll_srv_t* pPollSrv = NULL;

            pPollSrv = poll_srv_get(poll_pos);

            if (NULL != pPollSrv)
            {
                /* �������Ƿ��б仯 */
                if (!strPollName.empty())
                {
                    if (0 != sstrcmp(pPollSrv->poll_name, (char*)strPollName.c_str()))
                    {
                        memset(pPollSrv->poll_name, 0, MAX_128CHAR_STRING_LEN + 4);
                        osip_strncpy(pPollSrv->poll_name, (char*)strPollName.c_str(), MAX_128CHAR_STRING_LEN);
                    }
                }
                else
                {
                    memset(pPollSrv->poll_name, 0, MAX_128CHAR_STRING_LEN + 4);
                }

                iOldStartTime = pPollSrv->start_time;
                pPollSrv->start_time = iStartTime;
                pPollSrv->duration_time = iDurationTime;
                pPollSrv->del_mark = 0;

                if (0 == pPollSrv->status || 3 == pPollSrv->status)
                {
                    if ((iTimeNow == iStartTime) || (iTimeNow > iStartTime && iTimeNow - iStartTime < 30)) /* 30��֮�ڵĲ����� */
                    {
                        if (iScheduledRun)
                        {
                            pPollSrv->status = 2;
                            //DEBUG_TRACE(MODULE_POLL_SRV, LOG_INFO, "check_db_data_to_poll_srv_list() poll_srv_add:poll_id=%u, StartTime=%d,DurationTime=%d,Status=%d, \r\n", pPollSrv->poll_id, iStartTime, iDurationTime, pPollSrv->status);
                            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "��Ӷ�ʱ�ٷ�ִ�е���Ѳ: poll_id=%d, poll_name=%s, start_time=%d, duration_time=%d", pPollSrv->poll_id, pPollSrv->poll_name, pPollSrv->start_time, pPollSrv->duration_time);
                            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Add time triggered polling: poll_id=%d, poll_name=%s, start_time=%d, duration_time=%d", pPollSrv->poll_id, pPollSrv->poll_name, pPollSrv->start_time, pPollSrv->duration_time);

                        }
                    }
                    else if (0 == iStartTime && 0 == iDurationTime && 1 == iResved1) /* �ֶ�ִ�еģ�״̬��1����Ҫ���� */
                    {
                        if (!iScheduledRun && 0 == pPollSrv->status) /* 3��״̬���û��ֶ�ֹͣ�ˣ����ܻ�û���ü���⣬�����ٴ����� */
                        {
                            pPollSrv->status = 2;
                            //DEBUG_TRACE(MODULE_POLL_SRV, LOG_INFO, "check_db_data_to_poll_srv_list() poll_srv_add:poll_id=%u, StartTime=%d,DurationTime=%d,Status=%d, \r\n", pPollSrv->poll_id, iStartTime, iDurationTime, pPollSrv->status);
                            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "����ֶ��ٷ�ִ�еĵ���û��ֹͣ����Ѳ: poll_id=%d, poll_name=%s, start_time=%d, duration_time=%d", pPollSrv->poll_id, pPollSrv->poll_name, pPollSrv->start_time, pPollSrv->duration_time);
                            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Add a manually triggered but not stopped polling: poll_id=%d, poll_name=%s, start_time=%d, duration_time=%d", pPollSrv->poll_id, pPollSrv->poll_name, pPollSrv->start_time, pPollSrv->duration_time);

                        }
                    }
                }
                else if (1 == pPollSrv->status) /* �����޸�������ʱ�䣬�ٷ���һ�� */
                {
                    if ((iStartTime != iOldStartTime) && ((iTimeNow == iStartTime) || (iTimeNow > iStartTime && iTimeNow - iStartTime < 30))) /* 30��֮�ڵĲ����� */
                    {
                        if (iScheduledRun)
                        {
                            pPollSrv->status = 4;
                            DEBUG_TRACE(MODULE_POLL_SRV, LOG_INFO, "check_db_data_to_poll_srv_list() poll_srv_add:poll_id=%u, StartTime=%d,DurationTime=%d,Status=%d, \r\n", pPollSrv->poll_id, iStartTime, iDurationTime, pPollSrv->status);
                        }
                    }
                }
            }
        }
    }
    while (pPoll_Srv_dboper->MoveNext() >= 0);

    /* ���Ŀ�����ݺ�Դ���� */
    for (i = 0; i < osip_list_size(g_PollSrvList->pPollSrvList); i++)
    {
        pPollSrv2 = (poll_srv_t*)osip_list_get(g_PollSrvList->pPollSrvList, i);

        if (NULL == pPollSrv2)
        {
            continue;
        }

        /* ��Ӷ������ݵ����� */
        iRet = add_poll_action_dest_data_to_srv_list_proc(pPollSrv2->poll_id, pPollSrv2->pPollActionList, pPoll_Srv_dboper);

        if (iRet < 0)
        {
            DEBUG_TRACE(MODULE_POLL_SRV, LOG_ERROR, "check_db_data_to_poll_srv_list() add_poll_action_data_to_srv_list_proc Error:poll_id=%u \r\n", pPollSrv2->poll_id);
        }
        else
        {
            DEBUG_TRACE(MODULE_POLL_SRV, LOG_TRACE, "check_db_data_to_poll_srv_list() add_poll_action_data_to_srv_list_proc:poll_id=%u, iRet=%d \r\n", pPollSrv2->poll_id, iRet);
        }
    }

    return iRet;
}

/*****************************************************************************
 �� �� ��  : add_poll_action_dest_data_to_srv_list_proc
 ��������  : �����Ѳ������Ŀ�����ݵ���Ѳҵ�����
 �������  : int poll_id
                            osip_list_t* pPollActionList
                            DBOper* pPoll_Srv_dboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��4��2�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int add_poll_action_dest_data_to_srv_list_proc(unsigned int poll_id, osip_list_t* pPollActionList, DBOper* pPoll_Srv_dboper)
{
    int i = 0;
    int index = 0;
    int pos = -1;
    int iRet = 0;
    int record_count = 0;
    char strPollId[32] = {0};

    string strSQL = "";
    int while_count = 0;

    vector<string> strDestDeviceIDVector;

    if (NULL == pPoll_Srv_dboper)
    {
        DEBUG_TRACE(MODULE_POLL_SRV, LOG_DEBUG, "add_poll_action_dest_data_to_srv_list_proc() exit---: Poll Srv DB Oper Error \r\n");
        return -1;
    }

    if (NULL == pPollActionList)
    {
        DEBUG_TRACE(MODULE_POLL_SRV, LOG_DEBUG, "add_poll_action_dest_data_to_srv_list_proc() exit---: Poll Action List Error \r\n");
        return -1;
    }

    /* ����poll_id����ѯ��Ѳ��������ȡ��Ѳ�ľ������� */
    strSQL.clear();
    snprintf(strPollId, 32, "%u", poll_id);
    strSQL = "select DISTINCT DestID from PollActionConfig WHERE PollID = ";
    strSQL += strPollId;
    strSQL += " order by DestSortID asc";

    record_count = pPoll_Srv_dboper->DB_Select(strSQL.c_str(), 1);

    DEBUG_TRACE(MODULE_POLL_SRV, LOG_TRACE, "add_poll_action_dest_data_to_srv_list_proc:PollID=%u, record_count=%d \r\n", poll_id, record_count);

    if (record_count < 0)
    {
        DEBUG_TRACE(MODULE_POLL_SRV, LOG_ERROR, "add_poll_action_dest_data_to_srv_list_proc() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
        DEBUG_TRACE(MODULE_POLL_SRV, LOG_ERROR, "add_poll_action_dest_data_to_srv_list_proc() ErrorMsg=%s\r\n", pPoll_Srv_dboper->GetLastDbErrorMsg());
        return -1;
    }
    else if (record_count == 0)
    {
        //DEBUG_TRACE(MODULE_POLL_SRV, LOG_WARN, "add_poll_action_dest_data_to_srv_list_proc() exit---: No Record Count \r\n");
        return 0;
    }

    strDestDeviceIDVector.clear();

    /* ѭ�������Ѳ�������� */
    do
    {
        string strDestID = "";

        while_count++;

        if (while_count % 10000 == 0)
        {
            DEBUG_TRACE(MODULE_POLL_SRV, LOG_WARN, "add_poll_action_dest_data_to_srv_list_proc() While Count=%d \r\n", while_count);
        }

        pPoll_Srv_dboper->GetFieldValue("DestID", strDestID);

        if (strDestID.empty())
        {
            DEBUG_TRACE(MODULE_POLL_SRV, LOG_WARN, "add_poll_action_dest_data_to_srv_list_proc() DestID Empty \r\n");
            continue;
        }

        strDestDeviceIDVector.push_back(strDestID);
    }
    while (pPoll_Srv_dboper->MoveNext() >= 0);

    DEBUG_TRACE(MODULE_POLL_SRV, LOG_TRACE, "add_poll_action_dest_data_to_srv_list_proc:strDestDeviceIDVector.size()=%d \r\n", strDestDeviceIDVector.size());

    /* ѭ������Ϣд���ڴ��� */
    if (strDestDeviceIDVector.size() > 0)
    {
        for (index = 0; index < (int)strDestDeviceIDVector.size(); index++)
        {
            /* ����DestID���Ҷ����� */
            pos = poll_action_find((char*)strDestDeviceIDVector[index].c_str(), pPollActionList);

            DEBUG_TRACE(MODULE_POLL_SRV, LOG_TRACE, "add_poll_action_dest_data_to_srv_list_proc() DestID=%s:pos=%d \r\n", (char*)strDestDeviceIDVector[index].c_str(), pos);

            if (pos < 0)
            {
                poll_action_t* pPollAction = NULL;

                /* �����Ѳ���� */
                iRet = poll_action_init(&pPollAction);

                if (0 != iRet)
                {
                    DEBUG_TRACE(MODULE_POLL_SRV, LOG_ERROR, "add_poll_action_dest_data_to_srv_list_proc() Poll Action Init Error \r\n");
                    continue;
                }

                pPollAction->poll_id = poll_id;
                osip_strncpy(pPollAction->pcDestID, (char*)strDestDeviceIDVector[index].c_str(), MAX_ID_LEN);
                pPollAction->del_mark = 0;

                i = add_poll_action_source_data_to_srv_list_proc(pPollAction->poll_id, pPollAction->pcDestID, pPollAction->pPollActionSourceList, pPoll_Srv_dboper);

                if (i < 0)
                {
                    poll_action_free(pPollAction);
                    pPollAction = NULL;
                    DEBUG_TRACE(MODULE_POLL_SRV, LOG_ERROR, "add_poll_action_dest_data_to_srv_list_proc() add_poll_action_source_data_to_srv_list_proc Error \r\n");
                    continue;
                }

                /* ��ӵ����� */
                i = osip_list_add(pPollActionList, pPollAction, -1); /* add to list tail */

                DEBUG_TRACE(MODULE_POLL_SRV, LOG_TRACE, "add_poll_action_dest_data_to_srv_list_proc() PollAction:PollID=%u, DestID=%s, i=%d \r\n", poll_id, pPollAction->pcDestID, i);

                if (i < 0)
                {
                    poll_action_free(pPollAction);
                    pPollAction = NULL;
                    DEBUG_TRACE(MODULE_POLL_SRV, LOG_ERROR, "add_poll_action_dest_data_to_srv_list_proc() Poll Action Add Error \r\n");
                    continue;
                }
            }
            else
            {
                poll_action_t* pPollAction = NULL;

                pPollAction = (poll_action_t*)osip_list_get(pPollActionList, pos);

                if (NULL != pPollAction)
                {
                    pPollAction->del_mark = 0;

                    i = add_poll_action_source_data_to_srv_list_proc(pPollAction->poll_id, pPollAction->pcDestID, pPollAction->pPollActionSourceList, pPoll_Srv_dboper);

                    if (i < 0)
                    {
                        poll_action_free(pPollAction);
                        pPollAction = NULL;
                        DEBUG_TRACE(MODULE_POLL_SRV, LOG_ERROR, "add_poll_action_dest_data_to_srv_list_proc() add_poll_action_source_data_to_srv_list_proc Error \r\n");
                        continue;
                    }
                }
            }
        }
    }

    strDestDeviceIDVector.clear();

    return osip_list_size(pPollActionList);
}

/*****************************************************************************
 �� �� ��  : add_poll_action_source_data_to_srv_list_proc
 ��������  : �����Ѳ������Դ���ݵ���Ѳҵ�����
 �������  : int poll_id
                            char* pcDestID
                            osip_list_t* pPollActionSourceList
                            DBOper* pPoll_Srv_dboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��4��2�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int add_poll_action_source_data_to_srv_list_proc(unsigned int poll_id, char* pcDestID, osip_list_t* pPollActionSourceList, DBOper* pPoll_Srv_dboper)
{
    int i = 0;
    int record_count = 0;
    char strPollId[32] = {0};

    string strSQL = "";
    int while_count = 0;

    if (NULL == pcDestID || NULL == pPoll_Srv_dboper)
    {
        DEBUG_TRACE(MODULE_POLL_SRV, LOG_DEBUG, "add_poll_action_source_data_to_srv_list_proc() exit---: Poll Srv DB Oper Error \r\n");
        return -1;
    }

    if (NULL == pPollActionSourceList)
    {
        DEBUG_TRACE(MODULE_POLL_SRV, LOG_DEBUG, "add_poll_action_source_data_to_srv_list_proc() exit---: Poll Action List Error \r\n");
        return -1;
    }

    /* ����poll_id����ѯ��Ѳ��������ȡ��Ѳ�ľ������� */
    strSQL.clear();
    snprintf(strPollId, 32, "%u", poll_id);
    strSQL = "select * from PollActionConfig WHERE PollID = ";
    strSQL += strPollId;
    strSQL += " and DestID='";
    strSQL += pcDestID;
    strSQL += "'";
    strSQL += " order by SourceSortID asc";

    record_count = pPoll_Srv_dboper->DB_Select(strSQL.c_str(), 1);

    if (record_count < 0)
    {
        DEBUG_TRACE(MODULE_POLL_SRV, LOG_ERROR, "add_poll_action_source_data_to_srv_list_proc() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
        DEBUG_TRACE(MODULE_POLL_SRV, LOG_ERROR, "add_poll_action_source_data_to_srv_list_proc() ErrorMsg=%s\r\n", pPoll_Srv_dboper->GetLastDbErrorMsg());
        return -1;
    }
    else if (record_count == 0)
    {
        DEBUG_TRACE(MODULE_POLL_SRV, LOG_WARN, "add_poll_action_source_data_to_srv_list_proc() exit---: No Record Count \r\n");
        return 0;
    }

    /* ѭ�������Ѳ�������� */
    do
    {
        int pos = -1;
        int iRet = 0;
        int iType = -1;
        string strSourceID = "";
        int iSourceStreamType = 0;
        string strDestID = "";
        int iScreenID = -1;
        int iLiveTime = -1;

        while_count++;

        if (while_count % 10000 == 0)
        {
            DEBUG_TRACE(MODULE_POLL_SRV, LOG_WARN, "add_poll_action_source_data_to_srv_list_proc() While Count=%d \r\n", while_count);
        }

        pPoll_Srv_dboper->GetFieldValue("Type", iType);
        pPoll_Srv_dboper->GetFieldValue("SourceID", strSourceID);
        pPoll_Srv_dboper->GetFieldValue("StreamType", iSourceStreamType);
        pPoll_Srv_dboper->GetFieldValue("DestID", strDestID);
        pPoll_Srv_dboper->GetFieldValue("ScreenID", iScreenID);
        pPoll_Srv_dboper->GetFieldValue("LiveTime", iLiveTime);

        if (strSourceID.empty())
        {
            DEBUG_TRACE(MODULE_POLL_SRV, LOG_WARN, "add_poll_action_source_data_to_srv_list_proc() SourceID Empty \r\n");
            continue;
        }

        /* ����DestID���Ҷ����� */
        pos = poll_action_source_find((char*)strSourceID.c_str(), pPollActionSourceList);

        DEBUG_TRACE(MODULE_POLL_SRV, LOG_TRACE, "add_poll_action_source_data_to_srv_list_proc() Type=%d, SourceID=%s, ScreenID=%d, LiveTime=%d, pos=%d \r\n", iType, strSourceID.c_str(), iScreenID, iLiveTime, pos);

        if (pos < 0)
        {
            poll_action_source_t* pPollActionSource = NULL;

            /* ���Դ��Ϣ */
            iRet = poll_action_source_init(&pPollActionSource);

            if (0 != iRet)
            {
                DEBUG_TRACE(MODULE_POLL_SRV, LOG_ERROR, "add_poll_action_source_data_to_srv_list_proc() Poll Action Source Init Error \r\n");
                continue;
            }

            pPollActionSource->iStatus = 0;
            pPollActionSource->iType = iType;

            if (!strSourceID.empty())
            {
                osip_strncpy(pPollActionSource->pcSourceID, (char*)strSourceID.c_str(), MAX_ID_LEN);
            }

            pPollActionSource->iSourceStreamType = iSourceStreamType;
            pPollActionSource->iLiveTime = iLiveTime;
            pPollActionSource->iLiveTimeCount = 0;
            pPollActionSource->del_mark = 0;

            i = osip_list_add(pPollActionSourceList, pPollActionSource, -1); /* add to list tail */

            DEBUG_TRACE(MODULE_POLL_SRV, LOG_TRACE, "add_poll_action_source_data_to_srv_list_proc() PollActionSource:Type=%d, SourceID=%s, LiveTime=%d, i=%d \r\n", pPollActionSource->iType, pPollActionSource->pcSourceID, pPollActionSource->iLiveTime, i);

            if (i < 0)
            {
                poll_action_source_free(pPollActionSource);
                pPollActionSource = NULL;
                DEBUG_TRACE(MODULE_POLL_SRV, LOG_ERROR, "add_poll_action_source_data_to_srv_list_proc() Poll Action Source Add Error \r\n");
                continue;
            }
        }
        else
        {
            poll_action_source_t* pPollActionSource = NULL;

            pPollActionSource = (poll_action_source_t*)osip_list_get(pPollActionSourceList, pos);

            if (NULL != pPollActionSource)
            {
                if (0 == pPollActionSource->iStatus)
                {
                    pPollActionSource->iType = iType;

                    if (!strSourceID.empty())
                    {
                        if (0 != sstrcmp(pPollActionSource->pcSourceID, (char*)strSourceID.c_str()))
                        {
                            memset(pPollActionSource->pcSourceID, 0, MAX_ID_LEN + 4);
                            osip_strncpy(pPollActionSource->pcSourceID, (char*)strSourceID.c_str(), MAX_ID_LEN);
                        }
                    }
                    else
                    {
                        memset(pPollActionSource->pcSourceID, 0, MAX_ID_LEN + 4);
                    }

                    pPollActionSource->iSourceStreamType = iSourceStreamType;
                    pPollActionSource->iLiveTime = iLiveTime;
                    pPollActionSource->iLiveTimeCount = 0;
                }

                pPollActionSource->del_mark = 0;
            }
        }
    }
    while (pPoll_Srv_dboper->MoveNext() >= 0);

    return osip_list_size(pPollActionSourceList);
}

/*****************************************************************************
 �� �� ��  : delete_poll_srv_data
 ��������  : ɾ�����н�������Ѳ����
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
void delete_poll_srv_data()
{
    int pos1 = 0;
    int pos2 = 0;
    int pos3 = 0;
    poll_srv_t* pPollSrv = NULL;
    poll_action_t* pPollAction = NULL;
    poll_action_source_t* pPollActionSource = NULL;

    if ((NULL == g_PollSrvList) || (NULL == g_PollSrvList->pPollSrvList))
    {
        //DEBUG_TRACE(MODULE_POLL_SRV, LOG_DEBUG, "delete_poll_srv_data() exit---: Param Error \r\n");
        return;
    }

    POLL_SMUTEX_LOCK();

    if (osip_list_size(g_PollSrvList->pPollSrvList) <= 0)
    {
        POLL_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_POLL_SRV, LOG_WARN, "delete_poll_srv_data() exit---: Poll Srv List NULL \r\n");
        return;
    }

    pos1 = 0;

    while (!osip_list_eol(g_PollSrvList->pPollSrvList, pos1))
    {
        pPollSrv = (poll_srv_t*)osip_list_get(g_PollSrvList->pPollSrvList, pos1);

        if (NULL == pPollSrv)
        {
            osip_list_remove(g_PollSrvList->pPollSrvList, pos1);
            continue;
        }

        if (1 == pPollSrv->del_mark) /* ɾ����Ѳ */
        {
            osip_list_remove(g_PollSrvList->pPollSrvList, pos1);
            poll_srv_free(pPollSrv);
            pPollSrv = NULL;
        }
        else
        {
            if (NULL == pPollSrv->pPollActionList)
            {
                pos1++;
                continue;
            }

            if (osip_list_size(pPollSrv->pPollActionList) <= 0)
            {
                pos1++;
                continue;
            }

            pos2 = 0;

            while (!osip_list_eol(pPollSrv->pPollActionList, pos2))
            {
                pPollAction = (poll_action_t*)osip_list_get(pPollSrv->pPollActionList, pos2);

                if (NULL == pPollAction)
                {
                    osip_list_remove(pPollSrv->pPollActionList, pos2);
                    continue;
                }

                if (1 == pPollAction->del_mark) /* ɾ����Ѳ */
                {
                    osip_list_remove(pPollSrv->pPollActionList, pos2);
                    poll_action_free(pPollAction);
                    pPollAction = NULL;
                }
                else
                {
                    if (NULL == pPollAction->pPollActionSourceList)
                    {
                        pos2++;
                        continue;
                    }

                    if (osip_list_size(pPollAction->pPollActionSourceList) <= 0)
                    {
                        pos2++;
                        continue;
                    }

                    pos3 = 0;

                    while (!osip_list_eol(pPollAction->pPollActionSourceList, pos3))
                    {
                        pPollActionSource = (poll_action_source_t*)osip_list_get(pPollAction->pPollActionSourceList, pos3);

                        if (NULL == pPollActionSource)
                        {
                            osip_list_remove(pPollAction->pPollActionSourceList, pos3);
                            continue;
                        }

                        if (1 == pPollActionSource->del_mark) /* ɾ����Ѳ */
                        {
                            if (pPollAction->current_pos > pos3) /* �����Ѳ�ĵ�����Ҫɾ����֮��λ�ã���λ����Ҫ����һ */
                            {
                                pPollAction->current_pos--;
                            }

                            osip_list_remove(pPollAction->pPollActionSourceList, pos3);
                            poll_action_source_free(pPollActionSource);
                            pPollActionSource = NULL;
                        }
                        else
                        {
                            pos3++;
                        }
                    }

                    pos2++;
                }
            }

            pos1++;
        }
    }

    POLL_SMUTEX_UNLOCK();

    return;
}

/*****************************************************************************
 �� �� ��  : SendNotifyExecutePollActionToOnlineUser
 ��������  : ������Ѳִ��֪ͨ�����߿ͻ���
 �������  : int iPollID
             char* poll_name
             int iType
             DBOper* pPoll_Srv_dboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��4��2�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int SendNotifyExecutePollActionToOnlineUser(unsigned int uPollID, char* poll_name, int iType, DBOper* pPoll_Srv_dboper)
{
    int i = 0;
    int iRet = 0;
    int index = 0;
    CPacket outPacket;
    DOMElement* AccNode = NULL;
    char strPollID[32] = {0};
    vector<unsigned int> UserIndexVector;
    int iUserIndexCount = 0;
    unsigned int uUserIndex = 0;

    /*
     <?xml version="1.0"?>
         <Notify>
         <CmdType>ExecutePoll</CmdType>
         <SN>1234</SN>
         <PollID>��ѯID</PollID>
         </Notify>
     */

    /* �齨XML��Ϣ */
    outPacket.SetRootTag("Notify");
    AccNode = outPacket.CreateElement((char*)"CmdType");
    outPacket.SetElementValue(AccNode, (char*)"ExecutePoll");

    AccNode = outPacket.CreateElement((char*)"SN");
    outPacket.SetElementValue(AccNode, (char*)"1234");

    AccNode = outPacket.CreateElement((char*)"PollID");
    snprintf(strPollID, 32, "%u", uPollID);
    outPacket.SetElementValue(AccNode, strPollID);

    AccNode = outPacket.CreateElement((char*)"PollName");

    if (NULL == poll_name)
    {
        outPacket.SetElementValue(AccNode, (char*)"");
    }
    else
    {
        outPacket.SetElementValue(AccNode, poll_name);
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

    /* ��ȡpoll�û�Ȩ�ޱ� */
    UserIndexVector.clear();
    iRet = get_user_index_from_user_poll_config(strPollID, UserIndexVector, pPoll_Srv_dboper);

    iUserIndexCount = UserIndexVector.size();

    if (iUserIndexCount <= 0)
    {
        DEBUG_TRACE(MODULE_POLL_SRV, LOG_WARN, "SendNotifyExecutePollActionToOnlineUser() exit---: Get User Index NULL \r\n");
        return 0;
    }

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "������Ѳִ��֪ͨ�����߿ͻ���: ��ѯ�����û���������=%d", iUserIndexCount);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "SendNotifyExecutePollActionToOnlineUser: Query to the total number of users index=%d", iUserIndexCount);

    /* ѭ���������� */
    for (index = 0; index < iUserIndexCount; index++)
    {
        /* ��ȡ�û����� */
        uUserIndex = UserIndexVector[index];

        DEBUG_TRACE(MODULE_POLL_SRV, LOG_TRACE, "SendNotifyExecutePollActionToOnlineUser() index=%d, UserIndex=%u \r\n", index, uUserIndex);

        i |= SendMessageToOnlineUserByUserIndex(uUserIndex, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());
    }

    return i;
}

/*****************************************************************************
 �� �� ��  : UpdatePollConfigStatus2DB
 ��������  : ������Ѳ����״̬�����ݿ�
 �������  : int poll_id
             int status
             DBOper* pPoll_Srv_dboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��10��31�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int UpdatePollConfigStatus2DB(int poll_id, int status, DBOper* pPoll_Srv_dboper)
{
    int iRet = 0;
    string strSQL = "";
    char strPollID[64] = {0};
    char strStatus[16] = {0};

    //printf("\r\n UpdateUserRegInfo2DB() Enter--- \r\n");

    if (poll_id <= 0)
    {
        DEBUG_TRACE(MODULE_POLL_SRV, LOG_DEBUG, "UpdatePollConfigStatus2DB() exit---: Param Error \r\n");
        return -1;
    }

    if (NULL == pPoll_Srv_dboper)
    {
        DEBUG_TRACE(MODULE_POLL_SRV, LOG_DEBUG, "UpdatePollConfigStatus2DB() exit---: Poll Srv DB Oper Error \r\n");
        return -1;
    }

    snprintf(strPollID, 64, "%d", poll_id);
    snprintf(strStatus, 16, "%d", status);

    /* �������ݿ� */
    strSQL.clear();
    strSQL = "UPDATE PollConfig SET Resved1 = ";
    strSQL += strStatus;
    strSQL += " WHERE ID = ";
    strSQL += strPollID;

    iRet = pPoll_Srv_dboper->DB_Update(strSQL.c_str(), 1);

    if (iRet < 0)
    {
        DEBUG_TRACE(MODULE_POLL_SRV, LOG_ERROR, "UpdatePollConfigStatus2DB() DB Oper Error: strSQL=%s, iRet=%d \r\n", strSQL.c_str(), iRet);
        DEBUG_TRACE(MODULE_POLL_SRV, LOG_ERROR, "UpdatePollConfigStatus2DB() ErrorMsg=%s\r\n", pPoll_Srv_dboper->GetLastDbErrorMsg());
    }

    return iRet;
}

/*****************************************************************************
 �� �� ��  : get_user_index_from_user_poll_config
 ��������  : ���û���ѲȨ�ޱ������ȡ�û�����
 �������  : char* pcPollID
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
int get_user_index_from_user_poll_config(char* pcPollID, vector<unsigned int>& UserIndexVector, DBOper* pDBOper)
{
    int iRet = 0;
    int record_count = 0;
    int while_count = 0;
    string strSQL = "";

    if (NULL == pcPollID || NULL == pDBOper)
    {
        DEBUG_TRACE(MODULE_POLL_SRV, LOG_DEBUG, "get_user_index_from_user_poll_config() exit---: Param Error \r\n");
        return -1;
    }

    strSQL.clear();
    strSQL = "SELECT UserID FROM UserPollConfig WHERE PollID = ";
    strSQL += pcPollID;

    record_count = pDBOper->DB_Select(strSQL.c_str(), 1);

    if (record_count < 0)
    {
        DEBUG_TRACE(MODULE_POLL_SRV, LOG_ERROR, "get_user_index_from_user_poll_config() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
        DEBUG_TRACE(MODULE_POLL_SRV, LOG_ERROR, "get_user_index_from_user_poll_config() ErrorMsg=%s\r\n", pDBOper->GetLastDbErrorMsg());
        return -1;
    }
    else if (record_count > 0)
    {
        /* ѭ���������ݿ�*/
        do
        {
            while_count++;

            if (while_count % 10000 == 0)
            {
                DEBUG_TRACE(MODULE_POLL_SRV, LOG_WARN, "get_user_index_from_user_poll_config() While Count=%d \r\n", while_count);
            }

            unsigned int uUserIndex = 0;

            pDBOper->GetFieldValue("UserID", uUserIndex);

            iRet = AddUserIndexToUserIndexVector(UserIndexVector, uUserIndex);
        }
        while (pDBOper->MoveNext() >= 0);
    }

    /* ��ȡһ��admin�û���Ȩ�� */
    strSQL.clear();
    strSQL = "SELECT ID FROM UserConfig WHERE UserName like 'admin'";

    record_count = pDBOper->DB_Select(strSQL.c_str(), 1);

    if (record_count < 0)
    {
        DEBUG_TRACE(MODULE_POLL_SRV, LOG_ERROR, "get_user_index_from_user_poll_config() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
        DEBUG_TRACE(MODULE_POLL_SRV, LOG_ERROR, "get_user_index_from_user_poll_config() ErrorMsg=%s\r\n", pDBOper->GetLastDbErrorMsg());
        return -1;
    }
    else if (record_count > 0)
    {
        /* ѭ���������ݿ�*/
        do
        {
            while_count++;

            if (while_count % 10000 == 0)
            {
                DEBUG_TRACE(MODULE_POLL_SRV, LOG_WARN, "get_user_index_from_user_poll_config() While Count=%d \r\n", while_count);
            }

            unsigned int uUserIndex = 0;

            pDBOper->GetFieldValue("ID", uUserIndex);

            iRet = AddUserIndexToUserIndexVector(UserIndexVector, uUserIndex);
        }
        while (pDBOper->MoveNext() >= 0);
    }

    /* ��ȡһ��WiscomV�û���Ȩ�� */
    strSQL.clear();
    strSQL = "SELECT ID FROM UserConfig WHERE UserName like 'WiscomV'";

    record_count = pDBOper->DB_Select(strSQL.c_str(), 1);

    if (record_count < 0)
    {
        DEBUG_TRACE(MODULE_POLL_SRV, LOG_ERROR, "get_user_index_from_user_poll_config() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
        DEBUG_TRACE(MODULE_POLL_SRV, LOG_ERROR, "get_user_index_from_user_poll_config() ErrorMsg=%s\r\n", pDBOper->GetLastDbErrorMsg());
        return -1;
    }
    else if (record_count > 0)
    {
        /* ѭ���������ݿ�*/
        do
        {
            while_count++;

            if (while_count % 10000 == 0)
            {
                DEBUG_TRACE(MODULE_POLL_SRV, LOG_WARN, "get_user_index_from_user_poll_config() While Count=%d \r\n", while_count);
            }

            unsigned int uUserIndex = 0;

            pDBOper->GetFieldValue("ID", uUserIndex);

            iRet = AddUserIndexToUserIndexVector(UserIndexVector, uUserIndex);
        }
        while (pDBOper->MoveNext() >= 0);
    }
    return 0;
}

/*****************************************************************************
 �� �� ��  : ShowPollTaskInfo
 ��������  : ��ʾ��ǰ��Ѳ������Ϣ
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
void ShowPollTaskInfo(int sock, int status)
{
    int i = 0, j = 0, k = 0;
    char strLine[] = "\r-----------------------------------------------------------------------------------------------------------------------\r\n";
    char strHead[] = "\rPollID  DurationTime DurationTimeCount DestID               CurrentPos Type SourceID             LiveTime LiveTimeCount\r\n";
    poll_srv_t* pPollSrv = NULL;
    poll_action_t* pPollAction = NULL;
    poll_action_source_t* pPollActionSource = NULL;
    char rbuf[128] = {0};

    if (sock > 0)
    {
        send(sock, strLine, strlen(strLine), 0);
        send(sock, strHead, strlen(strHead), 0);
    }

    if ((NULL == g_PollSrvList) || (NULL == g_PollSrvList->pPollSrvList))
    {
        return;
    }

    POLL_SMUTEX_LOCK();

    if (osip_list_size(g_PollSrvList->pPollSrvList) <= 0)
    {
        POLL_SMUTEX_UNLOCK();
        return;
    }

    for (i = 0; i < osip_list_size(g_PollSrvList->pPollSrvList); i++)
    {
        pPollSrv = (poll_srv_t*)osip_list_get(g_PollSrvList->pPollSrvList, i);

        if (NULL == pPollSrv || NULL == pPollSrv->pPollActionList)
        {
            continue;
        }

        if (status <= 1)
        {
            if (pPollSrv->status != status) /* û����Ѳ�ĺ��� */
            {
                continue;
            }
        }

        /* ���Ҿ�����Ѳ���� */
        for (j = 0; j < osip_list_size(pPollSrv->pPollActionList); j++)
        {
            pPollAction = (poll_action_t*)osip_list_get(pPollSrv->pPollActionList, j);

            if (NULL == pPollAction || NULL == pPollAction->pPollActionSourceList)
            {
                continue;
            }

            for (k = 0; k < osip_list_size(pPollAction->pPollActionSourceList); k++)
            {
                pPollActionSource = (poll_action_source_t*)osip_list_get(pPollAction->pPollActionSourceList, k);

                if (NULL == pPollActionSource)
                {
                    continue;
                }

                if (status <= 1)
                {
                    if (1 == status && k != pPollAction->current_pos)  /* ���ǵ�ǰ���ڵ�ǰ��Ѳ�ĺ��� */
                    {
                        continue;
                    }

                    if (status != pPollActionSource->iStatus) /* û����������Ѳ���� */
                    {
                        continue;
                    }
                }

                snprintf(rbuf, 128, "\r%-7u %-12d %-17d %-20s %-10d %-4d %-20s %-8d %-13d\r\n", pPollSrv->poll_id, pPollSrv->duration_time, pPollSrv->duration_time_count, pPollAction->pcDestID, pPollAction->current_pos, pPollActionSource->iType, pPollActionSource->pcSourceID, pPollActionSource->iLiveTime, pPollActionSource->iLiveTimeCount);

                if (sock > 0)
                {
                    send(sock, rbuf, strlen(rbuf), 0);
                }
            }
        }
    }

    POLL_SMUTEX_UNLOCK();

    if (sock > 0)
    {
        send(sock, strLine, strlen(strLine), 0);
    }

    return;
}

/*****************************************************************************
 �� �� ��  : StopPollTask
 ��������  : ֹͣ��Ѳ����
 �������  : int sock
             unsigned int poll_id
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��8��7�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int StopPollTask(int sock, unsigned int poll_id)
{
    int iRet = 0;
    char rbuf[128] = {0};

    /* ֹͣҵ�� */
    iRet = stop_poll_srv_by_id(poll_id);

    if (sock > 0)
    {
        memset(rbuf, 0, 128);

        if (0 == iRet)
        {
            snprintf(rbuf, 128, "\rֹͣ��Ѳ����ɹ�: ��ѲID=%u\r\n$", poll_id);
            send(sock, rbuf, strlen(rbuf), 0);
        }
        else
        {
            snprintf(rbuf, 128, "\rֹͣ��Ѳ����ʧ��: ��ѲID=%u\r\n$", poll_id);
            send(sock, rbuf, strlen(rbuf), 0);
        }
    }

    return 0;
}

/*****************************************************************************
 �� �� ��  : StopAllPollTask
 ��������  : ֹͣ������Ѳ����
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
int StopAllPollTask(int sock)
{
    int i = 0;
    int iRet = 0;
    poll_srv_t* pPollSrv = NULL;
    needtoproc_pollsrv_queue needToStop;
    char rbuf[128] = {0};

    if ((NULL == g_PollSrvList) || (NULL == g_PollSrvList->pPollSrvList))
    {
        return -1;
    }

    needToStop.clear();

    POLL_SMUTEX_LOCK();

    if (osip_list_size(g_PollSrvList->pPollSrvList) <= 0)
    {
        POLL_SMUTEX_UNLOCK();
        return -1;
    }

    for (i = 0; i < osip_list_size(g_PollSrvList->pPollSrvList); i++)
    {
        pPollSrv = (poll_srv_t*)osip_list_get(g_PollSrvList->pPollSrvList, i);

        if (NULL == pPollSrv)
        {
            continue;
        }

        needToStop.push_back(pPollSrv);
        pPollSrv->duration_time_count = 0;
    }

    POLL_SMUTEX_UNLOCK();

    while (!needToStop.empty())
    {
        pPollSrv = (poll_srv_t*) needToStop.front();
        needToStop.pop_front();

        if (NULL != pPollSrv)
        {
            if (1 == pPollSrv->status || 4 == pPollSrv->status)
            {
                pPollSrv->status = 3;
            }
            else if (2 == pPollSrv->status)
            {
                pPollSrv->status = 0;

                /* ֪ͨ�ͻ��� */ /* ֹͣ��Ѳ������Ҫ���͸��ͻ��� */
                //iRet = SendNotifyExecutePollActionToOnlineUser(pPollSrv->poll_id, 1);

                pPollSrv->send_mark = 1;
            }

            if (sock > 0)
            {
                memset(rbuf, 0, 128);

                if (iRet >= 0)
                {
                    snprintf(rbuf, 128, "\rֹͣ��Ѳ����ɹ�: ��ѲID=%d\r\n", pPollSrv->poll_id);
                    send(sock, rbuf, strlen(rbuf), 0);
                }
                else
                {
                    snprintf(rbuf, 128, "\rֹͣ��Ѳ����ʧ��: ��ѲID=%d\r\n", pPollSrv->poll_id);
                    send(sock, rbuf, strlen(rbuf), 0);
                }
            }
        }
    }

    needToStop.clear();

    return 0;
}

/*****************************************************************************
 �� �� ��  : PollSrvConfig_db_refresh_proc
 ��������  : ������Ѳҵ��������Ϣ���ݿ���²�����ʶ
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
int PollSrvConfig_db_refresh_proc()
{
    if (1 == db_PollSrvInfo_reload_mark) /* ����ִ�� */
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "��Ѳҵ���������ݿ���Ϣ����ͬ��");
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Poll Srv Info database information are synchronized");
        return 0;
    }

    db_PollSrvInfo_reload_mark = 1;
    return 0;
}

/*****************************************************************************
 �� �� ��  : check_PollSrvConfig_need_to_reload_begin
 ��������  : ����Ƿ���Ҫͬ����Ѳҵ�����ÿ�ʼ
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
void check_PollSrvConfig_need_to_reload_begin(DBOper* pDboper)
{
    /* ����Ƿ���Ҫ�������ݿ��ʶ */
    if (!db_PollSrvInfo_reload_mark)
    {
        return;
    }

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "ͬ����Ѳҵ���������ݿ���Ϣ: ��ʼ---");
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Synchronization Poll Srv info database information: begain---");

    /* ������Ѳ���е�ɾ����ʶ */
    set_poll_srv_list_del_mark(1);

    /* �����ݿ��еı仯����ͬ�����ڴ� */
    check_db_data_to_poll_srv_list(pDboper);

    return;
}

/*****************************************************************************
 �� �� ��  : check_PollSrvConfig_need_to_reload_end
 ��������  : ����Ƿ���Ҫͬ����Ѳҵ�����ñ����
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
void check_PollSrvConfig_need_to_reload_end()
{
    /* ����Ƿ���Ҫ�������ݿ��ʶ */
    if (!db_PollSrvInfo_reload_mark)
    {
        return;
    }

    /* ɾ���Ѿ�ֹͣ����Ѳ���� */
    delete_poll_srv_data();

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "ͬ����Ѳҵ���������ݿ���Ϣ: ����---");
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Synchronization Poll Srv info database information: end---");
    db_PollSrvInfo_reload_mark = 0;

    return;
}
