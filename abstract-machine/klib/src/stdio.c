#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

int printf(const char *fmt, ...) {
  char buffer[1024];
  va_list ap;
  va_start(ap, fmt);
  int result = vsprintf(buffer, fmt, ap);
  va_end(ap);

  for (char *p = buffer; *p != '\0'; p++){
    putch(*p);
  }

  return result;
}

int vsprintf(char *out, const char *fmt, va_list ap) {
  char *dst = out;
  const char *p = fmt;

  while (*p != '\0'){
    if (*p != '%'){
      *dst++ = *p++;
      continue;
    }

    p++; // 跳过%
    // 检查 % 后面是否是字符串结束符
    if (*p == '\0') {
      *dst++ = '%';
      break;
    }
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

      case 'c':{
        char str = (char)va_arg(ap, int);
        *dst++ = str;
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


// 提取公共的格式化逻辑
static int vsnprintf_core(char *out, size_t n, const char *fmt, va_list ap) {
  char *dst = out;
  const char *p = fmt;
  if (n == 0 ) return 0;
  size_t remaining = n > 0 ? n - 1 : 0;
  size_t written = 0; // 记录生成的完整字符串的长度，包括被截断的部分

  while (*p != '\0'){
    if (*p != '%'){
      if(remaining > 0){
        *dst++ = *p;
        remaining--;
      }
      written++;
      p++;
      continue;
    }

    p++;
    // 检查 % 后面是否是字符串结束符
    if (*p == '\0') {
      if (remaining > 0) {
        *dst++ = '%';
        remaining--;
      }
      written++;
      break;
    }
    switch (*p){
      case 'd':{
        int value = va_arg(ap, int);
        char buffer[32];
        char *temp = buffer;
        if (value < 0){
          if (remaining){
            *dst++ = '-';
            remaining--;
          }
          written++;
        }

        do {
          *temp++ = '0' + (value % 10);
          value /= 10;
        }while (value);
        while (temp != buffer){
          if (remaining){
            *dst++ = *--temp;
            remaining--;
          }
          written++;
        }
        break;
      }

      case 's':{
        const char *str = va_arg(ap, const char *);
        while (*str != '\0'){
          if (remaining){
            *dst++ = *str;
            remaining--;
          }
          written++;
          str++;
        }
        break;
      }

      case 'c':{
        char str = (char)va_arg(ap, int);
        if (remaining > 0){
          *dst++ = str;
          remaining--;
        }
        written++;
        break;
      }

      default:{
        if (remaining > 0){
          *dst++ = '%';
          remaining--;
        }
        written++;
        if (remaining > 0){
          *dst++ = *p;
          remaining--;
        }
        written++;
        break;
      }
    }
    p++;

  }
  if (n > 0){
    *dst = '\0';
  }

  return written;
}

int snprintf(char *out, size_t n, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int result = vsnprintf_core(out, n, fmt, ap);
  va_end(ap);
  return result;
}

int vsnprintf(char *out, size_t n, const char *fmt, va_list ap) {
  return vsnprintf_core(out, n, fmt, ap);
}


#endif