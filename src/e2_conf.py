#!/usr/bin/env python3

import argparse
import shlex
import subprocess

if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        description='LastCall MCU config'
    )
    parser.add_argument('-d', '--days', type=int, dest='d')
    parser.add_argument('-l', '--light', type=int, dest='l')

    args = parser.parse_args()

    cmd = shlex.split('avrdude -c usbasp -p t13')
    if args.d:
        cmd.append('-Ueeprom:w:')
