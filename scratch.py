#!/usr/bin/env -S python -i
'''some tools that are nice to have open while debugging'''
from pprint import pprint as pp
import numpy
import sys
u64 = numpy.uint64
i64 = numpy.int64
u8 = numpy.uint8

p1KB = u64(1024)
p1MB = p1KB * p1KB
p1GB = p1KB * p1MB
n1GB = u64(-p1GB)
KERNEL_BASE = n1GB

p4KB = u64(4) * p1KB


def to_base(num, base=1024):
    '''convert a number to its big endian representation in any base'''
    if isinstance(num, numpy.integer):
        base = u64(base)
    digits = []

    while True:
        digits.append(num % base)
        num //= base

        if not num:
            # digits is thus far in little endian representation
            digits.reverse()
            return digits


def pp_bits(bits, start=0):
    '''print out each bit number with each bit'''
    pp(list(enumerate(reversed(bin(bits)[2:]), start=start)))


def in_range(base, size, num):
    return base <= num < (base + size)


# virtual memory
PAGE_SIZE = p4KB


def page_base(addr):
    addr = u64(addr)
    return addr & ~(PAGE_SIZE - u64(1))


def next_page(addr):
    addr = u64(addr)
    return page_base(addr + PAGE_SIZE)


def page_offset(addr):
    addr = u64(addr)
    return addr & (PAGE_SIZE - u64(1))


def num_pages(base, size):
    base = u64(base)
    size = u64(size)
    return next_page(base + size - u64(1) - page_base(base)) // PAGE_SIZE


def split_chunks(ll, n):
    return [
        ll[i * n:(i + 1) * n]
        for i in range((len(ll) + n - 1) // n)
    ]


Boot0014 = '/sys/firmware/efi/efivars/Boot0014-8be4df61-93ca-11d2-aa0d-00e098032b8c'


def chexdump(bs: bytes, file=sys.stdout) -> str:
    '''convert bytes into a c hex array. print to file'''
    print('{', file=file)

    for line in split_chunks(bs, 16):
        hexline = ', '.join(f'{num:#04x}' for num in line)
        print(f'    {hexline},', file=file)

    print('};', file=file)
