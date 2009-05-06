#
# Makefile for MXC SIM driver test app.
#

# list of platforms which want this test case
INCLUDE_LIST:= IMX25_3STACK IMX51

OBJS = mxc_sim_test.o

CFLAGS += $(INC)

ifeq ($(PLATFORM),$(findstring $(PLATFORM),$(INCLUDE_LIST)))
TARGET = $(OBJDIR)/mxc_sim_test.out
else
TARGET =
endif

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -lpthread -o $(TARGET) $(OBJS) -lsim

.PHONY: clean
clean :
	rm -f $(OBJS)

#
# include the Rules
#
include ../make.rules
