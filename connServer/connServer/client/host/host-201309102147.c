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
#include "../../server/parse.h"
//#include "../crc8.c"


/** 本客户端的ID **/
#define CCLIENT_ID  (102)
#define CBUFSIZE (1500)

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

	struct sockaddr_in server_addr;
	struct sockaddr_in src_addr;
	struct loginMessage_s loginMessage;
    union packet_un packet;
	int send_count;
	int recv_count;
    socklen_t socklen;
	int opt;		
	int ret;		
    struct timeval tv;
    //char buf[CBUFSIZE];

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
		        printf("eg: ./host -a ipAddr -p port -s srcClientId "
                          "(./host -a 192.168.1.112 -p 4852 -s 101)\n");
                break;
            case 'v':   /** version **/
                break;
            default:   /** help message **/
		        printf("eg: ./host -a ipAddr -p port -s srcClientId -d dstClientId -f function -k keyCode -h houseCode(./host -a 192.168.1.112 -p 4852 -s 101 -d 102 -h 15 -k 1 -f 5)\n");
                break;
        }
    }

	//if(-1 == (sock_fd = socket(AF_INET, SOCK_STREAM, 0)))
	if(-1 == (sock_fd = socket(AF_INET, SOCK_DGRAM, 0)))
	{
		perror("socket");
        iRet = 2;
        goto err0;
		//exit(2);
	}

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons((unsigned short)atoi(loginMessage.serverPort));
	server_addr.sin_addr.s_addr = inet_addr(loginMessage.serverIP);
    
    //bzero(buf, 1500);


#if 1
    while(1)    /** login **/
    {
        bzero(&packet, sizeof(packet));
        packet.packetData.formatId = CFORMAT_LGINREQ;
        packet.packetAuthen.dstClientId = 0;
        packet.packetAuthen.srcClientId = loginMessage.srcClientId;
        packet.packetAuthen.totalLen = sizeof(struct packetAuthentication_s);
        memcpy(packet.packetAuthen.srcImsi, "9876543210", CIMSI_LEN);
        memcpy(packet.packetAuthen.dstImsi, "1234567890", CIMSI_LEN);

        printf("[%s-%d]login ...\n", __FILE__, __LINE__);
        send_count = sendto(sock_fd, 
                &packet, sizeof(struct packetAuthentication_s),
                0,
                (struct sockaddr *)&server_addr, sizeof(struct sockaddr_in));
        if(send_count < 0) 
        {
            perror("sendto");
            printf("[%s-%d]login failed ...\n", __FILE__, __LINE__);
            continue;
        }

        fd_set read_set;
        FD_ZERO(&read_set);
        FD_SET(sock_fd, &read_set);
        tv.tv_sec = 1;
        tv.tv_usec = 0;

        ret = select(sock_fd + 1, &read_set, NULL, NULL, &tv);
        if(-1 == ret)
        {   
            printf("select\n");
            //goto    END4;
        }   
        else if(ret > 0)
        {   
            printf("[%s-%d]recviving login reply ...\n", __FILE__, __LINE__);
            recv_count  = recvfrom(sock_fd, 
                            (char *)&packet, sizeof(packet), 
                            0,  
			                (struct sockaddr *)&src_addr, 
                            &socklen);  
            if(-1 == recv_count)
            {   
                printf("recvfrom\n");
                exit(2);
            }   
            else
            {
                if(parsePacketFormat((char *)&packet) == CFORMAT_LOGINOK)
                {   
                    printf("[%s-%d]login ok! \n", __FILE__, __LINE__);
                    break;
                }
            }
        }   
    }
#endif

    do {    /** recvive data and reply **/
        bzero(&packet, sizeof(packet));
        packet.packetData.formatId = CFORMAT_DATA;
        packet.packetData.dstClientId = 0;
        packet.packetData.srcClientId = loginMessage.srcClientId;
        memcpy(packet.packetData.srcImsi, "9876543210", CIMSI_LEN);
        memcpy(packet.packetData.dstImsi, "1234567890", CIMSI_LEN);
        packet.packetData.totalLen = sizeof(struct packetAuthentication_s);

        send_count = sendto(sock_fd, 
                &packet, sizeof(struct packetAuthentication_s),
                0,
                (struct sockaddr *)&server_addr, sizeof(struct sockaddr_in));
        if(send_count < 0) 
        {
            perror("sendto");
            printf("[%s-%d]send data failed ...\n", __FILE__, __LINE__);
        }
        else
        {
            printf("[%s-%d]send data ok!\n", __FILE__, __LINE__);
        }

        fd_set read_set;
        FD_ZERO(&read_set);
        FD_SET(sock_fd, &read_set);
        tv.tv_sec = 1;
        tv.tv_usec = 0;        
        
        ret = select(sock_fd + 1, &read_set, NULL, NULL, &tv);
        if(-1 == ret)
        {   
            printf("select\n");
            continue;       /** 重来一次  **/
            //goto    END4;
        }   
        else if(ret > 0)
        {   
            printf("[%s-%d]receiving data ...\n", __FILE__, __LINE__);
            recv_count  = recvfrom(sock_fd, 
                    (char *)&packet, sizeof(packet), 
                    0,  
                    (struct sockaddr *)&src_addr, 
                    &socklen);  
            if(-1 == recv_count)
            {   
                printf("recvfrom\n");
                exit(2);
            }   
            else if(0 == recv_count)
            {
                printf("[%s-%d]received no data...\n", __FILE__, __LINE__);
                continue;
            }
            else    /** recv_count >= 1 **/
            {
                if(parsePacketFormat((char *)&packet) == CFORMAT_DATAACK)
                {   
                    printf("[%s-%d]received reply ok...\n", __FILE__, __LINE__);
                }   
                else
                {
                    continue;
                }
            }
        } 

    } while(0);     /** 命令了出端不必持续发数据  **/

	close(sock_fd);
err0:
	return iRet;
}

