#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "../../server/ptype.h"
#include "../../server/parse.h"
#include "../crc8.c"

typedef struct loginMessage_s {
    char* serverIP;
    char* serverPort;
    //int dstClientId;
    //int srcClientId;
} loginMessage_t;

typedef struct command_s{
    int user;
    int house;
    int key;
    int function;
    int devType;
} command_t;


/**
 * send data as "host srcClientId ipaddr port"
 **/
int	main(int argc, char *argv[])
{
	int sock_fd;		
	int opt;		
    char *pBuf;
    char *pCmdBuf;
	struct sockaddr_in server_addr;
	struct loginMessage_s loginMessage;
	//struct command_s cmd;
    //union packet_un packet;
	int send_count;

    loginMessage.serverIP = "192.168.1.112";    /** default server ip  **/
    loginMessage.serverPort = "4852";           /** defautl server port **/
    //loginMessage.srcClientId = 101;              /** default client Id  **/
    //loginMessage.dstClientId = 102;              /** default client Id  **/

#if 0
    cmd.user = 0;       /** default user **/
    cmd.house = 15;     /** default house code **/
    cmd.key = 1;        /** default key code **/
    cmd.function = 15;  /** default function **/
    cmd.devType = 2;    /** default type **/
#endif
    pBuf = malloc(HRM_MAX_PACKET_LEN);
    if(NULL == pBuf)
    {
        printf("[%s-%d]: malloc failed! \n", __FILE__, __LINE__);
        return -1;
    }

    pCmdBuf = malloc(HRM_MAX_PACKET_LEN);
    if(NULL == pCmdBuf)
    {
        printf("[%s-%d]: malloc failed! \n", __FILE__, __LINE__);
        return -1;
    }
    pCmdBuf[0] = '\0';
#if 1
    setPacketFormat(pCmdBuf, "login-request");
    setDstImsi(pCmdBuf, "102");
    setDstDevType(pCmdBuf, "2");
    setDstUser(pCmdBuf, "0");
    setDstHouse(pCmdBuf, "15");
    setDstKey(pCmdBuf, "15");
    setFunction(pCmdBuf, "15");

    setSrcImsi(pCmdBuf, "101");

#else
    sprintf(pCmdBuf, "packet-format:login-request\n");
    strcat(pCmdBuf, "dstClientId:102\n");
    strcat(pCmdBuf, "dstUserCode:0\n");
    strcat(pCmdBuf, "dstHouseCode:15\n");
    strcat(pCmdBuf, "dstKeyCode:1\n");
    strcat(pCmdBuf, "dstFunctionCode:15\n");
    strcat(pCmdBuf, "dstDeviceType:2\n");
    strcat(pCmdBuf, "srcClientId:101\n");
#endif

    while((opt = getopt(argc, argv,"a:d:f:k:h:p:s:t:u:vH")) != -1)
    {
        switch(opt)
        {
            case 'a':   /** ip address **/
                loginMessage.serverIP = optarg;
                break;
            case 'p':   /** port address **/
                loginMessage.serverPort = optarg;           /** defautl server port **/
                break;

            case 'd':   /** dstClientId address **/
                setDstImsi(pCmdBuf, optarg);
                break;
            case 's':   /** srcClientId address **/
                setSrcImsi(pCmdBuf, optarg);
                break;

            case 'f':   /** function **/
                setFunction(pCmdBuf, optarg);
                break;
            case 'k':   /** key code **/
                setDstKey(pCmdBuf, optarg);
                break;
            case 'h':   /** house code **/
                setDstHouse(pCmdBuf, optarg);
                break;
            case 't':   /** devType **/
                setDstDevType(pCmdBuf, optarg);
                break;
            case 'u':   /** userCode **/
                setDstUser(pCmdBuf, optarg);
                break;

            case 'H':   /** help message **/
		        printf("eg: ./host -a ipAddr -p port -s srcClientId -d dstClientId"
                        "-f function -k keyCode -h houseCode"
                        "(./host -a 192.168.1.6 -p 4852 -s 101 -d 102 -u 0 -h 15 -k 1 -t 2 -f 5)\n");
                return  0;
                //break;
            case 'v':   /** version **/
                printf("version[%s-%s]", __DATE__, __TIME__);
                break;
            default:   /** help message **/
		        printf("eg: ./host -a ipAddr -p port -s srcClientId -d dstClientId -f function -k keyCode -h houseCode(./host -a 192.168.1.6 -p 4852 -s 101 -d 102 -u 0 -h 15 -k 1 -t 2 -f 5)\n");
                break;
        }
    }

    if(-1 == (sock_fd = socket(PF_INET, SOCK_DGRAM, 0)))
	{
		perror("socket");
		exit(2);
	}

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons((unsigned short)atoi(loginMessage.serverPort));
	server_addr.sin_addr.s_addr = inet_addr(loginMessage.serverIP);

    opt = strlen(pCmdBuf);
	send_count = sendto(sock_fd, pCmdBuf, opt/** length **/, 0, (struct sockaddr *)&server_addr, sizeof(struct sockaddr_in));  
    if(send_count < 0) 
	{
		perror("send login message");
	    printf("[%s-%d]send error ...\n", __FILE__, __LINE__);
	}
    else
    {
		printf("[%s-%d]%d bytes login packet had been sent !!!\n"
                "---\n"
                "%s\n"
                "---\n"
                , __FILE__, __LINE__, send_count, pCmdBuf);
    }

    setPacketFormat(pCmdBuf, "data-send");
    opt = strlen(pCmdBuf);
#if 0
    pCmdBuf[opt] = (unsigned char)makeCrc8(0, (unsigned char *)pCmdBuf, opt);
    opt = strlen(pCmdBuf);
#endif
	send_count = sendto(sock_fd, pCmdBuf, opt/** length **/, 0, (struct sockaddr *)&server_addr, sizeof(struct sockaddr_in));  
	if(send_count > 0) 
	{
		printf("[%s-%d]%d bytes data had been sent !!!\n", __FILE__, __LINE__, send_count);
	}
    else
    {
	    perror("send");
		printf("[%s-%d]send error: [%d] !!!\n", __FILE__, __LINE__, errno);
    }

    setPacketFormat(pCmdBuf, "logout");
    opt = strlen(pCmdBuf);
	send_count = sendto(sock_fd, pCmdBuf, opt/** length **/, 0, (struct sockaddr *)&server_addr, sizeof(struct sockaddr_in));  
    if(send_count < 0) 
    {
		printf("[%s-%d]send error: [%d] !!!\n", __FILE__, __LINE__, errno);
    }

    printf(" ---\n%s ---\n", pCmdBuf);
	close(sock_fd);
	return 0;
}

