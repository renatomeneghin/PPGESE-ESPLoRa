# ESP-Sensors

Projeto mínimo em C++ para ESP-IDF e ESP32 clássico, preparado para aquisição analógica/digital e futura comunicação por LoRa, Wi-Fi ou BLE.

## Estrutura

- `main/data/SensorData.hpp`: pacote de dados sem heap.
- `main/sensors/`: classes pequenas para ADC/GPIO e funções para DHT11/DHT22.
- `main/sensors/DhtSensor.*`: funções `dht_sensor::begin()` e `dht_sensor::read()`.
- `main/sensors/AnalogTemperatureSensor.*`: funções para LM35 ou TMP36 usando ADC.
- `main/communication/`: interface `Communication` e implementação inicial por log serial.
- `main/data/SensorPacket.hpp`: protocolo binário de 30 bytes compartilhável com o receptor.
- `main/main.cpp`: inicialização e uma única task periódica de aquisição.

## Build, flash e monitor

Com o ambiente ESP-IDF carregado, a partir desta pasta:

```bash
idf.py set-target esp32
idf.py build
idf.py -p PORT flash
idf.py -p PORT monitor
```

Substitua `PORT` por `COMx` no Windows ou `/dev/ttyUSBx` no Linux. Também é possível usar `idf.py flash monitor`.

## Sensores simulados e pinos

`main/config.hpp` mantém `USE_SIMULATED_SENSORS` igual a `1` por padrão. Nesse modo, o ADC retorna uma sequência variável e o GPIO alterna entre 0 e 1, permitindo testar sem sensores conectados. Altere para `0` para usar hardware real.

Para selecionar o sensor de temperatura, altere `USE_DHT_SENSOR` em `main/config.hpp`: `1` usa o DHT22 no GPIO4; `0` usa o sensor analógico no ADC1_CH6/GPIO34. Para LM35, mantenha `AnalogTemperatureType::Lm35`; para TMP36, use `AnalogTemperatureType::Tmp36` em `main/main.cpp`.

O LM35 fornece aproximadamente 10 mV/°C e deve ter sua saída ligada ao GPIO34. O TMP36 fornece aproximadamente 500 mV a 0 °C e também pode ser ligado ao GPIO34. A conversão implementada é inicial e usa referência aproximada de 3,3 V/4095; para maior precisão, faça calibração com um multímetro ou termômetro de referência.

Os pinos ficam no início de `main/main.cpp`: `ADC_CHANNEL_6` corresponde ao GPIO34, o digital usa GPIO35 e o DHT22 usa GPIO4. A leitura é feita por funções, sem instanciar uma classe DHT. O DHT precisa de resistor externo de pull-up, normalmente 4,7 kΩ entre DATA e 3,3 V. Confirme o mapeamento elétrico da sua revisão da Heltec WiFi LoRa 32 V2 antes de conectar sinais; GPIO34/35 são somente entrada e não possuem pull-up/pull-down interno.

O DHT22 deve ser consultado aproximadamente a cada 2 segundos. Para manter o período de aquisição em 1 segundo, use um DHT11 ou retenha o último valor válido entre leituras.

## Expansão futura

Uma futura `LoRaCommunication : public Communication` poderá substituir `SerialCommunication` sem alterar sensores ou a task. A RadioLib pode ser adicionada posteriormente como componente ESP-IDF e encapsulada nessa classe. O mesmo vale para implementações Wi-Fi e BLE.

Atualmente `SerialCommunication` usa UART1 (TX GPIO17, RX GPIO16, 115200 8N1) para enviar o pacote binário ao segundo ESP e também registra os valores no log.
