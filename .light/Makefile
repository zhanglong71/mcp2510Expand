
obj-m	= light.o

KERNELS = ~/linux-2.6.30.4
#KERNELS = /media/STUDY/linux/kernel/my2440-2.6.36
#KERNELS = /lib/modules/$(shell uname -r)/build/

default:
	make -C $(KERNELS) M=$(shell pwd) modules

.PHONY:clean
clean:
	make -C $(KERNELS) M=$(shell pwd) clean
install:
	cp -r light.ko ~/tftpboot/
	make -C test_APP isntall
