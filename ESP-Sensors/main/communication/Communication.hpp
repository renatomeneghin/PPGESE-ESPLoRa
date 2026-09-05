#pragma once

#include "data/SensorData.hpp"
#include "esp_err.h"

class Communication {
public:
    virtual ~Communication() = default;
    virtual esp_err_t begin() = 0;
    virtual esp_err_t send(const SensorData& data) = 0;
};
