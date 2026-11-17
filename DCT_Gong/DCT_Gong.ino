// Inverse Discrete Cosine Transform - Spectral gong engine //

#define SPEAKER_PIN 11
#define SAMPLE_RATE 31372

#define LOG2_N 7
#define N (1 << LOG2_N)

static int16_t sin_table_q15[257];
unsigned long last_decay = 0;

int16_t get_cos_q15(uint16_t idx) {

  uint16_t norm_idx = (idx * 1024) >> LOG2_N;
  norm_idx = (norm_idx + 256) & 1023;

  uint8_t quadrant = norm_idx >> 8;
  uint16_t table_pos = norm_idx & 255;

  if (quadrant == 0)      { return sin_table_q15[table_pos]; }
  else if (quadrant == 1) { return sin_table_q15[256 - table_pos]; }
  else if (quadrant == 2) { return -sin_table_q15[table_pos]; }
  else                    { return -sin_table_q15[256 - table_pos]; }

}

inline uint16_t reverse_bits(uint16_t x, uint8_t bits) {

  uint16_t rev = 0;
  for (uint8_t i = 0; i < bits; i++) {
    if (x & (1 << i)) {
    rev |= (1 << ((bits - 1) - i));
    }
  }
  return rev;

}


static uint8_t audio_buffer_0[N];
static uint8_t audio_buffer_1[N];

volatile uint8_t active_buffer = 0;
volatile int sample_index = 0;
volatile bool buffer_needs_calc = true;

volatile int16_t bin_amplitudes[N] = {0};
uint32_t phase_accumulator[N] = {0};

void idct_fast(int16_t* data) {

  for (int i = 0; i < N; i++) {
    int j = reverse_bits(i, LOG2_N);
    if (i < j) {
      int16_t tmp = data[i];
      data[i] = data[j];
      data[j] = tmp;
    }
  }

  for (int step = 1; step < N; step <<= 1) {
    int jump = step << 1;
    int delta = N / jump;

    for (int group = 0; group < step; group++) {
      int16_t w_re = get_cos_q15(group * delta);

      for (int pair = group; pair < N; pair += jump) {
        int match = pair + step;
        int32_t tr = ((int32_t)data[match] * w_re) >> 15;
        data[match] = data[pair] - tr;
        data[pair] = data[pair] + tr;
      }
    }
  }

}

int32_t acc1 = 0;
int32_t acc2 = 0;

void calculate_next(uint8_t* out_buffer) {

  static int16_t real_q15[N];

  for (int i = 0; i < N; i++) {
    real_q15[i] = 0;
  }

  for (int bin = 0; bin < N; bin++) {

    int16_t amp = bin_amplitudes[bin];

    if (amp > 5) {
      phase_accumulator[bin] += (uint32_t)bin << (LOG2_N - 1);
      phase_accumulator[bin] &= 1023;
      int16_t cos_p = get_cos_q15(phase_accumulator[bin]);
      real_q15[bin] = ((int32_t)amp * cos_p) >> 15;
    }

  }

  idct_fast(real_q15);

  for (int i = 0; i < N; i++) {

    int16_t val = real_q15[i];
    if (val > 1900) val = 1900;
    else if (val < -1900) val = -1900;

    int32_t input = ((int32_t)val) << 12; 

    acc1 += input;
    acc2 += acc1;

    int8_t top = acc2 >> 16;
    int32_t feedback = ((int32_t)top) << 16;

    acc1 -= feedback;
    acc2 -= feedback;

    int16_t sample = 128 + top;
    if (sample < 0) sample = 0;
    else if (sample > 255) sample = 255;

    out_buffer[i] = (uint8_t)sample;

  }

}

void setup() {

  for (int i = 0; i < 256; i++) {

    float angle = (2.0f * M_PI * (float)i) / 1024.0f;
    sin_table_q15[i] = (int16_t)(sinf(angle) * 32767.0f);

  }

  sin_table_q15[256] = 32767;

  pinMode(SPEAKER_PIN, OUTPUT);

  TCCR2A = _BV(COM2A1) | _BV(WGM20);
  TCCR2B = _BV(CS20);
  OCR2A  = 128;
  TIMSK2 = _BV(TOIE2);

}

void loop() {

  if (random(500) < 1) {

    int density = random(20, 80);
    for (int i = 1; i < N; i++) {
      if (random(100) < density) {
        bin_amplitudes[i] = random(50, 650) / i;
      }
    }
    
  }

  if (millis() - last_decay >= 5) {
    last_decay = millis();

    for (int i = 0; i < N; i++) {
      if (bin_amplitudes[i] > 0) {
        bin_amplitudes[i] = (bin_amplitudes[i] * (N - i)) / N;
        if (bin_amplitudes[i] < 5) bin_amplitudes[i] = 0;
      }
    }
  }

  if (buffer_needs_calc) {
    if (active_buffer == 0) { calculate_next(audio_buffer_1); }
    else { calculate_next(audio_buffer_0); }
    buffer_needs_calc = false;
  }

}

ISR(TIMER2_OVF_vect) {

  if (active_buffer == 0) {
    OCR2A = audio_buffer_0[sample_index];
  } else {
    OCR2A = audio_buffer_1[sample_index];
  }

  sample_index++;

  if (sample_index >= N) {
    sample_index = 0;
    if (!buffer_needs_calc) {
      active_buffer = !active_buffer;
      buffer_needs_calc = true;
    }
  }

}