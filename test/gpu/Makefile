# list of platforms which want this test case
INCLUDE_LIST:=IMX6Q IMX6S

ifeq ($(PLATFORM),$(findstring $(PLATFORM),$(INCLUDE_LIST)))
OBJS = $(OBJDIR)/gpu.sh
else
OBJS =
endif

all: $(OBJS)

.PHONY: clean
clean:
	rm -f $(OBJS)

include ../make.rules
