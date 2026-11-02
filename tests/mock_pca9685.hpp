/**
 * @file mock_pca9685.hpp
 * @brief A model of the PCA9685's I2C register interface for the host unit tests.
 *
 * It keeps the 256-byte register file with the power-on defaults of the datasheet (Rev. 4, section 7.3),
 * records every transaction the driver sends, and applies the rules of the chip that the driver depends on:
 * PRE_SCALE ignores writes while MODE1.SLEEP is 0, a multi-byte access moves the register pointer only when
 * MODE1.AI is 1, and a write to ALL_LED_ON_L..ALL_LED_OFF_H loads that byte into every channel. Failures can
 * be injected per transaction.
 *
 * It also keeps a model clock: every transaction advances it by its length in SCL periods at `bus_khz`, and
 * `DelayUs()` (present only when `HasDelay` is true) advances it by the requested time. With that clock it
 * checks the oscillator start-up rule of section 7.3.1.1: after SLEEP goes from 1 to 0, RESTART must not be
 * written with 1 for 500 us. MODE1.RESTART reads 1 after the chip was put to sleep with a channel running,
 * writing 1 clears it, and writing 0 leaves it unchanged (Table 5).
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

template <bool HasDelay>
class BasicMockPca9685 : public pca9685::I2cInterface<BasicMockPca9685<HasDelay>> {
public:
  static constexpr uint8_t MODE1 = 0x00, MODE2 = 0x01, LED0_ON_L = 0x06, ALL_LED_ON_L = 0xFA,
                           PRE_SCALE = 0xFE;

  explicit BasicMockPca9685(uint8_t address = 0x40) noexcept : address_(address) { powerOn(); }

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
    tx_start_us_ = now_us;
    now_us += busTimeUs(9 * (2 + len) + 2);       // START, address, register, data, STOP
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
    now_us += busTimeUs(9 * (3 + len) + 3);       // START, address, register, Sr, address, data, STOP
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

  void DelayUs(uint32_t us) noexcept
    requires HasDelay
  {
    delays_us.push_back(us);
    now_us += us;
  }

  uint8_t reg(uint8_t r) const noexcept { return regs_[r]; }
  void setReg(uint8_t r, uint8_t v) noexcept { regs_[r] = v; }

  /// Time since MODE1.SLEEP last went from 1 to 0, in microseconds of the model clock.
  double usSinceWake() const noexcept { return now_us - wake_us_; }

  std::vector<I2cTransaction> log;
  std::vector<uint32_t> delays_us;
  int fail_next = 0;
  bool init_ok = true;
  int init_calls = 0;
  unsigned bus_khz = 1000;                        // Fm+: the fastest the chip allows (Table 15)
  double now_us = 0.0;
  int restarts = 0;                               // RESTART written with 1 while it read 1
  int early_restarts = 0;                         // ... less than 500 us after SLEEP was cleared

private:
  double busTimeUs(size_t scl_periods) const noexcept {
    return static_cast<double>(scl_periods) * 1000.0 / bus_khz;
  }

  bool anyChannelRunning() const noexcept {
    for (int ch = 0; ch < 16; ++ch) {
      if ((regs_[LED0_ON_L + 4 * ch + 3] & 0x10) == 0) {
        return true;
      }
    }
    return false;
  }

  void store(uint8_t r, uint8_t v) noexcept {
    if (r == PRE_SCALE && (regs_[MODE1] & 0x10) == 0) {
      return;                                     // blocked while awake (Table 4, note 1)
    }
    if (r == MODE1) {
      const bool was_asleep = (regs_[MODE1] & 0x10) != 0;
      bool restart = (regs_[MODE1] & 0x80) != 0;  // a 0 written to RESTART has no effect
      if (v & 0x80) {
        if (tx_start_us_ - wake_us_ < 500.0) {
          ++early_restarts;                       // section 7.3.1.1: SLEEP low for 500 us first
        }
        if (restart) {
          ++restarts;
        }
        restart = false;                          // writing 1 to RESTART clears it
      }
      const bool asleep = (v & 0x10) != 0;
      if (!was_asleep && asleep && anyChannelRunning()) {
        restart = true;                           // set at the end of the PWM cycle (7.3.1.1)
      }
      if (was_asleep && !asleep) {
        wake_us_ = now_us;
      }
      v = static_cast<uint8_t>((v & 0x7F) | (restart ? 0x80 : 0x00));
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
  double wake_us_ = -1e9;                         // power-up: SLEEP = 1, never woken
  double tx_start_us_ = 0.0;
};

/// The usual port: provides DelayUs().
using MockPca9685 = BasicMockPca9685<true>;
/// A port without DelayUs(): the driver must fall back to bus traffic to wait.
using MockPca9685NoDelay = BasicMockPca9685<false>;
