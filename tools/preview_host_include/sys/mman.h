#ifndef JZ2440_HOST_SYS_MMAN_H
#define JZ2440_HOST_SYS_MMAN_H

#include <stddef.h>

#define PROT_READ 1
#define PROT_WRITE 2
#define MAP_SHARED 1
#define MAP_FAILED ((void *)-1)

void *mmap(void *, size_t, int, int, int, long);
int munmap(void *, size_t);

#endif
