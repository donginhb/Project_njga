// ***************************************************************
//  version:  1.0.0.0
//  -------------------------------------------------------------
//  
//  -------------------------------------------------------------
// ***************************************************************
// ý�����س��������ݽṹ����
// ***************************************************************

#ifndef MGWPUBLICDATA_H
#define MGWPUBLICDATA_H

#if defined(_WIN32) //windows

#ifdef DECODE_EXPORTS  
#define EV9000DECODER_API   __declspec(dllexport)
#else  
#define EV9000DECODER_API    __declspec(dllimport) 
#endif

#elif defined(__linux__)

#define TRACE printf
#include "Log/CLog.h"
#define MEMORY_DELETE(x)  if(x) {delete x; x=NULL;}              //delete �ڴ�
#define MEMORY_DELETE_EX(x)  if(x){ delete[] x; x=NULL;}         //delete �ڴ�����
#define EV9000DECODER_API    extern "C"
typedef     unsigned short   USHORT;
typedef     unsigned int     DWORD;
typedef     unsigned short   WORD;
typedef     unsigned char    BYTE;
//#define     BOOL             int;
typedef     unsigned int     UINT;
typedef     void*            LPVOID;
typedef     void*            HANDLE;
typedef     unsigned int     UINT64;
typedef     int              BOOL;
typedef     long long        LONGLONG;
typedef     char*            LPSTR;
#ifndef     TRUE
#define     TRUE    1
#endif
#ifndef     FALSE
#define     FALSE   0
#endif
#ifndef     NULL
#define	    NULL    0
#endif

#define __stdcall
#define CALLBACK

#ifndef __HWND_defined
#define __HWND_defined
typedef void* HWND; 
#else 
typedef void* HWND; 
#endif
#include <string.h>
#endif

#ifdef WIN32
#include "../public/Include/IComNetSdk.h"
#endif
#include <sstream>
#include <iostream>
#include <string>
using namespace std;

//////////////////////////////////////////////////////////////////////////
//------------����ΪEV9000 mgw���ݶ���------------------------------------
//////////////////////////////////////////////////////////////////////////

#define REFRESH_DEV_TIME    (1*60)          //ˢ���豸ʱ�� ��λs
#define MAX_DELMGWLOG_SPAN  (1000*60*60*1)    //ɾ��mgw�ռ�ʱ����
#define MAX_SO_NUM         4                //ÿ�η�������λ��
#define MAX_EXCEPTION_NUM  100               //����쳣���� 
#define PRINT_TIME_SPAN    (30*1000)        //��ӡʱ����
#if defined(__linux__)     
#define MAX_PATH           260
#endif
#define MAX_RINGBUF_LEN    (1024*1024*3)
#define REC_RINGBUF_LEN    (1024*1024*5)    //¼��4M����
#define MAX_PAYLOAD_LEN    1400             //�����س���
#define MAX_READ_LEN       (1024*100)
//#define MAX_ENCBUF_LEN     (1024*1024)
#define MAX_SENDBUF_LEN    (1400*40)
//#define PS_BUF_LEN         (256*1024)
#define MAX_CHNAME_LEN     100
#define MAX_SQL_LEN        1024
#define MAX_CH_NUM         16
#define MAX_SIP_TIMEOUT    30
#define MIN_SIP_TIMEOUT    2
#define NORMAL_STARTUP_TIME (10*60)         //��������ʱ�䵥λs
#define MAX_SEND_COUNT     20000
#define PACKETMAXSIZE      1460
#define MAX_DEV_NUMBER     128              //����豸�� 

//*****¼��ط���ؽṹ******
#define MAX_REC_QUERY_NUM       300
#define MAX_PLAYBACK_COUNT      100             //���֧�ֻط��� 
#define MAX_RECORD_DATABUF_LEN  (1024*1024*4)   //¼��طš�¼��洢�����С �мǼ�����()
#define	MAX_RECFILE_LENGTH		48

#define	EV9000APP_VIDEOENCODE_TYPE_YUV        0x1000  //��������ʽYUV
#define	EV9000APP_VIDEOENCODE_TYPE_HIKPS      0x1001  //����ps
#define	EV9000APP_VIDEOENCODE_TYPE_UNKNOW     0X2000  //δ֪����

//Rtp
#define MAX_RTP_PAYLOAD_LEN   1400        //������󳤶���MTU������luminsoft������˳���1400��rtp��ͷ��12����������payloadΪ1388��
#define RTP_HEAD_LEN          12

#define EV9000_IDCODE_LEN             20+4    /* ͳһ������󳤶� */
#define EV9000_SHORT_STRING_LEN       32+4    /* ���ַ������� */
#define EV9000_NORMAL_STRING_LEN      64+4    /* һ���ַ������� */
#define EV9000_LONG_STRING_LEN        128+4   /* ���ַ������� */
#define EV9000_MAX_CHANNDELID         1024   /* ����豸ͨ���� */

typedef   int   EDVCONHANDLE;    //���Ӿ��
typedef   int   EDVOPENHANDLE;    //�򿪾��

//���������
#define  PTZCMD_28181_LEN    8
#define  PTZCMD_WIS_LEN      7
#define  PTZCMD_WIS_AUTOZOOMIN_LEN 28

#define EV9K_STOP        0x00
#define EV9K_RIGHT       0x01
#define EV9K_LEFT        0x02
#define EV9K_DOWN        0x04
#define EV9K_UP          0x08  
#define EV9K_DOWN_RIGHT  (EV9K_DOWN|EV9K_RIGHT)
#define EV9K_DOWN_LEFT   (EV9K_DOWN|EV9K_LEFT)
#define EV9K_UP_RIGHT    (EV9K_UP|EV9K_RIGHT)  
#define EV9K_UP_LEFT     (EV9K_UP|EV9K_LEFT) 
#define EV9K_ZOOMIN      0x10
#define EV9K_ZOOMOUT     0x20
#define EV9K_FOCUS_FAR   0x41
#define EV9K_FOCUS_NEAR  0x42
#define EV9K_IRIS_OPEN   0x44
#define EV9K_IRIS_CLOSE  0x48
#define EV9K_SET_PRESET  0x81
#define EV9K_GOTO_PRESET 0x82
#define EV9K_CLE_PRESET  0x83

#ifndef WIN32  //linux

#define USER_NAME_LENGHT  40
typedef enum
{
	DEVICE_TYPE_IPC = 3,
		DEVICE_TYPE_HIK = 100,
		DEVICE_TYPE_DV,
		DEVICE_TYPE_DAH,
		DEVICE_TYPE_WISCOM_DVR,
		DEVICE_TYPE_YAAN,
		DEVICE_TYPE_XYT,
		DEVICE_TYPE_MINSU,
		DEVICE_TYPE_IFT,
		DEVICE_TYPE_YAAN_IPC,
		DEVICE_TYPE_HH,
		DEVICE_TYPE_EV8K,
		DEVICE_TYPE_CIPC,
		DEVICE_TYPE_JY,
		DEVICE_TYPE_RESEEK,     //����
		DEVICE_TYPE_LSHI,       //��ʿ
		DEVICE_TYPE_GMI,        //����
		DEVICE_TYPE_DFWL = 117,       //��������
		DEVICE_TYPE_CH,        //����
		DEVICE_TYPE_KEDA,      //�´�
		DEVICE_TYPE_KEDEDNNS,  //�´���϶�˹
		DEVICE_TYPE_KEDFRONTREC,   //�´�-ǰ��¼��
        DEVICE_TYPE_LB,            //����
		DEVICE_TYPE_BOSCH =124,
		DEVICE_TYPE_INVALID,
}DEVICE_TYPE;

/**********************Onvif��̨�������� begin*************************/
#define LIGHT_PWRON		2	/* ��ͨ�ƹ��Դ */
#define WIPER_PWRON		3	/* ��ͨ��ˢ���� */
#define FAN_PWRON		4	/* ��ͨ���ȿ��� */
#define HEATER_PWRON	5	/* ��ͨ���������� */
#define AUX_PWRON1		6	/* ��ͨ�����豸���� */
#define AUX_PWRON2		7	/* ��ͨ�����豸���� */
#define SET_PRESET		8	/* ����Ԥ�õ� */
#define CLE_PRESET		9	/* ���Ԥ�õ� */

#define ZOOM_IN			11	/* �������ٶ�SS���(���ʱ��) */
#define ZOOM_OUT		12	/* �������ٶ�SS��С(���ʱ�С) */
#define FOCUS_NEAR      13  /* �������ٶ�SSǰ�� */
#define FOCUS_FAR       14  /* �������ٶ�SS��� */
#define IRIS_OPEN       15  /* ��Ȧ���ٶ�SS���� */
#define IRIS_CLOSE      16  /* ��Ȧ���ٶ�SS��С */

#define TILT_UP			21	/* ��̨��SS���ٶ����� */
#define TILT_DOWN		22	/* ��̨��SS���ٶ��¸� */
#define PAN_LEFT		23	/* ��̨��SS���ٶ���ת */
#define PAN_RIGHT		24	/* ��̨��SS���ٶ���ת */
#define UP_LEFT			25	/* ��̨��SS���ٶ���������ת */
#define UP_RIGHT		26	/* ��̨��SS���ٶ���������ת */
#define DOWN_LEFT		27	/* ��̨��SS���ٶ��¸�����ת */
#define DOWN_RIGHT		28	/* ��̨��SS���ٶ��¸�����ת */
#define PAN_AUTO		29	/* ��̨��SS���ٶ������Զ�ɨ�� */

#define FILL_PRE_SEQ	30	/* ��Ԥ�õ����Ѳ������ */
#define SET_SEQ_DWELL	31	/* ����Ѳ����ͣ��ʱ�� */
#define SET_SEQ_SPEED	32	/* ����Ѳ���ٶ� */
#define CLE_PRE_SEQ		33	/* ��Ԥ�õ��Ѳ��������ɾ�� */
#define STA_MEM_CRUISE	34	/* ��ʼ��¼�켣 */
#define STO_MEM_CRUISE	35	/* ֹͣ��¼�켣 */
#define RUN_CRUISE		36	/* ��ʼ�켣 */
#define RUN_SEQ			37	/* ��ʼѲ�� */
#define STOP_SEQ		38	/* ֹͣѲ�� */
#define GOTO_PRESET		39	/* ����ת��Ԥ�õ� */

/**********************Onvif��̨�������� end*************************/
typedef enum
{
	CHANNELITEM_STATE_OPEN = 0,                //��ͼ��ɹ�
		CHANNELITEM_STATE_CLOSE,               //ͼ��ر�
		CHANNELITEM_STATE_NODATA,              //ͼ����������,������������(�豸�ϱ����������)
		CHANNELITEM_STATE_DEVICEERR,           //ͨ���豸����
		CHANNELITEM_STATE_PAUSE,               //��ͣ(¼����)
		CHANNELITEM_STATE_ONLINE,              //����
		CHANNELITEM_STATE_OFFLINE,             //����
		CHANNELITEM_STATE_LOSTVIDEO,           //��Ƶ��ʧ
}CHANNELITEM_STATE;
#endif

//**************EDVHEAD���� begin******************

typedef union  
{
	struct
	{
		//unsigned short wRet;
		unsigned short wResved1;    /*����*/
		unsigned short wResved2;    /*����*/
		//unsigned short wResved3;    /*����*/
		//unsigned short wResved4;    /*����*/
	} NORAML;
	struct
	{
		//unsigned short wRet;
		//unsigned short wResved1;		/*����*/
		unsigned int   sizeseek	;		/*����*/
		//unsigned short index;		/*����*/
		//unsigned short wResved4;		/*����*/
	} FILEPALY;
	struct  
	{
		unsigned int   FrameID;			/*֡���*/
		//unsigned short wResved3;    /*����*/
		//unsigned short wResved4;    /*����*/
	}FRAMEID;
	struct  
	{
		unsigned short   UserID;			/*�û�ID��*/
		unsigned short	wResved2;    /*����*/
		//unsigned short wResved3;    /*����*/
		//unsigned short wResved4;    /*����*/
	}USERID;
	struct  
	{
		unsigned int HDFreeSpace;		/*Ӳ��ʣ��ռ�*/
		//unsigned short wResved3;    /*����*/
		//unsigned short wResved4;    /*����*/
	}HDSPACE;
}HEADUNION;

typedef struct tag_EDVHEAD
{
	unsigned short wVersions;   /*��ʶ��ͬ�汾���ӵ�λ����λ��ÿλ��1��֧�ָð汾��0����֧��*/
	unsigned short wPayLoadLen; /*�غɳ���*/
	unsigned int   dwEvent;   /*Word32  �¼�����*/
	HEADUNION		Comm;
	unsigned int	dwComonID;/*����ID*/
	char          dummy[16]; /*���*/
	unsigned char ucPayLoad[MAX_PAYLOAD_LEN]; /*BYTE[1400]  Payload����*/
}EDVHEAD_EX,*PEDVHEAD_EX;

#define	EDV_HEAD_LEN_EX            (32)
#define EDV_HEAD_LEN	EDV_HEAD_LEN_EX
#define EDVHEAD EDVHEAD_EX
#define PEDVHEAD PEDVHEAD_EX
#define	COMM_MSG_HEAD_T COMM_MSG_HEAD_EX_T
#define JXBTB  1	/*���߱�ͬ��*/
typedef struct
{
    unsigned short  wVersions;      /* �汾�� */
    unsigned short  wPayLoadLen;    /* payLoad����*/
    unsigned int  dwEvent;    /* �¼��� */
    unsigned short  wResved1;       /* ���� */
    unsigned short  wResved2;       /* ���� */
	unsigned int	dwComonID;/*����ID*/
	char          dummy[16]; /*���*/
}COMM_MSG_HEAD_EX_T;

//**************EDVHEAD���� end******************
//ͨ��״̬
typedef struct _UnitInfo{
	string        strLogicDeviceID;        //ID
	string        strChannelName;          //ͨ������
	string        strDeviceIP;             //�����豸IP
	//״̬
	DWORD         dwTime;                  //��ʼ����ʱ��
	LONGLONG      llRcvByteCount;             //�������ݳ���
	LONGLONG      llSendByteCount;             //�������ݳ���
	LONGLONG      llLastSendByteCount;         //�ϴ�ȡ����ʱ���ݳ���
	int           iSrcPort;                //���ض˿�
	string        strDestIP;               //tsu��ַ
	int           iDestPort;               //tsu�˿�
	CHANNELITEM_STATE eChannelState;       //ͨ��״̬
	_UnitInfo()
	{
		strLogicDeviceID ="";
		strChannelName ="";
		strDeviceIP ="";
		dwTime =0;
		llRcvByteCount = 0;
		llSendByteCount =0;
		llLastSendByteCount =0;
		eChannelState = CHANNELITEM_STATE_ONLINE; 
	}
}UNITINFO;

//���������
typedef enum
{
	EV9000_CAM_TYPE_BALL,    //���
		EV9000_CAM_TYPE_GUN  //ǹ��
}EV9000_CAM_TYPE;

//mgw��������
typedef enum
{
	MEDIAITEM_TYPE_NET = 0,
		MEDIAITEM_TYPE_FVS,
		MEDIAITEM_TYPE_HD,
		MEDIAITEM_TYPE_DEVICE,		
		MEDIAITEM_TYPE_UNIT,
		MEDIAITEM_TYPE_DI,
		MEDIAITEM_TYPE_DO,
		MEDIAITEM_TYPE_COM,
		MEDIAITEM_TYPE_REPORT,
		MEDIAITEM_TYPE_PLAYBACK,
		MEDIAITEM_TYPE_STORAGESTREAM,
		MEDIAITEM_TYPE_DEVICEINFO,
		MEDIAITEM_TYPE_INVALID,
		MEDIAITEM_TYPE_ROOT,
}MEDIAITEM_TYPE;

//������
//�����ֶΣ��Ƿ��ܿ���

//��ӦUNGBPhyDeviceConfig��
typedef struct _ungb_devconfigrec{
	BOOL   bUsed;
	int    iID;
	string strDeviceName;
	int    iDeviceType;
	string strDeviceIP;
	int    iDevicePort;
	string strUseName;
	string strPassword;
	int    iRecordType;
	int    iReserved1;
	string strReserved2;
	_ungb_devconfigrec()
	{
		bUsed =TRUE;
	}
}UNGB_DEVCONFIGREC;

//��ӦUNGBPhyDeviceChannelConfig��
typedef struct _ungb_chconfigrec{
	BOOL   bUsed;
	int    iID;
	int    iDeviceIndex;        //�����豸����
	string strLogicDeviceID;    //ͨ��ID
	string strChannelName;      //ͨ����
	string strChannelIp;        //ͨ������IP
	int    iChannelPort;        //ͨ�������˿�
    string strMapChannel;     //ͨ����ʶ
	int    iNeedCodec;          //�Ƿ���Ҫ�����
	int    iStreamType;         //�������� 0 ps  2 h264  4 hik001  5 hik002
	int    iCamType;            //ǹ���������  0 ��� 1 ǹ��
	int    iReserved1;
	string strReserved2;
    _ungb_chconfigrec()
	{
		bUsed = TRUE;
		iNeedCodec =0;
		strChannelName ="";
		iStreamType =2;
		iCamType =1;       //ǹ��
		strChannelIp = "";
		iChannelPort = 554;     //Ĭ��rstp����˿�
	}
}UNGB_CHCONFIGREC;

//SIP��Ϣ����
typedef enum{
	SIP_MSG_MSG,
		SIP_MSG_INVITE,
		SIP_MSG_BYE,
        SIP_MSG_CANCEL,
		SIP_MSG_EXPIRE,
		SIP_MSG_INFORM
}SIPMSGTYPE;

//SIP��Ϣ�ȼ�
typedef enum{
	SIP_LEVEL_COM,      //��ͨ�ȼ�0-1-2  ����Խ��ȼ�Խ��
		SIP_LEVEL_1,  //�ȼ�1
		SIP_LEVEL_2
}SIPMSGLEVEL;

//ͨ��������
typedef enum{
	UNIT_OPEN_COM,      //������
	UNIT_OPEN_EMPTY     //�մ� 
}UNITOPENTYPE;

//invite��Ϣ����
typedef enum{
	INVITE_PLAY,         //ʵʱ��Ƶ
		INVITE_PLAYBACK  //�ط�
}INVITE_TYPE;

//�ص�Sip��Ϣ�ṹ��
typedef struct _cbsipmsg{
	SIPMSGLEVEL   eSipMsgLevel;
	SIPMSGTYPE    eSipMsgType;       
	string        strcaller_id;
	string        strcallee_id;
	string        strcall_id;
	string        strMsg;
	int           nMsgLen;
	int           idialog_index;
    _cbsipmsg()
	{
		eSipMsgLevel = SIP_LEVEL_COM;  //Ĭ��Ϊ0
		//      strcaller_id="";
		// 		strMsg="";
		nMsgLen =0;
		idialog_index =-1;
	}
}CBSIPMSG,*PCBSIPMSG;

//������������
typedef struct STREAM_CON_INFO
{
	string     strLogicDeviceID;
	DWORD      dwSrcIP;
	int        iSrcPort;
	DWORD      dwDestIP;
	int        iDestPort;
	BOOL       bConStatus;
	CBSIPMSG   stSipContext;
	string     strDestIP;
	int        iDeviceType;
	int        iStartTime;   //¼��ʼʱ��
	int        iStopTime;    //¼�����ʱ��
	int        iPlayTime;    //¼�񲥷ſ�ʼʱ��
	STREAM_CON_INFO()
	{
		iSrcPort =0;
		iDestPort =0;
		bConStatus = FALSE;
		strDestIP = "";
		iDeviceType = 0;
		iStartTime = 0;
		iStopTime = 0;
		iPlayTime = 0;
	}
	void Clear()
	{
		iSrcPort =0;
		iDestPort =0;
		bConStatus = FALSE;
		strDestIP = "";
		iStartTime = 0;
		iStopTime = 0;
		iPlayTime = 0;
	}
}STREAM_CON_INFO, *LPSTREAM_CON_INFO;

//�������Ϣ
typedef struct _codecinfo
{
	int   iDeviceType;      //�豸����
	BOOL  bNeedCodec;
	int   iStreamType;
	string strLogicID;       //ͨ���߼�ID
	int   nBitRate;
	int   nIFrameRate;
	int   nDataType;        //0 ��Ƶ 1 ¼��
	_codecinfo()
	{
		nDataType = 0;  //Ĭ����Ƶ
		iDeviceType = 0;
		bNeedCodec = FALSE;
		iStreamType =0;
		strLogicID ="";
		nBitRate = 2*1024*1024; //Ĭ��
	    nIFrameRate = 50;       //Ĭ��50
	}
}CODECINFO;

#ifdef WIN32
//YUV����
typedef struct _YuvPara{
	int         index;	//����
	int         lenth;	//֡��
	int         read;   //��ȡλ��
	SYSTEMTIME  time;  //֡ʱ��
	BYTE        streamtype;  //ͼ���ʽ
	DWORD       dwFlag;   //����һЩ��ʶ
	int         nWidth;   //֡��
	int         nHeight;  //֡��
	int         nFrameRate;  //֡�� 
}YUVPARA;
#endif

//Rtp���Ͳ���
typedef struct _rtppara{
	unsigned short portbase;  //�����˿�
	unsigned short destport;  //Ŀ�Ķ˿�
	//uint32_t destip;	      //Ŀ��IP
	string ipstr;             //Ŀ��IP
	string strLogicDeviceID;  //ͨ��ID   //added by chenyu 131024
	string strChannelName;    //ͨ����
    _rtppara()
	{
		portbase =0;
		destport =0;
		strLogicDeviceID ="";
		strChannelName ="";
	}
}RTP_PARA;

//������Ϣ����
typedef enum{
    SYS_CFG_LOCALIP_INTERCOM,  //�����ڲ�ͨѶIP ����(1.�豸�� 2.��Ƶ��)˳���ȡ��ַ
		SYS_CFG_LOCALIP_IN,    //�豸��
		SYS_CFG_LOCALIP_OUT,   //��Ƶ��
		SYS_CFG_CMSIP,
		SYS_CFG_DBIP,
		SYS_CFG_DBPATH,
		SYS_CFG_DBPORT,
		SYS_CFG_MGWDEVID,
		SYS_CFG_USERNAME,
		SYS_CFG_USERPWD,
		SYS_CFG_USERID,
		SYS_CFG_SERVERID,
        SYS_CFG_SERVERSIPPORT,
        SYS_CFG_SIPREFRESHTIME,     //sipˢ��ʱ��
		SYS_CFG_RECID,              //��¼ID
        SYS_CFG_CONSENDPACKLEN      //����������
}SYS_CFG_TYPE;

//#ifndef WIS
//Rtp��ؽṹ����
typedef signed char int8_t;
typedef unsigned char   uint8_t;
typedef short  int16_t;
typedef unsigned short  uint16_t;
typedef int  int32_t;
typedef unsigned   uint32_t;
//#endif

struct rtp_header_t {
	/* byte 0 */
	uint8_t csrc_len:4;     /* expect 0 */
	uint8_t extension:1;    /* expect 1, see RTP_OP below */
	uint8_t padding:1;      /* expect 0 */
	uint8_t version:2;      /* expect 2 */
	/* byte 1 */
	uint8_t payloadtype:7;  /* RTP_PAYLOAD_RTSP */
	uint8_t marker:1;       /* expect 1 */
	/* bytes 2,3 */
	uint16_t    seq_no;
	/* bytes 4-7 */
	uint32_t    timestamp;
	/* bytes 8-11 */
	uint32_t    ssrc;       /* stream number is used here. */
};

typedef struct
{
	rtp_header_t rtphead;
	char data[MAX_RTP_PAYLOAD_LEN];
}RTPPACKET;

typedef enum
{
	SHOW_CFG_TYPE_UNIT = 0,
		SHOW_CFG_TYPE_DI,
		SHOW_CFG_TYPE_DO,
		SHOW_CFG_TYPE_COM,
		SHOW_CFG_TYPE_DEVICE,
}SHOW_CFG_TYPE;

typedef struct MEDIA_DEVICE_TABLE 
{
	int nDeviceID;
	int nStationIP;
	DEVICE_TYPE eDeviceType;
	int nDeviceIP;
	int nDevicePort;
	char sUserName[USER_NAME_LENGHT];
	char sPassword[USER_NAME_LENGHT];
	unsigned char sResved1[4];
	unsigned char sResved2[32];
}MEDIA_DEVICE_TABLE, *LPMEDIA_DEVICE_TABLE;

typedef struct MEDIA_COM_TABLE 
{
	int nComID;
	int StationIP;
	int nComType;
	int nDeviceID;
	int nDeviceComID;
	unsigned char sResved1[4];
	unsigned char sResved2[32];
}MEDIA_COM_TABLE, *LPMEDIA_COM_TABLE;

#ifdef WIN32
//�豸״̬�ṹ
typedef struct DEVICESTATE_INFO
{
	int                      nDeviceID;         //�豸ID
	DEVICEITEM_STATE         eDeviceState;      //�豸״̬
	unsigned char		     byResved1[4];      //����1
	unsigned char		     byResved2[32];     //����2
}DEVICESTATE_INFO, *LPDEVICESTATE_INFO;
#endif

//ʵʱ��״̬�ṹ
typedef struct REALSTREAM_INFO
{
	int                      nUnitID;           //UNIT ID
	CHANNELITEM_STATE        eChannelState;     //��״̬
	LONGLONG                 llSendPackCount;   //������
	unsigned int             nBitRate;          //ƽ������(kbps)
	unsigned int             nMaxBitRate;       //�������(kbps)
#ifdef WIN32
	SYSTEMTIME               sMaxBitRateTime;   //������ʲ���ʱ��
#endif
	unsigned char		     byResved1[4];      //����1
	unsigned char		     byResved2[32];     //����2
	REALSTREAM_INFO()
	{
		eChannelState = CHANNELITEM_STATE_CLOSE;   //�ر�
	}
	void clear()
	{
		;
	}
}REALSTREAM_INFO, *LPREALSTREAM_INFO;

//¼��ط�״̬�ṹ
typedef struct RECORDSTREAM_INFO
{
	int                      nPlayBackID;       //�������(0-31)   ͬһ��Unit���ܲ��Ŷ�·
	CHANNELITEM_STATE        eChannelState;     //��״̬
	unsigned short           usRate;            //����
	LONGLONG                 llSendPackCount;   //������
	unsigned char		     byResved1[4];      //����1
	unsigned char		     byResved2[32];     //����2
}RECORDSTREAM_INFO, *LPRECORDSTREAM_INFO;

//¼���ϱ�״̬�ṹ
typedef struct REPORT_INFO
{
	int                      nUnitID;           //�����UNIT ID
	unsigned int             nSuccessReport;    //�ϱ��Ƿ�ɹ�   0�ɹ�
	char                     sFileName[MAX_PATH];     //¼����
	unsigned char		     byResved1[4];      //����1
	unsigned char		     byResved2[32];     //����2
}REPORT_INFO, *LPREPORT_INFO;

#ifdef WIN32
//�洢״̬�ṹ
typedef struct STORAGESTREAM_INFO
{
	int                      nUnitID;           //UNIT ID
	CHANNELITEM_STATE        eChannelState;     //��״̬
	unsigned int             nBitRate;          //ƽ������(kbps)
	unsigned int             nMaxBitRate;       //�������(kbps)
	SYSTEMTIME               sMaxBitRateTime;   //������ʲ���ʱ��
	char                     sFileName[MAX_PATH];     //¼����	
	unsigned char		     byResved1[4];      //����1
	unsigned char		     byResved2[32];     //����2
}STORAGESTREAM_INFO, *LPSTORAGESTREAM_INFO;
#endif

template<class TOUT, class TIN>
TOUT basic_cast(TIN inData)
{
    TOUT outData;
    stringstream ss;
    ss << inData;
	//    ss.clear();
    ss >> outData;
    return outData;
}

//ת��Ϊstring
static string CovertState2str(CHANNELITEM_STATE eChannelState)
{
	string strChState="OFF";
    //ӳ���ϵ��
	switch (eChannelState)
	{
	case CHANNELITEM_STATE_ONLINE:  
	case CHANNELITEM_STATE_OPEN:
	case CHANNELITEM_STATE_CLOSE:   //�򿪡��ر�
		{
			strChState = "ON";
		}
		break;
	case CHANNELITEM_STATE_LOSTVIDEO:
	case CHANNELITEM_STATE_NODATA:
	case CHANNELITEM_STATE_DEVICEERR:  //ͨ������
		{
			strChState = "NOVIDEO";
		}
		break;
	case CHANNELITEM_STATE_OFFLINE:
		{
			strChState = "OFF";
		}
		break;
	default:
		{
			strChState = "OFF";
		}
		break;
	}	
	return strChState;
}

#ifndef WIN32   //linux
//�����豸�ṹ
typedef struct CONNECT_DEVICE_INFO
{
	DEVICE_TYPE             eDeviceType;         //�豸����
	unsigned int            nDeviceIP;          //�豸IP
	unsigned int            nDevicePort;        //�˿�
	char                    sUserName[40];            //�û���
	char                    sPassword[40];        //����
	unsigned char		    byResved1[4];       //����1
	unsigned char		    byResved2[32];       //����2
}CONNECT_DEVICE_INFO, *LPCONNECT_DEVICE_INFO;

#if defined(__linux__)
//��������������
typedef enum
{
	EV9000_CONNECT_TYPE_28181 = 0,
}EV9000_CONNECT_TYPE;
//��¼��Ϣ
typedef struct EV9000_LOGININFO
{
	EV9000_CONNECT_TYPE  eConnectType;                                    //��������������
	char                 sServerIP[EV9000_NORMAL_STRING_LEN];             //������IP
	unsigned int         nServerPort;                                     //�������˿�
	char                 sUserName[EV9000_NORMAL_STRING_LEN];             //�û���
	char                 sUserPwd[EV9000_NORMAL_STRING_LEN];              //����
	char                 sServerID[EV9000_NORMAL_STRING_LEN];             //���������
	char                 sUserID[EV9000_NORMAL_STRING_LEN];               //�û����
	char                 sLocalIP[EV9000_NORMAL_STRING_LEN];              //����IP
	int                  nDigital;                                        //�Ƿ�ʹ������֤���¼ 0 ��ʹ�� 1 ʹ��(�ݲ�֧��)
	char                 sReserved[EV9000_NORMAL_STRING_LEN];
	int                  nReserved;
}EV9000_LOGININFO,*LPEV9000_LOGININFO;
#endif

// ��װ message ��Ϣ���ͽṹ
typedef struct SIP_MSG_INFO
{
	char CallerID[EV9000_NORMAL_STRING_LEN];
	char CalleedID[EV9000_NORMAL_STRING_LEN];
	char* pMsg;
	int   nMsgLen;
	int   nSN;                 // message ��Ϣ���к� 
	SIP_MSG_INFO()
	{
		memset(CallerID,0,EV9000_NORMAL_STRING_LEN);
		memset(CalleedID,0,EV9000_NORMAL_STRING_LEN);
		pMsg=NULL;
		nMsgLen=0;
		nSN=0;
	}
}SIP_MSG_INFO,*LPSIP_MSG_INFO;

#include <time.h>
// ������ϵͳ���������ĺ�������tick��
static unsigned long GetTickCount()
{
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);

    return (ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}
#include <ctype.h>
static char* strupr(char* s)
{
	//assert(s != (void*)0);
	while(*s)
	{
		*s = toupper((unsigned char)*s);
		s++;
	}
	return s;
}

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
static int iptoint( const char *ip )
{
	return ntohl( inet_addr( ip ) );
}
//void inttoip( int ip_num, char *ip )
//{
//	strcpy( ip, (char*)inet_ntoa( htonl( ip_num ) ) );
//}
#endif

#endif//end of MGWPUBLICDATA_H_DEFINE
