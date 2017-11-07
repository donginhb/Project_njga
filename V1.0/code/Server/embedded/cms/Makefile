GSOAP_ROOT=/usr/local/gSOAP
WSNAME0=soap
WSNAME=SmsWBS

CC=g++ -g -DWITH_NONAMESPACES

#编译选项
CFLAGS = -Wall -Winline -DNEW_RANDOM -DMFPA_TYPE_CMS\
         -DHAVE_PTHREAD_H -DMULTI_THR \
         -DHAVE_SIGNAL_H -DNEW_RANDOM \
         -DHAVE_SEMAPHORE_H -DHAVE_STDARG_H \

ifeq ($(LANG), en)
    CFLAGS += -DLANG_EN
endif

CFLAGS += -I/usr/local/mysql/include -I./

CFLAGS += -Wl,-rpath=.:/usr/local/mysql/lib:/usr/lib/mysql:/usr/lib64/mysql

CFLAGS += -g -DENABLE_LEAKD -DMEMORY_LEAKS -DENABLE_DEBUG_LOCK -DENABLE_DEBUG -DENABLE_TRACE

#头文件包含目录
INCLUDE_DIR = -I./ \
        -I./libsip_new \
        -I./libsip_new/include \
        -I./libsip_new/sipstack \
        -I./libsip_new/sipstack/include \
        -I/usr/include \
        -I/usr/local/include \
        -I/usr/include/mysql \
        -I./platformms \
        -I../../common/include \
        -I $(GSOAP_ROOT)/include

#编译库
LIBS := -L./libsip_new -lsip 
MYSQLLIB := -L/usr/local/mysql/lib -L/usr/lib/mysql -L/usr/lib64/mysql -lmysqlclient

LDFLAGS = $(LIBS) $(MYSQLLIB)

#编译目标文件
ifeq ($(DEBUG), y)
    target = cms_dbg
else
    target = cms
endif

RM = -rm -rf

OBJDIR = obj

MAKE = make

#编译子目录
SUBDIRS := ./common ./libsip_new

define make_subdir
 @for subdir in $(SUBDIRS) ; do \
 ( cd $$subdir && make $1 $2) \
 done;
endef

define make_target
 make target
endef

#编译目录及文件
vpath %.c common config device libxml record resource route service user jwpt_interface platformms
vpath %.cpp common config device libxml record resource route service user jwpt_interface platformms

C_SRCS =$(notdir $(wildcard common/*.c)) \
       $(notdir $(wildcard config/*.c)) \
       $(notdir $(wildcard device/*.c)) \
       $(notdir $(wildcard libxml/*.c)) \
       $(notdir $(wildcard record/*.c)) \
       $(notdir $(wildcard resource/*.c)) \
       $(notdir $(wildcard route/*.c)) \
       $(notdir $(wildcard service/*.c)) \
       $(notdir $(wildcard user/*.c))  \
       $(notdir $(wildcard jwpt_interface/*.c))  \
       $(notdir $(wildcard platformms/*.c)) \
       $(wildcard *.c)
       
CPP_SRCS = $(notdir $(wildcard common/*.cpp)) \
       $(notdir $(wildcard config/*.cpp)) \
       $(notdir $(wildcard device/*.cpp)) \
       $(notdir $(wildcard libxml/*.cpp)) \
       $(notdir $(wildcard record/*.cpp)) \
       $(notdir $(wildcard resource/*.cpp)) \
       $(notdir $(wildcard route/*.cpp)) \
       $(notdir $(wildcard service/*.cpp)) \
       $(notdir $(wildcard user/*.cpp))\
       $(notdir $(wildcard jwpt_interface/*.cpp))  \
       $(notdir $(wildcard platformms/*.cpp)) \
       $(wildcard *.cpp)

C_OBJS = $(C_SRCS:%.c=$(OBJDIR)/%.o)
CPP_OBJS := $(CPP_SRCS:%.cpp=$(OBJDIR)/%.o)	

#编译选项
all: 
		$(call make_subdir , all)
		$(make_target)

target:$(target)

$(target): $(CPP_OBJS) $(C_OBJS)
		$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS) -Bstatic -lpthread -lrt -luuid

$(C_OBJS): $(OBJDIR)/%.o : %.c
		@pwd
		mkdir -p $(OBJDIR)
		@pwd
		$(CC) $(CFLAGS) -c $< -o $@ $(INCLUDE_DIR)

$(CPP_OBJS): $(OBJDIR)/%.o : %.cpp
		@pwd
		mkdir -p $(OBJDIR)
		@pwd
		$(CC) $(CFLAGS) -c $< -o $@ $(INCLUDE_DIR)			

clean:
		@pwd
		rm -rf bin/web
		rm -rf bin/update_file
		rm -rf bin
		rm -rf lib/*
		@pwd
		$(call make_subdir , clean) 
		$(RM) $(C_OBJS) $(CPP_OBJS) $(OBJDIR) $(target) *.bak *~

install:
		rm -rf bin/web
		rm -rf bin/update_file
		rm -rf bin
		mkdir -p bin
		mkdir -p bin/web
		cp -rf cms ./bin

		cp -rf app_monitor_x86.sh ./bin/app_monitor.sh
		cp -rf start_app_x86 ./bin
		cp -rf start_app_x86_dbg ./bin
		cp -rf disk_check.sh ./bin
		cp -rf disk_division.sh ./bin
		cp -rf cms_log.sh ./bin
		cp -rf ../../common/db/Create_table.sh ./bin
		cp -rf ../../common/db/Create_trig.sh ./bin
		cp -rf ../../common/db/repair_EV9000.sh ./bin
		cp -rf ../../common/db/Check_EV9000.sh ./bin
		cp -rf ../../common/db/Delete_trig.sh ./bin
		cp -rf ../../common/db/BakeupDB.sh ./bin
		cp -rf ../../common/db/BakeupDB_TSU.sh ./bin
		cp -rf ../../common/db/delete_DB.sh ./bin
		cp -rf ../../common/db/update_file ./bin

#修改文件属性
		chmod -R 777 ./bin/*
		find ./bin -name '.svn' | xargs rm -rf

.PHONY: all clean target
