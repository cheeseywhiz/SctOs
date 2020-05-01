/* copied from xv6 */
#include "x86.h"
#include "string.h"
#include <stddef.h>

void*
memset(void *dst, int c, size_t n)
{
  if ((unsigned long)dst%4 == 0 && n%4 == 0){
    c &= 0xFF;
    stosl(dst, (c<<24)|(c<<16)|(c<<8)|c, n/4);
  } else
    stosb(dst, c, n);
  return dst;
}

int
memcmp(const void *v1, const void *v2, size_t n)
{
  const unsigned char *s1, *s2;

  s1 = v1;
  s2 = v2;
  while(n-- > 0){
    if(*s1 != *s2)
      return *s1 - *s2;
    s1++, s2++;
  }

  return 0;
}

void*
memmove(void *dst, const void *src, size_t n)
{
  const char *s;
  char *d;

  s = src;
  d = dst;
  if(s < d && s + n > d){
    s += n;
    d += n;
    while(n-- > 0)
      *--d = *--s;
  } else
    while(n-- > 0)
      *d++ = *s++;

  return dst;
}

// memcpy exists to placate GCC.  Use memmove.
void*
memcpy(void *dst, const void *src, size_t n)
{
  return memmove(dst, src, n);
}

int
strncmp(const char *p, const char *q, size_t n)
{
  while(n > 0 && *p && *p == *q)
    n--, p++, q++;
  if(n == 0)
    return 0;
  return (unsigned char)*p - (unsigned char)*q;
}

char*
strncpy(char *dst, const char *src, size_t n)
{
  char *os;

  os = dst;
  while(n-- > 0 && (*dst++ = *src++) != 0)
    ;
  while(n-- > 0)
    *dst++ = 0;
  return os;
}

// Like strncpy but guaranteed to NUL-terminate.
char*
safestrcpy(char *dst, const char *src, size_t n)
{
  char *os;

  os = dst;
  if(n <= 0)
    return os;
  while(--n > 0 && (*dst++ = *src++) != 0)
    ;
  *dst = 0;
  return os;
}

size_t
strlen(const char *s)
{
  size_t n;

  for(n = 0; s[n]; n++)
    ;
  return n;
}

