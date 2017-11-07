/******************************************************************************

                  ��Ȩ���� (C), 2001-2013, ������Ѷ�������޹�˾

 ******************************************************************************
  �� �� ��   : EV9000_InnerDef.h
  �� �� ��   : ����
  ��    ��   : yanghaifeng
  ��������   : 2013��7��2�� ���ڶ�
  ����޸�   :
  ��������   : EV9000ϵͳ���ڹ������ݶ���ͷ�ļ�
  �����б�   :
  �޸���ʷ   :
  1.��    ��   : 2013��7��2�� ���ڶ�
    ��    ��   : yanghaifeng
    �޸�����   : �����ļ�

******************************************************************************/
#ifndef EV9000_INNER_DEF_H
#define EV9000_INNER_DEF_H

/*----------------------------------------------*
 * ����ͷ�ļ�                                   *
 *----------------------------------------------*/
#include "EV9000_ExtDef.h"

#define  EV9000_VERSION        "V4.04B13"

#define  EV9000_FTP_USERNAME   "ftpuser"
#define  EV9000_FTP_PASSWORD   "ftppasswd"
#define  EV9000_FTP_DIR        "ftp"

#define  EV9000_CTS_NAME       "CTS"
#define  EV9000_MAP_NAME       "MAP"

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

/*----------------------------------------------*
 * ���ݿ�ṹ����                                *
 *----------------------------------------------*/
//Э�����Ͷ��� 
#define EV9000_STACKTYPE_GB28181           0     //GB28181Э��
#define EV9000_STACKTYPE_ONVIF             1     //ONVIFЭ��

//111~130��ʾ����Ϊǰ�����豸//
#define EV9000_DEVICETYPE_DVR              111   //DVR
#define EV9000_DEVICETYPE_VIDEOSERVER      112   //��Ƶ������
#define EV9000_DEVICETYPE_CODER            113   //������
#define EV9000_DEVICETYPE_DECODER          114   //������
#define EV9000_DEVICETYPE_VIDEOMITRIX      115   //��Ƶ�л�����
#define EV9000_DEVICETYPE_AUDIOMITRIX      116   //��Ƶ�л�����
#define EV9000_DEVICETYPE_ALARMCONTROLER   117   //����������
#define EV9000_DEVICETYPE_NVR              118   //������Ƶ¼���

//131~191��ʾ����Ϊǰ����Χ�豸//
#define EV9000_DEVICETYPE_CAMERA           131   //�����
#define EV9000_DEVICETYPE_IPC              132   //���������
#define EV9000_DEVICETYPE_SCREEN           133   //��ʾ��
#define EV9000_DEVICETYPE_ALARMINPUT       134   //���������豸
#define EV9000_DEVICETYPE_ALARMOUTPUT      135   //��������豸
#define EV9000_DEVICETYPE_AUDIOINPUT       136   //���������豸
#define EV9000_DEVICETYPE_AUDIOOUTPUT      137   //��������豸
#define EV9000_DEVICETYPE_MOBILE           138   //�ƶ������豸
#define EV9000_DEVICETYPE_PERIPHERY        139   //������Χ�豸

//RCU������߼��豸����
#define EV9000_DEVICETYPE_RCU_YAOXIN       180   //ң��
#define EV9000_DEVICETYPE_RCU_YAOKONG      181   //ң��
#define EV9000_DEVICETYPE_RCU_YAOCE        182   //ң��
#define EV9000_DEVICETYPE_RCU_YAOTIAO      183   //ң��
#define EV9000_DEVICETYPE_RCU_SHIJIAN      184   //�¼���
#define EV9000_DEVICETYPE_RCU_MINGLIN      185   //������

//200~299��ʾ����Ϊƽ̨�豸//
#define EV9000_DEVICETYPE_SIPSERVER        200   //sip������
#define EV9000_DEVICETYPE_WEBSERVER        201   //Web/Ӧ�÷�����
#define EV9000_DEVICETYPE_MEDIASERVER      202   //ý�������
#define EV9000_DEVICETYPE_PROCSERVER       203   //���������
#define EV9000_DEVICETYPE_SAFESERVER       204   //��ȫ������
#define EV9000_DEVICETYPE_ALARMSERVER      205   //����������
#define EV9000_DEVICETYPE_DBSERVER         206   //���ݿ������
#define EV9000_DEVICETYPE_GISSERVER        207   //GIS������
#define EV9000_DEVICETYPE_MANAGERSERVER    208   //���������
#define EV9000_DEVICETYPE_MGWSERVER        209   //��������
#define EV9000_DEVICETYPE_MEDIA_STORE_SERVER    210   //ý��洢����������
#define EV9000_DEVICETYPE_SIGNAL_SEC_GW    211   //���ȫ·�����ر���
#define EV9000_DEVICETYPE_TRAFFIC_GROUP    215   //ҵ��������
#define EV9000_DEVICETYPE_VIRTURAL_ORGAN   216   //������֯����

//Onfiv�����豸����
#define EV9000_DEVICETYPE_ONVIFPROXY             501   //ONVIF����

//��Ƶ��������豸����
#define EV9000_DEVICETYPE_VIDEODIAGNOSIS         601   //��Ƶ�������

//������Ϊ�����豸����
#define EV9000_DEVICETYPE_INTELLIGENTANALYSIS    602   //������Ϊ����

//Onvif�豸�����Ͷ���
#define EV9000_ONVIF_DEVICETYPE_DEVICE	10001	//Onvif�豸

//����״̬
#define EV9000_BOARD_STATUS_ONLINE          0x00000001    //����
#define EV9000_BOARD_STATUS_OFFLINE         0x00000002    //����
#define EV9000_BOARD_STATUS_ALARM           0x00000004    //����

//��־��Դ
#define EV9000_LOG_FROMTYPE_CMS             1    //CMS
#define EV9000_LOG_FROMTYPE_TSU             2    //TSU
#define EV9000_LOG_FROMTYPE_ONVIFPROXY      3    //ONVIF����
#define EV9000_LOG_FROMTYPE_DM8168DEC       4    //Ƕ��ʽ������
#define EV9000_LOG_FROMTYPE_DM8168IVA       5    //���ܷ����忨-��Ƶ��Ϊ����
#define EV9000_LOG_FROMTYPE_DM8168IVD       6    //���ܷ����忨-��Ƶ���

//��־����
#define EV9000_LOG_TYPE_USER                1    //�û�����
#define EV9000_LOG_TYPE_SYSTEM              2    //ϵͳ����
#define EV9000_LOG_TYPE_ALARM               3    //����

//��־����
#define EV9000_LOG_LEVEL_NORMAL             1    //һ��
#define EV9000_LOG_LEVEL_WARNING            2    //����
#define EV9000_LOG_LEVEL_ERROR              3    //����
#define EV9000_LOG_LEVEL_IMPORTANT          4    //��Ҫ

//�����豸����
#define  EV9000_ALARM_DEVICE_MJ             0x00010001   //�Ž�
#define  EV9000_ALARM_DEVICE_DZXG           0x00010002   //����Ѳ��
#define  EV9000_ALARM_DEVICE_HWBJ           0x00010003   //���ⱨ��
#define  EV9000_ALARM_DEVICE_DZWL           0x00010004   //����Χ��
#define  EV9000_ALARM_DEVICE_SGBJ           0x00010005   //���ⱨ��
#define  EV9000_ALARM_DEVICE_HJ             0x00010006   //��
#define  EV9000_ALARM_DEVICE_TEMPERATURE    0x00010007   //�¶ȱ���
#define  EV9000_ALARM_DEVICE_HUMIDITY       0x00010008   //ʪ�ȱ���
#define  EV9000_ALARM_DEVICE_YWCGQ          0x00010009   //��������
#define  EV9000_ALARM_DEVICE_TYKG           0x0001000A   //ͨ�ÿ���
#define  EV9000_ALARM_DEVICE_KTKG           0x0001000B   //�յ�����
#define  EV9000_ALARM_DEVICE_KTGZMS         0x0001000C   //�յ�����ģʽ

//�����豸���ͣ����RCU�궨��
#define  EV9000_ALARM_DEVICE_TYUNDEFINE     0x00000000   //ͨ��,δ����
#define  EV9000_ALARM_DEVICE_TYSTATUS       0x00000001   //ͨ��,�豸״̬
#define  EV9000_ALARM_DEVICE_YAOXIN         0x00010000   //ͨ��,ң����
#define  EV9000_ALARM_DEVICE_YAOKONG        0x00010001   //ͨ��,ң����
#define  EV9000_ALARM_DEVICE_YAOCE          0x00010002   //ͨ��,ң����
#define  EV9000_ALARM_DEVICE_YAOTIAO        0x00010003   //ͨ��,ң����
#define  EV9000_ALARM_DEVICE_SHIJIAN        0x00010004   //ͨ��,�¼���
#define  EV9000_ALARM_DEVICE_MINGLING       0x00010005   //ͨ��,������
#define  EV9000_ALARM_DEVICE_HWDS           0x00020000   //����,�������
#define  EV9000_ALARM_DEVICE_DZWL           0x00020001   //����,����Χ��
#define  EV9000_ALARM_DEVICE_BJSC           0x00020002   //����,�������
#define  EV9000_ALARM_DEVICE_MCZT           0x00030000   //�Ž�,�Ŵ�״̬
#define  EV9000_ALARM_DEVICE_CMAN           0x00030001   //�Ž�,���Ű�ť
#define  EV9000_ALARM_DEVICE_XPZT           0x00030002   //�Ž�,в��״̬
#define  EV9000_ALARM_DEVICE_BJSR           0x00030003   //�Ž�,��������
#define  EV9000_ALARM_DEVICE_MJBJSC         0x00030004   //�Ž�,�������
#define  EV9000_ALARM_DEVICE_SKJL           0x00030005   //�Ž�,ˢ����¼
#define  EV9000_ALARM_DEVICE_FKSQ           0x00030006   //�Ž�,������Ȩ
#define  EV9000_ALARM_DEVICE_SKSQ           0x00030007   //�Ž�,�տ���Ȩ
#define  EV9000_ALARM_DEVICE_YCKM           0x00030008   //�Ž�,Զ�̿���
#define  EV9000_ALARM_DEVICE_HJWD           0x00040000   //������,�¶�
#define  EV9000_ALARM_DEVICE_HJSD           0x00040001   //������,ʪ��
#define  EV9000_ALARM_DEVICE_HJSF6          0x00040002   //������,SF6Ũ��
#define  EV9000_ALARM_DEVICE_HJCO2          0x00040003   //������,CO2Ũ��
#define  EV9000_ALARM_DEVICE_HJO2           0x00040004   //������,����Ũ��
#define  EV9000_ALARM_DEVICE_XFYG           0x00050000   //����,�̸�̽����
#define  EV9000_ALARM_DEVICE_XFWG           0x00050001   //����,�¸�̽����
#define  EV9000_ALARM_DEVICE_KTKG           0x00060000   //�յ�,����
#define  EV9000_ALARM_DEVICE_KTFS           0x00060001   //�յ�,����
#define  EV9000_ALARM_DEVICE_KTMS           0x00060002   //�յ�,ģʽ1-�ͷ�|2-����|3-����|4-�Զ�|5-��ʪ|������Ч';
#define  EV9000_ALARM_DEVICE_FJQT           0x00070001   //���,��ͣ

//TCP�˿�
#define EV9000_TCP_SERVERPORT               20001        //���ӷ���˶˿�

//ϵͳ��Ϣ����
typedef enum
{
	EV9000_SYSTEMINFO_VERSERVER = 0,     //ϵͳ�汾
		EV9000_SYSTEMINFO_VERCLIENT,         //FTP�����������µĿͻ��˰汾,
		EV9000_SYSTEMINFO_VERMAP,            //FTP�����������µĵ�ͼ�汾,
		EV9000_SYSTEMINFO_SERVERPORT,
		EV9000_SYSTEMINFO_SERVERADDR,
		EV9000_SYSTEMINFO_CLIENTFORCEDUODATE, //�ͻ����Ƿ�ǿ�Ƹ���
		EV9000_SYSTEMINFO_MAPFORCEDUODATE,    //��ͼ�Ƿ�ǿ�Ƹ���
		EV9000_SYSTEMINFO_CLIENTDESCRIPTION,  //�ͻ��˸�������
		EV9000_SYSTEMINFO_MAPDESCRIPTION,     //��ͼ��������
		EV9000_SYSTEMINFO_SYSCURTIME,            //��ǰϵͳʱ��
}EV9000_SYSTEMINFO_TYPE;

//EV9000����������Ϣ�ṹ
typedef struct EV9000_BoardConfig
{
	unsigned int         nID;                                      //��¼���
	unsigned int         nBoardType;                               //��������
	unsigned int         nEnable;                                  //�Ƿ�����
	char                 strBoardID[EV9000_IDCODE_LEN];            //�������
	unsigned int         nSlotID;                                  //��λ��
	unsigned int         nStatus;                                  //����״̬
	char                 strCMSID[EV9000_IDCODE_LEN];              //CMS ���
	unsigned int         nResved1;                                 //����1
	char                 strResved2[EV9000_SHORT_STRING_LEN];      //����2
}EV9000_BoardConfig, *LPEV9000_BoardConfig;

typedef struct EV9000_BoardNetConfig
{
	unsigned int         nID;                                      //��¼���
	unsigned int         nBoardIndex;                             //��������
	unsigned int         nEnable;                                  //�Ƿ�����
	unsigned int         nEthID;                                   //����ڱ��
	unsigned int         nPort;                                    //SIP�˿ں�
	char                 strIP[EV9000_SHORT_STRING_LEN];           //�����ַ
	char                 strMask[EV9000_SHORT_STRING_LEN];         //��������
	char                 strGateWay[EV9000_SHORT_STRING_LEN];      //��������
	char                 strHost[EV9000_SHORT_STRING_LEN];         //��������
	unsigned int         nStatus;                                  //����˿�״̬
	char                 strCMSID[EV9000_IDCODE_LEN];              //CMS ���
	unsigned int         nResved1;                                 //����1
	char                 strResved2[EV9000_SHORT_STRING_LEN];      //����2
}EV9000_BoardNetConfig, *LPEV9000_BoardNetConfig;

//����·�ɱ�
typedef struct EV9000_RouteNetConfig
{
	unsigned int         nID;                                      //��¼���
	char                 strServerID[EV9000_IDCODE_LEN];           //Ŀ�ķ���������
	char                 strServerIP[EV9000_SHORT_STRING_LEN];     //Ŀ�ķ�������ַ
	char                 strServerHost[EV9000_SHORT_STRING_LEN];   //Ŀ�ķ���������
	unsigned int         nServerPort;                              //Ŀ�ķ������˿�
	char                 strUserName[EV9000_SHORT_STRING_LEN];     //ע���û���
	char                 strPassword[EV9000_SHORT_STRING_LEN];     //ע������
	unsigned int         nLinkType;                                //��������:0:���¼���1��ͬ����Ĭ��0
	unsigned int         nResved1;                                 //����1
	char                 strResved2[EV9000_SHORT_STRING_LEN];      //����2
}EV9000_RouteNetConfig, *LPEV9000_RouteNetConfig;

//�û��߼��豸�������ñ�
// typedef struct EV9000_UserLogicDeviceGroupConfig
// {
// 	unsigned int         nID;                                      //��¼���
// 	char                 strUserID[EV9000_IDCODE_LEN];              //�û�ID
// 	char                 strName[EV9000_LONG_STRING_LEN];          //������
// 	unsigned int         nSortID;                                  //ͬһ���ڵ����������ţ�Ĭ��0������
// 	unsigned int         nParentID;                                //���ڵ���
// 	unsigned int         nResved1;                                 //����1
// 	char                 strResved2[EV9000_SHORT_STRING_LEN];      //����2
// }EV9000_UserLogicDeviceGroupConfig, *LPEV9000_UserLogicDeviceGroupConfig;

//�û��߼��豸�����ϵ��
// typedef struct EV9000_UserLogicDeviceMapGroupConfig
// {
// 	unsigned int         nID;                                       //��¼���
//     char                 strUserID[EV9000_IDCODE_LEN];              //�û�ID
// 	int                  nGroupID;                                  //��λ����
// 	int                  nDeviceIndex;                              //�豸ID
// 	unsigned int         nSortID;                                   //ͬһ���ڵ����������ţ�Ĭ��0������
// 	unsigned int         nResved1;                                  //����1
// 	char                 strResved2[EV9000_SHORT_STRING_LEN];       //����2 
// }EV9000_UserLogicDeviceMapGroupConfig, *LPEV9000_UserLogicDeviceMapGroupConfig; 

//ϵͳ��Ϣ��
typedef struct EV9000_SystemConfig
{
	unsigned int         nID;                                       //��¼���
	char                 strKeyName[EV9000_SHORT_STRING_LEN];       //�ؼ��ֶ�����
	char                 strKeyValue[EV9000_NORMAL_STRING_LEN];     //�ؼ��ֶ�ֵ
	unsigned int         nResved1;                                  //����1
	char                 strResved2[EV9000_SHORT_STRING_LEN];       //����2 
}EV9000_SystemConfig, *LPEV9000_SystemConfig;   

//��׼�����豸��
typedef struct EV9000_GBPhyDeviceConfig
{
	unsigned int         nID;                                       //��¼���
	char                 strDeviceID[EV9000_IDCODE_LEN];            //�豸ID
	char                 strCMSID[EV9000_IDCODE_LEN];               //���������
	unsigned int         nEnable;                                   //�Ƿ�����
	unsigned int         nDeviceType;                               //�豸����
	char                 strDeviceIP[EV9000_SHORT_STRING_LEN];      //�豸IP
	unsigned int         nMaxCamera;                                //�豸ͨ����
	unsigned int         nMaxAlarm;                                 //�豸����ͨ����
	char                 strUserName[EV9000_SHORT_STRING_LEN];      //ע���û���
	char                 strPassword[EV9000_SHORT_STRING_LEN];      //ע������
	unsigned int         nStatus;                                   //�豸״̬
	char                 strModel[EV9000_SHORT_STRING_LEN];         //�豸�ͺ�
	char                 strFirmware[EV9000_SHORT_STRING_LEN];      //�豸�汾
	char                 strManufacturer[EV9000_NORMAL_STRING_LEN]; //�豸������
	unsigned int         nLinkType;                                 //��������:0:���¼���1��ͬ����Ĭ��0
	unsigned int         nResved1;                                  //����1
	char                 strResved2[EV9000_SHORT_STRING_LEN];       //����2 
}EV9000_GBPhyDeviceConfig, *LPEV9000_GBPhyDeviceConfig;

//��׼�����豸��ʱ��
typedef struct EV9000_GBPhyDeviceTempConfig
{
	unsigned int         nID;                                       //��¼���
	char                 strDeviceID[EV9000_IDCODE_LEN];            //��λ���
	char                 strCMSID[EV9000_IDCODE_LEN];               //���������
	unsigned int         nDeviceType;                               //�豸����
	char                 strDeviceIP[EV9000_SHORT_STRING_LEN];      //�豸IP
	unsigned int         nMaxCamera;                                //�豸ͨ����
	unsigned int         nMaxAlarm;                                 //�豸����ͨ����
	char                 strUserName[EV9000_SHORT_STRING_LEN];      //ע���û���
	char                 strPassword[EV9000_SHORT_STRING_LEN];      //ע������
	unsigned int         nStatus;                                   //�豸״̬
	char                 strModel[EV9000_SHORT_STRING_LEN];         //�豸�ͺ�
	char                 strFirmware[EV9000_SHORT_STRING_LEN];      //�豸�汾
	char                 strManufacturer[EV9000_NORMAL_STRING_LEN]; //�豸������
	unsigned int         nLinkType;                                 //��������:0:���¼���1��ͬ����Ĭ��0
	unsigned int         nResved1;                                  //����1
	char                 strResved2[EV9000_SHORT_STRING_LEN];       //����2 
}EV9000_GBPhyDeviceTempConfig, *LPEV9000_GBPhyDeviceTempConfig;

//��Χ�豸���ñ�
typedef struct EV9000_OuterDeviceConfig
{
	unsigned int         nID;                                       //��¼���
	char                 strDeviceID[EV9000_IDCODE_LEN];            //��λ���
	char                 strDeviceName[EV9000_SHORT_STRING_LEN];	//�ؼ��ֶ�����
	char                 strDeviceType[EV9000_NORMAL_STRING_LEN];   //�ؼ��ֶ�ֵ
	char                 strDeviceParam[EV9000_NORMAL_STRING_LEN];  //�豸����
	unsigned int         nResved1;                                  //����1
	char                 strResved2[EV9000_SHORT_STRING_LEN];       //����2 
}EV9000_OuterDeviceConfig, *LPEV9000_OuterDeviceConfig;

//�Ǳ������豸��
typedef struct EV9000_UNGBPhyDeviceConfig
{
	unsigned int         nID;                                       //��¼���
	unsigned int         nDeviceType;                               //�豸����
	char                 strDeviceIP[EV9000_SHORT_STRING_LEN];      //�豸IP
	unsigned int         nDevicePort;                               //�豸�˿�
	char                 strUserName[EV9000_SHORT_STRING_LEN];      //�û���
	char                 strPassword[EV9000_SHORT_STRING_LEN];      //����
	unsigned int         nStreamType;                               //��������ʾ
	unsigned int         nRecordType;                               //�Ƿ�ǰ��¼��
	unsigned int         nResved1;                                  //����1
	char                 strResved2[EV9000_SHORT_STRING_LEN];       //����2 
}EV9000_UNGBPhyDeviceConfig, *LPEV9000_UNGBPhyDeviceConfig;

//�Ǳ������豸ͨ�����ñ�
typedef struct EV9000_UNGBPhyDeviceChannelConfig
{
	unsigned int         nID;                                       //��¼���
	unsigned int         nDeviceID;                                 //�豸���
	char                 strDeviceChannel[EV9000_NORMAL_STRING_LEN];//�豸ͨ��
	unsigned int         nMapChannel;                               //ӳ��ͨ��
	unsigned int         nResved1;                                  //����1
	char                 strResved2[EV9000_SHORT_STRING_LEN];       //����2 
}EV9000_UNGBPhyDeviceChannelConfig, *LPEV9000_UNGBPhyDeviceChannelConfig;

//¼����Ա�
typedef struct EV9000_RecordSchedConfig
{
	unsigned int         nID;                                       //��¼���
	int                  nDeviceIndex;                              //��λ���
	int                  nRecordEnable;                             //�Ƿ�¼��
	unsigned int         nDays;                                     //¼������
	unsigned int         nTimeLength;                               //¼��ʱ��
	unsigned int         nType;                                     //¼������
	unsigned int         nStreamType;                                //��������
	unsigned int         nTimeOfAllWeek;                            //ȫ¼
	unsigned int         nBandWidth;                                //����
	unsigned int         nResved1;                                  //����1
	char                 strResved2[EV9000_SHORT_STRING_LEN];       //����2 
}EV9000_RecordSchedConfig, *LPEV9000_RecordSchedConfig;

//¼��ʱ�̲��Ա�
typedef struct EV9000_RecordTimeSchedConfig
{
	unsigned int         nID;                                       //��¼���
	unsigned int         nRecordSchedIndex;                         //���Ա��
	unsigned int         nDayInWeek;                                //���ڼ�
	unsigned int         nBeginTime;                                //¼��ʼʱ��
	unsigned int         nEndTime;                                  //¼�����ʱ��
	unsigned int         nResved1;                                  //����1
	char                 strResved2[EV9000_SHORT_STRING_LEN];       //����2 
    
}EV9000_RecordTimeSchedConfig, *LPEV9000_RecordTimeSchedConfig;

//�û��豸Ȩ�ޱ�
typedef struct EV9000_UserDevicePermConfig
{
	unsigned int         nID;                                       //��¼���
	unsigned int         nUserIndex;                                //�û����
	unsigned int         nDeviceIndex;                              //�豸���
	unsigned int         nResved1;                                  //����1
	char                 strResved2[EV9000_SHORT_STRING_LEN];       //����2 
}EV9000_UserDevicePermConfig, *LPEV9000_UserDevicePermConfig;

//�û����
typedef struct EV9000_UserGroupConfig
{
	unsigned int         nID;                                       //��¼���
	char                 strGroupName[EV9000_SHORT_STRING_LEN];     //����
	unsigned int         nPermission;                               //����Ȩ��
	unsigned int         nResved1;                                  //����1
	char                 strResved2[EV9000_SHORT_STRING_LEN];       //����2 
}EV9000_UserGroupConfig, *LPEV9000_UserGroupConfig;

//�û����豸Ȩ�ޱ�
typedef struct EV9000_GroupDevicePermConfig
{
	unsigned int         nID;                                       //��¼���
	int                  nGroupID;                                  //��λ����
	int                  nDeviceIndex;                              //�豸���
	unsigned int         nResved1;                                  //����1
	char                 strResved2[EV9000_SHORT_STRING_LEN];       //����2 
}EV9000_GroupDevicePermConfig, *LPEV9000_GroupDevicePermConfig;

//�û����û����ϵ
typedef struct EV9000_UserMapGroupConfig
{
	unsigned int         nID;                                       //��¼���
	int                  nGroupID;                                  //��λ����
	unsigned int         nUserIndex;                                //�û����
	unsigned int         nResved1;                                  //����1
	char                 strResved2[EV9000_SHORT_STRING_LEN];       //����2 
}EV9000_UserMapGroupConfig, *LPEV9000_UserMapGroupConfig;

//��ѲȨ�ޱ�
typedef struct EV9000_PollPermissionConfig
{
	unsigned int         nID;                                       //��¼���
	unsigned int         nPollID;                                   //��ѲID
	char                 strUserID[EV9000_IDCODE_LEN];              //�û�ID
	unsigned int         nResved1;                                  //����1
	char                 strResved2[EV9000_SHORT_STRING_LEN];       //����2 
}EV9000_PollPermissionConfig, *LPEV9000_PollPermissionConfig;


//Ԥ��Ȩ�ޱ�
typedef struct EV9000_PlanPermissionConfig
{
	unsigned int         nID;                                       //��¼���
	unsigned int         nPlanID;                                   //Ԥ�����
	char                 strUserID[EV9000_IDCODE_LEN];              //�û����
	unsigned int         nResved1;                                  //����1
	char                 strResved2[EV9000_SHORT_STRING_LEN];       //����2 
}EV9000_PlanPermissionConfig, *LPEV9000_PlanPermissionConfig;

//Ԥ��������
typedef struct EV9000_PlanLinkageConfig
{
	unsigned int         nID;                                       //��¼���
	unsigned int         nAlarmSourceID;                            //����Դ���
	unsigned int         nStartPlanID;                              //������ʼ����Ԥ�����
	unsigned int         nStopPlanID;                               //������������Ԥ�����
	unsigned int         nType;                                     //��������
	unsigned int         nLevel;                                    //��������
	unsigned int         nRepeatEnable;                             //�ظ�����
	unsigned int         nResved1;                                  //����1
	char                 strResved2[EV9000_SHORT_STRING_LEN];       //����2 
}EV9000_PlanLinkageConfig, *LPEV9000_PlanLinkageConfig;


//Ѳ��Ȩ�ޱ�
typedef struct EV9000_CruisePermissionConfig
{
	unsigned int         nID;                                       //��¼���
	unsigned int         nCruiseID;                                 //Ѳ�����
	char                 strUserID[EV9000_IDCODE_LEN];              //�û����
	unsigned int         nResved1;                                  //����1
	char                 strResved2[EV9000_SHORT_STRING_LEN];       //����2 
}EV9000_CruisePermissionConfig, *LPEV9000_CruisePermissionConfig;



//�û�������¼��
typedef struct EV9000_UserLogRecord
{
	unsigned int         nID;                                       //��¼���
	char                 strLogID[EV9000_SHORT_STRING_LEN];         //��־���
	char                 strUserID[EV9000_IDCODE_LEN];              //�û����
	unsigned int         nType;                                     //��־����
	unsigned int         nLevel;                                    //��־����
	unsigned int         nTime;                                     //����ʱ��
	char                 strInfo[EV9000_LONG_STRING_LEN];           //��־����
	unsigned int         nResved1;                                  //����1
	char                 strResved2[EV9000_SHORT_STRING_LEN];       //����2 
}EV9000_UserLogRecord, *LPEV9000_UserLogRecord;

//¼���Ǳ�
typedef struct EV9000_FileTagRecord
{
	unsigned int         nID;                                       //��¼���
	char                 strTagID[EV9000_SHORT_STRING_LEN];         //��Ǳ��
	int                  nDeviceIndex;                              //�豸���
	unsigned int         nType;                                     //�������
	unsigned int         nLevel;                                    //��Ǽ���
	unsigned int         nTime;                                     //����ʱ��
	char                 strInfo[EV9000_LONG_STRING_LEN];           //�������
	unsigned int         nResved1;                                  //����1
	char                 strResved2[EV9000_SHORT_STRING_LEN];       //����2 
}EV9000_FileTagRecord, *LPEV9000_FileTagRecord;

//�豸��ά��
typedef struct EV9000_DeviceStatusRecord
{
	unsigned int         nID;                                       //��¼���
	char                 strStatusID[EV9000_SHORT_STRING_LEN];      //״̬���
	int                  nDeviceIndex;                              //�豸���
	unsigned int         nStatus;                                   //״̬����
	unsigned int         nTime;                                     //����ʱ��
	unsigned int         nResved1;                                  //����1
	char                 strResved2[EV9000_SHORT_STRING_LEN];       //����2 
}EV9000_DeviceStatusRecord, *LPEV9000_DeviceStatusRecord;

typedef struct
{
    unsigned int  nIndex;                                          /* �豸������ */
	unsigned int  nDeviceType;                                     // �豸����
    unsigned int  nAlarmIP;                                        /* ����Դ��IP��ַ */
    unsigned int  nAlarmLevel;                                     /* ���ϸ澯���� */          
    unsigned int  nAlarmID;                                        /*  ���ϸ澯����*/     
    int           nAlarmTime;                                         /* ���ϸ澯���ָ���ʱ�� */      
    char          aucBoardID[24];                                     /* �������� */            
    unsigned char ucAlarmInfo[EV9000_LONG_STRING_LEN];             /* �������� */           
}EV9000_LOG;

typedef struct
{
    unsigned int         nID;                                            /* ��¼��� */
	char                 strCode[EV9000_SHORT_STRING_LEN];               /* ��� */
	char                 strName[EV9000_SHORT_STRING_LEN];               /* ���� */
    unsigned int         nResved1;                                       //����1
	char                 strResved2[EV9000_SHORT_STRING_LEN];            //����2           
}EV9000_GBCode;

//Onvif�����豸��
typedef struct _EV9000_OnvifPhyDeviceConfig
{
	unsigned int         nID;                                       //��¼���
	char                 strDeviceID[EV9000_Onvif_UUID_Len];        //�豸ID(UUID)
	char                 strCMSID[EV9000_IDCODE_LEN];               //���������
	char                 strOnvifProxyID[EV9000_IDCODE_LEN];        //ProxyID
	unsigned int         nEnable;                                   //�Ƿ�����
	unsigned int         nDeviceType;                               //�豸����
	char                 strDeviceIP[EV9000_SHORT_STRING_LEN];      //�豸IP
	unsigned int         nMaxCamera;                                //�豸ͨ����
	unsigned int         nMaxAlarm;                                 //�豸����ͨ����
	char                 strUserName[EV9000_SHORT_STRING_LEN];      //ע���û���
	char                 strPassword[EV9000_SHORT_STRING_LEN];      //ע������
	unsigned int         nStatus;                                   //�豸״̬
	char                 strModel[EV9000_SHORT_STRING_LEN];         //�豸�ͺ�
	char                 strFirmware[EV9000_SHORT_STRING_LEN];      //�豸�汾
	char                 strManufacturer[EV9000_NORMAL_STRING_LEN]; //�豸������
	unsigned int         nLinkType;                                 //��������:0:���¼���1��ͬ����Ĭ��0
	unsigned int         nResved1;                                  //����1
	char                 strResved2[EV9000_SHORT_STRING_LEN];       //����2
}EV9000_OnvifPhyDeviceConfig, *LPEV9000_OnvifPhyDeviceConfig;

//Onvif�߼��豸��
typedef struct _EV9000_OnvifLogicDeviceConfig
{
	unsigned int         nID;                                       //��¼���
	char                 strDeviceID[EV9000_IDCODE_LEN];            //��λ���
	char                 strCMSID[EV9000_IDCODE_LEN];               //CMS ���
	unsigned int         nEnable;                                   //�Ƿ�����	
	unsigned int         nDeviceType;                               //�豸����
	char                 strDeviceName[EV9000_NORMAL_STRING_LEN];   //��λ����	
	unsigned int         nPhyDeviceIndex;                           //ý�������豸ID
	unsigned int         nPhyDeviceChannel;                         //ý�������豸ͨ��
    char                 strPhyDeviceChannelMark[EV9000_LONG_STRING_LEN];   //ý�������豸ͨ����ʶ
	unsigned int         nCtrlEnable;                               //�Ƿ�ɿ�
	unsigned int         nMicEnable;                                //�Ƿ�֧�ֶԽ�
	unsigned int         nCtrlDeviceIndex;                          //�����豸ID
	unsigned int         nCtrlDeviceChannel;                        //�����豸ͨ��
	unsigned int         nStreamCount;                              //�Ƿ�֧�ֶ�����
	unsigned int         nStreamType;                               //��������
	unsigned int         nNeedCodec;                                //�Ƿ���Ҫ�����
	unsigned int         nRecordType;                               //¼������(ǰ�ˡ�����)
	char                 strManufacturer[EV9000_NORMAL_STRING_LEN]; //�豸������
	char                 strModel[EV9000_SHORT_STRING_LEN];         //�豸�ͺ�
	char                 strOwner[EV9000_SHORT_STRING_LEN];         //�豸����
	char                 strCivilCode[EV9000_SHORT_STRING_LEN];     //��������
	char                 strBlock[EV9000_SHORT_STRING_LEN];         //����
	char                 strAddress[EV9000_NORMAL_STRING_LEN];      //��װ��ַ
	unsigned int         nParental;                                 //�Ƿ������豸
	char                 strParentID[EV9000_IDCODE_LEN];            //���豸/����/ϵͳID
	unsigned int         nSafetyWay;                                //���ȫģʽ
	unsigned int         nRegisterWay;                              //ע�᷽ʽ
	char                 strCertNum[EV9000_SHORT_STRING_LEN];       //֤�����к�
	unsigned int         nCertifiable;                              //֤����Ч��ʶ
	unsigned int         nErrCode;                                  //��Чԭ����
	char                 strEndTime[EV9000_SHORT_STRING_LEN];       //֤����ֹ��Ч��
	unsigned int         nSecrecy;                                  //��������
	char                 strIPAddress[EV9000_SHORT_STRING_LEN];     //IP��ַ
	unsigned int         nPort;                                     //�˿ں�
	char                 strPassword[EV9000_SHORT_STRING_LEN];      //����
	unsigned int         nStatus;                                   //�豸״̬
	double               dLongitude;                                //����
	double               dLatitude;                                 //γ��
	unsigned int         nResved1;                                  //����1
	char                 strResved2[EV9000_SHORT_STRING_LEN];       //����2 
	unsigned int         nResved3;                                  //����3
	char                 strResved4[EV9000_SHORT_STRING_LEN];       //����4
}EV9000_OnvifLogicDeviceConfig, *LPEV9000_OnvifLogicDeviceConfig;

typedef enum
{
	WEBSTYTLE_PLAN = 0,
		WEBSTYTLE_POLL,
		WEBSTYTLE_ALARMLINKAGE,
		WEBSTYTLE_MANAGER,
		WEBSTYTLE_CRUISE,
		WEBSTYTLE_NPETVWALL,
		WEBSTYTLE_CLOUDREG,
}WEBSTYTLE_TYPE;

//�����ṹ�壺
typedef struct EV9000_WebInterFaceConfig
{
	unsigned int          nID;                                   //��¼���
	WEBSTYTLE_TYPE        eWebStytle;                            //ҳ������
	char                  strServerIP[EV9000_LONG_STRING_LEN];   //������IP
    unsigned int          nPort;                                 //�˿ں�
	char                  strURL[EV9000_LONG_STRING_LEN];         // URL��ַ
	unsigned int          nResved1;                              //����1
	char                  strResved2[EV9000_SHORT_STRING_LEN];   //����2 
}EV9000_WebInterFaceConfig, *LPEV9000_WebInterFaceConfig;

//TCP��������head
typedef struct EV9000_TCP_Head 
{
	char                   mark;                                  //ʹ��$��ʾÿ��rtp�Ŀ�ʼ
	unsigned short         length;                                //�������ĳ��� 
	unsigned int           nResved1;                              //����1
	char                   strResved2[EV9000_SHORT_STRING_LEN];   //����2 
}EV9000_TCP_Head, *LPEV9000_TCP_Head;
#endif
