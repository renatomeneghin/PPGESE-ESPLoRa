#pragma once

#include "DataTypes.hpp"

class DspProcessor {
public:
    FeatureVector process(const RawSample* samples, size_t count) const;
};
