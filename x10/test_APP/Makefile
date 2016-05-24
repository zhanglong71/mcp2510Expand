
CROSS_COMPILE   = arm-linux-
CC = $(CROSS_COMPILE)gcc
CFLAG = -Wall -g
RM = rm -rf

# src := $(shell ls *.c)
# $(foreach n, $(src), $CC $(CFLAG) $(n))

all:
	$(CC) $(CFLAG) app_x10.c -o app_x10
	$(CC) $(CFLAG) app_x10-loop.c -o app_x10-loop
clean:
	$(RM) app_x10 app_x10-loop
