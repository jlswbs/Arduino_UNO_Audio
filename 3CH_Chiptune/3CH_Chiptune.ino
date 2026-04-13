// 3 channel chiptune generator //

#define SAMPLE_RATE 8000
#define SPEAKER_PIN 11
#define BPM 128
#define FIXED_SHIFT 16
#define FIXED_SCALE (1UL << FIXED_SHIFT)

const uint32_t STEP_LEN_SAMPLES = (uint32_t)SAMPLE_RATE * 60 / BPM / 4;

volatile uint32_t global_sample_count = 0;
volatile uint32_t phase_sq = 0;
volatile uint32_t phase_saw = 0;
volatile uint32_t incr_sq = 0;
volatile uint32_t incr_saw = 0;
volatile uint8_t current_step_type = 0;

uint32_t scale_incs[6];

uint16_t rng_state = 0xACE1;
uint8_t fast_random_8() {

  rng_state ^= rng_state << 7;
  rng_state ^= rng_state >> 9;
  rng_state ^= rng_state << 8;
  return rng_state & 0xFF;

}

const uint8_t decay_table[32] PROGMEM = {

  255, 247, 239, 231, 223, 215, 207, 199,
  191, 183, 175, 167, 159, 151, 143, 135,
  127, 119, 111, 103, 95, 87, 79, 71,
  63, 55, 47, 39, 31, 23, 15, 7

};

uint8_t fast_env_shift(uint32_t local_idx, uint8_t shift) {

  uint8_t idx = (local_idx >> shift) & 0x1F;
  return pgm_read_byte(&decay_table[idx]);

}

int8_t fast_noise_simple(uint8_t env_val, uint8_t attenuation) {

  if(env_val == 0) return 0;
  uint8_t noise_val = fast_random_8();
  uint8_t shaped = (noise_val & env_val) >> attenuation;
  return (int8_t)shaped - 128;

}

int8_t fast_noise_xor(uint8_t env_val, uint8_t attenuation) {

  if(env_val < 16) return 0;
  static uint8_t noise_phase = 0;
  noise_phase = (noise_phase << 1) ^ (noise_phase >> 7) ^ (rng_state & 1);
  uint8_t out = (noise_phase & env_val) >> attenuation;
  return (int8_t)out - 128;

}

ISR(TIMER1_COMPA_vect) {

  global_sample_count++;
  
  static const uint32_t step_mask = STEP_LEN_SAMPLES - 1;
  uint32_t local_idx = global_sample_count & step_mask;
  
  phase_sq += incr_sq;
  int8_t square = (phase_sq & 0x8000) ? 30 : -30;
  
  int8_t saw = 0;
  if((global_sample_count / STEP_LEN_SAMPLES) & 1) {
    phase_saw += incr_saw;
    saw = ((int8_t)(phase_saw >> 8)) >> 2;
  }
  
  int8_t noise = 0;
  if(current_step_type == 0) {
    uint8_t env_val = fast_env_shift(local_idx, 3);
    noise = fast_noise_simple(env_val, 2);
  } 
  else if(current_step_type == 2) {
    uint8_t env_val = fast_env_shift(local_idx, 1);
    noise = fast_noise_simple(env_val, 1);
  }
  
  int16_t mixed = 128 + square + saw + noise;
  
  if(mixed < 0) mixed = 0;
  if(mixed > 255) mixed = 255;
  
  OCR2A = (uint8_t)mixed;

}

void setup() {

  pinMode(SPEAKER_PIN, OUTPUT);
  
  const float freqs[] = {110.0, 130.81, 146.83, 164.81, 196.0, 220.0};

  for(uint8_t i = 0; i < 6; i++) scale_incs[i] = (uint32_t)((freqs[i] * FIXED_SCALE) / SAMPLE_RATE);
  
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

  uint32_t step_idx = global_sample_count / STEP_LEN_SAMPLES;
  static uint32_t last_step = 999;
  
  if(step_idx != last_step) {

    last_step = step_idx;
    
    uint8_t scale_idx = fast_random_8() % 6;
    uint8_t octave = fast_random_8() & 0x03;
    
    incr_sq = scale_incs[scale_idx] << octave;
    incr_saw = (octave > 0) ? (scale_incs[scale_idx] << (octave - 1)) : (scale_incs[scale_idx] >> 1);
    current_step_type = step_idx & 0x03;

  }

}