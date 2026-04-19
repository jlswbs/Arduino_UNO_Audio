// 4 channel lofi generative Drum machine //

#define SPEAKER_PIN 11
#define SAMPLE_RATE 16000
#define BPM 175

volatile uint32_t global_t = 0;
volatile uint32_t step_timer = 0;
uint32_t samples_per_step;

uint16_t noise_reg = 0xACE1;
uint32_t ph_kick, ph_fm;
uint8_t env_k, env_s, env_h, env_f;

uint8_t prob_k = 200;
uint8_t prob_s = 150;
uint8_t prob_h = 220;

ISR(TIMER1_COMPA_vect) {

  global_t++;
  step_timer++;

  if(env_k > 0) {
    ph_kick += (uint32_t)env_k << 2; 
    env_k--;
  }
  int8_t kick = (ph_kick & 0x80) ? 30 : -30;
  if(env_k < 100) kick = (int8_t)((kick * env_k) >> 7);

  if(env_s > 0) env_s--;
  noise_reg = (noise_reg >> 1) ^ (-(noise_reg & 1u) & 0xB001u);
  int8_t snare = (int8_t)((noise_reg & env_s) - (env_s >> 1));

  if(env_h > 0) env_h--;
  int8_t hat = (int8_t)(((noise_reg ^ (global_t << 4)) & env_h) >> 2);

  if(env_f > 0) {
    ph_fm += (global_t & 0x20) ? 400 : 150;
    env_f--;
  }
  int8_t perc = (ph_fm & 0x80) ? (int8_t)(env_f >> 2) : (int8_t)-(env_f >> 2);

  int16_t mixed = 128 + kick + snare + hat + perc;
  if(mixed < 0) mixed = 0;
  if(mixed > 255) mixed = 255;

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

}

void loop() {

  if(step_timer >= samples_per_step) {

    step_timer = 0;
    static uint8_t step_count = 0;
    step_count = (step_count + 1) & 0x0F;

    uint8_t r = rand() % 255;

    if(step_count % 4 == 0 || (r < 40)) {
      env_k = 255;
      ph_kick = 0;
    }

    if(step_count % 8 == 4 || (r > 230)) {
      env_s = 200;
    }

    if(step_count % 2 == 0 || (r > 200)) {
      env_h = (step_count % 4 == 2) ? 150 : 60;
    }

    if(r > 240 && step_count % 3 == 0) {
      env_f = 180;
    }
    
    if((global_t / samples_per_step) % 32 == 0) {
       prob_s = 100 + (rand() % 100);
    }
    
  }

}