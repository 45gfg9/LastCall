#!/usr/bin/env python3

import shlex
import subprocess
import sys

if len(sys.argv) < 2:
    sys.exit(f'Usage: python3 {sys.argv[0]} <days>')

PORT = 'usb'
PROG = 'usbasp'
EDADR = 0
HFUSE = 0xFB
LFUSE = 0x25

scr = f"""
write eeprom {EDADR} {' '.join(map(str, int(sys.argv[1]).to_bytes(2, 'little')))}
write hfuse 0 {HFUSE}
write lfuse 0 {LFUSE}
quit
""".strip().encode()

subprocess.run(
    shlex.split(f'avrdude -c{PROG} -P{PORT} -pt13 -t'),
    input=scr
)
