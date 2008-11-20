CC=$(CROSS_COMPILE)gcc

SRCS = mxc_ipu_dev.c \
      mxc_ipu_lib.c

# list of platforms which want this test case
INCLUDE_LIST:=IMX37_3STACK IMX51_3STACK

ifeq ($(PLATFORM),$(findstring $(PLATFORM),$(INCLUDE_LIST)))
OBJS = mxc_ipu_dev.out
else
OBJS =
endif

all: $(OBJS)

$(OBJS): $(SRCS)
	$(CC) -g $(INC) $(CFLAGS) $^ -o $@
	mkdir -p $(OBJDIR)
	mv $(OBJS) $(OBJDIR)

.PHONY: clean
clean:
	rm -f $(OBJS)
