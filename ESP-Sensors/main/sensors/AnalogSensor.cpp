#include "AnalogSensor.hpp"

#include "config.hpp"
#include "esp_log.h"

#if !USE_SIMULATED_SENSORS
#include "esp_adc/adc_continuous.h"
#endif

namespace {
constexpr char kTag[] = "AnalogSensor";
}

AnalogSensor::AnalogSensor(adc_channel_t channel) : channel_(channel) {}

esp_err_t AnalogSensor::begin() {
#if USE_SIMULATED_SENSORS
    ESP_LOGI(kTag, "Using simulated ADC values (channel %d)", channel_);
    return ESP_OK;
#else
    adc_oneshot_unit_init_cfg_t unit_config{};
    unit_config.unit_id = ADC_UNIT_1;
    unit_config.ulp_mode = ADC_ULP_MODE_DISABLE;

    esp_err_t err = adc_oneshot_new_unit(&unit_config, &handle_);
    if (err != ESP_OK) {
        ESP_LOGE(kTag, "Could not initialize ADC unit: %s", esp_err_to_name(err));
        return err;
    }

    adc_oneshot_chan_cfg_t channel_config{};
    channel_config.bitwidth = ADC_BITWIDTH_DEFAULT;
    channel_config.atten = ADC_ATTEN_DB_12;
    err = adc_oneshot_config_channel(handle_, channel_, &channel_config);
    if (err != ESP_OK) {
        ESP_LOGE(kTag, "Could not configure ADC channel: %s", esp_err_to_name(err));
    }
    return err;
#endif
}

float AnalogSensor::read() {
#if USE_SIMULATED_SENSORS
    simulated_value_ = (simulated_value_ + 137) % 3300;
    return static_cast<float>(simulated_value_) / 1000.0F;
#else
    int raw_value = 0;
    const esp_err_t err = adc_oneshot_read(handle_, channel_, &raw_value);
    if (err != ESP_OK) {
        ESP_LOGW(kTag, "ADC read failed: %s", esp_err_to_name(err));
        return 0.0F;
    }
    return static_cast<float>(raw_value);
#endif
}
