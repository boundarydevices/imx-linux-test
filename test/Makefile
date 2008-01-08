export INC = -I$(TOPDIR)/include $(INCLUDE)

subdirs 	= $(shell /bin/ls -d */)

all:
	@for X in $(subdirs); do        \
	    if [ -r "$$X/Makefile" ]; then \
		$(MAKE) -C $$X MODULE_NAME=$$X OBJDIR=$$TOPDIR/platform/$$PLATFORM/$$X; \
	    fi; 			\
	done

.PHONY: clean
clean:
	@for X in $(subdirs); do        \
	    if [ -r "$$X/Makefile" ]; then \
		$(MAKE) -C $$X MODULE_NAME=$$X clean; \
	    fi; 			\
	done

%::
	if [ -r "$$X/Makefile" ]; then \
		$(MAKE) -C $@ MODULE_NAME=$@ OBJDIR=$$TOPDIR/platform/$$PLATFORM/$@; \
	fi;
	
