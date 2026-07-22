# I2S Microphone for micro:bit

MakeCode micro:bit extension for an external I2S microphone with pins:

| Microphone pin | Default micro:bit pin |
| --- | --- |
| G | GND |
| V | 3V |
| DATA | P0 |
| LRC / WS | P1 |
| BCK / SCK | P2 |

The DATA, LRC and BCK pins can be changed from Blocks with:

```typescript
i2sMicrophone.setPins(DigitalPin.P0, DigitalPin.P1, DigitalPin.P2)
```

## Can the module directly return an audio file?

No. An I2S microphone outputs a digital audio stream, not a file. A micro:bit also has very limited RAM and no normal file system, so it should not try to return a complete audio file from a block.

This extension captures 16-bit PCM samples. For speech recognition, it streams the PCM bytes over USB serial to a computer or network module. The gateway converts PCM to WAV, calls your backend speech-recognition API, and writes the text result back to the micro:bit.

## Blocks

- `set I2S microphone DATA ... LRC ... BCK ...`
- `set I2S microphone sample rate ...`
- `start I2S microphone`
- `stop I2S microphone`
- `I2S microphone sample`
- `recognize speech for ... ms`
- `speech result ready`
- `speech text`

## Speech-recognition flow

Example Blocks/TypeScript:

```typescript
i2sMicrophone.setPins(DigitalPin.P0, DigitalPin.P1, DigitalPin.P2)
i2sMicrophone.setSampleRate(i2sMicrophone.SampleRate.Rate16000)

input.onButtonPressed(Button.A, function () {
    basic.showIcon(IconNames.SmallDiamond)
    i2sMicrophone.recognizeSpeech(3000)
    if (i2sMicrophone.speechResultReady()) {
        basic.showString(i2sMicrophone.speechText())
    }
})
```

Run the serial gateway on the connected computer:

```bash
python3 -m pip install pyserial requests
python3 gateway/serial_speech_gateway.py \
  --port /dev/tty.usbmodem1102 \
  --backend http://localhost:8000/speech-to-text
```

The backend should accept multipart form field `file` containing a WAV file and return either plain text or JSON with one of these keys:

- `text`
- `transcript`
- `result`

## Hardware notes

This extension targets micro:bit v2 because it depends on the nRF52 I2S peripheral. micro:bit v1 does not have a compatible I2S hardware block exposed by this extension.

The C++ pin map includes the commonly used edge pins P0, P1, P2, P8, P12, P13, P14, P15 and P16. If your board or target uses different physical nRF GPIO mappings, update `physicalPinFromDigitalPin` in `shims.cpp`.
