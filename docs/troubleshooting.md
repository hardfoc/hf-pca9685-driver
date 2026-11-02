# Troubleshooting

Symptoms you may see while bringing up a PCA9685, their causes and the fix for each.

Start with the error flags of the failing call (`GetErrorFlags()`, [API guide](api-guide.md#handle-errors)):
`I2cWrite` or `I2cRead` is a bus problem, `OutOfRange` is a bad argument and nothing was sent.

## The chip does not answer

`EnsureInitialized()` returns `false` with `Error::I2cWrite`.

| Cause | Check | Fix |
| --- | --- | --- |
| No pull-ups | SCL and SDA idle low or slow edges on a scope | 4.7 kΩ to VDD on each line (many breakouts fit them) |
| Wrong address | An I2C scan of `0x40` to `0x7F` | Pass the address set by A0 to A5; address pins have no pull-ups and must not float |
| No supply | VDD at the chip: 2.3 V to 5.5 V | Power it; join the grounds |
| Bus not up | Your port's `EnsureInitialized()` returns `false` | Fix the SDK call; the driver reports it as `I2cWrite` |
| Address `0x70` | Several chips answer at once | Use another address: `0x70` is the All Call address |

## Calls succeed but the outputs stay low

| Cause | Check | Fix |
| --- | --- | --- |
| OE high or floating | OE pin level | Tie OE low or drive it low; it has no internal pull-up |
| Chip asleep | MODE1 bit 4 | `Wake()`; `Sleep()` and power-up both leave SLEEP set until the driver starts |
| Channel full off | `LEDn_OFF_H` bit 4 | `SetPwm()` with ON ≠ OFF, `SetDuty()` above 0 or `SetChannelFullOn()` clear it; `SetDuty(ch, 0.0f)` and equal ticks set it |
| Load wired to the wrong side | Totem pole versus open drain, INVRT | [Hardware](hardware.md#output-stage) |

## Channel outputs are wrong or do not change

The driver writes each channel as one four-byte transaction starting at `LEDn_ON_L` and relies on MODE1.AI
(register auto-increment) to move the register pointer through `LEDn_ON_H`, `LEDn_OFF_L` and `LEDn_OFF_H`
(Table 5, p. 14). `Reset()` sets AI and every later MODE1 write of the driver keeps it.

| Cause | Check | Fix |
| --- | --- | --- |
| Port splits or truncates the transfer | A logic analyser on the bus: one START, five data bytes, one STOP | One call, one transaction ([Porting](porting.md#i2c-port)) |
| Your own MODE1 write cleared AI | MODE1 bit 5 | Keep bit 5 set in raw MODE1 writes, or call `Reset()` again |

## The frequency is off

| Cause | Check | Fix |
| --- | --- | --- |
| Out of range | `SetPwmFreq()` returns `false`, `Error::OutOfRange` | 24 Hz to 1526 Hz only |
| Prescaler rounding | `GetPrescale()`, then f = 25 MHz / (4096 × (PRE_SCALE + 1)) | [PWM timing](pwm-timing.md#the-period) |
| Oscillator tolerance | Measure the period on a scope | Scale your pulse widths, or clock the chip from EXTCLK |

## Servos jitter or move to the wrong place

| Cause | Fix |
| --- | --- |
| Servo powered from the logic supply | A separate 5 V supply sized for the stall current of all servos; common ground |
| Pulse width beyond the servo's range | Stay within the servo datasheet's range, often 1.0 ms to 2.0 ms |
| Frequency not 50 Hz, or not measured | `SetPwmFreq(50.0f)`, then check the 20 ms frame on a scope |
| `SetPwmFreq()` called repeatedly | It stops the oscillator each time: call it once at start-up |

## Intermittent I2C errors

| Cause | Fix |
| --- | --- |
| Long wires, slow edges | Lower the clock to 100 kHz; stronger pull-ups; shorter wiring |
| Noise from motors or servos | Route I2C away from load wiring; decouple VDD with 100 nF at the chip |
| Retries without a pause | `SetRetryDelay()` with a 1 ms delay between attempts |
| Two tasks using the driver | One owner task, or one mutex around the driver and the port ([Porting](porting.md#threads)) |
