obj-m += fiber.o
fiber-y := device.o fibers.o kprobe.o subfolder_proc.o

all: 
#	make -C /usr/src/linux-headers-$(shell uname -r)  M=$(PWD) modules
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
	rmmod fiber
	insmod fiber.ko

install: 
	make -C /usr/src/linux-headers-$(shell uname -r)  M=$(PWD) modules_install


clean: 
	make -C /usr/src/linux-headers-%(shell uname -r)  M=$(PWD) clean

test:
	./../test/test
	dmesg
