#
# Makefile for MXC SIM driver test app.
#

OBJS = mxc_sim_test.o
INC += -I$(LINUXPATH)/include
CFLAGS += $(INC)

TARGET = $(OBJDIR)/mxc_sim_test.out

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -lpthread -o $(TARGET) $(OBJS)

.PHONY: clean
clean :
	rm -f $(OBJS)

#
# include the Rules
#
include ../make.rules
