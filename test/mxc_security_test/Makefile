# list of platforms which did not want this test case
EXCLUDE_LIST:=IMX37_3STACK

# list of platforms that have hacc
HACC_LIST:=MXC91131EVB

ifeq (,$(findstring $(PLATFORM), $(EXCLUDE_LIST)))
OBJS = $(OBJDIR)/mxc_security_test.out
else
OBJS =
endif

ifeq ($(PLATFORM),$(findstring $(PLATFORM),$(HACC_LIST)))
SCRIPTS = $(OBJDIR)/autorun-hacc.sh
else
SCRIPTS = $(OBJDIR)/autorun-rtic.sh
endif

ifeq (,$(findstring $(PLATFORM), $(EXCLUDE_LIST)))
SCRIPTS =
endif

all : $(OBJS) $(SCRIPTS)

.PHONY: clean
clean :
	rm -f $(OBJS) $(SCRIPTS)

#
# include the Rules
#
include ../make.rules

