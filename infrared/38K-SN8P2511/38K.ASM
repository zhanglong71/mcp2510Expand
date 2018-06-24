//*******************************************************************************
//
// Project NO.--->: x-10 signal generator
// Project Name-->: canbus-x10
// Customer------>:
// Author-------->: zhanglong
// Date---------->: 2014/9/18 16:28
// IC body------->: SN8P2511
// REVISION------>:
//                                       for SN8ICE 2K
//*******************************************************************************
//*  (c) Copyright 
//*******************************************************************************

CHIP	SN8P2511		//the selector for program into the chip.
//CHIP	SN8P2501B		//Select the chip for emulate 2511 on plus II.
//{{SONIX_CODE_OPTION
	.Code_Option	Noise_Filter	Disable
  .Code_Option     Security     Enable
	.Code_Option	Fcpu		#2     ; Fcpu = Fosc/4

  .Code_Option     High_Clk     IHRC_16M
	.Code_Option	Watch_Dog	Always_On
  .Code_Option     Reset_Pin     P11
	.Code_Option	LVD		LVD_L		; 2.0V Reset
//}}SONIX_CODE_OPTION

//-------------------------------------------------------------------------------
//			Include Files
//-------------------------------------------------------------------------------
.nolist							// do not list the macro file

	INCLUDESTD		MACRO1.H
	INCLUDESTD		MACRO2.H
	INCLUDESTD		MACRO3.H

.list							// Enable the listing function
	
//-------------------------------------------------------------------------------
//			Constants Definition
//-------------------------------------------------------------------------------

C_TC0_Rt		 equ		#0x70	    //Set TC0M Value
C_TC0_CNT		 equ		#116	    //Set TC0C Value,70us


RXL_HI0    equ  3
RXL_HI1    equ  9
RXL_LOW0   equ  10
RXL_LOW1   equ  22

RXH_HI0    equ  7
RXH_HI1    equ  18
RXH_LOW0   equ  4
RXH_LOW1   equ  10

RX_HEAD_HI    equ #50

CNT_10MS      equ #210       //143*70us=10ms
CNT_01S        equ #50       //50*10ms=0.1s


//----------------------------
//	Pin Definition
//----------------------------
PinB0 equ P2.0
PinB1 equ P2.1
PinB2 equ P2.2
PinB3 equ P2.3
PinB4 equ P2.4
PinRx equ P5.4
PinOpen     equ   P0.0
PinClose    equ   P1.0


//-------------------------------------------------------------------------------
//			Variables Definition
//-------------------------------------------------------------------------------
.DATA
		org	0h			//Data section start from RAM address 0
	//Wk00		  DS	1			//Temporary buffer for main loop
	//Iwk00		  DS	1			//Temporary buffer for ISR
	AccBuf		    DS 1		//Accumulater buffer
	PflagBuf	    DS 1		//PFLAG buffer
	flag0         DS 1
	cntRdEep      DS 1
  buf1          DS 1     //main loop data buf
  cntRxHi       DS 1    //高电平计数器
  cntRxLo       DS 1    //低电平计数器
  rx_bit        DS 1    //RX BIT BUF
  pre_bit       DS 1    //前一位缓存,作判断上升沿与下降沿用
  rx_bit_buf0   DS 1    //位缓存,防抖动用
  rx_bit_buf1   DS 1    //位缓存,防抖动用
  rx_bit_buf2   DS 1    //位缓存,防抖动用
  RxDataBuf0    DS 1    //接收数据缓存
  RxData0       DS 1    //接收数据
  
  PointRx       DS 1    //接收位指针
  PointDataBit  DS 1    //接收数据位指针
  CntTestHead   ds 1
  testbuf       ds 1
  
  
  cnt70us       ds 1
  cnt10ms       ds 1

  delayloop      ds 1

  
//-------------------------------------------------------------------------------
//			Bit Variables Definition
//-------------------------------------------------------------------------------

	f_01s      EQU     flag0.0
	f_10ms     EQU     flag0.1
	f_lo_hi     EQU     flag0.5       //rx low to hight level change flag
	f_rx_data   EQU     flag0.6       //RX data buf
	f_rx_finish EQU     flag0.7
	bit_buf     EQU     rx_bit.0      //RF rx bit buf


//-------------------------------------------------------------------------------
//			Code section
//-------------------------------------------------------------------------------
.CODE								
	ORG	0				      //Code section start
	jmp		Reset				//Reset vector
							      //Address 4 to 7 are reserved
	ORG	8				
	jmp		Isr				  //Interrupt vector
	ORG	10h
//-------------------------------------------------------------------------------
// 			Program reset section
//-------------------------------------------------------------------------------
Reset:
	mov 	    A,#07Fh				  //Initial stack pointer and
  b0mov 		STKP,A				  //disable global interrupt	
	clr		    PFLAG				    //pflag = x,x,x,x,x,c,dc,z
	mov     	A,#00h				  //Initial system mode
	b0mov   	OSCM,A
	mov		    A, #0x5A
	b0mov		  WDTR, A				  //Clear watchdog timer	
  call    	ClrRAM				  //Clear RAM
  call    	SysInit				  //System initial
  b0bset  	FGIE				    //Enable global interrupt
//-------------------------------------------------------------------------------
// 			Main routine
//-------------------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------enable tc0---------------------
mov a,#01110000b
b0mov  tc0m,a
//mov a,#1
mov a,#8
//mov a,#9
//mov a,#10
//mov a,#12
//mov a,#14
//mov a,#16
//mov a,#32
//mov a,#63
b0mov tc0c,a
b0mov tc0r,a

//b0bset ftc0out
//b0bset faload0
//b0bset fpwm0out


//b0bset ftc0enb
b0bset P5M.4
b0bclr P5.4


Main:
  mov_		WDTR,#0x5a				//Clear watchdog timer
  b0bts0 P0.0
  jmp @f

b0bclr P5.4	/** followed 32 nop **/

//mov	a, #3
mov	a, #14
call dlay
//nop

b0bset P5.4	/** followed 25 nop **/

//mov	a, #1
mov	a, #14
call dlay
//nop

  jmp     EndMain
@@:
	/** idle state **/
	//b0bset P5.4
	b0bclr P5.4
//----------------------------------------------------
  jmp     EndMain
EndMain:
	jmp Main

//-------------------------------------------------------------------------------
// Isr (Interrupt Service Routine)
//-------------------------------------------------------------------------------
Isr:
//-----------------------------------
// Save ACC
//-----------------------------------
	b0xch	  A,AccBuf	  //B0xch instruction do not change C,Z flag
	b0mov   A,PFLAG
  b0mov   PflagBuf,A
//-----------------------------------
// Interrupt service routine  
//-----------------------------------
INTTC0CHK:						// Check TC0 interrupt request
	B0BTS1	FTC0IEN			// Check TC0IEN
	JMP	    INT_EXIT		// Jump to exit of IRQ
	B0BTS0	FTC0IRQ			// Check TC0IRQ
	JMP	    INTTC0			// Jump to TC0 interrupt service routine
//-----------------------------------
// Exit interrupt service routine
//-----------------------------------
INT_EXIT:
  b0mov   A, PflagBuf
	b0mov   PFLAG, A			//Restore the PFlag
	b0xch		A,AccBuf			//Restore the Reg. A
					              //B0xch instruction do not change C,Z flag
	reti						      //Exit the interrupt routine
//-------------------------------------------------------------------------------
//	TC0 interrupt service routine
//-------------------------------------------------------------------------------
INTTC0:                //100us timer
	b0bclr	FTC0IRQ
//-----------------------------------------------------------
//                     开始数据接收
//-----------------------------------------------------------
	//incms testbuf
	//nop
	//nop
	//B2P   testbuf.0,PinTST
	jmp	INT_EXIT
//-------------------------------------------------------------------------------
// SysInit
// System initial to define Register, RAM, I/O, Timer......
//-------------------------------------------------------------------------------
SysInit:
	SysInitT0:
	b0bclr		FT0IEN
	b0bclr		FT0IRQ
	b0bclr		FT0ENB
SysInitTC0:	
	
SysInitP00:	
	b0bclr		FP00IEN
	
//--------------------------------------------------------------------------
SysInitPort:
//	b0bset PinPwr
SysInitPortMode:	
	mov    a,#0xff
	b0mov    P5M,a
	mov    a,#0xff
	b0mov    P2M,a
	mov    a,#0x01
	b0mov    P1M,a
	mov    a,#0x00
	b0mov    P0M,a
SysInitPortDataBuffer:
  mov    a,#0x00
  b0mov    P5,a
  b0mov    P2,a
  b0mov    P1,a
SysInitPortPullUp:
	mov  a,#0xff
	b0mov  P0UR,a
	b0mov  P1UR,a
	
	
	mov  a,#0x00
	b0mov  P2UR,a
	b0mov  P5UR,a
SysInitRAM:
	ret

//-------------------------------------------------------------------------------
// ClrRAM
// Use index @YZ to clear RAM (00h~2Fh)
//-------------------------------------------------------------------------------
ClrRAM:
	clr 		Y		 
	mov		A, #0x7f
	b0mov		Z, A				  //Set @YZ address from 2fh
ClrRAM10:
	clr 		@YZ				    //Clear @YZ content
	decms		Z   				  //z = z - 1 , skip next if z=0
	jmp 		ClrRAM10 
	clr 		@YZ				    //Clear address $00
	ret
//-------------------------------------------------------------------------------
/*******************************************************************************
 * delay
 * description:
 * input:  a
 * output: a
 *  
 *
 *******************************************************************************/
dlay:
	b0mov Z, A
dlayLoop:
	decms	Z 
	jmp		dlayLoop

	ret
//**********************************************************		
//-------------------------------------------------------------------------------
//**********************************************************		
//-------------------------------------------------------------------------------
	ENDP