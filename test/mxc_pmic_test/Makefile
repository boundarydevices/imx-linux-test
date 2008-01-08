# list of platforms which did not want this test case
EXCLUDE_LIST:= IMX37_3STACK

LTP_DIR = ../
LOADLIBES+= -L$(LTP_DIR)lib -lltp -lpthread -lm

ifeq (,$(findstring $(PLATFORM), $(EXCLUDE_LIST)))
OBJS = libltp.a $(OBJDIR)/autorun-pmic.sh
else
OBJS = 
endif

CFLAGS+= -Wall -I../include
CFLAGS+= $(INC) -D__CVS_TEST

ifeq "$(PLATFORM)" "MXC91131EVB"
CFLAGS+= -DCONFIG_MXC_PMIC_SC55112
else
CFLAGS+= -DCONFIG_MXC_PMIC_MC13783
endif

export CFLAGS LTP_DIR LOADLIBES

SUBDIRS =       protocol_tests \
		pmic_testapp_adc \
		pmic_testapp_light \
		pmic_testapp_battery \
		pmic_testapp_power \
		pmic_testapp_rtc 

all: $(OBJS)

libltp.a:
	@$(MAKE) -C lib $@
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
