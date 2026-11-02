# API Reference

Complete reference documentation for all public methods and types in the PCA9685 driver.

## Source Code

- **Main Header**: [`inc/pca9685.hpp`](../inc/pca9685.hpp)
- **I2C Interface**: [`inc/pca9685_i2c_interface.hpp`](../inc/pca9685_i2c_interface.hpp)
- **Implementation**: [`src/pca9685.ipp`](../src/pca9685.ipp) (template implementation, included by header)

## Core Class

### `PCA9685<I2cType>`

Main driver class for interfacing with the PCA9685 PWM controller.

**Template Parameter**: `I2cType` - Your I2C interface implementation (must inherit from `pca9685::I2cInterface<I2cType>`)

**Location**: [`inc/pca9685.hpp`](../inc/pca9685.hpp)

**Constructor:**
```cpp
PCA9685(I2cType* bus, uint8_t address);
```

## Methods

### Initialization

| Method | Signature | Description |
|--------|-----------|-------------|
| `EnsureInitialized()` | `bool EnsureInitialized() noexcept` | Lazy initialization - ensures bus and device are ready |
| `IsInitialized()` | `bool IsInitialized() const noexcept` | Check if driver has been initialized |
| `Reset()` | `bool Reset() noexcept` | Reset device to power-on default state |

### Frequency Control

| Method | Signature | Description |
|--------|-----------|-------------|
| `SetPwmFreq()` | `bool SetPwmFreq(float freq_hz) noexcept` | Set PWM frequency for all channels (24-1526 Hz) |
| `GetPrescale()` | `bool GetPrescale(uint8_t& prescale) noexcept` | Get current prescale register value |

### PWM Control

| Method | Signature | Description |
|--------|-----------|-------------|
| `SetPwm()` | `bool SetPwm(uint8_t channel, uint16_t on_time, uint16_t off_time) noexcept` | Set PWM on/off time for a channel |
| `SetDuty()` | `bool SetDuty(uint8_t channel, float duty) noexcept` | Set duty cycle (0.0-1.0) for a channel |
| `SetAllPwm()` | `bool SetAllPwm(uint16_t on_time, uint16_t off_time) noexcept` | Set all channels to the same PWM value |
| `SetChannelFullOn()` | `bool SetChannelFullOn(uint8_t channel) noexcept` | Set channel to fully ON (100% duty) |
| `SetChannelFullOff()` | `bool SetChannelFullOff(uint8_t channel) noexcept` | Set channel to fully OFF (0% duty) |

### Power Management

| Method | Signature | Description |
|--------|-----------|-------------|
| `Sleep()` | `bool Sleep() noexcept` | Put PCA9685 into low-power sleep mode |
| `Wake()` | `bool Wake() noexcept` | Wake PCA9685 from sleep mode |

### Output Configuration

| Method | Signature | Description |
|--------|-----------|-------------|
| `SetOutputInvert()` | `bool SetOutputInvert(bool invert) noexcept` | Set output polarity inversion |
| `SetOutputDriverMode()` | `bool SetOutputDriverMode(bool totem_pole) noexcept` | Set totem-pole or open-drain mode |

### Error Handling

| Method | Signature | Description |
|--------|-----------|-------------|
| `GetLastError()` | `Error GetLastError() const noexcept` | Get last error code (single-error convenience) |
| `GetErrorFlags()` | `uint16_t GetErrorFlags() const noexcept` | Get accumulated error flags (bitmask) |
| `HasError()` | `bool HasError(Error e) const noexcept` | Check if a specific error flag is set |
| `HasAnyError()` | `bool HasAnyError() const noexcept` | Check if any error flag is set |
| `ClearError()` | `void ClearError(Error e) noexcept` | Clear a single error flag |
| `ClearErrorFlags()` | `void ClearErrorFlags(uint16_t mask = 0xFFFF) noexcept` | Clear error flags by bitmask |

### Configuration

| Method | Signature | Description |
|--------|-----------|-------------|
| `SetRetries()` | `void SetRetries(int retries) noexcept` | Set I2C retry count for register operations |
| `SetRetryDelay()` | `void SetRetryDelay(RetryDelayFn fn) noexcept` | Set optional callback invoked between retries (e.g. 1 ms delay for bus recovery); default nullptr |

## Types

### Type Aliases

| Type | Definition | Description |
|------|-------------|-------------|
| `RetryDelayFn` | `void (*)()` | Optional callback for delay between I2C retries; used with `SetRetryDelay()`. |

### Enumerations

| Type | Values | Location |
|------|--------|----------|
| `Error` | `None`, `I2cWrite`, `I2cRead`, `InvalidParam`, `DeviceNotFound`, `NotInitialized`, `OutOfRange` | [`inc/pca9685.hpp`](../inc/pca9685.hpp) |
| `Register` | `MODE1`, `MODE2`, `LED0_ON_L`, `LED0_OFF_L`, `PRE_SCALE`, etc. | [`inc/pca9685.hpp`](../inc/pca9685.hpp) |

### Constants

| Constant | Value | Description |
|----------|-------|-------------|
| `MAX_CHANNELS_` | `16` | Maximum number of PWM channels |
| `MAX_PWM_` | `4095` | Maximum PWM value (12-bit) |
| `OSC_FREQ_` | `25000000` | Internal oscillator frequency (25 MHz) |

## I2C Interface

### `I2cInterface<Derived>` (CRTP)

Hardware-agnostic I2C interface using the Curiously Recurring Template Pattern.

**Location**: [`inc/pca9685_i2c_interface.hpp`](../inc/pca9685_i2c_interface.hpp)

The interface is **non-copyable and non-movable**; use references or pointers to the concrete bus type.

| Method | Signature | Description |
|--------|-----------|-------------|
| `Write()` | `bool Write(uint8_t addr, uint8_t reg, const uint8_t* data, size_t len) noexcept` | Write bytes to a device register |
| `Read()` | `bool Read(uint8_t addr, uint8_t reg, uint8_t* data, size_t len) noexcept` | Read bytes from a device register |
| `EnsureInitialized()` | `bool EnsureInitialized() noexcept` | Ensure I2C bus is initialized and ready |

The driver supports an optional retry delay via **SetRetryDelay()** (a function pointer). The I2C
implementation can expose a static delay (e.g. `Esp32Pca9685Bus::RetryDelay`) and the app passes it
to the driver after construction.

---

**Navigation**
⬅️ [Configuration](configuration.md) | [Next: Examples ➡️](examples.md) | [Back to Index](index.md)
