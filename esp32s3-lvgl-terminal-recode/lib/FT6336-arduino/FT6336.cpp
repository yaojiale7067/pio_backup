#include "FT6336.h"
#include <Wire.h>

// 静态实例指针（用于中断回调）
static FT6336* ft6336_instance = nullptr;

FT6336::FT6336(int sda, int scl, int rst, int irq, uint16_t width, uint16_t height) {
    _sda = sda;
    _scl = scl;
    _rst = rst;
    _irq = irq;
    _width = width;
    _height = height;
    _rotation = ROTATION_NORMAL;
    _addr = FT6336_ADDR;
    _event_available = false;
    memset(&data, 0, sizeof(touch_data_t));
}

uint8_t FT6336::reset() {
    uint8_t tmp[2];
    
    pinMode(_irq, INPUT);
    pinMode(_rst, OUTPUT);
    
    digitalWrite(_irq, HIGH);
    digitalWrite(_rst, HIGH);
    delay(20);
    digitalWrite(_rst, LOW);
    delay(20);
    digitalWrite(_rst, HIGH);
    delay(500);
    
    // 读取芯片 ID (0xA8 应为 0x11)
    readBlockData(tmp, 0xA8, 1);
    if (tmp[0] != 0x11) return 1;
    
    // 读取厂商 ID (0x9F 应为 0x26)
    readBlockData(tmp, 0x9F, 2);
    if (tmp[0] != 0x26) return 1;
    
    return 0;
}

void FT6336::setRotation(uint8_t rot) {
    _rotation = rot;
}

void FT6336::begin(int interrupt) {
    Wire.begin(_sda, _scl);
    reset();
    
    // 读取版本信息
    i2c_read(0x15, &data.version, 1);
    delay(5);
    i2c_read(0xA7, data.versionInfo, 3);
    
    // 保存实例指针，用于中断
    ft6336_instance = this;
    attachInterruptArg(digitalPinToInterrupt(_irq), handleISRStatic, this, interrupt);
}

void IRAM_ATTR FT6336::handleISRStatic(void* arg) {
    FT6336* instance = static_cast<FT6336*>(arg);
    if (instance) instance->handleISR();
}

void FT6336::handleISR() {
    _event_available = true;
    if (userISR) userISR();
}

void FT6336::read_touch() {
    uint8_t buf[6];
    uint8_t td_status = 0;
    
    // 读取触摸状态 (0x02)
    i2c_read(0x02, &td_status, 1);
    
    data.points = td_status & 0x0F;
    data.event = (td_status >> 6) & 0x03;
    
    if (data.points > 0 && data.points < 3) {
        // 读取第一个触摸点数据 (0x03 ~ 0x08)
        i2c_read(0x03, buf, 6);
        
        data.gestureID = buf[0];
        data.x = ((buf[2] & 0x0F) << 8) | buf[3];
        data.y = ((buf[4] & 0x0F) << 8) | buf[5];
        
        // 应用旋转
        uint16_t temp;
        switch (_rotation) {
            case ROTATION_LEFT:
                temp = data.x;
                data.x = _height - data.y;
                data.y = temp;
                break;
            case ROTATION_INVERTED:
                data.x = _width - data.x;
                data.y = _height - data.y;
                break;
            case ROTATION_RIGHT:
                temp = data.x;
                data.x = data.y;
                data.y = _width - temp;
                break;
            default:
                break;
        }
    } else {
        data.gestureID = GESTURE_NONE;
        data.x = 0;
        data.y = 0;
    }
}

bool FT6336::available() {
    // 如果中断引脚为低电平，表示有新事件
    if (digitalRead(_irq) == LOW) {
        _event_available = true;
    }
    
    if (_event_available) {
        read_touch();
        _event_available = false;
        return true;
    }
    return false;
}

void FT6336::sleep() {
    digitalWrite(_rst, LOW);
    delay(5);
    digitalWrite(_rst, HIGH);
    delay(50);
    uint8_t standby = 0x03;
    i2c_write(0xA5, &standby, 1);
}

void FT6336::enable_double_click() {
    uint8_t mask = readByteData(0xEC);
    mask |= 0x01;
    writeByteData(0xEC, mask);
}

void FT6336::disable_auto_sleep() {
    uint8_t val = 0xFE;
    i2c_write(0xFE, &val, 1);
}

void FT6336::enable_auto_sleep() {
    uint8_t val = 0x00;
    i2c_write(0xFE, &val, 1);
}

void FT6336::set_auto_sleep_time(int seconds) {
    if (seconds < 1) seconds = 1;
    if (seconds > 255) seconds = 255;
    uint8_t t = (uint8_t)seconds;
    i2c_write(0xF9, &t, 1);
}

void FT6336::attachUserInterrupt(std::function<void(void)> callback) {
    userISR = callback;
}

String FT6336::gesture() {
    switch (data.gestureID) {
        case GESTURE_NONE:          return "NONE";
        case GESTURE_SWIPE_DOWN:    return "SWIPE DOWN";
        case GESTURE_SWIPE_UP:      return "SWIPE UP";
        case GESTURE_SWIPE_LEFT:    return "SWIPE LEFT";
        case GESTURE_SWIPE_RIGHT:   return "SWIPE RIGHT";
        case GESTURE_SINGLE_CLICK:  return "SINGLE CLICK";
        case GESTURE_DOUBLE_CLICK:  return "DOUBLE CLICK";
        case GESTURE_LONG_PRESS:    return "LONG PRESS";
        default:                    return "UNKNOWN";
    }
}

// ---------- I2C 辅助函数 ----------
uint8_t FT6336::i2c_read(uint8_t reg_addr, uint8_t *reg_data, size_t length) {
    Wire.beginTransmission(_addr);
    Wire.write(reg_addr);
    if (Wire.endTransmission(true)) return 1;
    Wire.requestFrom(_addr, length, true);
    for (size_t i = 0; i < length; i++) {
        reg_data[i] = Wire.read();
    }
    return 0;
}

uint8_t FT6336::i2c_write(uint8_t reg_addr, const uint8_t *reg_data, size_t length) {
    Wire.beginTransmission(_addr);
    Wire.write(reg_addr);
    for (size_t i = 0; i < length; i++) {
        Wire.write(reg_data[i]);
    }
    if (Wire.endTransmission(true)) return 1;
    return 0;
}

uint8_t FT6336::readByteData(uint8_t reg) {
    uint8_t val = 0;
    i2c_read(reg, &val, 1);
    return val;
}

void FT6336::writeByteData(uint8_t reg, uint8_t val) {
    i2c_write(reg, &val, 1);
}

void FT6336::readBlockData(uint8_t *buf, uint8_t reg, uint8_t size) {
    i2c_read(reg, buf, size);
}