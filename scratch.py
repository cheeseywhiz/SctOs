#!/usr/bin/env -S python -i
'''some tools that are nice to have open while debugging'''
from pprint import pprint as pp
import numpy
u64 = numpy.uint64
i64 = numpy.int64
u8 = numpy.uint8

p1KB = u64(1024)
p1MB = p1KB * p1KB
p1GB = p1KB * p1MB
n1GB = u64(-p1GB)
KERNEL_BASE = n1GB


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
