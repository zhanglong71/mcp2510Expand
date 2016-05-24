
CROSS_COMPILE   = arm-linux-
CC = $(CROSS_COMPILE)gcc
CFLAG = -Wall -g
RM = rm -rf
CP = cp -rv

# src := $(shell ls *.c)
# $(foreach n, $(src), $CC $(CFLAG) $(n))

all:
	$(CC) $(CFLAG) huarainBtns.c -o  huarainBtns

install:
	$(CP) huarainBtns ~/tftpboot/

clean:
	$(RM) huarainBtns
