# Just use the pxp library to get contiguous physical memory for USERPTR testing
# However, be aware that this is not the only way.
CFLAGS += -DGET_CONTI_PHY_MEM_VIA_PXP_LIB

TARGET = $(OBJDIR)/pxp_v4l2_test.out \
	$(OBJDIR)/pxp_v4l2_out_test.sh \
	$(OBJDIR)/mx6s_v4l2_capture.out

all : $(TARGET)

LDFLAGS += -lpthread -lpxp -lstdc++ -pthread

$(OBJDIR)/pxp_v4l2_test.out : pxp_v4l2_test.o
	$(CC) -o $(OBJDIR)/pxp_v4l2_test.out pxp_v4l2_test.o $(LDFLAGS) -Os
	$(STRIP) $(OBJDIR)/pxp_v4l2_test.out

$(OBJDIR)/mx6s_v4l2_capture.out : mx6s_v4l2_capture.o
	$(CC) -o $(OBJDIR)/mx6s_v4l2_capture.out mx6s_v4l2_capture.o $(LDFLAGS) -Os
	$(STRIP) $(OBJDIR)/mx6s_v4l2_capture.out

$(OBJDIR)/pxp_v4l2_out_test.sh: pxp_v4l2_out_test.sh
	cp pxp_v4l2_out_test.sh $(OBJDIR)

.PHONY: clean
clean :
	rm -f $(OBJS)

#
# include the Rules
#
include ../make.rules

