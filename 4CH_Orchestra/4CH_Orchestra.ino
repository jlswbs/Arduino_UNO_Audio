// 4 channel generative Lofi Orchestra //

#define SPEAKER_PIN 11
#define SAMPLE_RATE 16000
#define BPM 60

volatile uint32_t t = 0;
volatile uint32_t step_t = 0;
uint32_t samples_per_step;

volatile uint16_t ph_kick, ph_acid, ph_fm;
volatile uint8_t env_k, env_h, env_a, env_f;
volatile uint16_t inc_acid, inc_fm;

const uint16_t notes[] = {220, 247, 261, 293, 329, 349, 392, 440}; 

bool get_euclidean(uint8_t step, uint8_t pulses, uint8_t total, uint8_t offset) {
  return (((uint16_t)(step + offset) * pulses) % total) < pulses;
}

ISR(TIMER1_COMPA_vect) {
  
  t++;
  step_t++;

  if(env_k > 0) { 
    ph_kick += (env_k >> 2); 
    env_k--; 
  }
  int8_t kick = (ph_kick & 0x80) ? 30 : -30;
  kick = (int8_t)((int16_t)kick * env_k >> 8);

  if(env_h > 0) env_h--;
  int8_t hat = ((t ^ (t >> 4) ^ (t >> 8)) & 0x01) ? (int8_t)(env_h >> 3) : (int8_t)-(env_h >> 3);

  ph_acid += inc_acid;
  uint8_t saw = (ph_acid >> 8) & 0xFF;
  int8_t acid = (int8_t)(saw ^ (saw >> (env_a >> 6)));
  if(env_a > 0) env_a--;
  acid = (int8_t)((int16_t)acid * env_a >> 9);

  ph_fm += inc_fm + (ph_acid >> env_f);
  int8_t fm = (ph_fm & 0x8000) ? 6 : -6;

  uint8_t mixed = 128 + kick + hat + acid + fm;
  if(mixed < 0) mixed = 0;
  if(mixed > 255) mixed = 255;

  OCR2A = (uint8_t)mixed;

}

void setup() {

  pinMode(SPEAKER_PIN, OUTPUT);
  samples_per_step = (uint32_t)8000 * 60 / BPM / 4;
  
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

  if(step_t >= samples_per_step) {
    step_t = 0;
    static uint16_t long_s = 0;
    long_s++;
    
    uint8_t s = long_s % 16;
    uint8_t phrase = (long_s / 16) % 4;
    uint8_t master_cycle = (long_s / 64);

    uint8_t k_pulses = (phrase == 3) ? 7 : 5; 
    if(get_euclidean(s, k_pulses, 16, 0)) { 
      env_k = 255; ph_kick = 0; 
    }

    uint8_t h_rot = (master_cycle % 3); 
    if(get_euclidean(s, 9, 16, h_rot)) { 
      env_h = (s % 4 == 0) ? 120 : 60;
    }

    if(get_euclidean(s, 6, 16, phrase) && (long_s & 0x02)) {
      env_f = 2 + rand() % 4;
      inc_fm = (notes[phrase % 8] * 65536ULL) / SAMPLE_RATE;
    }

    uint8_t note_idx = (s + (phrase * 2)) % 8;
    uint8_t a_pulses = 4 + (phrase % 4); 
    if(get_euclidean(s, a_pulses, 16, master_cycle % 8)) {
      inc_acid = (notes[note_idx] * 65536ULL) / SAMPLE_RATE;
      env_a = 240;
    }
  }

}