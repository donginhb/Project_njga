######################################################################
#               Makefile for SIP Stack's   APP  part                 #
#			          writen by yanghaifeng                                #
#						    Copy right 2013                                      #      
######################################################################

SHELL = /bin/sh

CC=g++

CFLAGS = -Wall -Winline -DHAVE_CONFIG_H \
         -DHAVE_PTHREAD_H -DMULTI_THR \
         -DHAVE_SIGNAL_H -DNEW_RANDOM \
         -DHAVE_SEMAPHORE_H -DHAVE_STDARG_H -fPIC

CFLAGS += -g -DENABLE_LEAKD -DMEMORY_LEAKS -DENABLE_DEBUG_LOCK -DENABLE_DEBUG -DENABLE_TRACE

INCLUDE_DIR = -I./ \
              -I../ \
              -I../../ \
              -I../sipstack \
              -I../sipstack/include
              
AR = $(CROSSPREFIX)ar

ARFLAGS = cr

LIB = libsipapp.a

RM = -rm -rf

OBJS :=  callback.o \
         csdbg.o \
         gblfunc.o \
         initfunc.o \
         registrar.o \
         sip_event.o \
         sipauth.o \
         sipheader.o \
         sipmsg.o \
         sipua.o \
         mansrtsp.o \
         sdp_negoc.o \
         timerproc.o \
         garbage.o \
         udp_tl.o

SRCS = $(subst .o,.c,$(OBJS))

all: $(LIB)

$(OBJS): $(SRCS)
		$(CC) $(CFLAGS) -c $^ $(INCLUDE_DIR)

$(LIB): $(OBJS)
		$(AR) $(ARFLAGS) $@ $(OBJS)

clean:
		$(RM) $(OBJS) $(LIB) *.bak *~

.PHONY: all clean
