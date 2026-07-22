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

## MQTT Speech Recognition

The `MQTT Speech` blocks add experimental speech-recognition streaming for micro:bit v2. It uses the built-in microphone and publishes audio as text MQTT messages, so it does not require HTTP POST or binary file upload from the micro:bit.

### Blocks

- `set speech MQTT topic prefix ...`
- `set speech recording duration ... seconds`
- `subscribe speech result`
- `record speech and send by MQTT`
- `record speech for ... seconds and send by MQTT`
- `speech result ready`
- `speech text`

### Topic Protocol

If the topic prefix is `speech/microbit`, the extension publishes:

- `speech/microbit/begin`
- `speech/microbit/chunk`
- `speech/microbit/end`

The service should publish recognition text back to:

- `speech/microbit/result`

Message payloads are plain text:

```text
session=12345;rate=16000;bits=16;channels=1;seconds=3;encoding=pcm16le-hex
```

```text
session=12345;index=0;data=001122...
```

```text
session=12345;chunks=120
```

The service should rebuild chunk data in `index` order, decode hex into PCM 16-bit little-endian mono audio, wrap it as a WAV file, run speech recognition, and publish either raw text or:

```text
text=recognized words
```

### Practical Limits

The ESP8266 serial link runs at 9600 bps and MQTT messages are text, so audio streaming is bandwidth-limited. Start testing with 1-3 seconds of speech. Longer recordings may drop samples or take much longer to transmit.

### Example Server

An example Python MQTT speech service is provided in `examples/mqtt_speech_server.py`.

Install dependencies:

```bash
python3 -m pip install paho-mqtt SpeechRecognition
```

Run:

```bash
python3 examples/mqtt_speech_server.py \
  --host broker.example.com \
  --port 1883 \
  --prefix speech/microbit
```
