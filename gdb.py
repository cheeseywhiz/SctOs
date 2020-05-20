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
        '--breakpoints', '-b', action='store', default='default-bp.gdb',
        help='gdb source file (from save breakpoints)')

    if len(argv) == 1:
        print(parser.format_help())
        sys.exit(0)

    py_args = parser.parse_args(argv[1:])
    print(py_args)

    args = [
        'set architecture i386:x86-64:intel',
        'set confirm off',
    ]
    loader, debug_loader, kernel = \
        system('make print-debug-execs').splitlines()

    if py_args.efi is not None:
        text, data = system((rf"objdump -h {loader}"
                             r" | grep 'text\|data'"
                             r" | awk '{ print $4 }'")).splitlines()
        text = str_hex_to_int(text)
        data = str_hex_to_int(data)
        args.append(f'add-symbol-file {debug_loader}'
                    f' {hex(py_args.efi+text)}'
                    f' -s .data {hex(py_args.efi+data)}')

    if py_args.kernel:
        text, = system((rf"objdump -h {kernel}"
                        r" | fgrep text"
                        r" | awk '{ print $4 }'")).splitlines()
        text = str_hex_to_int(text)
        kernel_base = 0xffffffffc0000000
        args.append(f'add-symbol-file {kernel} '
                    f' {hex(kernel_base+text)}')

    args.extend([
        'set confirm on',
        'target remote :1234',
        f'source {py_args.breakpoints}',
    ])

    args2 = []

    if not os.path.exists(py_args.breakpoints):
        system(f'cp default-bp.gdb {py_args.breakpoints}')

    for arg in args:
        args2.extend(['-ex', arg])

    execlp('gdb', 'gdb', *args2)


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
