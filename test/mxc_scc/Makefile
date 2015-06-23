# list of platforms which did not want this test case
#EXCLUDE_LIST:="IMX21ADS MXC30031ADS IMX233 IMX37_3STACK IMX51 IMX50 IMX6Q IMX6SL IMX6SX IMX7D"
INCLUDE_LIST:="IMX53 IMX25_STACK"

ifeq ($(PLATFORM),$(findstring $(PLATFORM),$(INCLUDE_LIST)))
OBJS = $(OBJDIR)/scc_test
else
endif


all : $(OBJS)
ifeq ($(PLATFORM),$(findstring $(PLATFORM),$(INCLUDE_LIST)))
	cp -r *.sh $(OBJDIR)
endif

.PHONY: clean
clean :
	rm -f $(OBJ)

#
# include the Rules
#
include ../make.rules

