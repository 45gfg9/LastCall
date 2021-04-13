#!/usr/bin/env python3

import shlex
import subprocess
import sys

if len(sys.argv) < 2:
    sys.exit('Please specify days')

PROG = 'usbasp'
EDADR = 0

scr = f"""
write eeprom {EDADR} {' '.join(map(str, int(sys.argv[1]).to_bytes(2, 'little')))}
quit
""".strip().encode()

subprocess.run(
    shlex.split(f'avrdude -c{PROG} -pt13 -t'),
    input=scr
)
