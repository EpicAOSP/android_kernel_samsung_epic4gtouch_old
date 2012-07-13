#
# Makefile for the kernel westbridge device driver
#

ifneq ($(CONFIG_WESTBRIDGE_DEBUG),y)
	EXTRA_CFLAGS		+= -DWESTBRIDGE_NDEBUG
endif

EXTRA_CFLAGS		+= -DWESTBRIDGE_ASTORIA
EXTRA_CFLAGS		+= -D__USE_CYAS_AUTO_SUSPEND__

obj-$(CONFIG_WESTBRIDGE_DEVICE_DRIVER)  += cyasdev.o


ifeq ($(CONFIG_MACH_C110_WESTBRIDGE_AST_PNAND_HAL),y)
cyasdev-y		:= ../arch/arm/mach-c110/cyashalc110_pnand.o cyasdevice.o \
			   ../api/src/cyasdma.o ../api/src/cyasintr.o ../api/src/cyaslep2pep.o \
			   ../api/src/cyaslowlevel.o ../api/src/cyasmisc.o \
			   ../api/src/cyasstorage.o  \
			   ../block/cyasblkdev_block.o ../block/cyasblkdev_queue.o cyasdiagnostics.o

else
# should not get here, need to be built with some hal
cyasdev-y		:= cyasdevice.o
endif
