#
# Determine what directories need to be built
#

# list of platforms which did not want this test case
INCLUDE_LIST:= "IMX27ADS"

ifeq (,$(findstring $(PLATFORM), $(INCLUDE_LIST)))
CONFIG_DEMO_DPTCHI=n
else
CONFIG_DEMO_DPTCHI=y
endif

subdir-$(CONFIG_DEMO_DPTCHI)		+=	dptc_hi_demo

all :
	@for X in $(subdir-y); \
		do echo "   Buiding $$X"; \
		$(MAKE) -C $$X MODULE_NAME=$$X OBJDIR=$$TOPDIR/platform/$$PLATFORM/$$X; \
	done

.PHONY: clean
clean :
	@for X in $(subdir-y); \
		do echo "   Cleaning $$X"; \
		$(MAKE) -C $$X MODULE_NAME=$$X clean; \
	done

