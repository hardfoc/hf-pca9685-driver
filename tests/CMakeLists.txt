# =============================================================================
# hf-pca9685-driver: host unit tests
# =============================================================================
# Builds the driver header against a model of the PCA9685's I2C registers and
# runs the tests with CTest. No hardware and no ESP-IDF needed:
#
#   cmake -S . -B build-host -DHF_PCA9685_BUILD_TESTS=ON
#   cmake --build build-host
#   ctest --test-dir build-host --output-on-failure
# =============================================================================

add_executable(hf_pca9685_host_tests
  test_main.cpp
  test_driver.cpp
)

target_link_libraries(hf_pca9685_host_tests PRIVATE ${HF_PCA9685_TARGET_NAME})
target_include_directories(hf_pca9685_host_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(hf_pca9685_host_tests PRIVATE cxx_std_20)
target_compile_options(hf_pca9685_host_tests PRIVATE -Wall -Wextra)

add_test(NAME hf_pca9685_host_tests COMMAND hf_pca9685_host_tests)

# The code shown on the documentation site (docs/ includes these files with pymdownx.snippets), built so a
# page cannot show code that does not compile against the current headers. Compiled, never linked or run.
add_library(hf_pca9685_doc_snippets OBJECT
  docs/quickstart.cpp
  docs/api_tasks.cpp
  docs/ports.cpp
)
target_link_libraries(hf_pca9685_doc_snippets PRIVATE ${HF_PCA9685_TARGET_NAME})
target_include_directories(hf_pca9685_doc_snippets PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/docs)
target_compile_features(hf_pca9685_doc_snippets PRIVATE cxx_std_20)
target_compile_options(hf_pca9685_doc_snippets PRIVATE -Wall -Wextra)
