#include "DhtSensor.hpp"

#include "config.hpp"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "esp_timer.h"

namespace {
constexpr char kTag[] = "DhtSensor";
constexpr int64_t kTimeoutUs = 200;
float s_simulated_temperature = 20.0F;
float s_last_temperature = 0.0F;
float s_last_humidity = 0.0F;
int64_t s_last_read_us = 0;

bool wait_for_level(gpio_num_t pin, int level) {
    const int64_t deadline = esp_timer_get_time() + kTimeoutUs;
    while (gpio_get_level(pin) != level) {
        if (esp_timer_get_time() >= deadline) return false;
    }
    return true;
}
}

esp_err_t dht_sensor::begin(gpio_num_t pin) {
    gpio_config_t config{};
    config.pin_bit_mask = 1ULL << pin;
    config.mode = GPIO_MODE_OUTPUT_OD;
    config.pull_up_en = GPIO_PULLUP_ENABLE;
    config.pull_down_en = GPIO_PULLDOWN_DISABLE;
    config.intr_type = GPIO_INTR_DISABLE;
    const esp_err_t err = gpio_config(&config);
    gpio_set_level(pin, 1);
    return err;
}

bool dht_sensor::read(gpio_num_t pin, DhtType type, float& temperature_celsius,
                      float& humidity_percent) {
#if USE_SIMULATED_SENSORS
    s_simulated_temperature += 0.2F;
    if (s_simulated_temperature > 30.0F) s_simulated_temperature = 20.0F;
    temperature_celsius = s_simulated_temperature;
    humidity_percent = 55.0F;
    return true;
#else
    if (s_last_read_us != 0 && esp_timer_get_time() - s_last_read_us < 2000000LL) {
        temperature_celsius = s_last_temperature;
        humidity_percent = s_last_humidity;
        return true;
    }
    uint8_t bytes[5]{};
    gpio_set_direction(pin, GPIO_MODE_OUTPUT_OD);
    gpio_set_level(pin, 0);
    esp_rom_delay_us(type == DhtType::Dht11 ? 20000U : 1100U);
    gpio_set_level(pin, 1);
    esp_rom_delay_us(30U);
    gpio_set_direction(pin, GPIO_MODE_INPUT);
    if (!wait_for_level(pin, 0) || !wait_for_level(pin, 1) || !wait_for_level(pin, 0)) {
        ESP_LOGW(kTag, "DHT response timeout");
        return false;
    }
    for (int bit_index = 0; bit_index < 40; ++bit_index) {
        if (!wait_for_level(pin, 1)) return false;
        const int64_t high_start = esp_timer_get_time();
        if (!wait_for_level(pin, 0)) return false;
        const int64_t high_time = esp_timer_get_time() - high_start;
        bytes[bit_index / 8] <<= 1U;
        if (high_time > 50) bytes[bit_index / 8] |= 1U;
    }
    gpio_set_direction(pin, GPIO_MODE_OUTPUT_OD);
    gpio_set_level(pin, 1);
    if (static_cast<uint8_t>(bytes[0] + bytes[1] + bytes[2] + bytes[3]) != bytes[4]) {
        ESP_LOGW(kTag, "DHT checksum error");
        return false;
    }
    if (type == DhtType::Dht11) {
        humidity_percent = bytes[0];
        temperature_celsius = bytes[2];
    } else {
        humidity_percent = static_cast<float>((bytes[0] << 8U) | bytes[1]) / 10.0F;
        const uint16_t raw_temperature = static_cast<uint16_t>(((bytes[2] & 0x7FU) << 8U) | bytes[3]);
        temperature_celsius = static_cast<float>(raw_temperature) / 10.0F;
        if ((bytes[2] & 0x80U) != 0U) temperature_celsius = -temperature_celsius;
    }
    s_last_temperature = temperature_celsius;
    s_last_humidity = humidity_percent;
    s_last_read_us = esp_timer_get_time();
    return true;
#endif
}
