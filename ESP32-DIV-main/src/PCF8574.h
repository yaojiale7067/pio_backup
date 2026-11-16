#pragma once

#include <stdint.h>

// PCF8574 按钮扩展板在本板不存在，给一个空实现占位（所有读返回释放态）。
// 这样 WiFi/BLE 等代码里 #if HAS_PCF8574_BUTTONS 的分支仍可编译，运行时不会用到。
class PCF8574 {
public:
  explicit PCF8574(uint8_t addr = 0x20) { (void)addr; }
  bool begin(uint8_t addr = 0x20) { (void)addr; return false; }
  void pinMode(int pin, uint8_t mode) { (void)pin; (void)mode; }
  bool digitalRead(int pin) { (void)pin; return true; } // true = 未按下
};
