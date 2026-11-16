#ifndef NRF24_H
#define NRF24_H

#include <stdint.h>

/* nRF24L01+ 驱动与两个功能（Scanner / Proto Kill）。
 * 总线：独立 SPI3(HSPI)，与 TFT/SD 的 SPI2 物理隔离，互不抢事务锁。
 * 引脚见 shared.h 的 NRF24_* 宏。 */
namespace Nrf24Radio {
bool begin();       // 上电并探测模块，返回是否在线
bool isPresent();   // 最近一次 begin() 的探测结果
void release();     // 关载波、CE 拉低、释放 SPI3
}  // namespace Nrf24Radio

namespace NrfScanner {
void setup();
void loop();
}  // namespace NrfScanner

namespace NrfProtoKill {
void setup();
void loop();
}  // namespace NrfProtoKill

#endif  // NRF24_H
