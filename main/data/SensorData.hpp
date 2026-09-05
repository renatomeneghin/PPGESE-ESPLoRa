#pragma once

#include <cstdint>

struct SensorData {
    float analog_value{0.0F};
    float temperature_celsius{0.0F};
    float humidity_percent{0.0F};
    bool digital_value{false};
    uint32_t timestamp_ms{0U};
};
