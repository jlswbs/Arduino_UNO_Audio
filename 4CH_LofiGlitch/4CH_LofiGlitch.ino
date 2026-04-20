// 4 channel generative Lofi Glitch //

#define SPEAKER_PIN 11
#define SAMPLE_RATE 16000
#define BPM 72

volatile uint32_t t = 0;
volatile uint32_t step_t = 0;
uint32_t samples_per_step;

volatile uint16_t ph_kick, ph_acid, ph_fm;
volatile uint8_t env_k, env_h, env_a, env_f, env_g;

volatile uint16_t inc_acid, inc_fm;

const uint16_t notes[] = {220, 247, 261, 293, 329, 349, 392, 440}; 

bool get_euclidean(uint8_t step, uint8_t pulses, uint8_t total, uint8_t offset) {
  return (((uint16_t)(step + offset) * pulses) % total) < pulses;
}

ISR(TIMER1_COMPA_vect) {
  
  t++;
  step_t++;

  if(env_k > 0) { 
    ph_kick += (env_k >> 1);
    env_k--; 
  }
  int8_t kick = (ph_kick & 0x80) ? 30 : -30;
  kick = (int8_t)((int16_t)kick * env_k >> 8);

  if(env_h > 0) env_h--;
  int8_t hat = ((t ^ (t >> 3) ^ (t >> 7)) & 0x01) ? (int8_t)(env_h >> 2) : (int8_t)-(env_h >> 2);

  ph_acid += inc_acid;
  uint8_t saw = (ph_acid >> 8) & 0xFF;
  uint8_t shift = (env_a >> 6);
  if ((t & 0x1F) == 0) shift ^= 1;
  int8_t acid = (int8_t)(saw ^ (saw >> shift));
  if(env_a > 0) env_a--;
  acid = (int8_t)((int16_t)acid * env_a >> 9);

  ph_fm += inc_fm + (ph_acid >> env_f);
  int8_t fm = (ph_fm & 0x8000) ? 8 : -8;

  int8_t glitch = 0;
  if(env_g > 0) {
    env_g--;
    uint8_t gnoise = (t ^ (t >> 4) ^ (t >> 8) ^ (t >> 12)) & 0xFF;
    glitch = (gnoise & 0x80) ? (env_g >> 3) : -(env_g >> 3);
  }

  uint8_t mixed = 128 + kick + hat + acid + fm + glitch;
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
    if(random(0, 48) == 0) k_pulses = 3 + random(0, 9);

    if(get_euclidean(s, k_pulses, 16, 0)) { 
      env_k = 255; ph_kick = 0; 
    }

    uint8_t h_rot = (master_cycle % 5); 
    if(get_euclidean(s, 9, 16, h_rot)) { 
      env_h = (s % 4 == 0) ? 140 : 70;
    }

    if(get_euclidean(s, 6, 16, phrase) && (long_s & 0x02)) {
      env_f = 2 + random(0, 6);
      inc_fm = (notes[phrase % 8] * 65536ULL) / SAMPLE_RATE;
      if(random(0, 12) == 0) inc_fm += random(800, 3000);
    }

    uint8_t note_idx = (s + (phrase * 2)) % 8;
    if(random(0, 18) == 0) note_idx = random(0, 8);

    uint8_t a_pulses = 4 + (phrase % 4); 
    if(get_euclidean(s, a_pulses, 16, master_cycle % 8)) {
      inc_acid = (notes[note_idx] * 65536ULL) / SAMPLE_RATE;
      env_a = 240;
      if(random(0, 9) == 0) {
        inc_acid = random(1200, 8000) * 65536ULL / SAMPLE_RATE;
        env_a = 180;
      }
    }

    if(random(0, 22) == 0) {
      env_g = 160 + random(0, 95);
    }

    if(random(0, 64) == 0) {
    }
  }

}