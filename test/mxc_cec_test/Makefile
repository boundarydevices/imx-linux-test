#
# Makefile for MediaLB driver test app.
#
# list of platforms which did not want this test case
INCLUDE_LIST:= IMX6Q

INC += -I$(LINUXPATH)/include
CFLAGS += -lpthread -lcec -lrt

ifeq ($(EXCLUDE_LIST),$(findstring $(PLATFORM), $(EXCLUDE_LIST)))
OBJS = $(OBJDIR)/mxc_cec_test.out
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
