/******************************************************************************

                  版权所有 (C), 2001-2013, 金智视讯技术有限公司

 ******************************************************************************
  文 件 名   : initfunc.h
  版 本 号   : 初稿
  作    者   : yanghaifeng
  生成日期   : 2013年4月1日
  最近修改   :
  功能描述   : initfunc.c 的头文件
  函数列表   :
  修改历史   :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 创建文件

******************************************************************************/
#ifndef __INITFUNC_H__
#define __INITFUNC_H__

/* modified by chenyu 130522 */
#ifdef WIN32

#ifdef EV9000SIP_EXPORTS
#define EV9000SIP_API    __declspec(dllexport)
#else
#define EV9000SIP_API    __declspec(dllimport)
#endif

#else
#define EV9000SIP_API    extern
#endif //end WIN32

/*----------------------------------------------*
 * 包含头文件                                   *
 *----------------------------------------------*/

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

    /*----------------------------------------------*
     * 外部变量说明                                 *
     *----------------------------------------------*/

    /*----------------------------------------------*
     * 外部函数原型说明                             *
     *----------------------------------------------*/

    /*----------------------------------------------*
     * 内部函数原型说明                             *
     *----------------------------------------------*/

    /*----------------------------------------------*
     * 全局变量                                     *
     *----------------------------------------------*/

    /*----------------------------------------------*
     * 模块级变量                                   *
     *----------------------------------------------*/

    /*----------------------------------------------*
     * 常量定义                                     *
     *----------------------------------------------*/

    /*----------------------------------------------*
     * 宏定义                                       *
     *----------------------------------------------*/

    EV9000SIP_API void SIP_Free();
    EV9000SIP_API int SIP_Init();
    extern int sip_stack_cell_free_for_recv();
    extern int sip_stack_cell_free_for_recv_msg();
    extern int sip_stack_cell_free_for_recv_register();
    extern int sip_stack_cell_free_for_send();
    extern int sip_stack_cell_free_for_send_msg();
    extern int sip_stack_cell_init_for_recv();
    extern int sip_stack_cell_init_for_recv_msg();
    extern int sip_stack_cell_init_for_recv_register();
    extern int sip_stack_cell_init_for_send();
    extern int sip_stack_cell_init_for_send_msg();
    EV9000SIP_API int sip_stack_free();
    EV9000SIP_API int sip_stack_init();

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif /* __INITFUNC_H__ */
