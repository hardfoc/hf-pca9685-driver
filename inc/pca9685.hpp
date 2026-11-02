/**
 * @file pca9685.hpp
 * @brief Hardware-agnostic driver for the PCA9685 16-channel 12-bit PWM
 * @copyright Copyright (c) 2024-2025 HardFOC. All rights reserved.
 */
#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

#include "pca9685_i2c_interface.hpp"
#include "pca9685_version.h"

namespace pca9685 {

/**
 * @class PCA9685
 * @brief Driver for the PCA9685 16-channel 12-bit PWM controller.
 *
 * This class provides all register-level operations required to configure and
 * operate the PCA9685, including frequency setting, per-channel PWM, and device
 * reset.
 *
 * All I2C operations are routed through the user-supplied I2cInterface.
 *
 * @tparam I2cType The I2C interface implementation type that inherits from
 * pca9685::I2cInterface<I2cType>
 *
 * @note The driver uses CRTP-based I2C interface for zero virtual call
 * overhead.
 */
template <typename I2cType>
class PCA9685 {
public:
  /**
   * @brief Error codes for PCA9685 operations (bitmask flags).
   *
   * Harmonized with PCAL95555 driver error model. Multiple errors can be
   * active simultaneously. Use GetErrorFlags() and ClearErrorFlags() to
   * inspect and clear errors.
   */
  enum class Error : uint16_t {
    None = 0,
    I2cWrite = 1 << 0,       ///< An I2C write operation failed
    I2cRead = 1 << 1,        ///< An I2C read operation failed
    InvalidParam = 1 << 2,   ///< Invalid parameter (channel, value, etc.)
    DeviceNotFound = 1 << 3, ///< Device did not respond
    NotInitialized = 1 << 4, ///< Driver not initialized
    OutOfRange = 1 << 5      ///< Value out of hardware range
  };

  /**
   * @brief PCA9685 register map (I2C register addresses).
   */
  enum class Register : uint8_t {
    MODE1 = 0x00,      ///< Mode register 1 (reset, sleep, auto-increment, etc.)
    MODE2 = 0x01,      ///< Mode register 2 (output drive, invert, output change behaviour)
    SUBADR1 = 0x02,    ///< I2C subaddress 1
    SUBADR2 = 0x03,    ///< I2C subaddress 2
    SUBADR3 = 0x04,    ///< I2C subaddress 3
    ALLCALLADR = 0x05, ///< All-call I2C address
    LED0_ON_L = 0x06,  ///< LED0 output and brightness control byte 0 (on time, low byte)
    LED0_ON_H = 0x07,  ///< LED0 output and brightness control byte 1 (on time, high bits + full-on)
    LED0_OFF_L = 0x08, ///< LED0 output and brightness control byte 2 (off time, low byte)
    LED0_OFF_H =
        0x09, ///< LED0 output and brightness control byte 3 (off time, high bits + full-off)
    /* LED1–LED15: registers 0x0A–0x45 (four bytes per channel, same layout as LED0) */
    ALL_LED_ON_L = 0xFA,  ///< All LED on time, low byte
    ALL_LED_ON_H = 0xFB,  ///< All LED on time, high bits
    ALL_LED_OFF_L = 0xFC, ///< All LED off time, low byte
    ALL_LED_OFF_H = 0xFD, ///< All LED off time, high bits
    PRE_SCALE = 0xFE,     ///< Prescaler for PWM frequency (oscillator / (4096 * freq))
    TESTMODE = 0xFF       ///< Test mode register
  };

  static constexpr uint8_t MAX_CHANNELS_ = 16;    ///< Number of PWM channels (0-15)
  static constexpr uint16_t MAX_PWM_ = 4095;      ///< Maximum tick value (12-bit)
  static constexpr uint32_t OSC_FREQ_ = 25000000; ///< Internal oscillator frequency (Hz)

  /**
   * @brief Construct a new PCA9685 driver instance.
   * @param bus Pointer to a user-implemented I2C interface (must inherit from
   * pca9685::I2cInterface<I2cType>).
   * @param address 7-bit I2C address of the PCA9685 device (0x00 to 0x7F).
   */
  PCA9685(I2cType* bus, uint8_t address);

  /**
   * @brief Ensure the driver and I2C bus are initialized (lazy initialization).
   *
   * On first call, ensures the I2C bus is initialized via
   * I2cInterface::EnsureInitialized(), then performs a device reset to
   * confirm communication. On subsequent calls, returns true immediately
   * if already initialized.
   *
   * @return true if already initialized or initialization succeeded;
   *         false if I2C bus init or device communication failed.
   *
   * @note This mirrors the PCAL95555 driver's lazy initialization pattern.
   */
  bool EnsureInitialized() noexcept;

  /**
   * @brief Check if the driver has been initialized.
   * @return true if EnsureInitialized() or Reset() has completed successfully.
   */
  bool IsInitialized() const noexcept {
    return initialized_;
  }

  /**
   * @brief Reset the device to its power-on default state.
   *
   * Ensures the I2C bus is initialized, then writes MODE1 = 0x20: oscillator on (SLEEP = 0) and
   * register auto-increment on (AI = 1), which the driver's four-byte channel writes need
   * (datasheet 7.3.1, Table 5). Sub-address and All Call responses are turned off. The channel
   * registers, MODE2 and PRE_SCALE are not touched.
   *
   * @return true on success; false on I2C failure.
   */
  bool Reset() noexcept;

  /**
   * @brief Set the PWM frequency for all channels.
   * @param freq_hz Desired frequency in Hz (24-1526 typical).
   * @return true on success; false on I2C failure or invalid parameter.
   */
  bool SetPwmFreq(float freq_hz) noexcept;

  /**
   * @brief Set the PWM on/off time for a channel.
   * @param channel Channel number (0-15).
   * @param on_time Tick count when signal turns ON (0-4095).
   * @param off_time Tick count when signal turns OFF (0-4095).
   * @return true on success; false on I2C failure or invalid parameter.
   */
  bool SetPwm(uint8_t channel, uint16_t on_time, uint16_t off_time) noexcept;

  /**
   * @brief Set the duty cycle for a channel (0.0-1.0).
   * @param channel Channel number (0-15).
   * @param duty Duty cycle (0.0 = always off, 1.0 = always on).
   * @return true on success; false on I2C failure or invalid parameter.
   */
  bool SetDuty(uint8_t channel, float duty) noexcept;

  /**
   * @brief Set all channels to the same PWM value.
   * @param on_time Tick count when signal turns ON (0-4095).
   * @param off_time Tick count when signal turns OFF (0-4095).
   * @return true on success; false on I2C failure.
   */
  bool SetAllPwm(uint16_t on_time, uint16_t off_time) noexcept;

  /**
   * @brief Get the accumulated error flags (bitmask).
   * @return Bitmask of Error values; 0 (Error::None) means no errors.
   */
  [[nodiscard]] uint16_t GetErrorFlags() const noexcept {
    return error_flags_;
  }

  /**
   * @brief Check if a specific error flag is set.
   *
   * @param e The error flag to test.
   * @return true if the flag is set in the current error bitmask.
   *
   * @code
   *   if (driver.HasError(PCA9685::Error::I2cWrite)) { ... }
   * @endcode
   */
  [[nodiscard]] bool HasError(Error e) const noexcept {
    return (error_flags_ & static_cast<uint16_t>(e)) != 0;
  }

  /**
   * @brief Check if any error flag is set.
   * @return true if one or more error flags are set.
   */
  [[nodiscard]] bool HasAnyError() const noexcept {
    return error_flags_ != 0;
  }

  /**
   * @brief Clear a single error flag.
   * @param e The error flag to clear.
   */
  void ClearError(Error e) noexcept {
    error_flags_ &= ~static_cast<uint16_t>(e);
  }

  /**
   * @brief Clear specific error flags by raw bitmask.
   * @param mask Bitmask of Error values to clear (default: all).
   */
  void ClearErrorFlags(uint16_t mask = 0xFFFF) noexcept {
    error_flags_ &= ~mask;
  }

  /**
   * @brief Get the last error code (single-error convenience accessor).
   * @return Error code from the most recent operation.
   */
  [[nodiscard]] Error GetLastError() const noexcept {
    return last_error_;
  }

  /**
   * @brief Get the current prescale value (frequency divider).
   * @param[out] prescale The prescale register value.
   * @return true on success; false on I2C failure.
   */
  [[nodiscard]] bool GetPrescale(uint8_t& prescale) noexcept;

  /**
   * @brief Set the I2C retry count for register read/write operations.
   * @param retries Number of retries (0 = no retries, just one attempt).
   */
  void SetRetries(int retries) noexcept {
    retries_ = retries;
  }

  /**
   * @brief Type of optional callback invoked between I2C retry attempts.
   *
   * Set via SetRetryDelay(). No parameters, no return value. Called only after a
   * failed Read/Write when retries remain (e.g. to perform a short delay for bus recovery).
   */
  using RetryDelayFn = void (*)();

  /**
   * @brief Set optional callback invoked between I2C retry attempts (after a failure, before next
   * try).
   *
   * Set to a function that performs a short delay (e.g. 1–5 ms) to allow bus recovery,
   * or leave default (nullptr) for no delay. The driver calls this only when a Read/Write
   * failed and retries remain.
   * @param fn Callback function, or nullptr to use no delay.
   */
  void SetRetryDelay(RetryDelayFn fn) noexcept {
    retry_delay_ = fn;
  }

  // ---- Power Management ----

  /**
   * @brief Put the PCA9685 into low-power sleep mode.
   *
   * Sets the SLEEP bit in MODE1. All PWM outputs are disabled during sleep.
   * The oscillator is stopped. Use Wake() to resume.
   *
   * @return true on success; false on I2C failure.
   */
  bool Sleep() noexcept;

  /**
   * @brief Wake the PCA9685 from sleep mode.
   *
   * Clears the SLEEP bit in MODE1 and sets the RESTART bit if it was set
   * prior to sleep. PWM outputs resume from their previous values.
   *
   * @return true on success; false on I2C failure.
   */
  bool Wake() noexcept;

  // ---- Output Configuration ----

  /**
   * @brief Set output polarity inversion.
   *
   * When inverted, the output logic is inverted (useful for common-anode LEDs).
   * Affects the INVRT bit in MODE2 register.
   *
   * @param invert true to invert outputs, false for normal polarity.
   * @return true on success; false on I2C failure.
   */
  bool SetOutputInvert(bool invert) noexcept;

  /**
   * @brief Set the output driver mode.
   *
   * Configures outputs as either totem-pole (push-pull) or open-drain.
   * Affects the OUTDRV bit in MODE2 register.
   *
   * @param totem_pole true for totem-pole (default), false for open-drain.
   * @return true on success; false on I2C failure.
   */
  bool SetOutputDriverMode(bool totem_pole) noexcept;

  // ---- Channel On/Off ----

  /**
   * @brief Set a channel to fully ON (100% duty, no PWM).
   *
   * Sets bit 12 of the LEDn_ON register (full-on flag).
   *
   * @param channel Channel number (0-15).
   * @return true on success; false on I2C failure or invalid parameter.
   */
  bool SetChannelFullOn(uint8_t channel) noexcept;

  /**
   * @brief Set a channel to fully OFF (0% duty, no PWM).
   *
   * Sets bit 12 of the LEDn_OFF register (full-off flag).
   *
   * @param channel Channel number (0-15).
   * @return true on success; false on I2C failure or invalid parameter.
   */
  bool SetChannelFullOff(uint8_t channel) noexcept;

  // ===========================================================================
  // Driver Version
  // ===========================================================================

  /**
   * @brief Get the compiled driver version string.
   *
   * Returns the version that was compiled into the binary, in
   * "MAJOR.MINOR.PATCH" format. This is a static method — it can be
   * called without a driver instance.
   *
   * @return Null-terminated version string, e.g. "1.0.0".
   *
   * @code
   *   ESP_LOGI("APP", "PCA9685 driver v%s", PCA9685::GetDriverVersion());
   * @endcode
   */
  static constexpr const char* GetDriverVersion() noexcept {
    return HF_PCA9685_VERSION_STRING;
  }

  /**
   * @brief Get the compiled driver major version number.
   * @return Major version (incremented on breaking API changes).
   */
  static constexpr uint8_t GetDriverVersionMajor() noexcept {
    return HF_PCA9685_VERSION_MAJOR;
  }

  /**
   * @brief Get the compiled driver minor version number.
   * @return Minor version (incremented on new features, backward-compatible).
   */
  static constexpr uint8_t GetDriverVersionMinor() noexcept {
    return HF_PCA9685_VERSION_MINOR;
  }

  /**
   * @brief Get the compiled driver patch version number.
   * @return Patch version (incremented on bug fixes).
   */
  static constexpr uint8_t GetDriverVersionPatch() noexcept {
    return HF_PCA9685_VERSION_PATCH;
  }

private:
  static constexpr uint8_t MODE1_RESTART_ = 0x80; ///< MODE1 bit 7: RESTART (Table 5)
  static constexpr uint8_t MODE1_AI_ = 0x20;      ///< MODE1 bit 5: register auto-increment
  static constexpr uint8_t MODE1_SLEEP_ = 0x10;   ///< MODE1 bit 4: SLEEP (oscillator off)

  I2cType* i2c_;
  uint8_t addr_;
  Error last_error_{Error::None};
  uint16_t error_flags_{0};
  int retries_{3};
  RetryDelayFn retry_delay_{nullptr};
  bool initialized_{false};

  /** @brief Set last error and add to error flags. */
  void setError(Error e) noexcept {
    last_error_ = e;
    error_flags_ |= static_cast<uint16_t>(e);
  }

  /** @brief Write one byte to a register. @param reg Register address. @param value Byte to write.
   * @return true on success. */
  bool writeReg(uint8_t reg, uint8_t value) noexcept;
  /** @brief Read one byte from a register. @param reg Register address. @param[out] value Byte
   * read. @return true on success. */
  bool readReg(uint8_t reg, uint8_t& value) noexcept;
  /** @brief Write a block of bytes to a register. @param reg Start register. @param data Buffer.
   * @param len Length. @return true on success. */
  bool writeRegBlock(uint8_t reg, const uint8_t* data, size_t len) noexcept;
  /** @brief Read a block of bytes from a register. @param reg Start register. @param data Buffer.
   * @param len Length. @return true on success. */
  bool readRegBlock(uint8_t reg, uint8_t* data, size_t len) noexcept;
  /** @brief Compute prescale value for given frequency. @param freq_hz Frequency in Hz. @return
   * Prescale value (0–255). */
  [[nodiscard]] uint8_t calcPrescale(float freq_hz) const noexcept;

  /**
   * @brief Write MODE1 with AI forced to 1 and RESTART forced to 0.
   * @param value New MODE1 value (the AI and RESTART bits are overridden).
   * @return true on success.
   */
  bool writeMode1(uint8_t value) noexcept;

  /**
   * @brief Read-modify-write a single register.
   * @param reg Register address.
   * @param mask Bits to modify.
   * @param value New value for those bits.
   * @return true on success.
   */
  bool modifyReg(uint8_t reg, uint8_t mask, uint8_t value) noexcept;
};

// Include template implementation
#define PCA9685_HEADER_INCLUDED
// NOLINTNEXTLINE(bugprone-suspicious-include) - Intentional: template
// implementation file
#include "../src/pca9685.ipp"
#undef PCA9685_HEADER_INCLUDED

// Public API: Get driver version string
inline const char* GetDriverVersion() noexcept {
  return HF_PCA9685_VERSION_STRING;
}

} // namespace pca9685
