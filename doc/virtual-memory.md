# virtual memory
## Memory Layout
| Virtual Address          | Variable      | Contents          |
| ------------------------ | ------------- | ----------------- |
| low                      |               |                   |
| mmio\_base - PAGE\_SIZE  | apic-\>vaddr  | apic registers    |
| paddr\_base - mmio\_size | mmio\_base    | EFI MMIO segments |
| -1GB - paddr\_size       | paddr\_base   | physical memory   |
| -1GB                     | KERNEL\_BASE  | kernel executable |
| high                     |               |                   |

## notes
* memory allocated in bootloader sequence steps 3 until 5 (currently, just the
  boot page tables) is still managed by the kernel, as that memory resides in
  new LoaderData segments. however, the memory will not be accessible, since
  only the memory allocated before step 2 will be paged in to the boot page
  tables. this does not currently seem an issue.

## new address space
note: the bootloader repeats much of this work.
- physical memory up to `pmem_tail`
- `bootloader\_data`
  - `free_memory`
  - runtime `MemoryMap` segments
- apic register region
- kernel segments from ELF program headers
  - with RELRO segment made RO

## allocating a new page
- if free list is null:
  * find a page on or after `pmem_tail` that resides in EfiConventionalMemory
  * increment `'pmem_tail`
  * page it in
  * add it to the free list
- pop from free list

## freeing a single page
- push it to free list
