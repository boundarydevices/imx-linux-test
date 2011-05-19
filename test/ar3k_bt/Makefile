# Makefile for atheros BT hciattach app
#
# This package is for atheros ar3k bt chip, you can use the binary,
# also provide the modified source code, if you need rebuild the tool
# please rebuild, it need the rootfs and bluez source & lib path
EXCLUDE_LIST:= IMX21ADS IMX27ADS IMX31ADS IMX32ADS IMX37_3STACK IMX25_3STACK

INC += -I$(LINUXPATH)/include

ifeq (,$(findstring $(PLATFORM), $(EXCLUDE_LIST)))
OBJS = $(OBJDIR)/hciattach-ar3k.bin $(OBJDIR)/ar3k-bt-start.sh
else
OBJS =
endif

all : $(OBJS)

.PHONY: clean
clean:
	rm -f $(OBJS)

include ../make.rules


# If you want to compile the source code, please reference follow make file.
# this is default off by here.
# it will be compile on your target envroioment.(eg, ubuntu).

#CC := gcc

#all: hciattach-ar3k

#hciattach-ar3k:
#	$(CC) -Wall -lbluetooth -g hciattach_ath3k.c  hciattach.c  hciattach_qualcomm.c  hciattach_st.c  hciattach_tialt.c  hciattach_ti.c -o hciattach-ar3k

#clean:
#	rm -f hciattach-ar3k *.o
