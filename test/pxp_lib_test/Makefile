# list of platforms which want this test case
INCLUDE_LIST:=IMX50 IMX51 IMX5 IMX6Q

SRCS = pxp_test.c utils.c
CFLAGS += -lpxp -lpthread

ifeq ($(PLATFORM),$(findstring $(PLATFORM),$(INCLUDE_LIST)))
OBJS = $(OBJDIR)/pxp_test.out \
	$(OBJDIR)/autorun-pxp.sh
$(OBJS): $(SRCS)
	mkdir -p $(OBJDIR)
	$(VERBOSE) $(CC) -g $(INC) $(CFLAGS) $^ $(LIBS) -o $@
else
OBJS =
endif

all: $(OBJS)

.PHONY: clean
clean:
	rm -f $(OBJS)

include ../make.rules
