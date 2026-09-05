#pragma once

#include <cstddef>
#include <cstdint>

namespace sensor_protocol {
constexpr uint16_t kMagic = 0x5345;
constexpr uint8_t kVersion = 1U;

struct __attribute__((packed)) SensorPacket {
    uint16_t magic{kMagic};
    uint8_t version{kVersion};
    uint8_t payload_size{0U};
    uint32_t sequence{0U};
    float analog_value{0.0F};
    float temperature_celsius{0.0F};
    float humidity_percent{0.0F};
    uint8_t digital_value{0U};
    uint8_t reserved[3]{0U, 0U, 0U};
    uint32_t timestamp_ms{0U};
    uint16_t crc16{0U};
};

static_assert(sizeof(SensorPacket) == 30U, "SensorPacket layout changed");

inline uint16_t crc16_ccitt(const uint8_t* data, size_t length) {
    uint16_t crc = 0xFFFFU;
    for (size_t index = 0; index < length; ++index) {
        crc ^= static_cast<uint16_t>(data[index]) << 8U;
        for (uint8_t bit = 0; bit < 8U; ++bit) {
            crc = (crc & 0x8000U) != 0U ? static_cast<uint16_t>((crc << 1U) ^ 0x1021U)
                                        : static_cast<uint16_t>(crc << 1U);
        }
    }
    return crc;
}

inline uint16_t calculate_crc(const SensorPacket& packet) {
    return crc16_ccitt(reinterpret_cast<const uint8_t*>(&packet), sizeof(SensorPacket) - 2U);
}
} // namespace sensor_protocol
