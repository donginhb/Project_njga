#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <osipparser2/osip_const.h>
#include <osipparser2/osip_port.h>
#include <osipparser2/osip_message.h>
#include <src/osipparser2/parser.h>

#include <mansrtsp.inc>

int find_next_crlf(const char* start_of_header, const char** end_of_header)
{
    const char* soh = start_of_header;

    *end_of_header = NULL;        /* AMD fix */

    while (('\r' != *soh) && ('\n' != *soh))
    {
        if (*soh)
        {
            soh++;
        }
        else if ('\0' == *soh)
        {
            return OSIP_SUCCESS;
        }
        else
        {
            OSIP_TRACE(osip_trace((char*)__FILE__, __LINE__, OSIP_ERROR, NULL, (char*)"Final CRLF is missing\n"));
            return OSIP_SYNTAXERROR;
        }
    }

    if (('\r' == soh[0]) && ('\n' == soh[1]))
        /* case 1: CRLF is the separator
           case 2 or 3: CR or LF is the separator */
    {
        soh = soh + 1;
    }


    /* VERIFY if TMP is the end of header or LWS.            */
    /* LWS are extra SP, HT, CR and LF contained in headers. */
    if ((' ' == soh[1]) || ('\t' == soh[1]))
    {
        /* From now on, incoming message that potentially
           contains LWS must be processed with
           -> void osip_util_replace_all_lws(char *)
           This is because the parser methods does not
           support detection of LWS inside. */
        OSIP_TRACE(osip_trace((char*)__FILE__, __LINE__, OSIP_BUG, NULL, (char*)"Message that contains LWS must be processed with osip_util_replace_all_lws(char *tmp) before being parsed.\n"));
        return -2;
    }

    *end_of_header = soh + 1;
    return OSIP_SUCCESS;
}

void mansrtsp_startline_init(start_line_t** strt_line)
{
    *strt_line = (start_line_t*) osip_malloc(sizeof(start_line_t));

    if (NULL == *strt_line)
    {
        return;
    }

    (*strt_line)->method = NULL;
    (*strt_line)->version = NULL;
    (*strt_line)->statuscode = NULL;
    (*strt_line)->reasonphrase = NULL;

    return;
}

void mansrtsp_startline_free(start_line_t* strt_line)
{
    if (strt_line == NULL)
    {
        return;
    }

    if (strt_line->method != NULL)
    {
        osip_free(strt_line->method);
        strt_line->method = NULL;
    }

    if (strt_line->version != NULL)
    {
        osip_free(strt_line->version);
        strt_line->version = NULL;
    }

    if (strt_line->statuscode != NULL)
    {
        osip_free(strt_line->statuscode);
        strt_line->statuscode = NULL;
    }

    if (strt_line->reasonphrase != NULL)
    {
        osip_free(strt_line->reasonphrase);
        strt_line->reasonphrase = NULL;
    }

    return;
}

int mansrtsp_cseq_init(cseqs_t** b)
{
    *b = (cseqs_t*) osip_malloc(sizeof(cseqs_t));

    if (*b == NULL)
    {
        return -1;
    }

    (*b)->number = NULL;
    return 0;
}

void mansrtsp_cseq_free(cseqs_t* b)
{
    if (b == NULL)
    {
        return;
    }

    if (b->number != NULL)
    {
        osip_free(b->number);
        b->number = NULL;
    }

    return;
}

int mansrtsp_cseq_parse(cseqs_t* cseq, char* hvalue)
{
    cseq->number = osip_getcopy(hvalue);

    return 0;         /* ok */
}

int mansrtsp_setcseq(mansrtsp_t* mansrtsp, char* hvalue)
{
    int i;

    if (hvalue == NULL || hvalue[0] == '\0')
    {
        return 0;
    }

    if (mansrtsp->cseq == NULL)
    {
        return -1;
    }

    i = mansrtsp_cseq_parse(mansrtsp->cseq, hvalue);

    if (i != 0)
    {
        mansrtsp_cseq_free(mansrtsp->cseq);
        osip_free(mansrtsp->cseq);
        mansrtsp->cseq = NULL;
        return -1;
    }

    return 0;
}

int mansrtsp_scale_init(scale_t** b)
{
    *b = (scale_t*) osip_malloc(sizeof(scale_t));

    if (*b == NULL)
    {
        return -1;
    }

    (*b)->number = NULL;
    return 0;
}

void mansrtsp_scale_free(scale_t* b)
{
    if (b == NULL)
    {
        return;
    }

    if (b->number != NULL)
    {
        osip_free(b->number);
        b->number = NULL;
    }

    return;
}

int mansrtsp_scale_parse(scale_t* scale, char* hvalue)
{
    scale->number = osip_getcopy(hvalue);

    return 0;         /* ok */
}

int mansrtsp_setscale(mansrtsp_t* mansrtsp, char* hvalue)
{
    int i;

    if (hvalue == NULL || hvalue[0] == '\0')
    {
        return 0;
    }

    if (mansrtsp->scale == NULL)
    {
        return -1;
    }

    i = mansrtsp_scale_parse(mansrtsp->scale, hvalue);

    if (i != 0)
    {
        mansrtsp_scale_free(mansrtsp->scale);
        osip_free(mansrtsp->scale);
        mansrtsp->scale = NULL;
        return -1;
    }

    return 0;
}

int mansrtsp_range_init(range_t** b)
{
    *b = (range_t*) osip_malloc(sizeof(range_t));

    if (*b == NULL)
    {
        return -1;
    }

    (*b)->type = NULL;
    (*b)->start = NULL;
    (*b)->end = NULL;
    return 0;
}

void mansrtsp_range_free(range_t* b)
{
    if (b == NULL)
    {
        return;
    }

    if (b->type != NULL)
    {
        osip_free(b->type);
        b->type = NULL;
    }

    if (b->start != NULL)
    {
        osip_free(b->start);
        b->start = NULL;
    }

    if (b->end != NULL)
    {
        osip_free(b->end);
        b->end = NULL;
    }

    return;
}

int mansrtsp_range_parse(range_t* range, char* hvalue)
{
    char* method = NULL;
    char* start = NULL;
    char* end = NULL;

    range->type = NULL;
    range->start = NULL;
    range->end = NULL;

    method = strchr(hvalue, '=');     /* SEARCH FOR = */
    end = hvalue + strlen(hvalue);

    if (method == NULL)
    {
        return -1;
    }

    if (method - hvalue + 1 < 2)
    {
        printf("\n mansrtsp_range_parse malloc len error \n");
        return -1;
    }

    range->type = (char*) osip_malloc(method - hvalue + 1);

    if (range->type == NULL)
    {
        return -1;
    }

    osip_strncpy(range->type, hvalue, method - hvalue);
    sclrspace(range->type);

    if (end - method + 1 < 2)
    {
        printf("\n mansrtsp_range_parse malloc len error2 \n");
        return -1;
    }

    start = strchr(method + 1, '-');     /* SEARCH FOR - */

    if (start == NULL) /* Range:npt=1378120516 这种定位的格式 */
    {
        range->start = (char*) osip_malloc(end - method + 1);

        if (range->start == NULL)
        {
            return -1;
        }

        osip_strncpy(range->start, method + 1, end - method);
        sclrspace(range->start);
    }
    else
    {
        if (start - method + 1 < 2)
        {
            printf("\n mansrtsp_range_parse malloc len error3 \n");
            return -1;
        }

        range->start = (char*) osip_malloc(start - method + 1);

        if (range->start == NULL)
        {
            return -1;
        }

        osip_strncpy(range->start, method + 1, start - method);
        sclrspace(range->start);

        if (end - start + 1 < 2)
        {
            printf("\n mansrtsp_range_parse malloc len error4 \n");
            return -1;
        }

        range->end = (char*) osip_malloc(end - start + 1);

        if (range->end == NULL)
        {
            return -1;
        }

        osip_strncpy(range->end, start + 1, end - start);
        sclrspace(range->end);
    }

    return 0;         /* ok */
}

int mansrtsp_setrange(mansrtsp_t* mansrtsp, char* hvalue)
{
    int i;

    if (hvalue == NULL || hvalue[0] == '\0')
    {
        return 0;
    }

    if (mansrtsp->range == NULL)
    {
        return -1;
    }

    i = mansrtsp_range_parse(mansrtsp->range, hvalue);

    if (i != 0)
    {
        mansrtsp_range_free(mansrtsp->range);
        osip_free(mansrtsp->range);
        mansrtsp->range = NULL;
        return -1;
    }

    return 0;
}

int mansrtsp_init(mansrtsp_t** rtsp)
{
    int i = 0;

    (*rtsp) = (mansrtsp_t*) osip_malloc(sizeof(mansrtsp_t));

    if (*rtsp == NULL)
    {
        return -1;
    }

    mansrtsp_startline_init(&((*rtsp)->strt_line));

    if ((*rtsp)->strt_line == NULL)
    {
        osip_free(*rtsp);
        *rtsp = NULL;

        return -1;
    }

    i = mansrtsp_cseq_init(&((*rtsp)->cseq));

    if (i != 0)
    {
        mansrtsp_startline_free((*rtsp)->strt_line);
        osip_free((*rtsp)->strt_line);
        (*rtsp)->strt_line = NULL;

        osip_free(*rtsp);
        *rtsp = NULL;

        return -1;
    }

    i = mansrtsp_scale_init(&((*rtsp)->scale));

    if (i != 0)
    {
        mansrtsp_startline_free((*rtsp)->strt_line);
        osip_free((*rtsp)->strt_line);
        (*rtsp)->strt_line = NULL;

        mansrtsp_cseq_free((*rtsp)->cseq);
        osip_free((*rtsp)->cseq);
        (*rtsp)->cseq = NULL;

        osip_free(*rtsp);
        *rtsp = NULL;

        return -1;
    }

    i = mansrtsp_range_init(&((*rtsp)->range));

    if (i != 0)
    {
        mansrtsp_startline_free((*rtsp)->strt_line);
        osip_free((*rtsp)->strt_line);
        (*rtsp)->strt_line = NULL;

        mansrtsp_cseq_free((*rtsp)->cseq);
        osip_free((*rtsp)->cseq);
        (*rtsp)->cseq = NULL;

        mansrtsp_scale_free((*rtsp)->scale);
        osip_free((*rtsp)->scale);
        (*rtsp)->scale = NULL;

        osip_free(*rtsp);
        *rtsp = NULL;
        return -1;
    }

    (*rtsp)->bodies = (osip_list_t*) osip_malloc(sizeof(osip_list_t));

    if ((*rtsp)->bodies == NULL)
    {
        mansrtsp_startline_free((*rtsp)->strt_line);
        osip_free((*rtsp)->strt_line);
        (*rtsp)->strt_line = NULL;

        mansrtsp_cseq_free((*rtsp)->cseq);
        osip_free((*rtsp)->cseq);
        (*rtsp)->cseq = NULL;

        mansrtsp_scale_free((*rtsp)->scale);
        osip_free((*rtsp)->scale);
        (*rtsp)->scale = NULL;

        mansrtsp_range_free((*rtsp)->range);
        osip_free((*rtsp)->range);
        (*rtsp)->range = NULL;

        osip_free(*rtsp);
        *rtsp = NULL;

        return -1;
    }

    osip_list_init((*rtsp)->bodies);

    (*rtsp)->headers = (osip_list_t*) osip_malloc(sizeof(osip_list_t));

    if ((*rtsp)->headers == NULL)
    {
        mansrtsp_startline_free((*rtsp)->strt_line);
        osip_free((*rtsp)->strt_line);
        (*rtsp)->strt_line = NULL;

        mansrtsp_cseq_free((*rtsp)->cseq);
        osip_free((*rtsp)->cseq);
        (*rtsp)->cseq = NULL;

        mansrtsp_scale_free((*rtsp)->scale);
        osip_free((*rtsp)->scale);
        (*rtsp)->scale = NULL;

        mansrtsp_range_free((*rtsp)->range);
        osip_free((*rtsp)->range);
        (*rtsp)->range = NULL;

        osip_free((*rtsp)->bodies);
        (*rtsp)->bodies = NULL;

        osip_free(*rtsp);
        *rtsp = NULL;

        return -1;
    }

    osip_list_init((*rtsp)->headers);

    return 0;
}

void mansrtsp_free(mansrtsp_t* rtsp)
{
    if (rtsp == NULL)
    {
        return;
    }

    if (NULL != rtsp->strt_line)
    {
        mansrtsp_startline_free(rtsp->strt_line);
        osip_free(rtsp->strt_line);
        rtsp->strt_line = NULL;
    }

    if (NULL != rtsp->cseq)
    {
        mansrtsp_cseq_free(rtsp->cseq);
        osip_free(rtsp->cseq);
        rtsp->cseq = NULL;
    }

    if (NULL != rtsp->scale)
    {
        mansrtsp_scale_free(rtsp->scale);
        osip_free(rtsp->scale);
        rtsp->scale = NULL;
    }

    if (NULL != rtsp->range)
    {
        mansrtsp_range_free(rtsp->range);
        osip_free(rtsp->range);
        rtsp->range = NULL;
    }

    if (NULL != rtsp->bodies)
    {
        osip_free(rtsp->bodies);
        rtsp->bodies = NULL;
    }

    if (NULL != rtsp->headers)
    {
        osip_header_t* header = NULL;

        while (!osip_list_eol(rtsp->headers, 0))
        {
            header = (osip_header_t*) osip_list_get(rtsp->headers, 0);

            if (header != NULL)
            {
                osip_list_remove(rtsp->headers, 0);
                osip_header_free(header);
                header = NULL;
            }
        }

        osip_free(rtsp->headers);
        rtsp->headers = NULL;
    }

    return;
}

static int mansrtsp_parse_startline_request(start_line_t* dest, const char* buf, const char** headers)
{
    const char* method;
    const char* tmpbuf;

    dest->method = NULL;
    dest->version = NULL;

    tmpbuf = buf;

    method = strchr(tmpbuf, ' ');    /* search for first SPACE */

    if (method == NULL)
    {
        return -1;
    }

    if (method - tmpbuf + 1 < 2)
    {
        printf("\n mansrtsp_parse_startline_request malloc len error \n");
        return -1;
    }

    dest->method = (char*) osip_malloc(method - tmpbuf + 1);

    if (dest->method == NULL)
    {
        return -1;
    }

    osip_strncpy(dest->method, tmpbuf, method - tmpbuf);

    {
        char* hp = (char*)method;

        while ((*hp != '\r') && (*hp != '\n'))
        {
            if (*hp)
            {
                hp++;
            }
            else
            {
                return -1;
            }
        }

        if (hp - method <= 0)
        {
            printf("\n mansrtsp_parse_startline_request malloc len error2 \n");
            return -1;
        }

        dest->version = (char*) osip_malloc(hp - method);

        if (dest->version == NULL)
        {
            osip_free(dest->method);
            dest->method = NULL;
            return -1;
        }

        osip_strncpy(dest->version, method + 1, hp - method - 1);

        hp++;

        if ((*hp) && ('\r' == hp[-1]) && ('\n' == hp[0]))
        {
            hp++;
        }

        (*headers) = hp;
    }
    return 0;
}

int mansrtsp_parse_startline_response(start_line_t* dest, const char* buf, const char** headers)
{
    const char* p1;
    const char* p2;
    const char* tmpbuf;

    dest->version = NULL;
    dest->statuscode = NULL;
    dest->reasonphrase = NULL;

    tmpbuf = buf;
    /* The first token is the version name: */
    p2 = strchr(tmpbuf, ' ');

    if (p2 == NULL)
    {
        return -1;
    }

    if (p2 - tmpbuf <= 0)
    {
        printf("\n mansrtsp_parse_startline_response malloc len error \n");
        return -1;
    }

    dest->version = (char*) osip_malloc(p2 - tmpbuf + 1);

    if (dest->version == NULL)
    {
        return -1;
    }

    osip_strncpy(dest->version, tmpbuf, p2 - tmpbuf);

    /* The second token is statuscode: */
    p1 = strchr(p2 + 2, ' ');

    if (p1 == NULL)
    {
        osip_free(dest->version);
        dest->version = NULL;
        return -1;
    }

    if (p1 - p2 < 2)
    {
        osip_free(dest->version);
        dest->version = NULL;
        return -1;
    }

    dest->statuscode = (char*) osip_malloc(p1 - p2);

    if (dest->statuscode == NULL)
    {
        osip_free(dest->version);
        dest->version = NULL;
        return -1;
    }

    osip_strncpy(dest->statuscode, p2 + 1, (p1 - p2 - 1));

    /* find the the version and the beginning of headers */
    {
        char* hp = (char*)p1;

        while ((*hp != '\r') && (*hp != '\n'))
        {
            if (*hp)
            {
                hp++;
            }
            else
            {
                osip_free(dest->version);
                dest->version = NULL;

                osip_free(dest->statuscode);
                dest->statuscode = NULL;
                return -1;
            }
        }

        if (hp - p1 < 2)
        {
            osip_free(dest->version);
            dest->version = NULL;

            osip_free(dest->statuscode);
            dest->statuscode = NULL;
            return -1;
        }

        dest->reasonphrase = (char*) osip_malloc(hp - p1);

        if (NULL == dest->reasonphrase)
        {
            osip_free(dest->version);
            dest->version = NULL;

            osip_free(dest->statuscode);
            dest->statuscode = NULL;
            return -1;
        }

        osip_strncpy(dest->reasonphrase, p1 + 1, (hp - p1 - 1));

        hp++;

        if ((*hp) && ('\r' == hp[-1]) && ('\n' == hp[0]))
        {
            hp++;
        }

        (*headers) = hp;
    }
    return 0;
}

int mansrtsp_parse_startline(start_line_t* dest, const char* buf, const char** headers)
{
    if (0 == strncmp(buf, (const char*) "MANSRTSP/", 9))
    {
        return mansrtsp_parse_startline_response(dest, buf, headers);
    }
    else
    {
        return mansrtsp_parse_startline_request(dest, buf, headers);
    }
}

int mansrtsp_setheader(mansrtsp_t* mansrtsp, char* hname, char* hvalue)
{
    osip_header_t* h;
    int i;

    if (hname == NULL)
    {
        return -1;
    }

    i = osip_header_init(&h);

    if (i != 0)
    {
        return -1;
    }

    h->hname = (char*) osip_malloc(strlen(hname) + 1);

    if (h->hname == NULL)
    {
        osip_header_free(h);
        h = NULL;
        return -1;
    }

    osip_strncpy(h->hname, hname, strlen(hname));
    sclrspace(h->hname);

    if (hvalue != NULL)
    {
        /* some headers can be null ("subject:") */
        h->hvalue = (char*) osip_malloc(strlen(hvalue) + 1);

        if (h->hvalue == NULL)
        {
            osip_header_free(h);
            h = NULL;
            return -1;
        }

        osip_strncpy(h->hvalue, hvalue, strlen(hvalue));
        sclrspace(h->hvalue);
    }
    else
    {
        h->hvalue = NULL;
    }

    osip_list_add(mansrtsp->headers, h, -1);
    return 0;         /* ok */
}


int mansrtsp_handle_multiple_values(mansrtsp_t* mansrtsp, char* hname, char* hvalue)
{
    int i;

    if (hvalue == NULL)
    {
        return 0;
    }

    stolowercase(hname);

    if (strncmp(hname, "cseq", 4) == 0 && strlen(hname) == 4)
    {
        i = mansrtsp_setcseq(mansrtsp, hvalue);

        if (i == -1)
        {
            return -1;
        }

        return 0;
    }
    else if (strncmp(hname, "scale", 5) == 0 && strlen(hname) == 5)
    {
        i = mansrtsp_setscale(mansrtsp, hvalue);

        if (i == -1)
        {
            return -1;
        }

        return 0;
    }
    else if (strncmp(hname, "range", 5) == 0 && strlen(hname) == 5)
    {
        i = mansrtsp_setrange(mansrtsp, hvalue);

        if (i == -1)
        {
            return -1;
        }

        return 0;
    }
    else
    {
        i = mansrtsp_setheader(mansrtsp, hname, hvalue);

        if (i == -1)
        {
            return -1;
        }

        return 0;
    }

    return -1;            /* if comma is NULL, we should have already return 0 */
}

/* set all headers */
int mansrtsp_headers_parse(mansrtsp_t* mansrtsp, const char* start_of_header, const char** body)
{
    const char* colon_index;      /* index of ':' */
    char* hname;
    char* hvalue;
    const char* end_of_header;
    int i;

    for (;;)
    {
        if (start_of_header[0] == '\0')     /* final CRLF is missing */
        {
            OSIP_TRACE(osip_trace((char*)__FILE__, __LINE__, OSIP_INFO1, NULL, (char*)"SIP message does not end with CRLFCRLF\n"));
            return OSIP_SUCCESS;
        }

        i = find_next_crlf(start_of_header, &end_of_header);

        if (i == -2)
        {
        }
        else if (i != 0)
        {
            OSIP_TRACE(osip_trace((char*)__FILE__, __LINE__, OSIP_ERROR, NULL, (char*)"End of header Not found\n"));
            return i;                 /* this is an error case!     */
        }

        /* the list of headers MUST always end with  */
        /* CRLFCRLF (also CRCR and LFLF are allowed) */
        if ((start_of_header[0] == '\r') || (start_of_header[0] == '\n'))
        {
            *body = start_of_header;
            return OSIP_SUCCESS;      /* end of header found        */
        }

        /* find the header name */
        colon_index = strchr(start_of_header, ':');

        if (colon_index == NULL)
        {
            OSIP_TRACE(osip_trace((char*)__FILE__, __LINE__, OSIP_ERROR, NULL, (char*)"End of header Not found\n"));
            return OSIP_SUCCESS;  /* this is also an error case */
        }

        if (colon_index - start_of_header + 1 < 2)
        {
            printf("\n mansrtsp_headers_parse malloc len error \n");
            return OSIP_SYNTAXERROR;
        }

        if (end_of_header <= colon_index)
        {
            OSIP_TRACE(osip_trace((char*)__FILE__, __LINE__, OSIP_ERROR, NULL, (char*)"Malformed message\n"));
            return OSIP_SYNTAXERROR;
        }

        hname = (char*) osip_malloc(colon_index - start_of_header + 1);

        if (hname == NULL)
        {
            return OSIP_NOMEM;
        }

        osip_clrncpy(hname, start_of_header, colon_index - start_of_header);

        {
            const char* end;

            /* END of header is (end_of_header-2) if header separation is CRLF */
            /* END of header is (end_of_header-1) if header separation is CR or LF */
            if ((end_of_header[-2] == '\r') || (end_of_header[-2] == '\n'))
            {
                end = end_of_header - 2;
            }
            else
            {
                end = end_of_header - 1;
            }

            if ((end) - colon_index < 2)
            {
                printf("\n mansrtsp_headers_parse malloc len error 2 \n");
                hvalue = NULL;    /* some headers (subject) can be empty */
            }
            else
            {
                hvalue = (char*) osip_malloc((end) - colon_index + 1);

                if (hvalue == NULL)
                {
                    osip_free(hname);
                    hname = NULL;
                    return OSIP_NOMEM;
                }

                osip_clrncpy(hvalue, colon_index + 1, (end) - colon_index - 1);
            }
        }

        /* hvalue MAY contains multiple value. In this case, they   */
        /* are separated by commas. But, a comma may be part of a   */
        /* quoted-string ("here, and there" is an example where the */
        /* comma is not a separator!) */
        i = mansrtsp_handle_multiple_values(mansrtsp, hname, hvalue);

        osip_free(hname);
        hname = NULL;

        if (hvalue != NULL)
        {
            osip_free(hvalue);
            hvalue = NULL;
        }

        if (i != 0)
        {
            OSIP_TRACE(osip_trace((char*)__FILE__, __LINE__, OSIP_ERROR, NULL, (char*)"End of header Not found\n"));
            return OSIP_SYNTAXERROR;
        }

        /* continue on the next header */
        start_of_header = end_of_header;
    }

    /* Unreachable code
     OSIP_TRACE (osip_trace
              (__FILE__, __LINE__, OSIP_BUG, NULL,
               "This code cannot be reached\n")); */

    return OSIP_SYNTAXERROR;
}

/* sip_t *sip is filled while analysing buf */
int mansrtsp_parse(mansrtsp_t* mansrtsp, const char* buf)
{
    int i;
    const char* next_header_index;
    const char* ptr;

    ptr = buf;

    /* parse request or status line */
    i = mansrtsp_parse_startline(mansrtsp->strt_line, ptr, &next_header_index);

    if (i == -1)
    {
        return -1;
    }

    ptr = next_header_index;

    /* parse headers */
    i = mansrtsp_headers_parse(mansrtsp, ptr, &next_header_index);

    if (i == -1)
    {
        return -1;
    }

    ptr = next_header_index;

    return 0;
}
