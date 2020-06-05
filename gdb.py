#!/usr/bin/env python
import os
import sys
import subprocess


def main(argv):
    if len(argv) != 2:
        print(f'{argv[0]} base')
        sys.exit(1)

    loader, debug_loader = system('make print-efi-execs').splitlines()
    base = str_hex_to_int(argv[1])
    text, data = system((rf"objdump -h {loader}"
                         r" | grep 'text\|data'"
                         r" | awk '{ print $4 }'")).splitlines()
    text = str_hex_to_int(text)
    data = str_hex_to_int(data)
    execlp(
        'gdb',
        '-ex', 'set confirm off',
        '-ex', (f'add-symbol-file {debug_loader}'
                f' {hex(base+text)}'
                f' -s .data {hex(base+data)}'),
        '-ex', 'set confirm on',
        '-ex', 'set architecture i386:x86-64:intel',
        '-ex', 'target remote :1234',
        '-ex', 'source hb.gdb',
    )


def str_hex_to_int(str_hex):
    '''convert either '0xdeadbeef' or 'deadbeef' to 0xdeadbeef'''
    return int(str_hex[(2 if len(str_hex) > 2 and str_hex[1] == 'x' else 0):],
               base=16)


def system(cmd):
    print(cmd)
    proc = subprocess.run(cmd, shell=True, stdout=subprocess.PIPE)
    if proc.returncode:
        sys.exit(proc.returncode)
    return proc.stdout.decode()


def execlp(file, *args):
    print(' '.join([file, *args]))
    os.execlp(file, file, *args)


if __name__ == '__main__':
    main(sys.argv)
