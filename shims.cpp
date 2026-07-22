#include "pxt.h"

#if MICROBIT_CODAL && (defined(NRF52833_XXAA) || defined(NRF52840_XXAA) || defined(NRF52))
#include "nrf.h"
#ifdef SCK
#undef SCK
#endif
#define I2S_MIC_HAS_HW 1
#else
#define I2S_MIC_HAS_HW 0
#endif

namespace i2sMicrophone {

static volatile bool running = false;
static volatile int lastStartStatus = 0;
static volatile int dataPinName = 0;
static volatile int lrcPinName = 1;
static volatile int bckPinName = 2;
static volatile int captureChannel = 0;

#if I2S_MIC_HAS_HW
static const int RING_SAMPLES = 4096;
static const int DMA_WORDS = 128;
static volatile int16_t ring[RING_SAMPLES];
static volatile int ringHead = 0;
static volatile int ringTail = 0;
static uint32_t rxA[DMA_WORDS];
static uint32_t rxB[DMA_WORDS];
static volatile int currentSampleBuffer = 0;
static volatile int completedSampleBuffer = -1;
#endif

#if I2S_MIC_HAS_HW
static void pushSample(int16_t sample) {
    int next = (ringHead + 1) % RING_SAMPLES;
    if (next == ringTail) {
        ringTail = (ringTail + 1) % RING_SAMPLES;
    }
    ring[ringHead] = sample;
    ringHead = next;
}

static bool popSample(int16_t *sample) {
    if (ringHead == ringTail) {
        return false;
    }
    *sample = ring[ringTail];
    ringTail = (ringTail + 1) % RING_SAMPLES;
    return true;
}

static void clearRing() {
    ringHead = 0;
    ringTail = 0;
}

static uint32_t pinValue(int pinName) {
    MicroBitPin *pin = pxt::getPin(pinName);
    if (!pin) {
        return 0xFFFFFFFF;
    }
    return pin->name;
}

static void processCompletedBuffer() {
    if (completedSampleBuffer < 0) {
        return;
    }

    uint32_t *samples = completedSampleBuffer == 0 ? rxA : rxB;
    completedSampleBuffer = -1;
    for (int i = 0; i < DMA_WORDS; i++) {
        if ((i & 1) != captureChannel) {
            continue;
        }

        int32_t sample = (int32_t)samples[i];
        sample >>= 8;
        pushSample((int16_t)sample);
    }
}

static void setupI2S() {
    NRF_I2S->ENABLE = I2S_ENABLE_ENABLE_Disabled << I2S_ENABLE_ENABLE_Pos;

    NRF_I2S->PSEL.SCK = ((uint32_t)pinValue(bckPinName) & I2S_PSEL_SCK_PIN_Msk)
        | (I2S_PSEL_SCK_CONNECT_Connected << I2S_PSEL_SCK_CONNECT_Pos);
    NRF_I2S->PSEL.LRCK = ((uint32_t)pinValue(lrcPinName) & I2S_PSEL_LRCK_PIN_Msk)
        | (I2S_PSEL_LRCK_CONNECT_Connected << I2S_PSEL_LRCK_CONNECT_Pos);
    NRF_I2S->PSEL.SDIN = ((uint32_t)pinValue(dataPinName) & I2S_PSEL_SDIN_PIN_Msk)
        | (I2S_PSEL_SDIN_CONNECT_Connected << I2S_PSEL_SDIN_CONNECT_Pos);
    NRF_I2S->PSEL.SDOUT = I2S_PSEL_SDOUT_CONNECT_Disconnected << I2S_PSEL_SDOUT_CONNECT_Pos;
    NRF_I2S->PSEL.MCK = I2S_PSEL_MCK_CONNECT_Disconnected << I2S_PSEL_MCK_CONNECT_Pos;

    NRF_I2S->CONFIG.MODE = I2S_CONFIG_MODE_MODE_Master << I2S_CONFIG_MODE_MODE_Pos;
    NRF_I2S->CONFIG.RXEN = I2S_CONFIG_RXEN_RXEN_Enabled << I2S_CONFIG_RXEN_RXEN_Pos;
    NRF_I2S->CONFIG.TXEN = I2S_CONFIG_TXEN_TXEN_Disabled << I2S_CONFIG_TXEN_TXEN_Pos;
    NRF_I2S->CONFIG.MCKEN = I2S_CONFIG_MCKEN_MCKEN_Enabled << I2S_CONFIG_MCKEN_MCKEN_Pos;
    NRF_I2S->CONFIG.MCKFREQ = I2S_CONFIG_MCKFREQ_MCKFREQ_32MDIV31 << I2S_CONFIG_MCKFREQ_MCKFREQ_Pos;
    NRF_I2S->CONFIG.RATIO = I2S_CONFIG_RATIO_RATIO_64X << I2S_CONFIG_RATIO_RATIO_Pos;
    NRF_I2S->CONFIG.SWIDTH = I2S_CONFIG_SWIDTH_SWIDTH_24Bit << I2S_CONFIG_SWIDTH_SWIDTH_Pos;
    NRF_I2S->CONFIG.ALIGN = I2S_CONFIG_ALIGN_ALIGN_Left << I2S_CONFIG_ALIGN_ALIGN_Pos;
    NRF_I2S->CONFIG.FORMAT = I2S_CONFIG_FORMAT_FORMAT_I2S << I2S_CONFIG_FORMAT_FORMAT_Pos;
    NRF_I2S->CONFIG.CHANNELS = I2S_CONFIG_CHANNELS_CHANNELS_Stereo << I2S_CONFIG_CHANNELS_CHANNELS_Pos;

    NRF_I2S->RXD.PTR = (uint32_t)rxA;
    NRF_I2S->RXTXD.MAXCNT = DMA_WORDS;
    NRF_I2S->EVENTS_RXPTRUPD = 0;
    NRF_I2S->EVENTS_STOPPED = 0;
    NRF_I2S->INTENSET = I2S_INTENSET_RXPTRUPD_Msk | I2S_INTENSET_STOPPED_Msk;

    NVIC_ClearPendingIRQ(I2S_IRQn);
    NVIC_SetPriority(I2S_IRQn, 1);
    NVIC_EnableIRQ(I2S_IRQn);

    NRF_I2S->ENABLE = I2S_ENABLE_ENABLE_Enabled << I2S_ENABLE_ENABLE_Pos;
    NRF_I2S->TASKS_START = 1;
}
#endif

//%
void start() {
    if (running) {
        return;
    }

#if I2S_MIC_HAS_HW
    lastStartStatus = 0;
    uint32_t data = pinValue(dataPinName);
    uint32_t lrc = pinValue(lrcPinName);
    uint32_t bck = pinValue(bckPinName);
    if (data == 0xFFFFFFFF || lrc == 0xFFFFFFFF || bck == 0xFFFFFFFF) {
        lastStartStatus = -2;
        return;
    }

    clearRing();
    currentSampleBuffer = 0;
    completedSampleBuffer = -1;

    setupI2S();
    running = true;
    lastStartStatus = 1;
#endif
#if !I2S_MIC_HAS_HW
    lastStartStatus = -1;
#endif
}

//%
void stop() {
#if I2S_MIC_HAS_HW
    if (running) {
        NRF_I2S->TASKS_STOP = 1;
        NVIC_DisableIRQ(I2S_IRQn);
        NRF_I2S->INTENCLR = I2S_INTENCLR_RXPTRUPD_Msk | I2S_INTENCLR_STOPPED_Msk;
        NRF_I2S->ENABLE = I2S_ENABLE_ENABLE_Disabled << I2S_ENABLE_ENABLE_Pos;
    }
#endif
    running = false;
}

//%
int readPcmIntoBuffer(Buffer buffer) {
#if I2S_MIC_HAS_HW
    processCompletedBuffer();
    if (!buffer) {
        return 0;
    }

    int written = 0;
    int16_t sample = 0;
    while (written + 1 < buffer->length && popSample(&sample)) {
        buffer->data[written++] = sample & 0xff;
        buffer->data[written++] = (sample >> 8) & 0xff;
    }
    return written;
#else
    return 0;
#endif
}

//%
int status() {
    return lastStartStatus;
}

//%
void configureI2SPins(int data, int lrc, int bck) {
    dataPinName = data;
    lrcPinName = lrc;
    bckPinName = bck;
}

//%
void setI2SCaptureChannel(int channel) {
    captureChannel = channel == 1 ? 1 : 0;
}

}

#if I2S_MIC_HAS_HW
extern "C" void I2S_IRQHandler(void) {
    if (NRF_I2S->EVENTS_RXPTRUPD) {
        NRF_I2S->EVENTS_RXPTRUPD = 0;
        if (i2sMicrophone::running) {
            i2sMicrophone::completedSampleBuffer = i2sMicrophone::currentSampleBuffer;
            i2sMicrophone::currentSampleBuffer = i2sMicrophone::currentSampleBuffer == 0 ? 1 : 0;
            NRF_I2S->RXD.PTR = (uint32_t)(i2sMicrophone::currentSampleBuffer == 0
                ? i2sMicrophone::rxA
                : i2sMicrophone::rxB);
        }
    }
    if (NRF_I2S->EVENTS_STOPPED) {
        NRF_I2S->EVENTS_STOPPED = 0;
    }
}
#endif
