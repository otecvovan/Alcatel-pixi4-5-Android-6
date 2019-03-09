export MTK_PLATFORM := $(subst ",,$(CONFIG_MTK_PLATFORM))
export ARCH_MTK_PROJECT := $(subst ",,$(CONFIG_ARCH_MTK_PROJECT))

MTK_PLATFORM_PATH_ROOT := $(srctree)/drivers/misc/mediatek/mach/$(MTK_PLATFORM)
MTK_PROJECT_PATH_ROOT := $(srctree)/drivers/misc/mediatek/mach/$(MTK_PLATFORM)/$(ARCH_MTK_PROJECT)
MTK_CUSTOM_PATH  := $(srctree)/drivers/misc/mediatek
MTK_CUSTOM_PATH1 := $(srctree)/drivers/misc/mediatek/include/mt-plat/mt6735
subdir-ccflags-y += -I$(MTK_CUSTOM_PATH)/flashlight/inc \
             -I$(MTK_CUSTOM_PATH)/imgsensor/inc \
			 -I$(MTK_CUSTOM_PATH1)/include/mach \

CUSTOM_KERNEL_FLAGS := $(addprefix -D, $(shell echo \
             $(CONFIG_CUSTOM_KERNEL_FLASHLIGHT) | tr a-z A-Z))
subdir-ccflags-y += $(CUSTOM_KERNEL_FLAGS)

ifneq ($(wildcard $(srctree)/drivers/misc/mediatek/flashlight/src/$(MTK_PLATFORM)),)
obj-y += $(MTK_PLATFORM)/
else
obj-y += ../dummy_flashlight/
endif



