# list of platforms which did not want this test case
EXCLUDE_LIST:= IMX21ADS IMX31ADS IMX31_3STACK IMX35_3STACK IMX37_3STACK IMX25_3STACK IMX50 IMX6Q IMX6SL IMX6SX

ifeq (,$(findstring $(PLATFORM), $(EXCLUDE_LIST)))
 OBJS= apitest

ifeq ("$(PLATFORM)", "IMX27ADS")
TARGET_ARCH=-DCONFIG_ARCH_MX27
endif
ifeq ($(findstring IMX5, $(PLATFORM)), IMX5)
TARGET_ARCH=-DCONFIG_ARCH_MX5
endif

# Add compilation checks
CFLAGS += -g -Wall -W -Wmissing-prototypes -Wstrict-prototypes -Wdeclaration-after-statement -fPIC

EXAMPLE_OBJECTS:= apitest.o run_tests.o sym.o hash.o gen_encrypt.o \
                  callback.o auth_decrypt.o rand.o hmac1.o hmac2.o cap.o \
                  wrap.o results.o apihelp.o smalloc.o user_wrap.o  dryice.o

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
dryice.o: dryice.c $(HDRS)
gen_encrypt.o: gen_encrypt.c  $(HDRS)
hash.o: hash.c  $(HDRS)
hmac1.o: hmac1.c  $(HDRS)
hmac2.o: hmac2.c  $(HDRS)
rand.o: rand.c  $(HDRS)
results.o: results.c $(HDRS)
smalloc.o: smalloc.c  $(HDRS)
sym.o: sym.c  $(HDRS)
user_wrap.o: user_wrap.c $(HDRS)
wrap.o: wrap.c $(HDRS)

else
all :

endif

include ../make.rules

