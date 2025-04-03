#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

static int itoa(int value, char *str, int base);
static int kstrcpy(char *dest, const char *src);
static int kstrlen(const char *str);


//将整数value转换为字符串格式
static int format_int(char *out, int value, size_t n) {
  char buffer[20];
  //此函数使用itoa函数将整数转换为字符串，并检查结果是否可以完全放入输出缓冲区
  int len = itoa(value, buffer, 10);
  if (len >= n) return len;
  kstrcpy(out, buffer);
  return len;
}
static int format_string(char *out, const char *value, size_t n) {
  int len = kstrlen(value);
  if (len >= n) return len;
  kstrcpy(out, value);
  return len;
}

static int format_char(char *out, int value, size_t n) {
  if (n < 1) return 1;
  *out = (char)value;
  return 1;
}

static int itoa(int value, char *str, int base) {
  char *p = str;
  char *q;
  char digit[] = "0123456789abcdef";
  int sign = 0;

  if (base < 2 || base > 16) return 0;

  if (value < 0) {
      sign = 1;
      value = -value;
  }

  do {
      *p++ = digit[value % base];
  } while ((value /= base) > 0);

  if (sign) *p++ = '-';
  *p = '\0';

  q = p;
  p = str;
  while (p < q) {
      char temp = *p;
      *p++ = *q;
      *q-- = temp;
  }

  return p - str;
}

static int kstrcpy(char *dest, const char *src) {
  char *start = dest;
  while (*src) {
      *dest++ = *src++;
  }
  *dest = '\0';
  return dest - start;
}

static int kstrlen(const char *str) {
  const char *s;
  for (s = str; *s; ++s);
  return s - str;
}

int printf(const char *fmt, ...) {
  panic("Not implemented");
}

int vsprintf(char *out, const char *fmt, va_list ap) {
  return vsnprintf(out, SIZE_MAX, fmt, ap);
}

int sprintf(char *out, const char *fmt, ...) {
  va_list ap;
    va_start(ap, fmt);
    int result = vsprintf(out, fmt, ap);
    va_end(ap);
    return result;
}

int snprintf(char *out, size_t n, const char *fmt, ...) {
  va_list ap;
    va_start(ap, fmt);
    int result = vsnprintf(out, n, fmt, ap);
    va_end(ap);
    return result;
}

int vsnprintf(char *out, size_t n, const char *fmt, va_list ap) {
  if (n == 0 || fmt == NULL) return 0;

    char *start = out;
    char *end = out + n - 1; // 留一个位置给 '\0'
    while (*fmt && out < end) {
        if (*fmt != '%') {
            *out++ = *fmt++;
            continue;
        }

        fmt++; // 跳过 '%'
        switch (*fmt) {
            case 'd':
                out += format_int(out, va_arg(ap, int), end - out + 1);
                break;
            case 's':
              const char *str = va_arg(ap, const char *);
              if (str == NULL) str = "NULL"; // 默认处理空指针
                out += format_string(out, va_arg(ap, const char *), end - out + 1);
                break;
            case 'c':
                out += format_char(out, va_arg(ap, int), end - out + 1);
                break;
            default:
                *out++ = '%';
                *out++ = *fmt;
                break;
        }
        fmt++;
    }
    *out = '\0';
    return out - start;
}

#endif
