#pragma once

#include <Arduino.h>
#include <cstddef>
#include <cstdint>

namespace config {
const bool USE_SIMULATED_SENSOR = false;
#ifndef USE_MPU6050
#define USE_MPU6050 1
#endif
#ifndef USE_ESP_DSP
#define USE_ESP_DSP 1
#endif
const uint8_t BMP_I2C_SDA_PIN = 21;
const uint8_t BMP_I2C_SCL_PIN = 22;
const uint8_t MPU_I2C_SDA_PIN = 4;
const uint8_t MPU_I2C_SCL_PIN = 15;
const uint8_t BMP280_I2C_ADDRESS = 0x76;
const uint8_t MPU6050_I2C_ADDRESS = 0x68;

// Common Heltec WiFi LoRa 32 V2 mapping. Confirm against the exact revision.
const int8_t LORA_CS_PIN = 18;
const int8_t LORA_DIO0_PIN = 26;
const int8_t LORA_RESET_PIN = 14;
const int8_t LORA_DIO1_PIN = 33;
const int8_t LORA_SCK_PIN = 5;
const int8_t LORA_MISO_PIN = 19;
const int8_t LORA_MOSI_PIN = 27;
const float LORA_FREQUENCY_MHZ = 915.0F;
const float LORA_BANDWIDTH_KHZ = 125.0F;
const uint8_t LORA_SPREADING_FACTOR = 7;
const uint8_t LORA_CODING_RATE = 5;
const int8_t LORA_TX_POWER_DBM = 10;

const uint32_t SAMPLE_PERIOD_MS = 20;
const size_t SAMPLE_BLOCK_SIZE = 32;
const uint8_t RAW_QUEUE_LENGTH = 2;
const uint8_t TELEMETRY_QUEUE_LENGTH = 2;
}
