/**
 * @file quickstart.cpp
 * @brief The Quickstart page's example, compiled by the host build (never run).
 */
#include "my_i2c.hpp"

// --8<-- [start:quickstart]
#include "pca9685.hpp"

int center_servo_on_channel_0(MyI2c& bus) {
  pca9685::PCA9685<MyI2c> pwm(&bus, 0x40);        // A0..A5 tied low

  if (!pwm.EnsureInitialized()) {                 // 1. bus up, MODE1 reset
    return -1;                                    // wiring, pull-ups or address
  }
  if (!pwm.SetPwmFreq(50.0f)) {                   // 2. 50 Hz: a 20 ms servo frame
    return -2;
  }
  // 3. 1.5 ms high out of 20 ms = 7.5 % duty: servo centre
  if (!pwm.SetDuty(0, 1.5f / 20.0f)) {
    return -3;
  }
  return 0;
}
// --8<-- [end:quickstart]
