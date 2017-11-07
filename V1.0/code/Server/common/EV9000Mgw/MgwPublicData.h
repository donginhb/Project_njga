// ***************************************************************
//  version:  1.0.0.0
//  -------------------------------------------------------------
//  
//  -------------------------------------------------------------
// ***************************************************************
// 媒体网关程序公用数据结构定义
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
#define MEMORY_DELETE(x)  if(x) {delete x; x=NULL;}              //delete 内存
#define MEMORY_DELETE_EX(x)  if(x){ delete[] x; x=NULL;}         //delete 内存数组
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
//------------以下为EV9000 mgw数据定义------------------------------------
//////////////////////////////////////////////////////////////////////////

#define REFRESH_DEV_TIME    (1*60)          //刷新设备时间 单位s
#define MAX_DELMGWLOG_SPAN  (1000*60*60*1)    //删除mgw日记时间间隔
#define MAX_SO_NUM         4                //每次发送最大点位数
#define MAX_EXCEPTION_NUM  100               //最大异常次数 
#define PRINT_TIME_SPAN    (30*1000)        //打印时间间隔
#if defined(__linux__)     
#define MAX_PATH           260
#endif
#define MAX_RINGBUF_LEN    (1024*1024*3)
#define REC_RINGBUF_LEN    (1024*1024*5)    //录像4M缓冲
#define MAX_PAYLOAD_LEN    1400             //包负载长度
#define MAX_READ_LEN       (1024*100)
//#define MAX_ENCBUF_LEN     (1024*1024)
#define MAX_SENDBUF_LEN    (1400*40)
//#define PS_BUF_LEN         (256*1024)
#define MAX_CHNAME_LEN     100
#define MAX_SQL_LEN        1024
#define MAX_CH_NUM         16
#define MAX_SIP_TIMEOUT    30
#define MIN_SIP_TIMEOUT    2
#define NORMAL_STARTUP_TIME (10*60)         //正常启动时间单位s
#define MAX_SEND_COUNT     20000
#define PACKETMAXSIZE      1460
#define MAX_DEV_NUMBER     128              //最大设备数 

//*****录像回放相关结构******
#define MAX_REC_QUERY_NUM       300
#define MAX_PLAYBACK_COUNT      100             //最大支持回放数 
#define MAX_RECORD_DATABUF_LEN  (1024*1024*4)   //录像回放、录像存储缓存大小 切记加括号()
#define	MAX_RECFILE_LENGTH		48

#define	EV9000APP_VIDEOENCODE_TYPE_YUV        0x1000  //特殊编码格式YUV
#define	EV9000APP_VIDEOENCODE_TYPE_HIKPS      0x1001  //海康ps
#define	EV9000APP_VIDEOENCODE_TYPE_UNKNOW     0X2000  //未知类型

//Rtp
#define MAX_RTP_PAYLOAD_LEN   1400        //包的最大长度是MTU，在了luminsoft里设成了常量1400，rtp包头长12，所有最大的payload为1388。
#define RTP_HEAD_LEN          12

#define EV9000_IDCODE_LEN             20+4    /* 统一编码最大长度 */
#define EV9000_SHORT_STRING_LEN       32+4    /* 短字符串长度 */
#define EV9000_NORMAL_STRING_LEN      64+4    /* 一般字符串长度 */
#define EV9000_LONG_STRING_LEN        128+4   /* 长字符串长度 */
#define EV9000_MAX_CHANNDELID         1024   /* 最大设备通道号 */

typedef   int   EDVCONHANDLE;    //连接句柄
typedef   int   EDVOPENHANDLE;    //打开句柄

//控球命令定义
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
		DEVICE_TYPE_RESEEK,     //锐视
		DEVICE_TYPE_LSHI,       //雷士
		DEVICE_TYPE_GMI,        //杰迈
		DEVICE_TYPE_DFWL = 117,       //东方网力
		DEVICE_TYPE_CH,        //长虹
		DEVICE_TYPE_KEDA,      //柯达
		DEVICE_TYPE_KEDEDNNS,  //柯达—艾迪恩斯
		DEVICE_TYPE_KEDFRONTREC,   //柯达-前段录像
        DEVICE_TYPE_LB,            //兰博
		DEVICE_TYPE_BOSCH =124,
		DEVICE_TYPE_INVALID,
}DEVICE_TYPE;

/**********************Onvif云台控制命令 begin*************************/
#define LIGHT_PWRON		2	/* 接通灯光电源 */
#define WIPER_PWRON		3	/* 接通雨刷开关 */
#define FAN_PWRON		4	/* 接通风扇开关 */
#define HEATER_PWRON	5	/* 接通加热器开关 */
#define AUX_PWRON1		6	/* 接通辅助设备开关 */
#define AUX_PWRON2		7	/* 接通辅助设备开关 */
#define SET_PRESET		8	/* 设置预置点 */
#define CLE_PRESET		9	/* 清除预置点 */

#define ZOOM_IN			11	/* 焦距以速度SS变大(倍率变大) */
#define ZOOM_OUT		12	/* 焦距以速度SS变小(倍率变小) */
#define FOCUS_NEAR      13  /* 焦点以速度SS前调 */
#define FOCUS_FAR       14  /* 焦点以速度SS后调 */
#define IRIS_OPEN       15  /* 光圈以速度SS扩大 */
#define IRIS_CLOSE      16  /* 光圈以速度SS缩小 */

#define TILT_UP			21	/* 云台以SS的速度上仰 */
#define TILT_DOWN		22	/* 云台以SS的速度下俯 */
#define PAN_LEFT		23	/* 云台以SS的速度左转 */
#define PAN_RIGHT		24	/* 云台以SS的速度右转 */
#define UP_LEFT			25	/* 云台以SS的速度上仰和左转 */
#define UP_RIGHT		26	/* 云台以SS的速度上仰和右转 */
#define DOWN_LEFT		27	/* 云台以SS的速度下俯和左转 */
#define DOWN_RIGHT		28	/* 云台以SS的速度下俯和右转 */
#define PAN_AUTO		29	/* 云台以SS的速度左右自动扫描 */

#define FILL_PRE_SEQ	30	/* 将预置点加入巡航序列 */
#define SET_SEQ_DWELL	31	/* 设置巡航点停顿时间 */
#define SET_SEQ_SPEED	32	/* 设置巡航速度 */
#define CLE_PRE_SEQ		33	/* 将预置点从巡航序列中删除 */
#define STA_MEM_CRUISE	34	/* 开始记录轨迹 */
#define STO_MEM_CRUISE	35	/* 停止记录轨迹 */
#define RUN_CRUISE		36	/* 开始轨迹 */
#define RUN_SEQ			37	/* 开始巡航 */
#define STOP_SEQ		38	/* 停止巡航 */
#define GOTO_PRESET		39	/* 快球转到预置点 */

/**********************Onvif云台控制命令 end*************************/
typedef enum
{
	CHANNELITEM_STATE_OPEN = 0,                //打开图像成功
		CHANNELITEM_STATE_CLOSE,               //图像关闭
		CHANNELITEM_STATE_NODATA,              //图像连接正常,但是无数据流(设备上报或主动检测)
		CHANNELITEM_STATE_DEVICEERR,           //通道设备故障
		CHANNELITEM_STATE_PAUSE,               //暂停(录像用)
		CHANNELITEM_STATE_ONLINE,              //在线
		CHANNELITEM_STATE_OFFLINE,             //掉线
		CHANNELITEM_STATE_LOSTVIDEO,           //视频丢失
}CHANNELITEM_STATE;
#endif

//**************EDVHEAD定义 begin******************

typedef union  
{
	struct
	{
		//unsigned short wRet;
		unsigned short wResved1;    /*保留*/
		unsigned short wResved2;    /*保留*/
		//unsigned short wResved3;    /*保留*/
		//unsigned short wResved4;    /*保留*/
	} NORAML;
	struct
	{
		//unsigned short wRet;
		//unsigned short wResved1;		/*保留*/
		unsigned int   sizeseek	;		/*保留*/
		//unsigned short index;		/*保留*/
		//unsigned short wResved4;		/*保留*/
	} FILEPALY;
	struct  
	{
		unsigned int   FrameID;			/*帧序号*/
		//unsigned short wResved3;    /*保留*/
		//unsigned short wResved4;    /*保留*/
	}FRAMEID;
	struct  
	{
		unsigned short   UserID;			/*用户ID号*/
		unsigned short	wResved2;    /*保留*/
		//unsigned short wResved3;    /*保留*/
		//unsigned short wResved4;    /*保留*/
	}USERID;
	struct  
	{
		unsigned int HDFreeSpace;		/*硬盘剩余空间*/
		//unsigned short wResved3;    /*保留*/
		//unsigned short wResved4;    /*保留*/
	}HDSPACE;
}HEADUNION;

typedef struct tag_EDVHEAD
{
	unsigned short wVersions;   /*标识不同版本，从低位到高位，每位置1：支持该版本；0：不支持*/
	unsigned short wPayLoadLen; /*载荷长度*/
	unsigned int   dwEvent;   /*Word32  事件类型*/
	HEADUNION		Comm;
	unsigned int	dwComonID;/*命令ID*/
	char          dummy[16]; /*填充*/
	unsigned char ucPayLoad[MAX_PAYLOAD_LEN]; /*BYTE[1400]  Payload部分*/
}EDVHEAD_EX,*PEDVHEAD_EX;

#define	EDV_HEAD_LEN_EX            (32)
#define EDV_HEAD_LEN	EDV_HEAD_LEN_EX
#define EDVHEAD EDVHEAD_EX
#define PEDVHEAD PEDVHEAD_EX
#define	COMM_MSG_HEAD_T COMM_MSG_HEAD_EX_T
#define JXBTB  1	/*接线表同步*/
typedef struct
{
    unsigned short  wVersions;      /* 版本号 */
    unsigned short  wPayLoadLen;    /* payLoad长度*/
    unsigned int  dwEvent;    /* 事件号 */
    unsigned short  wResved1;       /* 保留 */
    unsigned short  wResved2;       /* 保留 */
	unsigned int	dwComonID;/*命令ID*/
	char          dummy[16]; /*填充*/
}COMM_MSG_HEAD_EX_T;

//**************EDVHEAD定义 end******************
//通道状态
typedef struct _UnitInfo{
	string        strLogicDeviceID;        //ID
	string        strChannelName;          //通道名称
	string        strDeviceIP;             //所属设备IP
	//状态
	DWORD         dwTime;                  //开始发送时间
	LONGLONG      llRcvByteCount;             //发送数据长度
	LONGLONG      llSendByteCount;             //发送数据长度
	LONGLONG      llLastSendByteCount;         //上次取数据时数据长度
	int           iSrcPort;                //本地端口
	string        strDestIP;               //tsu地址
	int           iDestPort;               //tsu端口
	CHANNELITEM_STATE eChannelState;       //通道状态
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

//摄像机类型
typedef enum
{
	EV9000_CAM_TYPE_BALL,    //球机
		EV9000_CAM_TYPE_GUN  //枪机
}EV9000_CAM_TYPE;

//mgw对象类型
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

//待增加
//增加字段：是否能控球

//对应UNGBPhyDeviceConfig表
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

//对应UNGBPhyDeviceChannelConfig表
typedef struct _ungb_chconfigrec{
	BOOL   bUsed;
	int    iID;
	int    iDeviceIndex;        //所属设备索引
	string strLogicDeviceID;    //通道ID
	string strChannelName;      //通道名
	string strChannelIp;        //通道码流IP
	int    iChannelPort;        //通道码流端口
    string strMapChannel;     //通道标识
	int    iNeedCodec;          //是否需要编解码
	int    iStreamType;         //码流类型 0 ps  2 h264  4 hik001  5 hik002
	int    iCamType;            //枪机球机类型  0 球机 1 枪机
	int    iReserved1;
	string strReserved2;
    _ungb_chconfigrec()
	{
		bUsed = TRUE;
		iNeedCodec =0;
		strChannelName ="";
		iStreamType =2;
		iCamType =1;       //枪机
		strChannelIp = "";
		iChannelPort = 554;     //默认rstp服务端口
	}
}UNGB_CHCONFIGREC;

//SIP消息类型
typedef enum{
	SIP_MSG_MSG,
		SIP_MSG_INVITE,
		SIP_MSG_BYE,
        SIP_MSG_CANCEL,
		SIP_MSG_EXPIRE,
		SIP_MSG_INFORM
}SIPMSGTYPE;

//SIP消息等级
typedef enum{
	SIP_LEVEL_COM,      //普通等级0-1-2  数字越大等级越高
		SIP_LEVEL_1,  //等级1
		SIP_LEVEL_2
}SIPMSGLEVEL;

//通道打开类型
typedef enum{
	UNIT_OPEN_COM,      //正常打开
	UNIT_OPEN_EMPTY     //空打开 
}UNITOPENTYPE;

//invite消息类型
typedef enum{
	INVITE_PLAY,         //实时视频
		INVITE_PLAYBACK  //回放
}INVITE_TYPE;

//回调Sip消息结构体
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
		eSipMsgLevel = SIP_LEVEL_COM;  //默认为0
		//      strcaller_id="";
		// 		strMsg="";
		nMsgLen =0;
		idialog_index =-1;
	}
}CBSIPMSG,*PCBSIPMSG;

//码流接续参数
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
	int        iStartTime;   //录像开始时间
	int        iStopTime;    //录像结束时间
	int        iPlayTime;    //录像播放开始时间
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

//编解码信息
typedef struct _codecinfo
{
	int   iDeviceType;      //设备类型
	BOOL  bNeedCodec;
	int   iStreamType;
	string strLogicID;       //通道逻辑ID
	int   nBitRate;
	int   nIFrameRate;
	int   nDataType;        //0 视频 1 录像
	_codecinfo()
	{
		nDataType = 0;  //默认视频
		iDeviceType = 0;
		bNeedCodec = FALSE;
		iStreamType =0;
		strLogicID ="";
		nBitRate = 2*1024*1024; //默认
	    nIFrameRate = 50;       //默认50
	}
}CODECINFO;

#ifdef WIN32
//YUV参数
typedef struct _YuvPara{
	int         index;	//索引
	int         lenth;	//帧长
	int         read;   //读取位置
	SYSTEMTIME  time;  //帧时间
	BYTE        streamtype;  //图象格式
	DWORD       dwFlag;   //进行一些标识
	int         nWidth;   //帧宽
	int         nHeight;  //帧高
	int         nFrameRate;  //帧率 
}YUVPARA;
#endif

//Rtp发送参数
typedef struct _rtppara{
	unsigned short portbase;  //本机端口
	unsigned short destport;  //目的端口
	//uint32_t destip;	      //目的IP
	string ipstr;             //目的IP
	string strLogicDeviceID;  //通道ID   //added by chenyu 131024
	string strChannelName;    //通道名
    _rtppara()
	{
		portbase =0;
		destport =0;
		strLogicDeviceID ="";
		strChannelName ="";
	}
}RTP_PARA;

//配置信息类型
typedef enum{
    SYS_CFG_LOCALIP_INTERCOM,  //本地内部通讯IP 按照(1.设备网 2.视频网)顺序获取地址
		SYS_CFG_LOCALIP_IN,    //设备网
		SYS_CFG_LOCALIP_OUT,   //视频网
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
        SYS_CFG_SIPREFRESHTIME,     //sip刷新时间
		SYS_CFG_RECID,              //记录ID
        SYS_CFG_CONSENDPACKLEN      //连续发包数
}SYS_CFG_TYPE;

//#ifndef WIS
//Rtp相关结构定义
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
//设备状态结构
typedef struct DEVICESTATE_INFO
{
	int                      nDeviceID;         //设备ID
	DEVICEITEM_STATE         eDeviceState;      //设备状态
	unsigned char		     byResved1[4];      //保留1
	unsigned char		     byResved2[32];     //保留2
}DEVICESTATE_INFO, *LPDEVICESTATE_INFO;
#endif

//实时流状态结构
typedef struct REALSTREAM_INFO
{
	int                      nUnitID;           //UNIT ID
	CHANNELITEM_STATE        eChannelState;     //打开状态
	LONGLONG                 llSendPackCount;   //发包数
	unsigned int             nBitRate;          //平均码率(kbps)
	unsigned int             nMaxBitRate;       //最大码率(kbps)
#ifdef WIN32
	SYSTEMTIME               sMaxBitRateTime;   //最大码率产生时间
#endif
	unsigned char		     byResved1[4];      //保留1
	unsigned char		     byResved2[32];     //保留2
	REALSTREAM_INFO()
	{
		eChannelState = CHANNELITEM_STATE_CLOSE;   //关闭
	}
	void clear()
	{
		;
	}
}REALSTREAM_INFO, *LPREALSTREAM_INFO;

//录像回放状态结构
typedef struct RECORDSTREAM_INFO
{
	int                      nPlayBackID;       //播放序号(0-31)   同一个Unit可能播放多路
	CHANNELITEM_STATE        eChannelState;     //打开状态
	unsigned short           usRate;            //进度
	LONGLONG                 llSendPackCount;   //发包数
	unsigned char		     byResved1[4];      //保留1
	unsigned char		     byResved2[32];     //保留2
}RECORDSTREAM_INFO, *LPRECORDSTREAM_INFO;

//录像上报状态结构
typedef struct REPORT_INFO
{
	int                      nUnitID;           //编码板UNIT ID
	unsigned int             nSuccessReport;    //上报是否成功   0成功
	char                     sFileName[MAX_PATH];     //录像名
	unsigned char		     byResved1[4];      //保留1
	unsigned char		     byResved2[32];     //保留2
}REPORT_INFO, *LPREPORT_INFO;

#ifdef WIN32
//存储状态结构
typedef struct STORAGESTREAM_INFO
{
	int                      nUnitID;           //UNIT ID
	CHANNELITEM_STATE        eChannelState;     //打开状态
	unsigned int             nBitRate;          //平均码率(kbps)
	unsigned int             nMaxBitRate;       //最大码率(kbps)
	SYSTEMTIME               sMaxBitRateTime;   //最大码率产生时间
	char                     sFileName[MAX_PATH];     //录像名	
	unsigned char		     byResved1[4];      //保留1
	unsigned char		     byResved2[32];     //保留2
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

//转换为string
static string CovertState2str(CHANNELITEM_STATE eChannelState)
{
	string strChState="OFF";
    //映射关系表
	switch (eChannelState)
	{
	case CHANNELITEM_STATE_ONLINE:  
	case CHANNELITEM_STATE_OPEN:
	case CHANNELITEM_STATE_CLOSE:   //打开、关闭
		{
			strChState = "ON";
		}
		break;
	case CHANNELITEM_STATE_LOSTVIDEO:
	case CHANNELITEM_STATE_NODATA:
	case CHANNELITEM_STATE_DEVICEERR:  //通道故障
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
//连接设备结构
typedef struct CONNECT_DEVICE_INFO
{
	DEVICE_TYPE             eDeviceType;         //设备类型
	unsigned int            nDeviceIP;          //设备IP
	unsigned int            nDevicePort;        //端口
	char                    sUserName[40];            //用户名
	char                    sPassword[40];        //密码
	unsigned char		    byResved1[4];       //保留1
	unsigned char		    byResved2[32];       //保留2
}CONNECT_DEVICE_INFO, *LPCONNECT_DEVICE_INFO;

#if defined(__linux__)
//服务器连接类型
typedef enum
{
	EV9000_CONNECT_TYPE_28181 = 0,
}EV9000_CONNECT_TYPE;
//登录信息
typedef struct EV9000_LOGININFO
{
	EV9000_CONNECT_TYPE  eConnectType;                                    //服务器连接类型
	char                 sServerIP[EV9000_NORMAL_STRING_LEN];             //服务器IP
	unsigned int         nServerPort;                                     //服务器端口
	char                 sUserName[EV9000_NORMAL_STRING_LEN];             //用户名
	char                 sUserPwd[EV9000_NORMAL_STRING_LEN];              //密码
	char                 sServerID[EV9000_NORMAL_STRING_LEN];             //服务器编号
	char                 sUserID[EV9000_NORMAL_STRING_LEN];               //用户编号
	char                 sLocalIP[EV9000_NORMAL_STRING_LEN];              //本机IP
	int                  nDigital;                                        //是否使用数字证书登录 0 不使用 1 使用(暂不支持)
	char                 sReserved[EV9000_NORMAL_STRING_LEN];
	int                  nReserved;
}EV9000_LOGININFO,*LPEV9000_LOGININFO;
#endif

// 封装 message 消息发送结构
typedef struct SIP_MSG_INFO
{
	char CallerID[EV9000_NORMAL_STRING_LEN];
	char CalleedID[EV9000_NORMAL_STRING_LEN];
	char* pMsg;
	int   nMsgLen;
	int   nSN;                 // message 消息序列号 
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
// 返回自系统开机以来的毫秒数（tick）
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
