SHELL = /bin/sh

AR = ar

ARFLAGS = cr              

SUBDIRS := ./sipstack/src/osip2 ./sipstack/src/osipparser2 ./sipapp

target = libsip.a

RM = -rm -rf

MAKE = make

all: $(target)

$(target):
		for dir in $(SUBDIRS); \
		do $(MAKE) -C $$dir all || exit 1; \
		done

		@pwd	
		$(AR) x ./sipstack/src/osip2/libsipfsm.a
		$(AR) x ./sipstack/src/osipparser2/libsipparser.a
		$(AR) x ./sipapp/libsipapp.a
		$(AR) cru libsip.a *.o
		rm -rf *.o
		@pwd

clean:
		@for dir in $(SUBDIRS); do make -C $$dir clean|| exit 1; done
		$(RM) $(target) *.bak *~

.PHONY: all clean
