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
#define ERR_GBLOGIC_DEVICE_ERROR           0XF2000001 /* �߼��豸ID ���� */
#define ERR_GET_IDLE_TSU_INDEX_ERROR       0XF2000002 /* ��ȡ���õ�TSU ����ʧ�� */
#define ERR_GET_TSU_INFO_ERROR             0XF2000003 /* ��ȡ���õ�TSU ��Ϣʧ�� */
#define ERR_RECORD_TIME_INFO_ERROR         0XF2000004 /* ¼��ʱ����Ϣ����*/
#define ERR_TSU_RETURN_ERROR               0XF2000005  /* TSU ����ʧ�� */


/* 0X F3XX XXXX	Ӧ�ö� */
#define EV9000_APPS_ERROR_NOMEMORY                       0xF3010000          // �ڴ治��
#define EV9000_APPS_ERROR_CREATETHREAD_FAILD             0xF3010001          // �����߳�ʧ��
#define EV9000_APPS_ERROR_PARAM_INVALID                  0xF3010002          // ��������

//Ӧ��ģ�������
#define EV9000APP_EROR_NOCTRLRESOURCE                    0xF3020000          // û�з�����Դ
#define EV9000APP_EROR_NOFINDRESOURCE                    0xF3020001          // û�в�ѯ��Դ
#define EV9000APP_EROR_NOPLAYRESOURCE                    0xF3020002          // û�в�����Դ
#define EV9000APP_EROR_GETUSERINFOFAILD                  0xF3020003          // ��ȡ�û���Ϣʧ��,�û������벻��ȷ

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


/* 0X F8XX XXXX	���ݿ� */
#ifndef DB_ERROR_DEF
#define DB_ERROR_DEF
#define DB_ERR_OK                       0
#define DB_ERR_BASE                     0XF8000000     //����ֵ

#define DB_ERR_ICE                      0XF8010000     //ICE����
#define DB_ERR_DATABUS                  0XF8020000     //databus���ش���
#define DB_ERR_BDB                      0XF8040000     //BDB����
#define DB_ERR_OTHER                    0XF8080000     //��������
#define DB_ERR_INVALUDHANDLE            0XF8100000     //���ݿ�����Ч

/* 0X F9XX XXXX	SIPЭ��ջ */
#define	EV9000_SIPSTACK_PARAM_ERROR                          0XF9010001  //Э��ջ��������
#define	EV9000_SIPSTACK_NEW_CALLID_ERROR                     0XF9010002  //Э��ջ�����µ�ID����
#define	EV9000_SIPSTACK_GET_SOCKET_ERROR                     0XF9010003  //Э��ջ������Ϣ��ȡSocket����
#define	EV9000_SIPSTACK_TRANSACTION_INIT_ERROR               0XF9010003  //Э��ջ������Ϣ��ʼ���������
#define	EV9000_SIPSTACK_SEND_MESSAGE_ERROR                   0XF9010004  //Э��ջ������Ϣ����

#define	EV9000_SIPSTACK_UA_TIMER_INIT_ERROR                  0XF9020001  //Э��ջUA��ʱ����ʼ��ʧ��
#define	EV9000_SIPSTACK_SIP_TIMER_INIT_ERROR                 0XF9020002  //Э��ջSIP��ʱ����ʼ��ʧ��
#define	EV9000_SIPSTACK_UAC_TIMER_INIT_ERROR                 0XF9020103  //Э��ջUAC��ʱ����ʼ��ʧ��
#define	EV9000_SIPSTACK_UAS_TIMER_INIT_ERROR                 0XF9020104  //Э��ջUAS��ʱ����ʼ��ʧ��
#define	EV9000_SIPSTACK_UA_INIT_ERROR                        0XF9020105  //Э��ջUA��ʼ��ʧ��
#define	EV9000_SIPSTACK_SIP_MESSAGE_INIT_ERROR               0XF9020106  //Э��ջSIP MESSAGE��ʼ��ʧ��
#define	EV9000_SIPSTACK_CALL_BACK_INIT_ERROR                 0XF9020107  //Э��ջCall Back��ʼ��ʧ��
#define	EV9000_SIPSTACK_SIPSTACK_INIT_ERROR                  0XF9020108  //Э��ջsip stack��ʼ��ʧ��
#define	EV9000_SIPSTACK_RUN_THREAD_INIT_ERROR                0XF9020109  //Э��ջ�̳߳�ʼ��ʧ��
#define	EV9000_SIPSTACK_UDP_LIST_INIT_ERROR                  0XF9020110  //Э��ջudp���ճ�ʼ��ʧ��

#define	EV9000_SIPSTACK_REGISTER_GET_UAC_ERROR               0XF9030101  //Э��ջ����ע����Ϣ��ȡUAC����
#define	EV9000_SIPSTACK_REGISTER_GENERA_ERROR                0XF9030102  //Э��ջ����ע����Ϣ����ע����Ϣʧ��

#define	EV9000_SIPSTACK_INVITE_GET_UA_ERROR                  0XF9040101  //Э��ջ����Invite��Ϣ��ȡUA����
#define	EV9000_SIPSTACK_INVITE_GET_SDP_INFO_ERROR            0XF9040102  //Э��ջ����Invite��Ϣ��ȡSDP����
#define	EV9000_SIPSTACK_INVITE_GENERA_ERROR                  0XF9040103  //Э��ջ����Invite��Ϣ����Invite��Ϣʧ��

#define	EV9000_SIPSTACK_MESSAGE_GENERA_ERROR                 0XF9050101  //Э��ջ����Message��Ϣ����Message��Ϣʧ��

//ICE
#define DB_ERR_ICE_EXCEPTION           (DB_ERR_ICE | 1)

//DATABUS
#define DB_ERR_DATABUS_DBNOTEXIST      (DB_ERR_DATABUS | 1)
#define DB_ERR_DATABUS_FIELDNOTEXIST   (DB_ERR_DATABUS | 2)
#define DB_ERR_DATABUS_NOTGETREADY     (DB_ERR_DATABUS | 4)
#define DB_ERR_DATABUS_SQLEXCEPTION    (DB_ERR_DATABUS | 8)

//BDB


//OTHER
#define DB_ERR_OTHER_UNCONNECT         (DB_ERR_OTHER | 1)
#define DB_ERR_OTHER_STRISNULL         (DB_ERR_OTHER | 2)     //�ַ���Ϊ��
#define DB_ERR_OTHER_OVERMAXROW        (DB_ERR_OTHER | 3)    //�кų�����Χ
#define DB_ERR_OTHER_MOVENEXTFAIL      (DB_ERR_OTHER | 4)   //movenext ʧ��
#define DB_ERR_OTHER_GETVALUEFAIL      (DB_ERR_OTHER | 5)    //ȡֵʧ��
#define DB_ERR_OTHER_FIELDNUMERROR     (DB_ERR_OTHER | 6)
#define DB_ERR_OTHER_UNKNOWN           (DB_ERR_OTHER | 7) 


#define DB_PING_ERROR_RECON            -10000

#endif

#endif
