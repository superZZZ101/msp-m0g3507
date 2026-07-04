# W25Q64 + GB2312 字库 OLED 显示方案 (MSPM0G3507)

## 项目概述

本项目基于 **TI MSPM0G3507** MCU，通过 **GPIO 模拟 SPI** 驱动 **W25Q64 (8MB NOR Flash)** 存储 **GB2312 16×16 点阵中文字库**，在 **128×64 I2C OLED** 上显示中文。

支持 4 行 × 8 列 = 32 个汉字同时显示，ASCII 字符自动映射为全角显示。

---

## 目录结构

```
w25q64-gb2312/
├── empty.c                    # 主函数入口 (TI SDK 工程)
├── w25q64.c / w25q64.h        # W25Q64 SPI 驱动 (GPIO 模拟)
├── ti_msp_dl_config.c/h       # SysConfig 生成的外设配置
├── empty.syscfg               # SysConfig 配置文件
├── Event.dot                  # 事件跟踪配置
├── readme.txt                 # 旧版说明 (已废弃)
│
├── gcc/                       # GCC 编译工程
├── iar/                       # IAR 编译工程
├── ticlang/                   # TI CLANG 编译工程
│
├── keil/                      # Keil MDK 工程 (推荐)
│   ├── empty_LP_MSPM0G3507_nortos_keil.uvprojx  # Keil 工程文件
│   ├── led.c / led.h         # LED 控制
│   ├── uart.c / uart.h       # UART 驱动 (调试输出)
│   ├── Core/
│   │   ├── Src/
│   │   │   ├── w25q64.c      # W25Q64 驱动
│   │   │   ├── oled.c        # OLED 驱动 + GB2312 中文显示
│   │   │   ├── uart.c        # UART 通信驱动
│   │   │   ├── delay.c       # 毫秒延时
│   │   │   └── ...           # 其他模块
│   │   └── Inc/
│   │       ├── w25q64.h      # W25Q64 驱动头文件 + 指令表
│   │       ├── OLED.h        # OLED API 头文件
│   │       ├── OLED_Font.h   # ASCII 8×16 字模库
│   │       ├── delay.h       # 延时函数声明
│   │       ├── uart.h        # UART 驱动头文件
│   │       ├── key.h         # 三按键驱动头文件
│   │       └── strings.h     # GB2312 字符串常量宏定义
│
└── tools/                     # 字库生成/刷写工具
    ├── gen_gb2312_font.py     # 生成 GB2312 字库 (.bin + .h)
    ├── gen_send_font.py       # 生成字库 + 通过 UART 写入 W25Q64
    ├── send_font.bat          # 一键发送脚本
    ├── verify_font.py         # 校验 W25Q64 中字库 CRC
    ├── gen_strings.py         # 生成 GB2312 字符串头文件
    ├── gb2312_hzk16.bin       # 生成的 GB2312 字库二进制文件
    ├── gb2312_font.h          # 字库 C 头文件 (extern 声明)
    └── readme.txt             # 字库工具使用说明
```

---

## 硬件连接

| W25Q64 引脚 | MCU 引脚 | 说明 |
|-------------|----------|------|
| CS          | GPIOA.27 | 片选 (低电平有效) |
| CLK         | GPIOB.25 | SPI 时钟 |
| DI (MOSI)   | GPIOB.20 | 主机输出, 从机输入 |
| DO (MISO)   | GPIOA.25 | 主机输入, 从机输出 |

| OLED 引脚 | MCU 引脚 | 说明 |
|-----------|----------|------|
| SCL       | GPIOB.2  | I2C 时钟 (GPIO 模拟) |
| SDA       | GPIOB.3  | I2C 数据 |

| UART 引脚 | MCU 引脚 | 说明 |
|-----------|----------|------|
| TX        | GPIOA.10 | UART 发送 (字库传输) |
| RX        | GPIOA.11 | UART 接收 (字库传输) |

| LED 引脚 | MCU 引脚 | 说明 |
|----------|----------|------|
| LED      | GPIOB.22 | 运行指示灯 |

> **注意:** 引脚定义由 SysConfig (`empty.syscfg`) 生成，如需修改引脚请在 SysConfig 中更改后重新生成 `ti_msp_dl_config.h/c`。

---

## 快速开始

### 第 1 步: 编译固件

**Keil MDK (推荐):**
1. 打开 `keil/empty_LP_MSPM0G3507_nortos_keil.uvprojx`
2. 编译 (F7)，下载到 MCU (F8)
3. 首次烧录后 MCU 会等待字库传输

**其他工具链:**
- GCC: `gcc/` 目录, 使用 `make` 编译
- IAR: `iar/` 目录, 打开 `.ewp` 工程文件
- TI CLANG: `ticlang/` 目录, 使用 `make` 编译

### 第 2 步: 下载字库到 W25Q64

首次使用时，需要将 GB2312 字库通过 UART 写入 W25Q64：

```bat
# 方式一: 使用批处理脚本 (推荐)
cd tools
send_font.bat COM4

# 方式二: 直接运行 Python 脚本
cd tools
python gen_send_font.py --port COM4
```

> **注意:** 将 `COM4` 替换为实际串口号。
> W25Q64 擦除需要约 28 秒，请耐心等待。
> 脚本会自动进行 CRC 校验确保数据正确。

### 第 3 步: 验证

下载完成后:
- MCU 自动在 OLED 上显示演示内容
- 可以重新上电验证字库持久保存

如果你想手动校验字库完整性:

```bat
cd tools
python verify_font.py COM4
```

---

## W25Q64 驱动 API

### 初始化与检测

```c
W25Q64_Init();                  // 初始化 SPI 引脚
uint32_t id = W25Q64_ReadID();  // 读取 JEDEC ID, 0xEF4017 = W25Q64JV
```

### 擦除操作

```c
W25Q64_EraseSector(0);          // 擦除第 0 个 4KB 扇区 (地址必须 4KB 对齐)
W25Q64_EraseBlock32K(0);        // 擦除 32KB 块
W25Q64_EraseBlock64K(0);        // 擦除 64KB 块
W25Q64_ChipErase();             // 全片擦除 (耗时 > 40 秒)
```

### 读写操作

```c
// 写入 (自动跨页, 自动扇区擦除)
uint8_t data[] = "Hello World";
W25Q64_Write(0x1000, data, sizeof(data), 1);  // doErase=1 自动擦除

// 读取
uint8_t buf[64];
W25Q64_ReadData(0x1000, buf, sizeof(buf));
```

### 底层操作

```c
W25Q64_WriteEnable();           // 写使能 (每次擦除/写入前需要)
W25Q64_WriteDisable();          // 写禁止
W25Q64_WaitBusy(1000);          // 等待芯片空闲 (超时 1 秒)
W25Q64_DisableProtection();     // 清除块保护 (出厂后需要执行一次)
```

### 注意事项

1. **NOR Flash 特性:** 只能将 1 写成 0。写入前必须先擦除 (擦除将扇区恢复为全 0xFF)
2. **地址对齐:** 扇区擦除地址必须 4KB 对齐
3. **页写入限制:** `W25Q64_WritePage()` 单次最多 256 字节且不能跨页
4. **SPI 速度:** 默认 GPIO 模拟 ~500kHz，可通过 `SPI_HALF_CYCLE_US` 调整

---

## OLED 显示 API

### 基本显示

```c
OLED_Init();                    // OLED 初始化

OLED_ShowChar(1, 1, 'A');      // 第 1 行第 1 列显示 'A' (全角)
OLED_ShowString(1, 3, "ABC");   // 从第 1 行第 3 列开始显示字符串
OLED_ShowNum(2, 1, 1234, 4);   // 显示数字: 1234 (4 位)
OLED_ShowHexNum(2, 5, 0xFF, 2); // 显示十六进制: FF
```

### 中文显示 (从 W25Q64 字库)

```c
#include "strings.h"

// 方式一: 显示单个汉字 (GB2312 编码)
OLED_ShowChinese(1, 1, 0xC4E3);  // 显示 "你"

// 方式二: 显示中文字符串
OLED_ShowChineseString(2, 1, (const uint8_t*)"\xC4\xE3\xBA\xC3");  // "你好"

// 方式三: 使用预定义字符串宏
OLED_ShowChineseString(2, 1, STR_C4E3_BAC3_CAC0_BDE7);  // "你好世界"

// 方式四: 中英文混合显示
OLED_ShowMixedString(3, 1, (const uint8_t*)"Temp: 25\xB6\xC8");  // "Temp: 25度"
```

### 显示布局

```
行号    像素位置        列 (半角单位)
                   1 2 3 4 5 6 7 8 9 ... 16
Line 1  (pages 0-1) [   第 1 行   ]
Line 2  (pages 2-3) [   第 2 行   ]
Line 3  (pages 4-5) [   第 3 行   ]
Line 4  (pages 6-7) [   第 4 行   ]
```

- 每行 2 个 OLED page (16 像素高)，共 4 行
- 每列 = 1 个汉字宽度 (16 像素)，共 8 列
- 半角模式下共 16 列 (每半角 8 像素)
- 全角函数 (ShowChar/ShowString/ShowChinese): Column 1~8
- 混合函数 (ShowMixedString): Column 1~16

### 清屏

```c
OLED_Clear();  // 清空屏幕
```

---

## 字库工具使用

### 环境要求

- Windows 系统 (依赖 GDI 渲染字体)
- Python 3.8+
- 串口线连接 MCU UART

### 常用命令

```bat
cd tools

# 1. 生成字库 (不发送)
python gen_gb2312_font.py

# 2. 生成 + 发送到 W25Q64
python gen_send_font.py --port COM4

# 3. 使用其他字体
python gen_send_font.py --port COM4 --font-name SimHei

# 4. 校验 W25Q64 中字库
python verify_font.py COM4

# 5. 生成字符串常量头文件
python gen_strings.py
```

### 字库格式

- **格式:** HZK16 (94 区 × 94 位 × 32 字节/字符)
- **大小:** 282,752 字节 (~276 KB)
- **写入位置:** W25Q64 起始地址 0x000000
- **编码:** GB2312 (双字节, 区码 A1-F7, 位码 A1-FE)

---

## 字库传输协议

MCU 通过 UART 接收字库时使用如下协议 (逐块校验):

```
PC → MCU:  同步头 4B (EB 90 EB 90)
PC → MCU:  文件长度 4B (小端)
MCU → PC:  0x06 (ACK)
MCU:       擦除 W25Q64 扇区
MCU → PC:  0x06 (ACK)
PC → MCU:  字库数据 (每块 256B)
MCU:       写入 → 回读 → 发回 PC
MCU → PC:  回读 256B 数据
PC → MCU:  0x06 (ACK) 或 0x15 (NAK)
...重复直到全部完成
MCU → PC:  0x00 (写入完成)
PC → MCU:  CRC32 4B (小端)
MCU → PC:  0x06 (校验通过) / 0x15 + 本地 CRC32
```

---

## 常见问题

### Q: 读取 JEDEC ID 返回 0x000000?

- 检查 W25Q64 供电 (2.7V-3.6V)
- 检查 SPI 引脚连接是否正确
- 检查 CS 上拉电阻 (建议 10kΩ 上拉到 3.3V)

### Q: OLED 不显示?

- 检查 I2C 引脚 SCL/SDA 连接
- 检查 OLED 供电 (3.3V)
- 检查 I2C 地址是否正确 (0x3C)

### Q: 字库传输失败 (CRC 校验不通过)?

- 降低波特率: `send_font.bat COM4 38400`
- 检查 UART 接线 TX/RX 是否交叉
- 确保 W25Q64 已先执行过 `DisableProtection()`

### Q: 显示中文乱码?

- 确认字库已正确写入 W25Q64
- 确认 `OLED_SetFontFlashBase(0)` 地址正确
- 确认传入的是 GB2312 编码, 不是 UTF-8

### Q: 如何切换字库存储位置?

```c
OLED_SetFontFlashBase(0x100000);  // 字库存储在 1MB 偏移处
```

保存字库时使用对应偏移:
```bat
python gen_send_font.py --port COM4 --addr 0x100000
```

---

## 项目依赖

- **MCU:** TI MSPM0G3507 (Cortex-M0+)
- **Flash:** Winbond W25Q64JV (8MB SPI NOR Flash)
- **Display:** 0.96" 128×64 OLED (SH1106/SSD1306, I2C)
- **SDK:** MSPM0 SDK 2.10.00.04
- **Toolchain:** Keil MDK / IAR / GCC / TI CLANG
