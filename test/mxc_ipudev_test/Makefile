# list of platforms which want this test case
INCLUDE_LIST:=IMX31_3STACK IMX35_3STACK IMX37_3STACK IMX51_3STACK

SRCS = mxc_ipu_dev.c mxc_ipu_lib.c

ifeq ($(PLATFORM),$(findstring $(PLATFORM),$(INCLUDE_LIST)))
OBJS = mxc_ipu_dev.out
$(OBJS): $(SRCS)
	$(VERBOSE) $(CC) -g $(INC) $(CFLAGS) $^ $(LIBS) -o $@
	mkdir -p $(OBJDIR)
	mv $@ $(OBJDIR)
else
OBJS =
endif

all: $(OBJS)

.PHONY: clean
clean:
	rm -f $(OBJS)

include ../make.rules
