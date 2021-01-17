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

#define E2_WORD(n) (sizeof(word) * (n))

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

#define E2_LIGHT E2_WORD(0)
#define E2_TOLERANT E2_WORD(1)
#define E2_DAYS E2_WORD(2)

word lightBase;
word lightTolerant;
word targetDays;

volatile word curr = -1;

word __attribute__((noinline)) bcd2bin(word bcd) { return bcd - 6 * (bcd >> 4); }

word getDaysLeft();
void disp(int val);

void setup()
{
  // Only idle mode retains Overflow Interrupt
  set_sleep_mode(SLEEP_MODE_IDLE);

  // read config
  EEPROM.get(E2_LIGHT, lightBase);
  EEPROM.get(E2_TOLERANT, lightTolerant);
  EEPROM.get(E2_DAYS, targetDays);

  pinMode(OE, OUTPUT);
  pinMode(LCK, OUTPUT);
  pinMode(CE, OUTPUT);
  pinMode(SCK, OUTPUT);

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
  cli();
  // runs about every 2s
  // time to update!

  word lval = analogRead(RL);

  // if OE == HIGH threshold = base - tolerant
  // else threshold = base + tolerant
  digitalWrite(OE,
               (lval > lightBase + (digitalRead(OE) ? -lightTolerant : lightTolerant)));

  word left = getDaysLeft();
  if (curr != left)
  {
    curr = left;
    disp(curr);
  }

  sei();
}

// reads DS1302 and calculates days left
word getDaysLeft()
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
  digitalWrite(LCK, LOW);
}
