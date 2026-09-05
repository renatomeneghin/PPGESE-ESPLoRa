#include "AnalogTemperatureSensor.hpp"

#include "config.hpp"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"

namespace {
constexpr char kTag[] = "AnalogTemperature";
adc_oneshot_unit_handle_t s_adc_handle = nullptr;
float s_simulated_temperature = 20.0F;
}

esp_err_t analog_temperature::begin(adc_channel_t channel) {
#if USE_SIMULATED_SENSORS
    ESP_LOGI(kTag, "Using simulated analog temperature values (channel %d)", channel);
    return ESP_OK;
#else
    adc_oneshot_unit_init_cfg_t unit_config{};
    unit_config.unit_id = ADC_UNIT_1;
    unit_config.ulp_mode = ADC_ULP_MODE_DISABLE;
    esp_err_t err = adc_oneshot_new_unit(&unit_config, &s_adc_handle);
    if (err != ESP_OK) return err;

    adc_oneshot_chan_cfg_t channel_config{};
    channel_config.bitwidth = ADC_BITWIDTH_DEFAULT;
    channel_config.atten = ADC_ATTEN_DB_12;
    return adc_oneshot_config_channel(s_adc_handle, channel, &channel_config);
#endif
}

bool analog_temperature::read(adc_channel_t channel, AnalogTemperatureType type,
                              float& temperature_celsius) {
#if USE_SIMULATED_SENSORS
    s_simulated_temperature += 0.25F;
    if (s_simulated_temperature > 32.0F) s_simulated_temperature = 18.0F;
    temperature_celsius = s_simulated_temperature;
    return true;
#else
    int raw_value = 0;
    if (adc_oneshot_read(s_adc_handle, channel, &raw_value) != ESP_OK) {
        ESP_LOGW(kTag, "Analog temperature ADC read failed");
        return false;
    }
    const float millivolts = static_cast<float>(raw_value) * 3300.0F / 4095.0F;
    temperature_celsius = type == AnalogTemperatureType::Lm35
                              ? millivolts / 10.0F
                              : (millivolts - 500.0F) / 10.0F;
    return true;
#endif
}
