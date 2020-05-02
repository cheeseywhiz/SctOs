#pragma once
#include <stddef.h>

void *memset(void *dst, int c, size_t n);
int memcmp(const void *v1, const void *v2, size_t n);
void *memmove(void *dst, const void *src, size_t n);
void *memcpy(void *dst, const void *src, size_t n);
int strncmp(const char *p, const char *q, size_t n);
int strcmp(const char *p, const char *q);
char *strncpy(char *dst, const char *src, size_t n);
char* safestrcpy(char *dst, const char *src, size_t n);
size_t strlen(const char *s);
