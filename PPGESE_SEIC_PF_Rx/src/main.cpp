#include <Arduino.h>
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

#define FRAME_TYPE_TELEMETRY  0x01

#define EVENT_NORMAL           0x01
#define EVENT_IMPACT           0x02
#define EVENT_MOVEMENT         0x03
#define EVENT_UNKNOWN          0x00

struct BME280Data {
    float temperature;
    float pressure;
    float altitude;
};

struct TelemetryPacket {
    uint32_t timestamp_ms;

    float acceleration;
    float acceleration_filtered;

    BME280Data bme_data;

    uint8_t event;
} Packet;

#pragma pack(push, 1)
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
#pragma pack(pop)

// Create custom SPI instance
SPIClass customSPI(VSPI);

// Create the SX1276 radio instance
SX1276 radio = new Module(LORA_CS, LORA_DIO0, LORA_RST, LORA_DIO1, customSPI);

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

void processTelemetryFrame(const TelemetryFrame& frame)
{
    // ------------------------------------------
    // 1. Verifica SYNC
    // ------------------------------------------

    if (frame.sync != 0xAA55)
    {
        Serial.println("Invalid SYNC");
        return;
    }


    // ------------------------------------------
    // 2. Verifica CRC
    // ------------------------------------------

    uint16_t calculated_crc =
        calculateCRC16(
            reinterpret_cast<const uint8_t*>(&frame),
            offsetof(TelemetryFrame, crc)
        );

    if (calculated_crc != frame.crc)
    {
        Serial.printf(
            "CRC ERROR! Received: 0x%04X | Calculated: 0x%04X\n",
            frame.crc,
            calculated_crc
        );

        return;
    }


    // ------------------------------------------
    // 3. Decodifica valores
    // ------------------------------------------

    float temperature =
        frame.temperature / 100.0f;

    float pressure =
        frame.pressure / 100.0f;

    float altitude =
        frame.altitude / 100.0f;

    float acceleration =
        frame.acceleration / 100.0f;

    float acceleration_filtered =
        frame.acceleration_filtered / 100.0f;


    // ------------------------------------------
    // 4. Identifica evento
    // ------------------------------------------

    const char* event_name;

    switch (frame.event)
    {
        case EVENT_NORMAL:
            event_name = "NORMAL";
            break;

        case EVENT_IMPACT:
            event_name = "IMPACT";
            break;

        case EVENT_MOVEMENT:
            event_name = "MOVEMENT";
            break;

        default:
            event_name = "UNKNOWN";
            break;
    }


    // ------------------------------------------
    // 5. Mostra telemetria
    // ------------------------------------------

    Serial.println();
    Serial.println("========== TELEMETRY ==========");

    Serial.printf(
        "Timestamp : %lu ms\n",
        frame.timestamp_ms
    );

    Serial.printf(
        "Temperature: %.2f C\n",
        temperature
    );

    Serial.printf(
        "Pressure   : %.2f hPa\n",
        pressure
    );

    Serial.printf(
        "Altitude   : %.2f m\n",
        altitude
    );

    Serial.printf(
        "Accel      : %.2f g\n",
        acceleration
    );

    Serial.printf(
        "Accel FIR  : %.2f g\n",
        acceleration_filtered
    );

    Serial.printf(
        "Event      : %s (0x%02X)\n",
        event_name,
        frame.event
    );

    Serial.printf(
        "CRC        : 0x%04X [OK]\n",
        frame.crc
    );

    Serial.println(
        "==============================="
    );
}

void setup() {
  Serial.begin(115200);

  // 1. Enable Vext power supply
  pinMode(PIN_VEXT, OUTPUT);
  digitalWrite(PIN_VEXT, LOW); 
  delay(100); 

  // 2. Initialize the SPI bus with Heltec V2 specific routing
  customSPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);

  // 3. Initialize the radio module
  // IMPORTANT: This must match the transmitter frequency exactly (e.g., 915.0 or 868.0)
  Serial.print(F("[SX1276] Initializing ... "));
  int state = radio.begin(915.0, 125.0, 7, 5, 0x12, 17); 

  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true); 
  }
}

void loop() {
  Serial.print(F("[SX1276] Waiting for incoming packet ... "));

  TelemetryFrame frame;

  // receive() is a blocking call. It will wait here until a packet arrives.
  int state = radio.receive(
            reinterpret_cast<uint8_t*>(&frame),
            sizeof(frame)
        );
  if (state == RADIOLIB_ERR_NONE) {
    // Packet was successfully received
    Serial.println(F("success!"));

    // Print the received data payload
    Serial.print(F("[SX1276] Data:\t\t"));
    processTelemetryFrame(frame);
    // Print signal quality metrics (crucial for troubleshooting range issues)
    Serial.print(F("[SX1276] RSSI:\t\t"));
    Serial.print(radio.getRSSI());
    Serial.println(F(" dBm"));

    Serial.print(F("[SX1276] SNR:\t\t"));
    Serial.print(radio.getSNR());
    Serial.println(F(" dB"));

  } else if (state == RADIOLIB_ERR_RX_TIMEOUT) {
    // This won't trigger unless a timeout parameter is explicitly passed to receive()
    Serial.println(F("timeout!"));
    
  } else if (state == RADIOLIB_ERR_CRC_MISMATCH) {
    // The packet was received, but data was corrupted in transit
    Serial.println(F("CRC error!"));

  } else {
    // Any other error
    Serial.print(F("failed, code "));
    Serial.println(state);
  }
}

