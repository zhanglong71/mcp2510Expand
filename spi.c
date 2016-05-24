
//#include "spi.h"
#define rCLKCON   0x4C00000C

#define rGPECON   0x56000040
#define rGPEDAT   0x56000044
#define rGPEUP   0x56000048

#define rGPGCON  0x56000060
#define rGPGDAT   0x56000064
#define rGPGUP   0x56000068

#define rSPCON0   0x59000000
#define rSPSTA0   0x59000004
#define rSPPIN0   0x59000008
#define rSPPRE0   0x5900000C
#define rSPTDAT0  0x59000010
#define rSPRDAT0  0x59000014

#define SPI0_READY  (ioread8((void *)SPSTA0) & 0x1)

#define MCP2510_CS_H iowrite32(ioread32((void *)GPGDAT) | (0x1<<14), (void *)GPGDAT)
#define MCP2510_CS_L iowrite32(ioread32((void *)GPGDAT) & (~(0x1<<14)), (void *)GPGDAT)

#if 0
void MCP2510_reset(void);
u8 MCP2510_Read(u8 Addr);
void MCP2510_Write(u8 Addr, u8 Data);
void MCP2510_BitModi(u8 Addr, u8 Mask, u8 Data);
#endif

volatile unsigned int *GPGDAT;
volatile u8 *SPTDAT0;
volatile u8 *SPRDAT0;
volatile u8 *SPSTA0;
static unsigned char TXBnCTRL[3] = {0x30, 0x40, 0x50};

#if 0
static void MCP2510_reset(void)
//void MCP2510_reset(void)
{
    MCP2510_CS_L;
    iowrite8(0xc0, (void *)SPTDAT0);
    while(!SPI0_READY);
    MCP2510_CS_H;
}
#endif

static u8 MCP2510_Read(u8 Addr)
//u8 MCP2510_Read(u8 Addr)
{
    u8 result;
    MCP2510_CS_L;
    iowrite8(0x03, (void *)SPTDAT0);
    while(!SPI0_READY);
    iowrite8(Addr, (void *)SPTDAT0);
    while(!SPI0_READY);
    iowrite8(0xFF, (void *)SPTDAT0);
    while(!SPI0_READY);
    result = ioread8((void *)SPRDAT0);
    MCP2510_CS_H;
    return result;
}

static void MCP2510_Write(u8 Addr, u8 Data)
//void MCP2510_Write(u8 Addr, u8 Data)
{
    MCP2510_CS_L;
    while(!SPI0_READY);
    iowrite8(0x02, (void *)SPTDAT0);
    while(!SPI0_READY);
    iowrite8(Addr, (void *)SPTDAT0);
    while(!SPI0_READY);
    iowrite8(Data, (void *)SPTDAT0);
    while(!SPI0_READY);
    MCP2510_CS_H;
}

static void MCP2510_BitModi(u8 Addr, u8 Mask, u8 Data)
//void MCP2510_BitModi(u8 Addr, u8 Mask, u8 Data)
{
    MCP2510_CS_L;
    iowrite8(0x05, (void *)SPTDAT0);
    while(!SPI0_READY);
    iowrite8(Addr, (void *)SPTDAT0);
    while(!SPI0_READY);
    iowrite8(Mask, (void *)SPTDAT0);
    while(!SPI0_READY);
    iowrite8(Data, (void *)SPTDAT0);
    while(!SPI0_READY);
    MCP2510_CS_H;
}

static void __config_spi(void)
{
    volatile unsigned int *REGISTER;
/* GPG(2, 13)*/
    //设置GPG2为输出(底电平重启), GPG13（外部中断号21）为中断
    REGISTER = (unsigned int *)ioremap(rGPGCON, 4);
    PLOG("GPGCON Value is %u\n", ioread32((void *)REGISTER));
    iowrite32((ioread32((void *)REGISTER) & (~((0x03 << 26) | (0x03 << 4)))) | (0b10 << 26) | (0b01 << 4), (void *)REGISTER);
    PLOG("GPGCON Value is %u\n", ioread32((void *)REGISTER));
    iounmap(REGISTER);

    //使能GPG2、13上拉
    REGISTER = (unsigned int *)ioremap(rGPGUP, 4);
    PLOG("GPGUP Value is %u\n", ioread32((void *)REGISTER));
    iowrite32(ioread32((void *)REGISTER) & (~((0x1 << 13) | (0x1 << 2))), (void *)REGISTER);
    PLOG("GPGUP Value is %u\n", ioread32((void *)REGISTER));
    iounmap(REGISTER);

/* reset mcp2510. Is it necessary ? */
    //设置GPG2为高电平，GPG2和MCP2510的RST连接
    REGISTER = (unsigned int *)ioremap(rGPGDAT, 4);
    PLOG("GPGDAT Value is %u\n", ioread32((void *)REGISTER));
    iowrite32(ioread32((void *)REGISTER) | (0x1 << 2), (void *)REGISTER);
    PLOG("GPGDAT Value is %u\n", ioread32((void *)REGISTER));
    iounmap(REGISTER);

    //设置GPG14为输出，SPI_CS信号
    REGISTER = (unsigned int *)ioremap(rGPGCON, 4);
    PLOG("GPBCON Value is %u\n", ioread32((void *)REGISTER));
    iowrite32((ioread32((void *)REGISTER) & (~(0b11 << 28))) | (0b01 << 28), (void *)REGISTER);
    PLOG("GPBCON Value is %u\n", ioread32((void *)REGISTER));
    iounmap(REGISTER);

    //使能GPG14上拉
    REGISTER = (unsigned int *)ioremap(rGPGUP, 4);
    PLOG("GPBUP Value is %u\n", ioread32((void *)REGISTER));
    iowrite32(ioread32((void *)REGISTER) & (~(0x1 << 14)), (void *)REGISTER);
    PLOG("GPBUP Value is %u\n", ioread32((void *)REGISTER));
    iounmap(REGISTER);

    GPGDAT = (unsigned int *)ioremap(rGPGDAT, 4);
    SPTDAT0 = (u8 *)ioremap(rSPTDAT0, 1);
    SPRDAT0 = (u8 *)ioremap(rSPRDAT0, 1);
    SPSTA0 = (u8 *)ioremap(rSPSTA0, 1);

    //上拉GPG14
    PLOG("GPGDAT Value is %u\n", ioread32((void *)GPGDAT));
    MCP2510_CS_H;
    PLOG("GPGDAT Value is %u\n", ioread32((void *)GPGDAT));

    //打开SPI CLK使能
    REGISTER = (unsigned int *)ioremap(rCLKCON, 4);
    PLOG("CLKCON Value is %u\n", ioread32((void *)REGISTER));
    iowrite32((ioread32((void *)REGISTER) | (1 << 18)), (void *)REGISTER);
    PLOG("CLKCON Value is %u\n", ioread32((void *)REGISTER));
    iounmap(REGISTER);

    //设置GPE11、12、13为SPI功能
    REGISTER = (unsigned int *)ioremap(rGPECON, 4);
    PLOG("GPECON Value is %u\n", ioread32((void *)REGISTER));
    iowrite32((ioread32((void *)REGISTER) & (0xF03FFFFF)) | (0b10 << 26) | (0b10 << 24) | (0b10 << 22), (void *)REGISTER);
    PLOG("GPECON Value is %u\n", ioread32((void *)REGISTER));
    iounmap(REGISTER);

    //设置SPI0波特率
    REGISTER = (unsigned int *)ioremap(rSPPRE0, 4);
    PLOG("SPPRE0 Value is %u\n", ioread8((void *)REGISTER));
    iowrite8(0x9, (void *)REGISTER);
    PLOG("SPPRE0 Value is %u\n", ioread8((void *)REGISTER));
    iounmap(REGISTER);

    //设置SPCON0
    REGISTER = (unsigned int *)ioremap(rSPCON0, 4);
    PLOG("SPCON0 Value is %u\n", ioread8((void *)REGISTER));
    iowrite8((0 << 6)|(0 << 5)|(1 << 4)|(1 << 3)|(0 << 2)|(0 << 1)|(0 << 0), (void *)REGISTER);
    PLOG("SPCON0 Value is %u\n", ioread8((void *)REGISTER));
    iounmap(REGISTER);

    //设置SPPIN0
    REGISTER = (unsigned int *)ioremap(rSPPIN0, 4);
    PLOG("SPPIN0 Value is %u\n", ioread8((void *)REGISTER));
    iowrite8(0, (void *)REGISTER);
    PLOG("SPPIN0 Value is %u\n", ioread8((void *)REGISTER));
    iounmap(REGISTER);
}


