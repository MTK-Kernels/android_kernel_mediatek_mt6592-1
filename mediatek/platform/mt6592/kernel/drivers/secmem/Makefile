include $(MTK_ROOT_BUILD)/Makefile

ifeq ($(strip $(TRUSTONIC_TEE_SUPPORT)),yes)
ccflags-y += -I$(MTK_PATH_SOURCE)/../platform/${ARCH_MTK_PLATFORM}/kernel/core/include/trustzone/utils/tlutils
endif

ifeq ($(strip $(TRUSTONIC_TEE_SUPPORT)),yes)
ccflags-y += -I$(MTK_PATH_PLATFORM)/drivers/gud/MobiCoreKernelApi/include \
             -I$(MTK_PATH_PLATFORM)/drivers/gud/MobiCoreKernelApi/public 

obj-y += secmem.o
endif
