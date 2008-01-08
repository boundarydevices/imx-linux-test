# list of platforms which did not want this test case
EXCLUDE_LIST:="MXC91131EVB I30030ADS I30030EVB IMX21ADS MXC27520EVB MXC27530EVB MXC30020EVB MXC30030EVB MXC30031ADS"

ifeq (,$(findstring $(PLATFORM), $(EXCLUDE_LIST)))
OBJS = $(OBJDIR)/autorun-ata.sh
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

