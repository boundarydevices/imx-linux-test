#
# Makefile for MXC MCC TTY driver test app.
#

# list of platforms which want this test case
INCLUDE_LIST:= IMX6SX

OBJS = mxc_mcc_tty_test.o

CFLAGS += $(INC)

ifeq ($(PLATFORM),$(findstring $(PLATFORM),$(INCLUDE_LIST)))
TARGET = $(OBJDIR)/mxc_mcc_tty_test.out
else
TARGET =
endif

all : $(OBJS)

.PHONY: clean
clean :
	rm -f $(OBJS)

#
# include the Rules
#
include ../make.rules
