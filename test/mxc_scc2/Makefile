# list of platforms which did not want this test case
EXCLUDE_LIST:="IMX21ADS IMX27_ADS IMX31_ADS IMX31_3STACK IMX35_3STACK"

ifeq (,$(findstring $(PLATFORM), $(EXCLUDE_LIST)))
OBJS = $(OBJDIR)/scc2_test.out $(OBJDIR)/autorun-scc2.sh
else
OBJS =
endif

all : $(OBJS)
	cp -r *.sh $(OBJDIR)

.PHONY: clean
clean :
	rm -f $(OBJ)

#
# include the Rules
#
include ../make.rules

