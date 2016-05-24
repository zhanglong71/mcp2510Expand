#ifndef __HUARAIN_IOCTL__
#define __HUARAIN_IOCTL__

#define MCP2510_TYPE   'z'
#define READ_STATUS    _IOR(MCP2510_TYPE, 0, char)
#define READ_VERSION    _IOR(MCP2510_TYPE, 1, char)

#define WRITE_STATUS    _IOW(MCP2510_TYPE, 0, char)
#define MDevRgst     _IOW(MCP2510_TYPE, 1, char)     /** 设备注册信息 **/
#define MDevCtrl     _IOW(MCP2510_TYPE, 2, char)     /** 设备控制信息 **/
#define MDevSAdap     _IOW(MCP2510_TYPE, 3, char)     /** 设备控制信息, 设置适配器号 **/
#define MDevSAddr     _IOW(MCP2510_TYPE, 4, char)     /** 设备控制信息, 设置地址信息 **/
#define MDevSType     _IOW(MCP2510_TYPE, 5, char)     /** 设备控制信息, 设置设备类型 **/


#endif
