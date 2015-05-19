#
# Makefile for MXC SIM driver test app.
#

# list of platforms which want this test case
INCLUDE_LIST:= IMX51 IMX25_3STACKi IMX7D IMX6UL

OBJS = mxc_sim_test.o
INC += -I$(LINUXPATH)/include
CFLAGS += $(INC)

ifeq ($(PLATFORM),$(findstring $(PLATFORM),$(INCLUDE_LIST)))
TARGET = $(OBJDIR)/mxc_sim_test.out
else
TARGET =
endif

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
