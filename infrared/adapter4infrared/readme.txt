
 /***
  * 后续考虑过零检测时间偏移校正。
  * 将校正后的偏移值保存在最后一个page。
  * 对本项目使用的STM32F103-RBT6而言，20 Kbytes of SRAM, 128 Kbytes of Flash memory。
  * 128 Kbytes of Flash memory对应0x08000000-0x0801ffff
  * 从固件库使用手册中得知，每一页的大小为1024Bytes。以4bytes为一个字，共256个字。
  ***/

2014/10/10 13:14 定时器定为50kHz
2014/10/22 10:25 初步完成，录信号时400ms周期闪烁，发送时，闪烁。待验证
2014/10/29 10:25 定时器定为25kHz
2014/10/31 9:43 录入/发送信号正常。通道选择可用。数据长度及数据头都保存在缓冲后面.初始化关闭全部通道
2014/11/3 16:04 添加有效标志. 信号发出，但数据的正确性有待确认(使用PC7/PC9进行了测试没问题)
2014/11/4 15:11 此版本对"HDMI Matrix 4x2 HIFI操作成功"可以成功的通过主控制器发送数据控制4x2矩阵(但有效距离很近)  注意对应电路只是发没有38KHz的的普通电平信号

2014/11/6 13:43 实现数据发送，并且对"HDMI Matrix 4x2 HIFI操作成功" 注意与此前有响应的方式不同，对应电路发有38KHz的脉冲电平信号