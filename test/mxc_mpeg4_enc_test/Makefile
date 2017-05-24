#
# Setup the proper directories for make.rules to function
#
OBJDIR=$(TOPDIR)/platform/$(PLATFORM)/$(MODULE_NAME)
CFLAGS += -I . -lpthread

EXES = $(OBJDIR)/cam2mpeg4.out
SCRIPTS = $(OBJDIR)/autorun-hantro.sh
LIBS = libmpeg4enc_vga.a
OBJS = cam2mpeg4_vga.o

all: $(EXES) $(SCRIPTS)

LDFLAGS += -lpthread

$(EXES): $(OBJS)
	$(CC) -o $(EXES) $(OBJS) libmpeg4enc_vga.a $(LDFLAGS) -Os
	$(STRIP) $(EXES)

.PHONY: clean
clean:
	rm -f $(EXES) $(OBJS) $(SCRIPTS)

#
# include the Rules
#
include $(TOPDIR)/test/make.rules

#include $(EXES:.out=.d)
