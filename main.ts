/**
 * Blocks for an external I2S microphone on micro:bit v2.
 */
//% weight=90 color=175 icon="\uf130" block="I2S microphone"
//% groups='["Configuration", "Speech recognition"]'
namespace i2sMicrophone {
    const SAMPLE_RATE = 16000
    const MIN_SECONDS = 1
    const MAX_SECONDS = 30

    let serviceUrl = ""
    let recordSeconds = 3
    let resultText = ""
    let resultReady = false

    /**
     * Set the full speech-recognition service URL used by the serial gateway.
     */
    //% blockId=i2s_microphone_set_service_url
    //% block="set speech service URL %url"
    //% url.defl="http://localhost:8000/speech-to-text"
    //% group="Configuration"
    //% weight=100
    export function setSpeechServiceUrl(url: string): void {
        serviceUrl = url
    }

    /**
     * Bind the I2S microphone pins.
     */
    //% blockId=i2s_microphone_set_i2s_pins
    //% block="set I2S microphone pins data %data lrc %lrc bck %bck"
    //% data.defl=p0 lrc.defl=p1 bck.defl=p2
    //% group="Configuration"
    //% weight=98
    export function setI2SMicrophonePins(data: DigitalPin, lrc: DigitalPin, bck: DigitalPin): void {
        configureI2SPinsNative(data, lrc, bck)
    }

    /**
     * Select which I2S channel to capture.
     */
    //% blockId=i2s_microphone_set_channel
    //% block="set I2S capture channel %channel"
    //% group="Configuration"
    //% weight=97
    export function setI2SCaptureChannel(channel: I2SCaptureChannel): void {
        setI2SCaptureChannelNative(channel)
    }

    /**
     * Set the recording duration. Values outside 1-30 seconds are clamped.
     */
    //% blockId=i2s_microphone_set_record_seconds
    //% block="set speech recording duration %seconds seconds"
    //% seconds.min=1 seconds.max=30 seconds.defl=3
    //% group="Configuration"
    //% weight=95
    export function setRecordingDuration(seconds: number): void {
        recordSeconds = clampSeconds(seconds)
    }

    /**
     * Record with the I2S microphone, send audio to the configured service, and wait for text.
     */
    //% blockId=i2s_microphone_recognize_configured
    //% block="record speech and send to service"
    //% group="Speech recognition"
    //% weight=90
    export function recognizeSpeech(): void {
        recognizeSpeechFor(recordSeconds)
    }

    /**
     * Record with the I2S microphone for 1-30 seconds, then send audio to the configured service.
     */
    //% blockId=i2s_microphone_recognize_for
    //% block="record speech for %seconds seconds and send to service"
    //% seconds.min=1 seconds.max=30 seconds.defl=3
    //% group="Speech recognition"
    //% weight=85
    export function recognizeSpeechFor(seconds: number): void {
        const duration = clampSeconds(seconds)
        const chunkSize = 256
        const chunk = pins.createBuffer(chunkSize)
        const endAt = input.runningTime() + duration * 1000

        resultText = ""
        resultReady = false
        serial.writeLine("#MIC_BEGIN url=" + serviceUrl + " rate=" + SAMPLE_RATE + " bits=16 channels=1 seconds=" + duration + " source=i2s")
        startNative()

        const startStatus = statusNative()
        if (startStatus != 1) {
            stopNative()
            serial.writeLine("#MIC_ERROR i2s-start-" + startStatus)
            serial.writeLine("#MIC_END")
            return
        }

        while (input.runningTime() < endAt) {
            const n = readPcmIntoBufferNative(chunk)
            if (n > 0) {
                serial.writeLine("#MIC_CHUNK " + n)
                serial.writeBuffer(chunk.slice(0, n))
            } else {
                basic.pause(1)
            }
        }

        stopNative()
        serial.writeLine("#MIC_END")

        const deadline = input.runningTime() + 15000
        while (input.runningTime() < deadline) {
            const line = serial.readLine()
            if (line && line.indexOf("#MIC_TEXT ") == 0) {
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
    //% weight=70
    export function speechResultReady(): boolean {
        return resultReady
    }

    /**
     * Get the last speech-recognition result returned by the serial gateway.
     */
    //% blockId=i2s_microphone_speech_text
    //% block="speech text"
    //% group="Speech recognition"
    //% weight=65
    export function speechText(): string {
        return resultText
    }

    function clampSeconds(seconds: number): number {
        if (seconds < MIN_SECONDS) {
            return MIN_SECONDS
        }
        if (seconds > MAX_SECONDS) {
            return MAX_SECONDS
        }
        return Math.round(seconds)
    }

    //% shim=i2sMicrophone::start
    function startNative(): void {
        return
    }

    //% shim=i2sMicrophone::stop
    function stopNative(): void {
        return
    }

    //% shim=i2sMicrophone::readPcmIntoBuffer
    function readPcmIntoBufferNative(buffer: Buffer): number {
        return 0
    }

    //% shim=i2sMicrophone::status
    function statusNative(): number {
        return 0
    }

    //% shim=i2sMicrophone::configureI2SPins
    function configureI2SPinsNative(data: DigitalPin, lrc: DigitalPin, bck: DigitalPin): void {
        return
    }

    //% shim=i2sMicrophone::setI2SCaptureChannel
    function setI2SCaptureChannelNative(channel: I2SCaptureChannel): void {
        return
    }
}

enum I2SCaptureChannel {
    //% block="left"
    Left = 0,
    //% block="right"
    Right = 1
}
