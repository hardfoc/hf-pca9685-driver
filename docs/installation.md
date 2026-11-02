# Installation

Add the driver to a CMake or ESP-IDF project. It is a header-only C++20 library with no dependencies.

## Requirements

- A C++20 compiler: GCC 10+, Clang 12+ or a current MSVC.
- CMake 3.16 or later: it generates the version header the driver includes (`pca9685_version.h`).
- No RTOS, heap or exceptions: every call is `noexcept` and returns `bool`.

## Get the source

A git submodule keeps the driver pinned to a version you chose:

```bash
git submodule add https://github.com/hardfoc/hf-pca9685-driver.git external/hf-pca9685-driver
git submodule update --init external/hf-pca9685-driver
```

Only `inc/`, `src/`, `cmake/` and `CMakeLists.txt` are needed to build. The examples have their own submodule
(`examples/esp32/scripts`); you do not need it to use the driver.

## CMake

```cmake
add_subdirectory(external/hf-pca9685-driver)
target_link_libraries(my_app PRIVATE hf::pca9685)
```

`hf::pca9685` is an alias of the `hf_pca9685` INTERFACE target. It carries the include directories (the headers
and the generated version header) and requires C++20 (`cxx_std_20`), so your target does not need to set either.

| Option | Default | Effect |
| --- | --- | --- |
| `HF_PCA9685_ENABLE_WARNINGS` | `OFF` | `-Wall -Wextra -Wpedantic` on every target that links the driver |
| `HF_PCA9685_BUILD_TESTS` | `OFF` | Builds the host unit tests in `tests/` ([Testing and CI](testing.md)) |

Use the driver from its source tree as above. The `install()` rules do not yet produce a usable package: they
leave out `src/pca9685.ipp`, which the header includes, and export the target as `hf::hf_pca9685`.

## ESP-IDF

The ESP-IDF component wrapper is
[`examples/esp32/components/hf_pca9685/`](../examples/esp32/components/hf_pca9685/CMakeLists.txt). Copy that
folder into your project's `components/` (or point `EXTRA_COMPONENT_DIRS` at it) and set the `HF_PCA9685_ROOT`
cache variable to the driver checkout; the wrapper stops with a clear message when the path is wrong. Your
component then declares `REQUIRES hf_pca9685`.

## How the build is organised

| Layer | File | Role |
| --- | --- | --- |
| Settings | [`cmake/hf_pca9685_build_settings.cmake`](../cmake/hf_pca9685_build_settings.cmake) | Target name, include directories, version: the single source of truth |
| Library | [`CMakeLists.txt`](../CMakeLists.txt) | INTERFACE target, install and package export |
| ESP-IDF | [`examples/esp32/components/hf_pca9685/CMakeLists.txt`](../examples/esp32/components/hf_pca9685/CMakeLists.txt) | IDF component wrapper over the same settings |

The version is set once in the settings file and written into `pca9685_version.h` in the build tree at
configure time. Read it at run time:

```cpp
const char* v = pca9685::PCA9685<MyI2c>::GetDriverVersion();   // "1.0.0"
```

## Next

[Quickstart](quickstart.md) wires the chip and centres a servo; [Porting](porting.md) writes the I2C interface
for your MCU.
