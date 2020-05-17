# Memory Layout
| Virtual Address       | Variable      | Contents          |
| --------------------- | ------------- | ----------------- |
| low                   |               |                   |
| immediately before... |               | EFI MMIO segments |
| -1GB - paddr\_max     | paddr\_base   | physical memory   |
| -1GB                  | KERNEL\_BASE  | kernel executable |
| -1 page               | %rsp          | kernel stack      |
| high                  |               |                   |
