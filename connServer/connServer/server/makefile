
# CROSS_COMPILE = arm-linux-
CC = ${CROSS_COMPILE}gcc
LD = ${CROSS_COMPILE}gcc

RM = rm -f
CCFLAG = -c -g -Wall
LDFLAG = -g -Wall

all:
	${CC} ${CCFLAG} start.c -o start.o
	${CC} ${CCFLAG} parse.c -o parse.o
	${CC} ${CCFLAG} rbtree.c -o rbtree.o
	${LD} ${LDFLAG} start.o parse.o rbtree.o -o connServer
clean:
	${RM} *.o connServer

