# =============================================================================
# hf-pca9685-driver: install check (run by CTest as hf_pca9685_install)
# =============================================================================
# Installs the driver from BUILD_DIR into a scratch prefix, then configures, builds and runs
# tests/install_consumer against that prefix only. Fails if the package is incomplete (a missing
# header, version header or .ipp) or exports the wrong target name.
#
# Variables: BUILD_DIR, SOURCE_DIR, WORK_DIR, CXX_COMPILER, GENERATOR
# =============================================================================
file(REMOVE_RECURSE "${WORK_DIR}")
set(prefix "${WORK_DIR}/prefix")

function(run)
  execute_process(COMMAND ${ARGN} RESULT_VARIABLE rc)
  if(NOT rc EQUAL 0)
    message(FATAL_ERROR "Install check failed (${rc}): ${ARGN}")
  endif()
endfunction()

run("${CMAKE_COMMAND}" --install "${BUILD_DIR}" --prefix "${prefix}")
run("${CMAKE_COMMAND}" -S "${SOURCE_DIR}/tests/install_consumer" -B "${WORK_DIR}/consumer"
    -G "${GENERATOR}" "-DCMAKE_CXX_COMPILER=${CXX_COMPILER}" "-DCMAKE_PREFIX_PATH=${prefix}"
    -DCMAKE_FIND_USE_PACKAGE_REGISTRY=OFF)
run("${CMAKE_COMMAND}" --build "${WORK_DIR}/consumer")
find_program(consumer NAMES consumer consumer.exe PATHS "${WORK_DIR}/consumer"
             PATH_SUFFIXES Debug Release NO_DEFAULT_PATH REQUIRED)
run("${consumer}")
message(STATUS "Install check passed: ${prefix}")
