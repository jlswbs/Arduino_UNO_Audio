// Atari Pokey like generator //

#define SAMPLE_RATE 44100
#define SPEAKER_PIN 11

#define POKEY_CHANNELS 4

#define MODE_TONE  0
#define MODE_NOISE 1

typedef struct {
  uint16_t divider;
  uint16_t counter;
  uint8_t  output;
  uint8_t  volume;
  uint8_t  mode;
} pokey_chan_t;

pokey_chan_t ch[POKEY_CHANNELS];
uint16_t lfsr = 0xACE1;

const uint16_t scale[] = {
  120, 135, 150, 160, 180, 200, 225, 240,
  270, 300, 320, 360
};

#define SCALE_LEN (sizeof(scale)/sizeof(scale[0]))

uint32_t lastTick = 0;
uint32_t lastDrum = 0;

ISR(TIMER1_COMPA_vect) {

  int16_t mix = 0;

  for (uint8_t i = 0; i < POKEY_CHANNELS; i++) {

    ch[i].counter++;
    if (ch[i].counter >= ch[i].divider) {
      ch[i].counter = 0;

      if (ch[i].mode == MODE_TONE) {
        ch[i].output ^= 1;
      } else {
        uint16_t bit = ((lfsr >> 0) ^ (lfsr >> 5)) & 1;
        lfsr = (lfsr >> 1) | (bit << 15);
        ch[i].output = lfsr & 1;
      }
    }

    int16_t s = ch[i].output ? 127 : -127;
    mix += (s * ch[i].volume) >> 4;
  }

  if (mix > 127)  mix = 127;
  if (mix < -127) mix = -127;

  OCR2A = 127 + mix;

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

  ch[0] = (pokey_chan_t){ 200, 0, 0, 15, MODE_TONE  };
  ch[1] = (pokey_chan_t){ 300, 0, 0, 12, MODE_TONE  };
  ch[2] = (pokey_chan_t){  40, 0, 0,  8, MODE_NOISE };
  ch[3] = (pokey_chan_t){ 120, 0, 0, 10, MODE_NOISE };

}

void loop() {

  uint32_t now = millis();

  if (now - lastTick > 200) {
    lastTick = now;

    ch[0].divider = scale[random(SCALE_LEN)] / 2;
    ch[0].divider += (lfsr & 1);
    ch[0].volume  = random(7, 13);

    ch[1].divider = scale[random(0, SCALE_LEN / 2)] * 2;
    ch[1].volume  = random(5, 12);
  }

  if (now - lastDrum > 120) {
    lastDrum = now;

    ch[2].divider = random(20, 60);
    ch[2].volume  = random(0, 9);

    ch[3].divider = random(80, 200);
    ch[3].volume  = random(0, 9);
  }

}