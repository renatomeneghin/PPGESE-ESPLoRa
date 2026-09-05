#include "SerialCommunication.hpp"

#include "data/SensorPacket.hpp"
#include "esp_log.h"

namespace {
constexpr char kTag[] = "SerialCommunication";
}

esp_err_t SerialCommunication::begin() {
    uart_config_t config{};
    config.baud_rate = 115200;
    config.data_bits = UART_DATA_8_BITS;
    config.parity = UART_PARITY_DISABLE;
    config.stop_bits = UART_STOP_BITS_1;
    config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
    config.source_clk = UART_SCLK_DEFAULT;
    ESP_ERROR_CHECK(uart_driver_install(port_, 256U, 512U, 0U, nullptr, 0U));
    ESP_ERROR_CHECK(uart_param_config(port_, &config));
    ESP_ERROR_CHECK(uart_set_pin(port_, 17, 16, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    ESP_LOGI(kTag, "UART1 sensor link ready: TX=17 RX=16 baud=115200");
    return ESP_OK;
}

esp_err_t SerialCommunication::send(const SensorData& data) {
    sensor_protocol::SensorPacket packet{};
    packet.sequence = sequence_++;
    packet.analog_value = data.analog_value;
    packet.temperature_celsius = data.temperature_celsius;
    packet.humidity_percent = data.humidity_percent;
    packet.digital_value = data.digital_value ? 1U : 0U;
    packet.timestamp_ms = data.timestamp_ms;
    packet.crc16 = sensor_protocol::calculate_crc(packet);
    const int written = uart_write_bytes(port_, &packet, sizeof(packet));
    if (written != static_cast<int>(sizeof(packet))) {
        ESP_LOGE(kTag, "Could not send complete sensor packet");
        return ESP_FAIL;
    }
    ESP_LOGI(kTag, "analog=%.2f digital=%d timestamp=%lu",
             data.analog_value,
             data.digital_value ? 1 : 0,
             static_cast<unsigned long>(data.timestamp_ms));
    return ESP_OK;
}
