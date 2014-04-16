	obj-m=mega_loader.o
	KDIR?=/home/user/kernel
	PWD=$(shell pwd)
	PLATFORM=arm
	PREFIX?=/opt/arm-2009q1/bin/arm-none-linux-gnueabi-
	EXTRA_CFLAGS=-Wall -Wformat=2 #-DDEBUG
modules:
	$(MAKE) ARCH=$(PLATFORM) CROSS_COMPILE=$(PREFIX) -C $(KDIR) M=$(PWD) modules

clean:
	rm *.o *.ko *.order *.symvers *.mod.c
