# list of platforms which did not want this test case
EXCLUDE_LIST:="IMX35_3STACK"

ifeq (,$(findstring $(PLATFORM), $(EXCLUDE_LIST)))
OBJS = \
	$(OBJDIR)/mxc_i2c_test.out \
	$(OBJDIR)/mxc_i2c_test_usb.out \
	$(OBJDIR)/mxc_i2c_test_samsung_cam.out \
	$(OBJDIR)/mxc_i2c_test_magna_cam.out
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

