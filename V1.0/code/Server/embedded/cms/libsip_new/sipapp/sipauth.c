/******************************************************************************

                  版权所有 (C), 2001-2013, 金智视讯技术有限公司

 ******************************************************************************
  文 件 名   : sipauth.c
  版 本 号   : 初稿
  作    者   : yanghaifeng
  生成日期   : 2013年4月1日
  最近修改   :
  功能描述   : SIP认证
  函数列表   :
              CheckDigestAuth
              CvtHex
              DigestCalcHA1
              DigestCalcResponse
  修改历史   :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 创建文件

******************************************************************************/

/*----------------------------------------------*
 * 包含头文件                                   *
 *----------------------------------------------*/
#include <osipparser2/osip_list.h>
#include <osipparser2/osip_port.h>
#include <osipparser2/osip_md5.h>

#include <string.h>

#include "sipauth.inc"
#include "gblfunc.inc"

#include "csdbg.inc"

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

/*****************************************************************************
 函 数 名  : CvtHex
 功能描述  : Hex计算
 输入参数  : IN HASH Bin
             OUT HASHHEX Hex
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void CvtHex(IN HASH Bin, OUT HASHHEX Hex)
{
    unsigned short i = 0;
    unsigned char j = 0;

    for (i = 0; i < HASHLEN; i++)
    {
        j = (Bin[i] >> 4) & 0xf;

        if (j <= 9)
        {
            Hex[i * 2] = (j + '0');
        }
        else
        {
            Hex[i * 2] = (j + 'a' - 10);
        }

        j = Bin[i] & 0xf;

        if (j <= 9)
        {
            Hex[i * 2 + 1] = (j + '0');
        }
        else
        {
            Hex[i * 2 + 1] = (j + 'a' - 10);
        }
    }

    Hex[HASHHEXLEN] = '\0';
    return;
}

/*****************************************************************************
 函 数 名  : DigestCalcHA1
 功能描述  : calculate H(A1) as per spec
 输入参数  : IN char *pszAlg
             IN char *pszUserName
             IN char *pszRealm
             IN char *pszPassword
             IN char *pszNonce
             IN char *pszCNonce
             OUT HASHHEX SessionKey
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void DigestCalcHA1(IN char* pszAlg,
                   IN char* pszUserName,
                   IN char* pszRealm,
                   IN char* pszPassword,
                   IN char* pszNonce, IN char* pszCNonce, OUT HASHHEX SessionKey)
{
    osip_MD5_CTX Md5Ctx;
    HASH HA1;

    osip_MD5Init(&Md5Ctx);

    if (pszUserName == NULL)
    {
        osip_MD5Update(&Md5Ctx, (unsigned char*)pszUserName, 0);
    }
    else
    {
        osip_MD5Update(&Md5Ctx, (unsigned char*)pszUserName, strlen(pszUserName));
    }

    osip_MD5Update(&Md5Ctx, (unsigned char*)":", 1);

    if (pszRealm == NULL)
    {
        osip_MD5Update(&Md5Ctx, (unsigned char*)pszRealm, 0);
    }
    else
    {
        osip_MD5Update(&Md5Ctx, (unsigned char*)pszRealm, strlen(pszRealm));
    }

    osip_MD5Update(&Md5Ctx, (unsigned char*)":", 1);

    if (pszPassword == NULL)
    {
        osip_MD5Update(&Md5Ctx, (unsigned char*)pszPassword, 0);
    }
    else
    {
        osip_MD5Update(&Md5Ctx, (unsigned char*)pszPassword, strlen(pszPassword));
    }

    osip_MD5Final((unsigned char*)HA1, &Md5Ctx);

    if (sstrcmp(pszAlg, "md5-sess") == 0)
    {
        osip_MD5Init(&Md5Ctx);
        osip_MD5Update(&Md5Ctx, (unsigned char*)HA1, HASHLEN);
        osip_MD5Update(&Md5Ctx, (unsigned char*)":", 1);
        osip_MD5Update(&Md5Ctx, (unsigned char*)pszNonce, strlen(pszNonce));
        osip_MD5Update(&Md5Ctx, (unsigned char*)":", 1);
        osip_MD5Update(&Md5Ctx, (unsigned char*)pszCNonce, strlen(pszCNonce));
        osip_MD5Final((unsigned char*)HA1, &Md5Ctx);
    }

    CvtHex(HA1, SessionKey);
    return;
}

/*  */
/*****************************************************************************
 函 数 名  : DigestCalcResponse
 功能描述  : calculate request-digest/response-digest as per HTTP Digest spec
 输入参数  : IN HASHHEX HA1
             IN char *pszNonce
             IN char *pszNonceCount
             IN char *pszCNonce
             IN char *pszQop
             IN char *pszMethod
             IN char *pszDigestUri
             IN HASHHEX HEntity
             OUT HASHHEX Response
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void DigestCalcResponse(IN HASHHEX HA1,     /* H(A1) */
                        IN char* pszNonce,  /* nonce from server */
                        IN char* pszNonceCount, /* 8 hex digits */
                        IN char* pszCNonce, /* client nonce */
                        IN char* pszQop,    /* qop-value: "", "auth", "auth-int" */
                        IN char* pszMethod, /* method from the request */
                        IN char* pszDigestUri,  /* requested URL */
                        IN HASHHEX HEntity, /* H(entity body) if qop="auth-int" */
                        OUT HASHHEX Response    /* request-digest or response-digest */
                       )
{
    osip_MD5_CTX Md5Ctx;
    HASH HA2;
    HASH RespHash;
    HASHHEX HA2Hex;

    // calculate H(A2)
    osip_MD5Init(&Md5Ctx);
    osip_MD5Update(&Md5Ctx, (unsigned char*)pszMethod, strlen(pszMethod));
    osip_MD5Update(&Md5Ctx, (unsigned char*)":", 1);
    osip_MD5Update(&Md5Ctx, (unsigned char*)pszDigestUri, strlen(pszDigestUri));

    if (pszQop != NULL)
    {
#ifdef AUTH_INT_SUPPORT                   /* experimental  */
        char* index;
        index = strchr(pszQop, 'i');

        while (index != NULL && index - pszQop >= 5 && strlen(index) >= 3)
        {
            if (strncmp(index - 5, "auth-int", 8) == 0)
            {
                SIP_DEBUG_TRACE(LOG_DEBUG, "DigestCalcResponse() exit---: auth-int Error \r\n");
                goto auth_withqop;
            }

            index = strchr(index + 1, 'i');
        }

        index = strchr(pszQop, 'a');

        while (index != NULL && strlen(index) >= 4)
        {
            if (strncmp(index - 5, "auth", 4) == 0)
            {
                /* and in the case of a unknown token
                like auth1. It is not auth, but this
                implementation will think it is!??
                This is may not happen but it's a bug!
                  */
                SIP_DEBUG_TRACE(LOG_DEBUG, "DigestCalcResponse() exit---: auth Error \r\n");
                goto auth_withqop;
            }

            index = strchr(index + 1, 'a');
        }

#endif

        if (strncmp(pszQop, "auth-int", 9) == 0)
        {
            osip_MD5Update(&Md5Ctx, (unsigned char*)":", 1);
            osip_MD5Update(&Md5Ctx, (unsigned char*)HEntity, HASHHEXLEN);

            SIP_DEBUG_TRACE(LOG_DEBUG, "DigestCalcResponse() exit---: auth-int 1 Error \r\n");
            goto auth_withqop;
        }
        else if (strncmp(pszQop, "auth", 5) == 0)
        {
            SIP_DEBUG_TRACE(LOG_DEBUG, "DigestCalcResponse() exit---: auth 1 Error \r\n");
            goto auth_withqop;
        }

        goto auth_withoutqop;
    }

auth_withoutqop:
    osip_MD5Final((unsigned char*)HA2, &Md5Ctx);
    CvtHex(HA2, HA2Hex);

    /* calculate response */
    osip_MD5Init(&Md5Ctx);
    osip_MD5Update(&Md5Ctx, (unsigned char*)HA1, HASHHEXLEN);
    osip_MD5Update(&Md5Ctx, (unsigned char*)":", 1);
    osip_MD5Update(&Md5Ctx, (unsigned char*)pszNonce, strlen(pszNonce));
    osip_MD5Update(&Md5Ctx, (unsigned char*)":", 1);
    goto end;

auth_withqop:
    /*
      MD5Update(&Md5Ctx, (unsigned char*)":", 1);
      MD5Update(&Md5Ctx, (unsigned char*)HEntity, HASHHEXLEN);
    */
    osip_MD5Final((unsigned char*)HA2, &Md5Ctx);
    CvtHex(HA2, HA2Hex);

    /* calculate response */
    osip_MD5Init(&Md5Ctx);
    osip_MD5Update(&Md5Ctx, (unsigned char*)HA1, HASHHEXLEN);
    osip_MD5Update(&Md5Ctx, (unsigned char*)":", 1);
    osip_MD5Update(&Md5Ctx, (unsigned char*)pszNonce, strlen(pszNonce));
    osip_MD5Update(&Md5Ctx, (unsigned char*)":", 1);
    osip_MD5Update(&Md5Ctx, (unsigned char*)pszNonceCount, strlen(pszNonceCount));
    osip_MD5Update(&Md5Ctx, (unsigned char*)":", 1);
    osip_MD5Update(&Md5Ctx, (unsigned char*)pszCNonce, strlen(pszCNonce));
    osip_MD5Update(&Md5Ctx, (unsigned char*)":", 1);
    osip_MD5Update(&Md5Ctx, (unsigned char*)pszQop, strlen(pszQop));
    osip_MD5Update(&Md5Ctx, (unsigned char*)":", 1);

end:
    osip_MD5Update(&Md5Ctx, (unsigned char*)HA2Hex, HASHHEXLEN);
    osip_MD5Final((unsigned char*)RespHash, &Md5Ctx);
    CvtHex(RespHash, Response);
    return;
}

/*****************************************************************************
 函 数 名  : CheckDigestAuth
 功能描述  : SIP认证校验
 输入参数  : authorization_t* authorization
             int permit_anonym
             char * username
             char* password
             char* method
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int CheckDigestAuth(osip_authorization_t* authorization, int permit_anonym, char* username, char* password, char* method)
{
    int i;
    char* pszNonce = NULL;
    char* pszCNonce = NULL;
    char* pszUser = NULL;
    char* pszRealm = NULL;
    char* pszPass = NULL;
    char* pszAlg = NULL;
    char* szNonceCount = NULL;
    char* pszMethod = NULL;
    char* pszQop = NULL;
    char* pszURI = NULL;
    char* pszResponse = NULL;
    HASHHEX HA1;
    HASHHEX HA2 = "";
    HASHHEX Response;

    if (NULL == authorization || NULL == method)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "CheckDigestAuth() exit---: Param Error \r\n");
        return -1;
    }

    if (NULL == authorization->realm || NULL == authorization->auth_type)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "CheckDigestAuth() exit---: Authorization Param Error \r\n");
        return -1;
    }

#ifdef WIN32   //added by chenyu 130522

    if (stricmp(authorization->auth_type, "Digest"))
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "CheckDigestAuth() exit---: Auth Type Error \r\n");
        return -1;
    }

#else

    if (strcasecmp(authorization->auth_type, "Digest"))
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "CheckDigestAuth() exit---: Auth Type Error \r\n");
        return -1;
    }

#endif

    pszMethod = method;
    pszPass = osip_getcopy(password);

    if (pszPass == NULL)
    {
        pszPass = osip_getcopy("");
    }

    /* check Realm */
    /*pszRealm = sgetcopy_unquoted_string(authorization->realm);
    if (!IsLocalAuthRealm(pszRealm))
    {
        sfree(pszPass);
        sfree(pszRealm);
        return 0;
    }*/

    /* check algorithm is MD5 */
    if (authorization->algorithm != NULL)
    {
        pszAlg = osip_getcopy(authorization->algorithm);
        stolowercase(pszAlg);

        if (sstrcmp(pszAlg, "md5") != 0)
        {
            osip_free(pszPass);
            pszPass = NULL;
            osip_free(pszRealm);
            pszRealm = NULL;
            osip_free(pszAlg);
            pszAlg = NULL;
            SIP_DEBUG_TRACE(LOG_DEBUG, "CheckDigestAuth() exit---: Algorithm Error \r\n");
            return -1;
        }
    }
    else
    {
        pszAlg = osip_getcopy("md5");
    }

    /* check username , nonce and response */
    if (authorization->username == NULL
            || authorization->nonce == NULL
            || authorization->response == NULL)
    {
        osip_free(pszPass);
        pszPass = NULL;
        osip_free(pszRealm);
        pszRealm = NULL;
        osip_free(pszAlg);
        pszAlg = NULL;
        SIP_DEBUG_TRACE(LOG_DEBUG, "CheckDigestAuth() exit---: Check User Name Nonce And Response Error \r\n");
        return -1;
    }

    pszNonce = osip_getcopy_unquoted_string(authorization->nonce);
    pszResponse = osip_getcopy_unquoted_string(authorization->response);
    pszUser = osip_getcopy_unquoted_string(authorization->username);

    /* zhouh 2004/08/31 */
    if (username == NULL || sstrcmp(username, pszUser))
    {
        stolowercase(pszUser);

        if (!permit_anonym  || sstrcmp(pszUser, "anonymous"))
        {
            osip_free(pszPass);
            pszPass = NULL;
            osip_free(pszRealm);
            pszRealm = NULL;
            osip_free(pszAlg);
            pszAlg = NULL;
            osip_free(pszNonce);
            pszNonce = NULL;
            osip_free(pszResponse);
            pszResponse = NULL;
            osip_free(pszUser);
            pszUser = NULL;
            SIP_DEBUG_TRACE(LOG_DEBUG, "CheckDigestAuth() exit---: Check Anonymous Error \r\n");
            return -1;
        }

        osip_free(pszPass);
        pszPass = osip_getcopy("");
    }

    if (authorization->uri != NULL)
    {
        pszURI = osip_getcopy_unquoted_string(authorization->uri);
    }

    DigestCalcHA1(pszAlg, pszUser, pszRealm, pszPass, pszNonce, pszCNonce, HA1);
    DigestCalcResponse(HA1, pszNonce, szNonceCount, pszCNonce, pszQop,
                       pszMethod, pszURI, HA2, Response);


    osip_free(pszUser);
    pszUser = NULL;
    osip_free(pszPass);
    pszPass = NULL;
    osip_free(pszRealm);
    pszRealm = NULL;
    osip_free(pszNonce);
    pszNonce = NULL;
    osip_free(pszCNonce);
    pszCNonce = NULL;
    osip_free(szNonceCount);
    szNonceCount = NULL;
    osip_free(pszQop);
    pszQop = NULL;
    osip_free(pszURI);
    pszURI = NULL;
    osip_free(pszAlg);
    pszAlg = NULL;

    i = sstrcmp(Response, pszResponse);
    osip_free(pszResponse);
    pszResponse = NULL;

    if (i == 0)
    {
        return 1;
    }

    SIP_DEBUG_TRACE(LOG_DEBUG, "CheckDigestAuth() exit---: Auth Error \r\n");
    return -1;
}

int check_digest_auth(osip_authorization_t* authorization, int permit_anonym, char* username, char* password, char* method)
{
    int i = 0;
    char* pszNonce = NULL;
    char* pszCNonce = NULL;
    char* pszUser = NULL;
    char* pszRealm = NULL;
    char* pszPass = NULL;
    char* pszAlg = NULL;
    char* szNonceCount = NULL;
    char* pszMethod = NULL;
    char* pszQop = NULL;
    char* pszURI = NULL;
    char* pszResponse = NULL;
    HASHHEX HA1;
    HASHHEX HA2 = "";
    HASHHEX Response;

    if (authorization == NULL || method == NULL)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "check_digest_auth() exit---: Param Error \r\n");
        return -1;
    }

    if (authorization->realm == NULL || authorization->auth_type == NULL)
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "check_digest_auth() exit---: Authorization Param Error \r\n");
        return -1;
    }

#ifdef WIN32   //added by chenyu 130522

    if (stricmp(authorization->auth_type, "Digest"))
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "check_digest_auth() exit---: Auth Type Error \r\n");
        return -1;
    }

#else

    if (strcasecmp(authorization->auth_type, "Digest"))
    {
        SIP_DEBUG_TRACE(LOG_DEBUG, "check_digest_auth() exit---: Auth Type Error \r\n");
        return -1;
    }

#endif

    pszMethod = method;

    if (NULL == password)
    {
        pszPass = osip_getcopy("");
    }
    else
    {
        pszPass = osip_getcopy(password);
    }
    
    if (pszPass == NULL)
    {
        pszPass = osip_getcopy("");
    }

    /* check Realm */
    pszRealm = osip_getcopy_unquoted_string(authorization->realm);

    if (NULL == pszRealm)
    {
        osip_free(pszPass);
        pszPass = NULL;
        osip_free(pszRealm);
        pszRealm = NULL;

        SIP_DEBUG_TRACE(LOG_DEBUG, "check_digest_auth() exit---: Check Realm Error \r\n");
        return 0;
    }

    /* check algorithm is MD5 */
    if (authorization->algorithm != NULL)
    {
        pszAlg = osip_getcopy(authorization->algorithm);
        stolowercase(pszAlg);

        if (sstrcmp(pszAlg, "md5") != 0)
        {
            osip_free(pszPass);
            pszPass = NULL;
            osip_free(pszRealm);
            pszRealm = NULL;
            osip_free(pszAlg);
            pszAlg = NULL;
            SIP_DEBUG_TRACE(LOG_DEBUG, "check_digest_auth() exit---: Check Algorithm Error \r\n");
            return -1;
        }
    }
    else
    {
        pszAlg = osip_getcopy("md5");
    }

    /* check username , nonce and response */
    if (authorization->username == NULL
            || authorization->nonce == NULL
            || authorization->response == NULL)
    {
        osip_free(pszPass);
        pszPass = NULL;
        osip_free(pszRealm);
        pszRealm = NULL;
        osip_free(pszAlg);
        pszAlg = NULL;
        SIP_DEBUG_TRACE(LOG_DEBUG, "check_digest_auth() exit---: Check User Name, Nonce And Response Error \r\n");
        return -1;
    }

    pszNonce = osip_getcopy_unquoted_string(authorization->nonce);
    pszResponse = osip_getcopy_unquoted_string(authorization->response);
    pszUser = osip_getcopy_unquoted_string(authorization->username);

    if (username == NULL || sstrcmp(username, pszUser))
    {
        stolowercase(pszUser);

        if (!permit_anonym  || sstrcmp(pszUser, "anonymous"))
        {
            osip_free(pszPass);
            pszPass = NULL;
            osip_free(pszRealm);
            pszRealm = NULL;
            osip_free(pszAlg);
            pszAlg = NULL;
            osip_free(pszNonce);
            pszNonce = NULL;
            osip_free(pszResponse);
            pszResponse = NULL;
            osip_free(pszUser);
            pszUser = NULL;
            SIP_DEBUG_TRACE(LOG_DEBUG, "check_digest_auth() exit---: Check Anonymous Error \r\n");
            return -1;
        }

        osip_free(pszPass);
        pszPass = osip_getcopy("");
    }

    if (authorization->uri != NULL)
    {
        pszURI = osip_getcopy_unquoted_string(authorization->uri);
    }

    //printf("pszAlg:%s, pszUser:%s, pszRealm:%s, pszPass:%s, pszNonce:%s szNonceCount:%s pszCNonce:%s, pszURI:%s
    //         pszQop:%s \n",
    //    pszAlg, pszUser, pszRealm, pszPass, pszNonce, pszCNonce, szNonceCount, pszURI, pszQop);
    DigestCalcHA1(pszAlg, pszUser, pszRealm, pszPass, pszNonce, pszCNonce, HA1);
    DigestCalcResponse(HA1, pszNonce, szNonceCount, pszCNonce, pszQop,
                       pszMethod, pszURI, HA2, Response);


    osip_free(pszUser);
    pszUser = NULL;
    osip_free(pszPass);
    pszPass = NULL;
    osip_free(pszRealm);
    pszRealm = NULL;
    osip_free(pszNonce);
    pszNonce = NULL;
    osip_free(pszCNonce);
    pszCNonce = NULL;
    osip_free(szNonceCount);
    szNonceCount = NULL;
    osip_free(pszQop);
    pszQop = NULL;
    osip_free(pszURI);
    pszURI = NULL;
    osip_free(pszAlg);
    pszAlg = NULL;

    i = sstrcmp(Response, pszResponse);

    if (i == 0)
    {
        osip_free(pszResponse);
        pszResponse = NULL;
        return 1;
    }

    //SIP_DEBUG_TRACE(LOG_INFO, "check_digest_auth() Response=%s \r\n", Response);
    //SIP_DEBUG_TRACE(LOG_INFO, "check_digest_auth() pszResponse=%s \r\n", pszResponse);
    SIP_DEBUG_TRACE(LOG_DEBUG, "check_digest_auth() exit---: Auth Error \r\n");
    osip_free(pszResponse);
    pszResponse = NULL;
    
    return -1;
}

int SIP_Auth(osip_authorization_t* authorization, char* username, char* password, char* method)
{
    if (authorization != NULL && username != NULL
            && check_digest_auth(authorization, 0, username, password, method) == 1) /* check auth is ok ? */
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

