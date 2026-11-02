/**
 * @file esp32_pca9685_bus.hpp
 * @brief ESP32 I2C bus implementation for PCA9685 driver
 *
 * This file provides the ESP32-specific implementation of the pca9685::I2cInterface
 * interface using ESP-IDF's I2C master driver.
 *
 * @note This implementation mirrors the proven esp32_pcal95555_bus.hpp pattern
 *       for consistent, working I2C communication across HardFOC drivers.
 *
 * @author Nebiyu Tadesse
 * @date 2025
 * @copyright HardFOC
 */

#pragma once

// System headers
#include <array>
#include <cstring>
#include <memory>

// Third-party headers (ESP-IDF)
#ifdef __cplusplus
extern "C" {
#endif
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#ifdef __cplusplus
}
#endif

// Project headers (interface only; include pca9685.hpp in app after this header so driver sees full
// bus type)
#include "pca9685_i2c_interface.hpp"

static constexpr const char* TAG_I2C = "PCA9685_I2C";

/**
 * @class Esp32Pca9685I2cBus
 * @brief ESP32 implementation of pca9685::I2cInterface using ESP-IDF I2C master driver.
 *
 * This class mirrors the proven Esp32Pcal9555Bus implementation for consistent
 * I2C communication behavior across HardFOC drivers.
 */
class Esp32Pca9685I2cBus : public pca9685::I2cInterface<Esp32Pca9685I2cBus> {
public:
  /**
   * @brief I2C bus configuration structure
   */
  struct I2CConfig {
    i2c_port_t port = I2C_NUM_0;     ///< I2C port number
    gpio_num_t sda_pin = GPIO_NUM_4; ///< SDA pin (default GPIO4)
    gpio_num_t scl_pin = GPIO_NUM_5; ///< SCL pin (default GPIO5)
    uint32_t frequency = 100000;     ///< I2C frequency in Hz (default 100kHz for PCA9685)
    uint32_t scl_wait_us = 0;  ///< SCL clock-stretching timeout in us (0 = ESP-IDF default; set >0
                               ///< to allow slave stretching)
    bool pullup_enable = true; ///< Enable internal pullups
  };

  /**
   * @brief Constructor with default configuration
   */
  Esp32Pca9685I2cBus() : Esp32Pca9685I2cBus(I2CConfig{}) {}

  /**
   * @brief Constructor with custom I2C configuration
   * @param config I2C bus configuration
   */
  explicit Esp32Pca9685I2cBus(const I2CConfig& config)
      : config_(config), bus_handle_(nullptr), initialized_(false) {}

  /**
   * @brief Destructor - cleans up I2C resources
   */
  ~Esp32Pca9685I2cBus() {
    Deinit();
  }

  /**
   * @brief Ensure the I2C bus is initialized and ready (required by I2cInterface)
   * @return true if successful, false otherwise
   */
  bool EnsureInitialized() noexcept {
    return Init();
  }

  /**
   * @brief Initialize the I2C bus
   * @return true if successful, false otherwise
   */
  bool Init() noexcept {
    if (initialized_) {
      ESP_LOGW(TAG_I2C, "I2C bus already initialized");
      return true;
    }

    ESP_LOGI(TAG_I2C, "Initializing I2C bus on port %d (SDA:GPIO%d, SCL:GPIO%d, Freq:%lu Hz)",
             config_.port, config_.sda_pin, config_.scl_pin, config_.frequency);

    // Configure I2C master bus
    i2c_master_bus_config_t bus_config = {};
    bus_config.i2c_port = config_.port;
    bus_config.sda_io_num = config_.sda_pin;
    bus_config.scl_io_num = config_.scl_pin;
    bus_config.clk_source = I2C_CLK_SRC_DEFAULT;
    bus_config.glitch_ignore_cnt = 7;
    bus_config.flags.enable_internal_pullup = config_.pullup_enable;
    bus_config.intr_priority = 0;
    bus_config.trans_queue_depth = 0;

    esp_err_t ret = i2c_new_master_bus(&bus_config, &bus_handle_);
    if (ret != ESP_OK) {
      ESP_LOGE(TAG_I2C, "Failed to create I2C master bus: %s", esp_err_to_name(ret));
      return false;
    }

    initialized_ = true;
    ESP_LOGI(TAG_I2C, "I2C bus initialized successfully");
    return true;
  }

  /**
   * @brief Deinitialize the I2C bus
   */
  void Deinit() noexcept {
    if (!initialized_) {
      return;
    }

    // Remove cached device handle before deleting bus
    if (dev_handle_ != nullptr) {
      i2c_master_bus_rm_device(dev_handle_);
      dev_handle_ = nullptr;
      cached_dev_addr_ = 0xFF;
    }

    if (bus_handle_ != nullptr) {
      i2c_del_master_bus(bus_handle_);
      bus_handle_ = nullptr;
    }

    initialized_ = false;
    ESP_LOGI(TAG_I2C, "I2C bus deinitialized");
  }

  /**
   * @brief Write bytes to a device register
   * @param addr 7-bit I2C address of the target device
   * @param reg Register address to write to
   * @param data Pointer to the data buffer containing bytes to send
   * @param len Number of bytes to write from the buffer
   * @return true if the device acknowledges the transfer; false on NACK or error
   */
  bool Write(uint8_t addr, uint8_t reg, const uint8_t* data, size_t len) noexcept {
    if (!initialized_ || bus_handle_ == nullptr) {
      ESP_LOGE(TAG_I2C, "I2C bus not initialized");
      return false;
    }

    i2c_master_dev_handle_t dev = getOrCreateDeviceHandle(addr);
    if (dev == nullptr) {
      return false;
    }

    // Prepare write buffer: register address + data
    std::array<uint8_t, 32> write_buffer{}; // Max 32 bytes (register + 31 data bytes)
    if (len > 31) {
      ESP_LOGE(TAG_I2C, "Write length %zu exceeds maximum (31 bytes)", len);
      return false;
    }

    write_buffer[0] = reg;
    if (len > 0 && data != nullptr) {
      memcpy(&write_buffer[1], data, len);
    }

    // Perform I2C write transaction
    esp_err_t ret = i2c_master_transmit(dev, write_buffer.data(), len + 1, pdMS_TO_TICKS(1000));
    if (ret != ESP_OK) {
      ESP_LOGE(TAG_I2C, "I2C write failed: %s (addr=0x%02X, reg=0x%02X, len=%zu)",
               esp_err_to_name(ret), addr, reg, len);
      return false;
    }

    return true;
  }

  /**
   * @brief Read bytes from a device register
   * @param addr 7-bit I2C address of the target device
   * @param reg Register address to read from
   * @param data Pointer to the buffer to store received data
   * @param len Number of bytes to read into the buffer
   * @return true if the read succeeds; false on NACK or error
   */
  bool Read(uint8_t addr, uint8_t reg, uint8_t* data, size_t len) noexcept {
    if (!initialized_ || bus_handle_ == nullptr) {
      ESP_LOGE(TAG_I2C, "I2C bus not initialized");
      return false;
    }

    if (data == nullptr || len == 0) {
      ESP_LOGE(TAG_I2C, "Invalid read parameters");
      return false;
    }

    i2c_master_dev_handle_t dev = getOrCreateDeviceHandle(addr);
    if (dev == nullptr) {
      return false;
    }

    // Write register address, then read data
    esp_err_t ret = i2c_master_transmit_receive(dev, &reg, 1, data, len, pdMS_TO_TICKS(1000));
    if (ret != ESP_OK) {
      ESP_LOGE(TAG_I2C, "I2C read failed: %s (addr=0x%02X, reg=0x%02X, len=%zu)",
               esp_err_to_name(ret), addr, reg, len);
      return false;
    }

    return true;
  }

  /**
   * @brief Busy-wait for at least @p us microseconds (optional I2cInterface hook).
   *
   * The driver calls it with 500 after clearing MODE1.SLEEP, for the oscillator start-up time
   * (PCA9685 datasheet Table 5, note 2).
   * @param us Time to wait in microseconds.
   */
  void DelayUs(uint32_t us) noexcept {
    esp_rom_delay_us(us);
  }

  /**
   * @brief Optional delay callback for PCA9685 driver retries (1 ms task delay).
   *
   * Pass to driver via SetRetryDelay(Esp32Pca9685I2cBus::RetryDelay) after creating
   * the driver to allow I2C bus recovery between retry attempts.
   */
  static void RetryDelay() noexcept {
    vTaskDelay(pdMS_TO_TICKS(1));
  }

  /**
   * @brief Get the I2C configuration
   * @return Reference to the I2C configuration
   */
  [[nodiscard]] const I2CConfig& GetConfig() const noexcept {
    return config_;
  }

  /**
   * @brief Check if the bus is initialized
   * @return true if initialized, false otherwise
   */
  [[nodiscard]] bool IsInitialized() const noexcept {
    return initialized_;
  }

private:
  I2CConfig config_;
  i2c_master_bus_handle_t bus_handle_;
  bool initialized_;

  // Cached device handle -- avoids add_device/rm_device per transaction
  i2c_master_dev_handle_t dev_handle_{nullptr};
  uint8_t cached_dev_addr_{0xFF};

  /**
   * @brief Get or create a cached I2C device handle for the given address.
   *
   * If the cached handle matches the requested address, it is reused.
   * Otherwise the old handle is removed and a new one is created.
   *
   * @param addr 7-bit I2C device address
   * @return Device handle, or nullptr on failure
   */
  i2c_master_dev_handle_t getOrCreateDeviceHandle(uint8_t addr) noexcept {
    if (dev_handle_ != nullptr && cached_dev_addr_ == addr) {
      return dev_handle_; // Reuse cached handle
    }

    // Address changed or first call -- evict old handle
    if (dev_handle_ != nullptr) {
      i2c_master_bus_rm_device(dev_handle_);
      dev_handle_ = nullptr;
    }

    i2c_device_config_t dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = addr,
        .scl_speed_hz = config_.frequency,
        .scl_wait_us = config_.scl_wait_us,
        .flags = {},
    };

    esp_err_t ret = i2c_master_bus_add_device(bus_handle_, &dev_config, &dev_handle_);
    if (ret != ESP_OK) {
      ESP_LOGE(TAG_I2C, "Failed to add device 0x%02X: %s", addr, esp_err_to_name(ret));
      dev_handle_ = nullptr;
      cached_dev_addr_ = 0xFF;
      return nullptr;
    }

    cached_dev_addr_ = addr;
    return dev_handle_;
  }
};

/**
 * @brief Factory function to create an ESP32 I2C bus instance
 * @param config I2C configuration (optional, uses defaults if not provided)
 * @return Unique pointer to Esp32Pca9685I2cBus instance
 */
inline std::unique_ptr<Esp32Pca9685I2cBus> CreateEsp32Pca9685I2cBus(
    const Esp32Pca9685I2cBus::I2CConfig& config = Esp32Pca9685I2cBus::I2CConfig{}) {
  auto bus = std::make_unique<Esp32Pca9685I2cBus>(config);
  if (!bus->Init()) {
    ESP_LOGE(TAG_I2C, "Failed to initialize I2C bus");
    return nullptr;
  }
  return bus;
}
