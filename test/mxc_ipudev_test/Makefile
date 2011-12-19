# list of platforms which want this test case
INCLUDE_LIST:=IMX51 IMX53 IMX6Q

SRCS = mxc_ipudev_test.c utils.c
CFLAGS += -lm -lrt

ifeq ($(PLATFORM),$(findstring $(PLATFORM),$(INCLUDE_LIST)))
OBJS = $(OBJDIR)/mxc_ipudev_test.out
$(OBJS): $(SRCS)
	mkdir -p $(OBJDIR)
	$(VERBOSE) $(CC) -g $(INC) $(CFLAGS) $^ $(LIBS) -o $@
	cp ipudev_config_file $(OBJDIR)
	cp autorun-ipu.sh $(OBJDIR)
	cp stefan_interlaced_320x240_5frames.yv12 $(OBJDIR)
else
OBJS =
endif

all: $(OBJS)

.PHONY: clean
clean:
	rm -f $(OBJS)

include ../make.rules
