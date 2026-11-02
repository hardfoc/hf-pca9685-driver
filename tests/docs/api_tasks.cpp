/**
 * @file api_tasks.cpp
 * @brief The API guide's task snippets, compiled by the host build (never run).
 */
#include "my_i2c.hpp"

#include "pca9685.hpp"

#include <cstdint>
#include <cstdio>

using Driver = pca9685::PCA9685<MyI2c>;

void platform_delay_ms(uint32_t ms) noexcept;  // your SDK's delay
void platform_delay_us(uint32_t us) noexcept;

bool init(MyI2c& bus) {
  // --8<-- [start:init]
  Driver pwm(&bus, 0x40);

  pwm.SetRetries(2);                              // up to 3 attempts per register access
  pwm.SetRetryDelay([] { platform_delay_ms(1); }); // between attempts: let the bus recover

  if (!pwm.EnsureInitialized()) {                 // bus up, then MODE1 = 0x20 (oscillator on, AI)
    return false;
  }
  // --8<-- [end:init]
  return true;
}

bool frequency(Driver& pwm) {
  // --8<-- [start:frequency]
  if (!pwm.SetPwmFreq(1000.0f)) {                 // 24 .. 1526 Hz, same for all 16 channels
    return false;                                 // Error::OutOfRange outside that range
  }
  uint8_t prescale = 0;
  if (!pwm.GetPrescale(prescale)) {               // 1000 Hz -> PRE_SCALE = 5
    return false;
  }
  const float actual_hz = 25'000'000.0f / (4096.0f * (prescale + 1));  // 1017 Hz
  std::printf("PWM %.0f Hz (prescale %u)\n", static_cast<double>(actual_hz), prescale);
  // --8<-- [end:frequency]
  return true;
}

bool outputs(Driver& pwm) {
  // --8<-- [start:outputs]
  if (!pwm.SetDuty(0, 0.25f)) {                   // channel 0: high for the first 25 % of the period
    return false;
  }
  if (!pwm.SetPwm(1, 1024, 3072)) {               // channel 1: 50 %, rising at tick 1024 (phase shift)
    return false;
  }
  if (!pwm.SetChannelFullOn(2) || !pwm.SetChannelFullOff(3)) {  // 2 constantly high, 3 constantly low
    return false;
  }
  return pwm.SetAllPwm(0, 2048);                  // all 16 channels: 50 %, overrides the above
  // --8<-- [end:outputs]
}

// --8<-- [start:servo]
// Servo pulse width to PWM ticks: one period is 4096 ticks of 1 / f_pwm.
bool set_servo_us(Driver& pwm, uint8_t channel, float pulse_us, float pwm_hz) {
  const float period_us = 1'000'000.0f / pwm_hz;  // 20 000 us at 50 Hz
  const auto ticks = static_cast<uint16_t>(pulse_us / period_us * 4096.0f + 0.5f);
  return pwm.SetPwm(channel, 0, ticks);           // 1500 us at 50 Hz -> 307 ticks
}
// --8<-- [end:servo]

bool configure_outputs(Driver& pwm) {
  // --8<-- [start:output-config]
  if (!pwm.SetOutputDriverMode(false)) {          // MODE2.OUTDRV = 0: open-drain (sink only)
    return false;
  }
  return pwm.SetOutputInvert(true);               // MODE2.INVRT = 1: for LEDs wired to VDD
  // --8<-- [end:output-config]
}

bool power(Driver& pwm) {
  // --8<-- [start:power]
  if (!pwm.Sleep()) {                             // MODE1.SLEEP = 1: oscillator off, outputs stop
    return false;
  }
  // ... low-power period ...
  if (!pwm.Wake()) {                              // MODE1.SLEEP = 0, then RESTART if it was set
    return false;
  }
  platform_delay_us(500);                         // oscillator start-up, 500 us max (datasheet)
  // --8<-- [end:power]
  return true;
}

void errors(Driver& pwm) {
  // --8<-- [start:errors]
  if (!pwm.SetPwm(16, 0, 100)) {                  // channel 16 does not exist
    if (pwm.HasError(Driver::Error::OutOfRange)) {
      std::printf("bad argument\n");
    }
    if (pwm.HasError(Driver::Error::I2cWrite) || pwm.HasError(Driver::Error::I2cRead)) {
      std::printf("bus failure after all retries\n");
    }
    pwm.ClearErrorFlags();                        // flags accumulate until you clear them
  }
  // --8<-- [end:errors]
}
