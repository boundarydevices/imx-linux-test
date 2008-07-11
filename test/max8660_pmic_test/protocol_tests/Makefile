#
# Here are a list of OBJS needed
#
TARGET=max8660_pmic_testapp.out
SRCS=$(wildcard *.c)
OBJS2=$(patsubst %.c,%.o,$(SRCS))


all : $(TARGET)
	mkdir -p $(OBJDIR)
	cp -f $(TARGET) $(OBJDIR)
$(TARGET): $(OBJS2)
	$(CC) $(OBJS2) -o $@

$(OBJS2) : $(SRCS)

.PHONY: clean
clean:
	rm -f $(TARGET) $(OBJS2)
#
# include the Rules
#
include ../../make.rules
