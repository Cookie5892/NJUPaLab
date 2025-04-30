#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

int printf(const char *fmt, ...) {
  panic("Not implemented");
}

int vsprintf(char *out, const char *fmt, va_list ap) {
  char *dst = out;
  const char *p = fmt;

  while (*p != '\0'){
    if (*p != '%'){
      *dst++ = *p++;
      continue;
    }

    p++; //跳过%
// 判断格式类型
    switch (*p) {
      case 'd': {
        int value = va_arg(ap, int);
        char buffer[32];
        char *temp = buffer;
        if (value < 0){
          *dst++ = '-';
          value = -value;
        }

        do {
          *temp++ = '0' + value % 10;
          value /= 10;
        }while (value);
        while (temp != buffer){
          *dst++ = *--temp;
        }
        break;
      }

      case 's': {
        const char *str = va_arg(ap, const char*);
        while (*str != '\0'){
          *dst++ = *str++;
        }
      break;
      }
      default:{
        *dst++ = '%';
        *dst++ = *p;
        break;
      }
    }
    p++;
  }

  *dst = '\0';
  return dst - out;
}

int sprintf(char *out, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int result = vsprintf(out, fmt, ap);
  va_end(ap);
  return result;
}

int snprintf(char *out, size_t n, const char *fmt, ...) {
  panic("Not implemented");
}

int vsnprintf(char *out, size_t n, const char *fmt, va_list ap) {
  panic("Not implemented");
}

#endif