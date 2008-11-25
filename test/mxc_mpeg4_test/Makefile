#
# Setup the proper directories for make.rules to function
#
OBJDIR=$(TOPDIR)/platform/$(PLATFORM)/$(MODULE_NAME)
SRCDIR=$(TOPDIR)/test/$(MODULE_NAME)

LIBS = $(SRCDIR)/libmpeg4_enc_ELINUX.a $(SRCDIR)/libmpeg4_dec_ELINUX.a -lm -lc -lrt
CFLAGS += -DNDEBUG -DPROFILE -msoft-float -mfpu=vfp

# list of platforms which did not want this test case
EXCLUDE_LIST:="IMX21ADS IMX27ADS IMX51_3STACK IMX25_3STACK"

ifeq (,$(findstring $(PLATFORM), $(EXCLUDE_LIST)))
EXES = \
	$(OBJDIR)/mxc_mpeg4enc_test.out \
	$(OBJDIR)/mxc_mpeg4dec_test.out
else
EXES =
endif

all : $(EXES) clean_build

#
# include the Rules
#
include $(TOPDIR)/test/make.rules

# Override default rules
$(OBJDIR)/%.out : ./%.c

$(OBJDIR)/%.out : $(SRCDIR)/%.c

include $(EXES:.out=.d)

.PHONY: clean_build
clean_build:
	rm -f $(OBJDIR)/*.d $(OBJDIR)/*.o

.PHONY: clean
clean:
	rm -f $(EXES)
