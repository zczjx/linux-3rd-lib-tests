CROSS_COMPILE 	?=

AS		= $(CROSS_COMPILE)as
LD		= $(CROSS_COMPILE)ld
CC		= $(CROSS_COMPILE)gcc
CPP		= $(CROSS_COMPILE)g++
AR		= $(CROSS_COMPILE)ar
NM		= $(CROSS_COMPILE)nm
STRIP		= $(CROSS_COMPILE)strip
OBJCOPY		= $(CROSS_COMPILE)objcopy
OBJDUMP		= $(CROSS_COMPILE)objdump
export AS LD CC CPP AR NM
export STRIP OBJCOPY OBJDUMP

TOPDIR=$(shell pwd)
BIN_DIR=$(TOPDIR)/bin
export TOPDIR
export BIN_DIR

CFLAGS = -Wall  -g -fPIC  -rdynamic
CFLAGS += -I $(shell pwd)/include
CFLAGS += -I ${TOPDIR}
LDFLAGS ?= -lm -lpthread -L$(TOPDIR)
export CFLAGS LDFLAGS


LINUX_3RD_LIB_FFMPEG ?= n
LINUX_3RD_LIB_LIBV4L2 ?= n
obj-y += common-case/

ifeq ($(LINUX_3RD_LIB_FFMPEG),y)
obj-y += ffmpeg/
endif

ifeq ($(LINUX_3RD_LIB_LIBV4L2),y)
obj-y += libv4l2/
endif

all : lib
	mkdir $(BIN_DIR)
	make -f $(TOPDIR)/case.build.mk

LINUX_3RD_COMMON_DLIB ?= lib3rd_common.so
libobj-y += common-lib/

lib:
	make -f $(TOPDIR)/lib.build.mk

clean:
	rm -f $(shell find -name "*.o")
	rm -f $(shell find -name "*.so")
	rm -f $(shell find -name "*.gcno")
	rm -f $(shell find -name "*.gcda")
	rm -f $(shell find -name "*.gcov")
	rm -rf $(BIN_DIR)

distclean:
	rm -f $(shell find -name "*.o")
	rm -f $(shell find -name "*.so")
	rm -f $(shell find -name "*.gcno")
	rm -f $(shell find -name "*.gcda")
	rm -f $(shell find -name "*.gcov")
	rm -rf $(BIN_DIR)
	
