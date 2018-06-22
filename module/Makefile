
#turn on/off debug message
DEBUG = y

KERNELDIR ?= /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

ifeq ($(DEBUG), y)
	DEBFLAGS += -O -g -DBSL_DEBUG
else
	DEBFLAGS += -O2
endif

EXTRA_CFLAGS += $(DEBFLAGS)

EXTRA_CFLAGS += -DBSL_SW_VERSION=\"$(BSL_SW_VERSION)\" 

obj-m		:= bsl.o
bsl-objs	:= bsl_main.o bsl_pci.o bsl_ctl.o \
			   bsl_util.o \
			   bsl_phy.o \

#			   bsl_acl.o bsl_syncookie.o bsl_ratelimit.o bsl_stat.o bsl_stat_timer.o 

default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

install:
	cp -f bsl.ko bsl_load $(INSTALL_PATH)

clean:
	rm -rf *.ko
	rm -rf *.mod.*
	rm -rf .*.cmd
	rm -rf *.o
	rm -rf .tmp_versions
	rm -rf Module.*
	rm -rf modules.*
	rm -rf .tmp_versions
