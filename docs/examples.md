# Examples

Complete ESP32-S3 applications that exercise the driver on a PCA9685 board, from a full test suite to a 16-servo
animation.

They run on an ESP32-S3 with the PCA9685 at `0x40` (SDA on GPIO4, SCL on GPIO5, 100 kHz); the pins are in
[`esp32_pca9685_test_config.hpp`](../examples/esp32/main/esp32_pca9685_test_config.hpp) and the port in
[`esp32_pca9685_bus.hpp`](../examples/esp32/main/esp32_pca9685_bus.hpp). CI builds every app below on each pull
request.

## The apps

<!-- generated: examples -->
| App | What it shows | Source | CI |
| --- | --- | --- | --- |
| `pca9685_comprehensive_test` :material-star:{ title="featured" } | Comprehensive PCA9685 driver test suite covering all functionality | [`pca9685_comprehensive_test.cpp`](../examples/esp32/main/pca9685_comprehensive_test.cpp) | yes |
| `pca9685_servo_demo` :material-star:{ title="featured" } | 16-channel hobby servo demo with synchronized animations (1000-2000 µs) | [`pca9685_servo_demo.cpp`](../examples/esp32/main/pca9685_servo_demo.cpp) | yes |

Build and flash one (from `examples/esp32`, ESP-IDF environment loaded):

```bash
./scripts/build_app.sh pca9685_comprehensive_test Release
./scripts/flash_app.sh flash_monitor pca9685_comprehensive_test Release
```

Source of this table: [`examples/esp32/app_config.yml`](../examples/esp32/app_config.yml).
<!-- /generated -->

## Set up once

```bash
git submodule update --init examples/esp32/scripts   # the HardFOC ESP-IDF build scripts
cd examples/esp32
. $IDF_PATH/export.sh                                 # ESP-IDF v5.5
./scripts/build_app.sh list                           # every app and build type
```

The scripts read [`app_config.yml`](../examples/esp32/app_config.yml) (the same file this page's table comes
from) and build each app as its own firmware image.

## What to read in an example

| You want to | Read |
| --- | --- |
| See every driver call exercised, with pass / fail output | `pca9685_comprehensive_test.cpp` |
| Drive hobby servos from pulse widths in microseconds | `pca9685_servo_demo.cpp` |
| Write an ESP-IDF port | `esp32_pca9685_bus.hpp` |

The [example folder's README](../examples/esp32/README.md) has the wiring, the build options and the expected
serial output.
