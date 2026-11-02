# Examples

This guide provides complete, working examples demonstrating various use cases for the PCA9685 driver.

## Example 1: Basic Servo Control

This example shows how to control a standard servo motor using the PCA9685.

```cpp
#include "pca9685.hpp"
#include "driver/i2c.h"

// ESP32 I2C implementation
class Esp32I2cBus : public pca9685::I2cInterface<Esp32I2cBus> {
public:
    bool EnsureInitialized() noexcept { return true; }
    bool Write(uint8_t addr, uint8_t reg, const uint8_t* data, size_t len) noexcept {
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
        i2c_master_write_byte(cmd, reg, true);
        i2c_master_write(cmd, (uint8_t*)data, len, true);
        i2c_master_stop(cmd);
        esp_err_t ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000 / portTICK_PERIOD_MS);
        i2c_cmd_link_delete(cmd);
        return ret == ESP_OK;
    }
    
    bool Read(uint8_t addr, uint8_t reg, uint8_t* data, size_t len) noexcept {
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
        i2c_master_write_byte(cmd, reg, true);
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_READ, true);
        i2c_master_read(cmd, data, len, I2C_MASTER_LAST_NACK);
        i2c_master_stop(cmd);
        esp_err_t ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000 / portTICK_PERIOD_MS);
        i2c_cmd_link_delete(cmd);
        return ret == ESP_OK;
    }
};

extern "C" void app_main() {
    // Initialize I2C (not shown - use your platform's I2C init)
    
    Esp32I2cBus i2c;
    pca9685::PCA9685<Esp32I2cBus> pwm(&i2c, 0x40);
    
    // Initialize
    if (!pwm.Reset()) {
        printf("Failed to initialize PCA9685!\n");
        return;
    }
    
    // Set frequency for servos (50 Hz)
    pwm.SetPwmFreq(50.0f);
    
    // Move servo to center position (1.5ms pulse = 7.5% duty at 50 Hz)
    pwm.SetDuty(0, 0.075f);
    
    // Move servo to 90 degrees (2.0ms pulse = 10% duty)
    pwm.SetDuty(0, 0.10f);
    
    // Move servo to 0 degrees (1.0ms pulse = 5% duty)
    pwm.SetDuty(0, 0.05f);
    
    printf("Servo control example complete.\n");
}
```

### Explanation

1. **I2C Interface**: Implement `I2cInterface` for your platform (ESP32 example shown)
2. **Initialization**: Call `Reset()` to initialize the device
3. **Frequency**: Set 50 Hz for standard servos (20ms period)
4. **Duty Cycle**: Use `SetDuty()` with values 0.05-0.10 for typical servo range
   - 0.05 = 1.0ms pulse (0 degrees)
   - 0.075 = 1.5ms pulse (center/90 degrees)
   - 0.10 = 2.0ms pulse (180 degrees)

### Expected Output

The servo should move to different positions as the duty cycle changes.

---

## Example 2: LED Dimming

This example demonstrates smooth LED dimming using PWM.

```cpp
#include "pca9685.hpp"

// ... I2C implementation (same as Example 1) ...

void fade_led(pca9685::PCA9685<Esp32I2cBus>& pwm, uint8_t channel) {
    // Set high frequency for smooth dimming (1 kHz)
    pwm.SetPwmFreq(1000.0f);
    
    // Fade in
    for (float duty = 0.0f; duty <= 1.0f; duty += 0.01f) {
        pwm.SetDuty(channel, duty);
        vTaskDelay(pdMS_TO_TICKS(10)); // 10ms delay
    }
    
    // Fade out
    for (float duty = 1.0f; duty >= 0.0f; duty -= 0.01f) {
        pwm.SetDuty(channel, duty);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

extern "C" void app_main() {
    Esp32I2cBus i2c;
    pca9685::PCA9685<Esp32I2cBus> pwm(&i2c, 0x40);
    
    pwm.Reset();
    
    // Fade LED on channel 0
    fade_led(pwm, 0);
}
```

### Explanation

1. **High Frequency**: Use 1 kHz for smooth LED dimming (avoids visible flicker)
2. **Duty Cycle Sweep**: Gradually change duty from 0.0 to 1.0 for fade effect
3. **Timing**: Small delays between updates create smooth transitions

---

## Example 3: Multiple Channels

This example shows controlling multiple channels simultaneously.

```cpp
#include "pca9685.hpp"

// ... I2C implementation ...

extern "C" void app_main() {
    Esp32I2cBus i2c;
    pca9685::PCA9685<Esp32I2cBus> pwm(&i2c, 0x40);
    
    pwm.Reset();
    pwm.SetPwmFreq(1000.0f); // 1 kHz for LEDs
    
    // Set different duty cycles for different channels
    pwm.SetDuty(0, 0.25f);  // Channel 0: 25% brightness
    pwm.SetDuty(1, 0.50f);  // Channel 1: 50% brightness
    pwm.SetDuty(2, 0.75f);  // Channel 2: 75% brightness
    pwm.SetDuty(3, 1.0f);   // Channel 3: 100% brightness
    
    // Set all remaining channels to 50%
    for (uint8_t ch = 4; ch < 16; ch++) {
        pwm.SetDuty(ch, 0.5f);
    }
}
```

### Explanation

Each channel operates independently. You can set different duty cycles for each of the 16 channels.

---

## Example 4: Error Handling

This example demonstrates proper error handling using the bitmask error model.

```cpp
#include "pca9685.hpp"

// ... I2C implementation ...

extern "C" void app_main() {
    Esp32I2cBus i2c;
    pca9685::PCA9685<Esp32I2cBus> pwm(&i2c, 0x40);
    
    // Initialize with error checking
    if (!pwm.Reset()) {
        // Use GetErrorFlags() for bitmask-based error checking (uint16_t)
        uint16_t flags = pwm.GetErrorFlags();
        printf("Reset failed: error flags 0x%04X\n", flags);
        
        // Or use GetLastError() as a convenience accessor
        auto error = pwm.GetLastError();
        printf("Reset failed: error code %d\n", static_cast<int>(error));
        return;
    }
    
    // Set frequency with validation
    if (!pwm.SetPwmFreq(50.0f)) {
        uint16_t flags = pwm.GetErrorFlags();
        if (flags & static_cast<uint16_t>(pca9685::PCA9685<Esp32I2cBus>::Error::OutOfRange)) {
            printf("Frequency out of range!\n");
        }
        if (flags & static_cast<uint16_t>(pca9685::PCA9685<Esp32I2cBus>::Error::I2cWrite)) {
            printf("I2C write failed!\n");
        }
        pwm.ClearErrorFlags(flags); // Clear after handling
        return;
    }
    
    // Set channel with error checking
    if (!pwm.SetDuty(0, 0.5f)) {
        uint16_t flags = pwm.GetErrorFlags();
        printf("SetDuty failed: error flags 0x%04X\n", flags);
        pwm.ClearErrorFlags(flags);
    }
}
```

### Explanation

Error flags are now `uint16_t` bitmask values:
- `I2cWrite = 1<<0`, `I2cRead = 1<<1`, `InvalidParam = 1<<2`
- `DeviceNotFound = 1<<3`, `NotInitialized = 1<<4`, `OutOfRange = 1<<5`

Use `GetErrorFlags()` to retrieve all accumulated error flags, and `ClearErrorFlags(mask)` to clear
them after handling. The convenience accessor `GetLastError()` still works for simple cases. Multiple
error conditions can be set simultaneously since flags are a bitmask.

---

## Example 5: Bulk Channel Update

This example shows efficient bulk updates using `SetAllPwm()`.

```cpp
#include "pca9685.hpp"

// ... I2C implementation ...

extern "C" void app_main() {
    Esp32I2cBus i2c;
    pca9685::PCA9685<Esp32I2cBus> pwm(&i2c, 0x40);
    
    pwm.Reset();
    pwm.SetPwmFreq(1000.0f);
    
    // Turn all channels on to 50% simultaneously
    pwm.SetAllPwm(0, 2048);
    
    // Turn all channels off
    pwm.SetAllPwm(0, 0);
    
    // Turn all channels fully on
    pwm.SetAllPwm(0, 4095);
}
```

### Explanation

`SetAllPwm()` is more efficient than calling `SetPwm()` 16 times. It writes to the ALL_LED
registers, updating all channels in a single I2C transaction.

---

## Example 6: Power Management and Output Modes

This example demonstrates the power management and output configuration features.

```cpp
#include "pca9685.hpp"

// ... I2C implementation ...

extern "C" void app_main() {
    Esp32I2cBus i2c;
    pca9685::PCA9685<Esp32I2cBus> pwm(&i2c, 0x40);
    
    pwm.Reset();
    pwm.SetPwmFreq(1000.0f);
    
    // Configure retry count for I2C operations (default is 3)
    pwm.SetRetries(3);
    
    // Configure output driver mode (true = totem-pole, false = open-drain)
    pwm.SetOutputDriverMode(true);
    
    // Optionally invert outputs
    pwm.SetOutputInvert(false);
    
    // Set some channels
    pwm.SetDuty(0, 0.5f);
    
    // Use full ON/OFF without PWM
    pwm.SetChannelFullOn(1);   // Channel 1 fully on
    pwm.SetChannelFullOff(2);  // Channel 2 fully off
    
    // Power management: put device to sleep
    pwm.Sleep();
    // ... device is in low-power mode ...
    
    // Wake up and resume
    pwm.Wake();
    // Outputs resume from where they were
}
```

### Explanation

- **`SetRetries()`**: Configures how many times I2C operations are retried on failure (default 3)
- **`SetOutputDriverMode()`**: Selects totem-pole (true) or open-drain (false) output via MODE2 OUTDRV bit
- **`SetOutputInvert()`**: Inverts output logic via MODE2 INVRT bit
- **`SetChannelFullOn()` / `SetChannelFullOff()`**: Sets a channel fully on or off without PWM
- **`Sleep()` / `Wake()`**: Controls the MODE1 SLEEP bit for low-power mode

---

## Running the Examples

### ESP32 Applications

The ESP32 example project provides two applications, built and flashed via scripts (not raw `idf.py`):

| Application | Description |
|-------------|-------------|
| **pca9685_comprehensive_test** | Full driver test suite: init, frequency, PWM, duty cycle, all-channel control, prescale readback, sleep/wake, output config, error handling, stress tests (12 tests total). |
| **pca9685_servo_demo** | 16-channel hobby servo demo: 1000–2000 µs pulse, velocity-limited motion, synchronized animations (Wave, Breathe, Cascade, Mirror, etc.). |

**Prerequisites**: ESP-IDF (e.g. release/v5.5), target e.g. `esp32s3`. From the repo root:

```bash
cd examples/esp32
chmod +x scripts/*.sh
./scripts/setup_repo.sh   # first time only
./scripts/build_app.sh list
```

**Build:**
```bash
./scripts/build_app.sh pca9685_comprehensive_test Debug
# or
./scripts/build_app.sh pca9685_servo_demo Debug
```

**Flash and monitor:**
```bash
./scripts/flash_app.sh flash_monitor pca9685_comprehensive_test Debug
# or
./scripts/flash_app.sh flash_monitor pca9685_servo_demo Debug
```

**I2C pins**: Default SDA=GPIO4, SCL=GPIO5. Override at configure time if needed:
```bash
./scripts/build_app.sh pca9685_comprehensive_test Debug \
  -- -DPCA9685_EXAMPLE_I2C_SDA_GPIO=8 -DPCA9685_EXAMPLE_I2C_SCL_GPIO=9
```

See [examples/esp32/README.md](../examples/esp32/README.md) and
[examples/esp32/docs/](../examples/esp32/docs/) for hardware setup and app details.

### Other Platforms

Adapt the I2C interface implementation for your platform (see
[Platform Integration](platform_integration.md)) and compile with your platform's toolchain. The
ESP32 implementation in [`examples/esp32/main/esp32_pca9685_bus.hpp`](../examples/esp32/main/esp32_pca9685_bus.hpp)
uses ESP-IDF's `driver/i2c_master.h` (new I2C master API) as a reference.

## Next Steps

- Review the [API Reference](api_reference.md) for method details
- Check [Troubleshooting](troubleshooting.md) if you encounter issues
- Explore the [examples directory](../examples/) for more examples

---

**Navigation**
⬅️ [API Reference](api_reference.md) | [Next: Troubleshooting ➡️](troubleshooting.md) | [Back to Index](index.md)

