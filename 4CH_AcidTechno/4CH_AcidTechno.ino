// 4 channel lofi Acid Techno generator //

#define SPEAKER_PIN 11
#define SAMPLE_RATE 8000
#define BPM 135

volatile uint32_t t = 0;
volatile uint32_t step_t = 0;
uint32_t samples_per_step;

volatile uint8_t karplus_buf[128];
volatile uint8_t kp_pos = 0;
volatile uint8_t kp_len = 64;
volatile uint8_t kp_last = 128;

volatile uint16_t ph_acid, ph_fm, ph_ring;
volatile uint8_t env_k, env_h, env_a, env_r;

volatile uint16_t inc_acid, inc_fm;

const uint16_t notes[] = {220, 247, 261, 293, 329, 349, 392, 440}; 

ISR(TIMER1_COMPA_vect) {
  
  t++;
  step_t++;

  int8_t karplus = 0;
  if(env_k > 0) {
    uint8_t read_val = karplus_buf[kp_pos];
    uint16_t filtered = ((uint16_t)read_val + kp_last) >> 1;
    karplus_buf[kp_pos] = (uint8_t)filtered;
    kp_last = read_val;

    karplus = (int8_t)(read_val - 128);
    karplus = (int8_t)((int16_t)karplus * env_k >> 11);

    kp_pos++;
    if(kp_pos >= kp_len) kp_pos = 0;
  }

  if(env_h > 0) env_h--;
  static uint16_t lfsr = 0xACE1;
  lfsr = (lfsr >> 1) ^ ((lfsr & 1) ? 0xB400 : 0);
  int8_t hat = (lfsr & 0x01) ? (int8_t)(env_h >> 2) : (int8_t)-(env_h >> 2);

  ph_acid += inc_acid;
  int8_t saw = ((ph_acid >> 8) & 0xFF) - 128;
  int8_t acid = (int8_t)(((int16_t)saw * saw * saw) >> 14);

  ph_fm += inc_fm + (ph_acid >> 8);
  ph_ring += inc_fm >> 1;

  int8_t fm_base = (ph_fm & 0x8000) ? 12 : -12;
  int8_t ring_base = (ph_ring & 0x8000) ? 12 : -12;
  int8_t ring = (int8_t)(((int16_t)fm_base * ring_base) >> 8);

  static uint8_t env_counter = 0;
  env_counter++;

  if(env_counter >= 8) {
    env_counter = 0;
    if(env_k > 0) env_k--;
  }

  static uint8_t acid_counter = 0;
  acid_counter++;
  if(acid_counter >= 10) {
    acid_counter = 0;
    if(env_a > 0) {
      if(env_a > 80) env_a -= 2;
      else if(env_a > 25) env_a -= 1;
      else env_a = (env_a > 1) ? env_a - 1 : 0;
    }
  }

  static uint8_t ring_counter = 0;
  ring_counter++;
  if(ring_counter >= 36) {
    ring_counter = 0;
    if(env_r > 0) {
      if(env_r > 100) env_r -= 2;
      else if(env_r > 40) env_r -= 1;
      else env_r = (env_r > 1) ? env_r - 1 : 0;
    }
  }

  if(env_a > 0) {
    acid = (int8_t)((int16_t)acid * env_a >> 5);
  } else {
    acid = 0;
  }

  if(env_r > 0) {
    ring = (int8_t)((int16_t)ring * env_r >> 5);
  } else {
    ring = 0;
  }

  int16_t mixed = 128 + karplus + hat + acid + ring;
  if(mixed > 255) mixed = 255;
  if(mixed < 0)   mixed = 0;

  OCR2A = (uint8_t)mixed;

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
    
    uint8_t step = long_s % 16;
    uint8_t bar  = (long_s >> 4) % 16;
    uint8_t phrase = bar % 8;

    if (step == 0 || step == 8 || (step == 4 && (bar % 3 == 0))) {
      env_k = 200;
      kp_len = SAMPLE_RATE / 110;
      for(uint8_t i = 0; i < 128; i++) {
        karplus_buf[i] = random(0, 256);
      }
      kp_pos = 0;
      kp_last = 128;
    }

    if (step % 2 == 0) {
      env_h = (step % 6 == 0) ? 160 : 95;
    }

    bool acid_trigger = false;
    
    if (step == 0 || step == 3 || step == 5 || step == 7 || 
        step == 10 || step == 12 || step == 14) {
      acid_trigger = true;
    }
    
    if (random(0, 6) == 0) acid_trigger = true;

    if (acid_trigger) {
      uint8_t note_idx = (step / 2 + phrase * 2) % 8;
      
      if ((bar % 4 == 0) && random(0, 3) == 0) {
        note_idx = (note_idx + 3) % 8;
      }

      inc_acid = (notes[note_idx] * 65536ULL) / SAMPLE_RATE;
      
      if (random(0, 4) == 0) inc_acid >>= 1;

      env_a = 255;
    }

    if (step == 2 || step == 6 || step == 11 || step == 15) {
      env_r = 235 + random(0, 20);
      uint8_t ring_note = (phrase + (step >> 2)) % 8;
      inc_fm = (notes[ring_note] * 65536ULL) / SAMPLE_RATE;
      
      if (random(0, 7) == 0) inc_fm <<= 1;
    }

    if ((long_s & 0x7F) == 0) {
      if (random(0, 3) == 0) {
      }
    }
  }

}