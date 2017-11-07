

#ifndef _MANSRTSP_H_
#define _MANSRTSP_H_

#ifdef ENABLE_MPATROL
#include <mpatrol.h>
#endif

#include <osipparser2/osip_list.h>


/**
 * @file sdp.h
 * @brief oSIP SDP parser Routines
 *
 * This is the SDP accessor and parser related API.
 */

/**
 * @defgroup oSIP_SDP oSIP SDP parser Handling
 * @ingroup oSIP
 * @{
 */

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * Structure for CSeq headers.
     * @defvar cseq_t
     */
    typedef struct cseqs_t cseqs_t;

    struct cseqs_t
    {
        char* number;
    };

    /**
     * Allocate a cseq element.
     * @param elem The element to work on.
     */
    int mansrtsp_cseq_init(cseqs_t** elem);
    /**
     * Free a cseq element.
     * @param elem The element to work on.
     */
    void mansrtsp_cseq_free(cseqs_t* elem);

    /**
     * Structure for scale headers.
     * @defvar cseq_t
     */
    typedef struct scale_t scale_t;

    struct scale_t
    {
        char* number;
    };

    /**
     * Allocate a scale element.
     * @param elem The element to work on.
     */
    int mansrtsp_scale_init(scale_t** elem);
    /**
     * Free a cseq element.
     * @param elem The element to work on.
     */
    void mansrtsp_scale_free(scale_t* elem);

    /**
     * Structure for range headers.
     * @defvar cseq_t
     */
    typedef struct range_t range_t;

    struct range_t
    {
        char* type;
        char* start;
        char* end;
    };

    /**
     * Allocate a range element.
     * @param elem The element to work on.
     */
    int mansrtsp_range_init(range_t** elem);
    /**
     * Free a cseq element.
     * @param elem The element to work on.
     */
    void mansrtsp_range_free(range_t* elem);
    
    /**
     * Structure for startline (1st line of MANSRTSP message either REQUEST and RESPONSE).
     * @defvar startline_t
     */
    typedef struct start_line_t start_line_t;

    struct start_line_t
    {
        /* msgevttype_t method; */
        char* method;
        char* version;

        /* resp */
        char* statuscode;
        char* reasonphrase;

    };

    /**
     * Structure for MANSRTSP Message (REQUEST and RESPONSE).
     * @defvar mansrtsp_t
     */
    typedef struct mansrtsp_t mansrtsp_t;

    struct mansrtsp_t
    {
        start_line_t* strt_line;

        cseqs_t* cseq;
        scale_t* scale;
        range_t* range;

        osip_list_t* headers;

        osip_list_t* bodies;
    };

    /**
     * Allocate a SDP packet.
     * @param sdp The element to work on.
     */
    int mansrtsp_init(mansrtsp_t** mansrtsp);

    void mansrtsp_free(mansrtsp_t* rtsp);
    /**
     * Parse a mansrtsp packet.
     * @param mansrtsp The element to work on.
     * @param buf The buffer to parse.
     */
    int mansrtsp_parse(mansrtsp_t* mansrtsp, const char* buf);
    /**
     * Get a string representation of a mansrtsp packet.
     * @param mansrtsp The element to work on.
     * @param dest The resulting new allocated buffer.
     */
    int mansrtsp_2char(mansrtsp_t* mansrtsp, char** dest);
    /**
     * Free a mansrtsp packet.
     * @param mansrtsp The element to work on.
     */
    void mansrtsp_free(mansrtsp_t* mansrtsp);

#ifdef __cplusplus
}
#endif


#endif
