# list of platforms which did not want this test case
EXCLUDE_LIST:="MXC27520EVB MXC27530EVB IMX27ADS MXC30020EVB MXC30031ADS IMX37_3STACK"

# list of platforms which do not have either SIR or FIR
IR_EXCLUDE_LIST:=

ifeq (,$(findstring $(PLATFORM), $(EXCLUDE_LIST)))
OBJS = $(OBJDIR)/autorun-firi.sh
else
OBJS =
endif

ifeq (,$(findstring $(PLATFORM), $(IR_EXCLUDE_LIST)))
SCRIPTS = $(OBJDIR)/mxc_irlan_test.sh
else
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

