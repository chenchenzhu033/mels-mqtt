# EMmqtt

EMmqtt is a MakeCode extension for micro:bit that supports MQTT, HTTP and Aliyun MQTT using an ESP8266-based serial Wi-Fi module.

![image](image/index.png)

## Overview

This extension provides an easy way to connect micro:bit to Wi-Fi and cloud services through a serial-to-Wi-Fi MQTT module. It leverages ESP8266 AT commands and adds MQTT command support, exposing them as MakeCode blocks for fast visual programming.

## Hardware Specifications

- Operating voltage: 5V
- Serial baud rate: 9600 bps
- Wireless frequency: 2.4GHz
- Interface type: PH2.0-4Pin (G V TX RX)
- Wi-Fi mode: IEEE802.11b/g/n
- SRAM: 160KB
- External flash: 4MB
- Typical current: <240mA
- Module size: 4 x 2.1 cm
- Mounting: M4 screws and nuts

## Features

- Built-in 32-bit low-power CPU
- Built-in TCP/IP protocol stack
- Supports WPA/WPA2/WPA2-PSK encryption
- Compatible with ESP8266 AT command set
- Supports standard MQTT publishing and subscribing
- Supports Aliyun MQTT connection
- Supports HTTP GET requests over TCP

## MakeCode Blocks

Use the blocks provided by this extension to:

- Connect to Wi-Fi using serial pins, SSID and password
- Connect to an MQTT broker
- Publish data to MQTT topics
- Subscribe to MQTT topics and handle incoming messages
- Connect to Aliyun MQTT services
- Send HTTP GET requests

## Supported AT Commands

The module supports standard ESP8266 AT commands and MQTT-specific AT extensions such as:

- `AT+CWJAP` — connect to Wi-Fi
- `AT+MQTTUSERCFG` — configure MQTT user properties
- `AT+MQTTCONN` — connect to MQTT broker
- `AT+ALIYUN_MQTTCONN` — connect to Aliyun MQTT broker
- `AT+MQTTPUB` — publish MQTT messages
- `AT+MQTTSUB` — subscribe to MQTT topics
- `AT+CIPSTART` — start a TCP connection for HTTP

## Files

- `MQTT.ts` — MakeCode extension source with block definitions
- `MQTT.cpp` — native shim for the extension
- `README.md` — English documentation
- `_locales/zh/MQTT-strings.json` — Chinese translation strings (optional)

## Notes

The source file now uses English block labels and category names. The extension remains compatible with the existing module logic and keeps Chinese localization available through `_locales/zh/MQTT-strings.json`.
