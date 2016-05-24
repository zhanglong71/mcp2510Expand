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
#include "../crc8.c"

typedef struct loginMessage_s {
    char* serverIP;
    char* serverPort;
    //int dstClientId;
    //int srcClientId;
} loginMessage_t;

/**
 * send data as "host srcClientId ipaddr port"
 **/
int	main(int argc, char *argv[])
{
	int sock_fd;		
	int opt;		
    int ret;
    char *pCmdBuf;
    char *pRespBuf;
	char format[CFMAT_LEN];
    char dstImsi[CIMSI_LEN];
    char srcImsi[CIMSI_LEN];

    char user[CUSER_LEN];
    char house[CHOUSE_LEN];
    char key[CKEY_LEN];
    char function[CFUNCTION_LEN];
    char devType[CDEVTYPE_LEN];

	int stat;
	struct sockaddr_in server_addr;
	struct loginMessage_s loginMessage;
    socklen_t socklen;
	int send_count;
	int recv_count;
    struct timeval tv;

    loginMessage.serverIP = "192.168.1.112";    /** default server ip  **/
    loginMessage.serverPort = "4852";           /** defautl server port **/

    pRespBuf = malloc(HRM_MAX_PACKET_LEN);
    if(NULL == pRespBuf)
    {
        printf("[%s-%d]: malloc failed! \n", __FILE__, __LINE__);
        goto err1;
    }

    pCmdBuf = malloc(HRM_MAX_PACKET_LEN);
    if(NULL == pCmdBuf)
    {
        printf("[%s-%d]: malloc failed! \n", __FILE__, __LINE__);
        goto err2;
    }
    pCmdBuf[0] = '\0';
    setPacketFormat(pCmdBuf, "login-request");
    setSrcImsi(pCmdBuf, "102");

    if(argc == 1) 
    {
    }

    while((opt = getopt(argc, argv,"a:d:f:k:h:p:s:t:u:vVH")) != -1)
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
		        printf("eg: ./slave -a ipAddr -p port -s srcClientId"
                        "(./slave -a 192.168.1.112 -p 4852 -s 102)\n");
		        printf("version: [%s-%s]\n", __DATE__, __TIME__);
                return  0;
                //break;
            case 'v':   /** version **/
            case 'V':   /** version **/
                printf("version [%s-%s]", __DATE__, __TIME__);
                break;
            default:   /** help message **/
		        printf("eg: ./slave -a ipAddr -p port -s srcClientId -d dstClientId -f function -k keyCode -h houseCode(./slave -a 192.168.1.112 -p 4852 -s 102)\n");
                break;
        }
    }
    if(-1 == (sock_fd = socket(PF_INET, SOCK_DGRAM, 0)))
	{
		perror("socket");
        goto err3;
	}
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons((unsigned short)atoi(loginMessage.serverPort));
	server_addr.sin_addr.s_addr = inet_addr(loginMessage.serverIP);

    opt = strlen(pCmdBuf);
    fd_set read_set;
    /** login **/
    while(1)
    {
        FD_ZERO(&read_set);
        FD_SET(sock_fd, &read_set);
        tv.tv_sec = 13;
        tv.tv_usec = 0;

        send_count = sendto(sock_fd, pCmdBuf, opt/** length **/, 0, (struct sockaddr *)&server_addr, sizeof(struct sockaddr_in));  
        if(send_count < 0) 
        {
            perror("send");
            printf("[%s-%d]send error ...\n", __FILE__, __LINE__);
            continue;
        }
        else
        {
            printf("[%s-%d]login command[---%s---] send ok! \n", __FILE__, __LINE__, pCmdBuf);
        }

        ret = select(sock_fd + 1, &read_set, NULL, NULL, &tv);
        if(-1 == ret)
        {   
            perror("select");
            printf("[%s-%d]select! \n", __FILE__, __LINE__);
        }   
        else if(ret == 0)
        {
            printf("[%s-%d]select timeout! \n", __FILE__, __LINE__);
        }
        else if(ret > 0)
        {   
            printf("[%s-%d]receiving reply! \n", __FILE__, __LINE__);
            recv_count  = recvfrom(sock_fd, 
                            pRespBuf, 
                            BUF_LEN, 
                            0,  
			                (struct sockaddr *)&server_addr, 
                            &socklen);  
                            //sizeof(struct sockaddr_in));  
            if(recv_count < 0)
            {   
                perror("recvfrom");
                printf("[%s-%d]recvfrom error !\n", __FILE__, __LINE__);
                exit(2);
                //sleep(3);
                //continue;
            } 
            else if(recv_count == 0)
            {
                printf("[%s-%d]the peer orderly shutdown!\n", __FILE__, __LINE__);
            }
            else
            {
                printf("[%s-%d]paresing the reply!\n", __FILE__, __LINE__);
                parsePacketFormatId(pRespBuf, format);

                //printf(pRespBuf);
                if(strcmp(format, "login-ok") == 0)
                {   
                    printf("[%s-%d]paresd the reply over, login ok!\n", __FILE__, __LINE__);
                    break;
                }
            }
        } 
    }

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
        }
        else if(ret == 0)
        {
            printf("[%s-%d]select time out! \n", __FILE__, __LINE__);
        }
        else
        {   
            printf("[%s-%d]recvived data ...\n", __FILE__, __LINE__);
            recv_count  = recvfrom(sock_fd, 
                    pRespBuf, 
                    BUF_LEN, 
                    0,  
                    (struct sockaddr *)&server_addr, 
                    &socklen);  
            if(-1 == recv_count)
            {
                printf("recvfrom\n");
                exit(2);
            }
            else if(recv_count == 0)
            {
                printf("[%s-%d]the peer orderly shutdown!\n", __FILE__, __LINE__);
            }
            else /** recv_count > 0  **/
            {

                memcpy(pCmdBuf, pRespBuf, recv_count);
                parsePacketFormatId(pCmdBuf, format);
                //if(strcmp(pCmdBuf, "data-send") == 0)
                if(strcmp(format, "data-send") == 0)
                {   
                    #if 0
                    if(makeCrc8(0, (unsigned char *)pRespBuf, recv_count) != 0)
                    {
                        printf("[%s-%d]checksum[%d] = %x\n", __FILE__, __LINE__, recv_count - 1, pRespBuf[recv_count - 1]);
                        continue;
                    }
                    #endif
                    memcpy(pCmdBuf, pRespBuf, recv_count);
                    parseSrcImsi(pCmdBuf, srcImsi);
                    printf("[%s-%d]srcImsi: %s\n", __FILE__, __LINE__, srcImsi);
                    memcpy(pCmdBuf, pRespBuf, recv_count);
                    parseDstImsi(pCmdBuf, dstImsi);
                    printf("[%s-%d]dstImsi: %s\n", __FILE__, __LINE__, dstImsi);

                    //memcpy(pCmdBuf, pRespBuf, recv_count);
                    //parseSrcUser(pCmdBuf, user);
                    memcpy(pCmdBuf, pRespBuf, recv_count);
                    parseDstUser(pCmdBuf, user);
                    printf("[%s-%d]DstUser: %s(%02x)\n", __FILE__, __LINE__, user, atoi(user));

                    memcpy(pCmdBuf, pRespBuf, recv_count);
                    parseDstHouse(pCmdBuf, house);
                    printf("[%s-%d]DstHouse: %s(%02x)\n", __FILE__, __LINE__, house, atoi(house));

                    memcpy(pCmdBuf, pRespBuf, recv_count);
                    parseDstKey(pCmdBuf, key);
                    printf("[%s-%d]DstKey: %s(%02x)\n", __FILE__, __LINE__, key, atoi(key));

                    memcpy(pCmdBuf, pRespBuf, recv_count);
                    parseFunction(pCmdBuf, function);
                    printf("[%s-%d]function: %s(%02x)\n", __FILE__, __LINE__, function, atoi(function));

                    memcpy(pCmdBuf, pRespBuf, recv_count);
                    parseDstDevType(pCmdBuf, devType);
                    printf("[%s-%d]DstDevType: %s(%02x)\n", __FILE__, __LINE__, devType, atoi(devType));

                    /** reply **/
                    memcpy(pCmdBuf, pRespBuf, recv_count);
                    setPacketFormat(pCmdBuf, "data-ACK");
                    setSrcImsi(pCmdBuf, dstImsi);
                    setDstImsi(pCmdBuf, srcImsi);

                    printf("[%s-%d]sending reply ...\n", __FILE__, __LINE__);
                    send_count = sendto(sock_fd, 
                        pCmdBuf, 
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
                    /** execute the command **/
                    //memset(pCmdBuf, 0, HRM_MAX_PACKET_LEN);
#if 0               
                    /** destination  **/
                    ret = atoi(key);
                    sprintf(pRespBuf + 0, "%d", (char)((ret >> 4) && 0xf));
                    sprintf(pRespBuf + 1, "%d", (char)(ret && 0xf));

                    ret = atoi(house);
                    sprintf(pRespBuf + 2, "%d", (char)((ret >> 4) && 0xf));
                    sprintf(pRespBuf + 3, "%d", (char)(ret && 0xf));

                    ret = atoi(devType);
                    sprintf(pRespBuf + 4, "%d", (char)((ret >> 4) && 0xf));
                    sprintf(pRespBuf + 5, "%d", (char)(ret && 0xf));

                    ret = 0;
                    sprintf(pRespBuf + 6, "%d", (char)((ret >> 4) && 0xf));
                    sprintf(pRespBuf + 7, "%d", (char)(ret && 0xf));
                    /** source **/
                    ret = 0;
                    sprintf(pRespBuf + 8, "%d", (char)((ret >> 4) && 0xf));
                    sprintf(pRespBuf + 9, "%d", (char)(ret && 0xf));

                    ret = 0;
                    sprintf(pRespBuf + 10, "%d", (char)((ret >> 4) && 0xf));
                    sprintf(pRespBuf + 11, "%d", (char)(ret && 0xf));

                    ret = 0;
                    sprintf(pRespBuf + 12, "%d", (char)((ret >> 4) && 0xf));
                    sprintf(pRespBuf + 13, "%d", (char)(ret && 0xf));

                    ret = 0;
                    sprintf(pRespBuf + 14, "%d", (char)((ret >> 4) && 0xf));
                    sprintf(pRespBuf + 15, "%d", (char)(ret && 0xf));
                    /** reserved **/
                    ret = 0;
                    sprintf(pRespBuf + 16, "%d", (char)((ret >> 4) && 0xf));
                    sprintf(pRespBuf + 17, "%d", (char)(ret && 0xf));

                    ret = 0;
                    sprintf(pRespBuf + 18, "%d", (char)((ret >> 4) && 0xf));
                    sprintf(pRespBuf + 19, "%d", (char)(ret && 0xf));
                    /** command action **/
                    ret = 0x42;
                    sprintf(pRespBuf + 20, "%d", (char)((ret >> 4) && 0xf));
                    sprintf(pRespBuf + 21, "%d", (char)(ret && 0xf));

                    ret = 0x01;
                    sprintf(pRespBuf + 22, "%d", (char)((ret >> 4) && 0xf));
                    sprintf(pRespBuf + 23, "%d", (char)(ret && 0xf));
                    /** command value **/
                    ret = atoi(function);
                    sprintf(pRespBuf + 24, "%d", (char)((ret >> 12) && 0xf));
                    sprintf(pRespBuf + 25, "%d", (char)((ret >> 8) && 0xf));
                    sprintf(pRespBuf + 26, "%d", (char)((ret >> 4) && 0xf));
                    sprintf(pRespBuf + 27, "%d", (char)(ret && 0xf));
                    /** reserved **/
                    ret = 0;
                    sprintf(pRespBuf + 28, "%d", (char)((ret >> 4) && 0xf));
                    sprintf(pRespBuf + 29, "%d", (char)(ret && 0xf));
#else
                    pRespBuf[0] = (char)atoi(key);
                    pRespBuf[1] = (char)atoi(house);
                    pRespBuf[2] = (char)atoi(devType);
                    pRespBuf[3] = 0;

                    pRespBuf[4] = 0;
                    pRespBuf[5] = 0;
                    pRespBuf[6] = 0;
                    pRespBuf[7] = 0;

                    pRespBuf[8] = 0;
                    pRespBuf[9] = 0;

                    pRespBuf[10] = 0x42;
                    pRespBuf[11] = 0x01;

                    pRespBuf[12] = (char)(atoi(function) & 0xff);
                    pRespBuf[13] = (char)((atoi(function) >> 8) & 0xff);

                    pRespBuf[14] = 0;
                    pRespBuf[15] = 0;

                    for(ret = 0; ret < 16; ret++)
                    {
                        sprintf(pCmdBuf + ret * 2, "%02x", pRespBuf[ret]);
                    }
                    pCmdBuf[ret * 2] = '\0';
#endif
                    printf("---command-start:\n");
                    printf("%s\n", pCmdBuf);
                    printf("---command-over\n");

#if 1
                    pid_t pid = fork();
                    if(pid == -1) 
                    {
                        continue;
                    }
                    else if(pid == 0)   /** child **/
                    {
                        /** ./app_raw_write /dev/mcp2510 010f020000000000000042010500 **/
                        char *arglist[] = {"./app_raw_write", "/dev/mcp2510", pCmdBuf, NULL};
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
                else
                {
                    printf("[%s-%d]unrecognized format\n!!!!!!!!!!!!!\n%s!!!!!!!!!!!!!!!!!!!!!!!\n", __FILE__, __LINE__, pRespBuf);
                }
            }
        } 

    }

	close(sock_fd);
	return 0;
err3:
    free(pCmdBuf);
err2:
    free(pRespBuf);
err1:
    return  -1;
}

