# list of platforms which did not want this test case
EXCLUDE_LIST:="IMX37_3STACK IMX31_3STACK IMX31_ADS IMX27_3STACK IMX27_ADS"


LTP_DIR = $(TOPDIR)/test/mxc_pmic_test/
LOADLIBES+= -L$(LTP_DIR)lib -lltp

OBJS = autorun-mcu_pmic.sh libltp.a

CFLAGS+= -Wall -I$(LTP_DIR)/include
CFLAGS+= $(INC) -I$(LINUXPATH)/include/ -D__CVS_TEST

CFLAGS+= -DCONFIG_MXC_PMIC_MC9SDZ60

export CFLAGS LTP_DIR LOADLIBES

SUBDIRS =       protocol_tests \
		pmic_testapp_rtc

all: $(OBJS)
	@set -e; for i in $(SUBDIRS); do $(MAKE) -C $$i ; done

libltp.a:
	@$(MAKE) -C $(LTP_DIR)lib/ $@
	mkdir -p $(OBJDIR)
	@set -e; for i in $(SUBDIRS); do $(MAKE) -C $$i ; done

install:
	@set -e; for i in $(SUBDIRS); do $(MAKE) -C $$i install ; done

.PHONY: clean
clean:
	@$(MAKE) -C lib clean
	@set -e; for i in $(SUBDIRS); do $(MAKE) -C $$i clean ; done
#
# include the Rules
#
include ../make.rules
