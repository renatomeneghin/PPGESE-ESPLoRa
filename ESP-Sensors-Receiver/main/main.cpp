#include "communication/UartReceiver.hpp"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ml/TemperatureClassifier.hpp"

namespace {
constexpr char kTag[] = "ESP-Sensors-Receiver";
constexpr uart_port_t kSensorUart = UART_NUM_1;
constexpr int kSensorRxPin = 16;
constexpr int kSensorTxPin = 17;
constexpr int kSensorBaudRate = 115200;

void receiver_task(void* argument) {
    auto* receiver = static_cast<UartReceiver*>(argument);
    static const TemperatureClassifier classifier;
    sensor_protocol::SensorPacket packet{};

    for (;;) {
        if (receiver->receive(packet, pdMS_TO_TICKS(1000U))) {
            const auto temperature_class = classifier.classify(packet.temperature_celsius);
            ESP_LOGI(kTag, "analog=%.2f digital=%u timestamp=%lu sequence=%lu",
                     packet.analog_value,
                     static_cast<unsigned>(packet.digital_value),
                     static_cast<unsigned long>(packet.timestamp_ms),
                     static_cast<unsigned long>(packet.sequence));
            ESP_LOGI(kTag, "temperature=%.2f C humidity=%.1f%% classification=%s",
                     packet.temperature_celsius, packet.humidity_percent,
                     TemperatureClassifier::label(temperature_class));
        } else {
            ESP_LOGW(kTag, "Invalid or incomplete sensor packet");
        }
    }
}
} // namespace

extern "C" void app_main() {
    static UartReceiver receiver(kSensorUart, kSensorRxPin, kSensorTxPin, kSensorBaudRate);
    ESP_ERROR_CHECK(receiver.begin());
    xTaskCreate(receiver_task, "ReceiverTask", 4096U, &receiver, 5, nullptr);
}
