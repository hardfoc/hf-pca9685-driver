# Quick Start

This guide will get you up and running with the PCA9685 driver in just a few steps.

## Prerequisites

- [Driver installed](installation.md)
- [Hardware wired](hardware_setup.md)
- [I2C interface implemented](platform_integration.md)

## Minimal Example

Here's a complete working example:

```cpp
#include "pca9685.hpp"

// 1. Implement the I2C interface
class MyI2c : public pca9685::I2cInterface<MyI2c> {
public:
    bool EnsureInitialized() noexcept { return true; }
    bool Write(uint8_t addr, uint8_t reg, const uint8_t *data, size_t len) noexcept {
        // Your I2C write implementation
        return true;
    }
    
    bool Read(uint8_t addr, uint8_t reg, uint8_t *data, size_t len) noexcept {
        // Your I2C read implementation
        return true;
    }
};

// 2. Create instances
MyI2c i2c;
pca9685::PCA9685<MyI2c> pwm(&i2c, 0x40); // 0x40 is default I2C address

// 3. Initialize
if (!pwm.Reset()) {
    // Handle initialization error
    return;
}

// 4. Set PWM frequency (required before setting channels)
pwm.SetPwmFreq(50.0f); // 50 Hz for servos

// 5. Set channel 0 to 50% duty cycle
pwm.SetDuty(0, 0.5f);
```

## Step-by-Step Explanation

### Step 1: Include the Header

```cpp
#include "pca9685.hpp"
```

### Step 2: Implement the I2C Interface

You need to implement the `I2cInterface` for your platform. See
[Platform Integration](platform_integration.md) for detailed examples for ESP32, STM32, and Arduino.

The interface requires two methods:
- `Write(uint8_t addr, uint8_t reg, const uint8_t *data, size_t len)` - Write data to a register
- `Read(uint8_t addr, uint8_t reg, uint8_t *data, size_t len)` - Read data from a register

### Step 3: Create Driver Instance

```cpp
MyI2c i2c;
pca9685::PCA9685<MyI2c> pwm(&i2c, 0x40);
```

The constructor takes:
- Pointer to your I2C interface implementation
- I2C address (0x40 is the default, can be changed via A0-A5 pins)

### Step 4: Initialize

```cpp
if (!pwm.Reset()) {
    // Handle initialization failure
    uint16_t flags = pwm.GetErrorFlags(); // Bitmask error flags
    auto error = pwm.GetLastError();      // Convenience accessor
}
```

`Reset()` puts the device in a known state and must be called before other operations.

### Step 5: Set PWM Frequency

```cpp
pwm.SetPwmFreq(50.0f); // 50 Hz for servos
```

**Important**: You must set the frequency before setting channel outputs. Valid range is 24-1526 Hz.

### Step 6: Control Channels

```cpp
// Set duty cycle (0.0 = off, 1.0 = fully on)
pwm.SetDuty(0, 0.5f); // Channel 0 at 50%

// Or set precise timing
pwm.SetPwm(1, 0, 2048); // Channel 1: on at 0, off at 2048 (50% duty)
```

## Expected Output

When running this example with a servo on channel 0:
- The servo should move to its center position (1.5ms pulse at 50 Hz)
- No error messages should appear

## Troubleshooting

If you encounter issues:

- **Compilation errors**: Check that you've implemented all required I2C interface methods
- **Initialization fails**: Verify hardware connections and I2C address
- **No output**: Ensure `SetPwmFreq()` was called before setting channels
- **See**: [Troubleshooting](troubleshooting.md) for common issues

## Next Steps

- Explore [Examples](examples.md) for more advanced usage
- Review the [API Reference](api_reference.md) for all available methods
- Check [Configuration](configuration.md) for customization options

---

**Navigation**
⬅️ [Installation](installation.md) | [Next: Hardware Setup ➡️](hardware_setup.md) | [Back to Index](index.md)

