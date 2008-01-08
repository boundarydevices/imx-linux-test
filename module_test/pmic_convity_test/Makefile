#
# Build the pmic_convity unit test modules.
#

ifeq ($(CONFIG_MXC_PMIC_MC13783),y)
obj-m += utest9_set_callback.o
endif

utest9_set_callback-objs  := utest9_set_callback_main.o test_support.o

utest9_set_callback.o test_support.o : test_support.h
