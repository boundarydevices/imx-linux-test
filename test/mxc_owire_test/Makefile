# list of platforms which did not want this test case
EXCLUDE_LIST:="IMX31_3STACK IMX37_3STACK IMX35_3STACK IMX51 IMX25_3STACK"

ifeq (,$(findstring $(PLATFORM), $(EXCLUDE_LIST)))
OBJS = $(OBJDIR)/autorun-owire.sh
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

