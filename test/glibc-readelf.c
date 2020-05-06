#include "readelf.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

void
elf_on_not_elf(const struct elf_file *elf_file __unused)
{
    fprintf(stderr, "file is not an ELF file\n");
}

void*
elf_alloc(Elf64_Xword size)
{
    void *buf;
    if (!(buf = malloc(size)))
        fprintf(stderr, "malloc(%lu): %d %s\n", size, errno, strerror(errno));
    return buf;
}

void
elf_free(const void *ptr)
{
    free((void*)ptr);
}

bool
elf_read(void *fd_, void *buf, Elf64_Off offset, Elf64_Xword size)
{
    if (offset > INT64_MAX) {
        // would interfere with off_t cast
        fprintf(stderr, "offset too large: %lu\n", offset);
        return true;
    }

    int fd = *(int*)fd_;

    if (lseek(fd, (off_t)offset, SEEK_SET) < 0) {
        fprintf(stderr, "lseek(%lu): %d %s\n", offset, errno, strerror(errno));
        return true;
    }

    if ((Elf64_Xword)read(fd, buf, size) != size) {
        fprintf(stderr, "read(%lu): %d %s\n", size,
                errno, strerror(errno));
        return true;
    }

    return false;
}
