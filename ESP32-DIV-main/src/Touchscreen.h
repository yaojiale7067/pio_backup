#ifndef TOUCHSCREEN_H
#define TOUCHSCREEN_H

#include <Arduino.h>
#include <SPI.h>
#include "SettingsStore.h"
#include "shared.h"

// FT6336 电容触摸，I2C（SCL=1, SDA=2, RST=42, INT=-1）。
// 直接驱动面板的逻辑坐标（竖屏 240x320），再按屏幕方向映射到像素坐标。
extern bool feature_active;

void setupTouchscreen();

/** True while a finger is down. */
bool isTouchDown(uint16_t zThresh = 500);
/** Lower threshold for dismiss taps. */
bool isTouchDownDismiss(uint16_t zThresh = 120);
/** Raw panel coordinates (before screen mapping). */
bool readTouchRawXY(int16_t& x, int16_t& y, uint16_t zThresh = 500);

bool readTouchXY(int& x, int& y);
/** Lower threshold variant used by "tap anywhere to dismiss" handlers. */
bool readTouchXYDismiss(int& x, int& y);

#endif
