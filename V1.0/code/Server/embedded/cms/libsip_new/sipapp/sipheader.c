/******************************************************************************

                  版权所有 (C), 2001-2013, 金智视讯技术有限公司

 ******************************************************************************
  文 件 名   : sipheader.c
  版 本 号   : 初稿
  作    者   : yanghaifeng
  生成日期   : 2013年4月1日
  最近修改   :
  功能描述   : SIP头域处理
  函数列表   :
              allow_event_2char
              allow_event_free
              allow_event_init
              allow_event_parse
              event_2char
              event_free
              event_init
              event_parse
              generic_param_parseall1
              msg_getevent_if1
              msg_getexpires_if1
              msg_getreferred_by_if1
              msg_getrefer_to_if1
              msg_getreplaces_if1
              replaces_2char
              replaces_free
              replaces_init
              replaces_parse
              subscription_state_2char
              subscription_state_free
              subscription_state_init
              subscription_state_parse
  修改历史   :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 创建文件

******************************************************************************/

/*----------------------------------------------*
 * 包含头文件                                   *
 *----------------------------------------------*/
#include <malloc.h>
#include <time.h>

#include <osip2/internal.h>
#include <osip2/osip.h>
#include <osipparser2/osip_port.h>

#include "sipheader.inc"

//added by chenyu 130522
#ifdef WIN32
#define vsnprintf _vsnprintf
#define snprintf  _snprintf
#endif

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

int event_init(event_t** event)
{
    *event = (event_t*)osip_malloc(sizeof(event_t));

    if (NULL == *event)
    {
        return -1;
    }

    (*event)->event_type = NULL;
    (*event)->gen_params = (osip_list_t*) osip_malloc(sizeof(osip_list_t));

    if ((*event)->gen_params == NULL)
    {
        osip_free(*event);
        *event = NULL;
    }

    osip_list_init((*event)->gen_params);
    return 0;
}

void event_free(event_t* event)
{
    if (NULL == event)
    {
        return;
    }

    if (event->event_type != NULL)
    {
        osip_free(event->event_type);
        event->event_type = NULL;
    }

    if (NULL != event->gen_params)
    {
        osip_generic_param_freelist(event->gen_params);
        event->gen_params = NULL;
    }

    return;
}

int event_parse(event_t* event, char* hvalue)
{
    int size = 0;
    char* type = NULL;
    char* gen_params = NULL;

    if (NULL == event || NULL == hvalue)
    {
        return -1;
    }

    gen_params = strchr(hvalue, ';');

    if (NULL != gen_params)
    {
        size = gen_params - hvalue;
    }
    else
    {
        size = strlen(hvalue);
    }

    if (size <= 0)
    {
        printf("\n event_parse malloc len error \n");
        return -1;
    }

    type = (char*)osip_malloc(size + 1);

    if (NULL == type)
    {
        return -1;
    }

    osip_strncpy(type, hvalue, size);
    sclrspace(type);
    event->event_type = type;

    if (NULL != gen_params)
    {
        if (generic_param_parseall1(event->gen_params, gen_params) == -1)
        {
            return -1;
        }
    }

    return 0;
}

int event_2char(event_t* event, char** dest)
{
    char* buf = NULL;
//    int i;
    int len = 0;
    int pos = 0;
    osip_generic_param_t* u_param = NULL;
    int plen = 0;
    char* tmp = NULL;

    *dest = NULL;

    if (NULL == event || NULL == event->event_type)
    {
        return -1;
    }

    len = strlen(event->event_type) + 1;
    buf = (char*)osip_malloc(len);

    if (buf == NULL)
    {
        return -1;
    }

    snprintf(buf, len, "%s", event->event_type);

    while (!osip_list_eol(event->gen_params, pos))
    {
        u_param = (osip_generic_param_t*) osip_list_get(event->gen_params, pos);

        if (NULL == u_param)
        {
            pos++;
            continue;
        }

        if (u_param->gvalue == NULL)
        {
            plen = strlen(u_param->gname) + 2;
        }
        else
        {
            plen = strlen(u_param->gname) + strlen(u_param->gvalue) + 3;
        }

        tmp = buf;
#ifdef __PSOS__
        buf = realloc(buf, len + plen, len);
        len = len + plen;
#else
        len = len + plen;
        buf = (char*) osip_realloc(&buf, len);
#endif

        if (buf == NULL)
        {
            if (len != 0)
            {
                osip_free(tmp);
                tmp = NULL;
            }

            {
                return -1;
            }
        }

        tmp = buf;
        tmp = tmp + strlen(tmp);

        if (u_param->gvalue == NULL)
        {
            snprintf(tmp, plen, ";%s", u_param->gname);
        }
        else
        {
            snprintf(tmp, plen, ";%s=%s", u_param->gname, u_param->gvalue);
        }

        pos++;
    }

    *dest = buf;
    return 0;
}

int msg_getevent_if1(osip_message_t* sip, event_t** dest)
{
    int i = 0;
    int size = 0;
    int cnt = 0;
    int pos = 0;
    osip_header_t* header = NULL;
    event_t* event = NULL;
    *dest = NULL;

    if (NULL == sip)
    {
        return -1;
    }

    size = osip_list_size(&sip->headers);
    cnt = 0;

    for (i = 0; i < size; i++)
    {
        if (i == msg_getevent(sip, i, &header)) /* if get event header */
        {
            pos = i;
            cnt++;
        }
    }

    if (cnt == 0 || cnt > 1)
    {
        return cnt;
    }

    msg_getevent(sip, pos, &header);
    i = event_init(&event);

    if (i != 0)
    {
        return -1;
    }

    i = event_parse(event, header->hvalue);

    if (i != 0)
    {
        event_free(event);
        osip_free(event);
        event = NULL;
        return -1;
    }

    *dest = event;
    return 1;
}

int msg_getexpires_if1(osip_message_t* sip, int* expires)
{
    int i = 0;
    int size = 0;
    int cnt = 0;
    int pos = 0;
    osip_header_t* header = NULL;

    if (NULL == sip || NULL == expires)
    {
        return -1;
    }

    size = osip_list_size(&sip->headers);
    cnt = 0;

    for (i = 0; i < size; i++)
    {
        if (i == osip_message_get_expires(sip, i, &header)) /* if get event header */
        {
            pos = i;
            cnt++;
        }
    }

    if (cnt == 0 || cnt > 1)
    {
        return cnt;
    }

    osip_message_get_expires(sip, pos, &header);
    *expires = osip_atoi(header->hvalue);
    return 1;
}

int generic_param_parseall1(osip_list_t* gen_params, char* params)
{
    char* pname = NULL;
    char* pvalue = NULL;

    char* comma = NULL;
    char* equal = NULL;

    /* find '=' wich is the separator for one param */
    /* find ';' wich is the separator for multiple params */

    equal = next_separator(params + 1, '=', ';');
    comma = strchr(params + 1, ';');

    while (comma != NULL)
    {

        if (equal == NULL)
        {
            equal = comma;
            pvalue = NULL;
        }
        else
        {
            char* tmp = NULL;
            /* check for NULL param with an '=' character */
            tmp = equal + 1;

            for (; *tmp == '\t' || *tmp == ' '; tmp++)
            {
            }

            pvalue = NULL;

            if (*tmp != ',' && *tmp != '\0')
            {
                if (comma - equal < 2)
                {
                    printf("\n generic_param_parseall1 malloc len error \n");
                    return -1;
                }

                pvalue = (char*) osip_malloc(comma - equal);

                if (pvalue == NULL)
                {
                    return -1;
                }

                osip_strncpy(pvalue, equal + 1, comma - equal - 1);
            }
        }

        if (equal - params < 2)
        {
            osip_free(pvalue);
            pvalue = NULL;
            printf("\n generic_param_parseall1 malloc len error2 \n");
            return -1;
        }

        pname = (char*) osip_malloc(equal - params);

        if (pname == NULL)
        {
            osip_free(pvalue);
            pvalue = NULL;
            return -1;
        }

        osip_strncpy(pname, params + 1, equal - params - 1);

        osip_generic_param_add(gen_params, pname, pvalue);

        params = comma;
        equal = next_separator(params + 1, '=', ';');
        comma = strchr(params + 1, ';');
    }

    /* this is the last header (comma==NULL) */
    comma = params + strlen(params);

    if (equal == NULL)
    {
        equal = comma;        /* at the end */
        pvalue = NULL;
    }
    else
    {
        char* tmp = NULL;
        /* check for NULL param with an '=' character */
        tmp = equal + 1;

        for (; *tmp == '\t' || *tmp == ' '; tmp++)
        {
        }

        pvalue = NULL;

        if (*tmp != ',' && *tmp != '\0')
        {
            if (comma - equal < 2)
            {
                printf("\n generic_param_parseall1 malloc len error3 \n");
                return -1;
            }

            pvalue = (char*) osip_malloc(comma - equal);

            if (pvalue == NULL)
            {
                return -1;
            }

            osip_strncpy(pvalue, equal + 1, comma - equal - 1);
        }
    }

    if (equal - params < 2)
    {
        osip_free(pvalue);
        pvalue = NULL;
        printf("\n generic_param_parseall1 malloc len error4 \n");
        return -1;
    }

    pname = (char*) osip_malloc(equal - params);

    if (pname == NULL)
    {
        return -1;
    }

    osip_strncpy(pname, params + 1, equal - params - 1);

    osip_generic_param_add(gen_params, pname, pvalue);

    return 0;
}

int msg_set_data_header(osip_message_t* sip)
{
    time_t utc_time;
    struct tm local_time = { 0 };
    char str_date[12] = {0};
    char str_time[12] = {0};
    char str_head[32] = {0};

    if (NULL == sip)
    {
        return -1;
    }

#ifdef WIN32
    utc_time = time(NULL);
    struct tm* local_time_tmp = NULL;
    local_time_tmp = localtime(&utc_time);
    local_time = *local_time_tmp;                      //待修改
#else
    utc_time = time(NULL);
    localtime_r(&utc_time, &local_time);
#endif

    strftime(str_date, sizeof(str_date), "%Y-%m-%d", &local_time);
    strftime(str_time, sizeof(str_time), "%H:%M:%S", &local_time);
    snprintf(str_head, 32, "%sT%s", str_date, str_time);
    osip_message_set_date(sip, str_head);
    return 0;
}
