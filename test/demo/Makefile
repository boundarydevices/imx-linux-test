#
# Determine what directories need to be built
#

# list of platforms which did not want this test case
EXCLUDE_LIST:= "IMX35_3STACK IMX51_3STACK"

ifeq (,$(findstring $(PLATFORM), $(EXCLUDE_LIST)))
CONFIG_DEMO_DPTCHI=y
CONFIG_DEMO_DVFS_DPTCHI=y
else
CONFIG_DEMO_DPTCHI=n
CONFIG_DEMO_DVFS_DPTCHI=n
endif

subdir-$(CONFIG_DEMO_DPTCHI)		+=	dptc_hi_demo
subdir-$(CONFIG_DEMO_DVFS_DPTCHI)	+=	dvfs_dptc_hi_demo

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

