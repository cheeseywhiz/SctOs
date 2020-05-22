# boot
## bootloader sequence
1. allocate new stack and some free memory
2. acquire preliminary memory map
3. load kernel executable into physical memory
4. prepare boot page tables with Loader segments identity mapped,
   `bootloader\_data` and `free_memory` mapped to physical memory region, and
   kernel mapped to high half
5. acquire final memory map
6. ExitBootServices
7. SetVirtualAddressMap
8. set up new stack
9. enable paging with boot page tables
10. do relocations
11. jump to kernel, passing `bootloader\_data`
---
## kernel boot sequence
1. clear bss
2. initialize `bootloader\_data` global
3. take over memory management 
   - use free memory from bootloader (1)
   - set up the kernel's own page tables
