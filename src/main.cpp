#include <avr/sleep.h>
#include <Arduino.h>
#include <EEPROM.h>

// 74HC595 Output Enable (LOW trigger)
#define OE 0
// 74HC595 Register Lock (HIGH trigger)
#define LCK 1
// DS1302 CE pin (HIGH trigger)
#define CE 2
// Shared SCK pin
#define SCK 3
// Shared IO/DAT pin
#define DAT 4
// Light sensor ADC (A0 is PB5)
#define RL A0

#define DIGITS 3

// Days In Months
const byte DIM[] PROGMEM{31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

// digits for 0~9 digits
const byte p[] PROGMEM{
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

const byte LIGHT_E2ADDR = 0x00;
const byte TOLERANT_E2ADDR = 0x02;
const byte DAYS_E2ADDR = 0x04;

word lightBase;
word lightTolerant;
word targetDays;

word curr;

// damn this takes 100B
word bcd2bin(word bcd) { return bcd - 6 * (bcd >> 4); }
word readDaysLeft();
void disp(int val);

void setup()
{
  // Only idle mode retains Overflow Interrupt
  set_sleep_mode(SLEEP_MODE_IDLE);

  // read config
  EEPROM.get(LIGHT_E2ADDR, lightBase);
  EEPROM.get(TOLERANT_E2ADDR, lightTolerant);
  EEPROM.get(DAYS_E2ADDR, targetDays);

  pinMode(OE, OUTPUT);
  pinMode(LCK, OUTPUT);
  pinMode(CE, OUTPUT);
  pinMode(SCK, OUTPUT);

  TCCR0B = _BV(CS02) | _BV(CS00); // Prescaler: clk / 1024
  TIMSK0 = _BV(TOIE0);            // Enable Timer 0 Overflow Interrupt

  sei(); // of course you would enable global interrupt
}

void loop()
{
  // Minimize power usage
  sleep_mode();
}

ISR(TIM0_OVF_vect)
{
  // runs about every 2s
  // time to update!

  word result = analogRead(RL);
  if (digitalRead(OE))
  {
    // TODO
  }
  disp(readDaysLeft());
}

// reads DS1302 and calculates days left
word readDaysLeft()
{
  // Pt I
  // retrieve data from DS1302

  digitalWrite(SCK, HIGH);

  // Burst Read takes less space than
  // reading individual registers
  pinMode(DAT, OUTPUT);
  shiftOut(DAT, SCK, LSBFIRST, 0xBF);
  pinMode(DAT, INPUT);
  shiftIn(DAT, SCK, LSBFIRST);                   // second
  shiftIn(DAT, SCK, LSBFIRST);                   // minute
  shiftIn(DAT, SCK, LSBFIRST);                   // hour
  byte d = bcd2bin(shiftIn(DAT, SCK, LSBFIRST)); // date
  byte m = bcd2bin(shiftIn(DAT, SCK, LSBFIRST)); // month
  shiftIn(DAT, SCK, LSBFIRST);                   // day of week
  byte y = bcd2bin(shiftIn(DAT, SCK, LSBFIRST)); // year

  digitalWrite(SCK, LOW);

  // Pt II
  // convert y/m/d to days since 2021.1.1
  // though it is easy (for you) to implement one,
  // but no leap handle to save precious flash
  // I just want it to work to 2022

  // this logic is very specific to base date (2021.1.1)
  // you may rewrite this part if your base is not 1.1
  // "I dont know why, it just works"
  word ds = -1;                       // begin with Jan 1
  if (y != 21)                        // if not (20)21
    for (; y > 21; y--)               // for each year
      ds += 365;                      // add extra 365 days
  for (m--; m > 0; m--)               // for each month unspent this year
    ds += pgm_read_byte(DIM + m - 1); // add days corresponding to month
  ds += d;                            // add remaining days (current month)

  return targetDays - ds;
}

// output val to display
// least significant digit first
void disp(int val)
{
  pinMode(DAT, OUTPUT);
  digitalWrite(LCK, LOW);
  // %03d
  for (byte i = 0; i < DIGITS; i++)
  {
    shiftOut(DAT, SCK, LSBFIRST, pgm_read_byte(p + (val % 10)));
    val /= 10;
  }
  digitalWrite(LCK, LOW);
}
