/******************************************************************************

                  ��Ȩ���� (C), 2001-2013, ������Ѷ�������޹�˾

 ******************************************************************************
  �� �� ��   : EV9000_ExtDef.h
  �� �� ��   : ����
  ��    ��   : yanghaifeng
  ��������   : 2013��7��2�� ���ڶ�
  ����޸�   :
  ��������   : EV9000ϵͳ���⹫�����ݶ���ͷ�ļ�
  �����б�   :
  �޸���ʷ   :
  1.��    ��   : 2013��7��2�� ���ڶ�
    ��    ��   : yanghaifeng
    �޸�����   : �����ļ�

******************************************************************************/
#ifndef EV9000_EXT_DEF_H
#define EV9000_EXT_DEF_H

#if defined(_WIN32) //windows

#define EV9000APP_API  extern "C" __declspec(dllexport)
typedef  unsigned __int64 unsigned int64;

#elif defined(__linux__)

#define EV9000APP_API     extern "C"

#endif

/*----------------------------------------------*
 * ��������                                     *
 *----------------------------------------------*/


/*----------------------------------------------*
 * �궨��                                       *
 *----------------------------------------------*/

#define EV9000_IDCODE_LEN             20+4    //ͳһ������󳤶�
#define EV9000_SHORT_STRING_LEN       32+4    //���ַ�������
#define EV9000_NORMAL_STRING_LEN      64+4    //һ���ַ�������
#define EV9000_LONG_STRING_LEN        128+4   //���ַ�������
#define EV9000_MAX_CHANNDELID         1024    //����豸ͨ����
#define EV9000_Onvif_UUID_Len		  36+4    //Onvif ʹ�õ��豸�� UUID�ĳ���.����'-'�ַ�

//�û�Ȩ��
#define EV9000_USER_PERMISSION_REALPLAY     0x00000001    //ʵʱ��Ƶ
#define EV9000_USER_PERMISSION_CAMERACTRL   0x00000002    //�������
#define EV9000_USER_PERMISSION_PARAMCTRL    0x00000004    //��������

#define EV9000_USER_PERMISSION_PLAYBACK     0x00000008    //¼��ط�
#define EV9000_USER_PERMISSION_DOWNLOAD     0x00000010    //¼������

#define EV9000_USER_PERMISSION_GIS          0x00000020    //���ӵ�ͼ

#define EV9000_USER_PERMISSION_TVWALL       0x00000040    //����ǽ����

#define EV9000_USER_PERMISSION_PLAN         0x00000080    //Ԥ������

#define EV9000_USER_PERMISSION_POLL         0x00000100    //��Ѳ����

#define EV9000_USER_PERMISSION_ALARMLINKAGE 0x00000200    //��������

#define EV9000_USER_PERMISSION_MANAGER      0x00000400    //��������

//�û�����
#define EV9000_USER_LEVEL_SUPERADMIN        0             //��������Ա
#define EV9000_USER_LEVEL_ADMIN             1             //����Ա
#define EV9000_USER_LEVEL_OPERATOR          2             //����Ա
#define EV9000_USER_LEVEL_GENERAL           3             //һ���û�

//�����ͷ�����
//1��GB28181��Я����PS�����б�ʾ��
//2��RTP������Я��������
//3��EV9000��չ������

//PS������Ƶ����һ��ͨ���з���
//������������Ƶ��������ͨ���з���

//GB28181�ж����������(����Э��)
#define EV9000_STREAMDATA_TYPE_PS             96

#define EV9000_STREAMDATA_TYPE_VIDEO_MPEG4    97
#define EV9000_STREAMDATA_TYPE_VIDEO_H264     98
#define	EV9000_STREAMDATA_TYPE_VIDEO_SVAC     99
		
#define	EV9000_STREAMDATA_TYPE_VIDEO_HIK      500
#define	EV9000_STREAMDATA_TYPE_VIDEO_DAH      501

#define	EV9000_STREAMDATA_TYPE_AUDIO_G723     4		
#define	EV9000_STREAMDATA_TYPE_AUDIO_G711A    8
#define	EV9000_STREAMDATA_TYPE_AUDIO_G722     9
#define	EV9000_STREAMDATA_TYPE_AUDIO_G729     18
#define	EV9000_STREAMDATA_TYPE_AUDIO_SVAC     20

//��Ƶ��������(���ڱ����)
#define EV9000APP_VIDEOENCODE_TYPE_MPEG4      0x10
#define	EV9000APP_VIDEOENCODE_TYPE_H264       0x1B
#define EV9000APP_VIDEOENCODE_TYPE_GPU        0x6001
#define	EV9000APP_VIDEOENCODE_TYPE_SVAC       0x80
#define EV9000APP_VIDEOENCODE_TYPE_H265       0x48
 		
#define	EV9000APP_VIDEOENCODE_TYPE_HIK        0x500
#define	EV9000APP_VIDEOENCODE_TYPE_DAH        0x501

// ��Ƶ��������(���ڱ����)
#define EV9000APP_AUDIOENCODE_TYPE_G711       0x90
#define	EV9000APP_AUDIOENCODE_TYPE_G722       0x92
#define EV9000APP_AUDIOENCODE_TYPE_G723       0x93
#define EV9000APP_AUDIOENCODE_TYPE_G729       0x99
#define EV9000APP_AUDIOENCODE_TYPE_SAVC       0x9B

//��λ״̬
#define EV9000_LOGICDEVICE_STATUS_ONLINE      0x00000000  //��λ����
#define EV9000_LOGICDEVICE_STATUS_INVAILED    0x00000001  //��λ��Ч
#define EV9000_LOGICDEVICE_STATUS_OFFLINE     0x00000002  //��λ����
#define EV9000_LOGICDEVICE_STATUS_NOVIDEO     0x00000004  //��λ����Ƶ�ź�
#define EV9000_LOGICDEVICE_STATUS_INTEL       0x00000008  //��λ���������ܷ���
#define EV9000_LOGICDEVICE_STATUS_ALARM       0x00000010  //��λ���ڱ���

//¼������
#define EV9000_RECORD_TYPE_NORMAL           1    //��ͨ¼��
#define EV9000_RECORD_TYPE_INTELLIGENCE     2    //����¼��
#define EV9000_RECORD_TYPE_ALARM            3    //����¼��
#define EV9000_RECORD_TYPE_BACKUP           4    //����¼��

//��������
#define EV9000_STREAM_TYPE_MASTER           1    //����
#define EV9000_STREAM_TYPE_SLAVE            2    //����
#define EV9000_STREAM_TYPE_TSC            	3    //ת������

#define EV9000_STREAM_TYPE_INTELLIGENCE     10   //���ܷ�����

/*----------------------------------------------*
 * ���ݽṹ����                           *
 *----------------------------------------------*/

// ʱ��ṹ
typedef struct EV9000_TIME
{
	unsigned short   usYear;            //��
	unsigned short   usMonth;           //��
	unsigned short   usDay;             //��
	unsigned short   usHour;            //ʱ
	unsigned short   usMinute;          //��
	unsigned short   usSecond;          //��
}EV9000_TIME, *LPEV9000_TIME;

typedef struct  
{
	unsigned short   usLeft;            //��
	unsigned short   usRight;           //��
	unsigned short   usTop;             //��
	unsigned short   usBottom;          //��
}EV9000_RECT;

//�û���Ϣ
typedef struct EV9000_UserConfig
{
	unsigned int         nID;                                       //��¼���
	char                 strUserID[EV9000_IDCODE_LEN];              //�û����
	unsigned int         nEbable;                                   //�Ƿ�����
	char                 strUserName[EV9000_SHORT_STRING_LEN];      //ע���û���
	char                 strPassword[EV9000_SHORT_STRING_LEN];      //ע������
	unsigned int         nLevel;                                    //�û�����
	unsigned int         nPermission;                               //�û�Ȩ��
	char                 strLogInIP[EV9000_SHORT_STRING_LEN];       //�û�ָ����¼IP
	char                 strLogInMAC[EV9000_SHORT_STRING_LEN];      //�û�ָ����¼MAC
	char                 strRealName[EV9000_SHORT_STRING_LEN];      //�û���ʵ����
	char                 strTel[EV9000_SHORT_STRING_LEN];           //�û���ϵ��ʽ
	unsigned int         nResved1;                                  //����1
	char                 strResved2[EV9000_SHORT_STRING_LEN];       //����2 
}EV9000_UserConfig, *LPEV9000_UserConfig;

typedef struct EV9000_OnLineUserConfig
{
	unsigned int          nID;                                        //�û���¼���
	char                  strUserName[EV9000_SHORT_STRING_LEN];       //�û���
	char                  strRealName[EV9000_SHORT_STRING_LEN];       //�û���ʵ����
	char                  strLogInIP[EV9000_SHORT_STRING_LEN];        //�û���¼IP
	unsigned int          nPort;                                      //�û���¼�˿ں�
	char                  strTel[EV9000_SHORT_STRING_LEN];            //�û���ϵ��ʽ
	unsigned int          nResved1;                                   //����1
	char                  strResved2[EV9000_SHORT_STRING_LEN];        //����2
}EV9000_OnLineUserConfig, *LPEV9000_OnLineUserConfig;

//�߼���λ
typedef struct EV9000_GBLogicDeviceConfig
{
	unsigned int         nID;                                       //��¼���
	char                 strDeviceID[EV9000_IDCODE_LEN];            //��λ���
	char                 strCMSID[EV9000_IDCODE_LEN];               //CMS ���
	unsigned int         nEnable;                                   //�Ƿ�����	
	unsigned int         nDeviceType;                               //�豸����
	char                 strDeviceName[EV9000_NORMAL_STRING_LEN];   //��λ����	
	unsigned int         nPhyDeviceIndex;                           //ý�������豸ID
	unsigned int         nPhyDeviceChannel;                         //ý�������豸ͨ��
	unsigned int         nCtrlEnable;                               //�Ƿ�ɿ�
	unsigned int         nMicEnable;                                //�Ƿ�֧�ֶԽ�
	unsigned int         nFrameCount;                               //֡��
	unsigned int         nCtrlDeviceIndex;                          //�����豸ID
	unsigned int         nCtrlDeviceChannel;                        //�����豸ͨ��
	unsigned int         nStreamCount;                              //������
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
	char                 strParam[EV9000_LONG_STRING_LEN];          //����
	unsigned int         nIAEnable;                                 //�Ƿ����ڽ������ܷ���
	unsigned int         nRecordByTimeEnable;                       //�Ƿ�֧�ְ�ʱ��ط�
	unsigned int         nResved1;                                  //����1
	char                 strResved2[EV9000_SHORT_STRING_LEN];       //����2 
	unsigned int         nResved3;                                  //����3
	char                 strResved4[EV9000_SHORT_STRING_LEN];       //����4
}EV9000_GBLogicDeviceConfig, *LPEV9000_GBLogicDeviceConfig;

//�߼��豸�������ñ�
typedef struct EV9000_LogicDeviceGroupConfig
{
	unsigned int         nID;                                      //��¼���
	char                 strGroupID[EV9000_SHORT_STRING_LEN];      //����
	char                 strName[EV9000_LONG_STRING_LEN];          //������
	unsigned int         nSortID;                                  //ͬһ���ڵ����������ţ�Ĭ��0������
	char                 strParentID[EV9000_SHORT_STRING_LEN];     //���ڵ���
	unsigned int         nResved1;                                 //����1
	char                 strResved2[EV9000_SHORT_STRING_LEN];      //����2
}EV9000_LogicDeviceGroupConfig, *LPEV9000_LogicDeviceGroupConfig;

//�߼��豸�����ϵ��
typedef struct EV9000_LogicDeviceMapGroupConfig
{
	unsigned int         nID;                                      //��¼���
	char                 strGroupID[EV9000_SHORT_STRING_LEN];      //��λ����
	unsigned int         nDeviceIndex;                             //�豸ID
	char                 strCMSID[EV9000_IDCODE_LEN];              //CMS ���
	unsigned int         nSortID;                                  //ͬһ���ڵ����������ţ�Ĭ��0������
	unsigned int         nResved1;                                 //����1
	char                 strResved2[EV9000_SHORT_STRING_LEN];      //����2        
}EV9000_LogicDeviceMapGroupConfig, *LPEV9000_LogicDeviceMapGroupConfig;

//������Ϣ
typedef struct EV9000_AlarmRecord
{
	unsigned int         nID;                                       //��¼���
//	char                 strAlarmID[EV9000_SHORT_STRING_LEN];       //¼����
	unsigned int         nDeviceIndex;                              //�豸ID
	unsigned int         nType;                                     //��������
	unsigned int         nLevel;                                    //��������
	unsigned int         nStartTime;                                //��������ʱ��
	unsigned int         nStopTime;                                 //��������ʱ��
	char                 strInfo[EV9000_LONG_STRING_LEN];           //��������
	unsigned int         nResved1;                                  //����1
	char                 strResved2[EV9000_SHORT_STRING_LEN];       //����2 
}EV9000_AlarmRecord, *LPEV9000_AlarmRecord;

//¼����Ϣ
typedef struct EV9000_FileRecord
{
	unsigned int         nID;                                       //��¼���
	unsigned int         nDeviceIndex;                              //�߼��豸���
	unsigned int         nStorageIndex;                             //�洢�豸���
	unsigned int         nStartTime;                                //��ʼʱ��
	unsigned int         nStopTime;                                 //����ʱ��
	unsigned int         nSize;                                     //��С(�ֽ�)
	char                 strStorageIP[EV9000_SHORT_STRING_LEN];     //�����ַ
	char                 strStoragePath[EV9000_LONG_STRING_LEN];    //�洢·��
	unsigned int         nType;                                     //¼������
	unsigned int         nFileOver;                                 //¼�������� Ĭ��Ϊ0
	unsigned int         nResved1;                                  //����1
	char                 strResved2[EV9000_SHORT_STRING_LEN];       //����2 
}EV9000_FileRecord, *LPEV9000_FileRecord;

//ϵͳ��־��¼��
typedef struct EV9000_SystemLogRecord
{
	unsigned int         nID;                                       //��¼���
	unsigned int         nFromType;                                 //��־��Դ(CMS��TSU��ONVIF����)
	unsigned int         nDeviceIndex;                              //�豸ID
	char                 strDeviceIP[EV9000_SHORT_STRING_LEN];      //�豸��ַ
	unsigned int         nLogType;                                  //��־����(�û�������ϵͳ���С�����)
	unsigned int         nLogLevel;                                 //��־����(һ�㡢���桢����)
	unsigned int         nLogTime;                                  //����ʱ��
	char                 strInfo[EV9000_LONG_STRING_LEN];           //��־����
	unsigned int         nResved1;                                  //����1
	char                 strResved2[EV9000_SHORT_STRING_LEN];       //����2 
}EV9000_SystemLogRecord, *LPEV9000_SystemLogRecord;

//��Ѳ
typedef struct EV9000_PollConfig
{
	unsigned int         nID;                                       //��¼���
	char                 strPollName[EV9000_NORMAL_STRING_LEN];     //��Ѳ����
	unsigned int         nScheduledRun;                             //�Ƿ�ʱִ��
	unsigned int         nStartTime;                                //��ʼʱ��
	unsigned int         nDurationTime;                             //����ʱ��
	unsigned int         nUserID;                                   //�û�ID
	unsigned int         nType;                                     //��ѯ���ͣ�ǰ̨��ѯ���̨��ѯ
	unsigned int         nResved1;                                  //����1 1:��ѯ��ʼ 0:��ѯֹͣ
	char                 strResved2[EV9000_SHORT_STRING_LEN];       //����2 
}EV9000_PollConfig, *LPEV9000_PollConfig;

//��Ѳ����
typedef struct EV9000_PollActionConfig
{
	unsigned int         nID;                                       //��¼���
	unsigned int         nPollID;                                   //��Ѳ���
	unsigned int         nType;                                     //��������
	char                 strSourceID[EV9000_IDCODE_LEN];            //Դ�豸���
	char                 strDestID[EV9000_IDCODE_LEN];              //Ŀ���豸���
	unsigned int         nScreenID;                                 //�ͻ��˻�����
	unsigned int         nLiveTime;                                 //ͣ��ʱ��
	unsigned int         nDestSortID;                               //�ö����е�Ŀ�Ķ��������
	unsigned int         nSourceSortID;                             //�ö����е�Դ���������
    unsigned int         nStreamType;                               //��������
	unsigned int         nResved1;                                  //����1
	char                 strResved2[EV9000_SHORT_STRING_LEN];       //����2 
}EV9000_PollActionConfig, *LPEV9000_PollActionConfig;

//Ԥ��
typedef struct EV9000_PlanConfig
{
	unsigned int         nID;                                       //��¼���
	char                 strPlanName[EV9000_NORMAL_STRING_LEN];     //Ԥ������
	unsigned int         nScheduledRun;                             //�Ƿ�ʱִ��
	unsigned int         nStartTime;                                //��ʼʱ��
	unsigned int         nUserID;                                   //�û�ID
	unsigned int         nType;                                     //Ԥ������:ǰ̨Ԥ�����̨Ԥ��
	unsigned int         nUserLevel;                                //����ִ���û�����
	unsigned int         nResved1;                                  //����1
	char                 strResved2[EV9000_SHORT_STRING_LEN];       //����2 
}EV9000_PlanConfig, *LPEV9000_PlanConfig;

// Ԥ����������
typedef enum
{
	PLANACTION_PC = 0,
		PLANACTION_TVWALL,
		PLANACTION_PRESET,
}PLANACTION_TYPE;

//Ԥ������
typedef struct EV9000_PlanActionConfig
{
	unsigned int         nID;                                       //��¼���
	unsigned int         nPlanID;                                   //Ԥ�����
	unsigned int         nType;                                     //��������
	unsigned int         nDeviceIndex;                              //�豸ID
	unsigned int         nDestID;                                   //Ŀ���豸���
	unsigned int         nScreenID;                                 //�ͻ��˻�����
	unsigned int         nControlData;                              //����ֵ
	unsigned int         nStreamType;                               //��������
	unsigned int         nResved1;                                  //����1
	char                 strResved2[EV9000_SHORT_STRING_LEN];       //����2 
}EV9000_PlanActionConfig, *LPEV9000_PlanActionConfig;

//Ѳ��
typedef struct EV9000_CruiseConfig
{
	unsigned int         nID;                                       //��¼���
	char                 strCruiseName[EV9000_NORMAL_STRING_LEN];   //Ѳ������
	unsigned int         nStartTime;                                //��ʼʱ��
	unsigned int         nDurationTime;                             //����ʱ��
	unsigned int         nDeviceIndex;                              //�豸����
	char                 strDeviceID[EV9000_IDCODE_LEN];            //�豸ID
	unsigned int         nResved1;                                  //����1
	char                 strResved2[EV9000_SHORT_STRING_LEN];       //����2 
}EV9000_CruiseConfig, *LPEV9000_CruiseConfig;

//Ѳ������
typedef struct EV9000_CruiseActionConfig
{
	unsigned int         nID;                                       //��¼���
	unsigned int         nCruiseID;                                 //Ѳ�����
	int                  nDeviceIndex;                              //�豸ID
	unsigned int         nPresetID;                                 //Ԥ��λ���
	unsigned int         nLiveTime;                                 //����ʱ��
	unsigned int         nResved1;                                  //����1
	char                 strResved2[EV9000_SHORT_STRING_LEN];       //����2 
}EV9000_CruiseActionConfig, *LPEV9000_CruiseActionConfig;

//Ԥ��λ
typedef struct EV9000_PresetConfig
{
	unsigned int         nID;                                       //��¼���
	int                  nDeviceIndex;                              //�豸ID(ϵͳ�ڲ�ʹ��,�û�������)
	unsigned int         nPresetID;                                 //Ԥ��λ���
	char                 strPresetName[EV9000_NORMAL_STRING_LEN];   //Ԥ��λ����
	unsigned int         nResved1;                                  //����1
	char                 strResved2[EV9000_SHORT_STRING_LEN];       //����2 
}EV9000_PresetConfig, *LPEV9000_PresetConfig;

//����������
typedef struct EV9000_AlarmDeployment
{
	char                 strUUID[EV9000_SHORT_STRING_LEN];           
	unsigned int         nLogicDeviceIndex;                         //�߼��豸ID
	unsigned int         nDayOfWeek;                                //��������
	unsigned int         nBeginTime;                                //��ʼʱ��
	unsigned int         nEndTime;                                  //����ʱ��
	unsigned int         nResved1;                                  //����1
	char                 strResved2[EV9000_SHORT_STRING_LEN];       //����2 
}EV9000_AlarmDeployment, *LPEV9000_AlarmDeployment;

//������־��
typedef struct EV9000_FaultRecord
{
	unsigned int          nID;                                      //ID
	unsigned int          nLogicDeviceIndex;                        //�߼��豸ID
	unsigned int          nType;                                    //�������
	unsigned int          nLevel;                                   //���ϼ���
	unsigned int          nBegintime;                               //��ʼʱ��
	unsigned int          nEndTime;                                 //����ʱ��
	char                  strInfo[EV9000_LONG_STRING_LEN];          //��ϸ˵��
	unsigned int          nResved1;                                  //����1
	char                  strResved2[EV9000_SHORT_STRING_LEN];       //����2 
}EV9000_FaultRecord, *LPEV9000_FaultRecord;
#endif
