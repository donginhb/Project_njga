/******************************************************************************

                  ��Ȩ���� (C), 2001-2013, ������Ѷ�������޹�˾

 ******************************************************************************
  �� �� ��   : EV9000_DBDef.h
  �� �� ��   : ����
  ��    ��   : yanghaifeng
  ��������   : 2013��7��2�� ���ڶ�
  ����޸�   :
  ��������   : EV9000ϵͳ���������붨��
  �����б�   :
  �޸���ʷ   :
  1.��    ��   : 2013��7��2�� ���ڶ�
    ��    ��   : yanghaifeng
    �޸�����   : �����ļ�

******************************************************************************/
#ifndef EV9000_ERR_CODE_DEF_H
#define EV9000_ERR_CODE_DEF_H

/*----------------------------------------------*
 * ����ͷ�ļ�                                   *
 *----------------------------------------------*/

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

/*----------------------------------------------*
 * ��������                                     *
 *----------------------------------------------*/

/*----------------------------------------------*
 * �궨��                                       *
 *----------------------------------------------*/
#define EV9000_SUCCESS                    0

/* 0X F100 1XXX Ӳ�������硢�汾�������� */
/* 0x F100 FXXX   ������BSP */

/* 0X F2XX XXXX	CMS */
/* ͨ�� */
#define EV9000_CMS_ERR_LOCAL_CMS_ID_ERROR                                               0XF2000001 /* ����CMS IDû������ */
#define EV9000_CMS_ERR_GET_LOCALIP_ERROR                                                0XF2000002 /* ��ȡ����IP��ַ���� */
#define EV9000_CMS_ERR_GET_LOCALPORT_ERROR                                              0XF2000003 /* ��ȡ���ض˿ڴ��� */
#define EV9000_CMS_ERR_NO_DEAL_THREAD_ERROR                                             0XF2000004 /* �Ҳ�����Ӧ�Ĵ����߳� */
#define EV9000_CMS_ERR_PARAM_ERROR                                                      0XF2000005 /* �������� */
#define EV9000_CMS_ERR_PAY_ERROR                                                        0XF2000006 /* û�и��� */
#define EV9000_CMS_ERR_SYSTEM_ERROR                                                     0XF2000007 /* ϵͳ���� */
#define EV9000_CMS_ERR_SYSTEM_LICENSE_ERROR                                             0XF2000008 /* ϵͳû��License��Ȩ */

/* XML */
#define EV9000_CMS_ERR_XML_BUILD_TREE_ERROR                                             0XF2010001 /* ����XML ��ʧ�� */
#define EV9000_CMS_ERR_XML_GET_NODE_ERROR                                               0XF2010002 /* ��ȡXML Nodeʧ�� */
#define EV9000_CMS_ERR_XML_GET_MSG_TYPE_ERROR                                           0XF2010003 /* ��ȡXML ��Ϣ����ʧ�� */

/* SDP */
#define EV9000_CMS_ERR_SDP_MSG_INIT_ERROR                                               0XF2020001 /* SDP��ʼ��ʧ�� */
#define EV9000_CMS_ERR_SDP_MSG_PARSE_ERROR                                              0XF2020002 /* SDP����ʧ�� */
#define EV9000_CMS_ERR_SDP_GET_VIDEO_INFO_ERROR                                         0XF2020003 /* ��ȡSDP��Ƶ����ʧ�� */
#define EV9000_CMS_ERR_SDP_NOT_SUPPORT_S_TYPE_ERROR                                     0XF2020004 /* ��֧�ֵ�S SDP��Ϣ���� */
#define EV9000_CMS_ERR_SDP_MODIFY_S_NAME_ERROR                                          0XF2020005 /* �޸�SDP��ϢS�ֶ�����ʧ�� */
#define EV9000_CMS_ERR_SDP_MODIFY_IP_ERROR                                              0XF2020006 /* �޸�SDP��ϢIP��ַ�ֶ�����ʧ�� */
#define EV9000_CMS_ERR_SDP_MODIFY_PROTOCOL_ERROR                                        0XF2020007 /* �޸�SDP��ϢЭ���ֶ�����ʧ�� */
#define EV9000_CMS_ERR_SDP_GENERAL_MSG_ERROR                                            0XF2020008 /* ����SDPӦ����Ϣʧ�� */

/* ���ݿ� */
#define EV9000_CMS_ERR_DB_OPER_ERROR                                                    0XF2030001 /* ���ݿ��ѯʧ�� */
#define EV9000_CMS_ERR_DB_NORECORD_ERROR                                                0XF2030002 /* ���ݿ�û�в�ѯ����¼ */
#define EV9000_CMS_ERR_DB_USER_NOTENABLE_ERROR                                          0XF2030003 /* ���ݿ��û�û������ */
#define EV9000_CMS_ERR_DB_USERID_EMPTY_ERROR                                            0XF2030004 /* ���ݿ��û�IDΪ�� */

/* �û� */
#define EV9000_CMS_ERR_USER_FIND_USER_INFO_ERROR                                        0XF2040001 /* �����û�ʧ�� */
#define EV9000_CMS_ERR_USER_GET_USER_INFO_ERROR                                         0XF2040002 /* ��ȡ�û�ʧ�� */
#define EV9000_CMS_ERR_USER_CREAT_USER_INFO_ERROR                                       0XF2040003 /* �����û�ʧ�� */
#define EV9000_CMS_ERR_USER_NOT_ENABLE_ERROR                                            0XF2040004 /* �û�δ���� */
#define EV9000_CMS_ERR_USER_OFFLINE_ERROR                                               0XF2040005 /* �û������� */

/* �����豸 */
#define EV9000_CMS_ERR_DEVICE_FIND_DEVICE_INFO_ERROR                                    0XF2050001 /* ���������豸ʧ�� */
#define EV9000_CMS_ERR_DEVICE_GET_DEVICE_INFO_ERROR                                     0XF2050002 /* ��ȡ�����豸ʧ�� */
#define EV9000_CMS_ERR_DEVICE_CREAT_DEVICE_INFO_ERROR                                   0XF2050003 /* ���������豸ʧ�� */
#define EV9000_CMS_ERR_DEVICE_NOT_ENABLE_ERROR                                          0XF2050004 /* �����豸δ���� */
#define EV9000_CMS_ERR_DEVICE_OFFLINE_ERROR                                             0XF2050005 /* �����豸������ */

/* �߼��豸 */
#define EV9000_CMS_ERR_DEVICE_FIND_LOGIC_DEVICE_INFO_ERROR                              0XF2060001 /* �����߼��豸ʧ�� */
#define EV9000_CMS_ERR_DEVICE_GET_LOGIC_DEVICE_INFO_ERROR                               0XF2060002 /* ��ȡ�߼��豸ʧ�� */
#define EV9000_CMS_ERR_DEVICE_CREAT_LOGIC_DEVICE_INFO_ERROR                             0XF2060003 /* �����߼��豸ʧ�� */
#define EV9000_CMS_ERR_DEVICE_LOGIC_DEVICE_NOT_ENABLE_ERROR                             0XF2060004 /* �߼��豸δ���� */
#define EV9000_CMS_ERR_DEVICE_LOGIC_DEVICE_OFFLINE_ERROR                                0XF2060005 /* �߼��豸������ */
#define EV9000_CMS_ERR_DEVICE_LOGIC_DEVICE_UNREACHED_ERROR                              0XF2060006 /* �߼��豸���ɴ� */

/* ����·�� */
#define EV9000_CMS_ERR_ROUTE_FIND_ROUTE_INFO_ERROR                                      0XF2070001 /* ����·����Ϣ���� */
#define EV9000_CMS_ERR_ROUTE_GET_ROUTE_INFO_ERROR                                       0XF2070002 /* ��ȡ·����Ϣ���� */
#define EV9000_CMS_ERR_ROUTE_CREAT_ROUTE_INFO_ERROR                                     0XF2070003 /* ����·����Ϣʧ�� */
#define EV9000_CMS_ERR_ROUTE_NOT_ENABLE_ERROR                                           0XF2070004 /* ·����Ϣδ���� */

/* TSU */
#define EV9000_CMS_ERR_TSU_GET_IDLE_TSU_INDEX_ERROR                                     0XF2080001 /* ��ȡTSU ����ʧ�� */
#define EV9000_CMS_ERR_TSU_GET_TSU_INFO_ERROR                                           0XF2080002 /* ��ȡTSU ��Ϣʧ�� */
#define EV9000_CMS_ERR_TSU_GET_IP_ERROR                                                 0XF2080003 /* ��ȡTSU IP��ַʧ�� */
#define EV9000_CMS_ERR_TSU_GET_SEND_PORT_ERROR                                          0XF2080004 /* ��ȡTSU���Ͷ˿�ʧ�� */
#define EV9000_CMS_ERR_TSU_GET_RECV_PORT_ERROR                                          0XF2080005 /* ��ȡTSU���ն˿�ʧ�� */
#define EV9000_CMS_ERR_TSU_IP_CLONE_ERROR                                               0XF2080006 /* TSUIP��ַ����ʧ�� */
#define EV9000_CMS_ERR_TSU_GET_CALLER_TSU_IP_ERROR                                      0XF2080007 /* ��ȡ������Ƶҵ��ʹ�õ�TSU IP��ַʧ�� */
#define EV9000_CMS_ERR_TSU_NOTIFY_ADD_REPLAY_ERROR                                      0XF2080008 /* ֪ͨTSU��ӻط�����ʧ�� */
#define EV9000_CMS_ERR_TSU_NOTIFY_START_REPLAY_ERROR                                    0XF2080009 /* ֪ͨTSU��ʼ����¼���ļ�����ʧ�� */
#define EV9000_CMS_ERR_TSU_NOTIFY_ADD_TRANSFER_ERROR                                    0XF208000A /* ֪ͨTSU��ʼ��ת������ʧ�� */
#define EV9000_CMS_ERR_TSU_ICE_ERROR                                                    0XF208000B /* TSU ICEͨ���쳣 */

/* ע�� */
#define EV9000_CMS_ERR_REG_SERVER_ID_NOT_MATCH_ERROR                                    0XF2090001 /* ע���CMS ID�ͱ��ز�ƥ�� */
#define EV9000_CMS_ERR_REG_SERVER_IP_NOT_MATCH_ERROR                                    0XF2090002 /* ע���IP��ַ�ͱ��ز�ƥ�� */
#define EV9000_CMS_ERR_REG_DEVICE_TYPE_NOT_SUPPORT_ERROR                                0XF2090003 /* ע����豸���Ͳ�֧�� */
#define EV9000_CMS_ERR_REG_AUTH_REALM_NOT_LOCAL_ERROR                                   0XF2090004 /* ��֤�����Ǳ���CMS */
#define EV9000_CMS_ERR_REG_AUTH_FAILD_ERROR                                             0XF2090005 /* ��֤ʧ�� */
#define EV9000_CMS_ERR_REG_GET_SERVER_IP_ERROR                                          0XF2090006 /* ��ȡע��ķ�����IP��ַʧ�� */
#define EV9000_CMS_ERR_REG_GET_SERVER_PORT_ERROR                                        0XF2090007 /* ��ȡע��ķ������˿ڵ�ַʧ�� */
#define EV9000_CMS_ERR_REG_GET_SERVER_IP_ETHNAME_ERROR                                  0XF2090008 /* ��ȡע��ķ�����IP��ַ����������ʧ�� */
#define EV9000_CMS_ERR_REG_MSG_ERROR                                                    0XF2090009 /* ע����Ϣ���� */
#define EV9000_CMS_ERR_REG_EXPIRE_ERROR                                                 0XF209000A /* ע����Ϣ��ʱʱ����� */
#define EV9000_CMS_ERR_REG_CALLID_ERROR                                                 0XF209000B /* ע����ϢCallID�ֶδ��� */
#define EV9000_CMS_ERR_REG_ID_NOT_MATCH_ERROR                                           0XF209000C /* ע����豸ID�ͱ���CMS ID��ƥ�� */

/* �û�ע�� */
#define EV9000_CMS_ERR_USER_REG_ASSIGN_THREAD_ERROR                                     0XF209000B /* �����û�ҵ�����߳�ʧ�� */

/* �豸ע�� */
#define EV9000_CMS_ERR_DEVICE_REG_IP_CONFLICT_ERROR                                     0XF209000C /* �豸IP��ַ��ͻ */

/* ��Ƶ */
#define EV9000_CMS_ERR_INVITE_GET_IDLE_CR_DATA_ERROR                                    0XF20A0001 /* ��ȡ���еĺ�����Դʧ�� */
#define EV9000_CMS_ERR_INVITE_GET_CR_DATA_ERROR                                         0XF20A0002 /* ��ȡ������Դʧ�� */

#define EV9000_CMS_ERR_INVITE_CALLER_IP_ERROR                                           0XF20A0003 /* ��ȡ��Ƶ����IP��ַ���� */
#define EV9000_CMS_ERR_INVITE_CALLER_PORT_ERROR                                         0XF20A0004 /* ��ȡ��Ƶ���󷽶˿ڴ��� */

#define EV9000_CMS_ERR_INVITE_CALLER_USER_INFO_ERROR                                    0XF20A0005 /* ��ȡ��Ƶ�����û���Ϣ���� */
#define EV9000_CMS_ERR_INVITE_CALLER_DEVICE_INFO_ERROR                                  0XF20A0006 /* ��ȡ��Ƶ���������豸��Ϣ���� */
#define EV9000_CMS_ERR_INVITE_CALLER_ROUTE_INFO_ERROR                                   0XF20A0007 /* ��ȡ��Ƶ�����ϼ�·����Ϣ���� */

#define EV9000_CMS_ERR_INVITE_CALLER_MSG_BODY_ERROR                                     0XF20A0008 /* ��ȡ��Ƶ���󷽵�Invite��Ϣ����� */
#define EV9000_CMS_ERR_INVITE_CALLER_SDP_MSG_ERROR                                      0XF20A0009 /* ��ȡ��Ƶ���󷽵�SDP��Ϣ���� */
#define EV9000_CMS_ERR_INVITE_CALLER_SDP_PARAM_ERROR                                    0XF20A000A /* ��ȡ��Ƶ���󷽵�SDP������Ϣ���� */

#define EV9000_CMS_ERR_INVITE_CALLEE_IP_ERROR                                           0XF20A000B /* ��ȡ��Ƶ�����λ��IP��ַ���� */
#define EV9000_CMS_ERR_INVITE_CALLEE_PORT_ERROR                                         0XF20A000C /* ��ȡ��Ƶ�����λ�˿ڴ��� */

#define EV9000_CMS_ERR_INVITE_CALLEE_DEVICE_INFO_ERROR                                  0XF20A000D /* ��ȡ��Ƶ�����λ�����豸��Ϣ���� */
#define EV9000_CMS_ERR_INVITE_CALLEE_ROUTE_INFO_ERROR                                   0XF20A000E /* ��ȡ��Ƶ�����λ�ϼ�·����Ϣ���� */

#define EV9000_CMS_ERR_INVITE_CALLEE_MSG_BODY_ERROR                                     0XF20A000F /* ��ȡ��Ƶ�����λ��Invite��Ϣ����� */
#define EV9000_CMS_ERR_INVITE_CALLEE_SDP_MSG_ERROR                                      0XF20A0010 /* ��ȡ��Ƶ�����λ��SDP��Ϣ���� */
#define EV9000_CMS_ERR_INVITE_CALLEE_SDP_PARAM_ERROR                                    0XF20A0011 /* ��ȡ��Ƶ�����λ��SDP������Ϣ���� */


#define EV9000_CMS_ERR_INVITE_CALLEE_LOGIC_DEVICE_INFO_ERROR                            0XF20A0012 /* ��ȡ��Ƶ�����λ�豸��Ϣ���� */
#define EV9000_CMS_ERR_INVITE_CALLEE_NOT_SUPPORT_MULTI_STREAM_ERROR                     0XF20A0013 /* ��Ƶ�����λ�豸��֧�ֶ����� */
#define EV9000_CMS_ERR_INVITE_CALLEE_RECORD_NOT_START_ERROR                             0XF20A0014 /* ��Ƶ�����λ��¼��ҵ��û������ */
#define EV9000_CMS_ERR_INVITE_CALLEE_RECORD_NOT_COMPLETE_ERROR                          0XF20A0015 /* ��Ƶ�����λ��¼��ҵ��û�н��� */
#define EV9000_CMS_ERR_INVITE_CALLEE_VIDEO_NOT_COMPLETE_ERROR                           0XF20A0016 /* ��Ƶ�����λ���е���Ƶҵ��û�н��� */

#define EV9000_CMS_ERR_INVITE_GET_CALLEE_CR_DATA_ERROR                                  0XF20A0017 /* ��ȡ�����λ���еĺ�����Դʧ�� */

#define EV9000_CMS_ERR_INVITE_FIND_CALLEE_RECORD_INFO_ERROR                             0XF20A0018 /* ������Ƶ�����λ���е�¼����Ϣʧ�� */
#define EV9000_CMS_ERR_INVITE_GET_CALLEE_RECORD_INFO_ERROR                              0XF20A0019 /* ��ȡ��Ƶ�����λ���е�¼����Ϣʧ�� */

#define EV9000_CMS_ERR_INVITE_TRANSFER_MSG_TO_DEST_ERROR                                0XF20A001A /* ת��Invite��Ϣ��Ŀ�ĵ�ʧ�� */
#define EV9000_CMS_ERR_INVITE_ACCEPT_ERROR                                              0XF20A001B /* ���շ���������Invite��Ϣʧ�� */
#define EV9000_CMS_ERR_INVITE_FRONT_RETURN_ERROR                                        0XF20A001C /* ǰ��Invite��Ӧ������Ӧ��Ϣ */
#define EV9000_CMS_ERR_INVITE_GET_VIDEO_STREAM_ERROR                                    0XF20A001D /* ��Ƶ�����ȡǰ������ʧ�� */
#define EV9000_CMS_ERR_INVITE_TCP_SETUP_ERROR                                           0XF20A001E /* ��Ƶ����TCP���ӷ�ʽʧ�� */
#define EV9000_CMS_ERR_INVITE_TCP_CONNECTION_ERROR                                      0XF20A001F /* ��Ƶ����TCP���䷽ʽʧ��  */

/* ¼�� */
#define EV9000_CMS_ERR_RECORD_FIND_RECORD_INFO_ERROR                                    0XF20B0001 /* ����¼����Ϣʧ�� */
#define EV9000_CMS_ERR_RECORD_GET_RECORD_INFO_ERROR                                     0XF20B0002 /* ��ȡ¼����Ϣʧ�� */
#define EV9000_CMS_ERR_RECORD_TIME_INFO_ERROR                                           0XF20B0003 /* ¼��ʱ����Ϣ���� */

/* 0X F3XX XXXX	Ӧ�ö� */
#define EV9000_APPS_ERROR_NOMEMORY                       0xF3010000          // �ڴ治��
#define EV9000_APPS_ERROR_CREATETHREAD_FAILD             0xF3010001          // �����߳�ʧ��
#define EV9000_APPS_ERROR_PARAM_INVALID                  0xF3010002          // ��������

//Ӧ��ģ�������
#define EV9000APP_EROR_NOCTRLRESOURCE                    0xF3020000          // û�з�����Դ
#define EV9000APP_EROR_NOFINDRESOURCE                    0xF3020001          // û�в�ѯ��Դ
#define EV9000APP_EROR_NOPLAYRESOURCE                    0xF3020002          // û�в�����Դ
#define EV9000APP_EROR_GETUSERINFOFAILD                  0xF3020003          // ��ȡ�û���Ϣʧ��,�û������벻��ȷ
#define EV9000APP_EROR_SERVERIDEMPTY                     0xF3020004          // CMSIDΪ��     

//Ӧ��ģ�鲥�Ų��ִ�����
#define EV9000APP_EROR_OPENPORT_FAILD                    0xF3021000          // ����Ƶ���ն˿�ʧ��
#define EV9000APP_EROR_DATABUFFERFULL                    0xF3021001          // ������
#define EV9000APP_EROR_OPENFILE_FAILD                    0xF3021002          // ���ļ�ʧ��
#define EV9000APP_EROR_NOSUPPORT                         0xF3021003          // ��֧��
#define EV9000APP_EROR_NODATA                            0xF3021004          // û������
#define EV9000APP_EROR_OPENAD                            0xF3021005          // ����Ƶ�豸ʧ�� 
#define EV9000APP_EROR_LOADENC                           0xF3021006          // ���ؽ�����ʧ��
//ϵͳ��Ϣͨ��ģ�������
#define EV9000SystemInfo_ERROR                           0xF3040000          // SystemInfoģ�������붨��
#define EV9000SystemInfo_ERROR_OPENCLIENT_FAILED         0xF3040001          // ���ӷ����ʧ��
#define EV9000SystemInfo_ERROR_SENDDATA_FAILED           0xF3040002          // ��������ʧ��
#define EV9000SystemInfo_ERROR_PARSEXML_FAILED           0xF3040003          // ����XMLʧ��
#define EV9000SystemInfo_ERROR_RESPONSE                  0xF3040004          // �豸��Ӧ������Ϣ

//����ģ�������
#define EV9000ENC_EROR_NORESOURCE                        0xF3050000          // û�б�����Դ
#define EV9000ENC_EROR_NOENCMODE                         0xF3050001          // û���ҵ�����ģ��
#define EV9000ENC_EROR_CREATECONTEX_FAILD                0xF3050002          // ����������������ʧ��
#define EV9000ENC_EROR_OPENENCMODEL_FAILD                0xF3050003          // �򿪱���ģ��ʧ��
#define EV9000ENC_EROR_CREATEAVFRAME_FAILD               0xF3050004          // �������뻺��ʧ��
#define EV9000ENC_EROR_ENCMODE_INVALID                   0xF3050005          // ������󲻴���

//����ģ�������
#define EV9000DEC_EROR_NORESOURCE                        0xF3060000          // û�н�����Դ
#define EV9000DEC_EROR_NODECRESOURCE                     0xF3060001          // û�н���ģ����Դ
#define EV9000DEC_EROR_LOADDECMODE                       0xF3060002          // ���ؽ���ģ��ʧ��
#define EV9000DEC_EROR_LOADFUN_FAILD                     0xF3060003          // ����ģ�麯�����ز�ȫ
#define EV9000DEC_EROR_NODECMODE                         0xF3060004          // δ���ؽ���ģ��
#define EV9000DEC_EROR_NODECHANDLE                       0xF3060005          // δ�򿪽���ͨ��
#define EV9000DEC_EROR_NOAVDECMODE                       0xF3060006          // ����ģ��û���ҵ�������
#define EV9000DEC_EROR_CREATEPAS_FAILD                   0xF3060007          // �������������ʧ��
#define EV9000DEC_EROR_CREATECTX_FAILD                   0xF3060008          // ��������������ʧ��
#define EV9000DEC_EROR_CREATEAVFRAME                     0xF3060009          // �������뻺��֡ʧ��
#define EV9000DEC_EROR_OPENDEC_FAILD                     0xF306000A          // �򿪽�����ʧ��
#define EV9000DEC_EROR_BUFFERFULL                        0xF306000B          // ���뻺����
#define EV9000DEC_EROR_CREATECALLBACK_FAILD              0xF306000C          // ��������ص�ʧ��


//����ģ�������
#define EV9000PLAY_EROR_NORESOURCE                       0xF3070000          // ����ģ��û����Դ
#define EV9000PLAY_EROR_CREATESUR_FAILD                  0xF3070001          // ��������ʧ��,�������Կ���֧��
#define EV9000PLAY_EROR_LOCKSUR_FAILD                    0xF3070002          // �������ű���ʧ��
#define EV9000PLAY_EROR_OPENWAVE_FAILD                   0xF3070003          // ����Ƶ����ʧ��

//SIPͨ��ģ�������
#define EV9000SIPMODE_EROR_NORESOURCE                    0xF3080000          // û��SIPͨ����Դ
#define EV9000SIPMODE_EROR_STARTLOCAL_FAILD              0xF3080001          // ��������SIPͨ��ʧ��
#define EV9000SIPMODE_EROR_REGISTER_TIMEOUT              0xF3080002          // ע�ᳬʱ
#define EV9000SIPMODE_EROR_NOMESSAGE                     0xF3080003          // û���ҵ�SN��Ӧ��MESSAGE��
#define EV9000SIPMODE_EROR_MESSAGE_TIMEOUT               0xF3080004          // �ȴ�MESSAGE��ʱ
#define EV9000SIPMODE_EROR_NOINVITE                      0xF3080005          // û���ҵ�dialog_index��Ӧ��INVITE��
#define EV9000SIPMODE_EROR_INVITE_TIMEOUT                0xF3080006          // �ȴ�INVITE��ʱ
#define EV9000SIPMODE_EROR_NOINVITERESOURCE              0xF3080007          // û��INVITE��Դ
#define EV9000SIPMODE_ERROR_INVITE_480                   0xF3080008          // invite 480 ���� ��λ������
#define EV9000SIPMODE_ERROR_INVITE_503                   0xF3080009          // invite 503 ���� ����������
#define EV9000SIPMODE_ERROR_INVITE_404                   0xF308000A          // invite 404 ���� δ�ҵ���λ
#define EV9000SIPMODE_ERROR_INVITE_500                   0xF308000B          // invite 500 ���� ����������
#define EV9000SIPMODE_ERROR_INVITE_ELSE                  0xF308000C          // invite ��������
#define EV9000SIPMODE_ERROR_GETSDP_FAILD                 0xF308000D          // ��ȡ�Զ�SDPʧ��
#define EV9000SIPMODE_ERROR_SENDINFO_FAILD               0xF308000E          // ����info ��Ϣʧ��
#define EV9000SIPMODE_ERROR_INVITEACCEPT_FAILD           0xF308000F          // �ظ�Inviteʧ��
#define EV9000SIPMODE_ERROR_REFRESH_FAILD                0xF3080010          // ˢ��ע��ʧ��
#define EV9000SIPMODE_ERROR_KEEPALIVE_FAILD              0xF3080011          // ����ʧ��
#define EV9000SIPMODE_ERROR_INVITE_520                   0xF3080012          // invite 520

//PSģ�������
#define EV9000PS_EROR_NORESOURCE                         0xF3090000          // û��PS������Դ

//�豸ģ�������
#define EV9000DEVICE_EROR_NORESOURCE                     0xF30A0000          // û���豸��Դ
#define EV9000DEVICE_EROR_NODEVICERESOURCE               0xF30A0001          // û���豸ģ����Դ
#define EV9000DEVICE_EROR_LOADDEVICEMODE                 0xF30A0002          // �����豸ģ��ʧ��
#define EV9000DEVICE_EROR_LOADFUN_FAILD                  0xF30A0003          // �豸ģ�麯�����ز�ȫ
#define EV9000DEVICE_EROR_NODEVICEMODE                   0xF30A0004          // δ�����豸ģ��
#define EV9000DEVICE_EROR_NODEVICEHANDLE                 0xF30A0005          // δ���豸ͨ��
#define EV9000DEVICE_EROR_CONNECTFAILD                   0xF30A0006          // �����豸ʧ��
#define EV9000DEVICE_EROR_NOPLAYRESOURCE                 0xF30A0007          // û�в�����Դ
#define EV9000DEVICE_EROR_OPENREALPLAYFAILD              0xF30A0008          // ��ʵʱ��Ƶʧ��

//Э��������
#define EV9000PROTOCOL_EROR_NORESOURCE                   0xF30B0000          // û��Э����Դ

/* 0X F4XX XXXX	���ܿͻ��� */


/* 0X F5XX XXXX	TSU */
#define TSU_ERROR_DB_DISCONNECT                          0xF5000001          //  ���ݿ�δ����
#define TSU_ERROR_FINDRECORD_0                           0xF5000002          //  �鵽0��¼���¼
#define TSU_ERROR_DB_EXCEPTION                           0xF5000003          //  ���ݿ�����쳣
#define TSU_ERROR_ICE_EXCEPTION                          0xF5000004          //  ICE�쳣

/* 0X F6XX XXXX	ý������ */


/* 0X F7XX XXXX	DC������ */


#endif
