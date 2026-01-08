// DDS square oscillator //

#define SAMPLE_RATE 44100
#define SPEAKER_PIN 11

volatile uint8_t outputvalue = 127;

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
  outputvalue = 127 + ((s * amp) >> 8);

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

}

void loop() {

}