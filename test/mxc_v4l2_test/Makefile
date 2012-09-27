# list of platforms which did not want this test case
EXCLUDE_LIST:=
CSI_LIST:=IMX25_3STACK IMX6S

CFLAGS += -lpthread

ifeq (,$(findstring $(PLATFORM), $(EXCLUDE_LIST)))
OBJS = 	$(OBJDIR)/autorun-v4l2.sh \
	$(OBJDIR)/mxc_v4l2_output.out \
	$(OBJDIR)/mxc_v4l2_still.out \
	$(OBJDIR)/mxc_v4l2_tvin.out
ifeq ($(PLATFORM),$(findstring $(PLATFORM),$(CSI_LIST)))
OBJS += $(OBJDIR)/csi_v4l2_overlay.out \
	$(OBJDIR)/csi_v4l2_capture.out
else
OBJS += $(OBJDIR)/mxc_v4l2_overlay.out \
	$(OBJDIR)/mxc_v4l2_capture.out
endif

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


