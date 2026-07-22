i2sMicrophone.setPins(DigitalPin.P0, DigitalPin.P1, DigitalPin.P2)
i2sMicrophone.setSampleRate(i2sMicrophone.SampleRate.Rate16000)

input.onButtonPressed(Button.A, function () {
    i2sMicrophone.start()
    basic.pause(100)
    serial.writeLine("sample=" + i2sMicrophone.readSample())
    i2sMicrophone.stop()
})
