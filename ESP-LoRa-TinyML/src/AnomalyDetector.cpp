#include "AnomalyDetector.hpp"

namespace {
constexpr float kRmsAnomalyThreshold = 16.0F;
constexpr float kVarianceWarningThreshold = 1.5F;
constexpr float kPeakAnomalyThreshold = 20.0F;
}

AnomalyState detect_anomaly(const FeatureVector& features) {
    if (features.rms > kRmsAnomalyThreshold || features.peak > kPeakAnomalyThreshold) {
        return AnomalyState::Anomaly;
    }
    if (features.variance > kVarianceWarningThreshold) return AnomalyState::Warning;
    return AnomalyState::Normal;
}
