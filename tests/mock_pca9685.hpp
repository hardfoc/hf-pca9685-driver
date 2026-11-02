/**
 * @file mock_pca9685.hpp
 * @brief A model of the PCA9685's I2C register interface for the host unit tests.
 *
 * It keeps the 256-byte register file with the power-on defaults of the datasheet (Rev. 4, section 7.3),
 * records every transaction the driver sends, and applies the rules of the chip that the driver depends on:
 * PRE_SCALE ignores writes while MODE1.SLEEP is 0, a multi-byte access moves the register pointer only when
 * MODE1.AI is 1, and a write to ALL_LED_ON_L..ALL_LED_OFF_H loads that byte into every channel. Failures can
 * be injected per transaction.
 */
#pragma once

#include "pca9685_i2c_interface.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

struct I2cTransaction {
  bool write;
  uint8_t addr;
  uint8_t reg;
  std::vector<uint8_t> data;
};

class MockPca9685 : public pca9685::I2cInterface<MockPca9685> {
public:
  static constexpr uint8_t MODE1 = 0x00, MODE2 = 0x01, LED0_ON_L = 0x06, ALL_LED_ON_L = 0xFA,
                           PRE_SCALE = 0xFE;

  explicit MockPca9685(uint8_t address = 0x40) noexcept : address_(address) { powerOn(); }

  void powerOn() noexcept {
    regs_.fill(0);
    regs_[MODE1] = 0x11;                          // SLEEP, ALLCALL
    regs_[MODE2] = 0x04;                          // OUTDRV: totem pole
    regs_[0x02] = 0xE2;
    regs_[0x03] = 0xE4;
    regs_[0x04] = 0xE8;
    regs_[0x05] = 0xE0;
    for (int ch = 0; ch < 16; ++ch) {
      regs_[LED0_ON_L + 4 * ch + 3] = 0x10;       // LEDn_OFF_H: full off
    }
    regs_[PRE_SCALE] = 0x1E;                      // 200 Hz
  }

  bool EnsureInitialized() noexcept {
    ++init_calls;
    return init_ok;
  }

  bool Write(uint8_t addr, uint8_t reg, const uint8_t* data, size_t len) noexcept {
    log.push_back({true, addr, reg, std::vector<uint8_t>(data, data + len)});
    if (fail_next > 0) {
      --fail_next;
      return false;
    }
    if (addr != address_) {
      return false;                               // NACK
    }
    uint8_t p = reg;
    for (size_t i = 0; i < len; ++i) {
      store(p, data[i]);
      if (regs_[MODE1] & 0x20) {
        ++p;
      }
    }
    return true;
  }

  bool Read(uint8_t addr, uint8_t reg, uint8_t* data, size_t len) noexcept {
    log.push_back({false, addr, reg, {}});
    if (fail_next > 0) {
      --fail_next;
      return false;
    }
    if (addr != address_) {
      return false;
    }
    uint8_t p = reg;
    for (size_t i = 0; i < len; ++i) {
      data[i] = regs_[p];
      if (regs_[MODE1] & 0x20) {
        ++p;
      }
    }
    return true;
  }

  uint8_t reg(uint8_t r) const noexcept { return regs_[r]; }
  void setReg(uint8_t r, uint8_t v) noexcept { regs_[r] = v; }

  std::vector<I2cTransaction> log;
  int fail_next = 0;
  bool init_ok = true;
  int init_calls = 0;

private:
  void store(uint8_t r, uint8_t v) noexcept {
    if (r == PRE_SCALE && (regs_[MODE1] & 0x10) == 0) {
      return;                                     // blocked while awake (Table 4, note 1)
    }
    if (r == MODE1 && (v & 0x80)) {
      v &= 0x7F;                                  // writing 1 to RESTART clears it
    }
    regs_[r] = v;
    if (r >= ALL_LED_ON_L && r <= ALL_LED_ON_L + 3) {
      for (int ch = 0; ch < 16; ++ch) {
        regs_[LED0_ON_L + 4 * ch + (r - ALL_LED_ON_L)] = v;  // ALL_LED fans out to every channel (7.3.4)
      }
    }
  }

  uint8_t address_;
  std::array<uint8_t, 256> regs_{};
};
