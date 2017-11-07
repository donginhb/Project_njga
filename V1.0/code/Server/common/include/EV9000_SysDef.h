/******************************************************************************

                  版权所有 (C), 2001-2016, 金智视讯技术有限公司

 ******************************************************************************
  文 件 名   : EV9000_SysDef.h.h
  版 本 号   : 初稿
  作    者   : zb
  生成日期   : 2016年4月16日 星期六
  最近修改   :
  功能描述   : 不对外开放内容相关定义
  函数列表   :
  修改历史   :
  1.日    期   : 2016年4月16日 星期六
    作    者   : zb
    修改内容   : 创建文件

******************************************************************************/

#include "EV9000_ExtDef.h"

#define EV9000_SYSTEM_MODIFY_PSW               0xF0000001      //修改用户密码

typedef struct  
{
	char                 strUserID[EV9000_IDCODE_LEN];              //用户ID
	char                 strUserName[EV9000_SHORT_STRING_LEN];      //用户名
	char                 strLastPsw[EV9000_SHORT_STRING_LEN];       //原密码
	char                 strNewPsw[EV9000_SHORT_STRING_LEN];        //新密码
	unsigned int         nResved1;                                  //保留1
	char                 strResved2[EV9000_SHORT_STRING_LEN];       //保留2 
}EV9000_PSWCfg, *LPEV9000_PSWCfg;