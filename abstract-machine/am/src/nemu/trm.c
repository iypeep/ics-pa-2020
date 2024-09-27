#include <am.h>
#include <nemu.h>

extern char _heap_start;//标记了堆（heap）内存开始的地方
int main(const char* args);

Area heap = RANGE(&_heap_start, PMEM_END);//Area heap结构用于指示堆区的起始和末尾
#ifndef MAINARGS
#define MAINARGS ""
#endif
static const char mainargs[] = MAINARGS;

void putch(char ch) {//void putch(char ch)用于输出一个字符
  outb(SERIAL_PORT, ch);
}

void halt(int code) {//void halt(int code)用于结束程序的运行
  nemu_trap(code);

  // should not reach here
  while (1);
}

void _trm_init() {//void _trm_init()用于进行TRM相关的初始化工作
  //malloc_init();//实现malloc时添加
  int ret = main(mainargs);//main成功则返回0到ret
  halt(ret);
}
