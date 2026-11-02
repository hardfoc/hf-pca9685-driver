/**
 * @file test_driver.cpp
 * @brief Driver behaviour over the register model: initialisation, frequency, channel frames, errors.
 */
#include "mock_pca9685.hpp"
#include "test_harness.hpp"

#include "pca9685.hpp"

using Driver = pca9685::PCA9685<MockPca9685>;

TEST_CASE("EnsureInitialized resets MODE1 once and wakes the oscillator") {
  MockPca9685 bus;
  Driver pwm(&bus, 0x40);
  CHECK(!pwm.IsInitialized());
  REQUIRE(pwm.EnsureInitialized());
  CHECK(pwm.IsInitialized());
  CHECK_EQ(bus.reg(MockPca9685::MODE1), 0x20);   // awake, AI = 1 (Table 5)
  const auto writes = bus.log.size();
  REQUIRE(pwm.EnsureInitialized());
  CHECK_EQ(bus.log.size(), writes);               // second call: no bus traffic
  CHECK_EQ(bus.init_calls, 1);
}

TEST_CASE("EnsureInitialized fails when the bus does not come up") {
  MockPca9685 bus;
  bus.init_ok = false;
  Driver pwm(&bus, 0x40);
  CHECK(!pwm.EnsureInitialized());
  CHECK(!pwm.IsInitialized());
  CHECK(pwm.HasAnyError());
}

TEST_CASE("A device that does not acknowledge sets I2cWrite after every retry") {
  MockPca9685 bus(0x41);                          // chip strapped to 0x41, driver told 0x40
  Driver pwm(&bus, 0x40);
  pwm.SetRetries(2);
  CHECK(!pwm.EnsureInitialized());
  CHECK(pwm.HasError(Driver::Error::I2cWrite));
  CHECK_EQ(bus.log.size(), 3U);                   // 1 attempt + 2 retries
}

namespace {
int g_retry_delays = 0;
void countRetryDelay() { ++g_retry_delays; }
} // namespace

TEST_CASE("Retries recover from a transient failure and call the retry delay") {
  MockPca9685 bus;
  Driver pwm(&bus, 0x40);
  g_retry_delays = 0;
  pwm.SetRetryDelay(&countRetryDelay);
  bus.fail_next = 2;
  CHECK(pwm.EnsureInitialized());
  CHECK_EQ(g_retry_delays, 2);
}

TEST_CASE("SetPwmFreq writes PRE_SCALE while asleep and restores MODE1") {
  MockPca9685 bus;
  Driver pwm(&bus, 0x40);
  REQUIRE(pwm.SetPwmFreq(50.0f));
  CHECK_EQ(bus.reg(MockPca9685::PRE_SCALE), 121); // round(25 MHz / (4096 * 50)) - 1
  CHECK_EQ(bus.reg(MockPca9685::MODE1) & 0x10, 0); // awake again
  uint8_t prescale = 0;
  REQUIRE(pwm.GetPrescale(prescale));
  CHECK_EQ(prescale, 121);

  REQUIRE(pwm.SetPwmFreq(1526.0f));
  CHECK_EQ(bus.reg(MockPca9685::PRE_SCALE), 3);
  REQUIRE(pwm.SetPwmFreq(24.0f));
  CHECK_EQ(bus.reg(MockPca9685::PRE_SCALE), 253);
}

TEST_CASE("SetPwmFreq rejects frequencies outside 24 to 1526 Hz") {
  MockPca9685 bus;
  Driver pwm(&bus, 0x40);
  CHECK(!pwm.SetPwmFreq(10.0f));
  CHECK(pwm.HasError(Driver::Error::OutOfRange));
  pwm.ClearErrorFlags();
  CHECK(!pwm.SetPwmFreq(2000.0f));
  CHECK(pwm.HasError(Driver::Error::OutOfRange));
}

TEST_CASE("SetPwm sends one 4-byte frame to LEDn_ON_L") {
  MockPca9685 bus;
  Driver pwm(&bus, 0x40);
  REQUIRE(pwm.SetPwm(5, 0x123, 0xABC));
  const auto& t = bus.log.back();
  CHECK(t.write);
  CHECK_EQ(t.reg, MockPca9685::LED0_ON_L + 4 * 5);
  REQUIRE(t.data.size() == 4U);
  CHECK_EQ(t.data[0], 0x23);
  CHECK_EQ(t.data[1], 0x01);
  CHECK_EQ(t.data[2], 0xBC);
  CHECK_EQ(t.data[3], 0x0A);
}

TEST_CASE("SetPwm rejects channel 16 and ticks above 4095 without bus traffic") {
  MockPca9685 bus;
  Driver pwm(&bus, 0x40);
  REQUIRE(pwm.EnsureInitialized());
  const auto before = bus.log.size();
  CHECK(!pwm.SetPwm(16, 0, 100));
  CHECK(!pwm.SetPwm(0, 0, 4096));
  CHECK(pwm.HasError(Driver::Error::OutOfRange));
  CHECK_EQ(bus.log.size(), before);
}

TEST_CASE("SetDuty maps 0..1 to OFF ticks 0..4095 and clamps") {
  MockPca9685 bus;
  Driver pwm(&bus, 0x40);
  REQUIRE(pwm.SetDuty(0, 0.5f));
  auto off = [&] { const auto& d = bus.log.back().data; return d[2] | (d[3] << 8); };
  CHECK_EQ(off(), 2048);                          // lround(0.5 * 4095)
  REQUIRE(pwm.SetDuty(0, 2.0f));
  CHECK_EQ(off(), 4095);
  REQUIRE(pwm.SetDuty(0, -1.0f));
  CHECK_EQ(off(), 0);
}

TEST_CASE("Full on and full off set bit 4 of LEDn_ON_H or LEDn_OFF_H") {
  MockPca9685 bus;
  Driver pwm(&bus, 0x40);
  REQUIRE(pwm.SetChannelFullOn(3));
  CHECK_EQ(bus.log.back().data[1], 0x10);
  CHECK_EQ(bus.log.back().data[3], 0x00);
  REQUIRE(pwm.SetChannelFullOff(3));
  CHECK_EQ(bus.log.back().data[1], 0x00);
  CHECK_EQ(bus.log.back().data[3], 0x10);
}

TEST_CASE("SetAllPwm writes the ALL_LED registers") {
  MockPca9685 bus;
  Driver pwm(&bus, 0x40);
  REQUIRE(pwm.SetAllPwm(0, 1000));
  CHECK_EQ(bus.log.back().reg, MockPca9685::ALL_LED_ON_L);
}

TEST_CASE("Sleep and Wake toggle MODE1.SLEEP") {
  MockPca9685 bus;
  Driver pwm(&bus, 0x40);
  REQUIRE(pwm.Sleep());
  CHECK_EQ(bus.reg(MockPca9685::MODE1) & 0x10, 0x10);
  REQUIRE(pwm.Wake());
  CHECK_EQ(bus.reg(MockPca9685::MODE1) & 0x10, 0x00);
}

TEST_CASE("Output configuration changes only its MODE2 bit") {
  MockPca9685 bus;
  Driver pwm(&bus, 0x40);
  REQUIRE(pwm.SetOutputDriverMode(false));
  CHECK_EQ(bus.reg(MockPca9685::MODE2), 0x00);
  REQUIRE(pwm.SetOutputInvert(true));
  CHECK_EQ(bus.reg(MockPca9685::MODE2), 0x10);
  REQUIRE(pwm.SetOutputDriverMode(true));
  CHECK_EQ(bus.reg(MockPca9685::MODE2), 0x14);
}

// ---- MODE1.AI: multi-byte channel writes need register auto-increment (datasheet 7.3.1, Table 5) ----

namespace {
uint16_t onTicks(const MockPca9685& bus, int ch) {
  const auto base = static_cast<uint8_t>(MockPca9685::LED0_ON_L + 4 * ch);
  return static_cast<uint16_t>(bus.reg(base) | (bus.reg(base + 1) << 8));
}
uint16_t offTicks(const MockPca9685& bus, int ch) {
  const auto base = static_cast<uint8_t>(MockPca9685::LED0_ON_L + 4 * ch + 2);
  return static_cast<uint16_t>(bus.reg(base) | (bus.reg(base + 1) << 8));
}
} // namespace

TEST_CASE("SetPwm lands in all four LEDn registers (MODE1.AI set)") {
  MockPca9685 bus;
  Driver pwm(&bus, 0x40);
  REQUIRE(pwm.SetPwm(5, 0x123, 0xABC));
  CHECK_EQ(onTicks(bus, 5), 0x123);
  CHECK_EQ(offTicks(bus, 5), 0xABC);
  CHECK_EQ(onTicks(bus, 6), 0x000);               // neighbour untouched
  CHECK_EQ(offTicks(bus, 6), 0x1000);
}

TEST_CASE("SetAllPwm and the full on / full off calls land in the channel registers") {
  MockPca9685 bus;
  Driver pwm(&bus, 0x40);
  REQUIRE(pwm.SetAllPwm(0x100, 0x900));
  for (int ch = 0; ch < 16; ++ch) {
    CHECK_EQ(onTicks(bus, ch), 0x100);
    CHECK_EQ(offTicks(bus, ch), 0x900);
  }
  REQUIRE(pwm.SetChannelFullOn(2));
  CHECK_EQ(onTicks(bus, 2), 0x1000);
  CHECK_EQ(offTicks(bus, 2), 0x0000);
  REQUIRE(pwm.SetChannelFullOff(3));
  CHECK_EQ(onTicks(bus, 3), 0x0000);
  CHECK_EQ(offTicks(bus, 3), 0x1000);
}

TEST_CASE("MODE1.AI stays set through SetPwmFreq, Sleep, Wake and Reset") {
  MockPca9685 bus;
  Driver pwm(&bus, 0x40);
  REQUIRE(pwm.SetPwmFreq(50.0f));
  CHECK_EQ(bus.reg(MockPca9685::MODE1) & 0x20, 0x20);
  REQUIRE(pwm.Sleep());
  CHECK_EQ(bus.reg(MockPca9685::MODE1) & 0x20, 0x20);
  REQUIRE(pwm.Wake());
  CHECK_EQ(bus.reg(MockPca9685::MODE1) & 0x20, 0x20);
  bus.setReg(MockPca9685::MODE1, 0x00);           // someone cleared AI behind the driver's back
  REQUIRE(pwm.Reset());
  CHECK_EQ(bus.reg(MockPca9685::MODE1) & 0x20, 0x20);
  REQUIRE(pwm.SetPwm(1, 10, 20));
  CHECK_EQ(offTicks(bus, 1), 20);
}
