# list of platforms which did not want this test case
EXCLUDE_LIST:="MXC27520EVB IMX27ADS MXC30020EVB"

ifeq (,$(findstring $(PLATFORM), $(EXCLUDE_LIST)))
OBJS = $(OBJDIR)/autorun-oprofile.sh
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

