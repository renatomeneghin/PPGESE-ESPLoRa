#pragma once

#include <Arduino.h>

namespace config {
constexpr bool USE_SIMULATED_MPU6050 = false;

constexpr uint8_t I2C_SDA_PIN = 4;
constexpr uint8_t I2C_SCL_PIN = 15;
constexpr uint8_t MPU6050_ADDRESS = 0x68;

// Common Heltec WiFi LoRa 32 V2 mapping. Confirm against the exact board revision.
constexpr int8_t LORA_CS_PIN = 18;    // NSS / CS
constexpr int8_t LORA_DIO0_PIN = 26;  // DIO0
constexpr int8_t LORA_RESET_PIN = 14; // RESET
constexpr int8_t LORA_DIO1_PIN = 33;  // DIO1; confirm on hardware
constexpr int8_t LORA_SCK_PIN = 5;
constexpr int8_t LORA_MISO_PIN = 19;
constexpr int8_t LORA_MOSI_PIN = 27;

constexpr float LORA_FREQUENCY_MHZ = 915.0F;
constexpr float LORA_BANDWIDTH_KHZ = 125.0F;
constexpr uint8_t LORA_SPREADING_FACTOR = 7;
constexpr uint8_t LORA_CODING_RATE = 5;
constexpr int8_t LORA_TX_POWER_DBM = 10; // Moderate power; never transmit without antenna.

constexpr uint32_t SAMPLE_PERIOD_MS = 20;
constexpr size_t SAMPLE_BLOCK_SIZE = 32;
constexpr uint8_t RAW_QUEUE_LENGTH = 2;
constexpr uint8_t TELEMETRY_QUEUE_LENGTH = 2;
}
