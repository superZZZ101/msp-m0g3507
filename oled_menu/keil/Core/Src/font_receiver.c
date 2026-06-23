/**
 * font_receiver.c — MCU 通过 UART 接收 GB2312 字库并写入 W25Q64
 *
 * 与 PC 端 gen_send_font.py / send_font.py 配合使用。
 *
 * 协议流程 (PC 发 → MCU 收):
 *   PC → MCU:  同步头 4 字节 (EB 90 EB 90)
 *   PC → MCU:  文件长度 4 字节 (小端)
 *   MCU → PC:  0x06 (长度确认 ACK)
 *   MCU:       擦除 W25Q64 扇区
 *   MCU → PC:  0x06 (擦除完成 ACK)
 *   PC → MCU:  字库数据 (每块 256 字节, 依次发送)
 *   MCU → PC:  0x06 (每块 ACK)
 *   MCU → PC:  0x00 (全部写入完成)
 *   PC → MCU:  CRC32 4 字节 (小端)
 *   MCU → PC:  0x06 (校验通过)
 *   MCU → PC:  0x15 (校验失败) + MCU 算的 CRC32 4 字节 (小端)
 *
 * 使用:
 *   1. 初始化 UART (115200 8N1)
 *   2. 初始化 W25Q64 (SPI 或 GPIO 模拟)
 *   3. 调用 FontReceiver_Run(GB2312_FONT_FLASH_ADDR)
 *
 * 依赖:
 *   - w25q64.h (W25Q64_Init, W25Q64_EraseSector, W25Q64_Write, W25Q64_ReadData)
 *   - ti_msp_dl_config.h (UART0_INST 及系统时钟定义)
 */


#include "font_receiver.h"
#include "uart.h"
#include "w25q64.h"
#include "led.h"
#include "ti_msp_dl_config.h"
/* ============================================================
 * 协议常量定义
 * ============================================================ */
#define SYNC_HEADER      { 0xEB, 0x90, 0xEB, 0x90 }
#define SYNC_HEADER_SIZE  4
#define ACK               0x06
#define NAK               0x15
#define DONE              0x00
#define ERR               0xFF
#define BLOCK_SIZE        256



/* ============================================================
 * CRC32 计算 (与 PC 端算法一致)
 * ============================================================ */
static uint32_t crc32_calc(const uint8_t *data, uint32_t len)
{
    uint32_t crc = 0xFFFFFFFFUL;
    for (uint32_t i = 0; i < len; i++)
    {
        crc ^= data[i];
        for (uint8_t b = 0; b < 8; b++)
        {
            if (crc & 1)
                crc = (crc >> 1) ^ 0xEDB88320UL;
            else
                crc >>= 1;
        }
    }
    return crc ^ 0xFFFFFFFFUL;
}

/* ============================================================
 * 等待同步头
 * 返回 0=成功, -1=超时 (实际上 uart_getc 阻塞不会超时)
 * ============================================================ */
static int wait_sync(void)
{
    const uint8_t sync[] = SYNC_HEADER;
    uint8_t i = 0;

    for (uint32_t timeout = 0; timeout < 60000000; timeout++)
    {
        uint8_t b = uart_getc();

        if (b == sync[i])
            i++;
        else if (b == sync[0])
            i = 1;
        else
            i = 0;

        if (i == SYNC_HEADER_SIZE)
            return 0;
    }
    return -1;  /* 超时 */
}

/* ============================================================
 * 接收 4 字节 (小端)
 * ============================================================ */
static uint32_t recv_u32(void)
{
    uint32_t val;
    val  = (uint32_t)uart_getc();
    val |= (uint32_t)uart_getc() << 8;
    val |= (uint32_t)uart_getc() << 16;
    val |= (uint32_t)uart_getc() << 24;
    return val;
}

/* ============================================================
 * 发送 4 字节 (小端)
 * ============================================================ */
static void send_u32(uint32_t val)
{
    uart_putc((uint8_t)(val >> 0));
    uart_putc((uint8_t)(val >> 8));
    uart_putc((uint8_t)(val >> 16));
    uart_putc((uint8_t)(val >> 24));
}

/* ============================================================
 * 擦除 W25Q64 指定扇区范围
 * 从 sectorStart 到 sectorEnd, 按 4KB 扇区逐个擦除
 * 返回 0=成功, -1=擦除失败
 * ============================================================ */
static int erase_range(uint32_t sectorStart, uint32_t sectorEnd)
{
    for (uint32_t s = sectorStart; s < sectorEnd; s += W25Q64_SECTOR_SIZE)
    {
        if (W25Q64_EraseSector(s) != W25Q64_OK)
        {
            return -1;
        }
    }
    return 0;
}

/* ============================================================
 * FontReceiver_Run — 主接收函数
 *
 * 等待 PC 发来同步头, 接收字库数据并写入 W25Q64,
 * 最后做 CRC 校验确保数据正确。
 *
 * @param flashAddr  W25Q64 中字库起始地址 (通常是 0 或 312)
 * @return  0=成功, -1=协议错误/校验失败, -2=Flash 操作失败
 *
 * 调用前需完成:
 *   - UART 已初始化 (115200 8N1)
 *   - W25Q64_Init() 已调用
 * ============================================================ */
int FontReceiver_Run(uint32_t flashAddr)
{	
		uint8_t blockBuf[BLOCK_SIZE];
    uint32_t bytesRemaining;
    uint32_t writeAddr;
    /* 1. 等待同步头 */
    if (wait_sync() != 0)
    {
        uart_putc(ERR);
        return -1;
    }

    /* 2. 接收文件长度 */
    uint32_t fileLen = recv_u32();
    if (fileLen == 0 || fileLen > 512 * 1024)  /* 限制最大 512KB */
    {
      uart_putc(NAK);
      return -1;
    }

    /* 3. 计算需要擦除的扇区范围 (4KB 对齐) */
    uint32_t sectorStart = flashAddr & ~(W25Q64_SECTOR_SIZE - 1);
    uint32_t sectorEnd   = (flashAddr + fileLen + W25Q64_SECTOR_SIZE - 1)
                           & ~(W25Q64_SECTOR_SIZE - 1);

    /* ACK1 — 长度确认 */
    uart_putc(ACK);

    /* 清除 Block Protect, 确保扇区可写 */
    W25Q64_DisableProtection();

    /* 擦除扇区 */
    if (erase_range(sectorStart, sectorEnd) != 0)
    {
        uart_putc(ERR);
        return -2;
    }

    /* ACK2 — 擦除完毕, 准备接收数据 */
    uart_putc(ACK);

    /* 4. 接收数据块并写入 W25Q64 */

    bytesRemaining = fileLen;
    writeAddr = flashAddr;

    while (bytesRemaining > 0)
    {
        uint16_t chunk = (bytesRemaining >= BLOCK_SIZE)
                         ? BLOCK_SIZE
                         : (uint16_t)bytesRemaining;

        /* 排空 RX FIFO (清除 TX/RX 短路导致的 ACK 回声) */
        while (!DL_UART_isRXFIFOEmpty(UART0_INST))
            DL_UART_receiveData(UART0_INST);

        /* 从 UART 接收一块数据 */
        for (uint16_t i = 0; i < chunk; i++)
            blockBuf[i] = uart_getc();

        /* 写入 W25Q64 (边写边擦, 确保正确) */
        if (W25Q64_Write(writeAddr, blockBuf, chunk, 1) != W25Q64_OK)
        {
            uart_putc(ERR);
            return -2;
        }

        /* 每块回复 ACK */
        uart_putc(ACK);

        writeAddr += chunk;
        bytesRemaining -= chunk;
    }

    /* 6. 通知 PC 写入完成 */
    uart_putc(DONE);

    /* 7. 接收 CRC 并验证 */
    uint32_t expectedCRC = recv_u32();

		/* 从 W25Q64 重新读出数据, 计算 CRC */
		#define CRC_BUF_SIZE 256
		uint8_t crcBuf[CRC_BUF_SIZE];
		uint32_t crc = 0xFFFFFFFFUL;
		uint32_t addr = flashAddr;
		uint32_t remaining = fileLen;

		while (remaining > 0)
		{
				uint16_t chunk = (remaining >= CRC_BUF_SIZE) ? CRC_BUF_SIZE : (uint16_t)remaining;
				W25Q64_ReadData(addr, crcBuf, chunk);
				for (uint16_t i = 0; i < chunk; i++)
				{
						crc ^= crcBuf[i];
						for (uint8_t b = 0; b < 8; b++)
						{
								if (crc & 1)
										crc = (crc >> 1) ^ 0xEDB88320UL;
								else
										crc >>= 1;
						}
				}
				addr += chunk;
				remaining -= chunk;
		}
		crc ^= 0xFFFFFFFFUL;

    if (crc == expectedCRC)
    {
        uart_putc(ACK);
        return 0;
    }
    else
    {
        /* 失败: NAK + MCU 自己算的 CRC (供 PC 对比) */
        uart_putc(NAK);
        send_u32(crc);
        return -1;
    }
}


/* ============================================================
 * FontReceiver_VerifyFlash — 独立校验 W25Q64 中字库 CRC
 * 不重新下载, 只让 MCU 读 Flash 算 CRC 发给 PC 比对。
 *
 * 协议:
 *   PC → MCU:  0x56 ('V') 校验命令
 *   PC → MCU:  flashAddr 4B (小端) — W25Q64 起始地址
 *   PC → MCU:  fileLen 4B (小端)   — 校验长度
 *   MCU → PC:  CRC32 4B (小端)     — 计算出的 CRC 值
 *
 * 调用示例 (PC 端):
 *   ser.write(b'V')
 *   ser.write(struct.pack('<I', flashAddr))
 *   ser.write(struct.pack('<I', fileLen))
 *   mcu_crc = struct.unpack('<I', ser.read(4))[0]
 * ============================================================ */
void FontReceiver_VerifyFlash(void)
{
    /* 已由 main loop 收到 'V' 命令 */
    /* 继续接收地址和长度 */
    uint32_t verifyAddr = recv_u32();
    uint32_t verifyLen  = recv_u32();

    if (verifyLen == 0 || verifyLen > 1024 * 1024)
    {
        send_u32(0xFFFFFFFFUL);
        return;
    }

/* 从 W25Q64 读出数据并计算 CRC */
#define CRC_BUF_SIZE 256
uint8_t crcBuf[CRC_BUF_SIZE];
uint32_t crc = 0xFFFFFFFFUL;
uint32_t addr = verifyAddr;
uint32_t remaining = verifyLen;

while (remaining > 0)
{
    uint16_t chunk = (remaining >= CRC_BUF_SIZE) ? CRC_BUF_SIZE : (uint16_t)remaining;
    W25Q64_ReadData(addr, crcBuf, chunk);
    for (uint16_t i = 0; i < chunk; i++)
    {
        crc ^= crcBuf[i];
        for (uint8_t b = 0; b < 8; b++)
        {
            if (crc & 1)
                crc = (crc >> 1) ^ 0xEDB88320UL;
            else
                crc >>= 1;
        }
    }
    addr += chunk;
    remaining -= chunk;
}
crc ^= 0xFFFFFFFFUL;
    send_u32(crc);
}
