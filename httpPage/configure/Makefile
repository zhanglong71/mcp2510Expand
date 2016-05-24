# CROSS_COMPILE = arm-linux-
CC = $(CROSS_COMPILE)gcc
LD = $(CROSS_COMPILE)gcc

##CCFLAG = -c -Wall -DDEBUG
CCFLAG = -c -Wall
LDFLAG = -g -Wall
RM = rm -f

all:
	${CC} ${CCFLAG} addrcfg.c -o addrcfg.o
	${LD} ${LDFLAG} addrcfg.o -o addrcfg.cgi
clean:
	${RM} *.o addrcfg.cgi
install:
	#cp -r addrcfg.cgi ~/tftpboot/
