#include <klib.h>
#include <klib-macros.h>
#include <stdint.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

size_t strlen(const char *s) {
  assert(s != NULL);
  size_t count = 0;
  while (*s++ != '\0'){
    count++;
  }
  return count;
}

char *strcpy(char *dst, const char *src) {
  char *ret = dst;            // 保存目标指针起始位置
    while ((*dst++ = *src++));   // 复制字符，包括'\0'
    return ret;                   // 返回目标指针
  
}

char *strncpy(char *dst, const char *src, size_t n)
{
  size_t i;

  for (i = 0; i < n && src[i] != '\0'; i++)
    dst[i] = src[i];
  for (; i < n; i++)
    dst[i] = '\0';

  return dst;
}

char *strcat(char *dst, const char *src) {
  panic("Not implemented");
}

int strcmp(const char *s1, const char *s2) {
  assert(s1 != NULL && s2 !=NULL);
  while (*s1 && (*s1 == *s2)) {
    s1++;
    s2++;
  }
return *(unsigned char *)s1 - *(unsigned char *)s2;
  
}

int strncmp(const char *s1, const char *s2, size_t n) {
  panic("Not implemented");
}

void *memset(void *s, int c, size_t n) {
  panic("Not implemented");
}

void *memmove(void *dst, const void *src, size_t n) {
  panic("Not implemented");
}

void *memcpy(void *out, const void *in, size_t n) {
  panic("Not implemented");
}

int memcmp(const void *s1, const void *s2, size_t n) {
  panic("Not implemented");
}

#endif
