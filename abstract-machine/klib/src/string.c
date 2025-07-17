#include <klib.h>
#include <klib-macros.h>
#include <stdint.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

size_t strlen(const char *s) {
  const char *p = s;
  size_t str_len = 0;
  while ( *p != '\0' ){
    str_len++;
    p++;
  }
  return str_len;
}

char *strcpy(char *dst, const char *src) {
  size_t i;
  for ( i = 0; src[i] != '\0'; i++){
    dst[i] = src[i];
  }

  dst[i] = '\0';
  return dst;
}

char *strncpy(char *dst, const char *src, size_t n) {
  size_t i;

  for ( i = 0; i < n && src[i] != '\0'; i++ ) {
    dst[i] = src[i];
  }

  for ( ; i < n; i++){
    dst[i] = '\0';
  }

  return dst;
}

char *strcat(char *dst, const char *src) {
  size_t i;
  char *p = dst;
  while (*p != '\0'){
    p++;
  }
  for (i = 0; src[i] != '\0'; i++){
    p[i] = src[i];
  }
  p[i] = '\0';

  return dst;
}

int strcmp(const char *s1, const char *s2) {  //重点
  size_t i = 0;
  while ( s1[i] != '\0' && s1[i] == s2[i] ){
    i++;
  }
  return (unsigned)s1[i] - (unsigned)s2[i];
}

int strncmp(const char *s1, const char *s2, size_t n) {
  size_t i = 0;
  while (i < n && s1[i] != '\0' && s1[i] == s2[i]){
    i++;
  }

  if (i == n) return 0;

  return (unsigned)s1[i] - (unsigned)s2[i];
}

void *memset(void *s, int c, size_t n) {
  unsigned char *p = (unsigned char *)s;
  while (n--){
    *p++ = (unsigned char)c;
  }

  return s;
}

void *memmove(void *dst, const void *src, size_t n) {
  unsigned char *d = (unsigned char *)dst;
  const unsigned char *s = (unsigned char *)src;
  
  if (d == s) {
    return dst;
  }

  if ( d < s || d > s + n){
    while (n--){
      *d++ = *s++; 
    }
  }else {
    d = d + n;
    s = s + n;
    while (n--){
      *(d--) = *(s--);
    }
  }

  return dst;
}

void *memcpy(void *out, const void *in, size_t n) {
  unsigned char *dst = (unsigned char *)out;
  const unsigned char *src  = (unsigned char *)in;

  while (n--){
    *dst++ = *src++;
  }

  return out;
}
int memcmp(const void *s1, const void *s2, size_t n) {
  const unsigned char *p1 = (const unsigned char *)s1;
  const unsigned char *p2 = (const unsigned char *)s2; 
  
  while (n--){
    if (*p1 != *p2){
      return (int)(*p1 - *p2);
    }
    p1++;
    p2++;
  }

  return 0;
}

#endif
