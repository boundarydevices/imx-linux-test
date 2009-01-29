#
# Makefile for IMX ADC driver test app.
#
# list of platforms which did not want this test case
INCLUDE_LIST:= IMX25_3STACK

ifeq ($(PLATFORM),$(findstring $(PLATFORM),$(INCLUDE_LIST)))
OBJS = \
	$(OBJDIR)/imx_adc_test.out
else
OBJS =
endif

CFLAGS += -I$(LINUXPATH)/include/

all : $(OBJS)

.PHONY: clean
clean :
	rm -f $(OBJS)

#
# include the Rules
#
include ../make.rules
