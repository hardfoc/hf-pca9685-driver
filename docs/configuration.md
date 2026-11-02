# Configuration

Every setting the driver exposes, its default and where it lives: the constructor, run-time calls and the chip
state the driver leaves behind.

## Constructor

```cpp
pca9685::PCA9685<MyI2c> pwm(&bus, 0x40);
```

| Argument | Type | Meaning |
| --- | --- | --- |
| `bus` | `MyI2c*` | Your port; must outlive the driver. `nullptr` makes every call fail |
| `address` | `uint8_t` | 7-bit address, `0x40` to `0x7F` from A0 to A5 ([Hardware](hardware.md#address)) |

Several chips on one bus: one port object, one driver per address.

## Run-time settings

| Setting | Call | Default | Range |
| --- | --- | --- | --- |
| Retries per register access | `SetRetries(int)` | 3 (four attempts) | 0 or more |
| Delay between attempts | `SetRetryDelay(void (*)())` | none | Any function with no arguments; a capture-less lambda works |
| PWM frequency | `SetPwmFreq(float)` | 200 Hz (chip default PRE_SCALE = 30) | 24 Hz to 1526 Hz |
| Output driver | `SetOutputDriverMode(bool)` | totem pole | totem pole or open drain |
| Output inversion | `SetOutputInvert(bool)` | not inverted | |

The retry settings live in the driver object; the others are chip registers and survive until a power cycle or a
software reset.

## Chip state after start-up

| Register | Power-up value | After `EnsureInitialized()` | Changed by |
| --- | --- | --- | --- |
| MODE1 | `0x11`: SLEEP, ALLCALL | `0x00`: awake, AI off, ALLCALL off | `Reset()`, `SetPwmFreq()`, `Sleep()`, `Wake()` |
| MODE2 | `0x04`: totem pole, change on STOP, OUTNE = 00 | unchanged | `SetOutputDriverMode()`, `SetOutputInvert()` |
| PRE_SCALE | `0x1E` (30): about 200 Hz | unchanged | `SetPwmFreq()` |
| LEDn_ON / LEDn_OFF | full off (`LEDn_OFF_H` bit 4 = 1) | unchanged | channel calls |

Power-up values: Tables 4 to 8 of the [datasheet](datasheet/PCA9685.pdf). Because `Reset()` clears ALLCALL, the
chip stops answering the All Call address `0x70` after the driver starts. MODE1.AI is also left at 0; see
[Troubleshooting](troubleshooting.md#channel-outputs-are-wrong-or-do-not-change).

## Build options

| Option or define | Default | Effect |
| --- | --- | --- |
| `HF_PCA9685_ENABLE_WARNINGS` (CMake) | `OFF` | `-Wall -Wextra -Wpedantic` on targets that link the driver |
| `HF_PCA9685_BUILD_TESTS` (CMake) | `OFF` | Host unit tests and documentation snippets |
| `HF_PCA9685_VERSION_*` (CMake) | `1.0.0` | Set in `cmake/hf_pca9685_build_settings.cmake`, written to `pca9685_version.h` |

## Next

[API guide](api-guide.md) uses these settings in context; [Installation](installation.md) shows the CMake setup.
