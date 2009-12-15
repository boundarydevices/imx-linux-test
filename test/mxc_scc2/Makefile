# list of platforms which did not want this test case
EXCLUDE_LIST:="IMX21ADS IMX27ADS IMX31ADS IMX31_3STACK IMX35_3STACK IMX25_3STACK IMX233"

ifeq (,$(findstring $(PLATFORM), $(EXCLUDE_LIST)))
OBJS = $(OBJDIR)/scc2_test.out
else
endif

CFLAGS += -DFSL_PLATFORM_OTHER -DFSL_HAVE_SCC2

all : $(OBJS)
ifeq (,$(findstring $(PLATFORM), $(EXCLUDE_LIST)))
	cp -r *.sh $(OBJDIR)
endif

.PHONY: clean
clean :
	rm -f $(OBJ)

#
# include the Rules
#
include ../make.rules

