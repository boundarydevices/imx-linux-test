# list of platforms which did not want this test case
EXCLUDE_LIST:= 

ifeq (,$(findstring $(PLATFORM), $(EXCLUDE_LIST)))
OBJS = $(OBJDIR)/rtctest.out $(OBJDIR)/autorun-rtc.sh
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

