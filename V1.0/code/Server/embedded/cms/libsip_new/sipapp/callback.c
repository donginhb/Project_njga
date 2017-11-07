/******************************************************************************

                  ��Ȩ���� (C), 2001-2013, ������Ѷ�������޹�˾

 ******************************************************************************
  �� �� ��   : callback.c
  �� �� ��   : ����
  ��    ��   : yanghaifeng
  ��������   : 2013��4��1��
  ����޸�   :
  ��������   : �ص���������
  �����б�   :
              app_callback_free
              app_callback_init
              app_set_bye_received_cb
              app_set_bye_response_received_cb
              app_set_info_received_cb
              app_set_info_response_received_cb
              app_set_invite_received_cb
              app_set_invite_response_received_cb
              app_set_message_received_cb
              app_set_message_response_received_cb
              app_set_register_received_cb
              app_set_register_response_received_cb
              cs_cb_ict_2xx2
              cs_cb_ict_rcv1xx
              cs_cb_ict_rcv2xx
              cs_cb_ict_rcv3xx
              cs_cb_ict_rcv456xx
              cs_cb_ist_ack_for2xx
              cs_cb_ist_snd1xx
              cs_cb_ist_snd2xx
              cs_cb_ist_snd3456xx
              cs_cb_kill_ict_transaction
              cs_cb_kill_ist_transaction
              cs_cb_kill_nict_transaction
              cs_cb_kill_nist_transaction
              cs_cb_nict_rcv1xx
              cs_cb_nict_rcv2xx
              cs_cb_nict_rcv3xx
              cs_cb_nict_rcv456xx
              cs_cb_nist_snd1xx
              cs_cb_nist_snd2xx
              cs_cb_nist_snd3456xx
              cs_cb_rcvack
              cs_cb_rcvack2
              cs_cb_rcvbye
              cs_cb_rcvcancel
              cs_cb_rcvinfo
              cs_cb_rcvinvite
              cs_cb_rcvmessage
              cs_cb_rcvnotify
              cs_cb_rcvoptions
              cs_cb_rcvrefer
              cs_cb_rcvregister
              cs_cb_rcvreq_retransmission
              cs_cb_rcvresp_retransmission
              cs_cb_rcvsubscribe
              cs_cb_rcvunkrequest
              cs_cb_sndack
              cs_cb_sndbye
              cs_cb_sndcancel
              cs_cb_sndinfo
              cs_cb_sndinvite
              cs_cb_sndoptions
              cs_cb_sndregister
              cs_cb_sndreq_retransmission
              cs_cb_sndresp_retransmission
              cs_cb_sndunkrequest
              cs_cb_transport_error
              sip_callback_init
  �޸���ʷ   :
  1.��    ��   : 2013��4��1��
    ��    ��   : yanghaifeng
    �޸�����   : �����ļ�

******************************************************************************/

/*----------------------------------------------*
 * ����ͷ�ļ�                                   *
 *----------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#ifdef WIN32
#include <winsock2.h>
#include <windows.h>
#else
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#endif

#include <osipparser2/osip_const.h>
#include <osip2/internal.h>
#include <osip2/osip.h>
#include <osip2/osip_fifo.h>
#include <osipparser2/osip_port.h>

#include "gbltype.h"
#include "callback.inc"

#include "sip_event.inc"
#include "sipmsg.inc"
#include "udp_tl.inc"
#include "csdbg.inc"

/*----------------------------------------------*
 * �ⲿ����˵��                                 *
 *----------------------------------------------*/
extern osip_t* g_recv_cell;           /* ������Ϣ��SIPЭ��ջ,��Message��Ϣר�� */
extern osip_t* g_recv_register_cell;  /* ������Ϣ��SIPЭ��ջ,Register��Ϣר�� */
extern osip_t* g_recv_message_cell;   /* ������Ϣ��SIPЭ��ջ,Message��Ϣר�� */
extern osip_t* g_send_cell;           /* ������Ϣ��SIPЭ��ջ,��Message��Ϣר�� */
extern osip_t* g_send_message_cell;   /* ������Ϣ��SIPЭ��ջ,Message��Ϣר�� */

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
app_callback_t* g_AppCallback = NULL;

/*----------------------------------------------*
 * ��������                                     *
 *----------------------------------------------*/

/*----------------------------------------------*
 * �궨��                                       *
 *----------------------------------------------*/

#if DECS("Э��ջ�ص�����")
/*****************************************************************************
 �� �� ��  : cs_cb_rcvinvite
 ��������  : ����INVITE��Ϣ�ص�����
 �������  : transaction_t * tr
                            sip_t * sip
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��1��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void cs_cb_rcvinvite(int type, osip_transaction_t* tr, osip_message_t* sip)
{
    if (NULL == tr || NULL == sip)
    {
        return;
    }

    //SIP_DEBUG_TRACE(LOG_DEBUG, "cs_cb_rcvinvite() Enter--- \r\n");

    OnRcvInvite(tr, sip);

    return;
}

/*****************************************************************************
 �� �� ��  : cs_cb_rcvack
 ��������  : ����ACK��Ϣ�ص�����
 �������  : transaction_t * tr
                            sip_t * sip
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��1��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void cs_cb_rcvack(int type, osip_transaction_t* tr, osip_message_t* sip)
{
    if (NULL == tr || NULL == sip)
    {
        return;
    }

    //SIP_DEBUG_TRACE(LOG_DEBUG, "cs_cb_rcvack() Enter--- \r\n");

    OnRcvAck(tr, sip);

    return;
}

/*****************************************************************************
 �� �� ��  : cs_cb_rcvack2
 ��������  : ����ACK��Ϣ�ص�����
 �������  : transaction_t * tr
                            sip_t * sip
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��1��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void cs_cb_rcvack2(int type, osip_transaction_t* tr, osip_message_t* sip)
{
    //SIP_DEBUG_TRACE(LOG_DEBUG, "cs_cb_rcvack2() Enter--- \r\n");

    return;
}

/*****************************************************************************
 �� �� ��  : cs_cb_rcvregister
 ��������  : ����REGISTER��Ϣ�ص�����
 �������  : transaction_t * tr
                            osip_message_t * sip
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��1��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void cs_cb_rcvregister(int type, osip_transaction_t* tr, osip_message_t* sip)
{
    if (NULL == tr || NULL == sip)
    {
        return;
    }

    //SIP_DEBUG_TRACE(LOG_DEBUG, "cs_cb_rcvregister() Recv Register Message:From Host=%s \r\n", get_message_from_host(sip));
    //printf("\r\n ########## cs_cb_rcvregister() Recv Register Message:From Host=%s \r\n", get_message_from_host(sip));
    //printf_system_time1();

    OnRcvRegister(tr, sip);

    return;
}

/*****************************************************************************
 �� �� ��  : cs_cb_rcvbye
 ��������  : ����BYE��Ϣ�ص�����
 �������  : transaction_t * tr
                            sip_t * sip
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��1��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void cs_cb_rcvbye(int type, osip_transaction_t* tr, osip_message_t* sip)
{
    if (NULL == tr || NULL == sip)
    {
        return;
    }

    //SIP_DEBUG_TRACE(LOG_DEBUG, "cs_cb_rcvbye() Enter--- \r\n");

    OnRcvBye(tr, sip);

    return;
}

/*****************************************************************************
 �� �� ��  : cs_cb_rcvoptions
 ��������  : ����OPTIONS��Ϣ�ص�����
 �������  : transaction_t * tr
                            sip_t * sip
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��1��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void cs_cb_rcvoptions(int type, osip_transaction_t* tr, osip_message_t* sip)
{
    if (NULL == tr || NULL == sip)
    {
        return;
    }

    //SIP_DEBUG_TRACE(LOG_DEBUG, "cs_cb_rcvoptions() Enter--- \r\n");

    OnRcvOptions(tr, sip);

    return;
}

/*****************************************************************************
 �� �� ��  : cs_cb_rcvinfo
 ��������  : ����INFO��Ϣ�ص�����
 �������  : transaction_t * tr
                            sip_t * sip
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��1��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void cs_cb_rcvinfo(int type, osip_transaction_t* tr, osip_message_t* sip)
{
    if (NULL == tr || NULL == sip)
    {
        return;
    }

    //SIP_DEBUG_TRACE(LOG_DEBUG, "cs_cb_rcvinfo() Enter--- \r\n");

    OnRcvInfo(tr, sip);

    return;
}

/*****************************************************************************
 �� �� ��  : cs_cb_rcvupdate
 ��������  : ����UPDATE��Ϣ�ص�����
 �������  : transaction_t* tr
                            sip_t* sip
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��9��2�� ����һ
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void cs_cb_rcvupdate(int type, osip_transaction_t* tr, osip_message_t* sip)
{
    if (NULL == tr || NULL == sip)
    {
        return;
    }

    //SIP_DEBUG_TRACE(LOG_DEBUG, "cs_cb_rcvupdate() Enter--- \r\n");

    OnRcvUpdate(tr, sip);

    return;
}

/*****************************************************************************
 �� �� ��  : cs_cb_rcvcancel
 ��������  : ����CANCEL��Ϣ�ص�����
 �������  : transaction_t * tr
                            sip_t * sip
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��1��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void cs_cb_rcvcancel(int type, osip_transaction_t* tr, osip_message_t* sip)
{
    if (NULL == tr || NULL == sip)
    {
        return;
    }

    //SIP_DEBUG_TRACE(LOG_DEBUG, "cs_cb_rcvcancel() Enter--- \r\n");

    OnRcvCancel(tr, sip);

    return;
}

/*****************************************************************************
 �� �� ��  : cs_cb_rcvnotify
 ��������  : ����NODIFY��Ϣ�ص�����
 �������  : transaction_t * tr
                            sip_t * sip
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��1��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void cs_cb_rcvnotify(int type, osip_transaction_t* tr, osip_message_t* sip)
{
    if (NULL == tr || NULL == sip)
    {
        return;
    }

    //SIP_DEBUG_TRACE(LOG_DEBUG, "cs_cb_rcvnotify() Enter--- \r\n");

    OnRcvNotify(tr, sip);

    return;
}

/*****************************************************************************
 �� �� ��  : cs_cb_rcvsubscribe
 ��������  : ����SUBSCRIBE��Ϣ�ص�����
 �������  : transaction_t * tr
                            sip_t * sip
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��1��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void cs_cb_rcvsubscribe(int type, osip_transaction_t* tr, osip_message_t* sip)
{
    if (NULL == tr || NULL == sip)
    {
        return;
    }

    //SIP_DEBUG_TRACE(LOG_DEBUG, "cs_cb_rcvsubscribe() Enter--- \r\n");

    OnRcvSubscribe(tr, sip);

    return;
}

/*****************************************************************************
 �� �� ��  : cs_cb_rcvrefer
 ��������  : ����REFER��Ϣ�ص�����
 �������  : transaction_t * tr
                            sip_t * sip
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��1��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void cs_cb_rcvrefer(int type, osip_transaction_t* tr, osip_message_t* sip)
{
    if (NULL == tr || NULL == sip)
    {
        return;
    }

    //SIP_DEBUG_TRACE(LOG_DEBUG, "cs_cb_rcvrefer() Enter--- \r\n");

    OnRcvRefer(tr, sip);

    return;
}

/*****************************************************************************
 �� �� ��  : cs_cb_rcvmessage
 ��������  : ����MESSAGE��Ϣ�ص�����
 �������  : transaction_t * tr
                            sip_t * sip
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��1��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void cs_cb_rcvmessage(int type, osip_transaction_t* tr, osip_message_t* sip)
{
    if (NULL == tr || NULL == sip)
    {
        return;
    }

    //if (NULL != sip->from && NULL != sip->from->url && NULL != sip->to && NULL != sip->to->url)
    //{
    //if ((0 == sstrcmp(sip->from->url->username, (char*)"wiscomCallerID"))
    //&& (0 == sstrcmp(sip->to->url->username, (char*)"wiscomCalleeID"))) /* ��ѯ������ID���û�ID */
    //{
    //SIP_DEBUG_TRACE(LOG_DEBUG, "cs_cb_rcvmessage() Recv Get ServerID Message:From Host=%s \r\n", sip->from->url->host);
    //printf("\r\n ********** cs_cb_rcvmessage() Recv Get ServerID Message:From Host=%s \r\n", sip->from->url->host);
    //printf_system_time1();
    //}
    //}

    OnRcvMessage(tr, sip);

    return;
}

/*****************************************************************************
 �� �� ��  : cs_cb_rcvunkrequest
 ��������  : ��������δ֪������Ϣ�ص�����
 �������  : transaction_t * tr
                            sip_t * sip
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��1��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void cs_cb_rcvunkrequest(int type, osip_transaction_t* tr, osip_message_t* sip)
{
    if (NULL == tr || NULL == sip)
    {
        return;
    }

    //SIP_DEBUG_TRACE(LOG_DEBUG, "cs_cb_rcvunkrequest() Enter--- \r\n");

    OnRcvUnknown(tr, sip);

    return;
}

/*****************************************************************************
 �� �� ��  : cs_cb_ict_rcv1xx
 ��������  : ICT�������1XX��Ӧ��Ϣ�ص�����
 �������  : transaction_t * tr
                            sip_t * sip
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��1��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void cs_cb_ict_rcv1xx(int type, osip_transaction_t* tr , osip_message_t* sip)
{
    if (NULL == tr || NULL == sip)
    {
        return;
    }

    //SIP_DEBUG_TRACE(LOG_DEBUG, "cs_cb_ict_rcv1xx() Enter--- \r\n");

    OnRcv1xxForInvite(tr, sip);

    return;
}

/*****************************************************************************
 �� �� ��  : cs_cb_ict_rcv2xx
 ��������  : ICT�������2XX��Ӧ��Ϣ�ص�����
 �������  : transaction_t * tr
                            sip_t * sip
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��1��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void cs_cb_ict_rcv2xx(int type, osip_transaction_t* tr, osip_message_t* sip)
{
    if (NULL == tr || NULL == sip)
    {
        return;
    }

    //SIP_DEBUG_TRACE(LOG_DEBUG, "cs_cb_ict_rcv2xx() Enter--- \r\n");

    OnRcv2xxForInvite(tr, sip);

    return;
}

/*****************************************************************************
 �� �� ��  : cs_cb_ict_rcv3xx
 ��������  : ICT�������3XX��Ӧ��Ϣ�ص�����
 �������  : transaction_t * tr
                            sip_t * sip
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��1��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void cs_cb_ict_rcv3xx(int type, osip_transaction_t* tr, osip_message_t* sip)
{
    if (NULL == tr || NULL == sip)
    {
        return;
    }

    //SIP_DEBUG_TRACE(LOG_DEBUG, "cs_cb_ict_rcv3xx() Enter--- \r\n");

    OnRcv3xxForInvite(tr, sip);

    return;
}

/*****************************************************************************
 �� �� ��  : cs_cb_ict_rcv456xx
 ��������  : ICT�������456XX��Ӧ��Ϣ�ص�����
 �������  : transaction_t * tr
                            sip_t * sip
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��1��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void cs_cb_ict_rcv456xx(int type, osip_transaction_t* tr, osip_message_t* sip)
{
    if (NULL == tr || NULL == sip)
    {
        return;
    }

    //SIP_DEBUG_TRACE(LOG_DEBUG, "cs_cb_ict_rcv456xx() Enter--- \r\n");

    OnRcv456xxForInvite(tr, sip);

    return;
}

/*****************************************************************************
 �� �� ��  : cs_cb_nict_rcv1xx
 ��������  : NICT�������1XX��Ӧ��Ϣ�ص�����
 �������  : transaction_t * tr
                            sip_t * sip
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��1��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void cs_cb_nict_rcv1xx(int type, osip_transaction_t* tr, osip_message_t* sip)
{
    if (NULL == tr || NULL == sip)
    {
        return;
    }

    //SIP_DEBUG_TRACE(LOG_DEBUG, "cs_cb_nict_rcv1xx() Enter--- \r\n");

    OnRcv1xxForRequest(tr, sip);

    return;
}

/*****************************************************************************
 �� �� ��  : cs_cb_nict_rcv2xx
 ��������  : NICT�������2XX��Ӧ��Ϣ�ص�����
 �������  : transaction_t * tr
                            sip_t * sip
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��1��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void cs_cb_nict_rcv2xx(int type, osip_transaction_t* tr, osip_message_t* sip)
{
    if (NULL == tr || NULL == sip)
    {
        return;
    }

    //SIP_DEBUG_TRACE(LOG_DEBUG, "cs_cb_nict_rcv2xx() Enter--- \r\n");

    if (MSG_IS_RESPONSE_FOR(sip, REGISTER_METHOD))
    {
        OnRcv2xxForRegister(tr, sip);
    }
    else if (MSG_IS_RESPONSE_FOR(sip, CANCEL_METHOD))
    {
        OnRcv2xxForCancel(tr, sip);
    }
    else if (MSG_IS_RESPONSE_FOR(sip, BYE_METHOD))
    {
        OnRcv2xxForBye(tr, sip);
    }
    else if (MSG_IS_RESPONSE_FOR(sip, UPDATE_METHOD))
    {
        OnRcv2xxForUpdate(tr, sip);
    }
    else if (MSG_IS_RESPONSE_FOR(sip, INFO_METHOD))
    {
        OnRcv2xxForInfo(tr, sip);
    }
    else if (MSG_IS_RESPONSE_FOR(sip, OPTIONS_METHOD))
    {
        OnRcv2xxForOptions(tr, sip);
    }
    else if (MSG_IS_RESPONSE_FOR(sip, SUBSCRIBE_METHOD))
    {
        OnRcv2xxForSubscribe(tr, sip);
    }
    else if (MSG_IS_RESPONSE_FOR(sip, NOTIFY_METHOD))
    {
        OnRcv2xxForNotify(tr, sip);
    }
    else if (MSG_IS_RESPONSE_FOR(sip, REFER_METHOD))
    {
        OnRcv2xxForRefer(tr, sip);
    }
    else if (MSG_IS_RESPONSE_FOR(sip, MESSAGE_METHOD))
    {
        OnRcv2xxForMessage(tr, sip);
    }

    return;
}

/*****************************************************************************
 �� �� ��  : cs_cb_nict_rcv3xx
 ��������  : NICT�������3XX��Ӧ��Ϣ�ص�����
 �������  : transaction_t * tr
                            sip_t * sip
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��1��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void cs_cb_nict_rcv3xx(int type, osip_transaction_t* tr, osip_message_t* sip)
{
    if (NULL == tr || NULL == sip)
    {
        return;
    }

    //SIP_DEBUG_TRACE(LOG_DEBUG, "cs_cb_nict_rcv3xx() Enter--- \r\n");

    if (MSG_IS_RESPONSE_FOR(sip, REGISTER_METHOD))
    {
        OnRcv3xxForRegister(tr, sip);
    }
    else if (MSG_IS_RESPONSE_FOR(sip, CANCEL_METHOD))
    {
        OnRcv3xxForCancel(tr, sip);
    }
    else if (MSG_IS_RESPONSE_FOR(sip, BYE_METHOD))
    {
        OnRcv3xxForBye(tr, sip);
    }
    else if (MSG_IS_RESPONSE_FOR(sip, UPDATE_METHOD))
    {
        OnRcv3xxForUpdate(tr, sip);
    }
    else if (MSG_IS_RESPONSE_FOR(sip, INFO_METHOD))
    {
        OnRcv3xxForInfo(tr, sip);
    }
    else if (MSG_IS_RESPONSE_FOR(sip, OPTIONS_METHOD))
    {
        OnRcv3xxForOptions(tr, sip);
    }
    else if (MSG_IS_RESPONSE_FOR(sip, SUBSCRIBE_METHOD))
    {
        OnRcv3xxForSubscribe(tr, sip);
    }
    else if (MSG_IS_RESPONSE_FOR(sip, NOTIFY_METHOD))
    {
        OnRcv3xxForNotify(tr, sip);
    }
    else if (MSG_IS_RESPONSE_FOR(sip, REFER_METHOD))
    {
        OnRcv3xxForRefer(tr, sip);
    }

    return;
}

/*****************************************************************************
 �� �� ��  : cs_cb_nict_rcv456xx
 ��������  : NICT�������456XX��Ӧ��Ϣ�ص�����
 �������  : transaction_t * tr
                            sip_t * sip
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��1��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void cs_cb_nict_rcv456xx(int type, osip_transaction_t* tr, osip_message_t* sip)
{
    if (NULL == tr || NULL == sip)
    {
        return;
    }

    //SIP_DEBUG_TRACE(LOG_DEBUG, "cs_cb_nict_rcv456xx() Enter--- \r\n");

    if (MSG_IS_RESPONSE_FOR(sip, REGISTER_METHOD))
    {
        OnRcv456xxForRegister(tr, sip);
    }
    else if (MSG_IS_RESPONSE_FOR(sip, CANCEL_METHOD))
    {
        OnRcv456xxForCancel(tr, sip);
    }
    else if (MSG_IS_RESPONSE_FOR(sip, BYE_METHOD))
    {
        OnRcv456xxForBye(tr, sip);
    }
    else if (MSG_IS_RESPONSE_FOR(sip, UPDATE_METHOD))
    {
        OnRcv456xxForUpdate(tr, sip);
    }
    else if (MSG_IS_RESPONSE_FOR(sip, INFO_METHOD))
    {
        OnRcv456xxForInfo(tr, sip);
    }
    else if (MSG_IS_RESPONSE_FOR(sip, OPTIONS_METHOD))
    {
        OnRcv456xxForOptions(tr, sip);
    }
    else if (MSG_IS_RESPONSE_FOR(sip, SUBSCRIBE_METHOD))
    {
        OnRcv456xxForSubscribe(tr, sip);
    }
    else if (MSG_IS_RESPONSE_FOR(sip, NOTIFY_METHOD))
    {
        OnRcv456xxForNotify(tr, sip);
    }
    else if (MSG_IS_RESPONSE_FOR(sip, REFER_METHOD))
    {
        OnRcv456xxForRefer(tr, sip);
    }
    else if (MSG_IS_RESPONSE_FOR(sip, MESSAGE_METHOD))
    {
        OnRcv456xxForMessage(tr, sip);
    }

    return;
}

/*****************************************************************************
 �� �� ��  : cs_cb_ist_ack_for2xx
 ��������  : IST�������ACK for 2xx��Ӧ��Ϣ�ص�����
 �������  : sip_t * sip
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��1��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void cs_cb_ist_ack_for2xx(osip_message_t* sip)
{
    if (NULL == sip)
    {
        return;
    }

    //SIP_DEBUG_TRACE(LOG_DEBUG, "cs_cb_ist_ack_for2xx() Enter--- \r\n");

    OnRcvIstAckfor2xx(sip);

    return;
}

/*****************************************************************************
 �� �� ��  : cs_cb_ict_2xx2
 ��������  : ICT�������2xx�ط���Ϣ�ص�����
 �������  : sip_t * sip
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��1��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void cs_cb_ict_2xx2(osip_message_t* sip)
{
    if (NULL == sip)
    {
        return;
    }

    //SIP_DEBUG_TRACE(LOG_DEBUG, "cs_cb_ict_2xx2() Enter--- \r\n");

    OnRcvIct2xx2(sip);

    return;
}

/*****************************************************************************
 �� �� ��  : cs_cb_ist_snd1xx
 ��������  : IST������1xx��Ϣ�ص�����
 �������  : transaction_t * tr
                            sip_t * sip
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��1��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void cs_cb_ist_snd1xx(int type, osip_transaction_t* tr, osip_message_t* sip)
{
    return;
}

/*****************************************************************************
 �� �� ��  : cs_cb_ist_snd2xx
 ��������  : IST������2xx��Ϣ�ص�����
 �������  : transaction_t * tr
                            sip_t * sip
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��1��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void cs_cb_ist_snd2xx(int type, osip_transaction_t* tr, osip_message_t* sip)
{
    return;
}

/*****************************************************************************
 �� �� ��  : cs_cb_ist_snd3456xx
 ��������  : IST������3456xx��Ϣ�ص�����
 �������  : transaction_t * tr
                            sip_t * sip
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��1��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void cs_cb_ist_snd3456xx(int type, osip_transaction_t* tr, osip_message_t* sip)
{
    return;
}

/*****************************************************************************
 �� �� ��  : cs_cb_nist_snd1xx
 ��������  : NIST������1xx��Ϣ�ص�����
 �������  : transaction_t * tr
                            sip_t * sip
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��1��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void cs_cb_nist_snd1xx(int type, osip_transaction_t* tr, osip_message_t* sip)
{
    return;
}

/*****************************************************************************
 �� �� ��  : cs_cb_nist_snd2xx
 ��������  : NIST������2xx��Ϣ�ص�����
 �������  : transaction_t * tr
                            sip_t * sip
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��1��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void cs_cb_nist_snd2xx(int type, osip_transaction_t* tr, osip_message_t* sip)
{
    return;
}

/*****************************************************************************
 �� �� ��  : cs_cb_nist_snd3456xx
 ��������  : NIST������3456xx��Ϣ�ص�����
 �������  : transaction_t * tr
                            sip_t * sip
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��1��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void cs_cb_nist_snd3456xx(int type, osip_transaction_t* tr, osip_message_t* sip)
{
    return;
}

/*****************************************************************************
 �� �� ��  : cs_cb_kill_ict_transaction_for_rcv
 ��������  :  ɾ��ICT����ص�����
 �������  : transaction_t * tr
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��1��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void cs_cb_kill_ict_transaction_for_rcv(int type, osip_transaction_t* tr)
{
    if (NULL == tr)
    {
        return;
    }

    osip_remove_transaction(g_recv_cell, tr);

    //SIP_DEBUG_TRACE(LOG_DEBUG, "cs_cb_kill_ict_transaction() Enter--- \r\n");

    OnKillIctTransactionForRecv(tr);

    return;
}

/*****************************************************************************
 �� �� ��  : cs_cb_kill_ist_transaction_for_rcv
 ��������  : ɾ��IST����ص�����
 �������  : transaction_t * tr
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��1��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void cs_cb_kill_ist_transaction_for_rcv(int type, osip_transaction_t* tr)
{
    if (NULL == tr)
    {
        return;
    }

    osip_remove_transaction(g_recv_cell, tr);

    //SIP_DEBUG_TRACE(LOG_DEBUG, "cs_cb_kill_ist_transaction() Enter--- \r\n");

    OnKillIstTransactionForRecv(tr);

    return;
}

/*****************************************************************************
 �� �� ��  : cs_cb_kill_nict_transaction_for_rcv
 ��������  : ɾ��NICT����ص�����
 �������  : transaction_t * tr
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��1��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void cs_cb_kill_nict_transaction_for_rcv(int type, osip_transaction_t* tr)
{
    if (NULL == tr)
    {
        return;
    }

    osip_remove_transaction(g_recv_cell, tr);

    //SIP_DEBUG_TRACE(LOG_DEBUG, "cs_cb_kill_nict_transaction() Enter--- \r\n");

    OnKillNictTransactionForRecv(tr);

    return;
}

/*****************************************************************************
 �� �� ��  : cs_cb_kill_nist_transaction_for_rcv
 ��������  : ɾ��NIST����ص�����
 �������  : transaction_t * tr
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��1��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void cs_cb_kill_nist_transaction_for_rcv(int type, osip_transaction_t* tr)
{
    if (NULL == tr)
    {
        return;
    }

    osip_remove_transaction(g_recv_cell, tr);

    //SIP_DEBUG_TRACE(LOG_DEBUG, "cs_cb_kill_nist_transaction() Enter--- \r\n");

    OnKillNistTransactionForRecv(tr);

    return;
}

/*****************************************************************************
 �� �� ��  : cs_cb_kill_ict_transaction_for_rcv_register
 ��������  :  ɾ��ICT����ص�����
 �������  : transaction_t * tr
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��1��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void cs_cb_kill_ict_transaction_for_rcv_register(int type, osip_transaction_t* tr)
{
    if (NULL == tr)
    {
        return;
    }

    osip_remove_transaction(g_recv_register_cell, tr);

    //SIP_DEBUG_TRACE(LOG_DEBUG, "cs_cb_kill_ict_transaction() Enter--- \r\n");

    OnKillIctTransactionForRecvRegister(tr);

    return;
}

/*****************************************************************************
 �� �� ��  : cs_cb_kill_ist_transaction_for_rcv_register
 ��������  : ɾ��IST����ص�����
 �������  : transaction_t * tr
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��1��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void cs_cb_kill_ist_transaction_for_rcv_register(int type, osip_transaction_t* tr)
{
    if (NULL == tr)
    {
        return;
    }

    osip_remove_transaction(g_recv_register_cell, tr);

    //SIP_DEBUG_TRACE(LOG_DEBUG, "cs_cb_kill_ist_transaction() Enter--- \r\n");

    OnKillIstTransactionForRecvRegister(tr);

    return;
}

/*****************************************************************************
 �� �� ��  : cs_cb_kill_nict_transaction_for_rcv_register
 ��������  : ɾ��NICT����ص�����
 �������  : transaction_t * tr
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��1��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void cs_cb_kill_nict_transaction_for_rcv_register(int type, osip_transaction_t* tr)
{
    if (NULL == tr)
    {
        return;
    }

    osip_remove_transaction(g_recv_register_cell, tr);

    //SIP_DEBUG_TRACE(LOG_DEBUG, "cs_cb_kill_nict_transaction() Enter--- \r\n");

    OnKillNictTransactionForRecvRegister(tr);

    return;
}

/*****************************************************************************
 �� �� ��  : cs_cb_kill_nist_transaction_for_rcv_register
 ��������  : ɾ��NIST����ص�����
 �������  : transaction_t * tr
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��1��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void cs_cb_kill_nist_transaction_for_rcv_register(int type, osip_transaction_t* tr)
{
    if (NULL == tr)
    {
        return;
    }

    osip_remove_transaction(g_recv_register_cell, tr);

    //SIP_DEBUG_TRACE(LOG_DEBUG, "cs_cb_kill_nist_transaction() Enter--- \r\n");

    OnKillNistTransactionForRecvRegister(tr);

    return;
}

/*****************************************************************************
 �� �� ��  : cs_cb_kill_ict_transaction_for_rcv_msg
 ��������  :  ɾ��ICT����ص�����
 �������  : transaction_t * tr
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��1��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void cs_cb_kill_ict_transaction_for_rcv_msg(int type, osip_transaction_t* tr)
{
    if (NULL == tr)
    {
        return;
    }

    osip_remove_transaction(g_recv_message_cell, tr);

    //SIP_DEBUG_TRACE(LOG_DEBUG, "cs_cb_kill_ict_transaction() Enter--- \r\n");

    OnKillIctTransactionForRecvMsg(tr);

    return;
}

/*****************************************************************************
 �� �� ��  : cs_cb_kill_ist_transaction_for_rcv_msg
 ��������  : ɾ��IST����ص�����
 �������  : transaction_t * tr
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��1��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void cs_cb_kill_ist_transaction_for_rcv_msg(int type, osip_transaction_t* tr)
{
    if (NULL == tr)
    {
        return;
    }

    osip_remove_transaction(g_recv_message_cell, tr);

    //SIP_DEBUG_TRACE(LOG_DEBUG, "cs_cb_kill_ist_transaction() Enter--- \r\n");

    OnKillIstTransactionForRecvMsg(tr);

    return;
}

/*****************************************************************************
 �� �� ��  : cs_cb_kill_nict_transaction_for_rcv_msg
 ��������  : ɾ��NICT����ص�����
 �������  : transaction_t * tr
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��1��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void cs_cb_kill_nict_transaction_for_rcv_msg(int type, osip_transaction_t* tr)
{
    if (NULL == tr)
    {
        return;
    }

    osip_remove_transaction(g_recv_message_cell, tr);

    //SIP_DEBUG_TRACE(LOG_DEBUG, "cs_cb_kill_nict_transaction() Enter--- \r\n");

    OnKillNictTransactionForRecvMsg(tr);

    return;
}

/*****************************************************************************
 �� �� ��  : cs_cb_kill_nist_transaction_for_rcv_msg
 ��������  : ɾ��NIST����ص�����
 �������  : transaction_t * tr
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��1��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void cs_cb_kill_nist_transaction_for_rcv_msg(int type, osip_transaction_t* tr)
{
    if (NULL == tr)
    {
        return;
    }

    osip_remove_transaction(g_recv_message_cell, tr);

    //SIP_DEBUG_TRACE(LOG_DEBUG, "cs_cb_kill_nist_transaction() Enter--- \r\n");

    OnKillNistTransactionForRecvMsg(tr);

    return;
}

/*****************************************************************************
 �� �� ��  : cs_cb_kill_ict_transaction_for_send
 ��������  :  ɾ��ICT����ص�����
 �������  : transaction_t * tr
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��1��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void cs_cb_kill_ict_transaction_for_send(int type, osip_transaction_t* tr)
{
    if (NULL == tr)
    {
        return;
    }

    osip_remove_transaction(g_send_cell, tr);

    //SIP_DEBUG_TRACE(LOG_DEBUG, "cs_cb_kill_ict_transaction() Enter--- \r\n");

    OnKillIctTransactionForSend(tr);

    return;
}

/*****************************************************************************
 �� �� ��  : cs_cb_kill_ist_transaction_for_send
 ��������  : ɾ��IST����ص�����
 �������  : transaction_t * tr
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��1��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void cs_cb_kill_ist_transaction_for_send(int type, osip_transaction_t* tr)
{
    if (NULL == tr)
    {
        return;
    }

    osip_remove_transaction(g_send_cell, tr);

    //SIP_DEBUG_TRACE(LOG_DEBUG, "cs_cb_kill_ist_transaction() Enter--- \r\n");

    OnKillIstTransactionForSend(tr);

    return;
}

/*****************************************************************************
 �� �� ��  : cs_cb_kill_nict_transaction_for_send
 ��������  : ɾ��NICT����ص�����
 �������  : transaction_t * tr
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��1��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void cs_cb_kill_nict_transaction_for_send(int type, osip_transaction_t* tr)
{
    if (NULL == tr)
    {
        return;
    }

    osip_remove_transaction(g_send_cell, tr);

    //SIP_DEBUG_TRACE(LOG_DEBUG, "cs_cb_kill_nict_transaction() Enter--- \r\n");

    OnKillNictTransactionForSend(tr);

    return;
}

/*****************************************************************************
 �� �� ��  : cs_cb_kill_nist_transaction_for_send
 ��������  : ɾ��NIST����ص�����
 �������  : transaction_t * tr
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��1��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void cs_cb_kill_nist_transaction_for_send(int type, osip_transaction_t* tr)
{
    if (NULL == tr)
    {
        return;
    }

    osip_remove_transaction(g_send_cell, tr);

    //SIP_DEBUG_TRACE(LOG_DEBUG, "cs_cb_kill_nist_transaction() Enter--- \r\n");

    OnKillNistTransactionForSend(tr);

    return;
}

/*****************************************************************************
 �� �� ��  : cs_cb_kill_ict_transaction_for_send_msg
 ��������  :  ɾ��ICT����ص�����
 �������  : transaction_t * tr
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��1��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void cs_cb_kill_ict_transaction_for_send_msg(int type, osip_transaction_t* tr)
{
    if (NULL == tr)
    {
        return;
    }

    osip_remove_transaction(g_send_message_cell, tr);

    //SIP_DEBUG_TRACE(LOG_DEBUG, "cs_cb_kill_ict_transaction() Enter--- \r\n");

    OnKillIctTransactionForSendMsg(tr);

    return;
}

/*****************************************************************************
 �� �� ��  : cs_cb_kill_ist_transaction_for_send_msg
 ��������  : ɾ��IST����ص�����
 �������  : transaction_t * tr
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��1��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void cs_cb_kill_ist_transaction_for_send_msg(int type, osip_transaction_t* tr)
{
    if (NULL == tr)
    {
        return;
    }

    osip_remove_transaction(g_send_message_cell, tr);

    //SIP_DEBUG_TRACE(LOG_DEBUG, "cs_cb_kill_ist_transaction() Enter--- \r\n");

    OnKillIstTransactionForSendMsg(tr);

    return;
}

/*****************************************************************************
 �� �� ��  : cs_cb_kill_nict_transaction_for_send_msg
 ��������  : ɾ��NICT����ص�����
 �������  : transaction_t * tr
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��1��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void cs_cb_kill_nict_transaction_for_send_msg(int type, osip_transaction_t* tr)
{
    if (NULL == tr)
    {
        return;
    }

    osip_remove_transaction(g_send_message_cell, tr);

    //SIP_DEBUG_TRACE(LOG_DEBUG, "cs_cb_kill_nict_transaction() Enter--- \r\n");

    OnKillNictTransactionForSendMsg(tr);

    return;
}

/*****************************************************************************
 �� �� ��  : cs_cb_kill_nist_transaction_for_send_msg
 ��������  : ɾ��NIST����ص�����
 �������  : transaction_t * tr
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��1��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void cs_cb_kill_nist_transaction_for_send_msg(int type, osip_transaction_t* tr)
{
    if (NULL == tr)
    {
        return;
    }

    osip_remove_transaction(g_send_message_cell, tr);

    //SIP_DEBUG_TRACE(LOG_DEBUG, "cs_cb_kill_nist_transaction() Enter--- \r\n");

    OnKillNistTransactionForSendMsg(tr);

    return;
}

/*****************************************************************************
 �� �� ��  : cs_cb_rcvresp_retransmission
 ��������  : �յ���Ӧ��Ϣ�ش��ص�����
 �������  : transaction_t * tr
                            sip_t * sip
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��1��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void cs_cb_rcvresp_retransmission(int type, osip_transaction_t* tr, osip_message_t* sip)
{
    return;
}

/*****************************************************************************
 �� �� ��  : cs_cb_rcvreq_retransmission
 ��������  : �յ�������Ϣ�ش��ص�����
 �������  : transaction_t * tr
                            sip_t * sip
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��1��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void cs_cb_rcvreq_retransmission(int type, osip_transaction_t* tr, osip_message_t* sip)
{
    return;
}

/*****************************************************************************
 �� �� ��  : cs_cb_sndresp_retransmission
 ��������  : ���ͻ�Ӧ��Ϣ�ش��ص�����
 �������  : transaction_t * tr
                            sip_t * sip
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��1��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void cs_cb_sndresp_retransmission(int type, osip_transaction_t* tr, osip_message_t* sip)
{
    return;
}

/*****************************************************************************
 �� �� ��  : cs_cb_sndreq_retransmission
 ��������  : ����������Ϣ�ش��ص�����
 �������  : transaction_t * tr
                            sip_t * sip
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��1��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void cs_cb_sndreq_retransmission(int type, osip_transaction_t* tr, osip_message_t* sip)
{
    return;
}

/*****************************************************************************
 �� �� ��  : cs_cb_transport_error
 ��������  : �������ص�����
 �������  : transaction_t * tr
                            sip_t * sip
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��1��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void cs_cb_transport_error(int type, osip_transaction_t* tr, int error)
{
    return;
}

/*****************************************************************************
 �� �� ��  : cs_cb_sndinvite
 ��������  : ����INVITE�ص�����
 �������  : transaction_t * tr
                            sip_t * sip
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��1��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void cs_cb_sndinvite(int type, osip_transaction_t* tr, osip_message_t* sip)
{
    return;
}

/*****************************************************************************
 �� �� ��  : cs_cb_sndack
 ��������  : ����ACK�ص�����
 �������  : transaction_t * tr
                            sip_t * sip
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��1��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void cs_cb_sndack(int type, osip_transaction_t* tr, osip_message_t* sip)
{
    return;
}

/*****************************************************************************
 �� �� ��  : cs_cb_sndregister
 ��������  : ����REGISTER�ص�����
 �������  : transaction_t * tr
                            sip_t * sip
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��1��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void cs_cb_sndregister(int type, osip_transaction_t* tr, osip_message_t* sip)
{
    return;
}

/*****************************************************************************
 �� �� ��  : cs_cb_sndbye
 ��������  : ����BYE�ص�����
 �������  : transaction_t * tr
                            sip_t * sip
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��1��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void cs_cb_sndbye(int type, osip_transaction_t* tr, osip_message_t* sip)
{
    return;
}

/*****************************************************************************
 �� �� ��  : cs_cb_sndcancel
 ��������  : ����CANCEL�ص�����
 �������  : transaction_t * tr
                            sip_t * sip
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��1��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void cs_cb_sndcancel(int type, osip_transaction_t* tr, osip_message_t* sip)
{
    return;
}

/*****************************************************************************
 �� �� ��  : cs_cb_sndinfo
 ��������  : ����INFO�ص�����
 �������  : transaction_t * tr
                            sip_t * sip
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��1��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void cs_cb_sndinfo(int type, osip_transaction_t* tr, osip_message_t* sip)
{
    return;
}

/*****************************************************************************
 �� �� ��  : cs_cb_sndoptions
 ��������  : ����OPTIONS�ص�����
 �������  : transaction_t * tr
                            sip_t * sip
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��1��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void cs_cb_sndoptions(int type, osip_transaction_t* tr, osip_message_t* sip)
{
    return;
}

/*****************************************************************************
 �� �� ��  : cs_cb_sndunkrequest
 ��������  : ��������δ֪������Ϣ�ص�����
 �������  : transaction_t * tr
                            sip_t * sip
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��1��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void cs_cb_sndunkrequest(int type, osip_transaction_t* tr, osip_message_t* sip)
{
    return;
}

void cs_cb_ict_timeout(int type, osip_transaction_t* tr, osip_message_t* sip)
{
    return;
}

void cs_cb_nict_timeout(int type, osip_transaction_t* tr, osip_message_t* sip)
{
    return;
}
/*****************************************************************************
 �� �� ��  : sip_callback_init
 ��������  : SIPЭ��ջ�ص���������
 �������  : cell_t * cell
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��1��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int sip_callback_init(osip_t* cell)
{
    /*int i;*/
    if (cell == NULL)
    {
        return -1;
    }

    osip_set_cb_send_message(cell, &send_message_using_udp);

    osip_set_transport_error_callback(cell, OSIP_ICT_TRANSPORT_ERROR, &cs_cb_transport_error);
    osip_set_transport_error_callback(cell, OSIP_IST_TRANSPORT_ERROR, &cs_cb_transport_error);
    osip_set_transport_error_callback(cell, OSIP_NICT_TRANSPORT_ERROR, &cs_cb_transport_error);
    osip_set_transport_error_callback(cell, OSIP_NIST_TRANSPORT_ERROR, &cs_cb_transport_error);

    osip_set_message_callback(cell, OSIP_ICT_INVITE_SENT, &cs_cb_sndinvite);
    osip_set_message_callback(cell, OSIP_ICT_INVITE_SENT_AGAIN, &cs_cb_sndreq_retransmission);
    osip_set_message_callback(cell, OSIP_ICT_ACK_SENT, &cs_cb_sndack);
    osip_set_message_callback(cell, OSIP_ICT_ACK_SENT_AGAIN, &cs_cb_sndreq_retransmission);

    osip_set_message_callback(cell, OSIP_ICT_STATUS_1XX_RECEIVED, &cs_cb_ict_rcv1xx);
    osip_set_message_callback(cell, OSIP_ICT_STATUS_2XX_RECEIVED, &cs_cb_ict_rcv2xx);
    osip_set_message_callback(cell, OSIP_ICT_STATUS_2XX_RECEIVED_AGAIN, &cs_cb_rcvresp_retransmission);
    osip_set_message_callback(cell, OSIP_ICT_STATUS_3XX_RECEIVED, &cs_cb_ict_rcv3xx);
    osip_set_message_callback(cell, OSIP_ICT_STATUS_4XX_RECEIVED, &cs_cb_ict_rcv456xx);
    osip_set_message_callback(cell, OSIP_ICT_STATUS_5XX_RECEIVED, &cs_cb_ict_rcv456xx);
    osip_set_message_callback(cell, OSIP_ICT_STATUS_6XX_RECEIVED, &cs_cb_ict_rcv456xx);
    osip_set_message_callback(cell, OSIP_ICT_STATUS_3456XX_RECEIVED_AGAIN, &cs_cb_rcvresp_retransmission);

    osip_set_message_callback(cell, OSIP_IST_INVITE_RECEIVED, &cs_cb_rcvinvite);
    osip_set_message_callback(cell, OSIP_IST_INVITE_RECEIVED_AGAIN, &cs_cb_rcvreq_retransmission);
    osip_set_message_callback(cell, OSIP_IST_ACK_RECEIVED, &cs_cb_rcvack);
    osip_set_message_callback(cell, OSIP_IST_ACK_RECEIVED_AGAIN, &cs_cb_rcvreq_retransmission);

    osip_set_message_callback(cell, OSIP_IST_STATUS_1XX_SENT, &cs_cb_ist_snd1xx);
    osip_set_message_callback(cell, OSIP_IST_STATUS_2XX_SENT, &cs_cb_ist_snd2xx);
    osip_set_message_callback(cell, OSIP_IST_STATUS_2XX_SENT_AGAIN, &cs_cb_sndresp_retransmission);
    osip_set_message_callback(cell, OSIP_IST_STATUS_3XX_SENT, &cs_cb_ist_snd3456xx);
    osip_set_message_callback(cell, OSIP_IST_STATUS_4XX_SENT, &cs_cb_ist_snd3456xx);
    osip_set_message_callback(cell, OSIP_IST_STATUS_5XX_SENT, &cs_cb_ist_snd3456xx);
    osip_set_message_callback(cell, OSIP_IST_STATUS_6XX_SENT, &cs_cb_ist_snd3456xx);
    osip_set_message_callback(cell, OSIP_IST_STATUS_3456XX_SENT_AGAIN, &cs_cb_sndresp_retransmission);

    osip_set_message_callback(cell, OSIP_NICT_REGISTER_SENT, &cs_cb_sndregister);
    osip_set_message_callback(cell, OSIP_NICT_BYE_SENT, &cs_cb_sndbye);
    osip_set_message_callback(cell, OSIP_NICT_OPTIONS_SENT, &cs_cb_sndoptions);
    osip_set_message_callback(cell, OSIP_NICT_INFO_SENT, &cs_cb_sndinfo);
    osip_set_message_callback(cell, OSIP_NICT_CANCEL_SENT, &cs_cb_sndcancel);
    osip_set_message_callback(cell, OSIP_NICT_NOTIFY_SENT, &cs_cb_sndoptions);
    osip_set_message_callback(cell, OSIP_NICT_SUBSCRIBE_SENT, &cs_cb_sndoptions);
    osip_set_message_callback(cell, OSIP_NICT_UNKNOWN_REQUEST_SENT, &cs_cb_sndunkrequest);
    osip_set_message_callback(cell, OSIP_NICT_REQUEST_SENT_AGAIN, &cs_cb_sndreq_retransmission);

    osip_set_message_callback(cell, OSIP_NICT_STATUS_1XX_RECEIVED, &cs_cb_nict_rcv1xx);
    osip_set_message_callback(cell, OSIP_NICT_STATUS_2XX_RECEIVED, &cs_cb_nict_rcv2xx);
    osip_set_message_callback(cell, OSIP_NICT_STATUS_2XX_RECEIVED_AGAIN, &cs_cb_rcvresp_retransmission);
    osip_set_message_callback(cell, OSIP_NICT_STATUS_3XX_RECEIVED, &cs_cb_nict_rcv3xx);
    osip_set_message_callback(cell, OSIP_NICT_STATUS_4XX_RECEIVED, &cs_cb_nict_rcv456xx);
    osip_set_message_callback(cell, OSIP_NICT_STATUS_5XX_RECEIVED, &cs_cb_nict_rcv456xx);
    osip_set_message_callback(cell, OSIP_NICT_STATUS_6XX_RECEIVED, &cs_cb_nict_rcv456xx);
    osip_set_message_callback(cell, OSIP_NICT_STATUS_3456XX_RECEIVED_AGAIN, &cs_cb_rcvresp_retransmission);

    osip_set_message_callback(cell, OSIP_NIST_REGISTER_RECEIVED, &cs_cb_rcvregister);
    osip_set_message_callback(cell, OSIP_NIST_BYE_RECEIVED, &cs_cb_rcvbye);
    osip_set_message_callback(cell, OSIP_NIST_OPTIONS_RECEIVED, &cs_cb_rcvoptions);
    osip_set_message_callback(cell, OSIP_NIST_INFO_RECEIVED, &cs_cb_rcvinfo);
    osip_set_message_callback(cell, OSIP_NIST_CANCEL_RECEIVED, &cs_cb_rcvcancel);
    osip_set_message_callback(cell, OSIP_NIST_NOTIFY_RECEIVED, &cs_cb_rcvnotify);
    osip_set_message_callback(cell, OSIP_NIST_SUBSCRIBE_RECEIVED, &cs_cb_rcvsubscribe);
    osip_set_message_callback(cell, OSIP_NIST_MESSAGE_RECEIVED, &cs_cb_rcvmessage);
    osip_set_message_callback(cell, OSIP_NIST_UPDATE_RECEIVED, &cs_cb_rcvupdate);
    osip_set_message_callback(cell, OSIP_NIST_REFER_RECEIVED, &cs_cb_rcvrefer);
    osip_set_message_callback(cell, OSIP_NIST_UNKNOWN_REQUEST_RECEIVED, &cs_cb_rcvunkrequest);
    osip_set_message_callback(cell, OSIP_NIST_REQUEST_RECEIVED_AGAIN, &cs_cb_rcvreq_retransmission);

    osip_set_message_callback(cell, OSIP_NIST_STATUS_1XX_SENT, &cs_cb_nist_snd1xx);
    osip_set_message_callback(cell, OSIP_NIST_STATUS_2XX_SENT, &cs_cb_nist_snd2xx);
    osip_set_message_callback(cell, OSIP_NIST_STATUS_2XX_SENT_AGAIN, &cs_cb_nist_snd2xx);
    osip_set_message_callback(cell, OSIP_NIST_STATUS_3XX_SENT, &cs_cb_nist_snd3456xx);
    osip_set_message_callback(cell, OSIP_NIST_STATUS_4XX_SENT, &cs_cb_nist_snd3456xx);
    osip_set_message_callback(cell, OSIP_NIST_STATUS_5XX_SENT, &cs_cb_nist_snd3456xx);
    osip_set_message_callback(cell, OSIP_NIST_STATUS_6XX_SENT, &cs_cb_nist_snd3456xx);
    osip_set_message_callback(cell, OSIP_NIST_STATUS_3456XX_SENT_AGAIN, &cs_cb_sndresp_retransmission);

    osip_set_message_callback(cell, OSIP_ICT_STATUS_TIMEOUT, &cs_cb_ict_timeout);
    osip_set_message_callback(cell, OSIP_NICT_STATUS_TIMEOUT, &cs_cb_nict_timeout);

    return 0;
}
#endif

#if DECS("�ϲ�Ӧ�ûص�����")
/*****************************************************************************
 �� �� ��  : app_callback_init
 ��������  : Ӧ�ò�ص������ṹ��ʼ��
 �������  : app_callback_t** appcallback
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��1��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int app_callback_init()
{
    g_AppCallback = (app_callback_t*) osip_malloc(sizeof(app_callback_t));

    if (g_AppCallback == NULL)
    {
        return -1;
    }

    g_AppCallback->uas_register_received_cb = NULL;
    g_AppCallback->uas_register_received_timeout_cb = NULL;
    g_AppCallback->uac_register_response_received_cb = NULL;
    g_AppCallback->uac_register_response_received_cb_user_data = 0;
    g_AppCallback->invite_received_cb = NULL;
    g_AppCallback->invite_received_cb_user_data = 0;
    g_AppCallback->invite_response_received_cb = NULL;
    g_AppCallback->invite_response_received_cb_user_data = 0;
    g_AppCallback->cancel_received_cb = NULL;
    g_AppCallback->ack_received_cb = NULL;
    g_AppCallback->ack_received_cb_user_data = 0;
    g_AppCallback->bye_received_cb = NULL;
    g_AppCallback->bye_received_cb_user_data = 0;
    g_AppCallback->bye_response_received_cb = NULL;
    g_AppCallback->bye_response_received_cb_user_data = 0;
    g_AppCallback->message_received_cb = NULL;
    g_AppCallback->message_received_cb_user_data = 0;
    g_AppCallback->message_response_received_cb = NULL;
    g_AppCallback->message_response_received_cb_user_data = 0;
    g_AppCallback->info_received_cb = NULL;
    g_AppCallback->info_received_cb_user_data = 0;
    g_AppCallback->info_response_received_cb = NULL;
    g_AppCallback->info_response_received_cb_user_data = 0;
    g_AppCallback->ua_session_expires_cb = NULL;
    g_AppCallback->dbg_printf_cb = NULL;
    g_AppCallback->sip_message_trace_cb = NULL;

    return 0;
}

/*****************************************************************************
 �� �� ��  : app_callback_free
 ��������  : Ӧ�ò�ص������ṹ�ͷ�
 �������  : app_callback_t* appcallback
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��1��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void app_callback_free()
{
    if (g_AppCallback == NULL)
    {
        return;
    }

    g_AppCallback->uas_register_received_cb = NULL;
    g_AppCallback->uas_register_received_timeout_cb = NULL;
    g_AppCallback->uac_register_response_received_cb = NULL;
    g_AppCallback->uac_register_response_received_cb_user_data = 0;
    g_AppCallback->invite_received_cb = NULL;
    g_AppCallback->invite_received_cb_user_data = 0;
    g_AppCallback->invite_response_received_cb = NULL;
    g_AppCallback->invite_response_received_cb_user_data = 0;
    g_AppCallback->cancel_received_cb = NULL;
    g_AppCallback->ack_received_cb = NULL;
    g_AppCallback->ack_received_cb_user_data = 0;
    g_AppCallback->bye_received_cb = NULL;
    g_AppCallback->bye_received_cb_user_data = 0;
    g_AppCallback->bye_response_received_cb = NULL;
    g_AppCallback->bye_response_received_cb_user_data = 0;
    g_AppCallback->message_received_cb = NULL;
    g_AppCallback->message_received_cb_user_data = 0;
    g_AppCallback->message_response_received_cb = NULL;
    g_AppCallback->message_response_received_cb_user_data = 0;
    g_AppCallback->info_received_cb = NULL;
    g_AppCallback->info_received_cb_user_data = 0;
    g_AppCallback->info_response_received_cb = NULL;
    g_AppCallback->info_response_received_cb_user_data = 0;
    g_AppCallback->ua_session_expires_cb = NULL;
    g_AppCallback->dbg_printf_cb = NULL;
    g_AppCallback->sip_message_trace_cb = NULL;

    osip_free(g_AppCallback);
    g_AppCallback = NULL;
    return;
}

/*****************************************************************************
 �� �� ��  : app_set_uas_register_received_cb
 ��������  : ������յ�ע����Ϣ�ص���������
 �������  : int (*cb) (char*, char*, char*, int, char*, int, int, int)
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��1��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void app_set_uas_register_received_cb(int (*cb)(char*, char*, char*, int, char*, int, int, int))
{
    if ((NULL == cb) || (NULL == g_AppCallback))
    {
        return;
    }

    g_AppCallback->uas_register_received_cb = cb;
    return;
}

/*****************************************************************************
 �� �� ��  : app_set_uas_register_received_timeout_cb
 ��������  : ���÷�������û���յ��ͻ���ˢ��ע�ᳬʱ֪ͨ����
 �������  : int (*cb) (char*, char*, char*, int, int)
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��17��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void app_set_uas_register_received_timeout_cb(int (*cb)(char*, char*, char*, int, int))
{
    if ((NULL == cb) || (NULL == g_AppCallback))
    {
        return;
    }

    g_AppCallback->uas_register_received_timeout_cb = cb;
    return;
}

/*****************************************************************************
 �� �� ��  : app_set_uac_register_response_received_cb
 ��������  : �ͻ��˷���ע����Ϣ���յ�ע����Ӧ��Ϣ�ص���������
 �������  : int (*cb) (int, int, int, char*, unsigned int, int)
             int user_data
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��1��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void app_set_uac_register_response_received_cb(int (*cb)(int, int, int, char*, unsigned int, int), int user_data)
{
    if ((NULL == cb) || (NULL == g_AppCallback))
    {
        return;
    }

    g_AppCallback->uac_register_response_received_cb = cb;
    g_AppCallback->uac_register_response_received_cb_user_data = user_data;
    return;
}

/*****************************************************************************
 �� �� ��  : app_set_invite_received_cb
 ��������  : �յ�������Ϣ�ص�����
 �������  : int (*cb) (char*, char*, char*, int, int)
                            int user_data
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��1��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void app_set_invite_received_cb(int (*cb)(char*, char*, char*, int, char*, int, int), int user_data)
{
    if ((NULL == cb) || (NULL == g_AppCallback))
    {
        return;
    }

    g_AppCallback->invite_received_cb = cb;
    g_AppCallback->invite_received_cb_user_data = user_data;
    return;
}

/*****************************************************************************
 �� �� ��  : app_set_invite_response_received_cb
 ��������  : �յ�������Ӧ��Ϣ�ص�����
 �������  : int (*cb) (char*, char*, char*, int, int, char*, char*, int, int)
                            int user_data
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��1��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void app_set_invite_response_received_cb(int (*cb)(char*, char*, char*, int, int, char*, char*, int, int), int user_data)
{
    if ((NULL == cb) || (NULL == g_AppCallback))
    {
        return;
    }

    g_AppCallback->invite_response_received_cb = cb;
    g_AppCallback->invite_response_received_cb_user_data = user_data;
    return;
}

/*****************************************************************************
 �� �� ��  : app_set_cancel_received_cb
 ��������  : �յ�Cancel ��Ϣ�ص�����
 �������  : int (*cb)(char*
                            char*
                            char*
                            int
                            int)
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
void app_set_cancel_received_cb(int (*cb)(char*, char*, char*, int, int), int user_data)
{
    if ((NULL == cb) || (NULL == g_AppCallback))
    {
        return;
    }

    g_AppCallback->cancel_received_cb = cb;
    g_AppCallback->cancel_received_cb_user_data = user_data;
    return;
}

/*****************************************************************************
 �� �� ��  : app_set_ack_received_cb
 ��������  : �յ�ACK ��Ϣ�ص�����
 �������  : int (*cb) (char*, char*, char*, int,int)
                            int user_data
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��1��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void app_set_ack_received_cb(int (*cb)(char*, char*, char*, int, int), int user_data)
{
    if ((NULL == cb) || (NULL == g_AppCallback))
    {
        return;
    }

    g_AppCallback->ack_received_cb = cb;
    g_AppCallback->ack_received_cb_user_data = user_data;
    return;
}

/*****************************************************************************
 �� �� ��  : app_set_bye_received_cb
 ��������  : �յ����н�����Ϣ�ص�����
 �������  : int (*cb) (char*, char*, char*, int,int)
                            int user_data
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��1��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void app_set_bye_received_cb(int (*cb)(char*, char*, char*, int, int), int user_data)
{
    if ((NULL == cb) || (NULL == g_AppCallback))
    {
        return;
    }

    g_AppCallback->bye_received_cb = cb;
    g_AppCallback->bye_received_cb_user_data = user_data;
    return;
}

/*****************************************************************************
 �� �� ��  : app_set_bye_response_received_cb
 ��������  : �յ����н�����Ӧ��Ϣ�ص�����
 �������  : int (*cb) (char*, char*, int, int,int)
                            int user_data
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��1��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void app_set_bye_response_received_cb(int (*cb)(char*, char*, char*, int, int, int), int user_data)
{
    if ((NULL == cb) || (NULL == g_AppCallback))
    {
        return;
    }

    g_AppCallback->bye_response_received_cb = cb;
    g_AppCallback->bye_response_received_cb_user_data = user_data;
    return;
}

/*****************************************************************************
 �� �� ��  : app_set_message_received_cb
 ��������  : �յ�Message ��Ϣ�ص�����
 �������  : int (*cb) (char*
                            char*
                            char*
                            char*
                            int
                            int)
                            int user_data
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��1��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void app_set_message_received_cb(int (*cb)(char*, char*, int, char*, char*, int, char*, int, char*, int, int), int user_data)
{
    if ((NULL == cb) || (NULL == g_AppCallback))
    {
        return;
    }

    g_AppCallback->message_received_cb = cb;
    g_AppCallback->message_received_cb_user_data = user_data;
    return;
}

/*****************************************************************************
 �� �� ��  : app_set_message_response_received_cb
 ��������  : �յ�Message ��Ӧ��Ϣ�ص�����
 �������  : int (*cb) (char*
                            char*
                            char*
                            int
                            int)
                            int user_data
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��1��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void app_set_message_response_received_cb(int (*cb)(char*, char*, char*, int, int), int user_data)
{
    if ((NULL == cb) || (NULL == g_AppCallback))
    {
        return;
    }

    g_AppCallback->message_response_received_cb = cb;
    g_AppCallback->message_response_received_cb_user_data = user_data;
    return;
}

/*****************************************************************************
 �� �� ��  : app_set_subscribe_received_cb
 ��������  : �յ�Subscribe ��Ϣ�ص�����
 �������  : int (*cb) (char*
                            char*
                            char*
                            char*
                            char*
                            int,
                            char*
                            int
                            int)
                            int user_data
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��1��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void app_set_subscribe_received_cb(int (*cb)(char*, char*, int, char*, char*, int, char*, char*, char*, int, char*, int, int), int user_data)
{
    if ((NULL == cb) || (NULL == g_AppCallback))
    {
        return;
    }

    g_AppCallback->subscribe_received_cb = cb;
    g_AppCallback->subscribe_received_cb_user_data = user_data;
    return;
}

/*****************************************************************************
 �� �� ��  : app_set_subscribe_response_received_cb
 ��������  : �յ�Subscribe ��Ӧ��Ϣ�ص�����
 �������  : int (*cb) (char*
                            char*
                            char*
                            int
                            int)
                            int user_data
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��1��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void app_set_subscribe_response_received_cb(int (*cb)(char*, char*, char*, int, int, int), int user_data)
{
    if ((NULL == cb) || (NULL == g_AppCallback))
    {
        return;
    }

    g_AppCallback->subscribe_response_received_cb = cb;
    g_AppCallback->subscribe_response_received_cb_user_data = user_data;
    return;
}

void app_set_subscribe_within_dialog_received_cb(int (*cb)(char*, char*, int, char*, char*, int, int, char*, int))
{
    if ((NULL == cb) || (NULL == g_AppCallback))
    {
        return;
    }

    g_AppCallback->subscribe_within_dialog_received_cb = cb;
    return;
}

void app_set_subscribe_within_dialog_response_received_cb(int (*cb)(char*, char*, char*, int, int, int))
{
    if ((NULL == cb) || (NULL == g_AppCallback))
    {
        return;
    }

    g_AppCallback->subscribe_within_dialog_response_received_cb = cb;
    return;
}

/*****************************************************************************
 �� �� ��  : app_set_notify_received_cb
 ��������  : �յ� Notify ��Ϣ�ص�����
 �������  : int (*cb) (char*
                            char*
                            char*
                            char*
                            int
                            int)
                            int user_data
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��1��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void app_set_notify_received_cb(int (*cb)(char*, char*, int, char*, char*, int, char*, char*, int, int), int user_data)
{
    if ((NULL == cb) || (NULL == g_AppCallback))
    {
        return;
    }

    g_AppCallback->notify_received_cb = cb;
    g_AppCallback->notify_received_cb_user_data = user_data;
    return;
}

/*****************************************************************************
 �� �� ��  : app_set_notify_response_received_cb
 ��������  : �յ� Notify ��Ӧ��Ϣ�ص�����
 �������  : int (*cb) (char*
                            char*
                            char*
                            int
                            int)
                            int user_data
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��1��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void app_set_notify_response_received_cb(int (*cb)(char*, char*, char*, int, int), int user_data)
{
    if ((NULL == cb) || (NULL == g_AppCallback))
    {
        return;
    }

    g_AppCallback->notify_response_received_cb = cb;
    g_AppCallback->notify_response_received_cb_user_data = user_data;
    return;
}

/*****************************************************************************
 �� �� ��  : app_set_info_received_cb
 ��������  : �յ�Info ��Ϣ�ص�����
 �������  : int (*cb) (char*
                            char*
                            char*
                            int
                            char*
                            int
                            int)
                            int user_data
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��1��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void app_set_info_received_cb(int (*cb)(char*, char*, int, char*, char*, int, char*, int, int), int user_data)
{
    if ((NULL == cb) || (NULL == g_AppCallback))
    {
        return;
    }

    g_AppCallback->info_received_cb = cb;
    g_AppCallback->info_received_cb_user_data = user_data;
    return;
}

/*****************************************************************************
 �� �� ��  : app_set_info_response_received_cb
 ��������  : �յ�Info ��Ӧ��Ϣ�ص�����
 �������  : int (*cb) (char*
                            char*
                            char*
                            int
                            int)
                            int user_data
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��1��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void app_set_info_response_received_cb(int (*cb)(char*, char*, char*, int, int), int user_data)
{
    if ((NULL == cb) || (NULL == g_AppCallback))
    {
        return;
    }

    g_AppCallback->info_response_received_cb = cb;
    g_AppCallback->info_response_received_cb_user_data = user_data;
    return;
}

/*****************************************************************************
 �� �� ��  : app_set_ua_session_expires_cb
 ��������  : UA�Ự��ʱ�ص�����
 �������  : int (*cb)(int)
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��9��3�� ���ڶ�
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void app_set_ua_session_expires_cb(int (*cb)(int))
{
    if ((NULL == cb) || (NULL == g_AppCallback))
    {
        return;
    }

    g_AppCallback->ua_session_expires_cb = cb;
    return;
}

/*****************************************************************************
 �� �� ��  : app_set_dbg_printf_cb
 ��������  : debug���Դ�ӡ��������
 �������  : void (*cb) (int, char*, int, const char*, ...)
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��1��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void app_set_dbg_printf_cb(void (*cb)(int, const char*, const char*, int, const char*))
{
    if ((NULL == cb) || (NULL == g_AppCallback))
    {
        return;
    }

    g_AppCallback->dbg_printf_cb = cb;
    return;
}

/*****************************************************************************
 �� �� ��  : app_set_sip_message_trace_cb
 ��������  : SIP��Ϣ���ٵ��Իص���������
 �������  : void (*cb)(int
                             int
                             char*
                             int
                             char*)
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��9��4�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void app_set_sip_message_trace_cb(void (*cb)(int, int, char*, int, char*))
{
    if ((NULL == cb) || (NULL == g_AppCallback))
    {
        return;
    }

    g_AppCallback->sip_message_trace_cb = cb;
    return;
}
#endif
