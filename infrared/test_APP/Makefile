
CROSS_COMPILE   = arm-linux-
CC = $(CROSS_COMPILE)gcc
CFLAG = -Wall -g
RM = rm -rf

# src := $(shell ls *.c)
# $(foreach n, $(src), $CC $(CFLAG) $(n))

all:
	$(CC) $(CFLAG) app_infrared.c -o app_infrared
	$(CC) $(CFLAG) app_infrared-loop.c -o app_infrared-loop
clean:
	$(RM) app_infrared app_infrared-loop

