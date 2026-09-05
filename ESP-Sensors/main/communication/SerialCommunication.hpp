#pragma once

#include "Communication.hpp"

class SerialCommunication final : public Communication {
public:
    esp_err_t begin() override;
    esp_err_t send(const SensorData& data) override;

private:
    uart_port_t port_{UART_NUM_1};
    uint32_t sequence_{0U};
};
