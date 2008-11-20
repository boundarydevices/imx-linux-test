#
# Here are a list of OBJS needed
#
ifeq ("$(PLATFORM)", "IMX25_3STACK")
TARGET=mc34704_testapp.out
SRCS=$(wildcard *mc34704*.c)
CFLAGS+= -DCONFIG_MXC_PMIC_MC34704 -UCONFIG_MXC_PMIC_MC13783
else
TARGET=pmic_testapp.out
SRCS=$(wildcard *mc13783*.c)
endif

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
