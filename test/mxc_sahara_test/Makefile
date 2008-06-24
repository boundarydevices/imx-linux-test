###################################################################
#Copyright 2005-2006 Freescale Semiconductor, Inc. All Rights Reserved.
###################################################################

# list of platforms which did not want this test case
EXCLUDE_LIST:= MXC91131EVB I30030ADS I30030EVB IMX21ADS IMX31ADS IMX31_3STACK MXC30020EVB MXC27520EVB IMX35_3STACK

ifeq (,$(findstring $(PLATFORM), $(EXCLUDE_LIST)))
OBJS = apitest
else
OBJS =
endif

ifeq ("$(PLATFORM)", "MXC27530EVB")
TARGET_ARCH=-DCONFIG_ARCH_MXC91231
endif
ifeq ("$(PLATFORM)", "MXC30030ADS")
TARGET_ARCH=-DCONFIG_ARCH_MXC91321
endif
ifeq ("$(PLATFORM)", "MXC30030EVB")
TARGET_ARCH=-DCONFIG_ARCH_MXC91321
endif
ifeq ("$(PLATFORM)", "IMX27ADS")
TARGET_ARCH=-DCONFIG_ARCH_MX27
endif
ifeq ("$(PLATFORM)", "MXC30031ADS")
TARGET_ARCH=-DCONFIG_ARCH_MXC92323
endif

# Add compilation checks
CFLAGS += -g -Wall -W -Wmissing-prototypes -Wstrict-prototypes -Wdeclaration-after-statement

EXAMPLE_OBJECTS:= apitest.o run_tests.o sym.o hash.o gen_encrypt.o \
                  callback.o auth_decrypt.o rand.o hmac1.o hmac2.o cap.o \
                  wrap.o results.o apihelp.o

EXAMPLE_OBJECTS:= $(addprefix obj/, $(EXAMPLE_OBJECTS))

LIBS = -lsahara

CFLAGS += $(TARGET_ARCH) -DSAHARA2 -DSAHARA $(INC)

# Uncomment to debug Library's creation of structures for driver
# CFLAGS += -DDIAG_SECURITY_FUNC

all : $(OBJS)
	mkdir -p $(OBJDIR)
	cp $^ $(OBJDIR)
	cp -f autorun-*.sh $(OBJDIR)

$(OBJS) : $(EXAMPLE_OBJECTS)
	$(CC) $(CFLAGS) $^ $(LIBS) -o $@

obj/%.o : %.c
	@mkdir -p obj
	$(CC) -c -g $(INC) $(CFLAGS) $< -o $@

.PHONY : clean
clean:
	@rm -f $(EXAMPLE_OBJECTS) apitest .*.cmd

# Should build these from the source/object list
apitest.o: apitest.c $(HDRS)
apihelp.o: apihelp.c $(HDRS)
run_tests.o: run_tests.c $(HDRS)
auth_decrypt.o: auth_decrypt.c  $(HDRS)
callback.o: callback.c $(HDRS)
cap.o: cap.c  $(HDRS)
gen_encrypt.o: gen_encrypt.c  $(HDRS)
hash.o: hash.c  $(HDRS)
hmac1.o: hmac1.c  $(HDRS)
hmac2.o: hmac2.c  $(HDRS)
rand.o: rand.c  $(HDRS)
results.o: results.c $(HDRS)
sym.o: sym.c  $(HDRS)
wrap.o: wrap.c $(HDRS)

include ../make.rules
