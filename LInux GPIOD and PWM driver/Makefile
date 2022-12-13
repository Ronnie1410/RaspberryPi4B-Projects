
CROSS_COMPILE = arm-linux-gnueabihf-
ARCH=arm
# CC=i586-poky-linux-gcc
# CC=arm-linux-gnueabihf-gcc-8

APP= assignment2

KDIR=/lib/modules/$(shell uname -r)/build

obj-m = cycle_count_mod.o

ccflags-m += -Wall

all:
	make ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KDIR) M=$(PWD) modules
	gcc -Wall -o $(APP) main.c -lrt -lgpiod
	
clean:
	make ARCH=x86 CROSS_COMPILE=$(CROSS_COMPILE) -C $(KDIR) M=$(PWD) clean
