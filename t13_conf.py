#!/usr/bin/env python3

import shlex
import subprocess
import sys
from datetime import date


if len(sys.argv) < 4:
    sys.exit(f'Usage: python3 {sys.argv[0]} <year> <month> <date>')

PORT = 'usb'
PROG = 'usbasp'
EDADR = 0
HFUSE = 0xFB
LFUSE = 0x25
BASE = date(2021, 1, 1)

try:
    target = date(*map(int, sys.argv[1:4]))
except ValueError as e:
    sys.exit(f'Invalid date: {e}')

days = (target - BASE).days

print(f'Target days: {days}')

scr = f"""
write eeprom {EDADR} {' '.join(map(str, days.to_bytes(2, 'little')))}
write hfuse 0 {HFUSE}
write lfuse 0 {LFUSE}
quit
""".strip().encode()

subprocess.run(
    shlex.split(f'avrdude -c{PROG} -P{PORT} -pt13 -t'),
    input=scr
)
