/*
 * oled.c — OLED 128x64 I2C 驱动 (SH1106/SSD1306 兼容)
 *
 * 显示引擎:
 *   - 全角 16x16 汉字 (从 W25Q64 GB2312 字库读取)
 *   - ASCII 字符自动映射为全角显示
 *   - I2C 通信使用 GPIO 软件模拟 (bit-bang)
 *
 * 显示布局:
 *   4 行 x 8 列 = 32 个汉字 (每字 16x16 像素)
 *   底层使用 OLED 页模式: 每行 2 页 (page), 共 8 页
 *
 * 依赖:
 *   - w25q64.h   (从 W25Q64 读取 GB2312 字模)
 *   - delay.h    (毫秒延时)
 *   - ti_msp_dl_config.h (引脚宏定义)
 *
 * 引脚连接 (SysConfig):
 *   SCL: GPIOB.2
 *   SDA: GPIOB.3
 *   OLED I2C 地址: 0x78 (7-bit: 0x3C)
 */

#include "ti_msp_dl_config.h"
#include "delay.h"
#include "w25q64.h"
#include "OLED.h"

/* GB2312 字库在 W25Q64 中的基地址 (可由 OLED_SetFontFlashBase 修改) */
static uint32_t g_fontFlashAddr = GB2312_FONT_FLASH_ADDR;

#define GPIO_PIN_SET    1
#define GPIO_PIN_RESET  0

/* ============================================================
 * GPIO 软件 I2C 引脚操作
 * ============================================================ */

/**
 * @brief  GPIO 写入, 带 1 us 延时保证 I2C 时序
 */
static void GPIO_writePins(GPIO_Regs* gpio, uint32_t pins, uint8_t I)
{
    if (I) DL_GPIO_setPins(gpio, pins);
    else   DL_GPIO_clearPins(gpio, pins);
    delay_cycles(32);  /* ~1 us @ 32 MHz */
}

/* I2C 引脚宏 */
#define OLED_W_SCL(x)   GPIO_writePins(OLED_PORT, OLED_SCL_PIN, x)
#define OLED_W_SDA(x)   GPIO_writePins(OLED_PORT, OLED_SDA_PIN, x)

/* ============================================================
 * I2C 初始化
 * ============================================================ */
static void OLED_I2C_Init(void)
{
    OLED_W_SCL(GPIO_PIN_SET);
    OLED_W_SDA(GPIO_PIN_SET);
}

/**
 * @brief  I2C 起始条件: SDA 在 SCL 高电平时拉低
 */
static void OLED_I2C_Start(void)
{
    OLED_W_SDA(GPIO_PIN_SET);
    OLED_W_SCL(GPIO_PIN_SET);
    OLED_W_SDA(GPIO_PIN_RESET);
    OLED_W_SCL(GPIO_PIN_RESET);
}

/**
 * @brief  I2C 停止条件: SDA 在 SCL 高电平时拉高
 */
static void OLED_I2C_Stop(void)
{
    OLED_W_SDA(GPIO_PIN_RESET);
    OLED_W_SCL(GPIO_PIN_SET);
    OLED_W_SDA(GPIO_PIN_SET);
}

/**
 * @brief  I2C 发送一个字节 (MSB first)
 * @note   不检查应答信号, 每个字节后额外给一个时钟
 */
static void OLED_I2C_SendByte(uint8_t Byte)
{
    for (uint8_t i = 0; i < 8; i++)
    {
        OLED_W_SDA((Byte & (0x80 >> i)) ? GPIO_PIN_SET : GPIO_PIN_RESET);
        OLED_W_SCL(GPIO_PIN_SET);
        OLED_W_SCL(GPIO_PIN_RESET);
    }
    /* 额外时钟: 不处理应答信号 */
    OLED_W_SCL(GPIO_PIN_SET);
    OLED_W_SCL(GPIO_PIN_RESET);
}

/* ============================================================
 * OLED 底层命令/数据写入
 * ============================================================ */

/**
 * @brief  向 OLED 写入命令字节
 * @param  Command  命令码
 */
static void OLED_WriteCommand(uint8_t Command)
{
    OLED_I2C_Start();
    OLED_I2C_SendByte(0x78);    /* 从机地址 (0x3C << 1) + 写 */
    OLED_I2C_SendByte(0x00);    /* Co=0, D/C#=0 → 命令模式 */
    OLED_I2C_SendByte(Command);
    OLED_I2C_Stop();
}

/**
 * @brief  向 OLED 写入数据字节
 * @param  Data  显示数据
 */
static void OLED_WriteData(uint8_t Data)
{
    OLED_I2C_Start();
    OLED_I2C_SendByte(0x78);    /* 从机地址 + 写 */
    OLED_I2C_SendByte(0x40);    /* Co=0, D/C#=1 → 数据模式 */
    OLED_I2C_SendByte(Data);
    OLED_I2C_Stop();
}

/**
 * @brief  设置 OLED 光标位置 (页 + 列)
 *
 * @param Y  页号 (0~7, 每页 8 像素高, 128x64 共 8 页)
 * @param X  列号 (0~127)
 */
static void OLED_SetCursor(uint8_t Y, uint8_t X)
{
    OLED_WriteCommand(0xB0 | Y);                   /* 设置页地址 */
    OLED_WriteCommand(0x10 | ((X & 0xF0) >> 4));   /* 设置列高 4 位 */
    OLED_WriteCommand(0x00 | (X & 0x0F));          /* 设置列低 4 位 */
}

/* ============================================================
 * 清屏 / 初始化
 * ============================================================ */

/**
 * @brief  清空 OLED 屏幕 (全黑)
 */
void OLED_Clear(void)
{
    for (uint8_t j = 0; j < 8; j++)
    {
        OLED_SetCursor(j, 0);
        for (uint8_t i = 0; i < 128; i++)
            OLED_WriteData(0x00);
    }
}

/**
 * @brief  OLED 初始化 (SH1106/SSD1306 标准初始化序列)
 *
 * 配置:
 *   - 128x64 分辨率
 *   - I2C 地址 0x3C
 *   内部充电泵使能
 */
void OLED_Init(void)
{
    delay(100);

    OLED_I2C_Init();

    OLED_WriteCommand(0xAE);    /* 关闭显示 */

    OLED_WriteCommand(0xD5);    /* 设置显示时钟分频比/振荡器频率 */
    OLED_WriteCommand(0x80);

    OLED_WriteCommand(0xA8);    /* 设置多路复用率 */
    OLED_WriteCommand(0x3F);

    OLED_WriteCommand(0xD3);    /* 设置显示偏移 */
    OLED_WriteCommand(0x00);

    OLED_WriteCommand(0x40);    /* 设置显示开始行 */

    OLED_WriteCommand(0xA1);    /* 设置左右方向: 0xA1=正常, 0xA0=左右反置 */
    OLED_WriteCommand(0xC8);    /* 设置上下方向: 0xC8=正常, 0xC0=上下反置 */

    OLED_WriteCommand(0xDA);    /* 设置 COM 引脚硬件配置 */
    OLED_WriteCommand(0x12);

    OLED_WriteCommand(0x81);    /* 设置对比度控制 */
    OLED_WriteCommand(0xCF);

    OLED_WriteCommand(0xD9);    /* 设置预充电周期 */
    OLED_WriteCommand(0xF1);

    OLED_WriteCommand(0xDB);    /* 设置 VCOMH 取消选择级别 */
    OLED_WriteCommand(0x30);

    OLED_WriteCommand(0xA4);    /* 设置整个显示打开/关闭 */

    OLED_WriteCommand(0xA6);    /* 设置正常/反转显示 (0xA6=正常, 0xA7=反转) */

    OLED_WriteCommand(0x8D);    /* 设置充电泵 */
    OLED_WriteCommand(0x14);    /* 使能充电泵 */

    OLED_WriteCommand(0xAF);    /* 开启显示 */

    OLED_Clear();               /* 清屏 */
}

/* ============================================================
 * ASCII → GB2312 全角字符映射
 *
 * GB2312 第 3 区 (0xA3) 包含全角符号/字母/数字
 *   0xA3A1 ~ 0xA3FE 对应 ASCII 0x21 ~ 0x7E
 *   空格 (0x20) 映射到 0xA1A1 (表意空格)
 * ============================================================ */
static uint16_t ASCII_to_GB2312_FullWidth(char c)
{
    if (c == ' ')
        return 0xA1A1;
    if (c >= 0x21 && c <= 0x7E)
        return (0xA3UL << 8) | (uint8_t)(c - 0x21 + 0xA1);
    return 0xA1A1;
}

/* ============================================================
 * 字符/字符串显示 (全角 16x16)
 * ============================================================ */

/**
 * @brief  显示一个 ASCII 字符 (自动转为全角 16x16)
 *
 * @param Line    行 (1~4)
 * @param Column  列 (1~8, 每字 16 像素宽)
 * @param Char    要显示的 ASCII 字符
 */
void OLED_ShowChar(uint8_t Line, uint8_t Column, char Char)
{
    if (Column < 1 || Column > 8) return;
    if (Line   < 1 || Line   > 4) return;

    uint16_t gbCode = ASCII_to_GB2312_FullWidth(Char);
    OLED_ShowChinese(Line, Column, gbCode);
}

/**
 * @brief  显示字符串 (全角 16x16)
 *
 * @param Line    起始行 (1~4)
 * @param Column  起始列 (1~8)
 * @param String  以 \0 结尾的 ASCII 字符串
 */
void OLED_ShowString(uint8_t Line, uint8_t Column, char *String)
{
    for (uint8_t i = 0; String[i] != '\0'; i++)
    {
        if (Column + i > 8) break;  /* 防溢出 */
        OLED_ShowChar(Line, Column + i, String[i]);
    }
}

/* 内置次方函数 */
static uint32_t OLED_Pow(uint32_t X, uint32_t Y)
{
    uint32_t Result = 1;
    while (Y--)
        Result *= X;
    return Result;
}

/**
 * @brief  显示无符号十进制数
 *
 * @param Line    起始行 (1~4)
 * @param Column  起始列 (1~8)
 * @param Number  要显示的数字 (0~4294967295)
 * @param Length  显示位数 (1~10), 不足前补空格
 */
void OLED_ShowNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
    for (uint8_t i = 0; i < Length; i++)
        OLED_ShowChar(Line, Column + i, Number / OLED_Pow(10, Length - i - 1) % 10 + '0');
}

/**
 * @brief  显示有符号十进制数
 *
 * @param Line    起始行 (1~4)
 * @param Column  起始列 (1~8)
 * @param Number  要显示的数字 (-2147483648~2147483647)
 * @param Length  显示位数 (1~10)
 */
void OLED_ShowSignedNum(uint8_t Line, uint8_t Column, int32_t Number, uint8_t Length)
{
    uint32_t Number1;
    if (Number >= 0)
    {
        OLED_ShowChar(Line, Column, '+');
        Number1 = Number;
    }
    else
    {
        OLED_ShowChar(Line, Column, '-');
        Number1 = -Number;
    }
    for (uint8_t i = 0; i < Length; i++)
        OLED_ShowChar(Line, Column + i + 1, Number1 / OLED_Pow(10, Length - i - 1) % 10 + '0');
}

/**
 * @brief  显示十六进制数
 *
 * @param Line    起始行 (1~4)
 * @param Column  起始列 (1~8)
 * @param Number  要显示的数字 (0~0xFFFFFFFF)
 * @param Length  显示位数 (1~8)
 */
void OLED_ShowHexNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
    for (uint8_t i = 0; i < Length; i++)
    {
        uint8_t SingleNumber = Number / OLED_Pow(16, Length - i - 1) % 16;
        if (SingleNumber < 10)
            OLED_ShowChar(Line, Column + i, SingleNumber + '0');
        else
            OLED_ShowChar(Line, Column + i, SingleNumber - 10 + 'A');
    }
}

/**
 * @brief  显示二进制数
 *
 * @param Line    起始行 (1~4)
 * @param Column  起始列 (1~8)
 * @param Number  要显示的数字 (0~0xFFFF)
 * @param Length  显示位数 (1~16)
 */
void OLED_ShowBinNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
    for (uint8_t i = 0; i < Length; i++)
        OLED_ShowChar(Line, Column + i, Number / OLED_Pow(2, Length - i - 1) % 2 + '0');
}

/* ============================================================
 * GB2312 中文显示 (从 W25Q64 字库读取 16x16 字模)
 * ============================================================ */

/**
 * @brief  设置 GB2312 字库在 W25Q64 中的基地址
 *
 * @param addr  字库起始地址 (通常为 0, 需 4 KB 扇区对齐)
 */
void OLED_SetFontFlashBase(uint32_t addr)
{
    g_fontFlashAddr = addr;
}

/**
 * @brief  内部函数: GB2312 双字节编码 -> 字库文件偏移
 *
 * HZK16 排列: 第 01 区第 01 位开始, 依次 94 位后进入下一区
 * 每字符 32 字节 (16 行 x 2 字节)
 *
 * @param  gbCode  GB2312 编码 (双字节, hi 为区码, lo 为位码)
 * @return 文件偏移, -1 表示编码无效
 */
static int32_t GB2312_to_offset(uint16_t gbCode)
{
    uint8_t hi = (uint8_t)(gbCode >> 8);
    uint8_t lo = (uint8_t)(gbCode & 0xFF);

    /* GB2312 编码范围检查: 区码 A1-F7, 位码 A1-FE */
    if (hi < 0xA1 || hi > 0xF7) return -1;
    if (lo < 0xA1 || lo > 0xFE) return -1;

    uint16_t zone = hi - 0xA1;          /* 区码 0..86 */
    uint16_t pos  = lo - 0xA1;          /* 位码 0..93 */

    return (int32_t)(zone * GB2312_ZONE_COUNT + pos) * GB2312_BYTES_PER_CHAR;
}

/**
 * @brief  从 W25Q64 读取一个 GB2312 字符的 16x16 字模
 *
 * @param  gbCode  GB2312 编码
 * @param  bitmap  输出缓冲区, 至少 32 字节
 * @return 0=成功, -1=编码无效
 */
int8_t OLED_GetGB2312Bitmap(uint16_t gbCode, uint8_t *bitmap)
{
    int32_t offset = GB2312_to_offset(gbCode);
    if (offset < 0) return -1;

    uint32_t flashAddr = g_fontFlashAddr + (uint32_t)offset;
    W25Q64_ReadData(flashAddr, bitmap, GB2312_BYTES_PER_CHAR);

    return 0;
}

/* ============================================================
 * 内部函数: 将 HZK16 字模绘制到 OLED (16x16 像素)
 *
 * HZK16 格式: bitmap[0..31]
 *   - 每行 2 字节 (左 byte: column 0-7, 右 byte: column 8-15)
 *   - 共 16 行, 每行 2 字节 = 32 字节
 *
 * OLED 页模式:
 *   - 每 page 8 像素高
 *   - 16 行 = 2 page: pageY (上半), pageY+1 (下半)
 *   - 每 page 内, 垂直方向 bit r 对应 row r
 *
 * @param pageY  起始页 (0~6 偶数, 对应上半部分)
 * @param colX   起始列 (0~112, 16 的倍数)
 * @param bitmap HZK16 字模数据 (32 字节)
 * ============================================================ */
static void OLED_Draw16x16(uint8_t pageY, uint8_t colX, const uint8_t *bitmap)
{
    uint8_t colBuf[16];

    /* --- 上半部分 (rows 0-7, 对应 OLED pageY) --- */
    for (uint8_t c = 0; c < 16; c++)
    {
        uint8_t byteVal = 0;
        for (uint8_t r = 0; r < 8; r++)
        {
            uint8_t pixel;
            if (c < 8)
                pixel = (bitmap[r * 2] >> (7 - c)) & 1;
            else
                pixel = (bitmap[r * 2 + 1] >> (15 - c)) & 1;
            if (pixel)
                byteVal |= (1 << r);   /* OLED page: bit r = row r */
        }
        colBuf[c] = byteVal;
    }

    OLED_SetCursor(pageY, colX);
    for (uint8_t c = 0; c < 16; c++)
        OLED_WriteData(colBuf[c]);

    /* --- 下半部分 (rows 8-15, 对应 OLED pageY+1) --- */
    for (uint8_t c = 0; c < 16; c++)
    {
        uint8_t byteVal = 0;
        for (uint8_t r = 0; r < 8; r++)
        {
            uint8_t sr = r + 8;  /* row index 8..15 */
            uint8_t pixel;
            if (c < 8)
                pixel = (bitmap[sr * 2] >> (7 - c)) & 1;
            else
                pixel = (bitmap[sr * 2 + 1] >> (15 - c)) & 1;
            if (pixel)
                byteVal |= (1 << r);
        }
        colBuf[c] = byteVal;
    }

    OLED_SetCursor(pageY + 1, colX);
    for (uint8_t c = 0; c < 16; c++)
        OLED_WriteData(colBuf[c]);
}

/**
 * @brief  在指定位置显示一个 GB2312 汉字 (16x16 像素)
 *
 * @param Line    行 (1~4)
 * @param Column  列 (1~8)
 * @param gbCode  GB2312 编码 (如 0xC4E3 = "你")
 * @return 0=成功, -1=参数错误/字符不存在
 */
int8_t OLED_ShowChinese(uint8_t Line, uint8_t Column, uint16_t gbCode)
{
    /* 转内部 0 起点坐标 */
    uint8_t line = Line - 1;
    uint8_t col  = Column - 1;

    /* 边界检查 */
    if (line >= 4 || col >= 8) return -1;

    uint8_t bitmap[32];
    int8_t ret = OLED_GetGB2312Bitmap(gbCode, bitmap);
    if (ret != 0) return ret;

    /* OLED 坐标: 每行 2 页, 每字 16 列 */
    uint8_t pageY = line * 2;
    uint8_t colX  = col * 16;

    OLED_Draw16x16(pageY, colX, bitmap);
    return 0;
}

/**
 * @brief  显示 GB2312 中文字符串 (双字节编码, \0 结束)
 *
 * @param Line    起始行 (1~4)
 * @param Column  起始列 (1~8)
 * @param str     GB2312 编码的字符串, 每字符 2 字节, \0 结尾
 *
 * 注意: 字符串必须是 GB2312 双字节编码, 不能混入 ASCII
 */
void OLED_ShowChineseString(uint8_t Line, uint8_t Column, const uint8_t *str)
{
    uint8_t col = Column;

    while (*str != 0)
    {
        uint16_t gbCode = ((uint16_t)str[0] << 8) | str[1];

        /* 边界检查 */
        if (col > 8) break;

        OLED_ShowChinese(Line, col, gbCode);
        str += 2;
        col++;
    }
}

/**
 * @brief  显示中英文混合字符串
 *
 * 自动识别:
 *   - ASCII (首字节 < 0xA1) 单字节, 占 1 个半角列 (显示为全角)
 *   - GB2312 (首字节 >= 0xA1) 双字节, 占 2 个半角列
 *
 * @param Line    起始行 (1~4)
 * @param Column  起始半角列 (1~16, 每个半角 8 像素)
 * @param str     中英文混合字符串
 */
void OLED_ShowMixedString(uint8_t Line, uint8_t Column, const uint8_t *str)
{
    uint8_t col = Column - 1;  /* 转内部 0 起点 (半角单位) */

    while (*str != 0)
    {
        if (col >= 16) break;

        if (*str < 0xA1)
        {
            /* ASCII 字符: 显示为全角, 占 2 个半角列 */
            OLED_ShowChar(Line, (col / 2) + 1, (char)*str);
            str++;
            col += 2;
        }
        else
        {
            /* GB2312 双字节汉字 */
            uint16_t gbCode = ((uint16_t)str[0] << 8) | str[1];
            OLED_ShowChinese(Line, (col / 2) + 1, gbCode);
            str += 2;
            col += 2;
        }
    }
}
