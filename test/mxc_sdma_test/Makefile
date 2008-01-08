HPATH += -I$(LINUXPATH)/include

# list of platforms which did not want this test case
EXCLUDE_LIST:= IMX21ADS IMX27ADS

ifeq (,$(findstring $(PLATFORM), $(EXCLUDE_LIST)))
OBJS = $(OBJDIR)/mxc_sdma_test.out
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

