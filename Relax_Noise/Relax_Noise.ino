// Relaxing noisy glitch bells drone //

#define SPEAKER_PIN 11
#define LED_PIN     13
#define LED_PORT    PORTB
#define LED_BIT     5
#define SAMPLE_RATE 62500

int32_t acc = 0, acc2 = 0;

class Generator {

private:
    uint32_t rndm;
    int32_t lp1 = 0;
    int32_t lp2 = 0;
    int32_t bp2 = 0;
    int32_t sah1 = 0;
    int32_t sah2 = 0;
    int32_t sah3 = 0;
    int32_t sah4 = 0;
    int32_t lowns = 0;
    int32_t lfrt = 0;
    uint16_t droneP1 = 0;
    uint16_t droneP2 = 0;
    int16_t droneLp = 0;

    static const uint16_t REV_SIZE = 512;
    int16_t revBuf[REV_SIZE] = {0};
    uint16_t revIdx = 0;
    int16_t revLp = 0;

    int32_t nextRandom() {
        uint32_t bit = ((rndm >> 31) ^ (rndm >> 30) ^ (rndm >> 29) ^ (rndm >> 27) ^ (rndm >> 25) ^ rndm) & 1;
        rndm = (rndm << 1) | bit;
        return static_cast<int32_t>(rndm);
    }

    static inline int16_t fastTri(uint16_t p) {
        int16_t t = (p & 0x8000) ? (~p) : p;
        return (t - 16384);
    }

public:

    Generator(uint32_t seed = 0x12345678) : rndm(seed ? seed : 0x12345678) {}

    int16_t process() {

        int32_t r32 = nextRandom();
        int32_t r16 = r32 >> 16;

        if (((r32 >> 10) & 0x1FF) == 0) {
            sah3 = r32;
            LED_PORT ^= (1 << LED_BIT);
        }

        if (((r32 >> 10) & 0xFF) == 0) {
            sah4 = r16;
        }

        if (lfrt-- <= 0) {
            lfrt = (r32 & 2047) + 2048;
            sah1 = r16;
            sah2 = r32;
            LED_PORT ^= (1 << LED_BIT);
        }

        uint8_t shift1 = (sah2 >> 26) & 3;
        uint8_t shift2 = (sah3 >> 26) & 3;
        uint8_t shift3 = 1 + (sah4 >> 26) & 3;

        bp2 = (((sah1 >> 2) - (bp2 >> 9) - lp2) >> shift1) + bp2;
        lp2 = (bp2 >> shift3) + lp2;

        int32_t diff = r16 - lowns;
        lowns += (diff >> shift2) - (diff >> 4) + (r16 >> 5) - lowns;

        int32_t diff1 = (r16 >> 1) - lp1;
        lp1 += (diff1 >> 3) - (diff1 >> 6) - (lp1 + sah4);

        droneP1 += 461;
        droneP2 += 463;
        int16_t rawDrone = (fastTri(droneP1) + fastTri(droneP2)) >> 1;
        droneLp += (rawDrone - droneLp) >> 2;

        int32_t dry = (lp1 >> 4) + (bp2 >> 2) + (lowns >> 6) + (droneLp >> 1);

        int16_t delayed = revBuf[revIdx];
        revLp += (delayed - revLp) >> 1;

        int32_t fb = dry + (revLp >> 1);
        if (fb > 32767) fb = 32767;
        if (fb < -32768) fb = -32768;
        revBuf[revIdx] = static_cast<int16_t>(fb);
        revIdx = (revIdx + 1) & (REV_SIZE - 1);
        int32_t out = dry + (revLp >> 1);

        if (out > 32767) out = 32767;
        if (out < -32768) out = -32768;

        return static_cast<int16_t>(out);
    }

};

Generator rain(0x98765432);

ISR(TIMER2_OVF_vect) {

    static uint8_t divider = 0;
    static int32_t outputvalue = 0;

    if (++divider >= 2) {
        divider = 0;
        outputvalue = ((int32_t)rain.process()) << 16;
    }

    acc  += outputvalue;
    acc2 += acc;

    int8_t top = acc2 >> 24;
    int32_t feedback = ((int32_t)top) << 24;

    acc  -= feedback;
    acc2 -= feedback;

    OCR2A = 128 + top;

}

void setup() {

    pinMode(SPEAKER_PIN, OUTPUT);
    pinMode(LED_PIN, OUTPUT);

    TCCR2A = _BV(COM2A1) | _BV(WGM21) | _BV(WGM20);
    TCCR2B = _BV(CS20);
    OCR2A  = 128;

    TIMSK2 = _BV(TOIE2);

}

void loop() {

}