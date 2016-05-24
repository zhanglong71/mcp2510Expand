
CROSS_COMPILE   = arm-linux-
CC = $(CROSS_COMPILE)gcc
CFLAG = -Wall -g
RM = rm -rf

# src := $(shell ls *.c)
# $(foreach n, $(src), $CC $(CFLAG) $(n))

all:
	$(CC) $(CFLAG) app_zigbee.c -o app_zigbee
	$(CC) $(CFLAG) app_zigbee-loop.c -o app_zigbee-loop
clean:
	$(RM) app_zigbee app_zigbee-loop
install:
	cp -r app_zigbee ~/tftpboot/
