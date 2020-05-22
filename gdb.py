#!/usr/bin/env python
'''start gdb with symbols for the efi loader and the kernel'''
import os
import sys
import subprocess
import argparse


def main(argv):
    parser = argparse.ArgumentParser(prog=argv[0], description=__doc__)
    parser.add_argument(
        '--efi', '-e', action='store', type=str_hex_to_int,
        metavar='ADDR', help='efi loader base address')
    parser.add_argument(
        '--kernel', '-k', action='store_true',
        help='add symbols for the kernel')
    parser.add_argument(
        '--breakpoints', '-b', action='store',
        help='gdb source file (from save breakpoints)')

    if len(argv) == 1:
        print(parser.format_help())
        sys.exit(0)

    py_args = parser.parse_args(argv[1:])
    gdb_args = [
        'set height unlimited',  # no pagination
        'set confirm off',
        'set architecture i386:x86-64:intel',
    ]
    loader, debug_loader, kernel = \
        system('make print-debug-execs').splitlines()

    if py_args.efi is not None:
        text, data = system((rf"objdump -h {loader}"
                             r" | grep 'text\|data'"
                             r" | awk '{ print $4 }'")).splitlines()
        text = str_hex_to_int(text)
        data = str_hex_to_int(data)
        gdb_args.append(f'add-symbol-file {debug_loader}'
                        f' {hex(py_args.efi+text)}'
                        f' -s .data {hex(py_args.efi+data)}')

    if py_args.kernel:
        parts = ['add-symbol-file', kernel]
        cmd = (f'objdump -h {kernel}'
               ' | awk \'/^\\s*[0-9]/ { print $2 " " $4 }\'')
        kernel_base = 0xffffffffc0000000

        for line in system(cmd).splitlines():
            section, addr = line.split()
            addr = str_hex_to_int(addr)
            if not addr:
                continue
            addr += kernel_base

            if section == '.text':
                parts.insert(2, hex(addr))
            else:
                parts.extend(['-s', section, hex(addr)])

        gdb_args.append(' '.join(parts))

    gdb_args.append('target remote :1234')

    if py_args.breakpoints is not None:
        gdb_args.append(f'source {py_args.breakpoints}')

    gdb_args.append('set confirm on')
    args = []

    for arg in gdb_args:
        args.extend(['-ex', arg])

    execlp('gdb', 'gdb', *args)


def str_hex_to_int(str_hex):
    return int(str_hex[(2 if len(str_hex) > 2 and str_hex[1] == 'x' else 0):],
               base=16)


def system(cmd):
    print(cmd)
    proc = subprocess.run(cmd, shell=True, stdout=subprocess.PIPE)
    if proc.returncode:
        sys.exit(proc.returncode)
    return proc.stdout.decode()


def execlp(file, *args):
    print(' '.join(args))
    os.execlp(file, *args)


if __name__ == '__main__':
    main(sys.argv)
