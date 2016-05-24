#mcp2510can-objs = mcp2510can.o spi.o
#obj-m   = mcp2510can.o


# mcp2510mode-objs = mcp2510.o ./lib/rbtreeDataArea.o ./lib/rbtreeRcvData.o ./lib/rbtreeWhiteList.o
mcp2510mode-objs = mcp2510.o lib/rbtreeDataArea.o lib/rbtreeRcvData.o lib/rbtreeWhiteList.o lib/rbtreeRmtCmd.o
obj-m	= mcp2510mode.o

KERNELS = ~/linux-2.6.30.4
#KERNELS = /media/STUDY/linux/kernel/my2440-2.6.36
#KERNELS = /lib/modules/$(shell uname -r)/build/

default:
	make -C $(KERNELS) M=$(shell pwd) modules

.PHONY:clean
clean:
	make -C $(KERNELS) M=$(shell pwd) clean
install:
	cp -r mcp2510mode.ko ~/tftpboot/mcp2510-2.6.30.4.ko

tag:
	ctags -R .
