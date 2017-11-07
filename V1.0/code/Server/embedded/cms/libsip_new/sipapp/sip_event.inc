/******************************************************************************

                  版权所有 (C), 2001-2013, 金智视讯技术有限公司

 ******************************************************************************
  文 件 名   : sip_event.h
  版 本 号   : 初稿
  作    者   : yanghaifeng
  生成日期   : 2013年4月1日
  最近修改   :
  功能描述   : sip_event.c 的头文件
  函数列表   :
  修改历史   :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 创建文件

******************************************************************************/
#ifndef __SIP_EVENT_H__
#define __SIP_EVENT_H__
/*----------------------------------------------*
 * 包含头文件                                   *
 *----------------------------------------------*/
#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

    /*----------------------------------------------*
     * 外部变量说明                                 *
     *----------------------------------------------*/

    /*----------------------------------------------*
     * 外部函数原型说明                             *
     *----------------------------------------------*/

    /*----------------------------------------------*
     * 内部函数原型说明                             *
     *----------------------------------------------*/

    /*----------------------------------------------*
     * 全局变量                                     *
     *----------------------------------------------*/

    /*----------------------------------------------*
     * 模块级变量                                   *
     *----------------------------------------------*/

    /*----------------------------------------------*
     * 常量定义                                     *
     *----------------------------------------------*/

    /*----------------------------------------------*
     * 宏定义                                       *
     *----------------------------------------------*/
    extern int OnIctTransportError(osip_transaction_t* tr);
    extern int OnIstTransportError(osip_transaction_t* tr);
    extern int OnKillIctTransactionForRecv(osip_transaction_t* tr);
    extern int OnKillIctTransactionForRecvMsg(osip_transaction_t* tr);
    extern int OnKillIctTransactionForRecvRegister(osip_transaction_t* tr);
    extern int OnKillIctTransactionForSend(osip_transaction_t* tr);
    extern int OnKillIctTransactionForSendMsg(osip_transaction_t* tr);
    extern int OnKillIstTransactionForRecv(osip_transaction_t* tr);
    extern int OnKillIstTransactionForRecvMsg(osip_transaction_t* tr);
    extern int OnKillIstTransactionForRecvRegister(osip_transaction_t* tr);
    extern int OnKillIstTransactionForSend(osip_transaction_t* tr);
    extern int OnKillIstTransactionForSendMsg(osip_transaction_t* tr);
    extern int OnKillNictTransactionForRecv(osip_transaction_t* tr);
    extern int OnKillNictTransactionForRecvMsg(osip_transaction_t* tr);
    extern int OnKillNictTransactionForRecvRegister(osip_transaction_t* tr);
    extern int OnKillNictTransactionForSend(osip_transaction_t* tr);
    extern int OnKillNictTransactionForSendMsg(osip_transaction_t* tr);
    extern int OnKillNistTransactionForRecv(osip_transaction_t* tr);
    extern int OnKillNistTransactionForRecvMsg(osip_transaction_t* tr);
    extern int OnKillNistTransactionForRecvRegister(osip_transaction_t* tr);
    extern int OnKillNistTransactionForSend(osip_transaction_t* tr);
    extern int OnKillNistTransactionForSendMsg(osip_transaction_t* tr);
    extern int OnNictTransportError(osip_transaction_t* tr);
    extern int OnNistTransportError(osip_transaction_t* tr);
    extern int OnRcv1xxForInvite(osip_transaction_t* tr, osip_message_t* sip);
    extern int OnRcv1xxForRequest(osip_transaction_t* tr, osip_message_t* sip);
    extern int OnRcv2xxForBye(osip_transaction_t* tr, osip_message_t* sip);
    extern int OnRcv2xxForCancel(osip_transaction_t* tr, osip_message_t* sip);
    extern int OnRcv2xxForInfo(osip_transaction_t* tr, osip_message_t* sip);
    extern int OnRcv2xxForInvite(osip_transaction_t* tr, osip_message_t* sip);
    extern int OnRcv2xxForMessage(osip_transaction_t* tr, osip_message_t* sip);
    extern int OnRcv2xxForNotify(osip_transaction_t* tr, osip_message_t* sip);
    extern int OnRcv2xxForOptions(osip_transaction_t* tr, osip_message_t* sip);
    extern int OnRcv2xxForRefer(osip_transaction_t* tr, osip_message_t* sip);
    extern int OnRcv2xxForRegister(osip_transaction_t* tr, osip_message_t* sip);
    extern void OnRcv2xxForSubscribe(osip_transaction_t* tr, osip_message_t* sip);
    extern int OnRcv2xxForUpdate(osip_transaction_t* tr, osip_message_t* sip);
    extern int OnRcv3xxForBye(osip_transaction_t* tr, osip_message_t* sip);
    extern int OnRcv3xxForCancel(osip_transaction_t* tr, osip_message_t* sip);
    extern int OnRcv3xxForInfo(osip_transaction_t* tr, osip_message_t* sip);
    extern int OnRcv3xxForInvite(osip_transaction_t* tr, osip_message_t* sip);
    extern int OnRcv3xxForNotify(osip_transaction_t* tr, osip_message_t* sip);
    extern int OnRcv3xxForOptions(osip_transaction_t* tr, osip_message_t* sip);
    extern int OnRcv3xxForRefer(osip_transaction_t* tr, osip_message_t* sip);
    extern int OnRcv3xxForRegister(osip_transaction_t* tr, osip_message_t* sip);
    extern void OnRcv3xxForSubscribe(osip_transaction_t* tr, osip_message_t* sip);
    extern int OnRcv3xxForUpdate(osip_transaction_t* tr, osip_message_t* sip);
    extern int OnRcv408ForNotify(osip_transaction_t* tr, osip_message_t* sip);
    extern void OnRcv408ForSubscribe(osip_transaction_t* tr, osip_message_t* sip);
    extern int OnRcv456xxForBye(osip_transaction_t* tr, osip_message_t* sip);
    extern int OnRcv456xxForCancel(osip_transaction_t* tr, osip_message_t* sip);
    extern int OnRcv456xxForInfo(osip_transaction_t* tr, osip_message_t* sip);
    extern int OnRcv456xxForInvite(osip_transaction_t* tr, osip_message_t* sip);
    extern int OnRcv456xxForMessage(osip_transaction_t* tr, osip_message_t* sip);
    extern int OnRcv456xxForNotify(osip_transaction_t* tr, osip_message_t* sip);
    extern int OnRcv456xxForOptions(osip_transaction_t* tr, osip_message_t* sip);
    extern int OnRcv456xxForRefer(osip_transaction_t* tr, osip_message_t* sip);
    extern int OnRcv456xxForRegister(osip_transaction_t* tr, osip_message_t* sip);
    extern void OnRcv456xxForSubscribe(osip_transaction_t* tr, osip_message_t* sip);
    extern int OnRcv456xxForUpdate(osip_transaction_t* tr, osip_message_t* sip);
    extern int OnRcvAck(osip_transaction_t* tr, osip_message_t* sip);
    extern int OnRcvBye(osip_transaction_t* tr, osip_message_t* sip);
    extern int OnRcvCancel(osip_transaction_t* tr, osip_message_t* sip);
    extern int OnRcvIct2xx2(osip_message_t* sip);
    extern int OnRcvInfo(osip_transaction_t* tr, osip_message_t* sip);
    extern int OnRcvInvite(osip_transaction_t* tr, osip_message_t* sip);
    extern int OnRcvIstAckfor2xx(osip_message_t* sip);
    extern int OnRcvMessage(osip_transaction_t* tr, osip_message_t* sip);
    extern int OnRcvNotify(osip_transaction_t* tr, osip_message_t* sip);
    extern int OnRcvOptions(osip_transaction_t* tr, osip_message_t* sip);
    extern int OnRcvRefer(osip_transaction_t* tr, osip_message_t* sip);
    extern int OnRcvRegister(osip_transaction_t* tr, osip_message_t* sip);
    extern int OnRcvSubscribe(osip_transaction_t* tr, osip_message_t* sip);
    extern int OnRcvUnknown(osip_transaction_t* tr, osip_message_t* sip);
    extern int OnRcvUpdate(osip_transaction_t* tr, osip_message_t* sip);
    extern int OnSend1xx(osip_transaction_t* tr, osip_message_t* sip);
    extern int OnSend2xx(osip_transaction_t* tr, osip_message_t* sip);
    extern int OnSend3456xx(osip_transaction_t* tr, osip_message_t* sip);
    extern int OnSendInvite(osip_transaction_t* tr, osip_message_t* sip);
    extern int OnSendRequest(osip_transaction_t* tr, osip_message_t* sip);
    extern int OnTimeoutBye(int index);
    extern int OnTimeoutCancel(int index);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif /* __SIP_EVENT_H__ */
