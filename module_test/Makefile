
ifneq ($(KERNELRELEASE),)
# kbuild part of makefile

# When adding modules, either add to the 'module_list' for all platfoms or add
# below to the 'module_list +=...' list for each platform that needs it.
module_list-$(CONFIG_MXC_SECURITY_SCC) += scc_test_driver.o
module_list-$(CONFIG_MXC_SECURITY_SCC2) += scc2_test_driver.o
module_list-$(CONFIG_MXC_SECURITY_RNG) += rng_test_driver.o
module_list-$(CONFIG_MXC_SAHARA) += sahara_test_driver.o
module_list-$(CONFIG_MXC_HMP4E) += memalloc.o
module_list-$(CONFIG_MXC_DRYICE) += dryice_test.o

# Build Sahara Kernel Test Module for RNG Kernel Mode Tests
ifeq ($(CONFIG_MXC_SECURITY_RNG),y)
module_list-y += sahara_test_driver.o
endif
ifeq ($(CONFIG_MXC_SDMA_API),y)
module_list-y += mxc_sdma_mem_test.o
endif
ifeq ($(CONFIG_FB_MXC_EINK_PANEL),y)
module_list-y += epdc_test_driver.o
endif
ifeq ($(CONFIG_FB_MXC_SIPIX_PANEL),y)
module_list-y += spdc_test_driver.o
endif

# sahara uses common files for both test application and test module
SAHARA_TEST_SOURCE=../test/mxc_sahara_test/

sahara_test_driver-objs := km_test.o $(SAHARA_TEST_SOURCE)run_tests.o \
                           $(SAHARA_TEST_SOURCE)results.o \
                           $(SAHARA_TEST_SOURCE)cap.o \
                           $(SAHARA_TEST_SOURCE)hash.o \
                           $(SAHARA_TEST_SOURCE)rand.o \
                           $(SAHARA_TEST_SOURCE)sym.o \
                           $(SAHARA_TEST_SOURCE)hmac1.o \
                           $(SAHARA_TEST_SOURCE)hmac2.o \
                           $(SAHARA_TEST_SOURCE)wrap.o \
                           $(SAHARA_TEST_SOURCE)apihelp.o \
                           $(SAHARA_TEST_SOURCE)gen_encrypt.o \
                           $(SAHARA_TEST_SOURCE)callback.o \
                           $(SAHARA_TEST_SOURCE)dryice.o \
                           $(SAHARA_TEST_SOURCE)smalloc.o \
                           $(SAHARA_TEST_SOURCE)user_wrap.o


obj-m += $(module_list-y) $(module_list-m)

EXTRA_CFLAGS := -DLINUX_KERNEL -DIMX_NEEDS_DEPRECATED_SYMBOLS

## NOTE: The following should be removed. They are directly requesting kernel
##       source.
EXTRA_CFLAGS += -I$(LINUXPATH)/drivers/mxc/security -I$(LINUXPATH)/drivers/mxc/security/rng/include -I$(LINUXPATH)/drivers/mxc/security/sahara2/include

else

# Need to remove sahara objects because the test app uses same objects
all:
	$(MAKE) -C $(LINUXPATH) M=`pwd` KBUILD_OUTPUT=$(KBUILD_OUTPUT) ARCH=arm CROSS_COMPILE=$(CROSS_COMPILE)  modules

install:
	$(MAKE) -C $(LINUXPATH) INSTALL_MOD_DIR=test M=`pwd` KBUILD_OUTPUT=$(KBUILD_OUTPUT) ARCH=arm CROSS_COMPILE=$(CROSS_COMPILE)  modules_install

.PHONY: clean
clean :
	@rm -f *.o *.mod.c *.ko .*.cmd
	@rm -rf .tmp_versions
	@for X in $(shell /bin/ls -d */); do	\
		cd $$X;				\
		rm -f *.o *.mod.c *.ko .*.cmd;	\
		rm -rf .tmp_versions;		\
		cd ..;				\
	done
endif
