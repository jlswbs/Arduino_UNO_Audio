// 3 channel lofi-FM chiptune generator //

#define SAMPLE_RATE 8000
#define SPEAKER_PIN 11
#define BPM 135
#define FIXED_SHIFT 16
#define FIXED_SCALE (1UL << FIXED_SHIFT)

const uint32_t STEP_LEN_SAMPLES = (uint32_t)SAMPLE_RATE * 60 / BPM / 4;

volatile uint32_t global_sample_count = 0;
volatile uint32_t phase_1 = 0;
volatile uint32_t phase_2 = 0;
volatile uint32_t incr_1 = 0;
volatile uint32_t incr_2 = 0;
volatile uint8_t drum_type = 0;

uint32_t scale_incs[6];
uint16_t rng_state = 0xACE1;

uint8_t fast_random_8() {

  rng_state ^= rng_state << 7;
  rng_state ^= rng_state >> 9;
  rng_state ^= rng_state << 8;
  return rng_state & 0xFF;
  
}

const uint8_t decay_table[32] PROGMEM = {

  255, 220, 190, 160, 135, 115, 100, 85,
  70, 60, 50, 42, 35, 30, 25, 20,
  16, 12, 10, 8, 6, 5, 4, 3,
  2, 1, 1, 0, 0, 0, 0, 0

};

ISR(TIMER1_COMPA_vect) {

  global_sample_count++;
  uint32_t local_idx = global_sample_count % STEP_LEN_SAMPLES;
  uint8_t env = pgm_read_byte(&decay_table[(local_idx >> 6) & 0x1F]);

  phase_1 += incr_1 + (phase_2 >> 8); 
  phase_2 += incr_2;
  
  int8_t tri = (int8_t)((phase_2 >> 8) ^ (phase_2 >> 15));
  int8_t osc1 = (phase_1 & 0x8000) ? 25 : -25;
  
  int8_t noise = 0;
  if (drum_type == 0) {
     noise = (local_idx < 400) ? (fast_random_8() & env) : 0;
  } else if (drum_type == 2) {
     noise = (fast_random_8() ^ (local_idx << 2)) & env;
     noise >>= 2;
  }

  int16_t mixed = 128 + osc1 + (tri >> 1) + (noise - 64);

  if(mixed < 0) mixed = 0;
  if(mixed > 255) mixed = 255;
  
  OCR2A = (uint8_t)mixed;

}

void setup() {

  pinMode(SPEAKER_PIN, OUTPUT);

  const float freqs[] = {73.42, 82.41, 98.00, 110.0, 123.47, 146.83};
  for(uint8_t i = 0; i < 6; i++) scale_incs[i] = (uint32_t)((freqs[i] * FIXED_SCALE) / SAMPLE_RATE);
  
  TCCR2A = _BV(COM2A1) | _BV(WGM21) | _BV(WGM20);
  TCCR2B = _BV(CS20);
  
  cli();
  TCCR1A = 0;
  TCCR1B = _BV(WGM12) | _BV(CS10);
  OCR1A = (F_CPU / SAMPLE_RATE) - 1;
  TIMSK1 = _BV(OCIE1A);
  sei();

}

void loop() {

  static uint32_t last_step = 999;
  uint32_t current_step = global_sample_count / STEP_LEN_SAMPLES;

  if(current_step != last_step) {

    last_step = current_step;
    
    uint8_t note_sel = (current_step ^ (current_step >> 2)) % 6;
    uint8_t oct = (current_step % 3) == 0 ? 2 : 1;
    
    incr_1 = scale_incs[note_sel] << oct;
    incr_2 = scale_incs[(note_sel + 2) % 8] << 1;
    
    drum_type = current_step & 0x03;

  }

}