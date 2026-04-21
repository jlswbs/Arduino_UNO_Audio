// 4 channel generative lofi Ambient music //

#define SPEAKER_PIN 11
#define SAMPLE_RATE 16000
#define BPM 68

volatile uint32_t t = 0;
volatile uint32_t step_t = 0;
uint32_t samples_per_step;

volatile uint16_t ph_kick = 0;
volatile uint16_t ph_acid = 0;
volatile uint16_t ph_fm   = 0;
volatile uint16_t ph_ring = 0;

volatile uint16_t inc_kick = 0;
volatile uint16_t inc_acid = 0;
volatile uint16_t inc_fm   = 0;
volatile uint16_t inc_ring = 0;

volatile uint8_t env_k = 0, env_h = 0, env_a = 0, env_f = 0, env_r = 0;
volatile uint8_t env_a_target = 0;
volatile uint8_t env_r_target = 0;

const uint16_t notes[] = {220, 247, 261, 293, 329, 349, 392, 440}; 

bool get_euclidean(uint8_t step, uint8_t pulses, uint8_t total, uint8_t offset) {
  return (((uint16_t)(step + offset) * pulses) % total) < pulses;
}

ISR(TIMER1_COMPA_vect) {
  
  t++;
  step_t++;

  if(env_k > 0) { 
    ph_kick += inc_kick + (env_k >> 2);
    env_k = env_k > 5 ? env_k - 5 : 0;
  }
  uint8_t duty = 48 + (env_k >> 3);
  int8_t kick = ((ph_kick & 0xFF) > duty) ? 32 : -32;
  kick = (int8_t)((int16_t)kick * env_k >> 7);

  if(env_h > 0) env_h--;
  static uint16_t lfsr = 0xACE1;
  lfsr = (lfsr >> 1) ^ ((lfsr & 1) ? 0xB400 : 0);
  int8_t hat = (lfsr & 0x01) ? (int8_t)(env_h >> 2) : (int8_t)-(env_h >> 2);

  ph_acid += inc_acid;
  uint8_t raw = (ph_acid >> 8) & 0xFF;
  uint8_t tri = (raw & 0x80) ? (0xFF - raw) : raw;
  int8_t acid = (int8_t)tri - 128;

  if(env_a > env_a_target) env_a -= 2;
  else if(env_a < env_a_target) env_a++;
  else if(env_a > 18) env_a--;

  acid = (int8_t)((int16_t)acid * env_a >> 9);

  ph_fm   += inc_fm   + (ph_acid >> env_f);
  ph_ring += inc_ring;

  int8_t fm_base   = (ph_fm   & 0x8000) ? 12 : -12;
  int8_t ring_base = (ph_ring & 0x8000) ? 12 : -12;
  int8_t ring = (int8_t)(((int16_t)fm_base * ring_base) >> 4);

  if(env_r > env_r_target) env_r -= 1;
  else if(env_r > 8) env_r--;

  ring = (int8_t)((int16_t)ring * env_r >> 6);

  int16_t mix = 128 + kick + hat + acid + ring;
  if(mix < 0) mix = 0;
  if(mix > 255) mix = 255;

  OCR2A = (uint8_t)mix;

}

void setup() {

  pinMode(SPEAKER_PIN, OUTPUT);
  samples_per_step = (uint32_t)SAMPLE_RATE * 60 / BPM / 4;
  
  TCCR2A = _BV(COM2A1) | _BV(WGM21) | _BV(WGM20);
  TCCR2B = _BV(CS20);
  
  cli();
  TCCR1A = 0;
  TCCR1B = _BV(WGM12) | _BV(CS10);
  OCR1A = (F_CPU / SAMPLE_RATE) - 1;
  TIMSK1 = _BV(OCIE1A);
  sei();

  randomSeed(analogRead(0));

}

void loop() {

  if(step_t >= samples_per_step) {
    step_t = 0;
    static uint16_t long_s = 0;
    long_s++;
    
    uint8_t s = long_s % 16;
    uint8_t phrase = (long_s / 16) % 8;
    uint8_t master_cycle = (long_s / 64);

    uint8_t k_pulses = (phrase == 3 || phrase == 7) ? 7 : 5; 
    if(random(0, 42) == 0) k_pulses = 2 + random(0, 11);

    if(get_euclidean(s, k_pulses, 16, 0)) { 
      env_k = 255; 
      ph_kick = 0;
      inc_kick = 1200 + random(0, 3600);
    }

    uint8_t h_rot = (master_cycle % 5); 
    if(get_euclidean(s, 9, 16, h_rot)) { 
      env_h = (s % 3 == 0) ? 150 : 75;
    }

    if(get_euclidean(s, 6, 16, phrase) && (long_s & 0x03) == 0) {
      uint16_t base_freq = notes[phrase % 8];
      
      inc_fm   = (base_freq * 65536ULL) / SAMPLE_RATE;
      inc_ring = (base_freq * 65536ULL) / SAMPLE_RATE >> 1;

      if(random(0, 11) == 0) {
        inc_fm   = inc_fm * (3 + random(0, 4)) >> 1;
        inc_ring = inc_ring * 3 >> 1;
      }

      env_f = 3 + random(0, 7);
      env_r = 225 + random(0, 30);
      env_r_target = 40 + random(0, 25);
    }

    uint8_t note_idx = (s + (phrase * 3)) % 8;
    uint8_t a_pulses = 3 + (phrase % 5); 

    if(get_euclidean(s, a_pulses, 16, master_cycle % 7)) {
      inc_acid = (notes[note_idx] * 65536ULL) / SAMPLE_RATE;
      
      if(random(0, 14) == 0) inc_acid >>= 1;

      env_a = 245;
      env_a_target = 48 + random(0, 35);
    }
  }

}