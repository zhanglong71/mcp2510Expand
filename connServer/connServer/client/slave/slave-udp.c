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

#define SRCIMSI "22345678901"


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

	struct sockaddr_in server_addr;
	struct sockaddr_in src_addr;
	struct loginMessage_s loginMessage;
    char dstImsi[CIMSI_LEN];
    char srcImsi[CIMSI_LEN];
    //union packet_un packet;
	int send_count;
	int recv_count;
    char buf[BUF_LEN];
    char value[BUF_LEN];
	char format[CFMAT_LEN];
    socklen_t socklen;
	int opt;		
	int ret;		
    struct timeval tv;

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

	if(-1 == (sock_fd = socket(AF_INET, SOCK_DGRAM, 0)))
	{
		perror("socket");
        iRet = 2;
        goto err0;
	}

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons((unsigned short)atoi(loginMessage.serverPort));
	server_addr.sin_addr.s_addr = inet_addr(loginMessage.serverIP);

    fd_set read_set;
#if 1
    while(1)    /** login **/
    {
    #if 0
        bzero(&packet, sizeof(packet));
        packet.packetData.formatId = CFORMAT_LGINREQ;
        packet.packetAuthen.dstClientId = 0;
        packet.packetAuthen.srcClientId = loginMessage.srcClientId;
        packet.packetAuthen.totalLen = sizeof(struct packetAuthentication_s);
        memcpy(packet.packetAuthen.srcImsi, "1234567890", CIMSI_LEN);
        memcpy(packet.packetAuthen.dstImsi, "0", CIMSI_LEN);
    #else
        bzero(buf, BUF_LEN);
        setPacketFormat(buf, "login-request");
        setSrcImsi(buf, SRCIMSI);
    #endif
        FD_ZERO(&read_set);
        FD_SET(sock_fd, &read_set);
        tv.tv_sec = 3;
        tv.tv_usec = 0;

        printf("[%s-%d]login ...\n", __FILE__, __LINE__);
        send_count = sendto(sock_fd, 
                //&packet, 
                buf, 
                //sizeof(struct packetAuthentication_s),
                BUF_LEN,
                0,
                (struct sockaddr *)&server_addr, sizeof(struct sockaddr_in));
        if(send_count < 0) 
        {
            perror("sendto");
            printf("[%s-%d]login failed ...\n", __FILE__, __LINE__);
        }
        else
        {
            printf("[%s-%d]login command send ok !\n", __FILE__, __LINE__);
        }

        ret = select(sock_fd + 1, &read_set, NULL, NULL, &tv);
        if(-1 == ret)
        {   
            printf("select\n");
            //goto    END4;
        }   
        else if(ret > 0)
        {   
            printf("[%s-%d]received reply! paresd the reply!\n", __FILE__, __LINE__);
            recv_count  = recvfrom(sock_fd, 
                            //(char *)&packet, 
                            buf, 
                            //sizeof(packet), 
                            BUF_LEN, 
                            0,  
			                (struct sockaddr *)&src_addr, 
                            &socklen);  
            if(-1 == recv_count)
            {   
                printf("recvfrom\n");
                exit(2);
            }   

            memcpy(value, buf, recv_count);
            parsePacketFormatId(value, format);

            if(strcmp(format, "login-ok") == 0)
            {   
                printf("[%s-%d]paresd the reply over, login ok!\n", __FILE__, __LINE__);
                break;
            }   
        }   
    }
#endif
	while(1)    /** recvive data and reply **/
    {

        FD_ZERO(&read_set);
        FD_SET(sock_fd, &read_set);
        tv.tv_sec = 60;
        tv.tv_usec = 0;

        printf("[%s-%d]waiting command ...\n", __FILE__, __LINE__);
        ret = select(sock_fd + 1, &read_set, NULL, NULL, &tv);
        if(-1 == ret)
        {   
            printf("[%s-%d]select error! \n", __FILE__, __LINE__);
            //goto    END4;
        }
        else if(ret == 0)
        {
            printf("[%s-%d]select time out! \n", __FILE__, __LINE__);
        }
        else
        {   
            printf("[%s-%d]recvived data ...\n", __FILE__, __LINE__);
            recv_count  = recvfrom(sock_fd, 
                    //(char *)&packet, 
                    buf, 
                    //sizeof(packet), 
                    BUF_LEN, 
                    0,  
                    (struct sockaddr *)&src_addr, 
                    &socklen);  
            if(-1 == recv_count)
            {   
                printf("recvfrom\n");
                exit(2);
            }   

            memcpy(value, buf, recv_count);
            parsePacketFormatId(value, format);
            //if(parsePacketFormat((char *)&packet) == CFORMAT_DATA)
            if(strcmp(format, "data-send") == 0)
            {   
                /** 报文格式 **/
                //setPacketFormat((char *)&packet ,CFORMAT_DATAACK);

                /** 交换src/dst IMSI  **/
                //memcpy(srcImsi, parseSrcImsi((char *)&packet), CIMSI_LEN);
                memcpy(value, buf, recv_count);
                parseSrcImsi(value, srcImsi);

                //memcpy(dstImsi, parseDstImsi((char *)&packet), CIMSI_LEN);
                memcpy(value, buf, recv_count);
                parseDstImsi(value, dstImsi);

                printf("[%s-%d]srcImsi: %s\n", __FILE__, __LINE__, srcImsi);
                printf("[%s-%d]dstImsi: %s\n", __FILE__, __LINE__, dstImsi);

                setPacketFormat(buf, "data-ACK");
                setSrcImsi(buf, dstImsi);
                setDstImsi(buf, srcImsi);

                printf("[%s-%d]reply ...\n", __FILE__, __LINE__);
                send_count = sendto(sock_fd, 
                        buf, 
                        BUF_LEN,
                        0,
                        (struct sockaddr *)&server_addr, sizeof(struct sockaddr_in));
                if(send_count < 0) 
                {
                    perror("sendto");
                    printf("[%s-%d]send reply data failed ...\n", __FILE__, __LINE__);
                }
                else
                {
                    printf("[%s-%d]reply over...\n", __FILE__, __LINE__);
                }
            }   
        } 

    }

	close(sock_fd);
err0:
	return iRet;
}

