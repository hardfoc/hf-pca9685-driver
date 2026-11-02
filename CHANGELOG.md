# Changelog

All notable changes to this driver are listed here. Versions follow
[Semantic Versioning](https://semver.org/). Entries marked **BREAKING** need source changes in
code that used the affected API; together they call for a major version bump at release.

## Unreleased

### Documentation

- The documentation site moved to the HardFOC MkDocs platform: new page set (quickstart, hardware, PWM timing,
  architecture, API guide, configuration, porting, examples, troubleshooting), the Doxygen API reference under
  `/api/`, and every code sample compiled by the host build from `tests/docs/`.
- Corrected the hardware facts: OE has no internal pull-up, the standby current is 2.2 µA typical (15.5 µA
  maximum), and `0x70` and `0x78` to `0x7F` are not usable device addresses.

### Fixed

- `Reset()` / `EnsureInitialized()` now write MODE1 = `0x20` (AI = 1) instead of `0x00`, and every MODE1 write
  of the driver (`SetPwmFreq()`, `Sleep()`, `Wake()`) keeps AI set and never writes 1 to RESTART by accident.
  Without AI the register pointer does not advance, so the four-byte writes of `SetPwm()`, `SetDuty()`,
  `SetAllPwm()`, `SetChannelFullOn()` and `SetChannelFullOff()` only reached `LEDn_ON_L` (datasheet 7.3.1,
  Table 5). The auto-increment workaround is no longer needed.

### Added

- Host unit tests (`HF_PCA9685_BUILD_TESTS`) that run the driver against a model of the PCA9685 registers.
