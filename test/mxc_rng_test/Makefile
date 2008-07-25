#*****************************************************************************
#
# makefile for rng_test and apitest programs
#
#****************************************************************************/

# list of platforms which did not want this test case
EXCLUDE_LIST:="IMX27ADS IMX27EVB MXC27530EVB MXC30030ADS MXC30030EVB MXC30031ADS"

ifeq (,$(findstring $(PLATFORM), $(EXCLUDE_LIST)))
OBJS = apitest rng_test move
else
OBJS =
endif
KERNEL_DIR = $(LINUXPATH)
ifeq ("$(PLATFORM)", "MXC27520EVB")
TARGET_ARCH=-DCONFIG_ARCH_MXC91221
endif
ifeq ("$(PLATFORM)", "MXC30020EVB")
TARGET_ARCH=-DCONFIG_ARCH_MXC91311
endif
ifeq ("$(PLATFORM)", "IMX31ADS")
TARGET_ARCH=-DCONFIG_ARCH_MX3
endif
ifeq ("$(PLATFORM)", "IMX31_3STACK")
TARGET_ARCH=-DCONFIG_ARCH_MX3
endif
ifeq ("$(PLATFORM)", "MXC91131EVB")
TARGET_ARCH=-DCONFIG_ARCH_MXC91131
endif
ifeq ("$(PLATFORM)", "IMX37_3STACK")
TARGET_ARCH=-DCONFIG_ARCH_MX37
endif
ifeq ("$(PLATFORM)", "IMX35_3STACK")
TARGET_ARCH=-DCONFIG_ARCH_MX35
endif


# Location of local headers and API "common" sources
INCLUDEDIR= -I. -I../../include -I../mxc_sahara_test $(INC)

AR = $(CROSS_COMPILE)ar -crv
RM = rm -f

# Add compilation checks
CFLAGS += -g -Wall -W -Wmissing-prototypes -Wstrict-prototypes -Wdeclaration-after-statement

# Location of test sources
TSDIR=../mxc_sahara_test
TEST_SOURCES = apitest.c  run_tests.c  sym.c  hash.c  gen_encrypt.c         \
               callback.c  auth_decrypt.c  rand.c  hmac1.c  hmac2.c  cap.c  \
               wrap.c  results.c apihelp.c

LIBS_DIR=$(TOPDIR)/lib/rng
LIBS=$(LIBS_DIR)/libfsl_shw.a

OPTFLAGS = -O2
CFLAGS += $(OPTFLAGS) -DARM_LINUX -Wall $(INCLUDEDIR)

# In absence of kernel CONFIG flags, set API library to build on
# 'unknown' platform which has an RNGA.
CFLAGS += $(TARGET_ARCH)
 

CC=$(CROSS_COMPILE)gcc
LD=$(CROSS_COMPILE)ld

all: $(OBJS)

rng_test: rng_test.o apihelp.o $(LIBS)
	$(CC) -g -o $@ $^

apitest:  $(TEST_SOURCES:.c=.o) $(LIBS)
	$(CC) -g -o $@ $^

$(LIBS):
	$(MAKE) -C $(LIBS_DIR)

move:
	mkdir -p $(OBJDIR)
	mv apitest rng_test $(OBJDIR)
	cp -f autorun-*.sh $(OBJDIR)
#	mv *.o $(OBJDIR)

.PHONY: clean
clean:
	@rm -f *.o rng_test

# Tests "borrowed" from Sahara2 tree
apitest.o: $(TSDIR)/apitest.c $(HDRS)
	$(CC) -c $< $(CFLAGS)
apihelp.o: $(TSDIR)/apihelp.c $(HDRS)
	$(CC) -c $< $(CFLAGS)
run_tests.o: $(TSDIR)/run_tests.c $(HDRS)
	$(CC) -c $< $(CFLAGS)
auth_decrypt.o: $(TSDIR)/auth_decrypt.c  $(HDRS)
	$(CC) -c $< $(CFLAGS)
callback.o: $(TSDIR)/callback.c $(HDRS)
	$(CC) -c $< $(CFLAGS)
cap.o: $(TSDIR)/cap.c  $(HDRS)
	$(CC) -c $< $(CFLAGS)
gen_encrypt.o: $(TSDIR)/gen_encrypt.c  $(HDRS)
	$(CC) -c $< $(CFLAGS)
hash.o: $(TSDIR)/hash.c  $(HDRS)
	$(CC) -c $< $(CFLAGS)
hmac1.o: $(TSDIR)/hmac1.c  $(HDRS)
	$(CC) -c $< $(CFLAGS)
hmac2.o: $(TSDIR)/hmac2.c  $(HDRS)
	$(CC) -c $< $(CFLAGS)
rand.o: $(TSDIR)/rand.c  $(HDRS)
	$(CC) -c $< $(CFLAGS)
results.o: $(TSDIR)/results.c $(HDRS)
	$(CC) -c $< $(CFLAGS)
sym.o: $(TSDIR)/sym.c  $(HDRS)
	$(CC) -c $< $(CFLAGS)
wrap.o: $(TSDIR)/wrap.c $(HDRS)
	$(CC) -c $< $(CFLAGS)
