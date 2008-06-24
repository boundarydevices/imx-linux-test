# list of platforms which did not want this test case
EXCLUDE_LIST:="IMX35_3STACK"

ifeq (,$(findstring $(PLATFORM), $(EXCLUDE_LIST)))
OBJS = $(OBJDIR)/autorun-keypad.sh $(OBJDIR)/mxc_keyb_test.sh
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

