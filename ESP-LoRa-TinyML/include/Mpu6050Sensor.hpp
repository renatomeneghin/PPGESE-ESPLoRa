#pragma once

#include "DataTypes.hpp"
#include <stdint.h>

class Mpu6050Sensor {
public:
    bool begin(uint8_t address);
    RawSample read();
};
