/**
 * @file my_i2c.hpp
 * @brief The I2C port shown on the documentation's Porting page, compiled by the host build.
 *
 * Every platform call goes through the `platform_*` functions declared below; a real port
 * replaces them with its SDK (ESP-IDF, STM32 HAL, Zephyr, Linux i2c-dev). The class is what the
 * driver needs, nothing more.
 */
#pragma once

#include "pca9685_i2c_interface.hpp"

#include <cstddef>
#include <cstdint>

// Platform hooks: your SDK's calls. Declared only, so the docs build checks the port compiles.
bool platform_i2c_init() noexcept;
void platform_delay_us(uint32_t us) noexcept;                                            // busy-wait
bool platform_i2c_write(uint8_t addr7, const uint8_t* bytes, std::size_t len) noexcept;  // START .. STOP
bool platform_i2c_write_read(uint8_t addr7, const uint8_t* tx, std::size_t tx_len, uint8_t* rx,
                             std::size_t rx_len) noexcept;                                // repeated START

// --8<-- [start:port]
class MyI2c : public pca9685::I2cInterface<MyI2c> {
public:
  // Bring the bus up once; later calls return true at once.
  bool EnsureInitialized() noexcept {
    if (!ready_) {
      ready_ = platform_i2c_init();
    }
    return ready_;
  }

  // One write transaction: START, address + W, register, data bytes, STOP.
  bool Write(uint8_t addr, uint8_t reg, const uint8_t* data, size_t len) noexcept {
    uint8_t frame[1 + 4];                           // the driver writes at most 4 data bytes
    if (len > sizeof(frame) - 1) {
      return false;
    }
    frame[0] = reg;
    for (size_t i = 0; i < len; ++i) {
      frame[1 + i] = data[i];
    }
    return platform_i2c_write(addr, frame, len + 1);
  }

  // Register read: START, address + W, register, repeated START, address + R, data, STOP.
  bool Read(uint8_t addr, uint8_t reg, uint8_t* data, size_t len) noexcept {
    return platform_i2c_write_read(addr, &reg, 1, data, len);
  }

  // Optional: wait at least `us` microseconds. The driver uses it for the 500 us oscillator start-up.
  void DelayUs(uint32_t us) noexcept { platform_delay_us(us); }

private:
  bool ready_ = false;
};
// --8<-- [end:port]
