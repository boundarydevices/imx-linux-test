# list of platforms which want this test case
INCLUDE_LIST:=IMX233 IMX28 IMX50 IMX51 IMX5 IMX6S

CFLAGS += -lpthread

ifeq ($(PLATFORM),$(findstring $(PLATFORM),$(INCLUDE_LIST)))
OBJS = $(OBJDIR)/pxp_v4l2_test.out $(OBJDIR)/pxp_v4l2_out_test.sh
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

