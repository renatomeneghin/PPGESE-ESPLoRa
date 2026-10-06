# PPGESE-ESPLoRa

## Sistemas Embarcados, Instrumentação e Conectividade

Projeto desenvolvido no contexto da disciplina de **Sistemas Embarcados, Instrumentação e Conectividade**, com foco na integração de **aquisição de dados, processamento digital de sinais, TinyML, sistemas operacionais de tempo real e comunicação sem fio** em uma plataforma embarcada.

O protótipo utiliza uma **Heltec WiFi LoRa 32 V2**, baseada no ESP32, para realizar a aquisição de dados de sensores ambientais e inerciais, processamento local e classificação do estado de movimento. Os resultados são organizados em um quadro de telemetria binário e transmitidos por **LoRa**.

---

## Arquitetura

O fluxo principal do sistema é:

```text
                 ┌──────────────┐
                 │    BME280    │
                 │ Temperatura  │
                 │  Pressão     │
                 │  Altitude    │
                 └──────┬───────┘
                        │
                        │
┌──────────────┐        │
│   MPU6050    │        │
│ Acelerômetro │        │
│  Giroscópio  │        │
└──────┬───────┘        │
       │                │
       ▼                ▼
┌──────────────────────────────┐
│         FreeRTOS             │
│                              │
│  MPU6050 Task                │
│  BME280 Task                 │
│       │                      │
│       ▼                      │
│    IMU Queue                 │
│       │                      │
│       ▼                      │
│      DSP Task                │
│       │                      │
│       ├── Magnitude          │
│       ├── FIR                │
│       └── TinyML             │
│              │               │
│              ▼               │
│       Evento detectado       │
│                              │
│         LoRa Task            │
└──────────────┬───────────────┘
               │
               ▼
        ┌─────────────┐
        │  Telemetry  │
        │    Frame    │
        │    CRC16    │
        └──────┬──────┘
               │
               ▼
          ┌─────────┐
          │  LoRa   │
          │ SX1276  │
          └────┬────┘
               │
               ▼
        Sistema receptor
```

---

## Principais recursos

### Aquisição de dados

O sistema utiliza dois sensores conectados ao ESP32:

* **BME280**

  * Temperatura
  * Pressão
  * Altitude estimada

* **MPU6050**

  * Aceleração nos eixos X, Y e Z
  * Velocidade angular nos eixos X, Y e Z
  * Temperatura interna

Os sensores utilizam I²C, enquanto o transceptor LoRa utiliza SPI.

### Processamento digital

A aceleração é processada localmente no ESP32.

O firmware calcula a magnitude da aceleração:

```text
a = √(ax² + ay² + az²)
```

e aplica um **filtro FIR de 31 coeficientes**.

O filtro foi projetado considerando:

* Frequência de amostragem: 1000 Hz
* Frequência de corte: 150 Hz
* Banda de transição: 100 Hz
* Janela: Hamming

A implementação do FIR está presente diretamente no firmware para manter o protótipo simples e reproduzível.

### TinyML

O sistema utiliza um modelo desenvolvido com **Edge Impulse** para classificação do comportamento observado pelo IMU.

As classes utilizadas são:

```text
idle
incident
run
```

A inferência é executada localmente no ESP32, permitindo que a classificação seja realizada antes da transmissão dos dados.

### RTOS

A aplicação utiliza o **FreeRTOS integrado ao Arduino-ESP32**.

As principais tarefas são:

```text
BME280Task
MPU6050Task
DSPTask
LoRaTask
```

Uma fila FreeRTOS é utilizada para transportar as amostras do MPU6050 para a etapa de processamento.

Um mutex protege os dados compartilhados utilizados na montagem da telemetria.

---

## Telemetria

Os dados são organizados em um quadro binário compacto:

```text
┌────────┬──────┬───────────┬─────────────┬──────────┬──────────┐
│ Sync   │ Type │ Timestamp │ Sensor Data │ Event    │ CRC16    │
└────────┴──────┴───────────┴─────────────┴──────────┴──────────┘
```

O quadro contém:

* sincronização;
* tipo de mensagem;
* timestamp;
* temperatura;
* pressão;
* altitude;
* aceleração;
* aceleração filtrada;
* evento classificado;
* CRC16.

Os valores são convertidos para representação inteira antes da transmissão, reduzindo o tamanho do payload.

O CRC16 é calculado sobre o conteúdo do quadro antes do campo de CRC.

---

## Comunicação LoRa

A comunicação utiliza:

* **SX1276**
* **RadioLib**
* frequência configurada em 915 MHz
* largura de banda de 125 kHz
* spreading factor 7
* coding rate 4/5

A implementação utiliza a interface SPI da Heltec WiFi LoRa 32 V2.

> **Atenção:** a frequência de operação deve ser adequada à regulamentação aplicável à região onde o sistema for utilizado.

Também é necessário utilizar uma antena apropriada antes de realizar transmissões pelo rádio.

---

## Hardware

### Plataforma principal

**Heltec WiFi LoRa 32 V2**

Principais interfaces utilizadas:

| Interface | Função                |
| --------- | --------------------- |
| I²C       | BME280 + MPU6050      |
| SPI       | SX1276                |
| GPIO      | Controle do rádio     |
| UART      | Monitoramento e debug |

### Pinagem utilizada

#### I²C

```text
SDA → GPIO 21
SCL → GPIO 22
```

#### LoRa / SX1276

```text
MOSI → GPIO 27
MISO → GPIO 19
SCK  → GPIO 5
CS   → GPIO 18
DIO0 → GPIO 26
DIO1 → GPIO 35
RST  → GPIO 14
```

A pinagem deve ser confirmada de acordo com a revisão específica da placa utilizada.

---

## Software

O projeto principal foi desenvolvido utilizando:

* C++
* Arduino-ESP32
* FreeRTOS
* PlatformIO
* RadioLib
* Adafruit BME280 Library
* Adafruit MPU6050
* Edge Impulse

As versões das principais dependências estão fixadas no `platformio.ini` para facilitar a reprodução do projeto.

### Principais dependências

```text
RadioLib                 7.7.1
Adafruit BME280 Library  2.3.0
Adafruit MPU6050         2.2.9
Edge Impulse             modelo local
```

---

## Estrutura do repositório

```text
PPGESE-ESPLoRa/
│
├── ESP-LoRa-TinyML/
│   ├── src/
│   │   └── main.cpp
│   ├── TinyML/
│   │   └── modelo Edge Impulse
│   ├── platformio.ini
│   ├── README.md
│   └── PORTFOLIO.md
│
├── PPGESE_SEIC_PF_Rx/
│   ├── include/
│   ├── lib/
│   ├── src/
│   ├── test/
│   └── platformio.ini
│
├── Google-Sites-Upload/
│
├── ESP-LoRa-TinyML-final.zip
│
├── Relatório do projeto.pdf
│
├── VID_20260907_194731.mp4
│
└── VID_20260907_194851.mp4
```

---

## Compilação

O firmware principal pode ser compilado utilizando o **PlatformIO**.

Entre no diretório:

```bash
cd ESP-LoRa-TinyML
```

Compile:

```bash
pio run
```

Para realizar o upload:

```bash
pio run -t upload
```

Para acessar o monitor serial:

```bash
pio device monitor -b 115200
```

---

## Reprodutibilidade

O diretório `ESP-LoRa-TinyML` contém o projeto utilizado no protótipo final, incluindo o modelo Edge Impulse necessário para a inferência.

Também está disponível o pacote:

```text
ESP-LoRa-TinyML-final.zip
```

para facilitar a reprodução do projeto.

---

## Documentação e demonstrações

O repositório contém materiais complementares do projeto:

* **Relatório final** — documentação acadêmica do desenvolvimento.
* **Vídeos de demonstração** — registros do funcionamento do protótipo.
* **PORTFOLIO.md** — resumo dos principais resultados técnicos.
* **ESP-LoRa-TinyML-final.zip** — pacote do projeto final.

---

## Resultados

O projeto demonstra a integração, em uma única plataforma embarcada, de diferentes camadas de um sistema moderno:

```text
Sensoriamento
     ↓
Aquisição
     ↓
RTOS
     ↓
Processamento Digital
     ↓
TinyML
     ↓
Empacotamento
     ↓
CRC
     ↓
Comunicação LoRa
```

A arquitetura foi mantida deliberadamente simples, permitindo observar a interação entre **hardware, firmware, processamento de sinais, inteligência embarcada e comunicação sem fio**, sem depender de uma infraestrutura computacional externa para a etapa de classificação.

---

## Possíveis extensões

O protótipo estabelece uma base para futuras extensões, incluindo:

* utilização de outras técnicas de DSP;
* comparação entre diferentes filtros;
* utilização de modelos TinyML adicionais;
* otimização do consumo de energia;
* desenvolvimento de protocolo de telemetria mais completo;
* integração com LoRaWAN;
* implementação de mecanismos adicionais de detecção de falhas;
* separação do firmware em módulos C++ independentes;
* utilização de aceleradores de hardware para processamento.

---

## Autor

**Renato Augusto Schenkel Meneghin**

Projeto desenvolvido no contexto do **PPGESE/UFSC**.

---

## Licença

Consulte os arquivos e termos de licença das bibliotecas de terceiros utilizadas no projeto antes de redistribuir versões modificadas do firmware ou dos modelos.
