/******************************************************************************

                  版权所有 (C), 2001-2013, 金智视讯技术有限公司

 ******************************************************************************
  文 件 名   : sipauth.h
  版 本 号   : 初稿
  作    者   : yanghaifeng
  生成日期   : 2013年4月1日
  最近修改   :
  功能描述   : sipauth.c 的头文件
  函数列表   :
  修改历史   :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 创建文件

******************************************************************************/
#ifndef __SIPAUTH_H__
#define __SIPAUTH_H__

//modified by chenyu 130522
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
#include <osip2/internal.h>
#include <osip2/osip.h>

#include "gbltype.h"

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
#define HASHLEN 16
    typedef char HASH[HASHLEN];
#define HASHHEXLEN 32
    typedef char HASHHEX[HASHHEXLEN + 1];

    extern int CheckDigestAuth(osip_authorization_t* authorization, int permit_anonym, char* username, char* password, char* method);
    extern int check_digest_auth(osip_authorization_t* authorization, int permit_anonym, char* username, char* password, char* method);
    extern void CvtHex(IN HASH Bin, OUT HASHHEX Hex);
    extern void DigestCalcHA1(IN char* pszAlg,
                                IN char* pszUserName,
                                IN char* pszRealm,
                                IN char* pszPassword,
                                IN char* pszNonce, IN char* pszCNonce, OUT HASHHEX SessionKey);
    extern void DigestCalcResponse(IN HASHHEX HA1,     /* H(A1) */
                                     IN char* pszNonce,  /* nonce from server */
                                     IN char* pszNonceCount, /* 8 hex digits */
                                     IN char* pszCNonce, /* client nonce */
                                     IN char* pszQop,    /* qop-value: "", "auth", "auth-int" */
                                     IN char* pszMethod, /* method from the request */
                                     IN char* pszDigestUri,  /* requested URL */
                                     IN HASHHEX HEntity, /* H(entity body) if qop="auth-int" */
                                     OUT HASHHEX Response    /* request-digest or response-digest */
                                    );
    EV9000SIP_API int SIP_Auth(osip_authorization_t* authorization, char* username, char* password, char* method);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif /* __SIPAUTH_H__ */
