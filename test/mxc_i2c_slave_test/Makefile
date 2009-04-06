# list of platforms which did not want this test case
EXCLUDE_LIST:=IMX31ADS IMX27ADS IMX37_3STACK IMX31_3STACK IMX51

CFLAGS += -lpthread

ifeq (,$(findstring $(PLATFORM), $(EXCLUDE_LIST)))
OBJS = $(OBJDIR)/mxc_i2c_slave_test.out

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


