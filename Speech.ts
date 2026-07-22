/**
 * I2S microphone speech-recognition audio streaming over MQTT.
 */
//% weight=9 color=#0B7285 icon="\uf130" block="I2S microphone"
//% groups='["Configuration", "Recognition"]'
namespace MQTTSpeech {
    const SAMPLE_RATE = 16000
    const MIN_SECONDS = 1
    const MAX_SECONDS = 30

    let topicPrefix = "speech/microbit"
    let recordSeconds = 3
    let lastText = ""
    let ready = false

    /**
     * Set the MQTT topic prefix used for speech-recognition messages.
     */
    //% blockId=mqtt_speech_set_topic_prefix
    //% block="set speech MQTT topic prefix %prefix"
    //% prefix.defl="speech/microbit"
    //% group="Configuration"
    //% weight=100
    export function setTopicPrefix(prefix: string): void {
        topicPrefix = trimSlash(prefix)
    }

    /**
     * Set the I2S microphone pins.
     */
    //% blockId=mqtt_speech_set_i2s_pins
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
    //% blockId=mqtt_speech_set_i2s_channel
    //% block="set I2S capture channel %channel"
    //% group="Configuration"
    //% weight=97
    export function setI2SCaptureChannel(channel: I2SCaptureChannel): void {
        setI2SCaptureChannelNative(channel)
    }

    /**
     * Set the recording duration. Values outside 1-30 seconds are clamped.
     */
    //% blockId=mqtt_speech_set_duration
    //% block="set speech recording duration %seconds seconds"
    //% seconds.min=1 seconds.max=30 seconds.defl=3
    //% group="Configuration"
    //% weight=95
    export function setRecordingDuration(seconds: number): void {
        recordSeconds = clampSeconds(seconds)
    }

    /**
     * Subscribe to the speech result topic. Call this after connecting MQTT.
     */
    //% blockId=mqtt_speech_subscribe_result
    //% block="subscribe speech result"
    //% group="Configuration"
    //% weight=90
    export function subscribeResult(): void {
        MQTT.em_mqtt_subscribe(resultTopic(), 0)
        MQTT.em_mqtt_get_topic_message(resultTopic(), function (message: string) {
            lastText = parseResult(message)
            ready = true
        })
    }

    /**
     * Record with the I2S microphone and send audio chunks by MQTT.
     */
    //% blockId=mqtt_speech_record_send
    //% block="record speech and send by MQTT"
    //% group="Recognition"
    //% weight=80
    export function recordAndSend(): void {
        recordAndSendFor(recordSeconds)
    }

    /**
     * Record for 1-30 seconds and send audio chunks by MQTT.
     */
    //% blockId=mqtt_speech_record_send_for
    //% block="record speech for %seconds seconds and send by MQTT"
    //% seconds.min=1 seconds.max=30 seconds.defl=3
    //% group="Recognition"
    //% weight=75
    export function recordAndSendFor(seconds: number): void {
        const duration = clampSeconds(seconds)
        const session = "" + control.millis()
        const chunk = pins.createBuffer(64)
        const endAt = input.runningTime() + duration * 1000
        let index = 0

        ready = false
        lastText = ""
        MQTT.__speechPublish(beginTopic(), "session=" + session + ";rate=" + SAMPLE_RATE + ";bits=16;channels=1;seconds=" + duration + ";encoding=pcm16le-hex")
        startNative()
        const startStatus = statusNative()
        if (startStatus != 1) {
            stopNative()
            MQTT.__speechPublish(endTopic(), "session=" + session + ";chunks=0;error=i2s-start-" + startStatus)
            return
        }

        while (input.runningTime() < endAt) {
            const n = readPcmIntoBufferNative(chunk)
            if (n > 0) {
                MQTT.__speechPublish(chunkTopic(), "session=" + session + ";index=" + index + ";data=" + bufferToHex(chunk, n))
                index++
            } else {
                basic.pause(1)
            }
        }

        stopNative()
        MQTT.__speechPublish(endTopic(), "session=" + session + ";chunks=" + index)
    }

    /**
     * Return true after a speech result has arrived on the result topic.
     */
    //% blockId=mqtt_speech_result_ready
    //% block="speech result ready"
    //% group="Recognition"
    //% weight=60
    export function speechResultReady(): boolean {
        return ready
    }

    /**
     * Return the latest speech-recognition text received by MQTT.
     */
    //% blockId=mqtt_speech_text
    //% block="speech text"
    //% group="Recognition"
    //% weight=55
    export function speechText(): string {
        return lastText
    }

    /**
     * Return the native microphone status for troubleshooting.
     * 1 means started, -1 means this target has no I2S support, -2 means an I2S pin was not found.
     */
    //% blockId=mqtt_speech_microphone_status
    //% block="microphone status"
    //% group="Recognition"
    //% weight=50
    export function microphoneStatus(): number {
        return statusNative()
    }

    function beginTopic(): string {
        return topicPrefix + "/begin"
    }

    function chunkTopic(): string {
        return topicPrefix + "/chunk"
    }

    function endTopic(): string {
        return topicPrefix + "/end"
    }

    function resultTopic(): string {
        return topicPrefix + "/result"
    }

    function clampSeconds(seconds: number): number {
        if (seconds < MIN_SECONDS) return MIN_SECONDS
        if (seconds > MAX_SECONDS) return MAX_SECONDS
        return Math.round(seconds)
    }

    function trimSlash(value: string): string {
        while (value.length > 0 && value.charAt(value.length - 1) == "/") {
            value = value.substr(0, value.length - 1)
        }
        return value
    }

    function parseResult(message: string): string {
        const marker = "text="
        const at = message.indexOf(marker)
        if (at >= 0) {
            return message.substr(at + marker.length)
        }
        return message
    }

    function bufferToHex(buffer: Buffer, length: number): string {
        const chars = "0123456789abcdef"
        let out = ""
        for (let i = 0; i < length; i++) {
            const value = buffer.getNumber(NumberFormat.UInt8LE, i)
            out = out + chars.charAt(value >> 4) + chars.charAt(value & 15)
        }
        return out
    }

    //% shim=MQTTSpeech::start
    function startNative(): void {
        return
    }

    //% shim=MQTTSpeech::stop
    function stopNative(): void {
        return
    }

    //% shim=MQTTSpeech::readPcmIntoBuffer
    function readPcmIntoBufferNative(buffer: Buffer): number {
        return 0
    }

    //% shim=MQTTSpeech::status
    function statusNative(): number {
        return 0
    }

    //% shim=MQTTSpeech::configureI2SPins
    function configureI2SPinsNative(data: DigitalPin, lrc: DigitalPin, bck: DigitalPin): void {
        return
    }

    //% shim=MQTTSpeech::setI2SCaptureChannel
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
