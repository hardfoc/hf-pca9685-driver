# Hardware Setup

![PCA9685 16-channel PWM topology with 50 Hz servo timing example](assets/pca9685-topology.svg)

This guide covers the physical connections and hardware requirements for the PCA9685 chip.

## Pin Connections

### Basic I2C Connections

```
MCU              PCA9685
─────────────────────────
3.3V      ────── VDD
GND       ────── GND
SCL       ────── SCL (with 4.7kΩ pull-up to 3.3V)
SDA       ────── SDA (with 4.7kΩ pull-up to 3.3V)
```

### Pin Descriptions

| Pin | Name | Description | Required |
|-----|------|-------------|----------|
| VDD | Power | 2.3V to 5.5V power supply (typically 3.3V or 5V) | Yes |
| GND | Ground | Ground reference | Yes |
| SCL | Clock | I2C clock line | Yes |
| SDA | Data | I2C data line | Yes |
| OE | Output Enable | Active-low output enable (optional) | No |
| A0-A5 | Address | I2C address selection pins | No (for single device) |

### Output Channels

The PCA9685 provides 16 PWM output channels (OUT0-OUT15). Each channel can drive:
- LEDs (with current-limiting resistors)
- Servos (typically 5V servos)
- Other PWM-controlled devices

## Power Requirements

- **Supply Voltage**: 2.3V to 5.5V (3.3V or 5V typical)
- **Current Consumption**:
  - Active: ~10 mA typical
  - Sleep mode: < 1 µA
- **Power Supply**: Stable, low-noise supply recommended
- **Decoupling**: 100 nF ceramic capacitor close to VDD pin recommended

## I2C Configuration

### Address Configuration

The PCA9685 I2C address is determined by pins A0-A5:

| A5 | A4 | A3 | A2 | A1 | A0 | I2C Address (7-bit) |
|----|----|----|----|----|----|---------------------|
| 0  | 0  | 0  | 0  | 0  | 0  | 0x40 (default) |
| 0  | 0  | 0  | 0  | 0  | 1  | 0x41 |
| ... | ... | ... | ... | ... | ... | ... |
| 1  | 1  | 1  | 1  | 1  | 1  | 0x7F |

**Default**: All address pins to GND = **0x40** (used in examples)

### I2C Bus Configuration

- **Speed**: Up to 1 MHz (Fast Mode Plus)
  - Standard Mode: 100 kHz
  - Fast Mode: 400 kHz (most common)
  - Fast Mode Plus: 1 MHz
- **Pull-up Resistors**: 4.7 kΩ on SCL and SDA (required for I2C)
- **Bus Voltage**: Must match VDD (3.3V or 5V)

## Physical Layout Recommendations

- **Trace Length**: Keep I2C traces short (< 10 cm recommended for high speeds)
- **Ground Plane**: Use a ground plane for noise reduction
- **Decoupling**: Place 100 nF ceramic capacitor within 1 cm of VDD pin
- **Routing**: Route clock and data lines away from noise sources (switching regulators, motors)
- **Multiple Devices**: When daisy-chaining, use proper bus termination

## Example Wiring Diagram

![PCA9685 wiring](assets/pca9685-wiring.svg)

### Single PCA9685

```
                    PCA9685
                    ┌─────────┐
        3.3V ───────┤ VDD     │
        GND  ───────┤ GND     │
        SCL  ───────┤ SCL     │─── 4.7kΩ ─── 3.3V
        SDA  ───────┤ SDA     │─── 4.7kΩ ─── 3.3V
                    │         │
                    │ OUT0-15 │─── To LEDs/Servos
                    └─────────┘
```

### Multiple PCA9685 Devices (Daisy Chain)

```
MCU SCL ──┬─── PCA9685 #1 (A0=0, Addr=0x40) SCL
          │
          └─── PCA9685 #2 (A0=1, Addr=0x41) SCL

MCU SDA ──┬─── PCA9685 #1 SDA
          │
          └─── PCA9685 #2 SDA
```

## Output Enable (OE) Pin

The OE pin is an active-low output enable. When pulled HIGH, all outputs are disabled
(high-impedance). When pulled LOW or left floating, outputs are enabled.

**Typical Usage**:
- Connect to a GPIO for software control
- Leave floating if not needed (internal pull-up enables outputs)
- Use for synchronized startup of multiple devices

## Next Steps

- Verify connections with a multimeter
- Use an I2C scanner to verify device detection at expected address
- Proceed to [Quick Start](quickstart.md) to test the connection
- Review [Platform Integration](platform_integration.md) for software setup

---

**Navigation**
⬅️ [Quick Start](quickstart.md) | [Next: Platform Integration ➡️](platform_integration.md) | [Back to Index](index.md)

