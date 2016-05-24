CC = gcc
LD = gcc
CCFLAG = -c -Wall
LDFLAG = -g -Wall

all:
	${CC} ${CCFLAG} host.c -o host.o
	${CC} ${CCFLAG} ../../server/parse.c -o parse.o
	${LD} ${LDFLAG} host.o parse.o -o host
clean:
	rm -f *.o host

