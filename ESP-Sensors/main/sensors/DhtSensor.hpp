#pragma once

#include "driver/gpio.h"
#include "esp_err.h"
enum class DhtType { Dht11, Dht22 };

namespace dht_sensor {
esp_err_t begin(gpio_num_t pin);
bool read(gpio_num_t pin, DhtType type, float& temperature_celsius, float& humidity_percent);
} // namespace dht_sensor
