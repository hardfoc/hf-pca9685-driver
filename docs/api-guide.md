# API guide

How to do the common jobs with the driver: initialise, set the frequency, drive outputs, configure the output
stage, sleep and handle errors.

Every snippet on this page is compiled by the host CI from
[`tests/docs/api_tasks.cpp`](../tests/docs/api_tasks.cpp), with `Driver` standing for
`pca9685::PCA9685<MyI2c>`. The [API reference](api-reference.md) lists every member.

## The shape of the API

One `PCA9685<I2cType>` object per chip, constructed with a pointer to your port and the chip's 7-bit address.
Every call that touches the chip is `noexcept` and returns `bool`.

| Task | Calls |
| --- | --- |
| Start up | `EnsureInitialized()`, `Reset()`, `IsInitialized()` |
| Frequency | `SetPwmFreq(float hz)`, `GetPrescale(uint8_t&)` |
| One channel | `SetDuty(ch, duty)`, `SetPwm(ch, on, off)`, `SetChannelFullOn(ch)`, `SetChannelFullOff(ch)` |
| All channels | `SetAllPwm(on, off)` |
| Output stage | `SetOutputDriverMode(bool totem_pole)`, `SetOutputInvert(bool invert)` |
| Power | `Sleep()`, `Wake()` |
| Errors | `GetErrorFlags()`, `HasError(e)`, `HasAnyError()`, `ClearError(e)`, `ClearErrorFlags(mask)`, `GetLastError()` |
| Bus robustness | `SetRetries(int)`, `SetRetryDelay(void (*)())` |
| Version | `GetDriverVersion()`, `GetDriverVersionMajor()`, `GetDriverVersionMinor()`, `GetDriverVersionPatch()` |

Channels are `0` to `15` (`MAX_CHANNELS_ = 16`); ticks are `0` to `4095` (`MAX_PWM_`).

## Initialise

```cpp
--8<-- "tests/docs/api_tasks.cpp:init"
```

`EnsureInitialized()` brings your bus up and writes MODE1 = `0x20`: oscillator on, register auto-increment on
(MODE1.AI, which the four-byte channel writes need), sub-addresses and All Call off. Every other call runs it on
first use, so this step is optional, but calling it at start-up reports a wiring problem where you can act on it.
`Reset()` writes MODE1 again at any time; it does not touch the channel registers, MODE2 or PRE_SCALE.

## Set the frequency

```cpp
--8<-- "tests/docs/api_tasks.cpp:frequency"
```

One frequency serves all 16 channels. The chip only accepts a new PRE_SCALE while asleep, so `SetPwmFreq()`
briefly stops the oscillator and every output; call it once at start-up, not in a loop.
[PWM timing](pwm-timing.md#the-period) has the formula and a table of common frequencies.

## Drive the outputs

```cpp
--8<-- "tests/docs/api_tasks.cpp:outputs"
```

| Call | Use it for |
| --- | --- |
| `SetDuty(ch, duty)` | LED brightness, motor speed: `duty` from 0.0 to 1.0, clamped, rising edge at tick 0 |
| `SetPwm(ch, on, off)` | Exact pulse widths (servos) and phase-shifted channels |
| `SetChannelFullOn(ch)`, `SetChannelFullOff(ch)` | A constant high or low, with no PWM edges |
| `SetAllPwm(on, off)` | The same value on all 16 channels in one write |

For a servo, compute ticks from the pulse width:

```cpp
--8<-- "tests/docs/api_tasks.cpp:servo"
```

## Configure the output stage

```cpp
--8<-- "tests/docs/api_tasks.cpp:output-config"
```

The power-up default is totem pole, not inverted: right for servo signals and logic inputs. Use open drain for
LEDs with built-in Zener diodes, and inversion for LEDs wired from VDD to the pin. [Hardware](hardware.md#output-stage)
has the MODE2 bits.

## Sleep and wake

```cpp
--8<-- "tests/docs/api_tasks.cpp:power"
```

`Sleep()` sets MODE1.SLEEP: the oscillator stops, the outputs stop and the chip draws 2.2 µA typical. The channel
registers keep their values. `Wake()` clears SLEEP and, when the chip set RESTART on the way into sleep, writes
RESTART to resume every channel where it was. The oscillator needs up to 500 µs to start; wait that long before
you rely on output timing.

## Handle errors

```cpp
--8<-- "tests/docs/api_tasks.cpp:errors"
```

| Flag | Set when | What to do |
| --- | --- | --- |
| `Error::I2cWrite` | A write failed on every attempt, or the port's `EnsureInitialized()` failed | Check wiring, pull-ups and address; see [Troubleshooting](troubleshooting.md) |
| `Error::I2cRead` | A read failed on every attempt | As above |
| `Error::OutOfRange` | Channel above 15, tick above 4095, frequency outside 24 Hz to 1526 Hz | Fix the argument; nothing was sent |
| `Error::NotInitialized` | A call ran `EnsureInitialized()` and it failed | Set together with the I2C flag that caused it |

`InvalidParam` and `DeviceNotFound` are declared but the driver never sets them. Flags accumulate across calls:
read them, act, then clear them.

## Raw register access

The driver keeps its register helpers private. For a register it has no call for (SUBADR1 to SUBADR3, ALLCALLADR,
MODE2.OCH or OUTNE), write through your port directly, as one transaction:
`bus.Write(0x40, 0x01, &mode2, 1)`. The register addresses are in the `Register` enum. Read MODE2 first and change
only your bits: the driver's own MODE2 calls read-modify-write too, so the two stay consistent.

## Next

[Porting](porting.md) writes the port for your MCU; [Troubleshooting](troubleshooting.md) maps symptoms to causes.
