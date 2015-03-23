#
# Makefile for MXC MCC TTY driver test app.
#

# list of platforms which want this test case
INCLUDE_LIST:= IMX6SX IMX7D

ifeq ($(PLATFORM),$(findstring $(PLATFORM),$(INCLUDE_LIST)))
OBJS = $(OBJDIR)/mxc_mcc_tty_test.out
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
