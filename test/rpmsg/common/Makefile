#tests for "IMX6SX"

CFLAGS += -lpthread -Os
LINK ?=$(CROSS_COMPILE)gcc
STRIP ?=$(CROSS_COMPILE)strip

OBJ = ../common/common.o \
      main_core0.o

TARGET = $(OBJDIR)/main_core0.out

all : $(TARGET)

$(TARGET):$(OBJ)
	$(CC) -o $(TARGET) $(OBJ) $(CFLAGS)
	$(STRIP) $(TARGET)

.PHONY: clean
clean :
	rm -f $(OBJ)

#
# include the Rules
#
include ../../make.rules

