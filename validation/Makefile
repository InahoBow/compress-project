TOPDIR ?= $(shell pwd -P)

#CROSS_COMPILE = arm-linux-gnueabihf-

	LIBPATH = $(TOPDIR)/../../lib/
	EXT_LIB= 
	CFLAGS = -g -Wall -D_REENTRANT -D_GNU_SOURCE -fPIC $(MACRO_DEFINE) \
		$(DEBUG) 

CC=$(CROSS_COMPILE)gcc

APP = mydeflate
LIBS = -lz


ALL_TARGETS=$(APP)

%.o : %.c
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@ 

OBJS = mydeflate.o

all:

clean:
	-rm -f step1/*statis.z.step1.csv step1/*/*
	-rm -f step2/*statis.step2.*.csv step2/*/*
