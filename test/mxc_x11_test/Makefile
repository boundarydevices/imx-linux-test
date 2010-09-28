# list of platforms which did not want this test case
EXCLUDE_LIST:=
CFLAGS += -lm
LIBS += -lX11 -lXdamage

ifeq (,$(findstring $(PLATFORM), $(EXCLUDE_LIST)))
OBJS = $(OBJDIR)/mxc_epdc_x11_fb_test.out
else
OBJS =
endif

all : $(OBJS)

.PHONY: clean
clean :
	rm -f $(OBJS)

#
# include the Rules
#
include ../make.rules
