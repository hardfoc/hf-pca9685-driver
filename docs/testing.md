# Testing and CI

How the driver is checked: host unit tests against a model of the chip, compiled documentation, ESP32 builds and
static analysis, on every pull request.

## Host unit tests

The tests build the real driver against a model of the PCA9685's register interface: power-up register values,
PRE_SCALE ignoring writes while the chip is awake, the register pointer advancing only with MODE1.AI set, and
injected I2C failures. Every transaction the driver sends is logged, so tests check the exact bytes and
registers.

```bash
cmake -S . -B build-host -DHF_PCA9685_BUILD_TESTS=ON
cmake --build build-host
ctest --test-dir build-host --output-on-failure
```

| File | Covers |
| --- | --- |
| [`tests/test_driver.cpp`](../tests/test_driver.cpp) | Initialisation, retries, frequency and prescaler, channel frames, range checks, sleep, MODE2 |
| [`tests/mock_pca9685.hpp`](../tests/mock_pca9685.hpp) | The register model and transaction log |
| [`tests/test_harness.hpp`](../tests/test_harness.hpp) | A dependency-free test registry and `CHECK` macros |

## Documentation snippets

Every C++ block on this site is included from [`tests/docs/`](../tests/docs/my_i2c.hpp), which the host build
compiles (target `hf_pca9685_doc_snippets`) with the driver instantiated over the documented port. A change to the
API that breaks a page breaks the build first.

## CI workflows

| Workflow | What it checks |
| --- | --- |
| Host unit tests | GCC and Clang build and run the tests and the doc snippets |
| ESP32 examples build | Every app in `examples/esp32/app_config.yml`, for each listed ESP-IDF version and build type |
| C++ lint, C++ analysis | clang-format, clang-tidy and cppcheck |
| Docs | Strict site build, figure and generated-table drift, Doxygen, link check, Lighthouse budget |
| Markdown lint, YAML lint, link check | Repository text files |

The workflows call the shared, versioned workflows of
[hf-general-ci-tools](https://github.com/hardfoc/hf-general-ci-tools) and
[hf-espidf-ci-tools](https://github.com/hardfoc/hf-espidf-ci-tools).

## On hardware

The [ESP32 examples](examples.md) are the hardware test suite: `pca9685_comprehensive_test` prints a pass / fail
summary over the serial monitor. Check the outputs with a scope or logic analyser; the test cannot see them.
