// Poly bit mix Byte-Beat generator //

#define SAMPLE_RATE 44100
#define SPEAKER_PIN 11

  long t = 0;

ISR(TIMER1_COMPA_vect) {

  uint8_t out_byte = 0;

  if ((t>>13|t%24)&(t>>7|t%19)) out_byte |= (1 << 7);
        
  if ((t<<0^t>>10)-(t|t>>9)) out_byte |= (1 << 6);
        
  if ((t*((t>>9|t>>13)&15))&128) out_byte |= (1 << 5);
        
  if (t*t>>7*((t>>11))&(t&t>>7)|(t%1>>3)) out_byte |= (1 << 4);

  if (((t>>1%128)+20)*t>>14*t>>16) out_byte |= (1 << 3);
        
  if ((t<<2^t>>10)-(t|t>>9)) out_byte |= (1 << 2);
        
  if (t*5&(t>>7)|t*3&(t*4>>10)) out_byte |= (1 << 1);

  if (t*((t>>12|t>>8)&63&t>>4)) out_byte |= (1 << 0);

  OCR2A = out_byte;

  t++;

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

}

void loop() {

}