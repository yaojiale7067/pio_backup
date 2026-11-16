#ifndef SHARED_H
#define SHARED_H
#include <stdint.h>

/*──────────────────── Colors ────────────────────*/
const uint16_t GRAY = 0x8410, BLUE = 0x001F, RED = 0xF800,
               GREEN  = 0x07E0, BLACK = 0x0000, WHITE = 0xFFFF,
               LIGHT_GRAY = 0xC618, DARK_GRAY = 0x4208;

uint16_t uiUniversalColor();
#define ORANGE uiUniversalColor()

#define TFT_DARKBLUE   0x3166
#define TFT_LIGHTBLUE  0x051F
#define TFTWHITE       0xFFFF
#define TFT_GRAY       0x8410

#define BG_Dark        0x20e4
#define BG_Light       0xf7de
#define FG_Dark        0x3166
#define FG_Light       0xe73c
#define LINE_Dark      0x8410
#define LINE_Light     0x8410
#define ICON_Dark      0xFBE4
#define ICON_Light     0xFBE4
#define TEXT_Dark      0xFFFF
#define TEXT_Light     0x0000
#define UI_ACCENT_VAL  0x3166

#define L_Dark        0x4208
#define L_Light       0xC618

// Default background for feature screens (separate from main menu background).
#ifndef FEATURE_BG
#define FEATURE_BG BLACK
#endif

#define SELECTED_ICON_COLOR UI_ICON

/*──────────────────── UI Defaults ────────────────────*/
#ifndef UI_BG
#define UI_BG UI.bg
#endif
#ifndef UI_FG
#define UI_FG UI.fg
#endif
#ifndef UI_ICON
#define UI_ICON UI.icon
#endif
#ifndef UI_TEXT
#define UI_TEXT UI.text
#endif
#ifndef UI_LINE
#define UI_LINE UI.line
#endif
#ifndef UI_LABLE
#define UI_LABLE UI.lable
#endif
#ifndef UI_ACCENT
#define UI_ACCENT UI.accent
#endif
#ifndef UI_WARN
#define UI_WARN UI.warn
#endif
#ifndef UI_OK
#define UI_OK GREEN
#endif

/*──────────────────── Project Info ────────────────────*/
#ifndef ESP32DIV_NAME
#define ESP32DIV_NAME "ESP32-DIV"
#endif
#ifndef ESP32DIV_VERSION
#define ESP32DIV_VERSION "v1.7.0-pio"
#endif

static inline constexpr uint8_t k0() { return (uint8_t)('h' - '`'); }

static const uint8_t OBF_PN[]   = {77, 91, 88, 59, 58, 37, 76, 65, 94};
static const uint8_t OBF_DN[]   = {75, 97, 110, 109, 122, 92, 109, 107, 96};
static const uint8_t OBF_EM[]   = {107, 97, 110, 109, 122, 124, 109, 107, 96, 72, 111, 101, 105, 97, 100, 38, 107, 103, 101};
static const uint8_t OBF_GH[]   = {111, 97, 124, 96, 125, 106, 38, 107, 103, 101, 39, 107, 97, 110, 109, 122, 124, 109, 107, 96};
static const uint8_t OBF_WB[]   = {75, 97, 110, 109, 122, 92, 109, 107, 96, 38, 102, 109, 124};

/*──────────────────── Panel geometry (fixed) ────────────────────
 * ILI9341 240x320 SPI panel, kept in native portrait orientation so the
 * existing 240x320 UI layout stays valid. TFT_eSPI pin map lives in
 * lib/TFT_eSPI/User_Setup.h — do not duplicate it here.
 */
#ifndef TFT_WIDTH
#define TFT_WIDTH 240
#endif
#ifndef TFT_HEIGHT
#define TFT_HEIGHT 320
#endif
#ifndef TFT_ROTATION
#define TFT_ROTATION 0
#endif
#ifndef STATUS_BAR_Y_OFFSET
#define STATUS_BAR_Y_OFFSET 0
#endif

/* 老代码（ducky.cpp 等）用 DISPLAY_WIDTH/HEIGHT 这个旧名字，统一指向面板尺寸。 */
#ifndef DISPLAY_WIDTH
#define DISPLAY_WIDTH  TFT_WIDTH
#endif
#ifndef DISPLAY_HEIGHT
#define DISPLAY_HEIGHT TFT_HEIGHT
#endif

/*──────────────────── Feature flags ────────────────────*/
// 原 ESP32-DIV 的 NRF24 (2.4GHz Scanner / ProtoKill / BLE Jammer) 与
// Sub-GHz (CC1101 Replay/Jammer/Profiles) 已整体移除。
#define BOARD_HAS_ESP32S3 1

#ifndef FEATURE_WIFI_TOOLS
#define FEATURE_WIFI_TOOLS 1
#endif
#ifndef FEATURE_BLE_TOOLS
#define FEATURE_BLE_TOOLS  1
#endif
#ifndef FEATURE_BLE_DUCKY
#define FEATURE_BLE_DUCKY  1
#endif

/*──────────────────── Buttons ────────────────────
 * 本板没有 PCF8574 按键板，只有屏幕触摸导航条——因此导航条必须开启，
 * 否则所有 feature 都无法进入/退出。
 */
#define TOUCH_BUTTON_CUE_ENABLED 1
#define HAS_PCF8574_BUTTONS 0
/* 这些只是逻辑按键 ID（触摸导航条 5 个格子的标识），不是 GPIO 编号。
 * 取 0/1/2/8/9 是为了让源码里完全不出现用户禁用的那些脚号。 */
#define BTN_UP       0
#define BTN_DOWN     1
#define BTN_LEFT     2
#define BTN_RIGHT    8
#define BTN_SELECT   9

/* Buzzer: 本板无蜂鸣器（BUZZER_PIN < 0 = 关闭）。 */
#ifndef BUZZER_PIN
#define BUZZER_PIN -1
#endif

/* WS2812B: 本板无状态灯（STATUS_LED_COUNT = 0 = 关闭）。 */
#ifndef STATUS_LED_PIN
#define STATUS_LED_PIN -1
#endif
#ifndef STATUS_LED_COUNT
#define STATUS_LED_COUNT 0
#endif

/* Backlight / PWM — 背光在 IO21，可 PWM 调光。 */
#ifndef BACKLIGHT_PIN
#define BACKLIGHT_PIN 21
#endif
#define PWM_CHANNEL     0
#define PWM_FREQ        5000
#define PWM_RESOLUTION  8

/*──────────────────── SD card (shares the TFT SPI bus) ────────────────────
 * SCLK=12 MOSI=11 MISO=13 与 TFT 完全相同，只有 CS 不同：SD_CS=4。
 * 任何时刻只能有一个设备拉低 CS，TFT_eSPI 与 SD 都通过 SPI.beginTransaction
 * 拿同一把总线锁，因此这里不另开第二路 SPI 主机。
 */
#ifndef SD_CS
#define SD_CS    4
#endif
#ifndef SD_SCLK
#define SD_SCLK  12
#endif
#ifndef SD_MISO
#define SD_MISO  13
#endif
#ifndef SD_MOSI
#define SD_MOSI  11
#endif

/*──────────────────── Touch (FT6336, I2C) ────────────────────*/
#ifndef TOUCH_FT6336_SCL
#define TOUCH_FT6336_SCL 1
#endif
#ifndef TOUCH_FT6336_SDA
#define TOUCH_FT6336_SDA 2
#endif
#ifndef TOUCH_FT6336_RST
#define TOUCH_FT6336_RST 42
#endif
#ifndef TOUCH_FT6336_INT
#define TOUCH_FT6336_INT -1
#endif

/*──────────────────── nRF24L01+ (2.4GHz) ────────────────────
 * 用户实际接线（单模块）：
 *   CE -> 14   CSN -> 15   SCK -> 16   MOSI -> 18   MISO -> 17
 *   IRQ 悬空不接
 * 走独立 SPI3(HSPI)：SPIClass 的引脚映射是一次性绑定的，nRF 单独一路才
 * 不会和 TFT/SD 那条 SPI2 抢同一组寄存器。
 * 注意：这批脚正是上一条“不要引用”清单里的那些，按用户最新要求已经
 * 整体划给 nRF24；PN532 / IR / GPS 仍保持 -1（未连接）。 */
#ifndef NRF24_CE
#define NRF24_CE   14
#endif
#ifndef NRF24_CSN
#define NRF24_CSN  15
#endif
#ifndef NRF24_SCK
#define NRF24_SCK  16
#endif
#ifndef NRF24_MISO
#define NRF24_MISO 17
#endif
#ifndef NRF24_MOSI
#define NRF24_MOSI 18
#endif

/* PN532 RFID/NFC —— 本板未接该模块，而且这些脚都在禁用表里，
 * 因此全部置 -1（未连接）。RfidNfc::begin() 检测到引脚为负会直接
 * 返回不可用，连 SPI3 都不初始化。
 * 将来要接模块，请改成既没被占用、也不在禁用表里的脚。 */
#ifndef PN532_SCK
#define PN532_SCK  -1
#endif
#ifndef PN532_MISO
#define PN532_MISO -1
#endif
#ifndef PN532_MOSI
#define PN532_MOSI -1
#endif
#ifndef PN532_SS
#define PN532_SS   -1
#endif

/* IR remote —— 同上：本板没有收发管，脚号在禁用表内，故置 -1。
 * 引脚为 -1 时 IR_PIN_OR_NONE() 给 0xFFFF，core 的 pinMode/digitalRead/
 * attachInterrupt 都有 `pin >= SOC_GPIO_PIN_COUNT` 保护，不会碰任何真实脚。 */
#ifndef IR_RX_PIN
#define IR_RX_PIN -1
#endif
#ifndef IR_TX_PIN
#define IR_TX_PIN -1
#endif
#ifndef IR_DEFAULT_KHZ
#define IR_DEFAULT_KHZ 38
#endif
#define IR_PIN_OR_NONE(p) ((uint16_t)((p) < 0 ? 0xFFFF : (p)))

/* Neo-6M GPS on UART2 —— 未接模块，脚号在禁用表内，故置 -1。
 * 传 -1 给 Serial.begin 表示“不绑定引脚”（core: rxPin >= 0 才配置）。 */
#ifndef GPS_UART_RX
#define GPS_UART_RX -1
#endif
#ifndef GPS_UART_TX
#define GPS_UART_TX -1
#endif

/*──────────────────── Timing ────────────────────*/
#ifndef DEBOUNCE_MS
#define DEBOUNCE_MS 40
#endif
#ifndef LONG_PRESS_MS
#define LONG_PRESS_MS 800
#endif
#ifndef REPEAT_SCROLL_MS
#define REPEAT_SCROLL_MS 120
#endif

/*──────────────────── Backlight Levels ────────────────────*/
#ifndef BKL_LEVEL_MIN
#define BKL_LEVEL_MIN 10
#endif
#ifndef BKL_LEVEL_MED
#define BKL_LEVEL_MED 128
#endif
#ifndef BKL_LEVEL_MAX
#define BKL_LEVEL_MAX 255
#endif

/*──────────────────── Filesystem ────────────────────*/
#ifndef FS_MOUNT_OK_MSG
#define FS_MOUNT_OK_MSG "SD OK"
#endif
#ifndef FS_MOUNT_FAIL_MSG
#define FS_MOUNT_FAIL_MSG "SD Fail"
#endif
#ifndef LOG_DIR
#define LOG_DIR "/logs"
#endif
#ifndef CAPTURE_DIR
#define CAPTURE_DIR "/captures"
#endif

/*──────────────────── Battery ────────────────────*/
#ifndef BATTERY_ADC_PIN
#define BATTERY_ADC_PIN -1
#endif
#ifndef BATTERY_VDIV_R1
#define BATTERY_VDIV_R1 200000.0f
#endif
#ifndef BATTERY_VDIV_R2
#define BATTERY_VDIV_R2 100000.0f
#endif
#ifndef BATTERY_LOW_VOLT
#define BATTERY_LOW_VOLT 3.40f
#endif

/*──────────────────── Wi-Fi ────────────────────*/
#ifndef WIFI_SCAN_ACTIVE_MS
#define WIFI_SCAN_ACTIVE_MS  500
#endif
#ifndef WIFI_SCAN_PASSIVE_MS
#define WIFI_SCAN_PASSIVE_MS 0
#endif
#ifndef WIFI_SPECTRUM_FFT_N
#define WIFI_SPECTRUM_FFT_N  256
#endif

/*──────────────────── BLE ────────────────────*/
#ifndef BLE_ADV_INTERVAL_MS
#define BLE_ADV_INTERVAL_MS 100
#endif

/*──────────────────── Utilities ────────────────────*/
#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x)/sizeof((x)[0]))
#endif
#ifndef UNUSED
#define UNUSED(x) (void)(x)
#endif

/*──────────────────── Settings / Themes ────────────────────*/
enum class Theme : uint8_t { Dark = 0, Light = 1 };

#ifndef SETTINGS_PATH
#define SETTINGS_PATH "/config/settings.json"
#endif

class TFT_eSPI;
void displaySubmenu();

/*──────────────────── Runtime UI Palette ────────────────────*/
struct UiPalette { uint16_t bg, fg, icon, text, accent, line, lable, warn, ok; };
extern UiPalette UI;
void applyThemeToPalette(Theme t);
uint16_t uiUniversalColor();

/*──────────────────── UI Text Helpers ────────────────────*/
static inline uint16_t uiDimTextColor() {
  return (UI.bg == BG_Dark) ? GRAY : UI_LABLE;
}
#ifndef UI_DIM_TEXT
#define UI_DIM_TEXT uiDimTextColor()
#endif

/*──────────────────── Global State Flags ────────────────────*/
extern bool in_sub_menu;
extern bool feature_active;
extern bool submenu_initialized;
extern bool is_main_menu;
extern bool feature_exit_requested;

#endif