
ifneq ($(KERNELRELEASE),)
# kbuild part of makefile 

# When adding modules, either add to the 'module_list' for all platfoms or add 
# below to the 'module_list +=...' list for each platform that needs it.
module_list-y := mxc_rtic_test.o \
		mxc_udma_testdriver.o 

module_list-$(CONFIG_MXC_PMIC_MC13783) += mxc_pm_test.o mxc_pmic_power_testmod.o
module_list-$(CONFIG_MXC_IPC) += ipctestmod.o
module_list-$(CONFIG_MXC_SECURITY_SCC) += scc_test_driver.o
module_list-$(CONFIG_MXC_SECURITY_RNG) += rng_test_driver.o
module_list-$(CONFIG_MXC_SECURITY_HAC) += mxc_HAC_test.o
module_list-$(CONFIG_MXC_SAHARA) += sahara_test_driver.o
module_list-$(CONFIG_MXC_HMP4E) += memalloc.o

ifeq ($(CONFIG_MXC_SDMA_API),y)
module_list-y += mxc_sdma_mem_test.o
else
module_list-y += mxc_udma_testdriver.o
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
                           $(SAHARA_TEST_SOURCE)callback.o


obj-m += $(module_list-y)
obj-y := pmic_convity_test/

EXTRA_CFLAGS := -DLINUX_KERNEL

## NOTE: The following should be removed. They are directly requesting kernel
##       source.
EXTRA_CFLAGS += -I$(LINUXPATH)/drivers/mxc/security/rng/include -I$(LINUXPATH)/drivers/mxc/security/sahara2/include 

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
