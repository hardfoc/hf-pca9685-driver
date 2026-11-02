#===============================================================================
# PCA9685 Driver - Root CMake
# Defines the main build target for the driver. Minimal, platform-agnostic.
#===============================================================================

cmake_minimum_required(VERSION 3.16)

project(hf_pca9685_driver LANGUAGES CXX)

#===============================================================================
# Load shared build settings (target name, includes, sources, dependencies)
#===============================================================================
include(${CMAKE_CURRENT_LIST_DIR}/cmake/hf_pca9685_build_settings.cmake)
include(GNUInstallDirs)

# Installed layout keeps the source tree's inc/ + src/ pair, because pca9685.hpp includes
# "../src/pca9685.ipp" relative to itself:
#   <prefix>/include/hf_pca9685/inc/*.hpp, pca9685_version.h
#   <prefix>/include/hf_pca9685/src/pca9685.ipp
set(HF_PCA9685_INSTALL_INCLUDEDIR "${CMAKE_INSTALL_INCLUDEDIR}/hf_pca9685/inc")

#===============================================================================
# Target type selection
# INTERFACE for header-only, STATIC if sources present
#===============================================================================
# This pattern keeps the target name stable for consumers while allowing
# implementation mode changes without API churn.
if(HF_PCA9685_SOURCE_FILES)
    add_library(${HF_PCA9685_TARGET_NAME} STATIC ${HF_PCA9685_SOURCE_FILES})
    target_include_directories(${HF_PCA9685_TARGET_NAME}
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/inc>
            $<BUILD_INTERFACE:${HF_PCA9685_VERSION_HEADER_DIR}>
            $<INSTALL_INTERFACE:${HF_PCA9685_INSTALL_INCLUDEDIR}>
    )
    target_compile_features(${HF_PCA9685_TARGET_NAME} PUBLIC cxx_std_20)
else()
    add_library(${HF_PCA9685_TARGET_NAME} INTERFACE)
    target_include_directories(${HF_PCA9685_TARGET_NAME}
        INTERFACE
            $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/inc>
            $<BUILD_INTERFACE:${HF_PCA9685_VERSION_HEADER_DIR}>
            $<INSTALL_INTERFACE:${HF_PCA9685_INSTALL_INCLUDEDIR}>
    )
    target_compile_features(${HF_PCA9685_TARGET_NAME} INTERFACE cxx_std_20)
endif()

# =============================================================================
# Namespaced Consumer Alias
# =============================================================================
# Recommended consumption style in external projects:
#   target_link_libraries(my_app PRIVATE hf::pca9685)
#
# This alias remains stable even if the underlying concrete target changes.
add_library(hf::pca9685 ALIAS ${HF_PCA9685_TARGET_NAME})
# The installed package exports the same name: hf::pca9685 (not hf::hf_pca9685).
set_target_properties(${HF_PCA9685_TARGET_NAME} PROPERTIES EXPORT_NAME pca9685)

#===============================================================================
# Versioning
# Version variables and header generation are defined in:
#   cmake/hf_pca9685_build_settings.cmake (single source of truth)
#===============================================================================
set_target_properties(${HF_PCA9685_TARGET_NAME} PROPERTIES VERSION ${HF_PCA9685_VERSION} SOVERSION ${HF_PCA9685_VERSION_MAJOR})

#===============================================================================
# Optional: Enable extra warnings (can be toggled by user)
#===============================================================================
# Default OFF: INTERFACE warnings propagate to every consumer that links this
# library, which can generate noise in downstream builds.  Enable explicitly
# with -D HF_PCA9685_ENABLE_WARNINGS=ON when developing the driver itself.
# Example projects already set their own warning flags in main/CMakeLists.txt.
option(HF_PCA9685_ENABLE_WARNINGS "Enable extra warnings for PCA9685 driver" OFF)
if(HF_PCA9685_ENABLE_WARNINGS)
    if(TARGET ${HF_PCA9685_TARGET_NAME})
        target_compile_options(${HF_PCA9685_TARGET_NAME} INTERFACE -Wall -Wextra -Wpedantic)
    endif()
endif()

#===============================================================================
# Optional: host unit tests and the compiled documentation snippets (tests/)
#===============================================================================
option(HF_PCA9685_BUILD_TESTS "Build the host unit tests in tests/" OFF)
if(HF_PCA9685_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

#===============================================================================
# Install and export support (for find_package usage)
#===============================================================================
include(CMakePackageConfigHelpers)

install(TARGETS ${HF_PCA9685_TARGET_NAME} EXPORT hf_pca9685Targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${HF_PCA9685_INSTALL_INCLUDEDIR}
)

# Hand-written headers, the generated version header and the template implementation the
# header includes (src/pca9685.ipp), in the layout described at the top of this file.
install(DIRECTORY inc/ DESTINATION ${HF_PCA9685_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h")
install(FILES "${HF_PCA9685_VERSION_HEADER}" DESTINATION ${HF_PCA9685_INSTALL_INCLUDEDIR})
install(DIRECTORY src/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/hf_pca9685/src
    FILES_MATCHING PATTERN "*.ipp")

install(EXPORT hf_pca9685Targets
    FILE hf_pca9685Targets.cmake
    NAMESPACE hf::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/hf_pca9685
)

# Generate and install package config so find_package(hf_pca9685) works
configure_package_config_file(
    "${CMAKE_CURRENT_LIST_DIR}/cmake/hf_pca9685Config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/hf_pca9685Config.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/hf_pca9685
)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/hf_pca9685ConfigVersion.cmake"
    VERSION ${HF_PCA9685_VERSION}
    COMPATIBILITY SameMajorVersion
)
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/hf_pca9685Config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/hf_pca9685ConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/hf_pca9685
)
