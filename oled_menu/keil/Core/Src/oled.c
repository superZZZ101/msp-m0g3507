#include "ti_msp_dl_config.h"
#include "OLED_Font.h"
#include "delay.h"
#include "w25q64.h"
#include "OLED.h"

/* GB2312 字库在 W25Q64 中的基地址（可由 OLED_SetFontFlashBase 修改）*/
static uint32_t g_fontFlashAddr = GB2312_FONT_FLASH_ADDR;

#define GPIO_PIN_SET	1
#define GPIO_PIN_RESET	0

void GPIO_writePins(GPIO_Regs* gpio, uint32_t pins,uint8_t I){
	if(I) DL_GPIO_setPins(gpio,pins);
	else DL_GPIO_clearPins(gpio,pins);
	delay_cycles(32); 
}

/*引脚配置*/
#define OLED_W_SCL(x)		GPIO_writePins(OLED_PORT,OLED_SCL_PIN, x)
#define OLED_W_SDA(x)		GPIO_writePins(OLED_PORT,OLED_SDA_PIN, x)

/*引脚初始化*/
void OLED_I2C_Init(void)
{	

	OLED_W_SCL(GPIO_PIN_SET);

	OLED_W_SDA(GPIO_PIN_SET);
	
}

/**
  * @brief  I2C开始
  * @param  无
  * @retval 无
  */
void OLED_I2C_Start(void)
{
	OLED_W_SDA(GPIO_PIN_SET);
	OLED_W_SCL(GPIO_PIN_SET);
	OLED_W_SDA(GPIO_PIN_RESET);
	OLED_W_SCL(GPIO_PIN_RESET);
}

/**
  * @brief  I2C停止
  * @param  无
  * @retval 无
  */
void OLED_I2C_Stop(void)
{
	OLED_W_SDA(GPIO_PIN_RESET);
	OLED_W_SCL(GPIO_PIN_SET);
	OLED_W_SDA(GPIO_PIN_SET);
}

/**
  * @brief  I2C发送一个字节
  * @param  Byte 要发送的一个字节
  * @retval 无
  */
void OLED_I2C_SendByte(uint8_t Byte)
{
	uint8_t i;
	for (i = 0; i < 8; i++)
	{
	    OLED_W_SDA((Byte & (0x80 >> i)) ? GPIO_PIN_SET : GPIO_PIN_RESET);
		OLED_W_SCL(GPIO_PIN_SET);
		OLED_W_SCL(GPIO_PIN_RESET);
	}
	OLED_W_SCL(GPIO_PIN_SET);	//额外的一个时钟，不处理应答信号
	OLED_W_SCL(GPIO_PIN_RESET);
}

/**
  * @brief  OLED写命令
  * @param  Command 要写入的命令
  * @retval 无
  */
void OLED_WriteCommand(uint8_t Command)
{
	OLED_I2C_Start();
	OLED_I2C_SendByte(0x78);		//从机地址
	OLED_I2C_SendByte(0x00);		//写命令
	OLED_I2C_SendByte(Command); 
	OLED_I2C_Stop();
}

/**
  * @brief  OLED写数据
  * @param  Data 要写入的数据
  * @retval 无
  */
void OLED_WriteData(uint8_t Data)
{
	OLED_I2C_Start();
	OLED_I2C_SendByte(0x78);		//从机地址
	OLED_I2C_SendByte(0x40);		//写数据
	OLED_I2C_SendByte(Data);
	OLED_I2C_Stop();
}

/**
  * @brief  OLED设置光标位置
  * @param  Y 以左上角为原点，向下方向的坐标，范围：0~7
  * @param  X 以左上角为原点，向右方向的坐标，范围：0~127
  * @retval 无
  */
void OLED_SetCursor(uint8_t Y, uint8_t X)
{
	OLED_WriteCommand(0xB0 | Y);					//设置Y位置
	OLED_WriteCommand(0x10 | ((X & 0xF0) >> 4));	//设置X位置高4位
	OLED_WriteCommand(0x00 | (X & 0x0F));			//设置X位置低4位
}

/**
  * @brief  OLED清屏
  * @param  无
  * @retval 无
  */
void OLED_Clear(void)
{  
	uint8_t i, j;
	for (j = 0; j < 8; j++)
	{
		OLED_SetCursor(j, 0);
		for(i = 0; i < 128; i++)
		{
			OLED_WriteData(0x00);
		}
	}
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

/**
  * @brief  OLED显示一个字符 (全角 16×16, 从 W25Q64 字库取模)
  * @param  Line   行位置，范围：1~4
  * @param  Column 列位置，范围：1~8 (每字 16 像素)
  * @param  Char   要显示的字符，ASCII可见字符
  * @retval 无
  */
void OLED_ShowChar(uint8_t Line, uint8_t Column, char Char)
{
    if (Column < 1 || Column > 8) return;
    if (Line   < 1 || Line   > 4) return;

    uint16_t gbCode = ASCII_to_GB2312_FullWidth(Char);
    OLED_ShowChinese(Line, Column, gbCode);
}

/**
  * @brief  OLED显示字符串 (全角 16×16, 每字 16 像素)
  * @param  Line   起始行位置，范围：1~4
  * @param  Column 起始列位置，范围：1~8
  * @param  String 要显示的字符串，范围：ASCII可见字符
  * @retval 无
  */
void OLED_ShowString(uint8_t Line, uint8_t Column, char *String)
{
    uint8_t i;
    for (i = 0; String[i] != '\0'; i++)
    {
        if (Column + i > 8) break;  /* 防溢出 */
        OLED_ShowChar(Line, Column + i, String[i]);
    }
}

/**
  * @brief  OLED次方函数
  * @retval 返回值等于X的Y次方
  */
uint32_t OLED_Pow(uint32_t X, uint32_t Y)
{
	uint32_t Result = 1;
	while (Y--)
	{
		Result *= X;
	}
	return Result;
}

/**
  * @brief  OLED显示数字（十进制，正数）
  * @param  Line 起始行位置，范围：1~4
  * @param  Column 起始列位置，范围：1~16
  * @param  Number 要显示的数字，范围：0~4294967295
  * @param  Length 要显示数字的长度，范围：1~10
  * @retval 无
  */
void OLED_ShowNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
	uint8_t i;
	for (i = 0; i < Length; i++)							
	{
		OLED_ShowChar(Line, Column + i, Number / OLED_Pow(10, Length - i - 1) % 10 + '0');
	}
}

/**
  * @brief  OLED显示数字（十进制，带符号数）
  * @param  Line 起始行位置，范围：1~4
  * @param  Column 起始列位置，范围：1~16
  * @param  Number 要显示的数字，范围：-2147483648~2147483647
  * @param  Length 要显示数字的长度，范围：1~10
  * @retval 无
  */
void OLED_ShowSignedNum(uint8_t Line, uint8_t Column, int32_t Number, uint8_t Length)
{
	uint8_t i;
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
	for (i = 0; i < Length; i++)							
	{
		OLED_ShowChar(Line, Column + i + 1, Number1 / OLED_Pow(10, Length - i - 1) % 10 + '0');
	}
}

/**
  * @brief  OLED显示数字（十六进制，正数）
  * @param  Line 起始行位置，范围：1~4
  * @param  Column 起始列位置，范围：1~16
  * @param  Number 要显示的数字，范围：0~0xFFFFFFFF
  * @param  Length 要显示数字的长度，范围：1~8
  * @retval 无
  */
void OLED_ShowHexNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
	uint8_t i, SingleNumber;
	for (i = 0; i < Length; i++)							
	{
		SingleNumber = Number / OLED_Pow(16, Length - i - 1) % 16;
		if (SingleNumber < 10)
		{
			OLED_ShowChar(Line, Column + i, SingleNumber + '0');
		}
		else
		{
			OLED_ShowChar(Line, Column + i, SingleNumber - 10 + 'A');
		}
	}
}

/**
  * @brief  OLED显示数字（二进制，正数）
  * @param  Line 起始行位置，范围：1~4
  * @param  Column 起始列位置，范围：1~16
  * @param  Number 要显示的数字，范围：0~1111 1111 1111 1111
  * @param  Length 要显示数字的长度，范围：1~16
  * @retval 无
  */
void OLED_ShowBinNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
	uint8_t i;
	for (i = 0; i < Length; i++)							
	{
		OLED_ShowChar(Line, Column + i, Number / OLED_Pow(2, Length - i - 1) % 2 + '0');
	}
}

/**
  * @brief  OLED初始化
  * @param  无
  * @retval 无
  */
void OLED_Init(void)
{

	
	delay(100);
	
	OLED_I2C_Init();			//端口初始化
	
	OLED_WriteCommand(0xAE);	//关闭显示
	
	OLED_WriteCommand(0xD5);	//设置显示时钟分频比/振荡器频率
	OLED_WriteCommand(0x80);
	
	OLED_WriteCommand(0xA8);	//设置多路复用率
	OLED_WriteCommand(0x3F);
	
	OLED_WriteCommand(0xD3);	//设置显示偏移
	OLED_WriteCommand(0x00);
	
	OLED_WriteCommand(0x40);	//设置显示开始行
	
	OLED_WriteCommand(0xA1);	//设置左右方向，0xA1正常 0xA0左右反置
	
	OLED_WriteCommand(0xC8);	//设置上下方向，0xC8正常 0xC0上下反置

	OLED_WriteCommand(0xDA);	//设置COM引脚硬件配置
	OLED_WriteCommand(0x12);
	
	OLED_WriteCommand(0x81);	//设置对比度控制
	OLED_WriteCommand(0xCF);

	OLED_WriteCommand(0xD9);	//设置预充电周期
	OLED_WriteCommand(0xF1);

	OLED_WriteCommand(0xDB);	//设置VCOMH取消选择级别
	OLED_WriteCommand(0x30);

	OLED_WriteCommand(0xA4);	//设置整个显示打开/关闭

	OLED_WriteCommand(0xA6);	//设置正常/倒转显示

	OLED_WriteCommand(0x8D);	//设置充电泵
	OLED_WriteCommand(0x14);

	OLED_WriteCommand(0xAF);	//开启显示
		
	OLED_Clear();				//OLED清屏
}

/* === GB2312 中文显示 (追加) === */


void OLED_SetFontFlashBase(uint32_t addr)
{
    g_fontFlashAddr = addr;
}

/* ============================================================
 * 内部: GB2312 双字节 → 字库偏移 (HZK16 格式)
 * ============================================================ */
static int32_t GB2312_to_offset(uint16_t gbCode)
{
    uint8_t hi = (uint8_t)(gbCode >> 8);
    uint8_t lo = (uint8_t)(gbCode & 0xFF);

    /* 编码范围检查 */
    if (hi < 0xA1 || hi > 0xF7) return -1;
    if (lo < 0xA1 || lo > 0xFE) return -1;

    uint16_t zone = hi - 0xA1;          /* 区码 0..86 */
    uint16_t pos  = lo - 0xA1;          /* 位码 0..93 */

    return (int32_t)(zone * GB2312_ZONE_COUNT + pos) * GB2312_BYTES_PER_CHAR;
}

/* ============================================================
 * 读取一个 GB2312 字符的 16×16 字模
 * ============================================================ */
int8_t OLED_GetGB2312Bitmap(uint16_t gbCode, uint8_t *bitmap)
{
    int32_t offset = GB2312_to_offset(gbCode);
    if (offset < 0) return -1;

    uint32_t flashAddr = g_fontFlashAddr + (uint32_t)offset;
    W25Q64_ReadData(flashAddr, bitmap, GB2312_BYTES_PER_CHAR);

    return 0;
}

/* ============================================================
 * 内部: 将 HZK16 行扫描字模转为 OLED 页列字节并写入
 *
 * HZK16: bitmap[0..31], 每行 2 字节 (col 0-7, col 8-15)
 * OLED page[Y] 对应 bitmap 中的 rows[Y*8 .. Y*8+7]
 * 在列 col 处: 垂直 8 个 bit 组成一个字节
 * ============================================================ */
static void OLED_Draw16x16(uint8_t pageY, uint8_t colX, const uint8_t *bitmap)
{
    uint8_t colBuf[16];  /* 上半部分 16 列 */

    /* 上半部分 (rows 0-7, 对应 OLED pageY) */
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
                byteVal |= (1 << r);   /* OLED: bit r = row r in this page */
        }
        colBuf[c] = byteVal;
    }

    /* 写上半部分 */
    OLED_SetCursor(pageY, colX);
    for (uint8_t c = 0; c < 16; c++)
        OLED_WriteData(colBuf[c]);

    /* 下半部分 (rows 8-15, 对应 OLED pageY+1) */
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

/* ============================================================
 * 显示一个 GB2312 中文 (16×16 像素)
 *   Line:   1-4 (每行 16 像素高)
 *   Column: 1-8 (每个汉字 16 像素宽, OLED 128 宽 ÷ 16 = 8)
 * ============================================================ */
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

/* ============================================================
 * 显示 GB2312 中文字符串 (双字节, \0 结束)
 *   Line: 1-4,  Column: 1-8
 * ============================================================ */
void OLED_ShowChineseString(uint8_t Line, uint8_t Column, const uint8_t *str)
{
    uint8_t col = Column;

    while (*str != 0)
    {
        uint16_t gbCode = ((uint16_t)str[0] << 8) | str[1];

        /* 边界检查 (1-8) */
        if (col > 8) break;

        OLED_ShowChinese(Line, col, gbCode);
        str += 2;
        col++;
    }
}

/* ============================================================
 * 显示中英文混合字符串
 *   Line: 1-4,  Column: 1-16 (半角列, 每半角 8 像素)
 *   ASCII (首字节 < 0xA1) 单字节, 占 1 列
 *   GB2312 (首字节 >= 0xA1) 双字节, 占 2 列
 * ============================================================ */
void OLED_ShowMixedString(uint8_t Line, uint8_t Column, const uint8_t *str)
{
    uint8_t col = Column - 1;  /* 转内部 0 起点 */

    while (*str != 0)
    {
        if (col >= 16) break;

        if (*str < 0xA1)
        {
            /* ASCII 字符 */
            OLED_ShowChar(Line, (col / 2) + 1, (char)*str);
            str++;
            col++;
        }
        else
        {
            /* GB2312 双字节 */
            uint16_t gbCode = ((uint16_t)str[0] << 8) | str[1];
            OLED_ShowChinese(Line, (col / 2) + 1, gbCode);
            str += 2;
            col += 2;
        }
    }
}
