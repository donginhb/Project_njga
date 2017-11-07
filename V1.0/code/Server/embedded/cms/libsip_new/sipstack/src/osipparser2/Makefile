######################################################################
#               Makefile for SIP Stack's   parser  part                 #
#			          writen by yanghaifeng                                #
#						    Copy right 2013                                      #      
######################################################################

SHELL = /bin/sh

CC=gcc

CONFIG_HEADER = config.h

CFLAGS = -Wall -DHAVE_CONFIG_H -fPIC

CFLAGS += -g -DENABLE_LEAKD -DMEMORY_LEAKS -DENABLE_DEBUG_LOCK -DENABLE_DEBUG -DENABLE_TRACE

SIP_FLAGS = -pedantic -DUSE_TMP_BUFFER

SIP_PARSER_FLAGS = -DUSE_GPREF -DMULTI_THR

INCLUDE_DIR = -I./ \
              -I../ \
              -I../../ \
              -I../../include

AR = $(CROSSPREFIX)ar

ARFLAGS = cr

LIB = libsipparser.a

RM = -rm -rf

OBJS :=  osip_accept.o \
         osip_accept_encoding.o \
         osip_accept_language.o \
         osip_alert_info.o \
         osip_allow.o \
         osip_authentication_info.o \
         osip_authorization.o \
         osip_body.o \
         osip_call_id.o \
         osip_call_info.o \
         osip_contact.o \
         osip_content_disposition.o \
         osip_content_encoding.o \
         osip_content_length.o \
         osip_content_type.o \
         osip_cseq.o \
         osip_error_info.o \
         osip_from.o \
         osip_header.o \
         osip_list.o\
         osip_md5c.o \
         osip_message.o \
         osip_message_parse.o \
         osip_message_to_str.o \
         osip_mime_version.o \
         osip_parser_cfg.o \
         osip_port.o \
         osip_proxy_authenticate.o \
         osip_proxy_authentication_info.o \
         osip_proxy_authorization.o \
         osip_record_route.o \
         osip_route.o \
         osip_to.o \
         osip_uri.o \
         osip_via.o \
         osip_www_authenticate.o \
         sdp_accessor.o \
         sdp_message.o
         
SRCS = $(subst .o,.c,$(OBJS))

all: $(LIB)

$(OBJS): $(SRCS)
		$(CC) $(CFLAGS) $(SIP_FLAGS) $(SIP_PARSER_FLAGS) -c $^ $(INCLUDE_DIR)

$(LIB): $(OBJS)
		$(AR) $(ARFLAGS) $@ $(OBJS)

clean:
		$(RM) $(OBJS) $(LIB) *.bak *~

.PHONY: all clean
