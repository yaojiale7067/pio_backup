//                            USER DEFINED SETTINGS
//   ESP32-DIV PlatformIO port — 迁移到用户的 240x320 IPS (ILI9341) + FT6336 触摸板。
//
//   引脚（与 lvgl_esp32_320_240_1 工程完全一致）：
//     TFT_SCLK 12   TFT_MOSI 11   TFT_MISO 13
//     TFT_CS   10   TFT_DC    9   TFT_RST  8   TFT_BL 21
//     SD_CS     4   ← SD 与 TFT 共用上面同一条 SPI 总线，只有 CS 不同
//     触摸 FT6336：I2C SCL=1 SDA=2 RST=42 INT 未接（走 I2C，不占 SPI）
//
//   总线共用的关键：**不要**定义 USE_HSPI_PORT。
//   定义了会生成 `SPIClass spi = SPIClass(HSPI)`，TFT_eSPI 与 SD 各持一个
//   SPIClass 实例、各自调用 spiStartBus()，在 S3 上后者会
//   periph_module_reset(PERIPH_SPI3_MODULE) 把正在出图的总线复位掉；
//   而且两个实例的 _inTransaction 互不可见，事务不再是原子的。
//   不定义时 TFT_eSPI 用 `SPIClass& spi = SPI`（S3 上是 FSPI/SPI2），
//   SD.begin(CS) 默认也用同一个全局 SPI 实例，于是
//   SPI.beginTransaction() 的同一把 paramLock 天然把两条路径串行化。
//
//   !! 本目录是打过补丁的 TFT_eSPI 2.5.43，不要用官方源码覆盖 !!
//   Processors/TFT_eSPI_ESP32_S3.h 里默认分支原本写 `#define SPI_PORT FSPI`，
//   而 Arduino core 在 S3 上把 FSPI 定义为 0，IDF 的
//   REG_SPI_BASE(i) = (i>=2) ? ... : 0，于是 SPI_USER_REG/SPI_W0_REG 全部
//   折叠成 0x10/0x98 这种裸地址，tft.init() 第一发 writecommand 就
//   StoreProhibited (EXCVADDR 0x10)。已改为 #define SPI_PORT 2（SPI2 窗口
//   0x60024000，正好是 core 里 FSPI→PERIPH_SPI2_MODULE 对应的那组寄存器）。

#define USER_SETUP_INFO "ESP32-DIV-PIO"

#define ILI9341_DRIVER

#define TFT_BACKLIGHT_ON HIGH
#define TFT_BL   21

#define TFT_MISO 13
#define TFT_MOSI 11
#define TFT_SCLK 12
#define TFT_CS   10
#define TFT_DC    9
#define TFT_RST   8

#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8
#define LOAD_GFXFF

// 60MHz（不是 80MHz）：整屏 240x320x16bpp = 153600 字节一次推完，
// 80MHz 走排线已经在时序临界点，长突发中途丢/错位会在随机位置留下画面碎片。
// 60MHz 下整屏推送 ≈ 20.5ms，仍小于面板帧周期，切屏不撕裂。
#define SPI_FREQUENCY        60000000
#define SPI_READ_FREQUENCY   60000000
// 触摸是 I2C（FT6336），下面这个值只影响 TFT_eSPI 自带的 XPT2046 代码，保留默认值。
#define SPI_TOUCH_FREQUENCY  2500000