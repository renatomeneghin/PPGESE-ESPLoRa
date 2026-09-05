#pragma once

#include "driver/gpio.h"
#include "esp_err.h"

class DigitalSensor {
public:
    explicit DigitalSensor(gpio_num_t pin);

    esp_err_t begin();
    bool read() const;

private:
    gpio_num_t pin_;
    mutable bool simulated_state_{false};
};
