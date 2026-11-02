# PWM timing

How the PCA9685 makes a PWM waveform: one 12-bit counter, an ON tick and an OFF tick per channel, and a
prescaler that sets the period. Read it before you drive servos or need phase-shifted outputs.

Values are from the [datasheet](datasheet/PCA9685.pdf), sections 7.3.3 to 7.3.5 (pp. 16-25).

## The counter

Every channel compares one shared counter, running from 0 to 4095 and wrapping, with two 12-bit values:

- **ON tick** (`LEDn_ON`): the counter value at which the output goes high.
- **OFF tick** (`LEDn_OFF`): the counter value at which it goes low.

Duty cycle is `(OFF − ON) / 4096` when OFF is after ON, and `(4096 − ON + OFF) / 4096` when it wraps. ON and OFF
must never be equal (section 7.3.4, p. 25).

| Call | ON tick | OFF tick | Output |
| --- | --- | --- | --- |
| `SetPwm(ch, 0, 1024)` | 0 | 1024 | High for ticks 0 to 1023: 25 % |
| `SetPwm(ch, 1024, 3072)` | 1024 | 3072 | 50 %, shifted by a quarter period |
| `SetPwm(ch, 3072, 1024)` | 3072 | 1024 | 50 %, high across the wrap |
| `SetDuty(ch, d)` | 0 | `lround(d × 4095)` | Starts every period at tick 0 |
| `SetChannelFullOn(ch)` | bit 12 set | 0 | Constantly high |
| `SetChannelFullOff(ch)` | 0 | bit 12 set | Constantly low; overrides full on |

Staggering the ON ticks of channels spreads their switching edges over the period and lowers the current surge on
the load supply; `SetDuty()` always starts at tick 0, so use `SetPwm()` for that.

`SetDuty(ch, 1.0f)` gives OFF = 4095: high for 4095 of 4096 ticks, not constantly high. `SetDuty(ch, 0.0f)` writes
ON = OFF = 0, which the datasheet says not to program. Use `SetChannelFullOn()` and `SetChannelFullOff()` for the
two ends.

## The period

The counter is clocked from the 25 MHz internal oscillator (or EXTCLK) divided by `PRE_SCALE + 1`
(equation 1, p. 25):

```text
PRE_SCALE = round(f_osc / (4096 × f_pwm)) − 1
f_pwm     = f_osc / (4096 × (PRE_SCALE + 1))
```

The chip forces PRE_SCALE to at least 3, which gives the 1526 Hz maximum; 255 gives 23.8 Hz. The driver accepts
24 Hz to 1526 Hz and returns `false` with `Error::OutOfRange` outside it. PRE_SCALE can only be written while
MODE1.SLEEP = 1, so `SetPwmFreq()` puts the chip to sleep, writes it and restores MODE1.

| Requested | PRE_SCALE | Actual (internal 25 MHz) | Tick |
| --- | --- | --- | --- |
| 50 Hz | 121 | 50.0 Hz | 4.88 µs |
| 60 Hz | 101 | 59.8 Hz | 4.08 µs |
| 200 Hz (power-up default) | 30 | 196.9 Hz | 1.24 µs |
| 1000 Hz | 5 | 1017 Hz | 0.24 µs |
| 1526 Hz | 3 | 1526 Hz | 0.16 µs |

The internal oscillator is specified as 25 MHz typical, with no tolerance in the datasheet. When the period
matters (servos), measure it once with a scope or logic analyser and scale the pulse widths, or feed EXTCLK from
a crystal oscillator.

## Servo pulses

A hobby servo reads the high time of a pulse repeated every 20 ms (50 Hz). Typical servos centre at 1.5 ms and
travel over about 1.0 ms to 2.0 ms; check your servo's datasheet before you go wider. At 50 Hz one tick is
20 ms / 4096 = 4.88 µs, so 1.0 ms to 2.0 ms is about 205 ticks of resolution.

```cpp
--8<-- "tests/docs/api_tasks.cpp:servo"
```

| Pulse | Ticks at 50 Hz | Duty |
| --- | --- | --- |
| 1.0 ms | 205 | 5.0 % |
| 1.5 ms | 307 | 7.5 % |
| 2.0 ms | 410 | 10.0 % |

## When a change takes effect

A new ON or OFF value is applied at the end of the current low period, so a changing duty cycle produces no
glitch (section 7.3.4). With MODE2.OCH = 0 (the default and what the driver leaves), the outputs change at the
I2C STOP, so all four bytes of a channel take effect together.

## Next

[API guide](api-guide.md) shows these calls in context; [Hardware](hardware.md#timing-that-affects-the-host)
lists the oscillator start-up time.
