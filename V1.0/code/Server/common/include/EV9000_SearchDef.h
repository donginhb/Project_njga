/******************************************************************************

                  ��Ȩ���� (C), 2001-2013, ������Ѷ�������޹�˾

 ******************************************************************************
  �� �� ��   : EV9000_SearchDef.h
  �� �� ��   : ����
  ��    ��   : qiliang
  ��������   : 2013��8��26�� ����һ
  ����޸�   :
  ��������   : EV9000ϵͳ��������ר��
  �����б�   :
  �޸���ʷ   :
  1.��    ��   : 2013��8��26�� ����һ
    ��    ��   : qiliang
    �޸�����   : �����ļ�

******************************************************************************/
#ifndef EV9000_SEARCH_DEF_H
#define EV9000_SEARCH_DEF_H

#include "EV9000_ExtDef.h"
#include "EV9000_BoardType.h"

/*----------------------------------------------*
 * ��������                                     *
 *----------------------------------------------*/


/*----------------------------------------------*
 * �궨��                                       *
 *----------------------------------------------*/

#define EV9000_SEARCHBOARD_UDO_PORT           10200  //���������˿�
#define EV9000_SEARCHBOARD_BCAST_PORT         10201  //�㲥�����˿�
#define EV9000_SEARCHBOARD_RESPONSE_PORT      10202  //���������ظ��˿�

#define EV9000_HEAD_LEN                       32     //ͨ��ͷ�ṹ����
#define EV9000_MAX_PAYLOAD_LEN                1400   //�غɳ���

/*----------------------------------------------*
 * ���ݽṹ����                           *
 *----------------------------------------------*/

//ͨ�Žṹ
typedef struct
{
    unsigned short  nVersions;      /* �汾 */
    unsigned short  nPayLoadLen;    /* payLoad����*/
    unsigned int    nEvent;         /* �¼���*/
    unsigned short  nResved1;       /* ����,�ڲ�ͬ����������ͬ���ô�*/
    unsigned short  nResved2;       /* ���� */
    unsigned int	nComonID;       /* ����ID */
    char            dummy[16];      /* ��� */
	unsigned char   ucPayLoad[EV9000_MAX_PAYLOAD_LEN];    //�غ�
}EV9000_HEAD_T,*PEV9000_HEAD_T;

//�ظ�\�޸ĵ��� �ṹ
typedef struct EV9000_BoardInfo
{
	unsigned int         nBoardType;                               //��������
	unsigned int         nEnable;                                  //�Ƿ�����
	unsigned int         nSlotID;                                  //��λ��
	char                 strBoardID[EV9000_IDCODE_LEN];            //�������
	unsigned int         nStatus;                                  //����״̬
	char                 strEth0Mac[EV9000_SHORT_STRING_LEN];      //Eth0 MAC ��ַ
	char                 strDBIP[EV9000_SHORT_STRING_LEN];         //���ݿ� IP ��ַ
	unsigned int         nDBPort;                                  //���ݿ� �˿�
	char                 strDBPath[EV9000_NORMAL_STRING_LEN];      //���ݿ� ·��
	unsigned int         nResved1;                                 //����1
	char                 strResved2[EV9000_SHORT_STRING_LEN];      //����2
}EV9000_BoardInfo, *LPEV9000_BoardInfo;

//�ظ�\�޸ĵ�ַ �ṹ
typedef struct EV9000_BoardNetInfo
{
	unsigned int         nEthID;                                   //����ڱ��  
	unsigned int         nEnable;                                  //�Ƿ�����
	unsigned int         nPort;                                    //SIP�˿ں�
	char                 strIP[EV9000_SHORT_STRING_LEN];           //�����ַ
	char                 strMask[EV9000_SHORT_STRING_LEN];         //��������
	char                 strGateWay[EV9000_SHORT_STRING_LEN];      //��������
	char                 strHost[EV9000_SHORT_STRING_LEN];         //��������
	unsigned int         nStatus;                                  //����˿�״̬
	char                 strMac[EV9000_SHORT_STRING_LEN];          //MAC��ַ
	unsigned int         nResved1;                                 //����1   
	char                 strResved2[EV9000_SHORT_STRING_LEN];      //����2
}EV9000_BoardNetInfo, *LPEV9000_BoardNetInfo;

/*----------------------------------------------*
 * ͨ�������                           *
 *----------------------------------------------*/

#define  EVT_SEARCHBOARD_REQ          0x90000
//�����κ���Ϣ  ������Ҫ���ϱ���CMS��Ϣ,��֤������������CMS�µĵ���

#define  EVT_SEARCHBOARD_ACK          0x90001
//nResved1:EV9000_BoardNetInfo�ṹ����
//ucPayLoad:EV9000_BoardInfo+EV9000_BoardNetInfo+EV9000_BoardNetInfo+����

#define  EVT_UPDATEINFO_REQ           0x90002
//ucPayLoad:EV9000_BoardInfo+EV9000_BoardNetInfo+EV9000_BoardNetInfo+����

#define  EVT_UPDATEINFO_ACK           0x90003
//nResved1:EV9000_BoardNetInfo�ṹ����
//nResved2:0:��ʾ�ɹ�
//ucPayLoad:�¸ĵ�EV9000_BoardInfo+EV9000_BoardNetInfo+EV9000_BoardNetInfo+����



// ***BEGIN***  �޸�MAC wangqichao 2013/8/29 add

#define  EVT_SEARCHMAC_REQ          0x91000  
/*
  ��;:��ѯ���ӵ�ǰMac��ַ
  ����:Mac��д���� --> ����                
  ��ʽ:
  {
  nPayLoadLen = sizeof(EV9000_HEAD_T);
  nEvent =EVT_SEARCHMAC_REQ; 
  }
*/

#define  EVT_SEARCHMAC_RSP          0x81000  
/*
  ��;:ͨ����ӵ�ǰMac��ַ
  ����:���� --> Mac��д����             
  ��ʽ:
  {
  nPayLoadLen = sizeof(EV9000_HEAD_T);
  nResved1 = 0;       0--��ѯ�ɹ�  ����--ʧ��
  nResved2 = 3;       mac��ַ����
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
  ��;:��֪������Mac��ַ 
  ����:Mac��д���� --> ����               
  ��ʽ:
  {
  nPayLoadLen = sizeof(EV9000_HEAD_T);
  nResved2 = 3;       �����ɵ�mac��ַ����
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
  ��;:��֪Mac��д����Mac��ַ�Ƿ���д�ɹ�
  ����:���� --> Mac��д����     
  ��ʽ:
  {
  nPayLoadLen = sizeof(EV9000_HEAD_T);
  nResved1 = 0;   0-��д�ɹ�  ����--ʧ��
  nEvent   = EVT_UPDATEMAC_RSP; 
  } 
*/

#define  EVT_BOARDTYPE_REQ          0x93000
/*
  ��;:��ѯ��������
  ����:Mac��д���� --> ����                     
  ��ʽ:
  {
  nPayLoadLen = sizeof(EV9000_HEAD_T);
  nEvent = EVT_BOARDTYPE_REQ; 
  }
*/

#define  EVT_BOARDTYPE_RSP          0x83000   
/*
  ��;:�ظ���������
  ����:���� --> Mac��д����                
  ��ʽ:
  {
  nPayLoadLen = sizeof(EV9000_HEAD_T);
  nResved1 = 0;    0-��ѯ�ɹ�  ����--ʧ��
  nResved2;        0--cms/tsu  2--dsp  3--csua/vsua  4--eps6000  0xFF--�������ͣ�������ϸ��Ϣ��Payload������
  nEvent   = EVT_BOARDTYPE_RSP; 
  ucPayLoad[] = 
  {"BoardTypeName:MacNum\0"}    //��nResved2==0xFFʱ���ڴ˴�����������ϸ��Ϣ����ʽΪ:{����������:Mac��ַ����} ����:{"NewTypeBoard:3\n"}
  }   
*/

// ***END***  �޸�MAC wangqichao 2013/8/29 add

/*----------------------------------------------*
 * �ⲿ����ԭ��˵��                             *
 *----------------------------------------------*/

#endif
