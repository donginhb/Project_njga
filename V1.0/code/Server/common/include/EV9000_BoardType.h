
/***********************************************************************
* Copyright (C) 2013, WISCOM VISION TECHNOLOGY Corporation.
* 
* File Name: 	   EV9000_BoardType.h
* Description:   本文件定义平台管理的事件类型及相关的结构体
* Others:        无
* Version:  	   dev-v1.01.01
* Author:        输入作者名字及单位
* Date:  	       2013.05.31
* 
* History 1:  		// 修改历史记录，包括修改日期、修改者、修改版本及修改内容
*                2013.05.31    qyg    dev-v1.01.01   首次创建  
* History 2: …
**********************************************************************/


#ifndef EV9000_BOARD_TYPE_H
#define EV9000_BOARD_TYPE_H

/*物理单板类型*/

#define        PHY_BOARD_CAVIUM    0x01
#define        PHY_BOARD_X86_I3    0x02
#define        PHY_BOARD_X86_I7    0x03


/*单板功能分类*/
#define        FUNC_CMS            0x01    /* cms */
#define        FUNC_TSU            0x02    /* TSU */
#define        FUNC_MGW_L          0x03    /* MGW for Linux */
#define        FUNC_MGW_W          0x04    /* MGW for Windows */
#define        FUNC_DCE_L          0x05    /* DC for Linux */
#define        FUNC_DCE_W          0x06    /* DC for Windows */

/*逻辑单板类型*/

#define        LOGIC_BOARD_CMS                  ((PHY_BOARD_CAVIUM<<11)|(FUNC_CMS<<4))
#define        LOGIC_BOARD_TSU                  ((PHY_BOARD_CAVIUM<<11)|(FUNC_TSU<<4))
#define        LOGIC_BOARD_MGW_L                ((PHY_BOARD_X86_I3<<11)|(FUNC_MGW_L<<4))
#define        LOGIC_BOARD_MGW_W                ((PHY_BOARD_X86_I3<<11)|(FUNC_MGW_W<<4))
#define        LOGIC_BOARD_DCE_L                ((PHY_BOARD_X86_I7<<11)|(FUNC_DCE_L<<4))
#define        LOGIC_BOARD_DCE_W                ((PHY_BOARD_X86_I7<<11)|(FUNC_DCE_W<<4))


#endif












