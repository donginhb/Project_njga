/******************************************************************************

                  版权所有 (C), 2001-2013, 金智视讯技术有限公司

 ******************************************************************************
  文 件 名   : sipheader.h
  版 本 号   : 初稿
  作    者   : yanghaifeng
  生成日期   : 2013年4月1日
  最近修改   :
  功能描述   : sipheader.c 的头文件
  函数列表   :
  修改历史   :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 创建文件

******************************************************************************/
#ifndef __SIPHEADER_H__
#define __SIPHEADER_H__

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
    /**
     * Structure for Event headers.
     * @defvar Event
     */
    typedef struct _event_t
    {
        char*   event_type;
        osip_list_t* gen_params;
    } event_t;

    /**
    * Structure for Allow-Event headers.
    * @defvar Allow-Event
    */
    typedef struct allow_event_t allow_event_t;

    struct allow_event_t
    {
        osip_list_t* event_types;  /* list of char* */
    };

    /* other header(like Event,  Allow-Events, Refer-To, Referred-By,
       Subscription-State, Replaces, Join, Min-Expires etc)get and set functions.
    */
#define msg_setevent(header,value)         osip_message_set_header((osip_message_t *)header,(char *)"Event",value)
#define msg_getevent(header,pos,dest)      osip_message_header_get_byname(( osip_message_t *)header, (char *)"event",pos,(osip_header_t **)dest)

#define msg_setallow_events(header,value)         osip_message_set_header((osip_message_t *)header,(char *)"Allow-Events",value)
#define msg_getallow_events(header,pos,dest)      osip_message_header_get_byname(( osip_message_t *)header, (char *)"allow-events",pos,(osip_header_t **)dest)

#define msg_setrefer_to(header,value)         osip_message_set_header((osip_message_t *)header,(char *)"Refer-To",value)
#define msg_getrefer_to(header,pos,dest)      osip_message_header_get_byname(( osip_message_t *)header, (char *)"refer-to",pos,(osip_header_t **)dest)

#define msg_setreferred_by(header,value)         osip_message_set_header((osip_message_t *)header,(char *)"Referred-By",value)
#define msg_getreferred_by(header,pos,dest)      osip_message_header_get_byname(( osip_message_t *)header, (char *)"referred-by",pos,(osip_header_t **)dest)

#define msg_setsubscription_state(header,value)         osip_message_set_header((osip_message_t *)header,(char *)"Subscription-State",value)
#define msg_getsubscription_state(header,pos,dest)      osip_message_header_get_byname(( osip_message_t *)header, (char *)"subscription-state",pos,(osip_header_t **)dest)

#define msg_setreplaces(header,value)         osip_message_set_header((osip_message_t *)header,(char *)"Replaces",value)
#define msg_getreplaces(header,pos,dest)      osip_message_header_get_byname(( osip_message_t *)header, (char *)"replaces",pos,(osip_header_t **)dest)

#define msg_setjoin(header,value)         osip_message_set_header((osip_message_t *)header,(char *)"Join",value)
#define msg_getjoin(header,pos,dest)      osip_message_header_get_byname(( osip_message_t *)header, (char *)"join",pos,(osip_header_t **)dest)

#define msg_setmin_expires(header,value)         osip_message_set_header((osip_message_t *)header,(char *)"Min-Expires",value)
#define msg_getmin_expires(header,pos,dest)      osip_message_header_get_byname(( osip_message_t *)header, (char *)"min-expires",pos,(osip_header_t **)dest)

#define msg_set_expires(header,value)         osip_message_set_header((osip_message_t *)header,(char *)"Expires",value)
#define msg_get_expires(header,pos,dest)      osip_message_header_get_byname(( osip_message_t *)header, (char *)"expires",pos,(osip_header_t **)dest)

#define msg_set_session_expires(header,value)         osip_message_set_header((osip_message_t *)header,(char *)"Session-Expires",value)
#define msg_get_session_expires(header,pos,dest)      osip_message_header_get_byname(( osip_message_t *)header, (char *)"session-expires",pos,(osip_header_t **)dest)

#define msg_set_min_se(header,value)         osip_message_set_header((osip_message_t *)header,(char *)"Min-SE",value)
#define msg_get_min_se(header,pos,dest)      osip_message_header_get_byname(( osip_message_t *)header, (char *)"min-se",pos,(osip_header_t **)dest)

    extern int event_2char(event_t* event, char** dest);
    extern void event_free(event_t* event);
    extern int event_init(event_t** event);
    extern int event_parse(event_t* event, char* hvalue);
    extern int generic_param_parseall1(osip_list_t* gen_params, char* params);
    extern int msg_getevent_if1(osip_message_t* sip, event_t** dest);
    extern int msg_getexpires_if1(osip_message_t* sip, int* expires);
    extern int msg_set_data_header(osip_message_t* sip);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif /* __SIPHEADER_H__ */
