# list of platforms which did not want this test case
EXCLUDE_LIST:=
CFLAGS += -lm

ifeq (,$(findstring $(PLATFORM), $(EXCLUDE_LIST)))
OBJS = $(OBJDIR)/autorun-fb.sh $(OBJDIR)/mxc_tve_test.sh $(OBJDIR)/mxc_fb_test.out \
       $(OBJDIR)/mxc_epdc_fb_test.out $(OBJDIR)/mxc_tvout_test.out $(OBJDIR)/mxc_fb_vsync_test.out \
       $(OBJDIR)/Lara240x320-565.rgb $(OBJDIR)/desk240x180-565.rgb $(OBJDIR)/daisy-640x480-565.rgb \
       $(OBJDIR)/rose-800x600-565.rgb $(OBJDIR)/wall-1024x768-565.rgb $(OBJDIR)/pansy-1280x720-565.rgb \
       $(OBJDIR)/plumbago-1280x1024-565.rgb
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
