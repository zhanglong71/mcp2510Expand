#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/in.h>
#include <unistd.h>
/*
send data as: send xx.xx.xx.xx port message
*/

int	main(int argc, char *argv[])
{
	int sock_fd;
	struct sockaddr_in to;
	char buf[1024];
    char *bp = buf;

	if(4 != argc)
	{
		printf("argc\n");
		exit(1);
	}

	if(-1 == (sock_fd = socket(PF_INET, SOCK_DGRAM, 0)))
	{
		printf("socket\n");
		exit(2);
	}

	to.sin_family = AF_INET;
	to.sin_port = htons(atoi(argv[2]));
	to.sin_addr.s_addr = inet_addr(argv[1]);
	
	strcpy(buf, argv[3]);
	int ret = sendto(sock_fd, buf, strlen(argv[3]) + 1, 0,
				(struct sockaddr *)&to, sizeof(struct sockaddr_in));
	if(-1 == ret)
	{
		printf("sendto");
		exit(2);
	}

	close(sock_fd);
}
