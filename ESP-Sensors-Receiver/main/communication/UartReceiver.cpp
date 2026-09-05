#include "UartReceiver.hpp"

#include "esp_log.h"

namespace {
constexpr char kTag[] = "UartReceiver";
}

UartReceiver::UartReceiver(uart_port_t port, int rx_pin, int tx_pin, int baud_rate)
    : port_(port), rx_pin_(rx_pin), tx_pin_(tx_pin), baud_rate_(baud_rate) {}

esp_err_t UartReceiver::begin() {
    uart_config_t config{};
    config.baud_rate = baud_rate_;
    config.data_bits = UART_DATA_8_BITS;
    config.parity = UART_PARITY_DISABLE;
    config.stop_bits = UART_STOP_BITS_1;
    config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
    config.source_clk = UART_SCLK_DEFAULT;
    esp_err_t err = uart_driver_install(port_, 512U, 0U, 0U, nullptr, 0U);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(kTag, "UART driver install failed: %s", esp_err_to_name(err));
        return err;
    }
    ESP_ERROR_CHECK(uart_param_config(port_, &config));
    ESP_ERROR_CHECK(uart_set_pin(port_, tx_pin_, rx_pin_, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    ESP_LOGI(kTag, "UART%d ready: RX=%d TX=%d baud=%d", port_, rx_pin_, tx_pin_, baud_rate_);
    return ESP_OK;
}

bool UartReceiver::receive(sensor_protocol::SensorPacket& packet, TickType_t timeout_ticks) const {
    uint8_t byte = 0U;
    auto* packet_bytes = reinterpret_cast<uint8_t*>(&packet);
    size_t matched = 0U;

    while (matched < sizeof(sensor_protocol::SensorPacket)) {
        const int count = uart_read_bytes(port_, &byte, 1U, timeout_ticks);
        if (count != 1) {
            return false;
        }

        // Resynchronize on the first two magic bytes before collecting a packet.
        if (matched == 0U && byte != static_cast<uint8_t>(sensor_protocol::kMagic & 0xFFU)) {
            continue;
        }
        if (matched == 1U && byte != static_cast<uint8_t>(sensor_protocol::kMagic >> 8U)) {
            matched = 0U;
            continue;
        }
        packet_bytes[matched++] = byte;
    }

    return sensor_protocol::is_valid(packet);
}
