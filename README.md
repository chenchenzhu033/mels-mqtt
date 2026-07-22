# micro:bit Built-in Microphone Speech Recognition

MakeCode micro:bit extension for recording audio with the micro:bit v2 built-in microphone and sending it to a speech-recognition service through a serial gateway.

No external I2S microphone wiring is required.

## Important limitation

The micro:bit cannot directly send HTTP requests to an internet service by itself. This extension records PCM audio on the micro:bit and streams it over USB serial. A computer-side gateway receives the audio, converts it to WAV, posts it to the service URL configured in Blocks, and sends the recognized text back to the micro:bit.

## Blocks

- `set speech service URL ...`
- `set speech recording duration ... seconds`
- `record speech and send to service`
- `record speech for ... seconds and send to service`
- `speech result ready`
- `speech text`

The recording duration is clamped to 1-30 seconds. When the duration is reached, the microphone is stopped automatically and the collected audio is sent to the configured service.

## Example

```typescript
i2sMicrophone.setSpeechServiceUrl("http://localhost:8000/speech-to-text")
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

This extension targets micro:bit v2 because micro:bit v1 does not have the built-in microphone.
