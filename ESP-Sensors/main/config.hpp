#pragma once

// Keep enabled for bench testing without connected sensors. Set to 0 for real ADC/GPIO.
#ifndef USE_SIMULATED_SENSORS
#define USE_SIMULATED_SENSORS 1
#endif

// 1 uses the DHT22 on GPIO4; 0 uses the analog temperature sensor on ADC1_CH6/GPIO34.
#ifndef USE_DHT_SENSOR
#define USE_DHT_SENSOR 1
#endif
