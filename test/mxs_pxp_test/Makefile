# list of platforms which want this test case
INCLUDE_LIST:=IMX233 IMX28

ifeq ($(PLATFORM),$(findstring $(PLATFORM),$(INCLUDE_LIST)))
OBJS = $(OBJDIR)/mxs_pxp_test.out $(OBJDIR)/mxs_pxp_out_test.sh
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

