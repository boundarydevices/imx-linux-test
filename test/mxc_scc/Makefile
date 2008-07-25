# list of platforms which did not want this test case
EXCLUDE_LIST:="IMX21ADS MXC30031ADS"

ifeq (,$(findstring $(PLATFORM), $(EXCLUDE_LIST)))
OBJS = $(OBJDIR)/scc_test $(OBJDIR)/autorun-scc.sh
else
endif


all : $(OBJS)
	cp -rf *.sh $(OBJDIR)

.PHONY: clean
clean :
	rm -f $(OBJ)

#
# include the Rules
#
include ../make.rules

