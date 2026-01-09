// Relaxing noisy bells //

#define SAMPLE_RATE 44100
#define SPEAKER_PIN 11
#define LED_PIN     13
#define LED_PORT    PORTB
#define LED_BIT     5

float lpRain = 0.0f;
float bpRain = 0.0f;
float dropEnv = 0.0f;
float dropFreq = 0.0f;
int   dropTimer = 0;
float sh1 = 0.0f, sh2 = 0.0f;
float echoLP = 0.0f;

float RAIN_LP   = 0.025f;
float RAIN_BP   = 0.15f;
float DROP_DEC  = -0.995f;
float ECHO_LP   = 0.03f;
float ECHO_GAIN = 0.5f;

ISR(TIMER1_COMPA_vect) {

    float noise = ((float)rand() / RAND_MAX - 0.5f);
    float noise2 = noise * 255.0f;

    lpRain += (noise2 - lpRain) * RAIN_LP;
    bpRain += (lpRain - bpRain) * RAIN_BP;

    if (--dropTimer <= 0) {
        dropTimer = (rand() & 1500) + 256;
        dropEnv = 1.0f;
        sh1 = noise;
        sh2 = noise2;
        LED_PORT ^= (1 << LED_BIT);
    }

    dropEnv *= DROP_DEC;
    dropFreq += sh1;
    float drop = dropEnv * sinf(sh2 * dropFreq);
    float mix = lpRain * 0.7f + bpRain * 0.3f + drop * 10.0f;

    echoLP += (mix - echoLP) * ECHO_LP;
    mix += echoLP * ECHO_GAIN;

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