#
# Makefile for MediaLB driver test app.
#
INC += -I$(LINUXPATH)/include

TARGET = $(OBJDIR)/mxc_cec_test.out
OBJ = mxc_cec_test.o

all : $(TARGET)

LDFLAGS += -lpthread -lcec -lrt -lstdc++ -pthread

$(TARGET):$(OBJ)
	$(CC) -o $(TARGET) $(OBJ) $(LDFLAGS) -Os
	$(STRIP) $(TARGET)


.PHONY: clean
clean :
	rm -f $(OBJS)

#
# include the Rules
#
include ../make.rules
