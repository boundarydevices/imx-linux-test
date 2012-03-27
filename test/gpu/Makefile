# list of platforms which want this test case
INCLUDE_LIST:=IMX6Q

ifeq ($(PLATFORM),$(findstring $(PLATFORM),$(INCLUDE_LIST)))
OBJS = $(OBJDIR)/gpu.sh
$(OBJS): $(SRCS)
	mkdir -p $(OBJDIR)
	cp gpu.sh $(OBJDIR)
else
OBJS =
endif

all: $(OBJS)

.PHONY: clean
clean:
	rm -f $(OBJS)

include ../make.rules
