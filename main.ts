/**
 * Blocks for an external I2S microphone.
 */
//% weight=90 color=175 icon="\uf130" block="I2S microphone"
//% groups='["Configuration", "Recording", "Speech recognition"]'
namespace i2sMicrophone {
    export enum SampleRate {
        //% block="8 kHz"
        Rate8000 = 8000,
        //% block="16 kHz"
        Rate16000 = 16000,
        //% block="32 kHz"
        Rate32000 = 32000
    }

    let currentRate = SampleRate.Rate16000
    let resultText = ""
    let resultReady = false

    /**
     * Bind the DATA, LRC and BCK pins used by the I2S microphone.
     * Default wiring: DATA=P0, LRC=P1, BCK=P2.
     */
    //% blockId=i2s_microphone_set_pins
    //% block="set I2S microphone DATA %dataPin LRC %lrcPin BCK %bckPin"
    //% dataPin.defl=DigitalPin.P0 lrcPin.defl=DigitalPin.P1 bckPin.defl=DigitalPin.P2
    //% group="Configuration"
    //% weight=100
    export function setPins(dataPin: DigitalPin, lrcPin: DigitalPin, bckPin: DigitalPin): void {
        setPinsNative(dataPin, lrcPin, bckPin)
    }

    /**
     * Set the sample rate for I2S capture.
     */
    //% blockId=i2s_microphone_set_sample_rate
    //% block="set I2S microphone sample rate %rate"
    //% group="Configuration"
    //% weight=95
    export function setSampleRate(rate: SampleRate): void {
        currentRate = rate
        setSampleRateNative(rate)
    }

    /**
     * Start capturing raw PCM samples from the microphone.
     */
    //% blockId=i2s_microphone_start
    //% block="start I2S microphone"
    //% group="Recording"
    //% weight=90
    export function start(): void {
        startNative()
    }

    /**
     * Stop capturing raw PCM samples.
     */
    //% blockId=i2s_microphone_stop
    //% block="stop I2S microphone"
    //% group="Recording"
    //% weight=85
    export function stop(): void {
        stopNative()
    }

    /**
     * Read one signed 16-bit PCM sample. Returns 0 when no sample is available.
     */
    //% blockId=i2s_microphone_read_sample
    //% block="I2S microphone sample"
    //% group="Recording"
    //% weight=80
    export function readSample(): number {
        return readSampleNative()
    }

    /**
     * Record PCM audio and stream it to the serial speech-recognition gateway.
     *
     * The micro:bit cannot directly create and return an audio file or call an
     * internet API by itself. This block streams PCM bytes over USB serial. A
     * computer-side gateway should convert the stream to WAV, call the backend,
     * and write back a line starting with #I2S_TEXT.
     */
    //% blockId=i2s_microphone_recognize
    //% block="recognize speech for %milliseconds ms"
    //% milliseconds.min=250 milliseconds.max=10000 milliseconds.defl=3000
    //% group="Speech recognition"
    //% weight=70
    export function recognizeSpeech(milliseconds: number): void {
        const chunkSize = 128
        const chunk = pins.createBuffer(chunkSize)
        const endAt = input.runningTime() + milliseconds

        resultText = ""
        resultReady = false
        serial.writeLine("#I2S_BEGIN rate=" + currentRate + " bits=16 channels=1")
        start()

        while (input.runningTime() < endAt) {
            const n = readPcmIntoBufferNative(chunk)
            if (n > 0) {
                serial.writeLine("#I2S_CHUNK " + n)
                serial.writeBuffer(chunk.slice(0, n))
            } else {
                basic.pause(1)
            }
        }

        stop()
        serial.writeLine("#I2S_END")

        const deadline = input.runningTime() + 15000
        while (input.runningTime() < deadline) {
            const line = serial.readLine()
            if (line && line.indexOf("#I2S_TEXT ") == 0) {
                resultText = line.substr(10)
                resultReady = true
                return
            }
            basic.pause(20)
        }
    }

    /**
     * Return true after the backend has sent recognition text back to micro:bit.
     */
    //% blockId=i2s_microphone_speech_ready
    //% block="speech result ready"
    //% group="Speech recognition"
    //% weight=60
    export function speechResultReady(): boolean {
        return resultReady
    }

    /**
     * Get the last speech-recognition result returned by the serial gateway.
     */
    //% blockId=i2s_microphone_speech_text
    //% block="speech text"
    //% group="Speech recognition"
    //% weight=55
    export function speechText(): string {
        return resultText
    }

    //% shim=i2sMicrophone::setPins
    function setPinsNative(dataPin: number, lrcPin: number, bckPin: number): void {
        return
    }

    //% shim=i2sMicrophone::setSampleRate
    function setSampleRateNative(rate: number): void {
        return
    }

    //% shim=i2sMicrophone::start
    function startNative(): void {
        return
    }

    //% shim=i2sMicrophone::stop
    function stopNative(): void {
        return
    }

    //% shim=i2sMicrophone::readSample
    function readSampleNative(): number {
        return 0
    }

    //% shim=i2sMicrophone::readPcmIntoBuffer
    function readPcmIntoBufferNative(buffer: Buffer): number {
        return 0
    }
}
