/**
 * @file main.cpp
 * @brief Builds the driver from an installed package only: headers, version header and .ipp.
 */
#include "pca9685.hpp"

#include <cstddef>
#include <cstdint>

class NullBus : public pca9685::I2cInterface<NullBus> {
public:
  bool EnsureInitialized() noexcept { return true; }
  bool Write(uint8_t, uint8_t, const uint8_t*, size_t) noexcept { return true; }
  bool Read(uint8_t, uint8_t, uint8_t* data, size_t len) noexcept {
    for (size_t i = 0; i < len; ++i) {
      data[i] = 0;
    }
    return true;
  }
};

int main() {
  NullBus bus;
  pca9685::PCA9685<NullBus> pwm(&bus, 0x40);
  const bool ok = pwm.EnsureInitialized() && pwm.SetDuty(0, 0.5f);
  return (ok && pca9685::PCA9685<NullBus>::GetDriverVersion()[0] != '\0') ? 0 : 1;
}
