######################################################################
#               Makefile for SIP Stack's   FSM  part                 #
#			          writen by yanghaifeng                                #
#						    Copy right 2013                                      #      
######################################################################

SHELL = /bin/sh

CC=gcc

CONFIG_HEADER = config.h

CFLAGS = -Wall -DHAVE_CONFIG_H -fPIC

CFLAGS += -g -DENABLE_LEAKD -DMEMORY_LEAKS -DENABLE_DEBUG_LOCK -DENABLE_DEBUG -DENABLE_TRACE

SIP_FLAGS = -pedantic -DUSE_TMP_BUFFER

SIP_FSM_FLAGS = -DMULTI_THR

INCLUDE_DIR = -I./ \
              -I../ \
              -I../../ \
              -I../../include

AR = $(CROSSPREFIX)ar

ARFLAGS = cr

LIB = libsipfsm.a

RM = -rm -rf

OBJS :=  dict.o \
         hashtable.o \
         hb_tree.o \
         pr_tree.o \
         rb_tree.o \
         skiplist.o \
         sp_tree.o \
         tr_tree.o \
         tree_common.o \
         wb_tree.o \
         osip_dialog.o \
         fsm_misc.o \
         ict.o \
         ict_fsm.o \
         ist.o \
         ist_fsm.o \
         nict.o \
         nict_fsm.o\
         nist.o \
         nist_fsm.o \
         osip.o \
         osip_event.o \
         osip_time.o \
         osip_transaction.o\
         port_condv.o\
         port_fifo.o \
         port_sema.o \
         port_thread.o

SRCS = $(subst .o,.c,$(OBJS))

all: $(LIB)

$(OBJS): $(SRCS)
		$(CC) $(CFLAGS) $(SIP_FLAGS) $(SIP_FSM_FLAGS) -c $^ $(INCLUDE_DIR)

$(LIB): $(OBJS)
		$(AR) $(ARFLAGS) $@ $(OBJS)

clean:
		$(RM) $(OBJS) $(LIB) *.bak *~

.PHONY: all clean
