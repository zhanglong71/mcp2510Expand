
CROSS_COMPILE   = arm-linux-
CC = $(CROSS_COMPILE)gcc
CFLAG = -Wall -g
RM = rm -rf


all:
	echo $(SRC)
	$(CC) $(CFLAG) app_rw.c -o app_rw
##	cp -rv app_rw ~/tftpboot/
clean:
	$(RM) app_rw 
