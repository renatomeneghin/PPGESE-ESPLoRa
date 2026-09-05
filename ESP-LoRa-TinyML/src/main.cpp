#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Adafruit_MPU6050.h>

// Define ESP32 I2C pins
#define I2C_SDA 21
#define I2C_SCL 22

#include "AnomalyDetector.hpp"
#include "Config.hpp"
#include "DspProcessor.hpp"
#include "LoRaManager.hpp"
#include "SensorManager.hpp"



namespace {
QueueHandle_t raw_queue = nullptr;
QueueHandle_t telemetry_queue = nullptr;
StaticQueue_t raw_queue_storage;
StaticQueue_t telemetry_queue_storage;
uint8_t raw_queue_memory[config::RAW_QUEUE_LENGTH * sizeof(SampleBlock)];
uint8_t telemetry_queue_memory[config::TELEMETRY_QUEUE_LENGTH * sizeof(TelemetryPacket)];


Adafruit_BME280 bmp; 
Adafruit_MPU6050 mpu;

struct BME280Data {
    float temperature;
    float pressure;
    float altitude;
    uint32_t timestamp_ms;
};

struct MPU6050Data {
    sensors_event_t acc;
    sensors_event_t gyro;
    sensors_event_t temp;
};

MPU6050Data mpu_block_1[1024]; // Buffer to hold MPU6050 data

QueueHandle_t mpu_queue = nullptr; // Queue to hold MPU6050 data blocks

void BME280_task(void* argument) {

    BME280Data block{};
    TickType_t last_wake = xTaskGetTickCount();

    if (!bmp.begin(0x76)) 
    {
        Serial.println(F("Could not find a valid BMP280 sensor, check wiring or address!"));
        vTaskDelete(nullptr);
    }

    while (1) {
        block.pressure = bmp.readPressure() / 100.0F;
        block.temperature = bmp.readTemperature();
        block.altitude = bmp.readAltitude(1013.25);
        block.timestamp_ms = millis();
        
        //if (block.count == config::SAMPLE_BLOCK_SIZE) {
        //    xQueueSend(raw_queue, &block, portMAX_DELAY);
        //    block.count = 0U;
        //}

        Serial.printf("BMP280: Temp=%.2f C, Pressure=%.2f hPa, Altitude=%.2f m\n",
                      block.temperature,
                      block.pressure,
                      block.altitude);
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(1000)); // Delay for 1 second
    }
}

void MPU6050_task(void* argument) {
    TickType_t last_wake = xTaskGetTickCount();
    uint32_t sample_index = 0;

    TwoWire* Mywire = static_cast<TwoWire*>(argument);
    Mywire->beginTransmission(0x68);
    Mywire->write(0x6B); 
    Mywire->write(0);    
    Mywire->endTransmission();
    delay(100); 

    mpu.begin(0x68);

    //if (!mpu.begin(0x68)) 
    //{
    //    Serial.println(F("Could not find a valid MPU6050 sensor, check wiring or address!"));
    //    vTaskDelete(nullptr);
    //}

    while (1) {
        mpu.getEvent(&mpu_record_p[sample_index].acc, &mpu_record_p[sample_index].gyro, &mpu_record_p[sample_index].temp);
        sample_index = (sample_index + 1);

        //if (block.count == config::SAMPLE_BLOCK_SIZE) {
        //    xQueueSend(raw_queue, &block, portMAX_DELAY);
        //    block.count = 0U;
        //}

        Serial.printf("MPU6050: Acc: (%.2f, %.2f, %.2f), Gyro: (%.2f, %.2f, %.2f), Temp: %.2f C\n",
                      block.acc.acceleration.x, block.acc.acceleration.y, block.acc.acceleration.z,
                      block.gyro.gyro.x, block.gyro.gyro.y, block.gyro.gyro.z,
                      block.temp.temperature);
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(1000)); // Delay for 1 second
    }
}

void dsp_task(void*) {
    SampleBlock block{};
    for (;;) {
        if (xQueueReceive(raw_queue, &block, portMAX_DELAY) == pdTRUE) {
            TelemetryPacket packet{};
            packet.features = process_samples(block.samples.data(), block.count);
            packet.state = detect_anomaly(packet.features);
            packet.timestamp_ms = block.samples[block.count - 1U].timestamp_ms;
            Serial.printf("DSP: RMS=%.3f VAR=%.3f PEAK=%.3f STATE=%u\n",
                          packet.features.rms, packet.features.variance,
                          packet.features.peak, static_cast<unsigned>(packet.state));
            xQueueSend(telemetry_queue, &packet, portMAX_DELAY);
        }
    }
}

void lora_task(void* argument) {
    LoRaManager* lora = (LoRaManager*)argument;
    TelemetryPacket packet{};

    int16_t state = lora->begin();
    Serial.printf("LoRa: %s (state=%d)\n", state == RADIOLIB_ERR_NONE ? "ready" : "error", state);

    for (;;) {
        if (xQueueReceive(telemetry_queue, &packet, portMAX_DELAY) == pdTRUE) {
            state = lora->send(packet);
            //Serial.printf("LoRa TX: %s (%d)\n", state == RADIOLIB_ERR_NONE ? payload : "error", state);
        }
    }
}
}

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("ESP-LoRa-TinyML / BME280 Test");

    // LoRa Manager initialization
    //static LoRaManager lora;

    // Sensor Manager initialization
    //sensor_manager::begin();
    


    
    //raw_queue = xQueueCreateStatic(config::RAW_QUEUE_LENGTH, sizeof(SampleBlock),
    //                               raw_queue_memory, &raw_queue_storage);
    //telemetry_queue = xQueueCreateStatic(config::TELEMETRY_QUEUE_LENGTH, sizeof(TelemetryPacket),
    //                                     telemetry_queue_memory, &telemetry_queue_storage);
    Wire.begin(21,22);

    xTaskCreate(BME280_task, "BME280Task", 4096, nullptr, 5, nullptr);
    xTaskCreate(MPU6050_task, "MPU6050Task", 8192, (void*) &Wire, 4, nullptr);
    //xTaskCreate(dsp_task, "DSPTask", 4096, nullptr, 4, nullptr);
    //xTaskCreate(lora_task, "LoRaTask", 4096, &lora, 3, nullptr);
    //Serial.println("Tasks and queues created");
}

void loop() {
    vTaskDelay(portMAX_DELAY);
}
