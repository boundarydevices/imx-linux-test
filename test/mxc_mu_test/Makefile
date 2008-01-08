# list of platforms which did not want this test case
EXCLUDE_LIST:="IMX27ADS IMX31ADS"

ifeq (,$(findstring $(PLATFORM), $(EXCLUDE_LIST)))
OBJS = \
	$(OBJDIR)/mxc_mu_test.out \
        $(OBJDIR)/autorun-mu.sh
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

