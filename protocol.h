
#ifndef __PROTOCOL_H__
#define __PROTOCOL_H__

#define CBROADCAST  (0x11)     /** 仅物理地址信息，表明设备是否还在 **/
#define CCANSWER    (0x12)      /** C 类回答 **/
#define CIdMESSAGE  (0x22)      /** 静态信息，用于完整身份识别 **/


#define CGETSTATUS  (0x41)      /** 查询设备的工作状态 **/
#define CSETSTATUS  (0x42)      /** 设置设备的工作状态 **/
#define CCANSTATUS  (0x43)      /** 设备的工作状态 **/

#define CRAWCMD  (0x44)      /** 原始数据发送 **/

#define CRMTGETSTATUS  (0x51)   /** 单向遥控查询设备的工作状态 (遥控器主动发起) **/
#define CRMTSETSTATUS  (0x52)   /** 单向遥控设置设备的工作状态 (遥控器主动发起) **/

#define CDUPLEXSETSTA  (0x62)   /** 设备的对端设定设备的工作状态 **/
#define CDUPLEXPUTACK  (0x63)   /** 主控制器回应设备的状态报告给设备的对端 **/

#define CDIRECTPUT  (0x73)      /** 设备的报告自己的工作状态 (受控设备主动发起) **/
#define CDIRECTACK  (0x74)      /** 主机回复自已收到工作状态 (受控设备主动发起) **/

//#define MIsCmdA(x)  (((x) == CSETSTATUS) || ((x) == CGETSTATUS) || ((x) == CCANSTATUS))
#define MBaseCMD(x)  (((x) == CSETSTATUS) || ((x) == CGETSTATUS) || ((x) == CCANSTATUS))
#define MIsCmdB(x)  ((x) ==  CIdMESSAGE)
#define MIsCmdC(x)  (((x) == CBROADCAST) || ((x) == CCANSWER))

#define MIsRawCmd(x)  ((x) == CRAWCMD)

//#define MIsRMTCmd(x) (((x) == CRMTSETSTATUS) || ((x) == CRMTGETSTATUS))
#define MIsDUPRMTCmd(x) (((x) == CDUPLEXSETSTA) || ((x) == CDUPLEXPUTACK))
//#define MIsDIRCmd(x) (((x) == CDIRECTPUT) || ((x) == CDIRECTACK))

#endif
