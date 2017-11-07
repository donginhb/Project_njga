/******************************************************************************

                  版权所有 (C), 2001-2013, 金智视讯技术有限公司

 ******************************************************************************
  文 件 名   : csdbg.c
  版 本 号   : 初稿
  作    者   : yanghaifeng
  生成日期   : 2013年4月1日
  最近修改   :
  功能描述   : DBG调试信息打印控制
  函数列表   :
                              SIPDebugTrace
  修改历史   :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 创建文件

******************************************************************************/

/*----------------------------------------------*
 * 包含头文件                                   *
 *----------------------------------------------*/
#include <string>
#include <osipparser2/osip_port.h>
#include "gbltype.h"
#include "csdbg.inc"
#include "callback.inc"

using namespace std;

/* added by chenyu 130522 */
#ifdef WIN32
#define vsnprintf _vsnprintf
#define snprintf  _snprintf
#endif
/*----------------------------------------------*
 * 外部变量说明                                 *
 *----------------------------------------------*/
extern app_callback_t* g_AppCallback;

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


#if DECS("调试信息")

/*****************************************************************************
 函 数 名  : SIPDebugTrace
 功能描述  : SIP协议栈打印函数
 输入参数  : int iLevel
                            char* FILENAME
                            const char* FUNCTIONNAME
                            int CODELINE
                            const char* fmt
                            ...
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void SIPDebugTrace(int iLevel, const char* FILENAME, const char* FUNCTIONNAME, int CODELINE, const char* fmt, ...)
{
    int len = 0;
    va_list args;
    char s[2048] = {0};

    if ((NULL == g_AppCallback) || (NULL == g_AppCallback->dbg_printf_cb))
    {
        return;
    }

    va_start(args, fmt);
    len = vsnprintf(s, 2048, fmt, args);
    va_end(args);

    g_AppCallback->dbg_printf_cb(iLevel, FILENAME, FUNCTIONNAME, CODELINE, s);

    return;
}

/*****************************************************************************
 函 数 名  : SIPMessageTrace
 功能描述  : SIP消息跟踪函数
 输入参数  : int type:
                            0,正确的
                            1:发送错误的
                            2:接收解析错误的
                            3:接收消息错误的
                            4:接收创建事务错误的
                            int iDirect:
                            0:发送的
                            1:接收的
                            char* ipaddr
                            int port
                            char* msg
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年9月4日 星期三
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void SIPMessageTrace(int type, int iDirect, char* ipaddr, int port, char* msg)
{
    if ((NULL == g_AppCallback) || (NULL == g_AppCallback->sip_message_trace_cb))
    {
        return;
    }

    g_AppCallback->sip_message_trace_cb(type, iDirect, ipaddr, port, msg);
}
#endif
