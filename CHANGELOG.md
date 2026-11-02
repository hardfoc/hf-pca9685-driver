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
  maximum), `0x70` and `0x78` to `0x7F` are not usable device addresses, and the driver does not enable register
  auto-increment.

### Added

- Host unit tests (`HF_PCA9685_BUILD_TESTS`) that run the driver against a model of the PCA9685 registers.
