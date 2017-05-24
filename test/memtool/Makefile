LINK ?=$(CROSS_COMPILE)gcc
STRIP ?=$(CROSS_COMPILE)strip

OBJ = memtool.o \
      mx6dl_modules.o \
      mx6q_modules.o \
      mx6sl_modules.o \
      mx6sx_modules.o \
      mx6ul_modules.o \
      mx7d_modules.o \
      mx6ull_modules.o

TARGET = $(OBJDIR)/memtool

CFLAGS+= -Os

all : $(TARGET)

$(TARGET):$(OBJ)
	$(CC) -o $(TARGET) $(OBJ) -Os
	$(STRIP) $(TARGET)

.PHONY: clean
clean :
	rm -f $(OBJ)

#
# include the Rules
#
include ../make.rules

