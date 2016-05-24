
CROSS_COMPILE   = arm-linux-
CC = $(CROSS_COMPILE)gcc
CFLAG = -Wall -g
RM = rm -rf

# src := $(shell ls *.c)
# $(foreach n, $(src), $CC $(CFLAG) $(n))

all:
	$(CC) $(CFLAG) app_lockBy315M.c -o app_lockBy315M
	$(CC) $(CFLAG) app_lockBy315M-loop.c -o app_lockBy315M-loop
clean:
	$(RM) app_lockBy315M app_lockBy315M-loop

