#pragma once

#include "data/SensorPacket.hpp"
#include "driver/uart.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"

class UartReceiver {
public:
    UartReceiver(uart_port_t port, int rx_pin, int tx_pin, int baud_rate);

    esp_err_t begin();
    bool receive(sensor_protocol::SensorPacket& packet, TickType_t timeout_ticks) const;

private:
    uart_port_t port_;
    int rx_pin_;
    int tx_pin_;
    int baud_rate_;
};
