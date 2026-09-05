#pragma once

#include "sdkconfig.h"
#include "soc/soc_caps.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_err.h"

class AnalogSensor {
public:
    explicit AnalogSensor(adc_channel_t channel);

    esp_err_t begin();
    float read();

private:
    adc_channel_t channel_;
    adc_continuous_handle_t handle = NULL;
    int simulated_value_{0};
};
