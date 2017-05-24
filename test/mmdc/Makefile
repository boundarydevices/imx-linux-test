LINK ?=$(CROSS_COMPILE)gcc
STRIP ?=$(CROSS_COMPILE)strip

OBJ = mmdc.o

TARGET = $(OBJDIR)/mmdc2

CFLAGS+= -Os

LDFLAGS+= -lstdc++

all : $(TARGET)

$(TARGET):$(OBJ)
	$(CC) -o $(TARGET) $(OBJ) $(LDFLAGS) -Os
	$(STRIP) $(TARGET)

.PHONY: clean
clean :
	rm -f $(OBJ)

#
# include the Rules
#
include ../make.rules

