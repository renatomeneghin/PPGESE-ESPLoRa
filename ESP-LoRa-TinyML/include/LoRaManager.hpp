#pragma once

#include "DataTypes.hpp"
#include <RadioLib.h>

class LoRaManager {
public:
    LoRaManager();
    int16_t begin();
    int16_t send(const TelemetryPacket& packet);

private:
    Module module_;
    SX1276 radio_;
};
