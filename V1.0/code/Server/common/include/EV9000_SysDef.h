/******************************************************************************

                  ��Ȩ���� (C), 2001-2016, ������Ѷ�������޹�˾

 ******************************************************************************
  �� �� ��   : EV9000_SysDef.h.h
  �� �� ��   : ����
  ��    ��   : zb
  ��������   : 2016��4��16�� ������
  ����޸�   :
  ��������   : �����⿪��������ض���
  �����б�   :
  �޸���ʷ   :
  1.��    ��   : 2016��4��16�� ������
    ��    ��   : zb
    �޸�����   : �����ļ�

******************************************************************************/

#include "EV9000_ExtDef.h"

#define EV9000_SYSTEM_MODIFY_PSW               0xF0000001      //�޸��û�����

typedef struct  
{
	char                 strUserID[EV9000_IDCODE_LEN];              //�û�ID
	char                 strUserName[EV9000_SHORT_STRING_LEN];      //�û���
	char                 strLastPsw[EV9000_SHORT_STRING_LEN];       //ԭ����
	char                 strNewPsw[EV9000_SHORT_STRING_LEN];        //������
	unsigned int         nResved1;                                  //����1
	char                 strResved2[EV9000_SHORT_STRING_LEN];       //����2 
}EV9000_PSWCfg, *LPEV9000_PSWCfg;