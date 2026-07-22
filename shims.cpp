#include "pxt.h"

#if MICROBIT_CODAL && (defined(NRF52833_XXAA) || defined(NRF52840_XXAA) || defined(NRF52))
#include "nrf.h"
#define BUILTIN_MIC_HAS_PDM 1
#else
#define BUILTIN_MIC_HAS_PDM 0
#endif

namespace i2sMicrophone {

static volatile bool running = false;

#if BUILTIN_MIC_HAS_PDM
static const int RING_SAMPLES = 2048;
static const int DMA_SAMPLES = 256;
static volatile int16_t ring[RING_SAMPLES];
static volatile int ringHead = 0;
static volatile int ringTail = 0;
static int16_t sampleA[DMA_SAMPLES];
static int16_t sampleB[DMA_SAMPLES];
static volatile int currentSampleBuffer = 0;
static volatile int completedSampleBuffer = -1;
#endif

#if BUILTIN_MIC_HAS_PDM
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

static uint32_t microphonePin(int key) {
    int value = getConfig(key, -1);
    if (value < 0) {
        return 0xFFFFFFFF;
    }

    MicroBitPin *pin = pxt::getPin(value & CFG_PIN_NAME_MSK);
    if (!pin) {
        return 0xFFFFFFFF;
    }
    return pin->name;
}

static void processCompletedBuffer() {
    if (completedSampleBuffer < 0) {
        return;
    }

    int16_t *samples = completedSampleBuffer == 0 ? sampleA : sampleB;
    completedSampleBuffer = -1;
    for (int i = 0; i < DMA_SAMPLES; i++) {
        pushSample(samples[i]);
    }
}
#endif

//%
void start() {
    if (running) {
        return;
    }

#if BUILTIN_MIC_HAS_PDM
    uint32_t data = microphonePin(CFG_PIN_MIC_DATA);
    uint32_t clock = microphonePin(CFG_PIN_MIC_CLOCK);
    if (data == 0xFFFFFFFF || clock == 0xFFFFFFFF) {
        return;
    }

    clearRing();
    currentSampleBuffer = 0;
    completedSampleBuffer = -1;

    NRF_PDM->ENABLE = 0;
    NRF_PDM->PSEL.CLK = clock;
    NRF_PDM->PSEL.DIN = data;
    NRF_PDM->MODE = PDM_MODE_OPERATION_Mono << PDM_MODE_OPERATION_Pos;
    NRF_PDM->GAINL = PDM_GAINL_GAINL_DefaultGain;
    NRF_PDM->GAINR = PDM_GAINR_GAINR_DefaultGain;
    NRF_PDM->SAMPLE.PTR = (uint32_t)sampleA;
    NRF_PDM->SAMPLE.MAXCNT = DMA_SAMPLES;
    NRF_PDM->EVENTS_END = 0;
    NRF_PDM->INTENSET = PDM_INTENSET_END_Msk;

    NVIC_ClearPendingIRQ(PDM_IRQn);
    NVIC_SetPriority(PDM_IRQn, 1);
    NVIC_EnableIRQ(PDM_IRQn);

    running = true;
    NRF_PDM->ENABLE = PDM_ENABLE_ENABLE_Enabled << PDM_ENABLE_ENABLE_Pos;
    NRF_PDM->TASKS_START = 1;
#endif
}

//%
void stop() {
#if BUILTIN_MIC_HAS_PDM
    if (running) {
        NRF_PDM->TASKS_STOP = 1;
        NVIC_DisableIRQ(PDM_IRQn);
        NRF_PDM->INTENCLR = PDM_INTENCLR_END_Msk;
        NRF_PDM->ENABLE = PDM_ENABLE_ENABLE_Disabled << PDM_ENABLE_ENABLE_Pos;
    }
#endif
    running = false;
}

//%
int readPcmIntoBuffer(Buffer buffer) {
#if BUILTIN_MIC_HAS_PDM
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

#if BUILTIN_MIC_HAS_PDM
extern "C" void PDM_IRQHandler(void) {
    if (NRF_PDM->EVENTS_END) {
        NRF_PDM->EVENTS_END = 0;
        if (i2sMicrophone::running) {
            i2sMicrophone::completedSampleBuffer = i2sMicrophone::currentSampleBuffer;
            i2sMicrophone::currentSampleBuffer = i2sMicrophone::currentSampleBuffer == 0 ? 1 : 0;
            NRF_PDM->SAMPLE.PTR = (uint32_t)(i2sMicrophone::currentSampleBuffer == 0
                ? i2sMicrophone::sampleA
                : i2sMicrophone::sampleB);
            NRF_PDM->SAMPLE.MAXCNT = i2sMicrophone::DMA_SAMPLES;
        }
    }
}
#endif
