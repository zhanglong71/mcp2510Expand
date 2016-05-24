#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/in.h>
#include <unistd.h>
/*
 *receive data as recv port 
 */

int	main(int argc, char *argv[])
{
	int sock_fd;
	struct sockaddr_in myaddr;
	struct sockaddr_in src_addr;

	char buf[1024];
	int	recv_num;
	int	recvlen;
/*
	if(3 != argc)
	{
		printf("argc\n");
		exit(1);
	}
*/
	if(-1 == (sock_fd = socket(PF_INET, SOCK_DGRAM, 0)))
	{
		printf("socket\n");
		exit(2);
	}

	myaddr.sin_family = AF_INET;
	myaddr.sin_port = htons(atoi(argv[1]));
	myaddr.sin_addr.s_addr = htonl(INADDR_ANY);

	if(-1 == bind(sock_fd, (struct sockaddr *)&myaddr, sizeof(struct sockaddr_in)))
	{
		perror("bind");
		exit(1);
	}
	recvlen = sizeof(struct sockaddr_in);
	recv_num = recvfrom(sock_fd, (char *)buf, sizeof(buf), 0, 
					(struct sockaddr *)&src_addr, &recvlen);  

	printf("%s\n", buf);
#if 1
	src_addr.sin_family = AF_INET;
	src_addr.sin_port = htons(atoi(argv[2]));
	src_addr.sin_addr.s_addr = inet_addr(argv[1]);
	
	int ret = sendto(sock_fd, buf, strlen(buf) + 1, 0,
				(struct sockaddr *)&src_addr, sizeof(struct sockaddr_in));
	if(-1 == ret)
	{
		printf("sendto");
		exit(2);
	}
#endif
	close(sock_fd);
}

