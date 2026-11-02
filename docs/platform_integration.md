# Platform Integration Guide

This guide explains how to implement the hardware abstraction interface for the PCA9685 driver on your platform.

## Understanding CRTP (Curiously Recurring Template Pattern)

The PCA9685 driver uses **CRTP** (Curiously Recurring Template Pattern) for hardware abstraction.
This design choice provides several critical benefits for embedded systems:

### Why CRTP Instead of Virtual Functions?

#### 1. **Zero Runtime Overhead**
- **Virtual functions**: Require a vtable lookup (indirect call) = ~5-10 CPU cycles overhead per call
- **CRTP**: Direct function calls = 0 overhead, compiler can inline
- **Impact**: In time-critical embedded code, this matters significantly

#### 2. **Compile-Time Polymorphism**
- **Virtual functions**: Runtime dispatch - the compiler cannot optimize across the abstraction boundary
- **CRTP**: Compile-time dispatch - full optimization, dead code elimination, constant propagation
- **Impact**: Smaller code size, faster execution

#### 3. **Memory Efficiency**
- **Virtual functions**: Each object needs a vtable pointer (4-8 bytes)
- **CRTP**: No vtable pointer needed
- **Impact**: Critical in memory-constrained systems (many MCUs have <64KB RAM)

#### 4. **Type Safety**
- **Virtual functions**: Runtime errors if method not implemented
- **CRTP**: Compile-time errors if method not implemented
- **Impact**: Catch bugs at compile time, not in the field

### How CRTP Works

```cpp
// Base template class (from pca9685_i2c_interface.hpp)
template <typename Derived>
class I2cInterface {
public:
    bool Write(uint8_t addr, uint8_t reg, const uint8_t *data, size_t len) noexcept {
        // Cast 'this' to Derived* and call the derived implementation
        return static_cast<Derived*>(this)->Write(addr, reg, data, len);
    }
    
    bool Read(uint8_t addr, uint8_t reg, uint8_t *data, size_t len) noexcept {
        return static_cast<Derived*>(this)->Read(addr, reg, data, len);
    }

    bool EnsureInitialized() noexcept {
        return static_cast<Derived*>(this)->EnsureInitialized();
    }
};

// Your implementation
class MyI2c : public pca9685::I2cInterface<MyI2c> {
public:
    // This method is called directly (no virtual overhead)
    bool Write(uint8_t addr, uint8_t reg, const uint8_t *data, size_t len) noexcept {
        // Your platform-specific I2C code
    }
    
    bool Read(uint8_t addr, uint8_t reg, uint8_t *data, size_t len) noexcept {
        // Your platform-specific I2C code
    }

    bool EnsureInitialized() noexcept {
        // Lazy-initialize your I2C bus
    }
};
```

The key insight: `static_cast<Derived*>(this)` allows the base class to call methods on the derived
class **at compile time**, not runtime.

### Performance Comparison

| Aspect | Virtual Functions | CRTP |
|--------|------------------|------|
| Function call overhead | ~5-10 cycles | 0 cycles (inlined) |
| Code size | Larger (vtables) | Smaller (optimized) |
| Memory per object | +4-8 bytes (vptr) | 0 bytes |
| Compile-time checks | No | Yes |
| Optimization | Limited | Full |

## Interface Definition

The PCA9685 driver requires you to implement the `I2cInterface` template:

**Location**: [`inc/pca9685_i2c_interface.hpp`](../inc/pca9685_i2c_interface.hpp) (interface); driver class in [`inc/pca9685.hpp`](../inc/pca9685.hpp)

```cpp
template <typename Derived>
class I2cInterface {
public:
    // Required methods (implement all three)
    bool Write(uint8_t addr, uint8_t reg, const uint8_t *data, size_t len) noexcept;
    bool Read(uint8_t addr, uint8_t reg, uint8_t *data, size_t len) noexcept;
    bool EnsureInitialized() noexcept;
};
```

**Note**: `I2cInterface` is non-copyable and non-movable; use references or pointers to your concrete bus type.

**Method Requirements**:
- `Write()`: Write `len` bytes from `data` to register `reg` at I2C address `addr` (7-bit address)
- `Read()`: Read `len` bytes into `data` from register `reg` at I2C address `addr` (7-bit address)
- `EnsureInitialized()`: Lazy-initialize the I2C bus; return true if ready
- Write/Read return `true` on success, `false` on failure (NACK, timeout, etc.)

**Optional retry delay**: The driver can call an optional callback between I2C retries for bus
recovery. Your bus class can expose a static delay (e.g.
`static void RetryDelay() noexcept { vTaskDelay(pdMS_TO_TICKS(1)); }`). After constructing the
driver, the application calls `driver->SetRetryDelay(MyBus::RetryDelay)`. If not set, no delay is
used between retries.

## Implementation Steps

### Step 1: Create Your Implementation Class

```cpp
#include "pca9685.hpp"

class MyPlatformI2c : public pca9685::I2cInterface<MyPlatformI2c> {
private:
    // Your platform-specific members
    i2c_handle_t i2c_handle_;
    bool initialized_ = false;
    
public:
    // Constructor
    MyPlatformI2c(i2c_handle_t handle) : i2c_handle_(handle) {}
    
    // Implement required methods (NO virtual keyword!)
    bool Write(uint8_t addr, uint8_t reg, const uint8_t *data, size_t len) noexcept {
        // Your I2C write implementation
        return true;
    }
    
    bool Read(uint8_t addr, uint8_t reg, uint8_t *data, size_t len) noexcept {
        // Your I2C read implementation
        return true;
    }

    bool EnsureInitialized() noexcept {
        if (initialized_) return true;
        // Initialize I2C hardware...
        initialized_ = true;
        return true;
    }

};
```

### Step 2: Platform-Specific Examples

#### ESP32 (ESP-IDF)

**Location**: See [`examples/esp32/main/esp32_pca9685_bus.hpp`](../examples/esp32/main/esp32_pca9685_bus.hpp)
for a complete ESP32 implementation using ESP-IDF's I2C master driver API.

For a complete working example, see [`examples/esp32/main/pca9685_comprehensive_test.cpp`](../examples/esp32/main/pca9685_comprehensive_test.cpp).

```cpp
#include "driver/i2c_master.h"
#include "pca9685.hpp"
#include "esp32_pca9685_bus.hpp"

// Use the provided ESP32 bus implementation
auto i2c_bus = CreateEsp32Pca9685Bus();
pca9685::PCA9685<Esp32Pca9685Bus> pwm(i2c_bus.get(), 0x40);

// Initialize
if (!pwm.Reset()) {
    // Handle error
    return;
}
```

#### STM32 (HAL)

```cpp
#include "stm32f4xx_hal.h"
#include "pca9685.hpp"

extern I2C_HandleTypeDef hi2c1;

class STM32I2cBus : public pca9685::I2cInterface<STM32I2cBus> {
public:
    bool Write(uint8_t addr, uint8_t reg, const uint8_t *data, size_t len) noexcept {
        // STM32 HAL uses 8-bit address (7-bit << 1)
        return HAL_I2C_Mem_Write(&hi2c1, addr << 1, reg, 
                                 I2C_MEMADD_SIZE_8BIT, 
                                 (uint8_t*)data, len, 
                                 HAL_MAX_DELAY) == HAL_OK;
    }
    
    bool Read(uint8_t addr, uint8_t reg, uint8_t *data, size_t len) noexcept {
        return HAL_I2C_Mem_Read(&hi2c1, addr << 1, reg,
                                I2C_MEMADD_SIZE_8BIT,
                                data, len,
                                HAL_MAX_DELAY) == HAL_OK;
    }

    bool EnsureInitialized() noexcept { return true; /* HAL_I2C_Init done elsewhere */ }

};
```

#### Arduino

```cpp
#include <Wire.h>
#include "pca9685.hpp"

class ArduinoI2cBus : public pca9685::I2cInterface<ArduinoI2cBus> {
public:
    bool Write(uint8_t addr, uint8_t reg, const uint8_t *data, size_t len) noexcept {
        Wire.beginTransmission(addr);
        Wire.write(reg);
        Wire.write(data, len);
        return Wire.endTransmission() == 0;
    }
    
    bool Read(uint8_t addr, uint8_t reg, uint8_t *data, size_t len) noexcept {
        Wire.beginTransmission(addr);
        Wire.write(reg);
        if (Wire.endTransmission(false) != 0) return false;
        
        Wire.requestFrom(addr, len);
        for (size_t i = 0; i < len && Wire.available(); i++) {
            data[i] = Wire.read();
        }
        return true;
    }

    bool EnsureInitialized() noexcept { Wire.begin(); return true; }

};
```

## Common Pitfalls

### Don't Use Virtual Functions

```cpp
// WRONG - defeats the purpose of CRTP
class MyI2c : public pca9685::I2cInterface<MyI2c> {
public:
    virtual bool Write(...) override {  // ❌ Virtual keyword not needed
        // ...
    }
};
```

### Correct CRTP Implementation

```cpp
// CORRECT - no virtual keyword, PascalCase, noexcept
class MyI2c : public pca9685::I2cInterface<MyI2c> {
public:
    bool Write(...) noexcept {  // ✅ Direct implementation
        // ...
    }
};
```

### Don't Forget the Template Parameter

```cpp
// WRONG - missing template parameter
class MyI2c : public pca9685::I2cInterface {  // ❌ Compiler error
    // ...
};
```

### Correct Template Parameter

```cpp
// CORRECT - pass your class as template parameter
class MyI2c : public pca9685::I2cInterface<MyI2c> {  // ✅
    // ...
};
```

### Address Format Confusion

The driver uses **7-bit I2C addresses**. Some platforms use 8-bit addresses (7-bit << 1):

```cpp
// WRONG - if your platform uses 8-bit addresses
i2c_write(addr, ...);  // ❌ Should be addr << 1

// CORRECT
i2c_write(addr << 1, ...);  // ✅ Convert 7-bit to 8-bit
```

## Testing Your Implementation

After implementing the interface, test it:

```cpp
MyPlatformI2c i2c;
pca9685::PCA9685<MyPlatformI2c> pwm(&i2c, 0x40);

if (pwm.Reset()) {
    // Interface works!
    pwm.SetPwmFreq(50.0f);
    pwm.SetDuty(0, 0.5f);
} else {
    // Check error flags (uint16_t bitmask)
    auto flags = pwm.GetErrorFlags();
    // Or use convenience accessor:
    auto error = pwm.GetLastError();
    // Debug your I2C implementation
    pwm.ClearErrorFlags(flags); // Clear after handling
}
```

## Next Steps

- See [Configuration](configuration.md) for driver configuration options
- Check [Examples](examples.md) for complete usage examples
- Review [API Reference](api_reference.md) for all available methods

---

**Navigation**
⬅️ [Hardware Setup](hardware_setup.md) | [Next: Configuration ➡️](configuration.md) | [Back to Index](index.md)

