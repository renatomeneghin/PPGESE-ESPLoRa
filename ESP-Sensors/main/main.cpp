#include "communication/SerialCommunication.hpp"
#include "config.hpp"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sensors/AnalogSensor.hpp"
#include "sensors/DigitalSensor.hpp"
#include "sensors/DhtSensor.hpp"
#include "sensors/AnalogTemperatureSensor.hpp"

namespace {
constexpr char kTag[] = "ESP-Sensors";
constexpr adc_channel_t kAnalogChannel = ADC_CHANNEL_6; // GPIO34 on the classic ESP32.
constexpr gpio_num_t kDigitalPin = GPIO_NUM_35;         // Input-only pin on the classic ESP32.
constexpr gpio_num_t kDhtPin = GPIO_NUM_4;
constexpr TickType_t kAcquisitionPeriod = pdMS_TO_TICKS(1000U);
constexpr uint32_t kTaskStackWords = 4096U;

struct AcquisitionContext {
    AnalogSensor* analog_sensor;
    DigitalSensor* digital_sensor;
    Communication* communication;
};

void acquisition_task(void* argument) {
    const auto* context = static_cast<const AcquisitionContext*>(argument);
    TickType_t last_wake = xTaskGetTickCount();

    for (;;) {
        SensorData data{};
        data.analog_value = context->analog_sensor->read();
        data.digital_value = context->digital_sensor->read();
#if USE_DHT_SENSOR
        const bool temperature_ok = dht_sensor::read(kDhtPin, DhtType::Dht22,
                                                     data.temperature_celsius,
                                                     data.humidity_percent);
#else
        const bool temperature_ok = analog_temperature::read(
            kAnalogChannel, AnalogTemperatureType::Lm35, data.temperature_celsius);
        data.humidity_percent = 0.0F;
#endif
        if (!temperature_ok) {
            ESP_LOGW(kTag, "DHT read failed; values remain zero for this sample");
        }
        data.timestamp_ms = static_cast<uint32_t>(esp_timer_get_time() / 1000LL);
        context->communication->send(data);
        vTaskDelayUntil(&last_wake, kAcquisitionPeriod);
    }
}
} // namespace

extern "C" void app_main() {
    static AnalogSensor analog_sensor(kAnalogChannel);
    static DigitalSensor digital_sensor(kDigitalPin);
    static SerialCommunication communication;
    static AcquisitionContext context{&analog_sensor, &digital_sensor, &communication};
    static StaticTask_t task_buffer;
    static StackType_t task_stack[kTaskStackWords];

    ESP_LOGI(kTag, "Starting ESP-Sensors (simulation=%d)", USE_SIMULATED_SENSORS);
    ESP_ERROR_CHECK(analog_sensor.begin());
    ESP_ERROR_CHECK(digital_sensor.begin());
#if USE_DHT_SENSOR
    ESP_ERROR_CHECK(dht_sensor::begin(kDhtPin));
#else
    ESP_ERROR_CHECK(analog_temperature::begin(kAnalogChannel));
#endif
    ESP_ERROR_CHECK(communication.begin());

    if (xTaskCreateStatic(acquisition_task, "AcquisitionTask", kTaskStackWords,
                          &context, 5, task_stack, &task_buffer) == nullptr) {
        ESP_LOGE(kTag, "Could not create acquisition task");
    }
}
