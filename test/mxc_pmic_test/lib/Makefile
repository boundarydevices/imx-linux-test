
CFLAGS+= -Wall -I../include 
LDFLAGS+=
TARGET=libltp.a
SRCS=$(wildcard *.c)
OBJS=$(patsubst %.c,%.o,$(SRCS))

$(TARGET): $(OBJS)
	$(AR) -cr $@ $^    

.PHONY: clean
clean:
	rm -f $(TARGET) $(OBJS)
#
# include the Rules
#
include ../../make.rules
