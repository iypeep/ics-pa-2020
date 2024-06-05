#include <am.h>
#include <klib.h>
#include <klib-macros.h>

// 仅当非本地 ISA 或启用了本地 klib 时才编译以下代码
#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)
static unsigned long int next = 1;

// 生成伪随机数，返回值在 [0, 32767] 范围内
int rand(void) {
  // RAND_MAX 假设为 32767
  next = next * 1103515245 + 12345;
  return (unsigned int)(next / 65536) % 32768;
}

// 设置随机数生成器的种子
void srand(unsigned int seed) {
  next = seed;
}

// 返回整数的绝对值
int abs(int x) {
  return (x < 0 ? -x : x);
}

// 将字符串转换为整数
int atoi(const char* nptr) {
  int x = 0;
  // 跳过前导空格
  while (*nptr == ' ') { nptr++; }
  // 转换每个字符为数字并累加
  while (*nptr >= '0' && *nptr <= '9') {
    x = x * 10 + *nptr - '0';
    nptr++;
  }
  return x;
}

// 下面是 malloc 和 free 的实现

typedef struct Block {
  size_t size;          // 块大小
  struct Block* next;   // 指向下一个块的指针
} Block;

#define PMEM_SIZE (128 * 1024 * 1024)
#define BLOCK_SIZE 1024  // 每个链表负责的基本块大小
#define NUM_LISTS (PMEM_SIZE / BLOCK_SIZE)
static Block* free_lists[NUM_LISTS];   // 空闲块链表数组
static char* heap_start = NULL;        // 堆的起始地址
static char* heap_end = NULL;          // 堆的结束地址

// 初始化 malloc 系统
void malloc_init() {
  heap_start = (char*)heap.start;
  heap_end = (char*)heap.end;
  size_t initial_size = heap_end - heap_start - sizeof(Block);

  Block* block = (Block*)heap_start;
  block->size = initial_size;
  block->next = NULL;
  free_lists[0] = block;  // 使用free_lists的第一个元素作为整个空闲链表的入口
}


// 分配指定大小的内存
void* malloc(size_t size) {
  if (size == 0) return NULL;

  // 调整大小以包含块头，并对齐到 sizeof(Block) 的倍数
  size = (size + sizeof(Block) - 1) & ~(sizeof(Block) - 1);
  size += sizeof(Block);

  Block* block = free_lists[0];
  Block* prev = NULL;

  // 查找第一个足够大的块
  while (block != NULL && block->size < size) {
    prev = block;
    block = block->next;
  }

  if (!block) return NULL; // 未找到合适的块

  // 分割块
  if (block->size > size) {
    Block* new_block = (Block*)((char*)block + size);
    new_block->size = block->size - size;
    new_block->next = block->next;
    block->size = size - sizeof(Block);

    if (prev) prev->next = new_block;
    else free_lists[0] = new_block;
  }
  else {
    if (prev) prev->next = block->next;
    else free_lists[0] = block->next;
  }

  return (char*)block + sizeof(Block);
}

// 释放已分配的内存
void free(void* ptr) {
  if (!ptr) return;

  Block* block = (Block*)((char*)ptr - sizeof(Block));
  Block* current = free_lists[0];
  Block* prev = NULL;

  // 在链表中找到正确的插入位置
  while (current != NULL && (char*)current < (char*)block) {
    prev = current;
    current = current->next;
  }

  block->next = current;

  // 尝试与下一个块合并
  if (current && (char*)block + block->size + sizeof(Block) == (char*)current) {
    block->size += current->size + sizeof(Block);
    block->next = current->next;
  }

  // 尝试与前一个块合并
  if (prev && (char*)prev + prev->size + sizeof(Block) == (char*)block) {
    prev->size += block->size + sizeof(Block);
    prev->next = block->next;
  }
  else {
    if (prev) prev->next = block;
    else free_lists[0] = block;
  }
}

#endif
