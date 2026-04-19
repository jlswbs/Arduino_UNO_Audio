// 3 channel Arp-Acid generator //

#define SAMPLE_RATE 8000
#define SPEAKER_PIN 11
#define BPM 140
#define FIXED_SHIFT 16
#define FIXED_SCALE (1UL << FIXED_SHIFT)

const uint32_t STEP_LEN_SAMPLES = (uint32_t)SAMPLE_RATE * 60 / BPM / 4;

volatile uint32_t global_sample_count = 0;
volatile uint32_t ph_lead = 0, ph_bass = 0;
volatile uint32_t inc_lead1 = 0, inc_lead2 = 0, inc_bass = 0;
volatile uint8_t cur_step = 0;

const uint8_t decay_table[32] PROGMEM = {

  255, 220, 190, 160, 135, 115, 100, 85,
  70, 60, 50, 42, 35, 30, 25, 20,
  16, 12, 10, 8, 6, 5, 4, 3,
  2, 1, 1, 0, 0, 0, 0, 0
  
};

uint32_t scale[6]; 
uint16_t rng = 0xACE1;

ISR(TIMER1_COMPA_vect) {

  global_sample_count++;
  uint32_t local_idx = global_sample_count % STEP_LEN_SAMPLES;
  
  ph_lead += ((global_sample_count >> 5) & 1) ? inc_lead1 : inc_lead2;
  int8_t lead = (ph_lead & 0x8000) ? 20 : -20;
  uint8_t lead_env = pgm_read_byte(&decay_table[(local_idx >> 7) & 0x1F]);
  lead = (int8_t)((lead * lead_env) >> 8);

  ph_bass += inc_bass;
  uint16_t pd_phase = ph_bass + ((ph_bass & 0x8000) ? 0x2000 : 0);
  int8_t bass = (int8_t)((pd_phase >> 8) - 128);
  bass >>= 2;

  int8_t drum = 0;
  if (cur_step % 4 == 0) {
    uint8_t k_env = pgm_read_byte(&decay_table[(local_idx >> 5) & 0x1F]);
    drum = (global_sample_count & (0x8000 >> (local_idx >> 8))) ? 30 : -30;
    drum = (int8_t)((drum * k_env) >> 8);
  } else if (cur_step % 2 == 1) {
    if (local_idx < 300) drum = (rng ^= (rng << 7), rng >> 8) & 0x1F;
  }

  int16_t mix = 128 + lead + bass + drum;
  OCR2A = (uint8_t)(mix < 0 ? 0 : (mix > 255 ? 255 : mix));

}

void setup() {

  pinMode(SPEAKER_PIN, OUTPUT);
  float f[] = {82.41, 98.00, 110.00, 123.47, 146.83, 164.81};
  for(uint8_t i=0; i<6; i++) scale[i] = (uint32_t)((f[i] * FIXED_SCALE) / SAMPLE_RATE);

  TCCR2A = _BV(COM2A1) | _BV(WGM21) | _BV(WGM20);
  TCCR2B = _BV(CS20);
  cli();
  TCCR1A = 0; TCCR1B = _BV(WGM12) | _BV(CS10);
  OCR1A = (F_CPU / SAMPLE_RATE) - 1;
  TIMSK1 = _BV(OCIE1A);
  sei();

}

void loop() {

  static uint32_t last_s = 999;
  uint32_t s = global_sample_count / STEP_LEN_SAMPLES;
  
  if(s != last_s) {

    last_s = s;
    cur_step = s % 16;
    uint8_t bar = (s / 16) % 4;

    uint8_t note1 = (cur_step * 3 + bar) % 6;
    uint8_t note2 = (cur_step * 2 ^ bar) % 6;
    
    inc_lead1 = scale[note1] << 3;
    inc_lead2 = scale[note2] << 2;
    
    uint8_t bass_note = (bar < 2) ? 0 : 2; 
    inc_bass = scale[bass_note] << 0;
    
    if (cur_step == 8) inc_bass <<= 1;

  }

}