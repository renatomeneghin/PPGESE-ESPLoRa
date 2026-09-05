#pragma once

#include "driver/adc.h"
#include "esp_err.h"

enum class AnalogTemperatureType {
    Lm35,
    Tmp36,
};

namespace analog_temperature {
esp_err_t begin(adc_channel_t channel);
bool read(adc_channel_t channel, AnalogTemperatureType type, float& temperature_celsius);
} // namespace analog_temperature
