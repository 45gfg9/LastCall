#include <avr/sleep.h>
#include <Arduino.h>
#include <EEPROM.h>

// 74HC595 Register Lock (LOW trigger)
#define LCK 3
// DS1302 CE pin (HIGH trigger)
#define CE 4
// Shared SCK pin
#define SCK 2
// Shared IO/DAT pin
#define DAT 0

#define DIGITS 3

// Days In Months
const byte DIM[] PROGMEM{31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

// 0~9 digits
// Bitwise NOTed for common anode
const byte p[] PROGMEM{
    (byte)~0b11111100,
    (byte)~0b01100000,
    (byte)~0b11011010,
    (byte)~0b11110010,
    (byte)~0b01100110,
    (byte)~0b10110110,
    (byte)~0b10111110,
    (byte)~0b11100000,
    (byte)~0b11111110,
    (byte)~0b11110110,
};

word targetDays;

volatile word curr = -1;

byte __attribute__((noinline)) bcd2bin(byte bcd) { return bcd - 6 * (bcd >> 4); }

word getDaysLeft();
void disp(int val);

void setup()
{
  pinMode(SCK, OUTPUT);
  pinMode(LCK, OUTPUT);
  pinMode(CE, OUTPUT);

  digitalWrite(LCK, HIGH);
  digitalWrite(CE, LOW);

  // read config
  EEPROM.get(EDADR, targetDays);

  // Only idle mode retains Overflow Interrupt
  set_sleep_mode(SLEEP_MODE_IDLE);

  TCCR0B = _BV(CS02) | _BV(CS00); // Prescaler: clk / 1024
  TIMSK0 = _BV(TOIE0);            // Enable Timer 0 Overflow Interrupt

  sleep_enable();
  sei(); // of course you would enable global interrupt
}

void loop()
{
  // Minimize power usage
  sleep_cpu();
}

ISR(TIM0_OVF_vect)
{
  // runs about every 2s
  // time to update!
  word left = getDaysLeft();
  if (curr != left)
  {
    curr = left;
    disp(curr);
  }
}

// reads DS1302 and calculates days left
word getDaysLeft()
{
  // Pt I
  // retrieve data from DS1302

  digitalWrite(CE, HIGH);

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

  digitalWrite(CE, LOW);

  // Pt II
  // convert y/m/d to days since 2021.1.1
  // no leap handle to save precious flash
  // I just want it to work to 2022

  // this logic is very specific to base date (2021.1.1)
  // rewrite this part if you have a different base date
  // "I don't know why, it just works"
  word ds = -1;                       // begin with Jan 1
  if (y != 21)                        // if not (20)21
    for (; y > 21; y--)               // for each year
      ds += 365;                      // add extra 365 days
  for (m--; m; m--)                   // for each month unspent this year
    ds += pgm_read_byte(DIM + m - 1); // add days corresponding to month
  ds += d;                            // add remaining days (current month)

  word left = targetDays - ds;

  return left > 0 ? left : 0;
}

// output val to display
// least significant digit first
void disp(int val)
{
  pinMode(DAT, OUTPUT);
  digitalWrite(LCK, LOW);
  // %03d
  for (byte i = DIGITS; i; i--)
  {
    shiftOut(DAT, SCK, LSBFIRST, pgm_read_byte(p + (val % 10)));
    val /= 10;
  }
  digitalWrite(LCK, HIGH);
}
