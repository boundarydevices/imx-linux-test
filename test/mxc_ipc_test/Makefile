# list of platforms which did not want this test case
EXCLUDE_LIST:="IMX21ADS IMX27ADS IMX31ADS MXC91131EVB IMX37_3STACK IMX35_3STACK IMX51_3STACK"

ifeq (,$(findstring $(PLATFORM), $(EXCLUDE_LIST)))
OBJS = $(OBJDIR)/mxc_ipc_test.out
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

