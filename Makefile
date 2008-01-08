TOPDIR	:= $(shell /bin/pwd)
OBJDIR=$(TOPDIR)/platform/$(PLATFORM)/

# Suffix for install dir (e.g. DESTDIR_SUFFIX=unittests)
DESTDIR_SUFFIX=

MISC_DIR := $(shell echo $(TOPDIR) | sed 's/^.*\///')
PKG_NAME := $(MISC_DIR).tar.gz

EXCLUDES := $(foreach ex, $(PKG_EXCLUDES), --exclude $(ex))
EXCLUDES += --exclude $(PKG_NAME)

#
# ltib requires CROSS_COMPILE to be undefined
#
#ifeq "$(CROSS_COMPILE)" ""
#$(error CROSS_COMPILE variable not set.)
#endif

ifeq "$(KBUILD_OUTPUT)" ""
$(warn KBUILD_OUTPUT variable not set.)
endif

ifeq "$(PLATFORM)" ""
$(warn "PLATFORM variable not set. Check if you have specified PLATFORM variable")
endif

ifeq "$(DESTDIR)" ""
install_target=install_dummy
else
install_target=$(shell if [ -d $(TOPDIR)/platform/$(PLATFORM) ]; then echo install_actual; \
		       else echo install_dummy; fi; )
endif

# remove the string './test/' from the list of targets
app_targets = $(subst ./test/, ,$(app_dir))

#
# Export all variables that might be needed by other Makefiles
#
export INC CROSS_COMPILE LINUXPATH PLATFORM TOPDIR OBJDIR

.PHONY: test misc demo tool module_test clean pkg
.PHONY: $(app_targets)

all  : misc test module_test

misc:
	@mkdir -p $(OBJDIR)
	@echo "CFLAGS:    $(CFLAGS)"
	@echo "AFLAGS:    $(AFLAGS)"
	@echo "OBJDIR:    $(OBJDIR)"
	@echo ""

test: 
	@echo
	@echo Invoking test make...
	$(MAKE) -C $(TOPDIR)/test
	@for script in autorun.sh test-utils.sh autorun-suite.txt misc-testdatabase.txt; do \
		echo "copying $$script..."; \
		cp -af $(TOPDIR)/$$script $(OBJDIR)/; \
		chmod u+x $(OBJDIR)/$$script; \
	done

%::
	$(MAKE) -C $(TOPDIR)/test $@

module_test:
	@echo
	@echo Building test modules...
	$(MAKE) -C $(TOPDIR)/module_test OBJDIR=$(OBJDIR)/modules

install: $(install_target)

install_dummy:
	@echo -e "\n**DESTDIR not set or Build not yet done. No installtion done."
	@echo -e "**If build is complete files will be under $(TOPDIR)/platform/$(PLATFORM)/ dir."

install_actual:
	@echo -e "\nInstalling files from platform/$(PLATFORM)/ to $(DESTDIR)/$(DESTDIR_SUFFIX)/"
	mkdir -p $(DESTDIR)/$(DESTDIR_SUFFIX)
	-rm -rf $(DESTDIR)/$(DESTDIR_SUFFIX)
	mv $(TOPDIR)/platform/$(PLATFORM) $(DESTDIR)/$(DESTDIR_SUFFIX)

clean :
	@for X in test module_test $(shell /bin/ls -d lib/*); do \
		if [ -r "$$X/Makefile" ]; then \
			$(MAKE) -C $$X clean; \
		fi \
	done
	-rm -rf platform

pkg : clean
	tar --exclude CVS -C .. $(EXCLUDES) -czf $(PKG_NAME) $(MISC_DIR)