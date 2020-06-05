# opsys
## apic-debug-bug
GDB loses single step when reading apic system registers.

steps to reproduce:
1. correctly set dependencies at the top of the Makefile (edk2/ovmf, gnu-efi).
2. start qemu with `make qemu EFI_DEBUG=y`.
   1. access the monitor (`Ctrl-A`, `C`) and continue execution in order to
      print the `ImageBase` address, then quit qemu from the monitor.
3. start qemu again.
   1. copy the `ImageBase` address from (2.1.) and pass it to `./gdb.py` in a
      new terminal.
   2. the debugger will stop right before reading the apic id register.
   3. step the debugger and observe where the debugger stops next. is it the
      line after the register access or at the next breakpoint?
   4. continue until the debugger stops before reading the version register.
   5. step the debugger and observe where the debugger stops next. is it the
      line after the register access or at the next breakpoint?
   6. type `leave` in gdb to quit the virtual machine and the gdb session.
4. start qemu again with `NOKVM=y` and repeat (3.) and observe that the emulated
   cpu correctly does not lose single step when reading the apic registers.
