# Configuration

This guide covers all configuration options available for the PCA9685 driver.

## I2C Address Configuration

The PCA9685 I2C address is configured via hardware pins A0-A5. Each pin represents one bit of the 6-bit address field.

| Pin | Address Bit | Description |
|-----|-------------|-------------|
| A0  | Bit 0       | Least significant address bit |
| A1  | Bit 1       | |
| A2  | Bit 2       | |
| A3  | Bit 3       | |
| A4  | Bit 4       | |
| A5  | Bit 5       | Most significant address bit |

**Default I2C address**: `0x40` (all address pins to GND)

**Address Range**: 0x40 to 0x7F (7-bit I2C addresses)

**Example**: To set address 0x41, connect A0 to VDD and A1-A5 to GND.

## PWM Frequency Configuration

### Setting Frequency

Use the `SetPwmFreq()` method to configure the PWM frequency:

```cpp
pwm.SetPwmFreq(50.0f); // 50 Hz for servos
pwm.SetPwmFreq(1000.0f); // 1 kHz for LEDs
```

**Valid Range**: 24 Hz to 1526 Hz (typical)

**Location**: [`src/pca9685.ipp`](../src/pca9685.ipp) (template implementation)

### Frequency Calculation

The driver automatically calculates the prescale value using the formula:

```
prescale = round(25,000,000 / (4096 * freq_hz)) - 1
```

Where:
- `25,000,000` is the internal oscillator frequency (25 MHz)
- `4096` is the PWM resolution (12 bits)
- `freq_hz` is the desired frequency

**Implementation**: [`src/pca9685.ipp`](../src/pca9685.ipp) (`calcPrescale`)

### Common Frequencies

| Application | Frequency | Prescale (approx) |
|-------------|-----------|-------------------|
| Servos | 50 Hz | 121 |
| LEDs (smooth dimming) | 1000 Hz | 5 |
| Motors | 100-500 Hz | 11-61 |

## Channel Configuration

### Individual Channel Control

Set PWM for a single channel:

```cpp
// Set channel 0: on at tick 0, off at tick 2048 (50% duty)
pwm.SetPwm(0, 0, 2048);

// Set duty cycle (0.0 = off, 1.0 = fully on)
pwm.SetDuty(0, 0.5f); // 50% duty
```

**Channel Range**: 0-15 (16 channels total)

**PWM Value Range**: 0-4095 (12-bit resolution)

### Bulk Channel Control

Set all channels simultaneously:

```cpp
// Set all channels to same PWM value
pwm.SetAllPwm(0, 2048); // All channels at 50% duty
```

**Use Case**: Synchronized control of all outputs

## Register Map Reference

The driver abstracts register access, but understanding the register map helps with advanced usage:

| Register | Address | Purpose |
|----------|---------|---------|
| `MODE1` | 0x00 | Main configuration (reset, sleep, auto-increment) |
| `MODE2` | 0x01 | Output configuration (open-drain, totem-pole, inversion) |
| `SUBADR1-3` | 0x02-0x04 | Subaddress registers |
| `ALLCALLADR` | 0x05 | All-call I2C address |
| `LED0_ON_L` | 0x06 | Channel 0 ON time (low byte) |
| `LED0_ON_H` | 0x07 | Channel 0 ON time (high byte) |
| `LED0_OFF_L` | 0x08 | Channel 0 OFF time (low byte) |
| `LED0_OFF_H` | 0x09 | Channel 0 OFF time (high byte) |
| ... | ... | Channels 1-15 follow sequentially |
| `ALL_LED_ON_L` | 0xFA | All channels ON time (low byte) |
| `ALL_LED_ON_H` | 0xFB | All channels ON time (high byte) |
| `ALL_LED_OFF_L` | 0xFC | All channels OFF time (low byte) |
| `ALL_LED_OFF_H` | 0xFD | All channels OFF time (high byte) |
| `PRE_SCALE` | 0xFE | PWM frequency prescaler |
| `TESTMODE` | 0xFF | Test mode register |

**Register Definitions**: [`inc/pca9685.hpp`](../inc/pca9685.hpp) (enum `Register`)

## Advanced Features

### Output Enable (OE) Pin

The OE pin provides hardware control over all outputs:
- **HIGH**: All outputs disabled (high-impedance)
- **LOW/Floating**: All outputs enabled

**Note**: The driver does not control the OE pin. You must handle it externally via GPIO if needed.

### Sleep Mode

The chip supports sleep mode for power saving. The driver handles sleep mode automatically when
changing frequency (see `SetPwmFreq()` implementation).

### Auto-Increment

The chip supports auto-increment for efficient register access. The driver uses this feature internally for multi-byte writes.

## Runtime Configuration

### Initialization Sequence

1. **Reset**: Call `Reset()` to put device in known state
2. **Set Frequency**: Call `SetPwmFreq()` before setting channels
3. **Configure Channels**: Use `SetPwm()` or `SetDuty()` to control outputs

**Example**:
```cpp
pwm.Reset();              // Initialize
pwm.SetPwmFreq(50.0f);   // Set frequency
pwm.SetDuty(0, 0.5f);    // Configure channel
```

## Recommended Settings

### For Servo Control

```cpp
pwm.Reset();
pwm.SetPwmFreq(50.0f); // Standard servo frequency

// Set servo to center position (1.5ms pulse)
pwm.SetDuty(0, 0.075f); // 7.5% duty = 1.5ms / 20ms
```

### For LED Dimming

```cpp
pwm.Reset();
pwm.SetPwmFreq(1000.0f); // High frequency for smooth dimming

// Fade LED on channel 0
for (float duty = 0.0f; duty <= 1.0f; duty += 0.01f) {
    pwm.SetDuty(0, duty);
    delay(10);
}
```

### For Motor Control

```cpp
pwm.Reset();
pwm.SetPwmFreq(200.0f); // Moderate frequency for motors

// Set motor speed
pwm.SetDuty(0, 0.75f); // 75% speed
```

## Next Steps

- See [Examples](examples.md) for configuration examples
- Review [API Reference](api_reference.md) for all configuration methods
- Check [Troubleshooting](troubleshooting.md) for configuration issues

---

**Navigation**
⬅️ [Platform Integration](platform_integration.md) | [Next: API Reference ➡️](api_reference.md) | [Back to Index](index.md)

