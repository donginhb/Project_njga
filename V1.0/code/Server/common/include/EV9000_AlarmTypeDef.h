/******************************************************************************

                  ��Ȩ���� (C), 2001-2013, ������Ѷ�������޹�˾

 ******************************************************************************
  �� �� ��   : EV9000_AlarmTypeDef.h
  �� �� ��   : ����
  ��    ��   : zb
  ��������   : 2014��9��26�� ������
  ����޸�   :
  ��������   : EV9000ϵͳ����
  �����б�   :
  �޸���ʷ   :
  1.��    ��   : 2013��9��26�� ������
    ��    ��   : zb
    �޸�����   : �����ļ�

******************************************************************************/
#ifndef EV9000_ALARM_TYPE_DEF_H
#define EV9000_ALARM_TYPE_DEF_H

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

/* 0x 01XX XXXX CMS */
#define	EV9000_CMS_SYSTEM_RUN_INFO                 0x01100000  //ϵͳ������Ϣ
#define	EV9000_CMS_SIPSTACK_RUN_INFO               0x01200000  //SIPЭ��ջ������Ϣ
#define	EV9000_CMS_USER_OPERATION_INFO             0x01300000  //�û�������¼

/* ϵͳ */
#define	EV9000_CMS_ICESTARTABNORMALITIES           0x01100001  //ICE���������쳣
#define	EV9000_CMS_BOARTINITFAIL                   0x01100002  //�����ʼ��ʧ��
#define	EV9000_CMS_STITCHDECETIONFAIL              0x01100003  //�����������ʧ��
#define	EV9000_CMS_BUSINESSTHREADINITFAIL          0x01100004  //ҵ���̳߳�ʼ��ʧ��
#define	EV9000_CMS_BUSINESSDATAINITFAIL            0x01100005  //ҵ�����ݳ�ʼ��ʧ��
#define	EV9000_CMS_MEMORYALLOCFAIL                 0x01100006  //�ڴ����ʧ��
#define	EV9000_CMS_RESOURCEAPPLICATIONFAIL         0x01100007  //��Դ����ʧ��
#define	EV9000_CMS_SEARCHBOARDFAIL                 0x01100008  //��������ʧ��
#define	EV9000_CMS_SETINGLOCALIPFAIL               0x01100009  //CMS���ñ���IP��ַʧ��
#define	EV9000_CMS_GETLOCALIPFAIL                  0x01100010  //CMS��ȡ����IP��ַʧ��
#define	EV9000_CMS_DBCONNECTFAIL                   0x01100011  //���ݿ�����ʧ��
#define	EV9000_CMS_DBQUERYFAIL                     0x01100012  //���ݿ��ѯʧ��
#define	EV9000_CMS_DBINSERTFAIL                    0x01100013  //���ݿ����ʧ��
#define	EV9000_CMS_DBMODIFYFAIL                    0x01100014  //���ݿ��޸�ʧ��
#define	EV9000_CMS_DBDELFAIL                       0x01100015  //���ݿ�ɾ��ʧ��
#define	EV9000_CMS_CPUUSAGEHIGH                    0x01100016  //CPU���ɹ���
#define	EV9000_CMS_RAMUSAGEHIGH                    0x01100017  //�ڴ渺�ɹ���
#define	EV9000_CMS_FANTEMPERATUREEHIGH             0x01100018  //�����¶ȹ���
                                                               
/* Э��ջ */                                                   
#define	EV9000_CMS_PROTOCALINITFAIL                0x01200001  //SIPЭ��ջ��ʼ��ʧ��
#define	EV9000_CMS_PROTOCALLISTENFAIL              0x01200002  //SIPЭ��ջ������Ϣ�˿�ʧ��
#define	EV9000_CMS_PROTOCALRECVMESSAGEFAIL         0x01200003  //SIPЭ��ջ������Ϣ����ʧ��
#define	EV9000_CMS_SENDSIPMESSAGEFAIL              0x01200004  //SIP����SIP��Ϣʧ��
#define	EV9000_CMS_BUILDLOCALSDPFAIL               0x01200005  //SIP��������SDP��Ϣʧ��
#define	EV9000_CMS_ANALYSELOCALSDPFAIL             0x01200006  //SIP��������SDP��Ϣʧ��

/* TSU */
#define	EV9000_CMS_TSU_ICE_ERROR                   0x01300001  //TSU ICE�ӿ��쳣
#define	EV9000_CMS_TSU_REGISTER_ERROR              0x01300002  //TSUע��ʧ��
#define	EV9000_CMS_TSU_OFFLINE                     0x01300003  //TSU����
#define	EV9000_CMS_TSU_NOTIFY_NOSTREAM             0x01300004  //TSU֪ͨǰ���豸û������
#define	EV9000_CMS_TSU_NOTIFY_PLAYCLOSE            0x01300005  //TSU֪ͨ���Ž���

/* ҵ�� */
#define	EV9000_CMS_DEVICE_REG_FAILED               0x01400001  //�豸ע��ʧ��
#define	EV9000_CMS_DEVICE_OFFLINE                  0x01400002  //�豸����
#define	EV9000_CMS_NETWORK_UNREACHED               0x01400003  //���粻�ɴ�
#define	EV9000_CMS_USER_REG_FAILED                 0x01400004  //�û���¼ʧ��
#define	EV9000_CMS_GET_SERVERID_ERROR              0x01400005  //��ȡ������IDʧ��
#define	EV9000_CMS_GET_DBIP_ERROR                  0x01400006  //��ȡ���ݿ�IP��ַʧ��
#define	EV9000_CMS_START_RECORD_ERROR              0x01400007  //¼��ҵ��ʧ��
#define	EV9000_CMS_CONNECT_TV_ERROR                0x01400008  //����ǽҵ��ʧ��
#define	EV9000_CMS_VIDEO_REQUEST_ERROR             0x01400009  //ʵʱ��Ƶҵ��ʧ��
#define	EV9000_CMS_DEVICE_CONTROL_ERROR            0x01400010  //ǰ���豸����ʧ��
#define	EV9000_CMS_SET_DEVICE_CONFIG_ERROR         0x01400011  //ǰ���豸����ʧ��
#define	EV9000_CMS_SET_VIDEO_PARAM_ERROR           0x01400012  //ǰ��ͼ���������ʧ��
#define	EV9000_CMS_REQUEST_IFRAM_ERROR             0x01400013  //����I֡ʧ��
#define	EV9000_CMS_CONTROL_AUTOZOMMIN_ERROR        0x01400014  //ǰ�˵���Ŵ�ʧ��
#define	EV9000_CMS_SET_XY_PARAM_ERROR              0x01400015  //����ǰ�˾�γ��ʧ��
#define	EV9000_CMS_GET_VIDEO_PARAM_ERROR           0x01400016  //��ȡǰ��ͼ�����ʧ��
#define	EV9000_CMS_GET_PRESET_ERROR                0x01400017  //��ȡǰ��Ԥ��λʧ��
#define	EV9000_CMS_GET_CATALOG_ERROR               0x01400018  //��ȡ�߼��豸Ŀ¼��Ϣʧ��
#define	EV9000_CMS_NOTIFY_CATALOG_ERROR            0x01400019  //֪ͨ�߼��豸Ŀ¼��Ϣʧ��
#define	EV9000_CMS_GET_DEVICE_INFO_ERROR           0x01400020  //��ȡ�豸��Ϣʧ��
#define	EV9000_CMS_GET_DEVICE_STATUS_ERROR         0x01400021  //��ȡ�豸״̬ʧ��
#define	EV9000_CMS_GET_DEVICE_CONFIG_ERROR         0x01400022  //��ȡǰ������ʧ��
#define	EV9000_CMS_QUERY_RECORD_INFO_ERROR         0x01400023  //¼���ѯʧ��
#define	EV9000_CMS_GET_USER_INFO_ERROR             0x01400024  //��ȡ�û���Ϣʧ��
#define	EV9000_CMS_GET_DEIVCE_GROUP_ERROR          0x01400025  //��ȡ�߼��豸������Ϣʧ��
#define	EV9000_CMS_GET_DEIVCE_MAP_GROUP_ERROR      0x01400026  //��ȡ�߼��豸�����ϵ��Ϣʧ��
#define	EV9000_CMS_GET_TOP_DEVICE_CONFIG_ERROR     0x01400027  //��ȡ���������豸������Ϣʧ��
#define	EV9000_CMS_GET_POLL_CONFIG_ERROR           0x01400028  //��ȡ��Ѳ������Ϣʧ��
#define	EV9000_CMS_GET_PLAN_CONFIG_ERROR           0x01400029  //��ȡԤ��������Ϣʧ��
#define	EV9000_CMS_GET_URL_CONFIG_ERROR            0x01400030  //��ȡWEB URL������Ϣʧ��
#define	EV9000_CMS_QUERY_ALARM_RECORD_ERROR        0x01400031  //��ȡ������¼��Ϣʧ��
#define	EV9000_CMS_GET_ONLINE_USER_ERROR           0x01400032  //��ȡ�����û���Ϣʧ��
#define	EV9000_CMS_NOTIFY_CONNECT_TV_ERROR         0x01400033  //֪ͨ���ӵ���ǽʧ��
#define	EV9000_CMS_NOTIFY_ALARM_ERROR              0x01400034  //֪ͨ������Ϣʧ��
#define	EV9000_CMS_NOTIFY_STATUS_ERROR             0x01400035  //֪ͨ״̬��Ϣʧ��
#define	EV9000_CMS_EXECUTE_PLAN_ERROR              0x01400036  //Ԥ��ִ��ʧ��
#define	EV9000_CMS_EXECUTE_POLL_ERROR              0x01400037  //��ѵִ��ʧ��
#define	EV9000_CMS_SEND_NOTIFY_ERROR               0x01400038  //����֪ͨʧ��
#define	EV9000_CMS_QUERY_DEC_STATUS_ERROR          0x01400039  //��ȡ������ͨ��״̬ʧ��
#define	EV9000_CMS_NOTIFY_DEC_STATUS_ERROR         0x01400040  //֪ͨ������ͨ��״̬ʧ��

//�û�������־
#define	EV9000_USER_LOG_LOGIN                      0x10100001 //�û���¼
#define	EV9000_USER_LOG_VIDEO_PLAY                 0x10100002 //�㲥��Ƶ
#define	EV9000_USER_LOG_DEVICE_CONTROL             0x10100003 //����
#define	EV9000_USER_LOG_EXECUTE_PLAN               0x10100004 //ִ��Ԥ��
#define	EV9000_USER_LOG_EXECUTE_POLL               0x10100005 //ִ����Ѳ
#define	EV9000_USER_LOG_STOP_POLL                  0x10100006 //ֹͣ��Ѳ
#define	EV9000_USER_LOG_CONNECT_TVWALL             0x10100007 //�л�����ǽ
#define	EV9000_USER_LOG_MODIFY_PWD                 0x10100008 //�޸��û�������
#define	EV9000_USER_LOG_VIDEO_PLAYBACK             0x10100009 //�ط�¼��
#define	EV9000_USER_LOG_VIDEO_DOWNLOAD             0x10100010 //����¼��
#define	EV9000_USER_LOG_LOCAL_RECORD               0x10100011 //����¼��
#define	EV9000_USER_LOG_LOCAL_CAPTURE              0x10100012 //ץ��
#define	EV9000_USER_LOG_ADD_PRESET                 0x10100013 //����Ԥ��λ
#define	EV9000_USER_LOG_EXECUTE_PRESET             0x10100014 //ִ��Ԥ��λ
#define	EV9000_USER_LOG_DEL_PRESET                 0x10100015 //ɾ��Ԥ��λ
#define	EV9000_USER_LOG_SET_AUTOBACK               0x10100016 //���ù�λ��
#define	EV9000_USER_LOG_DEL_AUTOBACK               0x10100017 //ȡ����λ��
#define	EV9000_USER_LOG_SET_CAMERA_PARAM           0x10100018 //�����������
#define	EV9000_USER_LOG_SET_CRUISE                 0x10100019 //����Ѳ��
#define	EV9000_USER_LOG_VERSION_DOWNLOAD           0x10100020 //���ذ汾

/* 0X 02XX XXXX	TSU */
#define TSU_SYS_RUN_LOG 0x02050000 //ϵͳ������־
#define TSU_MOUNT_DISK_FAILED 0x02050001 //�������ʧ�ܼ�����ʧ
#define TSU_DISK_NO_FREE_SPACE 0x02050002 //����ռ䲻��
#define TSU_DISK_WRITE_FAILED 0x02050003//tsuд�̳���ʧ��

#define TSU_RECV_SENDER_OVER_MAX 0x02060001 //TSU�����λ���ﵽ���
#define TSU_RECORD_OVER_MAX 0x02060002 //TSU¼�����ﵽ���
#define TSU_REALTRANSFER_OVER_MAX 0x02060003 //TSUʵʱ��Ƶ���ﵽ���

#define TSU_OPEN_RECORD_FAILED  0x02070401 //¼��طŴ�ʧ��
#define TSU_NO_FIND_RECORD_TSUDISK 0x02070402 //δ�ҵ�¼���ļ���Ӧ�Ĵ洢TSU������

#define TSU_OPEN_REALTIME_TRANSFER_FAILED 0x02070301 //ʵʱ��Ƶ��ʧ��
#define TSU_RECV_SENDER_NO_DATA 0x02070201 //ǰ��������

#define TSU_WRITE_DISK_FAILED 0x02070103 //д�����ʧ��
#define TSU_WRITE_LOST_DATA 0x02070102 //д����󶪰�
// #define	0x02050001	//�������ʧ�ܼ�����ʧ
// #define	0x02050002	//����ռ䲻��
// #define	0x02030001	//TSU�������ݿ�ʧ��   
// #define	0x02030002	//TSU�������ݿ�ʧ��
// #define	0x02030003	//TSU�޸����ݿ�ʧ��
// #define	0x02030004	//TSUɾ�����ݿ�ʧ��
// #define	0x02030005	//TSU��ѯ���ݿ�ʧ��
// #define	0x02020001	//TSU���ñ���IP��ַʧ��
// #define	0x02020002	//TSU��ȡ����IP��ַʧ��
// #define	0x02060001	//TSU�����λ���ﵽ���
// #define	0x02060002	//TSU¼�����ﵽ���
// #define	0x02060003	//TSUʵʱ��Ƶ���ﵽ���
// #define	0x02070101	//������������
// #define	0x02070102	//д����󶪰�
// #define	0x02070103	//д�����ʧ��
// #define	0x02070104	//����λת�������������Ͷ���
// #define	0x02070201	//ǰ��������
// #define	0x02070301	//ʵʱ��Ƶ��ʧ��
// #define	0x02070401	//¼��طŴ�ʧ��
// #define	0x02070402	//δ�ҵ�¼���ļ���Ӧ�Ĵ洢TSU������

/* 0X 04XX XXXX	Windows MGW */
// #define	0x04070001	//���ݿ�����ʧ��
// #define	0x04070002	//�豸����ʧ��
// #define	0x04070003	//��λ����Ƶ�ź�

/* 0X 05XX XXXX	Onvif ���� */
// #define	0x05070001	//���ݿ�����ʧ��
// #define	0x05070002	//�豸����ʧ��

/* 0X 06XX XXXX	Windows������ */
// #define	0x06010001	//VGAͨ���쳣
// #define	0x06010002	//���뿨ͨ���쳣
// #define	0x06020001	//�����쳣
// #define	0x06020002	//�����ʹ���
// #define	0x06060001	//�ڴ�ʹ���ʹ���
// #define	0x06060002	//CPU�¶ȹ���
// #define	0x06060003	//CPUʹ���ʹ���  
// #define	0x06070001	//�򿪶˿�ʧ��
// #define	0x06070002	//��ʱ��������
// #define	0x06070003	//��������ʧ��

/* 0X 07XX XXXX Ƕ��ʽ������ */
// #define	0x070101xx	//VGAͨ���쳣   ע xx Ϊ ���������ͨ��ID ��� Ϊ0��ע�������ͨ����
// #define	0x070102xx	//HDMIͨ���쳣  ע xx Ϊ ���������ͨ��ID ��� Ϊ0��ע�������ͨ����
// #define	0x070103xx	//DVIͨ���쳣  ע xx Ϊ ���������ͨ��ID ��� Ϊ0��ע�������ͨ����
// #define	0x070104xx	//BNCͨ���쳣  ע xx Ϊ ���������ͨ��ID ��� Ϊ0��ע�������ͨ����
// #define	0x070105xx	//��Ƶͨ���쳣  ע xx Ϊ ���������ͨ��ID ��� Ϊ0��ע�������ͨ����
// #define	0x070106xx	//����ͨ���쳣  ע xx Ϊ ���������ͨ��ID ��� Ϊ0��ע�������ͨ����
// #define	0x070107xx	//������ͨ���쳣  ע xx Ϊ ���������ͨ��ID ��� Ϊ0��ע�������ͨ����
// #define	0x070108xx	//����澯�쳣  ע xx Ϊ ���������ͨ��ID ��� Ϊ0��ע�������ͨ����
// #define	0x07020100	//�����쳣
// #define	0x07020200	//�����ʹ���
// #define	0x07020300	//�Ƿ�����
// #define	0x07020400	//�����ɶ�
// #define	0x07020500	//��������
// #define	0x07020600	//IP��ͻ
// #define	0x07030100	//�ڴ�ʹ���ʹ���
// #define	0x07030200	//CPU�¶ȹ���
// #define	0x07030300	//CPUʹ���ʹ���  
// #define	0x07030400	//�����ѹ����  
// #define	0x07040100	//ע��ʧ��
// #define	0x070402xx	//�򿪶˿�ʧ��  ע xx Ϊ ����Ľ���ͨ��ID ��� Ϊ0��ע�������ͨ����
// #define	0x070403xx	//�����ڴ���Դʧ��  ע xx Ϊ ����Ľ���ͨ��ID ��� Ϊ0��ע�������ͨ����
// #define	0x070404xx	//��������ͨ��ʧ��  ע xx Ϊ ����Ľ���ͨ��ID ��� Ϊ0��ע�������ͨ����
// #define	0x070405xx	//��ʱ��������  ע xx Ϊ ����Ľ���ͨ��ID ��� Ϊ0��ע�������ͨ����
// #define	0x070406xx	//����ʧ��  ע xx Ϊ ����Ľ���ͨ��ID ��� Ϊ0��ע�������ͨ����

/* 0X 08XX XXXX ������Ϊ���� */
#define	EV9000_BEHAVIORANALYSIS_SYSTEMABNORMALITIES                     0x08010000	//ϵͳ�쳣
#define	EV9000_BEHAVIORANALYSIS_VGAABNORMALITIES                        0x08010001	//VGAͨ���쳣
#define	EV9000_BEHAVIORANALYSIS_HDMIABNORMALITIES                       0x08010002	//HDMIͨ���쳣
#define	EV9000_BEHAVIORANALYSIS_DVIABNORMALITIES                        0x08010003	//DVIͨ���쳣
#define	EV9000_BEHAVIORANALYSIS_BNCABNORMALITIES                        0x08010004	//BNCͨ���쳣
#define	EV9000_BEHAVIORANALYSIS_AUDIOABNORMALITIES                      0x08010005	//��Ƶͨ���쳣
#define	EV9000_BEHAVIORANALYSIS_SERIALABNORMALITIES                     0x08010006	//����ͨ���쳣
#define	EV9000_BEHAVIORANALYSIS_SWITCHABNORMALITIES                     0x08010007	//������ͨ���쳣
#define	EV9000_BEHAVIORANALYSIS_SLALARMABNORMALITIES                    0x08010008	//����澯�쳣
#define	EV9000_BEHAVIORANALYSIS_NETABNORMALITIES                        0x08020000	//�����쳣
#define	EV9000_BEHAVIORANALYSIS_UNCONNECTABNORMALITIES                  0x08020001	//δ��������
#define	EV9000_BEHAVIORANALYSIS_IPCONFLICTABNORMALITIES                 0x08020002	//IP��ͻ
#define	EV9000_BEHAVIORANALYSIS_NETINSTABILITYABNORMALITIES             0x08020003	//���粻�ȶ�
#define	EV9000_BEHAVIORANALYSIS_DBABNORMALITIES                         0x08030000	//���ݿ��쳣
#define	EV9000_BEHAVIORANALYSIS_DBCONNECTABNORMALITIES                  0x08030001	//���ݿ������쳣
#define	EV9000_BEHAVIORANALYSIS_DBLOGINABNORMALITIES                    0x08030002	//���ݿ��¼�쳣
#define	EV9000_BEHAVIORANALYSIS_DBWORKABNORMALITIES                     0x08030003	//���ݿ�����쳣
#define	EV9000_BEHAVIORANALYSIS_PERFORMANCEABNORMALITIES                0x08060000	//�����쳣
#define	EV9000_BEHAVIORANALYSIS_CPUUSAGEHIGH                            0x08060001	//CPUʹ���ʹ���
#define	EV9000_BEHAVIORANALYSIS_RAMUSAGEHIGH                            0x08060002	//�ڴ�ʹ���ʹ���
#define	EV9000_BEHAVIORANALYSIS_CPUTEMPERATUREEHIGH                     0x08060003	//CPU�¶ȹ���
#define	EV9000_BEHAVIORANALYSIS_BOARDVOLINSTABILTY                      0x08060004	//�����ѹ����
#define	EV9000_BEHAVIORANALYSIS_BUSINESSABNORMALITIES                   0x08070000	//ҵ���쳣
#define	EV9000_BEHAVIORANALYSIS_OVERRALLABNORMALITIES                   0x08070100	//���������쳣
#define	EV9000_BEHAVIORANALYSIS_PROGRESSCRASHES                         0x08070101	//������
#define	EV9000_BEHAVIORANALYSIS_PROGRAMHANGS                            0x08070102	//�������
#define	EV9000_BEHAVIORANALYSIS_SIPABNORMALITIES                        0x08070200	//SIP�쳣
#define	EV9000_BEHAVIORANALYSIS_PROTOCALINITFAIL                        0x08070201	//Э��ջ��ʼ��ʧ��
#define	EV9000_BEHAVIORANALYSIS_PROTOCALLISTENFAIL                      0x08070202	//Э��ջ������Ϣ�˿�ʧ��
#define	EV9000_BEHAVIORANALYSIS_PROTOCALRECVMESSAGEFAIL                 0x08070203	//Э��ջ������Ϣ����ʧ��
#define	EV9000_BEHAVIORANALYSIS_SENDSIPMESSAGEFAIL                      0x08070204	//����SIP��Ϣʧ��
#define	EV9000_BEHAVIORANALYSIS_BUILDLOCALSDPFAIL                       0x08070205	//��������SDP��Ϣʧ��
#define	EV9000_BEHAVIORANALYSIS_ANALYSELOCALSDPFAIL                     0x08070206	//��������SDP��Ϣʧ��
#define	EV9000_BEHAVIORANALYSIS_STREAMABNORMALITIES                     0x08070300	//�����쳣
#define	EV9000_BEHAVIORANALYSIS_GETSTREAMFAIL                           0x08070301	//��ȡ��ʧ��
#define	EV9000_BEHAVIORANALYSIS_PACKLOSSRATEABNORMALITIES               0x08020002	//�������쳣
#define	EV9000_BEHAVIORANALYSIS_STREAMANALYSISABNORMALITIES             0x08020003	//���������쳣
#define	EV9000_BEHAVIORANALYSIS_STREAMPACKAGINGABNORMALITIES            0x08020004	//��������쳣
#define	EV9000_BEHAVIORANALYSIS_CODEORENCODEABNORMALITIES               0x08070400	//������쳣
#define	EV9000_BEHAVIORANALYSIS_CODEPARAMABNORMALITIES                  0x08070401	//��������쳣
#define	EV9000_BEHAVIORANALYSIS_ENCODEPARAMABNORMALITIES                0x08070402	//��������쳣
#define	EV9000_BEHAVIORANALYSIS_CODEDATAABNORMALITIES                   0x08070403	//�������ݴ���
#define	EV9000_BEHAVIORANALYSIS_ENCODEDATAABNORMALITIES                 0x08070404	//�������ݴ���
#define	EV9000_BEHAVIORANALYSIS_OVERLAYINFORABNORMALITIES               0x08070405	//������Ϣ�쳣
#define	EV9000_BEHAVIORANALYSIS_ANALYSISALGORITHMSABNORMALITIES         0x08070500	//�����㷨�쳣
#define	EV9000_BEHAVIORANALYSIS_ALGORITHINITPARAMSABNORMALITIES         0x08070501	//�㷨��ʼ�������쳣
#define	EV9000_BEHAVIORANALYSIS_ALGORITHINITSOURCESABNORMALITIES        0x08070502	//�㷨��ʼ����Դ�쳣
#define	EV9000_BEHAVIORANALYSIS_ALGORITHDATAABNORMALITIES               0x08070503	//�㷨�����쳣
#define	EV9000_BEHAVIORANALYSIS_ANALYSISRESULTABNORMALITIES             0x08070600	//��������쳣
#define	EV9000_BEHAVIORANALYSIS_UNDIRECTEDTRIPWIRE                      0x08070601	//�������
#define	EV9000_BEHAVIORANALYSIS_DIRECTEDTRIPWIRE                        0x08070602	//�������
#define	EV9000_BEHAVIORANALYSIS_IRRUPT                                  0x08070603	//�����������֣�
#define	EV9000_BEHAVIORANALYSIS_TRANSBOUNDARY                           0x08070604	//�뿪����Խ�磩
#define EV9000_BEHAVIORANALYSIS_ABANDON                                 0x08070605  //��Ʒ�������
#define EV9000_BEHAVIORANALYSIS_OBJLOST                                 0x08070606  //��Ʒ��ʧ���
#define EV9000_BEHAVIORANALYSIS_WANDER                                  0x08070607  //�ǻ����
#define EV9000_BEHAVIORANALYSIS_STOPCAR                                 0x08070608  //�Ƿ�ͣ��
#define EV9000_BEHAVIORANALYSIS_OUTDOOR                                 0x08070609  //���ż��
#define EV9000_BEHAVIORANALYSIS_LEAVEPOS                                0x0807060a  //��ڼ�� 

#define	EV9000_BEHAVIORANALYSIS_PRIVATEPROTOCOLANALYSISABNORMALITIES    0x08070700	//˽��Э������쳣
#define	EV9000_BEHAVIORANALYSIS_CONFIGMODYNOTIFYPROTOCOLABNORMALITIES   0x08070701	//�����޸�֪ͨЭ���쳣
#define	EV9000_BEHAVIORANALYSIS_GETPLATFORMPROTOCOL                     0x08070702	//��ȡƽ̨��ϢЭ��

/* 0X 09XX XXXX ��Ƶ������� */
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_SYSTEMABNORMALITIES                    0x09010000	//ϵͳ�쳣
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_VGAABNORMALITIES                       0x09010001	//VGAͨ���쳣
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_HDMIABNORMALITIES                      0x09010002	//HDMIͨ���쳣
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_DVIABNORMALITIES                       0x09010003	//DVIͨ���쳣
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_BNCABNORMALITIES                       0x09010004	//BNCͨ���쳣
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_AUDIOABNORMALITIES                     0x09010005	//��Ƶͨ���쳣
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_SERIALABNORMALITIES                    0x09010006	//����ͨ���쳣
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_SWITCHABNORMALITIES                    0x09010007	//������ͨ���쳣
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_SLALARMABNORMALITIES                   0x09010008	//����澯�쳣
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_NETABNORMALITIES                       0x09020000	//�����쳣
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_UNCONNECTABNORMALITIES                 0x09020001	//δ��������
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_IPCONFLICTABNORMALITIES                0x09020002	//IP��ͻ
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_NETINSTABILITY                         0x09020003	//���粻�ȶ�
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_DBABNORMALITIES                        0x09030000	//���ݿ��쳣
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_DBCONNECTABNORMALITIES                 0x09030001	//���ݿ������쳣
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_DBLOGINABNORMALITIES                   0x09030002	//���ݿ��¼�쳣
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_DBWORKABNORMALITIES                    0x09030003	//���ݿ�����쳣
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_PERFORMANCEABNORMALITIES               0x09060000	//�����쳣
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_CPUUSAGEHIGH                           0x09060001	//CPUʹ���ʹ���
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_RAMUSAGEHIGH                           0x09060002	//�ڴ�ʹ���ʹ���
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_CPUTEMPERATUREEHIGH                    0x09060003	//CPU�¶ȹ���
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_BOARDVOLINSTABILTY                     0x09060004	//�����ѹ����
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_BUSINESSABNORMALITIES                  0x09070000	//ҵ���쳣
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_OVERRALLABNORMALITIES                  0x09070100	//���������쳣
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_PROGRESSCRASHES                        0x09070101	//������
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_PROGRAMHANGS                           0x09070102	//�������
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_SIPABNORMALITIES                       0x09070200	//SIP�쳣
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_PROTOCALINITFAIL                       0x09070201	//Э��ջ��ʼ��ʧ��
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_PROTOCALLISTENFAIL                     0x09070202	//Э��ջ������Ϣ�˿�ʧ��
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_PROTOCALRECVMESSAGEFAIL                0x09070203	//Э��ջ������Ϣ����ʧ��
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_SENDSIPMESSAGEFAIL                     0x09070204	//����SIP��Ϣʧ��
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_BUILDLOCALSDPFAIL                      0x09070205	//��������SDP��Ϣʧ��
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_ANALYSELOCALSDPFAIL                    0x09070206	//��������SDP��Ϣʧ��
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_STREAMABNORMALITIES                    0x09070300	//�����쳣
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_GETSTREAMFAIL                          0x09070301	//��ȡ��ʧ��
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_PACKLOSSRATEABNORMALITIES              0x09020002	//�������쳣
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_STREAMANALYSISABNORMALITIES            0x09020003	//���������쳣
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_STREAMPACKAGINGABNORMALITIES           0x09020004	//��������쳣
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_CODEORENCODEABNORMALITIES              0x09070400	//������쳣
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_CODEPARAMABNORMALITIES                 0x09070401	//��������쳣
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_ENCODEPARAMABNORMALITIES               0x09070402	//��������쳣
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_CODEDATAABNORMALITIES                  0x09070403	//�������ݴ���
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_ENCODEDATAABNORMALITIES                0x09070404	//�������ݴ���
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_OVERLAYINFORABNORMALITIES              0x09070405	//������Ϣ�쳣
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_ANALYSISALGORITHMSABNORMALITIES        0x09070500	//�����㷨�쳣
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_ALGORITHINITPARAMSABNORMALITIES        0x09070501	//�㷨��ʼ�������쳣
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_ALGORITHINITSOURCESABNORMALITIES       0x09070502	//�㷨��ʼ����Դ�쳣
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_ALGORITHDATAABNORMALITIES              0x09070503	//�㷨�����쳣
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_ANALYSISRESULTABNORMALITIES            0x09070600	//��������쳣
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_DEFINITIONABNORMALITIES                0x09070601	//�������쳣����Ƶģ����
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_LIGHTABNORMALITIES                     0x09070602	//�����쳣(�ع⣩
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_COLORFULABNORMALITIES                  0x09070603	//��ɫ�쳣(ƫɫ��
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_SNOWABNORMALITIES                      0x09070604	//ѩ���쳣
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_VIDEOHUNGS                             0x09070605	//���涳��(��Ƶ���ᣩ
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_SIGNALLOSSABNORMALITIES                0x09070606	//�źŶ�ʧ�쳣
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_SCREENTINGLEABNORMALITIES              0x09070607	//���涶���쳣(��Ƶ������
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_VIDEOOCCLUSIONABNORMALITIES            0x09070608	//��Ƶ�ڵ��쳣
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_NIGHTMODE	                             0x09070609 //ͼ��ڰ�
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_LUMLOW  		                         0x0907060A //���ȹ���
#define EV9000_VIDEOQUALITYDIAGNOSTIC_CONTRASTLOW	                         0x0907060B	//�Աȶȹ���
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_UPHEAVAL		                         0x0907060C	//��Ƶ���
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_MOSAIC  		                         0x0907060D	//��Ƶ������
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_STRIPE                                 0x0907060E	//��Ƶ����
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_PTZ    		                         0x0907060F	//PTZ�쳣
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_SCENECHANGE	                         0x09070610	//�������
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_NETWORK 		                         0x09070611	//����
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_LOGIN   		                         0x09070612	//��������ʧ��
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_NOSTREAM                               0x09070613	//������

#define	EV9000_VIDEOQUALITYDIAGNOSTIC_PRIVATEPROTOCOLANALYSISABNORMALITIES   0x09070700	//˽��Э������쳣
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_CONFIGMODYNOTIFYPROTOCOLABNORMALITIES  0x09070701	//�����޸�֪ͨЭ���쳣
#define	EV9000_VIDEOQUALITYDIAGNOSTIC_GETPLATFORMPROTOCOL                    0x09070702	//��ȡƽ̨��ϢЭ��

/*----------------------------------------------*
 * �澯���Ͳ�ѯ˵��                             *
 *----------------------------------------------*/

/*��ѯ�ĸ澯��Ϣ����unsigned int������ʾ��ÿһλ��ʾһ�����͡����ɱ�ʾ32��
	
�ұ�Ϊ��һλ ��Ӧ��ϵ���£�

1	EV9000_BEHAVIORANALYSIS_ANALYSISRESULTABNORMALITIES 
2	EV9000_BEHAVIORANALYSIS_UNDIRECTEDTRIPWIRE    
3	EV9000_BEHAVIORANALYSIS_DIRECTEDTRIPWIRE         
4	EV9000_BEHAVIORANALYSIS_IRRUPT             
5	EV9000_BEHAVIORANALYSIS_TRANSBOUNDARY    
6	EV9000_VIDEOQUALITYDIAGNOSTIC_ANALYSISRESULTABNORMALITIES   
7	EV9000_VIDEOQUALITYDIAGNOSTIC_DEFINITIONABNORMALITIES         
8	EV9000_VIDEOQUALITYDIAGNOSTIC_LIGHTABNORMALITIES           
9	EV9000_VIDEOQUALITYDIAGNOSTIC_COLORFULABNORMALITIES              
10	EV9000_VIDEOQUALITYDIAGNOSTIC_SNOWABNORMALITIES            
11	EV9000_VIDEOQUALITYDIAGNOSTIC_VIDEOHUNGS                       
12	EV9000_VIDEOQUALITYDIAGNOSTIC_SIGNALLOSSABNORMALITIES        
13	EV9000_VIDEOQUALITYDIAGNOSTIC_SCREENTINGLEABNORMALITIES       
14	EV9000_VIDEOQUALITYDIAGNOSTIC_VIDEOOCCLUSIONABNORMALITIES   

*/

/* �豸���ϸ澯���� */
#define	EV9000_ALARM_CMS_ERROR                     0x90000001  //����������쳣
#define	EV9000_ALARM_TSU_ERROR                     0x90000002  //��ý��������쳣
#define	EV9000_ALARM_STORAGE_ERROR                 0x90000003  //�洢�豸�쳣
#define	EV9000_ALARM_DEC_TSU_ERROR                 0x90000004  //�������쳣
#define	EV9000_ALARM_DIAGNOSE_ERROR                0x90000005  //��Ϸ������쳣
#define	EV9000_ALARM_INTELLIGENT_ERROR             0x90000006  //���ܷ����������쳣
#define	EV9000_ALARM_LOGIC_DEVICE_ERROR            0x90000007  //��λ״̬�쳣
#define	EV9000_ALARM_ACCESS_DEVICE_ERROR           0x90000008  //�����豸�쳣��MGW/DVR/NVR��
#define	EV9000_ALARM_MOTION_DETECTION              0x90000009  //�ƶ���ⱨ��

#endif
