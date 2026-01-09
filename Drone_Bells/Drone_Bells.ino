// Dark noise drone bells //

#define SAMPLE_RATE 44100
#define SPEAKER_PIN 11
#define LED_PIN     13
#define LED_PORT    PORTB
#define LED_BIT     5
#define SIZE        64

float lpRain = 0.0f;
float bpRain = 0.0f;
float dropEnv = 0.0f;
float dropFreq = 0.0f;
int   dropTimer = 0;
float sh1 = 0.0f;
float sh2 = 0.0f;
float sh3 = 0.0f;
float echoLP = 0.0f;

float outKs = 0.0f;
float last = 0.0f;
float curr = 0.0f;
float tmp = 0.0f;
float delaymem[SIZE];
uint8_t locat = 0;
uint8_t bound = SIZE;

float RAIN_LP   = 0.025f;
float RAIN_BP   = 0.25f;
float DROP_DEC  = 0.998f;
float ECHO_LP   = 0.04f;
float ECHO_GAIN = 0.6f;

uint32_t rng = 1;
int lfrt = 0;

inline float fastNoise(){

    rng = rng * 1664525UL + 1013904223UL;
    return ((int32_t)rng) * (1.0f / 2147483648.0f);

}

ISR(TIMER1_COMPA_vect) {

    float noise = ((float)rand() / RAND_MAX - 0.5f);
    float noise2 = noise * 255.0f;

    lpRain += (noise2 - lpRain) * RAIN_LP;
    bpRain += (lpRain - bpRain) * RAIN_BP;

    if (--dropTimer <= 0) {
        dropTimer = (rand() & 2048) + 512;
        if (sh2 <= 50) dropEnv = 1.0f;
        sh1 = noise;
        sh2 = noise2;
        sh3 = 0.5f + fabs(sh1);
        if (sh2 >= 60) for (int i = 0; i < SIZE; i++) delaymem[i] = fastNoise();
        LED_PORT ^= (1 << LED_BIT);
    }

    delaymem[locat++] = -outKs;
    if (locat >= bound) locat = 0;
    curr = delaymem[locat];
    outKs = 0.5f * (last + curr - echoLP);
    last = curr;

    dropEnv *= DROP_DEC;
    dropFreq += sh1;
    float drop = dropEnv * sinf(sh2 * dropFreq);
    float mix = outKs * 15.0f + lpRain * sh3 + bpRain + drop * 5.0f;

    echoLP += (mix - echoLP) * ECHO_LP;
    mix += echoLP * ECHO_GAIN;
    mix *= 1.7f;

    int sample = (int)(mix + 128.0f);
    if (sample < 0)   sample = 0;
    if (sample > 255) sample = 255;

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