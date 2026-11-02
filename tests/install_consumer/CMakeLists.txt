# =============================================================================
# hf-pca9685-driver: install check consumer
# =============================================================================
# A project outside the source tree that uses only the installed package:
#   find_package(hf_pca9685) + hf::pca9685. Run by tests/check_install.cmake.
# =============================================================================
cmake_minimum_required(VERSION 3.16)
project(hf_pca9685_install_consumer LANGUAGES CXX)

find_package(hf_pca9685 1.0 REQUIRED CONFIG)

add_executable(consumer main.cpp)
target_link_libraries(consumer PRIVATE hf::pca9685)
