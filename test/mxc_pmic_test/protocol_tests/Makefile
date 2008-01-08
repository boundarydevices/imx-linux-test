#
# Here are a list of OBJS needed
#
TARGET=pmic_testapp
SRCS=$(wildcard *.c)
OBJS2=$(patsubst %.c,%.o,$(SRCS))

all : $(TARGET)
	mkdir -p $(OBJDIR)
	cp -f $(TARGET) $(OBJDIR)
        
$(TARGET): $(OBJS2)
	$(CC) $(OBJS2) $(LOADLIBES) -o $@

$(OBJS2) : $(SRCS)

.PHONY: clean
clean:
	rm -f $(TARGET) $(OBJS2)
#
# include the Rules
#
include ../../make.rules
