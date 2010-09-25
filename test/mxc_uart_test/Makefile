# list of platforms which did not want this test case
EXCLUDE_LIST:=
LIBS += -lpthread


ifeq (,$(findstring $(PLATFORM), $(EXCLUDE_LIST)))
OBJS = 	$(OBJDIR)/mxc_uart_test.out  $(OBJDIR)/autorun-mxc_uart.sh $(OBJDIR)/mxc_uart_xmit_test.out
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

