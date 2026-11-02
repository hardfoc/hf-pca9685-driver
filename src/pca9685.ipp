/**
 * @file pca9685.ipp
 * @brief Hardware-agnostic implementation for the PCA9685 16-channel 12-bit PWM controller (I2C)
 * @copyright Copyright (c) 2024-2025 HardFOC. All rights reserved.
 */
#ifndef PCA9685_IMPL
#define PCA9685_IMPL

// Include standard library headers BEFORE the namespace opens
#include <algorithm>
#include <array>
#include <cmath>
#include <string.h> // NOLINT(modernize-deprecated-headers) - Use C string.h for ESP-IDF compatibility (cstring has issues with ESP-IDF toolchain)

// When included from header, use relative path; when compiled directly, use standard include
#ifdef PCA9685_HEADER_INCLUDED
#include "../inc/pca9685.hpp"
#else
#include "../inc/pca9685.hpp"
#endif

template <typename I2cType>
pca9685::PCA9685<I2cType>::PCA9685(I2cType* bus, uint8_t address) : i2c_(bus), addr_(address) {
  // No initialization here - use EnsureInitialized() or Reset() when ready
}

template <typename I2cType>
bool pca9685::PCA9685<I2cType>::EnsureInitialized() noexcept {
  if (initialized_) {
    return true; // Already initialized
  }
  return Reset();
}

template <typename I2cType>
bool pca9685::PCA9685<I2cType>::Reset() noexcept {
  // Ensure I2C bus is initialized and ready (mirrors PCAL95555 pattern)
  if (!i2c_ || !i2c_->EnsureInitialized()) {
    setError(Error::I2cWrite);
    initialized_ = false;
    return false;
  }

  // MODE1 = AI only: oscillator on (SLEEP = 0), register auto-increment on (datasheet 7.3.1,
  // Table 5) so that the four-byte LEDn / ALL_LED frames reach all four registers.
  if (!writeMode1(0x00)) {
    initialized_ = false;
    return false;
  }
  waitOscillatorStartup(); // SLEEP may have been 1 (it is at power-up): 500 us start-up
  initialized_ = true;
  last_error_ = Error::None;
  return true;
}

template <typename I2cType>
bool pca9685::PCA9685<I2cType>::SetPwmFreq(float freq_hz) noexcept {
  if (!EnsureInitialized()) {
    setError(Error::NotInitialized);
    return false;
  }
  if (freq_hz < 24.0F || freq_hz > 1526.0F) {
    setError(Error::OutOfRange);
    return false;
  }
  uint8_t prescale = calcPrescale(freq_hz);
  uint8_t old_mode = 0;
  if (!readReg(static_cast<uint8_t>(Register::MODE1), old_mode)) {
    return false;
  }
  // PRE_SCALE accepts writes only while MODE1.SLEEP = 1 (Table 4, note 1)
  if (!writeMode1(static_cast<uint8_t>(old_mode | MODE1_SLEEP_))) {
    return false;
  }
  if (!writeReg(static_cast<uint8_t>(Register::PRE_SCALE), prescale)) {
    return false;
  }
  if ((old_mode & MODE1_SLEEP_) == 0) {
    // Was running: wake, give the oscillator 500 us, then write RESTART so that the channels
    // that were running resume (datasheet 7.3.1.1 steps 2 and 3). A 1 written to RESTART when it
    // reads 0 has no effect (Table 5), so it is written unconditionally.
    if (!restartAfterSleep(old_mode, true)) {
      return false;
    }
  }
  last_error_ = Error::None;
  return true;
}

template <typename I2cType>
bool pca9685::PCA9685<I2cType>::SetPwm(uint8_t channel, uint16_t on_time,
                                       uint16_t off_time) noexcept {
  if (!EnsureInitialized()) {
    setError(Error::NotInitialized);
    return false;
  }
  if (channel >= MAX_CHANNELS_ || on_time > MAX_PWM_ || off_time > MAX_PWM_) {
    setError(Error::OutOfRange);
    return false;
  }
  uint8_t reg = static_cast<uint8_t>(Register::LED0_ON_L) + (4 * channel);
  const auto data = pwmFrame(on_time, off_time);
  if (!writeRegBlock(reg, data.data(), 4)) {
    return false;
  }
  last_error_ = Error::None;
  return true;
}

template <typename I2cType>
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - Types are different enough (uint8_t vs
// float)
bool pca9685::PCA9685<I2cType>::SetDuty(uint8_t channel, float duty) noexcept {
  duty = ::std::max(duty, 0.0F);
  duty = ::std::min(duty, 1.0F);
  if (duty >= 1.0F) {
    // 100 %: the full-ON bit, LEDn_ON_H[4] (datasheet 7.3.3, Table 7, p. 24). ON = 0, OFF = 4095
    // would leave the output low for one tick of every period.
    return SetChannelFullOn(channel);
  }
  // Rounds to 0 for duty < 1/8190: SetPwm() turns ON = OFF = 0 into full OFF.
  auto off_time = static_cast<uint16_t>(lroundf((duty * MAX_PWM_)));
  return SetPwm(channel, 0, off_time);
}

template <typename I2cType>
bool pca9685::PCA9685<I2cType>::SetAllPwm(uint16_t on_time, uint16_t off_time) noexcept {
  if (!EnsureInitialized()) {
    setError(Error::NotInitialized);
    return false;
  }
  if (on_time > MAX_PWM_ || off_time > MAX_PWM_) {
    setError(Error::OutOfRange);
    return false;
  }
  const auto data = pwmFrame(on_time, off_time);
  if (!writeRegBlock(static_cast<uint8_t>(Register::ALL_LED_ON_L), data.data(), 4)) {
    return false;
  }
  last_error_ = Error::None;
  return true;
}

template <typename I2cType>
bool pca9685::PCA9685<I2cType>::GetPrescale(uint8_t& prescale) noexcept {
  if (!EnsureInitialized()) {
    setError(Error::NotInitialized);
    return false;
  }
  if (!readReg(static_cast<uint8_t>(Register::PRE_SCALE), prescale)) {
    return false;
  }
  last_error_ = Error::None;
  return true;
}

// ---- Power Management ----

template <typename I2cType>
bool pca9685::PCA9685<I2cType>::Sleep() noexcept {
  if (!EnsureInitialized()) {
    setError(Error::NotInitialized);
    return false;
  }
  uint8_t mode1 = 0;
  if (!readReg(static_cast<uint8_t>(Register::MODE1), mode1)) {
    return false;
  }
  if (!writeMode1(static_cast<uint8_t>(mode1 | MODE1_SLEEP_))) {
    return false;
  }
  last_error_ = Error::None;
  return true;
}

template <typename I2cType>
bool pca9685::PCA9685<I2cType>::Wake() noexcept {
  if (!EnsureInitialized()) {
    setError(Error::NotInitialized);
    return false;
  }
  uint8_t mode1 = 0;
  if (!readReg(static_cast<uint8_t>(Register::MODE1), mode1)) {
    return false;
  }
  const bool asleep = (mode1 & MODE1_SLEEP_) != 0;
  const bool restart = (mode1 & MODE1_RESTART_) != 0;
  if (asleep || restart) {
    // Datasheet 7.3.1.1: clear SLEEP, allow 500 us for the oscillator, then write 1 to RESTART
    // if it read 1, so that the channels that were running resume.
    if (!restartAfterSleep(mode1, restart)) {
      return false;
    }
  }
  last_error_ = Error::None;
  return true;
}

// ---- Output Configuration ----

template <typename I2cType>
bool pca9685::PCA9685<I2cType>::SetOutputInvert(bool invert) noexcept {
  if (!EnsureInitialized()) {
    setError(Error::NotInitialized);
    return false;
  }
  return modifyReg(static_cast<uint8_t>(Register::MODE2), 0x10,
                   invert ? static_cast<uint8_t>(0x10U) : static_cast<uint8_t>(0x00U));
}

template <typename I2cType>
bool pca9685::PCA9685<I2cType>::SetOutputDriverMode(bool totem_pole) noexcept {
  if (!EnsureInitialized()) {
    setError(Error::NotInitialized);
    return false;
  }
  return modifyReg(static_cast<uint8_t>(Register::MODE2), 0x04,
                   totem_pole ? static_cast<uint8_t>(0x04U) : static_cast<uint8_t>(0x00U));
}

template <typename I2cType>
bool pca9685::PCA9685<I2cType>::SetOutputEnable(bool enable) noexcept {
  if (!i2c_) {
    setError(Error::InvalidParam);
    return false;
  }
  // OE is active LOW (datasheet 7.4): ACTIVE = outputs enabled = pin low; the port maps the level.
  i2c_->GpioSet(CtrlPin::OE, enable ? GpioSignal::ACTIVE : GpioSignal::INACTIVE);
  return true;
}

// ---- Channel On/Off ----

template <typename I2cType>
bool pca9685::PCA9685<I2cType>::SetChannelFullOn(uint8_t channel) noexcept {
  if (!EnsureInitialized()) {
    setError(Error::NotInitialized);
    return false;
  }
  if (channel >= MAX_CHANNELS_) {
    setError(Error::OutOfRange);
    return false;
  }
  uint8_t reg = static_cast<uint8_t>(Register::LED0_ON_L) + (4 * channel);
  // Set LEDn_ON_H bit 4 (full-on), clear LEDn_OFF_H bit 4 (full-off)
  ::std::array<uint8_t, 4> data = {0x00, 0x10, 0x00, 0x00};
  if (!writeRegBlock(reg, data.data(), 4)) {
    return false;
  }
  last_error_ = Error::None;
  return true;
}

template <typename I2cType>
bool pca9685::PCA9685<I2cType>::SetChannelFullOff(uint8_t channel) noexcept {
  if (!EnsureInitialized()) {
    setError(Error::NotInitialized);
    return false;
  }
  if (channel >= MAX_CHANNELS_) {
    setError(Error::OutOfRange);
    return false;
  }
  uint8_t reg = static_cast<uint8_t>(Register::LED0_ON_L) + (4 * channel);
  // Clear LEDn_ON_H bit 4, set LEDn_OFF_H bit 4 (full-off)
  ::std::array<uint8_t, 4> data = {0x00, 0x00, 0x00, 0x10};
  if (!writeRegBlock(reg, data.data(), 4)) {
    return false;
  }
  last_error_ = Error::None;
  return true;
}

// ---- Low-level register access with retries ----

template <typename I2cType>
bool pca9685::PCA9685<I2cType>::writeReg(uint8_t reg, uint8_t value) noexcept {
  if (!i2c_) {
    return false;
  }
  for (int attempt = 0; attempt <= retries_; ++attempt) {
    if (i2c_->Write(addr_, reg, &value, 1)) {
      return true;
    }
    if (attempt < retries_ && retry_delay_) {
      retry_delay_();
    }
  }
  setError(Error::I2cWrite);
  return false;
}

template <typename I2cType>
bool pca9685::PCA9685<I2cType>::readReg(uint8_t reg, uint8_t& value) noexcept {
  if (!i2c_) {
    return false;
  }
  for (int attempt = 0; attempt <= retries_; ++attempt) {
    if (i2c_->Read(addr_, reg, &value, 1)) {
      return true;
    }
    if (attempt < retries_ && retry_delay_) {
      retry_delay_();
    }
  }
  setError(Error::I2cRead);
  return false;
}

template <typename I2cType>
bool pca9685::PCA9685<I2cType>::writeRegBlock(uint8_t reg, const uint8_t* data,
                                              size_t len) noexcept {
  if (!i2c_) {
    return false;
  }
  for (int attempt = 0; attempt <= retries_; ++attempt) {
    if (i2c_->Write(addr_, reg, data, len)) {
      return true;
    }
    if (attempt < retries_ && retry_delay_) {
      retry_delay_();
    }
  }
  setError(Error::I2cWrite);
  return false;
}

template <typename I2cType>
bool pca9685::PCA9685<I2cType>::readRegBlock(uint8_t reg, uint8_t* data, size_t len) noexcept {
  if (!i2c_) {
    return false;
  }
  for (int attempt = 0; attempt <= retries_; ++attempt) {
    if (i2c_->Read(addr_, reg, data, len)) {
      return true;
    }
    if (attempt < retries_ && retry_delay_) {
      retry_delay_();
    }
  }
  setError(Error::I2cRead);
  return false;
}

template <typename I2cType>
bool pca9685::PCA9685<I2cType>::writeMode1(uint8_t value) noexcept {
  // Keep AI set on every MODE1 write, and never write 1 to RESTART by accident: a 1 there restarts
  // the PWM channels (datasheet 7.3.1, Table 5 and 7.3.1.1).
  const auto mode1 = static_cast<uint8_t>((value & ~MODE1_RESTART_) | MODE1_AI_);
  return writeReg(static_cast<uint8_t>(Register::MODE1), mode1);
}

template <typename I2cType>
bool pca9685::PCA9685<I2cType>::restartAfterSleep(uint8_t mode1, bool restart) noexcept {
  const auto awake = static_cast<uint8_t>(mode1 & ~MODE1_SLEEP_);
  if (!writeMode1(awake)) {
    return false;
  }
  waitOscillatorStartup(); // SLEEP must be 0 for 500 us before RESTART is written (7.3.1.1)
  if (!restart) {
    return true;
  }
  return writeReg(static_cast<uint8_t>(Register::MODE1),
                  static_cast<uint8_t>((awake & ~MODE1_RESTART_) | MODE1_AI_ | MODE1_RESTART_));
}

template <typename I2cType>
void pca9685::PCA9685<I2cType>::waitOscillatorStartup() noexcept {
  if constexpr (requires(I2cType& bus, uint32_t us) { bus.DelayUs(us); }) {
    i2c_->DelayUs(OSC_STARTUP_US_);
  } else {
    // The port has no DelayUs(): time the wait with bus traffic instead. A one-byte register read
    // is at least 39 SCL periods (START, 4 x 9 bits, repeated START, STOP), 39 us at the chip's
    // fastest clock of 1 MHz (Table 15), so 13 reads of MODE1 take at least 507 us. MODE1 is not a
    // PWM control register, so reading it inside the start-up window is allowed (Table 5, note 2).
    uint8_t scratch = 0;
    for (uint32_t i = 0; i < OSC_STARTUP_FALLBACK_READS_; ++i) {
      (void)i2c_->Read(addr_, static_cast<uint8_t>(Register::MODE1), &scratch, 1);
    }
  }
}

template <typename I2cType>
::std::array<uint8_t, 4> pca9685::PCA9685<I2cType>::pwmFrame(uint16_t on_time,
                                                             uint16_t off_time) noexcept {
  if (on_time == off_time) {
    // LEDn_ON and LEDn_OFF must never hold the same count (datasheet 7.3.4, p. 25). Equal ticks
    // mean zero duty, so write the full-OFF bit, LEDn_OFF_H[4], instead.
    return {0x00, 0x00, 0x00, 0x10};
  }
  return {static_cast<uint8_t>(on_time & 0xFF), static_cast<uint8_t>((on_time >> 8) & 0x0F),
          static_cast<uint8_t>(off_time & 0xFF), static_cast<uint8_t>((off_time >> 8) & 0x0F)};
}

template <typename I2cType>
bool pca9685::PCA9685<I2cType>::modifyReg(uint8_t reg, uint8_t mask, uint8_t value) noexcept {
  uint8_t current = 0;
  if (!readReg(reg, current)) {
    return false;
  }
  uint8_t new_val = (current & static_cast<uint8_t>(~mask)) | (value & mask);
  if (!writeReg(reg, new_val)) {
    return false;
  }
  last_error_ = Error::None;
  return true;
}

template <typename I2cType>
uint8_t pca9685::PCA9685<I2cType>::calcPrescale(float freq_hz) const noexcept {
  float prescale_val = (static_cast<float>(OSC_FREQ_) / (4096.0F * freq_hz)) - 1.0F;
  prescale_val = ::std::max(prescale_val, 3.0F);
  prescale_val = ::std::min(prescale_val, 255.0F);
  return static_cast<uint8_t>(lroundf(prescale_val));
}

// Note: Namespace is closed in pca9685.hpp, not here

#endif // PCA9685_IMPL
