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

OBJS = mydeflate_main.o

all: $(ALL_TARGETS)

$(APP):$(OBJS)
	$(CC) $^ -o $@ $(LIBS) $(EXT_LIB)

clean:
	-rm -f *.o 
	-rm -f $(APP)
