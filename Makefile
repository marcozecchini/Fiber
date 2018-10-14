obj-m += fiber.o

all: 
	make -C /usr/src/linux-headers-4.9.0-7-common  M=$(PWD) modules

clean: 
	make -C /usr/src/linux-headers-4.9.0-7-common  M=$(PWD) clean

