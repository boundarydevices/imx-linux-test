# list of platforms which want this test case
INCLUDE_LIST:=IMX31_3STACK IMX35_3STACK IMX37_3STACK IMX51

SRCS = mxc_ipudev_test.c utils.c test_pattern.c
CFLAGS += -lipu -lIpuScreenLayer -lm

ifeq ($(PLATFORM),$(findstring $(PLATFORM),$(INCLUDE_LIST)))
OBJS = $(OBJDIR)/mxc_ipudev_test.out
$(OBJS): $(SRCS)
	mkdir -p $(OBJDIR)
	$(VERBOSE) $(CC) -g $(INC) $(CFLAGS) $^ $(LIBS) -o $@
	cp ipudev_config_file $(OBJDIR)
else
OBJS =
endif

all: $(OBJS)

.PHONY: clean
clean:
	rm -f $(OBJS)

include ../make.rules
