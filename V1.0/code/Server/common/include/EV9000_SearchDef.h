/******************************************************************************

                  版权所有 (C), 2001-2013, 金智视讯技术有限公司

 ******************************************************************************
  文 件 名   : EV9000_SearchDef.h
  版 本 号   : 初稿
  作    者   : qiliang
  生成日期   : 2013年8月26日 星期一
  最近修改   :
  功能描述   : EV9000系统搜索命令专用
  函数列表   :
  修改历史   :
  1.日    期   : 2013年8月26日 星期一
    作    者   : qiliang
    修改内容   : 创建文件

******************************************************************************/
#ifndef EV9000_SEARCH_DEF_H
#define EV9000_SEARCH_DEF_H

#include "EV9000_ExtDef.h"
#include "EV9000_BoardType.h"

/*----------------------------------------------*
 * 常量定义                                     *
 *----------------------------------------------*/


/*----------------------------------------------*
 * 宏定义                                       *
 *----------------------------------------------*/

#define EV9000_SEARCHBOARD_UDO_PORT           10200  //单播搜索端口
#define EV9000_SEARCHBOARD_BCAST_PORT         10201  //广播搜索端口
#define EV9000_SEARCHBOARD_RESPONSE_PORT      10202  //单播搜索回复端口

#define EV9000_HEAD_LEN                       32     //通信头结构长度
#define EV9000_MAX_PAYLOAD_LEN                1400   //载荷长度

/*----------------------------------------------*
 * 数据结构定义                           *
 *----------------------------------------------*/

//通信结构
typedef struct
{
    unsigned short  nVersions;      /* 版本 */
    unsigned short  nPayLoadLen;    /* payLoad长度*/
    unsigned int    nEvent;         /* 事件卿*/
    unsigned short  nResved1;       /* 保留,在不同场合用作不同的用处*/
    unsigned short  nResved2;       /* 保留 */
    unsigned int	nComonID;       /* 命令ID */
    char            dummy[16];      /* 填充 */
	unsigned char   ucPayLoad[EV9000_MAX_PAYLOAD_LEN];    //载荷
}EV9000_HEAD_T,*PEV9000_HEAD_T;

//回复\修改单板 结构
typedef struct EV9000_BoardInfo
{
	unsigned int         nBoardType;                               //单板类型
	unsigned int         nEnable;                                  //是否启用
	unsigned int         nSlotID;                                  //槽位号
	char                 strBoardID[EV9000_IDCODE_LEN];            //单板编码
	unsigned int         nStatus;                                  //单板状态
	char                 strEth0Mac[EV9000_SHORT_STRING_LEN];      //Eth0 MAC 地址
	char                 strDBIP[EV9000_SHORT_STRING_LEN];         //数据库 IP 地址
	unsigned int         nDBPort;                                  //数据库 端口
	char                 strDBPath[EV9000_NORMAL_STRING_LEN];      //数据库 路径
	unsigned int         nResved1;                                 //保留1
	char                 strResved2[EV9000_SHORT_STRING_LEN];      //保留2
}EV9000_BoardInfo, *LPEV9000_BoardInfo;

//回复\修改地址 结构
typedef struct EV9000_BoardNetInfo
{
	unsigned int         nEthID;                                   //网络口编号  
	unsigned int         nEnable;                                  //是否启用
	unsigned int         nPort;                                    //SIP端口号
	char                 strIP[EV9000_SHORT_STRING_LEN];           //单板地址
	char                 strMask[EV9000_SHORT_STRING_LEN];         //单板掩码
	char                 strGateWay[EV9000_SHORT_STRING_LEN];      //单板网关
	char                 strHost[EV9000_SHORT_STRING_LEN];         //单板域名
	unsigned int         nStatus;                                  //单板端口状态
	char                 strMac[EV9000_SHORT_STRING_LEN];          //MAC地址
	unsigned int         nResved1;                                 //保留1   
	char                 strResved2[EV9000_SHORT_STRING_LEN];      //保留2
}EV9000_BoardNetInfo, *LPEV9000_BoardNetInfo;

/*----------------------------------------------*
 * 通信命令定义                           *
 *----------------------------------------------*/

#define  EVT_SEARCHBOARD_REQ          0x90000
//不带任何消息  后续需要加上本局CMS信息,保证是搜索到本局CMS下的单板

#define  EVT_SEARCHBOARD_ACK          0x90001
//nResved1:EV9000_BoardNetInfo结构个数
//ucPayLoad:EV9000_BoardInfo+EV9000_BoardNetInfo+EV9000_BoardNetInfo+……

#define  EVT_UPDATEINFO_REQ           0x90002
//ucPayLoad:EV9000_BoardInfo+EV9000_BoardNetInfo+EV9000_BoardNetInfo+……

#define  EVT_UPDATEINFO_ACK           0x90003
//nResved1:EV9000_BoardNetInfo结构个数
//nResved2:0:表示成功
//ucPayLoad:新改的EV9000_BoardInfo+EV9000_BoardNetInfo+EV9000_BoardNetInfo+……



// ***BEGIN***  修改MAC wangqichao 2013/8/29 add

#define  EVT_SEARCHMAC_REQ          0x91000  
/*
  用途:查询板子当前Mac地址
  发送:Mac烧写程序 --> 板子                
  格式:
  {
  nPayLoadLen = sizeof(EV9000_HEAD_T);
  nEvent =EVT_SEARCHMAC_REQ; 
  }
*/

#define  EVT_SEARCHMAC_RSP          0x81000  
/*
  用途:通告板子当前Mac地址
  发送:板子 --> Mac烧写程序             
  格式:
  {
  nPayLoadLen = sizeof(EV9000_HEAD_T);
  nResved1 = 0;       0--查询成功  其他--失败
  nResved2 = 3;       mac地址个数
  nEvent   = EVT_SEARCHMAC_RSP;
  ucPayLoad[] = 
  {00:EA:1C:00:00:00 
   00:EA:1C:00:00:01 
   00:EA:1C:00:00:02
   .................}
  } 
*/ 

#define  EVT_UPDATEMAC_REQ          0x92000  
/*
  用途:告知新生成Mac地址 
  发送:Mac烧写程序 --> 板子               
  格式:
  {
  nPayLoadLen = sizeof(EV9000_HEAD_T);
  nResved2 = 3;       新生成的mac地址个数
  nEvent   = EVT_UPDATEMAC_REQ; 
  ucPayLoad[] = 
  {00:EA:1C:00:00:00 
   00:EA:1C:00:00:01 
   00:EA:1C:00:00:02
   .................}
  }
*/

#define  EVT_UPDATEMAC_RSP          0x82000   
/*
  用途:告知Mac烧写程序，Mac地址是否烧写成功
  发送:板子 --> Mac烧写程序     
  格式:
  {
  nPayLoadLen = sizeof(EV9000_HEAD_T);
  nResved1 = 0;   0-烧写成功  其他--失败
  nEvent   = EVT_UPDATEMAC_RSP; 
  } 
*/

#define  EVT_BOARDTYPE_REQ          0x93000
/*
  用途:查询板子类型
  发送:Mac烧写程序 --> 板子                     
  格式:
  {
  nPayLoadLen = sizeof(EV9000_HEAD_T);
  nEvent = EVT_BOARDTYPE_REQ; 
  }
*/

#define  EVT_BOARDTYPE_RSP          0x83000   
/*
  用途:回复板子类型
  发送:板子 --> Mac烧写程序                
  格式:
  {
  nPayLoadLen = sizeof(EV9000_HEAD_T);
  nResved1 = 0;    0-查询成功  其他--失败
  nResved2;        0--cms/tsu  2--dsp  3--csua/vsua  4--eps6000  0xFF--扩充类型，板子详细信息在Payload中描述
  nEvent   = EVT_BOARDTYPE_RSP; 
  ucPayLoad[] = 
  {"BoardTypeName:MacNum\0"}    //当nResved2==0xFF时，在此处描述板子详细信息，格式为:{板子类型名:Mac地址数量} 例如:{"NewTypeBoard:3\n"}
  }   
*/

// ***END***  修改MAC wangqichao 2013/8/29 add

/*----------------------------------------------*
 * 外部函数原型说明                             *
 *----------------------------------------------*/

#endif
