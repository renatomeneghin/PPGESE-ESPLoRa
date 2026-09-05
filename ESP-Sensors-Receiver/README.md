# ESP-Sensors-Receiver

Segundo programa ESP-IDF em C++ para receber pacotes do ESP de aquisição e encaminhá-los ao computador pelo monitor serial da própria USB.

## Protocolo de dados

O arquivo `main/data/SensorPacket.hpp` define um pacote binário fixo de 30 bytes:

| Campo | Tamanho |
|---|---:|
| magic (`0x5345`) | 2 bytes |
| version | 1 byte |
| payload_size | 1 byte |
| sequence | 4 bytes |
| analog_value | 4 bytes |
| temperature_celsius | 4 bytes |
| humidity_percent | 4 bytes |
| digital_value | 1 byte |
| reservado | 3 bytes |
| timestamp_ms | 4 bytes |
| CRC-16/CCITT | 2 bytes |

O pacote usa a representação nativa little-endian do ESP32 e CRC-16/CCITT com polinômio `0x1021`, inicial `0xFFFF`. O transmissor deve zerar `crc16` antes de calcular o CRC sobre os primeiros 28 bytes.

## Ligações

Por padrão, o receptor usa UART1: RX GPIO16, TX GPIO17, 115200 8N1. Faça ligação cruzada:

```text
ESP transmissor TX -> ESP receptor RX (GPIO16)
ESP transmissor RX <- ESP receptor TX (GPIO17)  # opcional se só houver envio
GND                -> GND
```

Não conecte diretamente sinais de 5 V. A USB do receptor continua ligada ao computador para alimentação e monitor serial.

## Compilação e execução

Com o ambiente ESP-IDF carregado, a partir desta pasta:

```bash
idf.py set-target esp32
idf.py build
idf.py -p PORT flash
idf.py -p PORT monitor
```

O programa cria apenas uma task de recepção. Os dados válidos aparecem no monitor como `analog`, `digital`, `timestamp` e `sequence`; pacotes incompletos ou com CRC inválido são descartados.

## Classificação leve de temperatura

O receptor inclui `main/ml/TemperatureClassifier`, um modelo mínimo por centróides, sem dependências externas. Ele compara o valor recebido em `temperature_celsius` com três centróides iniciais:

- `frio`: 10 °C
- `morno`: 23 °C
- `quente`: 35 °C

O valor mais próximo é exibido no monitor. Esses centróides são uma inicialização de demonstração, não substituem treinamento com medições reais. Os centróides podem ser ajustados em `main/ml/TemperatureClassifier.cpp` ou substituídos por médias obtidas de um conjunto de dados rotulado.

## Integração com o primeiro projeto

O primeiro projeto envia o pacote DHT via `SerialCommunication`. O receptor valida o CRC, mostra temperatura/umidade e classifica a temperatura. O DHT deve ter DATA ligado ao GPIO4 do transmissor e resistor externo de pull-up de 4,7 kΩ para 3,3 V.
