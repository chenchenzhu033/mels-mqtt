# I2S Microphone Speech Recognition for micro:bit

MakeCode micro:bit extension for recording audio with an external I2S microphone and sending it to a speech-recognition service through a serial gateway.

## Important limitation

The micro:bit cannot directly send HTTP requests to an internet service by itself. This extension records PCM audio on the micro:bit and streams it over USB serial. A computer-side gateway receives the audio, converts it to WAV, posts it to the service URL configured in Blocks, and sends the recognized text back to the micro:bit.

## Blocks

- `set speech service URL ...`
- `set I2S microphone pins data ... lrc ... bck ...`
- `set I2S capture channel ...`
- `set speech recording duration ... seconds`
- `record speech and send to service`
- `record speech for ... seconds and send to service`
- `speech result ready`
- `speech text`

### Wiring

- `G` -> `GND`
- `V` -> `3V`
- `DATA` -> `P0`
- `LRC` -> `P1`
- `BCK` -> `P2`

If your microphone module uses the opposite I2S channel, switch the capture channel block.

The recording duration is clamped to 1-30 seconds. When the duration is reached, the microphone is stopped automatically and the collected audio is sent to the configured service.

## Example

```typescript
i2sMicrophone.setSpeechServiceUrl("http://localhost:8000/speech-to-text")
i2sMicrophone.setI2SMicrophonePins(DigitalPin.P0, DigitalPin.P1, DigitalPin.P2)
i2sMicrophone.setI2SCaptureChannel(I2SCaptureChannel.Left)
i2sMicrophone.setRecordingDuration(3)

input.onButtonPressed(Button.A, function () {
    basic.showIcon(IconNames.SmallDiamond)
    i2sMicrophone.recognizeSpeech()
    if (i2sMicrophone.speechResultReady()) {
        basic.showString(i2sMicrophone.speechText())
    } else {
        basic.showString("NO")
    }
})
```

## Serial gateway

Install dependencies:

```bash
python3 -m pip install pyserial requests
```

Run the gateway:

```bash
python3 gateway/serial_speech_gateway.py \
  --port /dev/tty.usbmodem1102
```

The backend URL normally comes from the `set speech service URL ...` block. You can also provide a fallback URL:

```bash
python3 gateway/serial_speech_gateway.py \
  --port /dev/tty.usbmodem1102 \
  --backend http://localhost:8000/speech-to-text
```

The backend should accept multipart form field `file` containing a WAV file and return either plain text or JSON with one of these keys:

- `text`
- `transcript`
- `result`

## Hardware notes

This extension targets micro:bit v2 because I2S audio capture uses the CODAL runtime on the v2 target.
