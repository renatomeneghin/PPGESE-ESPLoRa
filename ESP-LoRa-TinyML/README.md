# ESP-LoRa-TinyML (BMP280)

Nó embarcado para Heltec WiFi LoRa 32 V2 usando VS Code, PlatformIO, Arduino-ESP32, FreeRTOS, Adafruit BMP280, ESP-DSP e RadioLib. O sensor principal é o BMP280, lido por I²C. A arquitetura usa somente `LoRaManager` e a classe opcional `Mpu6050Sensor`; sensor principal, DSP e detecção são simples.

## Fluxo

```text
BMP280
  ↓
SensorTask
  ↓
Raw Queue
  ↓
DSPTask
  ↓
Feature Extraction
  ↓
Anomaly Detection
  ↓
Telemetry Queue
  ↓
LoRaTask
  ↓
RadioLib → SX1276 → LoRa
```

`SensorTask` coleta uma amostra a cada 20 ms e envia blocos de 32 amostras. `DSPTask` calcula média, RMS, variância e pico da pressão; o RMS usa `dsps_dotprod_f32_ansi()` da biblioteca oficial ESP-DSP. `AnomalyDetector` aplica uma árvore de decisão pequena. `LoRaTask` transmite texto legível.

## Estrutura

- `include/Config.hpp`: I²C, pinos LoRa, parâmetros de rádio, período e tamanho dos blocos.
- `include/DataTypes.hpp`: amostras de pressão/temperatura, blocos, features e telemetria sem heap.
- `include/SensorManager.hpp` e `src/SensorManager.cpp`: funções de leitura usando Adafruit BMP280.
- `include/Mpu6050Sensor.hpp` e `src/Mpu6050Sensor.cpp`: classe opcional, compilada logicamente quando `USE_MPU6050=1`.
- `include/DspProcessor.hpp` e `src/DspProcessor.cpp`: função DSP usando ESP-DSP; FIR, IIR e FFT ficam como extensão futura.
- `include/AnomalyDetector.hpp` e `src/AnomalyDetector.cpp`: função de decisão por thresholds `constexpr`, substituível futuramente por TinyML.
- `include/LoRaManager.hpp` e `src/LoRaManager.cpp`: única classe própria, encapsulando RadioLib e transmissão.
- `src/main.cpp`: queues, tasks e composição dos módulos.

## Instalação e compilação

Instale VS Code e a extensão PlatformIO. Abra esta pasta no VS Code; RadioLib, Adafruit BMP280, Adafruit MPU6050 e ESP-DSP são baixadas automaticamente por `lib_deps` no `platformio.ini`. As bibliotecas Adafruit Unified Sensor, Adafruit BusIO, Adafruit GFX e Adafruit SSD1306 são dependências transitivas e são resolvidas automaticamente pelo PlatformIO.

```bash
pio run
pio run -t upload
pio device monitor -b 115200
```

Também é possível usar os botões Build, Upload e Monitor do PlatformIO no VS Code.

## Ligações do BMP280

```text
BMP280 VCC → 3V3
BMP280 GND → GND
BMP280 SDA → GPIO21
BMP280 SCL → GPIO22
```

O endereço padrão é `0x76`; alguns módulos usam `0x77` com SDO em nível alto. Altere `BMP280_I2C_ADDRESS` em `Config.hpp` se necessário. O módulo deve operar em 3,3 V e ter alimentação/níveis lógicos compatíveis.

## LoRa e pinagem

Os valores padrão mais comuns para a Heltec V2 são CS/NSS=18, DIO0=26, RESET=14, DIO1=33, SCK=5, MISO=19 e MOSI=27. A revisão da placa e a versão da biblioteca podem exigir confirmação; todos os pinos estão centralizados em `Config.hpp`. A frequência padrão é 915 MHz: ajuste `LORA_FREQUENCY_MHZ` conforme a regulamentação e a região.

Use antena adequada antes de transmitir. A potência inicial é moderada, 10 dBm; não aumente nem opere o rádio sem antena.

## Simulação

Para testar sem o BMP280, altere em `Config.hpp`:

```cpp
constexpr bool USE_SIMULATED_SENSOR = true;
```

Nesse modo são geradas amostras sintéticas de pressão e nenhuma leitura I²C real é feita.

## MPU6050 opcional

Por padrão `USE_MPU6050` é `0`, portanto o BMP280 é usado em SDA/SCL GPIO21/22. Para compilar o caminho alternativo com a classe `Mpu6050Sensor`, altere para `1` em `Config.hpp`; nesse modo o MPU6050 usa SDA/SCL GPIO4/15 e o BMP280 não é inicializado. O sinal DSP passa a ser a magnitude da aceleração do MPU6050; a aquisição, queues e LoRa permanecem iguais.

`USE_ESP_DSP` é `1` por padrão. Se o pacote ESP-DSP não integrar corretamente ao Arduino/PlatformIO, defina-o como `0`; o DSP usará o cálculo local de soma de quadrados, mantendo média, RMS, variância e pico disponíveis.

## Payload e evolução

O payload atual é texto, por exemplo:

```text
RMS=9.812,VAR=0.021,PEAK=9.945,STATE=0,T=12345
```

Um payload binário poderá reduzir o tamanho posteriormente. O ponto de extensão de DSP está em `DspProcessor.cpp`; TinyML pode substituir `AnomalyDetector` usando TensorFlow Lite Micro ou Edge Impulse, sem alterar as tasks. BLE, Wi-Fi ou LoRaWAN também podem ser adicionados em módulos de comunicação futuros.

## Riscos e compatibilidade

- O build depende da versão instalada do PlatformIO/espressif32, do Arduino-ESP32, das bibliotecas Adafruit, da API da RadioLib e da integração PlatformIO da ESP-DSP.
- As versões das dependências principais estão fixadas no `platformio.ini` para evitar alterações silenciosas da API entre builds.
- ESP-DSP é originalmente um componente ESP-IDF; a dependência Git é mantida para uso no Arduino/PlatformIO, mas pode exigir ajuste de include paths conforme a versão do PlatformIO.
- A pinagem LoRa deve ser confirmada para a revisão exata da Heltec V2.
- O BMP280 fornece pressão e temperatura; o caminho opcional do MPU6050 usa faixa de ±2 g e ±250 dps, sem calibração avançada.
- Este projeto não afirma compilação validada nesta máquina quando o PlatformIO não estiver instalado.
