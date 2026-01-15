// Square oscillator + LP filter //

#define SAMPLE_RATE 44100
#define SPEAKER_PIN 11

volatile uint8_t outputvalue = 127;
volatile int16_t lp_out = 0;
uint8_t lp_alpha = 0;

uint8_t calcLPAlpha(float cutoffHz, float samplerate) {
  float rc = 1.0 / (2.0 * 3.14159265 * cutoffHz);
  float dt = 1.0 / samplerate;
  float alpha = dt / (rc + dt);
  return (uint8_t)(alpha * 255.0);
}

struct oscillator {
  uint32_t phase;
  uint32_t phase_increment;
  uint16_t amplitude;
} o1;

uint32_t phaseinc(float freq) {
  return (uint32_t)(freq * (1UL << 24) / SAMPLE_RATE);
}

ISR(TIMER1_COMPA_vect) {

  int16_t s = (o1.phase & 0x00800000) ? 127 : -127;

  uint8_t amp = o1.amplitude >> 8;
  int16_t raw = ((s * amp) >> 8);

  lp_out = lp_out + (((raw - lp_out) * lp_alpha) >> 8);

  outputvalue = 127 + lp_out;
  OCR2A = outputvalue;

  o1.phase += o1.phase_increment;

}

void setup() {

  pinMode(SPEAKER_PIN, OUTPUT);

  TCCR2A = _BV(COM2A1) | _BV(WGM21) | _BV(WGM20);
  TCCR2B = _BV(CS20);
  OCR2A = 127;

  cli();

  TCCR1A = 0;
  TCCR1B = _BV(WGM12) | _BV(CS10);
  OCR1A  = (F_CPU / SAMPLE_RATE) - 1;
  TIMSK1 = _BV(OCIE1A);

  sei();

  o1.phase = 0;
  o1.phase_increment = phaseinc(220.0);
  o1.amplitude = 65535;
  lp_alpha = calcLPAlpha(150.0, SAMPLE_RATE);

}

void loop() {

}