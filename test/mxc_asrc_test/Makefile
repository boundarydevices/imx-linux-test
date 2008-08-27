#
# Makefile for OSS sound driver test app.
#
# list of platforms which did not want this test case
EXCLUDE_LIST:= IMX21ADS IMX27ADS IMX31ADS IMX32ADS IMX37_3STACK IMX51_3STACK

ifeq (,$(findstring $(PLATFORM), $(EXCLUDE_LIST)))
OBJS = \
	$(OBJDIR)/mxc_asrc_test.out \
	$(OBJDIR)/autorun-asrc.sh
else
OBJS =
endif

all : $(OBJS)
	cp audio8k16S.wav $(OBJDIR)

.PHONY: clean
clean :
	rm -f $(OBJS)

#
# include the Rules
#
include ../make.rules
