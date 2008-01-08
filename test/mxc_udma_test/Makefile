# list of platforms which did not want this test case
EXCLUDE_LIST:= IMX31ADS I30030EVB I30030ADS MXC30030EVB MXC30030ADS MXC91131EVB MXC27530EVB

ifeq (,$(findstring $(PLATFORM), $(EXCLUDE_LIST)))
OBJS = $(OBJDIR)/mxc_udma_test.out
else
OBJS = 
endif

all : $(OBJS)

#
# include the Rules
#
include ../make.rules
