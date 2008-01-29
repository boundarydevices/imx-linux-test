export INC = -I$(TOPDIR)/include $(INCLUDE)
 
# Get all dirs with a Makefile
TMP_DIRS := $(foreach dir, $(wildcard *), $(wildcard $(dir)/Makefile))
DIRS := $(patsubst %/Makefile,%,$(TMP_DIRS))

.PHONY: all
.PHONY: $(DIRS)
all: $(DIRS)

$(DIRS):
	-$(MAKE) -C $< $@
