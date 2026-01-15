// DDS pulse density oscillator //

#define SAMPLE_RATE 44100
#define SPEAKER_PIN 11

volatile uint8_t outputvalue = 127;

struct oscillator {
  uint32_t phase;
  uint32_t phase_increment;
  uint16_t amplitude;
  uint8_t  density;
} o1;

uint32_t phaseinc(float freq) {
  return (uint32_t)(freq * (1UL << 24) / SAMPLE_RATE);
}

ISR(TIMER1_COMPA_vect) {

  uint8_t phase8 = o1.phase >> 16;

  uint8_t period = 256 - o1.density;
  int16_t s;

  if (((phase8 / period) & 1) == 0) s = 127;
  else s = -127;

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
  o1.density = 200;

}

void loop() {

}