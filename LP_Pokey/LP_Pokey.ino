// Pokey like oscillator + LP filter //

#define SAMPLE_RATE 44100
#define SPEAKER_PIN 11
#define BPM         120

volatile uint8_t outputvalue = 127;

uint16_t pokey_divider = 200;
uint16_t pokey_cnt = 0;
uint8_t  pokey_bit = 0;
uint8_t  pokey_volume = 15;

uint16_t lfsr = 0xACE1;
uint16_t noise_div = 50;
uint16_t noise_cnt = 0;

volatile int16_t lp_out = 0;
uint8_t lp_alpha = 0;

uint8_t calcLPAlpha(float cutoffHz, float samplerate) {
  float rc = 1.0 / (2.0 * 3.14159265 * cutoffHz);
  float dt = 1.0 / samplerate;
  float alpha = dt / (rc + dt);
  return (uint8_t)(alpha * 255.0);
}

ISR(TIMER1_COMPA_vect) {

  pokey_cnt++;
  if (pokey_cnt >= pokey_divider) {
    pokey_cnt = 0;
    pokey_bit ^= 1;
  }

  noise_cnt++;
  if (noise_cnt >= noise_div) {
    noise_cnt = 0;
    uint16_t bit = ((lfsr >> 0) ^ (lfsr >> 5)) & 1;
    lfsr = (lfsr >> 1) | (bit << 15);
  }

  int16_t tone = pokey_bit ? 127 : -127;
  int16_t noise = (lfsr & 1) ? 127 : -127;

  int16_t raw = (tone + (noise >> 1)) >> 1;
  raw = (raw * pokey_volume) >> 4;

  lp_out = lp_out + (((raw - lp_out) * lp_alpha) >> 8);

  outputvalue = 127 + lp_out;
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

  lp_alpha = calcLPAlpha(300.0, SAMPLE_RATE);

}

void loop() {

  pokey_divider = random(20, 250);
  noise_div = random(10, 80);

  int tempo = 60000 / BPM;
  delay(tempo / 3);

}