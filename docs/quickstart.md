# Quickstart

Centre a hobby servo from your MCU: wire four signals, write a small port class, call three driver functions.

!!! warning "Before you power the servo"
    Power servos from their own 5 V supply, not from the PCA9685's VDD or your MCU's regulator, and join the
    grounds. A stalled servo draws far more than a logic rail can give.

## 1. Wire it

| MCU | PCA9685 | Notes |
| --- | --- | --- |
| I2C SCL | SCL (pin 26, TSSOP28) | Pull-up to VDD; 4.7 kΩ suits 100 kHz and 400 kHz |
| I2C SDA | SDA (pin 27) | Pull-up to VDD |
| 3.3 V or 5 V | VDD (pin 28) | 2.3 V to 5.5 V; 100 nF to VSS close to the pin |
| GND | VSS (pin 14) | Common with the servo supply ground |
| GND or a GPIO | OE (pin 23) | Active low: tie it low, or drive it to blank all outputs |
| GND | A0 to A5, EXTCLK | All low: address `0x40`; EXTCLK must be grounded when unused |
| | LED0 (pin 6) | Servo signal wire |

The [Hardware](hardware.md) page has the full pin list, the address table and the electrical limits. Breakout
boards usually fit the pull-ups and tie OE and the address pins already: check yours.

## 2. Add the driver

Add the repository as a submodule and link the `hf_pca9685` CMake target. [Installation](installation.md)
covers CMake and ESP-IDF.

```bash
git submodule add https://github.com/hardfoc/hf-pca9685-driver.git external/hf-pca9685-driver
```

## 3. Write the port

The port is the only platform code. It brings the bus up and moves register bytes. Replace the three
`platform_*` calls with your SDK's.

```cpp title="my_i2c.hpp"
--8<-- "tests/docs/my_i2c.hpp:port"
```

[Porting](porting.md) explains each method and the transfer rules.

## 4. Centre the servo

```cpp title="main.cpp"
--8<-- "tests/docs/quickstart.cpp:quickstart"
```

What each step does:

1. `EnsureInitialized()` calls your port's `EnsureInitialized()`, then writes MODE1 = `0x20`, which starts the
   oscillator and turns on register auto-increment. Every other call runs it for you if you skip it.
2. `SetPwmFreq(50.0f)` writes PRE_SCALE = 121 (the chip only accepts it while asleep; the driver handles that),
   giving a 20 ms frame.
3. `SetDuty(0, 0.075f)` sets channel 0 high for 7.5 % of each frame: 1.5 ms, the centre of a typical servo.

This example is built by the host CI, so it compiles against the current headers.

## If it does not work

| Symptom | First thing to check |
| --- | --- |
| `EnsureInitialized()` returns `false` | Pull-ups on SCL and SDA, VDD, the address (A0 to A5) |
| Calls succeed, output stays low | OE must be low; the chip is not asleep (`Wake()`) |
| Output wrong on a channel | Your port's transfers ([Troubleshooting](troubleshooting.md#channel-outputs-are-wrong-or-do-not-change)) |
| Servo jitters or sits off centre | Frequency and pulse width ([PWM timing](pwm-timing.md#servo-pulses)) |

## Next

- [API guide](api-guide.md): frequency, channels, sleep, output stage, errors.
- [Examples](examples.md): an ESP32-S3 test suite and a 16-servo demo.
- [PWM timing](pwm-timing.md): how the 12-bit counter, ON and OFF ticks and the prescaler fit together.
