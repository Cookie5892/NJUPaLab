#include <am.h>
#include <klib.h>
#include <klib-macros.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)
static unsigned long int next = 1;

int rand(void) {
  // RAND_MAX assumed to be 32767
  next = next * 1103515245 + 12345;
  return (unsigned int)(next/65536) % 32768;
}

void srand(unsigned int seed) {
  next = seed;
}

int abs(int x) {
  return (x < 0 ? -x : x);
}

int atoi(const char* nptr) {
  int x = 0;
  while (*nptr == ' ') { nptr ++; }
  while (*nptr >= '0' && *nptr <= '9') {
    x = x * 10 + *nptr - '0';
    nptr ++;
  }
  return x;
}

void *malloc(size_t size) {
  // On native, malloc() will be called during initializaion of C runtime.
  // Therefore do not call panic() here, else it will yield a dead recursion:
  //   panic() -> putchar() -> (glibc) -> malloc() -> panic()
  

#if !(defined(__ISA_NATIVE__) && defined(__NATIVE_USE_KLIB__))
  panic("Not implemented");
#endif

  // 静态变量 addr 用于记录当前堆的分配位置
  static uintptr_t addr = 0;

  // 如果 addr 尚未初始化，则将其设置为堆的起始地址
  if (addr == 0) {
    addr = (uintptr_t)heap.start;
  }

  // 确保返回的地址满足对齐要求（假设 8 字节对齐）
  #define ALIGNMENT 8
  size = (size + ALIGNMENT - 1) & ~(ALIGNMENT - 1); // 向上对齐到 ALIGNMENT 的倍数

  // 检查是否有足够的堆空间
  if (addr + size > (uintptr_t)heap.end) {
    return NULL; // 堆空间不足，返回 NULL
  }

  // 保存当前分配的起始地址
  uintptr_t aligned = (addr + ALIGNMENT - 1) & ~(ALIGNMENT - 1);
  void *allocated = (void *)aligned;

  // 更新 addr，指向下一段可用空间
  addr = aligned + size;

  return allocated;
}

void free(void *ptr) {
}

#endif
