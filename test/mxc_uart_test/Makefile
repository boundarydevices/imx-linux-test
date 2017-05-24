LIBS += -lpthread

OBJS = 	$(OBJDIR)/mxc_uart_test.out  $(OBJDIR)/autorun-mxc_uart.sh $(OBJDIR)/mxc_uart_xmit_test.out $(OBJDIR)/mxc_uart_stress_test.out

all : $(OBJS)

.PHONY: clean
clean :
	rm -f $(OBJS)

#
# include the Rules
#
include ../make.rules

