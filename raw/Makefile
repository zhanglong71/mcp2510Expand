
CROSS_COMPILE   = arm-linux-
CC = $(CROSS_COMPILE)gcc
CFLAG = -Wall -g
RM = rm -rf

# src := $(shell ls *.c)
# $(foreach n, $(src), $CC $(CFLAG) $(n))

all:
	$(CC) $(CFLAG) app_raw_read.c -o app_raw_read
	$(CC) $(CFLAG) app_raw_read-loop.c -o app_raw_read-loop
	$(CC) $(CFLAG) app_raw_write.c -o app_raw_write
	$(CC) $(CFLAG) app_raw_ioctl.c -o app_raw_ioctl
clean:
	$(RM) app_raw_read app_raw_write app_raw_ioctl app_raw_read-loop
install:
	cp -r app_raw_read ~/tftpboot/
	cp -r app_raw_write ~/tftpboot/
	cp -r app_raw_ioctl ~/tftpboot/
	cp -r app_raw_read-loop ~/tftpboot/
