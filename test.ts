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
