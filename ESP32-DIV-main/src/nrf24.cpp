#include <Arduino.h>
#include <SPI.h>
#include <TFT_eSPI.h>

#include "nrf24.h"
#include "shared.h"
#include "utils.h"

extern TFT_eSPI tft;

namespace {

/*──────────────── nRF24L01+ 寄存器与命令 ────────────────*/
constexpr uint8_t REG_CONFIG     = 0x00;
constexpr uint8_t REG_EN_AA      = 0x01;
constexpr uint8_t REG_EN_RXADDR  = 0x02;
constexpr uint8_t REG_SETUP_AW   = 0x03;
constexpr uint8_t REG_SETUP_RETR = 0x04;
constexpr uint8_t REG_RF_CH      = 0x05;
constexpr uint8_t REG_RF_SETUP   = 0x06;
constexpr uint8_t REG_RPD        = 0x09;

constexpr uint8_t CMD_R_REGISTER = 0x00;
constexpr uint8_t CMD_W_REGISTER = 0x20;
constexpr uint8_t CMD_FLUSH_TX   = 0xE1;
constexpr uint8_t CMD_FLUSH_RX   = 0xE2;
constexpr uint8_t CMD_NOP        = 0xFF;

constexpr uint8_t BIT_PWR_UP    = 0x02;
constexpr uint8_t BIT_PRIM_RX   = 0x01;
constexpr uint8_t BIT_CONT_WAVE = 0x80;
constexpr uint8_t BIT_PLL_LOCK  = 0x10;
constexpr uint8_t RF_PWR_0DBM   = 0x06;

/* SPI3(HSPI) 独立主机：TFT/SD 在 SPI2 上，两者不共享事务锁。 */
SPIClass s_spi(HSPI);
bool s_busStarted = false;
bool s_present = false;

inline void csnLow() { digitalWrite(NRF24_CSN, LOW); }
inline void csnHigh() { digitalWrite(NRF24_CSN, HIGH); }

void busWrite(uint8_t reg, uint8_t value) {
  s_spi.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));
  csnLow();
  s_spi.transfer((uint8_t)(CMD_W_REGISTER | (reg & 0x1F)));
  s_spi.transfer(value);
  csnHigh();
  s_spi.endTransaction();
}

uint8_t busRead(uint8_t reg) {
  s_spi.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));
  csnLow();
  s_spi.transfer((uint8_t)(CMD_R_REGISTER | (reg & 0x1F)));
  const uint8_t v = s_spi.transfer(CMD_NOP);
  csnHigh();
  s_spi.endTransaction();
  return v;
}

void busCommand(uint8_t cmd) {
  s_spi.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));
  csnLow();
  s_spi.transfer(cmd);
  csnHigh();
  s_spi.endTransaction();
}

/** 关掉连续载波并进入掉电，退出功能时一定要调。 */
void radioIdle() {
  digitalWrite(NRF24_CE, LOW);
  if (!s_present) {
    return;
  }
  busWrite(REG_RF_SETUP, RF_PWR_0DBM);
  busWrite(REG_CONFIG, 0x00);
}

/** 接收模式（RPD 需要 CE 高且处于 RX）。 */
void radioRxMode(uint8_t channel) {
  busWrite(REG_EN_AA, 0x00);
  busWrite(REG_EN_RXADDR, 0x01);
  busWrite(REG_SETUP_AW, 0x03);
  busWrite(REG_RF_SETUP, RF_PWR_0DBM);
  busWrite(REG_RF_CH, (uint8_t)(channel & 0x7F));
  busWrite(REG_CONFIG, (uint8_t)(BIT_PWR_UP | BIT_PRIM_RX));
}

/** 在指定信道输出未调制载波（CONT_WAVE + PLL_LOCK，0dBm）。 */
void radioCarrier(uint8_t channel) {
  busWrite(REG_EN_AA, 0x00);
  busWrite(REG_EN_RXADDR, 0x00);
  busWrite(REG_SETUP_RETR, 0x00);
  busWrite(REG_RF_CH, (uint8_t)(channel & 0x7F));
  busWrite(REG_RF_SETUP, (uint8_t)(BIT_CONT_WAVE | BIT_PLL_LOCK | RF_PWR_0DBM));
  busWrite(REG_CONFIG, BIT_PWR_UP);
}

/* 状态栏：功能界面里 500ms 刷一次就够，别把扫描节奏拖慢。 */
uint32_t s_lastStatusMs = 0;

void refreshStatusBar(bool force) {
  const uint32_t now = millis();
  if (!force && (now - s_lastStatusMs) < 500) {
    return;
  }
  s_lastStatusMs = now;
  currentBatteryVoltage = readBatteryVoltage();
  drawStatusBar(currentBatteryVoltage, force);
}

void drawTitle(const char* title) {
  tft.setTextFont(2);
  tft.setTextSize(1);
  tft.setTextColor(UI_TEXT, TFT_BLACK);
  tft.drawString(title, 8, 4);
}

}  // namespace

/*──────────────── 对外：硬件层 ────────────────*/
namespace Nrf24Radio {

bool begin() {
  if (!s_busStarted) {
    // 首次调用决定引脚映射；之后再调 begin() 不会重定向。
    s_spi.begin(NRF24_SCK, NRF24_MISO, NRF24_MOSI, -1);
    pinMode(NRF24_CSN, OUTPUT);
    csnHigh();
    pinMode(NRF24_CE, OUTPUT);
    digitalWrite(NRF24_CE, LOW);
    s_busStarted = true;
    delay(60);
  }

  // 探测：SETUP_AW 只有低两位有效，写 0x03 能读回 0x03 才算在线。
  busWrite(REG_SETUP_AW, 0x03);
  const uint8_t aw = busRead(REG_SETUP_AW);
  // 再确认一次：CONFIG 写 PWR_UP 后能读到 PWR_UP（部分克隆片会回 0x08）。
  busWrite(REG_CONFIG, BIT_PWR_UP);
  delay(2);
  const uint8_t cfg = busRead(REG_CONFIG);

  s_present = ((aw & 0x03) == 0x03) || ((cfg & BIT_PWR_UP) != 0);
  if (s_present) {
    busCommand(CMD_FLUSH_TX);
    busCommand(CMD_FLUSH_RX);
    radioIdle();
  }
  return s_present;
}

bool isPresent() { return s_present; }

void release() {
  radioIdle();
}

}  // namespace Nrf24Radio

/*──────────────── Scanner：逐信道读 RPD ────────────────*/
namespace NrfScanner {

constexpr int kChannels = 126;
constexpr int kColsPerRow = 63;
constexpr int kBarW = 3;
constexpr int kMaxBarH = 58;
constexpr int kRowBottom[2] = {112, 208};
constexpr int kX0 = 25;   // (240 - 63*3) / 2

uint8_t s_activity[kChannels];
uint8_t s_drawn[kChannels];
uint16_t s_sweeps = 0;
uint32_t s_lastStatusMs = 0;

uint16_t barColor(int h) {
  if (h >= 40) return RED;
  if (h >= 20) return ORANGE;
  return GREEN;
}

void clearBars() {
  for (int row = 0; row < 2; ++row) {
    tft.fillRect(kX0, kRowBottom[row] - kMaxBarH, kColsPerRow * kBarW, kMaxBarH, TFT_BLACK);
  }
  for (int i = 0; i < kChannels; ++i) {
    s_drawn[i] = 0;
  }
}

void sweepOnce() {
  for (int ch = 0; ch < kChannels; ++ch) {
    busWrite(REG_RF_CH, (uint8_t)ch);
    digitalWrite(NRF24_CE, HIGH);
    delayMicroseconds(180);
    const bool hit = (busRead(REG_RPD) & 0x01) != 0;
    digitalWrite(NRF24_CE, LOW);

    if (hit) {
      const int v = (int)s_activity[ch] + 26;
      s_activity[ch] = (uint8_t)(v > 255 ? 255 : v);
    } else if (s_activity[ch] > 1) {
      s_activity[ch] -= 2;
    } else {
      s_activity[ch] = 0;
    }
  }
  s_sweeps++;
}

void redrawBars() {
  for (int ch = 0; ch < kChannels; ++ch) {
    const int h = (int)s_activity[ch] * kMaxBarH / 255;
    if (h == s_drawn[ch]) {
      continue;
    }
    const int row = ch / kColsPerRow;
    const int col = ch % kColsPerRow;
    const int x = kX0 + col * kBarW;
    const int bottom = kRowBottom[row];
    if (h < s_drawn[ch]) {
      tft.fillRect(x, bottom - s_drawn[ch], kBarW, s_drawn[ch] - h, TFT_BLACK);
    }
    if (h > 0) {
      tft.fillRect(x, bottom - h, kBarW, h, barColor(h));
    }
    s_drawn[ch] = (uint8_t)h;
  }
}

void drawHeader() {
  tft.fillRect(0, 2, 240, 16, TFT_BLACK);
  drawTitle("2.4GHz Scanner");
  tft.setTextFont(2);
  tft.setTextColor(UI_DIM_TEXT, TFT_BLACK);
  tft.drawRightString("0-125", 232, 4, 2);
}

void drawFooterInfo() {
  tft.fillRect(0, 214, 240, 16, TFT_BLACK);
  tft.setTextFont(2);
  tft.setTextColor(UI_DIM_TEXT, TFT_BLACK);
  char buf[40];
  snprintf(buf, sizeof(buf), "2400-2525MHz  sweeps %u", (unsigned)s_sweeps);
  tft.drawString(buf, 8, 214);
}

void setup() {
  setTouchButtonInputEnabled(true);
  setTouchNavLabels(nullptr, nullptr, "Exit", nullptr, nullptr);

  for (int i = 0; i < kChannels; ++i) {
    s_activity[i] = 0;
    s_drawn[i] = 0;
  }
  s_sweeps = 0;

  featureClearContent(TFT_BLACK);
  drawTitle("2.4GHz Scanner");

  if (!Nrf24Radio::begin()) {
    tft.setTextColor(RED, TFT_BLACK);
    tft.drawString("nRF24 not found", 8, 60);
    tft.setTextColor(UI_DIM_TEXT, TFT_BLACK);
    tft.drawString("CE14 CSN15 SCK16", 8, 80);
    tft.drawString("MOSI18 MISO17", 8, 98);
    setTouchNavLabels(nullptr, nullptr, "Exit", nullptr, nullptr);
    redrawTouchButtonBar();
    return;
  }

  radioRxMode(0);
  clearBars();
  drawFooterInfo();
  refreshStatusBar(true);
  redrawTouchButtonBar();
}

void loop() {
  if (feature_active && (feature_exit_requested || featureExitButtonPressed())) {
    feature_exit_requested = true;
    return;
  }

  maintainTouchNavBar();

  if (!Nrf24Radio::isPresent()) {
    if (isTouchNavButtonPressedEdge(BTN_SELECT)) {
      feature_exit_requested = true;
    }
    delay(40);
    return;
  }

  sweepOnce();
  redrawBars();
  drawFooterInfo();
  refreshStatusBar(false);

  if (isTouchNavButtonPressedEdge(BTN_SELECT)) {
    feature_exit_requested = true;
  }
}

}  // namespace NrfScanner

/*──────────────── Proto Kill：定频载波 + 随机跳频 ────────────────*/
namespace NrfProtoKill {

enum Target : uint8_t {
  T_BLE = 0,
  T_BLUETOOTH,
  T_WIFI,
  T_VIDEO,
  T_RC,
  T_USB,
  T_ZIGBEE,
  T_NRF24,
  T_COUNT
};

/* 信道表照搬原 ESP32-DIV 的 ProtoKill 定义，未做改动。 */
const uint8_t kChBle[]       = {2, 26, 80};
const uint8_t kChBluetooth[] = {32, 34, 46, 48, 50, 52, 0, 1, 2, 4, 6, 8, 22, 24, 26, 28, 30, 74, 76, 78, 80};
const uint8_t kChWifi[]      = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
const uint8_t kChVideo[]     = {70, 75, 80};
const uint8_t kChRc[]        = {1, 3, 5, 7};
const uint8_t kChUsb[]       = {40, 50, 60};
const uint8_t kChZigbee[]    = {11, 15, 20, 25};
const uint8_t kChNrf24[]     = {76, 78, 79};

const char* const kNames[T_COUNT] = {"BLE", "Bluetooth", "WiFi", "Video TX",
                                     "RC", "USB Wireless", "Zigbee", "nRF24"};
const uint8_t* const kChans[T_COUNT] = {kChBle, kChBluetooth, kChWifi, kChVideo,
                                        kChRc, kChUsb, kChZigbee, kChNrf24};
const uint8_t kLens[T_COUNT] = {3, 21, 12, 3, 4, 3, 4, 3};

constexpr int kRowY0 = 44;
constexpr int kRowH = 20;

Target s_mode = T_WIFI;   // 原项目默认也是 WiFi
bool s_running = false;
uint8_t s_curCh = 0;
uint32_t s_lastHopMs = 0;
uint32_t s_hops = 0;
uint32_t s_lastStatusMs = 0;

void drawList() {
  for (int i = 0; i < T_COUNT; ++i) {
    const int y = kRowY0 + i * kRowH;
    const bool sel = ((int)s_mode == i);
    tft.fillRect(0, y, 240, kRowH, sel ? UI_ACCENT : TFT_BLACK);
    tft.setTextFont(2);
    tft.setTextSize(1);
    tft.setTextColor(sel ? TFT_WHITE : UI_TEXT, sel ? UI_ACCENT : TFT_BLACK);
    tft.drawString(kNames[i], 12, y + 2);
    tft.setTextColor(sel ? TFT_WHITE : UI_DIM_TEXT, sel ? UI_ACCENT : TFT_BLACK);
    tft.drawRightString(s_running && sel ? "ON" : "", 228, y + 2, 2);
  }
}

void drawStatus() {
  tft.fillRect(0, 212, 240, 40, TFT_BLACK);
  tft.setTextFont(2);
  tft.setTextColor(UI_DIM_TEXT, TFT_BLACK);
  char buf[48];
  snprintf(buf, sizeof(buf), "CH %u  hops %lu", (unsigned)s_curCh, (unsigned long)s_hops);
  tft.drawString(buf, 8, 214);
  tft.setTextColor(s_running ? RED : UI_DIM_TEXT, TFT_BLACK);
  tft.drawString(s_running ? "Carrier ON" : "Idle", 8, 232);
}

void applyMode(Target t) {
  s_mode = t;
  s_curCh = kChans[t][0];
}

void stopCarrier() {
  radioIdle();
  s_running = false;
}

void startCarrier() {
  if (!Nrf24Radio::isPresent()) {
    return;
  }
  s_running = true;
  s_hops = 0;
  s_curCh = kChans[s_mode][0];
  radioCarrier(s_curCh);
  digitalWrite(NRF24_CE, HIGH);
  s_lastHopMs = millis();
}

void setup() {
  setTouchButtonInputEnabled(true);
  setTouchNavLabels(nullptr, "Next", "Exit", "Prev", "Run");

  s_running = false;
  s_hops = 0;
  applyMode(s_mode);

  featureClearContent(TFT_BLACK);
  drawTitle("Proto Kill");

  if (!Nrf24Radio::begin()) {
    tft.setTextColor(RED, TFT_BLACK);
    tft.drawString("nRF24 not found", 8, 60);
    setTouchNavLabels(nullptr, nullptr, "Exit", nullptr, nullptr);
    redrawTouchButtonBar();
    return;
  }

  drawList();
  drawStatus();
  refreshStatusBar(true);
  redrawTouchButtonBar();
}

void loop() {
  if (feature_active && (feature_exit_requested || featureExitButtonPressed())) {
    stopCarrier();
    feature_exit_requested = true;
    return;
  }

  maintainTouchNavBar();

  if (!Nrf24Radio::isPresent()) {
    if (isTouchNavButtonPressedEdge(BTN_SELECT)) {
      feature_exit_requested = true;
    }
    delay(40);
    return;
  }

  if (isTouchNavButtonPressedEdge(BTN_UP)) {
    stopCarrier();
    applyMode((Target)(((int)s_mode + T_COUNT - 1) % T_COUNT));
    drawList();
    drawStatus();
  }

  if (isTouchNavButtonPressedEdge(BTN_DOWN)) {
    stopCarrier();
    applyMode((Target)(((int)s_mode + 1) % T_COUNT));
    drawList();
    drawStatus();
  }

  if (isTouchNavButtonPressedEdge(BTN_RIGHT)) {
    if (s_running) {
      stopCarrier();
    } else {
      startCarrier();
    }
    drawList();
    drawStatus();
  }

  if (s_running && (millis() - s_lastHopMs) >= 20) {
    s_lastHopMs = millis();
    const uint8_t len = kLens[s_mode];
    s_curCh = kChans[s_mode][random(0, len)];
    radioCarrier(s_curCh);
    s_hops++;
    drawStatus();
  }

  if (isTouchNavButtonPressedEdge(BTN_SELECT)) {
    stopCarrier();
    feature_exit_requested = true;
  }

  delay(5);
}

}  // namespace NrfProtoKill
