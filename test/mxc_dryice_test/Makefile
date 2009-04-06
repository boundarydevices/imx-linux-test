#*****************************************************************************
#
# makefile for dryice test program
#
#****************************************************************************/

# list of platforms which did not want this test case
EXCLUDE_LIST:="IMX27ADS IMX27EVB IMX31ADS IMX31_3STACK IMX35_3STACK IMX37_3STACK IMX51"

ifeq (,$(findstring $(PLATFORM), $(EXCLUDE_LIST)))
OBJS = $(OBJDIR)/dryice.sh
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
