#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Adafruit_MPU6050.h>
#include <RadioLib.h>
#include <SPI.h>

// Heltec V2 Hardwired SPI Pins
#define LORA_MOSI 27
#define LORA_MISO 19
#define LORA_SCK  5

// Heltec V2 Transceiver Control Pins
#define LORA_CS   18
#define LORA_DIO0 26
#define LORA_RST  14
#define LORA_DIO1 35

// Onboard Power Management Pin
#define PIN_VEXT  21

// Define ESP32 I2C pins
#define I2C_SDA 21
#define I2C_SCL 22

namespace {
//QueueHandle_t raw_queue = nullptr;
//QueueHandle_t telemetry_queue = nullptr;
//StaticQueue_t raw_queue_storage;
//StaticQueue_t telemetry_queue_storage;
//uint8_t raw_queue_memory[config::RAW_QUEUE_LENGTH * sizeof(SampleBlock)];
//uint8_t telemetry_queue_memory[config::TELEMETRY_QUEUE_LENGTH * sizeof(TelemetryPacket)];


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

//MPU6050Data mpu_block[1024]; // Buffer to hold MPU6050 data
MPU6050Data *mpu_block; // Buffer to hold MPU6050 data

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

    MPU6050Data mpu_sample; // Buffer to hold MPU6050 data

    while (1) {
        //mpu.getEvent(&mpu_block[sample_index].acc, &mpu_block[sample_index].gyro, &mpu_block[sample_index].temp);
        //sample_index = (sample_index + 1);

        //if (block.count == config::SAMPLE_BLOCK_SIZE) {
        //    xQueueSend(raw_queue, &block, portMAX_DELAY);
        //    block.count = 0U;
        //}

        mpu.getEvent(&mpu_sample.acc, &mpu_sample.gyro, &mpu_sample.temp);

        Serial.printf("MPU6050: Acc: (%.2f, %.2f, %.2f), Gyro: (%.2f, %.2f, %.2f), Temp: %.2f C\n",
                      mpu_sample.acc.acceleration.x, mpu_sample.acc.acceleration.y, mpu_sample.acc.acceleration.z,
                      mpu_sample.gyro.gyro.x, mpu_sample.gyro.gyro.y, mpu_sample.gyro.gyro.z,
                      mpu_sample.temp.temperature);
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(1000)); // Delay for 1 second
    }
}

//void dsp_task(void*) {
//    SampleBlock block{};
//    for (;;) {
//        if (xQueueReceive(raw_queue, &block, portMAX_DELAY) == pdTRUE) {
//            TelemetryPacket packet{};
//            packet.features = process_samples(block.samples.data(), block.count);
//            packet.state = detect_anomaly(packet.features);
//            packet.timestamp_ms = block.samples[block.count - 1U].timestamp_ms;
//            Serial.printf("DSP: RMS=%.3f VAR=%.3f PEAK=%.3f STATE=%u\n",
//                          packet.features.rms, packet.features.variance,
//                          packet.features.peak, static_cast<unsigned>(packet.state));
//            xQueueSend(telemetry_queue, &packet, portMAX_DELAY);
//        }
//    }
//}

void lora_task(void* argument) {
    // Create custom SPI instance using ESP32's VSPI hardware block
    SPIClass customSPI(VSPI);

    // Create the SX1276 radio instance
    SX1276 radio = new Module(LORA_CS, LORA_DIO0, LORA_RST, LORA_DIO1, customSPI);

    // 1. Enable Vext power supply (Crucial for Heltec boards to power internal peripherals)
    pinMode(PIN_VEXT, OUTPUT);
    digitalWrite(PIN_VEXT, LOW); // LOW turns on Vext power
    delay(100); // Give hardware a moment to stabilize
    
    customSPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS); // Initialize SPI with custom pins

    Serial.println("Initializing LoRa radio...");

    int state = radio.begin(915.0, 125.0, 7, 5, 0x12, 17);

    if (state == RADIOLIB_ERR_NONE) {
        Serial.println("LoRa radio initialized successfully!");
    } else {
        Serial.printf("Failed to initialize LoRa radio, error code: %d\n", state);
        vTaskDelete(nullptr); // Terminate the task if initialization fails
    }

    while (1) {
        Serial.printf("Transmitting LoRa packet...\n");

        String payload = "Hello, LoRa!";

        int state = radio.transmit(payload);

        if (state == RADIOLIB_ERR_NONE) {
            Serial.printf("LoRa TX: %s\n", payload.c_str());
        } else {
            Serial.printf("LoRa TX failed, error code: %d\n", state);
        }


        vTaskDelay(pdMS_TO_TICKS(5000)); // Delay for 5 seconds
    }
}
}

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("ESP-LoRa-TinyML / BME280 Test");
    
    //raw_queue = xQueueCreateStatic(config::RAW_QUEUE_LENGTH, sizeof(SampleBlock),
    //                               raw_queue_memory, &raw_queue_storage);
    //telemetry_queue = xQueueCreateStatic(config::TELEMETRY_QUEUE_LENGTH, sizeof(TelemetryPacket),
    //                                     telemetry_queue_memory, &telemetry_queue_storage);
    Wire.begin(21,22);

    mpu_block = new MPU6050Data[1024]; // Allocate memory for the buffer
        if (mpu_block == nullptr) {
        Serial.println("FALHA NA ALOCACAO");
        return;
    }

    Serial.printf(
        "Heap depois: %u bytes\n",
        ESP.getFreeHeap()
    );

    xTaskCreate(BME280_task, "BME280Task", 4096, nullptr, 5, nullptr);
    xTaskCreate(MPU6050_task, "MPU6050Task", 8192, (void*) &Wire, 4, nullptr);
    //xTaskCreate(dsp_task, "DSPTask", 4096, nullptr, 4, nullptr);
    xTaskCreate(lora_task, "LoRaTask", 4096, nullptr, 3, nullptr);
    //Serial.println("Tasks and queues created");
}

void loop() {
    vTaskDelay(portMAX_DELAY);
}
