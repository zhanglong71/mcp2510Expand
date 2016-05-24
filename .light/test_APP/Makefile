
CROSS_COMPILE   = arm-linux-
CC = $(CROSS_COMPILE)gcc
CFLAG = -Wall -g
RM = rm -f

# src := $(shell ls *.c)
# $(foreach n, $(src), $CC $(CFLAG) $(n))

all:
	$(CC) $(CFLAG) app_light.c -o app_light
	$(CC) $(CFLAG) app_light-loop.c -o app_light-loop
clean:
	$(RM) app_light app_light-loop
install:
	cp -r app_light ~/tftpboot/

