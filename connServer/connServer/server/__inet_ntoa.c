#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "ptype.h"
#include "rbtree.h"
#include "parse.h"


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

}


int main(int argc, char* argv[])
{
    struct clientNode_s *dstNode;
    struct sockaddr_in dst_addr;
    int i;

    dstNode = malloc(sizeof(struct clientNode_s));
    if(dstNode != NULL)
    {
        for(i = 0; i < 10; i++)
        {
            dstNode->clientImsi[i] = i + '0';
        }
        dstNode->clientImsi[i] = '\0';

        dst_addr.sin_family = AF_INET;
        dst_addr.sin_port = htons(1234);
        dst_addr.sin_addr.s_addr = inet_addr("192.168.1.1");
        memcpy(dstNode->ipAddr, &dst_addr, sizeof(dst_addr));

        printNode(dstNode); //????????????????????????????????
        fprintf(stderr, "[%s-%d]-[%s-%s:%u]!\n", 
                __FILE__, __LINE__, 
                dstNode->clientImsi,
                inet_ntoa(((struct sockaddr_in *)(dstNode->ipAddr))->sin_addr), 
                ((struct sockaddr_in *)(dstNode->ipAddr))->sin_port);
        printNode(dstNode); //????????????????????????????????

    }
}

