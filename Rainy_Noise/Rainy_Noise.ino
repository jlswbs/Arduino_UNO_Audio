// Relaxing rainy noise drops //

#define SAMPLE_RATE 44100
#define SPEAKER_PIN 11
#define LED_PIN     13
#define LED_PORT    PORTB
#define LED_BIT     5

float lp1 = 0.0f;
float lp2 = 0.0f;
float bp2 = 0.0f;
float sah = 0.0f;
float sah2 = 0.0f;
float sah3 = 0.0f;
float lowns = 0.0f;
float sat = 0.95f;

uint32_t rng = 1;
int lfrt = 0;

inline float fastNoise(){

    rng = rng * 1664525UL + 1013904223UL;
    return ((int32_t)rng) * (1.0f / 2147483648.0f);

}

inline float rainNoiseStep() {

    float rndm = fastNoise();

    if (--lfrt <= 0)
    {
        lfrt = (rng % 3000) + 500;
        sah += (rndm - sah) * 0.15f;
        sah2 = 4.5f * fabs(rndm);
        sah3 = 0.5f * fabs(sah);
        LED_PORT ^= (1 << LED_BIT);
    }

    bp2 += (sah - bp2 * 0.25f - lp2) * 0.6f;
    lp2 += sah2 * bp2;

    lowns += (rndm - lowns) * 0.25f + rndm * 0.03f;
    lp1   += (rndm * sah3 - lp1) * 0.1f;

    float out = lp1 + bp2 + lowns * 0.05f;

    if (out > sat) out = sat;
    if (out < -sat) out = -sat;

    return out;

}

ISR(TIMER1_COMPA_vect) {

    float sample = 128.0f + (127.0f * rainNoiseStep());

    OCR2A = (uint8_t)sample;

}

void setup() {

    pinMode(SPEAKER_PIN, OUTPUT);
    pinMode(LED_PIN, OUTPUT);

    TCCR2A = _BV(COM2A1) | _BV(WGM21) | _BV(WGM20);
    TCCR2B = _BV(CS20);
    OCR2A = 128;

    cli();

    TCCR1A = 0;
    TCCR1B = _BV(WGM12) | _BV(CS10);
    OCR1A = (F_CPU / SAMPLE_RATE) - 1;
    TIMSK1 = _BV(OCIE1A);
    
    sei();

}

void loop() {

}