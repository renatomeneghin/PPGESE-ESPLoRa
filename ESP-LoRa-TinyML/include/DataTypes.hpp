#pragma once

#include <Arduino.h>
#include <array>
#include "Config.hpp"

struct RawSample {
    float signal;
    float temperature_celsius;
    float pressure_hpa;
    uint32_t timestamp_ms;
};

struct SampleBlock {
    std::array<RawSample, config::SAMPLE_BLOCK_SIZE> samples;
    size_t count;
};

struct FeatureVector {
    float mean;
    float rms;
    float variance;
    float peak;
};

enum class AnomalyState : uint8_t { Normal, Warning, Anomaly };

struct TelemetryPacket {
    FeatureVector features;
    AnomalyState state;
    uint32_t timestamp_ms;
};
