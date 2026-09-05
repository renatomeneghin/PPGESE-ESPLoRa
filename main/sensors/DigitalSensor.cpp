#include "DigitalSensor.hpp"

#include "config.hpp"
#include "esp_log.h"

namespace {
constexpr char kTag[] = "DigitalSensor";
}

DigitalSensor::DigitalSensor(gpio_num_t pin) : pin_(pin) {}

esp_err_t DigitalSensor::begin() {
#if USE_SIMULATED_SENSORS
    ESP_LOGI(kTag, "Using simulated GPIO values (pin %d)", pin_);
    return ESP_OK;
#else
    gpio_config_t config{};
    config.pin_bit_mask = (1ULL << pin_);
    config.mode = GPIO_MODE_INPUT;
    config.pull_up_en = GPIO_PULLUP_DISABLE;
    config.pull_down_en = GPIO_PULLDOWN_DISABLE;
    config.intr_type = GPIO_INTR_DISABLE;
    return gpio_config(&config);
#endif
}

bool DigitalSensor::read() const {
#if USE_SIMULATED_SENSORS
    simulated_state_ = !simulated_state_;
    return simulated_state_;
#else
    return gpio_get_level(pin_) != 0;
#endif
}
