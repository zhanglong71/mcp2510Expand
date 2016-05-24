
此目录下是对mcp2510直接操作接口相关的文件. 数据的收发以原始形态存在。
Note: 发送时会在数据结尾加入一个byte的CRC校验. 相对于从x10接口发出的数据的区别：没有最后一个字节的CRC8


eg. ./app_raw_write /dev/mcp2510 010f000000000000000042010500 e
eg. ./app_raw_write /dev/mcp2510 010f000000000000000042010700 e


建议-s及-t参数设定相同的值，即设定适配器通道编号(canbus地址)，设备类型编号一致.
eg. ./app_raw_ioctl -a 040f -s 2 -t 2
eg. ./app_raw_ioctl -a 0f0f -s 3 -t 3

2014-05-28 18:02:54 修改app_raw_write 接收参数的方式
eg. ./app_raw_write /dev/mcp2510 010f020000000000000042010500   the function same as below
eg. ./app_raw_write /dev/mcp2510 010f020000000000000042010500 2

