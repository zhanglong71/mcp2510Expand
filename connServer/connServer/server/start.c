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

#if 0
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
#endif

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

/** 中序遍历二叉树 **/
void traverseBinaryTree(struct rb_node *__node)
{
    struct rb_node *node = __node;
    struct clientNode_s *clientNode = NULL ;

    if(node != NULL)
    {
        traverseBinaryTree(node->rb_left);
        /*********************************/
        clientNode = container_of(node, struct clientNode_s, node);
        printf("[%s-%s:%u]\n", 
                clientNode->clientImsi, 
                //inet_ntoa(((struct sockaddr_in *)(clientNode->ipAddr)->sin_addr),
                inet_ntoa(((struct sockaddr_in *)(clientNode->ipAddr))->sin_addr),
                ((struct sockaddr_in *)(clientNode->ipAddr))->sin_port);
        /*********************************/
        traverseBinaryTree(node->rb_right);
    }
}

#if 1
#define printNode(clientNode)    do{    \
    if(clientNode != NULL)  \
    {   \
        printf("[%s-%d]-[%s-%s:%u]\n", \
                __FILE__, __LINE__,     \
                clientNode->clientImsi, \
                inet_ntoa(((struct sockaddr_in *)(clientNode->ipAddr))->sin_addr),  \
                ((struct sockaddr_in *)(clientNode->ipAddr))->sin_port);    \
    }   \
    else    \
    {   \
        printf("null\n");   \
    }   \
}while(0);
#else
void printNode(struct clientNode_s *clientNode)
{
    if(clientNode != NULL)
    {
        printf("[%s-%s:%u]\n", 
                clientNode->clientImsi, 
                inet_ntoa(((struct sockaddr_in *)(clientNode->ipAddr))->sin_addr),
                ((struct sockaddr_in *)(clientNode->ipAddr))->sin_port);
    }
    else
    {
        printf("null\n");
    }

    printf("[sizeof(struct sockaddr) = %lu  sizeof(struct sockaddr_in) = %lu]\n", sizeof(struct sockaddr), sizeof(struct sockaddr_in));
}
#endif

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

int	main(int argc, char *argv[])
{
	//int i;
	int sock_fd;
	//int iLen = 0;
	struct sockaddr_in myaddr;
	struct sockaddr_in src_addr;
	struct sockaddr_in dst_addr;

    struct clientNode_s *dstNode;
    struct clientNode_s *srcNode;

    char dstImsi[CIMSI_LEN];
    char srcImsi[CIMSI_LEN];
	char format[CFMAT_LEN];

	char srcIp[16];
	char dstIp[16];
    char *pTmp = NULL;
    //unsigned int dstClientId = 0;
    //unsigned int srcClientId = 0;

    struct rb_root root = RB_ROOT;
	char buf[BUF_LEN];
	char value[BUF_LEN];
	int port = 4852;
	int iStrLen = 0;
	int	recv_num;
	int	recvlen;
	int	opt;

    while((opt = getopt(argc, argv,"p:P:vVhH")) != -1) 
    {   
        switch(opt)
        {
            case 'p':   /** port address **/
                port = atoi(optarg);           /** defautl server port **/
                break;
            case 'H':   /** help message **/
            case 'h':   /** help message **/
                printf("eg: %s -p port\n", argv[0]);
                //break;
                return  0;
            case 'v':   /** version **/
            case 'V':   /** version **/
                printf("version: [%s-%s]\n", __DATE__, __TIME__);
                break;
            default:   /** help message **/
                printf("unrecognized options !\n");
                break;
        }
    }   
    if(port <= 0)
    {
		printf("port\n");
		exit(1);
    }


	if(-1 == (sock_fd = socket(PF_INET, SOCK_DGRAM, 0)))
	{
		printf("socket\n");
		exit(2);
	}

	myaddr.sin_family = AF_INET;
	//myaddr.sin_port = htons(atoi(argv[1]));
	myaddr.sin_port = htons(port);
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

        if(recv_num != -1)
        {
            fprintf(stderr, "[%s-%d]parseing ... \n", __FILE__, __LINE__);
            iStrLen = strlen(buf);
            iStrLen = iStrLen > recv_num? iStrLen:recv_num;
            iStrLen = iStrLen < (BUF_LEN - 1)? iStrLen:(BUF_LEN - 1);

            memcpy(value, buf, iStrLen);
            value[iStrLen] = '\0';
            parsePacketFormatId(value, format);

            memcpy(value, buf, iStrLen);
            value[iStrLen] = '\0';
            parseSrcImsi(value, srcImsi);

            memcpy(value, buf, iStrLen);
            value[iStrLen] = '\0';
            parseDstImsi(value, dstImsi);

            if((strcmp(format, "data-send") == 0) || (strcmp(format, "data-ACK") == 0))
            {
                fprintf(stderr, "[%s-%d]received data !\n", __FILE__, __LINE__);

                //dstNode = searchImsi(&root, srcImsi, strcmp);
                dstNode = searchImsi(&root, dstImsi, strcmp);
                if(NULL != dstNode)
                {
                    /** received data and find the destination node, forward data **/
                    fprintf(stderr, "[%s-%d]ready to forward data(length: %d) !\n", __FILE__, __LINE__, iStrLen);
#if 1
                    memcpy(&dst_addr, dstNode->ipAddr, sizeof(struct sockaddr_in));
#else
                    dst_addr.sin_family = AF_INET;
                    dst_addr.sin_port = htons(1234);
                    dst_addr.sin_addr.s_addr = inet_addr("");
#endif
                    int ret = sendto(sock_fd, buf, iStrLen + 1, 0, (struct sockaddr *)dstNode->ipAddr, sizeof(struct sockaddr_in));
                    //int ret = sendto(sock_fd, buf, BUF_LEN, 0, (struct sockaddr *)&dst_addr, sizeof(struct sockaddr_in));
                    if(-1 == ret)
                    {
                        fprintf(stderr, "[%s-%d] send data error !\n", __FILE__, __LINE__);
                        exit(2);
                    }
                #if 1
                    memset(srcIp, 0, 16);
                    memset(dstIp, 0, 16);
                    pTmp = inet_ntoa(src_addr.sin_addr);
                    strcpy(srcIp, pTmp);
                    inet_ntoa(((struct sockaddr_in *)(dstNode->ipAddr))->sin_addr);
                    strcpy(dstIp, pTmp);
                    fprintf(stderr, "[%s-%d]forward data from [%s-%s:%u] to [%s-%s:%u]!\n", 
                            __FILE__, __LINE__, 
                            srcImsi, 
                            srcIp, 
                            src_addr.sin_port,
                            dstImsi,
                            dstIp,
                            ((struct sockaddr_in *)(dstNode->ipAddr))->sin_port);

                    printf("---%s---", buf);
                #endif                      
                }
                else
                {
                    fprintf(stderr, "[%s-%d]destination[%s] is not on line!\n", __FILE__, __LINE__, dstImsi);
                }
            }
            else if(strcmp(format, "login-request") == 0)       /** 登录请求，记录信息 **/
            {
                fprintf(stderr, "[%s-%d]received login message !\n", __FILE__, __LINE__);
                /** received login message **/
                srcNode = searchImsi(&root, srcImsi, strcmp);
                if(NULL == srcNode)
                {
                    /** create new node **/
                    srcNode = malloc(sizeof(struct clientNode_s));
                    if(NULL != srcNode)
                    {
                        bzero(srcNode, sizeof(struct clientNode_s));
                        memcpy(srcNode->clientImsi, srcImsi, CIMSI_LEN);
                        memcpy(srcNode->ipAddr, &src_addr, sizeof(src_addr));
                        srcNode->clientStatus = STATUS_LGINSUC;  /** 进入登录状态 **/

                        insertImsi(&root, srcNode, strcmp);

                        fprintf(stderr, "[%s-%d]Client Node[%s-%s:%u] created ok!\n", 
                                __FILE__, __LINE__, 
                                srcImsi, 
                                inet_ntoa(src_addr.sin_addr), 
                                src_addr.sin_port);
 
                    }
                    else
                    {
                        fprintf(stderr, "[%s-%d]malloc error. Client Node[%s] created failed!\n", __FILE__, __LINE__, srcImsi);
                    }
                }
                else
                {   
                    srcNode->clientStatus = STATUS_LGINSUC;  /** 进入登录状态 **/

                    memcpy(srcNode->clientImsi, srcImsi, CIMSI_LEN);
                    memcpy(srcNode->ipAddr, &src_addr, sizeof(src_addr));
                    fprintf(stderr, "[%s-%d]Client[%s-%s:%u] login again !\n", 
                            __FILE__, __LINE__, 
                            srcImsi,
                            inet_ntoa(src_addr.sin_addr), 
                            src_addr.sin_port);
                }

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
                    fprintf(stderr, "[%s-%d]send login ACK ok\n", __FILE__, __LINE__ );
                }
                /**
                 * store the client [IP:port] 
                 **/
                #if 0
                fprintf(stderr, "[%s-%d]Client [%s: %s: %d] login \n", 
                        __FILE__, __LINE__, 
                        srcImsi, 
                        inet_ntoa(((struct sockaddr_in *)(srcNode->ipAddr))->sin_addr), 
                        ((struct sockaddr_in *)(srcNode->ipAddr))->sin_port);
 
                #endif                      
            }
            else if((strcmp(format, "login-out") == 0) || (strcmp(format, "logout") == 0))
            {
                fprintf(stderr, "[%s-%d]Client[%s] logout!\n", __FILE__, __LINE__, srcImsi);
            }
            else
            {
                fprintf(stderr, "[%s-%d]buf[%s] is unrecognized[%s]!!!\n", __FILE__, __LINE__, buf, format);
            }
            /*********************************/
        }
    }
	close(sock_fd);

    return  0;
}
