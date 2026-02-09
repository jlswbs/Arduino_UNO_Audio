// Glitch mangler base a FFT/IFFT synthesis //

#include "fix_fft.h"
#include "table.h"

#define SPEAKER_PIN 11

#define FFT   64
#define SIZE  2048
#define BPM   120

char vReal[FFT];
char vImag[FFT];
uint16_t table_ptr = 0;
unsigned long trig = 0;
uint16_t pitch = 0;
uint16_t glitch = 1;
bool inverse = false;

unsigned long mstime = 60000UL / (uint32_t)BPM / 2UL;

void setup() {

  pinMode(SPEAKER_PIN, OUTPUT);
  TCCR2A = _BV(COM2A1) | _BV(WGM21) | _BV(WGM20);
  TCCR2B = _BV(CS20);
  OCR2A = 127;

}

void loop() {

  if (millis() - trig >= mstime) {

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

  table_ptr = (table_ptr + glitch) % SIZE;

  fix_fft(vReal, vImag, 6, inverse);

  for (int i = 0; i < FFT; i++) {

    int val = vReal[i] << 2;
    OCR2A = 128 + val;
    if (pitch > 0) delayMicroseconds(pitch);

  }

}