#include <klib.h>
#include <klib-macros.h>
#include <stdint.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

size_t strlen(const char *s) {
  //assert(s != NULL); 
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

char *strcat(char *dst, const char *src)
{
    //assert(dst != NULL && src != NULL);
    char *original_dest = dst;

    // 移动dest指针到目标字符串的末尾
    while (*dst != '\0')
    {
      dst++;
    }

    // 将src字符串的内容复制到dest指针当前位置
    while ((*dst++ = *src++) != '\0')
    {
      ;
    }
    return original_dest;
}

int strcmp(const char *s1, const char *s2) {
  //assert(s1 != NULL && s2 !=NULL);
  while (*s1 && (*s1 == *s2)) {
    s1++;
    s2++;
  }
return *(unsigned char *)s1 - *(unsigned char *)s2;
  
}

int strncmp(const char *s1, const char *s2, size_t n) {
  //assert(s1 != NULL && s2 != NULL);
    while (n-- > 0 && *s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char *)s1 - *(unsigned char *)s2;
}

void *memset(void *s, int c, size_t n) {
  //assert(s != NULL);
    unsigned char* pDest = (unsigned char*)s;
    c = (unsigned char)c;  // 确保c是一个无符号字符
    while (n-- > 0) {
        *pDest++ = c;
    }
    return s;
}

void *memmove(void *dst, const void *src, size_t n) {
  //assert(dst != NULL && src != NULL);
    char* d = (char*)dst;
    const char* s = (const char*)src;

    // 如果源地址和目标地址相同，则直接返回目标地址
    if (d == s) {
        return dst;
    }

    // 判断源地址和目标地址的相对位置
    if (d < s) {
        // 从前向后拷贝（源地址在目标地址之后或没有重叠）
        while (n--) {
            *d++ = *s++;
        }
    } else {
        // 从后向前拷贝（源地址在目标地址之前，有重叠）
        d += n;
        s += n;
        while (n--) {
            *--d = *--s;
        }
    }
    return dst;
}

void *memcpy(void *out, const void *in, size_t n) {
    //assert(out != NULL && in != NULL);
    char* d = (char*)out;
    const char* s = (const char*)in;

    while (n--) {
        *d++ = *s++;
    }
    return out;
}

int memcmp(const void *s1, const void *s2, size_t n) {
  //assert(s1 != NULL && s2 != NULL);
  const unsigned char *p1 = (const unsigned char*)s1;
  const unsigned char *p2 = (const unsigned char*)s2;
  while (n--)
  {
    if (*p1 !=  *p2){
      return (int)(*p1 - *p2);
    }
    p1++;
    p2++;
  }
  return 0;

}

#endif
