#include "SensorManager.hpp"

#include "Config.hpp"
#include <Adafruit_BME280.h>
#include <Wire.h>
#include "Mpu6050Sensor.hpp"

namespace sensor_manager {
namespace {
Adafruit_BME280 bmp;
uint32_t simulated_index = 0;

}


RawSample read() {
    RawSample sample{};
    sample.timestamp_ms = millis();
    if (config::USE_SIMULATED_SENSOR) {
        sample.signal = 1000.0F + static_cast<float>(simulated_index++ % 20U) * 0.2F;
        sample.pressure_hpa = sample.signal;
        sample.temperature_celsius = 22.0F;
        return sample;
    }

    sample.pressure_hpa = bmp.readPressure() / 100.0F;
    sample.temperature_celsius = bmp.readTemperature();
    sample.signal = sample.pressure_hpa;

    sample.timestamp_ms = millis();
    return sample;
}
} // namespace sensor_manager
