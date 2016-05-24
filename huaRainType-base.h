#ifndef __HUARAINTYPE_BASE_H__
#define __HUARAINTYPE_BASE_H__

#include    "CONST.h"

/** 
 * Note: make sure the all dataType define here have no dependent **/
//#define MTASKLETINIT    "taslket initial"   /** tasklet initial argment  **/
//#define MHuaRain    "HuaRain-taslket"   /** tasklet initial argment  **/
//#define MDEVRECEIVED    "received data"
//#define MDEVREGISTER    "register device"

struct CANBus_Message {
    unsigned int EID;
    unsigned short SID;
    unsigned char Length;
    unsigned char RTR;
    unsigned char Messages[8];
};

#if 1
typedef struct rgstInfo_s{
    unsigned char addr[4];      /** address **/
    unsigned char ucAdapterNo;  /** adapter NO. **/ 
    unsigned char ucDevType;    /** device type **/
} rgstInfo_t;
#endif

/** 静态信息  **/
typedef struct IdentifyMsg_s {
    unsigned char ucPhysAddr[4];    /** 物理地址 **/
    unsigned short usVendorNo;      /** 厂商编号 **/
    unsigned short usProductNo;     /** 产品编号 **/
    unsigned short usDevType;       /** 设备类型 (作用参看deviceType_t数组) **/
    /** 出厂日期 **/
    unsigned short usYear;          /** 0 - 65535 **/
    unsigned char  ucMonth;         /** 1 - 12 **/
    unsigned char  ucDay;           /** 1 - 31 **/

    unsigned char other[8];
}IdMsg_t;

/** 协议中的A类命令 **/
typedef struct DevCmdA_s {
    unsigned char ucDestAddr[4];
    unsigned char ucSrcAddr[4];

//    unsigned char ucPhysAddr[4];
    unsigned char ucVersion;
    unsigned char reserved;
    unsigned char ucOpcode;

    unsigned char ucAttr;
    unsigned short usValue;
    unsigned short usCRC;

}DevCmdA_t;

/** 协议中的B类命令 **/
typedef struct DevCmdB_s {
    unsigned char ucDestAddr[4];
    unsigned char ucSrcAddr[4];

//    unsigned char ucPhysAddr[4];
    unsigned char ucVersion;
    unsigned char reserved;
    unsigned char ucOpcode;

    struct IdentifyMsg_s stIdmsg;   /** 静态信息，也是身份识别信息 **/
    unsigned short usCRC;

}DevCmdB_t;

/** 协议中的C类命令 **/
typedef struct DevCmdC_s {
    unsigned char ucDestAddr[4];
    unsigned char ucSrcAddr[4];

//    unsigned char ucPhysAddr[4];
    unsigned char ucVersion;
    unsigned char reserved;
    unsigned char ucOpcode;

    unsigned char ucQuery;
    unsigned short usCRC;

}DevCmdC_t;

/** 协议中的D类命令 **/
typedef struct DevCmdD_s {
    unsigned char ucDestAddr[4];
    unsigned char ucSrcAddr[4];

//    unsigned char ucPhysAddr[4];
    unsigned char ucVersion;
    unsigned char reserved;
    unsigned char ucOpcode;

    unsigned char ucAttr;
    unsigned short usValue;

    unsigned char ucPairAddr[4];
    unsigned short usCRC;

}DevCmdD_t;

/** recv/send data packet format **/
typedef union DevCmd_s {
    struct DevCmdA_s stDevCmdA;
    struct DevCmdB_s stDevCmdB;
    struct DevCmdC_s stDevCmdC;
    struct DevCmdD_s stDevCmdD;
}DevCmd_ut;

/** 临时发送数据之用 **/
typedef struct sendCmd_s {
    unsigned char cmd[MMaxRcvLen];  /** note: the size must bigger than the longest cmd **/
    unsigned int len;

    unsigned int uiEID;
    unsigned short usSID;
    unsigned char ucAdapterNo;
}sendCmd_t;

typedef struct sendCmd_s recvCmd_t; /** Same as SendCmd_t **/


#endif /**  __HUARAINTYPE_BASE_H__ **/

