
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sdp_negoc.inc>
#include <osipparser2/osip_port.h>

#include "gbltype.h"

//added by chenyu 130522
#ifdef WIN32
#define vsnprintf _vsnprintf
#define snprintf  _snprintf
#endif

/* this MUST be initialized through a call to sdp_config_init() */
sdp_config_t* config = NULL;

int sdp_context_init(sdp_context_t** con)
{
    (*con) = (sdp_context_t*) osip_malloc(sizeof(sdp_context_t));

    if (*con == NULL)
    {
        return -1;
    }

    (*con)->mycontext = NULL;
    (*con)->remote = NULL;
    (*con)->local = NULL;
    return 0;
}

void sdp_context_free(sdp_context_t* con)
{
    if (con == NULL)
    {
        return;
    }

    if (con->remote != NULL)
    {
        sdp_message_free(con->remote);
        con->remote = NULL;
    }

    if (con->local != NULL)
    {
        sdp_message_free(con->local);
        con->local = NULL;
    }

    return;
}

/* this method is used by end-user application so any pointer can
   be associated with this context (usefull to link with your own context */
int sdp_context_set_mycontext(sdp_context_t* con, void* my_instance)
{
    if (con == NULL)
    {
        return -1;
    }

    con->mycontext = my_instance;
    return 0;
}

void* sdp_context_get_mycontext(sdp_context_t* con)
{
    if (con == NULL)
    {
        return NULL;
    }

    return con->mycontext;
}

sdp_message_t* sdp_context_get_local_sdp(sdp_context_t* con)
{
    if (con == NULL)
    {
        return NULL;
    }

    return con->local;
}

int sdp_context_set_local_sdp(sdp_context_t* con, sdp_message_t* sdp)
{
    if (con == NULL)
    {
        return -1;
    }

    con->local = sdp;
    return 0;
}

sdp_message_t* sdp_context_get_remote_sdp(sdp_context_t* con)
{
    if (con == NULL)
    {
        return NULL;
    }

    return con->remote;
}

int sdp_context_set_remote_sdp(sdp_context_t* con, sdp_message_t* sdp)
{
    if (con == NULL)
    {
        return -1;
    }

    con->remote = sdp;
    return 0;
}

int payload_init(payload_t** payload)
{
    *payload = (payload_t*) osip_malloc(sizeof(payload_t));

    if (*payload == NULL)
    {
        return -1;
    }

    (*payload)->payload = NULL;
    (*payload)->number_of_port = NULL;
    (*payload)->proto = NULL;
    (*payload)->c_nettype = NULL;
    (*payload)->c_addrtype = NULL;
    (*payload)->c_addr = NULL;
    (*payload)->c_addr_multicast_ttl = NULL;
    (*payload)->c_addr_multicast_int = NULL;
    (*payload)->a_rtpmap = NULL;
    return 0;
}

void payload_free(payload_t* payload)
{
    if (payload == NULL)
    {
        return;
    }

    if (payload->payload != NULL)
    {
        osip_free(payload->payload);
        payload->payload = NULL;
    }

    if (payload->number_of_port != NULL)
    {
        osip_free(payload->number_of_port);
        payload->number_of_port = NULL;
    }

    if (payload->proto != NULL)
    {
        osip_free(payload->proto);
        payload->proto = NULL;
    }

    if (payload->c_nettype != NULL)
    {
        osip_free(payload->c_nettype);
        payload->c_nettype = NULL;
    }

    if (payload->c_addrtype != NULL)
    {
        osip_free(payload->c_addrtype);
        payload->c_addrtype = NULL;
    }

    if (payload->c_addr != NULL)
    {
        osip_free(payload->c_addr);
        payload->c_addr = NULL;
    }

    if (payload->c_addr_multicast_ttl != NULL)
    {
        osip_free(payload->c_addr_multicast_ttl);
        payload->c_addr_multicast_ttl = NULL;
    }

    if (payload->c_addr_multicast_int != NULL)
    {
        osip_free(payload->c_addr_multicast_int);
        payload->c_addr_multicast_int = NULL;
    }

    if (payload->a_rtpmap != NULL)
    {
        osip_free(payload->a_rtpmap);
        payload->a_rtpmap = NULL;
    }

    osip_free(payload);
    payload = NULL;

    return;
}

int sdp_config_init()
{
    config = (sdp_config_t*) osip_malloc(sizeof(sdp_config_t));

    if (config == NULL)
    {
        return -1;
    }

    config->o_username = NULL;
    config->o_session_id = NULL;
    config->o_session_version = NULL;
    config->o_nettype = NULL;
    config->o_addrtype = NULL;
    config->o_addr = NULL;

    config->c_nettype = NULL;
    config->c_addrtype = NULL;
    config->c_addr = NULL;
    config->c_addr_multicast_ttl = NULL;
    config->c_addr_multicast_int = NULL;


    /* supported codec for the SIP User Agent */
    config->audio_codec = (osip_list_t*) osip_malloc(sizeof(osip_list_t));

    if (config->audio_codec == NULL)
    {
        osip_free(config);
        config = NULL;
        return -1;
    }

    osip_list_init(config->audio_codec);
    config->video_codec = (osip_list_t*) osip_malloc(sizeof(osip_list_t));

    if (config->video_codec == NULL)
    {
        osip_free(config);
        config = NULL;
        osip_free(config->audio_codec);
        config->audio_codec = NULL;
        return -1;
    }

    osip_list_init(config->video_codec);

    config->fcn_set_info = NULL;
    config->fcn_set_uri = NULL;
    config->fcn_set_emails = NULL;
    config->fcn_set_phones = NULL;
    config->fcn_set_attributes = NULL;
    config->fcn_accept_audio_codec = NULL;
    config->fcn_accept_video_codec = NULL;
    return 0;
}

void sdp_config_free()
{
    if (config == NULL)
    {
        return;
    }

    if (config->o_username != NULL)
    {
        osip_free(config->o_username);
        config->o_username = NULL;
    }

    if (config->o_session_id != NULL)
    {
        osip_free(config->o_session_id);
        config->o_session_id = NULL;
    }

    if (config->o_session_version != NULL)
    {
        osip_free(config->o_session_version);
        config->o_session_version = NULL;
    }

    if (config->o_nettype != NULL)
    {
        osip_free(config->o_nettype);
        config->o_nettype = NULL;
    }

    if (config->o_addrtype != NULL)
    {
        osip_free(config->o_addrtype);
        config->o_addrtype = NULL;
    }

    if (config->o_addr != NULL)
    {
        osip_free(config->o_addr);
        config->o_addr = NULL;
    }

    if (config->c_nettype != NULL)
    {
        osip_free(config->c_nettype);
        config->c_nettype = NULL;
    }

    if (config->c_addrtype != NULL)
    {
        osip_free(config->c_addrtype);
        config->c_addrtype = NULL;
    }

    if (config->c_addr != NULL)
    {
        osip_free(config->c_addr);
        config->c_addr = NULL;
    }

    if (config->c_addr_multicast_ttl != NULL)
    {
        osip_free(config->c_addr_multicast_ttl);
        config->c_addr_multicast_ttl = NULL;
    }

    if (config->c_addr_multicast_int != NULL)
    {
        osip_free(config->c_addr_multicast_int);
        config->c_addr_multicast_int = NULL;
    }

    if (config->audio_codec != NULL)
    {
        osip_list_special_free(config->audio_codec, (void (*)(void*)) &payload_free);
        osip_free(config->audio_codec);
        config->audio_codec = NULL;
    }

    if (config->video_codec != NULL)
    {
        osip_list_special_free(config->video_codec, (void (*)(void*)) &payload_free);
        osip_free(config->video_codec);
        config->video_codec = NULL;
    }

    /* other are pointer to func, they don't need free() calls */

    /* yes, this is done here... :) */
    osip_free(config);
    config = NULL;
    return;
}

int sdp_config_set_o_username(char* tmp)
{
    if (config == NULL)
    {
        return -1;
    }

    config->o_username = tmp;
    return 0;
}

int sdp_config_set_o_session_id(char* tmp)
{
    if (config == NULL)
    {
        return -1;
    }

    config->o_session_id = tmp;
    return 0;
}

int sdp_config_set_o_session_version(char* tmp)
{
    if (config == NULL)
    {
        return -1;
    }

    config->o_session_version = tmp;
    return 0;
}

int sdp_config_set_o_nettype(char* tmp)
{
    if (config == NULL)
    {
        return -1;
    }

    config->o_nettype = tmp;
    return 0;
}

int sdp_config_set_o_addrtype(char* tmp)
{
    if (config == NULL)
    {
        return -1;
    }

    config->o_addrtype = tmp;
    return 0;
}

int sdp_config_set_o_addr(char* tmp)
{
    if (config == NULL)
    {
        return -1;
    }

    if (config->o_addr == NULL)
    {
        config->o_addr = tmp;
    }

    return 0;
}

int sdp_config_set_c_nettype(char* tmp)
{
    if (config == NULL)
    {
        return -1;
    }

    config->c_nettype = tmp;
    return 0;
}

int sdp_config_set_c_addrtype(char* tmp)
{
    if (config == NULL)
    {
        return -1;
    }

    config->c_addrtype = tmp;
    return 0;
}

int sdp_config_set_c_addr(char* tmp)
{
    if (config == NULL)
    {
        return -1;
    }

    if (config->c_addr == NULL)
    {
        config->c_addr = tmp;
    }

    return 0;
}

int sdp_config_set_c_addr_multicast_ttl(char* tmp)
{
    if (config == NULL)
    {
        return -1;
    }

    config->c_addr_multicast_ttl = tmp;
    return 0;
}

int sdp_config_set_c_addr_multicast_int(char* tmp)
{
    if (config == NULL)
    {
        return -1;
    }

    config->c_addr_multicast_int = tmp;
    return 0;
}

int sdp_config_add_support_for_audio_codec(char* payload, char* number_of_port,
        char* proto, char* c_nettype,
        char* c_addrtype, char* c_addr,
        char* c_addr_multicast_ttl,
        char* c_addr_multicast_int,
        char* a_rtpmap)
{
    int i;
    payload_t* my_payload = NULL;

    i = payload_init(&my_payload);

    if (i != 0)
    {
        return -1;
    }

    my_payload->payload = payload;
    my_payload->number_of_port = number_of_port;
    my_payload->proto = proto;
    my_payload->c_nettype = c_nettype;
    my_payload->c_addrtype = c_addrtype;
    my_payload->c_addr = c_addr;
    my_payload->c_addr_multicast_ttl = c_addr_multicast_ttl;
    my_payload->c_addr_multicast_int = c_addr_multicast_int;
    my_payload->a_rtpmap = a_rtpmap;
    osip_list_add(config->audio_codec, my_payload, -1);
    return 0;
}

int sdp_config_add_support_for_video_codec(char* payload, char* number_of_port,
        char* proto, char* c_nettype,
        char* c_addrtype, char* c_addr,
        char* c_addr_multicast_ttl,
        char* c_addr_multicast_int,
        char* a_rtpmap)
{
    int i;
    payload_t* my_payload = NULL;

    i = payload_init(&my_payload);

    if (i != 0)
    {
        return -1;
    }

    my_payload->payload = payload;
    my_payload->number_of_port = number_of_port;
    my_payload->proto = proto;
    my_payload->c_nettype = c_nettype;
    my_payload->c_addrtype = c_addrtype;
    my_payload->c_addr = c_addr;
    my_payload->c_addr_multicast_ttl = c_addr_multicast_ttl;
    my_payload->c_addr_multicast_int = c_addr_multicast_int;
    my_payload->a_rtpmap = a_rtpmap;
    osip_list_add(config->video_codec, my_payload, -1);
    return 0;
}

int sdp_config_remove_audio_payloads()
{
    osip_list_special_free(config->audio_codec, (void (*)(void*)) &payload_free);
    return 0;
}

int sdp_config_remove_video_payloads()
{
    osip_list_special_free(config->video_codec, (void (*)(void*)) &payload_free);
    return 0;
}

payload_t* sdp_config_find_audio_payload(char* payload)
{
    payload_t* my = NULL;
    size_t length = strlen(payload);
    int pos = 0;

    if (payload == NULL)
    {
        return NULL;
    }

    while (!osip_list_eol(config->audio_codec, pos))
    {
        my = (payload_t*) osip_list_get(config->audio_codec, pos);

        if (strlen(my->payload) == length)
        {
            if (0 == strncmp(my->payload, payload, length))
            {
                return my;
            }
        }

        pos++;
    }

    return NULL;
}

payload_t* sdp_config_find_video_payload(char* payload)
{
    payload_t* my = NULL;
    size_t length = strlen(payload);
    int pos = 0;

    if (payload == NULL)
    {
        return NULL;
    }

    while (!osip_list_eol(config->video_codec, pos))
    {
        my = (payload_t*) osip_list_get(config->video_codec, pos);

        if (strlen(my->payload) == length)
        {
            if (0 == strncmp(my->payload, payload, length))
            {
                return my;
            }
        }

        pos++;
    }

    return NULL;
}

int sdp_config_set_fcn_set_info(int (*fcn)(sdp_context_t*, sdp_message_t*))
{
    if (config == NULL)
    {
        return -1;
    }

    config->fcn_set_info = (int (*)(void*, sdp_message_t*)) fcn;
    return 0;
}

int sdp_config_set_fcn_set_uri(int (*fcn)(sdp_context_t*, sdp_message_t*))
{
    if (config == NULL)
    {
        return -1;
    }

    config->fcn_set_uri = (int (*)(void*, sdp_message_t*)) fcn;
    return 0;
}

int sdp_config_set_fcn_set_emails(int (*fcn)(sdp_context_t*, sdp_message_t*))
{
    if (config == NULL)
    {
        return -1;
    }

    config->fcn_set_emails = (int (*)(void*, sdp_message_t*)) fcn;
    return 0;
}

int sdp_config_set_fcn_set_phones(int (*fcn)(sdp_context_t*, sdp_message_t*))
{
    if (config == NULL)
    {
        return -1;
    }

    config->fcn_set_phones = (int (*)(void*, sdp_message_t*)) fcn;
    return 0;
}

int sdp_config_set_fcn_set_attributes(int (*fcn)(sdp_context_t*, sdp_message_t*, int))
{
    if (config == NULL)
    {
        return -1;
    }

    config->fcn_set_attributes = (int (*)(void*, sdp_message_t*, int)) fcn;
    return 0;
}

int sdp_config_set_fcn_accept_audio_codec(int (*fcn)(sdp_context_t*, char*,
        char*, int, char*))
{
    if (config == NULL)
    {
        return -1;
    }

    config->fcn_accept_audio_codec = (int (*)(void*, char*,
                                      char*, int, char*)) fcn;
    return 0;
}

int sdp_config_set_fcn_accept_video_codec(int (*fcn)(sdp_context_t*, char*,
        char*, int, char*))
{
    if (config == NULL)
    {
        return -1;
    }

    config->fcn_accept_video_codec = (int (*)(void*, char*,
                                      char*, int, char*)) fcn;
    return 0;
}

int sdp_partial_clone(sdp_context_t* con, sdp_message_t* remote, sdp_message_t** dest, char* localip)
{
    int i = 0;

    sdp_message_v_version_set(*dest, osip_getcopy("0"));

    /* those fields MUST be set */
    if (localip != NULL)
    {
        sdp_message_o_origin_set(*dest,
                                 osip_getcopy(config->o_username),
                                 osip_getcopy(config->o_session_id),
                                 osip_getcopy(config->o_session_version),
                                 osip_getcopy(config->o_nettype),
                                 osip_getcopy(config->o_addrtype), osip_getcopy(localip));
    }
    else
    {
        sdp_message_o_origin_set(*dest,
                                 osip_getcopy(config->o_username),
                                 osip_getcopy(config->o_session_id),
                                 osip_getcopy(config->o_session_version),
                                 osip_getcopy(config->o_nettype),
                                 osip_getcopy(config->o_addrtype), osip_getcopy(config->o_addr));
    }

    sdp_message_s_name_set(*dest, osip_getcopy(remote->s_name));

    if (config->fcn_set_info != NULL)
    {
        config->fcn_set_info(con, *dest);
    }

    if (config->fcn_set_uri != NULL)
    {
        config->fcn_set_uri(con, *dest);
    }

    if (config->fcn_set_emails != NULL)
    {
        config->fcn_set_emails(con, *dest);
    }

    if (config->fcn_set_phones != NULL)
    {
        config->fcn_set_phones(con, *dest);
    }

    if (config->c_nettype != NULL)
    {
        if (localip != NULL)
        {
            sdp_message_c_connection_add(*dest, -1,
                                         osip_getcopy(config->c_nettype),
                                         osip_getcopy(config->c_addrtype),
                                         osip_getcopy(localip),
                                         osip_getcopy(config->c_addr_multicast_ttl),
                                         osip_getcopy(config->c_addr_multicast_int));
        }
        else
        {
            sdp_message_c_connection_add(*dest, -1,
                                         osip_getcopy(config->c_nettype),
                                         osip_getcopy(config->c_addrtype),
                                         osip_getcopy(config->c_addr),
                                         osip_getcopy(config->c_addr_multicast_ttl),
                                         osip_getcopy(config->c_addr_multicast_int));
        }
    }

    {
        /* offer-answer draft says we must copy the "t=" line */
        char* tmp = sdp_message_t_start_time_get(remote, 0);
        char* tmp2 = sdp_message_t_stop_time_get(remote, 0);

        if (tmp == NULL || tmp2 == NULL)
        {
            return -1;    /* no t line?? */
        }

        i = sdp_message_t_time_descr_add(*dest, osip_getcopy(tmp), osip_getcopy(tmp2));

        if (i != 0)
        {
            return -1;
        }
    }

    if (config->fcn_set_attributes != NULL)
    {
        config->fcn_set_attributes(con, *dest, -1);
    }

    return 0;
}

int sdp_confirm_media(sdp_context_t* context, sdp_message_t* remote, sdp_message_t** dest, int audio_code_type, int video_code_type)
{
    char* payload = NULL;
    char* tmp = NULL, *tmp2 = NULL, *tmp3 = NULL, *tmp4 = NULL;
    int ret = 0;
    int i = 0;
    int k = 0;
    int audio_qty = 0;        /* accepted audio line: do not accept more than one */
    int video_qty = 0;

    i = 0;

    while (!sdp_message_endof_media(remote, i))
    {
        tmp = sdp_message_m_media_get(remote, i);
        tmp2 = sdp_message_m_port_get(remote, i);
        tmp3 = sdp_message_m_number_of_port_get(remote, i);
        tmp4 = sdp_message_m_proto_get(remote, i);

        if (tmp == NULL)
        {
            return -1;
        }

        sdp_message_m_media_add(*dest, osip_getcopy(tmp), osip_getcopy("0"),
                                NULL, osip_getcopy(tmp4));
        k = 0;

        if (0 == strncmp(tmp, "audio", 5))
        {
            do
            {
                payload = sdp_message_m_payload_get(remote, i, k);

                if (payload != NULL)
                {
                    payload_t* my_payload = sdp_config_find_audio_payload(payload);

                    if (my_payload != NULL)   /* payload is supported */
                    {
                        ret = -1; /* somtimes, codec can be refused even if supported */

                        if (audio_code_type >= 0)
                        {
                            if (0 == audio_code_type || EV9000_STREAMDATA_TYPE_AUDIO_G723 == audio_code_type)
                            {
                                ret = strncmp(payload, "4", 1);
                            }
                            else if (1 == audio_code_type || EV9000_STREAMDATA_TYPE_AUDIO_G711A == audio_code_type)
                            {
                                ret = strncmp(payload, "8", 1);
                            }
                            else if (2 == audio_code_type || EV9000_STREAMDATA_TYPE_AUDIO_G722 == audio_code_type)
                            {
                                ret = strncmp(payload, "9", 1);
                            }
                            else if (3 == audio_code_type || EV9000_STREAMDATA_TYPE_AUDIO_G729 == audio_code_type)
                            {
                                ret = strncmp(payload, "18", 2);
                            }
                            else if (4 == audio_code_type || EV9000_STREAMDATA_TYPE_AUDIO_SVAC == audio_code_type)
                            {
                                ret = strncmp(payload, "20", 3);
                            }
                        }
                        else
                        {
                            if (config->fcn_accept_audio_codec != NULL)
                            {
                                ret = config->fcn_accept_audio_codec(context, tmp2, tmp3, audio_qty, payload);
                            }
                        }

                        if (0 == ret)
                        {
                            sdp_message_m_payload_add(*dest, i, osip_getcopy(payload));

                            if (my_payload->a_rtpmap != NULL)
                            {
                                sdp_message_a_attribute_add(*dest, i, osip_getcopy("rtpmap"), osip_getcopy(my_payload->a_rtpmap));
                            }

                            if (my_payload->c_nettype != NULL)
                            {
                                sdp_media_t* med = (sdp_media_t*)osip_list_get(&(*dest)->m_medias, i);

                                if (osip_list_eol(&med->c_connections, 0))
                                {
                                    sdp_message_c_connection_add(*dest, i,
                                                                 osip_getcopy(my_payload->c_nettype),
                                                                 osip_getcopy(my_payload->c_addrtype),
                                                                 osip_getcopy(my_payload->c_addr),
                                                                 osip_getcopy(my_payload->c_addr_multicast_ttl),
                                                                 osip_getcopy(my_payload->c_addr_multicast_int));
                                }
                            }
                        }
                    }
                }

                k++;
            }
            while (payload != NULL);

            if (NULL != sdp_message_m_payload_get(*dest, i, 0))
            {
                audio_qty = 1;
            }
        }
        else if (0 == strncmp(tmp, "video", 5))
        {
            do
            {
                payload = sdp_message_m_payload_get(remote, i, k);

                if (payload != NULL)
                {
                    payload_t* my_payload = sdp_config_find_video_payload(payload);

                    if (my_payload != NULL)   /* payload is supported */
                    {
                        ret = -1;

                        if (video_code_type >= 0)
                        {
                            if (0 == video_code_type || EV9000_STREAMDATA_TYPE_PS == video_code_type)
                            {
                                ret = strncmp(payload, "96", 2);
                            }
                            else if (1 == video_code_type || EV9000_STREAMDATA_TYPE_VIDEO_MPEG4 == video_code_type)
                            {
                                ret = strncmp(payload, "97", 2);
                            }
                            else if (2 == video_code_type || EV9000_STREAMDATA_TYPE_VIDEO_H264 == video_code_type)
                            {
                                ret = strncmp(payload, "98", 2);
                            }
                            else if (3 == video_code_type || EV9000_STREAMDATA_TYPE_VIDEO_SVAC == video_code_type)
                            {
                                ret = strncmp(payload, "99", 2);
                            }
                            else if (4 == video_code_type || EV9000_STREAMDATA_TYPE_VIDEO_HIK == video_code_type)
                            {
                                ret = strncmp(payload, "500", 3);
                            }
                            else if (5 == video_code_type || EV9000_STREAMDATA_TYPE_VIDEO_DAH == video_code_type)
                            {
                                ret = strncmp(payload, "501", 3);
                            }
                            else if (6 == video_code_type || EV9000_STREAMDATA_TYPE_VIDEO_NETPOSA == video_code_type)
                            {
                                ret = strncmp(payload, "502", 3);
                            }
                            else if (7 == video_code_type || EV9000_STREAMDATA_TYPE_VIDEO_WENAN == video_code_type)
                            {
                                ret = strncmp(payload, "503", 3);
                            }
                        }
                        else
                        {
                            if (config->fcn_accept_video_codec != NULL)
                            {
                                ret = config->fcn_accept_video_codec(context, tmp2, tmp3, video_qty, payload);
                            }
                        }

                        if (0 == ret)
                        {
                            sdp_message_m_payload_add(*dest, i, osip_getcopy(payload));

                            /* TODO  set the attribute list (rtpmap..) */
                            if (my_payload->a_rtpmap != NULL)
                            {
                                sdp_message_a_attribute_add(*dest, i, osip_getcopy("rtpmap"), osip_getcopy(my_payload->a_rtpmap));
                            }

                            if (my_payload->c_nettype != NULL)
                            {
                                sdp_media_t* med = (sdp_media_t*)osip_list_get(&(*dest)->m_medias, i);

                                if (osip_list_eol(&med->c_connections, 0))
                                {
                                    sdp_message_c_connection_add(*dest, i,
                                                                 osip_getcopy(my_payload->c_nettype),
                                                                 osip_getcopy(my_payload->c_addrtype),
                                                                 osip_getcopy(my_payload->c_addr),
                                                                 osip_getcopy(my_payload->c_addr_multicast_ttl),
                                                                 osip_getcopy(my_payload->c_addr_multicast_int));
                                }
                            }
                        }
                    }
                }

                k++;
            }
            while (payload != NULL);

            if (NULL != sdp_message_m_payload_get(*dest, i, 0))
            {
                video_qty = 1;
            }
        }

        i++;
    }

    return 0;
}

int sdp_context_execute_negociation(sdp_context_t* context, char* audio_port,
                                    char* video_port, char* localip, int audio_code_type, int video_code_type)
{
    int m_lines_that_match = 0;
    sdp_message_t* remote = NULL;
    sdp_message_t* local = NULL;
    int i = 0;

    if (context == NULL)
    {
        return -1;
    }

    remote = context->remote;

    if (remote == NULL)
    {
        return -1;
    }

    i = sdp_message_init(&local);

    if (i != 0)
    {
        return -1;
    }

    if (0 != strncmp(remote->v_version, "0", 1))
    {
        sdp_message_free(local);
        local = NULL;
        /*      sdp_context->fcn_wrong_version(context); */
        return 406;       /* Not Acceptable */
    }

    i = sdp_partial_clone(context, remote, &local, localip);

    if (i != 0)
    {
        sdp_message_free(local);
        local = NULL;
        return -1;
    }

    i = sdp_confirm_media(context, remote, &local, audio_code_type, video_code_type);

    if (i != 0)
    {
        sdp_message_free(local);
        local = NULL;
        return i;
    }

    i = 0;

    while (!sdp_message_endof_media(local, i))
    {
        /* this is to refuse each line with no codec that matches! */
        if (NULL == sdp_message_m_payload_get(local, i, 0))
        {
            sdp_media_t* med = (sdp_media_t*)osip_list_get(&(local)->m_medias, i);
            char* str = sdp_message_m_payload_get(remote, i, 0);

            sdp_message_m_payload_add(local, i, osip_getcopy(str));
            osip_free(med->m_port);
            med->m_port = NULL;
            med->m_port = osip_getcopy("0");  /* refuse this line */
        }
        else
        {
            /* number of "m" lines that match */
            sdp_media_t* med = (sdp_media_t*)osip_list_get(&local->m_medias, i);

            m_lines_that_match++;
            osip_free(med->m_port);
            med->m_port = NULL;

            /* AMD: use the correct fcn_get_xxx_port method: */
            if (0 == strcmp(med->m_media, "audio"))
            {
                if (audio_port != NULL)
                {
                    med->m_port = osip_getcopy(audio_port);
                }
                else
                {
                    med->m_port = osip_getcopy("0");    /* should never happen */
                }
            }
            else if (0 == strcmp(med->m_media, "video"))
            {
                if (video_port != NULL)
                {
                    med->m_port = osip_getcopy(video_port);
                }
                else
                {
                    med->m_port = osip_getcopy("0");    /* should never happen */
                }
            }
        }

        i++;
    }

    if (m_lines_that_match > 0)
    {
        context->local = local;
        return 200;
    }
    else
    {
        sdp_message_free(local);
        local = NULL;
        return 415;
    }
}

/*****************************************************************************
 函 数 名  : sdp_build_offer
 功能描述  : 构建SDP信息
 输入参数  : sdp_context_t* con
             sdp_message_t** sdp
             char* audio_port
             char* video_port
             char* localip
             char* s_name
             int start_time
             int end_time
             int play_time
             int media_direction
             int audio_code_type
             int video_code_type
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年8月22日 星期五
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int sdp_build_offer(sdp_context_t* con, sdp_message_t** sdp, char* audio_port,
                    char* video_port, char* localip, char* s_name, int start_time, int end_time, int play_time, int media_direction, int audio_code_type, int video_code_type)
{
    int i;
    int media_line = 0;

    if (NULL == localip)
    {
        return -1;
    }

    i = sdp_message_init(sdp);

    if (i != 0)
    {
        return -1;
    }

    sdp_message_v_version_set(*sdp, osip_getcopy("0"));

    /* those fields MUST be set */
    sdp_message_o_origin_set(*sdp,
                             osip_getcopy(config->o_username),
                             osip_getcopy(config->o_session_id),
                             osip_getcopy(config->o_session_version),
                             osip_getcopy(config->o_nettype),
                             osip_getcopy(config->o_addrtype), osip_getcopy(localip));

    if (NULL != s_name)
    {
        sdp_message_s_name_set(*sdp, osip_getcopy(s_name));
    }
    else
    {
        sdp_message_s_name_set(*sdp, osip_getcopy("A call"));
    }

    if (config->fcn_set_info != NULL)
    {
        config->fcn_set_info(con, *sdp);
    }

    if (config->fcn_set_uri != NULL)
    {
        config->fcn_set_uri(con, *sdp);
    }

    if (config->fcn_set_emails != NULL)
    {
        config->fcn_set_emails(con, *sdp);
    }

    if (config->fcn_set_phones != NULL)
    {
        config->fcn_set_phones(con, *sdp);
    }

    if (config->c_nettype != NULL)
    {
        sdp_message_c_connection_add(*sdp, -1,
                                     osip_getcopy(config->c_nettype),
                                     osip_getcopy(config->c_addrtype),
                                     osip_getcopy(localip),
                                     osip_getcopy(config->c_addr_multicast_ttl),
                                     osip_getcopy(config->c_addr_multicast_int));
    }

    {
        /* offer-answer draft says we must copy the "t=" line */
        char* tmp = (char*)osip_malloc(16);

        if (NULL == tmp)
        {
            return -1;
        }

        char* tmp2 = (char*)osip_malloc(16);

        if (NULL == tmp2)
        {
            osip_free(tmp);
            tmp = NULL;
            return -1;
        }

        char* tmp3 = (char*)osip_malloc(16);

        if (NULL == tmp3)
        {
            osip_free(tmp);
            tmp = NULL;
            osip_free(tmp2);
            tmp2 = NULL;
            return -1;
        }

        snprintf(tmp, 16, "%i", start_time);
        snprintf(tmp2, 16, "%i", end_time);

        i = sdp_message_t_time_descr_add(*sdp, tmp, tmp2);

        if (i != 0)
        {
            osip_free(tmp);
            tmp = NULL;
            osip_free(tmp2);
            tmp2 = NULL;
            osip_free(tmp3);
            tmp3 = NULL;
            return -1;
        }

        if (play_time > 0)
        {
            snprintf(tmp3, 16, "%i", play_time);

            i = sdp_message_r_repeat_add(*sdp, 0, tmp3);

            if (i != 0)
            {
                osip_free(tmp);
                tmp = NULL;
                osip_free(tmp2);
                tmp2 = NULL;
                osip_free(tmp3);
                tmp3 = NULL;
                return -1;
            }
        }
        else
        {
            osip_free(tmp3);
            tmp3 = NULL;
        }
    }

    if (config->fcn_set_attributes != NULL)
    {
        config->fcn_set_attributes(con, *sdp, -1);
    }

    if (osip_atoi(audio_port) > 0)
    {
        if (audio_code_type >= 0)
        {
            payload_t* my = NULL;

            if (0 == audio_code_type || EV9000_STREAMDATA_TYPE_AUDIO_G723 == audio_code_type)
            {
                my = (payload_t*) osip_list_get(config->audio_codec, 0);

                if (NULL == my)
                {
                    return -1;
                }
            }
            else if (1 == audio_code_type || EV9000_STREAMDATA_TYPE_AUDIO_G711A == audio_code_type)
            {
                my = (payload_t*) osip_list_get(config->audio_codec, 1);

                if (NULL == my)
                {
                    return -1;
                }
            }
            else if (2 == audio_code_type || EV9000_STREAMDATA_TYPE_AUDIO_G722 == audio_code_type)
            {
                my = (payload_t*) osip_list_get(config->audio_codec, 2);

                if (NULL == my)
                {
                    return -1;
                }
            }
            else if (3 == audio_code_type || EV9000_STREAMDATA_TYPE_AUDIO_G729 == audio_code_type)
            {
                my = (payload_t*) osip_list_get(config->audio_codec, 3);

                if (NULL == my)
                {
                    return -1;
                }
            }
            else if (4 == audio_code_type || EV9000_STREAMDATA_TYPE_AUDIO_SVAC == audio_code_type)
            {
                my = (payload_t*) osip_list_get(config->audio_codec, 4);

                if (NULL == my)
                {
                    return -1;
                }
            }
            else
            {
                return -1;
            }

            /* all media MUST have the same PROTO, PORT. */
            sdp_message_m_media_add(*sdp, osip_getcopy("audio"), osip_getcopy(audio_port),
                                    /*my->number_of_port, sgetcopy (my->proto));*/
                                    osip_getcopy(my->number_of_port), osip_getcopy(my->proto));   /* update to 2.0.6 */


            sdp_message_m_payload_add(*sdp, media_line, osip_getcopy(my->payload));

            if (my->a_rtpmap != NULL)
            {
                sdp_message_a_attribute_add(*sdp, media_line, osip_getcopy("rtpmap"),
                                            osip_getcopy(my->a_rtpmap));
            }

            switch (media_direction)
            {
                case 0 :
                    sdp_message_a_attribute_add(*sdp, media_line, osip_getcopy("inactive"), NULL);
                    break;

                case 1 :
                    sdp_message_a_attribute_add(*sdp, media_line, osip_getcopy("sendonly"), NULL);
                    break;

                case 2 :
                    sdp_message_a_attribute_add(*sdp, media_line, osip_getcopy("recvonly"), NULL);
                    break;

                case 3 :
                    sdp_message_a_attribute_add(*sdp, media_line, osip_getcopy("sendrecv"), NULL);
                    break;

                default:
                    sdp_message_a_attribute_add(*sdp, media_line, osip_getcopy("sendrecv"), NULL);
            }
        }
        else
        {
            /* add all audio codec */
            if (!osip_list_eol(config->audio_codec, 0))
            {
                int pos = 0;
                payload_t* my = (payload_t*) osip_list_get(config->audio_codec, pos);

                /* all media MUST have the same PROTO, PORT. */
                sdp_message_m_media_add(*sdp, osip_getcopy("audio"), osip_getcopy(audio_port),
                                        /*my->number_of_port, sgetcopy (my->proto));*/
                                        osip_getcopy(my->number_of_port), osip_getcopy(my->proto));   /* update to 2.0.6 */

                while (!osip_list_eol(config->audio_codec, pos))
                {
                    my = (payload_t*) osip_list_get(config->audio_codec, pos);

                    if (NULL == my)
                    {
                        pos++;
                        continue;
                    }

                    sdp_message_m_payload_add(*sdp, media_line, osip_getcopy(my->payload));

                    if (my->a_rtpmap != NULL)
                    {
                        sdp_message_a_attribute_add(*sdp, media_line, osip_getcopy("rtpmap"),
                                                    osip_getcopy(my->a_rtpmap));
                    }

                    pos++;
                }

                switch (media_direction)
                {
                    case 0 :
                        sdp_message_a_attribute_add(*sdp, media_line, osip_getcopy("inactive"), NULL);
                        break;

                    case 1 :
                        sdp_message_a_attribute_add(*sdp, media_line, osip_getcopy("sendonly"), NULL);
                        break;

                    case 2 :
                        sdp_message_a_attribute_add(*sdp, media_line, osip_getcopy("recvonly"), NULL);
                        break;

                    case 3 :
                        sdp_message_a_attribute_add(*sdp, media_line, osip_getcopy("sendrecv"), NULL);
                        break;

                    default:
                        sdp_message_a_attribute_add(*sdp, media_line, osip_getcopy("sendrecv"), NULL);
                }

                media_line++;
            }
        }
    }

    if (osip_atoi(video_port) > 0)
    {
        if (video_code_type >= 0)
        {
            payload_t* my = NULL;

            if (0 == video_code_type || EV9000_STREAMDATA_TYPE_PS == video_code_type)
            {
                my = (payload_t*) osip_list_get(config->video_codec, 0);

                if (NULL == my)
                {
                    return -1;
                }
            }
            else if (1 == video_code_type || EV9000_STREAMDATA_TYPE_VIDEO_MPEG4 == video_code_type)
            {
                my = (payload_t*) osip_list_get(config->video_codec, 1);

                if (NULL == my)
                {
                    return -1;
                }
            }
            else if (2 == video_code_type || EV9000_STREAMDATA_TYPE_VIDEO_H264 == video_code_type)
            {
                my = (payload_t*) osip_list_get(config->video_codec, 2);

                if (NULL == my)
                {
                    return -1;
                }
            }
            else if (3 == video_code_type || EV9000_STREAMDATA_TYPE_VIDEO_SVAC == video_code_type)
            {
                my = (payload_t*) osip_list_get(config->video_codec, 3);

                if (NULL == my)
                {
                    return -1;
                }
            }
            else if (4 == video_code_type || EV9000_STREAMDATA_TYPE_VIDEO_HIK == video_code_type)
            {
                my = (payload_t*) osip_list_get(config->video_codec, 4);

                if (NULL == my)
                {
                    return -1;
                }
            }
            else if (5 == video_code_type || EV9000_STREAMDATA_TYPE_VIDEO_DAH == video_code_type)
            {
                my = (payload_t*) osip_list_get(config->video_codec, 5);

                if (NULL == my)
                {
                    return -1;
                }
            }
            else if (6 == video_code_type || EV9000_STREAMDATA_TYPE_VIDEO_NETPOSA == video_code_type)
            {
                my = (payload_t*) osip_list_get(config->video_codec, 6);

                if (NULL == my)
                {
                    return -1;
                }
            }
            else if (7 == video_code_type || EV9000_STREAMDATA_TYPE_VIDEO_WENAN == video_code_type)
            {
                my = (payload_t*) osip_list_get(config->video_codec, 7);

                if (NULL == my)
                {
                    return -1;
                }
            }
            else
            {
                return -1;
            }

            /* all media MUST have the same PROTO, PORT. */
            sdp_message_m_media_add(*sdp, osip_getcopy("video"), osip_getcopy(video_port),
                                    /*my->number_of_port, sgetcopy (my->proto));*/
                                    osip_getcopy(my->number_of_port), osip_getcopy(my->proto));  /* update to 2.0.6 */


            sdp_message_m_payload_add(*sdp, media_line, osip_getcopy(my->payload));

            if (my->a_rtpmap != NULL)
            {
                sdp_message_a_attribute_add(*sdp, media_line, osip_getcopy("rtpmap"),
                                            osip_getcopy(my->a_rtpmap));
            }

            switch (media_direction)
            {
                case 0 :
                    sdp_message_a_attribute_add(*sdp, media_line, osip_getcopy("inactive"), NULL);
                    break;

                case 1 :
                    sdp_message_a_attribute_add(*sdp, media_line, osip_getcopy("sendonly"), NULL);
                    break;

                case 2 :
                    sdp_message_a_attribute_add(*sdp, media_line, osip_getcopy("recvonly"), NULL);
                    break;

                case 3 :
                    sdp_message_a_attribute_add(*sdp, media_line, osip_getcopy("sendrecv"), NULL);
                    break;

                default:
                    sdp_message_a_attribute_add(*sdp, media_line, osip_getcopy("sendrecv"), NULL);
            }
        }
        else
        {
            /* add all video codec */
            if (!osip_list_eol(config->video_codec, 0))
            {
                int pos = 0;
                payload_t* my = (payload_t*) osip_list_get(config->video_codec, pos);

                /* all media MUST have the same PROTO, PORT. */
                sdp_message_m_media_add(*sdp, osip_getcopy("video"), osip_getcopy(video_port),
                                        /*my->number_of_port, sgetcopy (my->proto));*/
                                        osip_getcopy(my->number_of_port), osip_getcopy(my->proto));  /* update to 2.0.6 */

                while (!osip_list_eol(config->video_codec, pos))
                {
                    my = (payload_t*) osip_list_get(config->video_codec, pos);

                    if (NULL == my)
                    {
                        pos++;
                        continue;
                    }

                    sdp_message_m_payload_add(*sdp, media_line, osip_getcopy(my->payload));

                    if (my->a_rtpmap != NULL)
                    {
                        sdp_message_a_attribute_add(*sdp, media_line, osip_getcopy("rtpmap"),
                                                    osip_getcopy(my->a_rtpmap));
                    }

                    pos++;
                }

                switch (media_direction)
                {
                    case 0 :
                        sdp_message_a_attribute_add(*sdp, media_line, osip_getcopy("inactive"), NULL);
                        break;

                    case 1 :
                        sdp_message_a_attribute_add(*sdp, media_line, osip_getcopy("sendonly"), NULL);
                        break;

                    case 2 :
                        sdp_message_a_attribute_add(*sdp, media_line, osip_getcopy("recvonly"), NULL);
                        break;

                    case 3 :
                        sdp_message_a_attribute_add(*sdp, media_line, osip_getcopy("sendrecv"), NULL);
                        break;

                    default:
                        sdp_message_a_attribute_add(*sdp, media_line, osip_getcopy("sendrecv"), NULL);
                }

                media_line++;
            }
        }
    }

    return 0;
}

int sdp_put_on_hold(sdp_message_t* sdp)
{
    int pos = 0;
    int pos_media = -1;
    char* rcvsnd = NULL;
    int recv_send = -1;
    char* c_addr = NULL;

    pos = 0;
    rcvsnd = sdp_message_a_att_field_get(sdp, pos_media, pos);

    while (rcvsnd != NULL)
    {
        /*if (rcvsnd != NULL && 0 == strcmp (rcvsnd, "sendonly")
        && 0 == strcmp (rcvsnd, "sendrecv"))*/
        if (rcvsnd != NULL && 0 == strcmp(rcvsnd, "sendonly"))
        {
            recv_send = 0;
        }
        else if (rcvsnd != NULL && (0 == strcmp(rcvsnd, "recvonly") || 0 == strcmp(rcvsnd, "sendrecv")))
        {
            recv_send = 0;
            snprintf(rcvsnd, strlen(rcvsnd), "sendonly");
        }

        pos++;
        rcvsnd = sdp_message_a_att_field_get(sdp, pos_media, pos);
    }

    pos_media = 0;

    while (!sdp_message_endof_media(sdp, pos_media))
    {
        pos = 0;
        rcvsnd = sdp_message_a_att_field_get(sdp, pos_media, pos);

        while (rcvsnd != NULL)
        {
            if (rcvsnd != NULL && 0 == strcmp(rcvsnd, "sendonly"))
            {
                recv_send = 0;
            }
            /*else if (rcvsnd != NULL && 0 == strcmp (rcvsnd, "recvonly"))*/
            else if (rcvsnd != NULL && (0 == strcmp(rcvsnd, "recvonly")
                                        || 0 == strcmp(rcvsnd, "sendrecv")))
            {
                recv_send = 0;
                snprintf(rcvsnd, strlen(rcvsnd), "sendonly");
            }

            pos++;
            rcvsnd = sdp_message_a_att_field_get(sdp, pos_media, pos);
        }

        pos_media++;
    }

    if (recv_send == -1)
    {
        /* we need to add a global attribute with a feild set to "sendonly" */
        sdp_message_a_attribute_add(sdp, -1, osip_getcopy("sendonly"), NULL);
    }

    /*
     * To compliant with Cisco 7940G SIP phone, here I have to set
     * c=IN IP4 the.normal.ip.addr to c=IN IP4 0.0.0.0
     * NOTICE: the hold SDP still has the attribute: a=sendonly
     */
    c_addr = sdp_message_c_addr_get(sdp, -1 /* media position */ , 0 /* no meaning here */);

    /* set the addr to 0.0.0.0 */
    if (NULL != c_addr)
    {
        snprintf(c_addr, strlen(c_addr), "0.0.0.0");
    }

    return 0;
}

int sdp_put_off_hold(sdp_message_t* sdp, char* c_addr)
{
    int pos = 0;
    int pos_media = -1;
    char* rcvsnd = NULL;

    pos = 0;
    rcvsnd = sdp_message_a_att_field_get(sdp, pos_media, pos);

    while (rcvsnd != NULL)
    {
        if (rcvsnd != NULL && (0 == strcmp(rcvsnd, "sendonly")
                               || 0 == strcmp(rcvsnd, "recvonly")))
        {
            snprintf(rcvsnd, strlen(rcvsnd), "sendrecv");
        }

        pos++;
        rcvsnd = sdp_message_a_att_field_get(sdp, pos_media, pos);
    }

    pos_media = 0;

    while (!sdp_message_endof_media(sdp, pos_media))
    {
        pos = 0;
        rcvsnd = sdp_message_a_att_field_get(sdp, pos_media, pos);

        while (rcvsnd != NULL)
        {
            if (rcvsnd != NULL && (0 == strcmp(rcvsnd, "sendonly")
                                   || 0 == strcmp(rcvsnd, "recvonly")))
            {
                snprintf(rcvsnd, strlen(rcvsnd), "sendrecv");
            }

            pos++;
            rcvsnd = sdp_message_a_att_field_get(sdp, pos_media, pos);
        }

        pos_media++;
    }

    /*
     * c=IN IP4 0.0.0.0 ---> c=IN IP4 c_addr
     */
    if (!strncmp(sdp->c_connection->c_addr, "0.0.0.0", 7))
    {
        osip_free(sdp->c_connection->c_addr);
        sdp->c_connection->c_addr = NULL;
        sdp->c_connection->c_addr = osip_getcopy(c_addr);
    }

    return 0;
}
