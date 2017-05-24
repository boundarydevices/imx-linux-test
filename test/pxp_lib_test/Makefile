SRCS = pxp_test.c utils.c

TARGET = $(OBJDIR)/pxp_test.out

all: $(TARGET)

OBJ = pxp_test.o utils.o

LDFLAGS += -pthread -lpxp -lstdc++ -pthread

$(OBJDIR)/pxp_test.out: $(OBJ)
	$(CC) -o  $(OBJDIR)/pxp_test.out $(OBJ) $(LDFLAGS) -Os
	$(STRIP) $(OBJDIR)/pxp_test.out

.PHONY: clean
clean:
	rm -f $(OBJS)

include ../make.rules
