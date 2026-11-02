---
hide:
  - navigation
  - toc
---

<div class="hf-hero" markdown>
<span class="hf-kicker">HardFOC driver</span>

# PCA9685 Driver

<p>A C++20 driver for the NXP PCA9685 16-channel, 12-bit PWM controller. It sets the PWM frequency, drives each
channel by duty cycle or by on/off tick, and controls sleep and the output stage, on any MCU with I2C.</p>

[Quickstart](quickstart.md){ .md-button .md-button--primary }
[API guide](api-guide.md){ .md-button }
[Source on GitHub](https://github.com/hardfoc/hf-pca9685-driver){ .md-button }
</div>

The driver has no platform code. You give it a small class that brings an I2C bus up and reads and writes
register bytes; it does the rest: the prescaler calculation, the sleep sequence the PRE_SCALE register needs,
channel frames, retries and error flags.

<div class="hf-chip" markdown>
<div class="hf-chip__head">
<span class="hf-chip__kicker">16-channel, 12-bit PWM LED controller</span>
<span class="hf-chip__name">PCA9685</span>
<span class="hf-chip__vendor">NXP Semiconductors</span>
</div>

Outputs
:   16 channels, 12-bit (4096 steps) each, with a programmable on time and off time

PWM frequency
:   24 Hz to 1526 Hz, one frequency for all channels (25 MHz internal oscillator, or EXTCLK up to 50 MHz)

Host interface
:   I2C up to 1 MHz (Fast-mode Plus); 7-bit address `0x40` to `0x7F` from pins A0 to A5

Output stage
:   Totem pole (25 mA sink, 10 mA source at 5 V) or open drain; OE pin, active low

Supply
:   2.3 V to 5.5 V, inputs and outputs 5.5 V tolerant; −40 °C to +85 °C

Package
:   TSSOP28, HVQFN28

[Datasheet (Rev. 4)](datasheet/PCA9685.pdf)
</div>

## What you get

- **One object per chip.** `PCA9685<YourBus>` takes the bus and the 7-bit address; several chips share one bus.
  See [API guide](api-guide.md).
- **Frequency in hertz.** `SetPwmFreq()` computes PRE_SCALE and runs the sleep / write / wake sequence the chip
  requires. See [PWM timing](pwm-timing.md).
- **Channels by duty or by tick.** `SetDuty()`, `SetPwm()` with a phase offset, full on, full off and all
  channels at once. See [API guide](api-guide.md#drive-the-outputs).
- **Errors you can inspect.** Every call returns `bool`; a bitmask of `Error` flags says why. Register accesses
  retry with an optional delay. See [API guide](api-guide.md#handle-errors).
- **Zero-overhead portability.** The port is a CRTP class with three methods: no virtual calls, no heap, no RTOS
  dependency. See [Porting](porting.md).
- **Tested without hardware.** Host unit tests run the driver against a model of the chip's registers. See
  [Testing and CI](testing.md).

## Where to go next

<div class="grid cards" markdown>

-   :material-rocket-launch-outline: **[Quickstart](quickstart.md)**

    ---

    Wire the chip, write the port, centre a servo in about 15 lines.

-   :material-chip: **[Hardware](hardware.md)**

    ---

    Pins, addresses, bus settings, output stage and power.

-   :material-layers-outline: **[Architecture](architecture.md)**

    ---

    The three layers and what each one owns.

-   :material-code-braces: **[API guide](api-guide.md)**

    ---

    Initialise, set the frequency, drive outputs and handle errors, task by task.

-   :material-swap-horizontal: **[Porting](porting.md)**

    ---

    Implement the I2C interface for your MCU.

-   :material-book-open-variant: **[API reference](api-reference.md)**

    ---

    Every class and member, generated from the headers.

</div>
