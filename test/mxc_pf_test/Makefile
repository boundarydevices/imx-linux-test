# list of platforms which did not want this test case
EXCLUDE_LIST:="IMX27ADS IMX37_3STACK"

ifeq (,$(findstring $(PLATFORM), $(EXCLUDE_LIST)))
OBJS = $(OBJDIR)/autorun-pf.sh $(OBJDIR)/mxc_pf_test.out
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

