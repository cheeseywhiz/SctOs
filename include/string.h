#pragma once
#include <stddef.h>

int strncmp(const char *p, const char *q, size_t n);
int strcmp(const char *p, const char *q);
char *strncpy(char *dst, const char *src, size_t n);
char* safestrcpy(char *dst, const char *src, size_t n);
size_t strlen(const char *s);
