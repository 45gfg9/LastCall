#!/usr/bin/env python3

import shlex
import subprocess
import sys

if len(sys.argv) < 2:
    sys.exit(f'Usage: python3 {sys.argv[0]} <days> [port]')

PORT = sys.argv[2] if len(sys.argv) > 2 else 'usb'
PROG = 'usbasp'
EDADR = 0

scr = f"""
write eeprom {EDADR} {' '.join(map(str, int(sys.argv[1]).to_bytes(2, 'little')))}
quit
""".strip().encode()

subprocess.run(
    shlex.split(f'avrdude -c{PROG} -P{PORT} -pt13 -B4kHz -t'),
    input=scr
)
