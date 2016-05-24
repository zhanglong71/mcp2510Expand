
mcp-objs = mcp2510t.o ../../lib/rbtreeRcvData.o
obj-m	= mcp.o

KERNELS = ~/linux-2.6.32.2
#KERNELS = /lib/modules/$(shell uname -r)/build/

default:
	make -C $(KERNELS) M=$(shell pwd) modules

.PHONY:clean
clean:
	make -C $(KERNELS) M=$(shell pwd) clean
