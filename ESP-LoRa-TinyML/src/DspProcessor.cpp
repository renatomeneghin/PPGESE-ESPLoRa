#include "DspProcessor.hpp"

#include "Config.hpp"
#if USE_ESP_DSP
#include <esp_dsp.h>
#endif
#include <cmath>

namespace {
float acceleration_magnitude(const RawSample& sample) {
    return sample.signal;
}
}

FeatureVector process_samples(const RawSample* samples, size_t count) {
    FeatureVector result{};
    if (samples == nullptr || count == 0U) return result;
    if (count > config::SAMPLE_BLOCK_SIZE) count = config::SAMPLE_BLOCK_SIZE;
    float magnitudes[config::SAMPLE_BLOCK_SIZE]{};
    float sum = 0.0F;
    result.peak = 0.0F;
    for (size_t index = 0; index < count; ++index) {
        const float value = acceleration_magnitude(samples[index]);
        magnitudes[index] = value;
        sum += value;
        if (value > result.peak) result.peak = value;
    }
    result.mean = sum / static_cast<float>(count);
    float sum_squares = 0.0F;
    // Set USE_ESP_DSP to 0 if the Arduino/PlatformIO integration is unavailable.
#if USE_ESP_DSP
    if (dsps_dotprod_f32_ansi(magnitudes, magnitudes, &sum_squares,
                              static_cast<int>(count)) != ESP_OK) {
        return FeatureVector{};
    }
#else
    for (size_t index = 0; index < count; ++index) {
        sum_squares += magnitudes[index] * magnitudes[index];
    }
#endif
    result.rms = std::sqrt(sum_squares / static_cast<float>(count));
    for (size_t index = 0; index < count; ++index) {
        const float difference = magnitudes[index] - result.mean;
        result.variance += difference * difference;
    }
    result.variance /= static_cast<float>(count);
    // Future ESP-DSP extension point: FIR/IIR filtering, FFT and vectorized math.
    return result;
}
