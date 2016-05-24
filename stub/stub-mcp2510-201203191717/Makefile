
obj-m	= mcp.o

#KERNELS = /home/zl/linux-2.6.30.4
KERNELS = /home/zl/linux-2.6.32.2
#KERNELS = /lib/modules/$(shell uname -r)/build/

default:
	make -C $(KERNELS) M=$(shell pwd) modules

.PHONY:clean
clean:
	make -C $(KERNELS) M=$(shell pwd) clean
