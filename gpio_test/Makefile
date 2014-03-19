	obj-m=gpio_test.o
	KDIR?=/home/michael/ti/linux-3.1.10
	PWD=$(shell pwd)
	PLATFORM=arm
	PREFIX?=/home/michael/ti/ti-sdk-omapl138-lcdk-01.00.00/linux-devkit/bin/arm-arago-linux-gnueabi-
	EXTRA_CFLAGS=-Wall -Wformat=2
	#EXTRA_CFLAGS=-Wall -Wformat=2 -DDEBUG
modules:
	@echo "Make module..."
	@$(MAKE) ARCH=$(PLATFORM) CROSS_COMPILE=$(PREFIX) -C $(KDIR) M=$(PWD) modules

clean:
	@echo "Clean..."
	@rm -f *.o *.ko *.order *.symvers *.mod.c
