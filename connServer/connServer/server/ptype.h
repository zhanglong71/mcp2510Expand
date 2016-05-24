#ifndef __PTYPE_H__
#define __PTYPE_H__

#include    "const.h"

#if defined(__cplusplus)
#define NULL 0
#else
#define NULL ((void *)0)
#endif

#ifdef __compiler_offsetof
#define offsetof(TYPE,MEMBER) __compiler_offsetof(TYPE,MEMBER)
#else
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#endif

#define container_of(ptr, type, member) ({			\
	const typeof( ((type *)0)->member ) *__mptr = (ptr);	\
	(type *)( (char *)__mptr - offsetof(type,member) );})

struct rb_node
{
	unsigned long  rb_parent_color;
#define	RB_RED		0
#define	RB_BLACK	1
	struct rb_node *rb_right;
	struct rb_node *rb_left;
} __attribute__((aligned(sizeof(long))));
    /* The alignment might seem pointless, but allegedly CRIS needs it */

struct rb_root
{
	struct rb_node *rb_node;
    int num;
};

#define RB_ROOT	(struct rb_root) {NULL, 0}


typedef union ipAddr_un{
//   unsigned char ucIpaddr[4];
   unsigned char ucIpaddr[16];
   unsigned int uiIpaddr;
} ipAddr_t;

typedef enum clientStatus_s {
    STATUS_IDLE = 0,    /** initial, no login **/

    STATUS_LGINSUC, /** login success **/
    STATUS_LGINFAI, /** login fail **/

} clientStatus_t;

typedef struct clientNode_s {
    unsigned int     clientId;
    char    clientImsi[CIMSI_LEN];    /** 手机卡号(比手机号更具唯一性)用来代替clientId **/
    int     socketId;
    enum    clientStatus_s clientStatus;
    char    password[PASSWORD_LEN];
    char    passwordLen;
    char    __pad;              /** alignment **/
    //union   ipAddr_un ipAddr;
    char ipAddr[16];    /** 设成当前知道的地址中长度最大的 **/

    int netAF;
    char loginAddr[16];
/** make sure the port is stored in mechine byte order **/
    unsigned short port;
    struct  rb_node node;       /** all client **/
    struct  rb_node friendNode;
} clientNode_t;

/** the data class descriptes the packet class **/
typedef enum format_s {
    CFORMAT_IDLE = 0,
/** login/logout **/
    CFORMAT_LGINREQ,    /** login request **/
    CFORMAT_LGINSUC,    /** login ACK: success **/
    CFORMAT_LGINFAI,    /** login ACK: fail **/
/*** only-for-test: login success directly ***/
    CFORMAT_LOGINOK,
    CFORMAT_LOGOUT,     /** when received this packet, make sure PASSWORD matched **/

    CFORMAT_DATA,       /** nornal data **/
    CFORMAT_DATAACK,    /** nornal data ack **/
    CFORMAT_RAW,        /** row data **/
    CFORMAT_CGI,        /** CGI data **/

} format_t;

/**
 * 用字符串取代下面的两种包的格式(ASCII形式)
 * key:value\n
 *
 * Packet-Format:value[login:data]
 * Destinatioan-Imis:1234567890
 * Source-Imsi:1234567891
 * Destinatioan-Telephone:11111111111
 * Source-Telephone:22222222222
 * Destinatioan-ClientId:33333333333
 * Source-ClientId:44444444444
 * Text:stringTextForDislay
 **/

typedef struct packetData_s {
    enum format_s formatId;
    char    dstImsi[CIMSI_LEN];    /** 手机卡号(比手机号更具唯一性)用来代替clientId **/
    char    srcImsi[CIMSI_LEN];    /** 手机卡号(比手机号更具唯一性)用来代替clientId **/
    unsigned int dstClientId;
    unsigned int srcClientId;
    unsigned int totalLen;

    unsigned char buf[HRM_MAX_PACKET_LEN - (3 * sizeof(unsigned int)) - sizeof(enum format_s)];
} packetData_t;

typedef struct packetAuthentication_s {
    enum format_s formatId;
    char    dstImsi[CIMSI_LEN];    /** 手机卡号(比手机号更具唯一性)用来代替clientId **/
    char    srcImsi[CIMSI_LEN];    /** 手机卡号(比手机号更具唯一性)用来代替clientId **/
    unsigned int dstClientId;
    unsigned int srcClientId;
    unsigned int totalLen;

    unsigned int passwordLen;
    char    password[PASSWORD_LEN];
} packetAuthentication_t;

typedef union packet_un {
    struct packetData_s packetData;
    struct packetAuthentication_s  packetAuthen;
} packet_t;

#if 0
struct mytype {
    struct rb_node node;
    char *keystring;
};
#endif

typedef unsigned int (*cmp_t)(const unsigned int arg1, const unsigned int arg2);
typedef int (*charsCmp_t)(const char* __arg1, const char* __arg2);

#endif

