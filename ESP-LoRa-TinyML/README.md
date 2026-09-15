# ESP-LoRa-TinyML — versão final

Nó embarcado para a Heltec WiFi LoRa 32 V2, desenvolvido em C++ com Arduino-ESP32, FreeRTOS, PlatformIO, RadioLib e Edge Impulse. O firmware coleta dados do BME280 e do MPU6050, aplica filtragem FIR, classifica o movimento e transmite um quadro binário por LoRa.

## Fluxo

```text
BME280 + MPU6050 → FreeRTOS Tasks → Magnitude → FIR → Edge Impulse → TelemetryFrame + CRC16 → RadioLib → LoRa
```

## Recursos

- Temperatura, pressão e altitude pelo BME280.
- Aceleração e giroscópio pelo MPU6050.
- Fila FreeRTOS para amostras do IMU.
- Magnitude da aceleração e filtro FIR de 31 coeficientes.
- Inferência Edge Impulse para `idle`, `incident` e `run`.
- Quadro binário com timestamp, sensores, evento e CRC16.
- Transmissão LoRa usando RadioLib e SX1276.

## Estrutura

- `src/main.cpp`: sensores, tasks, DSP, inferência, empacotamento e LoRa.
- `TinyML/`: biblioteca compilada do modelo Edge Impulse.
- `platformio.ini`: placa, framework e dependências.

## Dependências e compilação

As dependências são instaladas pelo PlatformIO: Arduino-ESP32/FreeRTOS, RadioLib `7.7.1`, Adafruit BME280 `2.3.0`, Adafruit MPU6050 `2.2.9` e o modelo Edge Impulse local em `TinyML/`. Unified Sensor e BusIO são dependências transitivas.

```bash
pio run
pio run -t upload
pio device monitor -b 115200
```

## Pinagem

I²C: SDA GPIO21, SCL GPIO22.

LoRa Heltec V2: MOSI GPIO27, MISO GPIO19, SCK GPIO5, CS GPIO18, DIO0 GPIO26, DIO1 GPIO35 e RESET GPIO14. Confirme os pinos na revisão exata da placa e conecte a antena antes de transmitir.

## Materiais do portfólio

- `VID_20260907_194731.mp4`
- `VID_20260907_194851.mp4`
- `Relatýrio_Projeto_Final___Sistemas_Embarcados_Instrumentaýýo_e_Conectividade.pdf`
- `ESP-LoRa-TinyML-final.zip`

## Observações

O firmware atual usa uma implementação concentrada em `src/main.cpp` para facilitar a reprodução do protótipo. O modelo Edge Impulse é uma dependência local e o `TelemetryFrame` usa escala inteira para reduzir o payload. A integração física deve ser validada com a revisão da placa, alimentação e frequência LoRa regulamentar.
