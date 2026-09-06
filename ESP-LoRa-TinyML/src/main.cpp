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
#include <car-state_inferencing.h>
#include <math.h>

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

// NN parameters
#define NN_SAMPLE_COUNT EI_CLASSIFIER_RAW_SAMPLE_COUNT
#define NN_AXES EI_CLASSIFIER_RAW_SAMPLES_PER_FRAME
#define NN_FEATURE_COUNT (NN_SAMPLE_COUNT * NN_AXES)

static float nn_buffer[NN_FEATURE_COUNT];
static size_t nn_index = 0;


Adafruit_BME280 bmp; 
Adafruit_MPU6050 mpu;

struct BME280Data {
    float temperature;
    float pressure;
    float altitude;
};

struct MPU6050Data
{
    sensors_event_t acc;
    sensors_event_t gyro;
    sensors_event_t temp;
};

struct IMUSample
{
    float ax;
    float ay;
    float az;

    float gx;
    float gy;
    float gz;

    float temperature;

    uint32_t timestamp_ms;
};

struct TelemetryPacket {
    uint32_t timestamp_ms;

    float acceleration;
    float acceleration_filtered;

    BME280Data bme_data;

    uint8_t event;
} Packet;

struct TelemetryFrame
{
    uint16_t sync;
    uint8_t type;

    uint32_t timestamp_ms;

    int16_t temperature;
    uint32_t pressure;
    uint16_t altitude;

    int16_t acceleration;
    int16_t acceleration_filtered;

    uint8_t event;

    uint16_t crc;
};

QueueHandle_t imu_queue;
SemaphoreHandle_t Packet_mutex;

/* Constant defines -------------------------------------------------------- */
#define CONVERT_G_TO_MS2    9.80665f
#define MAX_ACCEPTED_RANGE  2.0f        // starting 03/2022, models are generated setting range to +-2,
                                        // but this example use Arudino library which set range to +-4g.
                                        // If you are using an older model, ignore this value and use 4.0f instead
/** Number sensor axes used */
#define N_SENSORS     7

/* Private variables ------------------------------------------------------- */
static const bool debug_nn = false; // Set this to true to see e.g. features generated from the raw signal
static float data[N_SENSORS];
static int8_t fusion_sensors[N_SENSORS];
static int fusion_ix = 0;

/*
FIR filter designed with
http://t-filter.appspot.com

sampling frequency: 1000 Hz
* 0 Hz - 100 Hz
  gain = 1
  desired ripple = 5 dB
  actual ripple = 2.3133909166191504 dB
* 200 Hz - 500 Hz
  gain = 0
  desired attenuation = -40 dB
  actual attenuation = -45.01618674525617 dB
*/

#define FILTER_TAP_NUM 15

static double filter_taps[FILTER_TAP_NUM] = {
  -0.01259277478717816,
  -0.02704833486706803,
  -0.031157016036431583,
  -0.003351666747179282,
  0.06651710329324828,
  0.1635643048779222,
  0.249729473226146,
  0.2842779082622769,
  0.249729473226146,
  0.1635643048779222,
  0.06651710329324828,
  -0.003351666747179282,
  -0.031157016036431583,
  -0.02704833486706803,
  -0.01259277478717816
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

        // Atualiza os dados compartilhados
        if (xSemaphoreTake(Packet_mutex, pdMS_TO_TICKS(10)) == pdTRUE)
        {
            Packet.bme_data = block;
            xSemaphoreGive(Packet_mutex);
        }

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
    vTaskDelay(pdMS_TO_TICKS(100)); 

    mpu.begin(0x68);

    MPU6050Data mpu_sample; // Buffer to hold MPU6050 data

    while (1) {
        mpu.getEvent(&mpu_sample.acc, &mpu_sample.gyro, &mpu_sample.temp);

        IMUSample imu_sample;
        imu_sample.ax = mpu_sample.acc.acceleration.x;
        imu_sample.ay = mpu_sample.acc.acceleration.y;
        imu_sample.az = mpu_sample.acc.acceleration.z;

        imu_sample.gx = mpu_sample.gyro.gyro.x;
        imu_sample.gy = mpu_sample.gyro.gyro.y;
        imu_sample.gz = mpu_sample.gyro.gyro.z;

        imu_sample.temperature = mpu_sample.temp.temperature;
        
        imu_sample.timestamp_ms = millis();
        
        xQueueSend(imu_queue, 
            &imu_sample, 
            portMAX_DELAY);

        //Serial.printf("MPU6050: ax=%.2f, ay=%.2f, az=%.2f | gx=%.2f, gy=%.2f, gz=%.2f | Temp=%.2f C\n",
        //              imu_sample.ax,
        //              imu_sample.ay,
        //              imu_sample.az,
        //              imu_sample.gx,
        //              imu_sample.gy,
        //              imu_sample.gz,
        //              imu_sample.temperature);

        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(1)); // Delay for 1 second
    }
}

float fir_filter(float input)
{
    static float fir_buffer[FILTER_TAP_NUM] = {0.0};
    static uint8_t fir_index = 0;
    
    fir_buffer[fir_index] = input;
    float output = 0.0f;

    for (int i = 0, index = fir_index; i < FILTER_TAP_NUM; i++)
    {
        output += filter_taps[i] * fir_buffer[index];

        index--;

        if (index < 0)
        {
            index = FILTER_TAP_NUM - 1;
        }
    }

    fir_index++;
    fir_index %= FILTER_TAP_NUM; // Wrap around the index to stay within the buffer size

    return output;
}

int get_signal_data(size_t offset, size_t length, float *out_ptr) // Edge Impulse SDK function to get signal data for inference
{
    for (size_t i = 0; i < length; i++)
    {
        out_ptr[i] = nn_buffer[offset + i];
    }

    return 0;
}

void DSP_task(void* argument)
{
    IMUSample sample;

    while (1)
    {
        if (xQueueReceive(
                imu_queue,
                &sample,
                portMAX_DELAY) == pdTRUE)
        {
            // Acceleration magnitude calculation
            float acceleration =
                sqrtf(
                    sample.ax * sample.ax +
                    sample.ay * sample.ay +
                    sample.az * sample.az
                );

            // Apply FIR filter to the acceleration magnitude
            float filtered_acceleration =
                fir_filter(acceleration);
            
            
            // Atualiza os dados compartilhados
            if (xSemaphoreTake(Packet_mutex, pdMS_TO_TICKS(10)) == pdTRUE)
            {
                Packet.acceleration = acceleration;
                Packet.acceleration_filtered = filtered_acceleration;
                xSemaphoreGive(Packet_mutex);
            }

            nn_buffer[nn_index++] = sample.ax;
            nn_buffer[nn_index++] = sample.ay;
            nn_buffer[nn_index++] = sample.az;

            if (nn_index >= NN_FEATURE_COUNT)
            {
                signal_t signal;
                signal.total_length = NN_FEATURE_COUNT;

                signal.get_data = &get_signal_data;

                ei_impulse_result_t result = { 0 };
                EI_IMPULSE_ERROR res = run_classifier(&signal, &result, false);

                if(res == EI_IMPULSE_OK)
                {
                    Serial.printf("Predictions: ");
                    // Atualiza os dados compartilhados
                    if (xSemaphoreTake(Packet_mutex, pdMS_TO_TICKS(10)) == pdTRUE)
                    {
                        if(strcmp(result.classification[0].label, "idle") == 0)
                        {
                            Packet.event = 1; // Idle
                            Serial.printf("Idle detected\n");
                        }
                        else if(strcmp(result.classification[0].label, "incident") == 0)
                        {
                            Packet.event = 2; // Incident
                            Serial.printf("Incident detected\n");
                        }
                        else if(strcmp(result.classification[0].label, "run") == 0)
                        {
                            Packet.event = 3; // Run
                            Serial.printf("Run detected\n");
                        }
                        else
                        {
                            Packet.event = 0; // Unknown
                            Serial.printf("Unknown event detected: %s\n", result.classification[0].label);
                        }
                                                
                        xSemaphoreGive(Packet_mutex);
                    }
                }
                else
                {
                    Serial.printf("Error: Failed to run classifier (%d)\n", res);
                    Packet.event = 0; // Set to unknown state on error
                }

                nn_index = 0; // Reset index for next batch of samples
            }

            //Serial.printf(
            //    "A=%.3f g | Af=%.3f\n",
            //    acceleration / 9.80665f,
            //    filtered_acceleration / 9.80665f
            //);
        }
    }
}

uint16_t calculateCRC16(const uint8_t* data, size_t length)
{
    uint16_t crc = 0xFFFF;

    for (size_t i = 0; i < length; i++)
    {
        crc ^= static_cast<uint16_t>(data[i]) << 8;

        for (uint8_t bit = 0; bit < 8; bit++)
        {
            if (crc & 0x8000)
            {
                crc = (crc << 1) ^ 0x1021;
            }
            else
            {
                crc <<= 1;
            }
        }
    }

    return crc;
}

void lora_task(void* argument) {
    // Create a telemetry packet instance
    TelemetryPacket packet{};
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
        TelemetryFrame frame{};

        frame.sync = 0xAA55;
        frame.type = 0x01;
        frame.timestamp_ms = millis();
        
        if (xSemaphoreTake(Packet_mutex, pdMS_TO_TICKS(10)) == pdTRUE)
        {
            frame.temperature = (int16_t)(Packet.bme_data.temperature * 100.0f);
            frame.pressure = (uint32_t)(Packet.bme_data.pressure * 100.0f);
            frame.altitude = (uint16_t)(Packet.bme_data.altitude * 100.0f);
            frame.acceleration = (int16_t)(Packet.acceleration * 100.0f);
            frame.acceleration_filtered = (int16_t)(Packet.acceleration_filtered * 100.0f);
            frame.event = Packet.event;

            xSemaphoreGive(Packet_mutex);
        }

        //frame.event = data.event;

        // CRC será calculado aqui
        frame.crc = calculateCRC16(
            reinterpret_cast<uint8_t*>(&frame),
            sizeof(frame) - sizeof(frame.crc)
        );

        Serial.printf("Transmitting LoRa packet...\n");
        Serial.printf(
            "TX: T=%d C, P=%d hPa, A=%d g, Af=%d g\n",
            frame.temperature,
            frame.pressure,
            frame.acceleration,
            frame.acceleration_filtered
        );
        
        int state = radio.transmit(
            (uint8_t*)&frame,
            sizeof(frame)
        );

        if (state == RADIOLIB_ERR_NONE)
        {
            Serial.printf(
                "LoRa TX successful (%d bytes)\n",
                sizeof(frame)
            );
        }
        else
        {
            Serial.printf(
                "LoRa TX failed, error code: %d\n",
                state
            );
        }

        vTaskDelay(pdMS_TO_TICKS(1000)); // Delay for 1 second
    }
}


void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("ESP-LoRa-TinyML / BME280 Test");

    Wire.begin(21,22);

    Packet_mutex = xSemaphoreCreateMutex();
    imu_queue = xQueueCreate(32, sizeof(IMUSample));

    xTaskCreate(BME280_task, "BME280Task", 4096, nullptr, 5, nullptr);
    xTaskCreate(MPU6050_task, "MPU6050Task", 8192, (void*) &Wire, 4, nullptr);
    xTaskCreate(DSP_task, "DSPTask", 8192, nullptr, 4, nullptr);
    xTaskCreate(lora_task, "LoRaTask", 4096, nullptr, 3, nullptr);
    //Serial.println("Tasks and queues created");
}

void loop() {
    vTaskDelay(portMAX_DELAY);
}
