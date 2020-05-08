/* this file contains overrides to have readelf work in a gnu/linux
 * environment */
#include "readelf.h"
#include "glibc-readelf.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

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

    struct elf_glibc_fd *fd = fd_;

    if (lseek(fd->fd, (off_t)offset, SEEK_SET) < 0) {
        fprintf(stderr, "lseek(%lu): %d %s\n", offset, errno, strerror(errno));
        return true;
    }

    if ((Elf64_Xword)read(fd->fd, buf, size) != size) {
        fprintf(stderr, "read(%lu): %d %s\n", size,
                errno, strerror(errno));
        return true;
    }

    return false;
}

void
elf_on_not_elf(void *fd_)
{
    struct elf_glibc_fd *fd = fd_;
    fprintf(stderr, "%s is not an ELF file\n", fd->fname);
}
