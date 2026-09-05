#pragma once

#include "DataTypes.hpp"

namespace sensor_manager {
bool begin();
RawSample read();
}
