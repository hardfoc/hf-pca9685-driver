# Troubleshooting

This guide helps you diagnose and resolve common issues when using the PCA9685 driver.

## Common Error Messages

### Error: Initialization Fails (`Reset()` returns false)

**Symptoms:**
- `Reset()` returns `false`
- `GetLastError()` returns `I2cWrite` (or `GetErrorFlags()` has `I2cWrite` bit set)

**Causes:**
- I2C bus not initialized
- Wrong I2C address
- Hardware connections incorrect
- Pull-up resistors missing

**Solutions:**
1. **Verify I2C bus initialization**:
   ```cpp
   // Ensure I2C is initialized before creating driver.
   // ESP32: use driver/i2c_master.h (e.g. i2c_new_master_bus) as in
   // examples/esp32/main/esp32_pca9685_bus.hpp — or your platform's I2C init.
   ```

2. **Check I2C address**:
   - Default is 0x40 (all A0-A5 pins to GND)
   - Use I2C scanner to verify device address
   - Update address in constructor if different

3. **Verify hardware connections**:
   - Check SDA/SCL connections
   - Verify 4.7kΩ pull-up resistors on SDA and SCL
   - Ensure power (3.3V or 5V) is connected

4. **Test I2C interface**:
   - Verify your I2C interface implementation works with other devices
   - Check I2C bus speed (try 100 kHz if 400 kHz fails)

---

### Error: Frequency Out of Range

**Symptoms:**
- `SetPwmFreq()` returns `false`
- `GetLastError()` returns `OutOfRange` (or `GetErrorFlags()` has `OutOfRange` bit set)

**Causes:**
- Frequency value outside valid range (24-1526 Hz)

**Solutions:**
```cpp
// Clamp frequency to valid range
float freq = 2000.0f; // Too high
freq = std::max(24.0f, std::min(1526.0f, freq));
pwm.SetPwmFreq(freq);
```

---

### Error: Channel Out of Range

**Symptoms:**
- `SetPwm()` or `SetDuty()` returns `false`
- `GetLastError()` returns `OutOfRange` (or `GetErrorFlags()` has `OutOfRange` bit set)

**Causes:**
- Channel number >= 16
- PWM values > 4095

**Solutions:**
```cpp
// Validate channel number
if (channel < 16) {
    pwm.SetDuty(channel, 0.5f);
}

// Validate PWM values
if (on_time <= 4095 && off_time <= 4095) {
    pwm.SetPwm(channel, on_time, off_time);
}
```

---

### Error: Not Initialized

**Symptoms:**
- Methods return `false`
- `GetLastError()` returns `NotInitialized` (or `GetErrorFlags()` has `NotInitialized` bit set)

**Causes:**
- `Reset()` not called before other operations

**Solutions:**
```cpp
// Always call Reset() first
pwm.Reset();
pwm.SetPwmFreq(50.0f); // Now this will work
```

---

## Hardware Issues

### Device Not Detected

**Symptoms:**
- Initialization fails
- No response from device
- I2C scanner doesn't find device

**Checklist:**
- [ ] Verify power supply voltage (2.3V-5.5V)
- [ ] Check all connections are secure
- [ ] Verify pull-up resistors (4.7kΩ) on SCL and SDA
- [ ] Check I2C address configuration (A0-A5 pins)
- [ ] Use I2C scanner to detect device address
- [ ] Verify ground connection
- [ ] Check for short circuits

**Debugging:**
- Use oscilloscope/logic analyzer to verify I2C bus activity
- Check for proper I2C start/stop conditions
- Verify ACK/NACK responses

---

### Communication Errors

**Symptoms:**
- Timeout errors
- NACK errors (I2C)
- Intermittent failures

**Solutions:**
1. **Check bus speed**:
   ```cpp
   // Reduce speed if using long wires (e.g. in Esp32Pca9685Bus::I2CConfig)
   config.frequency = 100000;  // 100 kHz instead of 400 kHz
   ```

2. **Verify signal integrity**:
   - Check for noise on I2C lines
   - Ensure proper ground plane
   - Keep traces short (< 10 cm)

3. **Check bus termination**:
   - Verify pull-up resistors are correct value
   - Check for multiple pull-ups (should only be one set)

4. **Verify power supply**:
   - Ensure 3.3V/5V is stable
   - Check for voltage drops under load
   - Add decoupling capacitor (100 nF) close to VDD pin

---

### No Output on Channels

**Symptoms:**
- Driver initializes successfully
- No PWM output on channels
- Servos/LEDs don't respond

**Checklist:**
- [ ] Did you call `SetPwmFreq()` before setting channels?
- [ ] Are channel values within valid range (0-4095)?
- [ ] Is OE pin pulled LOW (or floating)? (OE HIGH disables outputs)
- [ ] Are outputs connected correctly?
- [ ] Check with oscilloscope/multimeter for PWM signal

**Solutions:**
```cpp
// Correct sequence
pwm.Reset();           // 1. Initialize
pwm.SetPwmFreq(50.0f); // 2. Set frequency (REQUIRED!)
pwm.SetDuty(0, 0.5f);  // 3. Now set channels
```

---

## Software Issues

### Compilation Errors

**Error: "No matching function"**

**Solution:**
- Ensure you've implemented all required `I2cInterface` methods
- Check method signatures match exactly:
  ```cpp
  bool EnsureInitialized() noexcept;
  bool Write(uint8_t addr, uint8_t reg, const uint8_t *data, size_t len) noexcept;
  bool Read(uint8_t addr, uint8_t reg, uint8_t *data, size_t len) noexcept;
  ```

**Error: "Undefined reference"**

**Solution:**
- The driver is template-based: implementation is in `pca9685.ipp`, included by `pca9685.hpp`.
  Ensure both `inc/` and `src/` are on the include path so the header can find the `.ipp` file.
  No separate library needs to be linked.

---

### Runtime Errors

**Initialization Fails**

**Checklist:**
- [ ] I2C bus is properly initialized
- [ ] Hardware connections are correct
- [ ] I2C address matches hardware configuration
- [ ] Pull-up resistors are present

**Unexpected Behavior**

**Checklist:**
- [ ] Frequency is set before setting channels
- [ ] Channel numbers are 0-15
- [ ] Duty cycle values are 0.0-1.0
- [ ] PWM values are 0-4095
- [ ] Error handling code checks return values

---

## Debugging Tips

### Enable Debug Output

Add debug prints to your I2C interface:

```cpp
bool Write(uint8_t addr, uint8_t reg, const uint8_t *data, size_t len) noexcept {
    printf("I2C Write: addr=0x%02X, reg=0x%02X, len=%zu\n", addr, reg, len);
    // ... your implementation
    bool result = /* ... */;
    printf("I2C Write result: %s\n", result ? "OK" : "FAIL");
    return result;
}
```

### Use I2C Scanner

Scan the I2C bus to verify device detection:

```cpp
void i2c_scanner() {
    for (uint8_t addr = 0x08; addr < 0x78; addr++) {
        // Try to write to address
        if (/* device responds */) {
            printf("Device found at 0x%02X\n", addr);
        }
    }
}
```

### Check Error Flags

Always check and log error flags. Errors are now `uint16_t` bitmask flags:

```cpp
if (!pwm.SetPwmFreq(50.0f)) {
    // Preferred: use bitmask error flags
    uint16_t flags = pwm.GetErrorFlags();
    printf("Error flags: 0x%04X\n", flags);
    pwm.ClearErrorFlags(flags); // Clear after handling

    // Or use convenience accessor
    auto error = pwm.GetLastError();
    printf("Error: %d\n", static_cast<int>(error));
}
```

### Use Logic Analyzer

For I2C communication issues, a logic analyzer can help:
- Verify correct I2C protocol
- Check for ACK/NACK responses
- Identify timing issues
- Debug address conflicts

---

## FAQ

### Q: Why do I need to call SetPwmFreq() before setting channels?

**A:** The PCA9685 requires the prescale register to be set before PWM outputs work correctly. The
driver enforces this by checking initialization state. Always call `Reset()` then `SetPwmFreq()`
before setting channels.

### Q: Can I change frequency after setting channels?

**A:** Yes, but it will affect all channels. Changing frequency updates the prescale register, which
changes the PWM period for all 16 channels simultaneously.

### Q: What's the difference between SetPwm() and SetDuty()?

**A:**
- `SetPwm()` gives you precise control over on/off timing (0-4095 ticks each)
- `SetDuty()` is a convenience method that sets duty cycle as a float (0.0-1.0), automatically calculating the off_time

### Q: Can I use multiple PCA9685 devices?

**A:** Yes! Configure different I2C addresses via A0-A5 pins, then create separate driver instances:
```cpp
pca9685::PCA9685<MyI2c> pwm1(&i2c, 0x40); // First device
pca9685::PCA9685<MyI2c> pwm2(&i2c, 0x41); // Second device
```

### Q: Why are my servos not moving?

**A:** Common causes:
1. Frequency not set (must call `SetPwmFreq(50.0f)` first)
2. Duty cycle out of servo range (try 0.05-0.10)
3. Servo power supply insufficient
4. OE pin pulled HIGH (disables outputs)

### Q: How do I control the OE (Output Enable) pin?

**A:** The driver doesn't control OE. You must control it via your platform's GPIO:
```cpp
// Example: ESP32
gpio_set_direction(OE_PIN, GPIO_MODE_OUTPUT);
gpio_set_level(OE_PIN, 0); // Enable outputs
```

---

## Getting More Help

If you're still experiencing issues:

1. Check the [API Reference](api_reference.md) for method details
2. Review [Examples](examples.md) for working code
3. Search existing issues on GitHub
4. Open a new issue with:
   - Description of the problem
   - Steps to reproduce
   - Hardware setup details
   - Error messages/logs
   - I2C bus analyzer output (if available)

---

**Navigation**
⬅️ [Examples](examples.md) | [Back to Index](index.md)

