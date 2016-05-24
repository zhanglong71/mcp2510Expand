#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "../../server/ptype.h"
#include "../crc8.c"


/** 本客户端的ID **/
#define CCLIENT_ID  (102)

typedef struct loginMessage_s {
    char* serverIP;
    char* serverPort;
    int dstClientId;
    int srcClientId;
} loginMessage_t;

int	main(int argc, char *argv[])
{
	int sock_fd;		
	int iRet = 0;		
#ifdef DEBUG
#else
	int stat;
#endif
	struct sockaddr_in server_addr;
	struct loginMessage_s loginMessage;
    union packet_un packet;
	int send_count;
	int recv_count;
	int opt;		
    char *cmd;

    loginMessage.serverIP = "192.168.1.112";    /** default server ip  **/
    loginMessage.serverPort = "4852";           /** defautl server port **/
    loginMessage.srcClientId = 102;              /** default client Id  **/

    while((opt = getopt(argc, argv,"a:p:s:v:H")) != -1)
    {
        switch(opt)
        {
            case 'a':   /** ip address **/
                loginMessage.serverIP = optarg;
                break;
            case 'p':   /** port address **/
                loginMessage.serverPort = optarg;           /** defautl server port **/
                break;
            case 's':   /** srcClientId address **/
                loginMessage.srcClientId = atoi(optarg);
                break;
            case 'H':   /** help message **/
		        printf("eg: ./slave -a ipAddr -p port -s srcClientId "
                          "(./slave -a 192.168.1.112 -p 4852 -s 101)\n");
                break;
            case 'v':   /** version **/
                break;
            default:   /** help message **/
		        printf("eg: ./host -a ipAddr -p port -s srcClientId -d dstClientId -f function -k keyCode -h houseCode(./host -a 192.168.1.112 -p 4852 -s 101 -d 102 -h 15 -k 1 -f 5)\n");
                break;
        }
    }

	if(-1 == (sock_fd = socket(AF_INET, SOCK_STREAM, 0)))
	{
		perror("socket");
        iRet = 2;
        goto err0;
		//exit(2);
	}

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons((unsigned short)atoi(loginMessage.serverPort));
	server_addr.sin_addr.s_addr = inet_addr(loginMessage.serverIP);

retry:
	if(-1 == connect(sock_fd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)))
    {
		perror("connect");
	    printf("[%s-%d]connect errno... %d!\n", __FILE__, __LINE__, errno);
        iRet++;
        if(iRet >= 3) {
            goto err1;
        } else {    //wait 31s then retry
            sleep(31);
            goto retry;
        }
		//exit(3);
    }
	printf("[%s-%d]connect ...\n", __FILE__, __LINE__);
    
    cmd = malloc(sizeof(cmd));
    if(NULL == cmd)
    {
	    printf("[%s-%d]malloc failed !\n", __FILE__, __LINE__);
        iRet = 4;
        goto err1;
    }
    bzero(cmd, sizeof(cmd));

    bzero(&packet, sizeof(packet));
    packet.packetData.formatId = CFORMAT_LOGINOK;
    packet.packetAuthen.dstClientId = 0;
    packet.packetAuthen.srcClientId = loginMessage.srcClientId;
    packet.packetAuthen.totalLen = sizeof(struct packetAuthentication_s);
	send_count = send(sock_fd, 
                &packet, 
                sizeof(struct packetAuthentication_s),
                0);
    if(send_count < 0) 
	{
		perror("send");
	    printf("[%s-%d]login failed ...\n", __FILE__, __LINE__);
	}

	while(1) 
	{
	    printf("receiving ...\n");
		bzero(&packet, sizeof(packet));
		recv_count = recv(sock_fd, &packet, sizeof(packet), 0);
		if(recv_count > 0) 
        {
            int i;
            int iLen = recv_count - (3 * sizeof(unsigned int) + sizeof(enum format_s));
            if(makeCrc8(0, packet.packetData.buf, iLen - 1) != packet.packetData.buf[iLen - 1])
            {
                /** check CRC again **/
                if(makeCrc8(0, packet.packetData.buf, 15) != packet.packetData.buf[15])
                {
	                printf("[%s-%d]CRC error ... giveup the data\n", __FILE__, __LINE__);
                    continue;
                }
            }
	        printf("[%s-%d]received %d bytes data !\n", __FILE__, __LINE__, iLen);

            for(i = 0; i < iLen - 1/** giveup the CRC **/; i++)
            {
                sprintf(cmd + i * 2 , "%02x", packet.packetData.buf[i]);
            }
            cmd[i * 2] = 0;         /** end **/
#ifdef DEBUG
            printf("%s\n", cmd);
#else
            pid_t pid = fork();
            if(pid == -1)
            {
                continue;
            }
            else if(pid == 0)   /** child **/
            {
                /** ./app_raw_write /dev/mcp2510 010f000000000000000042010500 e **/
                    //char *arglist[] = {"./app_raw_write", "/dev/mcp2510", cmd, "e", NULL};
                    char *arglist[] = {"./app_raw_write", "/dev/mcp2510", cmd, NULL};
                    //char *arglist[] = {"./app_raw_write", "/dev/mcp2510", cmd, "2", NULL};
                execvp("./app_raw_write", arglist);
                printf("process never go to here!\n");
                _exit(0);	
            }
            else    /** parent **/
            {
	            pid = wait(&stat);
                if(WIFEXITED(stat))
                    printf("child exited with code %d\n\n",WEXITSTATUS(stat));
                else
                    printf("child exit abnormally\n\n");
            }
#endif
        }
	}

err1:
	close(sock_fd);
err0:
	return iRet;
}

