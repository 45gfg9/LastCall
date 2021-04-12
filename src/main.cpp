#include <Arduino.h>
#include <EEPROM.h>
#include <avr/sleep.h>
#include <avr/power.h>

// 74HC595 Output Enable (LOW trigger)
#define OE PB1
// 74HC595 Register Lock (LOW trigger)
#define LCK PB3
// DS1302 CE pin (HIGH trigger)
#define CE PB4
// Shared SCK pin
#define SCK PB2
// Shared IO/DAT pin
#define DAT PB0

#define DIGITS 3

// Days In Months
const byte DIM[] PROGMEM {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

// 0~9 digits
// Bitwise NOTed for common anode
const byte p[] PROGMEM {
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

void run();

void setup() {
  DDRB = _BV(CE) | _BV(LCK) | _BV(SCK); // Set these pins to output
  digitalWrite(LCK, HIGH);

  bitSet(PCMSK, OE);   // INT on OE
  bitSet(GIMSK, PCIE); // Enable PCINT
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  power_all_disable(); // picoPower

  run();

  sleep_enable();
  sei();
}

void loop() {
  sleep_bod_disable();
  sleep_cpu(); // Minimize power
}

ISR(PCINT0_vect) {
  if (!digitalRead(1))
    run(); // Display on
}

byte bcd2bin(byte bcd) {
  return bcd - 6 * (bcd >> 4);
}

uint8_t shiftInMod(uint8_t dataPin, uint8_t clockPin) {
  uint8_t value = 0;

  for (uint8_t i = 8; i; i--) {
    value >>= 1;
    if (PINB & _BV(dataPin))
      value |= 0x80;
    PORTB |= _BV(clockPin);
    PORTB &= ~_BV(clockPin);
  }

  return value;
}

// reads DS1302 and calculates days left
word getDaysLeft() {
  // Pt I
  // retrieve data from DS1302

  digitalWrite(CE, HIGH);

  // Burst Read takes less space than
  // reading individual registers
  pinMode(DAT, OUTPUT);
  shiftOut(DAT, SCK, LSBFIRST, 0xBF);
  pinMode(DAT, INPUT);
  shiftInMod(DAT, SCK);                   // second
  shiftInMod(DAT, SCK);                   // minute
  shiftInMod(DAT, SCK);                   // hour
  byte d = bcd2bin(shiftInMod(DAT, SCK)); // date
  byte m = bcd2bin(shiftInMod(DAT, SCK)); // month
  shiftInMod(DAT, SCK);                   // day of week
  byte y = bcd2bin(shiftInMod(DAT, SCK)); // year

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

  word targetDays;
  EEPROM.get(EDADR, targetDays); // read config

  word left = targetDays - ds;

  return left > 0 ? left : 0;
}

// output val to display
// least significant digit first
void disp(word val) {
  pinMode(DAT, OUTPUT);
  digitalWrite(LCK, LOW);
  // %03d
  for (byte i = DIGITS; i; i--) {
    shiftOut(DAT, SCK, LSBFIRST, pgm_read_byte(p + (val % 10)));
    val /= 10;
  }
  digitalWrite(LCK, HIGH);
}

void run() {
  disp(getDaysLeft());
}
