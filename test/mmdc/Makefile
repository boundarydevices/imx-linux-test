# list of platforms which did not want this test case
EXCLUDE_LIST:=

LINK ?=$(CROSS_COMPILE)gcc
STRIP ?=$(CROSS_COMPILE)strip

OBJ = mmdc.o

ifeq (,$(findstring $(PLATFORM), $(EXCLUDE_LIST)))
TARGET = $(OBJDIR)/mmdc2
else
TARGET =
endif

CFLAGS+= -Os

LDFLAGS+= -lstdc++

all : $(TARGET)

$(TARGET):$(OBJ)
	$(LINK) -o $(TARGET) $(OBJ) $(LDFLAGS) -Os
	$(STRIP) $(TARGET)

.PHONY: clean
clean :
	rm -f $(OBJ)

#
# include the Rules
#
include ../make.rules

