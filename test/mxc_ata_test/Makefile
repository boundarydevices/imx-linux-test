# Makefile for mxc_ata_test
#mxc_ata_test: mxc_ata_test.o identify.o mxc_ata_test.h
#	$(CC) $(LDFLAGS) -o $@  mxc_ata_test.o identify.o

OBJS = \
        $(OBJDIR)/mxc_ata_test.out

all : $(OBJS)

.PHONY: clean
clean :
	rm -f $(OBJS)

#
# include the Rules
#
include ../make.rules

