#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#if 1
#include "ptype.h"
#include "rbtree.h"
//#include "read.h"
#include "parse.h"
#endif

/*
 *receive data as recv port 
 */

int insertImsi(struct rb_root *root, struct clientNode_s *__clientNode, charsCmp_t cmp)
{
    struct rb_node **new = &(root->rb_node), *parent = NULL;

    /* Figure out where to put new node */
    while (*new) 
    {
        //struct mytype *this = container_of(*new, struct mytype, node);
        struct clientNode_s *clientNode = container_of(*new, struct clientNode_s, node);
        int result = cmp(__clientNode->clientImsi, clientNode->clientImsi);

        parent = *new;
        if (result < 0)
            new = &((*new)->rb_left);
        else if (result > 0)
            new = &((*new)->rb_right);
        else
            return 0;
    }

    /* Add new node and rebalance tree. */
    rb_link_node(&(__clientNode->node), parent, new);
    rb_insert_color(&(__clientNode->node), root);

    return 1;
}

int insertClientId(struct rb_root *root, struct clientNode_s *__clientNode, cmp_t cmp)
{
    struct rb_node **new = &(root->rb_node), *parent = NULL;

    /* Figure out where to put new node */
    while (*new) 
    {
        //struct mytype *this = container_of(*new, struct mytype, node);
        struct clientNode_s *clientNode = container_of(*new, struct clientNode_s, node);
        int result = cmp(__clientNode->clientId, clientNode->clientId);

        parent = *new;
        if (result < 0)
            new = &((*new)->rb_left);
        else if (result > 0)
            new = &((*new)->rb_right);
        else
            return 0;
    }

    /* Add new node and rebalance tree. */
    rb_link_node(&(__clientNode->node), parent, new);
    rb_insert_color(&(__clientNode->node), root);

    return 1;
}


struct clientNode_s *searchClientId(struct rb_root *root, unsigned  int __clientId, cmp_t cmp)
{
    struct rb_node *node = root->rb_node;
    while (node) 
    {
        struct clientNode_s *clientNode = container_of(node, struct clientNode_s, node);
        int result = cmp(__clientId, clientNode->clientId);
        if (result < 0)
            node = node->rb_left;
        else if (result > 0)
            node = node->rb_right;
        else
            return clientNode;
    }   
    return NULL;
}

struct clientNode_s *searchImsi(struct rb_root *root, char* __imsi, charsCmp_t cmp)
{
    struct rb_node *node = root->rb_node;
    while (node) 
    {
        struct clientNode_s *clientNode = container_of(node, struct clientNode_s, node);
        int result = cmp(__imsi, clientNode->clientImsi);
        if (result < 0)
            node = node->rb_left;
        else if (result > 0)
            node = node->rb_right;
        else
            return clientNode;
    }   
    return NULL;
}

unsigned int uicmp(const unsigned int __arg1, const unsigned int __arg2)
{
    return (__arg1 - __arg2);
}

#if 1
#if 1
int charsCmp(const char* __arg1, const char* __arg2)
{
    int i;
    for(i = 0; ((i < __arg1[0]) && (i < CIMSI_LEN)); i++)     /** Note: arg1[0] is the length of string **/
    {
        if(__arg1[i] != __arg2[i])
        {
            return  __arg1[i] - __arg2[i];
        }
    }
    return 0;
}
#else
unsigned int charsCmp(const char* __arg1, const char* __arg2, int __len)
{
    for(int i = 0; (i < CIMSI_LEN) && (i < __len); i++)
    {
        if(__arg1[i] != __arg2[i])
        {
            return  __arg1[i] - __arg2[i];
        }
    }
    return 0;
}
#endif

#else
unsigned int charsCmp(const char* __arg1, const char* __arg2)
{
    int i;

    if(__arg1[0] != __arg2[0])      /** 先比较长度 **/
    {
        return  __arg1[0] - __arg2[0];
    }
    
    for(i = 1; (i < CIMSI_LEN) && (i < __arg1[0]); i++)
    {
        if(__arg1[i] != __arg2[i])
        {
            return  __arg1[i] - __arg2[i];
        }
    }
    return 0;
}
#endif

int	main(int argc, char *argv[])
{
	//int i;
	int sock_fd;
	//int iLen = 0;
	struct sockaddr_in myaddr;
	struct sockaddr_in src_addr;

    struct clientNode_s *dstNode;
    struct clientNode_s *srcNode;

    char dstImsi[CIMSI_LEN];
    char srcImsi[CIMSI_LEN];
	char format[CFMAT_LEN];
    //unsigned int dstClientId = 0;
    //unsigned int srcClientId = 0;

    struct rb_root root = RB_ROOT;
	char buf[BUF_LEN];
	char value[BUF_LEN];
	int iStrLen = 0;
	int	recv_num;
	int	recvlen;

	if(2 > argc)
	{
		printf("argc\n");
		exit(1);
	}

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

    while(1)
    {
        fprintf(stderr, "[%s-%d]receiving ... \n", __FILE__, __LINE__);
        recvlen = sizeof(struct sockaddr_in);
        bzero(buf, BUF_LEN);
        recv_num = recvfrom(sock_fd, (char *)buf, sizeof(buf), 0, 
                (struct sockaddr *)&src_addr, (socklen_t *)&recvlen);  

        fprintf(stderr, "[%s-%d]received ! \n", __FILE__, __LINE__);
        if(recv_num != -1)
        {
            iStrLen = strlen(buf);
            iStrLen = iStrLen > recv_num? iStrLen:recv_num;

            memcpy(value, buf, iStrLen);
            parsePacketFormatId(value, format);

            memcpy(value, buf, iStrLen);
            parseSrcImsi(value, srcImsi);

            memcpy(value, buf, iStrLen);
            parseDstImsi(value, dstImsi);

            if((strcmp(format, "data-send") == 0) || (strcmp(format, "data-ACK") == 0))
            {
                fprintf(stderr, "[%s-%d]received data !\n", __FILE__, __LINE__);
                //dstNode = searchImsi(&root, dstImsi, charsCmp);
                dstNode = searchImsi(&root, dstImsi, strcmp);
                if(NULL != dstNode)
                {
                    /** received data and find the destination node, forward data **/
                    fprintf(stderr, "[%s-%d]ready to forward data !\n", __FILE__, __LINE__);
                    int ret = sendto(sock_fd, buf, BUF_LEN, 0, (struct sockaddr *)dstNode->ipAddr, sizeof(struct sockaddr_in));
                    if(-1 == ret)
                    {
                        fprintf(stderr, "[%s-%d]send data error !\n", __FILE__, __LINE__);
                        exit(2);
                    }
                    //fprintf(stderr, "[%s-%d]forward data from [%d] to [%d]!\n", __FILE__, __LINE__, srcClientId, dstClientId);
                    fprintf(stderr, "[%s-%d]forward data[%s] from [%s] to [%s-ipAddr-%s:%u]!\n", 
                            __FILE__, __LINE__, 
                            buf,  
                            srcImsi, 
                            dstImsi,
                            inet_ntoa(dstNode->ipAddr.sin_addr),
                            dstNode->ipAddr.sin_port);
                }
                else
                {
                    //fprintf(stderr, "[%s-%d]destination[%d] is not on line!\n", __FILE__, __LINE__, dstClientId);
                    fprintf(stderr, "[%s-%d]destination[%s] is not on line!\n", __FILE__, __LINE__, dstImsi);
                }
            }
            else if(strcmp(format, "login-request") == 0)       /** 登录请求，记录信息 **/
            {
                /** received login message **/
                //srcNode = searchImsi(&root, srcImsi, charsCmp);
                srcNode = searchImsi(&root, srcImsi, strcmp);
                if(NULL == srcNode)
                {
                    /** create new node **/
                    srcNode = malloc(sizeof(struct clientNode_s));
                    if(NULL != srcNode)
                    {
                        bzero(srcNode, sizeof(struct clientNode_s));
                        memcpy(srcNode->clientImsi, srcImsi, CIMSI_LEN);
                        memcpy(srcNode->ipAddr, &src_addr, sizeof(struct sockaddr));
                        srcNode->clientStatus = STATUS_LGINSUC;  /** 进入登录状态 **/

                        //insertImsi(&root, srcNode, charsCmp);
                        insertImsi(&root, srcNode, strcmp);

                        fprintf(stderr, "[%s-%d]Client Node[%s-ipAddr-%s:%u] created ok!\n", 
                                __FILE__, __LINE__, 
                                srcImsi, 
                                inet_ntoa(src_addr.sin_addr),
                                src_addr.sin_port);
                        //fprintf(stderr, "[%s-%d]Client Node[%d] created ok!\n", __FILE__, __LINE__, srcClientId);
                    }
                    else
                    {
                        //fprintf(stderr, "[%s-%d]malloc error. Client Node[%d] created failed!\n", __FILE__, __LINE__, srcClientId);
                        fprintf(stderr, "[%s-%d]malloc error. Client Node[%s-ipAddr-%s:%u] created failed!\n", 
                                __FILE__, __LINE__, 
                                srcImsi, 
                                inet_ntoa(src_addr.sin_addr),
                                src_addr.sin_port);
                    }
                }
                else
                {   
                    srcNode->clientStatus = STATUS_LGINSUC;  /** 进入登录状态 **/

                    memcpy(srcNode->clientImsi, srcImsi, CIMSI_LEN);
                    memcpy(srcNode->ipAddr, &src_addr, sizeof(struct sockaddr));
                    fprintf(stderr, "[%s-%d]Client[%s] login again !\n", __FILE__, __LINE__, srcImsi);
                }

                //setPacketFormat(buf, CFORMAT_LOGINOK);
                setPacketFormat(buf, "login-ok");
                int ret = sendto(sock_fd, buf, strlen(buf) + 1, 0, (struct sockaddr *)&src_addr, sizeof(struct sockaddr_in));
                if(-1 == ret)
                {
                    perror("sendto");
                    fprintf(stderr, "[%s-%d]sendto error !!!!!!\n", __FILE__, __LINE__ );
                    //exit(2);
                }
                else
                {
                    fprintf(stderr, "[%s-%d]sendto ok\n", __FILE__, __LINE__ );
                }
                fprintf(stderr, "[%s-%d]Client [%s: %s: %d] login \n", 
                        __FILE__, __LINE__, 
                        srcImsi, 
                        inet_ntoa(src_addr.sin_addr), 
                        src_addr.sin_port);
            }
            else if(strcmp(format, "login-out") == 0)
            {
                //fprintf(stderr, "[%s-%d]Client[%d] logout!\n", __FILE__, __LINE__, srcClientId);
                fprintf(stderr, "[%s-%d]Client[%s] logout!\n", __FILE__, __LINE__, srcImsi);
            }
            else
            {
                fprintf(stderr, "[%s-%d]buf[%s] is unrecognized!!!\n", __FILE__, __LINE__, buf);
            }
        }
    }
	close(sock_fd);

    return  0;
}
