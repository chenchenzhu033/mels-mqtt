#include "pxt.h"

#if defined(NRF52833_XXAA) || defined(NRF52840_XXAA) || defined(NRF52)
#ifdef SCK
#undef SCK
#endif
#include "nrf.h"
#define I2S_MIC_HAS_NRF_I2S 1
#else
#define I2S_MIC_HAS_NRF_I2S 0
#endif

namespace i2sMicrophone {

static int dataPinId = MICROBIT_ID_IO_P0;
static int lrcPinId = MICROBIT_ID_IO_P1;
static int bckPinId = MICROBIT_ID_IO_P2;
static int sampleRate = 16000;
static volatile bool running = false;

#if I2S_MIC_HAS_NRF_I2S
static const int RING_SAMPLES = 2048;
static volatile int16_t ring[RING_SAMPLES];
static volatile int ringHead = 0;
static volatile int ringTail = 0;
static const int DMA_WORDS = 256;
static uint32_t rxA[DMA_WORDS];
static uint32_t rxB[DMA_WORDS];
static volatile int currentRx = 0;
static volatile int lastCompletedRx = -1;
#endif

#if I2S_MIC_HAS_NRF_I2S
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
#endif

static uint32_t physicalPinFromDigitalPin(int pinId) {
    switch (pinId) {
        case MICROBIT_ID_IO_P0: return 2;
        case MICROBIT_ID_IO_P1: return 3;
        case MICROBIT_ID_IO_P2: return 4;
        case MICROBIT_ID_IO_P8: return 10;
        case MICROBIT_ID_IO_P12: return 12;
        case MICROBIT_ID_IO_P13: return 17;
        case MICROBIT_ID_IO_P14: return 1;
        case MICROBIT_ID_IO_P15: return 13;
        case MICROBIT_ID_IO_P16: return 14;
        default: return 0xFFFFFFFF;
    }
}

#if I2S_MIC_HAS_NRF_I2S
static uint32_t mckForRate(int rate) {
    if (rate <= 8000) {
        return I2S_CONFIG_MCKFREQ_MCKFREQ_32MDIV63;
    }
    if (rate >= 32000) {
        return I2S_CONFIG_MCKFREQ_MCKFREQ_32MDIV15;
    }
    return I2S_CONFIG_MCKFREQ_MCKFREQ_32MDIV31;
}

static void processCompletedBuffer() {
    if (lastCompletedRx < 0) {
        return;
    }

    uint32_t *samples = lastCompletedRx == 0 ? rxA : rxB;
    lastCompletedRx = -1;
    for (int i = 0; i < DMA_WORDS; i++) {
        pushSample((int16_t)(samples[i] >> 16));
    }
}
#endif

//%
void setPins(int dataPin, int lrcPin, int bckPin) {
    dataPinId = dataPin;
    lrcPinId = lrcPin;
    bckPinId = bckPin;
}

//%
void setSampleRate(int rate) {
    if (rate <= 8000) {
        sampleRate = 8000;
    } else if (rate >= 32000) {
        sampleRate = 32000;
    } else {
        sampleRate = 16000;
    }
}

//%
void start() {
    if (running) {
        return;
    }

#if I2S_MIC_HAS_NRF_I2S
    clearRing();

    uint32_t data = physicalPinFromDigitalPin(dataPinId);
    uint32_t lrc = physicalPinFromDigitalPin(lrcPinId);
    uint32_t bck = physicalPinFromDigitalPin(bckPinId);
    if (data == 0xFFFFFFFF || lrc == 0xFFFFFFFF || bck == 0xFFFFFFFF) {
        return;
    }

    NRF_I2S->ENABLE = 0;
    NRF_I2S->PSEL.SCK = bck;
    NRF_I2S->PSEL.LRCK = lrc;
    NRF_I2S->PSEL.MCK = 0xFFFFFFFF;
    NRF_I2S->PSEL.SDOUT = 0xFFFFFFFF;
    NRF_I2S->PSEL.SDIN = data;
    NRF_I2S->CONFIG.MODE = I2S_CONFIG_MODE_MODE_Master;
    NRF_I2S->CONFIG.RXEN = I2S_CONFIG_RXEN_RXEN_Enabled;
    NRF_I2S->CONFIG.TXEN = I2S_CONFIG_TXEN_TXEN_Disabled;
    NRF_I2S->CONFIG.MCKEN = I2S_CONFIG_MCKEN_MCKEN_Disabled;
    NRF_I2S->CONFIG.MCKFREQ = mckForRate(sampleRate);
    NRF_I2S->CONFIG.RATIO = I2S_CONFIG_RATIO_RATIO_64X;
    NRF_I2S->CONFIG.SWIDTH = I2S_CONFIG_SWIDTH_SWIDTH_16Bit;
    NRF_I2S->CONFIG.ALIGN = I2S_CONFIG_ALIGN_ALIGN_Left;
    NRF_I2S->CONFIG.FORMAT = I2S_CONFIG_FORMAT_FORMAT_I2S;
    NRF_I2S->CONFIG.CHANNELS = I2S_CONFIG_CHANNELS_CHANNELS_Left;

    currentRx = 0;
    lastCompletedRx = -1;
    NRF_I2S->RXD.PTR = (uint32_t)rxA;
    NRF_I2S->RXTXD.MAXCNT = DMA_WORDS;
    NRF_I2S->EVENTS_RXPTRUPD = 0;
    NRF_I2S->INTENSET = I2S_INTENSET_RXPTRUPD_Msk;
    NVIC_ClearPendingIRQ(I2S_IRQn);
    NVIC_SetPriority(I2S_IRQn, 1);
    NVIC_EnableIRQ(I2S_IRQn);

    running = true;
    NRF_I2S->ENABLE = 1;
    NRF_I2S->TASKS_START = 1;
#endif
}

//%
void stop() {
#if I2S_MIC_HAS_NRF_I2S
    if (running) {
        NRF_I2S->TASKS_STOP = 1;
        NVIC_DisableIRQ(I2S_IRQn);
        NRF_I2S->INTENCLR = I2S_INTENCLR_RXPTRUPD_Msk;
        NRF_I2S->ENABLE = 0;
    }
#endif
    running = false;
}

//%
int readSample() {
#if I2S_MIC_HAS_NRF_I2S
    processCompletedBuffer();
    int16_t sample = 0;
    if (!popSample(&sample)) {
        return 0;
    }
    return sample;
#else
    return 0;
#endif
}

//%
int readPcmIntoBuffer(Buffer buffer) {
#if I2S_MIC_HAS_NRF_I2S
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

}

#if I2S_MIC_HAS_NRF_I2S
extern "C" void I2S_IRQHandler(void) {
    if (NRF_I2S->EVENTS_RXPTRUPD) {
        NRF_I2S->EVENTS_RXPTRUPD = 0;
        if (i2sMicrophone::running) {
            i2sMicrophone::lastCompletedRx = i2sMicrophone::currentRx;
            i2sMicrophone::currentRx = i2sMicrophone::currentRx == 0 ? 1 : 0;
            NRF_I2S->RXD.PTR = (uint32_t)(i2sMicrophone::currentRx == 0 ? i2sMicrophone::rxA : i2sMicrophone::rxB);
            NRF_I2S->RXTXD.MAXCNT = i2sMicrophone::DMA_WORDS;
        }
    }
}
#endif
