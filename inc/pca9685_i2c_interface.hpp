/**
 * @file pca9685_i2c_interface.hpp
 * @brief CRTP-based I2C interface for PCA9685 driver
 * @copyright Copyright (c) 2024-2025 HardFOC. All rights reserved.
 */
#pragma once
#include <cstddef>
#include <cstdint>

namespace pca9685 {

// ============================================================================
// GPIO Enums -- Standardised Control Pin Model
// ============================================================================

/**
 * @enum CtrlPin
 * @brief Identifies the hardware control pins of the PCA9685.
 *
 * Used with `GpioSet()` / `GpioSetActive()` / `GpioSetInactive()`
 * to control the IC's dedicated GPIO pins through the I2cInterface.
 *
 * The mapping from `GpioSignal::ACTIVE` / `INACTIVE` to physical HIGH / LOW
 * is determined by the platform bus implementation:
 * - **OE**: Active-low (ACTIVE → physical LOW, outputs enabled)
 */
enum class CtrlPin : uint8_t {
  OE = 0 ///< Output Enable (active-low on the physical pin)
};

/**
 * @enum GpioSignal
 * @brief Abstract signal level for control pins.
 *
 * Decouples the driver's intent from the physical pin polarity.
 */
enum class GpioSignal : uint8_t {
  INACTIVE = 0, ///< Pin function is deasserted
  ACTIVE   = 1  ///< Pin function is asserted
};

/**
 * @brief CRTP-based template interface for I2C bus operations
 *
 * This template class provides a hardware-agnostic interface for I2C
 * communication using the CRTP pattern. Platform-specific implementations
 * should inherit from this template with themselves as the template parameter.
 *
 * Benefits of CRTP:
 * - Compile-time polymorphism (no virtual function overhead)
 * - Static dispatch instead of dynamic dispatch
 * - Better optimization opportunities for the compiler
 *
 * Example usage:
 * @code
 * class MyI2C : public pca9685::I2cInterface<MyI2C> {
 * public:
 *   bool Write(uint8_t addr, uint8_t reg, const uint8_t* data, size_t len) noexcept { ... }
 *   bool Read(uint8_t addr, uint8_t reg, uint8_t* data, size_t len) noexcept { ... }
 *   bool EnsureInitialized() noexcept { ... }
 *   void DelayUs(uint32_t us) noexcept { ... }                  // optional, recommended
 * };
 * @endcode
 *
 * Optional hooks, detected at compile time (a port that does not declare them still compiles):
 * - `void GpioSet(CtrlPin pin, GpioSignal signal) noexcept`: drive the OE pin (see GpioSet()
 *   below). Called by PCA9685::SetOutputEnable(); the default does nothing.
 * - `void DelayUs(uint32_t us) noexcept`: busy-wait or sleep for at least `us` microseconds. The
 *   driver calls it with 500 after clearing MODE1.SLEEP (Reset(), Wake(), SetPwmFreq()), for the
 *   oscillator start-up time of datasheet Table 5 note 2 and section 7.3.1.1. Without it the
 *   driver times the wait with 13 one-byte MODE1 reads, which is at least 500 us on any bus clock
 *   up to the chip's 1 MHz maximum, but only if each Read() really runs a bus transaction.
 *
 * @tparam Derived The derived class type (CRTP pattern)
 */
template <typename Derived>
class I2cInterface {
public:
  /**
   * @brief Write bytes to a device register.
   * @param addr 7-bit I2C address of the target device.
   * @param reg Register address to write to.
   * @param data Pointer to the data buffer containing bytes to send.
   * @param len Number of bytes to write from the buffer.
   * @return true if the device acknowledges the transfer; false on NACK or error.
   */
  bool Write(uint8_t addr, uint8_t reg, const uint8_t* data, size_t len) noexcept {
    return static_cast<Derived*>(this)->Write(addr, reg, data, len);
  }

  /**
   * @brief Read bytes from a device register.
   * @param addr 7-bit I2C address of the target device.
   * @param reg Register address to read from.
   * @param data Pointer to the buffer to store received data.
   * @param len Number of bytes to read into the buffer.
   * @return true if the read succeeds; false on NACK or error.
   */
  bool Read(uint8_t addr, uint8_t reg, uint8_t* data, size_t len) noexcept {
    return static_cast<Derived*>(this)->Read(addr, reg, data, len);
  }

  /**
   * @brief Ensure the I2C bus is initialized and ready for communication.
   *
   * This method performs lazy initialization of the I2C bus. It should initialize
   * the I2C hardware, configure pins, set up the bus, and verify it's ready for
   * communication. On subsequent calls, it should return true immediately if
   * already initialized.
   *
   * @return true if initialization succeeded or was already initialized;
   *         false if initialization failed.
   */
  bool EnsureInitialized() noexcept {
    return static_cast<Derived*>(this)->EnsureInitialized();
  }

  // --------------------------------------------------------------------------
  /// @name GPIO Pin Control
  ///
  /// Unified interface for controlling PCA9685 hardware control pins.
  /// @{

  /**
   * @brief Set a control pin to the specified signal state.
   *
   * For CtrlPin::OE, ACTIVE means outputs enabled, which is a LOW level on the pin; INACTIVE
   * means outputs blanked to the MODE2.OUTNE state, a HIGH level (datasheet 7.4, Table 11). The
   * port does the polarity mapping.
   *
   * @param[in] pin     Which control pin to drive (OE).
   * @param[in] signal  ACTIVE to assert the pin function, INACTIVE to deassert.
   *
   * @note The default implementation is a no-op, for boards where OE is tied low. Declare the
   *       same function in the derived class if the OE pin is wired to a GPIO: the driver's
   *       SetOutputEnable() and the helpers below call the derived version.
   */
  void GpioSet(CtrlPin pin, GpioSignal signal) noexcept {
    (void)pin;
    (void)signal;
  }

  /**
   * @brief Assert a control pin (set to ACTIVE).
   * @param[in] pin  Which control pin to assert.
   */
  void GpioSetActive(CtrlPin pin) noexcept {
    static_cast<Derived*>(this)->GpioSet(pin, GpioSignal::ACTIVE);
  }

  /**
   * @brief Deassert a control pin (set to INACTIVE).
   * @param[in] pin  Which control pin to deassert.
   */
  void GpioSetInactive(CtrlPin pin) noexcept {
    static_cast<Derived*>(this)->GpioSet(pin, GpioSignal::INACTIVE);
  }

  /// @}

  /** @brief Copy constructor deleted (non-copyable; avoids slicing). */
  I2cInterface(const I2cInterface&) = delete;
  /** @brief Copy assignment deleted (non-copyable). */
  I2cInterface& operator=(const I2cInterface&) = delete;
  /** @brief Move constructor deleted (non-movable; use derived type to move). */
  I2cInterface(I2cInterface&&) = delete;
  /** @brief Move assignment deleted (non-movable). */
  I2cInterface& operator=(I2cInterface&&) = delete;

protected:
  /**
   * @brief Protected constructor to prevent direct instantiation
   */
  I2cInterface() = default;

  /**
   * @brief Protected destructor
   * @note Derived classes can have public destructors
   */
  ~I2cInterface() = default;
};

} // namespace pca9685
