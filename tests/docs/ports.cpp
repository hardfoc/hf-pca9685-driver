/**
 * @file ports.cpp
 * @brief Instantiates the driver over the documented port, so every member compiles.
 */
#include "my_i2c.hpp"

#include "pca9685.hpp"

template class pca9685::PCA9685<MyI2c>;

void platform_gpio_write(int pin, bool high) noexcept;  // your SDK's GPIO call

// A port with OE wired to a GPIO. The I2C methods are those of MyI2c (Porting page, "I2C port").
class MyOePort : public pca9685::I2cInterface<MyOePort> {
public:
  bool EnsureInitialized() noexcept { return i2c_.EnsureInitialized(); }
  bool Write(uint8_t a, uint8_t r, const uint8_t* d, size_t n) noexcept { return i2c_.Write(a, r, d, n); }
  bool Read(uint8_t a, uint8_t r, uint8_t* d, size_t n) noexcept { return i2c_.Read(a, r, d, n); }
  void DelayUs(uint32_t us) noexcept { i2c_.DelayUs(us); }

  // --8<-- [start:oe-port]
  // OE is active low: ACTIVE (outputs enabled) is a LOW level, INACTIVE (blanked) a HIGH level.
  void GpioSet(pca9685::CtrlPin pin, pca9685::GpioSignal signal) noexcept {
    if (pin == pca9685::CtrlPin::OE) {
      platform_gpio_write(OE_GPIO, signal == pca9685::GpioSignal::INACTIVE);
    }
  }
  // --8<-- [end:oe-port]

private:
  static constexpr int OE_GPIO = 7;               // your board's pin
  MyI2c i2c_;
};

template class pca9685::PCA9685<MyOePort>;

// GpioSetActive() / GpioSetInactive() of the base class reach the port's GpioSet().
void blank_then_enable(MyOePort& port) noexcept {
  port.GpioSetInactive(pca9685::CtrlPin::OE);
  port.GpioSetActive(pca9685::CtrlPin::OE);
}
