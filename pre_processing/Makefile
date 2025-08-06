TOPDIR ?= $(shell pwd -P)

#CROSS_COMPILE = arm-linux-gnueabihf-

	LIBPATH = $(TOPDIR)/../../lib/
	EXT_LIB= 
	CFLAGS = -g -Wall -D_REENTRANT -D_GNU_SOURCE -fPIC $(MACRO_DEFINE) \
		$(DEBUG) 

CC=$(CROSS_COMPILE)gcc

APP = pre_processing
LIBS = 


ALL_TARGETS=$(APP)

%.o : %.c
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@ 

OBJS = pre_processing_main.o

all: $(ALL_TARGETS)

$(APP):$(OBJS)
	$(CC) $^ -o $@ $(LIBS) $(EXT_LIB)
	mkdir -p out

clean:
	-rm -f *.o $(APP)
	-rm -f *.b diff.res out/*res* out/readme
	-rm -r out
