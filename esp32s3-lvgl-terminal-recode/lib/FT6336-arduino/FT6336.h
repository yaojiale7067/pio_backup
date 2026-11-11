#ifndef FT6336_H
#define FT6336_H

#include <Arduino.h>
#include <Wire.h>
#include <functional>

#define FT6336_ADDR         0x38

// 旋转方向
#define ROTATION_NORMAL     0
#define ROTATION_LEFT       1
#define ROTATION_INVERTED   2
#define ROTATION_RIGHT      3

// 手势定义（与 CST816S 兼容）
enum GESTURE {
    GESTURE_NONE        = 0x00,
    GESTURE_SWIPE_UP    = 0x01,
    GESTURE_SWIPE_DOWN  = 0x02,
    GESTURE_SWIPE_LEFT  = 0x03,
    GESTURE_SWIPE_RIGHT = 0x04,
    GESTURE_SINGLE_CLICK = 0x05,
    GESTURE_DOUBLE_CLICK = 0x0B,
    GESTURE_LONG_PRESS   = 0x0C
};

// 与 CST816S 相同的数据结构
struct touch_data_t {
    uint8_t gestureID;      // 手势 ID
    uint8_t points;         // 触摸点数
    uint8_t event;          // 事件 (0=按下, 1=抬起, 2=接触)
    uint16_t x;             // X 坐标
    uint16_t y;             // Y 坐标
    uint8_t version;        // 版本号
    uint8_t versionInfo[3]; // 版本信息
};

class FT6336 {
public:
    FT6336(int sda, int scl, int rst, int irq, uint16_t width = 320, uint16_t height = 240);
    
    void begin(int interrupt = RISING);
    bool available();
    void sleep();
    
    // 手势与配置（与 CST816S 兼容）
    void enable_double_click();
    void disable_auto_sleep();
    void enable_auto_sleep();
    void set_auto_sleep_time(int seconds);
    void attachUserInterrupt(std::function<void(void)> callback);
    String gesture();
    
    // 触摸数据（与 CST816S.data 完全兼容）
    touch_data_t data;
    
private:
    int _sda, _scl, _rst, _irq;
    uint16_t _width, _height;
    uint8_t _rotation;
    uint8_t _addr;
    bool _event_available;
    std::function<void(void)> userISR;
    
    // 内部函数
    void read_touch();
    void setRotation(uint8_t rot);
    uint8_t reset();
    
    // I2C 读写
    uint8_t i2c_read(uint8_t reg_addr, uint8_t *reg_data, size_t length);
    uint8_t i2c_write(uint8_t reg_addr, const uint8_t *reg_data, size_t length);
    uint8_t readByteData(uint8_t reg);
    void writeByteData(uint8_t reg, uint8_t val);
    void readBlockData(uint8_t *buf, uint8_t reg, uint8_t size);
    
    // 中断处理（静态函数）
    static void IRAM_ATTR handleISRStatic(void* arg);
    void handleISR();
};

#endif