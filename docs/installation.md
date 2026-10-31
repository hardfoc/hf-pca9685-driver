---
layout: default
title: "🛠️ Installation"
description: "Installation and build instructions for the PCA9685 driver"
nav_order: 1
parent: "📚 Documentation"
permalink: /docs/installation/
---

# Installation

This guide covers how to obtain, build, and verify the PCA9685 driver library.

## Prerequisites

Before installing the driver, ensure you have:

- **C++11 Compiler**: GCC 4.8+, Clang 3.3+, or MSVC 2013+
- **Build System**: Make or CMake (optional, for building static library)
- **Platform SDK**: Your platform's I2C driver (ESP-IDF, STM32 HAL, Arduino Wire, etc.)

## Obtaining the Source

### Option 1: Git Clone

```bash
git clone https://github.com/hardfoc/hf-pca9685-driver.git
cd hf-pca9685-driver
```

### Option 2: Copy Files

Copy the following files into your project:

```
inc/
  ├── pca9685.hpp
  └── pca9685_i2c_interface.hpp
src/
  └── pca9685.ipp
```

**Note**: The driver uses a template design: the implementation is in `src/pca9685.ipp` and is
included by `inc/pca9685.hpp`. Your build must have `inc/` and `src/` on the include path so that
`pca9685.hpp` can include the `.ipp` file. You do not compile `pca9685.ipp` as a separate
translation unit.

## Building the Library

### As Part of Your Project (Include Path)

Since the driver is header-only (template implementation in `.ipp`), add the repository `inc/` and
`src/` directories to your include path and include the main header:

```cpp
#include "pca9685.hpp"
```

Your build system must allow the header to find `../src/pca9685.ipp` (relative to the header) or
you can add `src/` to the include path as well.

### Using CMake

```cmake
# Include path must include both inc and src (for .ipp include from header)
target_include_directories(your_target
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/hf-pca9685-driver/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/hf-pca9685-driver/src
)
# No separate library: template is instantiated in your code
```

### Using ESP-IDF (ESP32 Examples)

The driver is provided as an ESP-IDF component in the ESP32 examples. Build and flash using the
example scripts:

```bash
cd examples/esp32
./scripts/build_app.sh list                    # List available apps
./scripts/build_app.sh pca9685_comprehensive_test Debug
./scripts/flash_app.sh flash_monitor pca9685_comprehensive_test Debug
```

See [examples/esp32/README.md](../examples/esp32/README.md) and
[examples/esp32/docs/](../examples/esp32/docs/) for full setup and app descriptions.

## Verification

To verify the installation:

1. Include the header in a test file:
   ```cpp
   #include "pca9685.hpp"
   // Provide an I2C implementation and instantiate PCA9685<YourI2c>
   ```

2. Ensure your include path contains both `inc/` and `src/` (so `pca9685.hpp` can include `pca9685.ipp`).

3. For hardware verification, use the ESP32 comprehensive test app (see [Examples](examples.md)):
   ```bash
   cd examples/esp32
   ./scripts/build_app.sh pca9685_comprehensive_test Debug
   ./scripts/flash_app.sh flash_monitor pca9685_comprehensive_test Debug
   ```
   All 12 tests should pass when the PCA9685 is connected at 0x40 on the configured I2C pins.

## Next Steps

- Follow the [Quick Start](quickstart.md) guide to create your first application
- Review [Hardware Setup](hardware_setup.md) for wiring instructions
- Check [Platform Integration](platform_integration.md) to implement the I2C interface

---

**Navigation**
⬅️ [Back to Index](index.md) | [Next: Quick Start ➡️](quickstart.md)

