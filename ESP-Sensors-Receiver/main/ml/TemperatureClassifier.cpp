#include "TemperatureClassifier.hpp"

#include <cmath>

namespace {
// Initial model for a first bench test. Replace with centroids learned from
// labeled measurements when a real temperature dataset is available.
constexpr float kColdCentroid = 10.0F;
constexpr float kWarmCentroid = 23.0F;
constexpr float kHotCentroid = 35.0F;
}

TemperatureClassifier::Class TemperatureClassifier::classify(float temperature_celsius) const {
    const float cold_distance = std::fabs(temperature_celsius - kColdCentroid);
    const float warm_distance = std::fabs(temperature_celsius - kWarmCentroid);
    const float hot_distance = std::fabs(temperature_celsius - kHotCentroid);

    if (cold_distance <= warm_distance && cold_distance <= hot_distance) {
        return Class::Cold;
    }
    if (warm_distance <= hot_distance) {
        return Class::Warm;
    }
    return Class::Hot;
}

const char* TemperatureClassifier::label(Class value) {
    switch (value) {
    case Class::Cold:
        return "frio";
    case Class::Warm:
        return "morno";
    case Class::Hot:
        return "quente";
    }
    return "desconhecido";
}
