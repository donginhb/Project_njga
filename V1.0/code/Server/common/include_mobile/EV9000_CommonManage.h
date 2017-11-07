/***********************************************************************
* Copyright (C) 2013, WISCOM VISION TECHNOLOGY Corporation.
* 
* File Name: 	   EV9000_CommonManage.h
* Description:   ���ļ�����ƽ̨������¼����ͼ���صĽṹ��
* Others:        ��
* Version:  	   dev-v1.01.01
* Author:        �����������ּ���λ
* Date:  	       2013.05.31
* 
* History 1:  		// �޸���ʷ��¼�������޸����ڡ��޸��ߡ��޸İ汾���޸�����
*                2013.05.31    qyg    dev-v1.01.01   �״δ���  
* History 2: ?
**********************************************************************/


#ifndef EV9000_COMMON_MANAGE_H
#define EV9000_COMMON_MANAGE_H

#include "EV9000_BoardType.h"
#include "EV9000_InnerDef.h"
#include <time.h>

#define  SWAPL(x)  ((((x) & 0x000000ff) << 24) | \
                   (((x) & 0x0000ff00) << 8)  | \
                    (((x) & 0x00ff0000) >> 8)  | \
                    (((x) & 0xff000000) >> 24))

#define  SWAPS(x)  ((((x) & 0x00ff) << 8) | \
                    (((x) & 0xff00) >> 8) )


#define  CMS_PLAT_OK                0x00;
#define  CMS_PLAT_ERR               0xFFFFFFFF

#define  COMM_MSG_HEAD_LEN     0x20  
#define  HARDWARE_VERSION_LEN  0x20   /* Ӳ���汾���� */
#define  SOFTWARE_VERSION_LEN  0x20   /* ����汾���� */
#define  PLATFORM_NAME_LEN     0x20
#define  DEVICE_ID_LEN         0x14   /* ����ID���� */
#define  DB_PATH_LEN           0x100

#define  ALARM_ADDINFO_STRING_MAX  128
#define  LOG_STRING_MAX_LEN  128


#define  EV9000_VERSION_1     0x01  

/* ������������ */
#define DEVICE_NET      0x00
#define VIDEO_NET       0x01
#define STORE_NET       0x02  
#define MS_CONNECT_NET  0x03

#define COMMON_DEVICE_NET   0x04
#define COMMON_VIDEO_NET    0x05
#define SLAVE_DEVICE_NET    0x06
#define SLAVE_VIDEO_NET     0x07
#define SLAVE_MS_NET        0x08
#define DB_CONF_NET         0x0A


#define ALL_NET       0x03 
/* �����Ƿ����� */
#define NET_ENABLE     0x00
#define NET_UNENABLE ��0x01 

/* ʱ����� */
#define  LOCAL_TIMER   0x00
#define  NTP_TIMER     0x01


/* ͳһICE�ķ������ƺͶ˿ںţ����������֣�IP+�˿ںţ��˿ںŶ�Ӧ���� */

#define CMS_ICE_ID            "CmsIceService"
#define CMS_PLAT_ICE_PORT     40010   //21456

#define CMS_RUN_STATE_ICE_ID            "RunStateService"
#define CMS_PLAT_RUN_STATE_ICE_PORT    40011   //21456

#define VER_UPDATE_ICE_ID     "VerUpdateIceService"

#if defined X86
	#define VER_UPDATE_ICE_PORT   21466
#else
	#define VER_UPDATE_ICE_PORT   21456
#endif	

/*������ϢͷSocketͨ��ʱʹʹ�ü���EV8000 */
typedef struct
{
    unsigned short  wVersions;      /* �汾 */
    unsigned short  wPayLoadLen;    /* payLoad����*/
    unsigned int    dwEvent;        /* �¼���*/
    unsigned short  wResved1;       /* ����,�ڲ�ͬ����������ͬ�����{*/
    unsigned short  wResved2;       /* ���� */
    unsigned int	dwComonID;        /* ����ID */
    unsigned int    dwDestIP;      /* Ŀ��IP */ 
    char            dummy[16];               /* ��� */
   
}COMM_MSG_HEAD_T;

/* ʱ�� */
typedef struct
{
    unsigned short wYear;
    unsigned short wMonth;
    unsigned short wDay;
    unsigned short wHour;
    unsigned short wMinute;
    unsigned short wSecond;
	
}TIME_T;

/*  IP��ַ�Ľṹ�� */
typedef struct
{
    unsigned int  dwIPAddr;          /* IP��ַ */
    unsigned int  dwIPMask;          /* IP�������� */
    unsigned int  dwGetway;           /* IP ���� */
    unsigned int  dwResved1;         /* ΪIPV6Ԥ�����������IPV6�����ַ4���ֽ�ȫ��ΪIP��ַ */
    unsigned int  dwPort;
    unsigned char ucHostname[64];             /* �������� */
    
}IP_ADDR_T;

typedef struct
{
    unsigned int dwEth;
    unsigned int UsedFlag;
    IP_ADDR_T    tNetIP;    
}ETH_ATTR;

    
typedef struct
{
    unsigned char ucCmsID[24];
    unsigned int  dwMSFlag;   /* 0:�����ã�1:���� */
    ETH_ATTR tCmsVideoIP;    /* ��Ƶר����IP��ַ��Ϣ */
    ETH_ATTR tCmsDeviceIp;   /* �豸����IP��ַ��Ϣ */
    ETH_ATTR tCmsCommonVideoIP;
    ETH_ATTR tCmsCommonDeviceIP;
    ETH_ATTR tCmsSVideoIP;
    ETH_ATTR tCmsSDeviceIP;
    ETH_ATTR tCmsDBIP;
    ETH_ATTR tCmsAlarmIP;
    ETH_ATTR tCmsNtpIP;
    ETH_ATTR tCmsSDBIP;
    ETH_ATTR tCmsUsingDeviceIP;
    ETH_ATTR tCmsUsingVideoIP;
}BOARD_NET_ATTR;

#define SHELF_FAN_NUM                     6
#define SHELF_TEMPERATURE_SENSOR          4
#define SHELF_SLOT_NUM_LX                17
#define SHELF_SLOT_NUM_MX                6

typedef struct
{

    unsigned short  u16Speed[SHELF_FAN_NUM]; 
    unsigned short  u16ShelfTemperature[SHELF_TEMPERATURE_SENSOR];
    unsigned char   ucShelfSlotCpuTemp[SHELF_SLOT_NUM_LX];
    unsigned char   u8Rsv[3];  
}SHELF_TEMPERATURE_INFO;
/* ֪ͨ�࣬����OMMP-->CMS ���ݿ���첽�޸� */

#define  EVT_DB_MODIFY_CMD               0x10000
#define  EVT_DB_MODIFY_LOCALIP           0x10001  /* CMS�����������ñ�����*/
#define  EVT_DB_MODIFY_ROUTER            0x10003  /* CMS��·����Ϣ��������*/
#define  EVT_DB_MODIFY_FU_GROUP_CONF     0x10005  /* ǰ�˷����������*/

/* �汾�����¼� */

#define     EVT_LOADVER_REQ                     0x10000
#define     EVT_VER_GET_INFO_REQ                0x10001
#define     EVT_VER_GET_INFO_ACK                0x10002
#define     EVT_VER_DELETE_REQ                  0x10003
#define     EVT_VER_DELETE_ACK                  0x10004
#define     EVT_VER_UPDATE_REQ                  0x10005
#define     EVT_VER_UPDATE_ACK                  0x10006
#define     EVT_VER_PAYLOAD_TRAN_REQ            0x10007
#define     EVT_VER_PAYLOAD_TRAN_ACK            0x10008
#define     EVT_VER_OPERATION_ACK               0x1000a

typedef enum
{
    VERSION_TYPE_APP0   = 0,
    VERSION_TYPE_APP1   = 1,
    VERSION_TYPE_BOOT   = 2,
    VERSION_TYPE_KERNEL = 3,
    VERSION_TYPE_ROOTFS = 4,
    VERSION_TYPE_MAX    = 5

}VERSION_TYPE;

typedef enum
{
    VERSION_MAIN    = 0,
    VERSION_BACKUP  = 1
}VERSION_FLAG;
#define VERSION_STRING_LENGTH   64

/* �汾ͷ�ļ���c*/
typedef struct 
{
    unsigned short u16Sign;   /* 0x4e59 */
    unsigned short u16CRC;       /* �汾�ļ���У���*/   
    unsigned int   u32Len;        /* �汾�Ĵ�С*/
    unsigned int   stTime;       /* �汾�ļ�������ʱ��*/
    VERSION_TYPE   enVerType;          /* �汾���� */
    VERSION_FLAG   enFlag;     /*1����ʾ�ð汾Ϊ����ʹ�ð汾��0��ʾ�Ǳ��ð汾�������汾ʱ��λ0 */    
    char           s8VerStr[VERSION_STRING_LENGTH];
    char           s8Resv[44];
} TFileHeader;

/* ��ȡ�汾��Ϣ wResved1 �����Ƿ�ɹ���1:ʧ�ܣ�0�ɹ� �� 3: ���ڸ��°汾 */ 


/* ���°汾  wResved1 �����Ƿ�ɹ���1:ʧ�ܣ�0�ɹ�*/ 


/* ������ wResved1 ����ţ�wResved2��CRCֵ������ֵ�� wResved2 1:ʧ�ܣ�0�ɹ� */ 

/* �������*/

#define EVT_BOARD_CONTROL_CMD            0x30000
#define EVT_BOARD_CONTROL_RESET          0x30001    /* ������λ */




/* ƽ̨��������x40000  ��֮��Ӧ�Ĳ�ѯ���0x50000) */

#define  EVT_CONFIG_CMD      0x40000
#define  EVT_CONFIG_GET_CMD  0x50000


/*  ����ID�ŵ����û��߲�ѯ   */

#define  EVT_CONFIG_BOARD_ID  0x40001
#define  EVT_GET_BOARD_ID     0x50001

/* IP ���û��߲�ѯ */
#define  EVT_CONFIG_IP_REQ   0x40003

/* IP ��ѯ */
#define  EVT_CONFIG_GET_IP_REQ   0x50003

/* ��ַ���ýṹ */
typedef struct
{
   unsigned char wEnableFlag;  /* 0:���ø����磬���㣺��ֹ������ */
   unsigned char wIPV4orIPV6;  /* 0:IPV4, ���㣺IPV6 */
   unsigned short wBordType;    /* �������Ͳο����嶨��*/
   unsigned short wNetType;     /* ��������,�ο��������Ͷ��壻*/
   unsigned short wEthNO;       /* �˿ڱ��  */
   unsigned char  anotherName[32];
   IP_ADDR_T      tIPConfig;
}IP_CONFIG_T;


/* CMS��·������ */

/* CMS��·�����û��߲�ѯ */
#define  EVT_CONFIG_ROUTE_REQ   0x40005

/* CMS ��ѯ */
/************************************************************************/
#define  EVT_CONFIG_GET_ROUTE_REQ   0x50005
/*
   reserve1: 
   reserve2    ����·��	
*/

/************************************************************************/
/* ·�����ýṹ */
typedef struct
{
   unsigned short wUporDownFlag;   /* 0:����·������,1���������е�ע������ */
   unsigned short wPort;           /* ����·�����õĶ˿ں� */
   unsigned char  ucServerID[DEVICE_ID_LEN];
   IP_ADDR_T      tIPConfig;

}CMS_ROUTE_CONFIG_T;

/* CMS ϵͳʱ������ */
#define  EVT_CONFIG_TIME_REQ      0x40007

#define  EVT_CONFIG_GET_TIME_REQ  0x50007

typedef struct
{
    unsigned int   dwSystermTimeType;   /* ʱ����������0:�ֶ�����ʱ�䣬1������NTPʱ�� */
    TIME_T         tSystermTime;        /* �ƶ�ϵͳʱ�� */
    IP_ADDR_T      tNTPServer;          /* NTPʱ���������ַ */    

}PLATFORM_TIME_CONFIG_T;


/* ϵͳ�澯���� */

#define  EVT_CONFIG_ALARM_REQ      0x40009

#define  EVT_CONFIG_GET_ALARM_REQ  0x50009

/* ���ϸ澯���������������� */
typedef struct
{
    unsigned int   dwAlarmLevel;         /* ��Ҫ������ʾ�Ĺ��ϸ澯���� */    
    IP_ADDR_T      tAlarmServer;        /* �������͵ķ�������ַ */	     
    	
}PLATFORM_ALARM_CONFIG_T;

/* ���򵥰崦���������� */

#define EVT_CONFIG_MAX_CAPACITY_REQ 0x4000b
#define EVT_CONFIG_GET_CAPACITY_REQ 0x5000b
typedef struct
{
     unsigned short  wMaxAccessCnt;
     unsigned short  wMaxTransferCnt;
     unsigned short  wMaxStoreCnt;
     
}MAX_CAPACITY_CONFIG_T;

/* ���ݿ���Ϣ����*/

#define EVT_CONFIG_DB_REQ 0x4000c
#define EVT_CONFIG_GET_DB_REQ 0x5000c
typedef struct
{
     unsigned int    dwDBIp;
     unsigned int    dwDBPort;
     unsigned char   ucDBPath[DB_PATH_LEN];
     
}DB_CONFIG_T;

/* ��ӡ�������� */

#define EVT_CONFIG_LOGLEVEL_REQ 0x4000e

/* ��ӡ����ͨ��Reserved�Ӷ�����  */



/*  ������Ϣ������ͳ�� */

#define  EVT_GETINFO_CMD      0x60000



/*  ������Ϣ ҵ���ѯ */
#define EVT_GETINFO_SHELF_REQ  0x60001
typedef struct
{
    unsigned short  wCabinetType;     /* ��������9U�� ��  6U �� ��4U����  */
    unsigned short  wMasterSlaveFlag; /* �����Ƿ�����  */
    unsigned short  wSlotNo;          /* ��λ*/
    unsigned short  wBordType;        /* ��������0��CMS, 1:TSU, 2��ý������ 3:������ */
    	
}SHELF_BASE_INFO_T;

/*  �������������Ϣ */

#define EVT_GETINFO_STAT_CMD    0x60003

typedef struct
{
    TIME_T          tSysTime;                                   /* ϵͳʱ��  */
    unsigned int    dwCPURate;                                  /* CPU ռ���� */
    unsigned int    dwMemoryRate;                               /* �ڴ�ռ���� */
    unsigned short  wRuningTime;                                /* ����ʱ��  */
    unsigned short  wMaxCapacity;                               /* �����봦������� */
    unsigned short  wMaxStoreCapacity;                          /* ���洢��������� */
    unsigned char   aucDeviceID[DEVICE_ID_LEN];                 /*  �����ID */
}BOARD_RUNNING_INFO_T;


#define EVT_GETINFO_SERVICE_CMD    0x60005

/*  CMS  ҵ�������Ϣ */
typedef struct
{    
    unsigned short  wUserCnt;                               /* ע���û����� */             
    unsigned short  wOnlineUserCnt;                         /* �����û���*/            
    unsigned int    dwAnalogCameraCnt;                      /* ǰ��������� */             
    unsigned int    dwOnlineAnalogCameraCnt;                /* ��ǰ���ߵ������ */         
    unsigned int    dwIPCCnt;                                                        
    unsigned int    dwOnlineIPCnt;                                                   
    unsigned short  wDVRCnt;                                /* DVR��Ŀ */              
    unsigned short  wOnlineDVRCnt;                          /* ���ߵ�DVR��Ŀ */           
    unsigned short  wNvrCnt;                                                         
    unsigned short  wOnlineNvrCnt;                                                   
    unsigned short  wEncoderCnt;                            /* ��Ƶ�������� */             
    unsigned short  wOnlineEncoderCnt;                      /* ���ߵ���Ƶ������ */           
    unsigned short  wDecoderCnt;                                                     
    unsigned short  wOnlineDecoderCnt;                                               
    unsigned short  wMGWCnt;                                 /* ������Ŀ */              
    unsigned short  wOnlineMGWCnt;                                                   
    unsigned short  wTSUCnt;                                                         
    unsigned short  wOnlineTSUCnt;                                              
                                                                                     
    unsigned int    dwServiceCnt;                           /* ����ҵ���� */         
    
    unsigned short  wAlarmEncoderCnt;
    unsigned short  wOnlineAlarmCoderCnt;
    unsigned short  wAlarmDecoderCnt;
    unsigned short  wOnlineAlarmDecoderCnt;
    unsigned short  wAlarmInputDeviceCnt;
    unsigned short  wOnlineAlarmInputDeviceCnt;
    unsigned short  wAlarmOutputDeviceCnt;
    unsigned short  wOnlineAlarmOutputDeviceCnt;   
	
}BASE_SERVICE_INFO_T;  


/* TSU  */

typedef struct
{
    unsigned int dwFrontDeviceAccessCnt;
    unsigned int dwTransferCnt;
    unsigned int dwRecoderCnt;
    unsigned int dwRecoderReplayCnt;     

}TSU_SERVICE_INFO_T;

#define EVT_GETINFO_ETH_STATISTICS_CMD    0x60007

/* ����ͳ�� */
typedef struct
{
    unsigned int   dwRcvCnt;
    unsigned int   dwRcvFailCnt;    
    unsigned int   dwSendCnt;
    unsigned int   dwSendFailCnt;
    unsigned int   dwNetUseRate;
    unsigned int   dwCtrolDataCnt;
    unsigned int   dwMediaDataCnt;
}ETH_STATISTICS_T;      /* ���ڵ�����ͳ��  */


typedef struct
{
    unsigned short     wActionType;       /* ��������0:���� 1����ѯ */  
    unsigned short     wNetType;          /* ��ѯ/���õ������� */        
    ETH_STATISTICS_T   tNetStatistics;    /*    ���ڵ�����ͳ��  */
   
}NET_STATISTICS_T;



/* ���ϸ澯����  */

#define  EVT_ALARMINFO_CMD      0x70000


typedef struct
{
    unsigned int  dwAlarmIP;                                        /* ����Դ��IP��ַ */
    unsigned int  dwAlarmLevel;                                     /* ���ϸ澯���� */          
    unsigned int  dwAlarmID;                                        /*  ���ϸ澯��*/          
    unsigned int  dwAlarmSTatus;                                    /* ���ϸ澯�ϱ����ָ� */     
    TIME_T        dwAlarmTime;                                         /* ���ϸ澯���ָ���ʱ�� */      
    char          aucBoardID[20];                                     /* �������� */            
    unsigned char ucAlarmInfo[ALARM_ADDINFO_STRING_MAX];             /* �������� */           
}ALARM_INFORMATION_T;


/*��־���� */
#define  EVT_NETLOG_CMD      0x80000
typedef struct
{
    unsigned int  dwLogLevel;                                          /* ��־���� */                
    unsigned int  dwLogType;                                           /*  ��־���� */               
    IP_ADDR_T      tBoardIP;                                           /* �������͵ķ�������ַ */	         
    unsigned char ucAlarmInfo[LOG_STRING_MAX_LEN];                     /* �������� */                
}LOG_INFORMATION_T;

#define  EVT_NETLOG_TYPE_SYS          0x0002 0010                      /* ϵͳ���� */
#define  EVT_NETLOG_TYPE_ALARM        0x0002 0020                      /* ���ϱ��� */
#define  EVT_NETLOG_TYPE_OPERATION    0x0002 0030                      /* ������־ */
#define  EVT_NETLOG_TYPE_RESERVER     0x0002 00F0                      /* 0x0002 0030 ~F0 ���� */
/*��־��ӡ���� */
#define  EVT_NETLOG_LEVEL_URGENT         0X0000 0001
#define  EVT_NETLOG_LEVEL_IMPORTANT      0X0000 0002
#define  EVT_NETLOG_LEVEL_COMMON         0X0000 0003


#endif
