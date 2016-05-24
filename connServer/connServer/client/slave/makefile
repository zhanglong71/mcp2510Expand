CROSS_COMPILE = arm-linux-
CC = $(CROSS_COMPILE)gcc
LD = $(CROSS_COMPILE)gcc

##CCFLAG = -c -Wall -DDEBUG
CCFLAG = -c -Wall
LDFLAG = -g -Wall
RM = rm -f

all:
	${CC} ${CCFLAG} slave.c -o slave.o
	${CC} ${CCFLAG} ../../server/parse.c -o parse.o
	${LD} ${LDFLAG} slave.o parse.o -o slave
clean:
	${RM} *.o slave
install:
	cp -r slave ~/tftpboot/
