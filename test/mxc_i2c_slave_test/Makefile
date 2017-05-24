# list of platforms which did not want this test case

CFLAGS += -lpthread

TARGET = $(OBJDIR)/mxc_i2c_slave_test.out

all : $(TARGET)

LDFLAGS += -lpthread -lstdc++ -pthread

OBJ = mxc_i2c_slave_test.o

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


