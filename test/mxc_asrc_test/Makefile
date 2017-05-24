#
# Makefile for OSS sound driver test app.
#

CFLAGS += -lpthread

OBJS = \
	$(OBJDIR)/mxc_asrc_test.out \
	$(OBJDIR)/autorun-asrc.sh

all : $(OBJS)
	cp audio8k16S.wav $(OBJDIR)

.PHONY: clean
clean :
	rm -f $(OBJS)

#
# include the Rules
#
include ../make.rules
