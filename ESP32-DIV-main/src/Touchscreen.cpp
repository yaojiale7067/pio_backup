#include "Touchscreen.h"
#include <TFT_eSPI.h>
#include <FT6336.h>

extern TFT_eSPI tft;

// FT6336 面板原生分辨率（不随屏幕方向变化）
#define TOUCH_MAP_X1 0
#define TOUCH_MAP_X2 240
#define TOUCH_MAP_Y1 0
#define TOUCH_MAP_Y2 320

static FT6336 s_touch(TOUCH_FT6336_SDA, TOUCH_FT6336_SCL, TOUCH_FT6336_INT, TOUCH_FT6336_RST,
                      max(TOUCH_MAP_X1, TOUCH_MAP_X2), max(TOUCH_MAP_Y1, TOUCH_MAP_Y2));

bool feature_active = false;
static bool s_touchInitialized = false;

void setupTouchscreen() {
  if (s_touchInitialized) {
    return;
  }
  // 始终让驱动输出面板原生坐标，方向映射统一在 mapTouchToScreen() 里做，
  // 避免"驱动转一次 + 映射再转一次"的双重旋转。
  s_touch.setRotation(ROTATION_NORMAL);
  // 复位 FT6336 并读 ID 验证在线；失败也只是打日志，不影响其余启动。
  s_touch.begin();
  s_touchInitialized = true;
}

static bool touchSampleOk(uint16_t /*zThresh*/, int16_t& rawX, int16_t& rawY) {
  if (!s_touchInitialized) {
    return false;
  }
  s_touch.read();
  if (!s_touch.isTouched) {
    return false;
  }
  rawX = (int16_t)s_touch.points[0].x;
  rawY = (int16_t)s_touch.points[0].y;
  return true;
}

bool isTouchDown(uint16_t zThresh) {
  int16_t x = 0, y = 0;
  return touchSampleOk(zThresh, x, y);
}

bool isTouchDownDismiss(uint16_t zThresh) {
  return isTouchDown(zThresh);
}

bool readTouchRawXY(int16_t& x, int16_t& y, uint16_t zThresh) {
  return touchSampleOk(zThresh, x, y);
}

static void mapTouchToScreen(int16_t rawX, int16_t rawY, int& x, int& y) {
  // FT6336 直接输出面板逻辑坐标，按屏幕方向翻转/交换到像素坐标。
  switch (TFT_ROTATION) {
    case 0: // 竖屏 240x320
      x = (int)::map(rawX, TOUCH_MAP_X1, TOUCH_MAP_X2, 0, TFT_WIDTH - 1);
      y = (int)::map(rawY, TOUCH_MAP_Y1, TOUCH_MAP_Y2, 0, TFT_HEIGHT - 1);
      break;
    case 1: // 横屏
      x = (int)::map(rawX, TOUCH_MAP_X1, TOUCH_MAP_X2, 0, TFT_HEIGHT - 1);
      y = (int)::map(rawY, TOUCH_MAP_Y1, TOUCH_MAP_Y2, 0, TFT_WIDTH - 1);
      break;
    case 2: // 竖屏翻转
      x = (int)::map(rawX, TOUCH_MAP_X1, TOUCH_MAP_X2, TFT_WIDTH - 1, 0);
      y = (int)::map(rawY, TOUCH_MAP_Y1, TOUCH_MAP_Y2, TFT_HEIGHT - 1, 0);
      break;
    default:
      x = (int)::map(rawX, TOUCH_MAP_X1, TOUCH_MAP_X2, TFT_HEIGHT - 1, 0);
      y = (int)::map(rawY, TOUCH_MAP_Y1, TOUCH_MAP_Y2, TFT_WIDTH - 1, 0);
      break;
  }
}

bool readTouchXY(int& x, int& y) {
  int16_t rawX = 0, rawY = 0;
  if (!touchSampleOk(200, rawX, rawY)) {
    return false;
  }
  mapTouchToScreen(rawX, rawY, x, y);
  return true;
}

bool readTouchXYDismiss(int& x, int& y) {
  int16_t rawX = 0, rawY = 0;
  if (!touchSampleOk(120, rawX, rawY)) {
    return false;
  }
  mapTouchToScreen(rawX, rawY, x, y);
  return true;
}
