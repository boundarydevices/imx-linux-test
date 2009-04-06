#
# Makefile for MediaLB driver test app.
#
# list of platforms which did not want this test case
EXCLUDE_LIST:= IMX21ADS IMX27ADS IMX31ADS IMX32ADS IMX37_3STACK IMX51 IMX25_3STACK

INC += -I$(LINUXPATH)/include

ifeq (,$(findstring $(PLATFORM), $(EXCLUDE_LIST)))
OBJS = \
	$(OBJDIR)/mxc_mlb_test.out
else
OBJS =
endif

all : $(OBJS)

.PHONY: clean
clean :
	rm -f $(OBJS)

#
# include the Rules
#
include ../make.rules
