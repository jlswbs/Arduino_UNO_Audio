// DDS random step oscillator //

#define SAMPLE_RATE 44100
#define SPEAKER_PIN 11

#define PHASE_BITS 24
#define PHASE_MASK ((1UL << PHASE_BITS) - 1)

volatile uint8_t outputvalue = 127;

struct oscillator {
  uint32_t phase;
  uint32_t phase_increment;
  uint16_t amplitude;
  uint8_t  step;
} o1;

static inline uint8_t fastRandom8() {
  static uint8_t x = 73;
  x ^= x << 3;
  x ^= x >> 5;
  x ^= x << 1;
  return x;
}

uint32_t phaseinc(float freq) {
  return (uint32_t)(freq * (1UL << 24) / SAMPLE_RATE);
}

ISR(TIMER1_COMPA_vect) {

  uint32_t prev_phase = o1.phase;
  o1.phase = (o1.phase + o1.phase_increment) & PHASE_MASK;

  if (o1.phase < prev_phase) o1.step = fastRandom8();

  int16_t s = (int16_t)o1.step - 128;

  uint8_t amp = o1.amplitude >> 8;
  outputvalue = 127 + ((s * amp) >> 8);

  OCR2A = outputvalue;

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

}

void loop() {

}