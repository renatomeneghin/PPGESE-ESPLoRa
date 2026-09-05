#include "Mpu6050Sensor.hpp"
#include "Config.hpp"

#if USE_MPU6050
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Arduino.h>
#include <cmath>
#include <Wire.h>

namespace {
Adafruit_MPU6050 mpu;
bool ready = false;
}

bool Mpu6050Sensor::begin(uint8_t address) {
    ready = mpu.begin(address, &Wire);
    if (ready) {
        mpu.setAccelerometerRange(MPU6050_RANGE_2_G);
        mpu.setGyroRange(MPU6050_RANGE_250_DEG);
        mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
    }
    Serial.printf("MPU6050 optional mode: %s\n", ready ? "ready" : "not found");
    return ready;
}

RawSample Mpu6050Sensor::read() {
    RawSample sample{};
    if (!ready) return sample;
    sensors_event_t acceleration, gyro, temperature;
    mpu.getEvent(&acceleration, &gyro, &temperature);
    sample.signal = std::sqrt(acceleration.acceleration.x * acceleration.acceleration.x +
                               acceleration.acceleration.y * acceleration.acceleration.y +
                               acceleration.acceleration.z * acceleration.acceleration.z);
    sample.temperature_celsius = temperature.temperature;
    sample.timestamp_ms = millis();
    return sample;
}
#endif
