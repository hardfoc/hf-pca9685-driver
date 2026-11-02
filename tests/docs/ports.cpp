/**
 * @file ports.cpp
 * @brief Instantiates the driver over the documented port, so every member compiles.
 */
#include "my_i2c.hpp"

#include "pca9685.hpp"

template class pca9685::PCA9685<MyI2c>;
