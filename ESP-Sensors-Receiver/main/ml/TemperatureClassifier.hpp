#pragma once

class TemperatureClassifier {
public:
    enum class Class {
        Cold,
        Warm,
        Hot,
    };

    // Lightweight inference: selects the closest learned class centroid.
    Class classify(float temperature_celsius) const;
    static const char* label(Class value);
};
