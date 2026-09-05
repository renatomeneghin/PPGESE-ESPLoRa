#include "LoRaManager.hpp"

#include "Config.hpp"

#include <Arduino.h>
#include <cstdio>

LoRaManager::LoRaManager()
    : module_(config::LORA_CS_PIN, config::LORA_DIO0_PIN, config::LORA_RESET_PIN,
              config::LORA_DIO1_PIN),
      radio_(&module_) {}

int16_t LoRaManager::begin() {
    SPI.begin(config::LORA_SCK_PIN, config::LORA_MISO_PIN, config::LORA_MOSI_PIN,
              config::LORA_CS_PIN);
    return radio_.begin(config::LORA_FREQUENCY_MHZ,
                                       config::LORA_BANDWIDTH_KHZ,
                                       config::LORA_SPREADING_FACTOR,
                                       config::LORA_CODING_RATE,
                                       0x12, config::LORA_TX_POWER_DBM);
}

int16_t LoRaManager::send(const TelemetryPacket& packet) {
    char payload[128]{};
    const int length = snprintf(payload, sizeof(payload),
                                "RMS=%.3f,VAR=%.3f,PEAK=%.3f,STATE=%u,T=%lu",
                                packet.features.rms, packet.features.variance,
                                packet.features.peak, static_cast<unsigned>(packet.state),
                                static_cast<unsigned long>(packet.timestamp_ms));
    if (length <= 0 || length >= static_cast<int>(sizeof(payload))) return false;
    return radio_.transmit(payload);
    
}