// Karplus-Strong with decay //

#define SAMPLE_RATE 22050
#define SPEAKER_PIN 11
#define BPM         120
#define BUFFER_SIZE 512

int8_t buffer[BUFFER_SIZE];
uint16_t bufSize = 100;
uint16_t bufIndex = 0;
uint8_t decay = 245; 

ISR(TIMER1_COMPA_vect) {

  uint16_t nextIndex = bufIndex + 1;
  if (nextIndex >= bufSize) nextIndex = 0;

  int16_t avg = ((int16_t)buffer[bufIndex] + (int16_t)buffer[nextIndex]) >> 1;
  avg = (avg * decay) >> 8;

  buffer[bufIndex] = (int8_t)avg;

  OCR2A = (uint8_t)(avg + 128);

  bufIndex = nextIndex;

}

void pluckString(uint16_t freq) {

  cli();
  
  bufSize = SAMPLE_RATE / freq;
  if (bufSize > BUFFER_SIZE) bufSize = BUFFER_SIZE;
  if (bufSize < 2) bufSize = 2;
  for (uint16_t i = 0; i < bufSize; i++) buffer[i] = random(-128, 127);
  bufIndex = 0;

  sei();

}

void setup() {

  pinMode(SPEAKER_PIN, OUTPUT);

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

  pluckString(random(110, 880));

  int tempo = 60000 / BPM;
  delay(tempo / 3);

}