#include <Arduino.h>
#include <avr/sleep.h>
#include <Date.h>

#define OE 0
#define LCK 1
#define CE 2
#define SCK 3
#define DAT 4
#define RL A0

#define DIGITS 3

const Date TARGET{22, 6, 7};

const byte p[] = {
    0b11111100,
    0b01100000,
    0b11011010,
    0b11110010,
    0b01100110,
    0b10110110,
    0b10111110,
    0b11100000,
    0b11111110,
    0b11110110,
};

int read_DS1302();
void disp(int val);

void setup()
{
  set_sleep_mode(SLEEP_MODE_IDLE);
  pinMode(OE, OUTPUT);
  pinMode(LCK, OUTPUT);
  pinMode(CE, OUTPUT);
  pinMode(SCK, OUTPUT);

  TCCR0B = _BV(CS02) | _BV(CS00); // Prescaler clk / 1024
  TIMSK0 = _BV(TOIE0);            // Enable Timer 0 Overflow Interrupt

  sei();
}

void loop()
{
  sleep_mode();
}

ISR(TIM0_OVF_vect)
{
  int result = analogRead(RL);
  if (result > 200)
    disp(read_DS1302());
}

int read_DS1302()
{
  digitalWrite(SCK, HIGH);

  pinMode(DAT, OUTPUT);
  shiftOut(DAT, SCK, LSBFIRST, 0xBF);
  pinMode(DAT, INPUT);
  shiftIn(DAT, SCK, LSBFIRST);
  shiftIn(DAT, SCK, LSBFIRST);
  shiftIn(DAT, SCK, LSBFIRST);
  byte d = shiftIn(DAT, SCK, LSBFIRST);
  byte m = shiftIn(DAT, SCK, LSBFIRST);
  shiftIn(DAT, SCK, LSBFIRST);
  byte y = shiftIn(DAT, SCK, LSBFIRST);

  digitalWrite(SCK, LOW);

  return TARGET - Date{y, m, d};
}

void disp(int val)
{
  int dgts = DIGITS;
  digitalWrite(LCK, LOW);
  while (val)
  {
    shiftOut(DAT, SCK, LSBFIRST, p[val % 10]);
    val /= 10;
    dgts--;
  }
  while (dgts--)
    shiftOut(DAT, SCK, LSBFIRST, p[0]);
  digitalWrite(LCK, LOW);
}
