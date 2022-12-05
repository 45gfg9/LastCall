#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <avr/power.h>
#include <avr/pgmspace.h>
#include <avr/sleep.h>

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

#define bit_set(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#define bit_clear(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))

// Days In Months
const uint8_t DIM[] PROGMEM = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

// 0~9 digits
// Bitwise NOTed for common anode
const uint8_t p[] PROGMEM = {
    (uint8_t)~0b11111100,
    (uint8_t)~0b01100000,
    (uint8_t)~0b11011010,
    (uint8_t)~0b11110010,
    (uint8_t)~0b01100110,
    (uint8_t)~0b10110110,
    (uint8_t)~0b10111110,
    (uint8_t)~0b11100000,
    (uint8_t)~0b11111110,
    (uint8_t)~0b11110110,
};

uint8_t bcd2bin(uint8_t bcd) {
  return bcd - 6 * (bcd >> 4);
}

void shift_out(uint8_t value) {
  for (uint8_t i = 8; i; i--) {
    if (value & 1)
      bit_set(PORTB, DAT); // DAT high
    else
      bit_clear(PORTB, DAT); // DAT low
    bit_set(PORTB, SCK);     // SCK high
    bit_clear(PORTB, SCK);   // SCK low
    value >>= 1;
  }
}

uint8_t shift_in() {
  uint8_t value = 0;

  for (uint8_t i = 8; i; i--) {
    value >>= 1;
    bit_set(PORTB, SCK);       // SCK high
    if (bit_is_set(PINB, DAT)) // DAT is high
      value |= _BV(7);
    bit_clear(PORTB, SCK); // SCK low
  }

  return value;
}

void run() {
  // Pt I
  // retrieve data from DS1302

  bit_set(PORTB, CE); // CE high

  // Burst Read takes less space than
  // reading individual registers
  // DAT set to output before
  shift_out(0xBF);

  bit_clear(DDRB, DAT); // DAT in

  shift_in();                      // second
  shift_in();                      // minute
  shift_in();                      // hour
  uint8_t d = bcd2bin(shift_in()); // date
  uint8_t m = bcd2bin(shift_in()); // month
  shift_in();                      // day of week
  uint8_t y = bcd2bin(shift_in()); // year

  bit_clear(PORTB, CE); // CE low

  // Pt II
  // convert y/m/d to days since 2021.1.1

  y -= 21; // travel 21 years
  if (m <= 2 || y % 4)
    d--; // equivalent to add 1 day if leap
  while (--m)
    d += pgm_read_byte(DIM + m - 1); // add unspent months
  d += 365 * y + (y + 3) / 4;        // add years and leap days

  const uint16_t target = eeprom_read_word((uint16_t *)EDADR); // read config

  int left = target - d;
  if (left < 0)
    left = 0;

  // Pt III
  // display in format %03d
  // least significant digit first

  bit_set(DDRB, DAT);    // DAT out
  bit_clear(PORTB, LCK); // LCK low
  for (uint8_t i = DIGITS; i; i--) {
    shift_out(pgm_read_byte(p + (left % 10)));
    left /= 10;
  }
  bit_set(PORTB, LCK); // LCK high
}

int main() {
  DDRB = _BV(CE) | _BV(LCK) | _BV(SCK) | _BV(DAT); // Set these pins to output

  bit_set(PORTB, LCK); // LCK high

  bit_set(PCMSK, OE);   // INT on OE
  bit_set(GIMSK, PCIE); // Enable PCINT
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  power_all_disable(); // picoPower

  run();

  sleep_enable();
  sei();

  for (;;) {
    sleep_bod_disable();
    sleep_cpu(); // Minimize power
  }
}

ISR(PCINT0_vect) {
  if (bit_is_clear(PINB, OE)) // OE is low
    run();                    // Display enabled
}
