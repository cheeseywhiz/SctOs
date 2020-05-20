# boot
## bootloader sequence
1. allocate new stack
2. acquire preliminary memory map
3. load kernel executable into physical memory
4. prepare boot page tables with Loader segments identity mapped and
   kernel mapped to high half
5. acquire final memory map
6. ExitBootServices
7. SetVirtualAddressMap
8. set up new stack
9. enable paging with boot page tables
10. finish preparation of kernel executable
    - relocations
    - set load-time synbols
    - clear bss
    - set relro pages to ro
11. jump to kernel
