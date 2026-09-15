# Portfólio — ESP-LoRa-TinyML

## Resumo

Protótipo de monitoramento embarcado baseado em Heltec WiFi LoRa 32 V2. O sistema combina aquisição de sensores ambientais e inerciais, processamento digital, classificação TinyML e comunicação LoRa.

## Entregáveis

1. Código-fonte PlatformIO em `ESP-LoRa-TinyML`.
2. Pacote reproduzível `ESP-LoRa-TinyML-final.zip`.
3. Demonstrações em vídeo: `VID_20260907_194731.mp4` e `VID_20260907_194851.mp4`.
4. Relatório final em PDF.

## Resultado técnico

O firmware coleta BME280 e MPU6050 em tasks FreeRTOS, calcula magnitude e filtragem FIR, executa classificação Edge Impulse e envia um quadro binário com CRC16 por LoRa. A arquitetura foi mantida simples para facilitar a reprodução no VS Code e PlatformIO.
