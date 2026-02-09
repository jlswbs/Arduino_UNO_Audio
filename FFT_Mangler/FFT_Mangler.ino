// Glitch mangler base a FFT/IFFT synthesis //

#include "fix_fft.h"
#include "table.h"

#define FFT   64
#define SIZE  2048
#define TIME  120*128

char vReal[FFT];
char vImag[FFT];
uint16_t table_ptr = 0;
unsigned long trig = 0;
uint16_t pitch = 0;
uint16_t glitch = 1;
bool inverse = false;

void setup() {

  pinMode(6, OUTPUT);
  TCCR0A = _BV(COM0A1) | _BV(WGM01) | _BV(WGM00);
  TCCR0B = _BV(CS00);

}

void loop() {

  if (millis() - trig >= TIME) {

    table_ptr = 0;
    pitch = random(0, 150);
    glitch = random(0, 8);
    trig = millis();

  }

  for (int i = 0; i < FFT; i++) {

    uint8_t idx = (table_ptr + i) % SIZE;
    vReal[i] = pgm_read_byte(&wavetable_real[idx]);
    vImag[i] = pgm_read_byte(&wavetable_imag[idx]);
  
  }

  table_ptr += glitch;
  if (table_ptr >= SIZE) table_ptr = SIZE;

  fix_fft(vReal, vImag, 6, inverse);

  for (int i = 0; i < FFT; i++) {

    int val = vReal[i] << 2;
    OCR0A = 128 + val;
    delayMicroseconds(pitch);

  }

}