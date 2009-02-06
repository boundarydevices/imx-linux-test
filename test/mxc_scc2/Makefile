# list of platforms which did not want this test case
EXCLUDE_LIST:="IMX21ADS IMX27ADS IMX31ADS IMX31_3STACK IMX35_3STACK IMX25_3STACK"

ifeq (,$(findstring $(PLATFORM), $(EXCLUDE_LIST)))
OBJS = $(OBJDIR)/scc2_test.out $(OBJDIR)/autorun-scc2.sh
else
OBJS =
endif

CFLAGS += -DFSL_PLATFORM_OTHER -DFSL_HAVE_SCC2

all : $(OBJS)
	cp -r *.sh $(OBJDIR)

.PHONY: clean
clean :
	rm -f $(OBJ)

#
# include the Rules
#
include ../make.rules

